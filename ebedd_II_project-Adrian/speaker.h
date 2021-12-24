// speaker.h

#ifndef SPEAKER_H_
#define SPEAKER_H_

#include <stdint.h>
#include <stdbool.h>

void initspeakerHw();
void timer2Isr();
void playAlert();
bool jostleCheck();
uint16_t magY();

#endif /* SPEAKER_H_ */

