

#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *sound_buffer,
                              int tone_hz) {

  local_persist f32 t_sine;
  i16 tone_volume = 3000;
  int wave_period = sound_buffer->samples_per_second / tone_hz;

  i16 *sample_out = sound_buffer->samples;
  for (int sample_index = 0; sample_index < sound_buffer->sample_count;
       ++sample_index) {

    // Square wave: i16 sample_value = ((running_sample_index++ /
    // half_wave_period) % 2) ? tone_volume : -tone_volume;
    f32 sine_value = sinf(t_sine);
    i16 sample_value = (i16)(sine_value * tone_volume);

    *sample_out++ = sample_value;
    *sample_out++ = sample_value;

    t_sine += 2.0f * Pi32 * 1.0f / (f32)wave_period;
  }
}

internal void RenderWeirdGradient(game_offscreen_buffer *buffer,
                                  int blue_offset, int green_offset) {
  // TODO: lets see waht the optimizer does when passed by value
  u8 *row = (u8 *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y) {
    // uint32 *pixel = (uint32 *)row;
    u32 *pixel = (u32 *)row;

    for (int x = 0; x < buffer->width; ++x) {
      u8 blue = (u8)(x + blue_offset);
      u8 green = (u8)(y + green_offset);
      *pixel++ = ((green << 8) | blue);
    }

    row += buffer->pitch;
  }
}

internal void GameUpdateAndRender(game_memory *memory,
                                  game_offscreen_buffer *Buffer,
                                  game_sound_output_buffer *sound_buffer,
                                  game_input *input) {

  Assert(
      (&input->controllers[0].terminator - &input->controllers[0].buttons[0]) ==
      (ArrayCount(input->controllers[0].buttons)));
  Assert(sizeof(game_state) <= memory->permanent_storage_size);

  game_state *game_state = (struct game_state *)memory->permanent_storage;
  if (!memory->is_initialized) {

    char *filename = __FILE__;
    DEBUG_read_file_result file = DEBUG_PlatformReadEntireFile(filename);
    if (file.contents) {
      DEBUG_PlatformWriteEntireFile("test.out", file.contents_size,
                                    file.contents);
      DEBUG_PlatformFreeFileMemory(file.contents);
    };

    game_state->tone_hz = 256;

    // TODO: this may be more appropriate to do in the platform layer
    memory->is_initialized = true;
  }

  for (int controller_index = 0;
       controller_index < ArrayCount(input->controllers); ++controller_index) {

    game_controller_input *controller = GetController(input, controller_index);
    if (controller->is_analog) {
      // NOTE: use analog movement tuning;
      game_state->tone_hz = 256 + (int)(128.0f * (controller->stick_avg_X));
      game_state->blue_offset += (int)(4.0f * (controller->stick_avg_Y));
    } else {
      // NOTE: use digital movement tuning
      if (controller->move_left.ended_down) {
        game_state->blue_offset -= 1;
      }
      if (controller->move_right.ended_down) {
        game_state->blue_offset += 1;
      }
      if (controller->move_up.ended_down) {
        game_state->green_offset -= 1;
      }
      if (controller->move_down.ended_down) {
        game_state->green_offset += 1;
      }
    }
  }

  // TODO: allow sample offsets here for more robust platform options
  GameOutputSound(sound_buffer, game_state->tone_hz);

  RenderWeirdGradient(Buffer, game_state->blue_offset,
                      game_state->green_offset);
}
