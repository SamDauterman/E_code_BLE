//This is the UART system that is used between the PIC24FJ128GA010 and the RN 4870 BLE. 

#include <stdio.h> 

#include "mcc_generated_files/system.h" 

// comment to test committing

BufferOut( char *buff, long temp )   // Not used  

{ 

    char degreeSym = ' '; 

    int tenths = temp % 10; 

    int full = temp / 10; 

    sprintf(buff, "%3d.%1d%c", full, tenths, degreeSym); 

//    sprintf(buff, "%3d", (int)temp); 

} 

int write(int handle, void *buffer, unsigned int len) // Not used 

{ 

    int i, *p; 

    const char *pf; 

    switch (handle) 

    { 

        case 0: 

        case 1: 

        case 2: 

            for( i = 0; i <len; ++i) 

            writeLCD(1,*((char*)buffer+i)); 

            break; 

        default: 

            break; 

    } 

    return(len); 

} 

void initLCD(void)   //  Function to initialize the LCD 

{ 

    T1CON=0x8030;    

    TMR1=0;while(TMR1<2000);      

    //Initialize the PMP 

    PMCON = 0x8303; 

    PMMODE = 0x03FF; 

    PMAEN=0x0001; 

    PMADDR=0; 

    PMDIN1=0b00111000; 

    TMR1=0;while(TMR1<3); 

     

    PMDIN1=0b00001100; 

    TMR1=0;while(TMR1<3); 

     

    PMDIN1=0b00000001; 

    TMR1=0;while(TMR1<110); 

     

    PMDIN1=0b00000110; 

    TMR1=0;while(TMR1<3); 

     

} 

 

char readLCD(int addr)    // Function for reading in a character value 

{ 

    int dummy; 

    while(PMMODEbits.BUSY); 

    PMADDR=addr; 

    dummy=PMDIN1; 

    while(PMMODEbits.BUSY); 

    return(PMDIN1); 

} 

 

#define BusyLCD() readLCD(0) & 0x80 

 

void writeLCD(int addr, char c)  // Function for writing in a character value 

{ 

    while(BusyLCD()); 

    while(PMMODEbits.BUSY); 

    PMADDR=addr; 

    PMDIN1=c; 

} 

 

#define putLCD(d) writeLCD(1,(d)) 

#define cmdLCD(c) writeLCD(0,(c)) 

#define homeLCD() writeLCD(0,2) 

#define clrLCD() writeLCD(0,1) 

 

 

// Defining macros to use in the initialization of UART 2 which is what is being used on the PIC24 

#define U_ENABLE 0x8008   // Macro used for enable U2MODE register high speed mode 

#define BRATE      34              // % error is only 0.62% which is less then 1% 

#define U_TX     0x0400        // Enable U2STA register 

 

//  I/O defines for Explorer16 

#define RTS       _RF13  // Request To Send (out - Hardware Handshake) 

#define TRTS      TRISFbits.TRISF13   //  RF13 which is pin 39 on PIC24 

 

 

void _ISRFAST _T3Interrupt(void) 

{ 

    Nop();                  

    _T3IF = 0;           // Set interrupt flag to zero 

} 

 

void InitU2( ) 

{ 

    U2BRG = BRATE; 

    U2MODE = U_ENABLE;  // Enabling the UART peripheral for high speed transmission 

    U2STA  = U_TX;               // Enabling transmission 

    RTS    = 1;                         // set RTS default Status 

    TRTS   = 0;                       // make RTS output 

} 

 

char getU2( ) 

{ 

    RTS = 0;                                    // assert Request To Send (alert terminal ready to receive char) 

    while(!U2STAbits.URXDA);  // Wait to get a char in receiver buffer from BLE  

    RTS = 1;                   

    return U2RXREG;                  // Read character received from BLE in buffer 

} 

 

int main(void) 

{ 

    SYSTEM_Initialize(); 

    initLCD();                       // Initialize LCD on explorer board 

    TRISB = 0xFF00;           // Initialize 8 least significant bits of PORTB as outputs 

    TRISA = 0xFF00;           // Initialize 8 least significant bits of PORTA as outputs 

    AD1PCFG=0x00CF;      // Configure 4 least significant bits to be digital outputs 

    PORTB = 0x0000;         // Set PORTB to zero so motor does not run with voltage already stored 

    T3CON = 0x8000;         // Setting prescale to 1:1 

    char tstCh;  

    int cOutAddr = 0; 

    long b = 0; 

   // T1CON = 0x8000; 

    //TMR1 = 0; 

    TMR3 = 0;             

    _T3IP = 4; 

    PR3 = 400 - 1;              // Setting period register to be full period (T = 25us) 

    OC1CON = 0x000E;             // Initializing OC1CON for motor 1 

    OC1R = 400;                        // Initializing OC1R to start at 100% duty cycle 

    OC1RS = 0;                          // Initializing OC1RS to start at 0% duty cycle 

    OC2CON = 0x000E;           // Initializing OC2CON for motor 2  

    OC2R = 400;                       // Initializing OC2R to start at 100% duty cycle 

    OC2RS = 0;                          // Initializing OC2RS to start at 0% duty cycle 

    _T3IF = 0;                           // Setting interrupt flag to zero 

    _T3IE = 1;                          // Enabling compiler to except interrupts 

    PORTA = 0x01;                 // Setting PORTA to be 1 to see if EEPROM works when downloading code 

    PORTB = 0b00010001;  // Setting PORTB to allow motors to run with the value given from OC1,OC2                                                                           

    writeLCD(0,0xC0);          // Setting cursor to top left of the LCD display 

    InitU2();                          // Call function to initialize UART2 on PIC24 

    char buffer[256]; 

    for( int i; i < 256; ++i) 

    { 

        buffer[i] = 0; 

    } 

    while (1) 

    { 

//        writeLCD(0,0xC0); 

        tstCh = getU2();      // Display numerical number of the characters received on LED's when pairing  

        PORTA = tstCh; 

        if(tstCh == '`')          //  Check to see message is ready to initiate 

        { 

           TMR3 = 0; while(TMR3 < 160);  // Wait for buffer to clear 

           char M1 = getU2();                      // Call function to get first character for motor 1 from the BLE 

           char M2 = getU2();                      // Receive character for motor 2 

           TMR3 = 0; while(TMR3 < 160);  

       writeLCD(0,0x01);           //   Write characters sent on the LCD to make sure transmission was received 

       writeLCD(0,0x02); 

       writeLCD(1,M1); 

       writeLCD(1,M2); 

            if(M1 == 'a')          // If the character that was sent was an 'a', turn off motor 1 

            { 

             //   PORTA = 0x07; 

                OC1RS = 0; 

            } 

            else 

                OC1RS = (int)((float)((M1 - 'a') * 160)/ 25) + 240;    //  Equation for getting different speeds with corresponding motor driver 

            if(M2 == 'a')         // If the character that was sent was an 'a', turn off motor 2 

            { 

                OC2RS = 0; 

             //   PORTA = 0x08; 

            } 

            else 

                OC2RS = (int)((float)((M2 - 'a') * 160)/ 25) + 240;    //  Equation for getting different speeds with                                                                                                         //   corresponding motor driver 

        } 

    } 

 

    return 1; 

} 