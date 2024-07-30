#ifndef DISPLAY_THREADS_H
#define DISPLAY_THREADS_H

#include "led-matrix.h"
#include "graphics.h"
#include "thread.h"

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

#include <chrono>

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;

struct canvas_args{
  RGBMatrix *canvas;
  FrameCanvas *offscreen_canvas;
  pthread_mutex_t canvas_mutex;
};

void fontSetup();

void* clockThread(void *ptr);

void* imageThread(void *ptr);

void* spotifyThread(void *ptr);

void* baseballThread(void *ptr);

void* weatherThread(void *ptr);

#endif
