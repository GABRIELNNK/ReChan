#pragma once

namespace CrashReporter {
    // Installs process-wide fatal exception and signal handlers. Crash reports are
    // written beside the executable's working data and never uploaded automatically.
    void Install();

    // Intentional fatal signal used to verify packaging and report permissions.
    [[noreturn]] void TriggerTestCrash();
}
