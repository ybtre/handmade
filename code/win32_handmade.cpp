
#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
#define global_variable static

// unsigned integers
typedef uint8_t  u8;    // 1-byte long unsigned integer
typedef uint16_t u16;   // 2-byte long unsigned integer
typedef uint32_t u32;   // 4-byte long unsigned integer
typedef uint64_t u64;   // 8-byte long unsigned integer

// signed integers
typedef int8_t  i8;     // 1-byte long signed integer
typedef int16_t i16;    // 2-byte long signed integer
typedef int32_t i32;    // 4-byte long signed integer
typedef int64_t i64;    // 8-byte long signed integer
typedef i32     b32;

struct win32_offscreen_buffer
{
    // NOTE(casey): Pixels are always 32-bits wide,
    // Memory Order  0x BB GG RR xx
    // Little Endian 0x xx RR GG BB
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct win32_window_dimensions
{
    int width;
    int height;
};

//NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//NOTE XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


// TODO: this is a global for now
// static initialises everyuthing to 0 by default
global_variable bool global_running;
global_variable win32_offscreen_buffer global_backbuffer;
global_variable LPDIRECTSOUNDBUFFER global_secondary_buffer;



internal void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
       //TODO: diagnostics
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {
            XInputGetState = XInputGetStateStub;
        }

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {
            XInputSetState = XInputSetStateStub;
        }
           //TODO: diagnostics
    }
    else
    {
        //TODO: diagnostics
    };
}

internal void
Win32InitDSound(HWND Window, i32 Buffer_Size, i32 Samples_Per_Second)
{
    //NOTE: load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if(DSoundLibrary)
    {
        //NOTE: get a DirectSound object - cooperative mode
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND direct_sound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0)))
        {
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = Samples_Per_Second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
            wave_format.cbSize = 0;

            if(SUCCEEDED(direct_sound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwSize = sizeof(buffer_description);
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

                //NOTE: "Create" a primary buffer
                LPDIRECTSOUNDBUFFER primary_buffer;
                if(SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0)))
                {
                    HRESULT error = primary_buffer->SetFormat(&wave_format);
                    if(SUCCEEDED(error))
                    {
                        //NOTE: finalyl set format for primary buffer
                        OutputDebugStringA("Primary buffer format was set. \n");
                    }
                    else
                    {
                        //TODO: diagnostic
                    };
                }
                else
                {
                    //TODO: diagnostics
                };
            }
            else
            {
                //TODO: diagnostics
            };

            //NOTE: "Create" a secondary buffer
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwSize = sizeof(buffer_description);
            buffer_description.dwFlags = 0;
            buffer_description.dwBufferBytes = Buffer_Size;
            buffer_description.lpwfxFormat = &wave_format;

            HRESULT error = direct_sound->CreateSoundBuffer(&buffer_description, &global_secondary_buffer, 0);
            if(SUCCEEDED(error))
            {
                OutputDebugStringA("Secondary buffer created successfuly.\n");
            }
            else
            {
                //TODO: diagnostics
            };
        }
        else
        {
            //TODO: diagnostics
        };

    }
}

internal win32_window_dimensions
Win32GetWindowDimension(HWND window)
{
    win32_window_dimensions result{};

    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.height = client_rect.bottom - client_rect.top;
    result.width = client_rect.right - client_rect.left;

    return result;
}

internal void
RenderWeirdGradient(win32_offscreen_buffer *buffer, int x_offset, int y_offset)
{
      //TODO: lets see waht the optimizer does when passed by value
    u8 *row = (u8 *)buffer->memory;
    for(int y = 0; y < buffer->height; ++y)
    {
        //uint32 *pixel = (uint32 *)row;
        u32 *pixel = (u32 *)row;

        for(int x = 0; x < buffer->width; ++x)
        {
            u8 blue = (x + x_offset);
            u8 green = (y + y_offset);
            *pixel++ = ((green << 8) | blue);
        }

        row += buffer->pitch;
    }
}



internal void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int Width, int Height) // DIB = Device Independant Bitmap
{

    // TODO: bulletproof this,
    // Maybe dont free first, free after, then free first if that fails
    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = Width;
    buffer->height = Height;
    int bytes_per_pixel = 4;

    buffer->pitch = buffer->width * bytes_per_pixel; // pitch = offset to next row
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // NOTE(yakvi): Remember to VirtualFree the memory if we ever
    buffer->info.bmiHeader.biPlanes = 1;              // call this function more than once on the same buffer!
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    //NOTE: thank you to chris hecker of spy party fame for clarying the deal with StretchDIBits and Bitblt
    int bitmap_memory_size = (buffer->width*buffer->height)*bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    //TODO: probably clear this to black
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *buffer,
                           HDC DeviceContext,
                           int window_width, int window_height)
{
    //TODO: aspect ratio correction
    StretchDIBits(DeviceContext,
                    /*
                    X, Y, width, height,
                    X, Y, width, height,
                    */
                    0, 0, window_width, window_height,
                    0, 0, buffer->width, buffer->height,
                    buffer->memory,
                    &buffer->info,
                    DIB_RGB_COLORS,
                    SRCCOPY);
};


internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
        } break;

        case WM_DESTROY:
        {
            //TODO handle this as an error - recreate window?
            global_running = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 VKCode = WParam;
            //NOTE: bitshift LParam by 30 is the previuos key state
            //bitshift LParam by 31 is the transition state
            //https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keyup
            bool was_down = ((LParam & (1 << 30)) != 0);
            bool is_down = ((LParam & (1 << 31)) == 0);
            if(was_down != is_down)
            {
                if(VKCode == 'W')
                {
                }
                else if(VKCode == 'A')
                {
                }
                else if(VKCode == 'S')
                {
                }
                else if(VKCode == 'D')
                {
                }
                else if(VKCode == 'Q')
                {
                }
                else if(VKCode == 'E')
                {
                }
                else if(VKCode == VK_UP)
                {
                }
                else if(VKCode == VK_LEFT)
                {
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_ESCAPE)
                {
                OutputDebugStringA("ESCAPE: ");
                if(is_down)
                {
                OutputDebugStringA("is Down ");
                }
                if(was_down)
                {
                OutputDebugStringA("was Down ");
                }
                OutputDebugStringA("\n");
                }
                else if(VKCode == VK_SPACE)
                {
                }
            }

            b32 alt_key_was_down = (LParam & (1 << 29));
            if(VKCode == VK_F4 && alt_key_was_down)
            {
                global_running = false;
            };

        }break;

        case WM_CLOSE:
        {
            //TODO handle this with a message to the user?
            global_running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(Window, &paint);
            int X = paint.rcPaint.left;
            int Y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            win32_window_dimensions dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&global_backbuffer, device_context, dimension.width, dimension.height);
            EndPaint(Window, &paint);
        }break;

        default:
        {
            //      OutputDebugStringA("default\n");
            result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return result;
}

internal int CALLBACK
WinMain(HINSTANCE Instance,
      HINSTANCE PrevInstance,
      LPSTR CommandLine,
      int ShowCode)
{
    WNDCLASSA window_class{};

    Win32LoadXInput();
    Win32ResizeDIBSection(&global_backbuffer, 1280, 720);

    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = Win32MainWindowCallback;
    window_class.hInstance = Instance;
    //  window_class.hIcon = ;
    window_class.lpszClassName = "HandmadeHeroWindowClass";

    if(RegisterClass(&window_class))
    {
        HWND window =
        CreateWindowEx(0,
                        window_class.lpszClassName,
                        "Handmade Hero",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        0,
                        0,
                        Instance,
                        0);
        if(window)
        {
            //NOTE: since we specified CS_OWNDC, we can just
            //get one device context and use it forever because
            //we are not sharing it with anyone
            HDC DeviceContext = GetDC(window);

            //NOTE: graphics test
            int y_offset = 0;
            int x_offset = 0;

            //NOTE: sound test
            int samples_per_second = 48000;
            int tone_Hz = 256;
            i16 tone_volume = 4000;
            u32 running_sample_index = 0;
            int square_wave_period = samples_per_second / tone_Hz;
            int half_square_wave_period = square_wave_period / 2;
            int bytes_per_sample = sizeof(i16) * 2;
            int secondary_buffer_size = samples_per_second * bytes_per_sample;

            Win32InitDSound(window, samples_per_second, secondary_buffer_size);
            
            global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);

            global_running = true;
            while(global_running)
            {
                MSG message;
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                    {
                          global_running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
                for(DWORD controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index)
                {
                    XINPUT_STATE controller_state{};
                    if(XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS)
                    {
                        //NOTE: this controller IS plugged in
                        //TODO: see if controller_sate.dwPacketNumber increments too rapidly
                        XINPUT_GAMEPAD *pad = &controller_state.Gamepad;

                        bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

                        bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);

                        bool left_shoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool right_shoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                        bool A_button = (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool B_button = (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool X_button = (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool Y_button = (pad->wButtons & XINPUT_GAMEPAD_Y);

                        i16 stick_X = pad->sThumbLX;
                        i16 stick_Y = pad->sThumbLY;
                        if(A_button)
                        {
                            ++y_offset;
                        }
                    }
                    else
                    {
                     //NOTE: this controlelr is not available
                    }
                }

                XINPUT_VIBRATION vibration;
                vibration.wLeftMotorSpeed = 60000;
                vibration.wRightMotorSpeed = 60000;
                XInputSetState(0, &vibration);

                RenderWeirdGradient(&global_backbuffer, x_offset, y_offset);

                //NOTE: DirectSound output test
                DWORD play_cursor;
                DWORD write_cursor;

                if(SUCCEEDED(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor)))
                {
                    DWORD byte_to_lock = running_sample_index * bytes_per_sample % secondary_buffer_size;
                    DWORD bytes_to_write;

                    if(byte_to_lock > play_cursor)
                    {
                        bytes_to_write = (secondary_buffer_size - byte_to_lock);
                        bytes_to_write += play_cursor;
                    }
                    else
                    {
                        bytes_to_write = play_cursor - byte_to_lock;
                    }
    
                    //TODO: more testing to make sure its okay
                    VOID *region1;
                    DWORD region1_size;
                    VOID *region2;
                    DWORD region2_size;

                    if(SUCCEEDED(global_secondary_buffer->Lock(byte_to_lock, bytes_to_write,
                                                                &region1, &region1_size,
                                                                &region2, &region2_size,
                                                                0)))
                    {
                        DWORD region1_sample_count = region1_size / bytes_per_sample;
                        i16 *sample_out = (i16 *)region1;

                        for(DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index)
                        {
                            i16 sample_value = ((running_sample_index++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;

                            *sample_out++ = sample_value;
                            *sample_out++ = sample_value;
                        }

                        DWORD region2_sample_count = region2_size / bytes_per_sample;
                        sample_out = (i16 *)region2;

                        for(DWORD sample_index = 0; sample_index < region2_sample_count; ++sample_index)
                        {
                            i16 sample_value = ((running_sample_index++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;

                            *sample_out++ = sample_value;
                            *sample_out++ = sample_value;
                        }

                        global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
                    }
                }

                win32_window_dimensions dimensions = Win32GetWindowDimension(window);
                Win32DisplayBufferInWindow(&global_backbuffer, DeviceContext, dimensions.width, dimensions.height);

                ++x_offset;
                //++y_offset;
            }
        }
        else
        {
            //TODO: logging
        }
    }
    else
    {
        //TODO: logging
    }
    return(0);
};
