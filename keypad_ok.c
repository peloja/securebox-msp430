#include <MSP.h>
#include <msp430.h>
#include <teclado.h>
#include "grlib.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_MSP430G2_Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <stdint.h>

char keymap_char[4][3] = {'1', '2', '3',
                        '4', '5', '6',
                        '7', '8', '9',
                        '*', '0', '#'};

void espera(int T)
{
    int i;
    for(i=0;i<T; i++)
    {
        __delay_cycles(910);    //Función espera con el nro de ciclos corregidos en el ejercicio anterior
    }
}

void Set_Clk(char VEL){
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    switch(VEL){
    case 1:
        if (CALBC1_1MHZ != 0xFF) {
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;
        }
        break;
    case 8:

        if (CALBC1_8MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_8MHZ;      /* Set DCO to 8MHz */
            DCOCTL = CALDCO_8MHZ;
        }
        break;
    case 12:
        if (CALBC1_12MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_12MHZ;     /* Set DCO to 12MHz */
            DCOCTL = CALDCO_12MHZ;
        }
        break;
    case 16:
        if (CALBC1_16MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_16MHZ;     /* Set DCO to 16MHz */
            DCOCTL = CALDCO_16MHZ;
        }
        break;
    default:
        if (CALBC1_1MHZ != 0xFF) {
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;
        }
        break;

    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;
}

Graphics_Context g_sContext;
unsigned char  fil=0,col=0,col2=0;
char cadena[100];


int main(void)
{
    // Disable watch dog timer
    WDTCTL = WDTPW+WDTHOLD;

    Set_Clk(16);

    Crystalfontz128x128_Init();

    P1DIR |= (BIT0 | BIT1 | BIT2 | BIT3);
    P2DIR &=~ (BIT1 | BIT2 | BIT4);
    P2REN |= (BIT1 | BIT2 | BIT4);
    P2OUT |= (BIT1 | BIT2 | BIT4);
    P1OUT &=~ (BIT0 | BIT1 | BIT2 | BIT3);

    char fil_pins[4] = {BIT0, BIT1, BIT2, BIT3};
    char col_pins[3] = {BIT1,BIT2,BIT4};



    /*  TA1: CON SMCLK*/
    TA1CTL=TASSEL_2|ID_3| MC_1;         //SMCLK, DIV=8 (2MHz), UP
    TA1CCR0=19999;       //periodo=20000: 10ms
    TA1CCTL0=CCIE;      //CCIE=1

    //configuracion timer A0 : servo
    TA0CCTL0=CCIE;      //CCIE=1
    TA0CCTL1=OUTMOD_7;      //OUTMOD=7
    TA0CTL=TASSEL_2| ID_3 |MC_1;        //SMCLK, DIV=8, UP
    TA0CCR0=39999;      //periodo=40.000= 20ms
    TA0CCR1=3000;       //Ton inicial: 1.5ms %

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
    Graphics_clearDisplay(&g_sContext);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setFont(&g_sContext, &g_sFontCmss16);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    sprintf(cadena,"HOLA");
    Graphics_drawString(&g_sContext,cadena, 4, 10, 20, OPAQUE_TEXT);

    __bis_SR_register(GIE);


    while(1)
    {
        LPM0;
        P1OUT&=~fil_pins[0];
        P1OUT&=~fil_pins[1];
        P1OUT&=~fil_pins[2];
        P1OUT&=~fil_pins[3];
        //si se ha pulsado una tecla
        for (col=0;col<3;col++)
        {
            if(!(P2IN&col_pins[col]))
            {
                P1OUT|=fil_pins[0];
                P1OUT|=fil_pins[1];
                P1OUT|=fil_pins[2];
                P1OUT|=fil_pins[3];

                //barrido por filas
                for (fil=0;fil<4;fil++)
                {
                    (P1OUT&=~fil_pins[fil]);

                    //barrido en columnas
                    for (col2=0;col2<3;col2++)
                    {
                        if(!(P2IN&col_pins[col2]))
                        {
                            Graphics_clearDisplay(&g_sContext);
                            Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_drawString(&g_sContext,"Tecla: ", 15, 20, 60, TRANSPARENT_TEXT);
                            sprintf(cadena,"%c",keymap_char[fil][col]);
                            Graphics_drawString(&g_sContext,cadena, 15, 80, 60, OPAQUE_TEXT);

                        }
                        P2OUT|=col_pins[col2];
                    }
                    P1OUT|=fil_pins[fil];
                }
                P2OUT|=col_pins[col];
            }
        }
    }
}



#pragma vector=TIMER1_A0_VECTOR
__interrupt void Interrupcion_T1(void)
{
    LPM0_EXIT;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void)
{
    LPM0_EXIT;
}
