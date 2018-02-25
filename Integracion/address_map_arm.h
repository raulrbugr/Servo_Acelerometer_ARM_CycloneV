////////////////////////////////////////////////////////////////////
// address_map_arm.h
// Algunas definiciones de direcciones del ARM (DE1 Soc Ccomputer)
//
/////////////////////////
//
//////////////////////////////////////////////* Memory */
#define DDR_BASE              0x00000000
#define DDR_SPAN              0x3FFFFFFF
#define A9_ONCHIP_BASE        0xFFFF0000
#define A9_ONCHIP_SPAN        0x0000FFFF
#define SDRAM_BASE            0xC0000000
#define SDRAM_SPAN            0x03FFFFFF
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_ONCHIP_SPAN      0x0003FFFF
#define FPGA_CHAR_BASE        0xC9000000
#define FPGA_CHAR_SPAN        0x00001FFF

/* A9  and HPS Timers */
#define A9_PRIVATE_TIMER           0xFFFEC600
#define A9_PRIVATE_TIMER_SPAN      0x00001000

#define HPS_TIMER0            0xFFC08000
#define HPS_TIMER0_SPAN       0x00000014
#define HPS_TIMER1            0xFFC09000
#define HPS_TIMER1_SPAN       0x00000014
#define HPS_TIMER2            0xFFD00000
#define HPS_TIMER2_SPAN       0x00000014
#define HPS_TIMER3            0xFFD01000
#define HPS_TIMER3_SPAN       0x00000014

#define HPS_LEDSG             0xFF709000
#define HPS_LEDSG_SPAN        0x00000014

/* Cyclone V FPGA devices */
#define LW_BRIDGE_BASE        0xFF200000

#define LEDR_BASE             0x00000000
#define HEX3_HEX0_BASE        0x00000020
#define HEX5_HEX4_BASE        0x00000030
#define SW_BASE               0x00000040
#define KEY_BASE              0x00000050
#define JP1_BASE              0x00000060
#define JP2_BASE              0x00000070
#define PS2_BASE              0x00000100
#define PS2_DUAL_BASE         0x00000108
#define JTAG_UART_BASE        0x00001000
#define JTAG_UART_2_BASE      0x00001008
#define IrDA_BASE             0x00001020
#define TIMER1_BASE           0x00002000
#define TIMER2_BASE           0x00002020
#define AV_CONFIG_BASE        0x00003000
#define PIXEL_BUF_CTRL_BASE   0x00003020
#define CHAR_BUF_CTRL_BASE    0x00003030
#define AUDIO_BASE            0x00003040
#define VIDEO_IN_BASE         0x00003060
#define ADC_BASE              0x00004000

#define LW_BRIDGE_SPAN                  0x00005000
