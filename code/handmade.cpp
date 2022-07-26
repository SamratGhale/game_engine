#include "handmade.h"

inline s32 RoundReal32ToInt32(f32 value){
  s32 Result = (s32)(value + 0.5f);
  return (Result);
}

inline s32 RoundReal32ToUint32(f32 value){
  u32 Result = (u32)(value + 0.5f);
  return (Result);
}
//NOTE(samrat): how to implement these math function
#include "math.h"
inline s32 FloorReal32ToInt32(f32 Real32){
  s32 Result = (s32)floorf(Real32);
  return Result;
}

inline s32 TruncateReal32ToInt32(f32 Real32){
  s32 Result = (s32)Real32;
  return Result;
}

internal void DrawRectangle(game_offscreen_buffer * Buffer, f32 RealMinX, f32 RealMinY, f32 RealMaxX, f32 RealMaxY, f32 R, f32 G, f32 B){
  
  s32 MinX = RoundReal32ToInt32(RealMinX);
  s32 MinY = RoundReal32ToInt32(RealMinY);
  s32 MaxX = RoundReal32ToInt32(RealMaxX);
  s32 MaxY = RoundReal32ToInt32(RealMaxY);
  
  if(MinX <0){
    MinX = 0;
  }
  if(MinY < 0){
    MinY = 0;
  }
  
  if(MaxX > Buffer->Width){
    MaxX = Buffer->Width;
  }
  
  if(MaxY > Buffer->Height){
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

inline tile_map * GetTilemap(world * World, s32 TileMapX, s32 TileMapY){
  tile_map *Result = 0;
  if((TileMapX >= 0) && (TileMapX < World->TileMapCountX) &&
     (TileMapY >= 0) && (TileMapY < World->TileMapCountY)){
    Result = &World->Tiles[TileMapY * World->TileMapCountX + TileMapX];
  }
  return Result;
}

inline u32 GetTileValueUnchecked(world * OurWorld, tile_map * TileMap, s32 TileX, s32 TileY){
  u32 TileMapValue = TileMap->Tiles[TileX + OurWorld->CountX * TileY];
  return TileMapValue;
}


internal b32 IsTileMapPointEmpty(world * World, tile_map * TileMap, s32 TestX, s32 TestY){
  b32 Empty = false;
  
  if((TestX >= 0) && (TestX < World->CountX) &&
     (TestY >= 0) && (TestY < World->CountY))
  {
    u32 TileMapValue = TileMap->Tiles[World->CountX *  TestY + TestX ];
    Empty = (TileMapValue == 0);
  }
  return Empty;
}
inline canonical_position
GetCanonicalPosition(world * World, raw_position Pos){
  canonical_position Result = {};
  
  Result.TileMapX = Pos.TileMapX;
  Result.TileMapY = Pos.TileMapY;
  
  f32 X = (Pos.X - World->UpperLeftX);
  f32 Y = (Pos.Y - World->UpperLeftY);
  
  Result.TileX = FloorReal32ToInt32(X/World->TileWidth);
  Result.TileY = FloorReal32ToInt32(Y/World->TileHeight);
  
  Result.X = X - Result.TileX * World->TileWidth;
  Result.Y = Y - Result.TileY * World->TileHeight;
  
  Assert(Result.X >=0);
  Assert(Result.Y >=0);
  Assert(Result.X < World->TileWidth);
  Assert(Result.Y < World->TileHeight);
  
  if(Result.TileX < 0){
    Result.TileX = World->CountX + Result.TileX;
    --Result.TileMapX;
  }
  
  if(Result.TileY < 0){
    Result.TileY = World->CountY + Result.TileY;
    --Result.TileMapY;
  }
  
  if(Result.TileX >= World->CountX){
    Result.TileX = Result.TileX - World->CountX;
    ++Result.TileMapX;
  }
  
  if(Result.TileY >= World->CountY){
    Result.TileY = Result.TileY - World->CountY;
    ++Result.TileMapY;
  }
  return Result;
}

//Checks if the point in the tile is moveable or not (is it 0 or one)
internal b32 IsWorldPointEmpty(world * World, raw_position Pos) {
  
  canonical_position CanPos = GetCanonicalPosition(World, Pos);
  b32 Empty = false;
  tile_map * TileMap = GetTilemap(World, CanPos.TileMapX, CanPos.TileMapY);
  
  Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX , CanPos.TileY);
  return Empty;
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
    GameState->PlayerTileMapX = 0;
    GameState->PlayerTileMapY = 0;
    Memory->IsInitilized = true;
  }
  
#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
  
  u32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
    {1, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
  };
  
  u32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
  };
  
  u32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
  };
  
  u32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
  };
  
  tile_map TileMaps[2][2];
  TileMaps[0][0].Tiles     = (u32 *)Tiles00;
  TileMaps[0][1].Tiles     = (u32 *)Tiles10;
  TileMaps[1][0].Tiles     = (u32 *)Tiles01;
  TileMaps[1][1].Tiles     = (u32 *)Tiles11;
  
  world World = {};
  World.CountX        = TILE_MAP_COUNT_X;
  World.CountY        = TILE_MAP_COUNT_Y;
  World.UpperLeftX    = -30;
  World.UpperLeftY    = 0;
  World.TileMapCountX = 2;
  World.TileMapCountY = 2;
  World.TileHeight    = 60;
  World.TileWidth     = 60;
  World.Tiles = (tile_map*)TileMaps; 
  
  f32 PlayerWidth  = (f32)(0.75 * World.TileWidth);
  f32 PlayerHeight = (f32)(0.75 * World.TileHeight);
  
  tile_map * TileMap = GetTilemap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
  Assert(TileMap);
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
      
      raw_position PlayerPos = {};
      PlayerPos.TileMapX = GameState->PlayerTileMapX;
      PlayerPos.TileMapY = GameState->PlayerTileMapY;
      PlayerPos.X = NewPlayerX;
      PlayerPos.Y = NewPlayerY;
      
      raw_position PlayerLeft = PlayerPos;
      PlayerLeft.X -= 0.5f * PlayerWidth;
      
      raw_position PlayerRight = PlayerPos;
      PlayerRight.X += 0.5f * PlayerWidth;
      
      
      
      if(IsWorldPointEmpty(&World, PlayerPos) &&
         IsWorldPointEmpty(&World, PlayerLeft) &&
         IsWorldPointEmpty(&World, PlayerRight)){
        
        canonical_position CurPos = GetCanonicalPosition(&World, PlayerPos);
        GameState->PlayerTileMapX = CurPos.TileMapX;
        GameState->PlayerTileMapY = CurPos.TileMapY;
        
        GameState->PlayerX = World.UpperLeftX + World.TileWidth * CurPos.TileX + CurPos.X;
        GameState->PlayerY = World.UpperLeftY + World.TileHeight* CurPos.TileY + CurPos.Y;
      }
    }
  }
  
  DrawRectangle(Buffer, 0.0f, 0.0f, (f32)Buffer->Width, (f32)Buffer->Height, 1.0f, .5f, 0.0f);
  
  
  for(int Row = 0; Row < TILE_MAP_COUNT_Y; Row++){
    for(int Column = 0; Column < TILE_MAP_COUNT_X; Column++){
      u32 TileId = GetTileValueUnchecked(&World, TileMap, Column, Row);
      f32 Grey = .5f;
      if(TileId == 1){
        Grey = 1.0f;
      }
      f32 MinX = World.UpperLeftX + ((f32)Column) * World.TileWidth ;
      f32 MinY = World.UpperLeftY + ((f32)Row)    * World.TileHeight;
      
      f32 MaxX = MinX + World.TileWidth;
      f32 MaxY = MinY + World.TileHeight;
      
      DrawRectangle(Buffer, MinX , MinY , MaxX , MaxY, Grey, Grey, Grey);
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
