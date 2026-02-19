// Copyright 2024 Normal Browser Authors. All rights reserved.
// anti_bot_markers.h — Declarations for anti-bot and anti-detection markers.

#ifndef NICEBROWSER_ANTI_BOT_MARKERS_H_
#define NICEBROWSER_ANTI_BOT_MARKERS_H_

#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace blink {

// Clears all per-site permission decisions so every permission.query()
// returns 'prompt', matching a fresh browser install.
void ResetAllPermissionsForGhostMode(HostContentSettingsMap* settings_map);

// Returns true if the given filesystem path should be blocked (return
// ENOENT) to prevent root/Frida/debugger detection.
bool ShouldBlockProcAccess(const char* path);

}  // namespace blink

#endif  // NICEBROWSER_ANTI_BOT_MARKERS_H_
