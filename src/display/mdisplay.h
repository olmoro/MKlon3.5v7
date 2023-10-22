#ifndef _MDISPLAY_H_
#define _MDISPLAY_H_

#include "TFT_eSPI.h"
#include "Free_Fonts.h"

//#include "stdio.h"
//#include "stdint.h"

class TFT_eSPI;
class MBoard;
class Preferences;

class MDisplay
{
  public:
    MDisplay();
    ~MDisplay();

    Preferences * qPreferences = nullptr; // local

    void calcKeys();

    // Invoke the TFT_eSPI button class and create all the button objects
    enum KEY_PRESS {  KEY_NO,    Q,   OK,   // Q - кулоновские, временные
                          D7,   D8,   D9,
                          D4,   D5,   D6,
                          D1,   D2,   D3,
                          DP,   D0,  DEL,    // 12 13 14
                       START,   // 15
                          UP,   // 16
                          DN,   // 17
                        STOP,   // 18
                        EXIT,   // 19
                          GO,   // 20
                        WAIT,   // 21
                         ADJ,   // 22
                        NEXT,   // 23
                        BACK,   // 24
                        SAVE,   // 25
                       PAUSE,   // 26
                        num_btns          // Количество кнопок!
                         };

    TFT_eSPI_Button Buttons[ num_btns ];

    boolean getKey(uint8_t key);
    KEY_PRESS getKey();

    static constexpr uint16_t SEC   = 0;
    static constexpr uint16_t HOUR  = 1;

  //  void drawBtnKeypad();
    void newBtn( uint8_t btn1, uint8_t btn2, uint8_t btn3 );
    void newBtn( uint8_t btn1, uint8_t btn2 );

    void newBtn( uint8_t btn );     // Очистка списка с добавлением первой кнопки
    void newBtnAuto( uint8_t btn ); // То же с признаком нажатия
    void addBtn( uint8_t btn );     // Добавление кнопки

    void newMainWin();                // Очистка окна
    void newModeWin();                // Очистка окна

    void initBtnKeypad();

    void clearLine( uint8_t line );                         // Очистка поля строки в главном окне
    void clearLine( uint8_t init, uint8_t last );           // Очистка нескольких строк в главном окне

    void drawLabel( const char *s, uint8_t line );          //
    void drawLabel( const char *s, uint8_t line, bool );    // Инверсное отображение

    void drawAdj( const char *s, uint8_t line );              // 
    void drawAdj( const char *s, uint8_t line, uint8_t pnt ); //

    void drawParam( const char *s, uint8_t line, float par, uint8_t dp );
    void drawParam( const char *s, uint8_t line, float par, uint8_t dp, uint8_t pnt );
    void drawParFl( const char *s, uint8_t line, short par, uint8_t dp );
    void drawParFl( const char *s, uint8_t line, short par, uint8_t dp, uint8_t pnt );

    // void drawParam( const char *s, uint8_t line, short par );
    // void drawParam( const char *s, uint8_t line, short par, uint8_t pnt );
    void drawShort( const char *s, uint8_t line, short par );
    void drawShort( const char *s, uint8_t line, short par, uint8_t pnt );

  //  void initBtnKeypadNew();
  //  void drawBtnKeypadNew();

    // void clearVolt();   // Очистка поля отображения напряжения
    // void clearAmp();   // Очистка поля отображения тока

    void runDisplay(float celsius);
    void printVolt();
    void printAmp();
    void printAmpHour();
    void printCelsius(float celsius, uint8_t pls);
    void printDuration();
    void printHeap();
  
        // static constexpr uint16_t SEC   = 0;
        // static constexpr uint16_t HOUR  = 1;

    void showVolt(float volt, uint8_t pls);    // showVoltage(...) new name
    void showAmp(float amp, uint8_t pls);
    void showAh( float ah );
    void showCelsius( float val );
    void showDuration( int time, int plan );
    void showHeap();

//    void showMode(char *s);

    void drawMainWindow();    // Рамка выбранного режима (большая) - очистка
    void drawModeWindow();    // Рамка выбора режима (малая) - очистка

    uint8_t dqBuff = 0;       // Буфер принятого кода

  private:
    TFT_eSPI * tft = nullptr;
    MBoard * Board = nullptr;

    bool buzy = false;        // Запрет на опрос кнопок при выполнении очистки окон

    uint8_t keyBuff = 0;      // Код нажатой кнопки

    short multX;              // Поправочный коэффициент тачскрина
    short multY;

    void status(const char *msg);
    const int TFT_GREY = 0xBDF7;

    // Using two fonts since numbers are nice when bold nu?
//    #define LABEL1_FONT &FreeSansOblique12pt7b        // Key label font 1
//    #define LABEL2_FONT &FreeSansBoldOblique12pt7b    // Key label font 2
//    #define LABEL3_FONT &FreeSansBold12pt7b           // Key label font 3
//    #define LABEL4_FONT &FreeSansBold18pt7b           // Key label font 4
//    #define LABEL5_FONT &FreeSansBold24pt7b           // Key label font 
//    #define LABEL6_FONT &FreeSans12pt7b          // Key label font 

    struct MV   // Volts
    {
      static constexpr int16_t        x =   0;
      static constexpr int16_t        y = 261;
      static constexpr int16_t        w = 238;
      static constexpr int16_t        h =  58;
      static constexpr int16_t       tx = 190;        // 'V'
      static constexpr int16_t       ty = 309;
      static constexpr uint8_t    tsize =   2;        // 
      static constexpr int8_t      font =   4;        // float size
      static constexpr int16_t       vx = 184;
      static constexpr int16_t       vy = 297;
      static constexpr uint16_t  fcolor = TFT_CYAN;   // float colour
      static constexpr uint16_t bgcolor = TFT_BLACK;  // 
    };

    struct MA   // Ampers
    {
      static constexpr int16_t        x = 239;        // window
      static constexpr int16_t        y = 261;
      static constexpr int16_t        w = 239;
      static constexpr int16_t        h =  58;
      static constexpr int16_t       tx = 430;        // 'A'
      static constexpr int16_t       ty = 309;
      static constexpr uint8_t    tsize =   2;
      static constexpr int8_t      font =   4;        // float
      static constexpr int16_t       ax = 424;
      static constexpr int16_t       ay = 297;
      static constexpr uint16_t  fcolor = TFT_CYAN;   // float colour
      static constexpr uint16_t bgcolor = TFT_BLACK;
    };


    struct MAh   // Amper-hours
    {
      static constexpr int16_t        x = 360;
      static constexpr int16_t        y = 224;
      static constexpr int16_t        w = 119;
      static constexpr int16_t        h =  32;
      static constexpr int16_t       tx = 437;
      static constexpr int16_t       ty = 231;
      static constexpr int16_t      ahx = 433;
      static constexpr int16_t      ahy = 242;
      static constexpr int8_t      font =   4;
      static constexpr int8_t     tsize =   1;
      static constexpr uint16_t fgcolor = TFT_CYAN;   // foreground colour
      static constexpr uint16_t bgcolor = TFT_BLACK;  // backgorund colour
    };

    struct MCs   // Celsius
    {
      static constexpr int16_t        x = 360;
      static constexpr int16_t        y = 186;
      static constexpr int16_t        w = 119;
      static constexpr int16_t        h =  32;
      static constexpr int16_t       tx = 439;
      static constexpr int16_t       ty = 210;
      static constexpr int16_t       cx = 432;
      static constexpr int16_t       cy = 204;
      static constexpr int8_t      font =   4;
      static constexpr int8_t     tsize =   1;
      static constexpr uint16_t fgcolor = TFT_CYAN;   // foreground colour
      static constexpr uint16_t bgcolor = TFT_BLACK;  // backgorund colour
    };

    struct MDu    // Time
    {
      static constexpr int16_t        x = 360;
      static constexpr int16_t        y = 152;
      static constexpr int16_t        w = 119;
      static constexpr int16_t        h =  32;
      static constexpr int16_t       tx = 474;
      static constexpr int16_t       ty = 170;
      static constexpr int8_t   font    =   4;
      static constexpr int8_t   tsize   =   1;
      static constexpr uint16_t fgcolor = TFT_CYAN;   // foreground colour
      static constexpr uint16_t bgcolor = TFT_BLACK;  // backgorund colour
    };

    struct MHeap
    {
      static constexpr int16_t  xpos    =  25;
      static constexpr int16_t  ypos    = 245;
      static constexpr int8_t   font    =   4;  //4;
      static constexpr int8_t   size    =   2;
      static constexpr uint16_t fgcolor = TFT_BLACK;  // ORANGE;   // foreground colour
      static constexpr uint16_t bgcolor = TFT_DARKGREEN;  // backgorund colour
    };

    struct MMw    // Maim window
    {
    static constexpr int16_t          x =   4;  // 
    static constexpr int16_t          y =   4; 
    static constexpr int16_t          w = 352;  // Width and height
    static constexpr int16_t          h = 254;
    static constexpr int16_t          r =  10;
    static constexpr int16_t         tx = w/2+x;  // Центр рамки
    static constexpr int16_t         ty = h/2+y; 
    //static constexpr int16_t      tsize =   2;  //1;
    static constexpr int16_t       font =   2;
    };


    struct ML     // Label - текстовые в главном окне
    {
      static constexpr int16_t        w = MMw::w - 8;       // Ширина поля очистки
      static constexpr int16_t        h = MMw::h / 8 - 1;   // Высота поля очистки
      static constexpr int16_t        x = MMw::x + 4;       // X-Позиция любого поля
      static constexpr int16_t        y = MMw::ty+5 - h * 4;  // Y-Позиция нулевого поля
      static constexpr int8_t      font =   2;
      static constexpr int8_t     tsize =   1;
      static constexpr uint16_t fgcolor = TFT_YELLOW;       // foreground colour
      static constexpr uint16_t bgcolor = TFT_DARKGREEN;    // backgorund colour
    };





    struct MBm    //btnMode
    {
    // KeyMode start position, key sizes and spacing
    static constexpr int16_t          x = 417;  // Centre of key
    static constexpr int16_t          y =  40-4; 
    static constexpr int16_t          w = 112;  // Width and height
    static constexpr int16_t          h =  62;
    static constexpr int16_t      tsize =   1;
    };

    struct MBsg   //btnStopGo
    {
    // KeyMode start position, key sizes and spacing
    static constexpr int16_t          x = 417;  // Centre of key
    static constexpr int16_t          y = 110; 
    static constexpr int16_t          w = 112;  // Width and height
    static constexpr int16_t          h =  70;
    //static constexpr int16_t key_mode_spacing_x =  18;
    //static constexpr int16_t key_mode_spacing_y = 016;
    static constexpr int16_t      tsize =   1;
    };


//================ Кнопки клавиатуры ===============================
    struct MKpad   // Keypad
    {
      // Keypad start position, key sizes and spacing
      static constexpr int16_t        x = 190;        // Centre of key
      static constexpr int16_t        y =  32;
      static constexpr int16_t        w =  50+2;        // Width and height
      static constexpr int16_t        h =  32;
      static constexpr int16_t       sx =  16;        // X and Y gap
      static constexpr int16_t       sy =  16; 
      static constexpr int16_t    tsize =   1;        // Font size multiplier

    };

      // Цвет кнопок (неинвертированные)
    uint16_t btnColor[ num_btns ] = { TFT_BLUE,  TFT_BLUE,  TFT_BLUE,
                                      TFT_BLUE,  TFT_BLUE,  TFT_BLUE,
                                      TFT_BLUE,  TFT_BLUE,  TFT_BLUE,
                                      TFT_BLUE,  TFT_BLUE,  TFT_BLUE,
                                      TFT_BLUE,  TFT_BLUE,  TFT_BLUE,
                                      TFT_BLUE,  TFT_BLUE,  TFT_BLUE,
                                       TFT_RED,   TFT_RED, TFT_OLIVE,
                                  TFT_DARKGREY,  TFT_BLUE,  TFT_BLUE,
                                      TFT_BLUE,  TFT_BLUE,  TFT_BLUE  };

      // Надписи на кнопках
    char btnName[ num_btns ][6] = {   "",        "",      "OK",
                                     "7",       "8",       "9",
                                     "4",       "5",       "6",
                                     "1",       "2",       "3",
                                     ".",       "0",      "<-",
                                 "Start",      "Up",      "Dn",
                                  "Stop",    "Exit",      "Go",
                                   "...",     "Adj",    "Next",
                                  "Back",    "Save",   "Pause" };
//================ Окно ввода ================
    struct MBox   // Numeric display box
    {
      static constexpr int16_t        x = MKpad::x - MKpad::w/2;
      static constexpr int16_t        y = MKpad::y - MKpad::h/2 - 2;
      static constexpr int16_t        w = MKpad::w*2 + MKpad::sx;
      static constexpr int16_t        h = MKpad::h + 4;
      static constexpr int16_t    dsize =   3;
      static constexpr int16_t   dcolor = TFT_CYAN;
    };

    // Number length, buffer for storing it and character index
    static constexpr int16_t num_len = 6;
    char numberBuffer[num_len + 1] = "";
    uint8_t numberIndex = 0;

    // We have a status line for messages
   static constexpr int16_t  status_x = 120; //  Centred on this
   static constexpr int16_t  status_y =  65;

//================ Кнопки управления выбором режима ================
    struct MSel   //btns Mode, Up, Dn
    {
      // KeyMode start position, key sizes and spacing
      static constexpr int16_t        x = 417;
      static constexpr int16_t        y =  26;
      static constexpr int16_t        w = 115;
      static constexpr int16_t        h =  42;
      //static constexpr int16_t       tx =  60+2;
      //static constexpr int16_t       ty =  88;
      static constexpr int16_t       sy =   8;
      static constexpr int8_t      font =   4;
      static constexpr int8_t     tsize =   1;
      static constexpr uint16_t fgcolor = TFT_CYAN;   // foreground colour
      static constexpr uint16_t bgcolor = TFT_BLACK;  // backgorund colour
    };

    uint16_t plan              = SEC;
    unsigned long upSeconds    =   0;
    // char labelString[ 20 ] = { 0 };
    // char modeString[ 20 ]  = { 0 };
    // char helpString[ 20 ]  = { 0 };

    float volt    =  12.6f;   // вольты для отображения
    uint8_t dpV   =  2u;      // знаков после зпт

    float amp     =  5.5f;    // амперы для отображения
    uint8_t dpI   =  2u;      // знаков после зпт

    float celsius =  0.0f;    // градусы
    uint8_t dpC   =  1u;      // 

    float ah      =  0.0f;    // ампер-часы
    uint8_t dpAh  =  1u;      // 

    uint duration =  0u;      // отсчет времени

    uint32_t heap =  0u;
 
    bool activBtns[ num_btns ] = { false }; // массив активированных кнопок

};

#endif  // !_MDISPLAY_H_ 
