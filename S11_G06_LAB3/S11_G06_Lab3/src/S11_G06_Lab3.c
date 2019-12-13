#include <stdint.h>
#include <stdbool.h>
#include <string.h>
// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"
#include "cmsis_os2.h" // CMSIS-RTOS
#include "system_TM4C1294.h" 

osThreadId_t tid_elevC;                  
osThreadId_t tid_elevD;               
osThreadId_t tid_elevE;                         
osThreadId_t tid_UART_READ, tid_UART_WRITE;
  
osTimerId_t timerc_id,timerd_id,timere_id;

osMessageQueueId_t fila_e_id,fila_d_id,fila_c_id,fila_escrita_id;

void UART_WRITE(void *arg);
void UART_READ(void *arg);
void UART0_Handler(void);

void callbackc(void *arg){
     osThreadFlagsSet(tid_elevC, 0x0001);//será que poderia ser passado apenas o número 0x0001 para todas as threads, não ter mudado valor pois temos um id para cada thread
} // callback

void callbacke(void *arg){
     osThreadFlagsSet(tid_elevE, 0x0002);

} // callback

void callbackd(void *arg){
     osThreadFlagsSet(tid_elevD, 0x0003);

} // callback

/*----------------------------------------------------------------------------
 *      Thread 1 'elevC': Elevador Central
 *---------------------------------------------------------------------------*/
void elevC (void *argument) {
  static uint8_t andar_atual=0;
  static uint8_t proximo_andar=20; //proximo andar jamais recebera valor 20 pois são apenas 15 andares, portanto 20 significa só o momento de iniciar o elevador
  static uint8_t vetor_destino[16];
  
  static uint8_t i;
  static uint8_t botao_externo_andar = 0;
  static uint8_t sinal_feedback=0;  
  static uint8_t flag_vetor_zero=0;  
  volatile bool  flag_mudou_andar=false;  
  
  
  char comando[2];
  char mensagem[10];    
  char andares_letras[]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
 
  //ENVIA RESET
  comando[0]='c'; //Qual elevador
  comando[1]='r';//resetar
  osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
  osDelay(1000);

  //FECHAR AS PORTAS
  comando[0]='c';
  comando[1]='f';//fechar
  osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
    
  for (;;) {
    osMessageQueueGet(fila_c_id, mensagem ,NULL, osWaitForever);
    //TRATAMENTO DOS BOTÕES EXTERNOS 
    if(mensagem[1]=='E'){//botao externo
      sscanf(mensagem,"%*c%*c%d%*c", &botao_externo_andar);  //botao_externo_andar: Auxiliar que recebe o valor do andar do botão externo pressionado 
      vetor_destino[botao_externo_andar]=1;                  //Atualiza o vetor de destino do elevador     
      
      //Caso seja a primeira vez
      if((proximo_andar == 20) ||((flag_vetor_zero == 0) && (flag_mudou_andar))){  //se for a primeira vez (proximo_andar = 20), se o vetor de destino tiver somente zeros, ou seja, só foi pedido um andar (flag_vetor_zero), se ja mudou de andar (flag_mudou_andar)
        flag_mudou_andar = false;
        proximo_andar = botao_externo_andar;
      }
      
      //manda subir caso andar_atual for menor que o próximo andar
       if(andar_atual < proximo_andar){
         comando[0]='c'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
      //manda descer caso andar_atual for maior que o próximo andar
      else if(andar_atual > proximo_andar){
             comando[0]='c'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
   }//if do botão externo    
   else if(mensagem[1]=='I'){ //botao interno     
      //Tem um vetor de ketras, pois para os botões internos, ao invés de ter números indicando os andares são letras, então aqui ja converte pra números e ja salva no vetor destino, só existe um vetor de destino tanto para subir quanto para descer
      for(i=0;i<16;i++){
        if(andares_letras[i]==mensagem[2]){
          vetor_destino[i]=1;
        }
      }
      
      //Atualiza o próximo andar
      for(i=0;i<16;i++){
          if(vetor_destino[i]==1){
             proximo_andar = i;
          }      
      }
      
      //manda subir caso andar_atual for menor que o próximo andar
      if(andar_atual < proximo_andar){
         comando[0]='c'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
      //manda descer caso andar_atual for maior que o próximo andar
      else if(andar_atual > proximo_andar){
             comando[0]='c'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
    }//if do botão interno
    
    else if((mensagem[1]>='0' && mensagem[0]=='c' && mensagem[1]<='9') || (mensagem[2]>='0' && mensagem[1]<='2'))//NÃO FUNCIONOU if(!(mensagem[1]=='A' || mensagem[1]=='F')) // If para verificar se é feedback pois A e F é apenas feedback de porta aberta e fechada
    {
  //RESOLVIDO: PENSAR SOBRE ISSO AQUI PROBLEMA QUE EU ATUALIZO O ANDAR QUANDO PASSA SENSOR, PORÉM EU POSSO APERTAR BOTÃO ANTES DE TERMINAR O DESTINO              
      flag_mudou_andar = true; //Flag que vai indicar que teve mudança de andar
      
         sscanf(mensagem,"%*c%d", &sinal_feedback);  
               andar_atual = sinal_feedback;
          if(andar_atual == proximo_andar){
           
           vetor_destino[andar_atual]=0; //Zera a posição que ja foi atendida no vetor de destino
           
           comando[0]='c'; //Qual elevador
           comando[1]='p';//parar
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
           
           comando[0]='c'; //Qual elevador
           comando[1]='a';//abrir porta
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
           osTimerStart(timerc_id, 3000); //Da start no timer do sistema para que a porta fique aberta 3 segundos
           osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever); //Bloqueia a Thread esperando uma flag de sinal, só vai receber a flag quando for chamada a função de callback

          //RESOLVIDO: PAREI AQUI LÓGICA ERRADO
          for(i=0;i<16;i++){
              if(vetor_destino[i]==1){
                 proximo_andar = i;
                 flag_vetor_zero = 1;
                 break;
                }else
                  flag_vetor_zero =0;
          }     
         }  
    }//Fim else do feedback    
    else{// ultimo else são os feeback de porta aberta e porta fechada
        if(mensagem[1]=='A'){
           comando[0]='c'; //Qual elevador
           comando[1]='f';//fechar porta
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
        }        
    }
    
    //Aqui lógica para comandar a subida e descida do elevador, mesmo se os botões não forem mais apertados, porém o vetor de destino ainda tem elementos
    if(flag_vetor_zero == 1){
      
      if(andar_atual < proximo_andar){
         comando[0]='c'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }     
      else if(andar_atual > proximo_andar){
             comando[0]='c'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }      
      
      
    }
  }//for infinito
}//Thread elevC
/*----------------------------------------------------------------------------
 *      Thread 2 'elevD': Elevador Direito
 *---------------------------------------------------------------------------*/
void elevD (void *argument) {
  static uint8_t andar_atual=0;
  static uint8_t proximo_andar=20;
  static uint8_t vetor_destino[16];
  
  static uint8_t i;
  static uint8_t botao_externo_andar = 0;
  static uint8_t sinal_feedback=0;  
  static uint8_t flag_vetor_zero=0;  
  volatile bool flag_mudou_andar=false;  
  
  
  char comando[2];
  char mensagem[10];    
  char andares_letras[]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
  
  //ENVIA RESET
  comando[0]='d'; //Qual elevador
  comando[1]='r';//resetar
  osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
  osDelay(1000);

  //FECHAR AS PORTAS
  comando[0]='d';
  comando[1]='f';//fechar
  osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
 

         
  for (;;) {
    osMessageQueueGet(fila_d_id, mensagem ,NULL, osWaitForever);
    //TRATAMENTO DOS BOTÕES EXTERNOS 
    if(mensagem[1]=='E'){//botao externo
      sscanf(mensagem,"%*c%*c%d%*c", &botao_externo_andar);  //Auxiliar que recebe o valor do andar do botão externo pressionado 
      vetor_destino[botao_externo_andar]=1;               //Atualiza o vetor de destino do elevador     
      
      //Caso seja a primeira vez
      if((proximo_andar == 20) ||((flag_vetor_zero == 0) && (flag_mudou_andar))){
        flag_mudou_andar = false;
        proximo_andar = botao_externo_andar;
      }          
        if(andar_atual < proximo_andar){
         comando[0]='d'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
      else if(andar_atual > proximo_andar){
         comando[0]='d'; //Qual elevador
         comando[1]='d';//descer
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
   }//if do botão externo    
   else if(mensagem[1]=='I'){ //botao interno     
      for(i=0;i<16;i++){
        if(andares_letras[i]==mensagem[2]){
          vetor_destino[i]=1;
        }
      }
      
      for(i=0;i<16;i++){
          if(vetor_destino[i]==1){
             proximo_andar = i;
          }      
      }
      
      if(andar_atual < proximo_andar){
         comando[0]='d'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }     
      else if(andar_atual > proximo_andar){
             comando[0]='d'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
      
    }//if do botão interno
    
    else if((mensagem[1]>='0' && mensagem[1]<='9') || (mensagem[2]>='0' && mensagem[1]<='2'))//NÃO FUNCIONOU if(!(mensagem[1]=='A' || mensagem[1]=='F')) // If para verificar se é feedback pois A e F é apenas feedback de porta aberta e fechada
    {
      flag_mudou_andar = true;
      
         sscanf(mensagem,"%*c%d", &sinal_feedback);  
         andar_atual = sinal_feedback;

          if(andar_atual == proximo_andar){
           
           vetor_destino[andar_atual]=0; 
           comando[0]='d'; //Qual elevador
           comando[1]='p';//parar
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
           
           comando[0]='d'; //Qual elevador
           comando[1]='a';//abrir porta
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
           osTimerStart(timerd_id, 3000);
           osThreadFlagsWait(0x0003, osFlagsWaitAny, osWaitForever);

          for(i=0;i<16;i++){
              if(vetor_destino[i]==1){
                 proximo_andar = i;
                 flag_vetor_zero = 1;
                 break;
                }else
                  flag_vetor_zero =0;
          }
         }
    }//Fim else do feedback    
    else{
        if(mensagem[1]=='A'){
           comando[0]='d'; //Qual elevador
           comando[1]='f';//fechar porta
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
        }        
    }
    
    if(flag_vetor_zero == 1){
      if(andar_atual < proximo_andar){
         comando[0]='d'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }     
      else if(andar_atual > proximo_andar){
             comando[0]='d'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }      
    }
  }//for infinito
}

/*----------------------------------------------------------------------------
 *      Thread 3 'elevE': Elevador Esquerdo
 *---------------------------------------------------------------------------*/
void elevE (void *argument) {
  static uint8_t andar_atual=0;
  static uint8_t proximo_andar=20;
  static uint8_t vetor_destino[16];
  
  static uint8_t i;
  static uint8_t botao_externo_andar = 0;
  static uint8_t sinal_feedback=0;  
  static uint8_t flag_vetor_zero=0;  
  volatile bool flag_mudou_andar=false;  
  
  
  char comando[2];
  char mensagem[10];    
  char andares_letras[]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};

  //ENVIA RESET
  comando[0]='e'; //Qual elevador
  comando[1]='r';//resetar
  osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
  osDelay(1000);

  //FECHAR AS PORTAS
  comando[0]='e';
  comando[1]='f';//fechar
  osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);

  for (;;) {
    osMessageQueueGet(fila_e_id, mensagem ,NULL, osWaitForever);
    //TRATAMENTO DOS BOTÕES EXTERNOS 
    if(mensagem[1]=='E'){//botao externo
      sscanf(mensagem,"%*c%*c%d%*c", &botao_externo_andar);  //Auxiliar que recebe o valor do andar do botão externo pressionado 
      vetor_destino[botao_externo_andar]=1;               //Atualiza o vetor de destino do elevador     
      
      //Caso seja a primeira vez
      if((proximo_andar == 20) ||((flag_vetor_zero == 0) && (flag_mudou_andar))){
        flag_mudou_andar = false;
        proximo_andar = botao_externo_andar;
      }
      
       if(andar_atual < proximo_andar){
         comando[0]='e'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }     
      else if(andar_atual > proximo_andar){
             comando[0]='e'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }     
   }//if do botão externo    
   else if(mensagem[1]=='I'){ //botao interno     
      for(i=0;i<16;i++){
        if(andares_letras[i]==mensagem[2]){
          vetor_destino[i]=1;
        }
      }
      
      for(i=0;i<16;i++){
          if(vetor_destino[i]==1){
             proximo_andar = i;
          }      
      }
      
      if(andar_atual < proximo_andar){
         comando[0]='e'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }     
      else if(andar_atual > proximo_andar){
             comando[0]='e'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }
      
    }//if do botão interno
    
    else if((mensagem[1]>='0' && mensagem[0]=='c' && mensagem[1]<='9') || (mensagem[2]>='0' && mensagem[1]<='2'))//NÃO FUNCIONOU if(!(mensagem[1]=='A' || mensagem[1]=='F')) // If para verificar se é feedback pois A e F é apenas feedback de porta aberta e fechada
    {
      flag_mudou_andar = true;
      
         sscanf(mensagem,"%*c%d", &sinal_feedback);  
               andar_atual = sinal_feedback;
          if(andar_atual == proximo_andar){
           
           vetor_destino[andar_atual]=0; 
           comando[0]='e'; //Qual elevador
           comando[1]='p';//parar
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
           
           comando[0]='e'; //Qual elevador
           comando[1]='a';//abrir porta
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
           osTimerStart(timere_id, 3000);
           osThreadFlagsWait(0x0002, osFlagsWaitAny, osWaitForever);

          for(i=0;i<16;i++){
              if(vetor_destino[i]==1){
                 proximo_andar = i;
                 flag_vetor_zero = 1;
                 break;
                }else
                  flag_vetor_zero =0;
          }
         }
    }//Fim else do feedback    
    else{
        if(mensagem[1]=='A'){
           comando[0]='e'; //Qual elevador
           comando[1]='f';//fechar porta
           osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
        }        
    }
    
    if(flag_vetor_zero == 1){
      
      if(andar_atual < proximo_andar){
         comando[0]='e'; //Qual elevador
         comando[1]='s';//subir
         osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }     
      else if(andar_atual > proximo_andar){
             comando[0]='e'; //Qual elevador
             comando[1]='d';//descer
             osMessageQueuePut(fila_escrita_id, comando, NULL, osWaitForever);
      }      
      
      
    }
  }//for infinito
}


/*----------------------------------------------------------------------------
 *      Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
void app_main (void *argument) {
  
  tid_UART_READ= osThreadNew(UART_READ, NULL, NULL);
  tid_UART_WRITE= osThreadNew(UART_WRITE, NULL, NULL);
  
  fila_e_id =  osMessageQueueNew (15, sizeof(char)*6,NULL);
  fila_c_id =  osMessageQueueNew (15, sizeof(char)*6,NULL); 
  fila_d_id =  osMessageQueueNew (15, sizeof(char)*6,NULL);
  
  fila_escrita_id =  osMessageQueueNew (30, sizeof(char)*3,NULL);
  
  tid_elevC = osThreadNew(elevC, NULL, NULL);
  tid_elevD = osThreadNew(elevD, NULL, NULL);
  tid_elevE = osThreadNew(elevE, NULL, NULL);
  
  timerc_id = osTimerNew(callbackc, osTimerOnce, NULL,NULL);
  timere_id = osTimerNew(callbacke, osTimerOnce, NULL,NULL);
  timerd_id = osTimerNew(callbackd, osTimerOnce, NULL,NULL);
 
   
  osDelay(osWaitForever);
  while(1);
}


extern void UARTStdioIntHandler(void);

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
  UARTStdioConfig(0,115200, SystemCoreClock);
} // UARTInit


void UART0_Handler(void){
  UARTStdioIntHandler();
} // UART0_Handler

void UART_READ(void *arg){
  
  char msg[6];
  while(1){
    UARTgets(msg,20);
    if(msg[0]=='e'){
      osMessageQueuePut(fila_e_id, msg, NULL, 0);
    }
    else if(msg[0]=='c'){
      osMessageQueuePut(fila_c_id, msg, NULL, 0);
    }
    else if(msg[0]=='d'){
      osMessageQueuePut(fila_d_id, msg, NULL, 0);
    }  
    osThreadYield();
  } 
}
void UART_WRITE(void *arg){
  
  
  char msg[2];
  while(1){
    osMessageQueueGet(fila_escrita_id, msg ,NULL, osWaitForever);
    UARTprintf("%c%c\n\r",msg[0],msg[1]); 
  } 
} 

void main(void){
  //UART initialization
  UARTInit();
  
  
    
    // System Initialization
  osKernelInitialize();                 
  osThreadNew(app_main, NULL, NULL);    
  if (osKernelGetState() == osKernelReady) {
    osKernelStart();                    
  }
  while(1);
  
} // main