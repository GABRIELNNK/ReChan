#include "ai/arrow.h"
#include "gen/blockmgr.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/config.h"
#include "gen/scoremgr.h"
#include "gen/world.h"
#include "ai/player.h"

Arrow::Arrow(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x8001B688);
    arrowState = -7;
}

Arrow::~Arrow() {
}

void Arrow::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001B6E8);
    Obstacle::AnalyzeMesh(root);

    // PSX: builds point names "00".."43" (row+col), looks up DB points
    char buf[16] = {};
    strcpy(buf, "arrow  ");
    memset(&buf[7], 0, 3);

    for (s32 row = 0; row < 5; row++) {
        buf[5] = (char)('0' + row);
        for (s32 col = 0; col < 4; col++) {
            buf[6] = (char)('0' + col);
            buf[7] = '\0';

            DBPoint* child = g_database ? g_database->FindPoint(buf) : nullptr;
            if (child) {
                startPos[row][col] = child->pos;

                // PSX: orientation at +0x28 stores end position
                endPos[row][col].x = child->field40;
                endPos[row][col].y = child->field44;
                endPos[row][col].z = child->field48;

                const DBAttrib* a6 = child->FindAttrib(6);
                dirData[row][col].dirX = a6 ? (s32)a6->value : 0;

                const DBAttrib* a7 = child->FindAttrib(7);
                dirData[row][col].dirY = a7 ? (s32)a7->value : 5;

                const DBAttrib* a8 = child->FindAttrib(8);
                dirData[row][col].dirZ = a8 ? (s32)a8->value : 0;
            }
        }
    }

    DBVolume* vol = dynamic_cast<DBVolume*>(root);
    if (!vol && g_database) {
        for (DBVolume* dbv = g_database->GetFirstVolume(); dbv; dbv = static_cast<DBVolume*>(dbv->next)) {
            if (dbv->nameCRC == root->nameCRC) {
                vol = dbv;
                break;
            }
        }
    }
    if (vol) {
        tagCollisionBox box = {};
        FillCollisionBox(box, *vol);
        SetCollisionBox(box);
    }
}

void Arrow::CreateModel(const char* name) {
    MARKFUNCTION(0x8001B8CC);
    shadowFlag = 0;
    // PSX: calls AllocateAndCreateModel which creates GModel + Thing::CreateModel
    // PC: Thing::CreateModel creates SModel directly
    Thing::CreateModel(name);
}

void Arrow::DeleteModel() {
    Thing::DeleteModel();
}

void Arrow::Reset() {
    MARKFUNCTION(0x8001B90C);
    arrowState = -7;
    currentRow = -1;
    currentCol = -1;

    if (!g_scoreManager) {
        return;
    }

    // Forward scan: find first unplayed petal (fightScore == -1)
    for (s32 row = 0; row < 5; row++) {
        for (s32 col = 0; col < 3; col++) {
            PetalStats* ps = &g_scoreManager->petalStats[row * 3 + col];
            if (ps->fightScore == -1) {
                currentRow = (s16)row;
                currentCol = (s16)(col + 1);
            }
        }
    }

    if (currentRow >= 0) {
        g_arrowInside = 0;
        return;
    }

    // Backward scan: find last petal with collectCount < 10
    for (s32 row = 4; row >= 0; row--) {
        for (s32 col = 2; col >= 0; col--) {
            PetalStats* ps = &g_scoreManager->petalStats[row * 3 + col];
            if (ps->collectCount < 10) {
                currentRow = (s16)row;
                currentCol = (s16)(col + 1);
            }
        }
    }

    if (currentRow >= 0) {
        g_arrowInside = 0;
        return;
    }

    // Fallback: check for levels without gold dragons
    if (g_scoreManager->petalStats[14].goldDragons == 0) {
        currentRow = 4;
        currentCol = 3;
    } else if (g_scoreManager->petalStats[9].goldDragons == 0) {
        currentRow = 3;
        currentCol = 1;
    } else if (g_scoreManager->petalStats[6].goldDragons == 0) {
        currentRow = 2;
        currentCol = 1;
    } else if (g_scoreManager->petalStats[4].goldDragons == 0) {
        currentRow = 1;
        currentCol = 2;
    } else if (g_scoreManager->petalStats[2].goldDragons == 0) {
        currentRow = 0;
        currentCol = 3;
    }

    g_arrowInside = 0;
}

void Arrow::Think() {
    MARKFUNCTION(0x8001BBD8);
    if ((currentRow < 0 || currentCol < 0) && g_scoreManager) {
        Reset();
    }

    if (currentRow < 0 || currentCol < 0) {
        return;
    }

    // g_arrowInside selects between entry path (col 0) and petal path (currentCol)
    s32 col = currentCol * g_arrowInside;

    orientation = endPos[currentRow][col];

    LVector start = startPos[currentRow][col];

    if (g_blockManager) {
        blockNum = g_blockManager->GetBlockNumber(start);
    }

    s32 stateSq = arrowState * arrowState;
    ArrowDirData& dir = dirData[currentRow][col];

    pos.x = start.x - dir.dirX * stateSq;
    pos.y = start.y - dir.dirY * stateSq;
    pos.z = start.z - dir.dirZ * stateSq;

    arrowState++;
    if (arrowState >= 8) {
        arrowState = -7;
    }
}

void Arrow::Draw() {
    if (currentRow >= 0 && currentCol >= 0) {
        Obstacle::Draw();
    }
}

void Arrow::HandleHumanoidCollision(Humanoid* /*hum*/) {
}

void Arrow::HandlePickupCollision(Thing* /*pickup*/) {
}
