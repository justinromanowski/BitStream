#ifndef DISPLAY_CLASSES_H
  #define DISPLAY_CLASSES_H

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
  using ImageVector = std::vector<Magick::Image>;

/////////////////////////////////////////////////////////////////////////////
// FUNCTIONS //
/////////////////////////////////////////////////////////////////////////////

  extern void fontSetup();


/////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES  //
/////////////////////////////////////////////////////////////////////////////

  // FONTS //
  extern rgb_matrix::Font seven_fourteen_font;
  extern rgb_matrix::Font four_six_font;
  extern rgb_matrix::Font five_seven_font;
  extern rgb_matrix::Font six_ten_font;
  extern rgb_matrix::Font eight_thirteen_font;

/////////////////////////////////////////////////////////////////////////////
// CLASSES AND STRUCTS //
/////////////////////////////////////////////////////////////////////////////

  struct canvas_args{
    RGBMatrix *canvas;
    FrameCanvas *offscreen_canvas;
    pthread_mutex_t *canvas_mutex;
  };


  class ClockClass {
    private:
      ////////////////////////////////////////////////////////// CONSTANTS //
      // LED MATRIX VARIABLES //
      RGBMatrix *canvas;
      FrameCanvas *offscreen_canvas;
      pthread_mutex_t *canvas_mutex;

      // CLOCK VARIABLES //
      static const char wday_name[][4];
      static const char mon_name[][4];

      // CTIME VARIABLES //
      struct tm* time_ptr;
      time_t t;

      // COLOR VARIABLES //
      rgb_matrix::Color color = rgb_matrix::Color(180, 90, 0);
      rgb_matrix::Color bg_color = rgb_matrix::Color(0, 0, 0);
      rgb_matrix::Color flood_color = rgb_matrix::Color(0, 0, 0);
      rgb_matrix::Color outline_color = rgb_matrix::Color(0,0,0);
      bool with_outline = false; // NOT NEEDED ?

      // POSITION VARIABLES //
      int time_x = 0;
      int time_y = 52;

      int date_x = 0;
      int date_y = 47;

      int sec_x = 42;
      int sec_y = 54;

      // APP VARIABLES //
      char time_str[1024];
      char sec_str[1024];
      char date_str[1024];

      int letter_spacing = 0;

    public:
      //////////////////////////////////////////////////// FUNCTIONS //
      void updateTime();

      void drawDisplay();

      ClockClass(void *ptr);

  };

#endif
