#pragma once
#include "gen/manager.h"
#include "gen/lights.h"

struct ccMinList;

class EnvironmentManager : public Manager {
public:
    LightingClass lighting;

    EnvironmentManager();
    ~EnvironmentManager() override;

    void InternalOpen() override;
    void InternalReset() override;

    void SetupEnvironment();
    void SetupModelAmbientLighting(ccMinList* list);
};

extern EnvironmentManager* g_environmentManager;