Repo for my BitStream project - a retro dashboard that can display bits of information, such as the weather, clock, sports scores, stock prices, and images. 
This project is still currently in development, but feel free to use any of this code to help in any of your own personal projects.

This project is built ontop of hzeller's RGB Matrix library (https://github.com/hzeller/rpi-rgb-led-matrix/tree/master) that makes it easier to interface the RGB Matrix. Below are the most important files that I've written for the display - check them out! :)

src/

  bitstream_main.cc - main file that manages the control flow for the program
  
  fifo_requests.py - handles REST API requests, communicates with the C++ process using a named pipe

lib/

  canvas_gpio.cc - contains functions necessary for interfacing hardware components via GPIO
 
  display-classes.cc - holds class objects for display apps
 
  display-threads.cc - holds thread functions for display apps


Credit to hzeller for the Raspberry Pi RGB Matrix library ()
