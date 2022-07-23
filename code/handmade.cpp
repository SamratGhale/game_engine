#include "handmade.h"

inline s32 RoundReal32ToInt32(f32 value){
  s32 Result = (s32)(value + 0.5f);
  return (Result);
}

inline s32 RoundReal32ToUint32(f32 value){
  u32 Result = (u32)(value + 0.5f);
  return (Result);
}

inline s32 TruncateReal32ToInt32(f32 Real32){
  s32 Result = (s32)Real32;
  return Result;
}

struct tile_map{
  s32 MapCountX;
  s32 MapCountY;

  f32 UpperLeftX;
  f32 UpperLeftY;
  f32 TileHeight;
  f32 TileWidth;
  u32 * Tiles;
};

internal b32 IsTileMapPointEmpty(tile_map * TileMap, f32 TestX, f32 TestY){

  b32 Empty = false;
  s32 PlayerTileX = TruncateReal32ToInt32((TestX - TileMap->UpperLeftX)/TileMap->TileWidth);
  s32 PlayerTileY = TruncateReal32ToInt32((TestY - TileMap->UpperLeftY)/TileMap->TileHeight);

  if((PlayerTileX > 0) && (PlayerTileX < TileMap->MapCountX) &&
     (PlayerTileY > 0) && (PlayerTileY < TileMap->MapCountY))
    {
      u32 TileMapValue = TileMap->Tiles[TileMap->MapCountX *  PlayerTileY + PlayerTileX];
      Empty = (TileMapValue == 0);
    }
  return Empty;
}

internal void DrawRectangle(game_offscreen_buffer * Buffer, f32 RealMinX,
			    f32 RealMinY, f32 RealMaxX, f32 RealMaxY, f32 R, f32 G, f32 B){

  s32 MinX = RoundReal32ToInt32(RealMinX);
  s32 MinY = RoundReal32ToInt32(RealMinY);
  s32 MaxX = RoundReal32ToInt32(RealMaxX);
  s32 MaxY = RoundReal32ToInt32(RealMaxY);

  if(MinX <0){
    MinX = 0;
  }
  if(MinY  < 0){
    MinY = 0;
  }

  if(MaxX  > Buffer->Width){
    MaxX = Buffer->Width;
  }

  if(MaxY  > Buffer->Height){
    MaxY = Buffer->Height;
  }

  u32 Color = (RoundReal32ToUint32(R * 255.0f) << 16) |
    (RoundReal32ToUint32(G * 255.0f) << 8)  |
    (RoundReal32ToUint32(B * 255.0f) << 0);

  u8 * Row = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

  for(int Y = MinY; Y < MaxY; ++Y){

    u32 * Pixel = (u32 *)Row;
    for(int X = MinX; X < MaxX; ++X){
      *Pixel++ = Color;
    }
    Row += Buffer->Pitch;
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
#if 0 
      f32 SineValue = sinf(GameState->tSine);
      s16 SampleValue = (s16)(SineValue * ToneVolume);
#endif
#if 0
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
      GameState->tSine += (2.0f * Pi32 * 1.0f) / (f32)WavePeriod;

      if(GameState->tSine > 2.0f * Pi32){
	GameState->tSine -= 2.0f * Pi32;
      }
#endif
    }
}

#if defined __cplusplus
extern "C"
#endif
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state*)Memory->PermanentStorage;

  if(!Memory->IsInitilized){
    GameState->PlayerX = 150;
    GameState->PlayerY = 150;
    Memory->IsInitilized = true;
  }

  DrawRectangle(Buffer, 0.0f, 0.0f, (f32)Buffer->Width, (f32)Buffer->Height, 1.0f, .5f, 0.0f);


#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9

  tile_map TileMap = {};

  TileMap.MapCountX  = TILE_MAP_COUNT_X;
  TileMap.MapCountY  = TILE_MAP_COUNT_Y;

  TileMap.UpperLeftX = -30;
  TileMap.UpperLeftY = 0;
  TileMap.TileHeight = 60;
  TileMap.TileWidth  = 60;

  f32 PlayerWidth  = (f32)(0.75 * TileMap.TileWidth);
  f32 PlayerHeight = (f32)(0.75 * TileMap.TileHeight);

  s32 Tiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
  };

  TileMap.Tiles = (u32*)Tiles;
  for(int Row = 0; Row < TILE_MAP_COUNT_Y; Row++){
    for(int Column = 0; Column < TILE_MAP_COUNT_X; Column++){

      u32 TileId = Tiles[Row][Column];
      f32 Grey = .5f;
      if(TileId == 1){
	Grey = 1.0f;
      }
      f32 MinX = TileMap.UpperLeftX + ((f32)Column) * TileMap.TileWidth ;
      f32 MinY = TileMap.UpperLeftY + ((f32)Row)    * TileMap.TileHeight;

      f32 MaxX = MinX + TileMap.TileWidth;
      f32 MaxY = MinY + TileMap.TileHeight;

      DrawRectangle(Buffer, MinX , MinY , MaxX , MaxY, Grey, Grey, Grey);
    }
  }

  for (int ControllerIndex = 0;
       ControllerIndex < ArrayCount(Input->Controllers);
       ++ControllerIndex){
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if(Controller->IsAnalog){
    }else{
      f32 dPlayerX = 0.0f;
      f32 dPlayerY = 0.0f;

      if(Controller->MoveUp.EndedDown){
	dPlayerY = -1.0f;
      }
      if(Controller->MoveDown.EndedDown){
	dPlayerY = 1.0f;
      }
      if(Controller->MoveLeft.EndedDown){
	dPlayerX = -1.0f;
      }
      if(Controller->MoveRight.EndedDown){
	dPlayerX = 1.0f;
      }
      dPlayerX *= 64.0f;
      dPlayerY *= 64.0f;
      f32 NewPlayerX = GameState->PlayerX + (f32)(Input->dtForFrame) * dPlayerX;
      f32 NewPlayerY = GameState->PlayerY + (f32)(Input->dtForFrame) * dPlayerY;
      if(IsTileMapPointEmpty(&TileMap, (f32)(NewPlayerX - 0.5 *PlayerWidth), NewPlayerY) &&
	 IsTileMapPointEmpty(&TileMap, (f32)(NewPlayerX + 0.5 *PlayerWidth), NewPlayerY) && IsTileMapPointEmpty(&TileMap, NewPlayerX, NewPlayerY) ){
	GameState->PlayerX = NewPlayerX;
	GameState->PlayerY = NewPlayerY;
      }
    }
  }
  f32 PlayerR = 1.0f;
  f32 PlayerG = 0.0f;
  f32 PlayerB = 0.0f;
  f32 PlayerTop    = GameState->PlayerY - PlayerHeight; 
  f32 PlayerLeft   = (f32)(GameState->PlayerX - 0.5 * PlayerWidth);
  DrawRectangle(Buffer, PlayerLeft , PlayerTop , PlayerLeft + PlayerWidth , PlayerTop + PlayerHeight, PlayerR , PlayerG, PlayerB);
}

#if defined __cplusplus
extern "C"
#endif
GAME_GET_SOUND_SAMPLES(GameGetSoundSamples){
  game_state *GameState = (game_state*)Memory->PermanentStorage;
  GameOutputSound(GameState,SoundBuffer, 400);
}

/*
internal void RenderWeiredGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{

  u8 *Row = (u8 *)Buffer->Memory; // u8 so we can multiply with pitch(which unit is in bytes) and get into the next row

  for (int x = 0; x < Buffer->Height; ++x)
  {
    u32 *Pixel = (u32 *)Row; // First pixel of the row

    for (int y = 0; y < Buffer->Width; ++y)
    {
      u8 Red = 128;
      u8 Green = (u8)(y + YOffset);
      u8 Blue = (u8)(x + XOffset);

      *Pixel++ = Red << 16 | Green << 8 | Blue; // PP RRi GG BB
    }

    Row += Buffer->Pitch;
  }
}
*/
