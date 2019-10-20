// Anderson, Leonaldo e Thais 
// Grupo 06
// S11

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"

#include "system_TM4C1294.h" 

#define PipeLine 2
#define NumberOfCyclesUP120  80    //(9*P)+21+(3*P)+7
#define NumberOfCyclesDOWN120 81   //(9*P)+21+(3*P)+6
#define NumberOfCyclesUP  52    //(9*P)+21+(3*P)+7
#define NumberOfCyclesDOWN 53

void systemStart()
{
 
}

typedef enum estPWM
{
    Leitura,
    Display,
}estadosPWM;

void Acha_Estados ( estadosPWM estados);


void inicializa_portas(void)
{
  //PORT M PINO 0 PARA O PWM
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM); 
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM)); 
  //Endereço de memória porta M GPIO Port M 0x4006.3000
  
  GPIOPinTypeGPIOInput(GPIO_PORTM_BASE, GPIO_PIN_0); //Setar valor como pino de input
  GPIOPadConfigSet(GPIO_PORTM_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  
  //PORT J PINO 0 PARA INICIAR MEDIÇÕES e PINO 1 PARA INICIAR TESTES
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // push-buttons SW1 e SW2 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  
}

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

int PegaValorPwm(void)
{
 if( GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_0)== 1){return 1;} //TESTE
  else{return 0;}
}

void main(void)
{
    //INICIALIZA PORTAS
    inicializa_portas();
    //INICIALIZA UART 
    UARTInit();
    Acha_Estados (Leitura);
} // !main

void Acha_Estados(estadosPWM estados)
{
    int mediaInt=0,FreqInt=0, leituraPwm = 0, amostras=0,bordaSubida=0,bordaDescida=0,t_on=0,t_off=0,i,T_pwm[10],bordaSubida1=0,bordaDescida1=0;
    volatile float desvio=0,media=0,freq=0,periodo=0,duty[10],pwm_atual=0,Sduty=0,duty_atual=0, T_clock = 1.0/666666.0; //nosso programa é aproximadamente 5 clocks por ciclo
    while(1)
    {
      leituraPwm = PegaValorPwm();;  
      switch(estados)
      {				
          case Leitura:
          while(leituraPwm==1)
          {
              leituraPwm = PegaValorPwm();;  
              if(leituraPwm==1) 
              {
                  bordaSubida=1;		
              }
          }
          while(leituraPwm==0)
          {   
              leituraPwm = PegaValorPwm();;  
              if(leituraPwm==0) 
              {
                  bordaDescida=1;		
              }
          }
          if(bordaDescida == 1 && bordaSubida ==1)
          {
              while(leituraPwm==1)
              {
                  leituraPwm = PegaValorPwm();; 
                  t_on++;
                  bordaSubida1=2;
              }
              while(leituraPwm==0)
              {
                  leituraPwm = PegaValorPwm();; 
                  t_off++;
                  bordaDescida1=2;
              }
              if(bordaDescida1==2 && bordaSubida1==2) 
              {
                  bordaDescida1=0;
                  bordaSubida1=0; 
                  T_pwm[amostras] = (t_on + t_off);
                  pwm_atual = T_pwm[amostras];
                  duty[amostras]= t_on/pwm_atual;
                  duty_atual = duty[amostras];
                  Sduty+=duty_atual;
                  amostras++;
                  t_on=0;
                  t_off=0;	
                }
            }
            if(amostras==10)
            {	  //calcula DutyCycle, média, desvio padrão,período e frequência
                media=Sduty/10.0;
                for(i=0;i<10;i++)
                {
                   periodo += T_pwm[i];
                }
                periodo = (periodo*T_clock)/10;
                freq = 1/periodo;
                estados=Display;
            }
          break;
          
          case Display:
              mediaInt = (int)(media*100);
              FreqInt = (int)freq;
              FreqInt = (int)(desvio*100);
              UARTprintf("Duty Final = %d\n", mediaInt);
              UARTprintf("Desvio Padrao= %d\n", desvio);
              UARTprintf("Frequencia Final = %d\n", FreqInt);
              estados = Leitura;
          break;		
        }
    }
}



