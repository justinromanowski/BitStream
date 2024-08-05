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
<<<<<<< HEAD
  using ImageVector = std::vector<Magick::Image>;
=======
>>>>>>> dd2df64942b046e817853a15e3e2f7a11054ec14

/////////////////////////////////////////////////////////////////////////////
// FUNCTIONS //
/////////////////////////////////////////////////////////////////////////////
<<<<<<< HEAD

  extern void fontSetup();

=======
/*
  void fontSetup();
>>>>>>> dd2df64942b046e817853a15e3e2f7a11054ec14

  ImageVector LoadImageAndScaleImage(const char *filename,
                                     int target_width,
                                     int target_height);

<<<<<<< HEAD

=======
>>>>>>> dd2df64942b046e817853a15e3e2f7a11054ec14
  void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas,
                       const int *x_pos, const int *y_pos);

  void CopyImageToCanvasTransparent(const Magick::Image &image, Canvas *canvas,
                                    const int *x_pos, const int *y_pos);

  void ShowAnimatedImage(const Magick::Image &image, RGBMatrix *canvas,
                         const int *x_pos, const int *y_pos,
                         FrameCanvas *offscreen_canvas);

<<<<<<< HEAD
  void SetCanvasArea(FrameCanvas *offscreen_canvas, int x, int y,
                     int width, int height, rgb_matrix::Color *color);

  bool FullSaturation(const rgb_matrix::Color &c);

=======
  bool FullSaturation(const rgb_matrix::Color &c) {
    return (c.r == 0 || c.r == 255)
      && (c.g == 0 || c.g == 255)
      && (c.b == 0 || c.b == 255);
  }
*/
>>>>>>> dd2df64942b046e817853a15e3e2f7a11054ec14

/////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES  //
/////////////////////////////////////////////////////////////////////////////

  // FONTS //
<<<<<<< HEAD
  extern rgb_matrix::Font seven_fourteen_font;
  extern rgb_matrix::Font four_six_font;
  extern rgb_matrix::Font five_seven_font;
  extern rgb_matrix::Font six_ten_font;
  extern rgb_matrix::Font eight_thirteen_font;
=======
  rgb_matrix::Font seven_fourteen_font;
  rgb_matrix::Font four_six_font;
  rgb_matrix::Font five_seven_font;
  rgb_matrix::Font six_ten_font;
  rgb_matrix::Font eight_thirteen_font;
  rgb_matrix::Font *outline_font = NULL;
>>>>>>> dd2df64942b046e817853a15e3e2f7a11054ec14


/////////////////////////////////////////////////////////////////////////////
// CLASSES AND STRUCTS //
/////////////////////////////////////////////////////////////////////////////

  struct canvas_args{
    RGBMatrix *canvas;
    FrameCanvas *offscreen_canvas;
<<<<<<< HEAD
    pthread_mutex_t *canvas_mutex;
=======
    pthread_mutex_t canvas_mutex;
>>>>>>> dd2df64942b046e817853a15e3e2f7a11054ec14
  };


  class ClockClass {
    private:
      ////////////////////////////////////////////////////////// CONSTANTS //
      // LED MATRIX VARIABLES //
      RGBMatrix *canvas;
      FrameCanvas *offscreen_canvas;
<<<<<<< HEAD
      pthread_mutex_t *canvas_mutex;
=======
      pthread_mutex_t canvas_mutex;
>>>>>>> dd2df64942b046e817853a15e3e2f7a11054ec14

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
