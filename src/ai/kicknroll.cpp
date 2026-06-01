#include "ai/kicknroll.h"
#include "ai/humanoid.h"
#include "ai/pickup.h"
#include "ai/player.h"
#include "gen/common.h"
#include "gen/ai.h"
#include "gen/animmgr.h"
#include "gen/animstruct.h"
#include "gen/database.h"
#include "gen/geffect.h"
#include "gen/model.h"
#include "gen/scoremgr.h"
#include "p3d/hash.h"
#include "ai/obstacle_shared.h"
#include "snd/kndnsnd.h"
#include "snd/rsevent.h"
#include "snd/sndfact.h"
#include "p3d/skeleton.h"
#include <cstdio>

KickNRoll::KickNRoll(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001C398);
    sound = nullptr;
    aliveFlag = 1;
}

KickNRoll::~KickNRoll() {
    MARKFUNCTION(0x8001C3DC);
}

void KickNRoll::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001C444);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void KickNRoll::CreateModel(const char* name) {
    MARKFUNCTION(0x8001C6FC);
    Obstacle::CreateModel(name);
}

void KickNRoll::DeleteModel() {
    MARKFUNCTION(0x8001C750);
    Obstacle::DeleteModel();
}

void KickNRoll::Reset() {
    MARKFUNCTION(0x8001C7A0);
}

void KickNRoll::Think() {
    MARKFUNCTION(0x8001C7B8);
    Move();
    MovePassengers();
}

void KickNRoll::Move() {
    MARKFUNCTION(0x8001C850);
}

void KickNRoll::HandleEnvironmentCollision(const LVector& normal) {
    MARKFUNCTION(0x8001CB60);
}

void KickNRoll::Destroy() {
    MARKFUNCTION(0x8001CEEC);
    aliveFlag = 0;

    if (effectHash) {
        LVector center = {};
        FillBoxCentre(center, pos, orientation, collBox);
        GEffect_Create(effectHash, &center, nullptr, nullptr, 0, 0, effectParam);
    }

    extern const tagCollisionBox INVALID_COLLISION_BOX;
    SetCollisionBox(INVALID_COLLISION_BOX);
}

void KickNRoll::UpdatePosition() {
    MARKFUNCTION(0x8001CF74);
}

void KickNRoll::Draw() {
    MARKFUNCTION(0x8001CF7C);
    Obstacle::Draw();
}

void KickNRoll::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001CFAC);
}

void KickNRoll::MovePassengers() {
    MARKFUNCTION(0x8001CFEC);
    MovePassengersBasic();
}

void KickNRoll::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001D178);
}

void KickNRoll::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x8001D45C);
}

KnockDown::KnockDown(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001D5D4);
    field116 = 0;
    field168 = 0;
    field172 = 0;
    field184 = 0;
    aliveFlag = 1;
}

KnockDown::~KnockDown() {
    MARKFUNCTION(0x8001D650);
}

void KnockDown::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001D6B8);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void KnockDown::CreateModel(const char* name) {
    MARKFUNCTION(0x8001D8D8);
    Obstacle::CreateModel(name);
}

void KnockDown::Draw() {
    MARKFUNCTION(0x8001D92C);
    Obstacle::Draw();
}

void KnockDown::DeleteModel() {
    MARKFUNCTION(0x8001D9A8);
    Obstacle::DeleteModel();
}

void KnockDown::Reset() {
    MARKFUNCTION(0x8001D9F8);
}

void KnockDown::Think() {
    MARKFUNCTION(0x8001DA48);
}

void KnockDown::Move() {
    MARKFUNCTION(0x8001DB8C);
}


void KnockDown::UpdateCollisionBox() {
    MARKFUNCTION(0x8001DCA0);
}

void KnockDown::UpdatePosition() {
    MARKFUNCTION(0x8001F674);
}

void KnockDown::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001E120);
}

void KnockDown::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001E16C);
}

void KnockDown::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x8001E4EC);
}

static const u32 STACK_DEFAULT_EFFECT_HASH = 0x065C8E90;
static const char STACK_DEFAULT_PUSHABLE_NAME[] = "BoxStack1";
static constexpr u32 STACK_CALLBACK_JOINT_HASH_A = 0x08857E62;
static constexpr u32 STACK_CALLBACK_JOINT_HASH_B = 0x08857E63;

static void StackBuildModelWorldMatrix(const Model* model, Mat4& worldMatrix) {
    worldMatrix = Mat4();
    if (!model) {
        return;
    }

    p3dBuildRotMatrixZYX(model->rotX, model->rotY, model->rotZ, worldMatrix);
    worldMatrix.SetTranslation((f32)model->posX, (f32)model->posY, (f32)model->posZ);
}

static STreeJoint* StackFindJointByHash(STreeData* skeleton, u32 jointHash) {
    if (!skeleton || !skeleton->joints || jointHash == 0) {
        return nullptr;
    }

    for (u32 i = 0; i < skeleton->numJoints; i++) {
        STreeJoint* joint = &skeleton->joints[i];
        if (joint->nameUID == jointHash) {
            return joint;
        }
    }

    return nullptr;
}

static s32 StackEJointCallback(STreeJoint* joint, u32 jointIndex, const Mat4& currentMatrix) {
    MARKFUNCTION(0x8001F5A0);

    if (!joint || !joint->callbackData) {
        return 1;
    }

    Stack* stack = static_cast<Stack*>(joint->callbackData);

    Mat4 worldMatrix = currentMatrix;
    Model* stackModel = static_cast<Model*>(stack->model);
    if (stackModel) {
        Mat4 modelWorld = Mat4();
        StackBuildModelWorldMatrix(stackModel, modelWorld);
        worldMatrix = modelWorld * currentMatrix;
    }

    LVector jointPos = {};
    jointPos.x = (s32)worldMatrix.GetTransX();
    jointPos.y = (s32)worldMatrix.GetTransY();
    jointPos.z = (s32)worldMatrix.GetTransZ();

    s32 callbackJointIndex = (s32)jointIndex;
    if (joint->nameUID == STACK_CALLBACK_JOINT_HASH_A) {
        callbackJointIndex = 2;
    }
    else if (joint->nameUID == STACK_CALLBACK_JOINT_HASH_B) {
        callbackJointIndex = 3;
    }

    stack->SetupJointPosition(callbackJointIndex, jointPos);
    return 1;
}

s32 Stack::LoadDialog(u32 dialogId, u32 priority) {
    MARKFUNCTION(0x8001E6E0);

    s32 handle = 0;
    if (jcsQueryDialogPriority() < (s32)priority) {
        // PSX uses event 0x1A here directly.
        handle = rsEvent(0x1A, 0, (s32)dialogId, (s32)priority);
    }

    if (handle) {
        dialogHandle = handle;
        dialogID = (s32)dialogId;
    }

    return 1;
}

Stack::Stack(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001E768);
    knockDownSound = nullptr;
    anim = nullptr;
    state = 0;
    effectHash = 0;
    dialogHandle = 0;
    dialogID = 0;
}

Stack::~Stack() {
    MARKFUNCTION(0x8001E7B8);

    if (knockDownSound) {
        delete knockDownSound;
        knockDownSound = nullptr;
    }
}

void Stack::Draw() {
    MARKFUNCTION(0x8001E820);

    if (state != 3) {
        Obstacle::Draw();
    }
}

void Stack::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001E850);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    Obstacle::AnalyzeMesh(root);

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    LVector localPos = root->pos;

    // PSX Stack grounds from DBVolume bounds before normalizing Y into local space.
    FillCollisionBox(localBox, *static_cast<const DBVolume*>(root));
    localPos.y += (s32)localBox.minY;

    pos = localPos;
    localBox.maxY = (s16)((u16)localBox.maxY - (u16)localBox.minY);
    localBox.minY = 0;

    SetCollisionBox(localBox);

    const DBAttrib* attr21 = root->FindAttrib(21);
    modelIndex = attr21 ? (s32)attr21->value : 0;

    // PSX queries attrib 6 here but still initializes wobble count to zero.
    (void)root->FindAttrib(6);
    timesToWobble = 0;

    const DBAttrib* attr7 = root->FindAttrib(7);
    damage = attr7 ? (s32)attr7->value : 0x32;

    const DBAttrib* attr20 = root->FindAttrib(20);
    if (attr20) {
        effectHash = p3dHash(attr20->GetAttribString());
    }
    else {
        effectHash = STACK_DEFAULT_EFFECT_HASH;
    }

    const DBAttrib* attr8 = root->FindAttrib(8);
    if (attr8) {
        const char* baseName = attr8->GetAttribString();
        if (baseName && baseName[0] != '\0') {
            const size_t baseLen = std::strlen(baseName);
            if (baseLen > 0 && baseName[baseLen - 1] == 'P') {
                std::snprintf(pushableName, sizeof(pushableName), "%s", baseName);
            }
            else {
                std::snprintf(pushableName, sizeof(pushableName), "%sP", baseName);
            }
        }
        else {
            std::snprintf(pushableName, sizeof(pushableName), "%s", STACK_DEFAULT_PUSHABLE_NAME);
        }
    }
    else {
        std::snprintf(pushableName, sizeof(pushableName), "%s", STACK_DEFAULT_PUSHABLE_NAME);
    }
    pushableName[sizeof(pushableName) - 1] = '\0';

    SetupCallbacks();
}

void Stack::CreateModel(const char* name) {
    MARKFUNCTION(0x8001EA18);

    anim = nullptr;
    animBasic = nullptr;

    if (!modelHash) {
        flags |= TF_MODEL_CREATED;
        return;
    }

    Thing::CreateModel(nullptr);

    if (!knockDownSound) {
        CSound* soundObj = nullptr;
        if (CSoundFactory::CreateObject(0x2792, &soundObj, 0) >= 0) {
            knockDownSound = static_cast<CKnockDownSound*>(soundObj);
            if (knockDownSound) {
                knockDownSound->Initialize(&pos);
            }
        }
    }

    animBasic = Obstacle_GetAnimation(modelIndex);
    TransformAnim* baseAnim = animBasic ? animBasic->anim : nullptr;

    Model* modelPtr = static_cast<Model*>(model);
    if (baseAnim && modelPtr && modelPtr->drawableType == 3) {
        EModel* eModel = static_cast<EModel*>(modelPtr);
        anim = eModel->ApplyAnimToModel(baseAnim, 3);
    }

    if (lightingFlag && modelPtr) {
        modelPtr->AllocateHardwareLights(3);
        modelPtr->AllocateAmbientLight();
    }

    SetupCallbacks();
}

void Stack::DeleteModel() {
    MARKFUNCTION(0x8001EB0C);

    Obstacle::DeleteModel();

    anim = nullptr;
    animBasic = nullptr;

    if (knockDownSound) {
        delete knockDownSound;
        knockDownSound = nullptr;
    }
}

void Stack::Reset() {
    MARKFUNCTION(0x8001EB5C);

    currentFrame = 0;
    state = 0;
}

void Stack::UpdatePosition() {
    MARKFUNCTION(0x8001EB68);
}

void Stack::Wobble() {
    MARKFUNCTION(0x8001EB70);

    currentFrame++;
    if (currentFrame < 20) {
        return;
    }

    if (timesToWobble > 0) {
        timesToWobble--;
        currentFrame = 0;
    }
    else {
        state = 2;
        LoadDialog(0x1B, 0x32);
        UpdateCollisionBox();
    }
}

void Stack::Fall() {
    MARKFUNCTION(0x8001EBE8);

    currentFrame++;
    if (currentFrame >= 42) {
        FinishStack();
    }
}

void Stack::FinishStack() {
    MARKFUNCTION(0x8001EC24);

    state = 3;

    if (effectHash) {
        GEffect_Create(effectHash, &jointPositions[0], nullptr, nullptr, 0, 0, 0);
        GEffect_Create(effectHash, &jointPositions[1], nullptr, nullptr, 0, 0, 0);
    }

    if (knockDownSound) {
        knockDownSound->Impact();
    }

    tagCollisionBox localBox = INVALID_COLLISION_BOX;
    SetCollisionBox(localBox);

    DBRoot tempRoot = {};
    tempRoot.name = const_cast<char*>("");
    tempRoot.AllocatePermanentAttributeArray(2);
    tempRoot.AddAttribString(0, 5, pushableName);
    tempRoot.AddAttribNumber(1, 0x0F, blockNum);

    Thing* spawned = g_ai->AddThingNoTagList(nullptr, 0x1CE, &pos, nullptr, pushableName, &tempRoot);

    tempRoot.DeallocatePermanentAttributeArray();

    if (spawned) {
        spawned->pos = pos;
        spawned->orientation = orientation;

        Model* spawnedModel = static_cast<Model*>(spawned->model);
        if (spawnedModel) {
            spawnedModel->rotY = (u16)(s16)(s8)physicalType;
        }

        Obstacle* spawnedObstacle = static_cast<Obstacle*>(spawned);
        spawnedObstacle->lightingFlag = lightingFlag;
        spawnedObstacle->shadowFlag = shadowFlag;
    }
}

void Stack::Think() {
    MARKFUNCTION(0x8001EE28);

    if (state == 1) {
        Wobble();
    }
    else if (state == 2) {
        Fall();
    }
    else if (state == 3) {
        return;
    }

    TransformFlip* flip = anim->flip;

    flip->SetFrame(currentFrame);
    flip->UpdateJoints();

    if (knockDownSound) {
        knockDownSound->Think();
    }
}

void Stack::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001EEE8);

    pickup->Kill();
}

void Stack::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001EF1C);

    tagCollisionBox localBox = collBox;

    LVector correctedPos = {};
    LVector correctionNormal = {};
    LVector correctionPushedPos = {};

    if (state < 0) {
        return;
    }

    if (state < 2) {
        CorrectThingPositionObstacle(
            pos,
            pos,
            orientation.y,
            orientation.y,
            localBox,
            hum->pos,
            hum->homePos,
            hum->collBboxMin.x,
            hum->collBboxMin.y,
            hum->collBboxMin.z,
            correctedPos,
            correctionNormal,
            correctionPushedPos);

        bool triggerNow = false;

        if (LedgeCheck(localBox, correctionNormal, correctionPushedPos, hum)) {
            if (hum->thingType != 0) {
                triggerNow = true;
            }
        }
        else {
            const s32 action = hum->actionState;
            if ((action == 0x3F || action == 0x38 || action == 0x39 || action == 0x3A)
                && hum->velocity.y < -25) {
                triggerNow = true;
            }
        }

        if (triggerNow) {
            TriggerStackAnimation();
        }

        if (correctionNormal.y > 0 && hum->velocity.y <= 0) {
            hum->SetFloorHeight(pos.y + (s32)localBox.maxY);
            hum->velocity.y = 0;
            AddPassenger(hum);
            TriggerStackAnimation();
        }

        hum->homePos = correctedPos;
        return;
    }

    if (state != 2) {
        return;
    }

    const u16 box0 = (u16)localBox.minX;
    localBox.maxX = (s16)(-(s32)box0);
    localBox.maxY = (s16)(-(s32)box0 - (s32)box0);

    CorrectThingPositionObstacle(
        pos,
        pos,
        orientation.y,
        orientation.y,
        localBox,
        hum->pos,
        hum->homePos,
        hum->collBboxMin.x,
        hum->collBboxMin.y,
        hum->collBboxMin.z,
        correctedPos,
        correctionNormal,
        correctionPushedPos);

    hum->homePos = correctedPos;

    if (correctionNormal.y > 0 && hum->velocity.y <= 0) {
        hum->SetFloorHeight(pos.y + (s32)localBox.maxY);
        hum->velocity.y = 0;
        AddPassenger(hum);
    }

    const tagCollisionCylinder& humCylinder =
        *reinterpret_cast<const tagCollisionCylinder*>(&hum->collBboxMin);
    tagCollisionSphere sphere = { 200 };

    const bool hit0 = CheckStaticCylinderSphereCollision(hum->pos, humCylinder, jointPositions[0], sphere);
    bool hit1 = false;
    bool hit = hit0;
    if (!hit) {
        hit1 = CheckStaticCylinderSphereCollision(hum->pos, humCylinder, jointPositions[1], sphere);
        hit = hit1;
        if (hit && currentFrame < 31) {
            hit = false;
        }
    }

    if (!hit) {
        return;
    }

    hum->HandleCollision(this, 1, damage, 0x80000007, 0);
    hum->SetActionState(AS_COLLAPSE_STUN, 0);

    if (hum->thingType != 0) {
        g_scoreManager->AddStylePoints(100);
    }

    if (hum == static_cast<Humanoid*>(Player::s_player)) {
        hum->soundParam = dialogID;
        hum->soundHandle = dialogHandle;
    }

    FinishStack();
}

void Stack::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x8001F320);

    if (state == 0) {
        TriggerStackAnimation();
        if (knockDownSound) {
            knockDownSound->Kick();
        }
    }
}

void Stack::UpdateCollisionBox() {
    MARKFUNCTION(0x8001F370);

    tagCollisionBox localBox = collBox;
    localBox.maxX = (s16)((u16)localBox.maxX + (u16)((u16)localBox.maxY - (u16)localBox.minY));
    SetCollisionBox(localBox);
}

void Stack::TriggerStackAnimation() {
    MARKFUNCTION(0x8001F3E8);

    state = 1;
}

void Stack::SetupCallbacks() {
    MARKFUNCTION(0x8001F3F4);

    Model* modelPtr = static_cast<Model*>(model);
    if (!modelPtr || modelPtr->drawableType != 3) {
        return;
    }

    AnimStructure* animStruct = anim;
    if (!animStruct && modelPtr->animStructure) {
        animStruct = static_cast<AnimStructure*>(modelPtr->animStructure);
    }
    if (!animStruct || !animStruct->flip || !animStruct->flip->tree) {
        return;
    }

    STreeData* skeleton = animStruct->flip->tree;

    STreeJoint* jointA = StackFindJointByHash(skeleton, STACK_CALLBACK_JOINT_HASH_A);
    if (jointA) {
        jointA->postCallback = StackEJointCallback;
        jointA->callbackData = this;
        jointA->flags |= STF_POST_CALLBACK_MASK;
    }

    STreeJoint* jointB = StackFindJointByHash(skeleton, STACK_CALLBACK_JOINT_HASH_B);
    if (jointB) {
        jointB->postCallback = StackEJointCallback;
        jointB->callbackData = this;
        jointB->flags |= STF_POST_CALLBACK_MASK;
    }

    if (!jointA || !jointB) {
        for (s32 i = 2; i <= 3; i++) {
            STreeJoint* joint = skeleton->GetJoint(i);
            if (!joint) {
                continue;
            }
            joint->postCallback = StackEJointCallback;
            joint->callbackData = this;
            joint->flags |= STF_POST_CALLBACK_MASK;
        }
    }
}

void Stack::SetupJointPosition(s32 index, LVector pos) {
    MARKFUNCTION(0x8001F55C);

    index -= 2;
    if ((u32)index < 2) {
        jointPositions[index] = pos;
    }
}

bool Stack::CareAboutAttack() const {
    MARKFUNCTION(0x8001F664);
    return true;
}
