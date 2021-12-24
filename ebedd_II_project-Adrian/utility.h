// utility.h
// This files will the Uart utility as a user interface

#ifndef UTILITY_H_
#define UTILITY_H_

typedef struct _USER_INPUT
{
    char command[128];
    char fields[128];
    uint8_t fieldCount;
}USER_INPUT;

void commands(); // Use user input to determine what command to do and call functions. Returns false if run is activated
void logFields(uint8_t log); // Outputs what log fields are set to take measurements
void getsUart0(char* input); // Get string of user input
void parseCommand(char* input, USER_INPUT *uIn); // Parse user input

// itoa and atoi functionalities
void itoa(int32_t num, char* buffer, uint8_t base);
void reverse(char* str, uint8_t length);

// EEPROM address
uint16_t getNextAdd(); // get the next EEPROM address to write to

uint32_t myatoi(char* str);

#endif /* UTILITY_H_ */
