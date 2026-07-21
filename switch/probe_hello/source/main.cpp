#include <switch.h>
#include <cstdio>

int main(int argc, char* argv[]) {
    consoleInit(NULL);

    printf("ReChan Switch toolchain probe\n");
    printf("If you can read this, the vendored devkitA64/libnx toolchain\n");
    printf("and elf2nro/nacptool packaging pipeline all work.\n\n");
    printf("Press PLUS to exit.\n");

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Plus) break;

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
