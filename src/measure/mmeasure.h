#ifndef _MMEASURE_H_
#define _MMEASURE_H_

#include "state/mstate.h"

class MTools;
class MBoard;
class MState;

class MMeasure
{
  public:
    MMeasure(MTools * tools);

    void run();
    //void delegateWork();

  private:
    MTools * Tools = nullptr;
    MBoard * Board = nullptr;

    MState * State = nullptr;
};

namespace MMeasureStates
{
       
  class MAdcCelsius : public MState
  {
    public:   
      MAdcCelsius(MTools * Tools) : MState(Tools) {}     
      MState * fsm() override;
  };
  
  class MAdcPowerGood : public MState
  {
    public:   
      MAdcPowerGood(MTools * Tools) : MState(Tools) {}     
      MState * fsm() override;
  };

};

#endif  // _MMEASURE_H_
