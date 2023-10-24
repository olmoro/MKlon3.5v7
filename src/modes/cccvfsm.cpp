/*
  Файл: cccvfsm.cpp            Как это работает.
    Режим простого заряда, известный как CC/CV, реализован в виде конечного автомата, 
  инициализация которого производится посредством выбора "CCCV" в меню диспетчера.
    Предварительно в OPTIONS задаются:
  - 
  - 
  График такого алгоритма заряда представлен на рисунке  http://www.balsat.ru/statia2.php.
    Есть два варианта ввода параметров заряда - по типу батареи или по выбору пользователем.
  В любом случае параметры сохраняются в энергонезависимой памяти. 
    Данная версия CCCV представляет минималистический вариант: исключены такие состояния, 
  как предзаряд, дозаряд, хранение, использование профилей пользователя, реализация которых 
  не составит труда для владеющего технологией программирования в среде Arduino и четкого 
  представления о принципе работы конечного автомата. 
    Нетрудно заметить, что каждое состояние, описываемое своим классом, в конструкторах
  классов в значительной степени повторяется. С одной стороны это приводит к дополнительному 
  расходу памяти, но позволяет абстрагироваться от необходимости держать в голове массу
  информации. Куда как проще начинать каждое состояние "с чистого листа" - вплоть до того, 
  чтобы поручить разработку конкретного состояния ком-то, кто лучше в нём разбирается, 
  не так ли?
    И напоследок: замечено, что не следует создавать сложные состояния, лучше их дробить
  на более простые. 
  Версия от 20.09.2023 
*/

#include "modes/cccvfsm.h"
#include "mdispatcher.h"        // Файл управления работой конечных автоматов
#include "project_config.h"     // Файл конфигурации проекта
#include "mtools.h"             // Файл "арсенал" разработчика
#include "mcmd.h"               // Команды обмена между контроллерами
#include "board/mboard.h"       // Файл утилит аппаратной поддержки
#include "board/msupervisor.h"  // Поддержка фоновых процессов
#include "display/mdisplay.h"   // Файл утилит графического RGB дисплея
#include <Arduino.h>
//#include <string>

namespace MCccv
{
  short maxV, minV, maxI, minI;
  short voltageNom    = MPrj::nominal_v_fixed;
  short capacity      = MPrj::capacity_fixed;
  static uint8_t mark = 3;

  //========================================================================= MStart
    // Состояние "Старт", инициализация выбранного режима работы (CC/CV).
  MStart::MStart(MTools * Tools) : MState(Tools)
  {
    /* Каждое состояние представлено своим классом. Это конструктор класса MStart, 
    исполняющий процесс инициализации, и только один раз, пока не произведен 
    вход и иное состояние. */

      //Отключить на всякий пожарный силовую часть.
    Tools->txPowerStop();                 // 0x21  Команда драйверу

    /*  Параметры заряда восстанавливаются из энергонезависимой памяти и соответствуют 
      предыдущему включению. Пороговые значения напряжений и токов рассчитываются исходя
      из типа батареи, её номинального напряжения и емкости, введенные в режиме "OPTION".
        Или путем коррекции пороговых значений, которые также сохраняются и восстанавливаются 
      из энергонезависимой памяти.
        В случае отсутствия в памяти таковых данных они заменяются рассчитанными (3-й аргумент).
    */
    voltageNom = Tools->readNvsShort("options", "nominalV", MPrj::nominal_v_fixed);
    capacity   = Tools->readNvsShort("options", "capacity", MPrj::capacity_fixed);
      // Параметры, вычисленные по технологии и емкости батареи, введенные в режиме OPTIONS
    maxV = Tools->readNvsShort("options", "maxV", MPrj::max_v_fixed);
    minV = Tools->readNvsShort("options", "minV", MPrj::min_v_fixed);
    maxI = Tools->readNvsShort("options", "maxI", MPrj::max_i_fixed);
    minI = Tools->readNvsShort("options", "minI", MPrj::min_i_fixed);

    /* Вывод в главное окно построчно (26 знакомест):
      текст, номер строки, параметр (для float число знаков после запятой) */ 
    Display->drawLabel(   "CCCV loaded:", 0);
                      //      Name        line    value
    Display->drawShort("voltageNom, V :", 1, voltageNom/1000);
    Display->drawShort( "capacity, Ah :", 2, capacity);
                            //  Name     line value dp
    Display->drawParFl(      "maxV, V :", 3, maxV, 2);
    Display->drawParFl(      "minV, V :", 4, minV, 2);
    Display->drawParFl(      "maxI, A :", 5, maxI, 2);
    Display->drawParFl(      "minI, A :", 6, minI, 2);

    /* Активировать группу кнопок: newBtn() отменит активацию ранее выведенных на 
      экран кнопок. Далее кнопки будут задаваться одной строкой. */
    Display->newBtn(MDisplay::START);   // Стартовать без уточнения параметров
#ifdef CCCV_ADJ
    Display->addBtn(MDisplay::ADJ);     // Корректировать параметры
#endif
    Display->addBtn(MDisplay::STOP);    // Отказаться от заряда

    Board->ledsOn();  // Подтверждение входа в любой режим белым свечением светодиода
  }

  MState *MStart::fsm()   // Вызов виртуальной функции
  {
    switch (Display->getKey())    //Здесь так можно
    {
      case MDisplay::STOP:  return new MStop(Tools);          // Прервано оператором
#ifdef CCCV_ADJ
      case MDisplay::ADJ:   
      /* Если введенные параметры заряда не устраивают, то по "ADJ" предлагается 
        ввести и при необходимости отредактировать сохраненные в Nvs. */
                                            return new MAdjParameters(Tools);
#endif
      case MDisplay::START:
          /* Используются вычисленные параметры по типу батареи, выбираемой в OPTIONS.
            Заявлен переход в состояние задержки пуска */
                                            return new MPostpone(Tools);
      default:;
    }
      /* Индикация текущих значений, указывается число знаков после запятой */
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    return this; };  // При следующем вызове задачи mainTask будет вызвана эта же функция



#ifdef CCCV_ADJ
  //======================================================================== MAdjParameters
  // Состояние "Коррекция параметров заряда".
  MAdjParameters::MAdjParameters(MTools * Tools) : MState(Tools)
  {
    maxV = Tools->readNvsShort("cccv", "maxV", MPrj::max_v_fixed);
    minV = Tools->readNvsShort("cccv", "minV", MPrj::min_v_fixed);
    maxI = Tools->readNvsShort("cccv", "maxI", MPrj::max_i_fixed);
    minI = Tools->readNvsShort("cccv", "minI", MPrj::min_i_fixed);

      // Вывести в главное окно
      /* Для выделения цветом строки, с которой будет начат выбор, пятым 
        аргументом указывается тот же номер строки */
    mark = 3;
    Display->drawLabel(                "CCCV", 0);
    Display->drawLabel("Adjusting parameters", 1);
    Display->clearLine(                        2);
    Display->drawParFl(           "maxV, V :", 3, maxV, 2, mark);
    Display->drawParFl(           "minV, V :", 4, minV, 2);
    Display->drawParFl(           "maxI, A :", 5, maxI, 2);
    Display->drawParFl(           "minI, A :", 6, minI, 3);
    Board->ledsBlue();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);  // Активировать группу кнопок
  }

  MState *MAdjParameters::fsm()
  {
    switch (Display->getKey())
    {
      //case MDisplay::STOP:  return new MStop(Tools);  // Прервано оператором
      /* Увы, такой кнопки нет, да и через MStop возвращаться для выбора иного 
        параметра было бы неудобно */
      case MDisplay::NEXT:  (mark >= 6) ? mark = 3 : mark++;  // Переместить курсор (выделение цветом)
                            Display->drawParFl("maxV, V :", 3, maxV, 2, mark);
                            Display->drawParFl("minV, V :", 4, minV, 2, mark);
                            Display->drawParFl("maxI, A :", 5, maxI, 2, mark);
                            Display->drawParFl("minI, A :", 6, minI, 2, mark);
                            break;
      case MDisplay::BACK:      // Мучения выбора:
            // Перейти на состояние работы с клавиатурой return new MLoadParameter(Tools);
            //return new MStop(Tools);        // Прервано оператором    test
            //return new MUpCurrent(Tools);    // Перейти к заряду    test
        //break;
                      return new MStart(Tools);    // Вернуться к старту заряда
      case MDisplay::GO:    switch ( mark )
                            {
                              case 3:   return new MSetVoltageMax(Tools);
                              case 4:   return new MSetVoltageMin(Tools);
                              case 5:   return new MSetCurrentMax(Tools);
                              case 6:   return new MSetCurrentMin(Tools);
                              default:  break;
                            }
      default:;
    }
    Tools->showVolt( Tools->getRealVoltage(), 2 );
    Tools->showAmp( Tools->getRealCurrent(), 2 );
    return this; };

   //======================================================================== MSetCurrentMax
    // Состояние "Коррекция максимального тока заряда".
  MSetCurrentMax::MSetCurrentMax(MTools * Tools) : MState(Tools)
  {
    /* Параметр maxI, как и некоторые ниже, уже восстановлен из энергонезависимой памяти, 
      иначе здесь была бы строка
      maxI = Tools->readNvsShort("cccv", "maxI", MPrj::max_i_fixed);
    */
      // В главное окно выводятся:
    mark  = 4;    // Регулируемый параметр всегда будет в 4-й строке, 
    Display->drawLabel(       "Mode CCCV", 0);            // режим,
    Display->drawLabel( "Adjusting maxI:", 1);            // полное название параметра (жёлтым)
    Display->clearLine(                    2);
    Display->drawParFl(     "Last maxI :", 3, maxI, 2);    // и его значение до   
    Display->drawParFl(     "New maxI :",  4, maxI, 2);    // и его значение    
    Display->clearLine(                    5, 7);          // Остальные строки очищаются
    Board->ledsBlue();                                     // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);  // Активировать группу кнопок
  }

  MState *MSetCurrentMax::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("cccv", "maxI", maxI);                
        return new MAdjParameters(Tools);                        // Там выбрать или завершить
      case MDisplay::UP:    maxI = Tools->updnInt(maxI, dn, up, +50);
                            Display->drawLabel("Max Current changed:", 1); 
                            Display->drawParFl( "New maxI :", 4, maxI, 2, mark );
                            break;
      case MDisplay::DN:    maxI = Tools->updnInt( maxI, dn, up, -50 );
                            Display->drawLabel("Max Current changed:", 1); 
                            Display->drawParFl(          "New maxI :", 4, maxI, 2, mark ); 
                            break;
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    return this; };

  //========================================================================= MSetVoltageMax
    // Состояние: "Коррекция максимального напряжения"
  MSetVoltageMax::MSetVoltageMax(MTools * Tools) : MState(Tools)
  {
    mark  = 4;
      // В главное окно выводятся:
//  Display->drawLabel(      "Mode CCCV", 0);           // режим,
    Display->drawLabel("Adjusting maxV:", 1);           // полное название параметра (жёлтым)
    Display->clearLine(                   2);                       
    Display->drawParFl(    "Last maxV :", 3, maxV, 2);  // и его значение    
    Display->drawParFl(     "New maxV :", 4, maxV, 2);  // и его значение    
    Display->clearLine(                   5, 7);        // Остальные строки очищаются
    Board->ledsBlue();                                  // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MSetVoltageMax::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("cccv", "maxV", maxV);                
        return new MAdjParameters(Tools);
      case MDisplay::UP:    maxV = Tools->updnInt( maxV, dn, up, +50);
                            Display->drawLabel("Max Voltage changed:", 1); 
                            Display->drawParFl(          "New maxV :", 4, maxV, 2, mark);
                            break;
      case MDisplay::DN:    maxV = Tools->updnInt( maxV, dn, up, -50 );
                            Display->drawLabel("Max Voltage changed:", 1); 
                            Display->drawParFl(          "New maxV :", 4, maxV, 2, mark); 
                            break;
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    return this; };

  //========================================================================= MSetCurrentMin
  // Состояние: "Коррекция минимального тока заряда"
  /* Торжествуйте, сторонники copy-past'а - далее будет почти без комментариев.
    Что? Угнетает некоторая избыточность? Зато есть полная уверенность, что именно вы 
    "виляете хвостом", а не хвост вами. */
  MSetCurrentMin::MSetCurrentMin(MTools * Tools) : MState(Tools)
  {
    mark = 4;
    Display->drawLabel(      "Mode CCCV", 0);
    Display->drawLabel("Adjusting minI:", 1);
    Display->clearLine(                   2);                       
    Display->drawParFl(    "Last minI :", 3, minI, 2);
    Display->drawParFl(     "New minI :", 4, minI, 2);
    Display->clearLine(                   5, 7);                       
    Board->ledsBlue();
    Display->newBtn( MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MSetCurrentMin::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("cccv", "minI", minI);                
        return new MAdjParameters(Tools);
      case MDisplay::UP:   minI = Tools->updnInt(minI, dn, up, +50);
        Display->drawLabel("Min Current changed:", 1); 
        Display->drawParFl(          "New minI :", 4, minI, 2, mark);
        break;
      case MDisplay::DN:   minI = Tools->updnInt( minI, dn, up, -50);
        Display->drawLabel("Min Current changed:", 1); 
        Display->drawParFl(          "New minI :", 4, minI, 2, mark); 
        break;
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    return this; };

  //========================================================================= MSetVoltageMin
  // Состояние: "Коррекция минимального напряжения окончания заряда"
  MSetVoltageMin::MSetVoltageMin(MTools * Tools) : MState(Tools)
  {
    mark = 4;
    Display->drawLabel(      "Mode CCCV", 0);
    Display->drawLabel("Adjusting minV:", 1);
    Display->clearLine(                   2);                       
    Display->drawParFl(    "Last minV :", 3, minV, 2);
    Display->drawParFl(     "New minV :", 4, minV, 2);
    Display->clearLine(                   5, 7);                       
    Board->ledsBlue();
    Display->newBtn( MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MSetVoltageMin::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("cccv", "minV", minV);                
        return new MAdjParameters(Tools);
      case MDisplay::UP:   minV = Tools->updnInt( minV, dn, up, +50);
        Display->drawLabel("Min Voltage changed:", 1); 
        Display->drawParFl(          "New minV :", 4, minV, 2, mark);
        break;
      case MDisplay::DN:   minV = Tools->updnInt( minV, dn, up, -50);
        Display->drawLabel("Min Voltage changed:", 1); 
        Display->drawParFl(          "New minV :", 4, minV, 2, mark); 
        break;
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    /* В последующих состояниях указывать отображение тока и напряжения не будем...*/
    return this; };

#endif


  //========================================================================= MPostpone
  // Состояние: "Задержка включения (отложенный старт)", время ожидания старта задается в OPTIONS.
  MPostpone::MPostpone(MTools * Tools) : MState(Tools)
  {
      // Параметр задержки начала заряда из энергонезависимой памяти, при первом включении - заводское
    Tools->postpone = Tools->readNvsShort("options", "postpone", 0);

    kp = Tools->readNvsFloat("device", "kpI", MPrj::kp_i_default);      //MConst::fixedKpI);
    ki = Tools->readNvsFloat("device", "kiI", MPrj::ki_i_default);      //MConst::fixedKiI);
    kd = Tools->readNvsFloat("device", "kdI", MPrj::kd_i_default);      //MConst::fixedKdI);
    Tools->txSetPidCoeffI(kp, ki, kd);  
    //Tools->txSetPidCoeffI(0.01, 0, 0);  


        vTaskDelay( 200 / portTICK_PERIOD_MS );


      // Восстановление пользовательских kp, ki
    //kp = Tools->readNvsFloat("cccv", "kpV", MConst::fixedKpV);
    kp = Tools->readNvsFloat("device", "kpV", MPrj::kp_v_default); // Так можно, но там другое
    ki = Tools->readNvsFloat("device", "kiV", MPrj::ki_v_default);      //MConst::fixedKiV);
    kd = Tools->readNvsFloat("device", "kdV", MPrj::kd_v_default);      //MConst::fixedKdV);
    //Tools->txSetPidCoeffV(kp, 0.5, 0.0);  // 0x41  Команда драйверу
    //Tools->txSetPidCoeffV(kp, ki, kd);  // 0x41  Команда драйверу
    Tools->txSetPidCoeffV(1.0, 1.8, 0.1);  // 0x41  Команда драйверу


      // Инициализация счетчика времени до старта
    Tools->setTimeCounter( Tools->postpone * 36000 );    // Отложенный старт ( * 0.1s в этой версии)
    Display->drawLabel(     "Mode CCCV", 0);
    Display->drawLabel("Delayed start:", 1);
    Display->clearLine(                  2);
    Display->drawShort("postpone, hr :", 3, Tools->postpone );
    Display->clearLine(                  4, 7);
    mark = 3;
  }

  MState *MPostpone::fsm()
  {
      // Старт по времени
    if(Tools->postponeCalculation())  return new MUpCurrent(Tools);

    switch (Display->getKey())
    {
      case MDisplay::STOP:            return new MStop(Tools);
      case MDisplay::GO:              return new MUpCurrent(Tools);
      default:;
    }
    // Индикация в период ожидания старта (обратный отсчет)
    Display->showDuration( Tools->getChargeTimeCounter(), MDisplay::SEC );
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    //Board->blinkWhite();                // Исполняется  некорректно - пока отменено
    return this; };

  //========================================================================= MUpCurrent
  /* Состояние: "Подъем и удержание максимального тока"
    Начальный этап заряда - ток поднимается не выше заданного уровня, при достижении 
  заданного максимального напряжения переход к его удержанию. Здесь и далее подсчитывается
  время до окончания и отданный заряд, а также сохраняется возможность прекращения заряда
  оператором. */
  MUpCurrent::MUpCurrent(MTools * Tools) : MState(Tools)
  {
    mark = 3;
    Display->drawLabel(            "CCCV", 0);
    Display->drawLabel(   "Const Current", 1);
    Display->clearLine(                    2);
    Display->drawParFl(       "maxI, A :", 3, maxI, 2);
    Display->drawParFl(       "maxV, V :", 4, maxV, 2);
    Display->drawLabel(". . . wait . . .", 5);
    Display->clearLine(                    6, 7);
    Board->ledsGreen();
    Display->newBtn( MDisplay::STOP, MDisplay::NEXT);
      // Обнуляются счетчики времени и отданного заряда
    Tools->clrTimeCounter();
    Tools->clrAhCharge();

    /* Включение преобразователя и коммутатора драйвером силовой платы.
     Параметры PID-регулятора заданы в настройках прибора (DEVICE).
     Здесь задаются сетпойнты по напряжению и току. Подъем тока
     производится ПИД-регулятором.
    */ 
    Tools->txPowerAuto(maxV, maxI);     // 0x20  Команда драйверу
  }

  MState *MUpCurrent::fsm()
  {
    Tools->chargeCalculations();                // Подсчет отданных ампер-часов.

    // // Плавое увеличение тока разряда, примерно 0.5А в секунду
    // if(targetI != spI)
    // {
    //   targetI += 50;
    //   if(targetI >= spI) targetI = spI;
    //   //Tools->txSetDiscurrent(targetI);
    //   Tools->txAdjCurrent(target);             // Новое имя
    // }

    switch (Display->getKey() )
    {
      case MDisplay::STOP:                        return new MStop(Tools);
        // Переход к следующей фазе оператором
      case MDisplay::NEXT:                        return new MKeepVmax(Tools);
      default:;
    }

    // Проверка напряжения и переход на поддержание напряжения.
    //if(Tools->getRealVoltage() >= maxV)         return new MKeepVmax(Tools);
    if(Tools->getMilliVolt() >= maxV)         return new MKeepVmax(Tools);
    
      // Индикация фазы подъема тока не выше заданного
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);

    return this;  };

  //========================================================================= MKeepVmax
    // Состояние: "Удержание максимального напряжения"
  /*  Вторая фаза заряда - достигнуто заданное максимальное напряжение.
    Настройки регулятора не меняются, по факту состояние необходимо только для 
    изменения индикации.
    При падении тока ниже заданного уровня - переход к третьей фазе. */

  MKeepVmax::MKeepVmax(MTools * Tools) : MState(Tools)
  {
    Display->drawLabel(             "CCCV", 0);
    Display->drawLabel(    "Const Voltage", 1);
    Display->clearLine(                     2);
    Display->drawParFl(        "maxI, A :", 3, maxI, 2);
    Display->drawParFl(        "maxV, V :", 4, maxV, 2);
    Display->drawLabel( ". . . wait . . .", 5);
    Display->clearLine(                     6, 7);
    Display->newBtn(MDisplay::STOP, MDisplay::NEXT);
    Board->ledsYellow();
    mark = 3;

    //Tools->txPowerAuto(maxV, maxI);              // 0x20  Команда драйверу
  }

  MState *MKeepVmax::fsm()
  {
    Tools->chargeCalculations();                 // Подсчет отданных ампер-часов.

    switch (Display->getKey())
    {
    case MDisplay::STOP:                          return new MStop(Tools);
      // Переход к следующей фазе оператором
    case MDisplay::NEXT:                          return new MKeepVmin(Tools);
    default:;
    }
      // Ожидание спада тока ниже C/20 ампер.
    if( Tools->getMilliAmper() <= minI )         return new MKeepVmin(Tools);

    // Индикация фазы удержания максимального напряжения
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2);
    return this; };

  //========================================================================= MKeepVmin
  /*  Третья фаза заряда - достигнуто снижение тока заряда ниже заданного предела.
    Проверки различных причин завершения заряда. */
  MKeepVmin::MKeepVmin(MTools * Tools) : MState(Tools)
  {
    timeOut = Tools->readNvsShort("options", "timeout", 0);

    Display->drawLabel("CCCV", 0);
    Display->drawLabel("Keep Vmin voltage", 1);
    Display->clearLine(2, 3);
    Display->drawParFl("minV, V :", 4, minV, 2);
    Display->drawLabel(". . . wait . . .", 5 );
    Display->clearLine(6, 7);
    Board->ledsYellow();
    Display->newBtn(MDisplay::STOP, MDisplay::NEXT);

      // Порог регулирования по минимальному напряжению
    Tools->txPowerAuto(minV, maxI);        // 0x20  Команда драйверу
  }

  MState *MKeepVmin::fsm()
  {
    Tools->chargeCalculations();           // Подсчет отданных ампер-часов.

    switch (Display->getKey())
    {
      case MDisplay::STOP:                  return new MStop(Tools);
      case MDisplay::NEXT:                  return new MStop(Tools);
      default:;
    }
    // Здесь возможны проверки других условий окончания заряда
    // if( ( ... >= ... ) && ( ... <= ... ) )  { return new MStop(Tools); }

      // Максимальное время заряда, задается в "Настройках"
    //if(Tools->getChargeTimeCounter() >= (Tools->charge_time_out_limit * 36000))  return new MStop(Tools);
    if(Tools->getChargeTimeCounter() >= (timeOut * 36000))    return new MStop(Tools);

    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
      /* Индикация тока и напряжения. Есть возможность ввести третьим аргументом, 
        названным "beauty" лучшее сглаживание выводимого параметра (по умолчанию 2),
        что никоим образом не скажется на регулировании, а только отфильтрует 
        картинку на браузере и дисплее. Для отмены придётся задать "2" */
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 3, 3);
    return this; };

  //========================================================================= MStop
  // Состояние: "Завершение заряда"
    /* Завершение режима заряда - до нажатия кнопки "С" удерживается индикация 
      о продолжительности и отданном заряде. */
  MStop::MStop(MTools * Tools) : MState(Tools)
  {
    Tools->txPowerStop();             // 0x21 Команда драйверу отключить преобразователь
    Display->drawLabel("CCCV", 0);
    Display->drawLabel("The charge is stopped", 1);
    Display->drawLabel("Statistics:", 2);
      // В строки 3 ... 7 можно добавить результаты заряда
    Display->drawLabel( ". . . . . .", 3);
    Display->drawLabel( ". . . . . .", 4);
    Display->drawLabel( ". . . . . .", 5);
    Display->drawLabel( ". . . . . .", 6);
    Display->drawLabel( ". . . . . .", 7);
    Board->ledsRed();
    Display->newBtn(MDisplay::START, MDisplay::EXIT);
  }    
  MState * MStop::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::START:         return new MStart(Tools); // Возобновить
      case MDisplay::EXIT:          return new MExit(Tools);  // Выйти в меню
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp(Tools->getRealCurrent(), 2, 2);
    return this; }; //MStop

  //========================================================================= MExit
  // Состояние: "Выход из режима заряда в меню диспетчера (автоматический)" 
  MExit::MExit(MTools * Tools) : MState(Tools)
  {
    Tools->aboutMode(MDispatcher::CCCV);
    Board->ledsOff();
    Display->newBtn(MDisplay::GO, MDisplay::UP, MDisplay::DN);
  }

  MState *MExit::fsm() { return nullptr; };  // В меню

}; //namespace MCccv
