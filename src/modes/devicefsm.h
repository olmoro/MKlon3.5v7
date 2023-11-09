/* 20231108 MKlon2v7a */

#ifndef _DEVICEFSM_H_
#define _DEVICEFSM_H_

#include "state/mstate.h"
#include "project_config.h"


namespace MDevice
{

  static constexpr short sp_u_default = 13000u;  // 13.0v
  static constexpr short sp_i_default =  1000u;  //  1.0A

  class MStart : public MState
  {       
    public:
      MStart(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MAdjPid : public MState
  {
    public:
      MAdjPid(MTools * Tools);
      MState * fsm() override;
  };

  class MAdjPidI : public MState
  {       
    public:
      MAdjPidI(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MAdjPidV : public MState
  {       
    public:
      MAdjPidV(MTools * Tools);
      MState * fsm() override;
  };

  class MAdjPidD : public MState
  {       
    public:
      MAdjPidD(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr short sp_d_default = 1000u;  // 1A
  };

  class MPidFrequency : public MState
  {
    public:
      MPidFrequency(MTools * Tools);
      MState * fsm() override;
    private:
//      short freq[6]{ 10, 20, 50, 100, 200, 250 };
      short i;
//      static constexpr unsigned short fixed = MPrj::pid_frequency_default;
      static constexpr unsigned short up = 5;
      static constexpr unsigned short dn = 0;     
  };












  class MAdjVoltage : public MState
  {       
    public:
      MAdjVoltage(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MShiftV : public MState
  {       
    public:
      MShiftV(MTools * Tools);
      MState * fsm() override;
    private:
      short shift;
      static constexpr short fixed = MPrj::shift_v_default;
      static constexpr short up = fixed + 200;
      static constexpr short dn = fixed - 200;
  };

  class MFactorV : public MState
  {       
    public:
      MFactorV(MTools * Tools);
      MState * fsm() override;
    private:
      short factor;
      static constexpr short fixed = MPrj::factor_v_default;
      static constexpr short up = fixed + 200;   // Около 2%
      static constexpr short dn = fixed - 200;
  };

  class MSmoothV : public MState
  {       
    public:
      MSmoothV(MTools * Tools);
      MState * fsm() override;
    private:
      short smooth;
      static constexpr short fixed = MPrj::smooth_v_default;
      static constexpr short up = 6;
      static constexpr short dn = 0;      
  };

  class MAdjCurrent : public MState
  {       
    public:
      MAdjCurrent(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MShiftI : public MState
  {       
    public:
      MShiftI(MTools * Tools);
      MState * fsm() override;
    private:
      short shift;
      static constexpr short fixed = MPrj::shift_i_default;
      static constexpr short up = fixed + 200;
      static constexpr short dn = fixed - 200;
  };

  class MFactorI : public MState
  {       
    public:
      MFactorI(MTools * Tools);
      MState * fsm() override;
    private:
      short factor;
      static constexpr short fixed = MPrj::factor_i_default;
      static constexpr short up = fixed + 200;
      static constexpr short dn = fixed - 200;
  };

  class MSmoothI : public MState
  {       
    public:
      MSmoothI(MTools * Tools);
      MState * fsm() override;
    private:
      short smooth;
      static constexpr short fixed = MPrj::smooth_i_default;
      static constexpr short up = 6;
      static constexpr short dn = 0; 
  };





//===== MLoadSpV, ввод порога PID-регулятора заряда по напряжению ===== 
  class MLoadSpV : public MState
  {
    public:  
      MLoadSpV(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания напряжения
      static constexpr short up    = 16200u;
      static constexpr short dn    =  200u;
      static constexpr short delta =  200u;
  };

  //======== MLoadKpV, ввод параметра KP PID-регулятора напряжения ========= 
  class MLoadKpV : public MState
  {
    public:  
      MLoadKpV(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;    //1.00f;
      static constexpr float dn = 0.01f; 
  };

  //======== MLoadKiV, ввод параметра KI PID-регулятора напряжения ========= 
  class MLoadKiV : public MState
  {
    public:  
      MLoadKiV(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;    //4.00f;
      static constexpr float dn = 0.00f;
  };

  //======== MLoadKdV, ввод параметра KD PID-регулятора напряжения ========= 
  class MLoadKdV : public MState
  {
    public:  
      MLoadKdV(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;    //1.00f;
      static constexpr float dn = 0.00f;
  };



//===== MLoadSpV, ввод порога PID-регулятора заряда по напряжению ===== 
  class MLoadSpI : public MState
  {
    public:  
      MLoadSpI(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания тока
      static constexpr short up    = 6000u;
      static constexpr short dn    =  200u;
      static constexpr short delta =  200u;
  };




  //======== MLoadKpI, ввод параметра KP PID-регулятора тока ============= 
  class MLoadKpI : public MState
  {
    public:  
      MLoadKpI(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;    //2.00f;
      static constexpr float dn = 0.01f; 
  };

  //======== MLoadKiI, ввод параметра KI PID-регулятора тока ============= 
  class MLoadKiI : public MState
  {
    public:  
      MLoadKiI(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;    //4.00f;
      static constexpr float dn = 0.00f;
  };

  //======== MLoadKdI, ввод параметра KD PID-регулятора тока ============= 
  class MLoadKdI : public MState
  {
    public:  
      MLoadKdI(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;    //1.00f;
      static constexpr float dn = 0.00f;
  };


//========== MLoadSp, ввод порога PID-регулятора разряда ================= 
  class MLoadSp : public MState
  {
    public:  
      MLoadSp(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания тока
      static constexpr short up           = 3000u;
      static constexpr short dn           =  200u;
      static constexpr short delta        =  200u;
  };

  //====== MLoadKpD, ввод параметра KP PID-регулятора тока разряда ========= 
  class MLoadKpD : public MState
  {
    public:  
      MLoadKpD(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up     = MPrj::par_float_max;  //    1.00f;
      static constexpr float dn     = 0.01f; 
      static constexpr float delta  = 0.01f;
  };

  //====== MLoadKiD, ввод параметра KI PID-регулятора тока разряда ========= 
  class MLoadKiD : public MState
  {
    public:  
      MLoadKiD(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;
      static constexpr float dn = 0.00f;
  };

  //====== MLoadKdD, ввод параметра KD PID-регулятора тока разряда ========= 
  class MLoadKdD : public MState
  {
    public:  
      MLoadKdD(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = MPrj::par_float_max;
      static constexpr float dn = 0.00f;
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

  class MClear : public MState
  {
    public:  
      MClear(MTools * Tools);
      MState * fsm() override;
  };

  class MClrDevKeys : public MState
  {
    public:
      MClrDevKeys(MTools * Tools);
      MState * fsm() override;
  };

  class MClrOptKeys : public MState
  {
    public:
      MClrOptKeys(MTools * Tools);
      MState * fsm() override;
  };

  class MClrCCCVKeys : public MState
  {
    public:
      MClrCCCVKeys(MTools * Tools);
      MState * fsm() override;
  };


  class MClrDisKeys : public MState
  {
    public:
      MClrDisKeys(MTools * Tools);
      MState * fsm() override;
  };

  class MClrTplKeys : public MState
  {
    public:
      MClrTplKeys(MTools * Tools);
      MState * fsm() override;
  };

  class MMultXY : public MState
  {
    public:
      MMultXY(MTools * Tools);
      MState * fsm() override;
  };

  class MMultX : public MState
  {
    public:
      MMultX(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr short fixed = 480;
      static constexpr short up = fixed + 50;
      static constexpr short dn = fixed - 50;
      short touchX = fixed;
  };

  class MMultY : public MState
  {
    public:
      MMultY(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr short fixed = 320;
      static constexpr short up = fixed + 40;
      static constexpr short dn = fixed - 40;
      short touchY = fixed;
  };

};

#endif  // !_DEVICEFSM_H_
