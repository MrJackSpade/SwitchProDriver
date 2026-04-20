#pragma once

#include <windows.h>

// Runs the Pro Controller -> ViGEm pipe until *StopFlag becomes nonzero.
// OnStatus is optional; if set, receives short human-readable status lines
// ("waiting for controller", "connected", "ViGEm init failed", etc.) suitable
// for console output or event log entries.
typedef void (*StatusCallback)(const char* msg, void* ctx);

int WorkerRun(volatile LONG* StopFlag, StatusCallback OnStatus, void* StatusCtx);
