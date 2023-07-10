

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
      u8 blue = (x + blue_offset);
      u8 green = (y + green_offset);
      *pixel++ = ((green << 8) | blue);
    }

    row += buffer->pitch;
  }
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer,
                                  game_sound_output_buffer *sound_buffer,
                                  game_input *input) {

  local_persist int BlueOffset{};
  local_persist int GreenOffset{};
  local_persist int tone_hz = 256 * 2;

  game_controller_input *input0 = &input->controllers[0];
  if (input0->is_analog) {
    // NOTE: use analog movement tuning;
    tone_hz = 256 + (int)(128.0f * (input0->end_X));
    BlueOffset += (int)4.0f * (input0->end_Y);
  } else {
    // NOTE: use digital movement tuning
  }

  if (input0->down.ended_down) {
    GreenOffset += 1;
  }

  // TODO: allow sample offsets here for more robust platform options
  GameOutputSound(sound_buffer, tone_hz);

  RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
