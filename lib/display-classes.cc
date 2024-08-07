/////////////////////////////////////////////////////////////////////////////
// Desc:
// Display classes holds the classes that are used in the thread functions
// in display-threads.cc. This way, the code is able to stay better organized
// in comparison to stuffing everything related to each app inside of a
// single function.
//
/////////////////////////////////////////////////////////////////////////////


#include "display-classes.h"


using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;
using ImageVector = std::vector<Magick::Image>;

extern volatile bool interrupt_received;

rgb_matrix::Font seven_fourteen_font;
rgb_matrix::Font four_six_font;
rgb_matrix::Font five_seven_font;
rgb_matrix::Font six_ten_font;
rgb_matrix::Font eight_thirteen_font;

/////////////////////////////////////////////////////////////////////////////
// FUNCTIONS //
/////////////////////////////////////////////////////////////////////////////

ImageVector LoadImageAndScaleImage(const char *filename,
                                   int target_width,
                                   int target_height) {
  // Given the filename, load the image and scale to the size of the matrix.
  // If this is an animated image, the resutlting vector will contain multiple.

  //Magick::InitializeMagick(NULL);
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


void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas,
                       const int *x_pos, const int *y_pos) {
  // Copy an image to a Canvas. Note, the RGBMatrix is implementing the Canvas
  // interface as well as the FrameCanvas we use in the double-buffering of the
  // animted image.

  // Copy all the pixels to the canvas.
  for (size_t y = 0; y < image.rows(); ++y) {
    for (size_t x = 0; x < image.columns(); ++x) {
      const Magick::Color &c = image.pixelColor(x, y);
      //printf("X: %d | Y: %d | ALPHAQUANTIM = %d\n", y, x, c.alphaQuantum());
//      if (c.alphaQuantum() < 70000) { // 256
        canvas->SetPixel(x + *x_pos, y + *y_pos,
                         ScaleQuantumToChar(c.redQuantum()),
                         ScaleQuantumToChar(c.greenQuantum()),
                         ScaleQuantumToChar(c.blueQuantum()));
//      }
    }
  }
}


void CopyImageToCanvasTransparent(const Magick::Image &image, Canvas *canvas,
                       const int *x_pos, const int *y_pos) {

  // Copy all the pixels to the canvas.
  for (size_t y = 0; y < image.rows(); ++y) {
    for (size_t x = 0; x < image.columns(); ++x) {
      const Magick::Color &c = image.pixelColor(x, y);
      if (c.redQuantum()>0 || c.greenQuantum()>0 || c.blueQuantum()>0) { // 256
        canvas->SetPixel(x + *x_pos, y + *y_pos,
                         ScaleQuantumToChar(c.redQuantum()),
                         ScaleQuantumToChar(c.greenQuantum()),
                         ScaleQuantumToChar(c.blueQuantum()));
      }
    }
  }
}


void ShowAnimatedImage(const Magick::Image &image, RGBMatrix *canvas,
                       const int *x_pos, const int *y_pos,
                       FrameCanvas *offscreen_canvas) {
  // An animated image has to constantly swap to the next frame.
  // We're using double-buffering and fill an offscreen buffer first, then show.

  CopyImageToCanvas(image, offscreen_canvas, x_pos, y_pos);
  offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
}


static bool FullSaturation(const rgb_matrix::Color &c) {
  return (c.r == 0 || c.r == 255)
    && (c.g == 0 || c.g == 255)
    && (c.b == 0 || c.b == 255);
}


void SetCanvasArea(FrameCanvas *offscreen_canvas, int x, int y,
                     int width, int height, rgb_matrix::Color *color) {
  for (int iy = 0; iy < height; ++iy) {
    for (int ix = 0; ix < width; ++ix) {
      offscreen_canvas->SetPixel(x + ix, y + iy, color->r, color->g, color->b);
    }
  }
}


void fontSetup() {
  // INITIALIZE FONT PTRS
  const char *seven_fourteen_ptr = NULL;
  const char *four_six_ptr = NULL;
  const char *five_seven_ptr = NULL;
  const char *six_ten_ptr = NULL;
  const char *eight_thirteen_ptr = NULL;

  // Get font types
  char seven_fourteen_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/7x14B.bdf";
  seven_fourteen_ptr = strdup(seven_fourteen_filepath);
  if (seven_fourteen_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return;
  }
  char four_six_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/4x6.bdf";
  four_six_ptr = strdup(four_six_filepath);
  if (four_six_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return;
  }
  char five_seven_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/5x7.bdf";
  five_seven_ptr = strdup(five_seven_filepath);
  if (five_seven_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return;
  }
  char six_ten_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/6x10.bdf";
  six_ten_ptr = strdup(six_ten_filepath);
  if (six_ten_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return;
  }
  char eight_thirteen_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/9x15B.bdf";
  eight_thirteen_ptr = strdup(eight_thirteen_filepath);
  if (eight_thirteen_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return;
  }

  // Load font. This needs to be a filename with a bdf bitmap font.
  if (!seven_fourteen_font.LoadFont(seven_fourteen_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", seven_fourteen_ptr);
    return;
  }
  if (!four_six_font.LoadFont(four_six_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", four_six_ptr);
    return;
  }
  if (!five_seven_font.LoadFont(five_seven_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", five_seven_ptr);
    return;
  }
  if (!six_ten_font.LoadFont(six_ten_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", six_ten_ptr);
    return;
  }
  if (!eight_thirteen_font.LoadFont(eight_thirteen_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", eight_thirteen_ptr);
    return;
  }
}


/////////////////////////////////////////////////////////////////////////////
// CLOCK CLASS //
/////////////////////////////////////////////////////////////////////////////

ClockClass::ClockClass(void *ptr) {
  // Load in the necessary pointers and objects
  if(ptr==NULL) {
    printf("ERROR: NULL POINTER");
  }

  canvas_args *canvas_ptrs = (struct canvas_args*)ptr;

  canvas = canvas_ptrs->canvas;
  offscreen_canvas = canvas_ptrs->offscreen_canvas;
  canvas_mutex = canvas_ptrs->canvas_mutex;

  printf("Clock Class initialized");
}

const char ClockClass::wday_name[][4] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char ClockClass::mon_name[][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};



void ClockClass::updateTime() {
  // Fetch current time
  t = time(NULL);
  time_ptr = localtime(&t);

  // MAKE GLOBAL AT SOME POINT
  bool twelve_hr = true;

  int time_hr;
  std::string am_pm;

  if(twelve_hr) {
    time_hr = time_ptr->tm_hour;
    // Check AM or PM
    if(time_hr > 12) {
      time_hr -= 12;
      am_pm = "PM";
    } else if(time_hr == 0) {
      time_hr = 12;
      am_pm = "AM";
    } else {
      am_pm = "AM";
    }
  } else {
    am_pm = "  ";
  }

  // Update char arrays w/ new time data
  sprintf(time_str, "%.2d:%.2d:", time_hr, time_ptr->tm_min);
  sprintf(sec_str, "%.2d %.2s", time_ptr->tm_sec, am_pm.c_str());
  sprintf(date_str, "%.3s %.3s %.2d %d", wday_name[time_ptr->tm_wday], mon_name[time_ptr->tm_mon],
                                         time_ptr->tm_mday, 1900+(time_ptr->tm_year));

}

void ClockClass::drawDisplay() {

  pthread_mutex_lock(canvas_mutex);
  printf("CLCOK AT MUTEX\n");

  SetCanvasArea(offscreen_canvas, 0, 45, 64, (64-45), &bg_color);

  // Draw the updated time onto the display
  rgb_matrix::DrawText(offscreen_canvas, seven_fourteen_font, time_x, time_y + seven_fourteen_font.baseline(),
                       color, NULL, time_str,
                       letter_spacing);

  rgb_matrix::DrawText(offscreen_canvas, four_six_font, date_x, date_y + four_six_font.baseline(),
                       color, NULL, date_str,
                       letter_spacing);

  rgb_matrix::DrawText(offscreen_canvas, four_six_font, sec_x, sec_y + four_six_font.baseline(),
                       color, NULL, sec_str,
                       letter_spacing);

  //offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
  canvas->SwapOnVSync(offscreen_canvas);

  pthread_mutex_unlock(canvas_mutex);
}


/////////////////////////////////////////////////////////////////////////////
// HOMEPAGE CLASS //
/////////////////////////////////////////////////////////////////////////////

HomePageClass::HomePageClass(void* ptr) {
  // Load in the necessary pointers and objects
  if(ptr==NULL) {
    printf("ERROR: NULL POINTER");
  }

  canvas_args *canvas_ptrs = (struct canvas_args*)ptr;
  canvas = canvas_ptrs->canvas;
  offscreen_canvas = canvas_ptrs->offscreen_canvas;
  canvas_mutex = canvas_ptrs->canvas_mutex;

  loadImages();
}

void HomePageClass::loadImages() {
  // BITS //
  const char* baseball_bit_fp = baseball_bit_path.c_str();
  const char* baseball_min_fp = baseball_min_path.c_str();
  const char* image_bit_fp = image_bit_path.c_str();
  const char* image_min_fp = image_min_path.c_str();
  const char* spotify_bit_fp = spotify_bit_path.c_str();
  const char* spotify_min_fp = spotify_min_path.c_str();
  const char* weather_bit_fp = weather_bit_path.c_str();
  const char* weather_min_fp = weather_min_path.c_str();

  baseball_bit = LoadImageAndScaleImage(baseball_bit_fp, bit_size, bit_size);
  baseball_min = LoadImageAndScaleImage(baseball_min_fp, min_size, min_size);
  image_bit = LoadImageAndScaleImage(image_bit_fp, bit_size, bit_size);
  image_min = LoadImageAndScaleImage(image_min_fp, min_size, min_size);
  spotify_bit = LoadImageAndScaleImage(spotify_bit_fp, bit_size, bit_size);
  spotify_min = LoadImageAndScaleImage(spotify_min_fp, min_size, min_size);
  weather_bit = LoadImageAndScaleImage(weather_bit_fp, bit_size, bit_size);
  weather_min = LoadImageAndScaleImage(weather_min_fp, min_size, min_size);

  // LOGO //
  const char* bitstream_logo_fp = "images/bitstream_3.png";
  bitstream_logo = LoadImageAndScaleImage(bitstream_logo_fp, 60, 20);

  printf("Images initialized successfully - homepage\n");
}

void HomePageClass::drawBits() {
  bit_x = 0;
  // IF the current bit is selected by the encoder, then it should be
  // enlarged for the user to see. ELSE, draw the minimized bit.

  if(encX_count==IMAGE) {
    CopyImageToCanvas(image_bit[0], offscreen_canvas, &bit_x, &bit_y);
    bit_x += bit_size+1;
  } else {
    CopyImageToCanvas(image_min[0], offscreen_canvas, &bit_x, &min_y);
    bit_x += min_size+1;
  }

  if(encX_count==SPOTIFY) {
    CopyImageToCanvas(spotify_bit[0], offscreen_canvas, &bit_x, &bit_y);
    bit_x += bit_size+1;
  } else {
    CopyImageToCanvas(spotify_min[0], offscreen_canvas, &bit_x, &min_y);
    bit_x += min_size+1;
  }

  if(encX_count==BASEBALL) {
    CopyImageToCanvas(baseball_bit[0], offscreen_canvas, &bit_x, &bit_y);
    bit_x += bit_size+1;
  } else {
    CopyImageToCanvas(baseball_min[0], offscreen_canvas, &bit_x, &min_y);
    bit_x += min_size+1;
  }

  if(encX_count==WEATHER) {
    CopyImageToCanvas(weather_bit[0], offscreen_canvas, &bit_x, &bit_y);
    bit_x += bit_size+1;
  } else {
    CopyImageToCanvas(weather_min[0], offscreen_canvas, &bit_x, &min_y);
    bit_x += min_size+1;
  }
}

void HomePageClass::drawBitStreamLogo() {
  CopyImageToCanvas(bitstream_logo[0], offscreen_canvas, &logo_x, &logo_y);
}

void HomePageClass::clearDisplay() {
  SetCanvasArea(offscreen_canvas, 0,0,64,44, &bg_color);
}
