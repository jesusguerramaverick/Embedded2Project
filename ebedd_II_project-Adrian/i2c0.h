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

#ifndef I2C0_H_
#define I2C0_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initI2c0(void);
// For simple devices with a single internal register
void writeI2c0Data(uint8_t add, uint8_t data);
uint8_t readI2c0Data(uint8_t add);
// For devices with multiple registers
void writeI2c0Register(uint8_t add, uint8_t reg, uint8_t data); // Write to one register
void writeI2c0Registers(uint8_t add, uint8_t reg, uint8_t data[], uint8_t size); // Write to multiple registers
uint8_t readI2c0Register(uint8_t add, uint8_t reg);
uint8_t readI2c0Register16(uint8_t add, uint16_t reg); // Read from devices that req 16 bit adds
bool pollI2c0Address(uint8_t add);
bool isI2c0Error(void);

#endif

