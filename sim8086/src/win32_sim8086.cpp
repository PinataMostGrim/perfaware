/* TODO (Aaron):
    - Adjust the per-frame sleep behaviour
    - Figure out WM_CLOSE and WM_QUIT
*/


#include <windows.h>
#include "base.h"


global B32 GlobalRunning;


LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        // case WM_QUIT: ?
        {

            GlobalRunning = FALSE;
            break;
        }
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
            break;
        }
    }

    return(Result);
}


function void
Win32ProcessPendingMessages()
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            // case WM_CLOSE: ?
            {
                GlobalRunning = FALSE;
                break;
            }
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
                break;
            }
        }
    }
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    // create window and register
    WNDCLASSA WindowClass = {};

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    // WindowClass.hIcon = ;
    WindowClass.lpszClassName = "win32sim8086";

    if (!RegisterClassA(&WindowClass))
    {
        // TODO (Aaron): Log error and exit
        exit(1);
    }

    HWND Window =
        CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "win32sim8086",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0);

    if (!Window)
    {
        // TODO (Aaron): Log error and exit
        exit(1);
    }

    GlobalRunning = TRUE;

    // program initialization


    // main loop
    while(GlobalRunning)
    {
        Win32ProcessPendingMessages();
        Sleep(20);
    }

    return 0;
}
