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

void UARTInit(void){
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

void UART0_Handler(void){
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
    IntEnable(INT_TIMER0A);
    IntEnable(INT_TIMER0B);
    TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT | TIMER_CAPB_EVENT);
    TimerEnable(TIMER0_BASE, TIMER_BOTH); // enable the timer. 
}//timerCounterInit

//Global variables
volatile uint32_t RisingEdgeValue1,RisingEdgeValue2= 0;
volatile uint32_t FallingEdgeValue1,FallingEdgeValue2= 0;
volatile uint8_t FlagValor1=0, teste=0;




void TIMER0A_Handler(void){
  //Criar a rotina de interrupção do timer a
  if(FlagValor1 == 0 && teste == 0)
  {
    RisingEdgeValue1 = TIMER0_TAR_R;
    FlagValor1 =1;
  }
  if(FlagValor1 == 2 && teste == 0)
  {
    RisingEdgeValue2 = TIMER0_TAR_R;
    FlagValor1 =0;
  } 
  TimerIntClear(TIMER0_BASE,TIMER_CAPA_EVENT);
} // TIMER0A_Handler

void TIMER0B_Handler(void){
  
  if(FlagValor1 == 1)
  {
    FlagValor1 = 2;
    FallingEdgeValue1 = TIMER0_TBR_R;
  }  
  TimerIntClear(TIMER0_BASE,TIMER_CAPB_EVENT);
} // TIMER0B_Handler


void main(void){
  //Inicializações
  UARTInit();
  timerCounterInit();
  
  //Variáveis
  volatile uint32_t Toff = 0,Ton = 0, T=0, T_final=0;
  volatile float Duty=0, f=0;
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // push-buttons SW1 e SW2 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);//fazer para a porta D para o teste das interrupções
  
     
  while(1){
    //Cálculos
    if((RisingEdgeValue2 > FallingEdgeValue1)&&(FallingEdgeValue1 > RisingEdgeValue1))
    {
      T = RisingEdgeValue2 - RisingEdgeValue1;
      Ton = FallingEdgeValue1 - RisingEdgeValue1;
      Toff = T - Ton;
      Duty = (int)(T/Ton*100);
      T_final = (float)T*(1/120000000); // Período em segundos
      T_final = (int)T_final;
      f = (int)(1/T_final);
      
        UARTprintf("Periodo T = %d \n", T_final);
        UARTprintf("Frequencia = %d \n", f);
        UARTprintf("Frequencia = %d \n", Duty);
      
    }
    
    

          
        //    if( GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0) // Se o SW1 for acionado então começa a ler sinal
        //    {  
        //       
        //      
        //      
        //
        ////       UARTprintf("Teste Periodo 1 = %d\n", T1);
        ////       UARTprintf("Teste Periodo 2 = %d\n", T2);
        ////       UARTprintf("Teste");
        //    }
   } // while
} // main