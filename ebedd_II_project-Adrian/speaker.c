// speaker.c

#include <stdint.h>//header files
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "adc0.h"
#include "uart0.h"

#define SPEAKER (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 1*4)))//PD1 bitband


void initspeakerHw()//function to initialize all hardware
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    SYSCTL_RCGCACMP_R = 1;//Enable the analog comparator clock by writing a value of 0x0000.0001
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1| SYSCTL_RCGCTIMER_R2;//enable timer 1 and timer 2
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2 | SYSCTL_RCGCGPIO_R1| SYSCTL_RCGCGPIO_R4| SYSCTL_RCGCGPIO_R3;
    _delay_cycles(3);//enable and provides a clock to GPIO ports B,C,D,and E


    GPIO_PORTD_DIR_R |= 1;//lab8 stuff
    GPIO_PORTD_DEN_R |= 1;//enable PortD 0, and 1

    GPIO_PORTD_DIR_R |= 2;
    GPIO_PORTD_DEN_R |= 2;

    GPIO_PORTD_AMSEL_R|= 1| 2;//The analog function of the pin is enabled, the isolation is
    //disabled, and the pin is capable of analog functions.


    GPIO_PORTD_DR2R_R |= SPEAKER;//GPIO_PORTD_DR2R_R |= MOTOR | SPEAKER;

    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;//enable timer 2
    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;
    TIMER2_TAMR_R =0x0;//write 0 to timer value
    TIMER2_TAILR_R = 40000000;//set load value to 40e6 for 1Hz interrupt rate
    TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)

}


// Periodic timer
   void timer2Isr()
   {
       SPEAKER^= 1;
       TIMER2_ICR_R = TIMER_ICR_TATOCINT; // clear interrupt flag
   }

uint16_t magY()
{
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register
    writeI2c0Register(0x68, 0x37, 0x02);
    writeI2c0Register(0x0C, 0x0A, 0x01);
    while(!(readI2c0Register(0x0C, 0x02) & 1));
    uint16_t y = readI2c0Register(0x0C, 0x06);
    y = (y << 8) | readI2c0Register(0x0C, 0x05);

    return y;
}


bool jostleCheck(){
    uint16_t first= magY();
    waitMicrosecond(5000000);
    uint16_t second= magY();
           if ((second-first)>=200){
               return true;
           }
}

   void playAlert(){//function to play battery low melody(4 tones)

       // TODO add while not jostled
       TIMER2_TAMR_R =0x2;//enables periodic timer mode
       TIMER2_TAILR_R = 45454;                       // set load value to 40e6 for 1 Hz interrupt rate
       TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
       NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
       TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
       waitMicrosecond(363636);
       TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer


       TIMER2_TAILR_R = 90908;                       // set load value to 40e6 for 1 Hz interrupt rate
       TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
       NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
       TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
       waitMicrosecond(363636);
       TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer


       TIMER2_TAILR_R = 181816;                       // set load value to 40e6 for 1 Hz interrupt rate
       TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
       NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
       TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
       waitMicrosecond(363636);
       TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer


       TIMER2_TAILR_R = 45454;                       // set load value to 40e6 for 1 Hz interrupt rate
       TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
       NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
       TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
       waitMicrosecond(363636);
       TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer

   }
