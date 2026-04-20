// Simple XInput poller: prints XInput state for slot 0 to the console.
// Used to visually validate that the driver translates Pro Controller
// input into the expected XInput bits/axes.

#include <Windows.h>
#include <Xinput.h>
#include <stdio.h>

#pragma comment(lib, "Xinput.lib")

static void PrintState(const XINPUT_STATE* s)
{
    const XINPUT_GAMEPAD* g = &s->Gamepad;
    printf("btn=0x%04X LT=%3u RT=%3u LX=%6d LY=%6d RX=%6d RY=%6d   \r",
           g->wButtons, g->bLeftTrigger, g->bRightTrigger,
           g->sThumbLX, g->sThumbLY, g->sThumbRX, g->sThumbRY);
    fflush(stdout);
}

int main(void)
{
    printf("Polling XInput slot 0. Ctrl-C to quit.\n");
    DWORD lastPacket = 0;
    for (;;) {
        XINPUT_STATE st = {0};
        DWORD rc = XInputGetState(0, &st);
        if (rc == ERROR_SUCCESS) {
            if (st.dwPacketNumber != lastPacket) {
                PrintState(&st);
                lastPacket = st.dwPacketNumber;
            }
        } else if (rc == ERROR_DEVICE_NOT_CONNECTED) {
            printf("No controller on slot 0.                                       \r");
            fflush(stdout);
        } else {
            printf("XInputGetState rc=%lu                                          \r", rc);
            fflush(stdout);
        }
        Sleep(16);
    }
}
