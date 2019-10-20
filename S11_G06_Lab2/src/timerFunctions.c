//Device Driver: Timer/Counter
#include <stdbool.h>
#include <stdint.h>

#include "timerFunctions.h"
#include "inc/hw_types.h"

#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h" // driverlib
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "inc/tm4c1294ncpdt.h"
#include "definitions.h"


extern uint32_t timerValue;
extern uint32_t lastValue;
extern uint16_t newAcquire;
extern uint16_t FrequencyDivider;

void SysTick_Handler(void){
  timerValue = TIMER0_TAV_R; //Read Counter
  newAcquire = 1;
} // SysTick_Handler

void timerCounterInit(void)

{
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // enable the GPIO used for the pin.
   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); // enable the timer.
   while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0))
   {}
   GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0); // configure the pin as a timer
   GPIOPinConfigure(GPIO_PD0_T0CCP0); // set the alternate function
   TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT_UP));
   TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE); // config the edge.
   TimerMatchSet(TIMER0_BASE, TIMER_A, 0xFFFF); //
   TimerPrescaleMatchSet(TIMER0_BASE, TIMER_A, 0xFF);   
   TimerEnable(TIMER0_BASE, TIMER_A); // enable the timer. 
}

void timerReset(void)
{
  timerValue = TIMER0_TAV_R;
  lastValue = timerValue;
}

void changesysTick(uint8_t settingsValue)
{
  if(settingsValue != 0)
  {
    if(settingsValue == 0x01)
    {
      uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_480),
                                              FREQMIN);
      FrequencyDivider = 1;
      SysTickPeriodSet(FREQMIN);
    }
    else if(settingsValue == 0x02)
    {
      uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_480),
                                              FREQMAX);
      FrequencyDivider = 8;
      SysTickPeriodSet(FREQMAX/1000);
    }
  }
}