//command.c

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
#include "adc0.h"


void printTemp()
{
    volatile long ADC_Output=0;
    volatile long temp_c=0;
    setAdc0Ss3Mux(1);
    ADC_Output = (readAdc0Ss3() & 0xFFF);
    temp_c = 147.5-((247.5*ADC_Output)/4096);//equation for converting temp to celsius

    char buffer[128];

    itoa(temp_c, buffer, 10);

    putsUart0("Current temperature: ");
    putsUart0(buffer);
    putsUart0("\n\n");
}
