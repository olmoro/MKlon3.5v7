/* 20231022 MKlon2v7a */

#ifndef _DISCHARGEFSM_H_
#define _DISCHARGEFSM_H_

#include "state/mstate.h"
#include "project_config.h"

namespace MDis
{
  class MStart : public MState
  {       
    public:
      MStart(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MGoType : public MState
  {       
    public:
      MGoType(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MSetVoltage : public MState
  {
    public:   
      MSetVoltage(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования минимального напряжения
      static constexpr short up   = 16200u;
      static constexpr short dn   =  1000u;
      static constexpr short corr =    50u;  // ±correction

  };

  class MSetCurrent : public MState
  {
    public:   
      MSetCurrent(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования max тока
      static constexpr short up   = 6000u;
      static constexpr short dn   =  200u;
      static constexpr short corr =   50u;  // ±correction
  };

  class MGoTask : public MState
  {       
    public:
      MGoTask(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MGo : public MState
  {
    public:   
      MGo(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MStop : public MState
  {
    public:  
      MStop(MTools * Tools);
      MState * fsm() override;
  };

  class MExit : public MState
  {
    public:
      MExit(MTools * Tools);
      MState * fsm() override;
  };

};

#endif //_DISCHARGEFSM_H_
