#include <p18cxxx.h>
#include <stdio.h>
#include <stdlib.h>
#include <timers.h>
#include <adc.h>
#include <delays.h>
#include <usart.h>
#include <spi.h>
#include "fft.h"
#include "lcd3310.h"
//include "classifier.h"

/* CONFIGURATION */
#pragma config WDTEN = OFF
#pragma config FOSC = INTIO7
#pragma config PLLCFG = OFF
#pragma config PRICLKEN = OFF
#pragma config PBADEN = ON
#pragma config LVP = OFF

#define SEND_TRAINING_DATA 0
#define SEND_GUESSES 1

#define THRESHOLD 20 // lower increases sensitivity

#define H2 3 // strong indicator
#define H1 2 // indicator
#define ZR 0 // neutral
#define L1 -1 // negative indicator
#define L2 -3 // strong negative indicator

// FFT
short imaginary_numbers[64];
short real_numbers[64];
#define _XTAL_FREQ 4000000 // set frequency here
// END FFT

// CLASSIFIER
short oldRealNumbers[31];
short guesses[5];
int alarm_prob;
int growl_prob;
int horn_prob;
int scream_prob;
int max_tally;
int total_tally;
int alarm_tally;
int growl_tally;
int horn_tally;
int scream_tally;                      //00,01,02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30
const short alarm_prob_weights[31] =    {ZR,ZR,L1,ZR,ZR,ZR,L1,L2,L1,L2,ZR,L2,ZR,ZR,ZR,ZR,ZR,ZR,ZR,ZR,ZR,H2,H2,H2,H1,ZR,H1,H1,ZR,ZR,ZR}; // +5
const short growl_prob_weights[31] =    {ZR,H2,H2,ZR,ZR,ZR,ZR,ZR,ZR,ZR,ZR,L1,L2,L2,L2,L1,L1,ZR,ZR,ZR,ZR,ZR,H1,ZR,ZR,ZR,H1,H2,H1,ZR,ZR}; // +2
const short horn_prob_weights[31] =     {ZR,ZR,ZR,ZR,ZR,ZR,H1,H2,H1,ZR,ZR,ZR,ZR,ZR,ZR,H2,H1,ZR,ZR,L2,L1,ZR,ZR,H2,ZR,ZR,L1,L2,L2,L1,ZR}; // -6
const short scream_prob_weights[31] =   {ZR,H2,ZR,ZR,H2,H1,H1,H2,H1,ZR,ZR,L1,L1,L2,ZR,ZR,ZR,ZR,ZR,L1,L2,L2,ZR,ZR,ZR,ZR,ZR,ZR,ZR,ZR,ZR}; // +4
// END CLASSIFIER

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

void setup(void);
void writeGuess(char, int, int);

void main(void) {
    unsigned int adc_value;


    setup();

    while(1){
        short i = 0;
        for (i = 0; i < 64; i++) {
            ConvertADC();

            // Wait for the ADC conversion to complete
            while(BusyADC());

            // Get the 10-bit ADC result
            adc_value = ((short)(ADRESH << 8) + (short)ADRESL) - 512;
            real_numbers[i] = adc_value;


            // Set the imaginary number to zero
            imaginary_numbers[i] = 0;
            _delay(1500);
        }

        // Send in Pre-FFT data. This is directly from ADC.

        fix_fft(real_numbers, imaginary_numbers, 6);
        
        long place, root;
        int k = 0;
        for (k=0; k < 32; k++) {
            real_numbers[k] = (real_numbers[k] * real_numbers[k] +
                             imaginary_numbers[k] * imaginary_numbers[k]);

            place = 0x40000000;
            root = 0;

            if (real_numbers[k] >= 0) { // Ensure we don't have a negative number

                while (place > real_numbers[k]) place = place >> 2;

                while (place) {
                    if (real_numbers[k] >= root + place) {
                        real_numbers[k] -= root + place;
                        root += place * 2;
                    }

                    root = root >> 1;
                    place = place >> 2;
                }
            }

            real_numbers[k] = root;
        }

        for(i = 1; i < 33; i++) {
            real_numbers[i] = oldRealNumbers[i - 1] * 10 / 16 +
                    real_numbers[i] * 6 / 16;
        }

        for (i = 1; i < 33; i++) {
            oldRealNumbers[i - 1] = real_numbers[i];
        }


        // Classify
        alarm_tally = 0;
        growl_tally = 0;
        horn_tally = 0;
        scream_tally = 0;

        // alarm
        for(i = 0; i < 31; i++) {
            alarm_tally += (alarm_prob_weights[i] * real_numbers[i+1]);
        }

        if(alarm_tally < 0) {
            alarm_tally = 0;
        } else if(real_numbers[16] > real_numbers[3] &&
            real_numbers[13] > real_numbers[23]) {
            alarm_tally = alarm_tally / 4;
        }

        // growl
        for(i = 0; i < 31; i++) {
            growl_tally += (growl_prob_weights[i] * real_numbers[i+1]);
        }

        if(growl_tally < 0) {
            growl_tally = 0;
        } else if(real_numbers[30] > real_numbers[24] &&
            real_numbers[15] > real_numbers[1] &&
            real_numbers[29] > real_numbers[8]) {
            growl_tally = growl_tally / 4;
        }

        // horn
        for(i = 0; i < 31; i++) {
            horn_tally += (horn_prob_weights[i] * real_numbers[i+1]);
        }

        if(horn_tally < 0) {
            horn_tally = 5;
        } else if(real_numbers[21] > real_numbers[7] &&
            real_numbers[16] > real_numbers[10] &&
            real_numbers[25] > real_numbers[1] &&
            real_numbers[20] > real_numbers[24] &&
            real_numbers[12] > real_numbers[5] &&
            real_numbers[27] > real_numbers[10]) {
            horn_tally = horn_tally / 4;
        }

        // scream
        for(i = 0; i < 31; i++) {
            scream_tally += (scream_prob_weights[i] * real_numbers[i+1]);
        }

        if(scream_tally < 0) {
            scream_tally = 0;
        } else if(real_numbers[28] > real_numbers[8] &&
            real_numbers[17] > real_numbers[27] &&
            real_numbers[29] > real_numbers[1] &&
            real_numbers[21] > real_numbers[2]) {
            scream_tally = scream_tally / 4;
        }

        guesses[0] = 255;
        guesses[1] = alarm_tally;
        guesses[2] = growl_tally;
        guesses[3] = horn_tally;
        guesses[4] = scream_tally;

        max_tally = 0;
        for(i = 1; i < 5; i++) {
            if(guesses[i] > max_tally) {
                max_tally = guesses[i];
            }
        }

        alarm_prob = 0;
        growl_prob = 0;
        horn_prob = 0;
        scream_prob = 0;

        if(max_tally > THRESHOLD) {
            LATBbits.LATB1 = 1;
            total_tally = alarm_tally + growl_tally + horn_tally + scream_tally;
            alarm_prob = 100 * alarm_tally / total_tally;
            growl_prob = 100 * growl_tally / total_tally;
            horn_prob = 100 * horn_tally / total_tally;
            scream_prob = 100 * scream_tally / total_tally;
            
            // LCD
            if(max_tally == alarm_tally) {
                writeGuess('A', alarm_prob, 1);
            } else if(max_tally == growl_tally) {
                writeGuess('G', growl_prob, 1);
            } else if(max_tally == horn_tally) {
                writeGuess('H', horn_prob, 1);
            } else if(max_tally == scream_tally) {
                writeGuess('S', scream_prob, 1);
            }
            
            for(i = 0; i < 10; i++) {
                _delay(100000);
            }
            LATBbits.LATB1 = 0;
            for(i = 0; i < 10; i++) {
                _delay(100000);
            }
        }

        if(SEND_TRAINING_DATA) {
            // Send FFT Data           
            real_numbers[0] = 255;
            sendIntArray(real_numbers, 32);
        } else if(SEND_GUESSES) {
            sendIntArray(guesses, 5);
        }
    }
}

void setup(void) {
    TRISAbits.RA3 = 1;
    ANSELAbits.ANSA3 = 1;
    TRISAbits.RA7 = 1;
    TRISC = 0x00;           // port C as outputs
    LATC = 0x00;            // set port C pins low
    ANSELC = 0x00;          // disable analog pins port c
    TRISCbits.TRISC6 = 1;   // rs-232 tx output
    TRISCbits.TRISC7 = 1;   // rs-232 rx
    TRISBbits.TRISB1 = 0;   // haptic feedback pin
    LATBbits.LATB1 = 0;     // set haptic feedback pin low

    //internal oscillator configuation
    OSCCONbits.IRCF = 0b111;
    OSCCONbits.SCS = 0b10;
    OSCTUNEbits.TUN = 0b000000;
    OSCTUNEbits.PLLEN = 1;
    OSCCON2bits.MFIOSEL = 0;
    OSCTUNEbits.INTSRC = 1;
    // end osc config

    OpenSPI1(SPI_FOSC_4,
            MODE_01,
            SMPMID);

    LCDInit();          //iniialize the lcd display

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
                baud_rate           );
}

void writeGuess(char guess, int prob, int row) {
    char percent[8];
    sprintf(percent, "%c %d %c", guess, prob, '%');     // example "H 85%": horn, 85% match
    LCDStr(1, percent, 0);                              // sends string to lcd write function
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
