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


// STATE MACHINE VARIABLES - APPS
#define IMAGE 0
#define SPOTIFY 1
#define BASEBALL 2
#define WEATHER 3

// HOME PAGE VARIAVLES //
const int number_apps = 4;
int current_app = 0;
volatile bool changing_app = true;

extern volatile int encX_count;


// GLOBAL VARIABLE DECLARATIONS ----------------------------------------------
// ---------------------------------------------------------------------------
using ImageVector = std::vector<Magick::Image>;

const int screen_divider = 44;

pthread_t gpio_canvas_thr, image_canvas_thr, clock_canvas_thr, spotify_canvas_thr, baseball_canvas_thr, weather_canvas_thr;
//pthread_mutex_t canvas_mutex;
pthread_t app_thr;

// FUNCTION SETUP - INTERRUPTS -----------------------------------------------
// ---------------------------------------------------------------------------
  // Used to peacefully terminate the program's execution
  // -------------------------------------------------------------------------
volatile bool interrupt_received = false;

static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static bool FullSaturation(const rgb_matrix::Color &c) {
  return (c.r == 0 || c.r == 255)
    && (c.g == 0 || c.g == 255)
    && (c.b == 0 || c.b == 255);
}

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

printf("%d\n", encX_count);

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

  // IMAGES CREATED
//  const char* bitstream_logo_fp = "images/bitstream_3.png";
//  ImageVector bitstream_logo = LoadImageAndScaleImage(bitstream_logo_fp,
//                                                       60,
//                                                       20);

//  const int logo_x = 2;
//  const int logo_y = 5;


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

  HomePageClass homepage((void*)&canvas_ptrs);

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


//////////////////////////////////////
// STATE MACHINE //
//////////////////////////////////////

  // Create threads that don't need state machine
  if(pthread_create(&clock_canvas_thr, NULL, clockThread, (void*)Clock) != 0) {
    printf("ERROR in clock thread creation");
    return 2;
  }

  if(pthread_create(&gpio_canvas_thr, NULL, gpioThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in clock thread creation");
    return 2;
  }

  void* (*thr_function_ptr) (void*) = &imageThread;
  if(thr_function_ptr==NULL) {
    printf("ERROR: Function ptr is null\n");
    return 5;
  }

  while(!interrupt_received) {
    int prev_encX_count = encX_count;

    // Check if app_change is active
    while(changing_app) {

      pthread_mutex_lock(canvas_ptrs.canvas_mutex);

      homepage.clearDisplay();
      homepage.drawBitStreamLogo();
      homepage.drawBits();

      canvas->SwapOnVSync(offscreen_canvas);
      pthread_mutex_unlock(canvas_ptrs.canvas_mutex);

      if(prev_encX_count != encX_count){
        // Redraw the display here
        printf("NEW APP SELECTED - %d\n", encX_count);
      }
      prev_encX_count = encX_count;

      usleep(10*1000); //50ms

    } // while changing_app

    pthread_mutex_lock(canvas_ptrs.canvas_mutex);
    SetCanvasArea(offscreen_canvas, 0,0,64,44, &bg_color);
    pthread_mutex_unlock(canvas_ptrs.canvas_mutex);


    switch(encX_count){
      case 0:
        thr_function_ptr = &imageThread;
        break;
      case 1:
        thr_function_ptr = &spotifyThread;
        break;
      case 2:
        thr_function_ptr = &baseballThread;
        break;
      case 3:
        thr_function_ptr = &weatherThread;
        break;
      default:
        thr_function_ptr = &imageThread;
        break;

    }


      if(thr_function_ptr==NULL) {
        printf("ERROR: Function ptr is null\n");
        return 5;
      }

      // Create thread, and join it
      if(pthread_create(&app_thr, NULL, *thr_function_ptr, (void*)&canvas_ptrs) != 0) {
        printf("ERROR in clock thread creation");
        return 2;
      }

      if(pthread_join(app_thr, NULL) != 0) {
        printf("ERROR in thread joining");
        return 3;
      }
  } // while !intr_rec

printf("Exited while loop\n");

  if(pthread_join(clock_canvas_thr, NULL) != 0) {
    printf("ERROR in clock thread joining");
    return 3;
  }

printf("Thread joined\n");

  if(pthread_join(gpio_canvas_thr, NULL) != 0) {
    printf("ERROR in spotify thread joining");
    return 3;
  }


  // CLEANUP AND EXIT CLEANLY
  // Program is exited, shut down the RGB Matrix
  printf("Exiting... clearing canvas\n");
  canvas->Clear();
  delete canvas;
}

// END OF FILE ---------------------------------------------------------------
// ---------------------------------------------------------------------------
