#ifndef CANVAS_GPIO_H
#define CANVAS_GPIO_H

#include "display-threads.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>

// STATE MACHINE VARIABLES - ROTARY ENCODER
// A_AB means that A had a falling edge first
// B_AB means that B had a rising edge first
// FSM can go IDLE -> A_01 -> A_00 -> A_10 -> IDLE
// or IDLE -> B_10 -> B_00 -> B_01 -> IDLE, when a full
// loop back to IDLE is completed then the count is incremented
#define IDLE_11 0
#define A_01 1
#define A_00 2
#define A_10 3
#define B_10 4
#define B_00 5
#define B_01 6

// STATE MACHINE VARIABLES - APPS
#define IMAGE 0
#define SPOTIFY 1
#define BASEBALL 2


const int outputApin = 25;
const int outputBpin = 24;
const int switchpin = 23;

class RotaryEncoder{
  private:
    static int rot_enc_state;
    static int count;
    static int prev_count;

  public:
    RotaryEncoder();
    static void init();
    static void rotateInterrupt(void);
    static void switchInterrupt(void);

};

#endif
