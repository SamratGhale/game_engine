#include "handmade.h"

internal void RenderWeiredGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{

  u8 *Row = (u8 *)Buffer->Memory; // u8 so we can multiply with pitch(which unit is in bytes) and get into the next row

  for (int x = 0; x < Buffer->Height; ++x)
  {
    u32 *Pixel = (u32 *)Row; // First pixel of the row

    for (int y = 0; y < Buffer->Width; ++y)
    {
      u8 Red = 0;
      u8 Green = (u8)(y + YOffset);
      u8 Blue = (u8)(x + XOffset);

      *Pixel++ = Red << 16 | Green << 8 | Blue; // PP RRi GG BB
    }

    Row += Buffer->Pitch;
  }
}

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer,int ToneHz)
{

  local_persist f32 tSine;
  s16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  s16 *SampleOut = SoundBuffer->Samples;

  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;
       ++SampleIndex)
  {
    f32 SineValue = sinf(tSine);
    s16 SampleValue = (s16)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += (2.0f * Pi32 * 1.0f) / (f32)WavePeriod;
  }
}

internal void GameUpdateAndRender(game_memory* Memory, game_input *Input, game_offscreen_buffer *Buffer,game_sound_output_buffer *SoundBuffer)
{
  debug_read_file_result FileData = DEBUGPlatformReadEntireFile(__FILE__);
  if(FileData.Contents){
    DEBUGPlatformWriteEntireFile("apple.out", FileData.ContentsSize, FileData.Contents);
    DEBUGPlatformFreeFileMemory(FileData.Contents);
  }


  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state*)Memory->PermanentStorage;


  if(!Memory->IsInitilized){
    GameState->XOffset = 0;
    GameState->YOffset = 0;
    GameState->ToneHz  = 256;
    Memory->IsInitilized = true;
  }

  game_controller_input *Input0 = &Input->Controllers[0];

  if(Input0->IsAnalog){
    GameState->XOffset += (int)(4.0f * Input0->EndX);
    GameState->ToneHz   = 256 + (int)(128.0f * (Input0->EndY));
  }else{
    //Note (samrat): Use digital movement tuning
  }
  if(Input0->Down.EndedDown){
    GameState->YOffset +=1;
  }

  GameOutputSound(SoundBuffer, GameState->ToneHz);
  RenderWeiredGradient(Buffer, GameState->XOffset, GameState->YOffset);
}
