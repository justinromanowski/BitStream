// A preliminary test script messing around with using a
// mutex to regulate different threads from drawing on
// the canvas screen

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

// NAMESPACE SETUP
// ---------------
using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;

using ImageVector = std::vector<Magick::Image>;

// Divides clock from the display
const int screen_divider = 44;

const char *img_filename = "/home/justin/rpi-rgb-led-matrix/examples-api-use/pixel_house.png";

ImageVector images;
//RGBMatrix *canvas;
//FrameCanvas *offscreen_canvas;

struct canvas_args {
  RGBMatrix *canvas;
  FrameCanvas *offscreen_canvas;
};


rgb_matrix::Font time_font;
rgb_matrix::Font date_font;
rgb_matrix::Font *outline_font = NULL;

// THREAD INITIALIZATION
// ---------------------
pthread_t image_canvas_thr, clock_canvas_thr;
pthread_mutex_t canvas_mutex;


// FUNCTION SETUP - INTERRUPTS
// Used to peacefully terminate the program's execution
volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}


// FUNCTION SETUP - IMAGE DISPLAYING
// ---------------------------------

// Given the filename, load the image and scale to the size of the matrix.
// // If this is an animated image, the resutlting vector will contain multiple.
static ImageVector LoadImageAndScaleImage(const char *filename,
                                          int target_width,
                                          int target_height) {
  ImageVector result;

  ImageVector frames;
  try {
    readImages(&frames, filename);
  } catch (std::exception &e) {
    if (e.what())
      fprintf(stderr, "%s\n", e.what());
    return result;
  }

  if (frames.empty()) {
    fprintf(stderr, "No image found.");
    return result;
  }

  // Animated images have partial frames that need to be put together
  if (frames.size() > 1) {
    Magick::coalesceImages(&result, frames.begin(), frames.end());
  } else {
    result.push_back(frames[0]); // just a single still image.
  }

  for (Magick::Image &image : result) {
    image.scale(Magick::Geometry(target_width, target_height));
  }

  return result;
}

// Copy an image to a Canvas. Note, the RGBMatrix is implementing the Canvas
// interface as well as the FrameCanvas we use in the double-buffering of the
// animted image.
void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas,
                       const int *x_pos, const int *y_pos) {
  //const int offset_x = x;
  //const int offset_y = y;

  // Copy all the pixels to the canvas.
  for (size_t y = 0; y < image.rows(); ++y) {
    for (size_t x = 0; x < image.columns(); ++x) {
      const Magick::Color &c = image.pixelColor(x, y);
      if (c.alphaQuantum() < 256) {
        canvas->SetPixel(x + *x_pos, y + *y_pos,
                         ScaleQuantumToChar(c.redQuantum()),
                         ScaleQuantumToChar(c.greenQuantum()),
                         ScaleQuantumToChar(c.blueQuantum()));
      }
    }
  }
  printf("Constructed image\n");
}

// An animated image has to constantly swap to the next frame.
// We're using double-buffering and fill an offscreen buffer first, then show.
void ShowAnimatedImage(const Magick::Image &image, RGBMatrix *canvas,
                       const int *x_pos, const int *y_pos,
                       FrameCanvas *offscreen_canvas) {
      CopyImageToCanvas(image, offscreen_canvas, x_pos, y_pos);
      offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
}


// FUNCTION SETUP - CLOCK DISPLAYING
// ---------------------------------

static bool FullSaturation(const rgb_matrix::Color &c) {
  return (c.r == 0 || c.r == 255)
    && (c.g == 0 || c.g == 255)
    && (c.b == 0 || c.b == 255);
}


// FUNCTION SETUP - THREADS
// ------------------------
void* clockThread(void *ptr){

  printf("entered clock thread");

  struct canvas_args *canvas_ptrs = (struct canvas_args*)ptr;
  RGBMatrix *canvas = canvas_ptrs->canvas;
  FrameCanvas *offscreen_canvas = canvas_ptrs->offscreen_canvas;

  //RGBMatrix *canvas;
  //canvas = (RGBMatrix *) ptr;

  // INITIALIZE CLOCK VARIABLES
  static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  // Related to time.h struct
  struct tm* time_ptr;
  time_t t;

  // Configure colors for text
  rgb_matrix::Color color(180, 90, 0); //text color
  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color flood_color(0, 0, 0);
  rgb_matrix::Color outline_color(0,0,0);
  bool with_outline = false;

  // Positions for the time on the canvas
  int time_x = 0;
  int time_y = 51;

  int date_x = 0;
  int date_y = 46;

  int sec_x = 42;
  int sec_y = 53;

  char time_str[1024];
  char sec_str[1024];
  char date_str[1024];

  int letter_spacing = 0;
/*  // INITIALIZE FONTS

  const char *time_font_ptr = NULL;
  const char *date_font_ptr = NULL;

  // Get font types
  char time_font_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/7x14.bdf";
  time_font_ptr = strdup(time_font_filepath);
  if (time_font_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    //return 1;
  }
printf("time font success\n");
  char date_font_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/4x6.bdf";
  date_font_ptr = strdup(date_font_filepath);
  if (date_font_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    //return 1;
  }
printf("date font success\n");
  // Load font. This needs to be a filename with a bdf bitmap font.
  rgb_matrix::Font time_font;
  if (!time_font.LoadFont(time_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", time_font_ptr);
    //return 1;
  } else {
printf("time font created\n");
}
  rgb_matrix::Font date_font;
  if (!date_font.LoadFont(date_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", date_font_ptr);
    //return 1;
  }
printf("date font created\n");
  /*
   * If we want an outline around the font, we create a new font with
   * the original font as a template that is just an outline font.

  rgb_matrix::Font *outline_font = NULL;
  if (with_outline) {
    outline_font = time_font.CreateOutlineFont();
  }
*/
printf("outline font created");
  // PUT TIME ONTO CANVAS
  while(!interrupt_received) {
    // Get current time
    t = time(NULL);
    time_ptr = localtime(&t);

    // Update char arrays w/ new data
    sprintf(time_str, "%.2d:%.2d:", time_ptr->tm_hour, time_ptr->tm_min);
    sprintf(sec_str, "%.2d AM", time_ptr->tm_sec);
    sprintf(date_str, "%.3s %.3s %.2d %d", wday_name[time_ptr->tm_wday], mon_name[time_ptr->tm_mon],
                                           time_ptr->tm_mday, 1900+(time_ptr->tm_year));

    pthread_mutex_lock(&canvas_mutex);
    printf("Clock at MUTEX\n");
   // Update the time to the display
    rgb_matrix::DrawText(offscreen_canvas, time_font, time_x, time_y + time_font.baseline(),
                         color, outline_font ? NULL : &bg_color, time_str,
                         letter_spacing);

    rgb_matrix::DrawText(offscreen_canvas, date_font, date_x, date_y + date_font.baseline(),
                         color, outline_font ? NULL : &bg_color, date_str,
                         letter_spacing);

    rgb_matrix::DrawText(offscreen_canvas, date_font, sec_x, sec_y + date_font.baseline(),
                         color, outline_font ? NULL : &bg_color, sec_str,
                         letter_spacing);

    //offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
    canvas->SwapOnVSync(offscreen_canvas);


    pthread_mutex_unlock(&canvas_mutex);

    usleep(200*1000);

  }

  printf("Exited clock thread");

  //return 0;
}

void* imageThread(void *ptr){

  printf("Entered image thread");

  struct canvas_args *canvas_ptrs = (struct canvas_args*)ptr;
  RGBMatrix *canvas = canvas_ptrs->canvas;
  FrameCanvas *offscreen_canvas = canvas_ptrs->offscreen_canvas;

  Magick::InitializeMagick(NULL);

  // INITIALIZE IMAGE

//  const char *img_filename = "/home/justin/rpi-rgb-led-matrix/examples-api-use/pixart.png";

//  ImageVector images = LoadImageAndScaleImage(img_filename,
//                                              canvas->width(),
//                                              screen_divider);

  const int offset_x = 0;
  const int offset_y = 0;

  int image_display_count = 0;
  int still_image_sleep = 1000000;

  while(!interrupt_received) {

    // PUT IMAGE ONTO CANVAS
    pthread_mutex_lock(&canvas_mutex);
    printf("Image at MUTEX\n");

    switch (images.size()) {
    case 0:   // failed to load image.
      pthread_mutex_unlock(&canvas_mutex);
      break;
    case 1:   // Simple example: one image to show
      // Shanty code with the delays, but it works

      CopyImageToCanvas(images[0], offscreen_canvas, &offset_x, &offset_y);
      canvas->SwapOnVSync(offscreen_canvas);
      pthread_mutex_unlock(&canvas_mutex);
      /*
      if(image_display_count>100){
        still_image_sleep = 1000000;
      } else {
        still_image_sleep = 10000;
        still_image_sleep += 1;
      }
      */
      usleep(still_image_sleep*3);
      break;
    default:  // More than one image: this is an animation.
     //FrameCanvas *offscreen_canvas = canvas->CreateFrameCanvas();
     for (const auto &image : images) {
       ShowAnimatedImage(image,canvas, &offset_x, &offset_y, offscreen_canvas);
       pthread_mutex_unlock(&canvas_mutex);
       usleep(image.animationDelay() * 10000);  // 1/100s converted to usec
     }
      break;
    }
  }

  printf("Exited image thread");

  //return 0;
}

int main(int argc, char *argv[]) {

  // INITIALIZE MATRIX/CANVAS
  /*
  RGBMatrix::Options matrix_options;
  matrix_options.hardware_mapping = "regular";
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.show_refresh_rate = false;
  */

  RGBMatrix::Options matrix_options;
  matrix_options.hardware_mapping = "regular";
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.show_refresh_rate = false;

  // INITIALIZE IMAGE
  Magick::InitializeMagick(*argv);


  RGBMatrix *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &matrix_options);
  if (canvas == NULL)
    return 1;

  FrameCanvas *offscreen_canvas = canvas->CreateFrameCanvas();

  struct canvas_args canvas_ptrs;
  canvas_ptrs.canvas = canvas;
  canvas_ptrs.offscreen_canvas = offscreen_canvas;

  // INITIALIZE INTERRUPTS
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


  images = LoadImageAndScaleImage(img_filename,
                                  64,
                                  screen_divider);

  // INITIALIZE CLOCK VARIABLES
  /*
  static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  // Related to time.h struct
  struct tm* time_ptr;
  time_t t;

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

  // Positions for the time on the canvas
  int time_x = 0;
  int time_y = 51;

  int date_x = 0;
  int date_y = 46;

  int sec_x = 42;
  int sec_y = 53;

  char time_str[1024];
  char sec_str[1024];
  char date_str[1024];
  */

  // INITIALIZE IMAGES
  /*
  Magick::InitializeMagick(*argv);

  const char *img_filename = "/home/justin/rpi-rgb-led-matrix/examples-api-use/pixart.png";

  ImageVector images = LoadImageAndScaleImage(img_filename,
                                              canvas->width(),
                                              screen_divider);

  const int offset_x = 0;
  const int offset_y = 0;
  */

  // INITIALIZE FONTS
  /*
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
   *
  rgb_matrix::Font *outline_font = NULL;
  if (with_outline) {
    outline_font = time_font.CreateOutlineFont();
  }
  */

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

    // INITIALIZE FONTS

  const char *time_font_ptr = NULL;
  const char *date_font_ptr = NULL;
  int letter_spacing = 0;

  // Get font types
  char time_font_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/7x14B.bdf";
  time_font_ptr = strdup(time_font_filepath);
  if (time_font_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    //return 1;
  }
  char date_font_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/4x6.bdf";
  date_font_ptr = strdup(date_font_filepath);
  if (date_font_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    //return 1;
  }
  // Load font. This needs to be a filename with a bdf bitmap font.
  if (!time_font.LoadFont(time_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", time_font_ptr);
    //return 1;
  } else {
}
  if (!date_font.LoadFont(date_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", date_font_ptr);
    //return 1;
  }
  /*
   * If we want an outline around the font, we create a new font with
   * the original font as a template that is just an outline font.
   */
  if (with_outline) {
    outline_font = time_font.CreateOutlineFont();
  }


  /*
  // PUT IMAGE ONTO CANVAS
  switch (images.size()) {
  case 0:   // failed to load image.
    break;
  case 1:   // Simple example: one image to show
    CopyImageToCanvas(images[0], canvas, &offset_x, &offset_y);
    while (!interrupt_received) sleep(1000);  // Until Ctrl-C is pressed
    break;
  default:  // More than one image: this is an animation.
    ShowAnimatedImage(images, canvas, &offset_x, &offset_y);
    break;
  }
  */

  // PUT TIME ONTO CANVAS
  /*
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
  */

  if (pthread_mutex_init(&canvas_mutex, NULL) != 0) {
        printf("Mutex INIT failed\n");
        return 1;
  }

  if(pthread_create(&clock_canvas_thr, NULL, clockThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in clock thread creation");
    return 2;
  }

  if(pthread_create(&image_canvas_thr, NULL, imageThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in image thread creation");
   return 2;
  }

//  clockThread(canvas, &matrix_options);
//  imageThread(canvas);

  // Join the threads to main thread -- don't execute cleanup code
  // until interrupt is received
  void *canvas_ret;
  void *image_ret;

  if(pthread_join(clock_canvas_thr, &canvas_ret) != 0) {
    printf("ERROR in clock thread joining");
    return 3;
  }

  if(pthread_join(image_canvas_thr, &image_ret) != 0) {
    printf("ERROR in image thread joining");
    return 3;
  }

  // Program is exited, shut down the RGB Matrix
  printf("Exiting... clearing canvas\n");
  canvas->Clear();
  delete canvas;

}
