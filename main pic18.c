#include <p18cxxx.h>
#include <stdio.h>
#include <stdlib.h>
#include <timers.h>
#include <adc.h>
#include <delays.h>
#include <usart.h>
#include "fft.h"
//include "classifier.h"

/* CONFIGURATION */
#pragma config WDTEN = OFF
#pragma config FOSC = INTIO7
#pragma config PLLCFG = OFF
#pragma config PRICLKEN = OFF
#pragma config PBADEN = ON
#pragma config LVP = OFF

// FFT
short imaginaryNumbers[64];
short realNumbers[64];
#define _XTAL_FREQ 4000000 // set frequency here
//define baud_rate 48
// END FFT

// RS 232
char input;
int i;
char data[5];
long baud_rate;
void sendString(char *);
void sendChar(char);
void sendCharArray(char *, unsigned int);
void sendIntArray(short *, unsigned int);
// END RS 232


void main(void) {
    unsigned int adc_value;
    TRISAbits.RA7 = 1;
    TRISC = 0x00;
    LATC = 0x00;
    ANSELC = 0x00;
    TRISCbits.TRISC6 = 1;
    TRISCbits.TRISC7 = 1;
    
    OSCCONbits.IRCF = 0b111;
    OSCCONbits.SCS = 0b11;
    OSCTUNEbits.TUN = 0b01111;
     // IMPORTANT: Set Baud Rate on Terminal to 9600
    baud_rate = (_XTAL_FREQ / 9600 / 16 - 1);
    OpenADC(ADC_FOSC_RC &
            ADC_RIGHT_JUST &
            ADC_0_TAD,
            ADC_CH0 &
            ADC_INT_OFF &
            ADC_REF_VDD_VREFPLUS &
            ADC_REF_VDD_VSS,
            0b1110);
    Close1USART();
    Open1USART(	USART_TX_INT_OFF    &
                USART_RX_INT_OFF    &
                USART_ASYNCH_MODE   &
                USART_EIGHT_BIT     &
                USART_CONT_RX       &
                USART_BRGH_LOW,
                baud_rate                  );
//    OSCCONbits.IRCF2 = 1;
//    OSCCONbits.IRCF1 = 1;
//    OSCCONbits.IRCF0 = 1;


//    while(1){
//        Delay10TCYx(5);
//        ConvertADC(); // Start conversion
//        while(BusyADC()); // Wait for completion
//        adc_value = ReadADC() >> 2; // Read result, remove 2 LSBs
//        LATC = adc_value;
//    }
    while(1){
        short i = 0;
        for (i = 0; i < 64; i++) {

            ConvertADC();

            // Wait for the ADC conversion to complete
            while(BusyADC());

            // Get the 10-bit ADC result and adjust the virtual ground of 2.5V
            // back to 0Vs to make the input wave centered correctly around 0
            // (i.e. -512 to +512)
            adc_value = ReadADC() >> 2;
            realNumbers[i] = adc_value;

            // Set the imaginary number to zero
            imaginaryNumbers[i] = 0;
        }

        fix_fft(realNumbers, imaginaryNumbers, 6);


        long place, root;
        int k = 0;
        for (k=0; k < 32; k++) {
            realNumbers[k] = (realNumbers[k] * realNumbers[k] +
                             imaginaryNumbers[k] * imaginaryNumbers[k]);

            place = 0x40000000;
            root = 0;

            if (realNumbers[k] >= 0) { // Ensure we don't have a negative number

                while (place > realNumbers[k]) place = place >> 2;

                while (place) {
                    if (realNumbers[k] >= root + place) {
                        realNumbers[k] -= root + place;
                        root += place * 2;
                    }

                    root = root >> 1;
                    place = place >> 2;
                }
            }

            realNumbers[k] = root;
        }

        sendIntArray(realNumbers, 64);

//        for(i = 0; i < 64; i++) {
//            LATC = realNumbers[i];
//            Delay10TCYx(5);
//        }
    }
}


//// RS 232
//void sendCharArray(char *array, unsigned int size) {
//    for(i = 0; i < size; i++) {
//        sendChar(array[i]);
//    }
//}
//
//// Sends a string to terminal when ready to send
//void sendString(char *string) {
//    char *temp = string;
//    while(*temp)
//    {
//        sendChar(*temp++);
//    }
//}
//
//// Sends a character to terminal when ready to send
//void sendChar(char character) {
//    while(Busy1USART());
//    putcUART(character);
//    _delay(10);
//}
//
//void sendIntArray(short *array, unsigned int size) {
//    for(i = 0; i < size; i++) {
//        while(Busy1USART());
//        //sprintf(data, "%d", array[i]);
//        //putc1USART(array[i]);
//        putc1USART(0x55);
//    }
//}
// RS 232
void sendCharArray(char *array, unsigned int size) {
    for(i = 0; i < size; i++) {
        sendChar(array[i]);
    }
}

// Sends a string to terminal when ready to send
void sendString(char *string) {
    while(Busy1USART());
    putrs1USART(string);
}

// Sends a character to terminal when ready to send
void sendChar(char character) {
    while(Busy1USART());
    putc1USART(0x55);
    _delay(10);
//    putc1USART(character);
}
void sendIntArray(short *array, unsigned int size) {
    for(i = 0; i < size; i++) {
        sendChar(array[i]);
    }
}
