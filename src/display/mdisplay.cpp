/*
  pcb:  cpu3.5v7
  Дисплей TFT 3.5" SPI 480*320
  VSPI  MISO  19   // -
  VSPI  MOSI  23   // pin 6
  VSPI  SCLK  18   // pin 7
        CS    32   // pin 3
        DC    15   // pin 5
        RST   04   // pin 4  

      OlMoro, 2023.09.06
*/

#include "TFT_eSPI.h"             // Graphics and font library
#include "mdisplay.h"
#include "board/mboard.h"       // Beep
#include "Free_Fonts.h"
#include <SPI.h>
#include <Preferences.h>
#include <Arduino.h>

    // #include <iostream>
    // #include<string>
    // #include<sstream> // for using stringstream
    // #include <cstring>
    // using namespace std;

#include <iostream>
#include <string>
using namespace std;

extern TaskHandle_t xTask_Display;
extern TaskHandle_t xTask_Main;

MDisplay::MDisplay() : qPreferences(new Preferences)  // ok
{
  tft = new TFT_eSPI();

  tft->init();
  tft->setRotation(3);                          // (3) поворот на 270°
  tft->setTextSize(2);
  tft->setFreeFont(&FreeSansBold12pt7b);
  tft->fillScreen(TFT_BLACK);                   // Цвет фона
  tft->fillRect(0, 0, 479, 319, TFT_DARKGREY);  // Draw screen background

    // Поправочные коэффициенты для тачскрина (учитывая поворот на 270°)
   qPreferences->begin( "device", true );
   multX = qPreferences->getShort( "touchx", TFT_HEIGHT );  // 480
   multY = qPreferences->getShort( "touchy", TFT_WIDTH );   // 320
   qPreferences->end();

    // Рамка выбранного режима
  tft->fillRoundRect( MMw::x, MMw::y, MMw::w, MMw::h, MMw::r, TFT_DARKGREEN );
  tft->drawRoundRect( MMw::x, MMw::y, MMw::w, MMw::h, MMw::r, TFT_GREEN );

    // Рамка выбора режима (малая) - очистка
  tft->fillRoundRect(360, 5, 115, 142, 10, TFT_DARKGREEN);
  tft->drawRoundRect(360, 5, 115, 142, 10, TFT_GREEN);  

    // Рамка V
  tft->fillRect(MV::x, MV::y, MV::w, MV::h, TFT_BLACK);
  tft->drawFastVLine(MV::x, MV::y, MV::h, TFT_WHITE);
  tft->drawFastHLine(MV::x, MV::y, MV::w, TFT_WHITE);
  tft->drawFastVLine(MV::w, MV::y, MV::h, TFT_WHITE);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->drawChar('V', MV::tx, MV::ty);

    // Рамка A
  tft->fillRect(MA::x, MA::y, MA::w, MA::h, TFT_BLACK);
  tft->drawFastVLine(MA::x, MA::y, MA::h, TFT_WHITE);
  tft->drawFastHLine(MA::x, MA::y, MA::w, TFT_WHITE);
  tft->drawFastVLine(MA::w, MA::y, MA::h, TFT_WHITE);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->drawChar('A', MA::tx, MA::ty);

    // Рамка Ah
  tft->fillRect(MAh::x, MAh::y, MAh::w, MAh::h, TFT_BLACK);
  tft->drawRect(MAh::x, MAh::y, MAh::w, MAh::h, TFT_WHITE);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->setTextSize(MAh::tsize);
  tft->drawString("Ah", MAh::tx, MAh::ty);

    // Рамка Celsius
  tft->fillRect(MCs::x, MCs::y, MCs::w, MCs::h, TFT_BLACK);
  tft->drawRect(MCs::x, MCs::y, MCs::w, MCs::h, TFT_WHITE);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->drawChar('o', MCs::tx-2, MCs::ty-6);
  tft->drawChar('C', MCs::tx+12, MCs::ty); 

    // Рамка Time
  tft->fillRect(MDu::x, MDu::y, MDu::w, MDu::h, TFT_BLACK);
  tft->drawRect(MDu::x, MDu::y, MDu::w, MDu::h, TFT_WHITE);
}

MDisplay::~MDisplay()
{ 
  delete tft;
      delete qPreferences;  // ok
}

//   // Очистка поля отображения напряжения
/* Заменено на добавление 3-х пробелов в начале (частный случай) */
// void MDisplay::clearVolt()
// {
//   //tft->drawRect( MV::x+24, MV::y+10, MV::w-106, MV::h-16, TFT_WHITE );
//   tft->fillRect( MV::x+24, MV::y+10, MV::w-106, MV::h-16, TFT_BLACK );
// }

//   // Очистка поля отображения тока
// void MDisplay::clearAmp()
// {

// }



  // Рамка выбранного режима (большая) - очистка
void MDisplay::drawMainWindow()
{
  tft->fillRoundRect( MMw::x, MMw::y, MMw::w, MMw::h, MMw::r, TFT_DARKGREEN );
  tft->drawRoundRect( MMw::x, MMw::y, MMw::w, MMw::h, MMw::r, TFT_GREEN );
}

  // Рамка выбора режима (малая) - очистка
void MDisplay::drawModeWindow()
{
  tft->fillRoundRect(360, 5, 115, 142, 10, TFT_DARKGREEN);
  tft->drawRoundRect(360, 5, 115, 142, 10, TFT_GREEN);  
}

  //
bool MDisplay::getKey( uint8_t key )
{
  if( keyBuff == key )
  {
    keyBuff = KEY_NO;
    return true;
  }
  else
    return false;

//  if(Buttons[b] > 1) {Buttons[b] = KEY_NO; return true;}
//  else  return false;

}

MDisplay::KEY_PRESS MDisplay::getKey()
{
    const KEY_PRESS key = static_cast<KEY_PRESS>(keyBuff);
    keyBuff = KEY_NO; 
    return key;
}



  // Вызывается задачей с периодичностью 125мс. Полное обновление индикации переменных
  // производится за 4 вызова (500мс). 
void MDisplay::runDisplay(float celsius)
{
  static short cnt = -1;

  cnt++;
  if(cnt >= 4) cnt = -1;

  switch (cnt)
  {
    case  0:  printVolt();                break;  // 11 ms
    case  1:  printAmp();                 break;  // 11 ms
    case  2:  printAmpHour();                     //  2 ms
              printCelsius(celsius, 1);   break;  //  2 ms
    case  3:  printHeap();                        //  3 ms
              printDuration();            break;  //  3 ms
    default:                              break;
  }
}

// Активация кнопок

  // Очистка окна
void MDisplay::newMainWin() { drawMainWindow(); }

  // Очистка окна
void MDisplay::newModeWin() { drawModeWindow(); }

/* В drawFloat() добавлены три пробела (проблема артефактов не решена)*/
void MDisplay::printVolt()
{
  tft->setFreeFont( &FreeSans12pt7b );
  tft->setTextDatum( MR_DATUM );                  // Центровка
  tft->setTextSize( MV::tsize );                  // Размер
  tft->setTextColor( MV::fcolor, MV::bgcolor );   // Цвет текста и заливки
  vTaskSuspend( xTask_Main );
  tft->drawFloat( volt, dpV, MV::vx, MV::vy, MV::font );
  vTaskResume( xTask_Main );
}

void MDisplay::printAmp()
{
  tft->setFreeFont( &FreeSans12pt7b );
  tft->setTextDatum( MR_DATUM );                   // Центровка
  tft->setTextSize( MA::tsize );                   // Размер
  tft->setTextColor( MA::fcolor, MA::bgcolor );    // Цвет текста и заливки
  vTaskSuspend( xTask_Main );
  tft->drawFloat( amp, dpI, MA::ax, MA::ay, MA::font );
  vTaskResume( xTask_Main );
}

void MDisplay::printAmpHour()
{
  tft->setFreeFont( &FreeSans12pt7b );
  tft->setTextDatum( MR_DATUM );
  tft->setTextSize( MAh::tsize );
  tft->setTextColor( MAh::fgcolor, MAh::bgcolor );        // Цвет текста и заливки
  tft->drawFloat( ah, dpAh, MAh::ahx, MAh::ahy, MAh::font);
}


void MDisplay::printCelsius(float celsius, uint8_t dpC)
{
  tft->setFreeFont( &FreeSans12pt7b );
  tft->setTextDatum(MR_DATUM);
  tft->setTextSize(MCs::tsize);
  tft->setTextColor(MCs::fgcolor, MCs::bgcolor);    // Цвет текста и заливки
  tft->drawFloat(celsius, dpC, MCs::cx, MCs::cy, MCs::font);
}


void MDisplay::printDuration()
{
  int sec = upSeconds;
  int pl = plan;                   // 0 - hhh:mm:ss,  1 - hhh

  tft->setFreeFont( &FreeSans12pt7b );
  tft->setTextDatum(MR_DATUM);
  tft->setTextSize(MDu::tsize);

  uint16_t xpos = MDu::tx;
  uint16_t ypos = MDu::ty;
  tft->setTextColor( MDu::fgcolor, MDu::bgcolor ); // Set foreground and backgorund colour

  sec = sec % 86400;          // С учетом периода вызова задачи
  unsigned long hours = sec / 3600;
  sec = sec % 3600;
  unsigned long minutes = sec / 60;
  sec = sec % 60;
  
  if( pl == SEC )
  {
    xpos -= tft->drawNumber(sec, xpos, ypos, MDu::font);
    if(sec <= 9) xpos -= tft->drawNumber(0, xpos, ypos, MDu::font);
    xpos -= tft->drawString(":", xpos, ypos, MDu::font);
    xpos -= tft->drawNumber(minutes, xpos, ypos, MDu::font);
    if(minutes <= 9) xpos -= tft->drawNumber(0, xpos, ypos, MDu::font);
    xpos -= tft->drawString(":", xpos, ypos, MDu::font);
    xpos -= tft->drawNumber(hours, xpos, ypos, MDu::font);
  }
  else
  {
    // hours
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    xpos -= tft->drawString( "h", xpos, ypos, MDu::font );
    xpos -= 20;
    tft->setTextColor( MDu::fgcolor, MDu::bgcolor );
    tft->drawNumber( sec/36000, xpos, ypos, MDu::font );
  }       
}



void MDisplay::printHeap()
{
  tft->setTextDatum(ML_DATUM);              // text plotting alignment
  tft->setTextColor( MHeap::fgcolor, MHeap::bgcolor ); // Set foreground and backgorund colour
  tft->drawNumber( ESP.getFreeHeap(), MHeap::xpos, MHeap::ypos, MHeap::font );
}


// ======================== Инициализация кнопок ========================
// 15 кнопок клавиатуры
void MDisplay::initBtnKeypad()
{
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t b = col + row * 3;
      if(b > 1) {
        Buttons[b].initButton( tft, MKpad::x + col * (MKpad::w + MKpad::sx),  // x
                                    MKpad::y + row * (MKpad::h + MKpad::sy),  // y
                                    MKpad::w, MKpad::h,                       // w, h
                                    TFT_WHITE, btnColor[b], TFT_WHITE,        // outline, fill
                                    btnName[b], MKpad::tsize);                // text
      }
    }
  } 
}


// ======================== Активация кнопок ========================

  // Активация группы из 3-х кнопок
void MDisplay::newBtn( uint8_t btn1, uint8_t btn2, uint8_t btn3 )
{
  newBtn( btn1 );   addBtn( btn2 );   addBtn( btn3 );
}

  // Активация группы из 2-х кнопок
void MDisplay::newBtn( uint8_t btn1, uint8_t btn2 )
{
  newBtn( btn1 );   addBtn( btn2 );
}

  // Добавление новой (первой) кнопки
void MDisplay::newBtn( uint8_t btn )
{
  for ( int i = 0; i < num_btns; i++ ) activBtns[i] = false;
  drawModeWindow();
  addBtn( btn );
}

  // Добавление новой (первой) кнопки с признаком нажатия
void MDisplay::newBtnAuto( uint8_t btn )
{
  newBtn( btn );
  Buttons[ btn ].press( true );
}

  // Добавление кнопки
void MDisplay::addBtn( uint8_t btn )
{
  switch ( btn)
  {
  case START:
    Buttons[ START ].initButton ( tft, MSel::x,  // x
                                       MSel::y,  // y
                              MSel::w, MSel::h,  // w, h
       TFT_WHITE, btnColor[ START ], TFT_WHITE,  // outline, fill
               btnName[ START ], MSel::tsize );  // text
    break;

  case UP:
    Buttons[ UP ].initButton ( tft,    MSel::x,  // x
                  MSel::y + MSel::h + MSel::sy,  // y
                              MSel::w, MSel::h,  // w, h
          TFT_WHITE, btnColor[ UP ], TFT_WHITE,  // outline, fill
                  btnName[ UP ], MSel::tsize );  // text
    break;

  case DN:
    Buttons[ DN ].initButton ( tft,    MSel::x,  // x
            MSel::y + 2 * (MSel::h + MSel::sy),  // y
                              MSel::w, MSel::h,  // w, h
          TFT_WHITE, btnColor[ DN ], TFT_WHITE,  // outline, fill
                  btnName[ DN ], MSel::tsize );  // text
    break;

  case STOP:
    Buttons[ STOP ].initButton ( tft, MSel::x,   // x
           MSel::y + 2 * (MSel::h + MSel::sy),   // y
                             MSel::w, MSel::h,   // w, h
       TFT_WHITE, btnColor[ STOP ], TFT_WHITE,   // outline, fill
               btnName[ STOP ], MSel::tsize );   // text
    break;

  case EXIT:
    Buttons[ EXIT ].initButton ( tft, MSel::x,   // x
           MSel::y + 2 * (MSel::h + MSel::sy),   // y
                             MSel::w, MSel::h,   // w, h
       TFT_WHITE, btnColor[ EXIT ], TFT_WHITE,   // outline, fill
               btnName[ EXIT ], MSel::tsize );   // text
    break;

  case GO:
    Buttons[ GO ].initButton ( tft,     MSel::x,  // x
                                        MSel::y,  // y
                               MSel::w, MSel::h,  // w, h
           TFT_WHITE, btnColor[ GO ], TFT_WHITE,  // outline, fill
                   btnName[ GO ], MSel::tsize );  // text
    break;

  case WAIT:
    Buttons[ WAIT ].initButton ( tft,   MSel::x,  // x
                                        MSel::y,  // y
                               MSel::w, MSel::h,  // w, h
         TFT_WHITE, btnColor[ WAIT ], TFT_WHITE,  // outline, fill
                 btnName[ WAIT ], MSel::tsize );  // text
    break;

  case ADJ:
    Buttons[ ADJ ].initButton ( tft,    MSel::x,  // x
                   MSel::y + MSel::h + MSel::sy,  // y
                               MSel::w, MSel::h,  // w, h
          TFT_WHITE, btnColor[ ADJ ], TFT_WHITE,  // outline, fill
                  btnName[ ADJ ], MSel::tsize );  // text  

  case NEXT:
    Buttons[ NEXT ].initButton ( tft,   MSel::x,  // x
                   MSel::y + MSel::h + MSel::sy,  // y
                               MSel::w, MSel::h,  // w, h
         TFT_WHITE, btnColor[ NEXT ], TFT_WHITE,  // outline, fill
                 btnName[ NEXT ], MSel::tsize );  // text
    break;

  case BACK:
    Buttons[ BACK ].initButton ( tft,    MSel::x,  // x
              MSel::y + 2 * (MSel::h + MSel::sy),  // y
                                MSel::w, MSel::h,  // w, h
          TFT_WHITE, btnColor[ BACK ], TFT_WHITE,  // outline, fill
                  btnName[ BACK ], MSel::tsize );  // text
    break;

  case SAVE:
    Buttons[ SAVE ].initButton ( tft,    MSel::x,  // x
                                         MSel::y,  // y
                                MSel::w, MSel::h,  // w, h
          TFT_WHITE, btnColor[ SAVE ], TFT_WHITE,  // outline, fill
                  btnName[ SAVE ], MSel::tsize );  // text
    break;

  case PAUSE:
    Buttons[ PAUSE ].initButton ( tft,   MSel::x,  // x
                                         MSel::y,  // y
                                MSel::w, MSel::h,  // w, h
         TFT_WHITE, btnColor[ PAUSE ], TFT_WHITE,  // outline, fill
                 btnName[ PAUSE ], MSel::tsize );  // text
    break;
    // 

  default:
    break;
  }

  activBtns[ btn ] = true;
  tft->setFreeFont( &FreeSansBold12pt7b );
  Buttons[ btn ].drawButton();
  Buttons[ btn ].press( false );  // Статус ненажатой кнопки
}


void MDisplay::clearLine( uint8_t line )
{
  tft->fillRect( ML::x, ML::y + ML::h * line, ML::w, ML::h, TFT_DARKGREEN );
}

  // Очистка нескольких строк в главном окне
void MDisplay::clearLine( uint8_t init, uint8_t last )
{
  uint8_t line = init;
  while ( line <= last ) { clearLine( line );  line++; }
}


void MDisplay::drawLabel( const char *s, uint8_t line )
{
  if ( line > 7 ) line = 7;
  tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );  
  tft->setTextDatum( CC_DATUM );
  tft->setTextColor( TFT_YELLOW );
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx, ty );
}

void MDisplay::drawLabel( const char *s, uint8_t line, bool )
{
  if ( line > 7 ) line = 7;
  tft->setTextDatum( CC_DATUM );
   tft->setFreeFont( &FreeSansBold12pt7b );
   tft->setTextSize( 1 );
  tft->setTextColor( TFT_YELLOW );
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx, ty );
}

void MDisplay::drawAdj( const char *s, uint8_t line )             // 
{
  tft->setFreeFont(&FreeSansBold12pt7b);
  tft->setTextSize( 1 );
  tft->setTextDatum( CR_DATUM );
  tft->setTextColor( TFT_YELLOW );
  clearLine( line );
  uint16_t ty = (MMw::ty - 100) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
}

void MDisplay::drawAdj( const char *s, uint8_t line, uint8_t pnt ) //
{
  tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );
  tft->setTextDatum( CR_DATUM );
  if ( line == 0 || line != pnt )  tft->setTextColor( TFT_YELLOW );
  else                             tft->setTextColor( TFT_WHITE );
  clearLine( line );
  uint16_t ty = (MMw::ty - 100) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
}



void MDisplay::drawParam( const char *s, uint8_t line, float par, uint8_t dp )
{
  tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );
  tft->setTextDatum( CR_DATUM );
  tft->setTextColor( TFT_YELLOW );
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
  tft->setTextDatum( CL_DATUM );
  tft->drawFloat( par, dp, MMw::tx + 70, ty );
}

void MDisplay::drawParam( const char *s, uint8_t line, float par, uint8_t dp, uint8_t pnt )
{
  tft->setTextDatum( CR_DATUM );
  tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );
  if ( line == 0 || line != pnt )  tft->setTextColor( TFT_YELLOW );
  else                             tft->setTextColor( TFT_WHITE );
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
  tft->setTextDatum( CL_DATUM );
  tft->drawFloat( par, dp, MMw::tx + 70, ty );
}

void MDisplay::drawParFl(const char *s, uint8_t line, short par, uint8_t dp)
{
  tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );
  tft->setTextDatum( CR_DATUM );
  tft->setTextColor( TFT_YELLOW );
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
  tft->setTextDatum( CL_DATUM );
  tft->drawFloat( (float)par/1000, dp, MMw::tx + 70, ty );
}
void MDisplay::drawParFl(const char *s, uint8_t line, short par, uint8_t dp, uint8_t pnt)
{
  tft->setTextDatum( CR_DATUM );
  tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );
  if ( line == 0 || line != pnt )  tft->setTextColor( TFT_YELLOW );
  else                             tft->setTextColor( TFT_WHITE );
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
  tft->setTextDatum( CL_DATUM );
  tft->drawFloat( (float)par/1000, dp, MMw::tx + 70, ty );
}




//void MDisplay::drawParam( const char *s, uint8_t line, short par )
void MDisplay::drawShort( const char *s, uint8_t line, short par )
{
  tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );
  tft->setTextDatum(CR_DATUM);
  tft->setTextColor(TFT_YELLOW);
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
  tft->setTextDatum(CL_DATUM);
  tft->drawNumber( par, MMw::tx + 70, ty );
}

  // Отображение параметра в строке меню с выделением белым цветом
//void MDisplay::drawParam( const char *s, uint8_t line, short par, uint8_t pnt  )
void MDisplay::drawShort( const char *s, uint8_t line, short par, uint8_t pnt  )
{
   tft->setFreeFont( &FreeSansBold12pt7b );
  tft->setTextSize( 1 );
  tft->setTextDatum(CR_DATUM);
  if ( line == 0 || line != pnt )  tft->setTextColor(TFT_YELLOW);
  else                             tft->setTextColor(TFT_WHITE);
  clearLine( line );
  uint16_t ty = ( MMw::ty - 100 ) + line * 30;
  tft->drawString( s, MMw::tx + 60, ty );
  tft->setTextDatum(CL_DATUM);
  tft->drawNumber( par, MMw::tx + 70, ty );
}

  //
void MDisplay::showVolt( float _volt, uint8_t _dp )
{
  volt    = _volt;
  dpV = _dp;
}

void MDisplay::showAmp( float _amp, uint8_t _dp )
{
  amp     = _amp;
  dpI = _dp;
}

void MDisplay::showAh( float val )
{ 
  ah = val;
} 

void MDisplay::showCelsius( float val )
{ 
  celsius = val;
} 

void MDisplay::showDuration( int duration, int _plan )
{
  upSeconds = duration;
  plan = _plan;                   // 0 - hhh:mm:ss,  1 - hhh
}

void MDisplay::showHeap()
{
  heap = ESP.getFreeHeap();
}

//void MDisplay::showMode(char *s) { strcpy( modeString, s ); }


// ============================= ILI9486 ===================================

void MDisplay::calcKeys()
{
  static uint8_t buf = 0;

  if ( buzy ) return;

  uint16_t t_x = 0;
  uint16_t t_y = 0; // To store the touch coordinates

  // Pressed will be set true is there is a valid touch on the screen
  boolean pressed = tft->getTouch(&t_x, &t_y);

  // Искусственный прием исправления калибровки дисплея
  if(pressed)
  {
    //Serial.print("Pressed = ");  //  Serial.print(pressed);
    //Serial.print("x=");  Serial.print(t_x);
    //Serial.print("\ty=");  Serial.println(t_y);
    t_y += t_y >> 2;    // Moro: +25%

    t_x = ( t_x * multX ) / TFT_HEIGHT;
    t_y = ( t_y * multY ) / TFT_WIDTH;
    //Serial.print("\nmultY=");  Serial.println(multY);
  }

  // Check if any key coordinate boxes contain the touch coordinates
  
  for ( uint8_t b = START; b < num_btns ; b++ )    // без клавиатуры
  {
    if ( !activBtns[b] ) continue;
    if ( pressed && Buttons[b].contains(t_x, t_y) )
      Buttons[b].press(true); 
    else  
      Buttons[b].press(false);

    // Check if this key has changed state
    tft->setFreeFont( &FreeSansBold12pt7b );

    if (Buttons[b].justReleased())
    {
      vTaskSuspend( xTask_Display );
      Buttons[b].drawButton();                  // Вернуть после инвертирования
      keyBuff = buf;                            // Код нажатой кнопки на исполнение
      vTaskResume( xTask_Display );
    }
    if (Buttons[b].justPressed())
    {
      vTaskSuspend( xTask_Display );
      Buttons[b].drawButton(true);              // Инвертировать
      Board->buzzerOn();
      buf = b;                                  // Код нажатой кнопки сохранить до отпускания
    #ifdef PRINT_BUTTON
      Serial.print("b=");  Serial.println(b);
    #endif
      vTaskResume( xTask_Display );
    }
  }

}

// Print something in the mini status bar
void MDisplay::status(const char *msg) {
  tft->setTextPadding(240);
  //tft->setCursor(STATUS_X, STATUS_Y);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft->setTextFont(0);
  tft->setTextDatum(TC_DATUM);
  tft->setTextSize(1);
  tft->drawString(msg, status_x, status_y);
}
