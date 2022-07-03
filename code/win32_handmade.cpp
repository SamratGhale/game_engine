#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#define Pi32 3.14159265359f

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

typedef s32 b32;

typedef float f32;
typedef double f64;

//Bitmap
struct win32_offscreen_buffer
{
  BITMAPINFO Info;
  void *     Memory;  
  int        Pitch;
  int        Width;
  int        Height;
  int        BytesPerPixel; 
};

struct win32_window_dimention{
  int Width;
  int Height;
};

struct win32_sound_output{
  int SamplesPerSecond;
  int BytesPerSample;
  int SecondaryBufferSize;
  u32 RunningSampleIndex;
  int ToneHz;
  int ToneVolume;
  int WavePeriod;
  f32 tSine;
  int LatencySampleCount;
};


global_variable bool       GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable IDirectSoundBuffer * GlobalSecondaryBuffer;

/**
 * XINPUT

 *NOTE(samrat) if we change function declearation to a typedef,
 * we are saying "There is a function of this type and I want to start declearing function that are pointers to it

 * Define a function macro
 * define typedef to the function using that macro so that we can use it as pointer
 * Define a stub function (default function)
 * use the typedef to create two global variables and it's default values to stub function
 * #define the function as the actual name of the function
 */

//XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)

typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub){
  return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_get_state * XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pState)

typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub){
  return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_set_state * XInputSetState_ = XInputSetStateStub;

#define XInputSetState XInputSetState_

//NOTE(samrat): DirectSoundCreate

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND * ppDS, LPUNKNOWN pUnkOuter)

typedef DIRECT_SOUND_CREATE(direct_sound_create);


//Function to load the xinput dll file
internal void Win32LoadXInput(){
  HMODULE XInputLibrary = LoadLibraryA("Xinput_3.dll");
  if(!XInputLibrary)
    XInputLibrary = LoadLibraryA("Xinput1_3.dll");

  if(!XInputLibrary)
    XInputLibrary = LoadLibraryA("Xinput9_1_0.dll");

  if(XInputLibrary){
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");

    if(!XInputGetState){
      XInputGetState = XInputGetStateStub;
    }
    if(!XInputSetState){
      XInputSetState = XInputSetStateStub;
    }
  }else{
    XInputGetState = XInputGetStateStub;
    XInputSetState = XInputSetStateStub;
  }
}

/**
 * Steps
 * Load the DSound
 * Load the library
 * Load the function from the library
 * Get the DirectSound Object (Like in oop)
 * Set Cooperative leve
 * Create primary buffer
 * Create secondary buffer
 */
internal void Win32InitDSound(HWND Window, s32 SamplesPerSecond , s32 BufferSize){

  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
  if(DSoundLibrary){

    direct_sound_create * DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");

    IDirectSound * DirectSound;

    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))){
      WAVEFORMATEX WaveFormat= {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) /8 ;
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      
      if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){

	DSBUFFERDESC BufferDescription = {} ;
	BufferDescription.dwSize = sizeof(BufferDescription);
	BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

	IDirectSoundBuffer * PrimaryBuffer;

	if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))){

	  if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))){
	    //Note(samrat) We have finally set the format of the primary buffer
	    OutputDebugStringA("Primary buffer has been set \n");
	  }

	  //Set up primary buffer;
	}
      }
      //"Create" a secodnary buffer

      DSBUFFERDESC BufferDescription = {};
      BufferDescription.dwSize = sizeof(BufferDescription);
      BufferDescription.dwBufferBytes = BufferSize;
      BufferDescription.lpwfxFormat = &WaveFormat;

      if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0))){
	OutputDebugStringA("Secondary buffer has been set \n");
      }
    }
  }
}

//Lock garera write garne matra
internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite){
  VOID * Region1;
  DWORD Region1Size; 
  VOID * Region2;
  DWORD Region2Size;
  if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
					   &Region1, &Region1Size,
					   &Region2, &Region2Size,
					   0)))
    {

      s16 *SampleOut = (s16 *)Region1;

      DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;    

      for(DWORD SampleIndex = 0; SampleIndex  < Region1SampleCount;
	  ++SampleIndex){

	f32 SineValue = sinf(SoundOutput->tSine);

	s16 SampleValue = (s16)(SineValue * SoundOutput->ToneVolume);
	*SampleOut++ = SampleValue;
	*SampleOut++ = SampleValue;

	SoundOutput->tSine += (2.0f * Pi32 * 1.0f)/(f32)SoundOutput->WavePeriod;
	++SoundOutput->RunningSampleIndex;
      }

      SampleOut = (s16 *)Region2;

      DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;    
      for(DWORD SampleIndex = 0; SampleIndex  < Region2SampleCount;
	  ++SampleIndex){
	f32 SineValue = sinf(SoundOutput->tSine);
	s16 SampleValue = (s16)(SineValue * SoundOutput->ToneVolume);
	*SampleOut++ = SampleValue;
	*SampleOut++ = SampleValue;
	SoundOutput->tSine += (2.0f * Pi32 * 1.0f)/(f32)SoundOutput->WavePeriod;
	++SoundOutput->RunningSampleIndex;
      }
      GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}


//Helper function
internal win32_window_dimention
Win32GetWindowDimention(HWND Window){
  win32_window_dimention Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);

  Result.Width  = ClientRect.right  - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return Result;
}


internal void RenderWeiredGradient(win32_offscreen_buffer * Buffer, int XOffset, int YOffset){

  u8 * Row = (u8 *)Buffer->Memory; //u8 so we can multiply with pitch(which unit is in bytes) and get into the next row

  for(int x = 0; x < Buffer->Height; ++x){
    u32 *Pixel = (u32*)Row; //First pixel of the row

    for(int y = 0; y < Buffer->Width; ++y){
      u8 Red   = 0;
      u8 Green = (u8)(y + YOffset);
      u8 Blue  = (u8)(x + XOffset);

      *Pixel++ = Red << 16 | Green << 8 | Blue;  // PP RRi GG BB
    }

    Row += Buffer->Pitch;
  }
  
}

/**
 * This function creates a new BitmapMemory on every call,
 * We need height and weight to allocate the memory by and to put it into the bitmapinfo 
 * global_variables BitmapWidth and BitmapHeight is also set here
 */
internal void
Win32ResizeDIBSection(win32_offscreen_buffer * Buffer, int Width, int Height){

  if(Buffer->Memory){
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }
  //Initilization
  Buffer->Width  = Width;
  Buffer->Height = Height;

  Buffer->Info.bmiHeader.biSize        = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biCompression = BI_RGB;
  Buffer->Info.bmiHeader.biPlanes      = 1;
  Buffer->Info.bmiHeader.biBitCount    = 32;
  Buffer->Info.bmiHeader.biWidth       = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight      = -Buffer->Height; //Top-down
  
  Buffer->BytesPerPixel = 4;
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel; //This is a constant variable which's size is equal to the total byes in a row

  int BitmapMemorySize = Buffer->BytesPerPixel * (Buffer->Width* Buffer->Height);
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

}

/**
 * Rectange to Rectange copy
 */
internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight){
  StretchDIBits( DeviceContext,
		 0, 0, WindowWidth, WindowHeight, 
		 0, 0, Buffer->Width, Buffer->Height,
		 Buffer->Memory, &Buffer->Info,
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
      }break;
    case WM_DESTROY:
      {
	GlobalRunning = false;
      }break;
    case WM_CLOSE:
      {
	GlobalRunning = false;
      }break;
    case WM_ACTIVATEAPP:
      {
      }break;
    case WM_PAINT:
      {
	PAINTSTRUCT Paint;
	HDC DeviceContext = BeginPaint(Window, &Paint);

	win32_window_dimention Dimention = Win32GetWindowDimention(Window);

	Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimention.Width, Dimention.Height);

	EndPaint(Window, &Paint);
      }break;

      //Keyboard stuffs here
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
      {
	bool IsDown = ((LParam & (1 << 31)) == 0);

	bool WasDown = ((LParam & (1 << 30)) != 0);

	u32 VKCode = WParam;

	if(IsDown != WasDown){
	  if(VKCode == 'W'){
	  }
	  else if(VKCode == 'A'){
	  }
	  else if(VKCode == 'S'){
	  }
	  else if(VKCode == 'D'){
	  }
	  else if(VKCode == 'Q'){
	  }
	  else if(VKCode == 'E'){
	  }
	  else if(VKCode == VK_UP){
	  }
	  else if(VKCode == VK_DOWN){
	  }
	  else if(VKCode == VK_LEFT){
	  }
	  else if(VKCode == VK_RIGHT){
	  }
	  else if(VKCode == VK_ESCAPE){
	    OutputDebugStringA("ESCAPE: ");
	    if(IsDown){
	      OutputDebugStringA("IsDown");
	    }
	    if(WasDown){
	      OutputDebugStringA("WasDown");
	    }
	    OutputDebugStringA("\n");
	  }
	  else if(VKCode == VK_SPACE){
	  }

	  b32 AltKeyWasDown =((LParam & (1 << 29)) != 0); 
	  if((VKCode == VK_F4) && AltKeyWasDown){
	    GlobalRunning = false;
	  }
	  
	}
      }break;

    default:
      {
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

  Win32LoadXInput();

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

      Win32ResizeDIBSection(&GlobalBackbuffer,1280, 720);
      int XOffset = 0;
      int YOffset = 0;


      //NOTE(samrat) Init DirectX Sound

      win32_sound_output SoundOutput = {};

      SoundOutput.SamplesPerSecond      = 48000;

      //16 For left and 16 for right, Everyone is happy

      SoundOutput.BytesPerSample        = sizeof(s16) * 2;

      //Total Sample in second (SamplesPerSecond) * Number of seconds (2) * BytesPerSample
      SoundOutput.SecondaryBufferSize   = 2 * SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

      //how many time will be occilate in a second (ps that's the defination of a Hz)
      SoundOutput.ToneHz                = 256;
      //How long does it take that wave to come up and down
      SoundOutput.WavePeriod      = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
      SoundOutput.RunningSampleIndex    = 0;
      SoundOutput.ToneVolume            = 3000;
      SoundOutput.LatencySampleCount    = SoundOutput.SamplesPerSecond / 15;

      Win32InitDSound(WindowHandle, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);

      GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);

      //Init DirectX Sound end

      GlobalRunning = true;

      HDC DeviceContext = GetDC(WindowHandle);


      while(GlobalRunning){

	MSG Message;
	while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)){

	  //Bulletproof
	  if(Message.message == WM_QUIT){
	    GlobalRunning = false;
	  }
	  
	  TranslateMessage(&Message);
	  DispatchMessage(&Message);
	}

	for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex){

	  XINPUT_STATE ControllerState;
	  if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS){

	    XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

	    bool Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
	    bool Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
	    bool Left = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
	    bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
	    bool Start = Pad->wButtons & XINPUT_GAMEPAD_START;
	    bool Back = Pad->wButtons & XINPUT_GAMEPAD_BACK;

	    bool LeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
	    bool RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

	    bool AButton = Pad->wButtons & XINPUT_GAMEPAD_A;
	    bool BButton = Pad->wButtons & XINPUT_GAMEPAD_B;
	    bool XButton = Pad->wButtons & XINPUT_GAMEPAD_X;
	    bool YButton = Pad->wButtons & XINPUT_GAMEPAD_Y;

	    s16 StickX = Pad->sThumbLX;
	    s16 StickY = Pad->sThumbLY;

	    XOffset += StickX >> 12;
	    YOffset += StickY >> 12;

	    SoundOutput.ToneHz = 512 + (int)(256.0f *((f32)StickY / 30000.0f));
	    SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
	  }
	  else{
	    //NOTE: Controller unavailable
	  }
	}


	RenderWeiredGradient(&GlobalBackbuffer,XOffset, YOffset);

	//Note(samrat): DirectSound Output Test start
	DWORD PlayCursor;
	DWORD WriteCursor;
	if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))){
	  //TODO(samrat) Assert that Region1Size and Region2Size are valid, (multiple of 2)

	  DWORD BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize ; //TODO
	  DWORD TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
	  DWORD BytesToWrite ;//TODO

	  if(BytesToLock == TargetCursor){
	    BytesToWrite = 0;
	  }
	  else if(BytesToLock > TargetCursor){
	    //Play cursor is behind
	    BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
	    BytesToWrite += TargetCursor;
	  }else{
	    BytesToWrite = TargetCursor - BytesToLock;
	  }
	  Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite);
	}


	//The cursors yo

	//DirectSound output Test end

	win32_window_dimention Dimention = Win32GetWindowDimention(WindowHandle);

	Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimention.Width, Dimention.Height);
      }
    }
  }else{
    //TODO logging
  }

  return(0);
}
