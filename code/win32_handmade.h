#include <dsound.h>
#include <memoryapi.h>
#include <stdio.h>
#include <windows.h>
#include <winnt.h>
#include <xinput.h>

#include "handmade.cpp"
#include "handmade.h"

#if !defined(WIN32_HANDMADE_H)

struct win32_offscreen_buffer {
  // NOTE(casey): Pixels are always 32-bits wide,
  // Memory Order  0x BB GG RR xx
  // Little Endian 0x xx RR GG BB
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int pitch;
};

struct win32_window_dimensions {
  int width;
  int height;
};

struct win32_sound_output {
  int samples_per_second;
  u32 running_sample_index;
  int bytes_per_sample;
  int secondary_buffer_size;
  f32 t_sine;
  int latency_sample_count;
};

#define WIN32_HANDMADE_H
#endif
