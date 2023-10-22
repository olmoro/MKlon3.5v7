/*
*   Диспетчер, Запускается задачей mainTask, период запуска 100мс для корректного
* подсчета ампер-часов. 
* Сентябрь, 5, 2023
*/
#include "mdispatcher.h"
#include "mtools.h"
#include "board/msupervisor.h"
#include "display/mdisplay.h"
#include <string.h>
#include "modes/bootfsm.h"
#include "modes/optionsfsm.h"
#include "modes/templatefsm.h"
#include "modes/cccvfsm.h"
#include "modes/dischargefsm.h"
#include "modes/devicefsm.h"
#include "Arduino.h"


MDispatcher::MDispatcher(MTools * tools) :
Tools(tools), Display(tools->Display)
{
  strcpy(sVers, "MKlon2v7a");             // Версия проекта

  Display->newMainWin();                  // Очистить главное окно
  Display->newModeWin();

  latrus = Tools->readNvsBool("device", "local", true );  // В этом проекте нет

  modeSelection = BOOT;    // При включении прибора BOOT загружается всегда 

  Display->newBtn( MDisplay::GO );
  Display->addBtn( MDisplay::UP );
  Display->addBtn( MDisplay::DN );

  Tools->aboutMode( modeSelection );      // Название режима и краткое описание на дисплей
}

  /* Если при вызове диспетчера флаг синхронизации параметров sync = false, то
    вызывается загрузчик параметров - режим BOOL.
    Иначе флаг sync = true указывает на то, что параметры контроллеров синхронизированы
    и запускается режим, сохраненный в энергонезависимой памяти (тот, что запускали в 
    предыдущий раз), но с проверкой, есть ли такой в списке режимов и только один раз. 
  */
void MDispatcher::run()
{
  if ( Tools->getSync() )
  {
    /* Восстановление режима, из которого произведен выход в предыдущем сеансе
      с простейшей проверкой */
    modeSelection = Tools->readNvsInt ( "device", "mode", OPTIONS );

    if ( modeSelection == 0 || modeSelection > num_modes )  modeSelection = OPTIONS;

    Display->newBtn( MDisplay::GO );
    Display->addBtn( MDisplay::UP );
    Display->addBtn( MDisplay::DN );
    Tools->aboutMode( modeSelection );
    Tools->setSync( false );
  }
    // Индикация при инициализации процедуры выбора режима работы
  Tools->showVolt(Tools->getRealVoltage(), 2);
  Tools->showAmp (Tools->getRealCurrent(), 2);

  if ( State )
  {  // Работаем с FSM
    MState * newState = State->fsm();      
    if (newState != State)                  // State изменён!
    {
      delete State;
      State = newState;
    } 
    // Если будет 0, на следующем цикле увидим
  }
  else // State не определён (0) - выбираем или показываем режим
  {
    if ( Tools->getSync() )
    {
      State = new MBoot::MStart(Tools);
      Tools->setSync( false );
    }  
    else  
    {
      if ( modeSelection == BOOT || Display->getKey( MDisplay::GO ) )
      {
#ifdef PRINT_DISP
  Serial.println("Do Go");
#endif
          // Запомнить крайний выбор режима, исключая BOOT
        if ( modeSelection != BOOT )  Tools->writeNvsInt( "device", "mode", modeSelection );

        switch ( modeSelection )
        {
          case BOOT:        State = new MBoot::MStart(Tools);     break;
          case OPTIONS:     State = new MOption::MStart(Tools);   break;
          case SAMPLE:      State = new Template::MStart(Tools);  break;
          case CCCV:        State = new MCccv::MStart(Tools);     break;
          case DISCHARGE:   State = new MDis::MStart(Tools);    break;
          case DEVICE:      State = new MDevice::MStart(Tools);   break;
          default:                                                break;
        }
      } // !START

      if (Display->getKey(MDisplay::UP))
      { 
#ifdef PRINT_DISP
  Serial.println("Do Up");
#endif
        /* Исключена возможность выбора BOOT'а через меню */
        if ( modeSelection == (int)DEVICE ) modeSelection = OPTIONS;
        else modeSelection++;
        Tools->aboutMode( modeSelection );
      }

      if ( Display->getKey(MDisplay::DN) )
      {
#ifdef PRINT_DISP
  Serial.println("Do Dn");
#endif
        /* Исключена возможность выбора BOOT'а через меню */
        if ( modeSelection == (int)OPTIONS ) modeSelection = DEVICE;
        else modeSelection--;
        Tools->aboutMode( modeSelection );
      }
    }
  }
}
