#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define local_persist static
#define global_variable static
#define internal static

#define Pi32 3.141592653589793

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

struct win32_off_screen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

// TODO this is global for now
global_variable bool GlobalRunning;
global_variable win32_off_screen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

struct win32_window_dimension
{
    int Width;
    int Height;
};

// NOTE(): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXinput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.h");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput1_3.h");
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else
    {
        // FIX: Diagnostic
    }
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // NOTE: Load the Library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary)
    {
        // NOTE: Get a DirectSound Object - cooperative

        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            // NOTE: "Create" a primary Buffer
            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // NOTE: maybe DSBCAPS_GLOBALFOCUS??
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if (SUCCEEDED(Error))
                    {
                        // We have finally set the format
                    }
                    else
                    {
                        // FIX: Diagnostic
                    }
                }
                else
                {
                    // FIX: Diagnostic
                }
            }
            else
            {
                // FIX: Diagnostic
            }

            // NOTE: "Create" a secondary Buffer
            // TODO() DSBCAPS_GETCURRENTPOSITION2
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(Error))
            {
                // NOTE: Start it playing
            }
        }
        else
        {
            // FIX: Diagnostic
        }
    }
    else
    {
        // FIX: Diagnostic
    }
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Height = ClientRect.bottom - ClientRect.top;
    Result.Width = ClientRect.right - ClientRect.left;

    return Result;
}

internal void
RenderWeirdGradient(win32_off_screen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;

    for (int Y = 0; Y < Buffer->Height; Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; X++)
        {
            // Pixel in memory BBGGRRxx
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);
            *Pixel++ = (Green << 8 | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void Win32ResizeDIBSection(win32_off_screen_buffer *Buffer, int Width, int Height)
{
    // TODO: Bulletproof this
    // Maybe dont free first, free after, then free first if that fails
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // NOTE: Chris Hecker clarified the deal with stretchDIbits and BitBlt!
    // No more DC for us
    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    // TODO Clear to Black
    Buffer->Pitch = BytesPerPixel * Buffer->Width;
}

internal void Win32DisplayBufferinWindow(win32_off_screen_buffer *Buffer,
                                         HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    // TODO: Aspect Ratio Correction
    // TODO: Play with stretch modes
    StretchDIBits(DeviceContext,
                  // X, Y, Width, Height,
                  // X, Y, Width, Height,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory, &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result;
    switch (Message)
    {
        case WM_SIZE:
        {
            OutputDebugString("WM_SIZE\n");
        }
        break;

        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user ???
            GlobalRunning = false;
        }
        break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugString("WM_ACTIVATEAPP\n");
        }
        break;

        case WM_DESTROY:
        {
            // TODO: Handle this is an error -> Recreate Window ?
            GlobalRunning = false;
        }
        break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);

            if (WasDown != IsDown)
            {
                if (VKCode == 'W')
                {
                }
                else if (VKCode == 'A')
                {
                }
                else if (VKCode == 'S')
                {
                }
                else if (VKCode == 'D')
                {
                }
                else if (VKCode == 'Q')
                {
                }
                else if (VKCode == 'E')
                {
                }
                else if (VKCode == VK_UP)
                {
                }
                else if (VKCode == VK_LEFT)
                {
                }
                else if (VKCode == VK_DOWN)
                {
                }
                else if (VKCode == VK_RIGHT)
                {
                }
                else if (VKCode == VK_ESCAPE)
                {
                }
                else if (VKCode == VK_SPACE)
                {
                }
            }

            bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
            if (VKCode == VK_F4 && AltKeyWasDown)
            {
                GlobalRunning = false;
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32ResizeDIBSection(&GlobalBackBuffer, Dimension.Width, Dimension.Height);
            Win32DisplayBufferinWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

            EndPaint(Window, &Paint);
        }
        break;

        default:
        {
            // OutputDebugString("default");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        }
        break;
    }

    return Result;
}

struct win32_sound_output
{
    int SamplesPerSecond;
    int ToneHz;
    int16 ToneVolume;
    uint32 RunningSampleIndex;
    int BytesPerSample;
    int WavePeriod;
    int SecondaryBufferSize;
};

internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
            ByteToLock,
            BytesToWrite,
            &Region1, &Region1Size,
            &Region2, &Region2Size,
            0)))
    {

        // TODO Assert that Region Size is Valid
        int16 *SampleOut = (int16 *)Region1;
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;

        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
        {
            real32 t = 2.0f * Pi32 * (real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
            real32 SineValue = sinf(t);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            SoundOutput->RunningSampleIndex++;
        }

        SampleOut = (int16 *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
        {
            real32 t = 2.0f * Pi32 * (real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
            real32 SineValue = sinf(t);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            SoundOutput->RunningSampleIndex++;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CommandLine, int ShowCode)
{
    Win32LoadXinput();
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WNDCLASS WindowClass = {};
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "OnyxGlanceWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window =
            CreateWindowEx(0, WindowClass.lpszClassName, "Onyx Glance", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

        if (Window)
        {
            HDC DeviceContext = GetDC(Window);

            // NOTE Graphics Test
            int Xoffset = 0;
            int Yoffset = 0;

            // NOTE Sound Test
            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.ToneHz = 256;
            SoundOutput.ToneVolume = 3000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;
            while (GlobalRunning)
            {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                // TODO should we poll more frequently
                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
                {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        // NOTE Controller plugged in
                        // TODO See if ControllerState.dwPacketNumber increments too rapidly
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;
                    }
                    else
                    {
                        // Controller not plugged in
                    }
                }
                RenderWeirdGradient(&GlobalBackBuffer, Xoffset, Yoffset);

                // NOTE: Direct Sound output test
                DWORD PlayCursor;
                DWORD WriteCursor;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(
                        &PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                    DWORD BytesToWrite;

                    // TODO(): Change this to lower latency offset from the playcursor
                    // when we actually start having sound effects
                    if (ByteToLock == PlayCursor)
                    {
                        BytesToWrite = 0;
                    }
                    else if (ByteToLock > PlayCursor)
                    {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }
                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
                }

                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferinWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

                ++Xoffset;
                ++Yoffset;
            }
        }
    }
    else
    {
        // TODO: add logging system
    }
    return 0;
}
