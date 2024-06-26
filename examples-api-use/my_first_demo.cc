// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static void DrawSmiley(Canvas *canvas, int start_x, int start_y){
  canvas->SetPixel(start_x+2, start_y, 150,150,150);
  canvas->SetPixel(start_x+2, start_y+1, 150,150,150);
  canvas->SetPixel(start_x+2, start_y+2, 150,150,150);
  canvas->SetPixel(start_x+4, start_y, 150,150,150);
  canvas->SetPixel(start_x+4, start_y+1, 150,150,150);
  canvas->SetPixel(start_x+4, start_y+2, 150,150,150);

  canvas->SetPixel(start_x+1, start_y+4, 150,150,150);
  canvas->SetPixel(start_x+2, start_y+4, 150,150,150);
  canvas->SetPixel(start_x+3, start_y+4, 150,150,150);
  canvas->SetPixel(start_x+4, start_y+4, 150,150,150);
  canvas->SetPixel(start_x+5, start_y+4, 150,150,150);
  canvas->SetPixel(start_x, start_y+3, 150,150,150);
  canvas->SetPixel(start_x+6, start_y+3, 150,150,150);

}

static void DrawOnCanvas(Canvas *canvas){
  // Call once and sequentially draw R, G, and B lines on the screen
  canvas->Fill(0,100,0);
  sleep(1);

  for(int i = 0; i<(canvas->width()); i++){
    if(interrupt_received) break;

    canvas->SetPixel(i, i, 100,0,0);
    usleep(200*1000);
  }

  DrawSmiley(canvas, 32, 5);
  sleep(1);
  DrawSmiley(canvas, 5, 32);
  sleep(3);
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options defaults;
  defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
  defaults.rows = 64;
  defaults.cols = 64;
  defaults.chain_length = 1;
  defaults.parallel = 1;
  defaults.show_refresh_rate = true;

  Canvas *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);
  if (canvas == NULL)
    return 1;

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  DrawOnCanvas(canvas);

  // Animation finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  return 0;
}
