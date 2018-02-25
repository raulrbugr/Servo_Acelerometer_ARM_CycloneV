#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "../address_map_arm.h"
#include "../interrupt_ID.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Altera University Program");
MODULE_DESCRIPTION("DE1SoC Pushbutton Interrupt Handler");

#define COUNTER 25000000; // timeout = 1/(200 MHz) x 200£10^6 = 1 sec

void * LW_virtual;         // Lightweight bridge base address
volatile int *LEDR_ptr;    // virtual address for the LEDR port
volatile int *HEX_ptr;
volatile int *KEY_ptr;     // virtual address for the KEY port
volatile int *SW_ptr;
unsigned int contador;
volatile unsigned int * MPcore_private_timer_ptr; // virtual address A9 private timer base address

volatile unsigned int * HPS_TIMER0_timer_ptr; // virtual address A9 timer base address
void *HPS_TIMER0_virtual;   // used to map physical addresses for the light-weight bridge
volatile unsigned int * HPS_TIMER1_timer_ptr; // virtual address A9 timer base address
void *HPS_TIMER1_virtual;   // used to map physical addresses for the light-weight bridge
volatile unsigned int * HPS_TIMER2_timer_ptr; // virtual address A9 timer base address
void *HPS_TIMER2_virtual;   // used to map physical addresses for the light-weight bridge
volatile unsigned int * HPS_TIMER3_timer_ptr; // virtual address A9 timer base address
void *HPS_TIMER3_virtual;   // used to map physical addresses for the light-weight bridge

volatile unsigned int * LEDSG_ptr; // virtual address A9 timer base address
void *LEDSG_virtual;   // used to map physical addresses for the light-weight bridge


volatile int *JP2_ptr;
void *JP2_virtual;

volatile int *TIMER0_low;
volatile int *TIMER2_low;
volatile int *TIMER3_low;

int acumuladorX;
int acumuladorY;
unsigned int flagServo;// 0 para servo X, 1 para servo Y



/************** Prototypes for functions used to access physical memory addresses *************/

void HexIncrement(void);




irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
   
	if((*(SW_ptr) & 0b1) !=0){ //Segun este el SwitchButton, usaremos un servo u otro, es el que esta pegado a los botones
		//contador++;
   		//HexIncrement();
		flagServo =1;
	}
	else{
		flagServo = 0;
	}

	/************************************
	* RUTINA DE TRATAMIENTO DE SERVOS	*
	************************************/
	

	if((*(KEY_ptr + 3) & 0b0011) !=0){
		
		if(flagServo){
			//acumuladorX=*(HPS_TIMER3_timer_ptr)+1000;
			acumuladorX=acumuladorX+1000; 		//sumamos 1000 más
			/////////////Variar PWM/////////////////

			*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina
			*(HPS_TIMER3_timer_ptr) = acumuladorX; //reconfiguramos el timer3 para cambiar de grados
			*(HPS_TIMER3_timer_ptr + 2) = 0b011;	//arrancamos el timer3
		
			////////////////////////////////////////
			
		}
		else{
			//acumuladorY=*(HPS_TIMER3_timer_ptr)+1000;
			acumuladorY=acumuladorY+1000;
			/////////////Variar PWM/////////////////

			*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina
			*(HPS_TIMER3_timer_ptr) = acumuladorY;
			*(HPS_TIMER3_timer_ptr + 2) = 0b011;
		
			////////////////////////////////////////
	
		}
		*HEX_ptr= 0x4F;//3, esto es para ver el boton que estas tocando
	}

	if((*(KEY_ptr + 3) & 0b0100) !=0){
		
		if(flagServo){
			//acumuladorX=*(HPS_TIMER3_timer_ptr)-1000;
			acumuladorX=acumuladorX-1000;
			/////////////Variar PWM/////////////////

			*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina
			*(HPS_TIMER3_timer_ptr) = acumuladorX;
			*(HPS_TIMER3_timer_ptr + 2) = 0b011;
		
			////////////////////////////////////////
		}
		else{
			//acumuladorY=*(HPS_TIMER3_timer_ptr)-1000;
			acumuladorY=acumuladorY-1000;
			/////////////Variar PWM/////////////////

			*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina
			*(HPS_TIMER3_timer_ptr) = acumuladorY;
			*(HPS_TIMER3_timer_ptr + 2) = 0b011;
		
			////////////////////////////////////////
	
		}

		*HEX_ptr= 0x66;//4
	}

	/*********************************************************************************/


	/************************************
	* RUTINA DE TRATAMIENTO ACELEROMETRO*
	*************************************/
	

	/*************************************/
   
   // Clear the edgecapture register (clears current interrupt)
   *(KEY_ptr + 3) = 0xF; 
    
   return (irq_handler_t) IRQ_HANDLED;
}




irq_handler_t irq_handler_Timer2(int irq, void *dev_id, struct pt_regs *regs)
{
    

	//Activamos TIMER3 y su interrupción
	*(HPS_TIMER3_timer_ptr + 2) = 0b1;
	*(HPS_TIMER3_timer_ptr + 3);

	//Flanco de subida en el PWM
	//*JP2_ptr = 1<<7;
	//*JP2_ptr = 1<<5;
	if(flagServo == 0){
 		//*JP2_ptr = 1<<7;
		*JP2_ptr = *JP2_ptr | 0x80; //El timer2 pone un 1 en el registro hasta que salta el timer3
	}
	if (flagServo ==1){
		*JP2_ptr = *JP2_ptr | 0x20;
	}
		



    *(HPS_TIMER2_timer_ptr + 3); 
	
   return (irq_handler_t) IRQ_HANDLED;
}

irq_handler_t irq_handler_Timer3(int irq, void *dev_id, struct pt_regs *regs)
{
   	//Flanco de bajada para el PWM
	//*JP2_ptr = 0<<7;
	//*JP2_ptr = 0<<5;
    if(flagServo == 0){
 		//*JP2_ptr = 0<<7;	//11111111111111111111111101111111
		*JP2_ptr = *JP2_ptr & 0xFFFFFF7F; //Cuando salta el timer3 pone a 0 el registro correspondiente
	}
	if (flagServo ==1){
		//*JP2_ptr = 0<<5;
		*JP2_ptr = *JP2_ptr & 0xFFFFFFDF; //11111111111111111111111111011111
	}
	
	//Paramos el TIMER3
	*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, no se lanzará hasta que se ejecute la rutina de interrupción del timer2
   return (irq_handler_t) IRQ_HANDLED;
}


static int __init initialize_pushbutton_handler(void)
{
   int value;
   
   contador=0;
   
   
   // generate a virtual address for the FPGA lightweight bridge
   LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
  
  
	HPS_TIMER2_virtual = ioremap_nocache (HPS_TIMER2, HPS_TIMER2_SPAN);
	HPS_TIMER3_virtual = ioremap_nocache (HPS_TIMER3, HPS_TIMER3_SPAN);
	
	
	//Set virtual address pointer to I/O port
	
    HPS_TIMER2_timer_ptr = (unsigned int *) (HPS_TIMER2_virtual);
	HPS_TIMER3_timer_ptr = (unsigned int *) (HPS_TIMER3_virtual);
	
	//Configuramos el timer aqui////////////////////////////////////////////////////////////////////////
	
    *(HPS_TIMER2_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina
	*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina

	
	*(HPS_TIMER2_timer_ptr) = 500000;
	
	*(HPS_TIMER3_timer_ptr) = 37500; //0 grados
	
   	*(HPS_TIMER2_timer_ptr + 2) = 0b011; 	// mode = 011 : 
								// 0 (1 disable int, 0 enable int), 1 (freerun =1), 1 (enable timer = 1)
								//habilitamos las interrupciones
								//donde pone +2 +4 etc, no se hace así, esto hay que meterlo en un define
								//
	*(HPS_TIMER3_timer_ptr + 2) = 0b011;
	////////////////////////////////////////////////////////////////////////////////////////////////
  

   LEDR_ptr = LW_virtual + LEDR_BASE;  // init virtual address for LEDR port
   *LEDR_ptr = 0x200;                  // turn on the leftmost light
   
   HEX_ptr = LW_virtual + HEX3_HEX0_BASE; //Init HEX
	*HEX_ptr= 0x3F;    						//Put 0 in 7 segment
	
   KEY_ptr = LW_virtual + KEY_BASE;    // init virtual address for KEY port
   // Clear the PIO edgecapture register (clear any pending interrupt)
   *(KEY_ptr + 3) = 0xF; 
   // Enable IRQ generation for the 4 buttons
   *(KEY_ptr + 2) = 0xF; 

	SW_ptr = LW_virtual + SW_BASE;
	*(SW_ptr + 3) = 0xF; 
   	// Enable IRQ generation for the 4 buttons
   	*(SW_ptr + 2) = 0xF; 

   
   	//Activamos el pin D7 como salida
	JP2_ptr= LW_virtual + JP2_BASE; //Mapeo de la dirección
	//*(JP2_ptr+1)= 1<<7; //Ponemos a 1 el registro de D7
	//*(JP2_ptr+1)= 1<<5; //Ponemos a 1 el registro de D5
	*(JP2_ptr+1)=0;			//Limpiamos todo el registro
	*(JP2_ptr+1)=*(JP2_ptr+1)+ 0xA0; //Ponemos a 1 D5 y D7
	
	//*JP2_ptr=*JP2_ptr+0x80; //ponemos un 1 en D7
	//*JP2_ptr=*JP2_ptr+0x20; //ponemos un 1 en D5
	*JP2_ptr=*JP2_ptr+0xA0; //ponemos un 1 en ambos registros
	
	
	acumuladorX=37500;
	acumuladorY=37500;

   // Register the interrupt handler.
   value = request_irq (KEYS_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED, 
      "pushbutton_irq_handler", (void *) (irq_handler));
	  
	

	request_irq (INTERVAL_TIMER_IRQ2, (irq_handler_t) irq_handler_Timer2, IRQF_SHARED, 
      "TIMER2", (void *) (irq_handler_Timer2));

	request_irq (INTERVAL_TIMER_IRQ3, (irq_handler_t) irq_handler_Timer3, IRQF_SHARED, 
      "TIMER3", (void *) (irq_handler_Timer3));
	  
   return value;
   
}

static void __exit cleanup_pushbutton_handler(void)
{
   	*LEDR_ptr = 0; // Turn off LEDs and de-register irq handler
   	free_irq (KEYS_IRQ, (void*) irq_handler);

	free_irq (INTERVAL_TIMER_IRQ2, (void*) irq_handler_Timer2);	
	free_irq (INTERVAL_TIMER_IRQ3, (void*) irq_handler_Timer3);	
   	*HEX_ptr =0;
	*JP2_ptr =0;
	*SW_ptr =0;
   

} 


void HexIncrement(void){
	if(contador >9)
		contador=0;
	
	switch(contador){
		case 1:
			*HEX_ptr= 0x6 ;
			break;
		case 2:
			*HEX_ptr= 0x5B;
			break;
		case 3:
			*HEX_ptr= 0x4F;
			break;
		case 4:
			*HEX_ptr= 0x66;
			break;
		case 5:
			*HEX_ptr= 0x6D;
			break;
		case 6:
			*HEX_ptr= 0x7D;
			break;
		case 7:
			*HEX_ptr= 0x7;
			break;
		case 8:
			*HEX_ptr= 0x7F;
			break;
		case 9:
			*HEX_ptr= 0x67;
			break;
		case 0:
			*HEX_ptr= 0x3F;
			break;
	}
	
}





module_init(initialize_pushbutton_handler);
module_exit(cleanup_pushbutton_handler);

