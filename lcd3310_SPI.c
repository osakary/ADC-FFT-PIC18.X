/*************************************************************************
 *
 *    Driver for the Nokia 3310 LCD
 *
 **************************************************************************/
#include <p18cxxx.h>
#include <stdio.h>
#include <stdlib.h>
#include <spi.h>
#include "lcd3310.h"


/* TO IMPLEMENT YOUR VERSION OF THE DRIVER YOU'LL HAVE TO EDIT THIS SECTION ONLY */

#define LCD_RES_MAKE_OUT()		TRISCbits.RC0 = 0
#define LCD_RES_HIGH()			LATCbits.LATC0 = 1
#define LCD_RES_LOW()			LATCbits.LATC0 = 0

#define LCD_CS_MAKE_OUT()		TRISCbits.RC1 = 0
#define LCD_CS_HIGH()  			LATCbits.LATC1 = 1
#define LCD_CS_LOW()  			LATCbits.LATC1 = 0

#define LCD_DC_MAKE_OUT()		TRISCbits.RC2 = 0
#define LCD_DC_HIGH()			LATCbits.LATC2 = 1
#define LCD_DC_LOW()			LATCbits.LATC2 = 0

//define SEND_BYTE_SPI()                 \
//	{				\
//		while(WriteSPI1(data)); \
//                Delay10TCYx(20);        \
//	}                               \


static void Initialize_SPI(void)
{
    // nothing
}

/* END OF SECTION */


#define LCD_START_LINE_ADDR	(66-2)

#if LCD_START_LINE_ADDR	> 66
  #error "Invalid LCD starting line address"
#endif

// LCD memory index
unsigned int  LcdMemIdx;

// represent LCD matrix
unsigned char  LcdMemory[LCD_CACHE_SIZE];

const unsigned char  FontLookup [][6] =             // 6 pixels wide
{
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // sp
    { 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00 },   // !
    { 0x00, 0x07, 0x00, 0x07, 0x00, 0x00 },   // "
    { 0x14, 0x7f, 0x14, 0x7f, 0x14, 0x00 },   // #
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12, 0x00 },   // $
    { 0xc4, 0xc8, 0x10, 0x26, 0x46, 0x00 },   // %
    { 0x36, 0x49, 0x55, 0x22, 0x50, 0x00 },   // &
    { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00 },   // '
    { 0x00, 0x1c, 0x22, 0x41, 0x00, 0x00 },   // (
    { 0x00, 0x41, 0x22, 0x1c, 0x00, 0x00 },   // )
    { 0x14, 0x08, 0x3E, 0x08, 0x14, 0x00 },   // *
    { 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00 },   // +
    { 0x00, 0x00, 0x50, 0x30, 0x00, 0x00 },   // ,
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 },   // -
    { 0x00, 0x60, 0x60, 0x00, 0x00, 0x00 },   // .
    { 0x20, 0x10, 0x08, 0x04, 0x02, 0x00 },   // /
    { 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 },   // 0
    { 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 },   // 1
    { 0x42, 0x61, 0x51, 0x49, 0x46, 0x00 },   // 2
    { 0x21, 0x41, 0x45, 0x4B, 0x31, 0x00 },   // 3
    { 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 },   // 4
    { 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 },   // 5
    { 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00 },   // 6
    { 0x01, 0x71, 0x09, 0x05, 0x03, 0x00 },   // 7
    { 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 },   // 8
    { 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00 },   // 9
    { 0x00, 0x36, 0x36, 0x00, 0x00, 0x00 },   // :
    { 0x00, 0x56, 0x36, 0x00, 0x00, 0x00 },   // ;
    { 0x08, 0x14, 0x22, 0x41, 0x00, 0x00 },   // <
    { 0x14, 0x14, 0x14, 0x14, 0x14, 0x00 },   // =
    { 0x00, 0x41, 0x22, 0x14, 0x08, 0x00 },   // >
    { 0x02, 0x01, 0x51, 0x09, 0x06, 0x00 },   // ?
    { 0x32, 0x49, 0x59, 0x51, 0x3E, 0x00 },   // @
    { 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00 },   // A
    { 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00 },   // B
    { 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 },   // C
    { 0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00 },   // D
    { 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00 },   // E
    { 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00 },   // F
    { 0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00 },   // G
    { 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00 },   // H
    { 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00 },   // I
    { 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00 },   // J
    { 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00 },   // K
    { 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00 },   // L
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00 },   // M
    { 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00 },   // N
    { 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00 },   // O
    { 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00 },   // P
    { 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00 },   // Q
    { 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00 },   // R
    { 0x46, 0x49, 0x49, 0x49, 0x31, 0x00 },   // S
    { 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00 },   // T
    { 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00 },   // U
    { 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00 },   // V
    { 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00 },   // W
    { 0x63, 0x14, 0x08, 0x14, 0x63, 0x00 },   // X
    { 0x07, 0x08, 0x70, 0x08, 0x07, 0x00 },   // Y
    { 0x61, 0x51, 0x49, 0x45, 0x43, 0x00 },   // Z
    { 0x00, 0x7F, 0x41, 0x41, 0x00, 0x00 },   // [
    { 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x00 },   // 55
    { 0x00, 0x41, 0x41, 0x7F, 0x00, 0x00 },   // ]
    { 0x04, 0x02, 0x01, 0x02, 0x04, 0x00 },   // ^
    { 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 },   // _
    { 0x00, 0x01, 0x02, 0x04, 0x00, 0x00 },   // '
    { 0x20, 0x54, 0x54, 0x54, 0x78, 0x00 },   // a
    { 0x7F, 0x48, 0x44, 0x44, 0x38, 0x00 },   // b
    { 0x38, 0x44, 0x44, 0x44, 0x20, 0x00 },   // c
    { 0x38, 0x44, 0x44, 0x48, 0x7F, 0x00 },   // d
    { 0x38, 0x54, 0x54, 0x54, 0x18, 0x00 },   // e
    { 0x08, 0x7E, 0x09, 0x01, 0x02, 0x00 },   // f
    { 0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00 },   // g
    { 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00 },   // h
    { 0x00, 0x44, 0x7D, 0x40, 0x00, 0x00 },   // i
    { 0x20, 0x40, 0x44, 0x3D, 0x00, 0x00 },   // j
    { 0x7F, 0x10, 0x28, 0x44, 0x00, 0x00 },   // k
    { 0x00, 0x41, 0x7F, 0x40, 0x00, 0x00 },   // l
    { 0x7C, 0x04, 0x18, 0x04, 0x78, 0x00 },   // m
    { 0x7C, 0x08, 0x04, 0x04, 0x78, 0x00 },   // n
    { 0x38, 0x44, 0x44, 0x44, 0x38, 0x00 },   // o
    { 0x7C, 0x14, 0x14, 0x14, 0x08, 0x00 },   // p
    { 0x08, 0x14, 0x14, 0x18, 0x7C, 0x00 },   // q
    { 0x7C, 0x08, 0x04, 0x04, 0x08, 0x00 },   // r
    { 0x48, 0x54, 0x54, 0x54, 0x20, 0x00 },   // s
    { 0x04, 0x3F, 0x44, 0x40, 0x20, 0x00 },   // t
    { 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00 },   // u
    { 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00 },   // v
    { 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00 },   // w
    { 0x44, 0x28, 0x10, 0x28, 0x44, 0x00 },   // x
    { 0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00 },   // y
    { 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00 },   // z
    { 0x08, 0x6C, 0x6A, 0x19, 0x08, 0x00 },   // { (gramotevichka)
    { 0x0C, 0x12, 0x24, 0x12, 0x0C, 0x00 },   // | (sarce)
    { 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00 },    // kvadratche
};


// simple delays
void Delay(volatile unsigned long a) { while (a!=0) a--; }

void Delayc(volatile unsigned char a) { Delay(a); }
    //while (a!=0) a--; }

/****************************************************************************/
/*  Init LCD Controler                                                      */
/*  Function : LCDInit                                                      */
/*      Parameters                                                          */
/*          Input   :  Nothing                                              */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDInit(void)
{  	
	// Initialie SPI Interface
    Initialize_SPI();
	
	// set pin directions
	LCD_CS_MAKE_OUT();
	LCD_CS_HIGH();
	LCD_DC_MAKE_OUT();
	LCD_RES_MAKE_OUT();
	
	// Toggle reset pin
	LCD_RES_LOW();
	Delay(1000);
	LCD_RES_HIGH();		
	Delay(1000);	
	
	// Send sequence of command
	LCDSend( 0x21, SEND_CMD );  // LCD Extended Commands.
	LCDSend( 0xc0, SEND_CMD );  // Set LCD Vop (Contrast). 0xC8
        LCDSend( 0x06, SEND_CMD);    // temp coef
	LCDSend( 0x12, SEND_CMD );  // LCD bias mode 1:68.
	LCDSend( 0x20, SEND_CMD );  // LCD Standard Commands, Horizontal addressing mode.
        LCDSend(0x09, SEND_CMD);  // all on (display black)
        Delay(750);
	LCDSend( 0x08, SEND_CMD );  // LCD blank
        Delay(50);
	LCDSend( 0x0C, SEND_CMD );  // LCD in normal mode.
	
	// Clear and Update
	LCDClear();
	LCDUpdate();
}

/****************************************************************************/
/*  Reset LCD 	                                                            */
/*  Function : LCDReset                                                     */
/*      Parameters                                                          */
/*          Input   :  Resets the LCD module		                        */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDReset(void)
{

	// Close SPI module - optional
	// NOT DONE
	
	LCD_RES_LOW();
}

/****************************************************************************/
/*  Update LCD                                                              */
/*  Function : LCDUpdate                                                    */
/*      Parameters                                                          */
/*          Input   :  sends buffer data to the LCD                         */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDUpdate ( void )
{
	int x,y;
	
	for(y=0; y<(LCD_Y_RES / 8); y++) {
		LCDSend(0x80, SEND_CMD );
		LCDSend(0x40 | y, SEND_CMD );	
		for(x=LCD_X_RES; x > 0; x--) {
			LCDSend( LcdMemory[y * LCD_X_RES + x ], SEND_CHR );
		}	
	}
}
/****************************************************************************/
/*  Send to LCD                                                             */
/*  Function : LCDSend                                                      */
/*      Parameters                                                          */
/*          Input   :  data and  SEND_CHR or SEND_CMD                       */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDSend(unsigned char data, unsigned char cd)
{
	LCD_CS_LOW();

	if(cd == SEND_CHR) {
    	LCD_DC_HIGH();
  	}
  	else {
    	LCD_DC_LOW();
  	}

	// send data over SPI
	//SEND_BYTE_SPI();
        while(WriteSPI1(data));
        Delay10TCYx(20);
		
	LCD_CS_HIGH();
}


/****************************************************************************/
/*  Clear LCD                                                               */
/*  Function : LCDClear                                                     */
/*      Parameters                                                          */
/*          Input   :  Nothing                                              */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDClear(void)
{
	int i;
	
	// loop all cashe array
	for (i=0; i<LCD_CACHE_SIZE; i++) {
		LcdMemory[i] = 0x00;
	}
}

/****************************************************************************/
/*  Write char at x position on y row                                       */
/*  Function : LCDChrXY                                                     */
/*      Parameters                                                          */
/*          Input   :  pos, row, char                                       */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDChrXY (unsigned char x, unsigned char y, unsigned char ch )
{
    unsigned int    index   = 0;
    unsigned int    offset  = 0;
    unsigned int    i       = 0;
    unsigned int mid_shift  = 0;

    // check for out off range
    if ( x > (LCD_X_RES / 6) ) return;// max 17 char
    if ( y > (LCD_Y_RES / 13) | (y < 1) ) return;

	index = (((unsigned int)x*5+(unsigned int)1*LCD_X_RES+66) % LCD_CACHE_SIZE);// offset 66

    for ( i = 0; i < 5; i++ )
    {
        mid_shift = index%103;
        if(mid_shift < 51){
            if((mid_shift-3)%5 == 0){
                index = (index + 412) % LCD_CACHE_SIZE; //824
            }
        }
        if(mid_shift == 46){
            index = (index +121) % LCD_CACHE_SIZE;
        }
        offset = FontLookup[ch - 32][i];
        LcdMemory[index] = offset;
        index++;

    }

}

/****************************************************************************/
/*  Write negative char at x position on y row                              */
/*  Function : LCDChrXYInverse                                              */
/*      Parameters                                                          */
/*          Input   :  pos, row, char                                       */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDChrXYInverse (unsigned char x, unsigned char y, unsigned char ch )
{
	unsigned int    index   = 0;
    unsigned int    i       = 0;

    // check for out off range
    if ( x > (LCD_X_RES / 6) ) return;
    if ( y > (LCD_Y_RES / 8) ) return;

	index=(unsigned int)x*5+(unsigned int)y*LCD_X_RES;

    for ( i = 0; i < 5; i++ )
    {
      LcdMemory[index] = ~(FontLookup[ch - 32][i]);
      index++;
    }

}

/****************************************************************************/
/*  Set LCD Contrast                                                        */
/*  Function : LcdContrast                                                  */
/*      Parameters                                                          */
/*          Input   :  contrast                                             */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDContrast(unsigned char contrast) {

    //  LCD Extended Commands.
    LCDSend( 0x21, SEND_CMD );

    // Set LCD Vop (Contrast).
    LCDSend( 0x80 | contrast, SEND_CMD );

    //  LCD Standard Commands, horizontal addressing mode.
    LCDSend( 0x20, SEND_CMD );
}


/****************************************************************************/
/*  Send string to LCD                                                      */
/*  Function : LCDStr                                                       */
/*      Parameters                                                          */
/*          Input   :  row, text, inversion                                 */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void LCDStr(unsigned char row, const unsigned char *dataPtr, unsigned char inv ) {

	// variable for X coordinate
	unsigned char x = 0;
	
	// loop to the and of string
	while ( *dataPtr ) {
            LCDChrXY( x, row, *dataPtr);
            x++;
            dataPtr++;
	}
	
	LCDUpdate();
}


