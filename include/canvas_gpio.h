#ifndef CANVAS_GPIO_H
#define CANVAS_GPIO_H


#include "led-matrix.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

extern volatile bool interrupt_received;
extern const int number_apps;
extern volatile bool changing_app;

#define SW_A_PIN 13
#define ENCX_A_PIN 26
#define ENCX_B_PIN 19

void* gpioThread(void *ptr);

#endif
