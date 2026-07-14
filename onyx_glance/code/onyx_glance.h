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

internal void GameUpdateAndRender(GameOffscreenBuffer *Buffer, int BlueOffset, int GreenOffset);

#define ONYXGLANCE_H
#endif
