#ifndef CANVAS_GPIO_H
#define CANVAS_GPIO_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>

void rotateInterrupt(void);

void switchInterrupt(void);

void gpioSetup(void);

#endif
