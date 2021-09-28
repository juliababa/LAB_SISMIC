//Aluno(a): Júlia Yuri Garcia Baba
//Matrícula: 190057921
//TURMA A

#include <msp430.h>

#define LED1_ON               (P1OUT |= BIT0)
#define LED1_OFF              (P1OUT &= ~BIT0)
#define LED1_TOGGLE           (P1OUT ^= BIT0)

#define LED2_ON              (P4OUT |= BIT7)
#define LED2_OFF             (P4OUT &= ~BIT7)
#define LED2_TOGGLE          (P4OUT ^= BIT7)


#define CHAVE2_PRESSIONADA   ((P1IN & BIT1) == 0)
#define CHAVE1_PRESSIONADA   ((P2IN & BIT1) == 0)

float inicio = 0, tempo, dist;


void configLED1();
void configLED2();
void configCHAVE2();
void configCHAVE1();

void configBUZZER();
void configHCSR04_TRIGGER();
void configHCSR04_ECHO();
void configHCSR04();

void leds();

int freq();

void distancia();

void main(void){
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    configLED1();
    configLED2();

    configCHAVE1();
    configCHAVE2();

    configBUZZER();
    configHCSR04_TRIGGER();
    configHCSR04_ECHO();

    __enable_interrupt();

    int f;

    while(1){
        distancia(tempo);
        leds();
        f = freq();
        if(f == 0){
            TA2CCR0 = 0;
            TA2CCR2 = 0;
        }else{
            TA2CCR0 = 1000000/f;
            TA2CCR2 = TA2CCR0/2;
        }
    }

}

//LED vermelho, P1.0
void configLED1()
{
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    LED1_OFF;
}

//LED verde, P4.7
void configLED2()
{
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    LED2_OFF;
}

//Chave da direita, P1.1
void configCHAVE2()
{
    P1SEL &= ~BIT1;
    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;
}

//Chave da esquerda, P2.1
void configCHAVE1()
{
    P2SEL &= ~BIT1;
    P2DIR &= ~BIT1;
    P2REN |= BIT1;
    P2OUT |= BIT1;
}

//BUZZER 2.5
void configBUZZER(){
    TA2CTL = TASSEL__SMCLK | MC__UP | TACLR; //Conta até o limiar
    TA2CCR0 = 0; //clk/freq total


    P2SEL |= BIT5;
    P2DIR |= BIT5;
    TA2CCTL2 = CM_0 | OUTMOD_6 | OUT;
    TA2CCR2 = 0; //metade
}

void configHCSR04_TRIGGER(){
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR; //Conta até o limiar
    TA0CCR0 = 50000-1;


    P1SEL |= BIT5;
    P1DIR |= BIT5;
    P1OUT &= ~BIT5;

    TA0CCTL4 = CM_0 | OUTMOD_6 | OUT;
    TA0CCR4 = 25000-1;
}

void configHCSR04_ECHO(){
    TA1CTL = TASSEL__ACLK | MC__CONTINUOUS;

    TA1CCTL1 = CM_3 | CCIS_0 | SCS | CAP | CCIE; //subida e descida

    P2DIR &= ~BIT0;
    P2SEL |= BIT0;

}

void distancia(){
    dist = (tempo*100*340/2)/32768;
}

#pragma vector = TIMER1_A1_VECTOR
__interrupt void timer_isr(){
    switch(TA1IV){
        case TA1IV_NONE:
            break;
        case TA1IV_TA1CCR1:
            if(inicio==0){
                inicio = TA1CCR1;
            }
            else{
                tempo = (TA1CCR1 > inicio) ? (TA1CCR1 - inicio): ((0xFFFF - inicio) + TA1CCR1);
                inicio = 0;
            }
            break;
        case TA1IV_TA1CCR2:
            break;
        case TA1IV_TAIFG:
            break;
    }
}

void leds(){
    if(dist<10){
        LED1_ON;
        LED2_ON;
    }
    if (dist>=10 && dist<30)
    {
        LED1_ON;
        LED2_OFF;
    }
    if(dist>=30 && dist<50){
        LED1_OFF;
        LED2_ON;
    }
    else{
        LED1_OFF;
        LED2_OFF;
    }
}

int freq(){
    int frequencia;

    if(dist < 5 || dist > 50){
        return 0;
    }
    else{
        if(CHAVE1_PRESSIONADA || CHAVE2_PRESSIONADA){
            frequencia = ((5000/45)*dist)-(25000/45);
            return frequencia;

        }
        else{
            frequencia = ((-5000/45)*dist)+(250000/45);
            return frequencia;
        }

    }

}
