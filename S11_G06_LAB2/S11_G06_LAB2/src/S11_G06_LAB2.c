#include <stdint.h>
#include <stdbool.h>
// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "inc/tm4c1294ncpdt.h"
#include "system_TM4C1294.h" 

extern void UARTStdioIntHandler(void);
void TIMER0A_Handler(void);
void TIMER0B_Handler(void);

void UARTInit(void)
{
  // Enable the GPIO Peripheral used by the UART.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

  // Enable UART0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

  // Configure GPIO Pins for UART mode.
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  // Initialize the UART for console I/O.
  UARTStdioConfig(0, 9600, SystemCoreClock);
} // UARTInit

void UART0_Handler(void)
{
  UARTStdioIntHandler();
} // UART0_Handler



void timerCounterInit(void)
{
  // get masked interrupt state as we need to check if its a capture
  // or an overflow interrupt
  //  uint32_t InterruptFlags = TimerIntStatus(TIMER5_BASE, true);
  
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // enable the GPIOD used for the pin, porque vamos usar o pino PD0 e PD1
   while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)) //só vai para frente quando garantir que terminou "Returns true if the specified peripheral is ready and false if it is not"
   {} 
   
   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); // enable the timer. porque vms usar o timer 0 A e B
   while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) //só vai para frente quando garantir que terminou "Returns true if the specified peripheral is ready and false if it is not"
   {}  

   //Disable Timer
   TimerDisable(TIMER0_BASE,TIMER_BOTH);        //1. Ensure the timer is disabled (the TnEN bit is cleared) before making any changes
   
   // Timer clock source = system clock (high precision 120 MHz)
   TimerClockSourceSet(TIMER0_BASE,TIMER_CLOCK_SYSTEM);

   //CONFIGURAR OS PINOS  
   GPIOPinConfigure(GPIO_PD0_T0CCP0); // set the alternate function TIMER A #define GPIO_PD0_T0CCP0 0x00030003
   GPIOPinConfigure(GPIO_PD1_T0CCP1); // set the alternate function TIMER B #define GPIO_PD1_T0CCP1 0x00030403
   GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1); // configure the pin as a timer
   
   //CONFIGURAR OS TIMER's
   TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP |  TIMER_CFG_B_CAP_TIME_UP));
      
   // upper limit for TIMEOUT interrupt in up counting mode
   TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFF);
   TimerLoadSet(TIMER0_BASE, TIMER_B, 0xFFFF);
   
    
   TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE); // No nosso caso TIMER A do TIMER0 será configurado para detectar eventos de borda de subida
   TimerControlEvent(TIMER0_BASE, TIMER_B, TIMER_EVENT_NEG_EDGE); // No nosso caso TIMER B do TIMER0 será configurado para detectar eventos de borda de descida
   //Pensar depois pq pode ser both edges
   
   
   // register Timer0A handler
    TimerIntRegister(TIMER0_BASE, TIMER_A, TIMER0A_Handler);
    TimerIntRegister(TIMER0_BASE, TIMER_B, TIMER0B_Handler);

    // enable capture interrupt
   // IntEnable(INT_TIMER0A|INT_TIMER0B);
    TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT | TIMER_CAPB_EVENT);
    TimerEnable(TIMER0_BASE, TIMER_BOTH); // enable the timer. 
}//timerCounterInit

//Global variables
volatile uint32_t RisingEdgeValue1,RisingEdgeValue2= 0;
volatile uint32_t FallingEdgeValue1,FallingEdgeValue2= 0;
volatile uint8_t FlagValor1=0, teste=0;
  
  //Variáveis
  volatile int32_t Toff = 0,Ton = 0, T=0, f_int; 
  volatile uint32_t duty2=0, amostras = 0, flagSinal, contador;
  volatile float Duty=0, f=0,  T_final=0, f_interm;

void TIMER0A_Handler(void)
{
  //Criar a rotina de interrupção do timer a
  if(FlagValor1 == 0)
  {
    RisingEdgeValue1 = TIMER0_TAR_R;
    FlagValor1 =1;
  }
  if(FlagValor1 == 2)
  {
    RisingEdgeValue2 = TIMER0_TAR_R;
    FlagValor1 = 3;
    flagSinal++;
  } 
  TimerIntClear(TIMER0_BASE,TIMER_CAPA_EVENT);
} // TIMER0A_Handler

void TIMER0B_Handler(void)
{
  if(FlagValor1 == 1)
  {
    FlagValor1 = 2;
    FallingEdgeValue1 = TIMER0_TBR_R;
  }  
  TimerIntClear(TIMER0_BASE,TIMER_CAPB_EVENT);
} // TIMER0B_Handler

  void main(void)
{
  //Inicializações
  UARTInit();
  timerCounterInit();
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // push-buttons SW1 e SW2 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);//fazer para a porta D para o teste das interrupções
       
  while(1)
  {
    if(FlagValor1 == 3)
    {
     T = RisingEdgeValue2 - RisingEdgeValue1;
     Ton = FallingEdgeValue1 - RisingEdgeValue1;
     FlagValor1 = 0;
    }//if(FlagValor1 == 3)
        contador++;
        if (contador >= 65535)
        {            
            if(flagSinal>0)
            {
              if(Ton > 0 && T > 0)
              {  
                Ton = Ton*100;
                Duty = (Ton/T);
                duty2 = (int)(Duty);
                Duty = (int)(Ton/T);
                f = 120000000/T;
                f_int = (int)(f);
                if(amostras < 10)
                {  
                  UARTprintf("Frequencia = %d \n", f_int);
                  UARTprintf("Duty cycle = %d \n\n", duty2+1); // duty+1 serve para compensar a conversão de float para int
                  amostras++;
                }//if(amostras < 10)
              }// if(Ton > 0 && T > 0)

              flagSinal =0;
              contador = 0;
            }// if (flagSinal>0)
            else
            {  
              if(amostras < 10)
              {
                if( GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_0)== 1)
                  UARTprintf("Frequencia NULA Sinal = 1 \n");
                else
                  UARTprintf("Frequencia NULA Sinal = 0 \n");
                amostras++;
              }//if (amostras < 10)
            
            }// else (flagSinal>0)
        }// if (contador >= 65535)

        
        
                  
  } // while
} // main