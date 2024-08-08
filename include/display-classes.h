#ifndef DISPLAY_CLASSES_H
  #define DISPLAY_CLASSES_H

  #define IMAGE 0
  #define SPOTIFY 1
  #define BASEBALL 2
  #define WEATHER 3

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

  extern volatile int encX_count;

/////////////////////////////////////////////////////////////////////////////
// FUNCTIONS //
/////////////////////////////////////////////////////////////////////////////

  extern void fontSetup();

  extern ImageVector LoadImageAndScaleImage(const char *filename,
                                     int target_width,
                                     int target_height);

  extern void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas,
                         const int *x_pos, const int *y_pos);

  extern void CopyImageToCanvasTransparent(const Magick::Image &image, Canvas *canvas,
                                    const int *x_pos, const int *y_pos);

  extern void ShowAnimatedImage(const Magick::Image &image, RGBMatrix *canvas,
                         const int *x_pos, const int *y_pos,
                         FrameCanvas *offscreen_canvas);


  extern void SetCanvasArea(FrameCanvas *offscreen_canvas, int x, int y,
                            int width, int height, rgb_matrix::Color *color);

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

///////////////////////////////////////////////////////////////////////////// CLOCK CLASS //
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


////////////////////////////////////////////////////////////////////////// HOMEPAGE CLASS //
class HomePageClass {
  private:
    /////////////////////////////////////////////////////// FILEPATHS //
    std::string base_path = "images/app_bits/";
    std::string baseball_bit_path = base_path + "baseball_bit.png";
    std::string baseball_min_path = base_path + "baseball_min.png";
    std::string image_bit_path = base_path + "image_bit.png";
    std::string image_min_path = base_path + "image_min.png";
    std::string spotify_bit_path = base_path + "spotify_bit.png";
    std::string spotify_min_path = base_path + "spotify_min.png";
    std::string weather_bit_path = base_path + "weather_bit.png";
    std::string weather_min_path = base_path + "weather_min.png";

    /////////////////////////////////////////////////////// VARIABLES //
    ImageVector baseball_bit, baseball_min, image_bit, image_min, spotify_bit, spotify_min, weather_bit, weather_min;
    ImageVector bitstream_logo;

    RGBMatrix *canvas;
    FrameCanvas *offscreen_canvas;
    pthread_mutex_t *canvas_mutex;

    rgb_matrix::Color bg_color = rgb_matrix::Color(0, 0, 0);

    /////////////////////////////////////////////////////// CONSTANTS //
    const int min_size = 6;
    const int bit_size = 10;

    const int min_y = 34;
    const int bit_y = 30;
    int bit_x = 0;

    const int logo_x = 2;
    const int logo_y = 5;

  public:
    HomePageClass(void* ptr);

    void loadImages();

    void drawBitStreamLogo();

    void drawBits();

    void clearDisplay();

};


#endif
