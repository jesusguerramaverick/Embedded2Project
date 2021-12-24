#ifndef HIBERNATION_H_
#define HIBERNATION_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initHibernationModule();
void hibernate(uint32_t ctl, uint32_t* time);
bool checkIfConfigured();
bool rtcCausedWakeUp();
bool wakePinCausedWakeUp();
void waitUntilWriteComplete();

void startTime();
void stopHibernation();

// set counter match value ... should not be used but required
void setRTCMatch(uint32_t time);

#endif
