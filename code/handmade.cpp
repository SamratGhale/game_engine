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
internal void RenderPlayer(game_offscreen_buffer * Buffer, int PlayerX, int PlayerY){
  u8 * EndOfBuffer = ((u8*)Buffer->Memory + Buffer->Pitch * Buffer->Height);

  u32 Color  = 0xFFFFFFFF;
  int Left   = PlayerX;
  int Right  = PlayerX + 10;

  int Top    = PlayerY;
  int Bottom = PlayerY + 10;

  for(int x = Left; x < Right; ++x){
    u8 * Pixel = ((u8 *)Buffer->Memory + x * Buffer->BytesPerPixel + Top * Buffer->Pitch);

    for(int y = Top; y < Bottom; ++y){

      //Clamping
      if((Pixel >= Buffer->Memory) && (Pixel < EndOfBuffer)){

	*(u32*)Pixel = Color;
	Pixel += Buffer->Pitch;
      }

    }
  }
}

void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer,int ToneHz)
{

  s16 ToneVolume = 10000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  s16 *SampleOut = SoundBuffer->Samples;

  for (int SampleIndex = 0;
       SampleIndex < SoundBuffer->SampleCount;
       ++SampleIndex)
    {
      f32 SineValue = sinf(GameState->tSine);
      s16 SampleValue = (s16)(SineValue * ToneVolume);
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
      GameState->tSine += (2.0f * Pi32 * 1.0f) / (f32)WavePeriod;

      if(GameState->tSine > 2.0f * Pi32){
	GameState->tSine -= 2.0f * Pi32;
      }
    }
}

#if defined __cplusplus
extern "C"
#endif
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  debug_read_file_result FileData = Memory->DEBUGPlatformReadEntireFile(Thread, __FILE__);
  if(FileData.Contents){
    Memory->DEBUGPlatformWriteEntireFile(Thread, "apple.out", FileData.ContentsSize, FileData.Contents);
    Memory->DEBUGPlatformFreeFileMemory(Thread, FileData.Contents);
  }


  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state*)Memory->PermanentStorage;


  if(!Memory->IsInitilized){
    GameState->XOffset = 0;
    GameState->YOffset = 0;
    GameState->ToneHz  = 256;

    GameState->PlayerX = 100;
    GameState->PlayerY = 100;
    Memory->IsInitilized = true;
  }

  for (int ControllerIndex = 0;
       ControllerIndex < ArrayCount(Input->Controllers);
       ++ControllerIndex)
    {
    
      game_controller_input *Controller = GetController(Input, ControllerIndex);
      if (Controller->IsAnalog)
	{
	  // NOTE(casey): Use analog movement tuning
	  GameState->XOffset += (int)(4.0f * Controller->StickAverageX);
	  GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageY));
	}
      else
	{
	  // NOTE(casey): Use digital movement tuning
	  if (Controller->MoveLeft.EndedDown)
	    {
	      GameState->XOffset -= 1;
	    }
	
	  if (Controller->MoveRight.EndedDown)
	    {
	      GameState->XOffset += 1;
	    }
	}
    
      if(Controller->MoveDown.EndedDown)
	{
	  GameState->YOffset += 1;
	}
      GameState->PlayerX +=(int)(4.0f * Controller->StickAverageX);
      GameState->PlayerY -=(int)(4.0f * Controller->StickAverageY);

      if(GameState->tJump > 0){
	GameState->PlayerY += (int)(10.0f * sinf(0.5f * Pi32 * GameState->tJump));
      }

      if(Controller->ActionDown.EndedDown){
	GameState->tJump = 4.0f;
      }
      GameState->tJump -= 0.33f;
    }

  RenderWeiredGradient(Buffer, GameState->XOffset, GameState->YOffset);
  RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);

  if(Input->MouseButtons[0].EndedDown){
    RenderPlayer(Buffer, Input->MouseX , Input->MouseY);
  }
}

#if defined __cplusplus
extern "C"
#endif
GAME_GET_SOUND_SAMPLES(GameGetSoundSamples){
  game_state *GameState = (game_state*)Memory->PermanentStorage;
  GameOutputSound(GameState,SoundBuffer, GameState->ToneHz);
}
