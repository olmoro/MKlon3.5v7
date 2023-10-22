#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

class MTools;
class MBoard;
class MDisplay;
class MState;
class MSupervisor;

class MDispatcher
{
  public:
    enum MODES
    {
      BOOT = 0,         // режим синхронизации
      OPTIONS,          // режим ввода настроек
      SAMPLE,           // шаблон режима 
      CCCV,             // режим заряда "постоянный ток / постоянное напряжение"
      DISCHARGE,         // режим разряда
      DEVICE,           // режим заводских регулировок
      num_modes         // автоподсчёт числа режимов
    };

  public:
    MDispatcher(MTools * tools);

    short modeSelection;              // Выбранный режим работы

    void run();
    void delegateWork();

  private:
    MTools      * Tools;
    MBoard      * Board;
    MDisplay    * Display;
    MState      * State = nullptr;
    MSupervisor * Supervisor;

    bool latrus;                      // Зарезервировано
    char sVers[20] = {0};

};

#endif //_DISPATCHER_H_
