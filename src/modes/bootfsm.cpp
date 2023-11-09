/*
  bootfsm.cpp
  Конечный автомат синхронизации данных между контроллерами.
            Как это работает.
    Процесс синхронизации (BOOT) запускается диспетчером при инициализации прибора. 
  Пользовательские параметры (OPTIONS) и параметры разработчика (DEVICE) восстанавливаются из
  энергонезависимой памяти ESP32 и одновременнно передаются на драйвер SAMD21, где заменяют 
  соответствующие дефолтные значения. Во время синхронизации на дисплей выводится информация 
  о ходе синхронизации. По окончании процесса синхронизации прибор готов к работе в выбранном
  режиме.

  08.11.2023  
*/

#include "modes/bootfsm.h"
#include "mtools.h"
#include "board/mboard.h"
#include "display/mdisplay.h"
#include "mdispatcher.h"
#include <Arduino.h>
#include <string>


namespace MBoot
{

  extern short modeSelection;

  // Старт и инициализация выбранного режима работы.
  MStart::MStart(MTools * Tools) : MState(Tools) 
  {
    Tools->setSync(false);
    Board->ledsBlue();                          // Подтверждение входа синим
#ifdef BOOT_STEP
    Display->drawLabel( "Boot", 0 );
    Display->drawLabel( "Blocking 3s", 1 );
    Display->clearLine( 2, 7 );
    Display->newBtn( MDisplay::NEXT );
#endif
  }
  MState * MStart::fsm()
  {
    Tools->setBlocking(true);                           // Блокировать обмен
    vTaskDelay(1000/portTICK_PERIOD_MS);                // Не беспокоим драйвер 3 секунды после рестарта 
    Tools->setBlocking(false);                          // Разблокировать обмен
    return new MTxPowerStop(Tools);                     // Перейти к отключению
  };

  MTxPowerStop::MTxPowerStop(MTools * Tools) : MState(Tools)
  {
#ifdef BOOT_STEP
    Display->drawLabel( "Power OFF", 1 );
#endif
  }
  MState * MTxPowerStop::fsm()
  {

#ifdef BOOT_STEP
    if ( Display->getKey( MDisplay::NEXT ) )
    {
      Tools->txPowerStop();                               // 0x21  Команда драйверу
      return new MTxSetFrequency(Tools);
    }
    else return this;
#else
    Tools->txPowerStop();                               // 0x21  Команда драйверу
    return new MTxSetFrequency(Tools);
#endif
  };

  MTxSetFrequency::MTxSetFrequency(MTools * Tools) : MState(Tools)
  {
    i = Tools->readNvsShort("device", "freq", MPrj::pid_frequency_default);   // Взять сохраненное из nvs.
#ifdef BOOT_STEP
    Display->drawLabel( "Pid Frequency", 2 );
#endif
  }

  MState * MTxSetFrequency::fsm()
  {
    if(i <= 0) i = 0;      if(i >= 5) i = 5;

#ifdef BOOT_STEP
    if ( Display->getKey( MDisplay::NEXT ) )
    {
      Tools->txSetPidFrequency(MPrj::f_hz[i]);                  // 0x4A  Команда драйверу
      return new MTxGetTreaty(Tools);
    }
    else return this;
#else
    Tools->txSetPidFrequency(MPrj::f_hz[i]);                  // 0x4A  Команда драйверу
    return new MTxGetTreaty(Tools);
#endif
  };
  
  // Получить согласованные данные для обмена с драйвером.
  MTxGetTreaty::MTxGetTreaty(MTools * Tools) : MState(Tools)
  {
#ifdef BOOT_STEP
    Display->drawLabel( "Agreement", 3 );
#endif
  }
  MState * MTxGetTreaty::fsm()
  {

#ifdef BOOT_STEP
    if ( Display->getKey( MDisplay::NEXT ) )
    {
      Tools->txGetPidTreaty();                            // 0x47  Команда драйверу
      return new MTxsetFactorV(Tools);
    }
    else return this;
#else
    Tools->txGetPidTreaty();                            // 0x47  Команда драйверу
    return new MTxsetFactorV(Tools);
#endif
  };

  // Восстановление пользовательского (или заводского) коэфициента преобразования в милливольты.
  MTxsetFactorV::MTxsetFactorV(MTools * Tools) : MState(Tools)
  {
    Tools->factorV = Tools->readNvsShort("device", "factorV", fixed);
#ifdef BOOT_STEP
    Display->clearLine( 2, 3 );
    Display->drawLabel( "Factor V", 1 );
#endif
  }
  MState * MTxsetFactorV::fsm()
  {
#ifdef BOOT_STEP
    if (Display->getKey(MDisplay::NEXT))
    {
      Tools->txSetFactorU(Tools->factorV);                // 0x31  Команда драйверу
      return new MTxSmoothV(Tools);
    }
    else  return this;
#else
    Tools->txSetFactorU(Tools->factorV);                // 0x31  Команда драйверу
    return new MTxSmoothV(Tools);
#endif
  };

  // Восстановление пользовательского (или заводского) коэффициента фильтрации по напряжению.
  MTxSmoothV::MTxSmoothV(MTools * Tools) : MState(Tools)
  {
    Tools->smoothV = Tools->readNvsShort("device", "smoothV", fixed);
#ifdef BOOT_STEP
    Display->drawLabel("Smooth V", 2);
#endif
  }
  MState * MTxSmoothV::fsm()
  {
#ifdef BOOT_STEP
    if (Display->getKey(MDisplay::NEXT))
    {
      Tools->txSetSmoothU(Tools->smoothV);                // 0x34  Команда драйверу
      return new MTxShiftV(Tools);
    }
    else return this;
#else
    Tools->txSetSmoothU(Tools->smoothV);                // 0x34  Команда драйверу
    return new MTxShiftV(Tools);
#endif
  };

  // Восстановление пользовательской (или заводской) настройки сдвига по напряжению.
  MTxShiftV::MTxShiftV(MTools * Tools) : MState(Tools)
  {
    Tools->shiftV = Tools->readNvsShort("device", "offsetV", fixed);
#ifdef BOOT_STEP
    Display->drawLabel("Shift V", 3);
#endif
  }

  MState * MTxShiftV::fsm()
  {
#ifdef BOOT_STEP
    if (Display->getKey(MDisplay::NEXT))
    {
      Tools->txSetShiftU(Tools->shiftV);                  // 0x36  Команда драйверу
      return new MTxFactorI(Tools);
    }
    else return this;
#else
    Tools->txSetShiftU(Tools->shiftV);                  // 0x36  Команда драйверу
    return new MTxFactorI(Tools);
#endif
  };

  // Восстановление пользовательского (или заводского) коэфициента преобразования в миллиамперы.
  MTxFactorI::MTxFactorI(MTools * Tools) : MState(Tools)
  {
#ifdef BOOT_STEP
    Display->clearLine( 2, 3 );
    Display->drawLabel("Factor I", 1);
#endif
  }
  MState * MTxFactorI::fsm()
  {
    Tools->factorI = Tools->readNvsShort("device", "factorI", fixed);

#ifdef BOOT_STEP
    if (Display->getKey(MDisplay::NEXT))
    {
      Tools->txSetFactorI(Tools->factorI);                // 0x39  Команда драйверу
      return new MTxSmoothI(Tools);
    }
    else return this;
#else
    Tools->txSetFactorI(Tools->factorI);                // 0x39  Команда драйверу
    return new MTxSmoothI(Tools);
#endif
  };

  // Восстановление пользовательского (или заводского) коэффициента фильтрации по току.
  MTxSmoothI::MTxSmoothI(MTools * Tools) : MState(Tools)
  {
    Tools->smoothI = Tools->readNvsShort("device", "smoothI", fixed);
#ifdef BOOT_STEP
    Display->drawLabel("Smooth I", 2);
#endif
  }
  MState * MTxSmoothI::fsm()
  {
#ifdef BOOT_STEP
    if (Display->getKey(MDisplay::NEXT))
    {
      Tools->txSetSmoothI(Tools->smoothI);                // 0x3C  Команда драйверу
      return new MTxShiftI(Tools);
    }
    else return this;
#else
    Tools->txSetSmoothI(Tools->smoothI);                // 0x3C  Команда драйверу
    return new MTxShiftI(Tools);
#endif
  };

  // Восстановление пользовательской (или заводской) настройки сдвига по току.
  MTxShiftI::MTxShiftI( MTools * Tools ) : MState( Tools )
  {
    Tools->shiftI = Tools->readNvsShort( "device", "offsetI", fixed );
#ifdef BOOT_STEP
    Display->drawLabel("Shift I", 3);
#endif
  }
  
  MState * MTxShiftI::fsm()
  {
#ifdef BOOT_STEP
    if (Display->getKey(MDisplay::NEXT))
    {
      Tools->txSetShiftI(Tools->shiftI);                  // 0x3E  Команда драйверу
      return new MExit(Tools);
    }  
    else return this;
#else
    Tools->txSetShiftI(Tools->shiftI);                  // 0x3E  Команда драйверу
    return new MDel(Tools);
#endif
  };

  // Пустое состояние для нормального завершения команды
  MDel::MDel(MTools *Tools) : MState(Tools) {}
  MState * MDel::fsm() {return new MExit( Tools );};

  // Процесс выхода из режима "BOOT".
  // Состояние: "Индикация итогов и выход из режима заряда в меню диспетчера" 
  MExit::MExit(MTools * Tools) : MState(Tools)
  {
    Tools->txReady();                                   // 0x15  Команда драйверу
#ifdef BOOT_STEP
    Display->drawLabel("Boot done:", 0);
    Display->drawShort( "factorV :", 1, Tools->factorV); 
    Display->drawShort( "smoothV :", 2, Tools->smoothV); 
    Display->drawShort(  "shiftV :", 3, Tools->shiftV); 
    Display->drawShort( "factorI :", 4, Tools->factorI); 
    Display->drawShort( "smoothI :", 5, Tools->smoothI); 
    Display->drawShort(  "shiftI :", 6, Tools->shiftI); 
#endif

    Board->ledsOff();
    Display->showVolt(Tools->getRealVoltage(), 2);
    Display->showAmp (Tools->getRealCurrent(), 2);
    Board->setReady(true);
  }    
  MState * MExit::fsm()
  {
#ifdef BOOT_STEP
    if (Display->getKey(MDisplay::NEXT))
    {
      Tools->setSync(true);
      return nullptr;                // Возврат к выбору режима
    }
    else return this;
#else
    Tools->setSync(true);
    return nullptr;                // Возврат к выбору режима
#endif
  }
};  //MBoot Конечный автомат синхронизации данных между контроллерами.
