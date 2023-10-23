#ifndef _CCCVFSM_H_
#define _CCCVFSM_H_

#include "state/mstate.h"

namespace MCccv
{
  // struct MConst
  // {
  //                                // 20230311     20230204
    // static constexpr float fixedKpV =  1.0;       //0.100f;
    // static constexpr float fixedKiV =  1.8;       //0.240f;
    // static constexpr float fixedKdV =  0.1;       //0.000f;
    // static constexpr float fixedKpI =  1.0;       //0.100f;
    // static constexpr float fixedKiI =  1.8;       //0.240f;
    // static constexpr float fixedKdI =  0.1;       //0.000f;
  // };

    // Режимы работы PID-регулятора
//  enum mode {MODE_OFF = 0, MODE_U, MODE_I, MODE_D, MODE_AUTO};

  class MStart : public MState
  {       
    public:
      MStart(MTools * Tools);
      MState * fsm() override;
    private:
  };

#ifdef CCCV_ADJ
  class MAdjParameters : public MState
  {
    public:   
      MAdjParameters(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MSetCurrentMax : public MState
  {
    public:   
      MSetCurrentMax(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования max тока
      static constexpr short up = 6000u;
      static constexpr short dn =  200u;
  };
  
  class MSetVoltageMax : public MState
  {
    public:   
      MSetVoltageMax(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования max напряжения
      static constexpr short up = 16200u;
      static constexpr short dn =  2000u;
  };

  class MSetCurrentMin : public MState
  {
    public:     
      MSetCurrentMin(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования min тока
      static constexpr short up = 2000u;
      static constexpr short dn =  200u;
  };

  class MSetVoltageMin : public MState
  {
    public:     
      MSetVoltageMin(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования min напряжения
      static constexpr short up = 14000u;
      static constexpr short dn =  2000u;
  };
#endif

  class MPostpone : public MState
  {
    public:   
      MPostpone(MTools * Tools);
      MState * fsm() override;
    private:
      float kp, ki, kd;
  };

  class MUpCurrent : public MState
  {
    public:   
      MUpCurrent(MTools * Tools);
      MState * fsm() override;
  };

  class MKeepVmax : public MState
  {
    public: 
      MKeepVmax(MTools * Tools);
      MState * fsm() override;
  };

  class MKeepVmin : public MState
  {
    public:   
      MKeepVmin(MTools * Tools);
      MState * fsm() override;
    private:
      short timeOut;    // Максимальное время заряда
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

#endif  // !_CCCVFSM_H_
