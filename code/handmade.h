#if !defined(HANDMADE_H)

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

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer,int XOffset, int YOffset, game_sound_output_buffer *SoundBuffer, int ToneHz);
internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz);
internal void RenderWeiredGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

#define HANDMADE_H
#endif
