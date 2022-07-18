/*
  TODO(samrat) This is not a final platform layer

  -Saved game locations
  -Getting a handle to our own executable file
  -Asset loading path
  -Multithreading (launch a thread)
  -Raw Input (support for multiple keyboard
  -SLeep/timeBeginPeriod
  -ClipCursor() (for multi-monitor support)
  -Fullscreen support
  -QueryCancelAutoplay
  -WM_SETCURSOR (control cursor visibility)
  -WM_ACTIVATEAPP (for when we are not the active application)
  -Blit speed improvements (BitBlt)
  -Hardware acceleration (OpenGL or Direct3D or both??)
  -GetKeyboardLayout (for french keyboards, international WASD support)
*/
// fejfelkfe

#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <math.h>


#define Pi32 3.14159265359f

#define internal static
#define local_persist static
#define global_variable static

// unsigned integers
typedef uint8_t u8;   // 1-byte
typedef uint16_t u16; // 2-byte
typedef uint32_t u32; // 4-byte
typedef uint64_t u64; // 8-byte

// signed integers
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef s32 b32;

typedef float f32;
typedef double f64;

#include "win32_handmade.h"
#include "handmade.cpp"

// Bitmap

global_variable bool GlobalRunning;
global_variable bool GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable IDirectSoundBuffer *GlobalSecondaryBuffer;
global_variable s64 GlobalPerfCountFrequency;

UINT DesiredSchedularMS = 1;
b32 SleepIsGrandular = (timeBeginPeriod(DesiredSchedularMS) == TIMERR_NOERROR);

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

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)

typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
  return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pState)

typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
  return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputSetState XInputSetState_

// NOTE(samrat): DirectSoundCreate

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)

typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void DEBUGPlatformFreeFileMemory(void *Memory){
  if(Memory){
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename){
  debug_read_file_result Result = {};
  /*Steps
    Get file handle
    Get file size
    Allocate memory for storing the file data
    Read the file into memory
    Close the file handle
  */
    

  HANDLE FileHandle = CreateFileA(Filename,GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0,0);
  if(FileHandle != INVALID_HANDLE_VALUE){

    LARGE_INTEGER FileSize;
    if(GetFileSizeEx(FileHandle, &FileSize)){

      u32 FileSize32 = SafeTruncateUint64(FileSize.QuadPart);

      Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE| MEM_COMMIT, PAGE_READWRITE);

      if(Result.Contents){
	DWORD BytesRead;
	if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead,0)
	   && (FileSize32 == BytesRead)){
	  Result.ContentsSize = BytesRead;
	}else{
	  DEBUGPlatformFreeFileMemory(Result.Contents);
	  Result.Contents = 0;
	}
      }else{
	//Error: Memory allocation failed
      }
    }else{
      //Error : File size evaluation failed
    }
    CloseHandle(FileHandle);
  }else{
    //Errror: handle creation failed
  }
  return (Result);
}

/* Steps
   Get file handle use GENERIC_WRITE insted od GENERIC_READ
   No File sharing
   No need to read the file size
   Write file
*/

internal b32 DEBUGPlatformWriteEntireFile(char *Filename, u32 MemorySize, void *Memory){
  b32 Result = false;

  HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0,0);
  if(FileHandle != INVALID_HANDLE_VALUE){
    DWORD BytesWritten;
    if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten,0)){
      //File written successfully
      Result = (BytesWritten == MemorySize);
    }
    else{
      //Write failed
    }
    CloseHandle(FileHandle);

  }
  else{
    //Handle creation failed
  }
  return (Result);
}

// Function to load the xinput dll file
internal void Win32LoadXInput()
{
  HMODULE XInputLibrary = LoadLibraryA("Xinput_3.dll");
  if (!XInputLibrary)
    XInputLibrary = LoadLibraryA("Xinput1_3.dll");

  if (!XInputLibrary)
    XInputLibrary = LoadLibraryA("Xinput9_1_0.dll");

  if (XInputLibrary)
    {
      XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
      XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");

      if (!XInputGetState)
	{
	  XInputGetState = XInputGetStateStub;
	}
      if (!XInputSetState)
	{
	  XInputSetState = XInputSetStateStub;
	}
    }
  else
    {
      XInputGetState = XInputGetStateStub;
      XInputSetState = XInputSetStateStub;
    }
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
					      game_button_state *OldState, DWORD ButtonBit,
					      game_button_state *NewState)
{
  NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
  NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
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
internal void Win32InitDSound(HWND Window, s32 SamplesPerSecond, s32 BufferSize)
{

  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
  if (DSoundLibrary)
    {

      direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

      IDirectSound *DirectSound;

      if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
	{
	  WAVEFORMATEX WaveFormat = {};
	  WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	  WaveFormat.nChannels = 2;
	  WaveFormat.nSamplesPerSec = SamplesPerSecond;
	  WaveFormat.wBitsPerSample = 16;
	  WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
	  WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

	  if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
	    {

	      DSBUFFERDESC BufferDescription = {};
	      BufferDescription.dwSize = sizeof(BufferDescription);
	      BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

	      IDirectSoundBuffer *PrimaryBuffer;

	      if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
		{

		  if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
		    {
		      // Note(samrat) We have finally set the format of the primary buffer
		      OutputDebugStringA("Primary buffer has been set \n");
		    }

		  // Set up primary buffer;
		}
	    }
	  //"Create" a secodnary buffer

	  DSBUFFERDESC BufferDescription  = {};
	  BufferDescription.dwSize        = sizeof(BufferDescription);
	  BufferDescription.dwBufferBytes = BufferSize;
	  BufferDescription.lpwfxFormat   = &WaveFormat;
	  BufferDescription.dwFlags       = DSBCAPS_GETCURRENTPOSITION2;

	  if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
	    {
	      OutputDebugStringA("Secondary buffer has been set \n");
	    }
	}
    }
}
internal void
Win32ClearBuffer(win32_sound_output *SoundOutput){
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size,0))){
    //Do the work
    u8 *DestSample = (u8*)Region1;

    for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
      {
	*DestSample++ = 0;
      }

    DestSample = (u8*)Region2;

    for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
      {
	*DestSample++ = 0;
      }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }

}

// Copy the buffer we got from the game code
internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer * SouceBuffer)
{
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;
  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
					    &Region1, &Region1Size,
					    &Region2, &Region2Size,
					    0)))
    {

      DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;

      s16 *SourceSample = SouceBuffer->Samples;
      s16 *DestSample= (s16 *)Region1;

      for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount;
	   ++SampleIndex)
	{
	  *DestSample++ = *SourceSample++;
	  *DestSample++ = *SourceSample++;
	  ++SoundOutput->RunningSampleIndex;
	}

      DestSample = (s16 *)Region2;

      DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
      for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount;
	   ++SampleIndex)
	{
	  *DestSample++ = *SourceSample++;
	  *DestSample++ = *SourceSample++;
	  ++SoundOutput->RunningSampleIndex;
	}
      GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}


// Helper function
internal win32_window_dimention
Win32GetWindowDimention(HWND Window)
{
  win32_window_dimention Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);

  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return Result;
}

/**
 * This function creates a new BitmapMemory on every call,
 * We need height and weight to allocate the memory by and to put it into the bitmapinfo
 * global_variables BitmapWidth and BitmapHeight is also set here
 */
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{

  if (Buffer->Memory){
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }
  // Initilization
  Buffer->Width = Width;
  Buffer->Height = Height;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biCompression = BI_RGB;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // Top-down

  Buffer->BytesPerPixel = 4;
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel; // This is a constant variable which's size is equal to the total byes in a row

  int BitmapMemorySize = Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

/**
 * Rectange to Rectange copy
 */
internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
  StretchDIBits(DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory, &Buffer->Info,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
			UINT Message,
			WPARAM WParam,
			LPARAM LParam)
{
  LRESULT Result = 0;
  switch (Message)
    {
    case WM_SIZE:
      {
      }
      break;
    case WM_DESTROY:
      {
	GlobalRunning = false;
      }
      break;
    case WM_CLOSE:
      {
	GlobalRunning = false;
      }
      break;
    case WM_ACTIVATEAPP:
      {
      }
      break;
    case WM_PAINT:
      {
	PAINTSTRUCT Paint;
	HDC DeviceContext = BeginPaint(Window, &Paint);

	win32_window_dimention Dimention = Win32GetWindowDimention(Window);

	Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimention.Width, Dimention.Height);

	EndPaint(Window, &Paint);
      }
      break;

      // Keyboard stuffs here
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
      {
	Assert(!"Keyboard input came in thru a non-dispatched message!");
      }
      break;

    default:
      {
	Result = DefWindowProc(Window, Message, WParam, LParam);
      }
      break;
    }
  return Result;
}


internal void
Win32ProcessKeyboardMessage(game_button_state * NewState, b32 IsDown){
  NewState->EndedDown = IsDown;
  ++NewState->HalfTransitionCount;
}

internal f32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold){
  f32 Result = 0;

  if(Value < -DeadZoneThreshold){
    Result = (f32)(Value + DeadZoneThreshold) /(23768.0f - DeadZoneThreshold);
  }else if(Value > DeadZoneThreshold){
    Result = (f32)(Value - DeadZoneThreshold) /(23767.0f - DeadZoneThreshold);
  }
  return Result;
}

internal void
Win32ProcessPendingMessages(game_controller_input * KeyboardController){
  MSG Message;
  while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {

      // Bulletproof
      switch(Message.message){
		  
      case WM_QUIT:
	{
	  GlobalRunning = false;
	}break;
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP:
	{
	  bool IsDown = ((Message.lParam & (1 << 31)) == 0);

	  bool WasDown = ((Message.lParam & (1 << 30)) != 0);

	  u32 VKCode = (u32)Message.wParam;

	  if (IsDown != WasDown)
	    {
	      if (VKCode == 'W')
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown); 
		}
	      else if (VKCode == 'A')
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown); 
		}
	      else if (VKCode == 'S')
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown); 
		}
	      else if (VKCode == 'D')
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown); 
		}
	      else if (VKCode == 'Q')
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown); 
		}
	      else if (VKCode == 'E')
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown); 
		}
	      else if (VKCode == VK_UP)
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown); 
		}
	      else if (VKCode == VK_DOWN)
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown); 
		}
	      else if (VKCode == VK_LEFT)
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown); 
		}
	      else if (VKCode == VK_RIGHT)
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown); 
		}
	      else if (VKCode == VK_ESCAPE)
		{
		  GlobalRunning = false;
		}
	      else if (VKCode == VK_SPACE)
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
		}
	      else if (VKCode == VK_BACK)
		{
		  Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
		}
#if HANDMADE_INTERNAL
	      else if (VKCode == 'P'){
		if(IsDown){
		  GlobalPause = !GlobalPause;
		}
	      }
#endif

	      b32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
	      if ((VKCode == VK_F4) && AltKeyWasDown)
		{
		  GlobalRunning = false;
		}
	    }
	}break;

      default:
	{
	  TranslateMessage(&Message);
	  DispatchMessage(&Message);
	}break;
      }
    }

}
inline LARGE_INTEGER Win32GetWallClock(){
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result;
}

inline f32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End){
  f32 Result = (f32)(End.QuadPart - Start.QuadPart)/(f32)(GlobalPerfCountFrequency);
  return Result;
}

inline void Win32DebugDrawVertical(win32_offscreen_buffer *Backbuffer,
				   int X, int Top, int Bottom, u32 Color)
{
  if(Top <= 0){
    Top = 0;
  }
  if(Bottom > Backbuffer->Height){
    Bottom = Backbuffer->Height;
  }
  if((X >=0) && (X < Backbuffer->Width)){
    u8 *Pixel = (u8*)Backbuffer->Memory +
      Top * Backbuffer->Pitch +
      X   * Backbuffer->BytesPerPixel;

    for(int Y = Top; Y < Bottom; ++Y){
      *(u32*)Pixel = Color;
      Pixel += Backbuffer->Pitch;
    }
  }
}

inline void Win32DrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer,
				       win32_sound_output * SoundOutput, f32 C,
				       int PadX, int Top, int Bottom,
				       DWORD Value, u32 Color){
  f32 XReal = C * (f32)Value;
  int X     = PadX + (int)XReal;

  Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}

inline void Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer,
				  int MarkerCount, win32_debug_time_marker * Markers ,
				  int CurrentMarkerIndex , win32_sound_output * SoundOutput,
				  f32 TargetSecondsPerFrame){
  int PadX = 16; 
  int PadY = 16; 

  int LineHeight = 64;

  f32 C = (f32)(Backbuffer->Width - 2*PadX) / (f32)SoundOutput->SecondaryBufferSize;

  for(int MarkerIndex = 0;
      MarkerIndex < MarkerCount;
      ++MarkerIndex){
    win32_debug_time_marker * ThisMarker = &Markers[MarkerIndex];

    Assert(ThisMarker->OutputPlayCursor   < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputWriteCursor  < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputLocation     < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputByteCount    < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->FlipPlayCursor     < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->FlipWriteCursor    < SoundOutput->SecondaryBufferSize);

    DWORD PlayColor         = 0xFFFFFFFF;
    DWORD WriteColor        = 0xFFFF0000;
    DWORD ExpectedFlipColor = 0xFFFFFF00;
    DWORD PlayWindowColor   = 0xFFFF00FF;

    int Top    = PadY;
    int Bottom = PadY + LineHeight;

    if(MarkerIndex == CurrentMarkerIndex){
      Top    += LineHeight + PadY;
      Bottom += LineHeight + PadY;
    }

    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor , PlayColor);
    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + (480 * SoundOutput->BytesPerSample), PlayWindowColor);
    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor , WriteColor);

    Top    += LineHeight + PadY;
    Bottom += LineHeight + PadY;

    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,C, PadX, Top, Bottom,ThisMarker->OutputByteCount + ThisMarker->OutputByteCount, WriteColor);

    Top    += LineHeight + PadY;
    Bottom += LineHeight + PadY;

    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,C, PadX, Top, Bottom,ThisMarker->ExpectedFlipPlayCursor , ExpectedFlipColor);

  }
}

int CALLBACK
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz/2)
  f32 TargetSecondsPerFrame = 1.0f/GameUpdateHz;

  Win32LoadXInput();

  if (RegisterClass(&WindowClass))
    {
      HWND WindowHandle = CreateWindowEx(
					 0,
					 WindowClass.lpszClassName,
					 "Handmade Hero",
					 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
					 CW_USEDEFAULT, CW_USEDEFAULT,
					 CW_USEDEFAULT, CW_USEDEFAULT,
					 0, 0, Instance, 0);

      if (WindowHandle)
	{

	  Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	  //Sound init start
	  win32_sound_output SoundOutput = {};
	  SoundOutput.SamplesPerSecond = 48000;
	  SoundOutput.BytesPerSample = sizeof(s16) * 2;
	  SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
	  SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz)/3;

	  Win32InitDSound(WindowHandle, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
	  Win32ClearBuffer(&SoundOutput);
	  GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

	  s16 *Samples = (s16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	  //Sound init stop

#if HANDMADE_INTERNAL
	  LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
	  LPVOID BaseAddress = 0;
#endif

	  game_memory GameMemory = {};
	  GameMemory.PermanentStorageSize = Megabytes(64);
	  GameMemory.TransientStorageSize = Gigabytes(2);
	  u64 TotalStorageSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

	  GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalStorageSize, MEM_RESERVE | MEM_COMMIT , PAGE_READWRITE);

	  GameMemory.TransientStorage = ((u8*) GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

	  if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage){
	    //Allocation successfull, go to the main loop
	  }else{
	    //Allocation failed logging
	  }

	  HDC DeviceContext = GetDC(WindowHandle);

	  game_input Input[2] = {};
	  game_input * OldInput = &Input[0];
	  game_input * NewInput = &Input[1];

	  int DebugTimeMarkerIndex = 0;
	  win32_debug_time_marker DebugTimeMarkers[GameUpdateHz/2] = {};

	  game_controller_input * NewKeyboardController = GetController(NewInput,0);
	  game_controller_input * OldKeyboardController = GetController(OldInput,0);
	  *NewKeyboardController = {};
	  NewKeyboardController->IsConnected = true;

	  GlobalRunning = true;

	  LARGE_INTEGER LastCounter   = Win32GetWallClock();
	  LARGE_INTEGER FlipWallClock = Win32GetWallClock(); 
	    
	  u64 LastCycleCount = __rdtsc();

	  // Constant values that tells us how many increments does the clock goes thru each second
	  LARGE_INTEGER PerfCountFrequencyResult;
	  QueryPerformanceFrequency(&PerfCountFrequencyResult);
	  GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	  // Note(samrat): DirectSound Output Test start

	  b32 SoundIsValid     = false;
	  while (GlobalRunning)
	    {

	      Win32ProcessPendingMessages(NewKeyboardController);
	      if(!GlobalPause){

		DWORD MaxControllerCount = XUSER_MAX_COUNT;

		if(MaxControllerCount > (ArrayCount(NewInput->Controllers) -1)){
		  MaxControllerCount = (ArrayCount(NewInput->Controllers) -1);
		}

		for (DWORD ControllerIndex = 0;
		     ControllerIndex < MaxControllerCount ;
		     ++ControllerIndex)
		  {

		    DWORD OurControllerIndex = ControllerIndex + 1;
		    game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
		    game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

		    XINPUT_STATE ControllerState;
		    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		      {

			XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
			NewController->IsConnected = true;

			bool Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
			bool Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
			bool Left = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
			bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A, &NewController->ActionDown);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X, &NewController->ActionLeft);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y, &NewController->ActionUp);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back , XINPUT_GAMEPAD_BACK, &NewController->Back);
			NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

			if((NewController->StickAverageX != 0.0f) || (NewController->StickAverageY != 0.0f)){
			  NewController->IsAnalog = true;
			}

			if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP){
			  NewController->StickAverageY = 1.0f;
			  NewController->IsAnalog = false;
			}
			else if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN){
			  NewController->StickAverageY = -1.0f;
			  NewController->IsAnalog = false;
			}
			if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT){
			  NewController->StickAverageX = -1.0f;
			  NewController->IsAnalog = false;
			}
			else if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT){
			  NewController->StickAverageX = 1.0f;
			  NewController->IsAnalog = false;
			}
			f32 Threshold = 0.5f;
			Win32ProcessXInputDigitalButton((NewController->StickAverageX >  Threshold)?1:0,  &NewController->MoveRight, 1, &OldController->MoveRight);
			Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold)?1:0,  &NewController->MoveLeft,  1, &OldController->MoveLeft);
			Win32ProcessXInputDigitalButton((NewController->StickAverageY >  Threshold)?1:0,  &NewController->MoveUp,    1, &OldController->MoveUp);
			Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold)?1:0,  &NewController->MoveDown,  1, &OldController->MoveDown);
		      }
		    else
		      {
			// NOTE: Controller unavailable
		      }
		  }

		//Graphics buffer for calling game code
		game_offscreen_buffer Buffer = {};

		Buffer.Memory = GlobalBackbuffer.Memory;
		Buffer.Height = GlobalBackbuffer.Height;
		Buffer.Width  = GlobalBackbuffer.Width;
		Buffer.Pitch  = GlobalBackbuffer.Pitch;
	      
		GameUpdateAndRender(&GameMemory, NewInput, &Buffer);

		LARGE_INTEGER AudioWallClock = Win32GetWallClock();
		f32 FromBeginToAudioSeconds  = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

		DWORD PlayCursor  = 0;
		DWORD WriteCursor = 0;
		if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK){
		  if(!SoundIsValid){
		    SoundOutput.RunningSampleIndex = WriteCursor/SoundOutput.BytesPerSample;
		    SoundIsValid = true;

		  }
		  DWORD ByteToLock  = 0;

		  DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;

		  f32 SecondsLeftUntilFlip       = TargetSecondsPerFrame - FromBeginToAudioSeconds;
		  DWORD ExpectedBytesUntilFlip   = (DWORD)((SecondsLeftUntilFlip /TargetSecondsPerFrame) * (f32)ExpectedSoundBytesPerFrame);
		  DWORD ExpectedFrameBoundryByte = PlayCursor + ExpectedBytesUntilFlip;
		  DWORD SafeWriteCursor          = WriteCursor;

		  if(SafeWriteCursor < PlayCursor){
		    SafeWriteCursor += SoundOutput.SecondaryBufferSize;
		  }
		  Assert(SafeWriteCursor >= PlayCursor);
		  SafeWriteCursor += SoundOutput.SafetyBytes;

		  b32 AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundryByte;
		  DWORD TargetCursor = 0;
		  if(AudioCardIsLowLatency){
		    TargetCursor = ExpectedFrameBoundryByte + ExpectedSoundBytesPerFrame;
		  }else{
		    TargetCursor = (SafeWriteCursor + ExpectedSoundBytesPerFrame);
		  }
		  TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;
		  ByteToLock   = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);

		  DWORD BytesToWrite = 0;
		  if(ByteToLock > TargetCursor){
		    BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
		    BytesToWrite += TargetCursor;
		  }else{
		    BytesToWrite = TargetCursor - ByteToLock;
		  }
		  game_sound_output_buffer SoundBuffer = {};
		  SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
		  SoundBuffer.SampleCount = BytesToWrite/ SoundOutput.BytesPerSample;
		  SoundBuffer.Samples = Samples;
		  GameGetSoundSamples(&GameMemory, &SoundBuffer);

#if HANDMADE_INTERNAL
		  win32_debug_time_marker * Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
		  Marker->OutputPlayCursor       = PlayCursor;
		  Marker->OutputWriteCursor      = WriteCursor;
		  Marker->OutputLocation         = ByteToLock;
		  Marker->OutputByteCount        = BytesToWrite;
		  Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundryByte;

		  DWORD UnwrappedWriteCursor = WriteCursor;

		  if(UnwrappedWriteCursor < PlayCursor){
		    UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
		  }

		  DWORD AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
		  f32 AudioLatencySeconds = ((f32)AudioLatencyBytes / (f32)SoundOutput.BytesPerSample)/ (f32)SoundOutput.SamplesPerSecond;

		  char DebugSoundBuffer[256];
		  _snprintf_s(DebugSoundBuffer, sizeof(DebugSoundBuffer),
			      "LPC%u, BTL:%u, TC:%u BTW:%u Latency:%u (%.3fs)\n",
			      PlayCursor, ByteToLock, TargetCursor, BytesToWrite, AudioLatencyBytes, AudioLatencySeconds);

		  OutputDebugStringA(DebugSoundBuffer);
#endif
		  Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
		}
		else{
		  SoundIsValid = false;
		}

		//Frame rate control
		LARGE_INTEGER WorkCounter = Win32GetWallClock();
		f32 SecondsElapsedForWork =Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());;

		if(SecondsElapsedForWork < TargetSecondsPerFrame){
		  if(SleepIsGrandular){
		    DWORD SleepMs = (DWORD)(1000.f * (TargetSecondsPerFrame - SecondsElapsedForWork));
		    Sleep(SleepMs);
		  }

		  SecondsElapsedForWork = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());

		  while(SecondsElapsedForWork < TargetSecondsPerFrame){
		    SecondsElapsedForWork = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
		  }
		}else{
		  //Missed frame rate
		}

		u64 EndCycleCount        = __rdtsc();
		LARGE_INTEGER EndCounter = Win32GetWallClock();
		u64 CyclesElapsed        = EndCycleCount - LastCycleCount;
		s64 CounterElapsed       = WorkCounter.QuadPart - LastCounter.QuadPart;

		LastCounter              = EndCounter;
		LastCycleCount           = EndCycleCount;
		win32_window_dimention Dimention = Win32GetWindowDimention(WindowHandle);

#if HANDMADE_INTERNAL
		Win32DebugSyncDisplay(&GlobalBackbuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, DebugTimeMarkerIndex -1,&SoundOutput, TargetSecondsPerFrame);
#endif

		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimention.Width, Dimention.Height);
		FlipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
		{
		  DWORD FlipPlayCursor = 0;
		  DWORD FlipWriteCursor = 0;
		  if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&FlipPlayCursor, &FlipWriteCursor))){
		    Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
		    win32_debug_time_marker * Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
		    Marker->FlipPlayCursor  = FlipPlayCursor;
		    Marker->FlipWriteCursor = FlipWriteCursor;
		  }

		}
#endif

		game_input * Temp = NewInput;
		NewInput = OldInput;
		OldInput = Temp;


#if 0
		//Performance Counter
		s32 FPS = (s32)((f32)GlobalPerfCountFrequency / (f32)CounterElapsed);
		f32 MCPF = CyclesElapsed / (1000 * 1000);
		char Buffer[256];
		wsprintfA(Buffer, "%dms/f, %df/s, %dMc/f \n", MSPerFrame, FPS, MCPF);
		OutputDebugStringA(Buffer);
#endif
#if HANDMADE_INTERNAL
		++DebugTimeMarkerIndex;
		if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers)){
		  DebugTimeMarkerIndex = 0;
		}
#endif
	      }
	    }
	}
    }
  else
    {
      // TODO logging
    }

  return (0);
}
