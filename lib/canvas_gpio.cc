#include "canvas_gpio.h"

// SW_A_PIN 13
// ENCX_A_PIN 19
// ENCX_B_PIN 26

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using rgb_matrix::FrameCanvas;

volatile int encX_count = 0;

struct canvas_args{
  RGBMatrix *canvas;
  FrameCanvas *offscreen_canvas;
  pthread_mutex_t *canvas_mutex;
};

void* gpioThread(void *ptr){
  printf("Entered gpio thread\n");

  struct canvas_args *canvas_ptrs = (struct canvas_args*)ptr;
  RGBMatrix *canvas = canvas_ptrs->canvas;
  FrameCanvas *offscreen_canvas = canvas_ptrs->offscreen_canvas;
  pthread_mutex_t *canvas_mutex = canvas_ptrs->canvas_mutex;


  bool prev_encX_a = 0;
  bool prev_sw_pressed = 0;

  const uint64_t available_inputs = canvas->RequestInputs(0x06082000);

  fprintf(stderr, "Available GPIO-bits: ");
  for (int b = 0; b < 32; ++b) {
      if (available_inputs & (1<<b))
          fprintf(stderr, "%d ", b);
  }
  fprintf(stderr, "\n");

  while (!interrupt_received) {
    // Block and wait until any input bit changed or 100ms passed
    uint32_t inputs = canvas->AwaitInputChange(100);

    bool sw_pressed = !(inputs & 1<<SW_A_PIN); // switch is active low
    bool encX_a = (inputs & 1<<ENCX_A_PIN);
    bool encX_b = (inputs & 1<<ENCX_B_PIN);

    // Process rotary encoder
    if(encX_a != prev_encX_a) {
      if(encX_a == encX_b) {
        printf("COUNTER CLOCKWISE ROTATION - A: %d, B: %d\n", encX_a, encX_b);
        encX_count = (0 < encX_count) && changing_app ? --encX_count : encX_count;
      } else {
        printf("CLOCKWISE ROTATION - A: %d, B: %d\n", encX_a, encX_b);
        encX_count = (encX_count < number_apps) && changing_app ? ++encX_count : encX_count;
      }
      printf("Count = %d\n", encX_count);
    }
    prev_encX_a = encX_a;

    // Switch
    if(prev_sw_pressed==0 && sw_pressed==1) {
      changing_app = !changing_app;
printf("SWITCH PRESSED\n------------------------------CHANGING_APP = %d", changing_app);
    }
    prev_sw_pressed = sw_pressed;
  } // while not intr rec

  printf("Exiting GPIO thread\n");
  return NULL;
}
