#if !defined(WIN32_HANDMADE_H)

struct win32_offscreen_buffer
{
  BITMAPINFO Info;
  void* Memory;
  int   Pitch;
  int   Width;
  int   Height;
  int   BytesPerPixel;
};

struct win32_window_dimention
{
  int Width;
  int Height;
};

struct win32_sound_output
{
  int   SamplesPerSecond;
  int   BytesPerSample;
  DWORD SecondaryBufferSize;
  u32   RunningSampleIndex;
  DWORD SafteyBytes;
};

struct win32_debug_time_marker{
  DWORD PlayCursor;
  DWORD WriteCursor;
};

#define WIN32_HANDMADE_H
#endif
