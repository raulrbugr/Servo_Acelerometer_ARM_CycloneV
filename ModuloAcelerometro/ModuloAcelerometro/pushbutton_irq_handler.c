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
#define COUNT_100HZ 100 				//contador 100Hz


/*----------------------------------------------*/
/*------------------Prototipos------------------*/
/*----------------------------------------------*/
void init_global(void);
void setup_fpga_timer1(void);
void setup_interruptions(void);
void setup_jp2_port(void);


/*----------------------------------------------*/
/*-------------Variables Globales---------------*/
/*----------------------------------------------*/

void * LW_virtual;         // Lightweight bridge base address

//leds
volatile int *LEDR_ptr;    // virtual address for the LEDR port

//fpga timer1
volatile int *TIMER1_sts, *TIMER1_ctrl, *TIMER1_low, *TIMER1_high;  // virtual address for the fpga timer1
int p_1hz_low, p_1hz_high, fpga_timer_100hz_counter, fpga_timer_50hz_counter;

//jp2
volatile int *JP2_ptr;

//accelerometro
int acc_x_count; //contador 0 -> 100 para x-axis accelerometro (100veces en 100Hz -> 1Hz)
int acc_y_count; //contador 0 -> 100 para y-axis accelerometro (100veces en 100Hz -> 1Hz)


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

        //visualización accelerometro eje y
        *LEDR_ptr = (acc_y_count & 0x7E)<<4; // desplazamiento para usar valores de 6 bits para eliminar ruido
        //visualización accelerometro eje x
        *LEDR_ptr = (acc_x_count & 0x7E)>>1; // desplazamiento para usar valores de 6 bits para eliminar ruido

				
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


static int __init initialize_pushbutton_handler(void){
    init_global();
	
    setup_fpga_timer1();
	
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

    fpga_timer_100hz_counter = 0;


    // accelerometer counter
    acc_x_count = 0;
    acc_y_count = 0;
    
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


void setup_interruptions(void){
	//FPGA Timer interruptions
    request_irq (INTERVAL_FPGA_TIMER_IRQ1, (irq_handler_t) irq_handler_FPGA_Timer1, IRQF_SHARED, 
        "FPGA_TIMER1", (void *) (irq_handler_FPGA_Timer1));
}


static void __exit cleanup_pushbutton_handler(void)
{
    *LEDR_ptr = 0; // Turn off LEDs and de-register irq handler
    free_irq (INTERVAL_FPGA_TIMER_IRQ1, (void*) irq_handler_FPGA_Timer1);
    *(TIMER1_ctrl) = 0b1000;
    *JP2_ptr = 0;
}


module_init(initialize_pushbutton_handler);
module_exit(cleanup_pushbutton_handler);