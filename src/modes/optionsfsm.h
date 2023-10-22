/*
 * Доступные пользователю настройки режимов работы
 *
 * Сентябрь, 19 2023 
 */

#ifndef _OPTIONSFSM_H_
#define _OPTIONSFSM_H_

#include "state/mstate.h"

namespace MOption
{
  enum tec {Ca=0, CaPl, Sur, Agm,  Gel, LiIon, LiFe, LiTit, NiCd};

  static constexpr char* TypeAkb[] = {(char *)"Pb Ca/Ca", (char *)"Pb Ca+", (char *)"Pb Sur",
                                      (char *)"Pb AGM",   (char *)"Pb Gel", (char *)"Li-ion",
                                      (char *)"LiFePo4",  (char *)"LiTit",  (char *)"NiCd/Mh"};

  class MStart : public MState
  {       
    public:
      MStart(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MSetPostpone : public MState
  {       
    public:
      MSetPostpone(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования задержки пуска, час
      static constexpr short up = 23;
      static constexpr short dn =  0;
  };

  class MTimeout : public MState
  {       
    public:
      MTimeout(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования максимального времени заряда, час
      static constexpr short up = 5u;
      static constexpr short dn = 0u;
  };

  class MNominalV : public MState
  {       
    public:
      MNominalV(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования 
      static constexpr short up = 12000u;
      static constexpr short dn =  2000u;
  };

  class MCapacity : public MState
  {       
    public:
      MCapacity(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования 
      static constexpr short up = 60u;
      static constexpr short dn =  1u;
      short delta = 5u;
  };

  class MAkb : public MState
  {       
    public:
      MAkb(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MAkbPb : public MState
  {       
    public:
      MAkbPb(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MAkbLi : public MState
  {       
    public:
      MAkbLi(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MAkbNi : public MState
  {       
    public:
      MAkbNi(MTools * Tools);
      MState * fsm() override;
    private:
  };

  class MApply : public MState
  {
    public:  
      MApply(MTools * Tools);   
      MState * fsm() override;
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

#endif // !_OPTIONSFSM_H_
