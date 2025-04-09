/*
CIV_template - z_userprog - adapted for the requirements of Phillip, VK3PE by K6PSM, Dec 1, 24
    vk3pe is using a "TTGO" ESP32 module.
    Band select BCD outputs set to be active Hi.NOTE: a BCD to Decimal chip will be used also
     to provide 10 band outputs.
    PTT output is active LOW

This is the part, where the user can put his own procedures in

The calls to this user programs shall be inserted wherever it suits - search for //!//
in all files

*/
#include <spi.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>       //using this LIB now.  https://github.com/Bodmer/TFT_eSPI   
#include "Free_Fonts.h"
#include <JPEGDecoder.h>
#include "BT.h"
#define minimum(a,b) ((a)<(b)?(a):(b))
//#include <avr/pgmspace.h>
// IMPORTANT!  
//      In the "User_Setup_Select.h" file, enable "#include <User_Setups/Setup25_TTGO_T_Display.h>"

//=========================================================================================
// user part of the defines

// if defined, the bit pattern of the output pins is inverted in order to compensate
// the effect of inverting HW drivers (active, i.e.uncommented by default)
#define invDriver         //if active, inverts band BCD out
#define Inv_PTT           //if active, PTT out is Low going.




#define VERSION_USER "usrprg K6PSM V0_5 Mar 24th, 2025"

#define NUM_BANDS 14   /* Number of Bands (depending on the radio) */

//-----------------------------------------------------------------------------------------
//for TFT
TFT_eSPI tft = TFT_eSPI();

#define screen_width  320      //placement of text etc must fit withing these boundaries.
#define screen_heigth 240

//all my known colors for ST7789 TFT (but not all used in program)
#define B_DD6USB 0x0004    //   0,   0,   4  my preferred background color !!!   now vk3pe ?
#define BLACK 0x0000       //   0,   0,   0
#define NAVY 0x000F        //   0,   0, 123
#define DARKGREEN 0x03E0   //   0, 125,   0
#define DARKCYAN 0x03EF    //   0, 125, 123
#define MAROON 0x7800      // 123,   0,   0
#define PURPLE 0x780F      // 123,   0, 123
#define OLIVE 0x7BE0       // 123, 125,   0
#define LIGHTGREY 0xC618   // 198, 195, 198
#define DARKGREY 0x7BEF    // 123, 125, 123
#define BLUE 0x001F        //   0,   0, 255
#define GREEN 0x07E0       //   0, 255,   0
#define CYAN 0x07FF        //   0, 255, 255
#define RED 0xF800         // 255,   0,   0
#define MAGENTA 0xF81F     // 255,   0, 255
#define YELLOW 0xFFE0      // 255, 255,   0
#define WHITE 0xFFFF       // 255, 255, 255
#define ORANGE 0xFD20      // 255, 165,   0
#define GREENYELLOW 0xAFE5 // 173, 255,  41
#define PINK 0xFC18        // 255, 130, 198
//*************************************************************

//=================================================
// Mapping of port-pins to functions on ESP32 TTGO
//=================================================

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

//SPIClass touchscreenSPI = SPIClass(VSPI);
//XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define PTTpin    17      //PTT out pin
#define PTTpinHF  23      //PTT out HF
#define PTTpinVHF 33      //PTT out VHF
#define PTTpinUHF 32      //PTT out UHF

boolean HF_ptt_Enable;
boolean VHF_ptt_Enable;
boolean UHF_ptt_Enable;







//=========================================================================================
// user part of the database
// e.g. :
uint8_t         G_currentBand = NUM_BANDS;  // Band in use (default: not defined)
//HF_ptt_Enable = 1;


//=====================================================
// this is called, when the RX/TX state changes ...
//=====================================================
void  userPTT(uint8_t newState) {
   unsigned long newState1;
  newState1=G_radioOn; 
  Serial.println(newState1);
#ifdef debug
    Serial.println(newState);    
    Serial.println(newState);                  //prints '1' for Tx, '0' for Rx
#endif
    //tft.setFreeFont(FF23);        //previous setup text was smaller.
    //tft.setTextColor(WHITE) ;
    //tft.setTextSize(2);

    if (newState) {                                    // '1' = Tx mode
        tft.setCursor(37, 80);
        tft.setFreeFont(FF24);
        tft.fillRect(10, 40, 120, 50, RED);            //Erase Rx mode.  vk3pe x,y,width,height,colour 10,40,137,40  
        tft.setTextColor(WHITE);
        tft.print("TX");
        
    }   //Tx mode
    else {
      //tft.setFreeFont(FF4); 
        tft.setCursor(37, 80);
        tft.setFreeFont(FF24);
        tft.fillRect(10, 40, 120, 50, GREEN);          //erase Tx mode:    location x, y, width, height,  colour
       tft.setTextColor(WHITE);                        // Better contrast
        tft.print("RX");
        
    } //Rx mode

    

#ifdef Inv_PTT 
    digitalWrite(PTTpin, !newState);    //--inverted-- output version:  Clr =Tx, Hi =Rx  
    if (HF_ptt_Enable) {
        digitalWrite(PTTpinHF, !newState);
    }
    if (VHF_ptt_Enable) {
        digitalWrite(PTTpinVHF, !newState);
    }
    if (UHF_ptt_Enable) {
        digitalWrite(PTTpinUHF, !newState);
    }
#else
    digitalWrite(PTTpin, newState);    // Clr =Rx, Hi =Tx 
    if (HF_ptt_Enable) {
        digitalWrite(PTTpinHF, newState);
    }
    if (VHF_ptt_Enable) {
        digitalWrite(PTTpinVHF, newState);
    }
    if (UHF_ptt_Enable) {
        digitalWrite(PTTpinUHF, newState);
    }
#endif 
}

//=========================================================================================
// creating bandinfo based on the frequency info

void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

  int x = xpos;
  int y = ypos;

  JpegDec.decodeArray(arrayname, array_size);
  
  //jpegInfo(); // Print information from the JPEG file (could comment this line out)
  
  renderJPEG(x, y);
  
  Serial.println("#########################");
}
void renderJPEG(int xpos, int ypos) {

  // retrieve information about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;


  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.readSwappedBytes()) {
	  
    // save a pointer to the image block
    pImg = JpegDec.pImage ;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
    {
      tft.pushRect(mcu_x, mcu_y, win_w, win_h, pImg);
    }
    else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;

  // print the results to the serial port
  Serial.print(F(  "Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
  Serial.println(F(""));
}



//-----------------------------------------------------------------------------------------
// tables for band selection and bittpattern calculation

//for IC-705 which has no 60M band:
//---------------------------------
// !!! pls adapt "NUM_BANDS" if changing the number of entries in the tables below !!!

// lower limits[kHz] of the bands: NOTE, these limits may not accord with band  edges in your country.
constexpr unsigned long lowlimits[NUM_BANDS] = {
  1800, 3500, 5330,  7000,  10100, 14000, 18068, 21000, 24890, 28000, 50000, 118000, 144000 , 430000
};
// upper limits[kHz] of the bands:  //see NOTE above.
constexpr unsigned long uplimits[NUM_BANDS] = {
  2000, 3999, 5404,  7299, 10149, 14349, 18167, 21449, 24989, 29699, 53999 ,136000, 148000 , 470000
};

const String(mod2string[10]) = {
    // 160     80   60    40     30      20     17      15     12     10      6     NDEF
      "  LSB","  USB", "   AM",  "   CW"," RTTY", "   FM"," WFM", "   CW-R","RTTR", "  DV"

};

// "xxM" display for the TFT display. ie show what band the unit is current on in "meters"
const String(band2string[NUM_BANDS + 1]) = {
    // 160     80   60    40     30      20     17      15     12     10      6     NDEF
      "160m"," 80m", " 60m",  " 40m"," 30m", " 20m"," 17m", " 15m"," 12m"," 10m","  6m", " AIR","  2m","70cm" ," OOB"

};



//------------------------------------------------------------
// set the bitpattern in the HW

void set_HW(uint8_t BCDsetting) {

   

#ifdef debug
    // Test output to control the proper functioning:
    Serial.print(" Pins ");
    #endif

}

//-----------------------------------------------------------------------------------------
// get the bandnumber matching to the frequency (in kHz)

byte get_Band(unsigned long frq) {
    byte i;
    for (i = 0; i < NUM_BANDS; i++) {
        //for (i=1; i<NUM_BANDS; i++) {   
        if ((frq >= lowlimits[i]) && (frq <= uplimits[i])) {
            return i;
        }
    }
    return NUM_BANDS; // no valid band found -> return not defined
}

//------------------------------------------------------------------
//    Show frequency in 'kHz' and band in 'Meters' text on TFT vk3pe
//------------------------------------------------------------------
void show_Meters(void)
{

    // Show Freq[KHz]
    tft.setCursor(50, 215);                //- 
    
    tft.setTextColor(WHITE);               //-
    //tft.setFreeFont(FF19);
    tft.setFreeFont(FF23);
    tft.print(band2string[G_currentBand]); //-
    //tft.print(mod2string[G_Mod]);
   //tft.setFreeFont(&FreeSans9pt7b); //bigger numbers etc from now on. <<<<<<<<-------------------
   
     

}



//------------------------------------------------------------
// process the frequency received from the radio
//------------------------------------------------------------

void set_PAbands(unsigned long frequency) {
    unsigned long freq_kHz;
    
    freq_kHz = G_frequency / 1000;            // frequency is now in kHz
    G_currentBand = get_Band(freq_kHz);     // get band according the current frequency
    //tft.setFreeFont(FF19);
    tft.setFreeFont(FF23);
    //tft.setFreeFont(&FreeSans9pt7b); //bigger numbers etc from now on. <<<<<<<<-------------------
    //tft.setTextSize(5);
    tft.setCursor(50, 150);                 // for bigger print size
    //already use white from previous :-
    tft.fillRect(10, 100, 180, 130, BLACK);
    tft.setTextColor(WHITE);               // at power up not set!

    
    tft.print(frequency / 1000);             //show Frequency in kHz
    tft.setCursor(280, 10); 
    drawArrayJpeg(BT,  sizeof(BT), 280, 5); // Draw a jpeg image stored in memory
    //set_mod();
    //drawArrayJpeg(BT,  sizeof(BT), 280, 10);
#ifdef debug
    // Test-output to serial monitor:
    Serial.print("Frequency: ");  Serial.println(freq_kHz);
    Serial.print("Band: ");     Serial.println(G_currentBand);
    Serial.println(band2string[G_currentBand]);
    
    
 #endif

    // MR Hier stellen we het voltage in
        int bandcode;
    bandcode = G_currentBand;
   
    switch (bandcode) {
    case 0:  // 160M
        bandvoltage=23;
        
        break;
    case 1:  // 80M
        bandvoltage = 46;
        break;
    case 2:  // 60M
        bandvoltage = 69;
        break;
    case 3:  // 40M
        bandvoltage = 92;
        break;
    case 4:  // 30M
        bandvoltage = 115;
        break;
    case 5:  // 20M
        bandvoltage = 138;
        break;
    case 6:  // 17M
        bandvoltage = 161;
        break;
    case 7:  // 15M
        bandvoltage = 184;
        break;
    case 8:  // 12M
        bandvoltage = 207;
        break;
    case 9:  // 10M
        bandvoltage = 230;
        break;
    case 10:  // 6M
        bandvoltage = 253;
        break;
    case 11:  // 2M
        bandvoltage = 0;
        break;
    case 12:  // 70CM
        bandvoltage = 0;
        break;
    case 13:  // NDEF
        bandvoltage = 0;
        break;

    }

    
    if (freq_kHz > 0 && freq_kHz < 60000) {
        digitalWrite(C_RELAIS, LOW);
        HF_ptt_Enable = 1;
        VHF_ptt_Enable = 0;
        UHF_ptt_Enable = 0;

        
    }

    else if
        (freq_kHz > 144000 && freq_kHz < 148000) {
        digitalWrite(C_RELAIS, HIGH);
        HF_ptt_Enable = 0;
        VHF_ptt_Enable = 1;
        UHF_ptt_Enable = 0;

    }
    else if
        (freq_kHz > 430000 && freq_kHz < 470000) {
        digitalWrite(C_RELAIS, HIGH);
        HF_ptt_Enable = 0;
        VHF_ptt_Enable = 0;
        UHF_ptt_Enable = 1;

    }



    Serial.print("Bandvoltage ");      Serial.println(bandvoltage);
    ledcWrite(LED, (bandvoltage * 1023/330));
    Serial.print ("Mod:  "); Serial.println   (G_Mod);
   

 


    show_Meters();            //Show frequency in kHz and band in Meters (80m etc) on TFT
    
}

//=========================================================================================
// this is called, whenever there is new frequency information ...
void userMod(unsigned long Mod) {
    //unsigned long freq_kHz;
   Mod=G_Mod;
tft.setCursor(180, 78); 
  //tft.setTextSize(3);
  tft.setFreeFont(FF23);
  //tft.fillRect(180, 40, 200, 50, BLACK);
  tft.setTextColor(WHITE);
  tft.fillRect(175, 40, 120 , 50, BLUE);
  tft.print(mod2string[Mod]);
}

    //set_mod();
// this is available in the global variable "G_frequency" ...

void userFrequency(unsigned long newFrequency) {

    set_PAbands(G_frequency);

}

// void userMod(unsigned long newModMode) {
// void userBT(void){
//  unsigned long newState1;
//  newState1=G_radioOn;

//}
// // }
//-----------------------------------------------------------------------------------------
// initialise the TFT display
//-----------------------------------------------------------------------------------------

void init_TFT(void)
{
    //tft.init(screen_heigth, screen_width) ;  //not used
    
    //tft.init();
    //tft.invertDisplay(1);
    //pinMode(TFT_BL, OUTPUT);
    //digitalWrite(TFT_BL, HIGH);              // switch backlight on
    tft.setFreeFont(FF33); 
    tft.fillScreen(BLACK);
    tft.setRotation(1);
    tft.fillRoundRect(0, 0, 260, 30, 5, BLACK);   // background for screen title
    tft.drawRoundRect(0, 0, 260, 30, 5, WHITE);    //with white border.

    //tft.setTextSize(2);                  //for default Font only.Font is later changed.
    tft.setTextColor(WHITE);
    tft.setCursor(25, 20);                //top line
    tft.print("IC-705 XPA125B Link");
    //tft.fillRect(10, 100, 300, 130, WHITE);
    //tft.fillRoundRect(0, 60, tft.width(), 15, 5, WHITE);
    //tft.setFreeFont(FF19);
    tft.setFreeFont(FF23);
    tft.setTextColor(WHITE);            //white from now on
    //tft.setTextSize(3);
    tft.setCursor(200, 150);               //
    tft.print("kHz");
    tft.setCursor(150, 95) ; 
    tft.setCursor(200, 215);             //
    tft.print("Band");                   //"160m" etc   or Out if invalid Freq. for Ham bands.
   
}

//=========================================================================================
// this will be called in the setup after startup
void  userSetup() {

    Serial.println(VERSION_USER);

    // set the used HW pins (see defines.h!) as output and set it to 0V (at the Input of the PA!!) initially

    pinMode(PTTpin, OUTPUT);     //PTT out pin
    pinMode(PTTpinHF, OUTPUT);   //PTTHF out pin
    pinMode(PTTpinVHF, OUTPUT);  //PTTVHF out pin
    pinMode(PTTpinUHF, OUTPUT);  //PTTUHF out pin

    pinMode(C_RELAIS, OUTPUT);   // Coax Relais HF / VHF-UHF
    digitalWrite(PTTpin, LOW);       //set 'Rx mode' > high
    digitalWrite(PTTpinHF, HIGH);
    digitalWrite(PTTpinVHF, LOW);
    digitalWrite(PTTpinUHF, LOW);
 
    tft.init();
    tft.invertDisplay(1);
    tft.setRotation(1);
    tft.fillRect(0, 0, 320, 240, WHITE);
    tft.setFreeFont(FF33);
    tft.setTextColor(BLACK);
    tft.setCursor(120, 200);  
    tft.print("BT Link Rev 0.4"); 
    tft.setCursor(280, 10); 
    drawArrayJpeg(BT,  sizeof(BT), 120, 120);
    delay(4000); 
    init_TFT();
    
    userPTT(0);  // initialize the "RX" symbol in the screen
    //userBT();
}

//-------------------------------------------------------------------------------------
// this will be called in the baseloop every BASELOOP_TICK[ms]
void  userBaseLoop() {

}
