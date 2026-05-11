#pragma once

#include <windows.h>

typedef struct _SVC_CONFIG {
    // 0..100. Applied to the post-calibration XInput axis value. 0 disables.
    int  LeftStickDeadzonePct;
    int  RightStickDeadzonePct;
    // FALSE: position-mapped (Pro's bottom button -> Xbox A regardless of label).
    // TRUE:  label-mapped (Pro A -> Xbox A even though they sit on opposite sides).
    // Effectively swaps both A<->B and X<->Y at the output stage.
    BOOL SwapFaceButtons;
} SVC_CONFIG;

// Loads switchprosvc.ini from the directory containing the running .exe.
// Missing file / missing keys fall back to defaults (all zero / FALSE).
// Out path receives the resolved INI path (may be used for logging); pass NULL to ignore.
void SvcConfigLoad(SVC_CONFIG* out, wchar_t* outIniPath, size_t outIniPathChars);
