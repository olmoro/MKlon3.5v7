/*
  Файл: cccvfsm.cpp            Как это работает.
    Режим простого заряда, известный как CC/CV (http://www.balsat.ru/statia2.php), 
  реализован в виде конечного автомата, инициализация которого производится посредством 
  выбора "CCCV" в меню диспетчера.
    Предварительно в OPTIONS задаются: тип батареи, номинальное напряжение и ёмкость,
  на основании которых вычисляются: максимальный ток заряда, максимальное напряжение 
  заряда и пороговое значение тока заряда. Дополнительно задаются временные параметры - 
  задержка пуска и время выдержки минимальным током заряда. 
    И это ещё не всё. Регулирование заряда производится ПИД-регулятором, коэффициенты 
  которого должны быть подобраны в режиме DEVICE под конкретные параметры прибора. Не 
  исключено, что для разных типов заряжаемых батарей коэффициенты будут отличаться. 
  В помощь разработчику рекомендую https://youtu.be/CgKPvyRrpzo - "Настройка 
  ПИД-регулятора методом граблей, бубна и напильника" - увлекательное занятие.
    Параметры сохраняются, а потом берутся из энергонезависимой памяти. В этом примере 
  некоторые параметры заменены на тестовые по понятной причине - сократить время между 
  переходами от одного состояния к другому в процессе тестирования до нескольких минут, 
  да и гонять батарею, сокращая её жизненный цикл жалко.
    Данная версия CCCV представляет минималистический вариант: исключены такие состояния, 
  как предзаряд, дозаряд, хранение, использование профилей пользователя, реализация которых 
  не составит труда для владеющего технологией программирования в среде Arduino и четкого 
  представления о принципе работы конечного автомата. 
    Нетрудно заметить, что каждое состояние, описываемое своим классом, в конструкторах
  классов в значительной степени повторяется. С одной стороны это приводит к дополнительному 
  расходу памяти, но позволяет абстрагироваться от необходимости держать в голове массу
  информации. Куда как проще начинать каждое состояние "с чистого листа" - вплоть до того, 
  чтобы поручить разработку конкретного состояния кому-то, кто лучше в нём разбирается, 
  не так ли?
    И напоследок: замечено, что не следует создавать сложные состояния, лучше их дробить
  на более простые. 
  Версия от 06.11.2023 
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

namespace MCccv
{
  short maxV, maxI, minI, minV;                 // Заданные параметры заряда
  float kpV, kiV, kdV;                          // Заданные коэффициенты ПИД-регулятора напряжения
  float kpI, kiI, kdI;                          // Заданные коэффициенты ПИД-регулятора тока
  short voltageNom    = MPrj::nominal_v_fixed;  // Номинальное напряжение батареи, вольты 
  short capacity      = MPrj::capacity_fixed;   // Ёмкость батареи, ампер-часы
  short timeOut       = MPrj::timeout_fixed;    // Длительность заряда, часы

  //========================================================================= MStart
    // Состояние "Старт", инициализация выбранного режима работы (CC/CV).
  MStart::MStart(MTools * Tools) : MState(Tools)
  {
    /* Каждое состояние представлено своим классом. Это конструктор класса MStart, 
    исполняющий процесс инициализации, и только один раз, пока не произведен 
    вход и иное состояние. */

      //Отключить на всякий пожарный силовую часть.
    Tools->txPowerStop();                 // 0x21  Команда драйверу
    /*  Параметры заряда восстанавливаются из энергонезависимой памяти. Пороговые значения 
    напряжений и токов рассчитываются исходя из типа батареи, её номинального напряжения и 
    емкости, введенные в режиме "OPTION".
      В случае отсутствия в памяти таковых данных они заменяются рассчитанными (3-й аргумент).
    */
    voltageNom = Tools->readNvsShort("options", "nominalV", MPrj::nominal_v_fixed);
    capacity   = Tools->readNvsShort("options", "capacity", MPrj::capacity_fixed);
      // Параметры, вычисленные по технологии и емкости батареи, введенные в режиме OPTIONS:
    maxV       = Tools->readNvsShort("options", "maxV", MPrj::max_v_fixed);
    minV       = Tools->readNvsShort("options", "minV", MPrj::min_v_fixed);  // Test 14000
    maxI       = Tools->readNvsShort("options", "maxI", MPrj::max_i_fixed);
    minI       = Tools->readNvsShort("options", "minI", MPrj::min_i_fixed);  // Test 550
    timeOut    = 1;  //Tools->readNvsShort("options", "timeout",  MPrj::timeout_fixed);   // Test

    /* Вывод в главное окно построчно (26 знакомест):
                              текст    номер строки  параметр (для float число знаков после запятой) */ 
    Display->drawLabel(  "CCCV loaded:",   0);
    Display->drawShort("voltageNom, V:",   1,      voltageNom/1000);
    Display->drawShort( "capacity, Ah:",   2,      capacity);
    Display->drawParFl(      "maxV, V:",   3,      maxV, 2);
    Display->drawParFl(      "minV, V:",   4,      minV, 2);
    Display->drawParFl(      "maxI, A:",   5,      maxI, 2);
    Display->drawParFl(      "minI, A:",   6,      minI, 2);
    Display->drawShort(  "timeout, hr:",   7,      timeOut);

    /* Активировать группу кнопок: newBtn() отменит активацию ранее выведенных на 
      экран кнопок. Далее кнопки будут задаваться одной строкой. */
    Display->newBtn(MDisplay::START);   // Стартовать без уточнения параметров
    Display->addBtn(MDisplay::STOP);    // Отказаться от заряда
    Board->ledsOn();  // Подтверждение входа в любой режим белым свечением светодиода
  }

  MState *MStart::fsm()   // Вызов виртуальной функции
  {
    switch (Display->getKey())    //Здесь так можно
    {
      case MDisplay::STOP:    return new MStop(Tools);          // Прервано оператором
      case MDisplay::START:   return new MPostpone(Tools);
      default:;
    }
      /* Индикация текущих значений, указывается число знаков после запятой */
    Tools->showVolt(Tools->getRealVoltage(), 2);
    Tools->showAmp (Tools->getRealCurrent(), 2);
    return this; };  // При следующем вызове задачи mainTask будет вызвана эта же функция




  //========================================================================= MPostpone
    /*  Состояние: "Задержка включения (отложенный старт)", время ожидания старта 
      задается в OPTIONS. Если не задан - заводское значение. */
  MPostpone::MPostpone(MTools * Tools) : MState(Tools)
  {
    Tools->postpone = Tools->readNvsShort("options", "postpone", MPrj::postpone_fixed);
      // Восстановление пользовательских kp, ki, kd
    kpI = Tools->readNvsFloat("device", "kpI", MPrj::kp_i_default);
    kiI = Tools->readNvsFloat("device", "kiI", MPrj::ki_i_default);
    kdI = Tools->readNvsFloat("device", "kdI", MPrj::kd_i_default);
    
//  Serial.print("\nkp="); Serial.print(kpI, 2);  
//  Serial.print("\nki="); Serial.print(kiI, 2);  
//  Serial.print("\nkd="); Serial.print(kdI, 2);  
    
    Tools->txSetPidCoeffI(kpI, kiI, kdI);                             // 0x41 Применить
 
    kpV = Tools->readNvsFloat("device", "kpV", MPrj::kp_v_default);
    kiV = Tools->readNvsFloat("device", "kiV", MPrj::ki_v_default);
    kdV = Tools->readNvsFloat("device", "kdV", MPrj::kd_v_default);
    Tools->txSetPidCoeffV(kpV, kiV, kdV);                             // 0x41 Применить

      // Инициализация счетчика времени до старта
    Tools->setTimeCounter( Tools->postpone * 36000 );    // Отложенный старт ( * 0.1s в этой версии)

    Display->drawLabel(     "Mode CCCV", 0);
    Display->drawLabel("Delayed start:", 1);
    Display->clearLine(                  2);
    Display->drawShort( "postpone, hr:", 3, Tools->postpone );
    Display->clearLine(                  4, 7);
    Board->ledsCyan();
  }

  MState *MPostpone::fsm()
  {
    if(Tools->postponeCalculation())  return new MUpCurrent(Tools);   // Старт по истекшему времени

    switch (Display->getKey())
    {
      case MDisplay::STOP:            return new MStop(Tools);
      case MDisplay::GO:              return new MUpCurrent(Tools);
      default:;
    }
    // Индикация в период ожидания старта (обратный отсчет)
    Display->showDuration( Tools->getChargeTimeCounter(), MDisplay::SEC );

    /* Для индикации повысим уровень сглаживания показаний измерений. */
    Tools->showVolt(Tools->getRealVoltage(), 2, 3);
    Tools->showAmp (Tools->getRealCurrent(), 2, 3);
    return this; };

  //========================================================================= MUpCurrent
  /* Состояние: "Подъем и удержание максимального тока"
    Начальный этап заряда - ток поднимается не выше заданного уровня, при достижении 
  заданного максимального напряжения переход к его удержанию. Здесь и далее подсчитывается
  время до окончания и отданный заряд, а также сохраняется возможность прекращения заряда
  оператором. */
  MUpCurrent::MUpCurrent(MTools * Tools) : MState(Tools)
  {
    Display->drawLabel(           "CCCV", 0);
    Display->drawLabel(  "Const Current", 1);
    Display->clearLine(                   2);
    Display->drawParFl(  "SetpointI, A:", 3, maxI, 2);
    Display->drawParFl(  "Wait maxV, V:", 4, maxV, 2);
    Display->drawParam(           "KpI:", 5,  kpI, 2);
    Display->drawParam(           "KiI:", 6,  kiI, 2);
    Display->drawParam(           "KdI:", 7,  kdI, 2);
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
    Tools->txPidClear();                // 0x44
    Tools->txPowerAuto(maxV, maxI);     /* 0x20  Команда драйверу запустить ПИД-регулятор
                                          в автоматическом режиме */
  }

  MState *MUpCurrent::fsm()
  {
    Tools->chargeCalculations();           // Подсчет отданных ампер-часов.

    switch (Display->getKey())
    {
      case MDisplay::STOP:                    return new MStop(Tools);
        /* Переход к следующей фазе оператором. По факту ПИД-регулятор в автоматическом режиме
        не "отдаст" управление, пока ток не достигнет установленного предела (в этом и есть суть
        автоматического режима - задаются и ток, и напряжение), здесь производится переход в 
        состояние поддержки напряжения для индикации условий, которые БУДУТ заданы. */
      case MDisplay::NEXT:                    return new MKeepVmax(Tools);
      default:;
    }

    // Проверка напряжения и переход на поддержание напряжения.
    if(Tools->getMilliVolt() >= maxV)         return new MKeepVmax(Tools);
    
      // Индикация фазы подъема тока не выше заданного
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    /* Уровни сглаживания можно не вводить всякий раз, однако при выходе из режима CCCV 
    всётаки лучше их вернуть к уровню 2. */
    Tools->showVolt(Tools->getRealVoltage(), 2, 3);
    Tools->showAmp (Tools->getRealCurrent(), 2, 3);

    return this;  };




  //========================================================================= MKeepVmax
    /* Состояние: "Удержание максимального напряжения"
    Вторая фаза заряда - достигнуто заданное максимальное напряжение.
    Настройки регулятора не меняются, по факту состояние необходимо только для 
    изменения индикации.
      При падении тока ниже заданного уровня - переход к третьей фазе. */

  MKeepVmax::MKeepVmax(MTools * Tools) : MState(Tools)
  {
    Display->drawLabel(            "CCCV", 0);
    Display->drawLabel(   "Const Voltage", 1);
    Display->clearLine(                    2);
    Display->drawParFl(   "SetpointV, V:", 3, maxV, 2);
    Display->drawParFl(   "Wait minI, A:", 4, minI, 2);
    Display->drawParam(            "KpV:", 5,  kpV, 2);
    Display->drawParam(            "KiV:", 6,  kiV, 2);
    Display->drawParam(            "KdV:", 7,  kdV, 2);
    Display->newBtn(MDisplay::STOP, MDisplay::NEXT);
    Board->ledsYellow();
  }

  MState *MKeepVmax::fsm()
  {
    Tools->chargeCalculations();                 // Подсчет отданных ампер-часов.

    switch (Display->getKey())
    {
    case MDisplay::STOP:                      return new MStop(Tools);
      // Переход к следующей фазе оператором
    case MDisplay::NEXT:                      return new MKeepVmin(Tools);
    default:;
    }
      // Ожидание спада тока ниже C/20 ампер.
    if(Tools->getMilliAmper() <= minI)        return new MKeepVmin(Tools);

    // Индикация фазы удержания максимального напряжения (текущие)
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    Tools->showVolt(Tools->getRealVoltage(), 2, 2); //Вольты покажем во всей красе
    Tools->showAmp (Tools->getRealCurrent(), 2, 3); //Скроем третий знак и отфильтруем
    return this; };



  //========================================================================= MKeepVmin
  /*  Третья фаза заряда - достигнуто снижение тока заряда ниже заданного предела.
    Проверки различных причин завершения заряда. */
  MKeepVmin::MKeepVmin(MTools * Tools) : MState(Tools)
  {
    Display->drawLabel(             "CCCV", 0);
    Display->drawLabel("Keep Vmin voltage", 1);
    Display->clearLine(                     2);
    Display->drawParFl(    "SetpointV, V:", 3, minV, 2);
    Display->drawShort("Wait timeout, hr:", 4, timeOut);
    Display->clearLine(                     5, 7);
    Board->ledsYellow();
    Display->newBtn(MDisplay::STOP, MDisplay::NEXT);

    Tools->clrTimeCounter();      // Обнуляются счетчики времени

      // Порог регулирования по минимальному напряжению
    //Tools->txPowerAuto(minV, maxI);        // 0x20  Команда драйверу
    Tools->txPowerAuto(minV, minI <<2);        // 0x20  Команда драйверу
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
    if(Tools->getChargeTimeCounter() >= (timeOut * 36000))    return new MStop(Tools);

    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
      /* Индикация тока и напряжения. Есть возможность ввести третьим аргументом, 
        названным "beauty" лучшее сглаживание выводимого параметра (по умолчанию 2),
        что никоим образом не скажется на регулировании, а только отфильтрует 
        картинку на браузере и дисплее. Для отмены придётся задать "2" */
    Tools->showVolt(Tools->getRealVoltage(), 2, 3);   // Зададим формат NN.NNN
    Tools->showAmp (Tools->getRealCurrent(), 2, 3);
    return this; };

  //========================================================================= MStop
    /* Состояние: "Завершение заряда". До нажатия на "EXIT" удерживается индикация 
      о продолжительности и отданном заряде. */
  MStop::MStop(MTools * Tools) : MState(Tools)
  {
    Tools->txPowerStop();             // 0x21 Команда драйверу отключить преобразователь
    Display->drawLabel(                 "CCCV", 0);
    Display->drawLabel("The charge is stopped", 1);
    Display->drawLabel(          "Statistics:", 2);
      // В строки 3 ... 7 можно добавить результаты заряда
    Display->drawLabel(          ". . . . . .", 3);
    Display->drawLabel(          ". . . . . .", 4);
    Display->drawLabel(          ". . . . . .", 5);
    Display->drawLabel(          ". . . . . .", 6);
    Display->drawLabel(          ". . . . . .", 7);
    Board->ledsRed();                                     // Стоп-сигнал
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
    Tools->showVolt(Tools->getRealVoltage(), 2, 2);
    Tools->showAmp (Tools->getRealCurrent(), 2, 2);
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
