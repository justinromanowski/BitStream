// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how write text.
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "graphics.h"

#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static bool FullSaturation(const rgb_matrix::Color &c) {
  return (c.r == 0 || c.r == 255)
    && (c.g == 0 || c.g == 255)
    && (c.b == 0 || c.b == 255);
}

int main(int argc, char *argv[]) {
  // Init time features
  static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  // Setup interrupts
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  // setup time_ptr
  struct tm* time_ptr;
  time_t t;

  // Init Matrix options
  RGBMatrix::Options matrix_options;
  matrix_options.hardware_mapping = "regular";
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.show_refresh_rate = false;

  // Configure colors
  rgb_matrix::Color color(180, 90, 0);
  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color flood_color(0, 0, 0);
  rgb_matrix::Color outline_color(0,0,0);
  bool with_outline = false;

  const char *time_font_ptr = NULL;
  const char *date_font_ptr = NULL;
  int letter_spacing = 0;

  // Get font types
  char time_font_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/7x14.bdf";
  time_font_ptr = strdup(time_font_filepath);
  if (time_font_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return 1;
  }

  char date_font_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/4x6.bdf";
  date_font_ptr = strdup(date_font_filepath);
  if (date_font_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return 1;
  }

  // Load font. This needs to be a filename with a bdf bitmap font.
  rgb_matrix::Font time_font;
  if (!time_font.LoadFont(time_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", time_font_ptr);
    return 1;
  }

  rgb_matrix::Font date_font;
  if (!date_font.LoadFont(date_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", date_font_ptr);
    return 1;
  }

  /*
   * If we want an outline around the font, we create a new font with
   * the original font as a template that is just an outline font.
   */
  rgb_matrix::Font *outline_font = NULL;
  if (with_outline) {
    outline_font = time_font.CreateOutlineFont();
  }
  RGBMatrix *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &matrix_options);
  if (canvas == NULL)
    return 1;

  const bool all_extreme_colors = (matrix_options.brightness == 100)
    && FullSaturation(color)
    && FullSaturation(bg_color)
    && FullSaturation(outline_color);
  if (all_extreme_colors)
    canvas->SetPWMBits(1);

  const int screen_divider = 44;

  int time_x = 0;
  int time_y = 51;

  int date_x = 0;
  int date_y = 46;

  int sec_x = 42;
  int sec_y = 53;

  canvas->Fill(flood_color.r, flood_color.g, flood_color.b);
  /*
  if (outline_font) {
    // The outline font, we need to write with a negative (-2) text-spacing,
    // as we want to have the same letter pitch as the regular text that
    // we then write on top.
    rgb_matrix::DrawText(canvas, *outline_font,
                         x - 1, y + time_font.baseline(),
                         outline_color, &bg_color, time, letter_spacing - 2);
  }
  */

  // Create variables to hold time, seconds, and date
  char time_str[1024];
  char sec_str[1024];
  char date_str[1024];

  while(!interrupt_received) {
    // Get current time
    t = time(NULL);
    time_ptr = localtime(&t);

    // Update char arrays w/ new data
    sprintf(time_str, "%.2d:%.2d:", time_ptr->tm_hour, time_ptr->tm_min);
    sprintf(sec_str, "%.2d AM", time_ptr->tm_sec);
    sprintf(date_str, "%.3s %.3s %.2d %d", wday_name[time_ptr->tm_wday], mon_name[time_ptr->tm_mon],
                                       time_ptr->tm_mday, 1900+(time_ptr->tm_year));
    // Update the time to the display
    rgb_matrix::DrawText(canvas, time_font, time_x, time_y + time_font.baseline(),
                         color, outline_font ? NULL : &bg_color, time_str,
                         letter_spacing);

    rgb_matrix::DrawText(canvas, date_font, date_x, date_y + date_font.baseline(),
                         color, outline_font ? NULL : &bg_color, date_str,
                         letter_spacing);

    rgb_matrix::DrawText(canvas, date_font, sec_x, sec_y + date_font.baseline(),
                         color, outline_font ? NULL : &bg_color, sec_str,
                         letter_spacing);

    usleep(500*1000);

  }

  // Finished. Shut down the RGB matrix.
  delete canvas;

  return 0;
}
