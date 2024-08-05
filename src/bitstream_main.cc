// ---------------------------------------------------------------------------
// ENGINEER: Justin Romanowski
//
// Project: BitStream
//
// File Desc: Main file for executing the various codes for the BitStream system
// that controls what is being displayed on the screen, IPC with the Python API
// code, and the control flow/state machine for the entire display.
//
// To Do: Create header files for the threads to clean up the main file,
// make a state machine for rotating the top display, and work on
// implementing the FIFO with the Spotify display
// ---------------------------------------------------------------------------

#include "led-matrix.h"
#include "graphics.h"
#include "thread.h"
#include "display-threads.h"
#include "display-classes.h"
#include "canvas_gpio.h"

#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <exception>
#include <Magick++.h>
#include <magick/image.h>
#include <pthread.h>
#include <wiringPi.h>

#include <errno.h>

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>


// NAMESPACE SETUP -----------------------------------------------------------
// ---------------------------------------------------------------------------
using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;

/*
// ROTARY ENCODER /////////////////////////////////////////////////////////////
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
*/

// STATE MACHINE VARIABLES - APPS
#define IMAGE 0
#define SPOTIFY 1
#define BASEBALL 2
#define WEATHER 3

// HOME PAGE VARIAVLES //
const int number_apps = 4;
int current_app = 0;
volatile bool changing_app = true;

//const int outputApin = 25;
//const int outputBpin = 24;
//const int switchpin = 23;


/*
int rot_enc_state = IDLE_11;
int count = 0;
int prev_count = 0;

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
////////////////////////////////////////////////////////////////////////////
*/


// GLOBAL VARIABLE DECLARATIONS ----------------------------------------------
// ---------------------------------------------------------------------------
using ImageVector = std::vector<Magick::Image>;

const int screen_divider = 44;

pthread_t gpio_canvas_thr, image_canvas_thr, clock_canvas_thr, spotify_canvas_thr, baseball_canvas_thr, weather_canvas_thr;
//pthread_mutex_t canvas_mutex;


// FUNCTION SETUP - INTERRUPTS -----------------------------------------------
// ---------------------------------------------------------------------------
  // Used to peacefully terminate the program's execution
  // -------------------------------------------------------------------------
volatile bool interrupt_received = false;

static void InterruptHandler(int signo) {
  interrupt_received = true;
}

//static bool FullSaturation(const rgb_matrix::Color &c) {
//  return (c.r == 0 || c.r == 255)
//    && (c.g == 0 || c.g == 255)
//    && (c.b == 0 || c.b == 255);
//}

// MAIN FUNCTION -------------------------------------------------------------
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  // GPIO Setup
  //  RotaryEncoder rotary_encoder;
/*
printf("trying wiringpi\n");

  if(wiringPiSetup() == -1) {
    printf("Error setting up wiringPi\n");
    return 1;
  }

printf("GPIO setup ok\n");

  // GPIO Pin Initialization
  pinMode(outputApin, INPUT);
  pinMode(outputBpin, INPUT);
  pinMode(switchpin, INPUT);

  // Set input as pullup
  pullUpDnControl(outputApin, PUD_UP);
  pullUpDnControl(outputBpin, PUD_UP);
  pullUpDnControl(switchpin, PUD_UP);

  if(wiringPiISR(outputApin, INT_EDGE_BOTH, &rotateInterrupt) < 0)
    {
    fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
    return 1;
    }

  wiringPiISR(switchpin, INT_EDGE_RISING, &switchInterrupt);
*/


  // INITIALIZE MATRIX AND IMAGE
  RGBMatrix::Options matrix_options;
  matrix_options.hardware_mapping = "regular";
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.show_refresh_rate = false;

  Magick::InitializeMagick(*argv);

  RGBMatrix *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &matrix_options);
  if (canvas == NULL)
    return 1;

  FrameCanvas *offscreen_canvas = canvas->CreateFrameCanvas();

  // PTHREAD RELATED
  pthread_mutex_t canvas_mutex;

  // CREATE CLASS ARGUMENTS STRUCT
  canvas_args canvas_ptrs;
  canvas_ptrs.canvas = canvas;
  canvas_ptrs.offscreen_canvas = offscreen_canvas;
  canvas_ptrs.canvas_mutex = &canvas_mutex;


  // Let's request all input bits and see which are actually available.
  // This will differ depending on which hardware mapping you use and how
  // many parallel chains you have.
  const uint64_t available_inputs = canvas->RequestInputs(0xffffffff);
  fprintf(stderr, "Available GPIO-bits: ");
  for (int b = 0; b < 32; ++b) {
      if (available_inputs & (1<<b))
          fprintf(stderr, "%d ", b);
  }
  fprintf(stderr, "\n");

  // CREATE CLASSES
  void *canvas_void_ptr = (void*)&canvas_ptrs;
  if(canvas_void_ptr==NULL){
    printf("ERROR: CANVAS_PTR NULL\n");
    return 5;
  }

  ClockClass clock((void*)&canvas_ptrs);
  ClockClass *Clock = &clock;

  // INITIALIZE INTERRUPTS
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


  // SET ALL PIXELS OFF TO START
  canvas->Clear();

  // Configure colors for text
  rgb_matrix::Color color(180, 90, 0); //text color
  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color flood_color(0, 0, 0);
  rgb_matrix::Color outline_color(0,0,0);
  bool with_outline = false;

  const bool all_extreme_colors = (matrix_options.brightness == 100)
    && FullSaturation(color)
    && FullSaturation(bg_color)
    && FullSaturation(outline_color);
  if (all_extreme_colors)
    canvas->SetPWMBits(1);

  fontSetup();

  // INITIALIZE MUTEX
  if (pthread_mutex_init(&canvas_mutex, NULL) != 0) {
        printf("Canvas Mutex INIT failed\n");
        return 1;
  }


  // CREATE THREADS
//  if(pthread_create(&clock_canvas_thr, NULL, clockThread, (void*)&canvas_ptrs) != 0) {
//    printf("ERROR in clock thread creation");
//    return 2;
//  }

  if(pthread_create(&clock_canvas_thr, NULL, clockThread, (void*)Clock) != 0) {
    printf("ERROR in clock thread creation");
    return 2;
  }

  if(pthread_create(&gpio_canvas_thr, NULL, gpioThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in clock thread creation");
    return 2;
  }

/*
  if(pthread_create(&image_canvas_thr, NULL, imageThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in image thread creation");
   return 2;
  }
*/

  if(pthread_create(&spotify_canvas_thr, NULL, spotifyThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in spotify thread creation");
    return 2;
  }

/*
  if(pthread_create(&baseball_canvas_thr, NULL, baseballThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in baseball thread creation");
    return 2;
  }
*/
/*
  if(pthread_create(&weather_canvas_thr, NULL, weatherThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in weather thread creation");
    return 2;
  }
*/
  // JOIN THREADS TOGETHER
  // Join the threads to main thread -- don't execute cleanup code
  // until interrupt is received
  void *canvas_ret;
  void *gpio_ret;
  void *image_ret;
  void *spotify_ret;
  void *baseball_ret;
  void *weather_ret;

  if(pthread_join(clock_canvas_thr, &canvas_ret) != 0) {
    printf("ERROR in clock thread joining");
    return 3;
  }

  if(pthread_join(gpio_canvas_thr, &gpio_ret) != 0) {
    printf("ERROR in spotify thread joining");
    return 3;
  }

/*
  if(pthread_join(image_canvas_thr, &image_ret) != 0) {
    printf("ERROR in image thread joining");
    return 3;
  }
*/

  if(pthread_join(spotify_canvas_thr, &spotify_ret) != 0) {
    printf("ERROR in spotify thread joining");
    return 3;
  }

/*
  if(pthread_join(baseball_canvas_thr, &baseball_ret) != 0) {
    printf("ERROR in baseball thread joining");
    return 3;
  }
*/
/*
  if(pthread_join(weather_canvas_thr, &weather_ret) != 0) {
    printf("ERROR in weather thread joining");
    return 3;
  }
*/
  // CLEANUP AND EXIT CLEANLY
  // Program is exited, shut down the RGB Matrix
  printf("Exiting... clearing canvas\n");
  canvas->Clear();
  delete canvas;

}


// END OF FILE ---------------------------------------------------------------
// ---------------------------------------------------------------------------
