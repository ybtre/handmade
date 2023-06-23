
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

// TODO: this is a global for now
// static initialises everyuthing to 0 by default
global_variable bool running;

global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable int bitmap_width;
global_variable int bitmap_height;
global_variable int bytes_per_pixel = 4;



  internal void
RenderWeirdGradient(int x_offset, int y_offset)
{
  int Width = bitmap_width;
  int Height = bitmap_height;

  int pitch = Width * bytes_per_pixel; // pitch = offset to next row
  u8 *row = (u8 *)bitmap_memory;
  for(int y = 0; y < bitmap_height; ++y)
  {
    //uint32 *pixel = (uint32 *)row; 
    u32 *pixel = (u32 *)row; 
    for(int x = 0; x < bitmap_width; ++x)
    {
      u8 blue = (x + x_offset);
      u8 green = (y + y_offset); 

      *pixel++ = ((green << 8) | blue); 
    }

    row += pitch;
  }

}

  internal void
Win32ResizeDIBSection(int Width, int Height) // DIB = Device Independant Bitmap
{

  // TODO: bulletproof this,
  // Maybe dont free first, free after, then free first if that fails

  if(bitmap_memory)
  {
    VirtualFree(bitmap_memory, 0, MEM_RELEASE);
  }

  bitmap_width = Width;
  bitmap_height = Height;

  bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
  bitmap_info.bmiHeader.biWidth = bitmap_width;
  bitmap_info.bmiHeader.biHeight = -bitmap_height;
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = 32;
  bitmap_info.bmiHeader.biCompression = BI_RGB;

  //NOTE: thank you to chris hecker of spy party fame for clarying the deal with StretchDIBits and Bitblt
  int bitmap_memory_size = (Width*Height)*bytes_per_pixel;

  bitmap_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

  //TODO: probably clear this to black
}

  internal void
Win32UpdateWindow(HDC DeviceContext, RECT *window_rect, int X, int Y, int width, int height)
{
  int window_width = window_rect->right - window_rect->left;
  int window_height = window_rect->bottom - window_rect->top;
  StretchDIBits(DeviceContext,
      /*
         X, Y, width, height,
         X, Y, width, height,
         */
      0, 0, bitmap_width, bitmap_height,
      0, 0, window_width, window_height,
      bitmap_memory,
      &bitmap_info,
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
        RECT client_rect;
        GetClientRect(Window, &client_rect);
        int height = client_rect.bottom - client_rect.top;
        int width = client_rect.right - client_rect.left;
        Win32ResizeDIBSection(width, height);
      } break;

    case WM_DESTROY:
      {   
        //TODO handle this as an error - recreate window?
        running = false;
        OutputDebugStringA("WM_DESTROY\n");
      } break;

    case WM_CLOSE:
      {
        //TODO handle this with a message to the user?
        running = false;
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

        RECT client_rect;
        GetClientRect(Window, &client_rect);

        Win32UpdateWindow(device_context, &client_rect, X, Y, width, height);
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
  //TODO: Check if HREDRAW/VREDRWA/CSOWNDC are still needed
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
      running = true;

      int y_offset = 0;
      int x_offset = 0;

      while(running)
      {
        MSG message;
        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
          if(message.message == WM_QUIT)
          {
            running = false;
          }
          TranslateMessage(&message);
          DispatchMessageA(&message);
        }

        RenderWeirdGradient(x_offset, y_offset);

        HDC DeviceContext = GetDC(window);

        RECT client_rect;
        GetClientRect(window, &client_rect);
        int window_width = client_rect.right - client_rect.left;
        int window_height = client_rect.bottom - client_rect.top;
        Win32UpdateWindow(DeviceContext, &client_rect, 0, 0, window_width, window_height);
        ReleaseDC(window, DeviceContext);

        ++x_offset;
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
