#include <msp430.h>
#include <stdint.h>


uint8_t updateLCD = 0;
uint32_t SumData[2] = {0}, dataOut[2];

uint8_t j = 0, i = 0, index1 = 0;


// LCD DEFINES
#define instruction 0x00
#define caracter    BIT0
#define EN          BIT2
#define BT          BIT3

#define lcdAddress  0x27

#define CHAVE_PRESSIONADA   ((P6IN & BIT2) == 0)

#define LED1_TOGGLE           (P1OUT ^= BIT0)


//visto 3 funções
void init_ADC();
void init_timer(unsigned int frequencia);
void start_timer();
void configLED1();

int adc12_conversion_ready;
int g=0;

//caracter
void configCaracteres();

//MID
void configMID();
void debounce();

void maxmin();
void lcdWriteDigits(int valor, int min_max);

// funções i2c
void i2cConfig();
uint8_t i2cWrite (uint8_t addres, uint8_t * data, uint16_t nBytes);

// LCD FUNÇÕES
void i2cWriteByte(uint8_t addres, uint8_t data);
void lcdWriteNibble(uint8_t nibble, uint8_t RS);
void lcdWriteByte(uint8_t byte, uint8_t RS);
void lcdInit(void);
void lcdWriteString(char vetor[]);
void atualizaLCD(uint32_t dado);

//delay time
void delay_time(uint32_t time_us);

int mode=0;

int main(void){
    WDTCTL = WDTPW | WDTHOLD;


    i2cConfig();         //Taxa de comunicação I²C à 200kHz
    lcdInit();
    configCaracteres();
    configMID();
    configLED1();

    init_timer(8);
    init_ADC();

    __enable_interrupt();

    adc12_conversion_ready = 0;

    start_timer();
    while(1){
        if((P6IN & BIT2)){
            while ((P6IN & BIT2));
            debounce();

            mode++;
        }
        if(mode==0){
            lcdWriteByte(0x80, 0);
            lcdWriteString("A0:");
            if (adc12_conversion_ready){
            updateLCD = 0;
            dataOut[0] = SumData[0];
            atualizaLCD(dataOut[0]);

            adc12_conversion_ready = 0;
            }
        }
        if(mode == 1){
            lcdWriteByte(0x80, 0);
            lcdWriteString("A1:");
            if (adc12_conversion_ready){
                updateLCD = 0;
                dataOut[1] = SumData[1];
                atualizaLCD(dataOut[1]);

                adc12_conversion_ready = 0;
            }
        }
        if(mode == 2){
            if(adc12_conversion_ready){
            int i;
                    lcdWriteByte(0x01, 0);

                    for(i = 0; i < 2; i++){
                        SumData[i] *= 16;
                        SumData[i] /= (4095*8);
                    }

                    if(SumData[1] > 7){
                        lcdWriteByte(0x80, 0);
                    }else{
                        lcdWriteByte(0xC0, 0);
                    }

                    if(SumData[1] >= 8){
                        SumData[1] -= 8;
                    }

                    for(i = 0; i < SumData[0]; i++){
                        lcdWriteString(" ");
                    }

                    lcdWriteByte(0x07 - SumData[1], 1);

                    SumData[0] = SumData[1] = 0;
                    adc12_conversion_ready = 0;

                    ADC12CTL0 |= ADC12ENC;
            }
        }
        if(mode>=3){
            mode = 0;
        }

    }
}

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_interrupt(void)
{
    SumData[0] += ADC12MEM0;
    SumData[1] += ADC12MEM1;
    ADC12CTL0 |= ADC12ENC;
    g++;
    if(g==8){
        adc12_conversion_ready = 1;
        ADC12CTL0 &= ~ADC12ENC;
        LED1_TOGGLE;
        g=0;
    }
}

// FUNÇÕES LCD

void lcdWriteNibble(uint8_t nibble, uint8_t RS){
    i2cWriteByte(lcdAddress, (nibble<<4) |BT|0 |0|RS);
    i2cWriteByte(lcdAddress, (nibble<<4) |BT|EN|0|RS);
    i2cWriteByte(lcdAddress, (nibble<<4) |BT|0 |0|RS);
}

void lcdWriteByte(uint8_t byte, uint8_t RS){
    lcdWriteNibble(byte>>4, RS);
    lcdWriteNibble(byte & 0x0F, RS);
}

void lcdInit(void){
    lcdWriteNibble(0x3, instruction);
    lcdWriteNibble(0x3, instruction);
    lcdWriteNibble(0x3, instruction);       //Garante o lcd no modo 8bits

    lcdWriteNibble(0x2, instruction);       //Lcd no modo 4bits

    lcdWriteByte(0x28, instruction);
    lcdWriteByte(0x14, instruction);        //Cursor move para direita
    lcdWriteByte(0x08, instruction);
    lcdWriteByte(0x06, instruction);        //Desloca o caracter para a direita e mantem o display fixo
    lcdWriteByte(0x0C, instruction);        //Configuração de display e cursor
    lcdWriteByte(0x01, instruction);        //Limpar display
    delay_time(1600);
}

void lcdWriteString(char vetor[]){
    int i = 0;
    char c = vetor[i++];
    while(c != '\0'){
        lcdWriteByte(c, 1);
        c = vetor[i++];
    }
}

// FUNÇÕES I2C
void i2cConfig(){

    //Desliga o módulo
    UCB0CTL1 |= UCSWRST;

    //Configura os pinos
    P3SEL |= BIT0;     //Configuro os pinos para "from module"
    P3SEL |= BIT1;
    P3REN &= ~BIT0; //Resistores externos.
    P3REN &= ~BIT1;


    UCB0CTL0 = UCMST |           //Master Mode
               UCMODE_3 |    //I2C Mode
               UCSYNC;         //Synchronous Mode

    UCB0CTL1 = UCSSEL__ACLK |    //Clock Source: ACLK
               UCTR |                      //Transmitter
               UCSWRST ;             //Mantém o módulo desligado

    //Divisor de clock para o BAUDRate
    UCB0BR0 = 2;
    UCB0BR1 = 0;

    //Liga o módulo.
    UCB0CTL1 &= ~UCSWRST;
}

uint8_t i2cWrite (uint8_t addres, uint8_t * data, uint16_t nBytes){
    UCB0I2CSA = addres;                     //UCBxI2CSA = endereÃ§o do escravo
    UCB0CTL1 |= (UCTR | UCTXSTT);           //Flag de transmission e da start

    while(!(UCB0IFG & UCTXIFG));            //Espera esvaziar o buffer
    UCB0TXBUF = *data++;                    //Coloca os dados no buffer
    nBytes--;

    //while(*pont_addr & UCTXSTT);          //Setamos o start, porém o bit só será resetado quando o start e a transmissão  ocorrerem
    while(!(UCB0IFG & UCTXIFG) && !(UCB0IFG & UCNACKIFG));

    if(UCB0IFG & UCNACKIFG){                //Se receber NACK
        UCB0CTL1 |= UCTXSTP;                //Seta o Stop
        while(UCB0CTL1 & UCTXSTP);          //Espera o Stop zerar
        return 1;                           //Retorna 1 sinalizando NACK
    }

    while(nBytes--){
        while(!(UCB0IFG & UCTXIFG));        //Espera esvaziar o buffer
        UCB0TXBUF = *data++;                //Coloca os dados no buffer
    }

    while(!(UCB0IFG & UCTXIFG));            //Espera esvaziar o buffer pela Ãºltima vez
    UCB0IFG &= ~UCTXIFG;                    //Se a função for chamada de novo, a flag jÃ¡ estarÃ¡ zerada

    UCB0CTL1 |= UCTXSTP;                    //Seta o Stop
    while(UCB0CTL1 & UCTXSTP);              //Espera o stop zerar

    return 0;                               //Retorna 0 sinalizando ACK
}

void i2cWriteByte(uint8_t addres, uint8_t data){
    i2cWrite(addres, &data, 1);
}

// FUNÇÃO DELAY TIME
void delay_time(uint32_t time_us){
    TA2CTL = TASSEL__SMCLK | MC__CONTINUOUS| TACLR;
    TA2CCR2 = time_us - 1;
    while(!(TA2CTL & TAIFG));
    TA2CTL = MC__STOP;
}

// ADC

void init_ADC()
{
    //Configuro o P6.0 para o pino A0 do ADC.
    P6SEL |= BIT0;
    //Configuro o P6.1 para o pino A1 do ADC.
    P6SEL |= BIT1;

    //Desliga o módulo
    ADC12CTL0 &= ~ADC12ENC;

    ADC12CTL0 = ADC12SHT0_3 |                               //Usando 32 ciclos para o tsample
                ADC12ON;                                    //Liga o ADC

    ADC12CTL1 = ADC12CSTARTADD_0   |                        //Start address: 0
                ADC12SHS_1 |                                //Conversão via TimerA0.1
                ADC12SHP |                                  //Sample and Hold Pulse mode: input
                ADC12DIV_0 |                                //Divide o clock por 1
                ADC12SSEL_0 |                               //Escolhe o clock MODCLK: 4.8 MHz
                ADC12CONSEQ_1;                              //Modo: single channel / REPEAT conversion

    ADC12CTL2 = ADC12TCOFF |                                //Desliga o sensor de temperatura
                ADC12RES_2;                                 //12 bits resolution


    //Configurações dos canais
    ADC12MCTL0 = ADC12SREF_0 |                              //Vcc/Vss = 3.3V/0V
                               //ADC12EOS |                 //END OF SEQUENCE (não importa no single channel)
                               ADC12INCH_0 ;                //Amostra o pino A0

    ADC12MCTL1 = ADC12SREF_0 |                              //Vcc/Vss = 3.3V/0V
                 ADC12EOS |                                 //END OF SEQUENCE
                 ADC12INCH_1 ;                              //Amostra o pino A1


    ADC12IE = ADC12IE1;                                     //Liga a interrupção do canal 1.

    //Liga o ADC.
    ADC12CTL0 |= ADC12ENC;
}

void init_timer(unsigned int frequencia)
{
    TA0CTL = TASSEL__ACLK |                                 //Usa o ACLK: 32768
                     MC__STOP;                              //Timer parado

    TA0CCTL1 = OUTMOD_6;

    TA0CCR0 = 32768/(4*frequencia);
    TA0CCR1 = 16384/(4*frequencia);
}

void start_timer()
{
    TA0CTL |= (MC__UP | TACLR); //Start timer.
}

void atualizaLCD(uint32_t dado){

    int dado1, saida, saida1;
    int k;

    SumData[0] = SumData[1] = 0;                        //Já permite que SumData receba os novos valores medidos

        lcdWriteByte((0x84 + (j*0x40)), 0);             //Leva o cursor para a posição adequada a depender do canal.

        dado1 = dado;

        dado *= 3300;
        dado /= (4095*8);                                //Tira a média das medidas do canal e converte para mV

        saida = dado/1000;
        saida += 0x30;
        lcdWriteByte(saida, 1);

        lcdWriteByte(',', 1);

        saida = (dado%1000)/100;
        saida += 0x30;
        lcdWriteByte(saida, 1);

        saida = (dado%100)/10;
        saida += 0x30;
        lcdWriteByte(saida, 1);

        saida = (dado%10);
        saida += 0x30;
        lcdWriteByte(saida, 1);                       //Aqui termina de printar em decimal: d,ddd

        lcdWriteString(" DEC: ");

        dado1 /= 8;                                  //Tira a média das medidas do canal e converte para mV

        saida1 = dado1/1000;
        saida1 += 0x30;
        lcdWriteByte(saida1, 1);

        saida1 = (dado1%1000)/100;
        saida1 += 0x30;
        lcdWriteByte(saida1, 1);

        saida1 = (dado1%100)/10;
        saida1 += 0x30;
        lcdWriteByte(saida1, 1);

        saida1 = (dado1%10);
        saida1 += 0x30;
        lcdWriteByte(saida1, 1);                    //Aqui termina de printar em decimal: d,ddd

        int max=0, min=10000;


        if(max < dado){
            max = dado;
        }

        if(min > dado){
            min = dado;
        }
        lcdWriteByte(0xC0,0);
        lcdWriteString("MAX:");
        lcdWriteDigits(max, 1);
        lcdWriteString(" MIN: ");
        lcdWriteDigits(min, 1);
        index1++;
        if(index1==10){
            index1=0;
            max=0;
            min=10000;
        }

        ADC12CTL0 |= ADC12ENC;
}



void lcdWriteDigits(int valor, int min_max){
    int result = 0;

    if(min_max){
        if(valor<10){
            valor *= 1000;

        }
       result = valor/1000;
       result += 0x30;
       lcdWriteByte(result, 1);

       lcdWriteByte(',', 1);

        result = (valor%1000)/100;
        result += 0x30;
        lcdWriteByte(result, 1);

        result = (valor%100)/10;
        result += 0x30;
        lcdWriteByte(result, 1);

    }
}

void configMID(){
    P6SEL &= ~BIT2;
    P6DIR &= ~BIT2;
}

void debounce()
{
    volatile unsigned int x = 50000;
    while (x--);
}

void configCaracteres(){
    int j, i = 0;

    for(j = 0; j <= 7; j++){
        for(i = 0; i <= 7; i++){
            lcdWriteByte(0x40 + i + (j << 3), 0);
            if(i == j){
                lcdWriteByte(0x1F, 1);
            }else{
                lcdWriteByte(0x00, 1);
            }
        }

    }
}

void configLED1()
{
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
}
