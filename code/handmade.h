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

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
  u32 ContentsSize;
  void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal b32 DEBUGPlatformWriteEntireFile(char *Filename, u32 MemorySize, void *Memory);
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
  game_controller_input Controllers[5];
};

struct game_memory{
  u64 PermanentStorageSize;
  void * PermanentStorage; //Note(samrat): Required to be cleared to zero startup

  u64 TransientStorageSize;
  void * TransientStorage;

  b32 IsInitilized;
};

struct game_state{
  int ToneHz;
  int XOffset;
  int YOffset;
};

internal void GameUpdateAndRender(game_memory *Memory, game_input * Input, game_offscreen_buffer *Buffer);
internal void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer);

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz);
internal void RenderWeiredGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

inline game_controller_input * GetController(game_input *Input, int ControllerIndex){
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return Result;
}

#define HANDMADE_H
#endif
