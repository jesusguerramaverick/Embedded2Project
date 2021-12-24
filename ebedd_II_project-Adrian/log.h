// log.h

#ifndef LOG_H_
#define LOG_H_

// EEPROM storage
bool storeEEPROMdata(uint32_t data);

// log functions
bool logGyro();
bool logAcc();
bool logMag();
bool logTemp();

// take measurements based off of

#endif /* LOG_H_ */
