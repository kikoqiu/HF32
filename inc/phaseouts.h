/*
 * phaseouts.h
 *
 *  Created on: Apr 22, 2020
 *      Author: Alka
 */

#ifndef INC_PHASEOUTS_H_
#define INC_PHASEOUTS_H_

#include "main.h"

void allOff();
void comStep (int newStep);
void fullBrake();
void allpwm();
void proportionalBrake();
void twoChannelForward();
void twoChannelReverse();

void phaseAPWM();
void phaseBPWM();
void phaseCPWM();
#endif /* INC_PHASEOUTS_H_ */
