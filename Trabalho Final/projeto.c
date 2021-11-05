#include <msp430.h> 
#include <stdint.h>
#include <stdio.h>

#define MOTOR_ON             (P3OUT &= ~BIT1)
#define MOTOR_OFF           (P3OUT |= BIT1)

#define LED_ON           (P2OUT |= BIT0)
#define LED_OFF           (P2OUT &= ~BIT0)

#define SENSORCHAMA_ON ((P1IN & BIT4) == 0)

#define BUFFER_SIZE 3

void initializeUART_UCA0();
void clearBuffer();

void configMotor();
void configSensorChama();
void configLed();

void sendStringUart(char string[]);

char rx_buffer[BUFFER_SIZE];
int contador = 0;
int tx_ready = 1;
int system_on = 1;

/*
 * main.c
 */

int main(void) {
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    initializeUART_UCA0();
    configMotor();
    configSensorChama();
    configLed();

    __enable_interrupt();

    clearBuffer();

    tx_ready = 1;
    contador = 0;

    while(1){
        if(system_on){
            if(SENSORCHAMA_ON){
                sendStringUart("TA PEGANDO FOGO BICHO!!\r\n");
                MOTOR_ON;
            }else{
                MOTOR_OFF;
            }
        }
    }

}

/*
 * CONFIG FUNCTIONS
 */

// Motor: P3.1
void configMotor(){
    P3SEL &= ~BIT1;
    P3DIR |= BIT1;
    MOTOR_OFF;
}

// Sensor de Chama: P1.4
void configSensorChama(){
    P1SEL &= ~BIT4;
    P1DIR &= ~BIT4;
}

void configLed(){
    P2SEL &= ~BIT0;
    P2DIR |= BIT0;
    LED_ON;
}

/*
 * UART FUNCTIONS
 */


void initializeUART_UCA0(){
    //P3.3 - TX
    //P3.4 - RX

    P3DIR |= BIT3;
    P3SEL |= BIT3;
    P3REN &= ~BIT3;

    P3DIR &= ~BIT4;
    P3SEL |= BIT4;


    //Desliga o módulo
    UCA0CTL1 |= UCSWRST;

    UCA0CTL0 = //UCPEN |    //Parity enable: 1=ON, 0=OFF
               //UCPAR |    //Parity: 0:ODD, 1:EVEN
               //UCMSB |    //LSB First: 0, MSB First:1
               //UC7BIT |   //8bit Data: 0, 7bit Data:1
               //UCSPB |    //StopBit: 0:1 Stop Bit, 1: 2 Stop Bits
               UCMODE_0 | //USCI Mode: 00:UART, 01:Idle-LineMultiprocessor, 10:AddressLine Multiprocessor, 11: UART with automatic baud rate
               //UCSYNC    //0:Assynchronous Mode, 1:Synchronous Mode
               0;

    UCA0CTL1 = UCSSEL__SMCLK | //00:UCAxCLK, 01:ACLK, 10:SMCLK, 11:SMCLK
               //UCRXEIE     | //Erroneous Character IE
               //UCBRKIE     | //Break Character IE
               //UCDORM      | //0:NotDormant, 1:Dormant
               //UCTXADDR    | //Next frame: 0:data, 1:address
               //UCTXBRK     | //TransmitBreak
               UCSWRST;        //Mantém reset.

    //BaudRate: 9600
    UCA0BRW = 6;
    UCA0MCTL = UCBRF_13 | UCOS16;

    //Liga o módulo
    UCA0CTL1 &= ~UCSWRST;

    UCA0IE =   UCTXIE | //Interrupt on transmission
               UCRXIE |   //Interrupt on Reception
               0;

}

// Interrupção do UART
#pragma vector = USCI_A0_VECTOR
__interrupt void UART_INTERRUPT(void){
    switch(_even_in_range(UCA0IV,4)){
        case 0: break;
        case 2:  //Reception Interrupt
            rx_buffer[contador] = UCA0RXBUF;
            contador++;

            if(contador == BUFFER_SIZE){
               if(rx_buffer[0] == 0xFE){
                   system_on = 1;
                   LED_ON;
               }else{
                   if(rx_buffer[0] == 0xFF){
                       system_on = 0;
                       LED_OFF;
                   }
               }
               contador = 0;
               clearBuffer();
            }

            break;
        case 4:  //Transmission Interrupt
            tx_ready = 1;
            break;
        default: break;
    }
}

void sendStringUart(char * string){
    while(* string){
        while(tx_ready == 0);
        UCA0TXBUF = *string++;
        tx_ready = 0;
    }
}

void clearBuffer(){
    uint8_t i;
    for (i = 0; i < BUFFER_SIZE; i++){
        rx_buffer[i] = 0;
    }
}