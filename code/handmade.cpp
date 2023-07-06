

#include "handmade.h"

internal void
RenderWeirdGradient(game_offscreen_buffer *buffer, int blue_offset, int green_offset)
{
      //TODO: lets see waht the optimizer does when passed by value
    u8 *row = (u8 *)buffer->memory;
    for(int y = 0; y < buffer->height; ++y)
    {
        //uint32 *pixel = (uint32 *)row;
        u32 *pixel = (u32 *)row;

        for(int x = 0; x < buffer->width; ++x)
        {
            u8 blue = (x + blue_offset);
            u8 green = (y + green_offset);
            *pixel++ = ((green << 8) | blue);
        }

        row += buffer->pitch;
    }
}

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
