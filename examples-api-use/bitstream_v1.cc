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

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>


// NAMESPACE SETUP -----------------------------------------------------------
// ---------------------------------------------------------------------------
using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;



// GLOBAL VARIABLE DECLARATIONS ----------------------------------------------
// ---------------------------------------------------------------------------
using ImageVector = std::vector<Magick::Image>;

const int screen_divider = 44;

pthread_t image_canvas_thr, clock_canvas_thr, spotify_canvas_thr, baseball_canvas_thr;
pthread_mutex_t canvas_mutex;


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

  canvas_args canvas_ptrs;
  canvas_ptrs.canvas = canvas;
  canvas_ptrs.offscreen_canvas = offscreen_canvas;
  canvas_ptrs.canvas_mutex = canvas_mutex;

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

   // If we want an outline around the font, we create a new font with
   // the original font as a template that is just an outline font.

  // INITIALIZE MUTEX
  if (pthread_mutex_init(&canvas_mutex, NULL) != 0) {
        printf("Canvas Mutex INIT failed\n");
        return 1;
  }


  // CREATE THREADS
  if(pthread_create(&clock_canvas_thr, NULL, clockThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in clock thread creation");
    return 2;
  }
/*
  if(pthread_create(&image_canvas_thr, NULL, imageThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in image thread creation");
   return 2;
  }
*/
/*
  if(pthread_create(&spotify_canvas_thr, NULL, spotifyThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in spotify thread creation");
    return 2;
  }
*/

  if(pthread_create(&baseball_canvas_thr, NULL, baseballThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in baseball thread creation");
    return 2;
  }


  // JOIN THREADS TOGETHER
  // Join the threads to main thread -- don't execute cleanup code
  // until interrupt is received
  void *canvas_ret;
  void *image_ret;
  void *spotify_ret;
  void *baseball_ret;

  if(pthread_join(clock_canvas_thr, &canvas_ret) != 0) {
    printf("ERROR in clock thread joining");
    return 3;
  }
/*
  if(pthread_join(image_canvas_thr, &image_ret) != 0) {
    printf("ERROR in image thread joining");
    return 3;
  }
*/
/*
  if(pthread_join(spotify_canvas_thr, &spotify_ret) != 0) {
    printf("ERROR in spotify thread joining");
    return 3;
  }
*/

  if(pthread_join(baseball_canvas_thr, &baseball_ret) != 0) {
    printf("ERROR in baseball thread joining");
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
