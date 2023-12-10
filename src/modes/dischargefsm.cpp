/*
    Файл: dischargefsm.cpp 07.11.2023
    Режим разряда реализован как конечный автомат, имеющий 7 состояний:

      MStart  - в этом состоянии восстанавливаются параметры разряда из 
    энергонезависимой памяти устройства, предлагая оператору на выбор 
    вычисленные по типу аккумулятора, заданного в режиме "Options", или 
    пользовательские, введенные при предыдущем сеансе работы в этом режиме.

      MSetCurrent - состояние коррекции максимального тока разряда. Может
    понадобится подключить внешнюю нагрузку.

      MSetVoltage - состояние коррекции минимального напряжения, при котором 
    прекращается разряд.

      MGoType - состояние, при котором инициализируются (обнуляются)
    счетчик залитых ампер-часов и таймер продолжительности разряда. Индикация
    на дисплее предназначена для принятия решения начать разряд c параметрами,
    заданными при выборе типа батареи.

      MGoTask - то же, но параметры заданы в этом режиме пользователем.

      MGo - состояние собственно разряда с контролем условий окончания разряда,
    который может быть прерван оператором.

      MStop - состояние завершения разряда. На дисплей выводится информация о
    причине завершения разряда и его итоги. Из этого состояния можно повторно
    перейти в состояние "Старт" или выйти из режима "Разряд" в меню выбора
    режима работы прибора.

      MExit - состояние, которое восстанавливает индикацию на дисплее об
    исполненном режиме и производит выход в меню.

    Каждое состояние реализовано своим классом, конструктором класса задается 
  инициализация: сообщения и подсказки на дисплее, вывод на экран необходимого
  минимума кнопок, восстановление из энергонезависимой памяти параметров, цвет
  RGB светодиода и т.п. По общепринятому правилу c++ конструктор класса ничего 
  не возвращает. 
    Методы класса представленны единственной функцией fsm(). Эта функция 
  виртуальная, вызывается по указателю, в ней производятся проверки условий
  перехода в иные состояния: по нажатию кнопок, счетчикам, таймерам и другим
  событиям. В этом случае функция возвращает через return new имя 
  соответствующего состояния, где опять-таки в конструкторе происходит инициализация 
  последнего. Если ни одного такого события не оказалось, возвращается return this.
  При следующем вызове функция fsm() будет исполнена снова.
    Если задать для выхода return nullptr, исполнение выбранного режима 
  прекратится и управление будет передано главному меню (диспетчеру).

    Весь "арсенал" программиста в основном сосредоточен в классах MTools, 
  MDisplay и MBoard, например (с аргументами разберётесь): 
  Tools->readNvsFloat()       - чтение из энергонезависимой памяти по имени;
  Display->drawLabel()        - вывод на дисплей текстовой строки;
  Display->drawParFl()        - то же параметра с переводом в float;
  Tools->showVolt()           - то же текущего напряжения; 
  Display->newBtn()           - то же активных кнопок;
  Tools->chargeCalculations() - подсчет ампер-часов;
  Board->ledsOn()             - подсветка RGB светодиодом.

    Константы, используемые только в единственном состоянии удобнее всего
  размещать в объявлении соответствующего класса состояния в заголовочном файле.
  Если они используются только в одном режиме, но в нескольких состояниях, то 
  объявлять в поле имен режима там же. Используемые же в разных режимах -
  в специальном файле конфигурации проекта project_config.h.
    
    Остальное ардуинщику с начальным опытом должно быть понятно.
  Чёткое представление о реализуемой задаче и аккуратность при копировании - 
  и успех обеспечен. 
  
    Управление силовым блоком производится командами, причем за один вызов fsm() 
  может быть обработана только одна команда  (дабы не плодить лишние сущности 
  автор отказался от использования очередей) что может быть легко обойдено, 
  задав vTaskDelay(). Исключения сделаны для некоторых команд.

    Наблюдательный программист обнаружит избыточность кода, особенно в части
  конструкторов состояний при переходах от одного состояния к другому. Что-ж, 
  если всецело полагаетесь на свою память - повторы выбрасывайте, автор же 
  предпочитает начинать каждое состояние "с чистого листа", получая взамен 
  уверенность, что именно он "виляет хвостом", а не наоборот. Такие строки
  "заботливо" закомментированы (но не все).

    Полезные ссылки о конечных автоматах
  попроще:
  https://chipenable.ru/index.php/programming-avr/item/90-realizatsiya-konechnogo-avtomata-state-machine.html 
  посложнее:
  https://habr.com/ru/companies/timeweb/articles/717628/

  В первую очередь следует выбрать из двух способов формализации задачи - 
  табличный или граф, после чего реализация алгоритма превращается в довольно 
  скучное монотонное занятие, где главное не пропустить какую-нибудь мелочь 
  вроде точки с запятой. В приведённом ниже исходнике намеренно между состояниями
  оставлены большие промежутки, подчёркивающие, что каждое состояние никаких 
  видимых связей с другими состояниями иначе чем через return не имеет. В процессе
  разработки отсутствующие состояния можно заменять на заглушки, исполняемая функция 
  fsm() лишь обеспечивает корректный переход к иному состоянию.
   
    Такая реализация конечного автомата вполне по силам третьекласснику, 
  освоившему азы технологии под названием "Ардуино". И не будет он заморачиваться 
  вопросами о том, как реализована работа устройства под управлением операционной 
  системы реального времени, какой это конечный автомат Мили или Мура ... поздними 
  связываниями и прочими тонкостями, о которых уже позаботились до него взрослые дяди.

    И наконец, не кажется ли вам, что такая реализация алгоритма может рассматриваиться 
  как хорошо формализованное задание для приложения, которое может быть разработано в 
  AndroidStudio для смартфона - чем каждое состояние не "activity"?

    Успехов!
*/

#include <Arduino.h>
#include "modes/dischargefsm.h"
#include "modes/optionsfsm.h"
#include "mdispatcher.h"
#include "mtools.h"
#include "board/mboard.h"
#include "display/mdisplay.h"

namespace MDis
{
  short spI;            // Максимальный ток разряда, мА
  short minV;           // Минимальное напряжение, при котором разряд прекращается, мВ
  float kp, ki, kd;     // Коэффициенты ПИД-регулятора
  static uint8_t mark;  // Номер подсвеченной строки
  static short targetI;

  /* Состояние "Старт", инициализация выбранного режима работы (DISCHARGE). */
  MStart::MStart(MTools *Tools) : MState(Tools)
  {
    /* Загрузить заданные для выбранного типа аккумулятора предельные значения.
    Если таковые не заданы или тип не выбран, задать минимальными. */
    spI  = Tools->readNvsShort("discharge",  "spI", 200u);
    minV = Tools->readNvsShort("discharge", "minV", 200u);
    /* Коэффициенты ПИД-регулятора. */
    kp = Tools->readNvsFloat("device", "kpD", MPrj::kp_d_default);
    ki = Tools->readNvsFloat("device", "kiD", MPrj::ki_d_default);
    kd = Tools->readNvsFloat("device", "kdD", MPrj::kd_d_default);
#ifdef PRINT_DISCHARGE
    Serial.print("\nspI="); Serial.println(spI);
    Serial.print("minV="); Serial.println(minV);
    Serial.print("kpD="); Serial.println(kp, 2);
    Serial.print("kiD="); Serial.println(ki, 2);
    Serial.print("kdD="); Serial.println(kd, 2);
#endif
    Tools->txPowerStop(); // 0x21 Команда драйверу отключить на всякий пожарный
    mark = 3;                                                       // Номер подсвеченной строки
    Display->drawLabel("Mode DISCHARGE loaded:", 0);                // Режим, напоминание
    Display->drawLabel(      "Select a source:", 1);                // Подсказка
    Display->clearLine(                          2);
    Display->drawAdj(       "By battery type :", 3, mark);          //
    Display->drawAdj(       "The user's task :", 4, mark);          //
    Display->clearLine(                          5,7);
    Board->ledsOn();                                                // Подтверждение входа
    Display->newBtn(MDisplay::GO, MDisplay::STOP, MDisplay::NEXT);  // Кнопки
  }

  MState *MStart::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::STOP:  return new MStop(Tools);                        // Goodbye режим DISCHARGE
      case MDisplay::NEXT:  (mark >= 4) ? mark = 3 : mark++;                // Переместить указатель
                            Display->drawAdj("By battery type :", 3, mark); //
                            Display->drawAdj("The user's task :", 4, mark); //
                            break;
      case MDisplay::GO:    switch (mark) // Так нагляднее
                            {
                              case 3: return new MGoType(Tools);            //
                              case 4: return new MSetVoltage(Tools);        // 
                              default:;
                            }
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp (Tools->getRealCurrent(), 2);
    return this;
  }; //MStart





    /* Состояние: "Коррекция напряжения окончания разряда 
      на 50 милливольт за один клик" */
  MSetVoltage::MSetVoltage(MTools * Tools) : MState(Tools)
  {
    mark  = 3;
      // В главное окно выводятся:
    Display->drawLabel( "Mode DISCHARGE", 0);
    Display->drawLabel("Adjusting minV:", 1);
    //Display->clearLine(                 2);                       
    Display->drawParFl(      "minV, V :", 3, minV, 2, mark);
    Display->drawParFl(      "maxI, A :", 4,  spI, 2); 
    //Display->clearLine(                 5,7);
    Board->ledsBlue();
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MSetVoltage::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("discharge", "minV", minV);                
                            return new MSetCurrent(Tools);
      case MDisplay::UP:    minV = Tools->updnInt(minV, dn, up, corr);
//                          Display->drawLabel(       "DISCHARGE", 0);
                            Display->drawLabel("Voltage changed:", 1); 
                            Display->drawParFl(       "minV, V :", 3, minV, 2, mark);
                            break;
      case MDisplay::DN:    minV = Tools->updnInt( minV, dn, up, -corr);
//                          Display->drawLabel(       "DISCHARGE", 0);
                            Display->drawLabel("Voltage changed:", 1); 
                            Display->drawParFl(       "minV, V :", 3, minV, 2, mark); 
                            break;
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(),  2);
    return this;
  };  // MSetVoltage




    // Состояние "Коррекция тока заряда на 50 миллиампер за один клик".
  MSetCurrent::MSetCurrent(MTools * Tools) : MState(Tools)
  {
    mark = 4;
//  Display->drawLabel("Mode DISCHARGE", 0);
    Display->drawLabel("Adjusting spI:", 1);
//  Display->clearLine(                  2);
    Display->drawParFl(     "minV, V :", 3, minV, 2);
    Display->drawParFl(     "maxI, A :", 4,  spI, 2, mark);
    Display->clearLine(                  5,7);          // Остальные строки очищаются
    Board->ledsBlue();                                  // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MSetCurrent::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("discharge", "spI", spI);
                            return new MGoTask(Tools);                      // Там выбрать или завершить
      case MDisplay::UP:    spI = Tools->updnInt(spI, dn, up, corr);
//                          Display->drawLabel(       "DISCHARGE", 0);
                            Display->drawLabel("Current changed:", 1);
                            Display->drawParFl(       "maxI, A :", 4, spI, 2, mark); // Подсветить
                            break;
      case MDisplay::DN:    spI = Tools->updnInt(spI, dn, up, -corr);
//                          Display->drawLabel(       "DISCHARGE", 0);
                            Display->drawLabel("Current changed:", 1);
                            Display->drawParFl(       "maxI, A :", 4, spI, 2, mark);
                            break;
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    return this;
  }; //MSetCurrent




  /* Состояние ввода и отображения параметров запуска разряда, 
    заданных пользователем.*/
  MGoTask::MGoTask(MTools * Tools) : MState(Tools)
  {
    Tools->txSetPidCoeffD(kp, ki, kd);                    // 0x41 Применить
//  Display->drawLabel(  "Mode DISCHARGE", 0);
    Display->drawLabel("User Assignment:", 1);
    Display->drawLabel(     "GO or BACK:", 2);
    Display->drawParFl(       "minV, V :", 3, minV, 2);
    Display->drawParFl(       "maxI, A :", 4,  spI, 2);
    Display->drawParam(            "kp :", 5,   kp, 2);
    Display->drawParam(            "ki :", 6,   ki, 2);
    Display->drawParam(            "kd :", 7,   kd, 2);
    Display->newBtn(MDisplay::GO, MDisplay::BACK);
    /* Обнулить счетчики времени и отданного заряда */
    Tools->clrTimeCounter();
    Tools->clrAhCharge();
  }

  MState *MGoTask::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::BACK:  return new MStart(Tools);
      case MDisplay::GO:    return new MGo(Tools);
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    return this;
  }




  /* Состояние ввода и отображения параметров запуска разряда по типу батареи,
    заданного в режиме OPTIONS пользователем.*/
  MGoType::MGoType(MTools * Tools) : MState(Tools)
  {
    Tools->txSetPidCoeffD(kp, ki, kd);                    // 0x41 Применить

    spI  = Tools->readNvsShort("options",  "spI", 200u);
    minV = Tools->readNvsShort("options", "minV", 200u);
    //  Display->drawLabel(  "Mode DISCHARGE", 0);
    Display->drawLabel("User Assignment:", 1);            // Назначение пользователя
    Display->drawLabel(     "GO or BACK:", 2);            // Подсказка
    Display->drawParFl(       "minV, V :", 3, minV, 2);   // Нижний порог по напряжению
    Display->drawParFl(       "maxI, A :", 4,  spI, 2);   // Максимальный ток разряда
    Display->drawParam(            "kp :", 5,   kp, 2);   // Коэффициент kp ПИД-регулятора
    Display->drawParam(            "ki :", 6,   ki, 2);   // то же для ki
    Display->drawParam(            "kd :", 7,   kd, 2);   // то же для kd 
    Display->newBtn(MDisplay::GO, MDisplay::BACK);
    /* Обнулить счетчики времени и отданного заряда */
    Tools->clrTimeCounter();
    Tools->clrAhCharge();
  }

  MState *MGoType::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::STOP:  return new MStart(Tools); // Передумали стартовать
      case MDisplay::GO:    return new MGo(Tools);    // Погнали!
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);      // Два после зпт (можно и три)
    Tools->showAmp (Tools->getRealCurrent(), 2);
    return this;
  }



    /* Состояние разряда */
  MGo::MGo(MTools * Tools) : MState(Tools)
  {
      //  targetI = 100;                                            // Начальное задание тока

//  Display->drawLabel(        "Mode DISCHARGE", 0);
    Display->drawLabel("The process is running", 1);
    Display->clearLine(                          2);
    Display->drawParFl(      "Max current, A :", 3,  spI, 2); 
    Display->drawParFl(      "Min voltage, V :", 4, minV, 2);
    Tools->txDischargeGo(spI);                                // 0x24 Команда драйверу разряжать
    Display->newBtn(MDisplay::GO, MDisplay::STOP);
  }

  MState *MGo::fsm()
  {
    Tools->chargeCalculations();                  /* Подсчет отданных ампер-часов. Период 100мс. */
    if(Tools->getMilliVolt() <= minV)   return new MStop(Tools);       /* Следить за напряжением */

    // // Плавое увеличение тока разряда, примерно 0.5А в секунду
    // if(targetI != spI)
    // {
    //   targetI += 50;
    //   if(targetI >= spI) targetI = spI;
    //   Tools->txSetDiscurrent(targetI);
    // }

    switch (Display->getKey())
    {
      case MDisplay::STOP:  return new MStop(Tools);
      case MDisplay::GO:    Tools->txDischargeGo(spI); // 0x24 Попытаться запустить снова
        break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();

    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);  // продолжительность разряда
    Display->showAh(Tools->getAhCharge());                                // отданные ампер-часы

    Tools->showVolt(Tools->getRealVoltage(), 3);
    Tools->showAmp (Tools->getRealCurrent(), 3);    
    return this;
  };




  /* Состояние: "Завершение режима разряда" - удерживается индикация 
    о продолжительности и отданном заряде. */
  MStop::MStop(MTools * Tools) : MState(Tools)
  {
    Tools->txPowerStop();                             // 0x21
    Display->drawLabel("The discharge is stopped", 1);
    Display->drawLabel(      "Results (example):", 2);
      // В строки 3 ... 7 можно добавить результаты разряда
    Display->drawLabel
      (MOption::TypeAkb[Tools->readNvsShort("options", "akb", MPrj::tec_fixed)], 3);
    Display->drawParam(  "capacity, Ah ", 4,
      Tools->readNvsShort("options", "capacity", MPrj::capacity_fixed), 1);
    Display->drawParam("Given away, Ah ", 5, Tools->getAhCharge(), 1);      // Отдано
    Display->drawLabel(  ". . . . . . .", 6);
    Display->drawLabel(    ". . . . . .", 7);
    Board->ledsRed();
    Display->newBtn(MDisplay::START, MDisplay::EXIT);
  }    
  MState *MStop::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::START: return new MStart(Tools);     // Возобновить
      case MDisplay::EXIT:  return new MExit(Tools);      // Выйти в меню
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(),  2);
    return this;
  }; //MStop




  /* Состояние: "Выход из режима разряда в меню диспетчера (автоматический)"  */
  MExit::MExit(MTools * Tools) : MState(Tools)
  {
    Tools->aboutMode(MDispatcher::DISCHARGE);
    Board->ledsOff();
    Display->newBtn(MDisplay::GO, MDisplay::UP, MDisplay::DN);
  }

  MState *MExit::fsm() { return nullptr; };          // В меню
};
