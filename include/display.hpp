#ifndef _DISPLAY_HPP_
#define _DISPLAY_HPP_

//#include <U8x8lib.h>
#include <U8g2lib.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif


//U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16); 
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

class Display {
  public:
    int lineH = 9;
    bool serialCopy;
    Display (bool serialCopy = false) {
        this->serialCopy = serialCopy;
    }
    void init() {
        /*
        u8x8.begin();
        u8x8.setPowerSave(0);
        u8x8.setFont(u8x8_font_chroma48medium8_r);
        */
        u8g2.begin();
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.clearBuffer();
    }
    void print(int linea, const char * s, bool refresh=true) {
        /*
        u8x8.drawString(0, linea ,"               " );
        u8x8.drawString(0, linea ,s );
        u8x8.refreshDisplay();		// only required for SSD1606/7  
        */
        int linePX = lineH * linea;
        u8g2.setDrawColor(0);
        u8g2.drawBox(0,linePX-lineH, 128, lineH);
        u8g2.setDrawColor(1);
        u8g2.drawStr(0,linePX,s);
        if (refresh) u8g2.sendBuffer();
        if (serialCopy){
            Serial.println(s);
        }
    }

};

#endif

