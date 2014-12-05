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
    TRISAbits.RA3 = 1;
    ANSELAbits.ANSA3 = 1;
    TRISAbits.RA7 = 1;
    TRISC = 0x00;
    LATC = 0x00;
    ANSELC = 0x00;
    TRISCbits.TRISC6 = 1;
    TRISCbits.TRISC7 = 1;

    // FFT
    TRISBbits.TRISB0 = 1;
    ANSELBbits.ANSB0 = 0;
    // END FFT

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

    while(1){
        short i = 0;
        for (i = 0; i < 64; i++) {

            ConvertADC();

            // Wait for the ADC conversion to complete
//            LATBbits.LATB0 = 1;
            while(BusyADC());
//            LATBbits.LATB0 = 0;

            // Get the 10-bit ADC result and shift by 2
//            adc_value = ReadADC();// >> 2;
            adc_value = ((short)(ADRESH << 8) + (short)ADRESL) - 512;
            realNumbers[i] = adc_value;

            // just put in the counter to test
//            realNumbers[i] = i;

            // Set the imaginary number to zero
            imaginaryNumbers[i] = 0;
            _delay(200);
        }

        // Send in Pre-FFT data. This is directly from ADC.
//        sendIntArray(realNumbers, 64);

        fix_fft(realNumbers, imaginaryNumbers, 6);
        sendIntArray(realNumbers, 64);

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

        // Send in FFT Data
//        sendIntArray(realNumbers, 64);
    }
}

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
    putc1USART(character);
}

void sendIntArray(short *array, unsigned int size) {
    for(i = 0; i < size; i++) {
        sendChar(array[i]);
    }
}
