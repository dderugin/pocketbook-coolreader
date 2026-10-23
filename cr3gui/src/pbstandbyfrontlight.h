#ifndef PB_STANDBY_FRONTLIGHT_H
#define PB_STANDBY_FRONTLIGHT_H

#include <inkview.h>

// FW6 remembers brightness even when the light is disabled. Standby must use
// the separate enabled flag, leaving brightness, colour and auto mode intact.
class PBStandbyFrontlight {
    bool _suspended = false;
    bool _restoreEnabled = false;
public:
    void suspend() {
        if (_suspended) return;
        _restoreEnabled = GetFrontlightEnabled() > 0;
        _suspended = true;
        if (_restoreEnabled) SetFrontlightEnabled(0);
    }
    void resume() {
        if (!_suspended) return;
        if (_restoreEnabled) SetFrontlightEnabled(1);
        _restoreEnabled = false;
        _suspended = false;
    }
};

#endif
