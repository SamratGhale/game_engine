#include <windows.h>
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static 

//unsigned integers
typedef uint8_t  u8;  //1-byte
typedef uint16_t u16; //2-byte
typedef uint32_t u32; //4-byte
typedef uint64_t u64; //8-byte

//signed integers
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;


global_variable bool       Running;
global_variable BITMAPINFO BitmapInfo;
global_variable int        BitmapWidth;
global_variable int        BitmapHeight;
global_variable void *     BitmapMemory; //The mem
global_variable int        BytesPerPixel; //The mem

/**
 * This function creates a new BitmapMemory on every call,
 * We need height and weight to allocate the memory by and to put it into the bitmapinfo 
 * global_variables BitmapWidth and BitmapHeight is also set here
 */

internal void RenderWeiredGradient(int XOffset, int YOffset){
  const int Pitch = BitmapWidth * BytesPerPixel; //This is a constant variable which's size is equal to the total byes in a row

  u8 * Row = (u8 *)BitmapMemory; //u8 so we can multiply with pitch(which unit is in bytes) and get into the next row

  for(int x = 0; x < BitmapHeight; ++x){
    u32 *Pixel = (u32*)Row; //First pixel of the row

    for(int y = 0; y < BitmapWidth; ++y){
      u8 Red   = 0;
      u8 Green = (u8)(y + YOffset);
      u8 Blue  = (u8)(x + XOffset);

      *Pixel++ = Red << 16 | Green << 8 | Blue;  // PP RRi GG BB
    }

    Row += Pitch;
  }
  
}

internal void
Win32ResizeDIBSection(int Width, int Height){

  if(BitmapMemory){
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }
  //Initilization
  BitmapWidth  = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize        = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biCompression = BI_RGB;
  BitmapInfo.bmiHeader.biPlanes      = 1;
  BitmapInfo.bmiHeader.biBitCount    = 32;
  BitmapInfo.bmiHeader.biWidth       = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight      = -BitmapHeight; //Top-down

  BytesPerPixel = 4;
  int BitmapMemorySize = BytesPerPixel * (BitmapWidth * BitmapHeight);
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

/**
 * Rectange to Rectange copy
 */
internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect){
  int WindowWidth = ClientRect->right - ClientRect->left;
  int WindowHeight = ClientRect->bottom - ClientRect->top;
  StretchDIBits( DeviceContext,
		 0, 0, WindowWidth, WindowHeight, 
		 0, 0, BitmapWidth, BitmapHeight,
		 BitmapMemory, &BitmapInfo,
		 DIB_RGB_COLORS, SRCCOPY );
}


LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
			UINT Message,
			WPARAM WParam,
			LPARAM LParam
			)
{
  LRESULT Result = 0;
  switch(Message)
    {
    case WM_SIZE:
      {
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	int Width  = ClientRect.right - ClientRect.left;
	int Height = ClientRect.bottom - ClientRect.top;
	Win32ResizeDIBSection(Width, Height);
      }break;
    case WM_DESTROY:
      {
	Running = false;
      }break;
    case WM_CLOSE:
      {
	Running = false;
      }break;
    case WM_ACTIVATEAPP:
      {
      }break;
    case WM_PAINT:
      {
	PAINTSTRUCT Paint;
	HDC DeviceContext = BeginPaint(Window, &Paint);

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

	Win32UpdateWindow(DeviceContext, &ClientRect);

	EndPaint(Window, &Paint);
      }break;
    default:
      {
	OutputDebugStringA("Default\n");
	Result = DefWindowProc(Window, Message, WParam, LParam);
      }break;
    }
  return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
  WNDCLASS WindowClass = {};
  WindowClass.style  = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClass(&WindowClass)){
    HWND WindowHandle = CreateWindowEx(
				       0,
				       WindowClass.lpszClassName,
				       "Handmade Hero",
				       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				       CW_USEDEFAULT,CW_USEDEFAULT,
				       CW_USEDEFAULT,CW_USEDEFAULT,
				       0, 0, Instance, 0);
    if(WindowHandle){
      int XOffset = 0;
      int YOffset = 0;
      Running = true;
      while(Running){

	MSG Message;
	while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)){

	  //Bulletproof
	  if(Message.message == WM_QUIT){
	    Running = false;
	  }
	  
	  TranslateMessage(&Message);
	  DispatchMessage(&Message);
	}
	RenderWeiredGradient(XOffset, YOffset);
	++XOffset;

	HDC DeviceContext = GetDC(WindowHandle);
	RECT ClientRect;

	GetClientRect(WindowHandle, &ClientRect);

	Win32UpdateWindow(DeviceContext, &ClientRect);
      }
    }
  }else{
    //TODO logging
  }

  return(0);
}
