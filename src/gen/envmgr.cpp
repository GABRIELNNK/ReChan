#include "common.h"
#include "gen/envmgr.h"
#include "ai/thing.h"
#include "gen/display.h"
#include "gen/model.h"

EnvironmentManager* g_environmentManager = nullptr;

EnvironmentManager::EnvironmentManager() {
    MARKFUNCTION(0x80053E3C);
    g_environmentManager = this;
}

EnvironmentManager::~EnvironmentManager() {
    MARKFUNCTION(0x80053E84);
    g_environmentManager = nullptr;
}

void EnvironmentManager::InternalOpen() {
    MARKFUNCTION(0x80053F1C);
    lighting.InternalOpen(g_display ? &g_display->GetView() : nullptr);
}

void EnvironmentManager::InternalReset() {
    MARKFUNCTION(0x80053FFC);
    lighting.Reset();
}

void EnvironmentManager::SetupEnvironment() {
    MARKFUNCTION(0x8005401C);
}

void EnvironmentManager::SetupModelAmbientLighting(ccMinList* list) {
    MARKFUNCTION(0x80054024);

    lighting.SetupLighting();

    if (!list) {
        return;
    }

    for (ccMinNode* node = list->head; node; node = node->next) {
        Thing* thing = static_cast<Thing*>(static_cast<ccNode*>(node));
        if (!thing || !thing->model) {
            continue;
        }

        Model* model = static_cast<Model*>(thing->model);
        AmbientLight* ambient = static_cast<AmbientLight*>(model->ambientLight);
        if (ambient) {
            ambient->SetToWorldAmbient();
        }
    }
}