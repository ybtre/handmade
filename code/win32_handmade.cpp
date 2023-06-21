
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO: this is a global for now
// static initialises everyuthing to 0 by default
global_variable bool running;

global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable HBITMAP bitmap_handle;
global_variable HDC bitmap_device_context; 


internal void
Win32ResizeDIBSection(int Width, int Height) // DIB = Device Independant Bitmap
{

    // TODO: bulletproof this,
    // Maybe dont free first, free after, then free first if that fails
    
    // TODO: free our DIBSection
    if(bitmap_handle)
    {
      DeleteObject(bitmap_handle);
    }

    if(!bitmap_device_context) 
    {
      //TODO: should we recreate these under certain special circumstances
      bitmap_device_context = CreateCompatibleDC(0);
    }

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = Width;
    bitmap_info.bmiHeader.biHeight = Height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    bitmap_handle = 
      CreateDIBSection(
        bitmap_device_context, &bitmap_info,
        DIB_RGB_COLORS,
        &bitmap_memory,
        0, 0);

}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int width, int height)
{
    StretchDIBits(DeviceContext,
                  X, Y, width, height,
                  X, Y, width, height,
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
      Win32UpdateWindow(device_context, X, Y, width, height);
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
    HWND window_handle =
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

    if(window_handle)
    {
      running = true;
      while(running)
      {
        MSG message;
        BOOL message_result = GetMessage(&message, 0, 0, 0);
        if(message_result > 0)
        {
          TranslateMessage(&message);

          DispatchMessageA(&message);
        }
        else
        {
          break;
        }
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
