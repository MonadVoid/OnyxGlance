#if !defined(ONYXGLANCE_H)

// TODO: Services that platform layer provides to the game

// INFO: Services that games provides to the platform layer
struct GameOffscreenBuffer
{
    // BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};
internal void GameUpdateAndRender(GameOffscreenBuffer *Buffer, int BlueOffset, int GreenOffset);

#define ONYXGLANCE_H
#endif
