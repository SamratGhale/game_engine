#if !defined(WIN32_HANDMADE_H)

struct win32_offscreen_buffer
{
  BITMAPINFO Info;
  void *Memory;
  int Pitch;
  int Width;
  int Height;
  int BytesPerPixel;
};

struct win32_window_dimention
{
  int Width;
  int Height;
};

struct win32_debug_time_marker
{
  DWORD OutputPlayCursor;
  DWORD OutputWriteCursor;
  DWORD OutputLocation;
  DWORD OutputByteCount;
  DWORD ExpectedFlipPlayCursor;

  DWORD FlipPlayCursor;
  DWORD FlipWriteCursor;
};

struct win32_game_code{
  HMODULE GameCodeDll;
  FILETIME DLLLastWriteTime;


  //Note: Either of the function could be zero, must check before using
  game_update_and_render * UpdateAndRender;
  game_get_sound_samples * GetSoundSamples;
  b32 IsValid;
};

struct win32_sound_output
{
  int SamplesPerSecond;
  int BytesPerSample;
  DWORD SecondaryBufferSize;
  u32 RunningSampleIndex;
  DWORD SafetyBytes;
};

#define WIN32_STATE_FILENAME_COUNT MAX_PATH

struct win32_replay_buffer{
  HANDLE FileHandle;
  HANDLE MemoryMap;
  char Filename[WIN32_STATE_FILENAME_COUNT];
  void *MemoryBlock;
};

struct win32_state
{

  u64 TotalSize;
  void * GameMemoryBlock;

  win32_replay_buffer ReplayBuffers[4];

  HANDLE RecordingHandle;
  int InputRecordingIndex;

  HANDLE PlaybackHandle;
  int InputPlayingIndex;

  char  EXEFilename[WIN32_STATE_FILENAME_COUNT];
  char* OnePastLastEXEFileNameSlash;
};

#define WIN32_HANDMADE_H
#endif
