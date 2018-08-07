/*
 * File:   main.c
 * Author: franc
 *
 * Created on 4 agosto 2018, 11.24
 */

#include <pic18f1220.h>
#include <xc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "configBits.h"

#define LOW             0x0
#define HIGH            0x1
#define OFF             0x0
#define ON              0x1
#define EOL          '\r\n'
#define MAX_BUFFER      50

void initUart(void);
void setUartInterrupt(unsigned, unsigned);
void __interrupt() ISR(void);
void handleRX(void);
void handleTX(void);
void startTX(void);
void clearBufferRx(void);
void clearBufferTx(void);
void startTimer0(void);
void stopTimer0(void);
void ltoa(char*, int, unsigned long);


unsigned int useTxInterrupt = 0;
char bufferTx[MAX_BUFFER];
unsigned int bufferTxSize = 0;
unsigned int bufferTxByteSend=0;


char bufferRx[MAX_BUFFER];
unsigned int bufferRxSize = 0;

unsigned long timer=0;


void main(void) {
    initUart();
    setUartInterrupt(ON,LOW);
    
    // Setto la porta RB0 Come input digitale
    ADCON1bits.PCFG4=1;
    TRISBbits.RB0=1;
    
    //Abilito interrupt porta RB0
    INTCONbits.INT0IE=1;

    //Abilito interrupt generale
    RCONbits.IPEN=0;
    INTCONbits.GIE=1;
    INTCONbits.PEIE=1;
    
    
    
    while(1){
        
    }
    return;
}
/**
 * inizializzazine del modulo EUSART
 * Modalità Asincrona 8bit a 9600 bps
 */
void initUart(){
    
    //Setto le porte TX e RX come digitali
    ADCON1bits.PCFG6 =1;
    ADCON1bits.PCFG5 =1;
    
    /*
     * Setto entrambe le porte com input come richiesto
     * dal datasheet.
     * Sarà il modulo EUSART a prenderne il controllo
    */
    TRISBbits.RB1 = 1;
    TRISBbits.RB4 = 1;
    
    // Modalità Asincrona
    SYNC = 0;
    
    // High speed 
    BRGH = 1;
    BRG16 = 0;
    // 9600 bps
    SPBRG = 129; 
    
    SPEN = 1;
    // 8bit input/output
    TX9 = 0;
    RX9 = 0;

    TXEN = 1;
    CREN = 1;
    return;
}


/**
 * Abilita interrupt sul buffer di RX e TX.
 * Setta a livello di priorità normale
 */
void setUartInterrupt(unsigned status, unsigned priority){
    //PIE1bits.TXIE = status;
    PIE1bits.TXIE = OFF;
    useTxInterrupt = status;
    PIE1bits.RCIE = status;
    
    IPR1bits.RCIP = priority;
    IPR1bits.TXIP = priority;
    return;
}


/**
 * Routine di Gestione interrupt
 */
void __interrupt() ISR(void){
    if (PIR1bits.RCIF){
        handleRX();
    }
    
    if(PIE1bits.TXIE && PIR1bits.TXIF){
        handleTX();
    }
    
    if(INTCONbits.INT0IE && INTCONbits.INT0IF){
        INTCONbits.INT0IF=0;
        stopTimer0();
        PORTAbits.RA0=0;
        
        unsigned long out=timer * 52;
        char str[20];
        memset(str,'0',20);
        
        ltoa(str,20,out);
        
        strcpy(bufferTx,  "FINE: ");
        strcat(bufferTx, str);
        strcat(bufferTx, "\r\n");
        
        
        bufferTxSize = strlen(bufferTx);
        timer=0;
        startTX();
        
    }
    if(INTCONbits.TMR0IE && INTCONbits.TMR0IF){
        INTCONbits.TMR0IF=0;
        if(timer == 0xFFFFFFFF)
        {
            timer=0;
        }
        timer++;
    }
    
}

void handleRX(){
    bufferRxSize++;
    bufferRx[bufferRxSize-1] = RCREG;
    
    
    if(bufferRxSize > 1 && bufferRx[bufferRxSize-2]==0xd && bufferRx[bufferRxSize-1] == 0xa){
        if(bufferRx[0]=='1'){
            clearBufferRx();
            TRISAbits.RA0=0;
            PORTAbits.RA0=1;
            
            INTCONbits.INT0IF=0;
            INTCONbits.INT0IE=1;
            
            startTimer0();
            
        }else if(bufferRx[0]=='R'){
            for(int i=0;i<=bufferRxSize; i++){
                bufferTx[i] = bufferRx[i];
            }
            bufferTxSize = bufferRxSize;
            clearBufferRx();
            startTX();
        }else{
            clearBufferRx();
        }
    }
    
}

void handleTX(){
    if(bufferTxSize==0){
        bufferTxByteSend=0;
        PIE1bits.TX1IE=0;
        return;
    }
    if (bufferTxByteSend < bufferTxSize){
        bufferTxByteSend++;
        TXREG = bufferTx[bufferTxByteSend-1];
    }else{
        clearBufferTx();
    }
      
}

void startTX(){
    while(!TXIF){
        __nop();
    }
    bufferTxByteSend++;
    TXREG = bufferTx[bufferTxByteSend-1];
    PIE1bits.TX1IE=useTxInterrupt;
}

void clearBufferRx(){
    bufferRxSize=0;
    memset(bufferRx,0,MAX_BUFFER);
}

void clearBufferTx(){
    bufferTxByteSend = 0;
    bufferTxSize=0;
    memset(bufferTx,0,MAX_BUFFER);
}

void startTimer0(){
    T0CONbits.T08BIT=1; //8 BIT
    T0CONbits.T0CS=0; //Instruction cicle clock = 20Mhz
    T0CONbits.T0PS=0x7;//prescaler 1:2;
    
    TMR0L=0x00;
    
    INTCONbits.TMR0IE=1;
    T0CONbits.TMR0ON=1;
}

void stopTimer0(){
    T0CONbits.TMR0ON=0;
    INTCONbits.TMR0IE=0;
    INTCONbits.TMR0IF=0;
}


void ltoa(char *buffer,int buffSize, unsigned long value) 
 {
     long original = value;        // save original value
 
     int c = buffSize;
 
     buffer[c] = 0;                // write trailing null in last byte of buffer    
 
     int pos=0;
     do                             // write least significant digit of value that's left
     {
         pos++;
         buffer[--c] = (value % 10) + '0';    
         value /= 10;
         if (pos==6){
             buffer[--c]=',';
         }
     } while (value);
     
     return;
 }