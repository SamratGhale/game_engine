#if !defined(HANDMADE_H)

/*
  Note(sarmat): Custom compiler flags
  HANDMADE_INTERNAL:
  0 - Build for public release
  1 - Build for developer only

  HANDMADE_SLOW:
  0 - No Slow code allowed
  1 - Slow code welcome
*/
#include <math.h>
#include <stdint.h>

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


struct thread_context{
  int PlaceHolder;
};

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
  u32 ContentsSize;
  void *Contents;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context * Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context * Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(thread_context * Thread, char *Filename, u32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
#endif

#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)){*(int*)0=0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)


#define ArrayCount(Array)(sizeof(Array)/sizeof((Array)[0]))

inline u32 SafeTruncateUint64(u64 Value){
  Assert(Value <= 0xFFFFFFFF);
  u32 Result = (u32)Value;
  return (Result);
}


struct game_offscreen_buffer
{
  void *     Memory;  
  int        Pitch;
  int        Width;
  int        Height;
  int        BytesPerPixel;
};

struct game_sound_output_buffer{
  int SamplesPerSecond;
  s16 *Samples;
  int SampleCount;
};

struct game_button_state{
  s32 HalfTransitionCount;
  b32 EndedDown;
};

struct game_controller_input{
  b32 IsConnected;
  b32 IsAnalog;

  f32 StickAverageX;
  f32 StickAverageY;

  union{
    game_button_state Buttons[12];
    struct{
      game_button_state MoveUp;
      game_button_state MoveDown;
      game_button_state MoveLeft;
      game_button_state MoveRight;

      game_button_state ActionUp;
      game_button_state ActionDown;
      game_button_state ActionLeft;
      game_button_state ActionRight;

      game_button_state LeftShoulder;
      game_button_state RightShoulder;

      game_button_state Back;
      game_button_state Start;
    };
  };
};

struct game_input{
  game_button_state MouseButtons[5];
  s32 MouseX, MouseY, MouseZ;
  game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex){
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return (Result);
}

struct game_memory{
  u64 PermanentStorageSize;
  void * PermanentStorage; //Note(samrat): Required to be cleared to zero startup

  u64 TransientStorageSize;
  void * TransientStorage;

  b32 IsInitilized;

  debug_platform_free_file_memory  *DEBUGPlatformFreeFileMemory;
  debug_platform_read_entire_file  *DEBUGPlatformReadEntireFile;
  debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile; 
};

struct game_state{
  int ToneHz;
  int XOffset;
  int YOffset;

  f32 tSine;
  int PlayerX;
  int PlayerY;

  f32 tJump;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context * Thread, game_memory *Memory, game_input * Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context * Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

void GameOutputSound(game_state* gameState, game_sound_output_buffer *SoundBuffer, int ToneHz);
internal void RenderWeiredGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset);
internal void RenderWeiredGradient(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY);

game_controller_input * GetController(game_input *Input, int ControllerIndex){
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return Result;
}

#define HANDMADE_H
#endif
