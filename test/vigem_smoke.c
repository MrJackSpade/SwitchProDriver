// Standalone smoke test: connects to ViGEmBus, plugs a virtual X360 pad,
// holds A then rotates dpad for ~5s. Validates Phase 0 stack end-to-end
// before the real driver is written.
//
// Build via scripts\build_tests.cmd. Requires ViGEmBus runtime installed.

#include <Windows.h>
#include <stdio.h>
#include <ViGEm/Client.h>

int main(void)
{
    PVIGEM_CLIENT client = vigem_alloc();
    if (!client) {
        fprintf(stderr, "vigem_alloc failed\n");
        return 1;
    }

    VIGEM_ERROR err = vigem_connect(client);
    if (!VIGEM_SUCCESS(err)) {
        fprintf(stderr, "vigem_connect failed: 0x%08X (ViGEmBus installed?)\n", err);
        vigem_free(client);
        return 2;
    }

    PVIGEM_TARGET pad = vigem_target_x360_alloc();
    err = vigem_target_add(client, pad);
    if (!VIGEM_SUCCESS(err)) {
        fprintf(stderr, "vigem_target_add failed: 0x%08X\n", err);
        vigem_target_free(pad);
        vigem_disconnect(client);
        vigem_free(client);
        return 3;
    }

    printf("Virtual X360 pad plugged. Cycling dpad + A for ~5s. Watch joy.cpl.\n");

    const USHORT seq[] = {
        XUSB_GAMEPAD_DPAD_UP | XUSB_GAMEPAD_A,
        XUSB_GAMEPAD_DPAD_RIGHT | XUSB_GAMEPAD_A,
        XUSB_GAMEPAD_DPAD_DOWN | XUSB_GAMEPAD_A,
        XUSB_GAMEPAD_DPAD_LEFT | XUSB_GAMEPAD_A,
    };
    for (int i = 0; i < 20; ++i) {
        XUSB_REPORT r;
        XUSB_REPORT_INIT(&r);
        r.wButtons = seq[i % 4];
        r.sThumbLX = (SHORT)((i % 2) ? 20000 : -20000);
        vigem_target_x360_update(client, pad, r);
        Sleep(250);
    }

    vigem_target_remove(client, pad);
    vigem_target_free(pad);
    vigem_disconnect(client);
    vigem_free(client);
    printf("Smoke test done.\n");
    return 0;
}
