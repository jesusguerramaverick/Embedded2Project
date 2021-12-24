// I2C0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// I2C devices on I2C bus 0 with 2kohm pullups on SDA and SCL

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "i2c0.h"

// PortB masks
#define SDA_MASK 8
#define SCL_MASK 4

// Pins
#define I2C0SCL PORTB,2
#define I2C0SDA PORTB,3

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initI2c0(void)
{
    // Enable clocks
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;
    _delay_cycles(3);
    enablePort(PORTB); // I2C is on Port B

    // Configure I2C
    selectPinPushPullOutput(I2C0SCL);
    setPinAuxFunction(I2C0SCL, GPIO_PCTL_PB2_I2C0SCL);
    selectPinOpenDrainOutput(I2C0SDA);
    setPinAuxFunction(I2C0SDA, GPIO_PCTL_PB3_I2C0SDA);

    // Configure I2C0 peripheral
    I2C0_MCR_R = 0;                                     // disable to program
    I2C0_MTPR_R = 199;                                  // (40MHz/2) / (199+1) = 100kbps
    I2C0_MCR_R = I2C_MCR_MFE;                           // master
    I2C0_MCS_R = I2C_MCS_STOP;
}

// For simple devices with a single internal register
void writeI2c0Data(uint8_t add, uint8_t data)
{
    I2C0_MSA_R = add << 1; // add:r/~w=0 THIS IS THE ADDRESS REGISTER...lowest bit is R/W
    I2C0_MDR_R = data;     // writing data to data register
    I2C0_MICR_R = I2C_MICR_IC;  // We clear this because we need it to see if the transmission completes
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP; // this says start, run and send data, then stop
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0); // this will be set to 1 when the transmission is complete because this chip is broke
}

uint8_t readI2c0Data(uint8_t add)
{
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;  // Clear interrupt
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP; // this says start, run and read data, then stop
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0); // Wait for hardware to make this 1 so we know the transmission is complete
    return I2C0_MDR_R; // return data register
}

// For devices with multiple registers
void writeI2c0Register(uint8_t add, uint8_t reg, uint8_t data)
{
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = reg;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN; // Start and send data...DO NOT STOP BECAUSE WE ARE NOT FINISHED
                                              // This run transmits the register #
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MDR_R = data;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP; // This run transmits the data
    while (!(I2C0_MRIS_R & I2C_MRIS_RIS));
}

// Write to multiple registers
void writeI2c0Registers(uint8_t add, uint8_t reg, uint8_t data[], uint8_t size)//changes for eepromp
{
    uint8_t i;
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = reg;
    if (size == 0)
    {
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    }
    else
    {
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        for (i = 0; i < size-1; i++)
        {
            I2C0_MDR_R = data[i];
            I2C0_MICR_R = I2C_MICR_IC;
            I2C0_MCS_R = I2C_MCS_RUN;
            while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        }
        I2C0_MDR_R = data[size-1];
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    }
}

uint8_t readI2c0Register(uint8_t add, uint8_t reg)
{
    I2C0_MSA_R = add << 1; // add:r/~w=0 ...write because you must write the register #
    I2C0_MDR_R = reg;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN; // Transmit the register #
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1 ...Now read to read from register
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP; //RESTART, transmit, stop
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C0_MDR_R;
}

uint8_t readI2c0Register16(uint8_t add, uint16_t reg)
{
    I2C0_MSA_R = add << 1; // Write address
    I2C0_MDR_R = (reg >> 8) & 0xFF; // Get HB of add
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN; // Transmit HB of add
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MDR_R = reg & 0xFF; // Get LB of add
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_RUN; // Transmit LB
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MSA_R = (add << 1) | 1; // BEGIN READ
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP; // Complete read and stop
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C0_MDR_R;
}

// Checks every address on board to see if anything is attached and what that device's address is
bool pollI2c0Address(uint8_t add)
{
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return !(I2C0_MCS_R & I2C_MCS_ERROR);
}

bool isI2c0Error(void)
{
    return (I2C0_MCS_R & I2C_MCS_ERROR);
}

