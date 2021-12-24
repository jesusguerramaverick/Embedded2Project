//log.c

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC: TM4C123GH6PM
// System Clock: 40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "gpio.h"
#include "hibernation.h"
#include "wait.h"
#include "i2c0.h"
#include "string.h"
#include "adc0.h"
#include "utility.h"
#include <stdlib.h>

// PortB masks for I2C
#define SDA_MASK 8
#define SCL_MASK 4

// Pins
#define I2C0SCL PORTB,2
#define I2C0SDA PORTB,3
#define LEVELSELECT  PORTA,7

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM

#define CURREG 0x0005
#define EEPROM 0xA0
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool storeEEPROMdata(uint32_t data)
{
    uint16_t add=getNextAdd();
    //uint32_t x = 0x01869F;
    //uint_t a[4]= {x, x>>8, x>>16};
    //add
    //add+1
    //add+2
    uint8_t i2cData[] = {LB(add), data>>16, data>>8, data};//Array for address low byte and data you are storing
    writeI2c0Registers(0xA0 >> 1, HB(add), i2cData, 4);//Writes to address in EEPROM using address high byte, array of address low byte, and 2 for size
    waitMicrosecond(10000);

    uint32_t readEEprom = readI2c0Register16(0xA0 >> 1, add);
    readEEprom= readEEprom<<8;
    uint8_t dummy= readI2c0Register16(0xA0 >> 1, add+1);
    readEEprom+=dummy;
    readEEprom= readEEprom<<8;
    dummy=readI2c0Register16(0xA0 >> 1, add+2);
    readEEprom+=dummy;


    if(readEEprom!= data) //data was time
        return false;

    uint8_t data2[] = {LB(CURREG), HB(add+2), LB(add+2)};
    writeI2c0Registers(EEPROM >> 1, HB(CURREG), data2, 3);
    waitMicrosecond(10000);

    return true;
}

bool logGyro()
{
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register
    writeI2c0Register(0x68, 0x1B, 0x18);//register 27 gyro write a value of 00011011(0x1B) or 0x18

    //int8_t gyroreg = readI2c0Register(0x68, 0x1B);//*change these lines if needed for accelerometer* this is just for checking that you wrote to register correctly
    uint16_t gyroreg2 = readI2c0Register(0x68, 0x43);//register 68 page 33  GYRO_XOUT_H
    uint8_t gyroreg3 = readI2c0Register(0x68, 0x44);//register 68 page 33  GYRO_XOUT_L
    uint16_t gyroX = (gyroreg2<<8) | gyroreg3;//combined high byte and low byte
    uint16_t gyroregy = readI2c0Register(0x68, 0x45);//register 68 page 33  GYRO_YOUT_H
    uint8_t gyroregy2 = readI2c0Register(0x68, 0x46);//register 68 page 33  GYRO_YOUT_L
    uint16_t gyroY = (gyroregy<<8) | gyroregy2;//combined high byte and low byte
    uint16_t gyroregz = readI2c0Register(0x68, 0x47);//register 68 page 33  GYRO_ZOUT_H
    uint8_t gyroregz2 = readI2c0Register(0x68, 0x48);//register 68 page 33  GYRO_ZOUT_L
    uint16_t gyroZ = (gyroregz<<8) | gyroregz2;//combined high byte and low byte

    if (!storeEEPROMdata(gyroX))
        return false;
    if (!storeEEPROMdata(gyroY))
        return false;
    if (!storeEEPROMdata(gyroZ))
        return false;

    return true;
}

bool logAcc()
{
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register
    writeI2c0Register(0x68, 0x1C, 0x18);//configure register 28 page 14  accelerometer

    uint16_t accregx = readI2c0Register(0x68, 0x3B);//register 68 page 31  ACC_XOUT_H 59
    uint8_t accregx2 = readI2c0Register(0x68, 0x3C);//register 68 page 31  ACC_XOUT_L 60
    uint16_t accX = (accregx<<8) | accregx2;//combined high byte and low byte
    uint16_t accregy = readI2c0Register(0x68, 0x3D);//register 68 page 31  ACC_YOUT_H 61
    uint8_t accregy2 = readI2c0Register(0x68, 0x3E);//register 68 page 31  ACC_YOUT_L 62
    uint16_t accY = (accregy<<8) | accregy2;//combined high byte and low byte
    uint16_t accregz = readI2c0Register(0x68, 0x3F);//register 68 page 31  ACC_ZOUT_H 63
    uint8_t accregz2 = readI2c0Register(0x68, 0x40);//register 68 page 31  ACC_ZOUT_L 64
    uint16_t accZ = (accregz<<8) | accregz2;//combined high byte and low byte

    if (!storeEEPROMdata(accX))
           return false;
    if (!storeEEPROMdata(accY))
           return false;
    if (!storeEEPROMdata(accZ))
           return false;

       return true;
}


bool logMag()
{
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register
    writeI2c0Register(0x68, 0x37, 0x02);
    writeI2c0Register(0x0C, 0x0A, 0x01);
    while(!(readI2c0Register(0x0C, 0x02) & 1));
    uint16_t x = readI2c0Register(0x0C, 0x04);
    x = (x << 8) | readI2c0Register(0x0C, 0x03);
    uint16_t y = readI2c0Register(0x0C, 0x06);
    y = (y << 8) | readI2c0Register(0x0C, 0x05);
    uint16_t z = readI2c0Register(0x0C, 0x08);
    z = (z << 8) | readI2c0Register(0x0C, 0x07);

    if (!storeEEPROMdata(x))
           return false;
    if (!storeEEPROMdata(y))
           return false;
    if (!storeEEPROMdata(z))
           return false;

       return true;
}

bool logTemp()
{
    volatile long ADC_Output=0;
    volatile long temp_c=0;
    setAdc0Ss3Mux(1);
    ADC_Output= (readAdc0Ss3() & 0xFFF);
    temp_c= 147.5-((247.5*ADC_Output)/4096);//equation for converting temp to celsius

    if (!storeEEPROMdata(temp_c))
           return false;

       return true;
}
