//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC: TM4C123GH6PM
// System Clock: 40 MHz

// Address 0x0000 - 0x0004 are reserved for the user set time and date

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "gpio.h"
#include "hibernation.h"
#include "wait.h"
#include "i2c0.h"
#include "string.h"
#include "time.h"
#include "utility.h"
#include "command.h"
#include "log.h"

// PortB masks for I2C
#define SDA_MASK 8
#define SCL_MASK 4

// Pins
#define LIGHT        PORTA,4 // 1 when light is on and 0 when light is off
#define LEVELSELECT  PORTA,7
#define I2C0SCL      PORTB,2
#define I2C0SDA      PORTB,3
#define RED_LED      PORTF,1
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


// Initialize Hardware
void initHw()
{
     //Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();//change to 400 kHz

    initI2c0();//initialize I2C
    initUart0();
    initAdc0Ss3();//for temperature

    enablePort(PORTA);// Enables port A sets LevelSelect as an output
    selectPinPushPullOutput(LEVELSELECT);
    setPinValue(LEVELSELECT, 1);
    selectPinDigitalInput(LIGHT);

    // Set up on board LEDs
    enablePort(PORTF);
    selectPinPushPullOutput(RED_LED);

    waitMicrosecond(5); // Let the leveling circuit completely turn on
}

int main(void)
{
    initHw();//initialize hardware

    setPinValue(RED_LED, 1);
    waitMicrosecond(1000000);
    setPinValue(RED_LED, 0);

    if(!checkIfConfigured()) // Only need to run the commands the first time through this code
        initHibernationModule();

    commands();

    while(true);

}
