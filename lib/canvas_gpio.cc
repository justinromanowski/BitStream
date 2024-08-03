#include "canvas_gpio.h"


int count = 0;
int prev_A_value = 0;

static const int outputApin = 25;
static const int outputBpin = 24;
static const int switchpin = 23;

void rotateInterrupt(void) {
  int A_value = digitalRead(outputApin);
  int B_value = digitalRead(outputBpin);

  printf("A value = %d, B value = %d\n", A_value, B_value);
  // Analyze direction
  if(A_value != prev_A_value){
    if(A_value == B_value) {
      printf("Counter clockwise rotation\n");
      count--;
    } else {
      printf("Clockwise rotation\n");
      count++;
    }
    prev_A_value = A_value;
    printf("Count = %d\n", count);
  } else {printf("ROTARY ENCODER BOUNCING\n");}
}

void switchInterrupt(void){
  int A_value = digitalRead(outputApin);
  int B_value = digitalRead(outputBpin);

  printf("A value = %d, B value = %d\n", A_value, B_value);

  printf("Switch pressed\n");
}

void gpioSetup(void) {

  // GPIO Setup
  if(wiringPiSetup() == -1) {
    printf("Error setting up wiringPi\n");
    }

  printf("GPIO setup ok");

  // GPIO Pin Initialization
  pinMode(outputApin, INPUT);
  pinMode(outputBpin, INPUT);
  pinMode(switchpin, INPUT);

  // Set input as pullup
  pullUpDnControl(outputApin, PUD_UP);
  pullUpDnControl(outputBpin, PUD_UP);
  pullUpDnControl(switchpin, PUD_UP);

  wiringPiISR(outputApin, INT_EDGE_BOTH, &rotateInterrupt);
//  wiringPiISR(outputBpin, INT_EDGE_FALLING, &rotateInterrupt);
  wiringPiISR(switchpin, INT_EDGE_RISING, &switchInterrupt);
}
