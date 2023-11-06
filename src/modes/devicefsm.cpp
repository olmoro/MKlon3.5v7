/*
  Файл: devicefsm.cpp 02.11.2023
  Конечный автомат заводских регулировок - арсенал разработчика и тех, кто вносит
  изменения в аппаратную часть прибора, вольно или не вольно изменяя технические параметры:
  - коррекция приборного смещения и коэффициента преобразования по напряжению;
  - коррекция приборного смещения и коэффициента преобразования по току;
  - коррекция коэффициентов фильтрации измерений;
  - коррекция коэффициентов ПИД-регуляторов и частоты регулирования;
  - Коррекция настроек тачскрина дисплея.
  - Удаление ключей (не забыть удалить "profil1", "profil2", "profil3",)
    Перед коррекцией прибор должен быть прогрет в течение нескольких минут, желательно
  под нагрузкой или в режиме разряда.
    Коррекцию измерителей производить, подключив к клеммам "+" и "-"  внешний источник с
  регулируемым напряжением порядка 12 вольт по четырёхточечной схеме и эталонный измеритель
  напряжения.
    Прибор, кстати, отобразит ток, потребляемый входным делителем порядка 10 килоом, что
  свиделельствует об исправности входных цепей измерителей. Цель коррекции - минимальные
  отклонения во всем диапазоне от -2 до +17 вольт и от -3 до +6 ампер.
  Процесс коррекции сдвига (shift) чередовать с коррекцией коэффициента пересчета (factor).
    Режим DEVICE не использует настройки выбора типа батареи режимом OPTIONS. 


    Подбор коэффициентов ПИД-регулятора:
    https://youtu.be/CgKPvyRrpzo 
    
*/

#include "modes/devicefsm.h"
//#include "driver/mcommands.h"
#include "mdispatcher.h"
#include "mtools.h"
#include "board/mboard.h"
#include "display/mdisplay.h"
#include <Arduino.h>

namespace MDevice
{
  static uint8_t mark = 3;  // Строка, выделяемая цветом
  short spV, spI, spD;           // Пороговые значения для отладки "на лету"
  float kpV, kiV, kdV;      // Коэффициенты ПИД-регулятора по напряжению
  float kpI, kiI, kdI;      // то же по току заряда
  float kpD, kiD, kdD;      // то же по току разряда

  //========================================================================= MStart
  // Состояние "Старт", инициализация выбранного режима работы (DEVICE).
  MStart::MStart(MTools *Tools) : MState(Tools)
  {
    /* Каждое состояние представлено своим классом. Это конструктор класса MStart,
      исполняющий процесс инициализации, и только один раз, пока не произведен
      выход из этого состояния. */
    Tools->txPowerStop();  vTaskDelay(80 / portTICK_PERIOD_MS);  // 0x21 Перейти в безопасный режим
    mark = 2;                                           // Курсор на 2-ю строку
    Display->drawLabel("Mode DEVICE loaded:", 0);       // Режим, напоминание
    Display->clearLine(                       1);       // Пустая строка
    Display->drawAdj(                  "PID", 2, mark); // Коэффициенты ПИД-регуляторов
    Display->drawAdj(              "voltage", 3, mark); // Эта строка (mark=2) будет выделена
    Display->drawAdj(              "current", 4, mark);
    Display->drawAdj(            "DisplayXY", 5, mark); // Подкалибровать тачскрин (подшаманить)
    Display->drawAdj(                "Clear", 6, mark); // Очистка ключей NVS
    Display->clearLine(                       7);
    Board->ledsOn();                                    // Подтверждение входа белым свечением
    Display->newBtn(MDisplay::START, MDisplay::STOP, MDisplay::NEXT); // Кнопки   
  }

  MState *MStart::fsm()
  {
    /* Методы класса, а вернее метод один - функция fsm(), здесь её определение. Выполняется
     под управлением диспетчера при каждом вызове задачи mainTask, здесь период 0,1 секунды */
    switch (Display->getKey())
    {
    case MDisplay::STOP:     return new MStop(Tools); // Goodbye режим DEVICE
    case MDisplay::NEXT:    (mark >= 6) ? mark = 2 : mark++;      // Переместить курсор
                            Display->drawAdj(           "PID", 2, mark);
                            Display->drawAdj(       "voltage", 3, mark);
                            Display->drawAdj(       "current", 4, mark);
                            Display->drawAdj(     "DisplayXY", 5, mark);
                            Display->drawAdj(         "Clear", 6, mark);
                            break;
    case MDisplay::START:   switch (mark)
                            {
                              case 2: return new MAdjPid(Tools);     // kp, ki, kd
                              case 3: return new MAdjVoltage(Tools); // Voltage
                              case 4: return new MAdjCurrent(Tools); // Current
                              case 5: return new MMultXY(Tools);     // Поправочные множители тачскрина
                              case 6: return new MClear(Tools);      // Очистка энергонезависимой памяти
                              default:;
                            }
    default:;
    }
    /* Индикация текущих значений, указывается число знаков после запятой,
      по умолчанию указывается два. Впрочем, здесь это не актуально. */
    // Tools->showVolt(Tools->getRealVoltage(), 2);
    // Tools->showAmp (Tools->getRealCurrent(), 2);
    return this;  /* При следующем вызове исполнить fsm() снова. */
  }; //MStart

  //========================================================================= MAdjPid
  /* Состояние: "Выбор объекта коррекции параметров регулятора по току".
  */
  MAdjPid::MAdjPid(MTools *Tools) : MState(Tools)
  {
    // Список доступных регулировок
    mark = 3;
    Display->drawLabel(        "DEVICE", 0);
    Display->drawLabel("Adjusting Pid:", 1);
    Display->clearLine(                  2);
    Display->drawAdj(         "PID_I",   3, mark);
    Display->drawAdj(         "PID_V",   4, mark);
    Display->drawAdj(         "PID_D",   5, mark);
    Display->drawAdj( "Pid Frequency",   6, mark);
    Display->clearLine(                  7);
    Board->ledsBlue();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);
  }

  MState *MAdjPid::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::NEXT:  (mark >= 6) ? mark = 3 : mark++;      // Переместить курсор
                            Display->drawAdj(         "PID_I", 3, mark);
                            Display->drawAdj(         "PID_V", 4, mark);
                            Display->drawAdj(         "PID_D", 5, mark);
                            Display->drawAdj( "Pid Frequency", 6, mark);
                            break;
      case MDisplay::GO:    switch (mark)
                            {
                              case 3: return new MAdjPidI(Tools);      //
                              case 4: return new MAdjPidV(Tools);      //
                              case 5: return new MAdjPidD(Tools);      //
                              case 6: return new MPidFrequency(Tools); // Pid Frequency
                              default:;
                            }
      case MDisplay::BACK:  return new MStart(Tools); // Перейти к 
      default:;
    }
    return this;
  };


  //========================================================================= MAdjPidI
  /* Состояние выбора параметра ПИД-регулятора по току. Предварительно сетпойнт 
    по напряжению заведомо выше напряжения на клеммах (через состояние MAdjPidV/spV).
    Включение и выключение заряда (GO/STOP) производятся без выхода из данного 
    состояния. Светодиод индицирует зеленым когда ПИД-регулятор находится в режиме 
    поддержания тока. Переход к подбору параметров происходит "на лету", без 
    остановки заряда. */  
  MAdjPidI::MAdjPidI(MTools *Tools) : MState(Tools)
  {
    spI   = Tools->readNvsShort("device", "spI", sp_i_default);
    spV   = Tools->readNvsShort("device", "spV", sp_u_default);
    // Восстановление пользовательских kp, ki, kp
    kpI = Tools->readNvsFloat("device", "kpI", MPrj::kp_i_default);
    kiI = Tools->readNvsFloat("device", "kiI", MPrj::ki_i_default);
    kdI = Tools->readNvsFloat("device", "kdI", MPrj::kd_i_default);
    Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить

    mark = 3;
    Display->drawLabel(           "DEVICE", 0);
    Display->drawLabel( "Adjusting PID_I:", 1);
    Display->clearLine(                     2);
    Display->drawAdj  (         "STOP/GO!", 3,          mark);
    Display->drawParFl(          "Sp, A :", 4,  spI, 2, mark);
    Display->drawParam(             "Kp :", 5,  kpI, 2, mark);
    Display->drawParam(             "Ki :", 6,  kiI, 2, mark);
    Display->drawParam(             "Kd :", 7,  kdI, 2, mark);

    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);
  }

  MState *MAdjPidI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::NEXT:  (mark >= 7) ? mark = 3 : mark++;
                            Display->drawAdj  ("STOP/GO!", 3,         mark);
                            Display->drawParFl( "Sp, A :", 4, spI, 2, mark);
                            Display->drawParam(    "Kp :", 5, kpI, 2, mark);
                            Display->drawParam(    "Ki :", 6, kiI, 2, mark);
                            Display->drawParam(    "Kd :", 7, kdI, 2, mark);
                            break;

      case MDisplay::GO:    switch (mark)
                            {
                              case 3: Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::STOP);
                                      //Tools->txPowerAuto(spV, spI);
                                      //Tools->txCurrentAdj(spI);// 0x26
          //Tools->txPowerVGo(spV, spI);
                  // Включить (0x24)или отключить (0x21)    // д.б. STOP/GO
        //    (Tools->getState() == Tools->getStatusPidCurrent()) ?
        //                                      Tools->txPowerStop() : Tools->txPowerIGo(spV, spI/2);
                                              //Tools->txPowerIGo(spV, spI/2);
                                              //Tools->txPowerIGo(0, spI/2);
                                              Tools->txPowerMode(spV, spI, MPrj::RI); // 0x22


                                      break;
                              case 4: return new MLoadSpI(Tools);
                              case 5: return new MLoadKpI(Tools);
                              case 6: return new MLoadKiI(Tools);
                              case 7: return new MLoadKdI(Tools);
                              default:;
                            }
                            break;
      case MDisplay::BACK:  return new MStart(Tools);
      case MDisplay::STOP:  Tools->txPowerStop();
                            break;
      default:;
    }

Serial.print("\nStateI=0b"); Serial.print(Tools->getState(), HEX);

    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };  //MAdjPidI


  //========================================================================= MAdjPidV
  /* Состояние выбора параметра */
  MAdjPidV::MAdjPidV(MTools *Tools) : MState(Tools)
  {
    spV = Tools->readNvsShort("device", "spV", sp_u_default);
    spI = Tools->readNvsShort("device", "spI", sp_i_default);
    // Восстановление пользовательских kp, ki, kp
    kpV = Tools->readNvsFloat("device", "kpV", MPrj::kp_v_default);
    kiV = Tools->readNvsFloat("device", "kiV", MPrj::ki_v_default);
    kdV = Tools->readNvsFloat("device", "kdV", MPrj::kd_v_default);
    Tools->txSetPidCoeffI(kpV, kiV, kdV);                           // 0x41 Применить

    mark = 3;
    Display->drawLabel(               "DEVICE", 0);
    Display->drawLabel("Adjusting Setpoint_V:", 1);
    Display->clearLine(                         2);
    Display->drawAdj (              "STOP/GO!", 3,         mark);
    Display->drawParFl(              "Sp, V :", 4, spV, 2, mark);
    Display->drawParam(                 "Kp :", 5, kpV, 2, mark);
    Display->drawParam(                 "Ki :", 6, kiV, 2, mark);
    Display->drawParam(                 "Kd :", 7, kdV, 2, mark);

    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);
  }

  MState *MAdjPidV::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::NEXT:  (mark >= 7) ? mark = 3 : mark++;      // Переместить курсор
                            Display->drawAdj  ("STOP/GO!", 3,         mark);
                            Display->drawParFl( "Sp, V :", 4, spV, 2, mark);
                            Display->drawParam(    "Kp :", 5, kpV, 2, mark);
                            Display->drawParam(    "Ki :", 6, kiV, 2, mark);
                            Display->drawParam(    "Kd :", 7, kdV, 2, mark);
                            break;
      case MDisplay::GO:    switch (mark)
                            {
                              case 3: Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::STOP);
                                      //Tools->txPowerAuto(spV, spI);
                                      //Tools->txPowerVGo(spV, spI);
                                      Tools->txVoltageAdj(spV);       // 0x25
                                      break;
                              case 4: return new MLoadSpV(Tools);
                              case 5: return new MLoadKpV(Tools);
                              case 6: return new MLoadKiV(Tools);
                              case 7: return new MLoadKdV(Tools);
                              default:;
                            }
                            break;
      case MDisplay::BACK:  return new MStart(Tools);
      case MDisplay::STOP:  Tools->txPowerStop();
                            break;
      default:;
    }

    Serial.print("\nStateV=0x"); Serial.print(Tools->getState(), HEX);

    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };  //MAdjPidV











  //========================================================================= MAdjVoltage
  // Состояние: "Выбор объекта коррекции параметров по напряжению".
  MAdjVoltage::MAdjVoltage(MTools *Tools) : MState(Tools)
  {
    mark = 3;
    // Список доступных регулировок
    Display->drawLabel(            "DEVICE", 0);
    Display->drawLabel("Adjusting voltage:", 1);
    Display->clearLine(                      2);
    Display->drawAdj(              "Shift",  3, mark );
    Display->drawAdj(              "Factor", 4, mark );
    Display->drawAdj(              "Smooth", 5, mark );
    Display->clearLine(                      6, 7);     // Очистка строк от и до включительно.
    Board->ledsBlue();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK); // Активировать группу кнопок
  }

  MState *MAdjVoltage::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::NEXT:  (mark >= 5) ? mark = 3 : mark++;        // Переместить курсор
                            Display->drawAdj( "Shift", 3, mark);
                            Display->drawAdj("Factor", 4, mark);
                            Display->drawAdj("Smooth", 5, mark);
                            break;
      case MDisplay::BACK:  return new MStart(Tools); // Вернуться
      case MDisplay::GO:    switch (mark)                           // Перейти
                            {
                              case 3:  return new MShiftV(Tools);
                              case 4:  return new MFactorV(Tools);
                              case 5:  return new MSmoothV(Tools);
                              default:;
                            }
      default:;
    }
    return this;                                          // Ждать выбора
  };  //MAdjVoltage

  //========================================================================= MShiftV
  // Состояние: "Коррекция приборного смещения (сдвига) по напряжению".
  MShiftV::MShiftV(MTools *Tools) : MState(Tools)
  {
    shift = Tools->readNvsShort( "device", "offsetV", fixed );
    // В главное окно выводятся:
    Display->drawLabel("Adjusting voltage shift:", 1);        // полное название параметра (жёлтым)
    Display->drawShort(                 "Shift :", 3, shift); // и его значение из Nvs (целочисленное)
    Display->clearLine(                            4, 5);
    mark = 3;
    Board->ledsBlue();                                  // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MShiftV::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "offsetV", shift); // Сохранить 
                            return new MAdjVoltage(Tools);                    // Там выбрать или завершить
      case MDisplay::UP:    shift = Tools->updnInt(shift, dn, up, +1);
                            Display->drawLabel("Voltage offset changed:", 1);
                            Display->drawShort("Shift :", 3, shift, mark);    // Белым - когда значение изменено
                            Tools->txSetShiftU(shift);                        // 0x36 Применить
                            break;
      case MDisplay::DN:    shift = Tools->updnInt(shift, dn, up, -1);
                            Display->drawLabel("Voltage offset changed:", 1);
                            Display->drawShort("Shift :", 3, shift, mark);
                            Tools->txSetShiftU(shift);                          // 0x36 Применить
                            break;
      default:;
    }
    return this;
  }; //MShiftV

  //========================================================================= MFactorV
  // Состояние: "Коррекция коэффициента преобразования в милливольты".
  MFactorV::MFactorV(MTools *Tools) : MState(Tools)
  {
    factor = Tools->readNvsShort("device", "factorV", fixed);
    // В главное окно выводятся:
    Display->drawLabel("Adjusting voltage factor:", 1);   // полное название параметра (жёлтым)
    Display->drawShort(                 "Factor :", 3, factor);  // и его значение из Nvs
    Display->clearLine(                             4, 5);
    mark = 3;
    Board->ledsBlue();                                  // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MFactorV::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "factorV", factor);
                            return new MAdjVoltage(Tools);        // Там выбрать или завершить
      case MDisplay::UP:    factor = Tools->updnInt(factor, dn, up, +1);
                            Display->drawLabel("Voltage factor changed:", 1);
                            Display->drawShort("Factor :", 3, factor, mark);
                            Tools->txSetFactorU(factor);                      // 0x31 Применить
                            break;
      case MDisplay::DN:    factor = Tools->updnInt(factor, dn, up, -1);
                            Display->drawLabel("Voltage factor changed:", 1);
                            Display->drawShort("Factor :", 3, factor, mark);
                            Tools->txSetFactorU(factor);                      // 0x31 Применить
                            break;
      default:;
    }
    return this;
  }; //MFactorV

  //========================================================================= MSmoothV
  // Состояние: "Коррекция коэффициента фильтрации по напряжению".
  MSmoothV::MSmoothV(MTools *Tools) : MState(Tools)
  {
    smooth = Tools->readNvsShort("device", "smoothV", fixed);
    // В главное окно выводятся:
//    Display->drawLabel(                   "DEVICE", 0);
    Display->drawLabel("Adjusting voltage smooth:", 1);         // полное название параметра
    Display->drawShort(                 "Smooth :", 3, smooth); // и его значение из Nvs
    Display->clearLine(                             4, 5);
    mark = 3;
    Board->ledsBlue();                                  // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MSmoothV::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "smoothV", smooth);
                            return new MAdjVoltage(Tools); // Там выбрать или завершить
      case MDisplay::UP:    smooth = Tools->updnInt(smooth, dn, up, +1);
                            Display->drawLabel("Voltage smooth changed:", 1);
                            Display->drawShort("Smooth :", 3, smooth, mark);
                            Tools->txSetFactorU(smooth);                       // 0x34 Применить
                            break;
      case MDisplay::DN:    smooth = Tools->updnInt(smooth, dn, up, -1);
                            Display->drawLabel("Voltage smooth changed:", 1);
                            Display->drawShort("Smooth :", 3, smooth, mark);
                            Tools->txSetFactorU(smooth);                        // 0x34 Применить
                            break;
      default:;
    }
    return this;
  }; //MSmoothV

  //========================================================================= MAdjCurrent
  // Состояние: "Выбор объекта коррекции параметров по току".
  /*...*/
  MAdjCurrent::MAdjCurrent(MTools *Tools) : MState(Tools)
  {
    // Список доступных регулировок
    Display->drawLabel("DEVICE", 0);
    Display->drawLabel("Adjusting Current:", 1);
    mark = 3;
    Display->clearLine(2);
    Display->drawAdj( "Shift", 3, mark);
    Display->drawAdj("Factor", 4, mark);
    Display->drawAdj("Smooth", 5, mark);
    Display->clearLine(6, 7);
    Board->ledsBlue();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);
  }

  MState *MAdjCurrent::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::NEXT:  (mark >= 5) ? mark = 3 : mark++;      // Переместить курсор (выделение цветом)
                            Display->drawAdj( "Shift", 3, mark);
                            Display->drawAdj("Factor", 4, mark);
                            Display->drawAdj("Smooth", 5, mark);
                            break;
      case MDisplay::GO:    switch (mark)
                            {
                              case 3:  return new MShiftI(Tools);
                              case 4:  return new MFactorI(Tools);
                              case 5:  return new MSmoothI(Tools);
                              default:;
                            }
      case MDisplay::BACK:  return new MStart(Tools); // Перейти к 
      default:;
    }
    return this;
  };

  //========================================================================= MShiftI
  // Состояние: "Коррекция приборного смещения (сдвига) по току".
  MShiftI::MShiftI(MTools *Tools) : MState(Tools)
  {
    shift = Tools->readNvsShort("device", "offsetI", fixed);
    // В главное окно выводятся:
    Display->drawLabel("Adjusting current shift:", 1);      // полное название параметра (жёлтым)
    Display->drawShort(                 "Shift :", 3, shift);           // и его значение из Nvs
    Display->clearLine(                            4, 5);
    mark = 3;
    Board->ledsBlue(); // Синий - что-то меняется
    Display->newBtn( MDisplay::SAVE, MDisplay::UP, MDisplay::DN );
  }

  MState *MShiftI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "offsetI", shift);
                            return new MAdjCurrent(Tools);          // Там выбрать или завершить
      case MDisplay::UP:    shift = Tools->updnInt(shift, dn, up, +1);
                            Display->drawLabel("Current offset changed:", 1);
                            Display->drawShort("Shift :", 3, shift, mark);
                            Tools->txSetShiftI(shift);                        // 0x3E Применить
                            break;
      case MDisplay::DN:    shift = Tools->updnInt(shift, dn, up, -1);
                            Display->drawLabel("Current offset changed:", 1);
                            Display->drawShort("Shift :", 3, shift, mark);
                            Tools->txSetShiftI(shift);                        // 0x3E Применить
                            break;
      default:;
    }
    return this;
  }; //MShiftI

  //========================================================================= MFactorI
  // Состояние: "Коррекция коэффициента преобразования в миллиамперы".
  MFactorI::MFactorI(MTools *Tools) : MState(Tools)
  {
    factor = Tools->readNvsShort("device", "factorI", fixed);
    // В главное окно выводятся:
    Display->drawLabel("Adjusting current factor:", 1); // полное название параметра (жёлтым)
    Display->drawShort(                 "Factor :", 3, factor);          // и его значение из Nvs
    Display->clearLine(                             4, 5);
    mark = 3;
    Board->ledsBlue(); // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MFactorI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "factorI", factor);
                            return new MAdjCurrent(Tools); // Там выбрать или завершить
      case MDisplay::UP:    factor = Tools->updnInt(factor, dn, up, +1);
                            Display->drawLabel("Current factor changed:", 1);
                            Display->drawShort("Factor :", 3, factor, mark);
                            Tools->txSetFactorI(factor);                      // 0x39 Применить
                            break;
      case MDisplay::DN:    factor = Tools->updnInt(factor, dn, up, -1);
                            Display->drawLabel("Current factor changed:", 1);
                            Display->drawShort("Factor :", 3, factor, mark);
                            Tools->txSetFactorI(factor);                      // 0x39 Применить
                            break;
      default:;
    }
    return this;
  }; //MFactorI

  //========================================================================= MSmoothI
  // Состояние: "Коррекция коэффициента фильтрации по току".
  MSmoothI::MSmoothI(MTools *Tools) : MState(Tools)
  {
    smooth = Tools->readNvsShort("device", "smoothI", fixed);
    Display->drawLabel("Adjusting current smooth:", 1); // полное название параметра (жёлтым)
    Display->drawShort(                 "Smooth :", 3, smooth);          // и его значение из Nvs
    Display->clearLine(                             4, 5);
    mark = 3;
    Board->ledsBlue(); // Синий - что-то меняется
    Display->newBtn( MDisplay::SAVE, MDisplay::UP, MDisplay::DN );
  }

  MState *MSmoothI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "smoothI", smooth);
                            return new MAdjCurrent(Tools); // Там выбрать или завершить
      case MDisplay::UP:    smooth = Tools->updnInt(smooth, dn, up, +1);
                            Display->drawLabel("Current smooth changed:", 1);
                            Display->drawShort("Smooth :", 3, smooth, mark);
                            Tools->txSetSmoothI(smooth);                      // 0x3C Применить
                            break;
      case MDisplay::DN:    smooth = Tools->updnInt(smooth, dn, up, -1);
                            Display->drawLabel("Current smooth changed:", 1);
                            Display->drawShort("Smooth :", 3, smooth, mark);
                            Tools->txSetSmoothI(smooth);                      // 0x3C Применить
                            break;
      default:;
    }
    return this;
  }; // MSmoothI





  MLoadSpV::MLoadSpV(MTools *Tools) : MState(Tools)
  {
    Tools->writeNvsShort("device", "spV", spV);
    Tools->txSetPidCoeffV(kpV, kiV, kdV);           // 0x41 Применить
    mark = 3;
    Display->drawLabel("Adjusting Setpoint V:", 1);
    Display->drawParFl(              "Sp, V :", 3, spV, 2, mark);
    Display->clearLine(                         4, 7);
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadSpV::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "spV", spV);
                            // Включить (0x22)или отключить (0x21)    // д.б. STOP/GO
        (Tools->getState() == Tools->getStatusPidVoltage()) ?
                                       Tools->txPowerStop() : Tools->txPowerAuto(spV, spI); // 0x20 Применить
                            return new MAdjPidV(Tools);
      case MDisplay::UP:    spV = Tools->updnInt(spV, dn, up, delta);
                            Display->drawLabel("Sp V changed:", 1);
                            Display->drawParFl(      "Sp, V :", 3, spV, 2, mark);
                            //Tools->txPowerMode(sp, sp_i_default, MCommands::RU); // 0x22 Применить
                            //Tools->txChargeVGo(spV); // 0x22 Применить
                            break;
      case MDisplay::DN:    spV = Tools->updnInt(spV, dn, up, -delta);
                            Display->drawLabel("Sp V changed:", 1);
                            Display->drawParFl(       "Sp V :", 3, spV, 2, mark);
                            //Tools->txPowerMode(sp, sp_i_default, MCommands::RU); // 0x22 Применить
                            //Tools->txChargeVGo(sp); // 0x22 Применить;
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };


  //========== MLoadKpV, ввод параметра KP PID-регулятора напряжения =========
  MLoadKpV::MLoadKpV(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel(         "DEVICE", 0);
    Display->drawLabel("Adjusting kp V:", 1);
    Display->drawParam(           "Kp :", 3, kpV, 2);
    Display->clearLine(                   4, 7);
    mark = 3;
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKpV::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("device", "kpV", kpV);
                            Tools->txSetPidCoeffV(kpV, kiV, kdV);       // 0x41 Применить
                            return new MAdjPidV(Tools);
      case MDisplay::UP:    kpV = Tools->updnFloat(kpV, dn, up, +0.01);
                            Display->drawLabel("Voltage kp changed:", 1);
                            Display->drawParam(             "Kp V :", 3, kpV, 2, mark);
                            Tools->txSetPidCoeffV(kpV, kiV, kdV);       // 0x41 Применить
                            break;
      case MDisplay::DN:    kpV = Tools->updnFloat( kpV, dn, up, -0.01);
                            Display->drawLabel("Voltage kp changed:", 1);
                            Display->drawParam(             "Kp V :", 3, kpV, 2, mark);
                            Tools->txSetPidCoeffV(kpV, kiV, kdV);       // 0x41 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };  //MLoadKpV

  //========== MLoadKiV, ввод параметра KI PID-регулятора напряжения =========
  MLoadKiV::MLoadKiV(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel(         "DEVICE", 0);
    Display->drawLabel("Adjusting ki V:", 1);
    Display->drawParam(           "Ki :", 3, kiV, 2);
    Display->clearLine(                   4, 7);
    mark = 3;
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKiV::fsm()
  {
    switch (Display->getKey())
      {
      case MDisplay::SAVE:      Tools->writeNvsFloat("device", "kiV", kiV);
                                Tools->txSetPidCoeffV(kpV, kiV, kdV);             // 0x41 Применить
                                return new MAdjPidV(Tools);
      case MDisplay::UP:
        kiV = Tools->updnFloat(kiV, dn, up, +0.01);
        Display->drawLabel("Voltage ki changed:", 1);
        Display->drawParam(             "Ki V :", 3, kiV, 2, mark);
        Tools->txSetPidCoeffV(kpV, kiV, kdV);             // 0x41 Применить
        break;
      case MDisplay::DN:
        kiV = Tools->updnFloat(kiV, dn, up, -0.01);
        Display->drawLabel("Voltage ki changed:", 1);
        Display->drawParam(             "Ki V :", 3, kiV, 2, mark);
        Tools->txSetPidCoeffV(kpV, kiV, kdV);             // 0x41 Применить
        break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };  //MLoadKiV

  //========== MLoadKdV, ввод параметра KD PID-регулятора напряжения =========
  MLoadKdV::MLoadKdV(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel("Adjusting kd V:", 1);
    Display->drawParam(           "Kd :", 3, kdV, 2);
    Display->clearLine(                   4, 7);
    mark = 3;
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKdV::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("device", "kdV", kdV);
                            Tools->txSetPidCoeffV(kpV, kiV, kdV);       // 0x41 Применить
                            return new MAdjPidV(Tools);
      case MDisplay::UP:    kdV = Tools->updnFloat(kdV, dn, up, +0.01);
                            Display->drawLabel("Voltage kd changed:", 1);
                            Display->drawParam(             "Kd V :", 3, kdV, 2, mark);
                            Tools->txSetPidCoeffV(kpV, kiV, kdV);               // 0x41 Применить
                            break;
      case MDisplay::DN:    kdV = Tools->updnFloat(kdV, dn, up, -0.01);
                            Display->drawLabel("Voltage kd changed:", 1);
                            Display->drawParam(             "Kd V :", 3, kdV, 2, mark);
                            Tools->txSetPidCoeffV(kpV, kiV, kdV);               // 0x41 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };  //MLoadKdV



  MLoadSpI::MLoadSpI(MTools *Tools) : MState(Tools)
  {
    Tools->writeNvsShort("device", "spI", spI);
    Tools->txSetPidCoeffV(kpI, kiI, kdI);           // 0x41 Применить
    mark = 3;
    Display->drawLabel("Adjusting Setpoint I:", 1);
    Display->drawParFl(              "Sp, A :", 3, spI, 2, mark);
    Display->clearLine(                         4, 7);
    Display->newBtn(MDisplay::PAUSE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadSpI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::PAUSE: Tools->writeNvsShort("device", "spI", spI);
                            // Включить (0x20)или отключить (0x21)
                     (Tools->getState() == Tools->getStatusPidCurrent()) ?
                                                    Tools->txPowerStop() : Tools->txPowerAuto(spV, spI);
                            return new MAdjPidI(Tools);
      case MDisplay::UP:    spI = Tools->updnInt(spI, dn, up, delta);
                            Display->drawLabel("Sp I changed:", 1);
                            Display->drawParFl(      "Sp, A :", 3, spI, 2, mark);
                            //Tools->txChargeIGo(sp);                         // 0x22 Применить
                            break;
      case MDisplay::DN:    spI = Tools->updnInt(spI, dn, up, -delta);
                            Display->drawLabel("Sp I changed:", 1);
                            Display->drawParFl(       "Sp A :", 3, spI, 2, mark);
                            //Tools->txChargeIGo(sp);                         // 0x22 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };

  //============= MLoadKpI, ввод параметра KP PID-регулятора тока ============

  MLoadKpI::MLoadKpI(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel("Adjusting kp I:", 1);
    Display->drawParam(           "Kp :", 3, kpI, 2);
    Display->clearLine(                   4, 7);
    mark = 3;
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKpI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("device", "kpI", kpI);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI);         // 0x41 Применить
                            return new MAdjPidV(Tools);
      case MDisplay::UP:    kpI = Tools->updnFloat(kpI, dn, up, +0.01);
                            Display->drawLabel("Current kp changed:", 1);
                            Display->drawParam("Kp I :", 3, kpI, 2, mark);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            break;
      case MDisplay::DN:    kpI = Tools->updnFloat(kpI, dn, up, -0.01);
                            Display->drawLabel("DEVICE", 0);
                            Display->drawLabel("Current kp changed:", 1);
                            Display->drawParam("Kp I :", 3, kpI, 2, mark);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };

  //============= MLoadKiI, ввод параметра KI PID-регулятора тока ============

  MLoadKiI::MLoadKiI(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel("Adjusting ki I:", 1);
    Display->drawParam(           "Ki :", 3, kiI, 2);
    Display->clearLine(                   4, 7);
    mark = 3;
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKiI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("device", "kiI", kiI);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            return new MAdjPidV(Tools);
      case MDisplay::UP:    kiI = Tools->updnFloat(kiI, dn, up, +0.01);
                            Display->drawLabel("Current ki changed:", 1);
                            Display->drawParam(             "Ki I :", 3, kiI, 2, mark);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            break;
      case MDisplay::DN:    kiI = Tools->updnFloat(kiI, dn, up, -0.01);
                            Display->drawLabel("Current ki changed:", 1);
                            Display->drawParam(             "Ki I :", 3, kiI, 2, mark);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            break;
      default:;
    }
                      (Tools->getState() == Tools->getStatusPidCurrent()) ? 
                                                      Board->ledsGreen() : Board->ledsRed();
    return this;
  };

  //============= MLoadKdI, ввод параметра KD PID-регулятора тока ============

  MLoadKdI::MLoadKdI(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel("Adjusting kd I:", 1);
    Display->drawParam(           "Kd :", 3, kdI, 2);
    Display->clearLine(                   4, 7);
    mark = 3;
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKdI::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("device", "kdI", kdI);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            return new MAdjPidV(Tools);
      case MDisplay::UP:    kdI = Tools->updnFloat(kdI, dn, up, +0.01);
                            Display->drawLabel("Current kd changed:", 1);
                            Display->drawParam(             "Kd I :", 3, kdI, 2, mark);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            break;
      case MDisplay::DN:    kdI = Tools->updnFloat(kdI, dn, up, -0.01);
                            Display->drawLabel("Current kd changed:", 1);
                            Display->drawParam(             "Kd I :", 3, kdI, 2, mark);
                            Tools->txSetPidCoeffI(kpI, kiI, kdI); // 0x41 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    return this;
  };



  //========================================================================= MAdjPidD
  //  
  MAdjPidD::MAdjPidD(MTools *Tools) : MState(Tools)
  {
    // Восстановление пользовательских sp, kp, ki, kp
    spD  = Tools->readNvsShort("device", "spD", sp_d_default);
    kpD = Tools->readNvsFloat("device", "kpD", MPrj::kp_d_default);
    kiD = Tools->readNvsFloat("device", "kiD", MPrj::ki_d_default);
    kdD = Tools->readNvsFloat("device", "kdD", MPrj::kd_d_default);
    mark = 3;
    Display->drawLabel(           "DEVICE", 0);
    Display->drawLabel( "Adjusting PID_D:", 1);
    Display->clearLine(                     2);
    Display->drawParFl(    "Setpoint, A :", 3, spD,  2, mark);
    Display->drawParam(             "Kp :", 4, kpD, 2, mark);
    Display->drawParam(             "Ki :", 5, kiD, 2, mark);
    Display->drawParam(             "Kd :", 6, kdD, 2, mark);
    Display->clearLine(                     7);
  //  Board->ledsCyan();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);
  }
  MState *MAdjPidD::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::NEXT:    (mark >= 6) ? mark = 3 : mark++;
                              Display->drawParFl("setpoint, A :", 3,  spD, 2, mark);
                              Display->drawParam(         "Kp :", 4, kpD, 2, mark);
                              Display->drawParam(         "Ki :", 5, kiD, 2, mark);
                              Display->drawParam(         "Kd :", 6, kdD, 2, mark);
                              break;
      case MDisplay::GO:      switch (mark)
                              {
                                case 3:  return new MLoadSp(Tools);
                                case 4:  return new MLoadKpD(Tools);
                                case 5:  return new MLoadKiD(Tools);
                                case 6:  return new MLoadKdD(Tools);
                                default:;
                              }
      case MDisplay::BACK:    Tools->txPowerStop();
                              return new MStart(Tools);
      default:;
    }
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
    return this;
  };


//========== MLoadSp, ввод порога PID-регулятора ========================= 

  MLoadSp::MLoadSp(MTools *Tools) : MState(Tools)
  {
    Tools->writeNvsShort("device", "spD", spD);
    Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
    mark = 3;
    Display->drawLabel(               "DEVICE", 0);
    Display->drawLabel("Adjusting Setpoint D:", 1);
    Display->clearLine(                         2);
    Display->drawParFl(        "Setpoint, A :", 3, spD, 2, mark);
    Display->clearLine(                         4, 7);
    Display->newBtn(MDisplay::PAUSE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadSp::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::PAUSE:  Tools->writeNvsShort("device", "spD", spD);
                            // Включить (0x24)или отключить (0x21)    // д.б. STOP/GO
            (Tools->getState() == Tools->getStatusPidDiscurrent()) ?
                                              Tools->txPowerStop() : Tools->txDischargeGo(spD);
                            return new MAdjPidD(Tools);
      case MDisplay::UP:    spD = Tools->updnInt(spD, dn, up, delta);
                            Display->drawLabel("Setpoint D changed:", 1);
                            Display->drawParFl(      "Setpoint, A :", 3, spD, 2, mark);
                            Tools->txDischargeGo(spD);                       // 0x24 Применить
                            break;
      case MDisplay::DN:    spD = Tools->updnInt(spD, dn, up, -delta);
                            Display->drawLabel("Setpoint D changed:", 1);
                            Display->drawParFl(      "Setpoint, A :", 3, spD, 2, mark);
                            Tools->txDischargeGo(spD);                       // 0x24 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
    return this;
  };

  //============= MLoadKpD, ввод параметра KP PID-регулятора тока разряда ============

  MLoadKpD::MLoadKpD(MTools *Tools) : MState(Tools)
  {
    mark = 3;
    Display->drawLabel(         "DEVICE", 0);
    Display->drawLabel("Adjusting kp D:", 1);
    Display->clearLine(                   2);
    Display->drawParam(           "Kp :", 3, kpD, 2, mark);
    Display->clearLine(                   4, 7);
    //Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
    Display->newBtn(MDisplay::PAUSE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKpD::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::PAUSE: Tools->writeNvsFloat("device", "kpD", kpD);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
                            return new MAdjPidD(Tools);
      case MDisplay::UP:    kpD = Tools->updnFloat(kpD, dn, up, delta);
                            Display->drawLabel("kp changed:", 1);
                            Display->drawParam(     "Kp D :", 3, kpD, 2, mark);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
                            break;
      case MDisplay::DN:    kpD = Tools->updnFloat(kpD, dn, up, -delta);
  //                        Display->drawLabel(     "DEVICE", 0);
                            Display->drawLabel("kp changed:", 1);
                            Display->drawParam(     "Kp D :", 3, kpD, 2, mark);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
    return this;
  };

  //============= MLoadKiD, ввод параметра KI PID-регулятора тока разряда ============

  MLoadKiD::MLoadKiD(MTools *Tools) : MState(Tools)
  {
    mark = 3;
    Display->drawLabel(         "DEVICE", 0);
    Display->drawLabel("Adjusting ki D:", 1);
    Display->drawParam(           "Ki :", 3, kiD, 2, mark);
    Display->clearLine(                   4, 7);
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKiD::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("device", "kiD", kiD);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
                            return new MAdjPidD(Tools);
      case MDisplay::UP:    kiD = Tools->updnFloat(kiD, dn, up, +0.01);
                            Display->drawLabel("ki changed:", 1);
                            Display->drawParam(     "Ki D :", 3, kiD, 2, mark);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
                            break;
      case MDisplay::DN:    kiD = Tools->updnFloat(kiD, dn, up, -0.01);
                            Display->drawLabel("ki changed:", 1);
                            Display->drawParam(     "Ki D :", 3, kiD, 2, mark);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
    return this;
  };

  //============= MLoadKdD, ввод параметра KD PID-регулятора тока разряда ============

  MLoadKdD::MLoadKdD(MTools *Tools) : MState(Tools)
  {
    mark = 3;
//    Display->drawLabel(         "DEVICE", 0);
    Display->drawLabel("Adjusting kd D:", 1);
    Display->drawParam(           "Kd :", 3, kdD, 2, mark);
    Display->clearLine(                   4, 7);
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MLoadKdD::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsFloat("device", "kdD", kdD);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);           // 0x41 Применить
                            return new MAdjPidD(Tools);
      case MDisplay::UP:    kdD = Tools->updnFloat(kdD, dn, up, +0.01);
                            Display->drawLabel("kd changed:", 1);
                            Display->drawParam(     "Kd D :", 3, kdD, 2, mark);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);               // 0x41 Применить
                            break;
      case MDisplay::DN:    kdD = Tools->updnFloat(kdD, dn, up, -0.01);
                            Display->drawLabel("kd changed:", 1);
                            Display->drawParam(     "Kd D :", 3, kdD, 2, mark);
                            Tools->txSetPidCoeffD(kpD, kiD, kdD);               // 0x41 Применить
                            break;
      default:;
    }
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
    return this;
  };


  //========================================================================= MPidFrequency
  // Состояние: "Коррекция частоты ПИД-регулятора".
  MPidFrequency::MPidFrequency(MTools *Tools) : MState(Tools)
  {
    i = Tools->readNvsShort("device", "freq", fixed);
    if (i <= dn) i = dn;
    if (i >= up) i = up;
#ifdef PRINTDEVICE
    Serial.print("\nNVS_freq=0x");
    Serial.print(i, HEX);
#endif
    // В главное окно выводятся:
    Display->drawLabel(                  "DEVICE", 0);
    Display->drawLabel("Adjusting PID Frequency:", 1);
    Display->clearLine(                            2);
    Display->drawShort(             "Frequency :", 3, freq[i]);
    Display->clearLine(                            4, 7);

    mark = 3;
    Board->ledsBlue();

    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }
  MState *MPidFrequency::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "freq", i);
                            Tools->txSetPidFrequency(freq[i]);          // 0x4A Применить
                            return new MStart(Tools);
      case MDisplay::UP:    i = Tools->updnInt(i, dn, up, +1);
                            Display->drawLabel("DEVICE", 0);
                            Display->drawLabel("PID Frequency changed:", 1);
                            Display->drawShort("Frequency :", 3, freq[i], mark); 
                            // Белым - когда значение изменено
                            Tools->txSetPidFrequency(freq[i]);           // 0x4A Применить
                            break;
      case MDisplay::DN:    i = Tools->updnInt(i, dn, up, -1);
  //                        Display->drawLabel(                "DEVICE", 0);
                            Display->drawLabel("PID Frequency changed:", 1);
                            Display->drawShort(       "Frequency, Hz :", 3, freq[i], mark);
                            Tools->txSetPidFrequency(freq[i]); // 0x4A Применить
                            break;
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 3);
    Tools->showAmp(Tools->getRealCurrent(), 3);

    return this;
  }; // MPidFrequency

  //========================================================================= MStop
  // Состояние: "Завершение режима DEVICE",
  MStop::MStop(MTools *Tools) : MState(Tools)
  {
    // Активировать группу кнопок для завершения режима
    Display->newBtn( MDisplay::START ); // очистить список и добавить кнопку
    Display->addBtn( MDisplay::EXIT );  // Добавить кнопку
#ifdef PRINTDEVICE
    Serial.println("\nDEVICE stopped");
#endif
    Display->drawLabel("DEVICE", 0);
    Display->drawLabel("Calibrations completed", 1);
    Display->drawLabel("Statistics:", 2);
    // В строки 3 ... 7 можно добавить результаты калибровки
    Board->ledsRed(); // Подтверждение
  }

  MState *MStop::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::START:     return new MStart(Tools); // Возобновить
      case MDisplay::EXIT:      Display->newModeWin();
                                Display->newMainWin();   // Очистить главное окно
                                return new MExit(Tools); //
      default:;
    }
    Tools->showVolt( Tools->getRealVoltage(), 3 );
    Tools->showAmp( Tools->getRealCurrent(), 3 );
    return this;
  }; // MStop

  //===================================================================================== MExit
  // Состояние: "Выход из режима в меню диспетчера".
  MExit::MExit(MTools *Tools) : MState(Tools)
  {
    Tools->aboutMode(MDispatcher::DEVICE);
    // Активировать группу кнопок меню диспетчера
    Display->newBtn(MDisplay::GO, MDisplay::UP, MDisplay::DN);

#ifdef PRINTDEVICE
    Serial.println("\nDEVICE completed");
#endif
    Board->ledsOff();
  }

  MState *MExit::fsm() { return nullptr; };   // MExit в меню

  //========================================================================= MClear
  /* Очистка всех настроек (ключей) в открытом пространстве имен (это не удаляет 
    само пространство имен) */
  MClear::MClear(MTools *Tools) : MState(Tools)
  {
    mark = 2;
    Display->drawLabel("Mode DEVICE",     0);
    Display->drawLabel("NVS: Clear Keys", 1);
    Display->drawAdj(           "device", 2, mark);
    Display->drawAdj(          "options", 3, mark);
    Display->drawAdj(             "cccv", 4, mark);
    Display->drawAdj(         "template", 5, mark);
    Display->clearLine(                   6, 7);
    Board->ledsCyan();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);
  }

  MState *MClear::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::NEXT:  (mark >= 5) ? mark = 2 : mark++;
                            Display->drawAdj(   "device", 2, mark);
                            Display->drawAdj(  "options", 3, mark);
                            Display->drawAdj(     "cccv", 4, mark);
                            Display->drawAdj("discharge", 5, mark);
                            Display->drawAdj( "template", 6, mark);
                            break;
      case MDisplay::BACK:  return new MStart(Tools); // Вернуться
      case MDisplay::GO:    switch (mark)
                            {
                              case 2:   return new MClrDevKeys(Tools);
                              case 3:   return new MClrOptKeys(Tools);
                              case 4:   return new MClrCCCVKeys(Tools);
                              case 5:   return new MClrDisKeys(Tools);
                              case 6:   return new MClrTplKeys(Tools);
                              default:;
                            }
      default:;
    }
    return this;
  }; // MClear

  //========================================================================= MClrDevKeys

  MClrDevKeys::MClrDevKeys(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel(            "Mode DEVICE", 0);
    Display->clearLine(                           1, 2);
    Display->drawLabel( "Clear All Device Keys?", 3);
    Display->clearLine(                           4, 7);
    // Активировать группу кнопок
    Display->newBtn(MDisplay::GO, MDisplay::BACK);
  }

  MState *MClrDevKeys::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::GO:    Tools->clearAllKeys("device");  // в открытом пространстве имен
      case MDisplay::BACK:  return new MClear(Tools);
      default:;
    }
    return this;
  };  //MClrDevKeys

  //========================================================================= MClrOptKeys

  MClrOptKeys::MClrOptKeys(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel(             "Mode DEVICE", 0);
    Display->clearLine(                            1, 2);
    Display->drawLabel( "Clear All Options Keys?", 3);
    Display->clearLine(                            4, 7);

    // Активировать группу кнопок
    Display->newBtn(MDisplay::GO);   // Подтверить
    Display->addBtn(MDisplay::BACK); // Отказаться
  }

  MState *MClrOptKeys::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::GO:    Tools->clearAllKeys("options");  // в открытом пространстве имен
      case MDisplay::BACK:  return new MClear(Tools);
      default:;
    }
    return this;
  };  //MClrOptKeys

  //========================================================================= MClrCCCVKeys

  MClrCCCVKeys::MClrCCCVKeys(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel("Mode DEVICE", 0);
    Display->clearLine(1, 2);
    Display->drawLabel("Clear All CCCV Keys?", 3);
    Display->clearLine(5, 7);

    // Активировать группу кнопок
    Display->newBtn(MDisplay::GO);   // Подтверить
    Display->addBtn(MDisplay::BACK); // Отказаться
  }

  MState *MClrCCCVKeys::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::GO:    Tools->clearAllKeys("cccv");  // в открытом пространстве имен
      case MDisplay::BACK:  return new MClear(Tools);
      default:;
    }
    return this;
  };  //MClrCCCVKeys

  //========================================================================= MClrDisKeys

  MClrDisKeys::MClrDisKeys(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel(           "Mode DISCHARGE", 0);
    Display->clearLine(                             1, 2);
    Display->drawLabel("Clear All DISCHARGE Keys?", 3);
    Display->clearLine(                             5, 7);

    // Активировать группу кнопок
    Display->newBtn(MDisplay::GO);   // Подтверить
    Display->addBtn(MDisplay::BACK); // Отказаться
  }

  MState *MClrDisKeys::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::GO:    Tools->clearAllKeys("discharge");  // в открытом пространстве имен
      case MDisplay::BACK:  return new MClear(Tools);
      default:;
    }
    return this;
  };  //MClrDisKeys


  //========================================================================= MClrTplKeys

  MClrTplKeys::MClrTplKeys(MTools *Tools) : MState(Tools)
  {
    Display->drawLabel("Mode DEVICE", 0);
    Display->clearLine(1, 2);
    Display->drawLabel("Clear All Template Keys?", 3);
    Display->clearLine(4, 7);

    // Активировать группу кнопок
    Display->newBtn(MDisplay::GO);   // Подтверить
    Display->addBtn(MDisplay::BACK); // Отказаться
  }

  MState *MClrTplKeys::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::GO:    Tools->clearAllKeys("template");  // в открытом пространстве имен
      case MDisplay::BACK:  return new MClear(Tools);
      default:;
    }
    return this;
  };  //MClrTplKeys

 //========================================================================= MMultXY
  /* Коррекция калибровки тачскрина. Файл TouchCalData2 должен быть 
    предварительно создан в FS */
  MMultXY::MMultXY(MTools *Tools) : MState(Tools)
  {
    mark = 2;
    Display->drawLabel("Mode DEVICE",     0);
    Display->drawLabel("Touch calibration", 1);
    Display->drawAdj("Touch X", 2, mark);
    Display->drawAdj("Touch Y", 3, mark);
    Display->clearLine(4, 7);
    Board->ledsCyan();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::BACK);
  }

  MState *MMultXY::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::NEXT:  (mark >= 3) ? mark = 2 : mark++;
                            Display->drawAdj( "Touch X", 2, mark);
                            Display->drawAdj( "Touch Y", 3, mark);
                            break;
      case MDisplay::BACK:  return new MStart(Tools); // Вернуться
      case MDisplay::GO:    switch (mark)
                            {
                              case 2:   return new MMultX(Tools);
                              case 3:   return new MMultY(Tools);
                              default:;
                            }
      default:;
    }
    return this;
  };

  //========================================================================= MMultX

  MMultX::MMultX(MTools *Tools) : MState(Tools)
  {
    touchX = Tools->readNvsShort("device", "touchx", fixed);
    Display->drawLabel("Mode DEVICE", 0);
    Display->clearLine(1, 2);
    Display->drawLabel("Touch X?", 3);
    Display->clearLine(4, 7);
    mark = 3;
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MMultX::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "touchx", touchX);
                            return new MMultXY(Tools); // Вернуться
      case MDisplay::UP:    touchX = Tools->updnInt( touchX, dn, up, -1);
                            Display->drawLabel("DEVICE", 0);
                            Display->drawLabel("X-factor changed:", 1);
                            Display->drawShort("X-factor :", 3, touchX);
                            break;
      case MDisplay::DN:    touchX = Tools->updnInt( touchX, dn, up, +1);
                            Display->drawLabel("DEVICE", 0);
                            Display->drawLabel("X-factor changed:", 1);
                            Display->drawShort("X-factor :", 3, touchX);
                            break;
      default:;
    }
    return this;
  };  //MMultX

  //========================================================================= MMultY

  MMultY::MMultY(MTools *Tools) : MState(Tools)
  {
    touchY = Tools->readNvsShort("device", "touchy", fixed);
    Display->drawLabel("Mode DEVICE", 0);
    Display->clearLine(1, 2);
    Display->drawLabel("Touch Y?", 3);
    Display->clearLine(4, 7);

    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);
  }

  MState *MMultY::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("device", "touchy", touchY);
                            return new MMultXY(Tools); // Вернуться
      case MDisplay::UP:    touchY = Tools->updnInt(touchY, dn, up, +1);
                            Display->drawLabel("DEVICE", 0);
                            Display->drawLabel("Y-factor changed:", 1);
                            Display->drawShort("Y-factor :", 3, touchY);
                            break;
      case MDisplay::DN:    touchY = Tools->updnInt(touchY, dn, up, -1);
                            Display->drawLabel("DEVICE", 0);
                            Display->drawLabel("Y-factor changed:", 1);
                            Display->drawShort("Y-factor :", 3, touchY);
                            break;
      default:;
    }
    return this;
  };  //MMultY

};
