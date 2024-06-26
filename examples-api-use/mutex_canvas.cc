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
const char *img_filename = "/home/justin/rpi-rgb-led-matrix/examples-api-use/pixel_house.png";
const char *album_cover_filename = "/home/justin/rpi-rgb-led-matrix/examples-api-use/spotify_album.png";
const char *spotify_icon_fn = "/home/justin/rpi-rgb-led-matrix/examples-api-use/spotify.png";
const char *spotify_pause_fn = "/home/justin/rpi-rgb-led-matrix/examples-api-use/spotify_pause.png";
const char *spotify_play_fn = "/home/justin/rpi-rgb-led-matrix/examples-api-use/spotify_play.png";

ImageVector images;

// Passed to all threads that need to use the canvas
struct canvas_args {
  RGBMatrix *canvas;
  FrameCanvas *offscreen_canvas;
};

rgb_matrix::Font time_font;
rgb_matrix::Font date_font;
rgb_matrix::Font five_seven_font;
rgb_matrix::Font six_ten_font;
rgb_matrix::Font *outline_font = NULL;

// FIFO Setup
int cmd_fd;
int data_fd;

// THREAD INITIALIZATION -----------------------------------------------------
// ---------------------------------------------------------------------------
  // Initialize all of the necessary threads to control different apps in
  // the display, as well as other tasks such as FIFO for IPC
  // -------------------------------------------------------------------------
pthread_t image_canvas_thr, clock_canvas_thr, spotify_canvas_thr;
pthread_mutex_t canvas_mutex;


// FUNCTION SETUP - INTERRUPTS -----------------------------------------------
// ---------------------------------------------------------------------------
  // Used to peacefully terminate the program's execution
  // -------------------------------------------------------------------------
volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}



// FUNCTION SETUP - IMAGE DISPLAYING -----------------------------------------
// ---------------------------------------------------------------------------

  // -------------------------------------------------------------------------
  // Given the filename, load the image and scale to the size of the matrix.
  // If this is an animated image, the resutlting vector will contain multiple.
  // -------------------------------------------------------------------------
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



  // ---------------------------------------------------------------------------
  // Copy an image to a Canvas. Note, the RGBMatrix is implementing the Canvas
  // interface as well as the FrameCanvas we use in the double-buffering of the
  // animted image.
  // ---------------------------------------------------------------------------
void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas,
                       const int *x_pos, const int *y_pos) {

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
  printf("Constructed image\n");
}



  // -------------------------------------------------------------------------
  // An animated image has to constantly swap to the next frame.
  // We're using double-buffering and fill an offscreen buffer first, then show.
  // -------------------------------------------------------------------------
void ShowAnimatedImage(const Magick::Image &image, RGBMatrix *canvas,
                       const int *x_pos, const int *y_pos,
                       FrameCanvas *offscreen_canvas) {
      CopyImageToCanvas(image, offscreen_canvas, x_pos, y_pos);
      offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
}


// FUNCTION SETUP - CLOCK DISPLAYING -----------------------------------------
// ---------------------------------------------------------------------------

static bool FullSaturation(const rgb_matrix::Color &c) {
  return (c.r == 0 || c.r == 255)
    && (c.g == 0 || c.g == 255)
    && (c.b == 0 || c.b == 255);
}

void ClearCanvasArea(FrameCanvas *offscreen_canvas, int x, int y,
                     int width, int height, rgb_matrix::Color *color) {
  for (int iy = 0; iy < height; ++iy) {
    for (int ix = 0; ix < width; ++ix) {
      offscreen_canvas->SetPixel(x + ix, y + iy, color->r, color->g, color->b);
    }
  }
}

// FUNCTION SETUP - THREADS --------------------------------------------------
// ---------------------------------------------------------------------------
void* clockThread(void *ptr){

  printf("entered clock thread");

  struct canvas_args *canvas_ptrs = (struct canvas_args*)ptr;
  RGBMatrix *canvas = canvas_ptrs->canvas;
  FrameCanvas *offscreen_canvas = canvas_ptrs->offscreen_canvas;

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
  rgb_matrix::Color color(180, 90, 0); // text color
  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color flood_color(0, 0, 0);
  rgb_matrix::Color outline_color(0,0,0);
  bool with_outline = false;

  // Positions for the time on the canvas
  int time_x = 0;
  int time_y = 52;

  int date_x = 0;
  int date_y = 47;

  int sec_x = 42;
  int sec_y = 54;

  char time_str[1024];
  char sec_str[1024];
  char date_str[1024];

  int letter_spacing = 0;

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
}



  // -------------------------------------------------------------------------
  // Thread for the image generation on the display, displays the image on the
  // display portion.
  // -------------------------------------------------------------------------
void* imageThread(void *ptr){

  printf("Entered image thread");

  struct canvas_args *canvas_ptrs = (struct canvas_args*)ptr;
  RGBMatrix *canvas = canvas_ptrs->canvas;
  FrameCanvas *offscreen_canvas = canvas_ptrs->offscreen_canvas;

  Magick::InitializeMagick(NULL);

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
}


  // -------------------------------------------------------------------------
  // Thread for displaying data from Spotify music. Uses data from the FIFO
  // with the Python code to grab data from the API.
  // -------------------------------------------------------------------------
void* spotifyThread(void *ptr){
  printf("Entered spotify thread");

  struct canvas_args *canvas_ptrs = (struct canvas_args*)ptr;
  RGBMatrix *canvas = canvas_ptrs->canvas;
  FrameCanvas *offscreen_canvas = canvas_ptrs->offscreen_canvas;

  Magick::InitializeMagick(NULL);

  const int album_cover_x = 0;
  const int album_cover_y = 0;
  const int album_cover_size = 32;
  const int spotify_pauseplay_x = 44;
  const int spotify_pauseplay_y = 6;
  const int spotify_logo_x = 55;
  const int spotify_logo_y = 0;
  const int progress_bar_x = 36;
  const int progress_bar_y = 22;
  const int song_x_orig = 0;
  const int song_y_orig = 35;
  const int artist_x_orig = 34;
  const int artist_y_orig = 26;

  int song_x = song_x_orig;
  int song_y = song_y_orig;
  int artist_x = artist_x_orig;
  int arist_y = artist_y_orig;

  int song_len;
  int artist_len;

  int letter_spacing = 0;
  const int refresh_sleep = 1000000;

  float song_count = 0;
  float artist_count = 0;
  float usleep_count = 0;
  const int cmd_refresh = 3;

  rgb_matrix::Color progress_color(100, 100, 100);
  rgb_matrix::Color progress_left_color(211, 91, 68);
  rgb_matrix::Color progress_bar_color(200,200,200);
  rgb_matrix::Color bg_color(0, 0, 0);

  //char song_name[1024] = "Take It Easy";
  //char artist_name[1024] = "Eagles";

  ImageVector album_cover = LoadImageAndScaleImage(album_cover_filename,
                                                   album_cover_size,
                                                   album_cover_size);

  ImageVector spotify_logo = LoadImageAndScaleImage(spotify_icon_fn,
                                                    9, 9);
  ImageVector spotify_pause = LoadImageAndScaleImage(spotify_pause_fn,
                                                     9, 14);
  ImageVector spotify_play = LoadImageAndScaleImage(spotify_play_fn,
                                                     9, 14);

  std::string prev_song_name = "meep";
  std::string song_name;
  std::string artist_name;
  bool music_playing = true;
  int track_length = 0; // in seconds
  int track_progress = 0;
  bool track_progress_bar[24] = {false};


  char cmd_tx[64] = "spotify_curr_playing";
  char data_rx[1024];


  // BEFORE DRAWING TEXT
  while(!interrupt_received){

    // TWO COUNTS RUNNING: one for scrolling text (200ms)
    // one for sending API requests (3s)

    pthread_mutex_lock(&canvas_mutex);

    printf("SPOTIFY AT MUTEX\n");

    if((int)usleep_count > cmd_refresh) {

      // OPEN THE FIFOS
      cmd_fd = open("/home/justin/rpi-rgb-led-matrix/examples-api-use/cmd_cc_to_py",O_WRONLY|O_NONBLOCK);
      printf("Cmd fifo opened on C++\n");
      data_fd = open("/home/justin/rpi-rgb-led-matrix/examples-api-use/data_py_to_cc",O_RDONLY|O_NONBLOCK);
      printf("Data fifo opened on C++\n");

      printf("C++ sending command for song data\n");
      write(cmd_fd, cmd_tx, sizeof(cmd_tx));

      printf("Reading from data fifo\n");

      usleep(500*1000); // 50ms
      read(data_fd, data_rx, 1024);
      printf("Received %s", data_rx);


      // PARSE FIFO DATA
      char *token;
      // Split string into array of strings
      token = strtok(data_rx,",");

      for(int i = 0; i<5; i++){
        if(token != NULL) {
          std::string data(token);
          switch(i) {
            case 0:
              track_progress = stoi(data);
              track_progress /= 1000;  // ms to s
              break;
            case 1:
              track_length = stoi(data);
              track_length /= 1000;
              break;
            case 2:
              prev_song_name = song_name;
              song_name = data;
              break;
            case 3:
              artist_name = data;
              break;
            case 4:
              printf("Data = %s\n", data.c_str());
              if(data == "False") {
                music_playing = false;
              } else {
                music_playing = true;
              }
              printf("Music playing: %d\n", music_playing);
              break;
          }
          token = strtok(NULL,",");
        } else i=5;
      }


      // Check if new song, if so then update some things
      if(song_name != prev_song_name) {
        song_x = song_x_orig;
        artist_x = artist_x_orig;

        album_cover = LoadImageAndScaleImage(album_cover_filename,
                                             album_cover_size,
                                             album_cover_size);
        printf("IMAGE UPDATED\n");

      }

      // RESUME ITEMS SENSITIVE TO 3S COUNT
      if((int)usleep_count > cmd_refresh) {

      // Change pause or play depending on the state of music playing or not
      if(music_playing) {
        CopyImageToCanvas(spotify_pause[0], offscreen_canvas, &spotify_pauseplay_x, &spotify_pauseplay_y);
      } else {
        CopyImageToCanvas(spotify_play[0], offscreen_canvas, &spotify_pauseplay_x, &spotify_pauseplay_y);
      }

      // Check track progress and update if needed
      if(track_progress > track_length) {
        track_progress = 0;
        for(int i = 0; i<sizeof(track_progress_bar); i++) {
          track_progress_bar[i] = false;
        }
      } else {
        float track_segments = track_length/24;
        for(int i = 0; i<sizeof(track_progress_bar); i++) {
          if((float)track_progress > track_segments*i) {
            track_progress_bar[i] = true;
          } else {
            track_progress_bar[i] = false;
          }
        }
      }

      // Update display based upon track progress
      for(int i = 0; i<sizeof(track_progress_bar); i++) {
        if(track_progress_bar[i]) {
          // sets grey "played"
          canvas->SetPixel(progress_bar_x+i, progress_bar_y, progress_color.r,
                           progress_color.g, progress_color.b);
          canvas->SetPixel(progress_bar_x+i, progress_bar_y+1, progress_color.r,
                           progress_color.g, progress_color.b);
          canvas->SetPixel(progress_bar_x+i, progress_bar_y-1, 0, 0, 0);
          canvas->SetPixel(progress_bar_x+i, progress_bar_y+2, 0, 0, 0);

        } else if ((i!=0 && track_progress_bar[i-1]) || (i==0 && !track_progress_bar[0]) ) {
          // Sets pressent bar
          for(int x = 0; x<=1; x++) {
            for(int y = -1; y<=2; y++) {
              canvas->SetPixel((progress_bar_x+i+x), (progress_bar_y+y), progress_bar_color.r,
                                progress_bar_color.g, progress_bar_color.b);
            }
          }
          canvas->SetPixel(progress_bar_x+i-1, progress_bar_y-1, 0, 0, 0);
          canvas->SetPixel(progress_bar_x+i-1, progress_bar_y+2, 0, 0, 0);
          i++; // if not, pink overwrites the player bar

        } else {
          // sets pink "unplayed"
          canvas->SetPixel(progress_bar_x+i, progress_bar_y, progress_left_color.r,
                           progress_left_color.g, progress_left_color.b);
          canvas->SetPixel(progress_bar_x+i, progress_bar_y+1, progress_left_color.r,
                           progress_left_color.g, progress_left_color.b);
          canvas->SetPixel(progress_bar_x+i, progress_bar_y-1, 0, 0, 0);
          canvas->SetPixel(progress_bar_x+i, progress_bar_y+2, 0, 0, 0);

        }
      }

      // Clear the very last bar - incase the track progress bar hits the end
      if(!track_progress_bar[22]) {
        for(int i = -1; i<=2; i++) {
          canvas->SetPixel(progress_bar_x+24, progress_bar_y+i, 0, 0, 0);
        }
      }

      track_progress += cmd_refresh;
      usleep_count = 0;


    }
    }

    // Clear text before adding new text
    ClearCanvasArea(offscreen_canvas, song_x_orig, song_y_orig, 64, 7, &bg_color);

    // Artist and song name
    song_len = rgb_matrix::DrawText(offscreen_canvas, five_seven_font, song_x, song_y_orig + five_seven_font.baseline(),
                         progress_bar_color, outline_font ? NULL : &bg_color, song_name.c_str(), letter_spacing);

    ClearCanvasArea(offscreen_canvas, 0, artist_y_orig, 64/*artist_x_orig-1*/, 6, &bg_color);

    artist_len = rgb_matrix::DrawText(offscreen_canvas, date_font, artist_x, artist_y_orig + date_font.baseline(),
                         progress_bar_color, outline_font ? NULL : &bg_color, artist_name.c_str(),
                         letter_spacing);

    ClearCanvasArea(offscreen_canvas, artist_x_orig-2, artist_y_orig, 2, 6, &bg_color);

    // Album cover and spotify logo
    CopyImageToCanvas(album_cover[0], offscreen_canvas, &album_cover_x, &album_cover_y);
    CopyImageToCanvas(spotify_logo[0], offscreen_canvas, &spotify_logo_x, &spotify_logo_y);

    // Scrolls text across screen

    if(((int)song_count>10) && (song_len>64) && ((--song_x+song_len) < 0)) {
      song_x = song_x_orig;
      song_count = 0;
    }

    if(((int)artist_count>10) && (artist_len>64-artist_x_orig) && ((--artist_x+artist_len-artist_x_orig) < 0)) {
      artist_x = artist_x_orig;
      artist_count = 0;
    }


    canvas->SwapOnVSync(offscreen_canvas);
    pthread_mutex_unlock(&canvas_mutex);

    usleep(200*1000);
    usleep_count += 0.2;
    song_count += 0.2;
    artist_count += 0.2;
    //usleep(refresh_sleep*3);
    //track_progress += 3;

  }

  close(data_fd);
  close(cmd_fd);
  printf("Exited spotify thread");
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

  struct canvas_args canvas_ptrs;
  canvas_ptrs.canvas = canvas;
  canvas_ptrs.offscreen_canvas = offscreen_canvas;


  // INITIALIZE INTERRUPTS
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  // OPEN THE FIFOS
//  cmd_fd = open("/home/justin/rpi-rgb-led-matrix/examples-api-use/cmd_cc_to_py",O_WRONLY|O_NONBLOCK);
//  printf("Cmd fifo opened on C++\n");
//  data_fd = open("/home/justin/rpi-rgb-led-matrix/examples-api-use/data_py_to_cc",O_RDONLY|O_NONBLOCK);
//  printf("Data fifo opened on C++\n");

  // LOAD IMAGE
  images = LoadImageAndScaleImage(img_filename,
                                  64,
                                  screen_divider);


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
  const char *five_seven_ptr = NULL;
  const char *six_ten_ptr = NULL;
  int letter_spacing = 0;

  // Get font types
  char time_font_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/7x14B.bdf";
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

  char five_seven_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/5x7.bdf";
  five_seven_ptr = strdup(five_seven_filepath);
  if (date_font_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return 1;
  }

  char six_ten_filepath[] = "/home/justin/rpi-rgb-led-matrix/fonts/6x10.bdf";
  six_ten_ptr = strdup(six_ten_filepath);
  if (six_ten_ptr == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return 1;
  }

  // Load font. This needs to be a filename with a bdf bitmap font.
  if (!time_font.LoadFont(time_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", time_font_ptr);
    return 1;
  }

  if (!date_font.LoadFont(date_font_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", date_font_ptr);
    return 1;
  }

  if (!five_seven_font.LoadFont(five_seven_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", five_seven_ptr);
    return 1;
  }

  if (!six_ten_font.LoadFont(six_ten_ptr)) {
    fprintf(stderr, "Couldn't load font '%s'\n", six_ten_ptr);
    return 1;
  }

   // If we want an outline around the font, we create a new font with
   // the original font as a template that is just an outline font.
  if (with_outline) {
    outline_font = time_font.CreateOutlineFont();
  }


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

  if(pthread_create(&spotify_canvas_thr, NULL, spotifyThread, (void*)&canvas_ptrs) != 0) {
    printf("ERROR in spotify thread creation");
    return 2;
  }

  // JOIN THREADS TOGETHER
  // Join the threads to main thread -- don't execute cleanup code
  // until interrupt is received
  void *canvas_ret;
  void *image_ret;
  void *spotify_ret;

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
  if(pthread_join(spotify_canvas_thr, &spotify_ret) != 0) {
    printf("ERROR in spotify thread joining");
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
