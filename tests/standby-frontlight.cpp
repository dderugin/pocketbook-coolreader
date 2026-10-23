#include "../cr3gui/src/pbstandbyfrontlight.h"
#include "testutil.h"

static int enabled;
static int brightness;
static int enabledWrites;
static int brightnessWrites;

int GetFrontlightEnabled() { return enabled; }
void SetFrontlightEnabled(int value) { enabled = value; ++enabledWrites; }
int GetFrontlightState() { return brightness; }
void SetFrontlightState(int value) { brightness = value; ++brightnessWrites; }
void SwitchFrontlightState() { enabled = !enabled; ++enabledWrites; }

int main() {
    // Off can retain any brightness. In particular, a remembered positive
    // value must not be mistaken for the enabled state and toggled on.
    const int levels[] = {-1, 0, 37, 100};
    for (int level : levels) {
        PBStandbyFrontlight light;
        enabled = 0;
        brightness = level;
        enabledWrites = brightnessWrites = 0;
        light.resume(); // Wake without standby must do nothing.
        light.suspend();
        light.suspend();
        CHECK(enabled == 0 && enabledWrites == 0);
        light.resume();
        light.resume();
        CHECK(enabled == 0 && enabledWrites == 0);
        CHECK(brightness == level && brightnessWrites == 0);

        // Reuse the same object after the user enables the light. Duplicate
        // standby/wake notifications must retain the original enabled state.
        enabled = 1;
        light.suspend();
        light.suspend();
        CHECK(enabled == 0 && enabledWrites == 1);
        // Simulate an automatic brightness update during standby.
        brightness = 62;
        light.resume();
        light.resume();
        CHECK(enabled == 1 && enabledWrites == 2);
        CHECK(brightness == 62 && brightnessWrites == 0);

        // The next cycle must remember a subsequent manual switch off.
        enabled = 0;
        light.suspend();
        light.resume();
        CHECK(enabled == 0 && enabledWrites == 2);
    }
    // Unsupported/unavailable hardware must never be enabled by waking.
    PBStandbyFrontlight unsupported;
    enabled = -1;
    enabledWrites = 0;
    unsupported.suspend();
    unsupported.resume();
    CHECK(enabled == -1 && enabledWrites == 0);
    return 0;
}
