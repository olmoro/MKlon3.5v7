/*
  project:      MKLON3.5v7
  pcb:          cpu3.5v7a  
  pcb:          pow3.5v7
  display:      480*320 IC: ILI94856 + XPT2046
  driver:       SAMD21 M0 MINI
  C/C++:        1.17.5
  VS:           1.81.1  1.82.2
  PIO:          v3.3.1
  Arduino       v0.6.230727001 Arduino for Visual Studio Code
  Espressif 32: 3.5.0 (с 4.0 не совместимо)
                6.3.1 не совместимо
  date:         20230905
*/    

#include "board/mboard.h"
#include "board/msupervisor.h"
#include "driver/mcommands.h"
#include "display/mdisplay.h"
#include "mtools.h"
#include "mdispatcher.h"
#include "mconnmng.h"
#include "measure/mmeasure.h"
#include "connect/connectfsm.h"
#include <Arduino.h>


// Идентификаторы задач
TaskHandle_t xTask_Display;     // Вывод текущих данных на дисплей
TaskHandle_t xTask_Cool;        // Управления системой теплоотвода
TaskHandle_t xTask_Main;        // Выбор режима работы и его исполнение
TaskHandle_t xTask_Measure;     // Измерения напряжения питания и температуры
TaskHandle_t xTask_Driver;      // Отправка команд драйверу SAMD21
TaskHandle_t xTask_Connect;     // Подключения к WiFi сети
TaskHandle_t xTask_Touch;       // Опрос активных кнопок

// // Идентификаторы семафоров    // В этом проекте не используется
// SemaphoreHandle_t tftMutex;    // Семафор вывода сообщений на дисплей

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

static MBoard      * Board      = 0;
static MDisplay    * Display    = 0;
static MTools      * Tools      = 0;
static MCommands   * Commands   = 0;
static MMeasure    * Measure    = 0;
static MDispatcher * Dispatcher = 0;
#ifndef NOWIFI
  static MConnect    * Connect    = 0;
#endif

#ifndef NOWIFI
  void connectTask ( void * );
#endif
void displayTask ( void * );
void coolTask    ( void * );
void mainTask    ( void * );
void measureTask ( void * );
void driverTask  ( void * );
void touchTask   ( void * );


void setup()
{
  Serial.begin(115200);
  while(!Serial) {}
  
  Display    = new MDisplay();
  Board      = new MBoard();
  Tools      = new MTools(Board, Display);
  Commands   = new MCommands(Tools);
  Measure    = new MMeasure(Tools);
  Dispatcher = new MDispatcher(Tools);
  #ifndef NOWIFI
    Connect    = new MConnect(Tools);
  #endif

  // tftMutex = xSemaphoreCreateMutex();     // В этом проекте не используется
  // if ( tftMutex == NULL )  Serial.println("Mutex can not be created");
  // else                     Serial.print("\nm : ");   Serial.println( (int) tftMutex );

  // Выделение ресурсов для каждой задачи: память, приоритет, ядро.
  // Все задачи исполняются ядром 1, ядро 0 выделено для радиочастотных задач - BT и WiFi.
  #ifndef NOWIFI
    xTaskCreatePinnedToCore( connectTask, "Connect", 10000, NULL, 1, &xTask_Connect, 0 ); 
  #endif
  xTaskCreatePinnedToCore( mainTask,    "Main",     4096, NULL, 3, &xTask_Main,    1 ); 
  xTaskCreatePinnedToCore( displayTask, "Display",  4096, NULL, 2, &xTask_Display, 1 ); 
  xTaskCreatePinnedToCore( touchTask,   "Touch",    4096, NULL, 1, &xTask_Touch,   1 ); 
  xTaskCreatePinnedToCore( coolTask,    "Cool",     2048, NULL, 1, &xTask_Cool,    1 ); 
  xTaskCreatePinnedToCore( measureTask, "Measure",  4096, NULL, 1, &xTask_Measure, 1 ); 
  xTaskCreatePinnedToCore( driverTask,  "Driver",   4096, NULL, 4, &xTask_Driver,  1 ); 
}

void loop() 
{
  if( Serial2.available() )    // В буфере приема есть принятые байты, не факт, что пакет полный
  {                            // Пока не принят весь пакет, время ожидания ограничено 5мс
    vTaskEnterCritical(&timerMux);
      Tools->setErr( Commands->dataProcessing() );
    vTaskExitCritical(&timerMux);
  }
}

// Задача подключения к WiFi сети (полностью заимствована как есть)
#ifndef NOWIFI
  void connectTask( void * )
  {
    while(true) {
    //unsigned long start = millis();   // Старт таймера 
    //Serial.print("*");
      Connect->run(); 
      // Период вызова задачи задается в TICK'ах, TICK по умолчанию равен 1мс.
      vTaskDelay( 10 / portTICK_PERIOD_MS );
    // Для удовлетворения любопытства о длительности выполнения задачи - раскомментировать как и таймер.
    //Serial.print("Autoconnect: Core "); Serial.print(xPortGetCoreID()); Serial.print(" Time = "); Serial.print(millis() - start); Serial.println(" mS");
    // Core 1, 2...3 mS
    }
    vTaskDelete( NULL );
  }
#endif

// Задача выдачи данных на дисплей
void displayTask( void *pvParameters )
{
  while(true)
  {
    const TickType_t xDelay = pdMS_TO_TICKS(125);
    Display->runDisplay( Board->getCelsius() );
    vTaskDelay( xDelay );
  }
  vTaskDelete( NULL );
}

// Задача управления системой теплоотвода. Предполагается расширить функциональность, добавив 
// слежение за правильностью подключения нагрузки, масштабирование тока и т.д. 
void coolTask( void * )
{
  while (true)
  {
    Board->Supervisor->runCool();
    vTaskDelay( 200 / portTICK_PERIOD_MS );
  }
  vTaskDelete( NULL );
}

// 1. Задача обслуживает выбор режима работы.
// 2. Управляет конечным автоматом выбранного режима вплоть да выхода из режима.
// И то и другое построены как конечные автоматы (FSM).
void mainTask ( void * )
{ 
  while (true)
  {
    // Выдерживается период запуска для вычисления амперчасов. Если прочие задачи исполняются в порядке 
    // очереди, то эта точно по таймеру - через 0,1с.
    portTickType xLastWakeTime = xTaskGetTickCount();   // To count the amp hours
    Dispatcher->run(); 
    vTaskDelayUntil( &xLastWakeTime, 100 / portTICK_PERIOD_MS );    // период 0,1с
  }
  vTaskDelete( NULL );
}

// Задача управления измерениями напряжения источника питания и датчика температуры
void measureTask( void * )
{
  while (true)
  {
    Measure->run();
    vTaskDelay( 100 / portTICK_PERIOD_MS );
  }
  vTaskDelete(NULL);
}

// Отправка команд драйверу SAMD21
void driverTask( void * )
{
  while (true)
  {
    vTaskEnterCritical(&timerMux);
    Commands->doCommand();
    vTaskExitCritical(&timerMux);
    vTaskDelay( 75 / portTICK_PERIOD_MS );
  }
  vTaskDelete(NULL);
}

// Задача опроса активных кнопок
void touchTask( void * )
{
  while(true)
  {
    Display->calcKeys();
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
  vTaskDelete( NULL );
}
