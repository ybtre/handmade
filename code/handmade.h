#if !defined(HANDMADE_H)

// NOTE:
// HANDMADE_INTERNAL:
// 0 - Build for public release
// 1 - Build for developer only
//
// HANDMADE_SLOW:
// 0 - No slow code allowed!
// 1 - Slow code welcome
//

#if HANDMADE_SLOW
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#else
#define Assert(Expression)
#endif

// TODO: should these always be 64bit
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// TODO: swap, min, max ... macros?

// TODO(): Services that the platform layer provides to the game

// NOTE(casey): Services that the game provides to the platform layer.
//(this may expand in the future - sound on separate thread, etc.)

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound
// buffer to use

// TODO(casey): In the future, rendering _specifically_ will become a
// three-tiered abstraction!!!
//

// NOTE: typedefs and #defines so that clangd can stop complaining:)) //////////
#include <stdint.h>
// TODO: implement sine ourselves
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

// unsigned integers
typedef uint8_t u8;   // 1-byte long unsigned integer
typedef uint16_t u16; // 2-byte long unsigned integer
typedef uint32_t u32; // 4-byte long unsigned integer
typedef uint64_t u64; // 8-byte long unsigned integer

// signed integers
typedef int8_t i8;   // 1-byte long signed integer
typedef int16_t i16; // 2-byte long signed integer
typedef int32_t i32; // 4-byte long signed integer
typedef int64_t i64; // 8-byte long signed integer
typedef i32 b32;

typedef float f32;
typedef double f64;
////////////////////////////////////////////////////////////////////////////////

struct game_offscreen_buffer {
  // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
  void *memory;
  int width;
  int height;
  int pitch;
};

struct game_sound_output_buffer {
  int samples_per_second;
  int sample_count;
  i16 *samples;
};

struct game_button_state {
  int half_transition_count;
  b32 ended_down;
};

struct game_controller_input {
  b32 is_analog;

  f32 start_X;
  f32 start_Y;

  f32 min_X;
  f32 min_Y;

  f32 max_X;
  f32 max_Y;

  f32 end_X;
  f32 end_Y;

  union {
    game_button_state buttons[6];
    struct {
      game_button_state up;
      game_button_state down;
      game_button_state left;
      game_button_state right;
      game_button_state left_shoulder;
      game_button_state right_shoulder;
    };
  };
};

struct game_input {
  // TODO: insert clock value here
  game_controller_input controllers[4];
};

struct game_memory {
  b32 is_initialized;

  u64 permanent_storage_size;
  void *permanent_storage; // NOTE: REQUIRED to be cleared to zero at startup

  u64 transient_storage_size;
  void *transient_storage; // NOTE: REQUIRED to be cleared to zero at startup
};

internal void GameUpdateAndRender(game_memory *memory,
                                  game_offscreen_buffer *Buffer,
                                  game_sound_output_buffer *sound_buffer,
                                  game_input *input);
//
//
//

struct game_state {
  int tone_hz;
  int green_offset;
  int blue_offset;
};

#define HANDMADE_H
#endif
