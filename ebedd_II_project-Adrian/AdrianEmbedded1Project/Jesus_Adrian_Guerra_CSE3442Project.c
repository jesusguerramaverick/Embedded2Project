//Jesus Adrian Guerra
//Embedded 1 Project

#include <stdint.h>//header files
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"


#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))//older bitband aliases
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))


#define GREEN_LED_MASK 8//older masks
#define RED_LED_MASK 2
#define PCSEVEN 128//Port C mask
#define DEINT 1//DEINT mask
#define DEI (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 0*4)))//PB0 bitband
#define MOTOR (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 0*4)))//PD0 bitband
#define SPEAKER (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 1*4)))//PD1 bitband



#define MAX_CHARS 80
#define MAX_FIELDS 5
typedef struct _USER_DATA {
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
}
USER_DATA;


void waitMicrosecond(uint32_t us)//function for waiting microseconds to create delay between operations
{
    __asm("WMS_LOOP0:   MOV  R1, #6");          // 1
    __asm("WMS_LOOP1:   SUB  R1, #1");          // 6
    __asm("             CBZ  R1, WMS_DONE1");   // 5+1*3
    __asm("             NOP");                  // 5
    __asm("             NOP");                  // 5
    __asm("             B    WMS_LOOP1");       // 5*2 (speculative, so P=1)
    __asm("WMS_DONE1:   SUB  R0, #1");          // 1
    __asm("             CBZ  R0, WMS_DONE0");   // 1
    __asm("             NOP");                  // 1
    __asm("             B    WMS_LOOP0");       // 1*2 (speculative, so P=1)
    __asm("WMS_DONE0:");                        // ---
                                                // 40 clocks/us + error
}
void initHw()//function to initialize all hardware
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
    GPIO_PORTC_DIR_R &= ~PCSEVEN;
    GPIO_PORTC_DEN_R &= ~PCSEVEN;
    COMP_ACREFCTL_R = 0x020F;//configure the internal voltage reference to 2.469V
    COMP_ACCTL0_R = 0x040C;//configure comparator 0 by writing to the register a value of 0x0000.040C
    waitMicrosecond(10);//wait 10 microseconds


    GPIO_PORTB_DIR_R |= DEINT;//enable port b as output
    GPIO_PORTB_DEN_R |= DEINT;

    DEI=1;//set DEINT high

    DEI=0;//set DEINT low

    GPIO_PORTE_DIR_R &= ~1;//enable PortE 0,1,and 2
    GPIO_PORTE_DEN_R &= ~1;

    GPIO_PORTE_DIR_R &= ~2;
    GPIO_PORTE_DEN_R &= ~2;

    GPIO_PORTE_DIR_R &= ~4;
    GPIO_PORTE_DEN_R &= ~4;

    GPIO_PORTE_AMSEL_R|= 1| 2| 4;//The analog function of the pin is enabled, the isolation is
    //disabled, and the pin is capable of analog functions.


    GPIO_PORTD_DIR_R |= 1;//lab8 stuff
    GPIO_PORTD_DEN_R |= 1;//enable PortD 0, and 1

    GPIO_PORTD_DIR_R |= 2;
    GPIO_PORTD_DEN_R |= 2;

    GPIO_PORTD_AMSEL_R|= 1| 2;//The analog function of the pin is enabled, the isolation is
    //disabled, and the pin is capable of analog functions.


    GPIO_PORTD_DR2R_R |= MOTOR | SPEAKER;

    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;//enable timer 2
    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;
    TIMER2_TAMR_R =0x0;//write 0 to timer value
    TIMER2_TAILR_R = 40000000;//set load value to 40e6 for 1Hz interrupt rate
    TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)


    //lab 9 inithw
    HIB_IM_R=0x10;//Write 0x0000.0010 to the HIBIM register to enable the WC interrupt
    HIB_CTL_R=0x41;//RTC is enabled by setting the RTCEN bit of the HIBCTL register
                    //Write 0x40 to the HIBCTL register to enable the oscillator input


}


//volume part
    uint32_t getVolume()
    {
        TIMER1_CTL_R &= ~TIMER_CTL_TAEN;//enable timer 1
        TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;
        TIMER1_TAMR_R =0x0;//write 0 to the timer value;
        TIMER1_TAILR_R = 40000000;//set load value to 40e6 for 1Hz interrupt rate
    DEI=1;//pull comparator output to high for long enough to deintegrate the capacitor(output will read as 1)
    while(COMP_ACSTAT0_R!=2)
        {
        //DEI=1;
        //COMP_ACSTAT0_R;
        }
    DEI=0;//set DEINT to low so the capacitor starts charging
    TIMER1_TAV_R =0x0;//write 0 to the timer value
    TIMER1_CTL_R |= TIMER_CTL_TAEN;//start the timer

    while(COMP_ACSTAT0_R!=0)//poll the comparator output until the bit goes to 0
    {
        //DEI=0;
        //COMP_ACSTAT0_R;
    }
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;//turn off timer
    int y=TIMER1_TAV_R;//measure timer value

    float x= ((y-39999671.45)/(-113.4545455));//equation to return the number of ml in container
        //return x;
        float v= 3.976*x;
        //return v;
        float v2= (v/61.024)*1000;
        return v2;
    }

    float getLightPercentage(){//function to get light percentage
        float p;
        float pp;
        setAdc0Ss3Mux(3);//function call for ADC0 with AIN3/PE0
        p= readAdc0Ss3();
        pp=(p/7);//equation for getting light percentage
        return pp;

    }



    float getMoisturePercentage(){//function to get moisture level
        float mp;
        float mpp;
        setAdc0Ss3Mux(2);//function call for ADC0 with AIN2/PE1
        mp= readAdc0Ss3();
        mpp=((mp-3062)/(-14.74));//equation for getting light level
        return mpp;
    }

    double getBatteryVoltage(){//function for getting battery voltage
        double ba;
        double bat;
        double batt;
        setAdc0Ss3Mux(1);//function call for ADC0 with AIN1/PE2
        ba= readAdc0Ss3();
        bat=((ba-287)/(935.5));//equation for getting battery level
        batt=bat*((100+47)/(47));
        return batt;
    }


    void enablePump(){//function to enable pump
        MOTOR=1;
    }

    void disablePump(){//function to disable pump
        MOTOR=0;
    }

    // Periodic timer
    void timer2Isr()
    {
        SPEAKER^= 1;
        TIMER2_ICR_R = TIMER_ICR_TATOCINT; // clear interrupt flag
    }

    void playBatteryLowAlert(){//function to play battery low melody(4 tones)

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

    void playWaterLowAlert(){//function to play water low melody(4 tones)


              TIMER2_TAMR_R =0x2;//enable periodic timer mode
              TIMER2_TAILR_R = 181816;                       // set load value to 40e6 for 1 Hz interrupt rate
              TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
              NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
              TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
              waitMicrosecond(727272);
              TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer



              TIMER2_TAILR_R = 45454;                       // set load value to 40e6 for 1 Hz interrupt rate
              TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
              NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
              TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
              waitMicrosecond(727272);
              TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer



              TIMER2_TAILR_R =90908;                       // set load value to 40e6 for 1 Hz interrupt rate
              TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
              NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
              TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
              waitMicrosecond(727272);
              TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer


              TIMER2_TAILR_R = 181816;                       // set load value to 40e6 for 1 Hz interrupt rate
              TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
              NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
              TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
              waitMicrosecond(727272);
              TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 //turn off timer

    }

    //lab 9 functions
    double getCurrentSeconds(){//function that returns current seconds from RTC
        int gcs=HIB_RTCC_R;
        return gcs;
    }

    bool isWateringAllowed(int second1, int second2){//function that returns bool value true based on
        bool WAX=false;                              //whether time is within allowed watering hours
        int WA= getCurrentSeconds();
        if((WA>=second1)&&(WA<=second2)){
            WAX=true;
        }
        return WAX;
    }

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

getsUart0(USER_DATA* data)//function that takes input from the user to turn into data
{
char c;
int count=0;
while(1)
{
    c=getcUart0();//gets character from Uart0
    if ((c==(8)||(c==127))&&(count>0))//if character is a backspace and count is greater than 0
        {                             //then decrement the count so that last character is overwritten
        count--;
        continue;
        }
        if (c==(10)||c==(13))//if character is line feed or carriage return then add a null terminator
        {                    //to the end of the string and return from the function
        data->buffer[count]=0;
        break;
        }
        if (c>=32)//if the character is a space or another printable character store the character in
        {         //data->buffer[count]. Increment the count.
         data->buffer[count++]=c;
         if (count==MAX_CHARS)// If count == maxChars
                              // then add a null terminator to the end of the
                              //string and return from the function.
                 {
                    data->buffer[count]=0;
                    break;
                 }
         else
         {
             continue;
         }
        }
        else
        {
            continue;
        }
        }

}
parseFields(USER_DATA* data)//function that separates user input into strings of character or numbers
{
    //int ascii=c;

       char a='a';// characters =a
       char n='n';//numbers= n
       int i=0;
       int count=0;
       for(i=0; data->buffer[i]!=NULL; i++)//while data is not equal to null traverse the user input
       {
          char c= data->buffer[i];
          char d= data->buffer[i-1];
          if(i==0)//if start of string
                {
                    if(c >= 65 && c <=90)//if between A and Z
                     {

                         data->fieldPosition[count]=i;
                         data->fieldType[count]=a;//set equal to a
                         count++;
                         data->fieldCount= count;
                     }
                    else if(c >=97 && c <= 122)//if between a and z
                     {

                         data->fieldPosition[count]=i;
                         data->fieldType[count]=a;//set equal to a
                         count++;
                         data->fieldCount= count;
                     }
                    else if((c >= 48 && c <= 57))//if between 0 and 9
                    {
                        data->fieldPosition[count]=i;
                        data->fieldType[count]=n;//set equal to n
                        count++;
                        data->fieldCount= count;
                    }
                    else
                    {
                        data->buffer[i]=NULL;//else equals NULL
                    }

                }
                else//if i is not equal to 0
                {
                    if((c >= 65 && c <=90) || (c >=97 && c <= 122))//if equal to A-Z or a-z
                    {
                        if(d >= 33 && d <= 47)//if previous input was delimiter
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=a;//set equal to a
                            count++;
                            data->fieldCount= count;
                        }
                        else if(d >= 58 && c <= 64)//if previous input was delimiter
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=a;//set equal to a
                            count++;
                            data->fieldCount= count;
                        }
                        else if(d >= 91 && d <= 96)//if previous input was delimiter
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=a;//set equal to a
                            count++;
                            data->fieldCount= count;
                        }
                        else if(d==NULL)//if previous input was null
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=a;//set equal to a
                            count++;
                            data->fieldCount= count;
                        }


                    }
                    else if((c >= 48 && c <= 57))//if equal to 0 thru 9
                    {
                        if(d >= 33 && d <= 47)//if previous input was delimiter
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=n;//set equal to n
                            count++;
                            data->fieldCount= count;
                        }
                        else if(d >= 58 && c <= 64)//if previous input was delimiter
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=n;//set equal to n
                            count++;
                            data->fieldCount= count;
                        }
                        else if(d >= 91 && d <= 96)//if previous input was delimiter
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=n;//set equal to n
                            count++;
                            data->fieldCount= count;
                        }
                        else if(d==NULL)//if previous input was NULL
                        {
                            data->fieldPosition[count]=i;
                            data->fieldType[count]=n;//set equal to n
                            count++;
                            data->fieldCount= count;
                        }


                    }
                    else
                    {
                       data->buffer[i]=NULL;
                    }
               }
        }
}
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)//function to return the value of a field requested if the field number is in range
                                                          //or NULL otherwise.
{
    int i=data->fieldPosition[fieldNumber-1];
    int n=0;
    char m[15];
    if (fieldNumber<sizeof(data->buffer)/sizeof(data->buffer[0]))
    {
        while(data->buffer[i]!=NULL)
        {
            m[n]=data->buffer[i];
            i++;
            n++;
        }
    return m;
    }
    else
    {
        return NULL;
    }
}

int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)//function to return a pointer to the field requested if the field number is in
{                                                            //range and the field type is numeric or 0 otherwise.
    int l=data->fieldPosition[fieldNumber];
       int k=0;
       char q[16];

       if (fieldNumber<=sizeof(data->fieldPosition))
       {
           while(data->buffer[l]!=NULL)
           {
               q[k]=data->buffer[l];
               l++;
               k++;
           }

           int O=atoi(q);//function for converting string to integer from https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_72/rtref/itoi.htm
           memset(q, 0, sizeof(q));// clears array after use https://www.quora.com/What-is-the-best-way-to-clear-an-array-using-C-language
       return O;

       }
       else
       {
           return NULL;
       }
}
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)// function which returns true if the command matches
{                                                                             //the first field and the number of arguments (excluding the command field) is
                                                                              //greater than or equal to the requested number of minimum arguments.
        int i=data->fieldPosition[0];
        int n=0;
        char m[5];
        bool a= false;
        int w=sizeof(strCommand);
        int x;
        if ((data->fieldCount-1)>=minArguments)
        {
            while(data->buffer[i]!=NULL)
            {
                m[n]=data->buffer[i];
                i++;
                n++;
            }
            for (x = 0; x < w-1; x++)
            {
                if (m[x] != strCommand[x])
                {
                    a=false;
                    break;
                }
                else
                {
                    a=true;
                }
            }
        }
        else
        {
            a=false;
        }
        return a;
}





int main(void)
{
    USER_DATA data;
    initHw();
    initAdc0Ss3();
    getVolume();//initialize functions and call functions for debugging
    getLightPercentage();
    getMoisturePercentage();
    getBatteryVoltage();
    //playBatteryLowAlert();
    //playWaterLowAlert();
    //int f=getVolume();
    //char f2[255];
    //sprintf(f2, "Volume in ml is %d",f);
    initUart0();
    float lightlevel= getLightPercentage();
        float batterylevel= getBatteryVoltage();
        float volumelevel= getVolume();
        float moisturelevel=getMoisturePercentage();
        int32_t h1 = 2;//initialize all variables used
        int32_t m1 = 30;
        int32_t h2 = 7;
        int32_t m2 = 30;

        int32_t mlevel=80;
        HIB_RTCLD_R=12600;
        HIB_CTL_R=0x41;

        int second1= ((h1*60*60)+(m1*60));
        int second2= ((h2*60*60)+(m2*60));
        double check1=getCurrentSeconds();
        bool check2=isWateringAllowed(second1,second2);
        int truelightlevel=0;

    while(1)//endless loop
    {
        if (kbhitUart0()){//if kbhit process commands
    getsUart0 (&data);
    putsUart0(data.buffer);
    putsUart0("\n\r");
    bool valid=false;

    // Parse fields
     parseFields(&data);
     // Echo back the parsed field information (type and fields)
     //#ifdef DEBUG
     uint8_t i;
    for (i = 0; i < data.fieldCount; i++)
     { putcUart0(data.fieldType[i]);
     putsUart0("\t");
     putsUart0(&data.buffer[data.fieldPosition[i]]);
     putsUart0("\n");
     putsUart0("\r");
     }
     //#endif
     // Command evaluationbool valid = false;
     // set add, data  add and data are integers
     if (isCommand(&data, "set", 2))//set function
     {
     int32_t add = getFieldInteger(&data, 1);
     int32_t dat = getFieldInteger(&data, 2);
     valid = true;
     // do something with this information
     }
     if (isCommand(&data, "time", 2))//time command to let user enter current time
         {
         int32_t loadhour = getFieldInteger(&data, 1);
         int32_t loadminutes = getFieldInteger(&data, 2);
         int currenttime=((loadhour*60*60)+(loadminutes*60));
         //HIB_RTCC_R=currenttime;
         HIB_RTCLD_R=currenttime;
         HIB_CTL_R=0x41;
         valid = true;
         }
     if (isCommand(&data, "water", 4))//command to let user input start time and end time for watering
         {
          h1 = getFieldInteger(&data, 1);
          m1 = getFieldInteger(&data, 2);
          h2 = getFieldInteger(&data, 3);
          m2 = getFieldInteger(&data, 4);
         //int second1= ((h1*60*60)+(m1*60));
         //int second2= ((h2*60*60)+(m2*60));
         valid = true;
         }
      second1= ((h1*60*60)+(m1*60));
      second2= ((h2*60*60)+(m2*60));
      check1=getCurrentSeconds();
      check2=isWateringAllowed(second1,second2);
     if (isCommand(&data, "level", 1))//command that allows user to input desired moisture level of the soil
         {
         mlevel = getFieldInteger(&data, 1);
         valid = true;
         }
     if (isCommand(&data, "status", 0))//command that displays current levels of battery voltage, volume
         {                             //light amount, and moisture level of soil
         double f=getVolume();
         double ppp=getLightPercentage();
         double mppp=getMoisturePercentage();
         double battery=getBatteryVoltage();

            char f2[255];
            char ppp2[255];
            char mppp2[255];
            char battery2[255];
            sprintf(f2, "Volume in ml is %.2f",f);
         putsUart0(f2);
         putsUart0("\n");
         putsUart0("\r");
         sprintf(ppp2, "Light percentage is %.2f",ppp);
         putsUart0(ppp2);
         putsUart0("\n");
         putsUart0("\r");
         sprintf(mppp2, "Moisture percentage is %.2f",mppp);
         putsUart0(mppp2);
         putsUart0("\n");
         putsUart0("\r");
         sprintf(battery2, "Battery voltage is %.2f",battery);
         putsUart0(battery2);

         valid = true;
        putsUart0("\n");
         putsUart0("\r");
         }

     if (isCommand(&data, "Pump", 1))//command to manually turn on and off the pump
         {
        char* str = getFieldString(&data, 2);
         valid = true;
         int pumpcheckon;
         int pumpcheckoff;
         //char spump[]= str;
         char pumpon[]="ON";
         char pumpoff[]="OFF";
             pumpcheckon=strcmp(str,pumpon);
             if (pumpcheckon==0){
                 enablePump();
             }
             pumpcheckoff=strcmp(str,pumpoff);
             if (pumpcheckoff==0){
                 disablePump();
             }
         }
     // alert ON|OFF  alert ON or alert OFF are the expected commands
     if (isCommand(&data, "alert", 1))//command to alter the minimum light level required for pumping
     {                                //also plays melody if light threshold is met and is battery is low or water reservoir is low
     int32_t dat = getFieldInteger(&data, 1);
     valid = true;
     int vlevel=getVolume();
     int blevel= getBatteryVoltage();
     int llevel= getLightPercentage();
     truelightlevel=getFieldInteger(&data, 1);

     if ((llevel>=dat)&&(vlevel<150)){
         playWaterLowAlert();
         waitMicrosecond(5000000);
         playWaterLowAlert();
         waitMicrosecond(5000000);
         playWaterLowAlert();
         waitMicrosecond(5000000);
         playWaterLowAlert();
         waitMicrosecond(5000000);
         playWaterLowAlert();

     }
     if ((llevel>=dat)&&(blevel<1)){
         playBatteryLowAlert();
         waitMicrosecond(5000000);
         playBatteryLowAlert();
         waitMicrosecond(5000000);
         playBatteryLowAlert();
         waitMicrosecond(5000000);
         playBatteryLowAlert();
         waitMicrosecond(5000000);
         playBatteryLowAlert();
     }

     // process the string with your custom strcmp instruction, then do something
     }
     // Process other commands here
     // Look for error
     if (!valid)
     putsUart0("Invalid command\n");
     putsUart0("\n");
     putsUart0("\r");
     }
        else{
            lightlevel= getLightPercentage();//loop that pumps water if time is within watering time and if
            batterylevel=getBatteryVoltage();//soil saturation level hasn't been met
            volumelevel= getVolume();
            moisturelevel=getMoisturePercentage();
            check2=isWateringAllowed(second1,second2);
            if ((moisturelevel<mlevel)&&(check2==true)){
                enablePump();
                waitMicrosecond(5000000);
                disablePump();
                waitMicrosecond(30000000);
                //waitMicrosecond(5000000);
            }
            if((lightlevel>=truelightlevel)&&(volumelevel<150)){
                playWaterLowAlert();//plays melody if light threshold is met and water container is low
                waitMicrosecond(5000000);
            }
            if((lightlevel>=truelightlevel)&&(batterylevel<1)){//plays melody if light threshold is met and battery is low
                playBatteryLowAlert();
                waitMicrosecond(5000000);
            }

        }
    }
}
