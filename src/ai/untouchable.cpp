#include "ai/untouchable.h"
#include "gen/common.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/particle.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"

#include "p3d/context.h"
#include "p3d/p3dmath.h"
#include "snd/esound.h"
#include "snd/sndfact.h"

Untouchable::Untouchable(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x800A6330);
    particleMgr = new ParticleSystemMgr();
    soundPtr = nullptr;
}

Untouchable::~Untouchable() {
    MARKFUNCTION(0x800A6380);

    ReleaseSound();
    delete particleMgr;
    particleMgr = nullptr;
}

void Untouchable::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800A63DC);
    Obstacle::AnalyzeMesh(root);

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    FillCollisionBox(localBox, *static_cast<const DBVolume*>(root));
    SetCollisionBox(localBox);

    const DBAttrib* attrib = root->FindAttrib(6);
    damageType = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 2;

    attrib = root->FindAttrib(7);
    damageValue = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 3;

}

void Untouchable::CreateModel(const char* name) {
    MARKFUNCTION(0x800A64C8);
    (void)name;
    flags |= TF_MODEL_CREATED;
}

void Untouchable::DeleteModel() {
    MARKFUNCTION(0x800A64DC);
    flags &= ~TF_MODEL_CREATED;
}

void Untouchable::Reset() {
    MARKFUNCTION(0x800A64F0);
    field148 = 0;
    pendingCreate = 0;
    countdownTimer = damageValue;

    if (particleMgr) {
        particleMgr->InitMgr(ParticleSystem_Find(60675069));
        particleMgr->PurgeParticles();
        particleMgr->ResetParticleDirection();
    }

    ReleaseSound();
}

void Untouchable::Think() {
    MARKFUNCTION(0x800A6540);

    if (countdownTimer <= 0) {
        countdownTimer = damageValue;
    }
    countdownTimer -= 1;

    if (!field148) {
        return;
    }

    if (particleMgr) {
        if (pendingCreate) {
            const LVector origin = { 0, 0, 0 };
            particleMgr->CreateParticles(origin, nullptr);
            pendingCreate = 0;
        }

        particleMgr->Update();
        if (soundPtr) {
            soundPtr->StartAnimating();
        }

        if (!particleMgr->ActiveParticles()) {
            field148 = 0;
            particleMgr->PurgeParticles();
            particleMgr->ResetParticleDirection();
            ReleaseSound();
        }
    }
}

void Untouchable::Draw() {
    MARKFUNCTION(0x800A6604);

    if (!field148 || !particleMgr || !p3d::context) {
        return;
    }

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    Mat4 world;
    LVector effectPos = { effectPosX, effectPosY, effectPosZ };
    p3dBuildTransMatrix(effectPos.x, effectPos.y, effectPos.z, world);
    p3d::context->SetWorldMatrix(world);
    particleMgr->Display();
    p3d::context->SetWorldMatrix(savedWorld);
}

void Untouchable::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800A6694);

    if (!hum || !particleMgr) {
        return;
    }

    switch (hum->actionState) {
        case 24:
        case 65:
        case 68:
        case 72:
            return;
        case 30:
            hum->combatFlag |= 8u;
            if (countdownTimer <= 0) {
                hum->SubtractHitPoints(static_cast<u16>(damageValue));
                if (!hum->field344) {
                    hum->LoadDialog(0x39, 0x40000100);
                    hum->SetActionState(72, 0);
                }
            }

            pendingCreate = 1;
            effectPosX = hum->pos.x;
            effectPosY = hum->pos.y;
            effectPosZ = hum->pos.z;
            pos = hum->pos;
            break;
        default:
            if (((hum->field368 >> 12) & 1) != 0) {
                if (countdownTimer <= 0) {
                    hum->SubtractHitPoints(static_cast<u16>(damageValue));
                }

                hum->SetActionState(30, 0);
                hum->combatFlag |= 8u;

                effectPosX = hum->pos.x;
                effectPosY = hum->pos.y;
                effectPosZ = hum->pos.z;
                pos = hum->pos;

                LVector direction = { 0x4000, 0, 0 };
                particleMgr->SetParticleDirection(&direction);

                pendingCreate = 1;
                field148 = 1;

                CreateSound();
            }
            break;
    }
}

s32 Untouchable::CreateSound() {
    MARKFUNCTION(0x800A68FC);

    if (soundPtr || !particleMgr) {
        return 0;
    }

    CSound* created = nullptr;
    const s32 result = CSoundFactory::CreateObject(10000, &created, 60675069);
    if (result < 0 || !created) {
        return result;
    }

    soundPtr = static_cast<CParticleEffectSound*>(created);
    return soundPtr->Initialize(&pos);
}

s32 Untouchable::ReleaseSound() {
    if (!soundPtr) {
        return 0;
    }

    soundPtr->StopAnimating();
    soundPtr->Release();
    soundPtr = nullptr;
    return 0;
}
