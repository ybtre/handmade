
#include <windows.h>
#include <winuser.h>

LRESULT CALLBACK 
MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
  LRESULT result = 0;

  switch(Message)
  {
    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
    } break;

    case WM_DESTROY:
    {
      OutputDebugStringA("WM_DESTROY\n");
    } break;

    case WM_CLOSE:
    {
      PostQuitMessage(0);
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
      PatBlt(device_context, X, Y, width, height, BLACKNESS);

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
  window_class.lpfnWndProc = MainWindowCallback;
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
      MSG message;
      for(;;)
      {
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
