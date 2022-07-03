#include <stdio.h>
#include <stdint.h>
#include <dsound.h>
#include <windows.h>
#include <math.h>

#define Pi32 3.14159265359f

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

IDirectSoundBuffer * GlobalBuffer;

//funcs sigs
void DirectSoundGoBrrrr(HWND);
void FillSoundBuffer(DWORD BytesToLock, DWORD BytesToWrite);

/**
 * 2 seconds
 * 48000 samples per second
 * 2 sides per sample (left right)
 */
int BufferSize = 2 * 48000 *4;
int RunningSampleIndex = 0;
f32 tSine = 0;

//Function signature for the DirectSoundCreate only function that we need from dsound library

typedef HRESULT WINAPI direct_sound_create(
					   LPCGUID pcGuidDevice,
					   LPDIRECTSOUND * ppDS,
					   LPUNKNOWN pUnkOuter);

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
			UINT Message,
			WPARAM WParam,
			LPARAM LParam
			)
{
  LRESULT Result = 0;
  Result = DefWindowProc(Window, Message, WParam, LParam);
  return Result;
}
int CALLBACK WinMain(
		     HINSTANCE Instance,
		     HINSTANCE PrevInstance,
		     LPSTR CommandLine,
		     int ShowCode
		     ){

  WNDCLASS WindowClass = {};
  WindowClass.style  = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.hInstance = Instance;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
  RegisterClass(&WindowClass);
  HWND WindowHandle = CreateWindowEx(
				     0,
				     WindowClass.lpszClassName,
				     "Handmade Hero",
				     WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				     CW_USEDEFAULT,CW_USEDEFAULT,
				     CW_USEDEFAULT,CW_USEDEFAULT,
				     0, 0, Instance, 0);

  DirectSoundGoBrrrr(WindowHandle);

  GlobalBuffer->Play(0,0, DSBPLAY_LOOPING);
  bool GlobalRunning = true;
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
    DWORD PlayCursor;
    DWORD WriteCursor;

    GlobalBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

    DWORD TargetCursor =((PlayCursor + (3200 * 4)) % BufferSize) ;
    //BytesToLock->where to start writing from

    DWORD BytesToLock = (RunningSampleIndex * 4) % BufferSize;

    DWORD BytesToWrite = 0;
    if(BytesToLock == TargetCursor){
      BytesToWrite = 0;
    }
    else if(BytesToLock > TargetCursor){
      BytesToWrite = BufferSize - BytesToLock;
      BytesToWrite += TargetCursor;
    }else{
      BytesToWrite = TargetCursor - BytesToLock;
    }

    FillSoundBuffer(BytesToLock, BytesToWrite);
  }
  return 0;
}

//Lock and wirte and unlock
void FillSoundBuffer(DWORD BytesToLock, DWORD BytesToWrite){

  VOID * Region1;
  DWORD Region1Size;
  VOID * Region2;
  DWORD Region2Size;

  GlobalBuffer->Lock(BytesToLock, BytesToWrite,
		     &Region1, &Region1Size,
		     &Region2, &Region2Size,0);

  int16_t * SampleOut = (int16_t*)Region1;

  DWORD Region1SampleCount = Region1Size / 4;

  for(DWORD i = 0; i < Region1SampleCount; i++){
    f32 SineValue = sinf(tSine);
    s16 SampleValue = (s16)(SineValue * 3000);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += (2.0f * Pi32 * 1.0f)/(f32)93;
    ++RunningSampleIndex;
  }

  SampleOut = (int16_t*)Region2;

  DWORD Region2SampleCount = Region2Size / 4;

  for(DWORD i = 0; i < Region2SampleCount; i++){
    f32 SineValue = sinf(tSine);
    s16 SampleValue = (s16)(SineValue * 3000);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += (2.0f * Pi32 * 1.0f)/(f32)93;
    ++RunningSampleIndex;
  }
  GlobalBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}


void DirectSoundGoBrrrr(HWND Window){
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

  direct_sound_create * DirectSoundCreate =
    (direct_sound_create * )GetProcAddress(DSoundLibrary, "DirectSoundCreate");

  IDirectSound * DirectSound;

  //Now i have a directsound object now we need to create buffer
  DirectSoundCreate(0, &DirectSound, 0);

  //Wave format
  WAVEFORMATEX WaveFormat= {};
  WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
  WaveFormat.nChannels = 2;
  WaveFormat.nSamplesPerSec = 48000;
  WaveFormat.wBitsPerSample = 16;
  WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) /8 ;
  WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

  //Set Primary buffer to set the format
      
  IDirectSoundBuffer * PrimaryBuffer;

  if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
      OutputDebugStringA("I'm succeeded\n");
    }
  DSBUFFERDESC PrimaryBufferDescription = {} ;
  PrimaryBufferDescription.dwSize = sizeof(PrimaryBufferDescription);
  PrimaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

  DirectSound->CreateSoundBuffer(&PrimaryBufferDescription, &PrimaryBuffer, 0);

  PrimaryBuffer->SetFormat(&WaveFormat);
    

  DSBUFFERDESC BufferDescription = {};
  BufferDescription.dwSize = sizeof(BufferDescription);
  BufferDescription.dwBufferBytes = BufferSize;
  BufferDescription.lpwfxFormat = &WaveFormat;

  DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalBuffer,0);

}
