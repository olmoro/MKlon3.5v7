/*
  Файл: optionsfsm.cpp
                          Как это работает.
  ВВод и коррекция пользовательских параметров реализованы в виде конечного автомата, 
инициализация которого производится посредством выбора "OPTIONS" в меню диспетчера.
  Начальное состояние MStart в свою очередь предоставляет оператору в виде подсказок 
назначение сенсорных кнопок.

  Вариант 202300919
*/

#include "modes/optionsfsm.h"
#include "mdispatcher.h"
#include "project_config.h"
#include "mtools.h"
#include "mcmd.h"
#include "board/mboard.h"
#include "display/mdisplay.h"
#include <Arduino.h>

// #include <iostream>
// #include <string>
// using namespace std;

namespace MOption
{
  short postpone  = MPrj::postpone_fixed;
  short timeout   = MPrj::timeout_fixed;
  short nominalV  = MPrj::nominal_v_fixed;
  short capacity  = MPrj::capacity_fixed;
  short tec       = MPrj::tec_fixed;

  short v_nom[] = {2100,2100,2100,2100,2100,3650,3200,2400,1250}; // напряжение ячейки, мВ
  static short v_max[] = {2450,2430,2410,2430,2350,4100,3300,2700,1600}; // напряжение заряда ячейки, мВ
  short v_min[] = {1930,1930,1930,1930,1930,2500,2800,1600,1000}; // напряжение разряда ячейки, мВ
  short i_max[] = {  10,  10,  10,  10,  10, 100, 100, 100, 100}; // ток заряда в %% от емкости

  static constexpr short maxVDefault = 555u;

    // Вычисленные по результатам ввода параметры заряда
  short maxV  = 0u;  // 
  short maxI  = 0u;  // 
  short minV  = 0u;  // 
  short minI  = 0u;  // 

  static int8_t line = -1;

  //========================================================================= MStart
  // Состояние "Старт", инициализация выбранного режима работы.
  MStart::MStart(MTools * Tools) : MState(Tools)
  {
    postpone = Tools->readNvsShort("options", "postpone", MPrj::postpone_fixed);
    timeout  = Tools->readNvsShort("options", "timeout",  MPrj::timeout_fixed);
    nominalV = Tools->readNvsShort("options", "nominalV", MPrj::nominal_v_fixed);
    capacity = Tools->readNvsShort("options", "capacity", MPrj::capacity_fixed);
    tec      = Tools->readNvsShort("options", "akb",      MPrj::tec_fixed);

    line = 1;
    Display->drawLabel("Mode OPTIONS loaded:", 0); // Режим, если кто-то забыл, напоминание
    Display->drawShort("Postpone, hr :", 1, postpone,    line); // От этой строки ниже можно выбирать
    Display->drawShort("Timeout, hr :",  2, timeout,     line); // 
    Display->drawParFl("NominalV, V :",  3, nominalV, 1, line); // 
    Display->drawShort("Capacity, ah :", 4, capacity,    line); // 
    Display->drawLabel(TypeAkb[tec],     5);                    // Выбранная технология
    Display->drawLabel("Apply!",         6);                    // Вычисление параметров заряда
    Display->clearLine(7);
    Board->ledsOn();                // Подтверждение входа в режим белым свечением
      /* Активировать группу кнопок для перехода к выбранному параметру,
        выбору или выходу из режима "OPTIONS" */
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::STOP);
  }

  MState * MStart::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::STOP:                  return new MApply(Tools);   // Прервано оператором   
      case MDisplay::NEXT:            (line >= 6) ? line = 1 : line++;  // Вниз по кругу
        Display->drawShort("Postpone, hr :", 1, postpone,      line);   // "Огласите весь список, пожлста"
        Display->drawShort("Timeout, hr :",  2, timeout,       line);   // 
        Display->drawParFl("NominalV, V :",  3, nominalV, 1, line);   // 
        Display->drawShort("Capacity, ah :", 4, capacity,      line);   // 
        Display->drawAdj  (TypeAkb[tec],     5,                line);   //
        Display->drawAdj  ("Apply!",         6,                line);   // Вычисление параметров заряда
        break;
      case MDisplay::GO:
        switch (line)                               // Так нагляднее
        {
          case 1:   return new MSetPostpone(Tools);     // 
          case 2:   return new MTimeout(Tools);         // 
          case 3:   return new MNominalV(Tools);        // 
          case 4:   return new MCapacity(Tools);        // 
          case 5:   return new MAkb(Tools);             // По технологии
          case 6:   return new MApply(Tools);           // Вычислить
          default:;
        }
      default:;
    }
    Tools->showVolt(Tools->getRealVoltage(), 3);
    Tools->showAmp (Tools->getRealCurrent(), 3);
    return this;
  };  //MStart

  //========================================================================= MSetPostpone
  // Состояние "Ввод времени отложенного старта, час"
  MSetPostpone::MSetPostpone(MTools * Tools) : MState(Tools) 
  {
      // В главное окно выводятся:
    Display->drawLabel("OPTIONS", 0);                     // режим,
    Display->drawLabel("Adjusting postpone", 1);          // полное название параметра (жёлтым)
    Display->clearLine(2);
    line = 3;                 // В строке 3 будет имя, размерность и значение параметра из Nvs
    Display->drawShort("Postpone, hr :", line, postpone);
    Display->clearLine(4,7);                              // Остальные строки очищаются
    Board->ledsBlue();                                    // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN);   // Активировать группу кнопок
  }

  MState * MSetPostpone::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::UP:   postpone = Tools->updnInt(postpone, dn, up, +1);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("Postpone changed:", 1); 
        Display->drawShort("Postpone, hr :", line, postpone, line); // Белым - когда значение изменено
        Tools->setPostpone(postpone);
        break;
      case MDisplay::DN:    postpone = Tools->updnInt(postpone, dn, up, -1);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("Postpone changed:", 1); 
        Display->drawShort("Postpone, hr :", line, postpone, line); 
        Tools->setPostpone(postpone);
        break;
      case MDisplay::SAVE:  Tools->writeNvsShort("options", "postpone", postpone);
                            return new MStart(Tools);     // Там выбрать или завершить
      default:;
    }
    Display->showDuration(postpone * 3600, MDisplay::SEC);  // Проверка
    return this;
  }; //MSetPostpone

  //========================================================================= MTimeout
  // Состояние "Ввод максимального времени заряда, час" 
  MTimeout::MTimeout(MTools * Tools) : MState(Tools) 
  {
      // В главное окно выводятся:
    Display->drawLabel("OPTIONS", 0);               // режим,
    Display->drawLabel("Adjusting timeout", 1);     // полное название параметра (жёлтым)
    Display->clearLine(2);
    line = 3;      // В строке 3 имя, размерность и значение параметра из Nvs
    Display->drawShort("Timeout, hr :", line, timeout);
    Display->clearLine(4 , 7);                      // Остальные строки очищаются
    Board->ledsBlue();                              // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN); // Активировать группу кнопок
  }

  MState * MTimeout::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE: Tools->writeNvsShort("options", "timeout", timeout);
                           return new MStart(Tools);     // Там выбрать или завершить
      case MDisplay::UP:   timeout = Tools->updnInt(timeout, dn, up, +1);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("Timeout changed:", 1); 
        Display->drawShort("Timeout, hr :", line, timeout, line);    // Белым - когда значение изменено
        Tools->setTimeCounter(timeout);
        break;
      case MDisplay::DN:   timeout = Tools->updnInt(timeout, dn, up, -1);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("Timeout changed:", 1); 
        Display->drawShort("Timeout, hr :", line, timeout, line); 
        Tools->setTimeCounter(timeout);
        break;
      default:;
    }
    Display->showDuration(timeout * 3600, MDisplay::SEC);   // Обновить
    return this;
  };  //MTimeout

  /*========================================================================= MNominalV
    Состояние "Выбор количества банок" в заряжаемом аккумуляторе для расчета
    номинального напряжения (частный случай). */
  MNominalV::MNominalV(MTools * Tools) : MState(Tools)
  {
      // В главное окно выводятся:
    Display->drawLabel("OPTIONS", 0);                 // режим,
    Display->drawLabel("Adjusting nominalV", 1);      // полное название параметра (жёлтым)
    Display->clearLine(2);
    line = 3;      // В строке 3 имя, размерность и значение параметра из Nvs
    Display->drawParFl("NominalV, V :", line, nominalV, 1);
    Display->clearLine(4, 7);                         // Остальные строки очищаются
    Board->ledsBlue();                                // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN); // Активировать группу кнопок
  }

  MState * MNominalV::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE: Tools->writeNvsShort("options", "nominalV", nominalV);
                           return new MStart(Tools);     // Там выбрать или завершить
      case MDisplay::UP:   nominalV = Tools->updnInt( nominalV, dn, up, +2000);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("NominalV changed:", 1); 
        Display->drawParFl("NominalV, V :", line, nominalV, 1, line); // Белым - когда значение изменено
        break;
      case MDisplay::DN:   nominalV = Tools->updnInt( nominalV, dn, up, -2000);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("NominalV changed:", 1); 
        Display->drawParFl("NominalV, V :", line, nominalV, 1, line); 
        break;
      default:;
    }
    return this;
  }; //MNominalV

  /*========================================================================= MCapacity
    Состояние "Выбор емкости батареи" для расчета рекомендуемого максимального
    тока заряда" */
  MCapacity::MCapacity(MTools * Tools) : MState(Tools)
  {
      // В главное окно выводятся:
    Display->drawLabel("OPTIONS", 0);                 // режим,
    Display->drawLabel("Adjusting capacity", 1);      // полное название параметра (жёлтым)
    Display->clearLine(2);
    line = 3;       // В строке 3 имя, размерность и значение параметра из Nvs
    //Display->drawShort("Capacity, ah :", line, capacity, 0);
    Display->drawShort("Capacity, ah :", line, capacity);
    Display->clearLine(4, 7);                         // Остальные строки очищаются
    Board->ledsBlue();                                // Синий - что-то меняется
    Display->newBtn(MDisplay::SAVE, MDisplay::UP, MDisplay::DN); // Активировать группу кнопок
  }

  MState * MCapacity::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::SAVE:  Tools->writeNvsShort("options", "capacity", capacity);
                            return new MStart(Tools);     // Там выбрать или завершить
      case MDisplay::UP:  capacity>14 ? delta=5 : delta=1;
        capacity = Tools->updnInt(capacity, dn, up, +delta);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("Capacity changed:", 1); 
        Display->drawShort("Capacity, ah :", line, capacity, line);    // Белым - когда значение изменено
        break;
      case MDisplay::DN:  capacity>14 ? delta=5 : delta=1;
        capacity = Tools->updnInt(capacity, dn, up, -delta);
        Display->drawLabel("OPTIONS", 0);
        Display->drawLabel("Capacity changed:", 1); 
        Display->drawShort("Capacity, ah :", line, capacity, line); 
        break;
      default:;
    }
    return this;
  }; //MCapacity

  //========================================================================= MAkb
  // Состояние "Выбор типа батареи" 
  MAkb::MAkb(MTools * Tools) : MState(Tools)
  {
    line = 3;
    Display->drawLabel("Mode OPTIONS loaded:", 0);  // Режим, если кто-то забыл, напоминание
    Display->drawLabel("Selection by type :", 1);   // Подбор по типу
    Display->clearLine(2);
    Display->drawAdj("Pb", 3, line);   //
    Display->drawAdj("Li", 4, line);   // 
    Display->drawAdj("Ni", 5, line);   // 
    Display->clearLine(6, 7);
    Board->ledsGreen();
    Display->newBtn(MDisplay::GO, MDisplay::NEXT, MDisplay::STOP);  // Активировать группу кнопок
  }
  MState * MAkb::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::STOP:  return new MStart(Tools);
      case MDisplay::NEXT:  (line >= 5) ? line = 3 : line++;   // Переместить курсор (выделение цветом)           
        Display->drawAdj("Pb", 3, line);   //
        Display->drawAdj("Li", 4, line);   //
        Display->drawAdj("Ni", 5, line);   //
        break;
      case MDisplay::GO:
        switch (line)
        {
          case 3: return new MAkbPb(Tools);
          case 4: return new MAkbLi(Tools);
          case 5: return new MAkbNi(Tools);
          default:;
        }
      default:    break;
    }
    return this;
  };  //MAkb

  //========================================================================= MAkbPb
  // Состояние "Выбор технологии свинцовой батареи" 
  MAkbPb::MAkbPb(MTools * Tools) : MState(Tools)
  {
    line = 2;
    Display->drawLabel("Selection by Pb technology:", 0);
    Display->clearLine(1);
    Display->drawAdj(TypeAkb[Ca],   2, line);
    Display->drawAdj(TypeAkb[CaPl], 3, line);
    Display->drawAdj(TypeAkb[Sur],  4, line);
    Display->drawAdj(TypeAkb[Agm],  5, line);
    Display->drawAdj(TypeAkb[Gel],  6, line); 
    Board->ledsBlue();
    Display->newBtn(MDisplay::SAVE, MDisplay::NEXT, MDisplay::STOP);  // Активировать группу кнопок
  }

  MState * MAkbPb::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::STOP:    return new MStart(Tools);
      case MDisplay::NEXT:                    
        (line >= 6) ? line = 2 : line++;         // Переместить курсор (выделение цветом)
        Display->drawAdj(TypeAkb[Ca],   2, line);
        Display->drawAdj(TypeAkb[CaPl], 3, line);
        Display->drawAdj(TypeAkb[Sur],  4, line);
        Display->drawAdj(TypeAkb[Agm],  5, line);
        Display->drawAdj(TypeAkb[Gel],  6, line);
        break;
      case MDisplay::SAVE:
        switch (line)
        {
          case 2: Tools->writeNvsShort("options", "akb", Ca);   return new MStart(Tools);
          case 3: Tools->writeNvsShort("options", "akb", CaPl); return new MStart(Tools);
          case 4: Tools->writeNvsShort("options", "akb", Sur);  return new MStart(Tools);
          case 5: Tools->writeNvsShort("options", "akb", Agm);  return new MStart(Tools);
          case 6: Tools->writeNvsShort("options", "akb", Gel);  return new MStart(Tools);
          default:  break;
        }
      default:      break;
    }
    return this;
  };  //MAkbPb

  //========================================================================= MAkbLi
  // Состояние "Выбор технологии литиевой батареи"
  MAkbLi::MAkbLi(MTools * Tools) : MState(Tools)
  {
    line = 2;
    Display->drawLabel("Mode OPTIONS loaded:", 0);
    Display->clearLine(1);
    Display->drawAdj(TypeAkb[LiIon], 2, line);
    Display->drawAdj(TypeAkb[LiFe],  3, line);
    Display->drawAdj(TypeAkb[LiTit], 4, line);
    Display->clearLine(5, 7);
    Board->ledsBlue();
    Display->newBtn(MDisplay::SAVE, MDisplay::NEXT, MDisplay::STOP); // Активировать группу кнопок
  }

  MState * MAkbLi::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::STOP:    return new MStart(Tools);
      case MDisplay::NEXT:                    
        (line >= 4) ? line = 2 : line++;         // Переместить курсор (выделение цветом)
        Display->drawAdj(TypeAkb[LiIon], 2, line);
        Display->drawAdj(TypeAkb[LiFe],  3, line);
        Display->drawAdj(TypeAkb[LiTit], 4, line);
        break;
      case MDisplay::SAVE:
        switch (line)
        {
          case 2: Tools->writeNvsShort("options", "akb", LiIon);  return new MStart(Tools);
          case 3: Tools->writeNvsShort("options", "akb", LiFe );  return new MStart(Tools);
          case 4: Tools->writeNvsShort("options", "akb", LiTit);  return new MStart(Tools);
          default:  break;
        }
      default:      break;
    }
    return this;
  };  //MAkbLi

  //========================================================================= MAkbNi
  // Состояние "Выбор технологии никелевой батареи"
  MAkbNi::MAkbNi(MTools * Tools) : MState(Tools)
  {
    line = 1;
    Display->drawLabel( "Mode OPTIONS loaded:", 0 ); // Режим, если кто-то забыл, напоминание
    Display->clearLine(1);
    Display->drawAdj( "NiCd/Mh",  2, line );   //
    Display->clearLine(3, 7);
    Board->ledsBlue();
    Display->newBtn(MDisplay::SAVE, MDisplay::NEXT, MDisplay::STOP); // Активировать группу кнопок
  }

  MState * MAkbNi::fsm()
  {
    switch (Display->getKey())
    {
      case MDisplay::STOP:  return new MStart(Tools);
      case MDisplay::NEXT:  break;
      case MDisplay::SAVE:  Tools->writeNvsShort("options", "akb", NiCd);
                            return new MStart(Tools);
      default:    break;
    }
    return this;
  };  //MAkbNi

  /*========================================================================= MApply
    */
  MApply::MApply(MTools * Tools) : MState(Tools)
  {
    Display->drawLabel( "OPTION", 0 );
    Display->drawLabel( ". . . Calculations . . .", 1 );
  }

  MState * MApply::fsm()
  {
    maxV = nominalV * v_max[tec] / 2000;  // от количества банок
    Tools->writeNvsShort("options", "maxV", maxV);

    minV = nominalV * v_min[tec] / 2000;  //
    Tools->writeNvsShort("options", "minV", minV);

    maxI = capacity * i_max[tec] * 10;    // с переводом в мАч
    Tools->writeNvsShort("options", "maxI", maxI);

    minI = constrain(capacity < 1, MPrj::cur_min_lo, MPrj::cur_min_hi); 
    Tools->writeNvsShort("options", "minI", minI);

    return new MStop(Tools);
  };

  /*========================================================================= MStop
    Состояние "Завершение режима" - до нажатия кнопки "EXIT" предоставляется
    возможность вернуться в начало. */
  MStop::MStop(MTools * Tools) : MState(Tools)
  {
    Display->drawLabel( "OPTION", 0 );
    Display->drawLabel( "Options completed", 1 );
    Display->drawLabel( "Setup has been made", 2 );
      // В строки 3 ... 7 можно добавить результаты или очистить:
    Display->drawParFl("MaxV, V :", 3, maxV, 2); // 
    Display->drawParFl("MaxI, A :", 4, maxI, 2); // 
    Display->drawParFl("MinV, V :", 5, minV, 2); // 
    Display->drawParFl("MinI, A :", 6, minI, 2); // 
    Display->clearLine(7);
    Board->ledsRed();         // Стоп-сигнал
      // Активировать группу кнопок для возврата в начало или завершения режима
    Display->newBtn(MDisplay::START, MDisplay::EXIT);
  }

  MState * MStop::fsm()
  {
    switch ( Display->getKey() )
    {
      case MDisplay::START:     return new MStart(Tools); // Возобновить
      case MDisplay::EXIT:      return new MExit(Tools);  // Выйти в меню
      default:;
    }
    return this;
  }; //MStop

  /*========================================================================= MExit
    Состояние "Выход из режима"*/
  MExit::MExit(MTools * Tools) : MState(Tools)
  {
    Tools->aboutMode( MDispatcher::OPTIONS );   // Название режима
    Board->ledsOff();
      // Активировать группу кнопок меню диспетчера
    Display->newBtn(MDisplay::GO, MDisplay::UP, MDisplay::DN);
  } 

  MState * MExit::fsm()
  {
    return nullptr;           // В меню
  }; //MExit

}; //MOption
