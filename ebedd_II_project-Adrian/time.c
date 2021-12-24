// time.c
// This file holds the commands for time change from seconds to Day, Month Year H:M:S

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "time.h"
#include "string.h"
#include "i2c0.h"
#include "wait.h"
#include "uart0.h"
#include "utility.h"

/*
 * Legend for reading months as numbers:
 * 1   -   Jan
 * 2   -   Feb
 * 3   -   Mar
 * 4   -   Apr
 * 5   -   May
 * 6   -   Jun
 * 7   -   Jul
 * 8   -   Aug
 * 9   -   Sept
 * 10  -   Oct
 * 11  -   Nov
 * 12  -   Dec
 */

#define DAYSEC 86400 // Amount of seconds in a day

// EEPROM
#define ADDMON 0x0000 // 1-12 - 0x1 - 0xC
#define ADDDAY 0x0001 // 1 - 31 - 0x01 - 0x
#define ADDHR  0x0002 // 0 - 24 - 0x00 - 0x
#define ADDMIN 0x0003 // 0 - 59 - 0x00 - 0x
#define ADDSEC 0x0004 // 0 - 59 - 0x00 - 0x
#define CURREG 0x0005 // Will hold value of the last register used
#define EEPROM 0xA0  // Add of EEPROM

// HIBDATA
#define HIBMON  (*((volatile uint32_t *)(0x400FC030 + (6*4)))) // Month
#define HIBDAY  (*((volatile uint32_t *)(0x400FC030 + (7*4)))) // Day
#define HIBHR   (*((volatile uint32_t *)(0x400FC030 + (8*4)))) // Hour
#define HIBMIN  (*((volatile uint32_t *)(0x400FC030 + (9*4)))) // Minute
#define HIBSEC  (*((volatile uint32_t *)(0x400FC030 + (10*4)))) // Second

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM

// Getters
// Returns as {Mon, Day, Hr, Min, Sec}
void getOrigDateTime(uint8_t* origDate)
{
    origDate[0] = readI2c0Register16(EEPROM >> 1, ADDMON);
    origDate[1] = readI2c0Register16(EEPROM >> 1, ADDDAY);
    origDate[2] = readI2c0Register16(EEPROM >> 1, ADDHR);
    origDate[3] = readI2c0Register16(EEPROM >> 1, ADDMIN);
    origDate[4] = readI2c0Register16(EEPROM >> 1, ADDSEC);

    return;
}

// Will get seconds from the HIBRTCC counter and send to secToDateTime. Get back only time
void outputTime() // TODO still an issue with the RTCC reg
{
    char str[128];
    uint32_t sec = HIB_RTCC_R; // Read the seconds value from the counter
    waitMicrosecond(10000);
    secToDateTime(str, 2, sec);

    putsUart0(str);
    putsUart0("\n\n");
}

// Will get seconds from the HIBRTCC counter and send to secToDateTime. Get back only date
void outputDate()
{
    char str[128];
    uint32_t sec = HIB_RTCC_R; // Read the seconds value from the counter
    waitMicrosecond(10000);
    secToDateTime(str, 1, sec);

    putsUart0(str);
    putsUart0("\n\n");
}

bool getTimeStamp(uint32_t* time)
{
    uint32_t seconds = HIB_RTCC_R;
    time[1] = HIB_RTCSS_R && HIB_RTCSS_RTCSSC_M;
    time[0] = HIB_RTCC_R;

    if(time[0] != HIB_RTCC_R)
        return false;

    return true;
}

// Setters
// Take user input of date and save it...return 0 is successful, 1 if invalid input, and 2 if error storing in EEPROM
uint8_t setUDate(char input[])
{
    // Save month as a number from the legend above
    uint32_t i = 0;
    uint8_t numM, numD, temp;
    uint8_t count = 0;
    uint8_t j = 0;
    char month[128], day[128];

    // Parse input
    while(input[i] != '\0') // While the current character of the input does not equal a
    {                                       // terminating character
        if(input[i] >= 32) // check is input is a printable value
        {
            if(input[i] == 32 && count == 0) // month store in complete, move to day
            {
                month[j] = '\0'; // Put null term on month
                j = 0; // reset j for day
                count++; // increase count to show day is complete
                i++; // move i to next spot in input
            }
            else if(count == 0) // Store values in month
            {
                month[j] = input[i];
                j++; // move j to next spot in month
                i++; // move i to next spot in input
            }
            else if(count == 1) // Store values in day
            {
                day[j] = input[i];
                j++; // move j to next spot in day
                i++; // move i to next spot in input
            }
        }
    }

    day[j] = '\0'; // Put null term on day

    numM = monthToNum(month);
    if(numM == 0) // if the above function returned a 0, then the month was invalid
        return 1;

    numD = myatoi(day);
    if(numD > monthDay(numM)) // if the day input is not a valid day for the month given, return error msg 1
        return 1;

    uint8_t data1[] = {LB(ADDMON), numM};
    writeI2c0Registers(EEPROM >> 1, HB(ADDMON), data1, 2);
    waitMicrosecond(10000);
    if(readI2c0Register16(EEPROM >>1, ADDMON) != numM) // If data was not stored correctly, return error msg 2
        return 2;
    HIBMON = numM; // Store month in HIBDATA
    while(!(HIB_CTL_R & HIB_CTL_WRC));

    uint8_t data2[] = {LB(ADDDAY), numD};
    writeI2c0Registers(EEPROM >> 1, HB(ADDDAY), data2, 2);
    waitMicrosecond(10000);
    if(readI2c0Register16(EEPROM >>1, ADDDAY) != numD) // If data was not stored correctly, return error msg 2
        return 2;
    HIBDAY = numD; // Store day in HIBDATA
    while(!(HIB_CTL_R & HIB_CTL_WRC));

    return 0; // If not other errors were hit, then the write worked
}

// Take user input of time and save it...return 0 is successful, 1 if invalid input, and 2 if error storing in EEPROM
uint8_t setUTime(char* input)
{
    // Save month as a number from the legend above
    // Change input to sec
    uint32_t i = 0;
    uint8_t numH, numM, numS;
    uint8_t count = 0;
    uint8_t j = 0;
    char hr[128], min[128], sec[128];

    // Parse input
    while(input[i] != '\0') // While the current character of the input does not equal a
    {                                       // terminating character
        if(input[i] >= 32) // check is input is a printable value
        {
            if(input[i] == 32) // space means move to the next field
            {
                if(count == 0) // if hr is complete
                {
                    hr[j] = '\0'; // Put null term on hr
                    j = 0; // reset j for min
                    count++; // increase count to show hr is complete
                    i++; // move i to next spot in input
                }
                else if(count == 1) // if min is complete
                {
                    min[j] = '\0'; // Put null term on min
                    j = 0; // reset j for sec
                    count++; // increase count to show min is complete
                    i++; // move i to next spot in input
                }

            }
            else if(count == 0) // Store values in hr
            {
                hr[j] = input[i];
                j++; // move j to next spot in month
                i++; // move i to next spot in input
            }
            else if(count == 1) // Store values in min
            {
                min[j] = input[i];
                j++; // move j to next spot in day
                i++; // move i to next spot in input
            }
            else if(count == 2)
            {
                sec[j] = input[i];
                j++; // move j to next spot in day
                i++; // move i to next spot in input
            }
        }
    }

    sec[j] = '\0'; // Put null term on day

    numH = myatoi(hr);
    if(numH > 24) // if the above function returned a number below 0 or above 24, then the hr was invalid
        return 1;

    numM = myatoi(min);
    if(numH > 59) // if the above function returned a number below 0 or above 59, then the min was invalid
        return 1;

    numS = myatoi(sec);
    if(numH > 59) // if the above function returned a number below 0 or above 59, then the sec was invalid
        return 1;

    uint8_t data1[] = {LB(ADDHR), numH};
    writeI2c0Registers(EEPROM >> 1, HB(ADDHR), data1, 2);
    waitMicrosecond(10000);
    if(readI2c0Register16(EEPROM >>1, ADDHR) != numH) // If data was not stored correctly, return error msg 2
        return 2;
    HIBHR = numH; // Store hour in HIBDATA
    while(!(HIB_CTL_R & HIB_CTL_WRC));

    uint8_t data2[] = {LB(ADDMIN), numM};
    writeI2c0Registers(EEPROM >> 1, HB(ADDMIN), data2, 2);
    waitMicrosecond(10000);
    if(readI2c0Register16(EEPROM >>1, ADDMIN) != numM) // If data was not stored correctly, return error msg 2
        return 2;
    HIBMIN = numM; // Store min in HIBDATA
    while(!(HIB_CTL_R & HIB_CTL_WRC));

    uint8_t data3[] = {LB(ADDSEC), numS};
    writeI2c0Registers(EEPROM >> 1, HB(ADDSEC), data3, 2);
    waitMicrosecond(10000);
    if(readI2c0Register16(EEPROM >>1, ADDSEC) != numS) // If data was not stored correctly, return error msg 2
        return 2;
    HIBSEC = numS; // Store seconds in HIBDATA
    while(!(HIB_CTL_R & HIB_CTL_WRC));

    return 0; // If not other errors were hit, then the write worked
}

// Stores time stamp when data is taken
bool setTimeStamp(uint16_t add)
{
    uint32_t time = HIB_RTCC_R;
    uint16_t subTime = HIB_RTCSS_R && HIB_RTCSS_RTCSSC_M;
    time = HIB_RTCC_R;

    uint8_t i2cData[] = {LB(add), time};//Array for address low byte and data you are storing
    writeI2c0Registers(0xA0 >> 1, HB(add), i2cData, 2);//Writes to address in EEPROM using address high byte, array of address low byte, and 2 for size
    if(readI2c0Register16(0xA0 >> 1, add) != time)
        return false;

    return true;
}

// Conversion functions
// Change seconds to Day, Month H:M:S. Type determines when to return out of function.
void secToDateTime(char* str, uint8_t type, uint32_t sec)
{
    // type 0 - get full time stamp
    // type 1 - get date and return
    // type 2 - get time and return

    uint32_t daysLeft, secLeft, d, s, h, m;
    uint16_t currMonth, currDay;
    uint8_t origDate[5];

    getOrigDateTime(origDate); // Returns as {Mon, Day, Hr, Min, Sec} for origDate
    d = origDate[1];
    h = origDate[2];
    m = origDate[3];
    s = origDate[4];

    daysLeft = monthDay(origDate[0]) - d; // Check the amount of day remaining in the month before the time
                                                  // rolls into the next month
    secLeft = DAYSEC * daysLeft; // Check the amt of time left in the month

    // While month subtraction does not cause seconds to go below 0...
    while(sec > 0)
    {
        if(sec > secLeft) // If more time has elapsed than what is left in the month....
        {
            sec -= secLeft; // Remove days in original month elapsed from seconds elapsed

            currMonth = origDate[0] + 1; // Move to next month since all time in previous month has elapsed
            if(currMonth > 12)
                currMonth = 1; // Overlap to January

            secLeft = DAYSEC * monthDay(currMonth); // Get the amount of seconds in the new month

        }
        else // else, just add days elapsed
        {
            d = sec / DAYSEC; // Get number of days left in the seconds elapsed
            s = sec - (d*DAYSEC); // seconds elapsed - (seconds in the days) = seconds remaining
            sec = 0; // To exit while loop since time is computed
        }
    }
    char dateTime[128], monthS[128], time[128], buffer[128];

    // Get month name and put into output str
    numToMonth(currMonth, monthS);
    strcpy(dateTime, monthS);
    strcat(dateTime, " ");

    // Put day into output str
    itoa(d, buffer, 10);
    strcat(dateTime, buffer);

    if(type == 1) // just return date
    {
        strcat(dateTime, "\0");
        strcpy(str, dateTime);
        return;
    }
    else
    {
        strcat(dateTime, " ");
    }

    // change remaining seconds to H:M:S
    h = s / 3600; // Get hrs in seconds remaining
    s -= h * 3600; // Remove amt of hrs from seconds remaining
    m = s / 60; // Get mins left in seconds remaining
    s -= m * 60; // Remove amt of mins from seconds remaining

    // Format time
    itoa(h, buffer, 10);
    strcpy(time, buffer);
    strcat(time, ":");
    itoa(m, buffer, 10);
    strcat(time, buffer);
    strcat(time, ":");
    itoa(s, buffer, 10);
    strcat(time, buffer);

    if(type == 2) // Just return time
    {
        strcat(time, "\0");
        strcpy(str, time);
        return;
    }

    strcat(dateTime, time);
    strcat(dateTime, "\t");

    putsUart0(dateTime);
    waitMicrosecond(100000);
}

// Change user input of subseconds to seconds for RTC match
void getSec(uint32_t subseconds, uint32_t* time)
{
    time[0] = subseconds/32768; // integer division to get seconds out of subseconds
    time[1] = subseconds - (time[0]*32768); // subtract out the seconds accounted for
}
// Table Storage
// Get number of seconds in a given month
uint8_t monthDay(uint8_t month)
{
    uint8_t day;

    switch(month){
        case 1:
            day = 31;
            break;
        case 2:
            day = 28;
            break;
        case 3:
            day = 31;
            break;
        case 4:
            day = 30;
            break;
        case 5:
            day = 31;
            break;
        case 6:
            day = 30;
            break;
        case 7:
            day = 31;
            break;
        case 8:
            day = 31;
            break;
        case 9:
            day = 30;
            break;
        case 10:
            day = 31;
            break;
        case 11:
            day = 30;
            break;
        case 12:
            day = 31;
            break;
    }

    return day;
}

// Change month name to number of month
uint8_t monthToNum(char* month)
{
    if(!strcmp(month, "January"))
        return 1;
    if(!strcmp(month, "February"))
        return 2;
    if(!strcmp(month, "March"))
        return 3;
    if(!strcmp(month, "April"))
        return 4;
    if(!strcmp(month, "May"))
        return 5;
    if(!strcmp(month, "June"))
        return 6;
    if(!strcmp(month, "July"))
        return 7;
    if(!strcmp(month, "August"))
        return 8;
    if(!strcmp(month, "September"))
        return 9;
    if(!strcmp(month, "October"))
        return 10;
    if(!strcmp(month, "November"))
        return 11;
    if(!strcmp(month, "December"))
        return 12;
    else
        return 0;
}

// Change month number to month name
void numToMonth(uint8_t month, char* monthS)
{
    switch(month){
        case 1:
            strcpy(monthS, "January");
            break;
        case 2:
            strcpy(monthS, "February");
            break;
        case 3:
            strcpy(monthS, "March");
            break;
        case 4:
            strcpy(monthS, "April");
            break;
        case 5:
            strcpy(monthS, "May");
            break;
        case 6:
            strcpy(monthS, "June");
            break;
        case 7:
            strcpy(monthS, "July");
            break;
        case 8:
            strcpy(monthS, "August");
            break;
        case 9:
            strcpy(monthS, "September");
            break;
        case 10:
            strcpy(monthS, "October");
            break;
        case 11:
            strcpy(monthS, "November");
            break;
        case 12:
            strcpy(monthS, "December");
            break;
    }
    return;
}
