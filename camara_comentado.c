#include <msp430.h> 
#include "grlib.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_MSP430G2_Crystalfontz128x128_ST7735.h"
#include <stdio.h>

//funcion para que cuando presionemos un boton del keypad no se lea repetitivamente sino que se lea solo una vez. es por esto que hay que dejar la tecla pulsada hasta
//que apareza en pantalla y luego solta rapido
void espera(int T)
{
    int i;
    for(i=0;i<T; i++)
    {
        __delay_cycles(910);    //Funci�n espera con el nro de ciclos
    }
}

//configuracion del reloj
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

char keymap_char[4][3] = {'1', '2', '3', //matriz del keypad
                          '4', '5', '6',
                          '7', '8', '9',
                          '*', '0', '#'};

//inicializacion de variables que vamos a usar
Graphics_Context g_sContext;
enum {cerrado, abierto, introducir_contrase�a,definir_contrase�a};
volatile unsigned char  estado=definir_contrase�a;
unsigned char  j1=0,x=0,digito=0,fil=0,col=0,col2=0,abrir=1,cerrar=0,emergencia=0;
char fil_pins[4] = {BIT0, BIT1, BIT2, BIT3};
char col_pins[3] = {BIT1,BIT2,BIT4};
char cadena[40];

int main(void)
{
    //contrx: contrase�a que se intruduce / segx:codigo de seguridad de la puerta
    char contr1=0,contr2=0,contr3=0,contr4=0,seg1=0,seg2=0,seg3=0,seg4=0;


    WDTCTL = WDTPW | WDTHOLD; //Stop watchdog timer

    Set_Clk(16); //configuracion del reloj

    Crystalfontz128x128_Init(); //inicializacion de la pantalla del BP

    //inicializacion de puertos y direcciones usadas
    P1DIR |= (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT6); //filas del teclado como salidas  y bits de los leds
    P2DIR &=~ (BIT1 | BIT2 | BIT4);//columnas del teclado como entradas
    P2DIR |= BIT5; //pin led salida
    P2REN |= (BIT1 | BIT2 | BIT4);//habilito resistencia en las entradas
    P2OUT |= (BIT1 | BIT2 | BIT4);//resistencias de pullup
    P2OUT &=~ BIT5;//led apagado
    P1OUT &=~ (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT6);//inicializo salidas a cero y leds apagados

    //inicializacion del bit de PWM
    P2DIR|=BIT6;
    P2SEL = BIT6;
    P2SEL2 =0;

    /*  TA1: CON SMCLK*/
    TA1CTL=TASSEL_2|ID_3| MC_1;
    TA1CCR0=1999;       //periodo=20000: 10ms
    TA1CCTL0=CCIE;   //CCIE=1

    //configuracion timer A0 : servo
    TA0CCTL0=CCIE;      //CCIE=1
    TA0CCTL1=OUTMOD_7;      //OUTMOD=7
    TA0CTL=TASSEL_2| ID_3 |MC_1;        //SMCLK, DIV=8, UP
    TA0CCR0=39999;      //periodo=40.000= 20ms
    TA0CCR1=3000;       //Ton inicial: 1.5ms %

    //configuracion de la pantalla del BP
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
    Graphics_clearDisplay(&g_sContext);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setFont(&g_sContext, &g_sFontCmss14);

    Graphics_Rectangle rectangulo = {50,30,80,80}; //puerta cerrada

    __bis_SR_register(GIE);

    while(1)
    {
        LPM0; //entramos en bajo consumo

        switch(estado)
        {
        case cerrado:

            emergencia=0; //variable que hace que cuando la puerta este abierta, se pase a la configuracion de cambiar codigo de seguridad. eso pasa cuando emergencia=1.
            cerrar=0; // para controlar el servo cuando la puerta se este cerrando
            contr1=0; //reiniciamos las variables de la contrase�a intruducida
            contr2=0;
            contr3=0;
            contr4=0;
            //pinto la puerta cerrada
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BROWN);
            Graphics_fillRectangle(&g_sContext,&rectangulo);
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
            Graphics_drawLine(&g_sContext, 49,30,49,80);
            Graphics_drawLine(&g_sContext, 81,30,81,80);
            Graphics_drawLine(&g_sContext, 49,30,81,30);
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_drawCircle(&g_sContext, 55,55,2);
            Graphics_fillCircle(&g_sContext, 55,55,2);
            //escribo en pantalla
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
            Graphics_drawString(&g_sContext,"Door closed", 11, 10, 12, OPAQUE_TEXT);
            Graphics_drawString(&g_sContext,"*: insert password", 18, 10, 90, OPAQUE_TEXT);

            //para insertar contrase�a pulso asterisco
            P1OUT&=~fil_pins[3];
            if(!(P2IN&col_pins[0]))
            {
                Graphics_clearDisplay(&g_sContext);
                estado=introducir_contrase�a;
            }
            break;

        case abierto://se abre la puerta
            if(abrir==1)//para el servo
            {
                //escribo en pantalla
                Graphics_clearDisplay(&g_sContext);
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                Graphics_drawString(&g_sContext,"Opening door", 12,20, 10, OPAQUE_TEXT);
                //pinto la puerta cerrada para poder abrirla
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BROWN);
                Graphics_fillRectangle(&g_sContext,&rectangulo);
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                Graphics_drawLine(&g_sContext, 49,30,49,80);
                Graphics_drawLine(&g_sContext, 81,30,81,80);
                Graphics_drawLine(&g_sContext, 49,30,81,30);
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_fillCircle(&g_sContext, 55,55,2);

                //pinto la puerta abriendose
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                Graphics_drawLine(&g_sContext, 49,30,49,80);
                Graphics_drawLine(&g_sContext, 81,30,81,80);
                Graphics_drawLine(&g_sContext, 49,30,81,30);
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                for(j1=50;j1<66;j1++){
                    Graphics_drawLine(&g_sContext,j1,80,j1,31);
                    Graphics_drawLine(&g_sContext,50,130-j1,130-j1,130-j1);
                    Graphics_fillCircle(&g_sContext, j1+2,50,1);
                    espera(5000);
                }
                abrir=0;
            }
            else if(abrir==0)
            {
                //puerta abierta
                P1OUT&=~fil_pins[3];
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                Graphics_drawString(&g_sContext,"#: change password", 18, 5, 90, OPAQUE_TEXT);
                Graphics_drawString(&g_sContext,"Door opened  ", 12,20,10, OPAQUE_TEXT);
                Graphics_drawString(&g_sContext,"Time left", 10,5, 110, OPAQUE_TEXT);
                espera(500);
                for(x=10;x>0;x--)//cuenta atras para el cierre automatico de la puerta
                {
                    espera(18000);
                    if(!(P2IN&col_pins[2]))//si se pusa # se pasa a cambiar la clave de seguridad
                    {
                        x=1;
                        emergencia=1;
                    }
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_fillCircle(&g_sContext, 83,115,9);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    sprintf(cadena,"%d",x);
                    Graphics_drawString(&g_sContext,cadena, 5, 77, 110, OPAQUE_TEXT);
                    if(x==1)//fin de la cuenta atras
                    {
                        cerrar=1;
                        if(emergencia==1)////si se pusa # se pasa a cambiar la clave de seguridad
                        {
                            Graphics_clearDisplay(&g_sContext);
                            espera(1000);
                            estado=definir_contrase�a;
                        }
                        else
                        {
                            //se empieza a cerrar la puerta despues de la cuenta atras
                            Graphics_drawString(&g_sContext,"Closing door", 12,20,10, OPAQUE_TEXT);
                            for(j1=65;j1>49;j1--){

                                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BROWN);
                                Graphics_drawLine(&g_sContext,j1,31,j1,130-j1);
                                Graphics_drawLine(&g_sContext,80,130-j1,j1,130-j1);
                                Graphics_fillCircle(&g_sContext, j1+4,50,1);
                                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                                Graphics_fillCircle(&g_sContext, j1+2,50,1);

                                espera(5000);
                            }
                            Graphics_clearDisplay(&g_sContext);
                            estado=cerrado;
                        }
                    }
                }
            }
            break;

        case introducir_contrase�a:
            espera(10000);
            emergencia=0;
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
            Graphics_drawString(&g_sContext,"Entering password...", 25, 8, 30, OPAQUE_TEXT);
            //codigo de uso del teclado
            if(digito<4)//para que solo se puedan pulsar 4 numeros para la contrase�a
            {
                P1OUT&=~fil_pins[0];//ponemos las filas (salidas) a 0 para poder reconocer si se ha pulsado alguna tecla
                P1OUT&=~fil_pins[1];
                P1OUT&=~fil_pins[2];
                P1OUT&=~fil_pins[3];
                //si se ha pulsado una tecla
                for (col=0;col<3;col++)
                {
                    if(!(P2IN&col_pins[col]))//si el valor de alguna columna pasa a cero es que se ha pulsado una tecla
                    {
                        P1OUT|=fil_pins[0];//ponemos las filas a 1 para ir poniendolas a 0 una a una para ver en que fila se ha pulsado la tecla
                        P1OUT|=fil_pins[1];
                        P1OUT|=fil_pins[2];
                        P1OUT|=fil_pins[3];

                        //barrido por filas
                        for (fil=0;fil<4;fil++)
                        {
                            (P1OUT&=~fil_pins[fil]);//ponemos una a una las filas a cero

                            //barrido en columnas
                            for (col2=0;col2<3;col2++)
                            {
                                if(!(P2IN&col_pins[col2]))//si la columna esta a cero cuando este a cero la fila, entonces esa [fila][columna] es la tecla pulsada
                                {
                                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                                    sprintf(cadena,"%c",keymap_char[fil][col]);//pinto la tecla pulsada
                                    Graphics_drawString(&g_sContext,cadena, 15,digito*30+10, 80, OPAQUE_TEXT);
                                    espera(10000);
                                    if(digito==0)//guardo cada tecla pulsada en una variable de contrase�a
                                        contr1=keymap_char[fil][col];
                                    else if(digito==1)
                                        contr2=keymap_char[fil][col];
                                    else if(digito==2)
                                        contr3=keymap_char[fil][col];
                                    else if(digito==3)
                                        contr4=keymap_char[fil][col];

                                    digito++;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                P1OUT&=~fil_pins[3];//si se pulsa # se pasa a comprobar si la contrase�a coincide con la clave de seguridad
                if(!(P2IN&col_pins[2]))
                {
                    Graphics_clearDisplay(&g_sContext);
                    if((contr1==seg1) && (contr2==seg2) && (contr3==seg3) && (contr4==seg4))//si esto ocurre la contrase�a es buena y se abre la puerta
                    {
                        Graphics_drawString(&g_sContext,"Correct password", 16, 5, 50, OPAQUE_TEXT);
                        espera(10000);
                        Graphics_clearDisplay(&g_sContext);
                        abrir=1;
                        estado=abierto;
                        digito=0;
                    }
                    else//la contrase�a no se la buena y se pasa al estado de cerrado de nuevo
                    {
                        Graphics_drawString(&g_sContext,"Incorrect password", 18, 5, 50, OPAQUE_TEXT);
                        espera(10000);
                        Graphics_clearDisplay(&g_sContext);
                        estado=cerrado;
                        digito=0;
                    }
                }
            }

            break;

        case definir_contrase�a://se crea la clave de seguridad de la puerta

            espera(10000);
            emergencia=0;
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
            Graphics_drawString(&g_sContext,"Define password", 15, 8, 30, OPAQUE_TEXT);
            //codigo de funcionamiento del teclado igual que antes
            if(digito<4)
            {
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
                                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                                    sprintf(cadena,"%c",keymap_char[fil][col]);
                                    Graphics_drawString(&g_sContext,cadena, 15,digito*30+10, 80, OPAQUE_TEXT);
                                    espera(10000);
                                    if(digito==0)
                                        seg1=keymap_char[fil][col];
                                    else if(digito==1)
                                        seg2=keymap_char[fil][col];
                                    else if(digito==2)
                                        seg3=keymap_char[fil][col];
                                    else if(digito==3)
                                        seg4=keymap_char[fil][col];

                                    digito++;

                                }
                            }
                        }
                    }
                }
            }
            else//en este caso cuando se pulse # se pasa a cerrado directamente
            {
                P1OUT&=~fil_pins[3];
                if(!(P2IN&col_pins[2]))
                {
                    digito=0;
                    Graphics_clearDisplay(&g_sContext);
                    espera(10000);
                    estado = cerrado;
                }
            }
            break;
        }
    }
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Interrupcion_T1(void)
{
    LPM0_EXIT;
    //encendido y apagado de los leds
    if (estado==abierto)//puerta abierta se enciende el verde
    {
        P2OUT &=~ BIT5;
        P1OUT &=~ BIT4;
        P1OUT |= BIT6;
    }
    if(estado==cerrado)//puerta cerrada se enciende el rojo
    {
        P2OUT |= BIT5;
        P1OUT &=~ BIT4;
        P1OUT &=~ BIT6;
    }
    if (estado==introducir_contrase�a || estado==definir_contrase�a)//cuando se esta usando el teclado se enciende el azul
    {
        P2OUT &=~ BIT5;
        P1OUT |= BIT4;
        P1OUT &=~ BIT6;
    }
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void)
{
    LPM0_EXIT;
    //posicion del servo con PWM
    if(abrir==1)//el servo pasa a su estado maximo para abrirse
    {
        TA0CCR1+=20;
        if(TA0CCR1>=5050)
            TA0CCR1=5050;
    }
    if(cerrar==1)//el servo pasa a su estado minimo para cerrarse
    {
        TA0CCR1-=20;
        if(TA0CCR1<=1150)
            TA0CCR1=1150;
    }
    if(estado==definir_contrase�a)//mantenemos el servo cerrado cuando se usa el teclado
        TA0CCR1=1150;
    if(estado==introducir_contrase�a)
            TA0CCR1=1150;
    if(estado==cerrado)
        TA0CCR1=1150;
}
