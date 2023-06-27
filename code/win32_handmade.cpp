
#include <windows.h>
#include <stdint.h>
#include <winuser.h>

#define internal static
#define local_persist static
#define global_variable static

// unsigned integers
typedef uint8_t u8;     // 1-byte long unsigned integer
typedef uint16_t u16;   // 2-byte long unsigned integer
typedef uint32_t u32;   // 4-byte long unsigned integer
typedef uint64_t u64;   // 8-byte long unsigned integer
                        // signed integers
typedef int8_t i8;      // 1-byte long signed integer
typedef int16_t i16;    // 2-byte long signed integer
typedef int32_t i32;    // 4-byte long signed integer
typedef int64_t i64;    // 8-byte long signed integer

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

// TODO: this is a global for now
// static initialises everyuthing to 0 by default
global_variable bool global_running;
global_variable win32_offscreen_buffer global_backbuffer;



win32_window_dimensions
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
RenderWeirdGradient(win32_offscreen_buffer buffer, int x_offset, int y_offset)
{
      //TODO: lets see waht the optimizer does when passed by value
      u8 *row = (u8 *)buffer.memory;
      for(int y = 0; y < buffer.height; ++y)
      {
            //uint32 *pixel = (uint32 *)row; 
            u32 *pixel = (u32 *)row; 
            for(int x = 0; x < buffer.width; ++x)
            {
                  u8 blue = (x + x_offset);
                  u8 green = (y + y_offset); 

                  *pixel++ = ((green << 8) | blue); 
            }

            row += buffer.pitch;
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

      buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

      //TODO: probably clear this to black
      
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, 
                           int window_width, int window_height,
                           win32_offscreen_buffer buffer,    
                           int X, int Y)
{
      //TODO: aspect ratio correction
      StretchDIBits(DeviceContext,
            /*
               X, Y, width, height,
               X, Y, width, height,
               */
            0, 0, window_width, window_height,
            0, 0, buffer.width, buffer.height,
            buffer.memory,
            &buffer.info,
            DIB_RGB_COLORS,
            SRCCOPY);
};


  LRESULT CALLBACK 
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

                  Win32DisplayBufferInWindow(device_context, dimension.width, dimension.height, global_backbuffer, X, Y);
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

  int CALLBACK 
WinMain(HINSTANCE Instance,
      HINSTANCE PrevInstance,
      LPSTR CommandLine,
      int ShowCode)
{
  
      WNDCLASSA window_class{};

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
                  global_running = true;

                  int y_offset = 0;
                  int x_offset = 0;

                  HDC DeviceContext = GetDC(window);

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

                        RenderWeirdGradient(global_backbuffer, x_offset, y_offset);


                        win32_window_dimensions dimensions = Win32GetWindowDimension(window);
                        Win32DisplayBufferInWindow(DeviceContext, dimensions.width, dimensions.height, global_backbuffer, 0, 0);
                        //ReleaseDC(window, DeviceContext);

                        ++x_offset;
                        ++y_offset;
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
