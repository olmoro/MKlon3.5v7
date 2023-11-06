/* Константы, общие для проекта MKlon3.5v7
    19.09.2023
*/

#ifndef _PROJECT_CONFIG_H_
#define _PROJECT_CONFIG_H_

namespace MPrj
{
    // Приборные ограничения:

  // Согласованные параметры измерителей SAMD21:
  static constexpr short factor_v_default = 0x5326u;
  static constexpr short factor_i_default = 0x7918u;
  static constexpr short smooth_v_default = 1u;
  static constexpr short smooth_i_default = 1u;
  static constexpr short shift_v_default  = 0;
  static constexpr short shift_i_default  = 0;
  
  // Согласованные параметры ПИД-регуляторов SAMD21 (частный случай):
  static constexpr short pid_frequency_default  = 3u; // 100Hz ([10, 20, 50, 100, 200, 250])
  
  static constexpr float kp_v_default = 1.00f;   //0.56f;
  static constexpr float ki_v_default = 1.80f;   //1.79f;
  static constexpr float kd_v_default = 0.01f;  //0.09f;
  
  static constexpr float kp_i_default = 1.00f;   //0.56f;
  static constexpr float ki_i_default = 3.60f;   //1.79f;
  static constexpr float kd_i_default = 0.00f;   //0.09f;
  
  static constexpr float kp_d_default = 0.70f;   //
  static constexpr float ki_d_default = 0.20f;   //
  static constexpr float kd_d_default = 0.02f;   //




  // Дефолтные для ввода пользовательских параметров
  static constexpr short postpone_fixed  =  0u;
  static constexpr short timeout_fixed   =  5u;
  static constexpr short nominal_v_fixed = 12000u;
  static constexpr short capacity_fixed  = 55u;
  static constexpr short tec_fixed       = 0u;
  // Минимальный ток заряда, "вилка" значений для разных емкостей батареи
  static constexpr short cur_min_lo      =  20u;     // Минимальный ток заряда, mA от
  static constexpr short cur_min_hi      = 200u;     // и до

  static constexpr short max_v_fixed     = nominal_v_fixed * 2100 / 2000;
  static constexpr short min_v_fixed     = nominal_v_fixed * 1930 / 2000;
  static constexpr short max_i_fixed     = capacity_fixed * 10 * 10;
  static constexpr short min_i_fixed     = cur_min_lo;
        
  enum ROLES
  {
    RS = 0,    // режим прямого регулирования
    RU,        // режим управления напряжением
    RI,        // режим управления током
    RD         // режим управления током разряда
  };
};

#endif //_PROJECT_CONFIG_H_
