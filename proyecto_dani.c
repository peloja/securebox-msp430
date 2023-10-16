#include <msp430.h> 
#include "grlib.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_MSP430G2_Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <stdlib.h>
#include "uart_STDIO.h"


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


//Declaración de ejes para movimiento del Jostick
unsigned int ejey;
int y, x;
unsigned char estado=0,servo=0,pintar=0,parar=0,green=0,red=0,blue=0;

//Declaración de variables para representación de coins-partidas a jugar
volatile unsigned int modo=1;

//Declaración de variables para movimiento del jostick en eje y en inicio de partida

//variable codigo
volatile unsigned int codigo=0; //variable código que lee la UART
volatile int c1, c2, c3, c4; //cada cifra del codigo introducido
char valido=0;

//Variables pantalla insert coins
unsigned int x_max=21;
unsigned int x_borra;

void espera(int T) //Funcion de espera a 16MHz
{
    int i;
    for(i=0;i<T; i++)
    {
        __delay_cycles(16000);
    }
}
unsigned char Boton_BP(unsigned char bot) //Lee botones con debouncing sw
{


    if(bot==1)
    {
        if(P1IN&BIT1) return 0;
        espera(10);
        while(!(P1IN&BIT1));
        espera(10);
        return 1;
    }

    if(bot==2)
    {
        if(P1IN&BIT2) return 0;
        espera(10);
        while(!(P1IN&BIT2));
        espera(10);
        return 1;
    }
    if(bot==3)
    { //Boton Joystick
        if(P2IN&BIT5) return 0;
        espera(10);
        while(!(P2IN&BIT5));
        espera(10);
        return 1;
    }
    return 0;

}


int lee_ch(char canal){
    ADC10CTL0 &= ~ENC;                  //deshabilita el ADC
    ADC10CTL1&=(0x0fff);                //Borra canal anterior
    ADC10CTL1|=canal<<12;               //selecciona nuevo canal
    ADC10CTL0|= ENC;                    //Habilita el ADC
    ADC10CTL0|=ADC10SC;                 //Empieza la conversión
    LPM0;                               //Espera fin en modo LPM0
    return(ADC10MEM);                   //Devuelve valor leido
}

void inicia_ADC(char canales){
    ADC10CTL0 &= ~ENC;      //deshabilita ADC
    ADC10CTL0 = ADC10ON | ADC10SHT_3 | SREF_0|ADC10IE; //enciende ADC, S/H lento, REF:VCC, con INT
    ADC10CTL1 = CONSEQ_0 | ADC10SSEL_0 | ADC10DIV_0 | SHS_0 | INCH_0;
    //Modo simple, reloj ADC, sin subdivision, Disparo soft, Canal 0
    ADC10AE0 = canales; //habilita los canales indicados
    ADC10CTL0 |= ENC; //Habilita el ADC
}

void comprueba_cod(void)
{
    //Variables necesarias
    int codigoMAX=9999; //codigos de 4 dígitos como maximo
    char codigo_pantalla[10]; //creamos una variable char con el valor obtenido en el codigo para poder emplear la funcion UARTprintCR
    c1= codigo/1000; //funciones matematicas implementadas para descomponer un numero en sus respectivas cifras
    c2 = (codigo % 1000) / 100;
    c3 = (codigo % 100) / 10;
    c4 = codigo % 10;

    if (c1>=0 && c1<=9 &&  c2>=0 && c2<=9 && c3>=0 && c3<=9 && c4>=0 && c4<=9 && codigo<=codigoMAX) //comprobar que cada cifra es un numero comprendido entre 0 y 9;                                                                                                    //y el codigo formado por dichas cifras es de 4 digitos
    {
        UARTprint("Has introducido: ");
        sprintf (codigo_pantalla, "%d", codigo);//funcion que implementamos para convertir el codigo a char
        UARTprintCR(codigo_pantalla);//representamos en pantalla el char obtenido anteriormente
        if(c1==9||c2==9||c3==9||c4==9) //imponemos como restricción al codigo que no pueda contener un 9 el codigo
        {
            UARTprintCR("Codigo denegado");
            P1OUT &= ~BIT0;
            P1OUT |= BIT6;//enciendo led rojo si es denegado
        }
        else
        {
            UARTprintCR("Codigo aceptado");
            UARTprintCR("Disfrute con cuidado");
            P1OUT |= BIT0; //Led verde encendido para un codigo favorable
            P1OUT &= ~BIT6;
            valido=1;
        }
    }
    else if (codigo>codigoMAX)
    {
        UARTprintCR("Codigo no valido");
        P1OUT |= BIT6;
        P1OUT |= BIT0;
    }
}

Graphics_Context g_sContext;


int main(void) {

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    //Pin para el botón/
    P1DIR&=~BIT3; // P1.3 entrada boton
    P1REN|=BIT3; // P1.3 con resistencia
    P1OUT|=BIT3; // Resistencia de Pull-up


    //Pines para los leds/
    P1DIR |= (BIT0|BIT6); //P1.0 y P1.6 de salida
    P1OUT &= ~BIT0; //Inicialmente P1.0 OFF;
    P1OUT &= ~BIT6; //Inicialmente P1.6 OFF
    Set_Clk(16);
    UARTinit(16);

    inicia_ADC(BIT0+BIT3);
    P1DIR = ~(BIT1+BIT2); //ENTRADA
    P1REN|= BIT1+BIT2; //RESISTENCIA
    P1OUT|= BIT1+BIT2; //PULL-UP

    //Declaro pines de Servomotor
    P2DIR|=BIT6;
    P2SEL=BIT6;
    P2SEL2=0;


    //timer 0 para servo
    TA0CCTL0=CCIE;
    TA0CCTL1=OUTMOD_7;
    TA0CTL=TASSEL_2|ID_3| MC_1;
    TA0CCR0=39999;//
    TA0CCR1=1500;//Intervalos de trabajo de 800-2200


    //timer 1 para pantalla
    TA1CTL=TASSEL_1|ID_0| MC_1;
    TA1CCTL0=CCIE;
    TA1CCR0=1999; //ciclos de 10ms









    __bis_SR_register(GIE);

    //Inicialización de pantalla
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);


    //Diseño de portada
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK); //Color de fondo
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCm16b);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD); //Color 2 fondo

    Graphics_Rectangle VerticalDollarI = {40,1,56,127};
    Graphics_Rectangle VerticalDollarII = {71,1,85,127};
    Graphics_Rectangle HorizontalDollarI = {8,16,119,32};
    Graphics_Rectangle HorizontalDollarII = {8,95,111,111};
    Graphics_Rectangle HorizontalDollarIII = {8,55,119,71};
    Graphics_Rectangle VerticalDollarIII = {8,32,24,63};
    Graphics_Rectangle VerticalDollarIV = {103,63,119,111};

    Graphics_fillRectangle(&g_sContext,&VerticalDollarI);
    Graphics_fillRectangle(&g_sContext,&VerticalDollarII);
    Graphics_fillRectangle(&g_sContext,&HorizontalDollarI);
    Graphics_fillRectangle(&g_sContext,&HorizontalDollarII);
    Graphics_fillRectangle(&g_sContext,&HorizontalDollarIII);
    Graphics_fillRectangle(&g_sContext,&VerticalDollarIII);
    Graphics_fillRectangle(&g_sContext,&VerticalDollarIV);


    //Para poder visualizar la portada antes de la 1ºpantalla
    while(!Boton_BP(1));
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_clearDisplay(&g_sContext);

    //Variables partida jugada



    while(1){
        LPM0;
        switch(estado)
        {
        case 0:  //Pantalla de START


            Graphics_drawString(&g_sContext,"Press botton", 12, 15, 22, GRAPHICS_COLOR_DARK_GOLDENROD);
            Graphics_drawString(&g_sContext,"to start", 12, 15, 57, GRAPHICS_COLOR_DARK_GOLDENROD);
            Graphics_drawString(&g_sContext,"Tragaperras", 12, 15, 92, GRAPHICS_COLOR_DARK_GOLDENROD);

            if (Boton_BP(1)) //pulsamos en boton P1.3 para poder introducior un codigo
            {
                estado=1;
                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_clearDisplay(&g_sContext);

            }

            if(Boton_BP(2)) //Ir a pantalla de INSERT COINS
            {
                estado=3;
                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_clearDisplay(&g_sContext);
            }
            break;


        case 1: //Pantalla de insertar monedas

            //Titular de la pantalla

            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
            Graphics_drawString(&g_sContext,"Insert coins", 20, 20, 15, OPAQUE_TEXT);

            //Diseño de contador de monedas según entrada de dinero (Joystick)
            Graphics_Rectangle contador_coins = {20,60,102,80};
            Graphics_drawRectangle(&g_sContext,&contador_coins);
            Graphics_Rectangle int_cont_coins_ant = {x_borra,61,101,79}; //Borra coins de desplazamiento
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
            Graphics_Rectangle int_cont_coins = {21,61,x_max,79};  //Pinta coins de desplazamiento nuevos
            Graphics_fillRectangle(&g_sContext,&int_cont_coins);


            ejey=lee_ch(3);
            if (ejey>=600){
                x_max+=10;
                x_borra=x_max;

            }
            else if (ejey<=300){
                if (x_max-=10){
                    x_borra=x_max;
                }
            }


            //Declaramos restricción de barra coins para que no salga del rectángulo
            if (x_max<=21){
                x_max=21;
                x_borra=21;
            }
            else if (x_max>=101){
                x_max=101;
                x_borra=101;
            }


            Graphics_setForegroundColor(&g_sContext,GRAPHICS_COLOR_BLACK);
            Graphics_fillRectangle(&g_sContext,&int_cont_coins_ant);//Borra barra anterior
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
            Graphics_fillRectangle(&g_sContext,&int_cont_coins); //Pinta barra nueva


            char coins_text[20];
            sprintf(coins_text, "Coins = %d", x_max-21);
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_fillRectangle(&g_sContext, &(Graphics_Rectangle){20, 83, 100, 110}); // Borra el texto anterior
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
            Graphics_drawString(&g_sContext, coins_text, 20, 20, 90, OPAQUE_TEXT); // Dibuja el texto actualizado

            if (x_max-21==0 && Boton_BP(1))//Insert more coins to start game (cuando eliges empezar con 0 coins)
            {
                estado=1;
                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_clearDisplay(&g_sContext);

            }


            if(Boton_BP(1)) //Empezar partida (intentar que aparezca el recuento de monedas y que se vayan restando o sumando según las partidas)
            {
                servo=1;
                pintar=1;
                estado=2;
                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_clearDisplay(&g_sContext);
            }
            if(Boton_BP(2)) //Abandonar juego
            {
                estado=0;//Intentar poner que se vaya a portada o apagado
                //Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                //Graphics_clearDisplay(&g_sContext);

            }
            break;

        case 2:   //Pantalla de inicio de partida
            servo=0;
            //Buscar alternativa a declaración de figuras por rrepresentación aleatoria o en cinta de figuras
            if(pintar==1)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE_SMOKE);
                Graphics_Rectangle CintaI = {22,32,42,96};
                Graphics_fillRectangle(&g_sContext, &CintaI);
                Graphics_Rectangle CintaII = {54,32,74,96};
                Graphics_fillRectangle(&g_sContext, &CintaII);
                Graphics_Rectangle CintaIII = {86,32,106,96};
                Graphics_fillRectangle(&g_sContext, &CintaIII);

                sprintf(coins_text, "Coins = %d", x_max-21);
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
                Graphics_drawString(&g_sContext, coins_text, 20, 25, 105, OPAQUE_TEXT); // Dibuja el texto actualizado
            }
            pintar=0;
            //if(ejey=lee_ch(3)){
                x=32;
                for (y=32; y<=96; y+=32){
                    espera(100);//para la presentacion poner esto a 500
                    if(Boton_BP(3))
                    {
                        if(y==32)
                            green++;//para la presentacion ponr en green blue
                        if(y==64)
                            red++;
                        if(y==96)
                            blue++;
                        estado=7;
                    }

                    else if(y==32)
                    {
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                        Graphics_fillCircle(&g_sContext, x, 32, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                        Graphics_fillCircle(&g_sContext, x, 64, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                        Graphics_fillCircle(&g_sContext, x, 96, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                        Graphics_Rectangle FondoI = {0, 0, 127, 32};
                        Graphics_fillRectangle(&g_sContext, &FondoI);
                        Graphics_Rectangle FondoII = {0, 96, 127, 103};
                        Graphics_fillRectangle(&g_sContext, &FondoII);
                    }
                    else if(y==64)
                    {
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                        Graphics_fillCircle(&g_sContext, x, 64, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                        Graphics_fillCircle(&g_sContext, x, 96, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                        Graphics_fillCircle(&g_sContext, x, 32, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                        Graphics_Rectangle FondoI = {0, 0, 127, 32};
                        Graphics_fillRectangle(&g_sContext, &FondoI);
                        Graphics_Rectangle FondoII = {0, 96, 127, 103};
                        Graphics_fillRectangle(&g_sContext, &FondoII);
                    }
                    else if(y==96)
                    {
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                        Graphics_fillCircle(&g_sContext, x, 96, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                        Graphics_fillCircle(&g_sContext, x, 32, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                        Graphics_fillCircle(&g_sContext, x, 64, 7);
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                        Graphics_Rectangle FondoI = {0, 0, 127, 32};
                        Graphics_fillRectangle(&g_sContext, &FondoI);
                        Graphics_Rectangle FondoII = {0, 96, 127, 103};
                        Graphics_fillRectangle(&g_sContext, &FondoII);
                    }

                }
            break;

        case 7:
            x=64;
            for (y=32; y<=96; y+=32){
                espera(100);
                if(Boton_BP(3))
                {
                    if(y==32)
                        green++;
                    if(y==64)
                        red++;
                    if(y==96)
                        blue++;
                    estado=8;
                }
                else if(y==32)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                    Graphics_fillCircle(&g_sContext, x, 32, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                    Graphics_fillCircle(&g_sContext, x, 64, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                    Graphics_fillCircle(&g_sContext, x, 96, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_Rectangle FondoI = {0, 0, 127, 32};
                    Graphics_fillRectangle(&g_sContext, &FondoI);
                    Graphics_Rectangle FondoII = {0, 96, 127, 103};
                    Graphics_fillRectangle(&g_sContext, &FondoII);
                }
                else if(y==64)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                    Graphics_fillCircle(&g_sContext, x, 64, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                    Graphics_fillCircle(&g_sContext, x, 96, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                    Graphics_fillCircle(&g_sContext, x, 32, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_Rectangle FondoI = {0, 0, 127, 32};
                    Graphics_fillRectangle(&g_sContext, &FondoI);
                    Graphics_Rectangle FondoII = {0, 96, 127, 103};
                    Graphics_fillRectangle(&g_sContext, &FondoII);
                }
                else if(y==96)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                    Graphics_fillCircle(&g_sContext, x, 96, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                    Graphics_fillCircle(&g_sContext, x, 32, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                    Graphics_fillCircle(&g_sContext, x, 64, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_Rectangle FondoI = {0, 0, 127, 32};
                    Graphics_fillRectangle(&g_sContext, &FondoI);
                    Graphics_Rectangle FondoII = {0, 96, 127, 103};
                    Graphics_fillRectangle(&g_sContext, &FondoII);
                }

            }
            break;

        case 8:
            x=96;
            for (y=32; y<=96; y+=32){
                espera(100);
                if(Boton_BP(3))
                {
                    if(y==32)
                        green++;
                    if(y==64)
                        red++;
                    if(y==96)
                        blue++;
                    estado=3;
                }
                else if(y==32)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                    Graphics_fillCircle(&g_sContext, x, 32, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                    Graphics_fillCircle(&g_sContext, x, 64, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                    Graphics_fillCircle(&g_sContext, x, 96, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_Rectangle FondoI = {0, 0, 127, 32};
                    Graphics_fillRectangle(&g_sContext, &FondoI);
                    Graphics_Rectangle FondoII = {0, 96, 127, 103};
                    Graphics_fillRectangle(&g_sContext, &FondoII);
                }
                else if(y==64)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                    Graphics_fillCircle(&g_sContext, x, 64, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                    Graphics_fillCircle(&g_sContext, x, 96, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                    Graphics_fillCircle(&g_sContext, x, 32, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_Rectangle FondoI = {0, 0, 127, 32};
                    Graphics_fillRectangle(&g_sContext, &FondoI);
                    Graphics_Rectangle FondoII = {0, 96, 127, 103};
                    Graphics_fillRectangle(&g_sContext, &FondoII);
                }
                else if(y==96)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                    Graphics_fillCircle(&g_sContext, x, 96, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                    Graphics_fillCircle(&g_sContext, x, 32, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
                    Graphics_fillCircle(&g_sContext, x, 64, 7);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_Rectangle FondoI = {0, 0, 127, 32};
                    Graphics_fillRectangle(&g_sContext, &FondoI);
                    Graphics_Rectangle FondoII = {0, 96, 127, 103};
                    Graphics_fillRectangle(&g_sContext, &FondoII);
                }

            }
            break;

        case 3:   //Partida en movimiento

            if(red==3 || blue==3 || green==3)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
                Graphics_drawString(&g_sContext,"WINNNNN", 12,20, 10, OPAQUE_TEXT);
            }
            else
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
                Graphics_drawString(&g_sContext,"TRY AGAIN", 12,20, 10, OPAQUE_TEXT);
            }
            if(Boton_BP(3))
            {
                Graphics_clearDisplay(&g_sContext);
                estado=1;
            }

        break;

//        case 4:   //Estado de abandono de partida (una vez se retire el dinero que el servo actue) (incluir altavoz de sonido cuando las monedas salgan)
//
//            P1OUT &= ~BIT0;//led verde apagado al pulsar
//            P1OUT &= ~BIT6;//le rojo apagado al pulsar
//            UARTprintCR("Introduce tu codigo en formato de 4 digitos: "); //imprime en pantalla la frase
//            codigo=UARTgetint(); //asignamos a nuestra variable codigo el número introducido por la UART
//            comprueba_cod();//nos metemos en el subprograma que comprueba el codigo
//            Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_LIGHT_YELLOW);
//            Graphics_clearDisplay(&g_sContext);
//
//            if(Boton_BP(1)) //Retirada de dinero con un boton
//            {
//                estado=1;
//                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
//
//                Graphics_clearDisplay(&g_sContext);
//            }
//            if(Boton_BP(2))
//            {
//                estado=1;
//                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_LIGHT_YELLOW);
//                Graphics_clearDisplay(&g_sContext);
//            }
//            break;
//
//        case 5:   //
//
//            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_RED);
//            Graphics_drawString(&g_sContext,"Retire sus ganancias", 20, 20, 15, TRANSPARENT_TEXT);
//            if(Boton_BP(1)) //Retirada de dinero con un boton
//            {
//                estado=0;
//                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
//
//                Graphics_clearDisplay(&g_sContext);
//            }
//            if(Boton_BP(2))
//            {
//                estado=1;
//                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_LIGHT_YELLOW);
//                Graphics_clearDisplay(&g_sContext);
//            }
//            break;
//
//        case 6:   //
//
//            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_RED);
//            Graphics_drawString(&g_sContext,"Retire sus ganancias", 20, 20, 15, TRANSPARENT_TEXT);
//            if(Boton_BP(1)) //Retirada de dinero con un boton
//            {
//                estado=0;
//                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
//
//                Graphics_clearDisplay(&g_sContext);
//            }
//            if(Boton_BP(2))
//            {
//                estado=1;
//                Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_LIGHT_YELLOW);
//                Graphics_clearDisplay(&g_sContext);
//            }
//            break;


        }

    }
}

//Para potenciometro joystick
#pragma vector=ADC10_VECTOR
__interrupt void ConvertidorAD(void)
{
    LPM0_EXIT;  //Despierta al micro al final de la conversión
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void)
{
    LPM0_EXIT;
    if(servo==1)
    {
        if(x_max-21==0)
            TA0CCR1=1500;
        if(x_max-21==10)
            TA0CCR1=1710;
        if(x_max-21==20)
            TA0CCR1=1930;
        if(x_max-21==30)
            TA0CCR1=2140;
        if(x_max-21==40)
            TA0CCR1=2350;
        if(x_max-21==50)
            TA0CCR1=2560;
        if(x_max-21==60)
            TA0CCR1=2770;
        if(x_max-21==70)
            TA0CCR1=2980;
        if(x_max-21==80)
            TA0CCR1=3199;
    }
}

//Para pantalla LCD
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_300ms(void)
{
    LPM0_EXIT;  //Despierta al micro cada 300ms

}
