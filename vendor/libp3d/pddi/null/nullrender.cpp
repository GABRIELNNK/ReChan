#include "pddi/null/nullrender.h"

#include <cstdio>
#include <cstdlib>

bool nullDisplay::InitDisplay(const pddiDisplayInit& init) {
    width = init.xSize;
    height = init.ySize;

    if (const char* env = std::getenv("RECHAN_HEADLESS_MAX_FRAMES")) {
        maxFrames = static_cast<u64>(std::strtoull(env, nullptr, 10));
    }

    std::printf("nullDisplay: headless mode (%llu max frame(s), 0 = unbounded)\n",
                static_cast<unsigned long long>(maxFrames));
    return true;
}

void nullDisplay::PollEvents() {
    ++frameCount;
}

bool nullDisplay::ShouldClose() {
    return maxFrames != 0 && frameCount >= maxFrames;
}

pddiDevice* pddiCreate() {
    return new nullDevice();
}
