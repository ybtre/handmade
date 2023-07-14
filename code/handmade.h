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

////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////
// UTIL FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
inline u32 SafeTruncateUInt64(u64 value) {
  // TODO: defines for maximum values UInt32Max
  Assert(value <= 0xFFFFFFFF);
  u32 result = (u32)value;

  return result;
}

// TODO(): Services that the platform layer provides to the game
struct DEBUG_read_file_result {
  u32 contents_size;
  void *contents;
};
// #if HANDMADE_INTERNAL
//  IMPORTANT:
//  These are NOT for doing anything in the shiping game - they are blocking and
//  the write doesnt protect against lost data!
//
internal void DEBUG_PlatformFreeFileMemory(void *memory);
internal DEBUG_read_file_result DEBUG_PlatformReadEntireFile(char *filename);

internal b32 DEBUG_PlatformWriteEntireFile(char *filename, u32 mem_size,
                                           void *memory);
// #endif

////////////////////////////////////////////////////////////////////////////////
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
  b32 is_connected;
  b32 is_analog;

  f32 stick_avg_X;
  f32 stick_avg_Y;

  union {
    game_button_state buttons[12];
    struct {
      game_button_state move_up;
      game_button_state move_down;
      game_button_state move_left;
      game_button_state move_right;

      game_button_state action_up;
      game_button_state action_down;
      game_button_state action_left;
      game_button_state action_right;

      game_button_state left_shoulder;
      game_button_state right_shoulder;

      game_button_state start;
      game_button_state back;
    };
  };
};

struct game_input {
  // TODO: insert clock value here
  game_controller_input controllers[5];
};
inline game_controller_input *GetController(game_input *input,
                                            int controller_index) {
  Assert(controller_index < ArrayCount(input->controllers));

  game_controller_input *result = &input->controllers[controller_index];

  return result;
}

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
