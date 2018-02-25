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

#define PERIODO_1HZ 100000 				// Periodo FPGA Timer1 -> 1Hz (FPGA Timer1 100Mhz)
#define PERIODO_10US 25000 				// Periodo FPGA Timer2 -> 10us (FPGA Timer2 100Mhz)
#define PERIODO_20000US 500000 			//Periodo HPS Timer2 -> 20000us (HPS Timer2 50Hz)
#define COUNT_100HZ 100 				//contador 100Hz
#define COUNT_50HZ 50 					//contador 50Hz
#define COUNT_2000US 2000 				//contador (50hz) -> 2000 * 10us => 20000us = 50Hz
#define COUNT_1500US 37500				//Valor de 1500us para HPS Timer3


/*----------------------------------------------*/
/*------------------Prototipos------------------*/
/*----------------------------------------------*/
void init_global(void);
void setup_fpga_timer1(void);
void setup_interruptions(void);
void setup_jp2_port(void);
void setup_HPS_timer2(void);
void setup_HPS_timer3(void);
unsigned int map(int x, unsigned int in_min, unsigned int in_max, unsigned int out_min,unsigned int out_max);
void Servo_Movement(void);


/*----------------------------------------------*/
/*-------------Variables Globales---------------*/
/*----------------------------------------------*/

void * LW_virtual;         // Lightweight bridge base address

//leds
volatile int *LEDR_ptr;    // virtual address for the LEDR port

//fpga timer1
volatile int *TIMER1_sts, *TIMER1_ctrl, *TIMER1_low, *TIMER1_high;  // virtual address for the fpga timer1
int p_1hz_low, p_1hz_high, fpga_timer_100hz_counter, fpga_timer_50hz_counter;

//fpga timer2
volatile int *TIMER2_sts, *TIMER2_ctrl, *TIMER2_low, *TIMER2_high;  // virtual address for the fpga timer2
int p_10us_low, fpga_timer_2000us_counter;

//HPS Timer2
volatile unsigned int * HPS_TIMER2_timer_ptr; // virtual address A9 timer base address
void *HPS_TIMER2_virtual;   // used to map physical addresses for the light-weight bridge

//HPS Timer3
volatile unsigned int * HPS_TIMER3_timer_ptr; // virtual address A9 timer base address
void *HPS_TIMER3_virtual;   // used to map physical addresses for the light-weight bridge

//jp2
volatile int *JP2_ptr;

//accelerometro
int acc_x_count; //contador 0 -> 100 para x-axis accelerometro (100veces en 100Hz -> 1Hz)
int acc_y_count; //contador 0 -> 100 para y-axis accelerometro (100veces en 100Hz -> 1Hz)

//servomotores
int servo_x_count, servo_x_flag;
int acumuladorX;
int acumuladorY;
int servoflag;


irq_handler_t irq_handler_FPGA_Timer1(int irq, void *dev_id, struct pt_regs *regs){
	
    *(TIMER1_ctrl) = 0b1000; //timer stop

    // conteo status accelerometro x-axis
    if((*(JP2_ptr) & 0x80000) !=0){ //chequeo estado JP2 pin 19
        acc_x_count++;     
    }
    // conteo status accelerometro y-axis
    if((*(JP2_ptr) & 0x20000) !=0){ //chequeo estado JP2 pin 17
        acc_y_count++;     
    }

    //chequeo 100Hz
    if(fpga_timer_100hz_counter >= COUNT_100HZ){

       // *LEDR_ptr = (acc_x_count & 0x7E)>>1; // desplazamiento para usar valores de 6 bits para eliminar ruido
		
		//Movimiento del servo correspondiente
		Servo_Movement();
				
        //reinicialización de conteo cada 100Hz
        fpga_timer_100hz_counter = 0;
		
		acc_x_count = 0;
        acc_y_count = 0;
    }else{
        fpga_timer_100hz_counter++;
    }

    // end of interruption
    *(TIMER1_sts) = 0; //borrar flag interrupción
	*(TIMER1_ctrl) = 0b0101; // reactivación timer
	return (irq_handler_t) IRQ_HANDLED;
}



irq_handler_t irq_handler_Timer2(int irq, void *dev_id, struct pt_regs *regs)
{
    

	//Activamos TIMER3 y su interrupción
	*(HPS_TIMER3_timer_ptr + 2) = 0b1;
	*(HPS_TIMER3_timer_ptr + 3);

	//Alternamos el movimiento entre ambos servos
	if(servoflag < 1){
 		//*JP2_ptr = 1<<7;
		*JP2_ptr = *JP2_ptr | 0x80; //El timer2 pone un 1 en el registro hasta que salta el timer3
	}
	else{
		*JP2_ptr = *JP2_ptr | 0x20;
	}
		

    *(HPS_TIMER2_timer_ptr + 3); 
	
   return (irq_handler_t) IRQ_HANDLED;
}

irq_handler_t irq_handler_Timer3(int irq, void *dev_id, struct pt_regs *regs)
{
   		
	//Alternamos el movimiento entre ambos servos
    if(servoflag < 1){
 		//*JP2_ptr = 0<<7;	//11111111111111111111111101111111
		*JP2_ptr = *JP2_ptr & 0xFFFFFF7F; //Cuando salta el timer3 pone a 0 el registro correspondiente
	}
	else{
		
		*JP2_ptr = *JP2_ptr & 0xFFFFFFDF; //11111111111111111111111111011111
	}
	
	//Paramos el TIMER3
	*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, no se lanzará hasta que se ejecute la rutina de interrupción del timer2
   return (irq_handler_t) IRQ_HANDLED;
}


static int __init initialize_pushbutton_handler(void){
    init_global();
	
    setup_fpga_timer1();
    	
	setup_HPS_timer2();
	setup_HPS_timer3();
	
    setup_jp2_port();

    setup_interruptions(); 
}

void init_global(void){
    // generate a virtual address for the FPGA lightweight bridge
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

    LEDR_ptr = LW_virtual + LEDR_BASE;  // init virtual address for LEDR port

    JP2_ptr= LW_virtual + JP2_BASE; // init virtual address for JP2 port
    
    // fpga timer1 period (1Hz)
    p_1hz_low = PERIODO_1HZ & 0x0000FFFF;
    p_1hz_high = (PERIODO_1HZ & 0xFFFF0000)>>16;

    // fpga timer2 period (10us)
    p_10us_low = PERIODO_10US & 0x0000FFFF;

    fpga_timer_100hz_counter = 0;
    fpga_timer_50hz_counter = 0;

    // accelerometer counter
    acc_x_count = 0;
    acc_y_count = 0;

    // servomotor counter
    servo_x_count = 150;
	servoflag=0;
	acumuladorX=COUNT_1500US; //init value for 0 grads
	acumuladorY=COUNT_1500US;
    
}

void setup_jp2_port(void){
    *(JP2_ptr + 1) = 0; // init JP2 direction register out
    *(JP2_ptr + 2) = 0xFFFF; // enable JP2 interruptions
    *(JP2_ptr + 3) = 0xFFFF; // clean JP2 edgecapture register

    // activate D5 & D7 pins
    *(JP2_ptr + 1) += 0xA0;
}

void setup_fpga_timer1(void){
    //FPGA TIMER
    TIMER1_sts = LW_virtual + TIMER1_BASE;
    TIMER1_ctrl = TIMER1_sts + 1;
    TIMER1_low = TIMER1_sts + 2;
    TIMER1_high = TIMER1_sts + 3;

    //stop timer
    *(TIMER1_ctrl) = 0b1000;

    //load period
    *(TIMER1_low) = p_1hz_low; //counter start value (low)
    *(TIMER1_high) = p_1hz_high; //counter start value (low)

    //init timer
    *(TIMER1_ctrl) = 0b0101;
}



void setup_HPS_timer2(void){
	
	//HPS TIMER
	HPS_TIMER2_virtual = ioremap_nocache (HPS_TIMER2, HPS_TIMER2_SPAN);
	HPS_TIMER2_timer_ptr = (unsigned int *) (HPS_TIMER2_virtual);
	
	//stop timer
	*(HPS_TIMER2_timer_ptr + 2) = 0b0; // mode = STOP
	
	//Load period
	*(HPS_TIMER2_timer_ptr) = PERIODO_20000US;
	
	//init timer
	*(HPS_TIMER2_timer_ptr + 2) = 0b011; 
}

void setup_HPS_timer3(void){
	
	//HPS TIMER
	HPS_TIMER3_virtual = ioremap_nocache (HPS_TIMER3, HPS_TIMER3_SPAN);
	HPS_TIMER3_timer_ptr = (unsigned int *) (HPS_TIMER3_virtual);
	
	//stop timer
	*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP
	
	//Load period
	*(HPS_TIMER3_timer_ptr) = COUNT_1500US; //0 grads
	
	//init timer
	*(HPS_TIMER3_timer_ptr + 2) = 0b011; 
}

unsigned int map(int x, unsigned int in_min, unsigned int in_max, unsigned int out_min,unsigned int out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  
  
  
}

void Servo_Movement(void){
	if(servoflag<1){
		//Convertimos medida, mapeada entre rangos
		acumuladorY=map(acc_y_count, 36, 64, 32500, 42500); //26250-> +45º, 48750 -> -45º /// 32500 -> -20º, 42500 -> +20º
				

		*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina
		*(HPS_TIMER3_timer_ptr) = acumuladorY; //reconfiguramos el timer3 para cambiar de grados
		*(HPS_TIMER3_timer_ptr + 2) = 0b011;	//arrancamos el timer3
		servoflag=1;
	}
	else{
		//Convertimos medida, mapeada entre rangos
		acumuladorX=map(acc_x_count, 36, 64, 32500, 42500); //26250-> +45grados, 48750 -> -45grados
				

		*(HPS_TIMER3_timer_ptr + 2) = 0b0; // mode = STOP, primero apagamos la maquina
		*(HPS_TIMER3_timer_ptr) = acumuladorX; //reconfiguramos el timer3 para cambiar de grados
		*(HPS_TIMER3_timer_ptr + 2) = 0b011;	//arrancamos el timer3
		servoflag=0;
	}
	
}

void setup_interruptions(void){
	//FPGA Timer interruptions
    request_irq (INTERVAL_FPGA_TIMER_IRQ1, (irq_handler_t) irq_handler_FPGA_Timer1, IRQF_SHARED, 
        "FPGA_TIMER1", (void *) (irq_handler_FPGA_Timer1));
   		
	//HPS Timer interruptions
	request_irq (INTERVAL_TIMER_IRQ2, (irq_handler_t) irq_handler_Timer2, IRQF_SHARED, 
      "TIMER2", (void *) (irq_handler_Timer2));
	request_irq (INTERVAL_TIMER_IRQ3, (irq_handler_t) irq_handler_Timer3, IRQF_SHARED, 
      "TIMER3", (void *) (irq_handler_Timer3));
}


static void __exit cleanup_pushbutton_handler(void)
{
    *LEDR_ptr = 0; // Turn off LEDs and de-register irq handler
    free_irq (INTERVAL_FPGA_TIMER_IRQ1, (void*) irq_handler_FPGA_Timer1);
	free_irq (INTERVAL_TIMER_IRQ2, (void*) irq_handler_Timer2);	
	free_irq (INTERVAL_TIMER_IRQ3, (void*) irq_handler_Timer3);	
    *(TIMER1_ctrl) = 0b1000;
    *(TIMER2_ctrl) = 0b1000;
    *JP2_ptr = 0;
}


module_init(initialize_pushbutton_handler);
module_exit(cleanup_pushbutton_handler);