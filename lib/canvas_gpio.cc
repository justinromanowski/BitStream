#include "canvas_gpio.h"
#include "display-threads.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>

volatile bool app_change;

int RotaryEncoder::rot_enc_state = IDLE_11;
int RotaryEncoder::count = 0;
int RotaryEncoder::prev_count = 0;

RotaryEncoder::RotaryEncoder(){
  init();
}

void RotaryEncoder::init() {
      // Initialize GPIO pins and interrupts for the rotary encoder

      /*
      // GPIO Setup
      if(wiringPiSetup() == -1) {
        printf("Error setting up wiringPi\n");
        //return 1;
      }
      */
      // GPIO Pin Initialization
      pinMode(outputApin, INPUT);
      pinMode(outputBpin, INPUT);
      pinMode(switchpin, INPUT);

      // Set input as pullup
      pullUpDnControl(outputApin, PUD_UP);
      pullUpDnControl(outputBpin, PUD_UP);
      pullUpDnControl(switchpin, PUD_UP);

/*
      wiringPiISR(outputApin, INT_EDGE_BOTH, &RotaryEncoder::rotateInterrupt);
      wiringPiISR(outputBpin, INT_EDGE_BOTH, &RotaryEncoder::rotateInterrupt);
      wiringPiISR(switchpin, INT_EDGE_RISING, &RotaryEncoder::switchInterrupt);
*/
      rot_enc_state = IDLE_11;
      count = 0;
      prev_count = 0;
      app_change = false;
    }


void RotaryEncoder::rotateInterrupt(void) {
      int A_value = digitalRead(outputApin);
      int B_value = digitalRead(outputBpin);
      printf("A value = %d\n", A_value);

      switch(rot_enc_state) {
        case IDLE_11:
          if(!A_value) {
            rot_enc_state = A_01;
          } else if(!B_value) {
            rot_enc_state = A_10;
          }
          break;
        case A_01:
          if(A_value) {
            rot_enc_state = IDLE_11;
          } else if(!B_value) {
            rot_enc_state = A_00;
          }
          break;
        case A_00:
          if(B_value) {
            rot_enc_state = A_01;
          } else if(A_value) {
            rot_enc_state = A_10;
          }
          break;
        case A_10:
          if(!A_value) {
            rot_enc_state = A_00;
          } else if(B_value) {
            rot_enc_state = IDLE_11;
            count++;
          }
          break;
        case B_10:
          if(B_value) {
            rot_enc_state = IDLE_11;
          } else if(!A_value) {
            rot_enc_state = B_00;
          }
          break;
        case B_00:
          if(A_value) {
            rot_enc_state = B_10;
          } else if(B_value) {
            rot_enc_state = B_01;
          }
          break;
        case B_01:
          if(!B_value) {
            rot_enc_state = B_00;
          } else if(A_value) {
            rot_enc_state = IDLE_11;
            count--;
          }
          break;
        }

      if(count != prev_count) {
        // Change the app that is displayed on the screen to select
        printf("Count = %d", count);
        prev_count = count;
      }
    }

    void RotaryEncoder::switchInterrupt(void){
      // When the switch is pressed down, it will toggle
      // app selection for the user to change the app

      int A_value = digitalRead(outputApin);
      int B_value = digitalRead(outputBpin);

      app_change = !app_change;
      printf("Switch pressed\n");
    }


/*
class RotaryEncoder{
  public:

    RotaryEncoder() {
      init();
    }

    void init() {
      // Initialize GPIO pins and interrupts for the rotary encoder

      // GPIO Setup
      if(wiringPiSetup() == -1) {
        printf("Error setting up wiringPi\n");
        return 1;
      }

      // GPIO Pin Initialization
      pinMode(outputApin, INPUT);
      pinMode(outputBpin, INPUT);
      pinMode(switchpin, INPUT);

      // Set input as pullup
      pullUpDnControl(outputApin, PUD_UP);
      pullUpDnControl(outputBpin, PUD_UP);
      pullUpDnControl(switchpin, PUD_UP);

      wiringPiISR(outputApin, INT_EDGE_BOTH, &rotateInterrupt);
      wiringPiISR(outputBpin, INT_EDGE_BOTH, &rotateInterrupt);
      wiringPiISR(switchpin, INT_EDGE_RISING, &switchInterrupt);

      rot_enc_state = IDLE_11;
      count = 0;
      prev_count = 0;
      app_change = false;
    }

    void rotateInterrupt(void) {
      int A_value = digitalRead(outputApin);
      int B_value = digitalRead(outputBpin);
      printf("A value = %d\n", A_value);

      switch(rot_enc_state) {
        case IDLE_11:
          if(!A_value) {
            rot_enc_state = A_01;
          } else if(!B_value) {
            rot_enc_state = A_10;
          }
          break;
        case A_01:
          if(A_value) {
            rot_enc_state = IDLE_11;
          } else if(!B_value) {
            rot_enc_state = A_00;
          }
          break;
        case A_00:
          if(B_value) {
            rot_enc_state = A_01;
          } else if(A_value) {
            rot_enc_state = A_10;
          }
          break;
        case A_10:
          if(!A_value) {
            rot_enc_state = A_00;
          } else if(B_value) {
            rot_enc_state = IDLE_11;
            count++;
          }
          break;
        case B_10:
          if(B_value) {
            rot_enc_state = IDLE_11;
          } else if(!A_value) {
            rot_enc_state = B_00;
          }
          break;
        case B_00:
          if(A_value) {
            rot_enc_state = B_10;
          } else if(B_value) {
            rot_enc_state = B_01;
          }
          break;
        case B_01:
          if(!B_value) {
            rot_enc_state = B_00;
          } else if(A_value) {
            rot_enc_state = IDLE_11;
            count--;
          }
          break;
        }

      if(count != prev_count) {
        // Change the app that is displayed on the screen to select
        printf("Count = %d", count);
        prev_count = count;
      }
    }


    void switchInterrupt(void){
      // When the switch is pressed down, it will toggle
      // app selection for the user to change the app

      int A_value = digitalRead(outputApin);
      int B_value = digitalRead(outputBpin);

      app_change = !app_change;
      printf("Switch pressed\n");
    }

};
*/

/*
class AppStateMachine {

  private:


  public:

    void AppStateMachine {

}
*/
