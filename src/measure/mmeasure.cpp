/*
    Конечный автомат обработки данных АЦП ESP32:
    температуры °С, напряжения питания.
    
    11.08.2023
*/
#include "measure/mmeasure.h"
#include "mtools.h"
#include "board/mboard.h"
#include "display/mdisplay.h"
#include "state/mstate.h"
#include <Arduino.h>
#include <stdint.h>

MMeasure::MMeasure(MTools * tools) : Tools(tools), Board(tools->Board)
{
  //State = new MMeasureStates::MAdcVI(Tools);
  State = new MMeasureStates::MAdcCelsius(Tools);
}

void MMeasure::run()
{
  MState * newState = State->fsm();      
  if (newState != State)                      //state changed!
  {
    delete State;
    Board->buzzerOff();         // Короткий "Биип" на нажатие
    State = newState;
  } 
}

namespace MMeasureStates
{

  MState * MAdcCelsius::fsm()
  {
    // Измерение и вычисление реальной температуры, результат в celsius
    Board->calculateCelsius();              //Tools->setCelsius( Board->getAdcT() );
    return new MAdcPowerGood(Tools);
  };

    // Измерение напряжения вторичного питания
  MState * MAdcPowerGood::fsm()
  {
    //Tools->setPowerGood( Board->getAdcPG() );     // В MTools пока не реализовано - в MBoard
    return new MAdcCelsius(Tools);
  };

};
