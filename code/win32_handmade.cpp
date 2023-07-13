
//
// TODO: THIS IS NOT A FINAL PLATFORM LAYER!!
//
// - saved game location
// - getting a handle to our own .exe
// - asset lodaing path
// - threading (launch a thread)
// - raw input(support for multiple keyboards)
// - sleep/time_begin_period
// - ClipCursor() (for multimonitor support)
// - fullscreen support
// - WM_SETCURSOR (control cursot visibility)
// - QueryCancleAutoplay
// - WM_ACTIVATEAPP (for when we are not the active app)
// - Blit speeed improvements (BitBlt)
// - Hadware acceleration (Opengl or Direct3D or both)
// - GetKeyboardLayout (for french keyboards, international support)
//
// Just a partial list of stuff
//
//
// #include "handmade.cpp"
// #include "handmade.h"
#include "win32_handmade.h"

// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE XInputSetState
#define X_INPUT_SET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name)                                              \
  HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS,               \
                      LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// TODO: this is a global for now
// static /initialises everyuthing to 0 by default
global_variable b32 global_running;
global_variable win32_offscreen_buffer global_backbuffer;
global_variable LPDIRECTSOUNDBUFFER global_secondary_buffer;

internal void DEBUG_PlatformFreeFileMemory(void *memory) {

  if (memory) {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
};

internal debug_read_file_result DEBUG_PlatformReadEntireFile(char *filename) {

  debug_read_file_result result{};

  HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                   OPEN_EXISTING, 0, 0);

  if (file_handle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER file_size;
    if (GetFileSizeEx(file_handle, &file_size)) {
      u32 file_size32 = SafeTruncateUInt64(file_size.QuadPart);
      result.contents = VirtualAlloc(0, file_size32, MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
      if (result.contents) {
        DWORD bytes_read;
        if (ReadFile(file_handle, result.contents, file_size32, &bytes_read,
                     0) &&
            (file_size32 == bytes_read)) {

          // NOTE: file read successfully
          result.contents_size = file_size32;
        } else {
          // TODO: logging
          DEBUG_PlatformFreeFileMemory(result.contents);
          result.contents = 0;
        }
      } else {
        // TODO: logging
      }
    }

    CloseHandle(file_handle);
  } else {
    // TODO: logging
  }

  return result;
};

internal b32 DEBUG_PlatformWriteEntireFile(char *filename, u32 mem_size,
                                           void *memory) {

  b32 result = false;

  HANDLE file_handle =
      CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  if (file_handle != INVALID_HANDLE_VALUE) {
    DWORD bytes_written;
    if (WriteFile(file_handle, memory, mem_size, &bytes_written, 0)) {
      // NOTE: file read successfully
      result = (bytes_written == mem_size);
    } else {
      // TODO: logging
    }

    CloseHandle(file_handle);
  } else {
    // TODO: logging
  }

  return result;
};

internal void Win32LoadXInput(void) {
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!XInputLibrary) {
    // TODO: diagnostics
    XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
  }

  if (!XInputLibrary) {
    // TODO: diagnostics
    XInputLibrary = LoadLibraryA("xinput1_3.dll");
  }

  if (XInputLibrary) {
    XInputGetState =
        (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    if (!XInputGetState) {
      XInputGetState = XInputGetStateStub;
    }

    XInputSetState =
        (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    if (!XInputSetState) {
      XInputSetState = XInputSetStateStub;
    }
    // TODO: diagnostics
  } else {
    // TODO: diagnostics
  };
}

internal void Win32InitDSound(HWND Window, i32 Buffer_Size,
                              i32 Samples_Per_Second) {
  // NOTE: load the library
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

  if (DSoundLibrary) {
    // NOTE: get a DirectSound object - cooperative mode
    direct_sound_create *DirectSoundCreate =
        (direct_sound_create *)GetProcAddress(DSoundLibrary,
                                              "DirectSoundCreate");

    LPDIRECTSOUND direct_sound;
    if (DirectSoundCreate &&
        SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0))) {
      WAVEFORMATEX wave_format = {};
      wave_format.wFormatTag = WAVE_FORMAT_PCM;
      wave_format.nChannels = 2;
      wave_format.nSamplesPerSec = Samples_Per_Second;
      wave_format.wBitsPerSample = 16;
      wave_format.nBlockAlign =
          (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
      wave_format.nAvgBytesPerSec =
          wave_format.nSamplesPerSec * wave_format.nBlockAlign;
      wave_format.cbSize = 0;

      if (SUCCEEDED(
              direct_sound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
        DSBUFFERDESC buffer_description = {};
        buffer_description.dwSize = sizeof(buffer_description);
        buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

        // NOTE: "Create" a primary buffer
        LPDIRECTSOUNDBUFFER primary_buffer;
        if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description,
                                                      &primary_buffer, 0))) {
          HRESULT error = primary_buffer->SetFormat(&wave_format);
          if (SUCCEEDED(error)) {
            // NOTE: finalyl set format for primary buffer
            OutputDebugStringA("Primary buffer format was set. \n");
          } else {
            // TODO: diagnostic
          };
        } else {
          // TODO: diagnostics
        };
      } else {
        // TODO: diagnostics
      };

      // NOTE: "Create" a secondary buffer
      DSBUFFERDESC buffer_description = {};
      buffer_description.dwSize = sizeof(buffer_description);
      buffer_description.dwFlags = 0;
      buffer_description.dwBufferBytes = Buffer_Size;
      buffer_description.lpwfxFormat = &wave_format;

      HRESULT error = direct_sound->CreateSoundBuffer(
          &buffer_description, &global_secondary_buffer, 0);
      if (SUCCEEDED(error)) {
        OutputDebugStringA("Secondary buffer created successfuly.\n");
      } else {
        // TODO: diagnostics
      };
    } else {
      // TODO: diagnostics
    };
  }
}

internal win32_window_dimensions Win32GetWindowDimension(HWND window) {
  win32_window_dimensions result{};

  RECT client_rect;
  GetClientRect(window, &client_rect);
  result.height = client_rect.bottom - client_rect.top;
  result.width = client_rect.right - client_rect.left;

  return result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int Width,
                      int Height) // DIB = Device Independant Bitmap
{

  // TODO: bulletproof this,
  // Maybe dont free first, free after, then free first if that fails
  if (buffer->memory) {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = Width;
  buffer->height = Height;
  int bytes_per_pixel = 4;

  buffer->pitch = buffer->width * bytes_per_pixel; // pitch = offset to next row
  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight =
      -buffer->height; // NOTE(yakvi): Remember to VirtualFree the memory if we
                       // ever
  buffer->info.bmiHeader.biPlanes =
      1; // call this function more than once on the same buffer!
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;

  // NOTE: thank you to chris hecker of spy party fame for clarying the deal
  // with StretchDIBits and Bitblt
  int bitmap_memory_size = (buffer->width * buffer->height) * bytes_per_pixel;
  buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);

  // TODO: probably clear this to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *buffer,
                                         HDC DeviceContext, int window_width,
                                         int window_height) {
  // TODO: aspect ratio correction
  StretchDIBits(DeviceContext,
                /*
                X, Y, width, height,
                X, Y, width, height,
                */
                0, 0, window_width, window_height, 0, 0, buffer->width,
                buffer->height, buffer->memory, &buffer->info, DIB_RGB_COLORS,
                SRCCOPY);
};

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                                  WPARAM WParam,
                                                  LPARAM LParam) {
  LRESULT result = 0;

  switch (Message) {
  case WM_SIZE: {
  } break;

  case WM_DESTROY: {
    // TODO handle this as an error - recreate window?
    global_running = false;
    OutputDebugStringA("WM_DESTROY\n");
  } break;

  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
    u32 VKCode = WParam;
    // NOTE: bitshift LParam by 30 is the previuos key state
    // bitshift LParam by 31 is the transition state
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keyup
    b32 was_down = ((LParam & (1 << 30)) != 0);
    b32 is_down = ((LParam & (1 << 31)) == 0);
    if (was_down != is_down) {
      if (VKCode == 'W') {
      } else if (VKCode == 'A') {
      } else if (VKCode == 'S') {
      } else if (VKCode == 'D') {
      } else if (VKCode == 'Q') {
      } else if (VKCode == 'E') {
      } else if (VKCode == VK_UP) {
      } else if (VKCode == VK_LEFT) {
      } else if (VKCode == VK_DOWN) {
      } else if (VKCode == VK_RIGHT) {
      } else if (VKCode == VK_ESCAPE) {
        OutputDebugStringA("ESCAPE: ");
        if (is_down) {
          OutputDebugStringA("is Down ");
        }
        if (was_down) {
          OutputDebugStringA("was Down ");
        }
        OutputDebugStringA("\n");
      } else if (VKCode == VK_SPACE) {
      }
    }

    b32 alt_key_was_down = (LParam & (1 << 29));
    if (VKCode == VK_F4 && alt_key_was_down) {
      global_running = false;
    };

  } break;

  case WM_CLOSE: {
    // TODO handle this with a message to the user?
    global_running = false;
    OutputDebugStringA("WM_CLOSE\n");
  } break;

  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;

  case WM_PAINT: {
    PAINTSTRUCT paint;
    HDC device_context = BeginPaint(Window, &paint);
    int X = paint.rcPaint.left;
    int Y = paint.rcPaint.top;
    int height = paint.rcPaint.bottom - paint.rcPaint.top;
    int width = paint.rcPaint.right - paint.rcPaint.left;
    win32_window_dimensions dimension = Win32GetWindowDimension(Window);
    Win32DisplayBufferInWindow(&global_backbuffer, device_context,
                               dimension.width, dimension.height);
    EndPaint(Window, &paint);
  } break;

  default: {
    //      OutputDebugStringA("default\n");
    result = DefWindowProc(Window, Message, WParam, LParam);
  } break;
  }

  return result;
}

internal void Win32_ClearBuffer(win32_sound_output *sound_output) {

  VOID *region1{};
  DWORD region1_size{};
  VOID *region2{};
  DWORD region2_size{};
  if (SUCCEEDED(global_secondary_buffer->Lock(
          0, sound_output->secondary_buffer_size, &region1, &region1_size,
          &region2, &region2_size, 0))) {

    u8 *dest_sample = (u8 *)region1;
    for (DWORD byte_index = 0; byte_index < region1_size; ++byte_index) {

      *dest_sample++ = 0;
    }

    dest_sample = (u8 *)region2;
    for (DWORD byte_index = 0; byte_index < region2_size; ++byte_index) {

      *dest_sample++ = 0;
    }

    global_secondary_buffer->Unlock(region1, region1_size, region2,
                                    region2_size);
  }
}

internal void Win32_FillSoundBuffer(win32_sound_output *sound_output,
                                    DWORD byte_to_lock, DWORD bytes_to_write,
                                    game_sound_output_buffer *source_buffer) {
  // TODO: more testing to make sure its okay
  VOID *region1{};
  DWORD region1_size{};
  VOID *region2{};
  DWORD region2_size{};

  HRESULT err =
      global_secondary_buffer->Lock(byte_to_lock, bytes_to_write, &region1,
                                    &region1_size, &region2, &region2_size, 0);
  if (SUCCEEDED(err)) {
    DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;

    i16 *dest_sample = (i16 *)region1;
    i16 *source_sample = source_buffer->samples;

    for (DWORD sample_index = 0; sample_index < region1_sample_count;
         ++sample_index) {

      *dest_sample++ = *source_sample++;
      *dest_sample++ = *source_sample++;

      ++sound_output->running_sample_index;
    }

    DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
    dest_sample = (i16 *)region2;
    for (DWORD sample_index = 0; sample_index < region2_sample_count;
         ++sample_index) {

      *dest_sample++ = *source_sample++;
      *dest_sample++ = *source_sample++;

      ++sound_output->running_sample_index;
    }

    global_secondary_buffer->Unlock(region1, region1_size, region2,
                                    region2_size);
  }
};

internal void Win32_ProcessXInputDigitalButton(DWORD xinput_button_state,
                                               game_button_state *old_sate,
                                               DWORD button_bit,
                                               game_button_state *new_state) {

  new_state->ended_down = ((xinput_button_state & button_bit) == button_bit);
  new_state->half_transition_count =
      (old_sate->ended_down |= new_state->ended_down) ? 1 : 0;
};

internal int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                              LPSTR CommandLine, int ShowCode) {
  LARGE_INTEGER perf_counter_freq_result{};
  QueryPerformanceFrequency(&perf_counter_freq_result);
  i64 perf_count_freq = perf_counter_freq_result.QuadPart;

  WNDCLASSA window_class{};

  Win32LoadXInput();
  Win32ResizeDIBSection(&global_backbuffer, 1280, 720);

  window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  window_class.lpfnWndProc = Win32MainWindowCallback;
  window_class.hInstance = Instance;
  //  window_class.hIcon = ;
  window_class.lpszClassName = "HandmadeHeroWindowClass";

  if (RegisterClass(&window_class)) {
    HWND window = CreateWindowEx(0, window_class.lpszClassName, "Handmade Hero",
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, 0, 0, Instance, 0);
    if (window) {
      // NOTE: since we specified CS_OWNDC, we can just
      // get one device context and use it forever because
      // we are not sharing it with anyone
      HDC DeviceContext = GetDC(window);

      // NOTE: graphics test
      int y_offset = 0;
      int x_offset = 0;

      win32_sound_output sound_output{};

      sound_output.samples_per_second = 48000;
      sound_output.running_sample_index = 0;
      sound_output.bytes_per_sample = sizeof(int16_t) * 2;
      sound_output.secondary_buffer_size =
          sound_output.samples_per_second * sound_output.bytes_per_sample;
      sound_output.latency_sample_count =
          sound_output.samples_per_second / 15; // 15fps cutoff point

      Win32InitDSound(window, sound_output.secondary_buffer_size,
                      sound_output.samples_per_second);
      Win32_ClearBuffer(&sound_output);
      global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);

      i16 *samples = (i16 *)VirtualAlloc(
          0, 48000 * 2 * sizeof(i16), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
      LPVOID base_address = (LPVOID)Terabytes(2);
#else
      LPVOID base_address = 0;
#endif

      // NOTE: VirtualAlloc always initialises the memory to 0
      game_memory game_memory{};
      game_memory.permanent_storage_size = Megabytes(64);
      game_memory.transient_storage_size = Gigabytes(4);

      u64 total_size = game_memory.permanent_storage_size +
                       game_memory.transient_storage_size;
      game_memory.permanent_storage = VirtualAlloc(
          base_address, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
      game_memory.transient_storage = ((u8 *)game_memory.permanent_storage +
                                       game_memory.permanent_storage_size);

      if (samples && game_memory.permanent_storage &&
          game_memory.transient_storage) {

        game_input input[2]{};
        game_input *old_input = &input[0];
        game_input *new_input = &input[1];

        LARGE_INTEGER last_counter{};
        QueryPerformanceCounter(&last_counter);
        u64 last_cycle_count = __rdtsc();

        global_running = true;
        while (global_running) {

          MSG message;

          while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
              global_running = false;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
          }

          int max_controller_count = XUSER_MAX_COUNT;
          if (max_controller_count > ArrayCount(new_input->controllers)) {

            max_controller_count = ArrayCount(new_input->controllers);
          }

          for (DWORD controller_index = 0;
               controller_index < max_controller_count; ++controller_index) {

            game_controller_input *old_controller =
                &old_input->controllers[controller_index];
            game_controller_input *new_controller =
                &new_input->controllers[controller_index];

            XINPUT_STATE controller_state{};
            if (XInputGetState(controller_index, &controller_state) ==
                ERROR_SUCCESS) {
              // NOTE: this controller IS plugged in
              // TODO: see if controller_sate.dwPacketNumber increments too
              // rapidly
              XINPUT_GAMEPAD *pad = &controller_state.Gamepad;

              // TODO: dpad
              b32 up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
              b32 down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
              b32 left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
              b32 right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

              new_controller->is_analog = true;
              new_controller->start_X = old_controller->end_X;
              new_controller->start_Y = old_controller->end_Y;

              // TODO: Min/Max macros!!
              // TODO: collapse to single function
              f32 X;
              if (pad->sThumbLX < 0) {
                X = (f32)pad->sThumbLX / 32768.0f;
              } else {
                X = (f32)pad->sThumbLX / 32767.0f;
              }

              f32 Y;
              if (pad->sThumbLY < 0) {
                Y = (f32)pad->sThumbLY / 32768.0f;
              } else {
                Y = (f32)pad->sThumbLY / 32767.0f;
              }
              new_controller->min_X = new_controller->max_X =
                  new_controller->end_X = X;
              new_controller->min_Y = new_controller->max_Y =
                  new_controller->end_Y = Y;

              // b32 start = (pad->wButtons & XINPUT_GAMEPAD_START);
              // b32 back = (pad->wButtons & XINPUT_GAMEPAD_BACK);

              // TODO: we will do deadzone handling later using
              // XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
              // XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE

              Win32_ProcessXInputDigitalButton(
                  pad->wButtons, &old_controller->down, XINPUT_GAMEPAD_A,
                  &new_controller->down);
              Win32_ProcessXInputDigitalButton(
                  pad->wButtons, &old_controller->right, XINPUT_GAMEPAD_B,
                  &new_controller->right);
              Win32_ProcessXInputDigitalButton(
                  pad->wButtons, &old_controller->left, XINPUT_GAMEPAD_X,
                  &new_controller->left);
              Win32_ProcessXInputDigitalButton(
                  pad->wButtons, &old_controller->up, XINPUT_GAMEPAD_Y,
                  &new_controller->up);

              Win32_ProcessXInputDigitalButton(
                  pad->wButtons, &old_controller->left_shoulder,
                  XINPUT_GAMEPAD_LEFT_SHOULDER, &new_controller->left_shoulder);
              Win32_ProcessXInputDigitalButton(pad->wButtons,
                                               &old_controller->right_shoulder,
                                               XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                               &new_controller->right_shoulder);

            } else {
              // NOTE: this controlelr is not available
            }
          }

          XINPUT_VIBRATION vibration;
          vibration.wLeftMotorSpeed = 60000;
          vibration.wRightMotorSpeed = 60000;
          XInputSetState(0, &vibration);

          DWORD play_cursor{};
          DWORD write_cursor{};
          DWORD byte_to_lock{};
          DWORD bytes_to_write{};
          DWORD target_cursor{};
          b32 sound_is_valid = false;
          // TODO: tighten up sound logic so that we know where we should be
          // writing to and can anticipate the time spent in the game update
          if (SUCCEEDED(global_secondary_buffer->GetCurrentPosition(
                  &play_cursor, &write_cursor))) {

            byte_to_lock = ((sound_output.running_sample_index *
                             sound_output.bytes_per_sample) %
                            sound_output.secondary_buffer_size);
            target_cursor = ((play_cursor + (sound_output.latency_sample_count *
                                             sound_output.bytes_per_sample)) %
                             sound_output.secondary_buffer_size);

            if (byte_to_lock > target_cursor) {
              bytes_to_write =
                  (sound_output.secondary_buffer_size - byte_to_lock);

              bytes_to_write += target_cursor;
            } else {

              bytes_to_write = target_cursor - byte_to_lock;
            }

            sound_is_valid = true;
          }

          game_sound_output_buffer sound_buffer{};
          sound_buffer.samples_per_second = sound_output.samples_per_second;
          sound_buffer.sample_count =
              bytes_to_write / sound_output.bytes_per_sample;
          sound_buffer.samples = samples;

          game_offscreen_buffer buffer{};
          buffer.memory = global_backbuffer.memory;
          buffer.width = global_backbuffer.width;
          buffer.height = global_backbuffer.height;
          buffer.pitch = global_backbuffer.pitch;

          GameUpdateAndRender(&game_memory, &buffer, &sound_buffer, new_input);

          if (sound_is_valid) {

            Win32_FillSoundBuffer(&sound_output, byte_to_lock, bytes_to_write,
                                  &sound_buffer);
          }

          win32_window_dimensions dimensions = Win32GetWindowDimension(window);
          Win32DisplayBufferInWindow(&global_backbuffer, DeviceContext,
                                     dimensions.width, dimensions.height);

          ++x_offset;
          //++y_offset;

          u64 end_cycle_count = __rdtsc();

          LARGE_INTEGER end_counter{};
          QueryPerformanceCounter(&end_counter);

          // TODO: dispaly counter value
          u64 cycles_elapsed = end_cycle_count - last_cycle_count;
          i64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
          i32 ms_per_frame = (i32)((1000 * counter_elapsed) /
                                   perf_count_freq); // NOTE: x1000 to go from
                                                     // seconds to miliseconds
          i32 fps = perf_count_freq / counter_elapsed;
          i32 mcpf = (i32)(cycles_elapsed /
                           (1000 * 1000)); // NOTE: / (1000 * 1000) - get
                                           // mega cycles (mghz kinda)

#if 0
                char buffer[256];
                wsprintfA(buffer, "%dms/f, %df/s, %dmc/f \n", ms_per_frame, fps, mcpf);      
                OutputDebugStringA(buffer);
#endif

          last_cycle_count = end_cycle_count;
          last_counter = end_counter;

          game_input *temp = new_input;
          new_input = old_input;
          old_input = temp;
          // TODO: should i clear these here?
        }
      } else {
        // TODO: logging
      }
    } else {
      // TODO: logging
    }
  } else {
    // TODO: logging
  }
  return (0);
};
