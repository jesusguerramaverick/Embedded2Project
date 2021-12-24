//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "hibernation.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initHibernationModule()
{
    //provide system clock to the Hibernation Module
    SYSCTL_RCGCHIB_R = 1;
    _delay_cycles(3);

    HIB_CTL_R = HIB_CTL_CLK32EN;       // The 32.768 kHz clock needs to be set before any write
    waitUntilWriteComplete();

    HIB_IM_R = 9;
    waitUntilWriteComplete();
}

void hibernate(uint32_t ctl, uint32_t* time)
{
    HIB_IC_R = 9; //1001
    waitUntilWriteComplete();
    HIB_RTCM0_R = time[0]; // Set match value
    waitUntilWriteComplete();
    HIB_RTCSS_R = time[1]; // set max value to 1111 (30.9999999 - 31)
    waitUntilWriteComplete();
    HIB_RTCLD_R = 0; // Load RTC with 0 to clear counter
    waitUntilWriteComplete();
    HIB_CTL_R = ctl;
    // Bit 0 - RTC enable
    // Bit 1 - Hib request
    // Bit 3 - RTC wake up enable
    // Bit 4 - SW2 (wake pin) enable
    // Bit 6 - Hib mod orginally run on SYSCLK...when hib happens, SYSCLK sleeps...need
    //         second clk source, so enable hib mod clk source
    // Bit 8 - TOLD TO DO THIS...max power saving :D
    waitUntilWriteComplete();
}

bool checkIfConfigured()
{
    return HIB_CTL_R & HIB_CTL_CLK32EN;
}

bool rtcCausedWakeUp()
{
    return HIB_RIS_R & HIB_MIS_RTCALT0;
}

bool wakePinCausedWakeUp()
{
    return HIB_RIS_R & 8;
}

// Software must poll this between write requests and defer
// writes until WRC = 1 to ensure proper operation
void waitUntilWriteComplete()
{
    while (!(HIB_CTL_R & HIB_CTL_WRC));
}

void startTime()
{
    HIB_IC_R = 9; //1001
    waitUntilWriteComplete();
    HIB_RTCSS_R = HIB_RTCSS_RTCSSM_M; // set max value to 1111 (30.9999999 - 31)
    waitUntilWriteComplete();
    HIB_RTCLD_R = 10; // Load RTC with 0 to clear counter
    waitUntilWriteComplete();
    HIB_CTL_R = 0x43; // 0000 0100 0021
    waitUntilWriteComplete();
}

void stopHibernation()
{
    HIB_CTL_R = 0x00; // Cancel hibernation request
    waitUntilWriteComplete();
}

void setRTCMatch(uint32_t time)
{
    HIB_CTL_R &= ~0x1;
    waitUntilWriteComplete();
    HIB_IC_R = 9;
    waitUntilWriteComplete();
    HIB_RTCLD_R = 1;
    waitUntilWriteComplete();
    uint32_t newTime = (time * 33);
    uint32_t addToRTC = newTime / 32768;
    newTime = newTime % 32768;
    HIB_RTCM0_R = addToRTC + 1;
    waitUntilWriteComplete();
    HIB_RTCSS_R = 0;
    waitUntilWriteComplete();
    HIB_RTCSS_R |= ((newTime) << 16);
    waitUntilWriteComplete();
    HIB_CTL_R = 0x159;
    waitUntilWriteComplete();
}

