#pragma once

namespace DebugUI {
    void Init();
    void Draw();
    bool IsEnabled();
    bool IsHumanoidDamageDisabled();
    bool ShouldBlockGameInput();
    bool IsPlayerInputAllowed();
    bool IsDebugCameraInputAllowed();
}
