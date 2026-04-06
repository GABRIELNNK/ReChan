# Jackie Chan Stuntmaster - Reverse Engineering Reference

Auto-generated from debug symbols (SLUS-00684 / GAME_REL.SYM)

---

## Table of Contents

1. [Memory Map & Overlays](#memory-map--overlays)
2. [Module Statistics](#module-statistics)
3. [Class Hierarchy](#class-hierarchy)
4. [Data Structure Layouts](#data-structure-layouts)
5. [Class Method Reference](#class-method-reference)
6. [Free Functions](#free-functions)
7. [VTable Addresses](#vtable-addresses)
8. [Original Source Tree](#original-source-tree)
9. [Address-to-Function Index](#address-to-function-index)

---

## Memory Map & Overlays

| Region | Start | End | Size | Content |
|--------|-------|-----|------|---------|
| Overlay A | `0x80010000` | `0x8001A758` | 42,840 | OL1/OL2/BOL_REL.BIN |
| Overlay B | `0x8001A758` | `0x8002601C` | 47,300 | NBOL_REL.BIN |
| Main EXE | `0x8002601C` | `0x800DD81C` | 491,520 | GAME_REL.CPE |
| Stack | `0x801FFFF0` | - | - | Stack pointer |

### Overlays

| ID | Base Address | Length | Notes |
|----|-------------|--------|-------|
| 3 | `0x80010000` | `0xA758` (42,840 bytes) | OL1 (Game overlays) |
| 4 | `0x80010000` | `0xA758` (42,840 bytes) | OL2 (Game overlays) |
| 6 | `0x8001A758` | `0xB8C4` (47,300 bytes) | NBOL (Non-Boss overlays) |
| 7 | `0x8001A758` | `0xB8C4` (47,300 bytes) | BOL (Boss AI overlays) |

---

## Module Statistics

**Total functions:** 3,976

| Module | Functions | Description |
|--------|-----------|-------------|
| GEN | 1085 | Core engine (Game, Camera, Collision, Level, World) |
| AI | 860 | Gameplay entities (Player, Enemies, Obstacles, Pickups) |
| PURE3D | 629 | Pure3D v11.3 engine (entities, trees, loaders, rendering) |
| SND | 423 | Sound system (per-entity sound management) |
| FE | 274 | Front-end (Menus, Save/Load, HUD) |
| OTHER | 244 | Miscellaneous |
| PSX | 193 | PSX-specific (Display, MemCard, Shadows, Particles) |
| XCLIB | 131 | XC library (fonts, display, image, sorting) |
| RADLIB | 120 | Radical low-level library (tasks, math, memory, CD) |
| RADMOVIE | 17 | FMV playback (RadMovie2) |

---

## Class Hierarchy

Inheritance detected from struct embedding (first field at offset 0):

```
Thing (96 bytes) [vtable]
  DynamicThing (200 bytes) [vtable]
    Camera (492 bytes) [vtable]
    Humanoid (616 bytes) [vtable]
      Player (764 bytes) [vtable]
    Pickup (336 bytes) [vtable]
  Obstacle (116 bytes) [vtable]
    Arrow (844 bytes) [vtable]
    Blast (232 bytes) [vtable]
    Collectible (176 bytes) [vtable]
    Conveyor (148 bytes) [vtable]
    Crusher (176 bytes) [vtable]
    DestructibleThing (168 bytes) [vtable]
    Door (196 bytes) [vtable]
    DynamicObstacle (192 bytes) [vtable]
    Explosive (168 bytes) [vtable]
    FrontEndVolume (132 bytes) [vtable]
    Generator (216 bytes) [vtable]
      EnemyGenerator (300 bytes) [vtable]
      ThrowingGenerator (256 bytes) [vtable]
    HorizontalPole (156 bytes) [vtable]
    KickNRoll (172 bytes) [vtable]
    KnockDown (192 bytes) [vtable]
    Ladder (188 bytes) [vtable]
    Launcher (148 bytes) [vtable]
    Pendulum (180 bytes) [vtable]
    Platform (324 bytes) [vtable]
    Pushable (172 bytes) [vtable]
    SlipperyFloor (124 bytes) [vtable]
    Stack (236 bytes) [vtable]
    Teleporter (136 bytes) [vtable]
    TrapDoor (192 bytes) [vtable]
    TriggerThing (132 bytes) [vtable]
    Untouchable (156 bytes) [vtable]

Manager (28 bytes) [vtable]
  AI (116 bytes) [vtable]
  AnimationManager (40 bytes) [vtable]
  BlockManager (168 bytes) [vtable]
  CameraManager (32 bytes) [vtable]
  CharacterManager (3004 bytes) [vtable]
  Database (120 bytes) [vtable]
  Director (212 bytes) [vtable]
  Display (32 bytes) [vtable]
  EnvironmentManager (140 bytes) [vtable]
  Game (140 bytes) [vtable]
  InputManager (1492 bytes) [vtable]
  LevelManager (136 bytes) [vtable]
  ScoreManager (504 bytes) [vtable]
  Sound (44 bytes) [vtable]
  Time (40 bytes) [vtable]
  World (160 bytes) [vtable]

ccNode (24 bytes) [vtable]
  ActiveZone (104 bytes) [vtable]
  AnimStructureBasic (32 bytes)
    AnimStructure (104 bytes) [vtable]
  Behaviour (280 bytes) [vtable]
  BehaviourAttrib (56 bytes) [vtable]
  Callback (36 bytes)
  Control (728 bytes) [vtable]
  DBRoot (60 bytes)
    DBLine (76 bytes)
    DBMesh (64 bytes)
    DBPath (76 bytes)
    DBSphere (64 bytes)
    DBVolume (84 bytes)
      DBColourVolume (96 bytes)
      DBHLightVolume (100 bytes)
  DataAnchor (36 bytes)
    CameraAnchor (48 bytes) [vtable]
    SoundAnchor (60 bytes)
  Effects (36 bytes) [vtable]
    CBVEffect (72 bytes) [vtable]
    GEffect (128 bytes) [vtable]
    PWEffect (96 bytes) [vtable]
      FPWEffect (128 bytes) [vtable]
    Trails (104 bytes) [vtable]
    WEffect (132 bytes) [vtable]
      FWEffect (212 bytes) [vtable]
        LensFlare (300 bytes) [vtable]
      SpotLight (168 bytes) [vtable]
  Handler (36 bytes)
  HandlerSet (36 bytes)
  Model (88 bytes) [vtable]
    EModel (120 bytes) [vtable]
    GModel (120 bytes) [vtable]
    SModel (96 bytes) [vtable]
      HumanoidModel (136 bytes) [vtable]
  OriginalBasic (36 bytes) [vtable]
    OriginalGeo (40 bytes) [vtable]
    OriginalTree (52 bytes) [vtable]
      OriginalSTree (60 bytes) [vtable]
  ParticleSystem (100 bytes) [vtable]
  Path (68 bytes)
    LinearPath (80 bytes) [vtable]
    SplinePath (128 bytes) [vtable]
  SubZoneVolume (48 bytes) [vtable]
  Ticket (32 bytes) [vtable]
  WDBSwitch (76 bytes) [vtable]
    WDBSphereSwitch (80 bytes) [vtable]
    WDBVolumeSwitch (100 bytes) [vtable]
  World3DPoint (40 bytes)
  WorldParPoint (28 bytes)
  ccFile (60 bytes) [vtable]


oxScreen (16 bytes)
  hdMenu (36 bytes) [vtable]
    hdDynItemMenu (44 bytes) [vtable]
    hdDynMenu (60 bytes) [vtable]
    hdMemCardMenu (80 bytes) [vtable]

hdMenuItem (28 bytes) [vtable]
  hdItemGoto (32 bytes) [vtable]
    hdDynItemGoto (36 bytes) [vtable]
  hdItemSelection (36 bytes) [vtable]
    hdControllerSelection (40 bytes) [vtable]
    hdDynItemSelection (48 bytes)
    hdSndItemSelection (68 bytes) [vtable]
  hdNumericSelection (44 bytes) [vtable]

MenuMgr (80 bytes) [vtable]
  feMenuMgr (100 bytes) [vtable]
  gameMenu (92 bytes) [vtable]

AmbientLight (8 bytes) [vtable]
BGGeo
  BackG (28 bytes)
Boss (616 bytes) [vtable]
  Butch (620 bytes) [vtable]
  Dante (684 bytes) [vtable]
CPublishedSoundData (1 bytes)
  CPublishedData (82 bytes)
CSound (16 bytes) [vtable]
  CDestructibleSound (28 bytes) [vtable]
  CDirectorSound (20 bytes) [vtable]
  CFrontEndSound (32 bytes) [vtable]
  CGenericPersistentSound (24 bytes) [vtable]
  CGenericTransientSound (28 bytes) [vtable]
  CHumanoidSound (132 bytes) [vtable]
  CKickNRollSound (32 bytes) [vtable]
  CKnockDownSound (32 bytes) [vtable]
  CParticleEffectSound (24 bytes) [vtable]
  CPendulumSound (24 bytes) [vtable]
  CPlatformSound (56 bytes) [vtable]
  CPushableSound (36 bytes) [vtable]
  CWeaponSound (28 bytes) [vtable]
  CWorldEffectSound (32 bytes) [vtable]
CTM
  tContext (64 bytes)
Chair (192 bytes) [vtable]
CharMgrCallback (8 bytes)
  AnimCallback (24 bytes) [vtable]
  AsyncAnimCallback (16 bytes)
  nisCharMgrCallback (48 bytes) [vtable]
DBLight (24 bytes)
  AnimLight (48 bytes)
DrawableBasic (24 bytes)
  DrawableGeo (32 bytes) [vtable]
  DrawableTree (32 bytes) [vtable]
    DrawableSTree (36 bytes) [vtable]
DrawableETree (32 bytes) [vtable]
ErrorScreen (48 bytes) [vtable]
FogMode
  tFog (20 bytes)
Grontar (616 bytes) [vtable]
LineFile (216 bytes) [vtable]
MagicNumber
  MCFILEHEADER (128 bytes)
Offsets
  UVdata (32 bytes)
OriginalETree (52 bytes) [vtable]
Oscar (616 bytes) [vtable]
PathInfo (60 bytes) [vtable]
Paul (616 bytes) [vtable]
PlayerModel (136 bytes) [vtable]
Pos
  VisTrack (16 bytes)
Queue
  RADCD_PRIORITYQUEUE (1288 bytes)
Shadow (16 bytes) [vtable]
  SimpleShadow (32 bytes) [vtable]
Table (192 bytes) [vtable]
TreeShadow (16 bytes) [vtable]
action
  MovieAction (12 bytes)
    MoviePlay (92 bytes)
    MovieRandom (20 bytes) [vtable]
base
  VertexInfo (20 bytes)
ccMinNode (12 bytes) [vtable]
  BlockNode (16 bytes)
  DBCameraPath (52 bytes) [vtable]
  DBCameraPathNode (64 bytes) [vtable]
  DBLineVertex (24 bytes)
  ItemNode (20 bytes)
  ParticleInfo (68 bytes) [vtable]
  SubDivNode (36 bytes)
  TrailInfo (80 bytes)
centre
  tSphere (16 bytes)
clip
  DRAWENV (92 bytes)
colourVolumes
  LightAnchor (44 bytes) [vtable]
direction
  HardwareLight (24 bytes) [vtable]
disp
  DISPENV (20 bytes)
draw
  tBuffer (128 bytes)
fDestDragon
  hdDestSelect (24 bytes)
fHitOvl
  hdHits (64 bytes) [vtable]
fightingMove
  StrikeFightingMove (28 bytes)
  ThrowFightingMove (60 bytes)
geo
  BGGEO (16 bytes)
hLightHandles
  LightingClass (112 bytes) [vtable]
hdAlphaSelection (44 bytes) [vtable]
hdItemButton (28 bytes) [vtable]
  hdDynItemButton (32 bytes) [vtable]
hdShockSelection (36 bytes) [vtable]
headers
  Stream (48 bytes)
    tFile (268 bytes) [vtable]
id
  CelFileHeader (24 bytes)
idleMove
  MoveAnimStruct (40 bytes)
m
  MATRIX (32 bytes)
mFile
  CDFile (236 bytes)
mHeader
  PaletteData (44 bytes)
  ScaleData (16 bytes)
mUVOffsets
  UVinfo (28 bytes)
mWall
  Wall (56 bytes)
m_CMS
  xcFontDC (140 bytes)
  xcImageDC (128 bytes)
m_Character
  CharDataSet (424 bytes)
m_FreeList
  xcVRAMAllocator (36 bytes)
m_Reverb
  rsdAmbSpace (52 bytes)
m_ScreenChange
  oxScreenManager (48 bytes) [vtable]
    GameOverScreen (56 bytes) [vtable]
    HUD (712 bytes) [vtable]
    TitleScreen (56 bytes) [vtable]
m_chunk
  xcInventory (20 bytes)
m_pCellsAlloc
  tCellAlligator (8204 bytes)
m_pMatrix
  xc3x3MatrixStack (112 bytes)
m_pTable
  CPhaseManager (8 bytes)
mc_fileList
  MemoryCard (700 bytes)
name
  DIRENTRY (40 bytes)
  _TEMPDIRENTRY (68 bytes)
oxOvl (8 bytes) [vtable]
  hdTtlive (12 bytes) [vtable]
    hdHealth (36 bytes) [vtable]
    hdTextOvl (32 bytes) [vtable]
      hdAnimTextOvl (128 bytes) [vtable]
        hdDragon (140 bytes) [vtable]
pLetter
  xciSpriteLetter (16 bytes)
pivot
  TeeterType (28 bytes)
pmin
  tBox3D (24 bytes)
pointListNIS
  WorldPoints (16 bytes)
pos
  CheckpointInfo (56 bytes)
position0
  ._64 (48 bytes)
r01
  CRVECTOR3 (88 bytes)
  CRVECTOR4 (140 bytes)
rect
  RectList (12 bytes)
rsdLoadCallback (8 bytes)
  rsdAmbiance (328 bytes) [vtable]
  rsdClip (68 bytes) [vtable]
  rsdStream (128 bytes) [vtable]
rsdStreamCallback (4 bytes)
  rsdMusicPlayer (208 bytes) [vtable]
step
  hdTally (128 bytes)
tAnimation (12 bytes) [vtable]
  tCBVAnim (40 bytes) [vtable]
    tCBVParamAnim (44 bytes) [vtable]
  tClutList (40 bytes) [vtable]
  tCompositeAnim (24 bytes) [vtable]
  tFrameList (24 bytes) [vtable]
  tParamAnim (36 bytes) [vtable]
  tRAMTexAnim (32 bytes) [vtable]
  tSequenceAnim (20 bytes) [vtable]
  tTexList (40 bytes) [vtable]
  tTransformAnim (40 bytes) [vtable]
  tUVAnim (40 bytes) [vtable]
  tVizAnim (24 bytes) [vtable]
tByteStream (4 bytes)
  tMemByteStream (20 bytes) [vtable]
tCBVAnimLoader (12 bytes) [vtable]
tCBVParamAnimLoader (12 bytes) [vtable]
tCache (20 bytes) [vtable]
  tInvCache (276 bytes) [vtable]
tChanSequenceAnimLoader (12 bytes) [vtable]
tChunk (20 bytes) [vtable]
tClutAnimLoader (12 bytes) [vtable]
tCompAnimLoader (12 bytes) [vtable]
tCompositeFlip (32 bytes) [vtable]
tDoubleLayer (48 bytes) [vtable]
tDrawable (12 bytes)
  tGeometry (64 bytes)
    tDynGeom (100 bytes) [vtable]
    tPrimGeom (108 bytes) [vtable]
  tMTree (20 bytes) [vtable]
  tTree (24 bytes) [vtable]
    tETree (28 bytes) [vtable]
    tPoseTree (32 bytes)
    tSTree (40 bytes) [vtable]
tETreeLoader (12 bytes) [vtable]
tEntity (12 bytes) [vtable]
  tCamera (24 bytes) [vtable]
    t2PointMatrixCamera (52 bytes) [vtable]
    tMatrixCamera (56 bytes) [vtable]
  tDrawTable (60 bytes) [vtable]
  tIndexList (32 bytes) [vtable]
  tInventory (44 bytes) [vtable]
    tP3Dinventory (428 bytes) [vtable]
  tLight (24 bytes) [vtable]
    tDirectionalLight (36 bytes) [vtable]
  tMaterial (32 bytes)
  tPuppet (20 bytes) [vtable]
    tFlipbook (32 bytes) [vtable]
      tCBVFlip (36 bytes) [vtable]
      tClutFlip (52 bytes) [vtable]
      tParamFlip (48 bytes) [vtable]
        t2PointCamFlip (52 bytes) [vtable]
        tCBVParamFlip (56 bytes) [vtable]
      tRAMTexFlip (40 bytes) [vtable]
      tTexFlip (40 bytes) [vtable]
      tTransformFlip2 (68 bytes) [vtable]
      tUVFlip (36 bytes) [vtable]
      tVertexFlip (36 bytes) [vtable]
  tTexture (24 bytes) [vtable]
  tView (216 bytes) [vtable]
tGameLoader (12 bytes) [vtable]
tGeoLoader (12 bytes) [vtable]
tKeyList (8 bytes)
  tDynamicKeyList (16 bytes) [vtable]
    tJoint1DOF (24 bytes) [vtable]
    tJoint1DOFangle (24 bytes) [vtable]
    tJoint3DOF (20 bytes) [vtable]
    tJoint3DOFangle (20 bytes) [vtable]
    tJoint3DOFlpPSX (20 bytes) [vtable]
tLayer (48 bytes) [vtable]
tLitFarTable (60 bytes) [vtable]
tLitTable (60 bytes) [vtable]
tMatLoader (12 bytes) [vtable]
tParamAnimLoader (12 bytes) [vtable]
tPose (12 bytes) [vtable]
tPrimLoader (12 bytes) [vtable]
tRAMTexAnimLoader (12 bytes) [vtable]
tReadChunk (20 bytes) [vtable]
tSTreeLoader (12 bytes) [vtable]
tSTreeUnLit (40 bytes) [vtable]
tSequenceAnimLoader (12 bytes) [vtable]
tSequenceFlip (32 bytes) [vtable]
tStaticKeyList (8 bytes)
  tStatic3DOFKeyList (20 bytes) [vtable]
tTexAnimLoader (12 bytes) [vtable]
tTexLoader (12 bytes) [vtable]
tTranAnimLoader2 (12 bytes) [vtable]
tTreeFlip (68 bytes) [vtable]
tTreeJoint (44 bytes)
  tEJoint (52 bytes)
  tSJoint (68 bytes)
tUVAnimLoader (12 bytes) [vtable]
tVertAnimLoader (12 bytes) [vtable]
tVizAnimLoader (12 bytes) [vtable]
tVizFlip (32 bytes) [vtable]
tZFarTable (60 bytes) [vtable]
tZSortTable (60 bytes) [vtable]
translation
  tPoseJoint (24 bytes)
upperBox
  VehicleBoxStructure (32 bytes)
v
  RVECTOR (24 bytes)
volume
  SpuExtAttr (12 bytes)
wallArray
  CollisionCache (536 bytes)
xcCell
  xcCELLFile (32 bytes)
xcPrimObj (4 bytes)
  xcClipObj (12 bytes)
  xcTextureObj (48 bytes)
    xcSprite (52 bytes)
    xcTextObj (60 bytes)
```

---

## Data Structure Layouts

Key game classes with field offsets and types.

### Thing (96 bytes)

* — inherits from ccNode — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | ccNode | 24 | ← base class |
| `0x0018` | objType | 0 |  |
| `0x001A` | objSubType | 0 |  |
| `0x001C` | position | 12 |  |
| `0x0028` | orientation | 12 |  |
| `0x0034` | mass | 0 |  |
| `0x0038` | hitPoints | 0 |  |
| `0x003A` | maxHitPoints | 0 |  |
| `0x003C` | pth | 8 |  |
| `0x0040` | passengers | 12 |  |
| `0x004C` | myModelNameCRC | 0 |  |
| `0x0050` | pModel | 0 | pointer |
| `0x0054` | myBlockNumber | 0 |  |
| `0x0056` | serialNumber | 0 |  |
| `0x0058` | thingFlags | 0 |  |
| `0x005C` | specialFlags | 0 |  |

### DynamicThing (200 bytes)

* — inherits from Thing — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Thing | 96 | ← base class |
| `0x0060` | maxVelocity | 0 |  |
| `0x0064` | velocity | 12 |  |
| `0x0070` | obstacleVelocity | 12 |  |
| `0x007C` | newPos | 12 |  |
| `0x0088` | force | 12 |  |
| `0x0094` | oldForce | 12 |  |
| `0x00A0` | deltaPos | 12 |  |
| `0x00AC` | floorNormal | 12 |  |
| `0x00B8` | landedFloorHeight | 0 |  |
| `0x00BC` | pTicket | 32 | pointer |
| `0x00C0` | dragCoefficient | 0 |  |
| `0x00C4` | gravity | 0 |  |

### Humanoid (616 bytes)

* — inherits from DynamicThing — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | DynamicThing | 200 | ← base class |
| `0x00C8` | mDebugAttackJoint | 0 |  |
| `0x00CC` | mExtraCallbackJoint | 0 |  |
| `0x00D0` | desiredRunForce | 0 |  |
| `0x00D4` | effectiveRunForce | 0 |  |
| `0x00D8` | characterSubType | 4 |  |
| `0x00DC` | straifMovementStructure | 0 |  |
| `0x00E0` | stunTwirl | 128 |  |
| `0x00E4` | pBehaviorMenu | 0 | pointer |
| `0x00E8` | myOrigin | 12 |  |
| `0x00F4` | myOriginalRot | 12 |  |
| `0x0100` | target | 616 |  |
| `0x0104` | m_targetReferance | 0 |  |
| `0x0108` | miscTarget | 12 |  |
| `0x0114` | desiredMoveDirection | 0 |  |
| `0x0118` | mCollisionCylinder | 12 |  |
| `0x0124` | mFightingVictimRadius | 0 |  |
| `0x0128` | collisionPoint | 12 |  |
| `0x0134` | timer | 0 |  |
| `0x0138` | timerflicker | 0 |  |
| `0x013C` | tauntAnim | 0 |  |
| `0x0140` | straifDirection | 0 |  |
| `0x0144` | pauseLength | 0 |  |
| `0x0148` | m_pHumanoidSound | 0 |  |
| `0x014C` | m_soundIDDialogHandle | 0 |  |
| `0x0150` | m_soundIDDialog | 0 |  |
| `0x0154` | soundFlag | 0 |  |
| `0x0156` | turnRate | 0 |  |
| `0x0158` | actionHandler | 8 |  |
| `0x0160` | requestedState | 0 |  |
| `0x0164` | actionState | 4 |  |
| `0x0168` | nextActionState | 4 |  |
| `0x016C` | actionStateAtBirth | 4 |  |
| `0x0170` | dynamicWorldObjectContext | 0 |  |
| `0x0174` | groundFriction | 0 |  |
| `0x0178` | hitHighAnimationOffset | 0 |  |
| `0x017A` | hitMediumAnimationOffset | 0 |  |
| `0x017C` | hitHighRearAnimationOffset | 0 |  |
| `0x0180` | idleCheck | 36 |  |
| `0x01A4` | myObstacle | 116 |  |
| `0x01A8` | omega | 12 |  |
| `0x01B4` | floorMaterial | 0 |  |
| `0x01B8` | control | 280 |  |
| `0x01BC` | allocatedBehaviourOnce | 0 |  |
| `0x01C0` | controlHandlerIsIdle | 0 |  |
| `0x01C4` | inOverlord | 0 |  |
| `0x01C8` | behaviourAttribCRC | 0 |  |
| `0x01CC` | heavyClass | 0 |  |
| `0x01D0` | knockDownFrameTime | 0 |  |
| `0x01D2` | hitStunFrames | 0 |  |
| `0x01D4` | stunTime | 0 |  |
| `0x01D6` | healRate | 0 |  |
| `0x01D8` | comboConnect | 0 |  |
| `0x01DC` | fightingStyle | 20 |  |
| `0x01E0` | activeFightingStyle | 20 |  |
| `0x01E4` | currentFightingNode | 20 |  |
| `0x01E8` | nextFightingNode | 20 |  |
| `0x01EC` | upJumpForce | 0 |  |
| `0x01F0` | dynamicObstacle | 0 |  |
| `0x01F4` | rightHandObj | 0 |  |
| `0x01F8` | leftHandObj | 0 |  |
| `0x01FC` | m_grabStrength | 0 |  |
| `0x0200` | mTrailLeftArm | 0 |  |
| `0x0204` | basePosition | 12 |  |
| `0x0210` | currentThrowMove | 60 |  |
| `0x0214` | cActiveZone | 0 |  |
| `0x0218` | path | 80 |  |

### Player (764 bytes)

* — inherits from Humanoid — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Humanoid | 616 | ← base class |
| `0x0268` | miscCounter | 0 |  |
| `0x026C` | dead | 0 |  |
| `0x0270` | encounterDialogIndex | 0 |  |
| `0x0274` | encounterDialogResetCounter | 0 |  |
| `0x0278` | m_enemyEncounterPhase | 0 |  |
| `0x027C` | myCheckpoint | 56 |  |
| `0x02B4` | livesLeft | 0 |  |
| `0x02B8` | playerFlags | 0 |  |
| `0x02BC` | whichJump | 4 |  |
| `0x02C0` | buttonHold | 0 |  |
| `0x02C2` | buttonTap | 0 |  |
| `0x02C4` | lastJumpTime | 0 |  |
| `0x02C8` | pJS | 12 | pointer |
| `0x02CC` | takeOffYHeight | 0 |  |
| `0x02D0` | initialJumpOrientation | 12 |  |
| `0x02DC` | soundFlag | 0 |  |
| `0x02E0` | desiredRot | 12 |  |
| `0x02EC` | playersAsyncIdleCallback | 8 |  |
| `0x02F4` | inActiveIdleAnimation | 4 |  |
| `0x02F8` | inActiveIdleLoopType | 0 |  |

### Camera (492 bytes)

* — inherits from DynamicThing — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | DynamicThing | 200 | ← base class |
| `0x00C8` | cameraActive | 0 |  |
| `0x00CC` | desiredPos | 12 |  |
| `0x00D8` | curLookAtPos | 12 |  |
| `0x00E4` | desiredLookAtPos | 12 |  |
| `0x00F0` | curVel | 12 |  |
| `0x00FC` | desiredVel | 12 |  |
| `0x0108` | curLookAtVel | 12 |  |
| `0x0114` | desiredLookAtVel | 12 |  |
| `0x0120` | positionOffset | 12 |  |
| `0x012C` | movementTime | 12 |  |
| `0x0138` | trackingTime | 12 |  |
| `0x0144` | curFOV | 0 |  |
| `0x0148` | desiredFOV | 0 |  |
| `0x014C` | curFOVv | 0 |  |
| `0x0150` | desiredFOVv | 0 |  |
| `0x0154` | defaultFOV | 0 |  |
| `0x0158` | fovUpdateTime | 12 |  |
| `0x0164` | target | 96 |  |
| `0x0168` | lookAtJoint | 0 |  |
| `0x016C` | cut | 0 |  |
| `0x0170` | OrderHandler | 8 |  |
| `0x0178` | yzQuadrant | 0 |  |
| `0x017A` | xzQuadrant | 0 |  |
| `0x017C` | internalOrient | 12 |  |
| `0x0188` | cameraAnchor | 48 |  |
| `0x018C` | fov | 0 |  |
| `0x0190` | shakeFrames | 0 |  |
| `0x0194` | cam | 56 |  |
| `0x01CC` | asyncAnimEnum | 0 |  |
| `0x01D0` | asyncAnim | 36 |  |
| `0x01D4` | cameraAnim | 104 |  |
| `0x01D8` | cameraFlags | 0 |  |
| `0x01DC` | timer | 0 |  |
| `0x01E0` | shakeStrength | 12 |  |

### Game (140 bytes)

* — inherits from Manager — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Manager | 28 | ← base class |
| `0x001C` | stateHandler | 0 |  |
| `0x0020` | curState | 0 |  |
| `0x0024` | prevState | 0 |  |
| `0x0028` | managers | 12 |  |
| `0x0034` | errorScreen | 4 |  |
| `0x0038` | myHandlers | 36 |  |
| `0x005C` | variableFrameRateHandlers | 36 |  |
| `0x0080` | inputMask | 8 |  |
| `0x0088` | m_preErrorState | 4 |  |

### World (160 bytes)

* — inherits from Manager — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Manager | 28 | ← base class |
| `0x001C` | fog | 20 |  |
| `0x0020` | buf | 0 |  |
| `0x0024` | levelList | 8 |  |
| `0x0028` | levelNames | 0 |  |
| `0x002C` | petalNames | 0 |  |
| `0x0030` | petalSoundIDs | 0 |  |
| `0x0034` | highestPetal | 0 |  |
| `0x0038` | levelCount | 0 |  |
| `0x003C` | currentLevel | 0 |  |
| `0x0040` | desiredLevel | 0 |  |
| `0x0044` | currentPetal | 0 |  |
| `0x0048` | desiredPetal | 0 |  |
| `0x004C` | previousLevel | 0 |  |
| `0x0050` | lcf | 0 |  |
| `0x0054` | loadCallbacks | 12 |  |
| `0x0060` | unloadCallbacks | 12 |  |
| `0x006C` | generalSwitches | 12 |  |
| `0x0078` | playerSwitches | 12 |  |
| `0x0084` | enemySwitches | 12 |  |
| `0x0090` | humanoidSwitches | 12 |  |
| `0x009C` | has_level | 0 |  |

### Pickup (336 bytes)

* — inherits from DynamicThing — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | DynamicThing | 200 | ← base class |
| `0x00C8` | carrier | 96 |  |
| `0x00CC` | mThrowie | 96 |  |
| `0x00D0` | carryPoint | 0 |  |
| `0x00D4` | fightingMoves | 0 |  |
| `0x00D8` | idleEnum | 0 |  |
| `0x00DC` | moveStructure | 0 |  |
| `0x00E0` | pickupFromHighMove | 0 |  |
| `0x00E4` | pickupFromLowMove | 0 |  |
| `0x00E8` | activePickup | 0 |  |
| `0x00EC` | throwMove | 0 |  |
| `0x00F0` | collisionPointArray | 48 |  |
| `0x0120` | collisionPointArraySize | 0 |  |
| `0x0124` | grabPoint | 12 |  |
| `0x0130` | damage | 0 |  |
| `0x0134` | breakOnDrop | 0 |  |
| `0x0138` | isStatic | 0 |  |
| `0x013C` | fungFuGrip | 0 |  |
| `0x0140` | m_pWeaponSound | 0 |  |
| `0x0144` | pickupFlags | 0 |  |
| `0x0148` | soundFlag | 0 |  |
| `0x014C` | pickupStrayWeapon | 0 |  |

### Model (88 bytes)

* — inherits from ccNode — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | ccNode | 24 | ← base class |
| `0x0018` | drawable | 4 |  |
| `0x001C` | drawableType | 0 |  |
| `0x0020` | anim | 104 |  |
| `0x0024` | myShadow | 0 |  |
| `0x0028` | myAmbient | 0 |  |
| `0x002C` | myHLights | 0 |  |
| `0x0030` | numAllocatedHardwareLights | 0 |  |
| `0x0034` | rotation | 12 |  |
| `0x0040` | position | 12 |  |
| `0x004C` | myThing | 96 |  |
| `0x0050` | modelDrawFlags | 0 |  |
| `0x0054` | fudgeValue | 0 |  |

### Boss (616 bytes)

* — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Humanoid | 616 |  |

### DestructibleThing (168 bytes)

* — inherits from Obstacle — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Obstacle | 116 | ← base class |
| `0x0074` | mGEffectName | 0 |  |
| `0x0078` | mRenderTypes | 0 |  |
| `0x007C` | mDialog | 0 |  |
| `0x0080` | mOnlyOnce | 0 |  |
| `0x0084` | mDamageLevel | 0 |  |
| `0x0088` | mExtraDamage | 0 |  |
| `0x008C` | mItemType | 0 |  |
| `0x0090` | mItemName | 0 |  |
| `0x0094` | mItemAttribs | 16 |  |
| `0x00A4` | m_pDestructibleSound | 0 |  |

### Generator (216 bytes)

* — inherits from Obstacle — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Obstacle | 116 | ← base class |
| `0x0074` | dbr | 60 |  |
| `0x00B0` | dbrAttributeIndex | 0 |  |
| `0x00B4` | mDelayCount | 0 |  |
| `0x00B8` | mDelay | 0 |  |
| `0x00BC` | objType | 0 |  |
| `0x00C0` | nItems | 0 |  |
| `0x00C4` | nActive | 0 |  |
| `0x00C8` | nMaxItems | 0 |  |
| `0x00CC` | nMaxActive | 0 |  |
| `0x00D0` | aObj | 8 |  |
| `0x00D4` | objName | 0 |  |

### Explosive (168 bytes)

* — inherits from Obstacle — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Obstacle | 116 | ← base class |
| `0x0074` | state | 4 |  |
| `0x0078` | triggerTime | 0 |  |
| `0x007C` | explodeTime | 0 |  |
| `0x0080` | explodeEffect | 0 |  |
| `0x0084` | radiusA | 0 |  |
| `0x0088` | radiusB | 0 |  |
| `0x008C` | oldBox | 16 |  |
| `0x009C` | timer | 0 |  |
| `0x00A0` | mGEffectName | 0 |  |
| `0x00A4` | mRenderTypes | 0 |  |

### Collectible (176 bytes)

* — inherits from Obstacle — has vtable*

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| `0x0000` | Obstacle | 116 | ← base class |
| `0x0074` | mAnim | 104 |  |
| `0x0078` | mAnimB | 32 |  |
| `0x007C` | mCurrentFrame | 0 |  |
| `0x0080` | mModelIndex | 0 |  |
| `0x0084` | mFloatAngle | 0 |  |
| `0x0088` | mFloatRadius | 0 |  |
| `0x008C` | mFlipAngle | 0 |  |
| `0x0090` | mInitialPos | 12 |  |
| `0x009C` | mCollectible | 0 |  |
| `0x00A0` | mPointsToAdd | 0 |  |
| `0x00A4` | mHealthToAdd | 0 |  |
| `0x00A8` | mLivesToAdd | 0 |  |
| `0x00AC` | mTimer | 0 |  |

---

## Class Method Reference

### AI : Manager [116 bytes]

*Source: C:\CHAN\GAME\SRC\AI\AI.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80054180` | `AI()` | 32 |
| `0x8005436C` | `InternalOpen()` | 24 |
| `0x8005438C` | `InternalClose()` | 24 |
| `0x800543AC` | `AddActiveZone(DBVolume*)` | 32 |
| `0x80054404` | `AddThingNoTagList(const char*, unsigned short, const tagLVector*, const _RMVECT16*, const char*, const DBRoot*)` | 64 |
| `0x800553A4` | `InternalReset()` | 24 |
| `0x800553DC` | `privMoveList(ccList&)` | 40 |
| `0x80055BE4` | `UpdatePositions(ccList&)` | 24 |
| `0x80055CB4` | `KillThings(long)` | 32 |
| `0x80055D10` | `MoveThings()` | 32 |
| `0x80055F14` | `MoveThingsObstacleCollisions()` | 24 |
| `0x80055F44` | `MoveThingsPickupCollisions()` | 24 |
| `0x80055F6C` | `MoveCamera()` | 24 |
| `0x80055FC8` | `PopulateActiveZones()` | 32 |
| `0x80056038` | `PopulateActiveZonesPaths()` | 48 |
| `0x80056164` | `PopulateActiveZonesSubZones()` | 32 |
| `0x80056214` | `Populate()` | 72 |
| `0x800567CC` | `UnPopulate(short)` | 32 |
| `0x80056A34` | `PopulateBlock()` | 24 |
| `0x80056AC4` | `UnpopulateBlock()` | 24 |
| `0x80056B04` | `GetPickupWithinReach(Humanoid*)` | 56 |
| `0x80056BFC` | `CheckObstacleAttack(Obstacle**, int, const Humanoid*, const FightingCollisionAttackType*)` | 112 |
| `0x80056DE4` | `FindThing(unsigned long)` | 32 |
| `0x800CA650` | `ParseBehaviourAttribScript()` | 144 |

### ActiveZone : ccNode [104 bytes]

*Source: C:\CHAN\GAME\SRC\AI\ACTIVEZN.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A6B40` | `GetActiveZoneCenterPoint()` | 16 |
| `0x800A6BC4` | `ActiveZone(DBVolume*, unsigned long)` | 32 |
| `0x800A6DA4` | `AddHumanoidToOverlordMembers(Humanoid*)` | 0 |
| `0x800A6DEC` | `RemoveHumanoidFromOverlordMembers(Humanoid*)` | 0 |
| `0x800A6E30` | `GetNumberOfThinkingMembers()` | 0 |
| `0x800A6E7C` | `AllowedToMoveIn(Humanoid*)` | 24 |
| `0x800A6F40` | `AddLinearPath(LinearPath&)` | 24 |
| `0x800A6F6C` | `AddSubZoneVolume(SubZoneVolume&)` | 24 |
| `0x800A6F98` | `FindFirstValidPath(Humanoid*)` | 32 |
| `0x800A701C` | `IsInActiveZone(Thing*)` | 24 |
| `0x800A7040` | `DoAICheck(LinearPath*, long, Humanoid*)` | 136 |
| `0x800A76AC` | `IsPathNodeTerminator(LinearPath*, long)` | 8 |
| `0x800A7718` | `AllowBreakoffOfDestinationNode(LinearPath*, long)` | 8 |
| `0x800A7784` | `DoActionsAtNode(LinearPath*, long, Humanoid*)` | 56 |

### Arrow : Obstacle [844 bytes]

*Source: C:\CHAN\GAME\SRC\AI\ARROW.CPP, \CHAN\GAME\INC\AI\ARROW.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001B688` | `Arrow(const tagLVector*, unsigned short)` | 24 |
| `0x8001B6E8` | `AnalyzeMesh(DBRoot*)` | 64 |
| `0x8001B8CC` | `CreateModel(const char*)` | 24 |
| `0x8001B8EC` | `DeleteModel()` | 24 |
| `0x8001B90C` | `Reset()` | 16 |
| `0x8001BBD8` | `Think()` | 112 |
| `0x8001BDDC` | `UpdatePosition()` | 0 |
| `0x8001BDE4` | `Draw()` | 24 |
| `0x8001BE24` | `HandleHumanoidCollision(Humanoid*)` | 0 |
| `0x8001BE2C` | `HandlePickupCollision(Pickup*)` | 0 |

### Behaviour : ccNode [280 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BEHAVE.CPP, C:\CHAN\GAME\SRC\AI\BEHAVEB.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001CB20` | `_ButchDMS()` | 144 |
| `0x8001D178` | `_ButchDMS_Charge()` | 144 |
| `0x8001D314` | `_GrontarDMS()` | 72 |
| `0x8001D7CC` | `_PaulDMS()` | 176 |
| `0x8001E0B0` | `_OscarDMS()` | 112 |
| `0x8001E7DC` | `_OscarHenchmanDMS()` | 40 |
| `0x8001EDB4` | `CounterAttack()` | 0 |
| `0x8001EEE0` | `_DanteDMS_Phase1()` | 48 |
| `0x8001F2B4` | `_DanteDMS_Phase2()` | 40 |
| `0x8001F678` | `_DanteDMS_Phase3()` | 32 |
| `0x8007458C` | `Behaviour(Humanoid*, unsigned long, long)` | 40 |
| `0x800746DC` | `SetAIHandler(unsigned long)` | 32 |
| `0x80074888` | `InActiveZone()` | 24 |
| `0x800748AC` | `Process()` | 24 |
| `0x80074914` | `ComplexAttack()` | 24 |
| `0x80074BA0` | `BreakOffPathAndFight()` | 72 |
| `0x80074C80` | `_AiFollowPath()` | 32 |
| `0x80074F5C` | `_PlayerUserControl()` | 72 |
| `0x800753C4` | `_NisControl()` | 24 |
| `0x80075408` | `MoveToDestinationPoint(unsigned long)` | 32 |
| `0x800754DC` | `_Idle()` | 40 |
| `0x800757EC` | `_BackoffAndTaunt()` | 32 |
| `0x80075A68` | `_BackOutOfTheFight()` | 24 |
| `0x80075BE4` | `_NDMS()` | 96 |
| `0x80076870` | `NavigateWorld(long&)` | 152 |
| `0x80076C84` | `NavigateEnemies(int)` | 88 |
| `0x80077004` | `_SubwayDodgeRight()` | 104 |
| `0x80077200` | `_SubwayDodgeLeft()` | 104 |
| `0x800773FC` | `_SubwayDodgeJump()` | 24 |
| `0x800774BC` | `_Jumping()` | 24 |
| `0x8007762C` | `_DieWhenWeHitTheGround()` | 40 |
| `0x800776FC` | `_GetBackIntoActiveZone()` | 72 |
| `0x800778C4` | `InitPathAIState(LinearPath*)` | 0 |
| `0x800778F8` | `LookAheadFloorCheck(long, long, long)` | 56 |
| `0x80077A14` | `LookAheadWallCheck(long, long, long)` | 32 |
| `0x80077A4C` | `DisableInputProcessing()` | 0 |

### BehaviourAttrib : ccNode [56 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BEHAVE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80074434` | `BehaviourAttrib()` | 24 |

### Blast : Obstacle [232 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BLAST.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80015AB0` | `Blast(const tagLVector*, unsigned short)` | 24 |
| `0x80015B54` | `AnalyzeMesh(DBRoot*)` | 88 |
| `0x80016284` | `CreateSound()` | 24 |
| `0x800162E4` | `UpdateSound()` | 24 |
| `0x8001631C` | `ReleaseSound()` | 24 |
| `0x80016368` | `CreateModel(const char*)` | 24 |
| `0x8001639C` | `DeleteModel()` | 24 |
| `0x800163C8` | `Reset()` | 24 |
| `0x800164A4` | `Activate()` | 24 |
| `0x800164C4` | `Deactivate()` | 24 |
| `0x80016528` | `Think()` | 64 |
| `0x80016A10` | `Trigger(Thing*, const char*, Thing*)` | 40 |
| `0x80016ACC` | `Draw()` | 32 |
| `0x80016B3C` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x80016B44` | `HandleHumanoidCollision(Humanoid*)` | 96 |

### Boss [616 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BOSS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001A758` | `Boss(const tagLVector*, unsigned short)` | 24 |
| `0x8001A7C4` | `CreateModel(const char*)` | 40 |
| `0x8001A8EC` | `SetActionState(unsigned long, long)` | 24 |
| `0x8001A90C` | `_Collapse()` | 32 |
| `0x8001AAF4` | `_CrouchUp()` | 24 |
| `0x8001AB68` | `TestAndSetBackGrab()` | 0 |

### Butch : Boss [620 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BOSS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001AB70` | `Butch(const tagLVector*)` | 24 |
| `0x8001AC00` | `SetActionState(unsigned long, long)` | 40 |
| `0x8001AD30` | `_Stomp()` | 88 |
| `0x8001AEF0` | `_Charge()` | 24 |
| `0x8001B058` | `_ThrowPot()` | 32 |

### Chair [192 bytes]

*Source: C:\CHAN\GAME\SRC\AI\TABLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001531C` | `Chair(const tagLVector*, unsigned short)` | 24 |
| `0x8001537C` | `AnalyzeMesh(DBRoot*)` | 24 |
| `0x8001539C` | `CreateModel(const char*)` | 24 |
| `0x800153BC` | `DeleteModel()` | 0 |
| `0x800153C4` | `Think()` | 24 |
| `0x800153E4` | `UpdatePosition()` | 0 |
| `0x800153EC` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x800153F4` | `Throw(long, long, const _RMVECT16&, const tagLVector&)` | 32 |
| `0x80015434` | `HandleHumanoidCollision(Humanoid*)` | 32 |

### Collectible : Obstacle [176 bytes]

*Source: C:\CHAN\GAME\SRC\AI\COLLECT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80012744` | `Collectible(const tagLVector*, unsigned short)` | 24 |
| `0x800127CC` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x80012970` | `CreateModel(const char*)` | 56 |
| `0x80012C5C` | `DeleteModel()` | 24 |
| `0x80012C7C` | `Reset()` | 0 |
| `0x80012C84` | `Think()` | 48 |
| `0x80012EC8` | `UpdatePosition()` | 0 |
| `0x80012ED0` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x80012ED8` | `HandleHumanoidCollision(Humanoid*)` | 32 |

### Conveyor : Obstacle [148 bytes]

*Source: C:\CHAN\GAME\SRC\AI\CONVEYOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001BE34` | `Conveyor(const tagLVector*, unsigned short)` | 24 |
| `0x8001BEE4` | `AnalyzeMesh(DBRoot*)` | 112 |
| `0x8001C1DC` | `CreateModel(const char*)` | 24 |
| `0x8001C214` | `DeleteModel()` | 24 |
| `0x8001C240` | `Reset()` | 24 |
| `0x8001C260` | `Think()` | 32 |
| `0x8001C2BC` | `UpdatePosition()` | 0 |
| `0x8001C2C4` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x8001C2CC` | `HandleHumanoidCollision(Humanoid*)` | 40 |

### Crusher : Obstacle [176 bytes]

*Source: C:\CHAN\GAME\SRC\AI\CRUSHER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001F6A4` | `Crusher(const tagLVector*, unsigned short)` | 24 |
| `0x8001F760` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x8001F924` | `CreateModel(const char*)` | 24 |
| `0x8001F980` | `DeleteModel()` | 24 |
| `0x8001F9D0` | `Reset()` | 0 |
| `0x8001F9F0` | `Think()` | 48 |
| `0x8001FAE0` | `UpdatePosition()` | 0 |
| `0x8001FAE8` | `Draw()` | 24 |
| `0x8001FB08` | `Move()` | 40 |
| `0x8001FC58` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x8001FC98` | `HandleHumanoidCollision(Humanoid*)` | 144 |

### Dante : Boss [684 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BOSS.CPP, \CHAN\GAME\INC\AI\BOSS.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001B60C` | `Dante(const tagLVector*)` | 24 |
| `0x8001B8D0` | `AnalyzeMesh(DBRoot*)` | 32 |
| `0x8001BA80` | `Think()` | 24 |
| `0x8001BAA0` | `SetActionState(unsigned long, long)` | 40 |
| `0x8001BBA4` | `_Stand()` | 24 |
| `0x8001BC18` | `_Taunt()` | 24 |
| `0x8001BC70` | `_GotHitHigh()` | 24 |
| `0x8001BCC4` | `_GotHitMed()` | 24 |
| `0x8001BD18` | `_GotHitFreeForm()` | 24 |
| `0x8001BD38` | `_ThrowFreeFall()` | 24 |
| `0x8001BD58` | `_MissilePrepare()` | 32 |
| `0x8001BDE4` | `_MissileAttack()` | 176 |
| `0x8001C358` | `_TargetMissileAttack()` | 136 |
| `0x8001C70C` | `LoadCombatDialog()` | 24 |
| `0x8001C72C` | `PlayCombatKnockDownDialog(DamageTypesTags)` | 24 |
| `0x8001CA10` | `GetTargetingFrame(const StrikeFightingMove&)` | 0 |
| `0x8001CA18` | `PlayCombatThrowDialog()` | 24 |

### DestructibleThing : Obstacle [168 bytes]

*Source: C:\CHAN\GAME\SRC\AI\DESTROY.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80010000` | `Destroy()` | 72 |
| `0x800100F0` | `GenerateItem()` | 152 |
| `0x800102B8` | `DestructibleThing(const tagLVector*, unsigned short)` | 24 |
| `0x80010388` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x80010658` | `CreateModel(const char*)` | 24 |
| `0x800106AC` | `DeleteModel()` | 24 |
| `0x800106FC` | `Reset()` | 0 |
| `0x80010710` | `Think()` | 24 |
| `0x80010770` | `UpdatePosition()` | 0 |
| `0x80010778` | `Draw()` | 24 |
| `0x800107B8` | `MovePassengers()` | 24 |
| `0x800107D8` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x80010834` | `HandleHumanoidCollision(Humanoid*)` | 144 |
| `0x80010B64` | `CareAboutAttack() const` | 0 |
| `0x80010B6C` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 24 |
| `0x80010BFC` | `HandleObstacleDestructibleThingCollision(Obstacle*)` | 56 |
| `0x80010CF8` | `GetFloorMaterial() const` | 32 |

### Door : Obstacle [196 bytes]

*Source: C:\CHAN\GAME\SRC\AI\DOOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001AB0C` | `Door(const tagLVector*, unsigned short)` | 24 |
| `0x8001ABB4` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x8001AE88` | `CreateModel(const char*)` | 24 |
| `0x8001AEA8` | `DeleteModel()` | 24 |
| `0x8001AEC8` | `Reset()` | 40 |
| `0x8001AF5C` | `Think()` | 40 |
| `0x8001B058` | `UpdatePosition()` | 0 |
| `0x8001B060` | `Draw()` | 24 |
| `0x8001B0D4` | `Trigger()` | 24 |
| `0x8001B1D8` | `Open()` | 24 |
| `0x8001B210` | `Move()` | 24 |
| `0x8001B2FC` | `DeathCheck()` | 48 |
| `0x8001B43C` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x8001B47C` | `HandleHumanoidCollision(Humanoid*)` | 120 |
| `0x8001B624` | `TeleportPlayer()` | 24 |

### DynamicObstacle : Obstacle [192 bytes]

*Source: C:\CHAN\GAME\SRC\AI\TABLE.CPP, \CHAN\GAME\INC\AI\TABLE.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80013F24` | `DynamicObstacle(const tagLVector*, unsigned short)` | 40 |
| `0x80013FFC` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x80014120` | `CreateModel(const char*)` | 24 |
| `0x80014140` | `DeleteModel()` | 0 |
| `0x80014148` | `Reset()` | 0 |
| `0x80014158` | `Think()` | 24 |
| `0x80014198` | `Move()` | 160 |
| `0x80014484` | `Draw()` | 24 |
| `0x800144B4` | `AddForce(long, const _RMVECT16*)` | 104 |
| `0x8001456C` | `AddMomentVector(const _RMVECT16&, const tagLVector&)` | 104 |
| `0x80014738` | `MovePassengers()` | 24 |
| `0x80014758` | `Throw(long, long, const _RMVECT16&, const tagLVector&)` | 56 |
| `0x80014818` | `UpdatePosition()` | 0 |
| `0x80014820` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x80014828` | `HandleHumanoidCollision(Humanoid*)` | 128 |
| `0x80014934` | `HandleObjectInterAction(Humanoid*)` | 112 |
| `0x80014BF8` | `Destroy()` | 56 |
| `0x80014CA8` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 0 |
| `0x80014CB0` | `HandleEnvironmentCollision(const tagLVector&)` | 176 |
| `0x80015470` | `CareAboutAttack() const` | 0 |

### DynamicThing : Thing [200 bytes]

*Source: C:\CHAN\GAME\SRC\AI\THING.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80061B44` | `AddForce(long, const _RMVECT16*)` | 104 |
| `0x80061C78` | `Land()` | 0 |
| `0x80061CC4` | `DisembarkObstacle(const tagLVector&)` | 16 |
| `0x80061D68` | `DynamicThing(const tagLVector*, unsigned short)` | 32 |
| `0x80061E38` | `Reset()` | 24 |
| `0x80061EC4` | `Move()` | 120 |
| `0x800624C4` | `Disembark()` | 24 |
| `0x80062550` | `GetTicketIssuer()` | 0 |
| `0x8006272C` | `UpdatePosition()` | 24 |
| `0x800628C8` | `HandleLand(long)` | 0 |

### EnemyGenerator : Generator [300 bytes]

*Source: C:\CHAN\GAME\SRC\AI\GENERATOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80011414` | `GenerateObject()` | 72 |
| `0x8001157C` | `Reset()` | 0 |
| `0x8001158C` | `AnalyzeMesh(DBRoot*)` | 40 |
| `0x800117D8` | `Think()` | 32 |
| `0x80011914` | `SetupTargets(const char*)` | 40 |

### Explosive : Obstacle [168 bytes]

*Source: C:\CHAN\GAME\SRC\AI\EXPLODE.CPP, \CHAN\GAME\INC\AI\EXPLODE.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001304C` | `Explosive(const tagLVector*, unsigned short)` | 24 |
| `0x800130E8` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x800132C8` | `CreateModel(const char*)` | 24 |
| `0x800132E8` | `DeleteModel()` | 24 |
| `0x80013308` | `Reset()` | 24 |
| `0x8001333C` | `CheckObstacleCollisions()` | 48 |
| `0x800134BC` | `AdjustCollisionBox()` | 40 |
| `0x80013554` | `ExplodeThing(Thing*)` | 144 |
| `0x80013A18` | `Draw()` | 24 |
| `0x80013A48` | `Think()` | 40 |
| `0x80013BAC` | `Trigger(Thing*, const char*, Thing*)` | 0 |
| `0x80013BD0` | `ExplosiveTrigger(int, const char*)` | 0 |
| `0x80013C0C` | `MovePassengers()` | 24 |
| `0x80013C2C` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x80013CA8` | `HandleHumanoidCollision(Humanoid*)` | 128 |
| `0x80013E40` | `HandleObstacleCollision(Obstacle*)` | 24 |
| `0x80013ECC` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 24 |
| `0x80013F1C` | `CareAboutAttack() const` | 0 |

### FrontEndVolume : Obstacle [132 bytes]

*Source: C:\CHAN\GAME\SRC\AI\FEVOLUME.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001A758` | `FrontEndVolume(const tagLVector*, unsigned short)` | 24 |
| `0x8001A7C4` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x8001A8D8` | `CreateModel(const char*)` | 0 |
| `0x8001A8EC` | `DeleteModel()` | 0 |
| `0x8001A900` | `Reset()` | 0 |
| `0x8001A908` | `Think()` | 0 |
| `0x8001A910` | `UpdatePosition()` | 0 |
| `0x8001A918` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x8001A920` | `HandleHumanoidCollision(Humanoid*)` | 24 |
| `0x8001A9CC` | `HandleVolumeExit(Humanoid*)` | 72 |

### Generator : Obstacle [216 bytes]

*Source: C:\CHAN\GAME\SRC\AI\GENERATOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80010D54` | `Generator(const tagLVector*, unsigned short)` | 32 |
| `0x80010EEC` | `GenerateObject(int)` | 48 |
| `0x80011018` | `AnalyzeMesh(DBRoot*)` | 32 |
| `0x8001121C` | `CreateModel(const char*)` | 0 |
| `0x80011230` | `DeleteModel()` | 24 |
| `0x80011250` | `Reset()` | 32 |
| `0x80011300` | `Think()` | 32 |
| `0x800113F4` | `UpdatePosition()` | 0 |
| `0x800113FC` | `Trigger(Thing*, const char*, Thing*)` | 0 |
| `0x80011404` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x8001140C` | `HandleHumanoidCollision(Humanoid*)` | 0 |

### Grontar [616 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BOSS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001B0BC` | `Grontar(const tagLVector*)` | 24 |
| `0x8001B148` | `SetActionState(unsigned long, long)` | 40 |
| `0x8001B20C` | `_Stand()` | 24 |
| `0x8001B274` | `_Run()` | 24 |
| `0x8001B2DC` | `_Taunt()` | 24 |
| `0x8001B344` | `_Straif()` | 24 |
| `0x8001B3AC` | `_DiveRoll()` | 24 |
| `0x8001B47C` | `_GotHitHigh()` | 24 |
| `0x8001B4F0` | `_GotHitMed()` | 24 |
| `0x8001B564` | `FindFoe(unsigned long, long, int)` | 24 |
| `0x8001B5A0` | `GetTargetingFrame(const StrikeFightingMove&)` | 0 |

### HorizontalPole : Obstacle [156 bytes]

*Source: C:\CHAN\GAME\SRC\AI\HPOLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80015478` | `HorizontalPole(const tagLVector*, unsigned short)` | 24 |
| `0x800154FC` | `AnalyzeMesh(DBRoot*)` | 88 |
| `0x80015750` | `CreateModel(const char*)` | 0 |
| `0x80015764` | `DeleteModel()` | 0 |
| `0x8001576C` | `Reset()` | 0 |
| `0x80015774` | `Think()` | 0 |
| `0x8001577C` | `UpdatePosition()` | 0 |
| `0x80015784` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x8001578C` | `HandleHumanoidCollision(Humanoid*)` | 104 |

### Humanoid : DynamicThing [616 bytes]

*Source: C:\CHAN\GAME\SRC\AI\FIGHTANI.CPP, C:\CHAN\GAME\SRC\AI\HUMANOID.CPP, \CHAN\GAME\INC\AI\HUMANOID.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80062A34` | `Humanoid(const tagLVector*, unsigned short)` | 40 |
| `0x80062DC0` | `Reset()` | 24 |
| `0x80062E54` | `AnalyzeMesh(DBRoot*)` | 40 |
| `0x80063210` | `Activate()` | 32 |
| `0x80063270` | `Deactivate()` | 24 |
| `0x800632B4` | `CreateModel(const char*)` | 40 |
| `0x800634C4` | `CreateSound()` | 24 |
| `0x80063514` | `DeleteModel()` | 24 |
| `0x80063614` | `ReleaseSound()` | 24 |
| `0x80063660` | `ProcessControl()` | 24 |
| `0x80063690` | `LoadEnemyTaunts()` | 32 |
| `0x80063808` | `Think()` | 32 |
| `0x80063A88` | `Draw()` | 128 |
| `0x80064100` | `Move()` | 24 |
| `0x80064194` | `HandleAnimationControl()` | 64 |
| `0x800643B8` | `RestorePositionFromBip01()` | 56 |
| `0x80064528` | `HandleCollisionReactionStates(long, long)` | 24 |
| `0x8006475C` | `HandleCollisionSound(long)` | 24 |
| `0x80064808` | `HandleCollision(Thing*, long, ...)` | 56 |
| `0x80064B98` | `FaceThing(Thing*, int)` | 40 |
| `0x80064BD0` | `FacePoint(const tagLVector&, int)` | 64 |
| `0x80064D7C` | `FaceThingDesired(Thing*)` | 40 |
| `0x80064DB4` | `FacePointDesired(const tagLVector&)` | 56 |
| `0x80064EA8` | `SetDesiredMoveDirection(long)` | 0 |
| `0x80064EB0` | `FaceAngleY(long, int)` | 16 |
| `0x80064F94` | `FindFoe(unsigned long, long, int)` | 56 |
| `0x8006511C` | `SetTarget(Humanoid*)` | 24 |
| `0x800651B4` | `SetHumanoidTarget(Humanoid*)` | 32 |
| `0x80065200` | `ReleaseTarget()` | 0 |
| `0x80065230` | `IsInActiveZone()` | 32 |
| `0x80065290` | `IsTargetInActiveZone()` | 32 |
| `0x800652F4` | `IsInMyFieldOf(Humanoid*, long, long)` | 48 |
| `0x80065340` | `IsInMyDesiredFieldOf(Humanoid*, long, long)` | 48 |
| `0x8006538C` | `ProcessAction()` | 24 |
| `0x800653F4` | `SetTauntAnim(long)` | 24 |
| `0x800654C4` | `SetIdleAnimation(long, int)` | 48 |
| `0x800655B4` | `StitchIdleAnimation()` | 32 |
| `0x80065618` | `TestIdleAnimation()` | 0 |
| `0x80065680` | `SetActionState(unsigned long, long)` | 96 |
| `0x80066CA0` | `_Stand()` | 24 |
| `0x80066E3C` | `_DiveRoll()` | 64 |
| `0x8006710C` | `_Taunt()` | 24 |
| `0x80067288` | `_Pause()` | 24 |
| `0x800672EC` | `_Run()` | 24 |
| `0x80067610` | `_Straif()` | 96 |
| `0x80067DBC` | `_Jump()` | 48 |
| `0x80067F2C` | `_Fall()` | 0 |
| `0x80067F34` | `_FindLatch()` | 0 |
| `0x80067F3C` | `_Push()` | 0 |
| `0x80067F44` | `_Teetering()` | 0 |
| `0x80067F4C` | `_WallJump()` | 0 |
| `0x80067F54` | `_Hotfoot()` | 24 |
| `0x800680B8` | `GetImpactRegion(const tagLVector&)` | 72 |
| `0x80068264` | `_JumpKick()` | 0 |
| `0x8006826C` | `_BackGrabCharacter()` | 32 |
| `0x80068338` | `_BackGrabCharacterRelease()` | 32 |
| `0x800683C4` | `_BackGrabCharacterReceivePreLatch()` | 24 |
| `0x80068410` | `_BackGrabCharacterReceiveLatch()` | 24 |
| `0x80068460` | `_BackGrabCharacterReceive()` | 24 |
| `0x80068508` | `_Pickup()` | 32 |
| `0x800685A8` | `_Throw()` | 40 |
| `0x80068718` | `_TableThrow()` | 40 |
| `0x8006882C` | `_GotHitHigh()` | 32 |
| `0x800688B4` | `_GotHitMed()` | 32 |
| `0x80068914` | `_GotHitBackGrab()` | 24 |
| `0x800689B4` | `_GotHitLow()` | 32 |
| `0x80068A14` | `_GotHitCrusher()` | 24 |
| `0x80068A64` | `_GotHitFire()` | 24 |
| `0x80068AB4` | `_Stunned()` | 24 |
| `0x80068B78` | `_SpinBack()` | 24 |
| `0x80068BC8` | `_FlyingBack()` | 24 |
| `0x80068C9C` | `_Floating()` | 24 |
| `0x80068D38` | `TestAndSetRisingAttack()` | 24 |
| `0x80068DD4` | `_Collapse()` | 32 |
| `0x80068EF8` | `_ThrowCharacterReceive()` | 40 |
| `0x80068FFC` | `_ThrowFreeFall()` | 24 |
| `0x8006909C` | `BodyThrowAttack(long)` | 128 |
| `0x800691DC` | `_Dead()` | 24 |
| `0x8006934C` | `_CrouchUp()` | 24 |
| `0x80069420` | `_CounterAttack()` | 24 |
| `0x80069518` | `_CounterAttackPreLatch()` | 24 |
| `0x800695A8` | `_CounterAttackLatch()` | 24 |
| `0x80069638` | `_CounterAttackRecovery()` | 24 |
| `0x80069688` | `CheckForLanding()` | 32 |
| `0x800697C4` | `CheckForPickup()` | 32 |
| `0x80069894` | `CheckforPickup(unsigned long)` | 40 |
| `0x80069968` | `DoJump()` | 56 |
| `0x80069A04` | `_DoStand()` | 24 |
| `0x80069A34` | `_DoRun()` | 32 |
| `0x80069A70` | `_CallNextAction()` | 24 |
| `0x80069AB4` | `HandleLand(long)` | 48 |
| `0x80069B94` | `_LadderLatchTop()` | 24 |
| `0x80069C2C` | `_LadderLatch()` | 24 |
| `0x80069CC8` | `_LadderDismount()` | 24 |
| `0x80069CF8` | `_ClimbLadder()` | 40 |
| `0x80069FEC` | `PrepareLedgeLatch(const tagLVector*, const _RMVECT16*)` | 64 |
| `0x8006A1D8` | `CheckForLedges()` | 120 |
| `0x8006A3B0` | `CheckForLedges2(_RMVECT16&, tagLVector&, long)` | 96 |
| `0x8006A4C4` | `_LedgeLatch()` | 24 |
| `0x8006A538` | `_LedgePullup()` | 40 |
| `0x8006A5D4` | `_SlopeSlide()` | 0 |
| `0x8006A5DC` | `_HorizontalPoleSwing()` | 0 |
| `0x8006A5E4` | `FillSphere(tSphere&) const` | 24 |
| `0x8006A650` | `ProcessSoundEvent(long, long)` | 24 |
| `0x8006A6D4` | `ProcessFightingMove(const FightingMove&, long)` | 24 |
| `0x8006A714` | `ProcessFightingMoveStrikeJoint(const FightingJoint&, long, long, long, int, int)` | 168 |
| `0x8006ADC8` | `GetTargetingFrame(const StrikeFightingMove&)` | 0 |
| `0x8006AE0C` | `ProcessGenericFightingMove(const StrikeFightingMove&, long)` | 56 |
| `0x8006B0A0` | `ProcessBodyThrow(const ThrowFightingMove&, long)` | 64 |
| `0x8006B5A8` | `FindSiblingWithRequestedCommand(const FightingComboNode*, long)` | 0 |
| `0x8006B5EC` | `FindSiblingWithRequestedCommand(const FightingComboNode*, long, long)` | 0 |
| `0x8006B658` | `FindChildWithRequestedCommand(const FightingComboNode*, long)` | 24 |
| `0x8006B67C` | `FindChildWithRequestedCommand(const FightingComboNode*, long, long)` | 24 |
| `0x8006B6A0` | `DropPickup(int, int)` | 40 |
| `0x8006B778` | `FightTargetAndThrowLatch(FightingType)` | 24 |
| `0x8006B8C8` | `EnterCombatCombo()` | 32 |
| `0x8006BA30` | `SetCurrentFightingNode()` | 40 |
| `0x8006BC40` | `DoTrailCallbacks(const FightingJoint&)` | 88 |
| `0x8006BEB4` | `DisableTrailCallbacks()` | 32 |
| `0x8006BF04` | `ReSyncOrientation(const FightingMove&)` | 0 |
| `0x8006BFE0` | `_ProcessFightingComboNode()` | 40 |
| `0x8006C2C8` | `TestAndSetWeaponKungFU()` | 0 |
| `0x8006C31C` | `TestWallContextFightingRequestRemap()` | 64 |
| `0x8006C3EC` | `_GotHitFreeForm()` | 24 |
| `0x8006C42C` | `LetGoOfLedge()` | 48 |
| `0x8006C564` | `_NISMode()` | 0 |
| `0x8006C59C` | `QuickCheckWallCollision(long, long, long, long)` | 96 |
| `0x8006C5E8` | `CheckWallCollision(long, long, long, long, long&, _RMVECT16&, tagLVector&, long&, long&)` | 144 |
| `0x8006C750` | `CheckDWOCollision(long, long)` | 104 |
| `0x8006C9B8` | `CheckWallConstraint(unsigned long, unsigned long, long, long&, tagLVector&)` | 88 |
| `0x8006CB4C` | `LoadDialog(unsigned long, long)` | 32 |
| `0x8006CBA0` | `PlayDialog(unsigned long, unsigned long)` | 32 |
| `0x8006CC38` | `PlayDialogBasedOnPriority(long, long)` | 32 |
| `0x8006CCF8` | `KillDialog(int, long, long)` | 40 |
| `0x8006CDC0` | `KillDialogBasedOnID(int, long)` | 32 |
| `0x8006CE5C` | `HasEnemyTauntDialog()` | 32 |
| `0x8006CEB4` | `SubtractHitPoints(unsigned short)` | 24 |
| `0x8006CF00` | `Kill()` | 0 |
| `0x8006CF50` | `_Killed()` | 0 |
| `0x8006CF7C` | `GetStraifPhase()` | 0 |
| `0x8006CFFC` | `RequestAction(unsigned long)` | 0 |
| `0x8006D014` | `DeleteLeftHandObj()` | 24 |
| `0x8006D070` | `DeleteRightHandObj()` | 24 |
| `0x8006D0CC` | `SetRightHandObj(Pickup*)` | 24 |
| `0x8006D104` | `PlayCombatThrowDialog()` | 0 |
| `0x8006D10C` | `PlayCombatKnockDownDialog(DamageTypesTags)` | 0 |
| `0x8006D114` | `LoadCombatDialog()` | 0 |
| `0x8006D11C` | `HandleHitShock(DamageTypesTags)` | 0 |
| `0x8007DF28` | `_BackGrabCharacterLatch()` | 40 |
| `0x8007E014` | `TestAndSetBackGrab()` | 56 |

### KickNRoll : Obstacle [172 bytes]

*Source: C:\CHAN\GAME\SRC\AI\KICK.CPP, \CHAN\GAME\INC\AI\KICK.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001C398` | `KickNRoll(const tagLVector*, unsigned short)` | 24 |
| `0x8001C444` | `AnalyzeMesh(DBRoot*)` | 112 |
| `0x8001C6FC` | `CreateModel(const char*)` | 24 |
| `0x8001C750` | `DeleteModel()` | 24 |
| `0x8001C7A0` | `Reset()` | 0 |
| `0x8001C7B8` | `Think()` | 24 |
| `0x8001C850` | `Move()` | 104 |
| `0x8001CB60` | `HandleEnvironmentCollision(tagLVector&)` | 256 |
| `0x8001CEEC` | `Destroy()` | 56 |
| `0x8001CF74` | `UpdatePosition()` | 0 |
| `0x8001CF7C` | `Draw()` | 24 |
| `0x8001CFAC` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x8001CFEC` | `MovePassengers()` | 80 |
| `0x8001D178` | `HandleHumanoidCollision(Humanoid*)` | 144 |
| `0x8001D45C` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 48 |
| `0x8001D5CC` | `CareAboutAttack() const` | 0 |

### KnockDown : Obstacle [192 bytes]

*Source: C:\CHAN\GAME\SRC\AI\KNOCKDWN.CPP, \CHAN\GAME\INC\AI\KNOCKDWN.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001D5D4` | `KnockDown(const tagLVector*, unsigned short)` | 24 |
| `0x8001D6B8` | `AnalyzeMesh(DBRoot*)` | 32 |
| `0x8001D8D8` | `CreateModel(const char*)` | 24 |
| `0x8001D92C` | `Draw()` | 24 |
| `0x8001D9A8` | `DeleteModel()` | 24 |
| `0x8001D9F8` | `Reset()` | 24 |
| `0x8001DA48` | `Think()` | 56 |
| `0x8001DB8C` | `Move()` | 0 |
| `0x8001DCA0` | `UpdateCollisionBox()` | 40 |
| `0x8001E120` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x8001E16C` | `HandleHumanoidCollision(Humanoid*)` | 176 |
| `0x8001E4EC` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 104 |
| `0x8001F66C` | `CareAboutAttack() const` | 0 |
| `0x8001F674` | `UpdatePosition()` | 0 |

### Ladder : Obstacle [188 bytes]

*Source: C:\CHAN\GAME\SRC\AI\LADDER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80089CA4` | `Ladder(const tagLVector*, unsigned short)` | 24 |
| `0x80089D40` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x80089F48` | `CreateModel(const char*)` | 0 |
| `0x80089F5C` | `DeleteModel()` | 0 |
| `0x80089F70` | `Reset()` | 24 |
| `0x80089FD4` | `Think()` | 24 |
| `0x8008A068` | `Move()` | 0 |
| `0x8008A070` | `DeathCheck()` | 24 |
| `0x8008A10C` | `UpdatePosition()` | 0 |
| `0x8008A114` | `Draw()` | 0 |
| `0x8008A11C` | `Trigger()` | 24 |
| `0x8008A19C` | `TeleportPlayer()` | 24 |
| `0x8008A1F4` | `CloseHatch()` | 24 |
| `0x8008A258` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x8008A260` | `HandleHumanoidCollision(Humanoid*)` | 104 |
| `0x8008A570` | `PutHumanoidOnLadder(Humanoid*)` | 32 |
| `0x8008A5DC` | `CheckForLedges(_RMVECT16&, tagLVector&)` | 96 |

### Launcher : Obstacle [148 bytes]

*Source: C:\CHAN\GAME\SRC\AI\LAUNCHER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001FE34` | `Launcher(const tagLVector*, unsigned short)` | 24 |
| `0x8001FEA4` | `AnalyzeMesh(DBRoot*)` | 56 |
| `0x8001FFB0` | `CreateModel(const char*)` | 40 |
| `0x800200F0` | `Draw()` | 24 |
| `0x80020164` | `DeleteModel()` | 24 |
| `0x8002018C` | `Reset()` | 0 |
| `0x800201A4` | `Think()` | 48 |
| `0x80020290` | `UpdatePosition()` | 0 |
| `0x80020298` | `HandlePickupCollision(Pickup*)` | 32 |
| `0x80020328` | `HandleHumanoidCollision(Humanoid*)` | 176 |
| `0x80020730` | `HandleHumanoidDefaultLaunch(Humanoid*)` | 48 |

### Obstacle : Thing [116 bytes]

*Source: C:\CHAN\GAME\SRC\AI\OBSTACLE.CPP, \CHAN\GAME\INC\AI\OBSTACLE.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007AE04` | `Draw()` | 24 |
| `0x8007AEF0` | `FillVectorArray(tagLVector*, const DBLine&)` | 24 |
| `0x8007AF14` | `FillVectorArray(tagLVector*, unsigned long, const DBLine&)` | 0 |
| `0x8007AF6C` | `FillCollisionBox(tagCollisionBox&, const DBRoot&, unsigned long)` | 24 |
| `0x8007B068` | `FillVehicleCollisionBoxes(tagCollisionBox&, tagCollisionBox&, const DBRoot&, unsigned long, long)` | 40 |
| `0x8007B184` | `FillBoxCentre(tagLVector&, const tagLVector&, const _RMVECT16&, const tagCollisionBox&)` | 48 |
| `0x8007B2F0` | `GetWorldFloorHeight(const tagLVector&)` | 64 |
| `0x8007B328` | `GetYRotation(long, long)` | 24 |
| `0x8007B398` | `CorrectThingPosition(const tagLVector&, const tagLVector&, long, long, const tagCollisionBox&, const tagLVector&, const tagLVector&, long, long, long, tagLVector&, _RMVECT16&, tagLVector&, _)` | 272 |
| `0x8007BE24` | `SetCollisionBox(const tagCollisionBox&)` | 24 |
| `0x8007BE84` | `LedgeCheck(const tagCollisionBox&, const _RMVECT16&, const tagLVector&, Humanoid*)` | 88 |
| `0x8007C034` | `HandlePickupObstacleCollision(Pickup*)` | 48 |
| `0x8007C178` | `HandleHumanoidObstacleCollision(Humanoid*)` | 144 |
| `0x8007C6DC` | `DetectObstacleAboveLedge(const _RMVECT16&, const tagLVector&)` | 72 |
| `0x8007C7B0` | `DetectObstacle(const tagLVector&, const tagLVector&, long)` | 152 |
| `0x8007CA08` | `Obstacle(const tagLVector*, unsigned short)` | 24 |
| `0x8007CAA4` | `Load(tReadChunk&, void**)` | 24 |
| `0x8007CB5C` | `ClearPetalAnimList()` | 0 |
| `0x8007CB88` | `GetAnimation(long)` | 0 |
| `0x8007CBC4` | `AnalyzeMesh(DBRoot*)` | 32 |
| `0x8007CC64` | `CreateModel(const char*)` | 0 |
| `0x8007CC6C` | `AllocateAndCreateModel(const char*)` | 32 |
| `0x8007CD2C` | `AllocateAndCreateShadow()` | 32 |
| `0x8007CD94` | `DeleteModel()` | 0 |
| `0x8007CD9C` | `Reset()` | 0 |
| `0x8007CDA4` | `Think()` | 24 |
| `0x8007CDD4` | `Move()` | 0 |
| `0x8007CDDC` | `UpdatePosition()` | 0 |
| `0x8007CDE4` | `Trigger()` | 0 |
| `0x8007CDEC` | `ExplosiveTrigger(int, const char*)` | 0 |
| `0x8007CDF4` | `GetDeltaVelocity() const` | 0 |
| `0x8007CE00` | `Trigger(Thing*, const char*, Thing*)` | 0 |
| `0x8007CE08` | `FillSphere(tSphere&) const` | 40 |
| `0x8007D034` | `GetPhysical() const` | 0 |
| `0x8007D180` | `GetFloorMaterial() const` | 0 |
| `0x8007D188` | `GetObstacleFloorHeight(const tagLVector&) const` | 0 |
| `0x8007D198` | `StaticGetObstacleFloorHeight(const tagLVector&)` | 80 |
| `0x8007D354` | `UpdateShadowFloorHeight()` | 56 |
| `0x8007D40C` | `MovePassengersBasic()` | 72 |
| `0x8007D8A4` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 0 |
| `0x8007D8AC` | `CareAboutAttack() const` | 0 |

### Oscar [616 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BOSS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001C8FC` | `Oscar(const tagLVector*)` | 24 |
| `0x8001C994` | `SetActionState(unsigned long, long)` | 24 |
| `0x8001C9B4` | `_Straif()` | 24 |

### Paul [616 bytes]

*Source: C:\CHAN\GAME\SRC\AI\BOSS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001C77C` | `Paul(const tagLVector*)` | 24 |
| `0x8001C808` | `_Straif()` | 24 |
| `0x8001C834` | `_GotHitHigh()` | 24 |
| `0x8001C888` | `_GotHitMed()` | 24 |
| `0x8001C8DC` | `SetActionState(unsigned long, long)` | 24 |

### Pendulum : Obstacle [180 bytes]

*Source: C:\CHAN\GAME\SRC\AI\PENDULUM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80024DF0` | `Pendulum(const tagLVector*, unsigned short)` | 24 |
| `0x80024EA0` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x8002520C` | `CreateModel(const char*)` | 24 |
| `0x8002525C` | `DeleteModel()` | 24 |
| `0x800252AC` | `Reset()` | 0 |
| `0x800252B4` | `Think()` | 112 |
| `0x800256C4` | `UpdatePosition()` | 0 |
| `0x800256CC` | `Draw()` | 24 |
| `0x800256EC` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x80025720` | `HandleHumanoidCollision(Humanoid*)` | 168 |

### Pickup : DynamicThing [336 bytes]

*Source: C:\CHAN\GAME\SRC\AI\PICKUP.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8006D45C` | `Pickup(const tagLVector*, unsigned short)` | 32 |
| `0x8006D618` | `Reset()` | 40 |
| `0x8006D688` | `CreateModel(const char*)` | 32 |
| `0x8006D710` | `AnalyzeMesh(DBRoot*)` | 64 |
| `0x8006D984` | `Think()` | 24 |
| `0x8006D9D0` | `SetupPickup(Thing*, unsigned long)` | 0 |
| `0x8006DA00` | `UpdatePosition()` | 112 |
| `0x8006DBC0` | `Release(Thing*, ccList*, _RMVECT16*, long)` | 104 |
| `0x8006DD00` | `Move()` | 24 |
| `0x8006DD30` | `HandleCollision(Thing*, long, ...)` | 24 |
| `0x8006DD5C` | `DamageExtra()` | 0 |
| `0x8006DD64` | `PlayEffect()` | 56 |
| `0x8006DDDC` | `PickupDeactivate() const` | 0 |
| `0x8006DE54` | `FillSphere(tSphere&) const` | 24 |
| `0x8006DEDC` | `GetCollisionYMin() const` | 88 |
| `0x8006DF9C` | `GetWeaponSoundPtr()` | 0 |
| `0x8006DFA8` | `SetPickupMove(long)` | 0 |
| `0x8006DFD8` | `GetPickupMove()` | 0 |
| `0x8006DFEC` | `GetPickupMoveGrabFrame()` | 0 |
| `0x8006E008` | `GetThrowMove()` | 0 |
| `0x8006E01C` | `GetThrowMoveThrowFrame()` | 0 |

### Platform : Obstacle [324 bytes]

*Source: C:\CHAN\GAME\SRC\AI\PLATFORM.CPP, \CHAN\GAME\INC\AI\PLATFORM.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80020844` | `Platform(const tagLVector*, unsigned short)` | 24 |
| `0x800209BC` | `Draw()` | 24 |
| `0x80020A24` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x80021578` | `CreateModel(const char*)` | 32 |
| `0x80021698` | `DeleteModel()` | 24 |
| `0x800216E8` | `Reset()` | 24 |
| `0x80021918` | `Think()` | 96 |
| `0x80021E54` | `OnNewPathNode()` | 64 |
| `0x800222F4` | `OnXorZRot()` | 200 |
| `0x80022788` | `Move()` | 128 |
| `0x80022B90` | `Teeter()` | 144 |
| `0x80023190` | `Bob()` | 80 |
| `0x8002337C` | `MovePassengers()` | 112 |
| `0x80023830` | `SetPlatformToPathNode(const char*)` | 88 |
| `0x80023A80` | `Trigger(Thing*, const char*, Thing*)` | 48 |
| `0x80023CE4` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x80023D38` | `HandleEnvironmentCollision(tagLVector&)` | 144 |
| `0x80023FC4` | `CheckForSquash(const tagLVector&, const _RMVECT16&, const tagCollisionCylinder&)` | 16 |
| `0x800240C4` | `HandleHumanoidCollision(Humanoid*)` | 304 |
| `0x80024B7C` | `GetDeltaVelocity() const` | 0 |
| `0x80024BA0` | `AtEndOfPath()` | 24 |
| `0x80024BE0` | `DeathCheck()` | 24 |
| `0x80024C70` | `FillSphere(tSphere&) const` | 24 |
| `0x80024CD0` | `GetFloorMaterial() const` | 32 |
| `0x80024D04` | `GetInitialPos()` | 0 |

### Player : Humanoid [764 bytes]

*Source: C:\CHAN\GAME\SRC\AI\PLAYER.CPP, \CHAN\GAME\INC\AI\PLAYER.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002FA80` | `Player(const tagLVector*)` | 24 |
| `0x8002FC24` | `Reset()` | 24 |
| `0x8002FD34` | `CreateModel(const char*)` | 40 |
| `0x8002FE30` | `Think()` | 40 |
| `0x800300B0` | `SignalEnemyGetUp()` | 0 |
| `0x80030100` | `Move()` | 24 |
| `0x80030120` | `DoJump()` | 56 |
| `0x800301D8` | `DoJump(long)` | 48 |
| `0x8003027C` | `GetViewSpot(tagLVector*, tagLVector*)` | 24 |
| `0x800303BC` | `SetActionState(unsigned long, long)` | 72 |
| `0x8003123C` | `_InactiveIdle()` | 40 |
| `0x80031350` | `_Stand()` | 80 |
| `0x80031A78` | `_Flip()` | 64 |
| `0x80031C68` | `_Jump()` | 112 |
| `0x80032368` | `FallingPhysics()` | 48 |
| `0x80032444` | `_Fall()` | 24 |
| `0x800324E8` | `_HardFall()` | 32 |
| `0x80032560` | `_HardLand()` | 24 |
| `0x800325CC` | `_Run()` | 88 |
| `0x80032A48` | `_Push()` | 24 |
| `0x80032B80` | `_PushObject()` | 24 |
| `0x80032C70` | `_Teetering()` | 0 |
| `0x80032C78` | `DoWallJump()` | 64 |
| `0x80032D8C` | `_WallJump()` | 32 |
| `0x80032EB0` | `_Collapse()` | 24 |
| `0x80032F48` | `_DoStand()` | 24 |
| `0x80032F8C` | `_HorizontalPoleSwing()` | 160 |
| `0x8003352C` | `_LedgeLatch()` | 48 |
| `0x800337A8` | `_LedgePullup()` | 40 |
| `0x80033858` | `_Dead()` | 24 |
| `0x8003389C` | `_SlopeSlide()` | 88 |
| `0x80033C00` | `CheckForLanding()` | 40 |
| `0x80033D0C` | `OnCheckpoint()` | 24 |
| `0x80033D9C` | `SetLivesLeft(long)` | 0 |
| `0x80033DB8` | `_LadderDismount()` | 24 |
| `0x80033DD8` | `_ClimbLadder()` | 24 |
| `0x80033DF8` | `_TableRoll()` | 64 |
| `0x80033FF8` | `_Straif()` | 24 |
| `0x80034140` | `PlayerSingleEncounterCheak()` | 40 |
| `0x80034210` | `LoadPlayerTauntResponse(Humanoid*)` | 24 |
| `0x80034290` | `PlayPlayerTauntResponse()` | 24 |
| `0x8003431C` | `SignalEnemyDead(Humanoid*)` | 0 |
| `0x800343D4` | `EnterCombatCombo()` | 24 |
| `0x800343F4` | `LoadCombatDialog()` | 32 |
| `0x80034510` | `PlayCombatKnockDownDialog(DamageTypesTags)` | 24 |
| `0x800345B8` | `HandleHitShock(DamageTypesTags)` | 24 |
| `0x80034618` | `PlayCombatThrowDialog()` | 24 |

### Pushable : Obstacle [172 bytes]

*Source: C:\CHAN\GAME\SRC\AI\PUSHABLE.CPP, \CHAN\GAME\INC\AI\PUSHABLE.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80017CCC` | `Pushable(const tagLVector*, unsigned short)` | 24 |
| `0x80017D70` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x80017EE0` | `CreateModel(const char*)` | 24 |
| `0x80017F34` | `DeleteModel()` | 24 |
| `0x80017F84` | `Reset()` | 0 |
| `0x80017FAC` | `Think()` | 32 |
| `0x8001811C` | `Move()` | 56 |
| `0x80018244` | `MovePassengers()` | 104 |
| `0x800184A8` | `UpdatePosition()` | 0 |
| `0x800184B0` | `HandleEnvironmentCollision(const tagLVector&)` | 168 |
| `0x800188E8` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x80018928` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 72 |
| `0x80018AF8` | `HandleHumanoidCollision(Humanoid*)` | 208 |
| `0x80019264` | `GetFloorMaterial() const` | 32 |
| `0x80019298` | `CareAboutAttack() const` | 0 |

### SimpleBox [24 bytes]

*Source: C:\CHAN\GAME\SRC\AI\SIMPLBOX.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AEB64` | `SimpleBox()` | 0 |
| `0x800AEB84` | `SetBox(DBVolume*)` | 0 |
| `0x800AEBDC` | `IsValid() const` | 0 |
| `0x800AEBF4` | `IsInside(const tagLVector&) const` | 0 |
| `0x800AEC88` | `IsInside(long, long) const` | 0 |

### SlipperyFloor : Obstacle [124 bytes]

*Source: C:\CHAN\GAME\SRC\AI\SLIPPERY.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800122C4` | `SlipperyFloor(const tagLVector*, unsigned short)` | 24 |
| `0x80012390` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x80012454` | `CreateModel(const char*)` | 0 |
| `0x80012468` | `DeleteModel()` | 24 |
| `0x80012488` | `Reset()` | 0 |
| `0x80012490` | `Think()` | 0 |
| `0x80012498` | `UpdatePosition()` | 0 |
| `0x800124A0` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x800124AC` | `HandleHumanoidCollision(Humanoid*)` | 32 |
| `0x800125A4` | `DoTrailEffect(Humanoid*)` | 96 |

### Stack : Obstacle [236 bytes]

*Source: C:\CHAN\GAME\SRC\AI\KNOCKDWN.CPP, \CHAN\GAME\INC\AI\KNOCKDWN.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001E6E0` | `LoadDialog(unsigned long, long)` | 48 |
| `0x8001E768` | `Stack(const tagLVector*, unsigned short)` | 24 |
| `0x8001E820` | `Draw()` | 24 |
| `0x8001E850` | `AnalyzeMesh(DBRoot*)` | 64 |
| `0x8001EA18` | `CreateModel(const char*)` | 32 |
| `0x8001EB0C` | `DeleteModel()` | 24 |
| `0x8001EB5C` | `Reset()` | 0 |
| `0x8001EB68` | `UpdatePosition()` | 0 |
| `0x8001EB70` | `Wobble()` | 24 |
| `0x8001EBE8` | `Fall()` | 24 |
| `0x8001EC24` | `FinishStack()` | 152 |
| `0x8001EE28` | `Think()` | 32 |
| `0x8001EEE8` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x8001EF1C` | `HandleHumanoidCollision(Humanoid*)` | 168 |
| `0x8001F320` | `HandleAttack(Humanoid*, DamageTypesTags, long, short)` | 24 |
| `0x8001F370` | `UpdateCollisionBox()` | 40 |
| `0x8001F3E8` | `TriggerStackAnimation()` | 0 |
| `0x8001F3F4` | `SetupCallbacks()` | 32 |
| `0x8001F55C` | `SetupJointPosition(int, G10tagLVector)` | 0 |
| `0x8001F664` | `CareAboutAttack() const` | 0 |

### SubZoneVolume : ccNode [48 bytes]

*Source: C:\CHAN\GAME\SRC\AI\ACTIVEZN.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A69D8` | `SubZoneVolume(DBVolume*)` | 32 |
| `0x800A6A70` | `IsInSubZoneVolume(Thing*)` | 32 |
| `0x800A6ADC` | `Draw(const _RMVECT16&)` | 56 |

### Table [192 bytes]

*Source: C:\CHAN\GAME\SRC\AI\TABLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80015190` | `Table(const tagLVector*, unsigned short)` | 24 |
| `0x800151F0` | `AnalyzeMesh(DBRoot*)` | 24 |
| `0x80015210` | `CreateModel(const char*)` | 24 |
| `0x80015230` | `DeleteModel()` | 0 |
| `0x80015238` | `Think()` | 24 |
| `0x80015258` | `UpdatePosition()` | 0 |
| `0x80015260` | `Throw(long, long, const _RMVECT16&, const tagLVector&)` | 32 |
| `0x800152A0` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x800152A8` | `HandleHumanoidCollision(Humanoid*)` | 32 |

### Teleporter : Obstacle [136 bytes]

*Source: C:\CHAN\GAME\SRC\AI\TELEPORT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AA48C` | `Teleporter(const tagLVector*, unsigned short)` | 24 |
| `0x800AA500` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x800AA618` | `CreateModel(const char*)` | 0 |
| `0x800AA62C` | `DeleteModel()` | 0 |
| `0x800AA640` | `Reset()` | 0 |
| `0x800AA648` | `Think()` | 0 |
| `0x800AA650` | `UpdatePosition()` | 0 |
| `0x800AA658` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x800AA660` | `HandleHumanoidCollision(Humanoid*)` | 64 |

### Thing : ccNode [96 bytes]

*Source: C:\CHAN\GAME\SRC\AI\THING.CPP, \CHAN\GAME\INC\AI\THING.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80061558` | `Thing(const tagLVector*, unsigned short)` | 32 |
| `0x800616BC` | `Think()` | 24 |
| `0x800616EC` | `Draw()` | 24 |
| `0x80061760` | `Reset()` | 0 |
| `0x80061790` | `Activate()` | 32 |
| `0x8006182C` | `Deactivate()` | 32 |
| `0x800618E0` | `CreateModel(const char*)` | 32 |
| `0x80061AAC` | `DeleteModel()` | 24 |
| `0x80061B08` | `HandleCollision(Thing*, long, ...)` | 0 |
| `0x80061BFC` | `ClearFloorHeight()` | 0 |
| `0x80061C38` | `SetFloorHeight(long)` | 0 |
| `0x80061D60` | `Move()` | 0 |
| `0x80062400` | `AddPassenger(DynamicThing*)` | 32 |
| `0x8006247C` | `RemPassenger(Ticket*)` | 24 |
| `0x80062504` | `RemAllPassengers()` | 32 |
| `0x80062574` | `GetThingHandle()` | 24 |
| `0x800625C0` | `DistanceFromPointXZ(const tagLVector&) const` | 24 |
| `0x800625F4` | `DistanceFromPoint(const tagLVector&) const` | 24 |
| `0x80062638` | `GetViewSpot(tagLVector*, tagLVector*)` | 0 |
| `0x80062680` | `AnalyzeMesh(DBRoot*)` | 32 |
| `0x8006286C` | `FillSphere(tSphere&) const` | 0 |
| `0x80062874` | `GetObjectToWorldSpaceVector(const _RMVECT16&, _RMVECT16&)` | 64 |
| `0x800628D0` | `Kill()` | 0 |
| `0x800628E4` | `GetSoundPosPtr()` | 0 |
| `0x800628EC` | `GetInitialPos()` | 0 |
| `0x800628F4` | `UpdatePosition()` | 0 |

### ThingHandle [8 bytes]

*Source: \CHAN\GAME\INC\AI\THING.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800628FC` | `Close()` | 24 |

### ThrowingGenerator : Generator [256 bytes]

*Source: C:\CHAN\GAME\SRC\AI\GENERATOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80011A80` | `GenerateObject(int)` | 136 |
| `0x80011DC8` | `TargetInFOF()` | 24 |
| `0x80011E34` | `AnalyzeMesh(DBRoot*)` | 40 |
| `0x800120F4` | `Reset()` | 0 |
| `0x80012104` | `Think()` | 32 |

### Ticket : ccNode [32 bytes]

*Source: C:\CHAN\GAME\SRC\AI\THING.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800627F0` | `Ticket(Thing*, DynamicThing*)` | 32 |

### TrapDoor : Obstacle [192 bytes]

*Source: C:\CHAN\GAME\SRC\AI\TRAPDOOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80016FB8` | `TrapDoor(const tagLVector*, unsigned short)` | 24 |
| `0x80017054` | `AnalyzeMesh(DBRoot*)` | 32 |
| `0x800172EC` | `CreateModel(const char*)` | 24 |
| `0x8001730C` | `DeleteModel()` | 24 |
| `0x8001732C` | `Reset()` | 0 |
| `0x80017334` | `Trigger(Thing*, const char*, Thing*)` | 0 |
| `0x80017360` | `Think()` | 24 |
| `0x80017484` | `UpdatePosition()` | 0 |
| `0x8001748C` | `Draw()` | 32 |
| `0x800174C8` | `Move()` | 24 |
| `0x800175F4` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x80017628` | `HandleHumanoidCollision(Humanoid*)` | 192 |
| `0x80017AE4` | `SetupCollisionBox()` | 56 |
| `0x80017CC4` | `GetFloorMaterial() const` | 0 |

### TriggerThing : Obstacle [132 bytes]

*Source: C:\CHAN\GAME\SRC\AI\TRIGGER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A9958` | `TriggerThing(const tagLVector*, unsigned short)` | 24 |
| `0x800A9A20` | `AnalyzeMesh(DBRoot*)` | 320 |
| `0x800A9BE8` | `CreateModel(const char*)` | 0 |
| `0x800A9BFC` | `DeleteModel()` | 24 |
| `0x800A9C1C` | `Reset()` | 0 |
| `0x800A9C24` | `Think()` | 0 |
| `0x800A9C2C` | `UpdatePosition()` | 0 |
| `0x800A9C34` | `HandleCollision(Thing*)` | 40 |
| `0x800A9D20` | `HandlePickupCollision(Pickup*)` | 24 |
| `0x800A9D40` | `HandleHumanoidCollision(Humanoid*)` | 24 |

### Untouchable : Obstacle [156 bytes]

*Source: C:\CHAN\GAME\SRC\AI\UNTOUCH.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A6330` | `Untouchable(const tagLVector*, unsigned short)` | 24 |
| `0x800A63DC` | `AnalyzeMesh(DBRoot*)` | 48 |
| `0x800A64C8` | `CreateModel(const char*)` | 0 |
| `0x800A64DC` | `DeleteModel()` | 0 |
| `0x800A64F0` | `Reset()` | 24 |
| `0x800A6540` | `Think()` | 40 |
| `0x800A6604` | `Draw()` | 56 |
| `0x800A6684` | `UpdatePosition()` | 0 |
| `0x800A668C` | `HandlePickupCollision(Pickup*)` | 0 |
| `0x800A6694` | `HandleHumanoidCollision(Humanoid*)` | 80 |
| `0x800A68FC` | `CreateSound()` | 24 |
| `0x800A695C` | `UpdateSound()` | 24 |
| `0x800A698C` | `ReleaseSound()` | 24 |

### AmbientLight [8 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LIGHTS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A4264` | `AmbientLight()` | 24 |
| `0x800A42CC` | `SetToWorldAmbient()` | 0 |
| `0x800A42E4` | `SetDesired(unsigned long)` | 0 |
| `0x800A42EC` | `SetPortToLight()` | 32 |

### AnimCallback : CharMgrCallback [24 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CHARMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8003B964` | `AnimCallback(Q22AI10ThingTypesiUlP15CharMgrCallback)` | 0 |
| `0x8003B99C` | `Callback()` | 24 |

### AnimLight : DBLight [48 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LIGHTS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A23A8` | `CalcLight(long&, N21)` | 40 |

### AnimStructure : AnimStructureBasic [104 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80070740` | `AnimStructure(long, tAnimation*, long, Model*, DrawableBasic*)` | 56 |
| `0x80070B6C` | `ReAttachTree(long, long)` | 32 |
| `0x80070C20` | `SetLoopType(long, int)` | 32 |
| `0x80070D30` | `ResetCountsToAnim()` | 24 |
| `0x80070DB8` | `ForceFrame(long)` | 24 |
| `0x80070E1C` | `ExecuteHandler(int)` | 32 |
| `0x80071108` | `ProcessHumanoidCB()` | 32 |
| `0x8007119C` | `Loop()` | 0 |
| `0x80071200` | `LoopReverse()` | 0 |
| `0x80071234` | `HoldFirst()` | 24 |
| `0x80071278` | `HoldLast()` | 24 |
| `0x800712BC` | `RunToLast()` | 24 |
| `0x80071300` | `IncFrame()` | 24 |
| `0x800713D8` | `DecFrame()` | 24 |
| `0x80071410` | `RunToLastBlend()` | 48 |

### AnimationManager : Manager [40 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\ANIMMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80057118` | `AnimationManager()` | 24 |
| `0x800571CC` | `InternalOpen()` | 0 |
| `0x800571D4` | `InternalClose()` | 24 |
| `0x80057228` | `InternalReset()` | 0 |
| `0x80057230` | `PurgePetal()` | 40 |
| `0x800572B4` | `PurgeLevel()` | 24 |
| `0x80057308` | `GetMiscAnim(unsigned long)` | 0 |

### AnimationMatrices [660 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\ANIMMAT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80078880` | `AnimationMatrices()` | 40 |
| `0x80078908` | `SetHumanoid(Humanoid*)` | 0 |
| `0x80078910` | `GetHumanoid()` | 0 |
| `0x8007891C` | `Copy() const` | 0 |
| `0x80078928` | `SetupCallbacks(Model*, const char**)` | 40 |
| `0x80078BD0` | `SetupExtraCallbacks(Model*, const char**)` | 48 |
| `0x80078CAC` | `SetExtraCallbacks(Q217AnimationMatrices14AM_MatrixTypesi)` | 40 |
| `0x80078E0C` | `CopyMatrix(unsigned long, tSJoint*)` | 160 |
| `0x80078E80` | `Swap()` | 0 |
| `0x80078E98` | `GetMatrix(unsigned long) const` | 0 |
| `0x80078EB8` | `GetAttack(unsigned long, tagLVector&, tagLVector&) const` | 0 |
| `0x80078F64` | `GetWeaponAttack(unsigned long, const tagLVector&, tagLVector&, tagLVector&) const` | 0 |

### AsyncAnimCallback : CharMgrCallback [16 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CAMERA.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004A310` | `Callback()` | 24 |

### Block [104 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\BLOCK.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80052B24` | `Block()` | 0 |
| `0x80052BB0` | `Init(const DBVolume*)` | 32 |
| `0x80052EA0` | `SetDimension(const tagLVector&, const tagLVector&)` | 0 |
| `0x80052F80` | `Parse(unsigned long, char*)` | 24 |
| `0x80052FF0` | `Unload()` | 24 |
| `0x80053024` | `PointInBlock(const tagLVector&) const` | 0 |
| `0x800530B0` | `GetNextBlockNumber() const` | 0 |
| `0x800530D4` | `GetPrevBlockNumber() const` | 0 |
| `0x800530F8` | `Draw(const tagLVector&)` | 32 |
| `0x8005328C` | `LoadPrim(void*)` | 24 |

### BlockManager : Manager [168 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\BLKMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004FE98` | `BlockManager()` | 24 |
| `0x80050014` | `AllocBlockPool()` | 24 |
| `0x800500CC` | `FreeBlockPool()` | 24 |
| `0x8005010C` | `_LoadBlocksFunc(Callback*)` | 40 |
| `0x80050220` | `_UnloadBlocksFunc(Callback*)` | 24 |
| `0x800502BC` | `InternalOpen()` | 32 |
| `0x80050384` | `InternalClose()` | 24 |
| `0x800503A4` | `InternalReset()` | 0 |
| `0x800503AC` | `RemoveBlock()` | 32 |
| `0x80050480` | `RemoveBlockHelper()` | 40 |
| `0x80050624` | `AddBlock(BlockNode*)` | 32 |
| `0x800506BC` | `CrossedBoundary()` | 0 |
| `0x800506E8` | `DemandLoading()` | 32 |
| `0x8005085C` | `LoadBlock(unsigned long, Block*)` | 40 |
| `0x800508CC` | `UnloadBlocks()` | 32 |
| `0x800509D4` | `LoadSingleBlockAndParse(unsigned long, char*)` | 40 |
| `0x80050A98` | `LoadBlocks(unsigned long)` | 48 |
| `0x80050C04` | `GetBlockNumber(const tagLVector&)` | 32 |
| `0x80050C70` | `IsValidBlockNumber(unsigned long)` | 0 |
| `0x80050CB4` | `InLoadList(unsigned long) const` | 0 |
| `0x80050CF4` | `InDrawList(unsigned long) const` | 0 |
| `0x80050D44` | `InActiveList(unsigned long) const` | 0 |
| `0x80050D94` | `UpdateToBeLoadedList(unsigned long)` | 96 |
| `0x80051750` | `UpdateAlreadyLoadedList()` | 40 |
| `0x800518C4` | `GetBlock(unsigned long)` | 0 |

### Button [40 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CONTROL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002D898` | `Button()` | 24 |
| `0x8002D934` | `Reset()` | 0 |
| `0x8002D960` | `Default()` | 24 |
| `0x8002D980` | `GetState()` | 0 |
| `0x8002D9B0` | `_RawHandler(long)` | 0 |
| `0x8002D9FC` | `_OneshotHandler(long)` | 0 |
| `0x8002DA58` | `_DeadHandler(long)` | 0 |
| `0x8002DA60` | `_AnalogHandler(long)` | 0 |
| `0x8002DA68` | `_AutoRepeatHandler(long)` | 0 |
| `0x8002DB40` | `SetMode(short)` | 0 |

### CBVEffect : Effects [72 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\WEFFECT.CPP, \CHAN\GAME\INC\GEN\WEFFECT.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008CD74` | `Create2(unsigned long, int)` | 32 |
| `0x8008CE18` | `CBVEffect()` | 24 |
| `0x8008CE98` | `Create()` | 40 |
| `0x8008CF24` | `Create2(int)` | 40 |
| `0x8008CF94` | `Update()` | 24 |
| `0x8008CFC4` | `PutBackEffect()` | 24 |
| `0x8008D094` | `Display(int)` | 0 |

### CBVPrimData [72 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\UVDATA.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009864C` | `Load(tReadChunk&, void**)` | 32 |
| `0x80098744` | `FindCBVPrimInfo(unsigned long)` | 0 |
| `0x80098790` | `Unload()` | 32 |
| `0x800987FC` | `Update(int, tPrimGeom*)` | 56 |
| `0x8009890C` | `CBVPrimData()` | 0 |
| `0x80098960` | `Init(int, int, unsigned long, int, int, int)` | 48 |
| `0x80098B2C` | `Release()` | 24 |
| `0x80098B4C` | `FreeColourInfo()` | 32 |
| `0x80098BE0` | `Update()` | 40 |

### Camera : DynamicThing [492 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CAMERA.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80047AD4` | `Camera(const tagLVector*)` | 24 |
| `0x80047BE0` | `PurgeAnims()` | 32 |
| `0x80047C5C` | `Reset()` | 72 |
| `0x80047EAC` | `UpdateAnim()` | 24 |
| `0x80047F28` | `Think()` | 24 |
| `0x80047FD4` | `Move()` | 56 |
| `0x800482DC` | `Update()` | 216 |
| `0x8004850C` | `LookAtTarget(tagLVector*)` | 56 |
| `0x80048718` | `_DebugCam()` | 96 |
| `0x8004897C` | `_RigidCam()` | 48 |
| `0x80048AC0` | `_FollowPath()` | 464 |
| `0x80049C44` | `SetMode(long)` | 40 |
| `0x80049DC0` | `SetLookAtTarget(Thing*, unsigned short)` | 0 |
| `0x80049DE4` | `ShakeCamera(long)` | 0 |
| `0x80049DEC` | `CameraShake()` | 56 |
| `0x80049F9C` | `GetCameraVector()` | 72 |
| `0x8004A054` | `LoadAsyncAnim(long)` | 32 |
| `0x8004A0F0` | `PlayAsyncAnim()` | 48 |
| `0x8004A220` | `DeleteAsyncAnim()` | 32 |
| `0x8004A39C` | `SetTrackingTime(const _RMVECT16&)` | 0 |
| `0x8004A400` | `SetMovementTime(const _RMVECT16&)` | 0 |
| `0x8004A464` | `SetCurFOV(long)` | 32 |
| `0x8004A500` | `SetFOV(long)` | 0 |

### CameraAnchor : DataAnchor [48 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CAMMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004A780` | `CameraAnchor()` | 24 |
| `0x8004A870` | `AddCameraSourcePath(DBPath*)` | 40 |
| `0x8004A968` | `AddCameraTargetPath(DBPath*)` | 40 |
| `0x8004AA30` | `GetPathWithID(unsigned long)` | 0 |
| `0x8004AA6C` | `FindClosestNodes(G10tagLVectorPP16DBCameraPathNodeT2)` | 72 |

### CameraManager : Manager [32 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CAMMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004A548` | `CameraManager()` | 24 |
| `0x8004A5F4` | `InternalOpen()` | 32 |
| `0x8004A668` | `SetupPaths()` | 32 |

### CharFile [32 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CHARMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8003B694` | `CharFile(Q22AI10ThingTypes)` | 128 |
| `0x8003B83C` | `AddRef()` | 0 |
| `0x8003B850` | `DeleteRef()` | 24 |
| `0x8003B88C` | `Find(Q22AI10ThingTypes)` | 0 |
| `0x8003B8C4` | `FindAnim(unsigned long)` | 0 |
| `0x8003B914` | `EnableCache(bool)` | 24 |

### CharMgrCallback [8 bytes]

*Source: \CHAN\GAME\INC\GEN\CHARMGR.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80034670` | `Callback()` | 0 |
| `0x8003BACC` | `Callback()` | 0 |
| `0x80047100` | `Callback()` | 0 |
| `0x8004A53C` | `Callback()` | 0 |
| `0x80056EF8` | `Callback()` | 0 |
| `0x80078308` | `Callback()` | 0 |
| `0x80095438` | `Callback()` | 0 |

### CharacterManager : Manager [3004 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CHARMGR.CPP, \CHAN\GAME\INC\GEN\CHARMGR.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800396C8` | `CharacterManager()` | 24 |
| `0x800397C4` | `OpenCharacter(Q22AI10ThingTypes)` | 24 |
| `0x80039808` | `CloseCharacter(Q22AI10ThingTypes)` | 24 |
| `0x80039830` | `LoadCharacter(Q22AI10ThingTypesP15CharMgrCallback)` | 56 |
| `0x80039C3C` | `UnloadCharacter(Q22AI10ThingTypes)` | 32 |
| `0x80039DC4` | `ReloadCharacter(Q22AI10ThingTypesQ216CharacterManager8MeshTypeP15CharMgrCallback)` | 48 |
| `0x8003A078` | `LoadCharTexture(Q22AI10ThingTypes)` | 64 |
| `0x8003A1A4` | `IsCharacterLoaded(Q22AI10ThingTypes)` | 0 |
| `0x8003A1D4` | `GetNumberCharactersLoaded()` | 0 |
| `0x8003A20C` | `EnableCache(Q22AI10ThingTypesb)` | 24 |
| `0x8003A240` | `LoadAnimation(Q22AI10ThingTypesUlP15CharMgrCallback)` | 40 |
| `0x8003A328` | `LoadAnimation(Q22AI10ThingTypes9AnimEnumsUlP15CharMgrCallback)` | 48 |
| `0x8003A3EC` | `LoadAnimation(Q22AI10ThingTypes9AnimEnumsP15CharMgrCallback)` | 56 |
| `0x8003A7E8` | `UnloadAnimation(Q22AI10ThingTypesUl)` | 32 |
| `0x8003A8C0` | `UnloadAnimation(Q22AI10ThingTypes9AnimEnumsUl)` | 40 |
| `0x8003A930` | `UnloadAnimation(Q22AI10ThingTypes9AnimEnums)` | 56 |
| `0x8003AC44` | `GetAnimation(Q22AI10ThingTypes9AnimEnums)` | 0 |
| `0x8003ACBC` | `LookUpAnimation(Q22AI10ThingTypesPCc)` | 32 |
| `0x8003AD44` | `PurgeLevel()` | 40 |
| `0x8003AE5C` | `CharDataLoadCallback(long, long, long)` | 264 |
| `0x8003B2EC` | `AnimLoadCallback(long, long, long)` | 184 |
| `0x8003B660` | `AnimLoadCacheCallback(_RTASK*)` | 24 |
| `0x8003BA80` | `InternalReset()` | 0 |
| `0x8003BA88` | `InternalOpen()` | 0 |
| `0x8003BA90` | `InternalClose()` | 0 |

### CheckpointInfo : pos [56 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\SCOREMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004D72C` | `IsValid()` | 24 |
| `0x8004D798` | `SetValidState(int)` | 0 |

### CollisionSector [44 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\COLSECT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80040C94` | `Zero()` | 0 |
| `0x80040CD0` | `Reset()` | 32 |
| `0x80040D18` | `AsynchLoad(int, unsigned long*)` | 24 |
| `0x80040D8C` | `GetBlockNumber(const tagLVector&)` | 0 |
| `0x80040E44` | `CheckWorldWallCollision(const tagLVector&, const tagLVector&, long, long, long, int, long&, _RMVECT16&, tagLVector&, int&, long&, N27)` | 96 |
| `0x80041038` | `CheckWorldWallCollision(const tagLVector&, const tagLVector&, long, long, long, long&, _RMVECT16&, tagLVector&, long&)` | 96 |
| `0x800411F8` | `FillWorldWallArray(const tagLVector&, const tagLVector&, const Wall**, int)` | 8 |
| `0x80041384` | `CheckArrayWallCollision(const Wall**, int, const tagLVector&, const tagLVector&, long, long, long, int)` | 120 |
| `0x800415C0` | `CheckArrayWallIntersection(const Wall**, int, tagLVector&, const tagLVector&, long, long, long, int)` | 168 |
| `0x800417B8` | `GetWorldFloorHeight(const tagLVector&, long)` | 64 |
| `0x800417F0` | `GetWorldFloorAndCeilingHeight(long&, long&, _RMVECT16&, long&, const tagLVector&, long)` | 168 |
| `0x80041980` | `FillWorldFloorArray(const tagLVector&, const tagLVector&, const Floor**, int)` | 8 |
| `0x80041ADC` | `GetArrayFloorAndCeilingHeight(const Floor**, int, long&, long&, _RMVECT16&, long&, int&, tagLVector&, const tagLVector&, long)` | 56 |
| `0x80041DC4` | `LedgePrototype(const tagLVector&, const tagLVector&, long, long, _RMVECT16&, tagLVector&, long&, long)` | 256 |
| `0x80042278` | `CollisionSector()` | 24 |
| `0x800422A0` | `Unload()` | 24 |
| `0x800422C0` | `Load(unsigned long*)` | 0 |
| `0x80042340` | `DebugDrawSector(const tagLVector&)` | 32 |
| `0x80042670` | `DebugDrawSector() const` | 40 |

### ColourInfo [28 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\UVDATA.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80098F00` | `Init(unsigned long, unsigned long, int)` | 0 |
| `0x80099044` | `Reset(int)` | 0 |
| `0x80099080` | `Update()` | 0 |
| `0x800990F4` | `GetColour()` | 0 |

### ComEffect [60 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CMNEFFCT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004DA90` | `SetPrimMargin(unsigned long)` | 0 |
| `0x8004DA9C` | `PrimMarginSafe()` | 24 |
| `0x8004DAF0` | `BlownPrimMargin()` | 24 |
| `0x8004DB38` | `SetUpFirstGeo()` | 24 |
| `0x8004DB60` | `ComEffect()` | 0 |
| `0x8004DE40` | `GetClut(int)` | 24 |
| `0x8004DEE8` | `SetUpUVlists()` | 40 |
| `0x8004E028` | `AddUV(unsigned short*, short, short)` | 8 |
| `0x8004E140` | `SetUpVertexlists()` | 64 |
| `0x8004E3C8` | `SetVertexInfo(int, long)` | 32 |
| `0x8004E4D4` | `SetZFar()` | 32 |
| `0x8004E528` | `LoadGeo(int)` | 32 |
| `0x8004E580` | `LoadETree(int, int)` | 32 |
| `0x8004E6D4` | `LoadSTree(int, int)` | 32 |
| `0x8004E7F8` | `SetFrame(int)` | 24 |
| `0x8004E89C` | `EndOfFrame(int)` | 0 |
| `0x8004E8D4` | `PointInView(tagLVector&, long)` | 48 |
| `0x8004E950` | `Render(const tagLVector&, const _RMVECT16&, const _RMVECT16&, unsigned long)` | 64 |
| `0x8004EE48` | `Render(MATRIX*, unsigned long)` | 32 |
| `0x8004F040` | `GetGeo()` | 0 |
| `0x8004F04C` | `GetGeo(int)` | 40 |
| `0x8004F0AC` | `FindFirstGeo()` | 32 |
| `0x8004F0CC` | `FindFirstGeo(int*)` | 32 |
| `0x8004F204` | `FindNextGeo()` | 0 |
| `0x8004F288` | `InitFastRender(tGeometry*)` | 0 |
| `0x8004F318` | `DoFastRender()` | 24 |
| `0x8004F350` | `FastRender()` | 24 |
| `0x8004F538` | `FastPushMultMatrix(MATRIX*, MATRIX*)` | 24 |
| `0x8004F5A0` | `FastPopMatrix(MATRIX*)` | 0 |
| `0x8004F5E8` | `FastZSortDisplayGCT3(unsigned long)` | 104 |
| `0x8004F89C` | `FastZSortDisplayGCT4(unsigned long)` | 112 |

### Control : ccNode [728 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CONTROL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002DB6C` | `Control()` | 40 |
| `0x8002DC6C` | `Reset()` | 40 |
| `0x8002DCF0` | `Input(unsigned long)` | 56 |
| `0x8002DDEC` | `GetMask()` | 40 |
| `0x8002DE5C` | `SetControlMapArray(char*)` | 0 |
| `0x8002DE88` | `ApplyCurrentModeMap()` | 32 |
| `0x8002DEEC` | `GetButton(char) const` | 0 |
| `0x8002DF14` | `GetMappedButton(char) const` | 0 |

### ControlState [8 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CONTROL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002E7A8` | `ControlState()` | 0 |
| `0x8002E7B0` | `Save()` | 24 |
| `0x8002E7E0` | `Restore()` | 24 |

### DBAttrib [8 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DATABASE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80038188` | `DBAttrib()` | 0 |
| `0x80038200` | `GetAttribString() const` | 24 |
| `0x80038254` | `GetAttribValue() const` | 0 |
| `0x80038260` | `SetAttribString(unsigned long, const char*)` | 32 |
| `0x800382C8` | `SetAttribValue(unsigned long, unsigned long)` | 0 |

### DBCameraPath : ccMinNode [52 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CAMMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004AB6C` | `DBCameraPath()` | 24 |
| `0x8004AC40` | `AddSourceNode(DBPoint*)` | 48 |
| `0x8004ADD0` | `AddTargetNode(DBPoint*, int)` | 0 |
| `0x8004AED0` | `FinalizeBoundaries(long)` | 0 |
| `0x8004AF1C` | `InRange(G10tagLVector)` | 0 |
| `0x8004AFB0` | `FindClosestNodes(G10tagLVectorPP16DBCameraPathNodeT2)` | 120 |

### DBCameraPathNode : ccMinNode [64 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CAMMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004B480` | `DBCameraPathNode()` | 24 |

### DBLine : DBRoot [76 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DATABASE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80038774` | `AddVertex(long, long, long)` | 40 |

### DBMesh : DBRoot [64 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DATABASE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80038804` | `SetFileName(const char*)` | 32 |

### DBRoot : ccNode [60 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DATABASE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800382DC` | `GetAttribByIndex(unsigned int) const` | 0 |
| `0x80038304` | `FindAttrib(unsigned long) const` | 0 |
| `0x80038354` | `FindAttribValue(unsigned long, unsigned long*) const` | 24 |
| `0x800383AC` | `AllocatePermanentAttributeArray(unsigned int)` | 40 |
| `0x80038434` | `DeallocatePermanentAttributeArray()` | 32 |
| `0x800384B4` | `AddAttribNumber(unsigned int, unsigned long, unsigned long)` | 24 |
| `0x800384F8` | `AddAttribString(unsigned int, unsigned long, const char*)` | 24 |
| `0x8003853C` | `AddPermanentAttribString(unsigned int, unsigned long, const char*)` | 24 |
| `0x8003856C` | `AddPermanentAttribNumber(unsigned int, unsigned long, unsigned long)` | 24 |
| `0x8003859C` | `Process(unsigned long*)` | 48 |

### DBVolume : DBRoot [84 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DATABASE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80038854` | `IsInside(const tagLVector&) const` | 0 |

### DataAnchor : ccNode [36 bytes]

*Source: \CHAN\GAME\INC\GEN\ANCHOR.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004B530` | `RemElement(ccNode*)` | 24 |
| `0x8004B550` | `AddElement(ccNode*)` | 24 |
| `0x80059FBC` | `RemElement(ccNode*)` | 24 |
| `0x80059FDC` | `AddElement(ccNode*)` | 24 |

### Database : Manager [120 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DATABASE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800388E4` | `AnalyzeMesh(DBRoot*)` | 24 |
| `0x8003895C` | `Database()` | 24 |
| `0x80038AD8` | `InternalOpen()` | 0 |
| `0x80038AE0` | `InternalClose()` | 24 |
| `0x80038B00` | `PreScan()` | 24 |
| `0x80038B48` | `Scan(char*, unsigned long)` | 64 |
| `0x800390C8` | `Close()` | 24 |
| `0x80039154` | `GetFirstSphere()` | 0 |
| `0x80039160` | `GetFirstLine()` | 0 |
| `0x8003916C` | `GetFirstPath()` | 0 |
| `0x80039178` | `GetFirstPoint()` | 0 |
| `0x80039184` | `GetFirstVolume()` | 0 |
| `0x80039190` | `GetFirstMesh()` | 0 |
| `0x8003919C` | `GetFirstBlock()` | 0 |
| `0x800391A8` | `FindSphere(const char*, DBSphere*)` | 24 |
| `0x800391C8` | `FindSphere(const char*)` | 24 |
| `0x800391EC` | `FindLine(const char*)` | 24 |
| `0x80039210` | `FindPath(const char*)` | 24 |
| `0x80039234` | `FindPoint(const char*)` | 24 |
| `0x80039258` | `FindPath(unsigned long)` | 24 |
| `0x80039288` | `GetPointsList()` | 0 |

### DeadPool [88 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DEADPOOL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008D40C` | `InternalReset()` | 0 |
| `0x8008D414` | `AddUID(unsigned long)` | 0 |
| `0x8008D43C` | `IsUIDInDeadPool(unsigned long)` | 0 |

### Director : Manager [212 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DIRECTOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8003BB90` | `updateVramAnims()` | 24 |
| `0x8003BBE0` | `cleanUpTexAnim()` | 32 |
| `0x8003C044` | `LevelReset()` | 0 |
| `0x8003C04C` | `InternalReset()` | 40 |
| `0x8003C11C` | `InternalClose()` | 40 |
| `0x8003C234` | `SetScript()` | 0 |
| `0x8003C268` | `SetCodeSnip(long*, Thing*)` | 0 |
| `0x8003C298` | `Process()` | 96 |
| `0x8003D5A4` | `ProcessSoundScript()` | 32 |
| `0x8003D634` | `Timer()` | 0 |
| `0x8003D6CC` | `Loop()` | 0 |
| `0x8003D6D4` | `SetDesiredWideScreen()` | 0 |
| `0x8003D800` | `ProcessEdison()` | 24 |
| `0x8003D87C` | `ProcessModelFunc()` | 0 |
| `0x8003D884` | `ProcessCameraFunc()` | 64 |
| `0x8003DC44` | `ProcessHudFunc()` | 24 |
| `0x8003DD10` | `ProcessHumanoidFunc()` | 56 |
| `0x8003E0D4` | `ProcessLadderFunc()` | 72 |
| `0x8003E378` | `ProcessDoorFunc()` | 80 |
| `0x8003E71C` | `DetermineVictoryIdle()` | 24 |
| `0x8003E864` | `DetermineLevelIntro()` | 40 |
| `0x8003EA4C` | `DetermineDeath()` | 24 |
| `0x8003EB14` | `WaitAnimationDone()` | 24 |
| `0x8003EB88` | `ProcessDynamicAnimFunc()` | 32 |
| `0x8003ECD4` | `HandleWideScreen()` | 0 |
| `0x8003ED90` | `DrawWideScreenPolys()` | 32 |
| `0x8003F0E4` | `PurgeAnims()` | 24 |
| `0x8003F104` | `DoesLevelHaveExtraMem(long)` | 0 |
| `0x800CA1B8` | `Director()` | 40 |
| `0x800CA2AC` | `InternalOpen()` | 32 |

### Display : Manager [32 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\DISPLAY.CPP, C:\CHAN\GAME\SRC\PSX\PSXDISP.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004D8FC` | `InternalClose()` | 24 |
| `0x8004D928` | `InternalReset()` | 24 |
| `0x80057904` | `platConstructor()` | 32 |
| `0x80057958` | `platDestructor()` | 24 |
| `0x80057978` | `platClose()` | 24 |
| `0x800579B8` | `platReset()` | 0 |
| `0x800579C0` | `BeginFrame()` | 24 |
| `0x80057A00` | `EndFrame()` | 24 |
| `0x800CA39C` | `Display()` | 24 |
| `0x800CA3E4` | `InternalOpen()` | 32 |
| `0x800CAAF8` | `platOpen()` | 72 |

### DrawableETree [32 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80071830` | `DrawableETree(OriginalETree*)` | 32 |

### DrawableGeo : DrawableBasic [32 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800719B0` | `DrawableGeo(OriginalGeo*)` | 32 |
| `0x80072380` | `Draw(unsigned long)` | 24 |

### DrawableSTree : DrawableTree [36 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007152C` | `DrawableSTree(OriginalSTree*)` | 32 |
| `0x8007170C` | `MirrorTree(SModel*)` | 40 |
| `0x80072350` | `ChangeSuit(short)` | 24 |

### DrawableTree : DrawableBasic [32 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007149C` | `DrawableTree(OriginalTree*)` | 32 |
| `0x800723C0` | `Draw(unsigned long)` | 24 |

### EModel : Model [120 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8006FB50` | `EModel()` | 24 |
| `0x8006FBAC` | `SetOriginalETree(OriginalETree*, tAnimation*)` | 40 |
| `0x8006FC34` | `ApplyAnimToModel(long, long, long, long, long)` | 40 |
| `0x8006FCAC` | `ApplyAnimToModel(tAnimation*, long, long, long)` | 40 |
| `0x8006FD10` | `Animate()` | 24 |
| `0x8006FD44` | `Show(unsigned long)` | 56 |

### Effects : ccNode [36 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\EFFECTS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004C6C8` | `Die(int, int)` | 40 |
| `0x8004C778` | `Find(long, unsigned long)` | 0 |
| `0x8004C7D8` | `UnloadAll()` | 32 |
| `0x8004C854` | `UpdateAll()` | 24 |
| `0x8004C8BC` | `DrawEffects(int)` | 32 |
| `0x8004C9D8` | `AddEffect(int)` | 24 |
| `0x8004CA28` | `RemoveEffect()` | 24 |
| `0x8004CA5C` | `Effects()` | 24 |

### EnvironmentManager : Manager [140 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\ENVMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80053E3C` | `EnvironmentManager()` | 24 |
| `0x80053F1C` | `InternalOpen()` | 32 |
| `0x80053FFC` | `Reset()` | 24 |
| `0x8005401C` | `SetupEnvironment()` | 0 |
| `0x80054024` | `SetupModelAmbientLighting(ccList*)` | 24 |

### FPWEffect : PWEffect [128 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PWEFFECT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009BA80` | `FPWEffect()` | 24 |
| `0x8009BACC` | `Create(int)` | 40 |
| `0x8009BB90` | `Create()` | 24 |
| `0x8009BC38` | `Create2(unsigned long, tagLVector*, _RMVECT16*, int, int)` | 48 |
| `0x8009BD60` | `Create2(tagLVector*, _RMVECT16*, int)` | 56 |
| `0x8009BF24` | `SetMentor()` | 48 |
| `0x8009C004` | `Update()` | 48 |
| `0x8009C3D8` | `Display(int)` | 72 |

### FWEffect : WEffect [212 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\WEFFECT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008BE44` | `FWEffect()` | 24 |
| `0x8008BEDC` | `Find(unsigned long)` | 24 |
| `0x8008BF50` | `Create(int)` | 40 |
| `0x8008C024` | `Create()` | 24 |
| `0x8008C078` | `Create2(unsigned long, tagLVector*, _RMVECT16*, _RMVECT16*, int)` | 40 |
| `0x8008C13C` | `Create2(tagLVector*, _RMVECT16*, _RMVECT16*, int)` | 56 |
| `0x8008C2F0` | `SetMentor()` | 48 |
| `0x8008C3E0` | `SetScaleRoll(long, long)` | 32 |
| `0x8008C434` | `Continue()` | 24 |
| `0x8008C47C` | `Update()` | 32 |
| `0x8008C9D0` | `Display(int)` | 104 |

### FightingCollision

*Source: C:\CHAN\GAME\SRC\GEN\COLFIGHT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80072514` | `FindHumanoid(const Humanoid*)` | 0 |
| `0x80072550` | `Print()` | 40 |
| `0x800725FC` | `Init()` | 0 |
| `0x80072668` | `InsertHumanoid(Humanoid*)` | 32 |
| `0x800726E0` | `RemoveHumanoid(const Humanoid*)` | 24 |
| `0x80072768` | `GetHumanoidArray()` | 0 |
| `0x80072774` | `ClearAttack(const Humanoid*)` | 24 |
| `0x800727D8` | `CheckAttack(Humanoid**, int, const Humanoid*, const FightingCollisionAttackType*)` | 72 |
| `0x80072FB4` | `Set(const Humanoid*, const Humanoid*)` | 24 |

### Floor [80 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\COLFLOOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800926BC` | `GetFloorHeight(const tagLVector&) const` | 8 |
| `0x8009272C` | `GetFloorNormal(_RMVECT16&) const` | 24 |
| `0x800927A0` | `CheckFloorBounds(const tagLVector&, long) const` | 8 |
| `0x8009296C` | `GetRailingCorrection(tagLVector&, const tagLVector&) const` | 8 |
| `0x80092B24` | `LedgePrototype(const tagLVector&, const tagLVector&, long, long, _RMVECT16&, tagLVector&) const` | 128 |
| `0x80093244` | `BoundNumber() const` | 24 |
| `0x80093278` | `Get(tagLVector&, N31) const` | 72 |

### GEffect : Effects [128 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\GEFFECT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008DB2C` | `Load(tReadChunk&, void**)` | 48 |
| `0x8008DCE8` | `Unload()` | 24 |
| `0x8008DDD0` | `FindEffect(unsigned long)` | 0 |
| `0x8008DE18` | `Create(unsigned long, tagLVector*, _RMVECT16*, _RMVECT16*, int, int, unsigned long)` | 64 |
| `0x8008E184` | `GEffect()` | 24 |
| `0x8008E228` | `PutBackEffect()` | 24 |
| `0x8008E264` | `Create()` | 0 |
| `0x8008E26C` | `Update()` | 40 |
| `0x8008E3DC` | `Display(int)` | 120 |
| `0x8008E608` | `CreateSound()` | 24 |
| `0x8008E6A0` | `UpdateSound()` | 24 |
| `0x8008E714` | `ReleaseSound()` | 24 |

### GModel : Model [120 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8006E8C4` | `GModel()` | 24 |
| `0x8006E920` | `SetOriginalGeo(OriginalGeo*)` | 32 |
| `0x8006E96C` | `ApplyAnimToModel(long, long, long, long, long)` | 0 |
| `0x8006E974` | `Animate()` | 0 |
| `0x8006E97C` | `Show(unsigned long)` | 56 |

### Game : Manager [140 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\GAME.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80029328` | `gsNullState(Game*)` | 0 |
| `0x80029460` | `gsInitState(Game*)` | 40 |
| `0x80029574` | `gsQueueLevelLoad(Game*)` | 32 |
| `0x8002977C` | `gsQueuePetalLoad(Game*)` | 24 |
| `0x8002986C` | `gsQueueLevelPetalLoad(Game*)` | 24 |
| `0x80029924` | `gsDetermineNextGameState(Game*)` | 32 |
| `0x800299B0` | `gsDetermineGameOverState(Game*)` | 0 |
| `0x800299B8` | `gsOpenFEState(Game*)` | 24 |
| `0x80029A48` | `gsFEState(Game*)` | 24 |
| `0x80029A70` | `gsOpenLocationState(Game*)` | 24 |
| `0x80029AC0` | `gsPrePlayState(Game*)` | 32 |
| `0x80029C6C` | `gsPlayState(Game*)` | 32 |
| `0x80029EF8` | `gsMenuState(Game*)` | 24 |
| `0x80029F64` | `gsErrorState(Game*)` | 24 |
| `0x8002A004` | `gsErrorExitState(Game*)` | 24 |
| `0x8002A064` | `gsErrorLoopState(Game*)` | 24 |
| `0x8002A128` | `gsLocationMenuState(Game*)` | 24 |
| `0x8002A174` | `gsDbgMenuState(Game*)` | 0 |
| `0x8002A17C` | `gsEndState(Game*)` | 0 |
| `0x8002B4F0` | `ProcessHandlers()` | 32 |
| `0x8002B588` | `InternalClose()` | 32 |
| `0x8002B61C` | `InternalReset()` | 24 |
| `0x8002B65C` | `Step()` | 24 |
| `0x8002B688` | `gsEndLevelState(Game*)` | 24 |
| `0x8002B6B0` | `gsEndLevelLoopState(Game*)` | 24 |
| `0x8002B744` | `gsEndLevelExitState(Game*)` | 32 |
| `0x8002BAB8` | `GetNextToken(char*, char**, char*)` | 40 |
| `0x8002BBF0` | `PlayMovie(const char*, int, int)` | 72 |
| `0x8002BE0C` | `gsTitleLoopState(Game*)` | 40 |
| `0x8002C22C` | `gsEndGameLoopState(Game*)` | 24 |
| `0x8002C3B4` | `gsEndGameState(Game*)` | 24 |
| `0x8002C474` | `gsTitleState(Game*)` | 32 |
| `0x8002C5AC` | `SetState(Q24Game9GameState)` | 24 |
| `0x8002C648` | `LoadXconFE()` | 48 |
| `0x8002C7A4` | `FreeXconFE()` | 24 |
| `0x8002C838` | `InitXconFSImage()` | 40 |
| `0x8002C998` | `FreeXconFSImage()` | 0 |
| `0x8002C9A0` | `FadeBegin()` | 0 |
| `0x8002C9B4` | `FadeEnd()` | 0 |
| `0x8002C9BC` | `FadeUpdate()` | 0 |
| `0x8002C9F8` | `FadeRender()` | 32 |
| `0x8002CB28` | `gsPlayMovieCredits(Game*)` | 24 |
| `0x800C99A0` | `gsIntroState(Game*)` | 40 |
| `0x800C9AEC` | `Game()` | 48 |
| `0x800C9CF8` | `InternalOpen()` | 32 |
| `0x800C9F5C` | `LoadConfigFile()` | 240 |

### Handler : ccNode [36 bytes]

*Source: \CHAN\GAME\INC\GEN\HNDLRSET.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002CDE8` | `RemoveFromList()` | 24 |
| `0x8002E8A0` | `RemoveFromList()` | 24 |
| `0x8003F410` | `RemoveFromList()` | 24 |
| `0x8004039C` | `RemoveFromList()` | 24 |
| `0x80044A9C` | `RemoveFromList()` | 24 |
| `0x8004D830` | `RemoveFromList()` | 24 |
| `0x8004DA44` | `RemoveFromList()` | 24 |
| `0x800570DC` | `RemoveFromList()` | 24 |

### HandlerSet : ccNode [36 bytes]

*Source: \CHAN\GAME\INC\GEN\HNDLRSET.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002CBB8` | `PurgeHandlers()` | 32 |
| `0x8003F1E0` | `PurgeHandlers()` | 32 |

### HardwareLight : direction [24 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LIGHTS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A4328` | `HardwareLight()` | 0 |
| `0x800A4384` | `SetLight(unsigned long, _RMVECT16*)` | 0 |

### HumanoidModel : SModel [136 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MHUMAN.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8006E038` | `HumanoidModel()` | 24 |
| `0x8006E114` | `SetupModelCallbacks()` | 32 |
| `0x8006E1B0` | `SetAnim(long, long, int, long)` | 48 |
| `0x8006E3E8` | `_Loop(AnimStructure*)` | 24 |
| `0x8006E418` | `Animate()` | 24 |
| `0x8006E46C` | `SetTransitionAnim(long, long)` | 56 |

### InputManager : Manager [1492 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CONTROL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002DF74` | `InputManager()` | 40 |
| `0x8002E0D4` | `ServiceInput(unsigned long, unsigned short)` | 24 |
| `0x8002E2F0` | `SetControlMapArray(short, char*)` | 24 |
| `0x8002E33C` | `SetControlModeArray(short, short*)` | 40 |
| `0x8002E3F0` | `SetPlayerConfig(char)` | 24 |
| `0x8002E414` | `UpdateReverseMap()` | 0 |
| `0x8002E460` | `DefaultMapArray()` | 0 |
| `0x8002E46C` | `SetButtonCallback(short, short, ButtonHook*(*)()*, void, void*)` | 0 |
| `0x8002E4D0` | `ClearButtonCallback(short, short)` | 0 |
| `0x8002E528` | `InternalOpen()` | 0 |
| `0x8002E530` | `InternalClose()` | 24 |
| `0x8002E550` | `InternalReset()` | 32 |
| `0x8002E5BC` | `GetControlVal(unsigned short)` | 32 |
| `0x8002E6E8` | `Step()` | 24 |
| `0x8002E73C` | `PlayerMapArray()` | 0 |
| `0x8002E754` | `FindButtonMapping(char)` | 0 |

### ItemNode : ccMinNode [20 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\ITEMNODE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AD6C4` | `ItemNode(char*, long)` | 32 |
| `0x800AD718` | `GetStoreID()` | 0 |
| `0x800AD724` | `DeletePermMem()` | 24 |
| `0x800AD760` | `GetNext()` | 0 |

### LensFlare : FWEffect [300 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LENSFLRE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BEE3C` | `LensFlare()` | 24 |
| `0x800BEF44` | `Create()` | 40 |
| `0x800BF000` | `InitLensFlare(int, DBPath*)` | 56 |
| `0x800BF244` | `BigScreenGlow()` | 24 |
| `0x800BF45C` | `ComputeTracking(tagLVector&, tagLVector&)` | 48 |
| `0x800BF578` | `Update()` | 168 |
| `0x800BFB34` | `Display(int)` | 112 |

### LevelManager : Manager [136 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LEVELMGR.CPP, \CHAN\GAME\INC\GEN\LEVELMGR.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80058AE4` | `LevelManager()` | 24 |
| `0x80058CB8` | `LoadLevel()` | 0 |
| `0x80058CC0` | `PurgeLevelP3DInventory()` | 0 |
| `0x80058CC8` | `PurgeLevel()` | 32 |
| `0x80058DB4` | `PurgePetal()` | 24 |
| `0x80058E68` | `LoadPetal()` | 0 |
| `0x80058E70` | `DeleteOriginalModelsByID(long)` | 48 |
| `0x80058F30` | `DeleteInventoryByID(long)` | 32 |
| `0x80058F84` | `AddOriginal(OriginalBasic*, long)` | 24 |
| `0x80058FF0` | `DeleteOriginal(OriginalBasic*)` | 32 |
| `0x800590D8` | `AddPermMemory(char*, long)` | 32 |
| `0x8005913C` | `DeleteAllPermMem()` | 32 |
| `0x800591C8` | `DeletePermMemID(long)` | 40 |
| `0x80059268` | `FindModel(Q212LevelManager13ModelListEnuml)` | 24 |
| `0x800592A0` | `FindModel(long)` | 40 |
| `0x80059314` | `FindGeo(long)` | 24 |
| `0x80059338` | `FindSTree(long)` | 24 |
| `0x8005935C` | `FindETree(long)` | 24 |
| `0x80059380` | `InternalReset()` | 0 |
| `0x80059388` | `InternalOpen()` | 0 |
| `0x80059390` | `InternalClose()` | 0 |

### LightAnchor : colourVolumes [44 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LIGHTS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A39E8` | `LightAnchor()` | 0 |
| `0x800A3B10` | `SetupLightMemory()` | 32 |
| `0x800A3B98` | `AddColourVolume(DBVolume*)` | 96 |
| `0x800A3E08` | `AddHardwareLightVolume(DBVolume*)` | 72 |
| `0x800A400C` | `SetupLight(DBLight*, DBSphere*)` | 64 |
| `0x800A4194` | `AddRadialHardwareLight(DBSphere*, int)` | 48 |

### LightingClass : hLightHandles [112 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LIGHTS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A24F8` | `InternalOpen()` | 0 |
| `0x800A2538` | `AnalyzeShpere(DBPoint*)` | 56 |
| `0x800A2720` | `LightingClass()` | 40 |
| `0x800A2864` | `Reset()` | 32 |
| `0x800A2928` | `AllocHLight(unsigned long, unsigned long, long, long, long)` | 48 |
| `0x800A2A0C` | `DeallocHLight(long)` | 32 |
| `0x800A2A88` | `AddLightToPort(long, _RMVECT16*, unsigned long)` | 32 |
| `0x800A2AEC` | `RemoveLightFromPort(long)` | 32 |
| `0x800A2B44` | `SetupHLight(long, _RMVECT16*, unsigned long)` | 32 |
| `0x800A2B98` | `SetHLightToOriginal(long)` | 32 |
| `0x800A2BFC` | `SetupLighting()` | 48 |
| `0x800A2D8C` | `ClampWithinRGBLimit(unsigned long*, N21)` | 0 |
| `0x800A2DDC` | `ClampWithinNormalLimit(long*, N21)` | 0 |
| `0x800A2E5C` | `SetupStageAttributes(DBVolume*)` | 96 |
| `0x800A3110` | `DoModelLighting(Thing*)` | 32 |
| `0x800A314C` | `FindAmbientVolumes(Thing*)` | 72 |
| `0x800A3550` | `FindHardwareVolumes(Thing*)` | 112 |

### Line [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\COLLINE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BFFD4` | `GetXOnLine(long) const` | 24 |
| `0x800C003C` | `GetZOnLine(long) const` | 24 |
| `0x800C00A4` | `Equal(const Line&, const Line&)` | 0 |
| `0x800C0160` | `Intersection(const Line&, const Line&, long, long&, long&)` | 72 |

### LinearPath : Path [80 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PATH.CPP, \CHAN\GAME\INC\GEN\PATH.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A4958` | `Subdivide(long)` | 80 |
| `0x800A4D9C` | `Init(const DBPath*)` | 40 |
| `0x800A4F8C` | `Init(const DBLine*)` | 40 |
| `0x800A513C` | `Move(long)` | 88 |
| `0x800A6078` | `EndOfPath()` | 0 |
| `0x800A6090` | `Reset()` | 0 |

### Manager : ccNode [28 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MANAGER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002EC40` | `InternalClose()` | 0 |
| `0x8002EC48` | `InternalOpen()` | 0 |
| `0x8002EC50` | `InternalReset()` | 0 |
| `0x8002EC58` | `Close()` | 24 |
| `0x8002ECA4` | `Open()` | 24 |
| `0x8002ECF4` | `Reset()` | 24 |
| `0x8002ED24` | `Manager()` | 24 |

### MemBankMgr [1 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MEMTRACK.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002F13C` | `SetBankNo(int)` | 0 |
| `0x8002F14C` | `ResetBankNo()` | 24 |

### Model : ccNode [88 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8006E654` | `Model()` | 24 |
| `0x8006E758` | `DeleteDrawable()` | 24 |
| `0x8006E83C` | `DeleteAnimStructures()` | 24 |
| `0x8006E888` | `Reset()` | 24 |
| `0x80070034` | `SetAnim(long, long, int, long)` | 0 |
| `0x8007003C` | `_Loop(AnimStructure*)` | 24 |
| `0x8007005C` | `_LoopReverse(AnimStructure*)` | 24 |
| `0x8007007C` | `_HoldFirst(AnimStructure*)` | 24 |
| `0x8007009C` | `_HoldLast(AnimStructure*)` | 24 |
| `0x800700BC` | `_HoldFrame(AnimStructure*)` | 0 |
| `0x800700C4` | `_RunToLast(AnimStructure*)` | 24 |
| `0x800700E4` | `_RunToFrame(AnimStructure*)` | 0 |
| `0x800700EC` | `_LoopDesired(AnimStructure*)` | 0 |
| `0x800700F4` | `_IncFrame(AnimStructure*)` | 0 |
| `0x800700FC` | `_DecFrame(AnimStructure*)` | 24 |
| `0x8007011C` | `_RunToLastBlend(AnimStructure*)` | 24 |
| `0x8007013C` | `AllocateAmbientLight()` | 24 |
| `0x80070170` | `DeleteAmbientLight()` | 24 |
| `0x800701BC` | `AllocateHardwareLights(unsigned long)` | 48 |
| `0x80070254` | `DeleteHardwareLights()` | 32 |
| `0x80072400` | `DrawShadow(unsigned long)` | 24 |

### NodeAttribs [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PATH.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A45D8` | `Init(DBPoint*)` | 40 |
| `0x800A46F4` | `GetAttrib(int)` | 0 |

### OriginalBasic : ccNode [36 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80071CA8` | `OriginalBasic()` | 24 |

### OriginalETree [52 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80071EC4` | `OriginalETree()` | 24 |
| `0x80071F28` | `Draw()` | 24 |

### OriginalGeo : OriginalBasic [40 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80071D44` | `OriginalGeo()` | 24 |
| `0x80071DA8` | `Draw()` | 24 |

### OriginalSTree : OriginalTree [60 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80071F7C` | `OriginalSTree()` | 24 |
| `0x80072024` | `Draw()` | 24 |
| `0x8007205C` | `SetSemiMode(int)` | 0 |
| `0x800722A8` | `ChangeSuit(DrawableSTree*, short)` | 32 |

### OriginalTree : OriginalBasic [52 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP, \CHAN\GAME\INC\GEN\MODEL.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80071DFC` | `OriginalTree()` | 24 |
| `0x80072448` | `Draw()` | 0 |

### PWEffect : Effects [96 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PWEFFECT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009AC58` | `InitPWorldEffects(DBPoint*)` | 64 |
| `0x8009B238` | `AddFragTemps(unsigned long, unsigned long, unsigned long)` | 64 |
| `0x8009B3C4` | `CreateSound()` | 24 |
| `0x8009B424` | `UpdateSound()` | 24 |
| `0x8009B454` | `ReleaseSound()` | 24 |
| `0x8009B4A0` | `PutBackEffect()` | 24 |
| `0x8009B534` | `Unload()` | 24 |
| `0x8009B554` | `Create(int)` | 40 |
| `0x8009B618` | `Create()` | 24 |
| `0x8009B6EC` | `PWEffect()` | 24 |
| `0x8009B7FC` | `Purge()` | 24 |
| `0x8009B860` | `Update()` | 48 |
| `0x8009B994` | `Display(int)` | 56 |
| `0x8009BEE0` | `IsDone(int&)` | 24 |

### PaletteData : mHeader [44 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PALDATA.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009CE34` | `PaletteData()` | 0 |
| `0x8009CEA4` | `Load(tReadChunk&, void**)` | 32 |
| `0x8009CFEC` | `FindPaletteInfo(unsigned long)` | 0 |
| `0x8009D038` | `Clone()` | 32 |
| `0x8009D0F4` | `SetupClut(ComEffect*, int, unsigned long)` | 32 |
| `0x8009D134` | `Unload()` | 32 |
| `0x8009D1A0` | `InitPalette()` | 0 |
| `0x8009D1B8` | `NextFrame()` | 0 |
| `0x8009D208` | `Update()` | 0 |
| `0x8009D2E0` | `TransferVram()` | 32 |

### Path : ccNode [68 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PATH.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A4760` | `Flip()` | 72 |
| `0x800A4894` | `Draw()` | 48 |

### PathInfo [60 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PATHINFO.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BE54C` | `Init(DBPath*, DBPoint*)` | 48 |
| `0x800BE73C` | `PathInfo()` | 0 |
| `0x800BE7F8` | `Reset()` | 24 |
| `0x800BE844` | `Update()` | 40 |
| `0x800BEA44` | `OnNewPathNode(int)` | 40 |
| `0x800BED44` | `GetPosition()` | 0 |
| `0x800BED50` | `GetRotation()` | 0 |

### PlayerModel [136 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MPLAYER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80077A60` | `PlayerModel()` | 24 |
| `0x80077ABC` | `SetAnim(long, long, int, long)` | 88 |
| `0x80077DF8` | `SetupModelCallbacks()` | 24 |
| `0x80077E18` | `_RunToLast(AnimStructure*)` | 40 |
| `0x80077F44` | `_Loop(AnimStructure*)` | 24 |
| `0x80077F64` | `_IncFrame(AnimStructure*)` | 24 |
| `0x80077F84` | `MirrorTree()` | 24 |
| `0x80078088` | `LoadNIS(unsigned long, const char**, int, int)` | 72 |

### Profile

*Source: C:\CHAN\GAME\SRC\GEN\PROFILE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004723C` | `Update()` | 24 |
| `0x800472BC` | `Begin(ProfileCodeEnum)` | 24 |
| `0x800472FC` | `End(ProfileCodeEnum)` | 32 |
| `0x80047360` | `Sum()` | 0 |
| `0x800473A4` | `Print()` | 56 |
| `0x800474E0` | `Clear()` | 0 |

### SModel : Model [96 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP, \CHAN\GAME\INC\GEN\MODEL.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8006ED68` | `SModel()` | 24 |
| `0x8006EDD4` | `SetOriginalSTree(OriginalSTree*, tAnimation*)` | 32 |
| `0x8006EE20` | `InitSemiTransMode()` | 24 |
| `0x8006EE4C` | `IsAnimationLoaded(long)` | 24 |
| `0x8006EEAC` | `ApplyAnimToModel(long, long, long, long, long)` | 48 |
| `0x8006EF98` | `ApplyAnimToModel(tAnimation*, long, long, long)` | 32 |
| `0x8006F068` | `ApplyAnimToModelBasic(tAnimation*)` | 40 |
| `0x8006F438` | `InitBlendPose()` | 32 |
| `0x8006F4A0` | `ApplyBlending(tAnimation*, long, long)` | 56 |
| `0x8006F640` | `Animate()` | 24 |
| `0x8006F68C` | `Show(unsigned long)` | 64 |
| `0x8006FAD4` | `MirrorTree()` | 24 |
| `0x8006FAFC` | `PlayDynamicAnim(int)` | 40 |
| `0x80072440` | `SetupModelCallbacks()` | 0 |

### ScaleData : mHeader [16 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\SCALEDAT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009C6CC` | `Load(tReadChunk&, void**)` | 56 |
| `0x8009C918` | `FindScaleInfo(int)` | 0 |
| `0x8009C9B8` | `ScaleECallback(tEJoint*, int)` | 40 |
| `0x8009CA4C` | `ScaleMCallback(tMJoint*, int)` | 112 |
| `0x8009CB40` | `Unload()` | 32 |
| `0x8009CBB4` | `UnloadLevel()` | 32 |
| `0x8009CC20` | `CommonScaleData(int)` | 0 |
| `0x8009CC2C` | `SetFrame(int)` | 0 |
| `0x8009CC34` | `GetScale(_RMVECT16*, ScaleKeyFrames*)` | 0 |

### ScoreManager : Manager [504 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\SCOREMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004CC5C` | `ScoreManager()` | 24 |
| `0x8004CCD8` | `InternalOpen()` | 32 |
| `0x8004CD64` | `InternalClose()` | 24 |
| `0x8004CD84` | `InternalReset()` | 0 |
| `0x8004CD8C` | `InitGameStats()` | 0 |
| `0x8004CDEC` | `InitLevelStats()` | 0 |
| `0x8004CEA0` | `SetPar()` | 24 |
| `0x8004CEE0` | `OpenAllLevels()` | 0 |
| `0x8004CF24` | `GiveAllDragons()` | 0 |
| `0x8004CF84` | `Step()` | 24 |
| `0x8004CFA4` | `HandleLevelBegin()` | 24 |
| `0x8004CFC4` | `HandleLevelEnd()` | 24 |
| `0x8004D0D4` | `HandleLevelAbort()` | 0 |
| `0x8004D0DC` | `GetLevelEndRating()` | 24 |
| `0x8004D144` | `OpenPetal(unsigned long, unsigned long)` | 0 |
| `0x8004D184` | `HandleCheckpoint()` | 0 |
| `0x8004D1DC` | `HandleCheckpointBegin()` | 0 |
| `0x8004D234` | `Print() const` | 0 |
| `0x8004D260` | `RegisterCollectible(const Collectible*, int)` | 0 |
| `0x8004D2E0` | `RegisterGotCollectible(const Collectible*, int)` | 0 |
| `0x8004D388` | `AddFightPoints(long)` | 0 |
| `0x8004D39C` | `AddComboPoints(long)` | 0 |
| `0x8004D3B0` | `AddStylePoints(long)` | 0 |
| `0x8004D3C4` | `StepFighting()` | 24 |
| `0x8004D408` | `BreakFightingChain()` | 24 |
| `0x8004D45C` | `AddFightingPoints(long)` | 32 |
| `0x8004D4C8` | `HandleGameBegin()` | 24 |
| `0x8004D518` | `CalcGrade()` | 0 |
| `0x8004D5AC` | `CalcGradeXTakes(unsigned char)` | 0 |
| `0x8004D5C0` | `CalcGDrags(int)` | 0 |
| `0x8004D5CC` | `GetTotalGoldDragon()` | 56 |
| `0x8004D69C` | `IsDrunkenMasterSuitEnabled()` | 24 |

### SplinePath : Path [128 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\PATH.CPP, \CHAN\GAME\INC\GEN\PATH.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A54BC` | `Subdivide(long)` | 0 |
| `0x800A54C4` | `CalcCMRCoefficiants(long&, N31llll)` | 0 |
| `0x800A556C` | `Init(const DBPath*)` | 40 |
| `0x800A57BC` | `Init(const DBLine*)` | 40 |
| `0x800A59CC` | `Move(long)` | 144 |
| `0x800A5F4C` | `EndOfPath()` | 0 |
| `0x800A5F64` | `Reset()` | 0 |

### SpotLight : WEffect [168 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\SPOTLIGHT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BE270` | `SpotLight()` | 24 |
| `0x800BE2CC` | `Create()` | 24 |
| `0x800BE344` | `Update()` | 24 |
| `0x800BE450` | `Display(int)` | 48 |
| `0x800BE510` | `SetUp(unsigned long, unsigned long)` | 0 |
| `0x800BE51C` | `PutBackEffect()` | 24 |

### Stream : headers [48 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\STREAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80099188` | `Stream()` | 0 |
| `0x800991A0` | `Open(const char*, int)` | 72 |
| `0x800993E8` | `Close()` | 32 |
| `0x800994D4` | `LoadPermChunk()` | 32 |
| `0x80099570` | `HandleTPGChunk()` | 64 |
| `0x80099A24` | `HandleRCBChunk()` | 504 |
| `0x80099EA4` | `HandlePCBChunk()` | 504 |
| `0x8009A318` | `HandleWDBChunk()` | 32 |
| `0x8009A3EC` | `HandleLLNChunk()` | 56 |
| `0x8009A5A4` | `Read()` | 24 |
| `0x8009A74C` | `AsyncLoad(unsigned long, long, void*, int, int)` | 40 |
| `0x8009A860` | `Load(void*, long)` | 32 |
| `0x8009A900` | `_TPGLoadCallback(long, long, long)` | 48 |
| `0x8009A98C` | `_BLKLoadCallback(long, long, long)` | 40 |
| `0x8009AA74` | `LoadPetal(long)` | 32 |

### StreamHeaderNode [16 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\STREAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80099148` | `StreamHeaderNode(unsigned long, unsigned long)` | 0 |

### Time : Manager [40 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\TIME.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80044950` | `Time()` | 32 |
| `0x80044A10` | `InternalOpen()` | 0 |
| `0x80044A18` | `InternalClose()` | 24 |
| `0x80044A38` | `InternalReset()` | 0 |
| `0x80044A40` | `Step()` | 0 |

### TrailInfo : ccMinNode [80 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\TRAIL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800791B8` | `SetupDecrements(int)` | 16 |
| `0x800793C4` | `SetVelocity(tagLVector*)` | 0 |
| `0x80079400` | `Update()` | 0 |

### Trails : Effects [104 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\TRAIL.CPP, \CHAN\GAME\INC\GEN\TRAIL.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80079614` | `Trails(int)` | 48 |
| `0x80079894` | `Add(tagLVector*, tagLVector*, unsigned long, int, tagLVector*)` | 48 |
| `0x80079AB4` | `PutBackEffect()` | 24 |
| `0x80079AF4` | `Flush()` | 32 |
| `0x80079B54` | `FindDoneTrail(int)` | 0 |
| `0x80079B88` | `Update()` | 40 |
| `0x80079C44` | `SetCurrentPos(tagLVector*)` | 0 |
| `0x80079C4C` | `Display(int)` | 32 |
| `0x80079D1C` | `ChanZSortDisplayNonTexture(int)` | 104 |
| `0x8007A134` | `ChanZSortDisplayTexture(int)` | 112 |
| `0x8007A61C` | `Create()` | 0 |

### UVPrimData [40 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\UVDATA.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80098334` | `Load(tReadChunk&, void**)` | 32 |
| `0x8009842C` | `FindUVPrimInfo(unsigned long)` | 0 |
| `0x80098478` | `Unload()` | 32 |
| `0x800984E4` | `Update(int, tPrimGeom*)` | 0 |
| `0x800985A4` | `UVPrimData()` | 0 |
| `0x800985D8` | `Init(int, int, int, int)` | 0 |
| `0x80098604` | `Update()` | 0 |

### WDBSphereSwitch : WDBSwitch [80 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\SWITCH.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800952B4` | `IsInside(const tagLVector&)` | 0 |

### WDBSwitch : ccNode [76 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\SWITCH.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80094CE4` | `WDBSwitch()` | 32 |
| `0x80094E10` | `Setup(DBRoot*)` | 296 |
| `0x80095064` | `Bind()` | 56 |
| `0x80095204` | `Execute(Thing*)` | 24 |
| `0x8009525C` | `Reject(Thing*)` | 24 |

### WDBVolumeSwitch : WDBSwitch [100 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\SWITCH.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800952BC` | `IsInside(const tagLVector&)` | 0 |
| `0x80095384` | `SetVolume(DBVolume*)` | 0 |

### WEffect : Effects [132 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\WEFFECT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008A708` | `Load(tReadChunk&, void**)` | 32 |
| `0x8008A848` | `Unload()` | 32 |
| `0x8008A8E0` | `Find(unsigned long)` | 24 |
| `0x8008A954` | `SetupPaletteData(unsigned long, unsigned long, unsigned long)` | 32 |
| `0x8008A9C0` | `InitWorldEffects(DBPoint*)` | 80 |
| `0x8008B538` | `WEffect()` | 24 |
| `0x8008B67C` | `CreateSound(tagLVector*)` | 32 |
| `0x8008B6F0` | `UpdateSound()` | 24 |
| `0x8008B728` | `ReleaseSound()` | 24 |
| `0x8008B774` | `Create(int)` | 40 |
| `0x8008B850` | `Create()` | 32 |
| `0x8008B924` | `Purge()` | 24 |
| `0x8008B988` | `PutBackEffect()` | 24 |
| `0x8008B9C4` | `IsDone(int&)` | 24 |
| `0x8008BA08` | `EnablePath(int)` | 0 |
| `0x8008BA24` | `Update()` | 56 |
| `0x8008BD28` | `Display(int)` | 32 |
| `0x8008BE08` | `NISRemoveEffect()` | 24 |

### Wall : mWall [56 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\COLWALL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80091C90` | `CheckWallIntersection(tagLVector&, const tagLVector&, long, long, long, int) const` | 112 |
| `0x80091EAC` | `CheckWallCollision(const tagLVector&, const tagLVector&, long, long, long, int, long&, _RMVECT16&, tagLVector&) const` | 64 |
| `0x80092144` | `IsCurb() const` | 8 |
| `0x80092250` | `CheckWallBounds(const tagLVector&, long, long, long, int) const` | 16 |
| `0x800923D8` | `Get(tagLVector&, N31) const` | 48 |

### World : Manager [160 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\WORLD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004521C` | `PackLevelName(unsigned long, unsigned long)` | 0 |
| `0x8004522C` | `UnpackLevelName(unsigned long, unsigned long&, unsigned long&)` | 0 |
| `0x80045244` | `World()` | 24 |
| `0x800455BC` | `InternalOpen()` | 0 |
| `0x800455C4` | `InternalClose()` | 24 |
| `0x800455FC` | `InternalReset()` | 24 |
| `0x80045634` | `_LevelMenuExecute(hdMenuItem*)` | 32 |
| `0x800456D8` | `GetCurLevelPetals()` | 0 |
| `0x800456F4` | `GetCurLevelID()` | 0 |
| `0x80045710` | `LevelIDToIndex(int)` | 0 |
| `0x80045768` | `LoadLevelNames()` | 3528 |
| `0x80045D6C` | `LoadPermanent()` | 80 |
| `0x80045F34` | `UnloadPetal()` | 24 |
| `0x8004604C` | `LoadPetal(unsigned long)` | 32 |
| `0x80046170` | `EstimateLoadTime(unsigned long, unsigned long, bool)` | 0 |
| `0x80046208` | `UnloadLevelPart2()` | 24 |
| `0x8004624C` | `LoadLevel(unsigned long)` | 64 |
| `0x800463F0` | `PopulateWEffects()` | 32 |
| `0x80046464` | `UnPopulateWEffects(unsigned long)` | 24 |
| `0x800464D8` | `SwitchSetup(WDBSwitch*, DBRoot*)` | 32 |
| `0x8004657C` | `ProcessSwitches()` | 40 |
| `0x80046648` | `CheckSwitches(ccList*, Thing*)` | 40 |
| `0x80046724` | `Construct()` | 56 |
| `0x80046CB0` | `UnloadPermanent()` | 0 |
| `0x80046CB8` | `Destruct()` | 32 |
| `0x80046DE0` | `ResetLevel()` | 24 |
| `0x80046E2C` | `UnloadLevel()` | 24 |
| `0x80046F74` | `ExecuteLoadCallbacks()` | 24 |
| `0x80046FC4` | `ExecuteUnloadCallbacks()` | 24 |

### WorldPoints : pointListNIS [16 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\WORLDPTS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008D714` | `InternalReset()` | 24 |
| `0x8008D768` | `AddPoint(DBPoint*)` | 40 |
| `0x8008D8A4` | `GetNISPoint(unsigned long)` | 24 |
| `0x8008D8C4` | `GetParPointValue()` | 24 |

### ccFile : ccNode [60 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CCFILE.CPP, \CHAN\GAME\INC\GEN\CCFILE.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004BF88` | `ConvertLong(unsigned long)` | 0 |
| `0x8004BFD4` | `ConvertWord(unsigned short)` | 0 |
| `0x8004C000` | `SetDebug(short)` | 0 |
| `0x8004C008` | `ccFile()` | 24 |
| `0x8004C0BC` | `OpenMem(unsigned char*, unsigned long)` | 0 |
| `0x8004C0D4` | `Open(const char*, unsigned short)` | 40 |
| `0x8004C18C` | `Close()` | 24 |
| `0x8004C1F0` | `ReadString(void*, unsigned long)` | 48 |
| `0x8004C2B4` | `Read(void*, unsigned long)` | 32 |
| `0x8004C374` | `Write(void*, unsigned long)` | 24 |
| `0x8004C3BC` | `Seek(unsigned long, unsigned short)` | 24 |
| `0x8004C4A0` | `WriteLong(unsigned long)` | 24 |
| `0x8004C4E0` | `WriteWord(unsigned short)` | 24 |
| `0x8004C524` | `WriteByte(unsigned char)` | 32 |
| `0x8004C558` | `ReadLong(unsigned long*)` | 40 |
| `0x8004C5D0` | `ReadWord(unsigned short*)` | 40 |
| `0x8004C648` | `ReadByte(unsigned char*)` | 40 |
| `0x8004C6A4` | `GetError()` | 0 |
| `0x8004C6B0` | `GetPosition()` | 0 |
| `0x8004C6BC` | `GetLength()` | 0 |

### ccList [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CCLIST.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80037620` | `FindNode(const char*, ccNode*) const` | 32 |
| `0x80037664` | `FindNodeCRC(unsigned long, ccNode*) const` | 0 |
| `0x800376A4` | `Sort(ccNode*(*)(ccNode*)*, int)` | 56 |
| `0x8003780C` | `SortPriReverse()` | 24 |
| `0x80037830` | `AddNodePri(ccNode*)` | 24 |

### ccMinList [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CCLIST.CPP, \CHAN\GAME\INC\GEN\CCLIST.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8002CD44` | `Purge()` | 24 |
| `0x800374EC` | `GetNumElements() const` | 0 |
| `0x80037510` | `AddNode(ccMinNode*, ccMinNode*)` | 0 |
| `0x80037570` | `RemNode(ccMinNode*)` | 0 |
| `0x800375E8` | `RemHead()` | 24 |
| `0x80039554` | `Purge()` | 24 |
| `0x8003F36C` | `Purge()` | 24 |
| `0x8004715C` | `Purge()` | 24 |
| `0x8004B624` | `Purge()` | 24 |
| `0x8004CB94` | `Purge()` | 24 |
| `0x800519F4` | `Purge()` | 24 |
| `0x80057038` | `Purge()` | 24 |
| `0x80057394` | `Purge()` | 24 |
| `0x800593E8` | `Purge()` | 24 |
| `0x8005A0B0` | `Purge()` | 24 |
| `0x8005E650` | `Purge()` | 24 |
| `0x800605B4` | `Purge()` | 24 |
| `0x80062990` | `Purge()` | 24 |
| `0x8007A69C` | `Purge()` | 24 |
| `0x8008D0EC` | `Purge()` | 24 |
| `0x8008DA88` | `Purge()` | 24 |
| `0x8008E860` | `Purge()` | 24 |
| `0x8009821C` | `Purge()` | 24 |
| `0x8009C628` | `Purge()` | 24 |
| `0x800A4450` | `Purge()` | 24 |
| `0x800A61A8` | `Purge()` | 24 |
| `0x800A7ADC` | `Purge()` | 24 |

### ccMinNode [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CCLIST.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80037310` | `ccMinNode()` | 0 |

### ccNode : ccMinNode [24 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\CCLIST.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80037358` | `ccNode()` | 24 |
| `0x800373F0` | `SetName(const char*, int)` | 40 |
| `0x80037494` | `SetNameNoAlloc(const char*)` | 24 |

### nisCharMgrCallback : CharMgrCallback [48 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\MPLAYER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80077FA4` | `nisCharMgrCallback(int, Q22AI10ThingTypesP9AnimEnums*)` | 0 |
| `0x80078008` | `Callback()` | 24 |

### tChanLitFarTable [60 bytes]

*Source: \CHAN\GAME\INC\GEN\DRAWTABL.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007D6C4` | `Install()` | 0 |

### tChanLitTable [60 bytes]

*Source: \CHAN\GAME\INC\GEN\DRAWTABL.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007D734` | `Install()` | 0 |

### tChanSequenceAnimLoader [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LOADERS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80087774` | `Load(tReadChunk&, void**)` | 64 |

### tChanZFarTable [60 bytes]

*Source: \CHAN\GAME\INC\GEN\DRAWTABL.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007D7A4` | `Install()` | 0 |

### tChanZSortTable [60 bytes]

*Source: \CHAN\GAME\INC\GEN\DRAWTABL.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007D824` | `Install()` | 0 |

### tGameLoader [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LOADERS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008735C` | `Load(tReadChunk&, void**)` | 32 |

### tTexLoader [12 bytes]

*Source: C:\CHAN\GAME\SRC\GEN\LOADERS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800874F8` | `Load(tReadChunk&, void**)` | 176 |

### GameOverScreen : oxScreenManager [56 bytes]

*Source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80011A28` | `GameOverScreen()` | 24 |
| `0x80011A64` | `SelfUpdate()` | 40 |
| `0x80011AE0` | `SelfInit()` | 32 |
| `0x80011B54` | `GetScreenNames()` | 0 |

### GameStorage [12 bytes]

*Source: C:\CHAN\GAME\SRC\FE\GAMESTOR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800148BC` | `InitForSave()` | 24 |
| `0x80014934` | `InitForRestore()` | 24 |
| `0x800149A8` | `FreeBuffer()` | 24 |

### LineFile [216 bytes]

*Source: C:\CHAN\GAME\SRC\FE\LINEFILE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80017F50` | `LineFile()` | 0 |
| `0x80017FD4` | `Open(char*)` | 96 |
| `0x80018078` | `Next()` | 40 |
| `0x800181E0` | `Word(int)` | 0 |

### MenuMgr : oxScreenManager [80 bytes]

*Source: C:\CHAN\GAME\SRC\FE\MENUMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005F4A4` | `MenuMgr()` | 24 |
| `0x8005F564` | `FindMenu(unsigned long)` | 0 |
| `0x8005F5A0` | `FindScreen(unsigned long)` | 0 |
| `0x8005F5AC` | `InputItemPop()` | 24 |
| `0x8005F660` | `InputPadUp()` | 24 |
| `0x8005F6B0` | `InputPadRight()` | 24 |
| `0x8005F6F8` | `InputPadLeft()` | 24 |
| `0x8005F740` | `InputPadDown()` | 24 |
| `0x8005F790` | `InputItemPush()` | 24 |
| `0x8005F7DC` | `SetTopMenu(unsigned long)` | 32 |
| `0x8005F830` | `PushMenu(hdMenu*)` | 32 |
| `0x8005F894` | `PostFlightDef()` | 32 |
| `0x8005F8E0` | `ParseDefFile(char*)` | 248 |
| `0x8005FB00` | `Invoke()` | 24 |
| `0x8005FBA4` | `Activate()` | 40 |
| `0x8005FD30` | `Deactivate()` | 40 |
| `0x8005FDF4` | `QueryInput(bool)` | 32 |
| `0x8005FF38` | `GetScreenHash(unsigned long)` | 0 |
| `0x8005FF40` | `GetScreenNames()` | 0 |
| `0x8005FF4C` | `PopMenu()` | 24 |
| `0x8005FFD0` | `ParseMenu(LineFile&, hdMenu*)` | 64 |

### TitleScreen : oxScreenManager [56 bytes]

*Source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800118F0` | `TitleScreen()` | 24 |
| `0x8001192C` | `SelfUpdate()` | 40 |
| `0x800119A8` | `SelfInit()` | 32 |
| `0x80011A1C` | `GetScreenNames()` | 0 |

### VBlankLogo [32 bytes]

*Source: C:\CHAN\GAME\SRC\FE\LOADANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80047560` | `VBlankLogo(long)` | 32 |
| `0x80047710` | `SetActive(int)` | 24 |
| `0x80047770` | `ClearVram()` | 0 |
| `0x80047778` | `Update(_RTASK*)` | 72 |
| `0x80047968` | `StartLogo(long)` | 24 |
| `0x800479BC` | `StopLogo()` | 24 |
| `0x80047A68` | `FillMeter(unsigned char)` | 24 |

### feMenuMgr : MenuMgr [100 bytes]

*Source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80010988` | `_ResumeGame(hdMenuItem*)` | 0 |
| `0x80010990` | `_NewGame(hdMenuItem*)` | 24 |
| `0x80010A04` | `_LoadGame(hdMenuItem*)` | 24 |
| `0x80010A2C` | `_SaveGame(hdMenuItem*)` | 24 |
| `0x80010A54` | `_ShowCredits(hdMenuItem*)` | 24 |
| `0x80010A7C` | `InputItemPush()` | 32 |
| `0x80010B40` | `InputPadUp()` | 24 |
| `0x80010BC0` | `InputPadDown()` | 24 |
| `0x80010C40` | `InputPadLeft()` | 24 |
| `0x80010CA4` | `InputPadRight()` | 24 |
| `0x80010D08` | `PushMenu(hdMenu*)` | 32 |
| `0x80010E30` | `PopMenu()` | 24 |
| `0x80010EB4` | `feMenuMgr()` | 24 |
| `0x80010F2C` | `HandleInputChange()` | 32 |
| `0x8001103C` | `SelfInit()` | 32 |
| `0x80011188` | `LevelValid(int, long)` | 24 |
| `0x80011218` | `ShowLevel(FrontEndVolume*, Humanoid*)` | 24 |
| `0x80011260` | `InitLevelMenu()` | 64 |
| `0x80011540` | `Deactivate()` | 24 |
| `0x800115BC` | `GotoStartScreen()` | 24 |
| `0x800115F0` | `ShowNewGameMenu()` | 24 |
| `0x80011618` | `PushLoadSaveMenu(int)` | 32 |
| `0x80011680` | `OpenDoors()` | 296 |
| `0x80011774` | `QueryInput(bool)` | 32 |

### gameMenu : MenuMgr [92 bytes]

*Source: C:\CHAN\GAME\SRC\FE\GAMEMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8003791C` | `_ResumeGame(hdMenuItem*)` | 0 |
| `0x80037924` | `_ExitGame(hdMenuItem*)` | 24 |
| `0x80037970` | `PushMenu(hdMenu*)` | 32 |
| `0x80037A3C` | `PopMenu()` | 24 |
| `0x80037A88` | `Activate()` | 32 |
| `0x80037B6C` | `InputItemPush()` | 24 |
| `0x80037BEC` | `InputPadUp()` | 24 |
| `0x80037C40` | `InputPadDown()` | 24 |
| `0x80037C94` | `gameMenu()` | 24 |
| `0x80037D0C` | `SelfInit()` | 24 |
| `0x80037DA0` | `GotoStartScreen()` | 24 |
| `0x80037E14` | `Deactivate()` | 24 |
| `0x80037E64` | `ShowPauseMenu()` | 24 |
| `0x80037E88` | `ShowLoadingScreenText(unsigned long, unsigned long)` | 32 |
| `0x80037FC0` | `HandleInputChange()` | 32 |

### hdAlphaSelection [44 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005D728` | `hdAlphaSelection(xcOverlay*, char*, int, int)` | 32 |
| `0x8005D764` | `ChangeValueText()` | 0 |

### hdAnimTextOvl : hdTextOvl [128 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008F360` | `hdAnimTextOvl()` | 24 |
| `0x8008F3B0` | `SelfInit()` | 48 |
| `0x8008F454` | `SetPos(int, int)` | 48 |
| `0x8008F508` | `GoToMinPos()` | 24 |
| `0x8008F550` | `SetAnimInfo(int, int, int, int)` | 40 |
| `0x8008F65C` | `Reset()` | 0 |
| `0x8008F664` | `Update()` | 24 |
| `0x8008F790` | `Play()` | 0 |
| `0x8008F79C` | `SetPauseState(int, int)` | 0 |

### hdControllerSelection : hdItemSelection [40 bytes]

*Source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80013134` | `hdControllerSelection(xcOverlay*, char*, xcOverlay*)` | 32 |
| `0x80013178` | `IncItem()` | 24 |
| `0x800131A4` | `DecItem()` | 24 |
| `0x800131D0` | `SetControlDescription()` | 40 |

### hdDestSelect : fDestDragon [24 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008EFA4` | `hdDestSelect()` | 24 |
| `0x8008EFD0` | `Start(long)` | 32 |
| `0x8008F05C` | `Init(oxScreenManager*)` | 32 |
| `0x8008F0F4` | `Hide()` | 24 |
| `0x8008F138` | `ShowLevel(int)` | 32 |
| `0x8008F26C` | `Update()` | 24 |

### hdDragon : hdAnimTextOvl [140 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008F81C` | `SetNum(int)` | 24 |
| `0x8008F84C` | `hdDragon()` | 24 |
| `0x8008F884` | `SelfInit()` | 24 |
| `0x8008F8C4` | `SetGoldDragons(short)` | 24 |

### hdDynItemButton : hdItemButton [32 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005E1AC` | `DynSetup()` | 0 |

### hdDynItemGoto : hdItemGoto [36 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005DDD8` | `DynSetup()` | 0 |

### hdDynItemMenu : hdMenu [44 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005D2F8` | `hdDynItemMenu(int)` | 32 |
| `0x8005D340` | `DynSetup()` | 32 |
| `0x8005D3B4` | `InputNextItem()` | 24 |
| `0x8005D428` | `InputPrevItem()` | 24 |

### hdDynItemSelection : hdItemSelection [48 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005DCD0` | `hdDynItemSelection(xcOverlay*, char*)` | 32 |

### hdDynMenu : hdMenu [60 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005DF40` | `hdDynMenu(MenuMgr*, xcOverlay*, int)` | 40 |
| `0x8005DFC0` | `DynSetup()` | 32 |
| `0x8005E0D8` | `GetTextObj(int)` | 32 |

### hdHealth : hdTtlive [36 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008ECFC` | `hdHealth()` | 24 |
| `0x8008ED58` | `SelfInit()` | 24 |
| `0x8008EDB0` | `SetMax(long)` | 0 |
| `0x8008EDB8` | `SetValue(long)` | 0 |
| `0x8008EE50` | `SetText(char*)` | 0 |
| `0x8008EE98` | `Update()` | 24 |

### hdHits : fHitOvl [64 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008F93C` | `Init(oxScreenManager*)` | 24 |
| `0x8008FA2C` | `Update()` | 48 |
| `0x8008FBC4` | `TriggerUpdate()` | 24 |
| `0x8008FBEC` | `IncrementHits()` | 48 |
| `0x80090DC4` | `hdHits()` | 0 |
| `0x80090DD8` | `SetVisible(int)` | 24 |

### hdItemButton [28 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005E118` | `hdItemButton(xcTextObj*, char*)` | 32 |
| `0x8005E174` | `SelectItem(MenuMgr*)` | 24 |

### hdItemGoto : hdMenuItem [32 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005DD14` | `hdItemGoto(xcTextObj*, char*)` | 32 |
| `0x8005DDA4` | `PostFlight(MenuMgr*)` | 24 |
| `0x8005DDF8` | `SelectItem(MenuMgr*)` | 24 |

### hdItemSelection : hdMenuItem [36 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP, \CHAN\GAME\INC\FE\HDMENU.H*

| Address | Method | Size |
|---------|--------|------|
| `0x8005D778` | `hdItemSelection(xcOverlay*, char*)` | 32 |
| `0x8005D800` | `IncItem()` | 32 |
| `0x8005D8A8` | `DecItem()` | 32 |
| `0x8005D954` | `SetValue(unsigned long)` | 0 |
| `0x8005D97C` | `GetValue()` | 0 |
| `0x8005D990` | `SetColour(xcColour1555&, bool)` | 48 |
| `0x8005E494` | `CanBeSelected()` | 0 |

### hdMemCardMenu : hdMenu [80 bytes]

*Source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80011B60` | `hdMemCardMenu(MenuMgr*, xcOverlay*, xcOverlay*)` | 40 |
| `0x80011CA4` | `TerminateMemCard()` | 24 |
| `0x80011CD0` | `StateStart(int)` | 32 |
| `0x80011D58` | `InitMemCard()` | 40 |
| `0x80011DD4` | `Update()` | 720 |
| `0x80012434` | `SaveYes()` | 24 |
| `0x80012474` | `SaveOkOrNo()` | 24 |
| `0x800124BC` | `PromptYesNo(int, int)` | 32 |
| `0x80012544` | `DynSetup()` | 24 |
| `0x800125A4` | `InputNextItem()` | 24 |
| `0x800125C4` | `InputPrevItem()` | 24 |
| `0x800125E4` | `_Yes(hdMenuItem*)` | 24 |
| `0x80012608` | `_OkorNo(hdMenuItem*)` | 24 |
| `0x8001262C` | `GameSave()` | 64 |
| `0x80012A7C` | `GameLoad()` | 48 |
| `0x80012F70` | `CalcChecksum()` | 0 |
| `0x80012FAC` | `SetChecksum()` | 24 |
| `0x80012FE0` | `TestChecksum()` | 24 |
| `0x8001301C` | `HasMenu()` | 0 |
| `0x80013030` | `PromptOk(int)` | 32 |
| `0x800130AC` | `CanAbortNow()` | 0 |
| `0x800130F4` | `Cleanup()` | 24 |

### hdMenu : oxScreen [36 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP, \CHAN\GAME\INC\FE\HDMENU.H*

| Address | Method | Size |
|---------|--------|------|
| `0x8005CD94` | `hdMenu()` | 24 |
| `0x8005CE44` | `PostFlight(MenuMgr*)` | 32 |
| `0x8005CEB8` | `UpdateScreen(oxScreenManager*)` | 24 |
| `0x8005CEE8` | `SetCallback(unsigned long, hdMenuItem*(*)()*, int)` | 32 |
| `0x8005CF2C` | `FindItem(unsigned long)` | 0 |
| `0x8005CF68` | `InputPush(MenuMgr*)` | 24 |
| `0x8005CFD0` | `ClearItem()` | 24 |
| `0x8005D010` | `DynSetup()` | 40 |
| `0x8005D0BC` | `AddItem(hdMenuItem*)` | 24 |
| `0x8005D0E8` | `Update()` | 32 |
| `0x8005D15C` | `SetItem(hdMenuItem*)` | 32 |
| `0x8005D1B0` | `InputNextItem()` | 32 |
| `0x8005D238` | `InputPrevItem()` | 32 |
| `0x8005D2CC` | `SetID(const char*)` | 24 |
| `0x8005E3FC` | `Cleanup()` | 0 |
| `0x8005E404` | `CanAbortNow()` | 0 |

### hdMenuItem : ccMinNode [28 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP, \CHAN\GAME\INC\FE\HDMENU.H*

| Address | Method | Size |
|---------|--------|------|
| `0x8005DE2C` | `hdMenuItem()` | 24 |
| `0x8005DE98` | `PostFlight(MenuMgr*)` | 0 |
| `0x8005DEA0` | `SetColour(xcColour1555&, bool)` | 40 |
| `0x8005DF10` | `SelectItem(MenuMgr*)` | 0 |
| `0x8005DF18` | `IncItem()` | 0 |
| `0x8005DF20` | `DecItem()` | 0 |
| `0x8005DF28` | `SetValue(unsigned long)` | 0 |
| `0x8005DF30` | `SetCallback(hdMenuItem*(*)()*, int)` | 0 |
| `0x8005DF38` | `GetValue()` | 0 |
| `0x8005E55C` | `DynSetup()` | 0 |
| `0x8005E564` | `CanBeSelected()` | 0 |

### hdNumericSelection : hdMenuItem [44 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005D49C` | `hdNumericSelection(xcOverlay*, char*, int, int)` | 40 |
| `0x8005D55C` | `GetValue()` | 0 |
| `0x8005D568` | `ChangeValueText()` | 24 |
| `0x8005D598` | `IncItem()` | 32 |
| `0x8005D634` | `DecItem()` | 32 |
| `0x8005D6D0` | `SetValue(unsigned long)` | 24 |

### hdShockSelection [36 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005E1CC` | `hdShockSelection(xcOverlay*, char*)` | 24 |
| `0x8005E200` | `SetColour(xcColour1555&, bool)` | 48 |
| `0x8005E330` | `CanBeSelected()` | 24 |

### hdSndItemSelection : hdItemSelection [68 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP, \CHAN\GAME\INC\FE\HDMENU.H*

| Address | Method | Size |
|---------|--------|------|
| `0x8005DA28` | `hdSndItemSelection(xcOverlay*, char*, int, int)` | 32 |
| `0x8005DABC` | `DecItem()` | 32 |
| `0x8005DB54` | `GetValue()` | 0 |
| `0x8005DB60` | `UpdateShown()` | 0 |
| `0x8005DBD4` | `SetValue(unsigned long)` | 24 |
| `0x8005DC14` | `IncItem()` | 32 |
| `0x8005E454` | `SetColour(xcColour1555&, bool)` | 24 |

### hdTally : step [128 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008FDB4` | `hdTally()` | 24 |
| `0x8008FEC0` | `Init(oxScreenManager*)` | 32 |
| `0x800900A0` | `DoScoreTally(char*, bool)` | 24 |
| `0x8009013C` | `UpdateCombo(bool)` | 32 |
| `0x80090220` | `UpdateFight(bool)` | 32 |
| `0x80090308` | `UpdateStyle(bool)` | 32 |
| `0x8009041C` | `UpdateGrade(bool)` | 40 |
| `0x800905DC` | `UpdateRdragon(bool)` | 32 |
| `0x80090718` | `UpdateRdragonBonus(bool)` | 32 |
| `0x800907F0` | `UpdateGdragon(bool)` | 32 |
| `0x80090914` | `UpdateMovieBonus(bool)` | 32 |
| `0x800909D0` | `DoDoneStuff()` | 48 |
| `0x80090AC0` | `Update()` | 40 |
| `0x80090C58` | `Start(int)` | 32 |
| `0x80090D2C` | `Show(int)` | 32 |

### hdTextOvl : hdTtlive [32 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008F28C` | `hdTextOvl()` | 24 |
| `0x8008F2EC` | `SelfInit()` | 24 |
| `0x8008F334` | `SetNumber(long)` | 24 |

### hdTtlive : oxOvl [12 bytes]

*Source: C:\CHAN\GAME\SRC\FE\HDITEM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008EF0C` | `Update()` | 24 |
| `0x8008EF44` | `hdTtlive()` | 24 |
| `0x8008EF80` | `SetTtlive(long)` | 24 |

### oxFontFile [48 bytes]

*Source: C:\CHAN\GAME\SRC\FE\OXSCRMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80040998` | `FontInit(char*)` | 40 |
| `0x80040A8C` | `ReloadFont(char*)` | 48 |
| `0x80040BA0` | `FindFont(char*)` | 24 |

### oxOvl [8 bytes]

*Source: C:\CHAN\GAME\SRC\FE\OXOVL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80091050` | `oxOvl()` | 0 |
| `0x80091098` | `Init(xcOverlay*)` | 24 |
| `0x800910C8` | `SelfInit()` | 0 |
| `0x800910D0` | `SetVisible(short)` | 24 |
| `0x800910F8` | `IsVisible()` | 0 |
| `0x8009110C` | `GetPrimPos(xcPrimObj*, short&, short&)` | 24 |
| `0x80091134` | `SetPrimPos(xcPrimObj*, short, short)` | 24 |

### oxScreen : ccMinNode [16 bytes]

*Source: C:\CHAN\GAME\SRC\FE\OXSCREEN.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80091164` | `oxScreen()` | 24 |

### oxScreenManager : m_ScreenChange [48 bytes]

*Source: C:\CHAN\GAME\SRC\FE\OXSCRMGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80040420` | `oxScreenManager()` | 0 |
| `0x80040514` | `Update()` | 24 |
| `0x8004059C` | `Render()` | 32 |
| `0x800405F0` | `FindScreen(unsigned long)` | 0 |
| `0x800405F8` | `FindOverlay(char*)` | 24 |
| `0x80040634` | `GetScreenNames()` | 0 |
| `0x8004063C` | `GotoScreen(unsigned long)` | 0 |
| `0x8004064C` | `ScreenOperation()` | 32 |
| `0x80040784` | `FindOverlay(unsigned long)` | 24 |
| `0x800407B4` | `Init(char*, oxScreenManager*)` | 40 |
| `0x80040910` | `SelfUpdate()` | 0 |
| `0x80040918` | `GetScreenHash(unsigned long)` | 24 |
| `0x80040948` | `GotoStartScreen()` | 24 |
| `0x80040968` | `SelfInit()` | 0 |
| `0x80040970` | `PushScreen(unsigned long)` | 0 |
| `0x80040980` | `PopScreen()` | 0 |
| `0x8004098C` | `GetSection()` | 0 |

### BackG : BGGeo [28 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\BACKG.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80057D4C` | `BackG()` | 88 |
| `0x800580B4` | `InitBG()` | 24 |
| `0x800580DC` | `LoadBG()` | 40 |
| `0x80058370` | `DeleteBG()` | 24 |
| `0x8005839C` | `GetScrollY(long)` | 0 |
| `0x800583E4` | `DrawBG()` | 24 |
| `0x80058448` | `Draw()` | 72 |
| `0x80058660` | `DrawSprite(BGGEO*, int, int)` | 48 |
| `0x8005878C` | `DrawPolyG4(BGGEO*, short, short)` | 0 |
| `0x800588B4` | `UpdateSplat()` | 0 |
| `0x80058938` | `UpdateBG()` | 24 |
| `0x8005896C` | `GetCamVect()` | 40 |

### ErrorScreen [48 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\HUD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800402EC` | `GetScreenNames()` | 0 |
| `0x800402F8` | `SetErrorMessage(int)` | 24 |

### HUD : oxScreenManager [712 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\HUD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8003F44C` | `HUD()` | 32 |
| `0x8003F650` | `InternalReset()` | 0 |
| `0x8003F658` | `DisplayXHUD(Handler*)` | 24 |
| `0x8003F67C` | `Display()` | 32 |
| `0x8003F6F8` | `SetHUDVisible(int, int)` | 32 |
| `0x8003F88C` | `DisplayTally(int)` | 32 |
| `0x8003F8D0` | `ShowDestLevel()` | 24 |
| `0x8003F918` | `SelfInit()` | 48 |
| `0x8003FAB4` | `EnableInput(int)` | 40 |
| `0x8003FB34` | `DebugDisplay(int)` | 32 |
| `0x8003FC08` | `UpdateScreen(oxScreenManager*)` | 0 |
| `0x8003FC10` | `SelfUpdate()` | 32 |
| `0x8003FCA0` | `FindScreen(unsigned long)` | 0 |
| `0x8003FCB4` | `GetScreenNames()` | 0 |
| `0x8003FCC0` | `GetGameData()` | 32 |
| `0x8003FD84` | `UpdateBonusScore(long, long, const tagLVector&)` | 24 |
| `0x8003FDA4` | `TriggerBonusUpdate()` | 24 |
| `0x8003FDC4` | `DisplayTake(int, bool)` | 32 |
| `0x8003FE14` | `DisplayExtraTake(const tagLVector&)` | 24 |
| `0x8003FE54` | `UpdateHealth(long, long)` | 32 |
| `0x8003FF98` | `SetFoe(Humanoid*)` | 32 |
| `0x80040050` | `UpdateFoe(Humanoid*)` | 0 |
| `0x8004007C` | `TriggerButtonCallback(ButtonHook*)` | 24 |
| `0x800400DC` | `ToggleShowAll()` | 32 |
| `0x80040184` | `ShowBossHealth(const char*)` | 40 |
| `0x8004028C` | `OnLoadLevel()` | 0 |
| `0x80040294` | `OnUnloadLevel()` | 24 |

### MovieAction : action [12 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\MOVIES.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8001468C` | `MovieAction(MovieActionType)` | 0 |

### MoviePlay : MovieAction [92 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\MOVIES.CPP, \CHAN\GAME\INC\PSX\MOVIES.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800146A8` | `MoviePlay(const char*)` | 32 |
| `0x8001484C` | `GetMovie() const` | 0 |

### MoviePlayer [348 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\MOVIES.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80014338` | `MoviePlayer()` | 0 |
| `0x800143D0` | `AddAction(MovieAction*)` | 0 |
| `0x800143F4` | `MakePath(const char*)` | 24 |
| `0x80014434` | `SetPath(const char*)` | 24 |
| `0x800144B0` | `AddPlayMovie(const char*)` | 32 |
| `0x80014534` | `Play(void*(*)()*, int, void*)` | 56 |

### MovieRandom : MovieAction [20 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\MOVIES.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80014790` | `AddMovie(const char*)` | 32 |
| `0x800147EC` | `GetMovie() const` | 24 |

### ParticleInfo : ccMinNode [68 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\PARTICLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80097C4C` | `ParticleInfo()` | 24 |

### ParticleStats [136 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\PARTICLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80097CB4` | `ParticleStats()` | 0 |

### ParticleSystem : ccNode [100 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\PARTICLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80095494` | `Load(tReadChunk&, void**)` | 32 |
| `0x80095610` | `InitParticleInfoMemory()` | 40 |
| `0x800956D0` | `Unload()` | 24 |
| `0x80095740` | `UnloadLevel()` | 40 |
| `0x80095854` | `CommonParticles(int)` | 0 |
| `0x80095860` | `ParseData(tReadChunk&)` | 88 |
| `0x80095F78` | `AnalyzeMesh()` | 88 |
| `0x80096318` | `Find(unsigned long)` | 0 |
| `0x800963A4` | `SetParticleDirection(_RMVECT16*)` | 0 |
| `0x800963C4` | `ResetParticleDirection()` | 0 |
| `0x800963EC` | `PurgeParticles()` | 40 |
| `0x8009648C` | `ActiveParticles()` | 0 |
| `0x800964C0` | `CreateParticles(const _RMVECT16&, ParticleStats*)` | 40 |
| `0x80096698` | `InitParticles(const _RMVECT16&)` | 96 |
| `0x80096EF0` | `Update()` | 144 |
| `0x80097540` | `Display()` | 176 |
| `0x80097B18` | `ParticleSystem()` | 24 |

### ParticleSystemMgr [28 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\PARTICLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80097D1C` | `ParticleSystemMgr(ParticleSystem*)` | 0 |
| `0x80097D60` | `ParticleSystemMgr()` | 0 |
| `0x80097E00` | `InitMgr(ParticleSystem*)` | 0 |
| `0x80097E1C` | `CreateParticles(const _RMVECT16&, ParticleStats*)` | 32 |
| `0x80097ECC` | `SetParticleDirection(_RMVECT16*)` | 32 |
| `0x80097F30` | `ResetParticleDirection()` | 24 |
| `0x80097F60` | `Update()` | 24 |
| `0x80097F90` | `Display()` | 24 |
| `0x80097FC0` | `ActiveParticles()` | 24 |
| `0x80097FF0` | `PurgeParticles()` | 24 |

### Shadow [16 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\SHADOW.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AED38` | `Shadow(Model*)` | 0 |

### SimpleShadow : Shadow [32 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\SHADOW.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AF258` | `SimpleShadow(Model*)` | 24 |
| `0x800AF2D4` | `Place(tagLVector&, tagLVector*)` | 88 |
| `0x800AF4A4` | `Show(void*)` | 144 |

### Sound : Manager [44 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\SOUND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80059794` | `Sound()` | 24 |
| `0x80059818` | `InternalOpen()` | 32 |
| `0x800598D8` | `SetupSound()` | 32 |
| `0x800599B0` | `CleanupSound()` | 32 |
| `0x80059A4C` | `InstallMenu(MenuMgr*)` | 32 |
| `0x80059B84` | `OnMenuSelect(hdMenu*)` | 32 |

### SoundAnchor : DataAnchor [60 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\SOUND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80059C38` | `SetupSoundSphere()` | 32 |

### SoundMenuState [12 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\SOUND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80059DB4` | `SoundMenuState()` | 0 |
| `0x80059DBC` | `Restore()` | 24 |
| `0x80059E7C` | `Save()` | 0 |

### TreeShadow [16 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\SHADOW.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AEFE0` | `TreeShadow(Model*)` | 24 |
| `0x800AF03C` | `Place(tagLVector&, tagLVector*)` | 112 |
| `0x800AF214` | `Show(void*)` | 112 |

### tCellAlligator : m_pCellsAlloc [8204 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCCIMAGE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80044AD8` | `tCellAlligator()` | 0 |
| `0x80044AEC` | `InitCellArea(const xcRectSint16&)` | 32 |
| `0x80044B4C` | `InitPal4Area(const xcRectSint16&)` | 32 |
| `0x80044BAC` | `InitPal8Area(const xcRectSint16&)` | 32 |
| `0x80044C0C` | `DeleteAllocators()` | 24 |
| `0x80044C80` | `AllocCells(xcCellList*, unsigned long)` | 24 |
| `0x80044CA4` | `AllocPalettes4(xcCellList*, unsigned long)` | 24 |
| `0x80044CC8` | `AllocPalettes8(xcCellList*, unsigned long)` | 24 |

### tRAMTexAnim : tAnimation [32 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\RAMTEXANIM.CPP, \CHAN\GAME\INC\PSX\RAMTEXANIM.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80089324` | `tRAMTexAnim()` | 24 |
| `0x800893C4` | `MakePuppet()` | 32 |
| `0x8008981C` | `SetTextureData(tTexture**)` | 0 |
| `0x80089824` | `SetNumTextures(int)` | 0 |
| `0x8008982C` | `SetFrameData(unsigned char*)` | 0 |
| `0x80089834` | `SetNumFrames(int)` | 0 |
| `0x8008983C` | `GetTexture(int)` | 0 |
| `0x80089854` | `GetNumTextures()` | 0 |
| `0x80089860` | `GetFrame(int)` | 0 |
| `0x80089878` | `GetNumFrames()` | 0 |
| `0x80089884` | `GetEntityType()` | 0 |

### tRAMTexAnimLoader [12 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\RAMTEXANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800895A8` | `Load(tReadChunk&, void**)` | 592 |

### tRAMTexFlip : tFlipbook [40 bytes]

*Source: C:\CHAN\GAME\SRC\PSX\RAMTEXANIM.CPP, \CHAN\GAME\INC\PSX\RAMTEXANIM.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80089434` | `tRAMTexFlip()` | 24 |
| `0x80089490` | `Reset()` | 24 |
| `0x800894D0` | `Update()` | 32 |
| `0x80089810` | `GetEntityType()` | 0 |

### xc3x3Matrix [36 bytes]

*Source: C:\devsys\psx\xclib\INDEP\SRC\XC3X3MAT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C9414` | `SetUnit()` | 24 |
| `0x800C9450` | `SetTrans(long, long)` | 32 |
| `0x800C9494` | `Mult(const xc3x3Matrix&)` | 72 |
| `0x800C95A8` | `Mult(_RMVECT216*, _RMVECT216*, unsigned long)` | 8 |

### xc3x3MatrixStack : m_pMatrix [112 bytes]

*Source: C:\devsys\psx\xclib\INDEP\SRC\XC3X3MAT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C96A8` | `Inc()` | 0 |
| `0x800C96BC` | `Dec()` | 0 |

### xcCellImage [32 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCCIMAGE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80044CEC` | `xcCellImage(void*, xcCellImageMemoryTypeEnum)` | 32 |
| `0x80044E1C` | `FreeRamIfOwner()` | 24 |
| `0x80044E50` | `FreeVram()` | 24 |
| `0x80044ECC` | `FreeRam()` | 24 |
| `0x80044F0C` | `LoadToVram()` | 64 |

### xcCellList [8 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCVRAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80093D50` | `AddTail(xcCellNode*)` | 0 |
| `0x80093D78` | `AppendListTail(xcCellNode*, xcCellNode*)` | 0 |
| `0x80093DA0` | `AppendListHead(xcCellNode*, xcCellNode*)` | 0 |

### xcClipObj : xcPrimObj [12 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCDO.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AE4E0` | `sDraw(xcPrimObj*)` | 40 |

### xcColour1555 [2 bytes]

*Source: \CHAN\DEVSYS\PSX\XCLIB\INCLUDE\XCCOLOUR.H*

| Address | Method | Size |
|---------|--------|------|
| `0x800132C4` | `GetAlpha8() const` | 0 |
| `0x800132E4` | `GetBlue8() const` | 0 |
| `0x80013308` | `GetGreen8() const` | 0 |
| `0x8001332C` | `GetRed8() const` | 0 |
| `0x8005E56C` | `GetAlpha8() const` | 0 |
| `0x8005E58C` | `GetBlue8() const` | 0 |
| `0x8005E5B0` | `GetGreen8() const` | 0 |
| `0x8005E5D4` | `GetRed8() const` | 0 |
| `0x8005E5F4` | `Set8(unsigned char, unsigned char, unsigned char)` | 8 |
| `0x80090E6C` | `GetAlpha8() const` | 0 |
| `0x80090E8C` | `GetBlue8() const` | 0 |
| `0x80090EB0` | `GetGreen8() const` | 0 |
| `0x80090ED4` | `GetRed8() const` | 0 |

### xcFont [276 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCFONT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800915A0` | `xcFont(void*)` | 144 |
| `0x80091AC8` | `FindLetter(unsigned short) const` | 24 |
| `0x80091B20` | `ReloadData(void*)` | 48 |
| `0x80091C64` | `FindLetter(unsigned char) const` | 0 |

### xcFontDC : m_CMS [140 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCFONTDC.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C4E20` | `xcFontDC(xcTextObj&)` | 32 |
| `0x800C4ED4` | `Draw()` | 48 |
| `0x800C4F44` | `PushJustTrans(short, short)` | 40 |
| `0x800C505C` | `MakePolys(POLY_FT4*, xcPolyHandleFT4*)` | 136 |
| `0x800C53D0` | `GetSize(short*)` | 48 |
| `0x800C5514` | `GetWidthLine(long)` | 48 |

### xcImageDC : m_CMS [128 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCIDC.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C5610` | `xcImageDC(xcSprite&)` | 32 |
| `0x800C56AC` | `Draw()` | 24 |
| `0x800C5744` | `FindWalkingVectors(unsigned long, unsigned long, _RMVECT216*)` | 40 |
| `0x800C57C4` | `FindJust(unsigned long, unsigned long, _RMVECT216*)` | 0 |
| `0x800C5840` | `DrawPolys(POLY_FT4*)` | 208 |
| `0x800C5CC4` | `DrawSprite(SPRT*)` | 152 |

### xcInventory : m_chunk [20 bytes]

*Source: C:\devsys\psx\xclib\psx\INC\XCINV.H, C:\devsys\psx\xclib\psx\SRC\XCINV.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005F498` | `GetSizeInBytes() const` | 0 |
| `0x80091370` | `Sort()` | 32 |
| `0x800913B0` | `FixDataPointers(unsigned long)` | 40 |
| `0x80091420` | `FindItem(unsigned long)` | 0 |

### xcInventoryItem [8 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCINV.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009134C` | `FixDataPointers(unsigned long)` | 0 |

### xcOverlay [16 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCSOS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005E6F4` | `LoadAll()` | 32 |
| `0x8005E754` | `UnloadAll()` | 32 |
| `0x8005E7B4` | `SetVisible(unsigned long)` | 32 |
| `0x8005E810` | `FixDataPointers(unsigned long)` | 0 |
| `0x8005E848` | `FindNamedData(xcSectionMan*)` | 40 |
| `0x8005E8B8` | `DrawAll()` | 32 |
| `0x8005E90C` | `FindDOsh(unsigned long)` | 0 |
| `0x8005E954` | `GetPrimObj(unsigned long, xcChunkEnum)` | 24 |
| `0x8005E978` | `GetPrimObj(const char*, xcChunkEnum)` | 24 |
| `0x8005E9B0` | `GetSprite(unsigned long)` | 24 |
| `0x8005E9D0` | `GetTextObj(unsigned long)` | 24 |
| `0x8005E9F0` | `GetTextObj(const char*)` | 24 |
| `0x8005EA10` | `GetPolyG4(unsigned long)` | 24 |

### xcPolyHandleFT4 [16 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCFONTDC.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C4D8C` | `AddPolysToOT()` | 0 |

### xcPrimObj [4 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCDO.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AE2A8` | `sDraw(xcPrimObj*)` | 48 |
| `0x800AE3E8` | `FindNamedData(xcSectionMan*)` | 24 |
| `0x800AE430` | `Draw()` | 24 |
| `0x800AE478` | `Load()` | 24 |
| `0x800AE4A8` | `Unload()` | 24 |

### xcScreen [8 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCSOS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005EA30` | `FindNamedData(xcSectionMan*)` | 40 |
| `0x8005EAA4` | `LoadOverlays()` | 32 |

### xcSection [32 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCSOS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005EBD8` | `Init(unsigned char*, xcSectionMan*, unsigned long)` | 24 |
| `0x8005EC04` | `FixUpPointers()` | 24 |
| `0x8005EC40` | `GotoScreen(xcScreen*)` | 32 |
| `0x8005EC8C` | `UnloadOverlays()` | 40 |
| `0x8005ED04` | `Draw()` | 32 |
| `0x8005ED9C` | `FindImage(unsigned long)` | 24 |
| `0x8005EDC4` | `FindOverlay(unsigned long)` | 24 |
| `0x8005EE00` | `FindScreen(unsigned long)` | 24 |
| `0x8005EE28` | `FindString(unsigned long)` | 24 |
| `0x8005EE50` | `FixInventories()` | 40 |
| `0x8005EFA4` | `LoadCells()` | 48 |
| `0x8005F07C` | `FreeDiscardableData()` | 24 |
| `0x8005F0B0` | `FixScreenAndOverlayandDO()` | 40 |

### xcSectionMan [8 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCSOS.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8005F180` | `xcSectionMan()` | 0 |
| `0x8005F190` | `FindFont(unsigned long)` | 24 |
| `0x8005F1B8` | `FindFont(const char*)` | 24 |
| `0x8005F1EC` | `FreeSection()` | 24 |
| `0x8005F228` | `CreateNewSection()` | 24 |
| `0x8005F274` | `LoadSection(const char*, unsigned long)` | 40 |
| `0x8005F300` | `DeleteFonts()` | 40 |
| `0x8005F3B4` | `LoadFonts(xcInventory*)` | 40 |

### xcSprite : xcTextureObj [52 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCDO.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AE5F4` | `Load()` | 32 |
| `0x800AE670` | `Unload()` | 32 |
| `0x800AE6F8` | `FindNamedData(xcSectionMan*)` | 40 |
| `0x800AE76C` | `sDraw(xcPrimObj*)` | 152 |

### xcSpriteLetter [16 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCFONT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80091468` | `Init(const xciSpriteLetter*)` | 32 |

### xcTextObj : xcTextureObj [60 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCDO.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AE798` | `FindNamedData(xcSectionMan*)` | 40 |
| `0x800AE828` | `sDraw(xcPrimObj*)` | 168 |

### xcVRAMAllocator : m_FreeList [36 bytes]

*Source: C:\devsys\psx\xclib\psx\SRC\XCVRAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009394C` | `xcVRAMAllocator(const xcRectSint16&, unsigned long, unsigned long)` | 32 |
| `0x80093BE8` | `FreeAllVRAM()` | 40 |
| `0x80093C84` | `AllocCells(xcCellList*, unsigned long)` | 32 |
| `0x80093D18` | `FreeCells(xcCellList*)` | 24 |

### CDestructibleSound : CSound [28 bytes]

*Source: C:\CHAN\GAME\SRC\SND\DSTRSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AC584` | `Initialize(const tagLVector*)` | 24 |
| `0x800AC5A4` | `Smash()` | 24 |
| `0x800AC5F4` | `Think()` | 0 |
| `0x800AC610` | `CDestructibleSound()` | 24 |
| `0x800AC6A4` | `Load(const char*)` | 0 |
| `0x800AC6BC` | `GetMaterial(CSoundMaterial*)` | 0 |

### CDirectorSound : CSound [20 bytes]

*Source: C:\CHAN\GAME\SRC\SND\DRCTRSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008EB24` | `Initialize()` | 24 |
| `0x8008EB44` | `ProcessNISEvent(unsigned long, unsigned long)` | 24 |
| `0x8008EBAC` | `CDirectorSound()` | 24 |

### CFrontEndSound : CSound [32 bytes]

*Source: C:\CHAN\GAME\SRC\SND\FESND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800534C8` | `ProcessSoundEvent(Q214CFrontEndSound18FrontEndSoundEvent)` | 24 |
| `0x800535FC` | `CFrontEndSound()` | 24 |
| `0x800536B8` | `Initialize()` | 24 |
| `0x800536D8` | `Load(const char*)` | 0 |
| `0x800536E0` | `ProcessLocationSpecificSound(Q214CFrontEndSound18FrontEndSoundEvent)` | 32 |
| `0x80053850` | `HandleCursorEvent(Q214CFrontEndSound18FrontEndSoundEvent)` | 32 |

### CGenericPersistentSound : CSound [24 bytes]

*Source: C:\CHAN\GAME\SRC\SND\PRSTSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AC28C` | `CGenericPersistentSound()` | 24 |
| `0x800AC328` | `SetVol(unsigned char)` | 24 |
| `0x800AC3AC` | `Initialize(const tagLVector*, unsigned short)` | 24 |
| `0x800AC3CC` | `Begin()` | 56 |
| `0x800AC498` | `End()` | 24 |
| `0x800AC4F8` | `Load(const char*)` | 0 |
| `0x800AC564` | `Initialize(const tagLVector*)` | 24 |

### CGenericTransientSound : CSound [28 bytes]

*Source: C:\CHAN\GAME\SRC\SND\TRNSSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AA804` | `CGenericTransientSound()` | 24 |
| `0x800AA8A0` | `Initialize(const tagLVector*, unsigned short)` | 24 |
| `0x800AA8C0` | `InitializeStereo(unsigned char, unsigned char)` | 24 |
| `0x800AA8E8` | `Trigger(unsigned short)` | 32 |
| `0x800AA938` | `TriggerDialogWorld(unsigned short)` | 64 |
| `0x800AAA68` | `TriggerPositional(unsigned short)` | 64 |
| `0x800AAB9C` | `TriggerNotPositional(unsigned short)` | 56 |
| `0x800AAD4C` | `Load(const char*)` | 0 |

### CHumanoidSound : CSound [132 bytes]

*Source: C:\CHAN\GAME\SRC\SND\HMNDSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80060658` | `Initialize(const tagLVector*, const Humanoid*)` | 24 |
| `0x80060678` | `CHumanoidSound()` | 24 |
| `0x8006074C` | `Footstep(CSoundMaterial)` | 24 |
| `0x80060790` | `Strafe(CSoundMaterial)` | 24 |
| `0x800607B8` | `Fall()` | 24 |
| `0x800607E0` | `Land(CSoundMaterial)` | 32 |
| `0x80060850` | `DiveRoll(CSoundMaterial)` | 24 |
| `0x80060878` | `HitWorldStructure(CSoundMaterial)` | 24 |
| `0x800608A0` | `HandPlant()` | 24 |
| `0x800608C8` | `Grab(CSoundMaterial)` | 24 |
| `0x8006090C` | `GrabHumanoid()` | 24 |
| `0x80060934` | `FrontFlip()` | 24 |
| `0x8006095C` | `WallJump()` | 24 |
| `0x8006097C` | `PoleSwing()` | 24 |
| `0x800609A4` | `PunchMiss()` | 24 |
| `0x800609C8` | `KickMiss()` | 24 |
| `0x800609EC` | `PunchHit()` | 24 |
| `0x80060A28` | `KickHit()` | 24 |
| `0x80060A64` | `SuperPunch()` | 24 |
| `0x80060AB4` | `SuperKick()` | 24 |
| `0x80060B04` | `Collapse(CSoundMaterial)` | 24 |
| `0x80060B48` | `FlyThroughAir()` | 24 |
| `0x80060B70` | `BeginStun()` | 24 |
| `0x80060B94` | `EndStun()` | 24 |
| `0x80060BB4` | `BeginSlideOnSurface(CSoundMaterial)` | 24 |
| `0x80060BD8` | `EndSlideOnSurface()` | 24 |
| `0x80060BF8` | `BeginSlideDownLadder()` | 24 |
| `0x80060C1C` | `EndSlideDownLadder()` | 24 |
| `0x80060C3C` | `EndAllSounds()` | 24 |
| `0x80060C74` | `Load(const char*)` | 0 |
| `0x80060D1C` | `FXDialogHit()` | 24 |
| `0x80060D7C` | `FXDialogAttack()` | 24 |
| `0x80060DDC` | `PlayAttack(unsigned short)` | 32 |
| `0x80060E40` | `PlayHit(unsigned short)` | 32 |
| `0x80060EA4` | `Think()` | 0 |
| `0x80060EE8` | `MapSoundScriptEvent(SSHumanoid)` | 24 |
| `0x80061170` | `GrabWeapon()` | 24 |
| `0x800611C0` | `WeaponMiss()` | 24 |
| `0x80061210` | `WeaponHit()` | 24 |
| `0x80061260` | `Breath()` | 24 |
| `0x80061288` | `Grunt()` | 24 |
| `0x800612D4` | `HitByFireBlast()` | 24 |
| `0x80061314` | `LoadHumanoidSoundScripts()` | 32 |
| `0x800613C0` | `UnloadHumanoidSoundScripts()` | 24 |
| `0x800613EC` | `ProcessSoundScript(unsigned long, unsigned long)` | 24 |

### CInteractiveMusicController

*Source: C:\CHAN\GAME\SRC\SND\MSCCTRLR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80082960` | `Think()` | 24 |
| `0x80082A60` | `UnloadLevel()` | 24 |
| `0x80082A9C` | `LoadLevel(rsSoundLocation)` | 24 |
| `0x80082ADC` | `LevelBegin()` | 24 |
| `0x80082B30` | `OpenPlayer(char*)` | 136 |

### CKickNRollSound : CSound [32 bytes]

*Source: C:\CHAN\GAME\SRC\SND\KICKSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AD210` | `BeginRoll()` | 24 |
| `0x800AD234` | `EndRoll()` | 24 |
| `0x800AD254` | `Kick()` | 24 |
| `0x800AD294` | `HitHumanoid()` | 24 |
| `0x800AD2B4` | `CKickNRollSound()` | 24 |
| `0x800AD34C` | `Load(const char*)` | 0 |
| `0x800AD36C` | `Initialize(const tagLVector*)` | 24 |
| `0x800AD38C` | `Think()` | 0 |

### CKnockDownSound : CSound [32 bytes]

*Source: C:\CHAN\GAME\SRC\SND\KNDNSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AD3A8` | `Initialize(const tagLVector*)` | 24 |
| `0x800AD3C8` | `BeginFall()` | 24 |
| `0x800AD3EC` | `EndFall()` | 24 |
| `0x800AD40C` | `Kick()` | 24 |
| `0x800AD44C` | `Impact()` | 24 |
| `0x800AD48C` | `HitHumanoid()` | 24 |
| `0x800AD4AC` | `Think()` | 0 |
| `0x800AD4DC` | `CKnockDownSound()` | 24 |
| `0x800AD578` | `Load(const char*)` | 0 |

### CParticleEffectSound : CSound [24 bytes]

*Source: C:\CHAN\GAME\SRC\SND\ESOUND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800ACC9C` | `CParticleEffectSound()` | 24 |
| `0x800ACD38` | `Initialize(const tagLVector*)` | 24 |
| `0x800ACD58` | `Load(const char*)` | 0 |
| `0x800ACD70` | `StartAnimating()` | 24 |
| `0x800ACD94` | `StopAnimating()` | 24 |

### CPendulumSound : CSound [24 bytes]

*Source: C:\CHAN\GAME\SRC\SND\PNDLMSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800ACB84` | `Initialize(const tagLVector*)` | 24 |
| `0x800ACBA4` | `Swing()` | 24 |
| `0x800ACBCC` | `HitHumanoid()` | 24 |
| `0x800ACBF4` | `CPendulumSound()` | 24 |
| `0x800ACC7C` | `Load(const char*)` | 0 |

### CPhaseManager : m_pTable [8 bytes]

*Source: C:\CHAN\GAME\SRC\SND\PHSMNGR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80082B98` | `CPhaseManager(unsigned char, unsigned char)` | 40 |
| `0x80082C70` | `PlayRequest(unsigned short)` | 0 |
| `0x80082CF0` | `Think()` | 0 |

### CPlatformSound : CSound [56 bytes]

*Source: C:\CHAN\GAME\SRC\SND\PLATSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AC894` | `CPlatformSound()` | 24 |
| `0x800AC94C` | `Initialize(const tagLVector*)` | 24 |
| `0x800AC984` | `BeginMove()` | 32 |
| `0x800AC9E0` | `EndMove()` | 24 |
| `0x800ACA2C` | `HitPathNode(long, long, long)` | 24 |
| `0x800ACA6C` | `Tilt()` | 24 |
| `0x800ACA94` | `Impact()` | 24 |
| `0x800ACABC` | `HitHumanoid()` | 24 |
| `0x800ACAFC` | `Think()` | 0 |
| `0x800ACB2C` | `Load(const char*)` | 0 |
| `0x800ACB70` | `GetMaterial(CSoundMaterial*)` | 0 |

### CPushableSound : CSound [36 bytes]

*Source: C:\CHAN\GAME\SRC\SND\PUSHSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AC6D0` | `BeginPush()` | 24 |
| `0x800AC6F4` | `EndPush()` | 24 |
| `0x800AC714` | `Kick()` | 24 |
| `0x800AC75C` | `HitHumanoid()` | 24 |
| `0x800AC77C` | `CPushableSound()` | 24 |
| `0x800AC824` | `Load(const char*)` | 0 |
| `0x800AC844` | `Initialize(const tagLVector*)` | 24 |
| `0x800AC864` | `Think()` | 0 |
| `0x800AC880` | `GetMaterial(CSoundMaterial*)` | 0 |

### CSound [16 bytes]

*Source: C:\CHAN\GAME\SRC\SND\BASESND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A1E18` | `CSound()` | 24 |
| `0x800A1F40` | `Load(const char*)` | 0 |
| `0x800A1F48` | `Initialize(const tagLVector*)` | 0 |
| `0x800A1F54` | `Release()` | 24 |
| `0x800A1FAC` | `GetPosPtr() const` | 0 |
| `0x800A1FB8` | `BeginPersistent(unsigned char, CGenericPersistentSound**)` | 32 |
| `0x800A2038` | `EndPersistent(CGenericPersistentSound**)` | 24 |
| `0x800A2088` | `PlayTransient(unsigned short, unsigned long, unsigned short)` | 48 |
| `0x800A2164` | `PlayTransientStereo(unsigned short, unsigned short)` | 32 |

### CSoundDirect

*Source: C:\CHAN\GAME\SRC\SND\SNDDRCT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008D5B0` | `PlayTransient(unsigned short, const tagLVector*, unsigned short, unsigned long)` | 48 |
| `0x8008D650` | `BeginPersistent(unsigned char, CGenericPersistentSound**, const tagLVector*)` | 32 |
| `0x8008D6C4` | `EndPersistent(CGenericPersistentSound**)` | 24 |

### CSoundFactory

*Source: C:\CHAN\GAME\SRC\SND\SNDFACT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80057468` | `Initialize(unsigned long)` | 24 |
| `0x8005752C` | `Destroy()` | 24 |
| `0x8005759C` | `CreateObject(unsigned long, CSound**, unsigned long)` | 40 |
| `0x800577E0` | `GetMemoryPoolPtr(void***)` | 0 |
| `0x80057808` | `ObjectCreated()` | 0 |
| `0x80057820` | `ObjectDestroyed()` | 0 |
| `0x80057838` | `LoadDatabase(const char*)` | 0 |
| `0x80057840` | `MaintenanceTask(_RTASK*)` | 24 |

### CSoundFactoryDatabase [1 bytes]

*Source: C:\CHAN\GAME\SRC\SND\SNDFDB.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AADBC` | `CSoundFactoryDatabase()` | 0 |
| `0x800AADEC` | `LoadObject(unsigned long, CSound*, unsigned long)` | 24 |
| `0x800AAF2C` | `CreateParticleEffectSound(CSound*, unsigned long)` | 32 |
| `0x800AB040` | `CreateWorldEffectSound(CSound*, unsigned long)` | 40 |
| `0x800AB234` | `CreatePushableSound(CSound*, unsigned long)` | 40 |
| `0x800AB370` | `CreateKickNRollSound(CSound*, unsigned long)` | 32 |
| `0x800AB3B4` | `CreatePlatformSound(CSound*, unsigned long)` | 48 |
| `0x800AB900` | `CreateWeaponSound(CSound*, unsigned long)` | 40 |
| `0x800AB9BC` | `CreateHumanoidSound(CSound*, unsigned long)` | 112 |
| `0x800ABB64` | `CreateDestructibleSound(CSound*, unsigned long)` | 32 |
| `0x800ABF54` | `CreatePendulumSound(CSound*, unsigned long)` | 32 |
| `0x800AC074` | `CreateGenericTransientSound(CSound*, unsigned long)` | 40 |
| `0x800AC100` | `CreateGenericPersistentSound(CSound*, unsigned long)` | 40 |
| `0x800AC18C` | `CreateKnockDownSound(CSound*, unsigned long)` | 32 |
| `0x800AC1D8` | `CreateFrontEndSound(CSound*, unsigned long)` | 24 |
| `0x800AC20C` | `CreateDirectorSound(CSound*, unsigned long)` | 24 |
| `0x800AC240` | `IsBasicSoundLoaded(unsigned long)` | 24 |

### CWeaponSound : CSound [28 bytes]

*Source: C:\CHAN\GAME\SRC\SND\WPNSND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AD074` | `Initialize(const tagLVector*)` | 24 |
| `0x800AD094` | `HitHumanoid()` | 24 |
| `0x800AD0BC` | `Grab()` | 24 |
| `0x800AD0E4` | `Explode()` | 24 |
| `0x800AD128` | `Miss()` | 24 |
| `0x800AD150` | `CWeaponSound()` | 24 |
| `0x800AD1D8` | `Load(const char*)` | 0 |

### CWorldEffectSound : CSound [32 bytes]

*Source: C:\CHAN\GAME\SRC\SND\ESOUND.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800ACDB4` | `CWorldEffectSound()` | 24 |
| `0x800ACE54` | `Initialize(const tagLVector*)` | 24 |
| `0x800ACE74` | `Load(const char*)` | 0 |
| `0x800ACEB0` | `StartAnimating()` | 24 |
| `0x800ACED4` | `Update(unsigned long)` | 32 |
| `0x800ACF90` | `StopAnimating()` | 24 |
| `0x800ACFB0` | `VolRiseAndFall(unsigned long)` | 24 |

### rsdAmbiance : rsdLoadCallback [328 bytes]

*Source: C:\CHAN\GAME\SRC\SND\RSDAMBCE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80080D74` | `rsdAmbiance()` | 0 |
| `0x80080E0C` | `Open(const char*, long, long, long)` | 168 |
| `0x8008132C` | `Close()` | 32 |
| `0x80081464` | `Start(unsigned long)` | 24 |
| `0x8008158C` | `Stop()` | 40 |
| `0x8008167C` | `CdYield()` | 0 |
| `0x80081694` | `CdAccess()` | 0 |
| `0x800816C0` | `SetSpace(unsigned long)` | 24 |
| `0x80081780` | `GetSpace()` | 0 |
| `0x8008178C` | `SetVolume(bool, unsigned short, unsigned short)` | 40 |
| `0x80081828` | `SetCrossFadeDuration(long)` | 0 |
| `0x800818C8` | `FadeIn(unsigned long, bool, unsigned long)` | 40 |
| `0x800819C0` | `FadeOut(unsigned long, bool)` | 32 |
| `0x80081A74` | `FadeTask(_RTASK*)` | 40 |
| `0x80081BF4` | `GetForegroundSample(unsigned long*, N21)` | 40 |
| `0x80081D0C` | `GetBackgroundSample(unsigned long*)` | 32 |
| `0x80081DCC` | `SetVoiceVol(long, unsigned long, unsigned long)` | 24 |
| `0x80081EB0` | `AmbianceTaskStub(_RTASK*)` | 24 |
| `0x80081ED4` | `AmbianceTask()` | 48 |
| `0x80082898` | `CDDoneCallback(long, long, long)` | 32 |
| `0x80082924` | `CDDoneFreeMemory(long, long, long)` | 24 |

### rsdClip : rsdLoadCallback [68 bytes]

*Source: C:\CHAN\GAME\SRC\SND\RSDCLIP.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80083C10` | `ReadDialog(void**, unsigned long, unsigned long)` | 32 |
| `0x80083D38` | `SetWorld(rsdWorld*)` | 0 |
| `0x80083D44` | `CDDoneCallback(long, long, long)` | 48 |
| `0x80083E50` | `FreeTransferBuffer()` | 24 |
| `0x80083E98` | `IsVoicePlaying()` | 24 |

### rsdLoad [24 bytes]

*Source: C:\CHAN\GAME\SRC\SND\RSDLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009361C` | `Initialize(unsigned long)` | 24 |
| `0x80093660` | `Terminate()` | 24 |
| `0x800936A8` | `IsBusy()` | 0 |
| `0x800936B4` | `SetHighMemoryLimit(unsigned long)` | 0 |
| `0x80093730` | `rsdLoad(unsigned long, const unsigned char*, unsigned long, rsdLoadCallback*)` | 24 |
| `0x80093818` | `Destroy()` | 24 |
| `0x800938A4` | `TransferComplete()` | 24 |

### rsdLoadCallback [8 bytes]

*Source: \CHAN\GAME\INC\SND\RSDLOAD.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8004491C` | `Callback(rsdLoad&)` | 0 |
| `0x80082950` | `Callback(rsdLoad&)` | 0 |
| `0x80083B48` | `Callback(rsdLoad&)` | 0 |
| `0x80083EF0` | `Callback(rsdLoad&)` | 0 |
| `0x800BA0E0` | `Callback(rsdLoad&)` | 0 |

### rsdMusicPlayer : rsdStreamCallback [208 bytes]

*Source: C:\CHAN\GAME\SRC\SND\RSMPLR.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007F458` | `rsdMusicPlayer()` | 32 |
| `0x8007F54C` | `Open(const char*, long, bool, bool, unsigned long)` | 112 |
| `0x8007F6E4` | `Close()` | 24 |
| `0x8007F754` | `Start(unsigned long)` | 32 |
| `0x8007F7E4` | `Stop()` | 24 |
| `0x8007F850` | `CdYield()` | 24 |
| `0x8007F880` | `CdAccess()` | 24 |
| `0x8007F8C0` | `SetVolume(bool, unsigned short, unsigned short)` | 24 |
| `0x8007F958` | `FadeIn()` | 24 |
| `0x8007FA08` | `CueNextSong(unsigned long)` | 32 |
| `0x8007FA50` | `FadeOut(bool)` | 32 |
| `0x8007FB0C` | `Callback(unsigned long*, unsigned long*)` | 0 |
| `0x8007FB70` | `FadeTask(_RTASK*)` | 24 |
| `0x8007FD74` | `GetLastMusicWakeUp()` | 24 |
| `0x8007FD94` | `IsCdYielded()` | 24 |

### rsdPersistent [32 bytes]

*Source: C:\CHAN\GAME\SRC\SND\RSDUTIL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80080508` | `Initialize(unsigned long, unsigned long, rsdWorld*)` | 24 |
| `0x80080558` | `Terminate()` | 24 |
| `0x800805C8` | `rsdPersistent(long, const tagLVector*, unsigned long, unsigned short, unsigned short, unsigned long)` | 40 |
| `0x80080758` | `ObjectExists(rsdPersistent*)` | 0 |
| `0x80080790` | `DeleteAll()` | 24 |
| `0x800807E0` | `FadeOutAll(unsigned long)` | 0 |
| `0x80080824` | `FadeInAll(unsigned long)` | 0 |
| `0x80080864` | `UnloadQuietest(rsdPersistent*)` | 56 |
| `0x80080998` | `CaptureVoice()` | 24 |
| `0x80080A38` | `UnloadVoice()` | 32 |
| `0x80080A98` | `Think()` | 24 |
| `0x80080B34` | `ApplyVolume(bool)` | 48 |
| `0x80080C5C` | `UpdateTask(_RTASK*)` | 24 |
| `0x80080D08` | `DelayTask(_RTASK*)` | 48 |

### rsdStream : rsdLoadCallback [128 bytes]

*Source: C:\CHAN\GAME\SRC\SND\RSDSTRM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B91A4` | `rsdStream()` | 0 |
| `0x800B921C` | `Open(void**, unsigned long, long, bool, bool, rsdStreamCallback*)` | 32 |
| `0x800B93F0` | `Close()` | 40 |
| `0x800B9520` | `StartCallback(long, long, long)` | 24 |
| `0x800B9540` | `Start(unsigned long, unsigned long)` | 24 |
| `0x800B9578` | `StartCDQueueISEmpty(unsigned long, unsigned long)` | 32 |
| `0x800B96F8` | `Stop()` | 24 |
| `0x800B98E4` | `CdYield()` | 0 |
| `0x800B98F0` | `CdAccess()` | 0 |
| `0x800B98F8` | `SetVolume(bool, unsigned short, unsigned short)` | 32 |
| `0x800B999C` | `GetLastMusicWakeUp()` | 0 |
| `0x800B99A8` | `IsCdYielded()` | 0 |
| `0x800B99B4` | `SPUAddrCallback()` | 24 |
| `0x800B9B60` | `Callback(rsdLoad&)` | 32 |
| `0x800B9C44` | `LoadTaskStub(_RTASK*)` | 24 |
| `0x800B9C68` | `LoadTask()` | 40 |
| `0x800B9F54` | `CDDoneCallback(long, long, long)` | 32 |
| `0x800BA04C` | `CDDoneFreeMemory(long, long, long)` | 24 |
| `0x800BA078` | `VoiceOffTask(_RTASK*)` | 24 |
| `0x800BA0D8` | `EnableSeamlessStitching(bool)` | 0 |

### rsdWorld [20 bytes]

*Source: C:\CHAN\GAME\SRC\SND\RSDUTIL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8007FDB4` | `rsdWorld()` | 0 |
| `0x8007FE00` | `SetMicrophone(const tagLVector*, const long*)` | 0 |
| `0x8007FE0C` | `SetVolumeScale(unsigned long)` | 0 |
| `0x8007FE14` | `SetStereo(bool)` | 0 |
| `0x8007FE30` | `GetObjectVolumes(unsigned short, unsigned short*, unsigned short*)` | 0 |
| `0x8007FE60` | `GetObjectVolumes(unsigned short, const tagLVector*, unsigned short*, unsigned short*, unsigned long)` | 80 |
| `0x800801B4` | `IsObjectAtPosAudible(const tagLVector*, unsigned long)` | 24 |
| `0x80080220` | `PlayTransient(long, const tagLVector*, unsigned short, unsigned short, unsigned short, unsigned long)` | 48 |
| `0x80080334` | `PlayTransient(long, unsigned short, unsigned short, unsigned short, unsigned int)` | 48 |
| `0x800804D4` | `PlayTransientDelayedTask(_RTASK*)` | 24 |

### GTELCMatrix

*Source: C:\v11.3\SOURCE\GTEMATRIX.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009F900` | `GetRot(MATRIX*)` | 0 |
| `0x8009F930` | `FillRot(const MATRIX&)` | 0 |
| `0x8009F960` | `FillRotZero()` | 0 |

### GTELDMatrix

*Source: C:\v11.3\SOURCE\GTEMATRIX.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009F884` | `GetRot(MATRIX*)` | 0 |
| `0x8009F8B4` | `FillRot(const MATRIX&)` | 0 |
| `0x8009F8E4` | `FillRotZero()` | 0 |

### GTERTMatrix

*Source: C:\v11.3\SOURCE\GTEMATRIX.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009F7F4` | `GetMatrix(MATRIX*)` | 0 |
| `0x8009F83C` | `FillMatrix(const MATRIX&)` | 0 |

### GTEVXMatrix

*Source: C:\v11.3\INCLUDE\GTEMATRIX.HPP, C:\v11.3\SOURCE\GTEMATRIX.CPP, \CHAN\DEVSYS\PSX\PURE3D\INCLUDE\GTEMATRIX.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80053DC8` | `FillRotZAfterSinCos()` | 0 |
| `0x80053DFC` | `FillRotYAfterSinCos()` | 0 |
| `0x80053E18` | `FillRotXAfterSinCos()` | 0 |
| `0x800724A0` | `FillRotZAfterSinCos()` | 0 |
| `0x800724D4` | `FillRotYAfterSinCos()` | 0 |
| `0x800724F0` | `FillRotXAfterSinCos()` | 0 |
| `0x800982C0` | `FillRotZAfterSinCos()` | 0 |
| `0x800982F4` | `FillRotYAfterSinCos()` | 0 |
| `0x80098310` | `FillRotXAfterSinCos()` | 0 |
| `0x8009F760` | `FillRotScale(const _RMVECT16&)` | 24 |
| `0x8009F790` | `FillRotScale(long, long, long)` | 0 |
| `0x8009F7C8` | `FillRotScale(long)` | 0 |
| `0x8009FC98` | `FillRotZAfterSinCos()` | 0 |
| `0x8009FCCC` | `FillRotYAfterSinCos()` | 0 |
| `0x8009FCE8` | `FillRotXAfterSinCos()` | 0 |

### InvCacheElem [8 bytes]

*Source: C:\v11.3\SOURCE\TCACHE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BBDD4` | `InvCacheElem()` | 0 |

### P3D

*Source: C:\v11.3\SOURCE\P3DGBL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80086A98` | `BeginFrame()` | 24 |
| `0x80086AC4` | `EndFrame(int)` | 24 |
| `0x800CAEA8` | `Init()` | 96 |

### RenderQueue [40 bytes]

*Source: C:\v11.3\SOURCE\RQUEUE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009FD3C` | `RenderQueue(unsigned long)` | 32 |
| `0x8009FDAC` | `WaitForLayer(unsigned long)` | 40 |
| `0x8009FE64` | `QueueLayer(unsigned long)` | 32 |
| `0x8009FF7C` | `QueueSwap()` | 24 |

### t2PointCamFlip : tParamFlip [52 bytes]

*Source: C:\v11.3\INCLUDE\2PTCAMFLIP.HPP, C:\v11.3\SOURCE\2PTCAMFLIP.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009D798` | `Update()` | 80 |
| `0x8009D9A8` | `Attach(t2PointMatrixCamera*, tParamAnim*)` | 32 |
| `0x8009DA38` | `GetEntityType()` | 0 |

### t2PointMatrixCamera : tCamera [52 bytes]

*Source: C:\v11.3\SOURCE\T2POINTCAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009D3DC` | `t2PointMatrixCamera()` | 24 |
| `0x8009D460` | `SetTarget(_RMVECT16*)` | 0 |
| `0x8009D480` | `SetPosition(_RMVECT16*)` | 0 |
| `0x8009D4A0` | `SetTwist(unsigned short)` | 0 |
| `0x8009D4A8` | `GetTarget(_RMVECT16*)` | 0 |
| `0x8009D4C8` | `GetPosition(_RMVECT16*)` | 0 |
| `0x8009D4E8` | `GetTwist()` | 0 |
| `0x8009D4F4` | `UpdateMatrix()` | 128 |

### tAnimation [12 bytes]

*Source: C:\v11.3\SOURCE\ANIMATE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008D22C` | `tAnimation()` | 24 |

### tByteStream [4 bytes]

*Source: C:\v11.3\INCLUDE\TFILE.HPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B8098` | `GetMemPosition()` | 0 |

### tCBVAnim : tAnimation [40 bytes]

*Source: C:\v11.3\INCLUDE\CBVANIM.HPP, C:\v11.3\SOURCE\CBVANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C3384` | `tCBVAnim()` | 24 |
| `0x800C343C` | `Init(int, int, int, unsigned long*, unsigned long*)` | 40 |
| `0x800C34D0` | `MakePuppet()` | 32 |
| `0x800C3A0C` | `GetNumFrames()` | 0 |
| `0x800C3A18` | `GetEntityType()` | 0 |

### tCBVAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\CBVANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C3728` | `Load(tReadChunk&, void**)` | 152 |

### tCBVFlip : tFlipbook [36 bytes]

*Source: C:\v11.3\INCLUDE\CBVANIM.HPP, C:\v11.3\SOURCE\CBVANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C3544` | `tCBVFlip()` | 24 |
| `0x800C35A0` | `Reset()` | 24 |
| `0x800C35D0` | `Update()` | 0 |
| `0x800C3A00` | `GetEntityType()` | 0 |

### tCBVParamAnim : tCBVAnim [44 bytes]

*Source: C:\v11.3\INCLUDE\CBVPARAM.HPP, C:\v11.3\SOURCE\CBVPARAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C2AA8` | `tCBVParamAnim()` | 24 |
| `0x800C2B04` | `SetBlendAnim(tParamAnim*)` | 0 |
| `0x800C2B0C` | `MakePuppet()` | 32 |
| `0x800C3344` | `GetEntityType()` | 0 |

### tCBVParamAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\CBVPARAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C301C` | `Load(tReadChunk&, void**)` | 192 |

### tCBVParamFlip : tParamFlip [56 bytes]

*Source: C:\v11.3\INCLUDE\CBVPARAM.HPP, C:\v11.3\SOURCE\CBVPARAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C2B7C` | `tCBVParamFlip()` | 24 |
| `0x800C2BD8` | `Reset()` | 24 |
| `0x800C2C08` | `Update()` | 48 |
| `0x800C2F84` | `Attach(tCBVParamAnim*, tParamAnim*)` | 32 |
| `0x800C3338` | `GetEntityType()` | 0 |

### tCache [20 bytes]

*Source: C:\v11.3\SOURCE\TCACHE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BBD7C` | `tCache()` | 0 |

### tCamera : tEntity [24 bytes]

*Source: C:\v11.3\SOURCE\TCAMERA.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008EC44` | `tCamera()` | 24 |
| `0x8008ECC0` | `SetFOV(long, long)` | 0 |
| `0x8008ECCC` | `GetFOV(long*, long*)` | 0 |
| `0x8008ECE4` | `GetClipPlanes(unsigned short*, unsigned short*)` | 0 |

### tChunk [20 bytes]

*Source: C:\v11.3\SOURCE\TCHUNK.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B1268` | `tChunk(tFile*)` | 0 |
| `0x800B12C0` | `SetFile(tFile*)` | 0 |
| `0x800B12C8` | `GetFile()` | 0 |

### tClutAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\TCLTLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008656C` | `Load(tReadChunk&, void**)` | 456 |

### tClutFlip : tFlipbook [52 bytes]

*Source: C:\v11.3\INCLUDE\CLUTANIM.HPP, C:\v11.3\SOURCE\CLUTANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BB78C` | `tClutFlip()` | 24 |
| `0x800BB7FC` | `Reset()` | 24 |
| `0x800BB84C` | `Update()` | 24 |
| `0x800BB94C` | `GetClutFrame(int)` | 24 |
| `0x800BB984` | `GetEntityType()` | 0 |

### tClutList : tAnimation [40 bytes]

*Source: C:\v11.3\INCLUDE\CLUTANIM.HPP, C:\v11.3\SOURCE\CLUTANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BB4F0` | `tClutList()` | 24 |
| `0x800BB5AC` | `MakePuppet()` | 32 |
| `0x800BB638` | `GetNumFrames()` | 0 |
| `0x800BB644` | `GetFrame(int)` | 0 |
| `0x800BB65C` | `DeleteFrames()` | 24 |
| `0x800BB69C` | `SetNumFrames(int)` | 32 |
| `0x800BB700` | `SetNumOffsets(int)` | 32 |
| `0x800BB764` | `SetFrame(int, unsigned short)` | 0 |
| `0x800BB778` | `SetOffset(int, unsigned short)` | 0 |
| `0x800BB990` | `GetEntityType()` | 0 |

### tCompAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\COMPANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80085E10` | `Load(tReadChunk&, void**)` | 32 |

### tCompositeAnim : tAnimation [24 bytes]

*Source: C:\v11.3\INCLUDE\COMPANIM.HPP, C:\v11.3\SOURCE\COMPANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008608C` | `tCompositeAnim()` | 24 |
| `0x80086110` | `DeleteAll()` | 32 |
| `0x80086190` | `SetNumParts(int)` | 48 |
| `0x8008624C` | `MakePuppet()` | 32 |
| `0x800862BC` | `GetPart(int)` | 0 |
| `0x800864EC` | `GetNumFrames()` | 0 |
| `0x800864F8` | `GetEntityType()` | 0 |

### tCompositeAnimPart [12 bytes]

*Source: C:\v11.3\SOURCE\COMPANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80086010` | `tCompositeAnimPart()` | 0 |

### tCompositeFlip [32 bytes]

*Source: C:\v11.3\INCLUDE\COMPANIM.HPP, C:\v11.3\SOURCE\COMPANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800862D4` | `tCompositeFlip()` | 24 |
| `0x80086330` | `Reset()` | 24 |
| `0x80086380` | `Update()` | 48 |
| `0x800864E0` | `GetEntityType()` | 0 |

### tDirectionalLight : tLight [36 bytes]

*Source: C:\v11.3\SOURCE\TDLIGHT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C4604` | `tDirectionalLight(tLightHardwareSlot)` | 24 |
| `0x800C4644` | `SetDirection(long, long, long)` | 24 |
| `0x800C467C` | `Update()` | 88 |

### tDoubleLayer [48 bytes]

*Source: C:\v11.3\SOURCE\TLAYER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A04D8` | `tDoubleLayer(unsigned long, unsigned long)` | 32 |
| `0x800A0588` | `Draw()` | 24 |
| `0x800A05C4` | `Flip()` | 0 |
| `0x800A05E8` | `FlipToDrawOT()` | 0 |
| `0x800A05FC` | `Check()` | 0 |
| `0x800A0608` | `Start()` | 24 |
| `0x800A068C` | `End()` | 0 |
| `0x800A06F0` | `Free()` | 24 |

### tDrawTable : tEntity [60 bytes]

*Source: C:\v11.3\SOURCE\TDTABLE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B493C` | `tDrawTable()` | 24 |
| `0x800B49C8` | `DrawGeometry(const tGeometry*)` | 40 |

### tDynGeom : tGeometry [100 bytes]

*Source: C:\v11.3\INCLUDE\TDYNGEOM.HPP, C:\v11.3\SOURCE\TDYNGEOM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BB168` | `Clone()` | 32 |
| `0x800BB2C8` | `Display()` | 24 |
| `0x800BB304` | `GetGeoType()` | 0 |
| `0x800BB334` | `GetEntityType()` | 0 |

### tDynamicKeyList : tKeyList [16 bytes]

*Source: C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AF814` | `tDynamicKeyList()` | 0 |
| `0x800AF834` | `tDynamicKeyList(int)` | 0 |
| `0x800AF8B0` | `FindFirstKey(int)` | 0 |

### tETree : tTree [28 bytes]

*Source: C:\v11.3\INCLUDE\ETREE.HPP, C:\v11.3\SOURCE\ETREE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C8F58` | `DeepCopy()` | 40 |
| `0x800C90E8` | `Display()` | 48 |
| `0x800C9280` | `GetJointAbsolute(int)` | 0 |
| `0x800C92A0` | `GetJoint(int)` | 0 |
| `0x800C92D4` | `GetJointList()` | 0 |

### tETreeLoader [12 bytes]

*Source: C:\v11.3\SOURCE\ETLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C0880` | `LoadInternal(tReadChunk&, void**)` | 328 |
| `0x800C0A90` | `Load(tReadChunk&, void**)` | 32 |

### tEntity [12 bytes]

*Source: C:\v11.3\INCLUDE\TENTITY.HPP, C:\v11.3\SOURCE\TENTITY.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008D190` | `tEntity()` | 0 |
| `0x8008D1D8` | `SetName(const char*)` | 24 |
| `0x8008D204` | `MakeUID(const char*)` | 24 |
| `0x8008D224` | `GetEntityType()` | 0 |

### tFile : Stream [268 bytes]

*Source: C:\v11.3\SOURCE\TFILE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B7BA4` | `tFile(tByteStream*, int)` | 24 |
| `0x800B7C28` | `AttachStream(tByteStream*, int)` | 0 |
| `0x800B7C34` | `DetachStream()` | 24 |
| `0x800B7C90` | `Eof()` | 24 |
| `0x800B7CC8` | `GetBytes(void*, unsigned long)` | 24 |
| `0x800B7D00` | `GetWord()` | 32 |
| `0x800B7D3C` | `GetLong()` | 32 |
| `0x800B7D78` | `GetPString(char*)` | 32 |
| `0x800B7DD4` | `GetChar()` | 32 |
| `0x800B7E10` | `IsOpen()` | 24 |
| `0x800B7E50` | `SetPosition(long)` | 24 |
| `0x800B7E88` | `GetPosition()` | 24 |
| `0x800B7EC0` | `GetFileSize()` | 24 |

### tFlipbook : tPuppet [32 bytes]

*Source: C:\v11.3\SOURCE\ANIMATE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008D2EC` | `tFlipbook()` | 24 |
| `0x8008D354` | `SetFrame(int)` | 0 |
| `0x8008D35C` | `SetFrameReal(long)` | 24 |
| `0x8008D38C` | `UpdateReal()` | 24 |
| `0x8008D3BC` | `SetAnimation(tAnimation*)` | 0 |
| `0x8008D3C4` | `AdvanceFrame()` | 24 |

### tFrameList : tAnimation [24 bytes]

*Source: C:\v11.3\INCLUDE\VERTANIM.HPP, C:\v11.3\SOURCE\VERTANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009DF70` | `tFrameList()` | 24 |
| `0x8009DFCC` | `MakePuppet()` | 32 |
| `0x8009E02C` | `GetNumFrames()` | 0 |
| `0x8009E038` | `GetFrame(int)` | 0 |
| `0x8009E050` | `DeleteFrames()` | 24 |
| `0x8009E090` | `SetNumFrames(int)` | 0 |
| `0x8009E098` | `SetFrame(int, SVECTOR*)` | 0 |
| `0x8009E20C` | `GetEntityType()` | 0 |

### tGeoLoader [12 bytes]

*Source: C:\v11.3\SOURCE\TGEOLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008461C` | `tGeoLoader()` | 40 |
| `0x800846F0` | `Load(tReadChunk&, void**)` | 320 |

### tGeometry : tDrawable [64 bytes]

*Source: C:\v11.3\INCLUDE\TGEOMTRY.INL*

| Address | Method | Size |
|---------|--------|------|
| `0x800A18A0` | `Clone()` | 0 |
| `0x800A18A8` | `GetVertexList()` | 0 |
| `0x800A18B4` | `SetVertexList(SVECTOR*)` | 0 |
| `0x800A18BC` | `GetBoundingSphere()` | 0 |
| `0x800A18C4` | `GetBoundingBox()` | 0 |
| `0x800BB33C` | `GetVertexList()` | 0 |
| `0x800BB348` | `SetVertexList(SVECTOR*)` | 0 |
| `0x800BB350` | `GetBoundingSphere()` | 0 |
| `0x800BB358` | `GetBoundingBox()` | 0 |
| `0x800BB360` | `Clone()` | 0 |

### tIndexList : tEntity [32 bytes]

*Source: C:\v11.3\SOURCE\TIDXLIST.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BB99C` | `tIndexList()` | 24 |
| `0x800BBA3C` | `Insert(tEntity*)` | 32 |
| `0x800BBB24` | `Remove(unsigned short)` | 0 |
| `0x800BBBF4` | `Empty()` | 0 |
| `0x800BBC08` | `CreateStore(int)` | 32 |
| `0x800BBC88` | `DeleteStore()` | 24 |
| `0x800BBCD8` | `GrowBy(int)` | 40 |

### tInvCache : tCache [276 bytes]

*Source: C:\v11.3\SOURCE\TCACHE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BBDEC` | `tInvCache()` | 40 |
| `0x800BBE90` | `Flush()` | 32 |
| `0x800BBEF4` | `ResetElem(InvCacheElem*)` | 0 |
| `0x800BBF08` | `Insert(unsigned long, unsigned short)` | 0 |
| `0x800BBF98` | `Search(unsigned long)` | 0 |

### tInventory : tEntity [44 bytes]

*Source: C:\v11.3\SOURCE\TINVNTRY.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008E924` | `tInventory()` | 24 |
| `0x8008E9B0` | `ApplyFunction(unsigned short, tEntity*(*)()*, int)` | 40 |
| `0x8008EA50` | `FindHandle(unsigned short, unsigned long)` | 24 |
| `0x8008EA90` | `FindListHandle(const char*)` | 24 |
| `0x8008EACC` | `SearchList(tIndexList*, unsigned long)` | 0 |

### tJoint1DOF : tDynamicKeyList [24 bytes]

*Source: C:\v11.3\INCLUDE\KEYNDOF.HPP, C:\v11.3\SOURCE\KEYNDOF.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BC5A0` | `tJoint1DOF()` | 24 |
| `0x800BC630` | `GetValue(long, long*, tJointCache*, int)` | 40 |
| `0x800BC840` | `GetKeyType()` | 0 |

### tJoint1DOFangle : tDynamicKeyList [24 bytes]

*Source: C:\v11.3\INCLUDE\CHANNEL.HPP, C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AFC10` | `tJoint1DOFangle()` | 24 |
| `0x800AFC4C` | `tJoint1DOFangle(int)` | 24 |
| `0x800AFCD4` | `GetValue(long, long*, tJointCache*, int)` | 48 |
| `0x800B0BC8` | `GetKeyType()` | 0 |

### tJoint3DOF : tDynamicKeyList [20 bytes]

*Source: C:\v11.3\INCLUDE\KEYNDOF.HPP, C:\v11.3\SOURCE\KEYNDOF.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BC22C` | `tJoint3DOF()` | 24 |
| `0x800BC2B8` | `GetValue(long, long*, tJointCache*, int)` | 40 |
| `0x800BC848` | `GetKeyType()` | 0 |

### tJoint3DOFangle : tDynamicKeyList [20 bytes]

*Source: C:\v11.3\INCLUDE\CHANNEL.HPP, C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AF948` | `tJoint3DOFangle()` | 24 |
| `0x800AF980` | `tJoint3DOFangle(int)` | 24 |
| `0x800AFA08` | `GetValue(long, long*, tJointCache*, int)` | 48 |
| `0x800B0BD0` | `GetKeyType()` | 0 |

### tJoint3DOFlpPSX : tDynamicKeyList [20 bytes]

*Source: C:\v11.3\INCLUDE\CHANNEL.HPP, C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AFE80` | `tJoint3DOFlpPSX(int)` | 24 |
| `0x800AFF08` | `GetValue(long, long*, tJointCache*, int)` | 48 |
| `0x800B0BC0` | `GetKeyType()` | 0 |

### tLayer [48 bytes]

*Source: C:\v11.3\SOURCE\TLAYER.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A0288` | `tLayer(unsigned long, unsigned long)` | 32 |
| `0x800A0370` | `Draw()` | 24 |
| `0x800A039C` | `Flip()` | 0 |
| `0x800A03A4` | `FlipToDrawOT()` | 0 |
| `0x800A03AC` | `Check()` | 0 |
| `0x800A03B8` | `Start()` | 24 |
| `0x800A0428` | `End()` | 0 |
| `0x800A04C0` | `Free()` | 0 |
| `0x800A089C` | `ScaleOT()` | 32 |
| `0x800A0C3C` | `DumpOT()` | 24 |

### tLight : tEntity [24 bytes]

*Source: C:\v11.3\SOURCE\TLIGHT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C49AC` | `tLight(tLightHardwareSlot)` | 32 |
| `0x800C4A2C` | `SetColour(unsigned long)` | 24 |

### tLitFarTable [60 bytes]

*Source: C:\v11.3\SOURCE\LITFARD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B149C` | `tLitFarTable()` | 24 |
| `0x800B14D8` | `Install()` | 0 |

### tLitTable [60 bytes]

*Source: C:\v11.3\SOURCE\LITD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B5BD0` | `tLitTable()` | 24 |
| `0x800B5C0C` | `Install()` | 0 |

### tMTree : tDrawable [20 bytes]

*Source: C:\v11.3\SOURCE\MTREE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C4394` | `FindJoint(unsigned long)` | 0 |
| `0x800C43E0` | `Display()` | 40 |

### tMatLoader [12 bytes]

*Source: C:\v11.3\SOURCE\TMATLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800868C0` | `Load(tReadChunk&, void**)` | 312 |

### tMatrixCamera : tCamera [56 bytes]

*Source: C:\v11.3\SOURCE\TMATRIXCAM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009D5E8` | `tMatrixCamera(MATRIX*)` | 32 |
| `0x8009D670` | `SetCameraMatrix(MATRIX*)` | 24 |
| `0x8009D698` | `GetCameraMatrix()` | 0 |
| `0x8009D6A0` | `UpdateMatrix()` | 24 |
| `0x8009D6C8` | `GetPosition(_RMVECT16*)` | 0 |

### tMemByteStream : tByteStream [20 bytes]

*Source: C:\v11.3\INCLUDE\TFILE.HPP, C:\v11.3\SOURCE\TFILE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B7F2C` | `tMemByteStream(unsigned char*, unsigned long)` | 24 |
| `0x800B7FE8` | `GetLength()` | 0 |
| `0x800B7FF4` | `GetPosition()` | 0 |
| `0x800B8004` | `SetPosition(long)` | 0 |
| `0x800B801C` | `GetBytes(void*, unsigned long)` | 32 |
| `0x800B8068` | `Eof()` | 0 |
| `0x800B8084` | `IsOpen()` | 0 |
| `0x800B808C` | `GetMemPosition()` | 0 |

### tP3Dinventory : tInventory [428 bytes]

*Source: C:\v11.3\SOURCE\P3DINV.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80086B70` | `tP3Dinventory()` | 40 |
| `0x80086C8C` | `Init()` | 40 |
| `0x80086D68` | `DeleteAllListsID(unsigned short)` | 40 |
| `0x80086DFC` | `DeleteListID(unsigned short, unsigned short)` | 48 |
| `0x80086F24` | `DeleteList(unsigned short)` | 48 |
| `0x80087064` | `DeleteObject(unsigned short, unsigned short)` | 32 |
| `0x80087114` | `SetListSize(unsigned short, unsigned long)` | 32 |
| `0x8008716C` | `StoreObject(unsigned short, tEntity*, int)` | 24 |
| `0x800871D0` | `FindObjectHandle(unsigned short, unsigned long)` | 48 |
| `0x800872CC` | `Find(unsigned short, unsigned long)` | 32 |
| `0x8008733C` | `FindCache(unsigned short)` | 0 |

### tParamAnim : tAnimation [36 bytes]

*Source: C:\v11.3\INCLUDE\PARAMANIM.HPP, C:\v11.3\SOURCE\PARAMANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BC0CC` | `tParamAnim()` | 24 |
| `0x800BC1D0` | `SetParam(int, tKeyList**)` | 0 |
| `0x800BC1DC` | `MakePuppet()` | 24 |
| `0x800BC214` | `GetNumFrames()` | 0 |
| `0x800BC220` | `GetEntityType()` | 0 |

### tParamAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\PARAMLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008879C` | `Load(tReadChunk&, void**)` | 32 |

### tParamFlip : tFlipbook [48 bytes]

*Source: C:\v11.3\SOURCE\PARAMFLIP.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AF6D0` | `tParamFlip()` | 24 |
| `0x800AF76C` | `Reset()` | 24 |
| `0x800AF7C0` | `SetFrame(int)` | 0 |
| `0x800AF7D0` | `SetFrameReal(long)` | 0 |
| `0x800AF7E0` | `SetAnimation(tAnimation*)` | 0 |

### tPort

*Source: C:\v11.3\SOURCE\PORTMATH.CPP, C:\v11.3\SOURCE\TPORT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80051A98` | `SetView(tView*)` | 40 |
| `0x80051B9C` | `GetWorldMatrix(MATRIX*)` | 32 |
| `0x80051BF4` | `TransMatrix(_RMVECT16*)` | 24 |
| `0x80051C24` | `TransMatrix(long, long, long)` | 24 |
| `0x80051C44` | `ScaleMatrix(long, long, long)` | 24 |
| `0x80051C6C` | `ScaleMatrix(_RMVECT16*)` | 24 |
| `0x80051C94` | `ScaleMatrix(long)` | 24 |
| `0x80051CBC` | `SyncCamView()` | 112 |
| `0x80052024` | `CVMToCTM()` | 24 |
| `0x80052058` | `SwapBuffers()` | 24 |
| `0x8005207C` | `FrameInit()` | 0 |
| `0x800520C0` | `SetLighting(unsigned long)` | 0 |
| `0x800539B4` | `TransformVector(_RMVECT16*, _RMVECT16*)` | 32 |
| `0x80053A78` | `ProjectVector(_RMVECT16*)` | 32 |
| `0x800CA4B4` | `Init(tPortInitData*)` | 48 |

### tPose [12 bytes]

*Source: C:\v11.3\SOURCE\POSE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B0D08` | `tPose()` | 0 |
| `0x800B0D80` | `Init(int)` | 32 |

### tPrimGeom : tGeometry [108 bytes]

*Source: C:\v11.3\INCLUDE\TPRIMGEO.HPP, C:\v11.3\SOURCE\TPRIMGEO.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800A1548` | `Clone()` | 40 |
| `0x800A1860` | `Display()` | 24 |
| `0x800A1888` | `GetGeoType()` | 0 |
| `0x800A1894` | `GetEntityType()` | 0 |

### tPrimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\TPRMLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80088A80` | `Load(tReadChunk&, void**)` | 40 |

### tPuppet : tEntity [20 bytes]

*Source: C:\v11.3\SOURCE\ANIMATE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008D288` | `tPuppet()` | 24 |

### tReadChunk [20 bytes]

*Source: C:\v11.3\SOURCE\TCHUNK.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B12D4` | `tReadChunk(tFile*)` | 32 |
| `0x800B134C` | `GetDataSize()` | 0 |
| `0x800B1358` | `Read()` | 24 |
| `0x800B13B4` | `Skip()` | 24 |
| `0x800B13F0` | `ReadNext()` | 24 |
| `0x800B1448` | `EndOfChunk()` | 24 |

### tSTree : tTree [40 bytes]

*Source: C:\v11.3\INCLUDE\STREE.HPP, C:\v11.3\SOURCE\STREE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8008499C` | `tSTree()` | 24 |
| `0x800849F4` | `DeepCopy()` | 40 |
| `0x80084B44` | `SetNumJoints(int)` | 32 |
| `0x80084C08` | `Display()` | 64 |
| `0x800859F8` | `GetJointAbsolute(int)` | 0 |
| `0x80085A10` | `GetJoint(int)` | 0 |

### tSTreeLoader [12 bytes]

*Source: C:\v11.3\SOURCE\STLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80088F54` | `tSTreeLoader()` | 0 |
| `0x80088F80` | `LoadInternal(tReadChunk&, void**)` | 600 |
| `0x8008920C` | `Load(tReadChunk&, void**)` | 32 |

### tSTreeUnLit [40 bytes]

*Source: C:\v11.3\SOURCE\STREEUNLIT.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BC850` | `DeepCopy()` | 40 |
| `0x800BC9D0` | `Display()` | 72 |

### tSequenceAnim : tAnimation [20 bytes]

*Source: C:\v11.3\INCLUDE\SEQUENCE.HPP, C:\v11.3\SOURCE\SEQUENCE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80087B68` | `tSequenceAnim()` | 24 |
| `0x80087BE4` | `DeleteAll()` | 32 |
| `0x80087C64` | `MakePuppet()` | 32 |
| `0x80087CD4` | `GetPart(int)` | 0 |
| `0x80087E28` | `GetNumFrames()` | 0 |
| `0x80087E34` | `GetEntityType()` | 0 |

### tSequenceAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\SEQUENCE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80087A44` | `Load(tReadChunk&, void**)` | 48 |

### tSequenceFlip [32 bytes]

*Source: C:\v11.3\INCLUDE\SEQUENCE.HPP, C:\v11.3\SOURCE\SEQUENCE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80087CE8` | `tSequenceFlip()` | 24 |
| `0x80087D44` | `Reset()` | 24 |
| `0x80087D94` | `Update()` | 32 |
| `0x80087E1C` | `GetEntityType()` | 0 |

### tStatic3DOFKeyList : tStaticKeyList [20 bytes]

*Source: C:\v11.3\INCLUDE\CHANNEL.HPP, C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800AF7F0` | `GetValue(long, long*, tJointCache*, int)` | 0 |
| `0x800B0BD8` | `GetKeyType()` | 0 |

### tTexAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\TTXTLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800898C8` | `Load(tReadChunk&, void**)` | 456 |

### tTexFlip : tFlipbook [40 bytes]

*Source: C:\v11.3\INCLUDE\TEXANIM.HPP, C:\v11.3\SOURCE\TEXANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BCDA8` | `tTexFlip()` | 24 |
| `0x800BCE0C` | `Reset()` | 24 |
| `0x800BCE5C` | `Update()` | 32 |
| `0x800BCF44` | `GetTexFrame(int)` | 24 |
| `0x800BD244` | `GetEntityType()` | 0 |

### tTexList : tAnimation [40 bytes]

*Source: C:\v11.3\INCLUDE\TEXANIM.HPP, C:\v11.3\SOURCE\TEXANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BCF7C` | `tTexList()` | 24 |
| `0x800BD038` | `MakePuppet()` | 32 |
| `0x800BD0A0` | `DeleteFrames()` | 24 |
| `0x800BD0E0` | `SetNumFrames(int)` | 24 |
| `0x800BD140` | `SetNumOffsets(unsigned short)` | 24 |
| `0x800BD1A4` | `SetOffset(unsigned short, unsigned short)` | 0 |
| `0x800BD1BC` | `GetTexFrame(int)` | 0 |
| `0x800BD1F8` | `SetTexFrame(int, unsigned short)` | 0 |
| `0x800BD22C` | `GetNumFrames()` | 0 |
| `0x800BD238` | `GetEntityType()` | 0 |

### tTexture : tEntity [24 bytes]

*Source: C:\v11.3\SOURCE\TTEXTURE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BC024` | `tTexture()` | 24 |
| `0x800BC0B0` | `GetTextureData()` | 0 |
| `0x800BC0BC` | `SetTextureData(unsigned long*)` | 0 |
| `0x800BC0C4` | `GetTextureRect()` | 0 |

### tTranAnimLoader2 [12 bytes]

*Source: C:\v11.3\SOURCE\TRANLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x80087EA8` | `Load(tReadChunk&, void**)` | 296 |

### tTransformAnim : tAnimation [40 bytes]

*Source: C:\v11.3\INCLUDE\CHANNEL.HPP, C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B0134` | `tTransformAnim(int)` | 24 |
| `0x800B0290` | `MakePuppet()` | 32 |
| `0x800B0BA8` | `GetNumFrames()` | 0 |
| `0x800B0BB4` | `GetEntityType()` | 0 |

### tTransformFlip2 : tFlipbook [68 bytes]

*Source: C:\v11.3\INCLUDE\CHANNEL.HPP, C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B0398` | `tTransformFlip2()` | 24 |
| `0x800B0470` | `SetFrame(int)` | 0 |
| `0x800B0480` | `SetFrameReal(long)` | 0 |
| `0x800B0490` | `Reset()` | 24 |
| `0x800B04E4` | `Update()` | 24 |
| `0x800B0514` | `Update(tTree*)` | 24 |
| `0x800B0578` | `UpdateJoints(tTree*)` | 80 |
| `0x800B0748` | `UpdateJointsMirrored(tTree*)` | 80 |
| `0x800B0A70` | `SetAnimation(tAnimation*)` | 0 |
| `0x800B0B9C` | `GetEntityType()` | 0 |

### tTree : tDrawable [24 bytes]

*Source: C:\v11.3\SOURCE\TREE.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800BB390` | `FindJoint(unsigned long)` | 32 |
| `0x800BB414` | `FindJointIndex(unsigned long)` | 32 |

### tTreeFlip [68 bytes]

*Source: C:\v11.3\INCLUDE\CHANNEL.HPP, C:\v11.3\SOURCE\CHANNEL.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B0A80` | `Attach(tTree*, tTransformAnim*)` | 32 |
| `0x800B0B5C` | `GetEntityType()` | 0 |

### tUVAnim : tAnimation [40 bytes]

*Source: C:\v11.3\INCLUDE\UVANIM.HPP, C:\v11.3\SOURCE\UVANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C1FC0` | `tUVAnim()` | 24 |
| `0x800C2078` | `Init(int, int, int, unsigned short*, unsigned long*)` | 40 |
| `0x800C210C` | `GetUVFrame(int, int)` | 0 |
| `0x800C2138` | `MakePuppet()` | 32 |
| `0x800C2608` | `GetNumFrames()` | 0 |
| `0x800C2614` | `GetEntityType()` | 0 |

### tUVAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\UVANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C233C` | `Load(tReadChunk&, void**)` | 152 |

### tUVFlip : tFlipbook [36 bytes]

*Source: C:\v11.3\INCLUDE\UVANIM.HPP, C:\v11.3\SOURCE\UVANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C21AC` | `tUVFlip()` | 24 |
| `0x800C2208` | `Reset()` | 24 |
| `0x800C2238` | `Update()` | 48 |
| `0x800C25FC` | `GetEntityType()` | 0 |

### tVertAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\TVRTLOAD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C1DC4` | `Load(tReadChunk&, void**)` | 304 |

### tVertexFlip : tFlipbook [36 bytes]

*Source: C:\v11.3\INCLUDE\VERTANIM.HPP, C:\v11.3\SOURCE\VERTANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x8009E0AC` | `tVertexFlip()` | 24 |
| `0x8009E10C` | `Reset()` | 24 |
| `0x8009E15C` | `Update()` | 24 |
| `0x8009E1C8` | `GetVertexFrame(int)` | 24 |
| `0x8009E200` | `GetEntityType()` | 0 |

### tView : tEntity [216 bytes]

*Source: C:\v11.3\SOURCE\TVIEW.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800521C0` | `tView(unsigned long)` | 32 |
| `0x8005233C` | `SetCamera(tCamera*)` | 0 |
| `0x80052344` | `SetFog(tFog*)` | 0 |
| `0x8005234C` | `SetViewPort(long, long, long, long)` | 0 |
| `0x80052364` | `SetAmbientLight(unsigned long)` | 0 |
| `0x8005236C` | `SetBackgroundColour(unsigned long)` | 0 |
| `0x80052374` | `GetState()` | 0 |
| `0x8005237C` | `AddLight(tLight*)` | 0 |
| `0x800523CC` | `RemoveLight(unsigned char)` | 0 |
| `0x800523F0` | `BeginRender()` | 40 |
| `0x80052490` | `EndRender()` | 0 |
| `0x800524A0` | `SetupLayer(unsigned long, unsigned long, unsigned long, unsigned long)` | 40 |
| `0x80052580` | `SetLayer(unsigned long)` | 0 |
| `0x80052598` | `DeleteLayer(unsigned long)` | 32 |
| `0x80052604` | `CheckLayer(unsigned long)` | 24 |
| `0x80052648` | `StartLayer(unsigned long)` | 24 |
| `0x8005268C` | `EndLayer(unsigned long)` | 24 |
| `0x800526D0` | `EnterLayer(unsigned long)` | 32 |
| `0x80052720` | `ExitLayer(unsigned long)` | 24 |
| `0x80052754` | `SetupViewPort()` | 24 |
| `0x80052780` | `ClearViewPort()` | 128 |
| `0x800528A0` | `ClipView()` | 40 |

### tVizAnim : tAnimation [24 bytes]

*Source: C:\v11.3\INCLUDE\VIZANIM.HPP, C:\v11.3\SOURCE\VIZANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C409C` | `tVizAnim()` | 24 |
| `0x800C4130` | `MakePuppet()` | 32 |
| `0x800C41A8` | `Init(int, int, tVizNode*)` | 0 |
| `0x800C4314` | `GetNumFrames()` | 0 |
| `0x800C4320` | `GetEntityType()` | 0 |

### tVizAnimLoader [12 bytes]

*Source: C:\v11.3\SOURCE\VIZANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C3FF8` | `Load(tReadChunk&, void**)` | 32 |

### tVizFlip [32 bytes]

*Source: C:\v11.3\SOURCE\VIZANIM.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800C41B8` | `Reset()` | 24 |
| `0x800C4208` | `Update()` | 40 |
| `0x800C4278` | `SetViz(tVizNode*)` | 0 |

### tZFarTable [60 bytes]

*Source: C:\v11.3\SOURCE\ZFARD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B3688` | `tZFarTable()` | 24 |
| `0x800B36C4` | `Install()` | 0 |

### tZSortTable [60 bytes]

*Source: C:\v11.3\SOURCE\ZSORTD.CPP*

| Address | Method | Size |
|---------|--------|------|
| `0x800B4A64` | `tZSortTable()` | 24 |
| `0x800B4AA0` | `Install()` | 0 |

---

## Free Functions

Functions not belonging to any class.

#### ACTIVEZN.CPP (C:\CHAN\GAME\SRC\AI\ACTIVEZN.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A6A48` | `_._13SubZoneVolume` | 24 |
| `0x800A6D00` | `_._10ActiveZone` | 32 |

#### AI.CPP (C:\CHAN\GAME\SRC\AI\AI.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800540E0` | `aiPrivHandler(Handler*)` | 24 |
| `0x800542C4` | `_._2AI` | 32 |
| `0x800554D0` | `HandleHumanoidHumanoidCollision(void)` | 48 |
| `0x80055584` | `HandleHumanoidHumanoidCollision(Humanoid*, Humanoid*)` | 120 |
| `0x80055C30` | `KillThingsInList(ccList&, long)` | 32 |
| `0x800569E4` | `PopulateBlockHelper(ccList&)` | 24 |
| `0x80056A74` | `UnpopulateBlockHelper(ccList&)` | 24 |

#### ARROW.CPP (C:\CHAN\GAME\SRC\AI\ARROW.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001B6C0` | `_._5Arrow` | 24 |

#### BEHAVE.CPP (C:\CHAN\GAME\SRC\AI\BEHAVE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800744C0` | `_._15BehaviourAttrib` | 32 |
| `0x800746B4` | `_._9Behaviour` | 24 |

#### BEHAVEB.CPP (C:\CHAN\GAME\SRC\AI\BEHAVEB.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001E05C` | `wallCheck(Humanoid*, long)` | 96 |

#### BLAST.CPP (C:\CHAN\GAME\SRC\AI\BLAST.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80015AF8` | `_._5Blast` | 32 |

#### BOSS.CPP (C:\CHAN\GAME\SRC\AI\BOSS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001A79C` | `_._4Boss` | 24 |
| `0x8001ABD0` | `_._5Butch` | 24 |
| `0x8001B118` | `_._7Grontar` | 24 |
| `0x8001B718` | `_._5Dante` | 32 |
| `0x8001C7D8` | `_._4Paul` | 24 |
| `0x8001C964` | `_._5Oscar` | 24 |

#### COLLECT.CPP (C:\CHAN\GAME\SRC\AI\COLLECT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800127A4` | `_._11Collectible` | 24 |

#### COMINTER.CPP (C:\CHAN\GAME\SRC\AI\COMINTER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B0E18` | `FindActionRequest(unsigned long&, unsigned long, unsigned long, const Control*)` | 96 |

#### CONVEYOR.CPP (C:\CHAN\GAME\SRC\AI\CONVEYOR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001BE98` | `_._8Conveyor` | 32 |

#### CRUSHER.CPP (C:\CHAN\GAME\SRC\AI\CRUSHER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001F6F8` | `_._7Crusher` | 32 |

#### DESTROY.CPP (C:\CHAN\GAME\SRC\AI\DESTROY.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80010308` | `_._17DestructibleThing` | 32 |

#### DOOR.CPP (C:\CHAN\GAME\SRC\AI\DOOR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001AB8C` | `_._4Door` | 24 |

#### EXPLODE.CPP (C:\CHAN\GAME\SRC\AI\EXPLODE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800130C0` | `_._9Explosive` | 24 |

#### FEVOLUME.CPP (C:\CHAN\GAME\SRC\AI\FEVOLUME.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001A79C` | `_._14FrontEndVolume` | 24 |

#### FIGHTANI.CPP (C:\CHAN\GAME\SRC\AI\FIGHTANI.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8007DBE0` | `FindFightingSystem(unsigned long)` | 0 |
| `0x8007DC34` | `FindBossFightingSystem(unsigned long)` | 0 |
| `0x8007DC8C` | `FindTypeFightingSystem(unsigned short, TypeFightingSystem*, unsigned long)` | 24 |
| `0x8007DD20` | `GetFightingSystem(unsigned short)` | 24 |
| `0x8007DD5C` | `GetPickupFighting(unsigned short)` | 24 |
| `0x8007DD88` | `GetPickupFightingHighPickup(unsigned short)` | 0 |
| `0x8007DDDC` | `GetPickupFightingLowPickup(unsigned short)` | 0 |
| `0x8007DE30` | `GetPickupFightingThrow(unsigned short)` | 0 |
| `0x8007DE84` | `GetPickupFightingIdle(unsigned short)` | 0 |
| `0x8007DED4` | `GetrelativeAngle(long, long)` | 0 |

#### GENERATOR.CPP (C:\CHAN\GAME\SRC\AI\GENERATOR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80010DE8` | `_._9Generator` | 32 |

#### HPOLE.CPP (C:\CHAN\GAME\SRC\AI\HPOLE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800154D4` | `_._14HorizontalPole` | 24 |

#### HUMANOID.CPP (C:\CHAN\GAME\SRC\AI\HUMANOID.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80062C58` | `_._8Humanoid` | 40 |
| `0x80065420` | `GetWeaponTransitionIdle(Pickup*)` | 0 |
| `0x80066C4C` | `ReturnMostSignificant32BitNumber(unsigned long)` | 0 |
| `0x800675C0` | `ClipAngle360(long)` | 0 |

#### HUMNDATA.CPP (C:\CHAN\GAME\SRC\AI\HUMNDATA.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8007D980` | `GetEnumFromHashTable(Hash_Enum*, unsigned long, long)` | 0 |
| `0x8007D9B8` | `GetPreActiveIdle(long)` | 24 |
| `0x8007D9F8` | `GetTauntAnim(long)` | 24 |
| `0x8007DA38` | `GetCharSubTypeEnumFromHashID(long)` | 24 |
| `0x8007DA78` | `CharSubTypeDataTableElement(Q28Humanoid17CharacterSubTypes)` | 0 |
| `0x8007DAC4` | `GetCharSubTypeScale(Q28Humanoid17CharacterSubTypes)` | 24 |
| `0x8007DAF8` | `GetCharSubTypeClut(Q28Humanoid17CharacterSubTypes)` | 24 |
| `0x8007DB2C` | `GetCharSubTypeHitPoints(Q28Humanoid17CharacterSubTypes)` | 24 |
| `0x8007DB60` | `GetBehaviourNameHash(Q28Humanoid17CharacterSubTypes)` | 24 |
| `0x8007DB94` | `GetHumanoidData(Q22AI10ThingTypes)` | 0 |

#### KICK.CPP (C:\CHAN\GAME\SRC\AI\KICK.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001C3DC` | `_._9KickNRoll` | 32 |

#### KNOCKDWN.CPP (C:\CHAN\GAME\SRC\AI\KNOCKDWN.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001D650` | `_._9KnockDown` | 32 |
| `0x8001E7B8` | `_._5Stack` | 32 |
| `0x8001F5A0` | `StackEJointCallback(tEJoint*, int)` | 104 |

#### LADDER.CPP (C:\CHAN\GAME\SRC\AI\LADDER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80089D18` | `_._6Ladder` | 24 |

#### LAUNCHER.CPP (C:\CHAN\GAME\SRC\AI\LAUNCHER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001FE7C` | `_._8Launcher` | 24 |

#### OBSTACLE.CPP (C:\CHAN\GAME\SRC\AI\OBSTACLE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8007A740` | `CheckXZStaticBoxCylinderCollision(const tagLVector&, const tagCollisionBox&, long, long, const tagCollisionCylinder&)` | 120 |
| `0x8007A970` | `GetXZStaticBoxCylinderCollisionSortDistance(const tagLVector&, const tagCollisionBox&, long, long)` | 112 |
| `0x8007AB90` | `CheckStaticBoxCylinderCollision_Obstacle(const tagLVector&, const tagCollisionBox&, long, long, const tagCollisionCylinder&)` | 128 |
| `0x8007AFD0` | `IncludeNumberInRange(short&, short, short)` | 0 |
| `0x8007B010` | `IncludeVertexInBox(tagCollisionBox&, const SVECTOR&)` | 32 |
| `0x8007CA7C` | `_._8Obstacle` | 24 |
| `0x8007D078` | `YRotate(tagLVector&, long, const tagLVector&)` | 40 |
| `0x8007D50C` | `static_destroy(PICKUP_OBSTACLE_CHECK_COUNT)` | 24 |
| `0x8007D5E0` | `static_init(PICKUP_OBSTACLE_CHECK_COUNT)` | 32 |

#### PENDULUM.CPP (C:\CHAN\GAME\SRC\AI\PENDULUM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80024E38` | `_._8Pendulum` | 32 |

#### PICKUP.CPP (C:\CHAN\GAME\SRC\AI\PICKUP.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8006D208` | `SetDefaultCollisionPoint(const DBRoot&, int, tagLVector&, int)` | 56 |
| `0x8006D3F4` | `GetMoveStruct(unsigned short)` | 0 |
| `0x8006D5B0` | `_._6Pickup` | 32 |

#### PLATFORM.CPP (C:\CHAN\GAME\SRC\AI\PLATFORM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800208E4` | `_._8Platform` | 32 |

#### PLAYER.CPP (C:\CHAN\GAME\SRC\AI\PLAYER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002FBC0` | `_._6Player` | 32 |
| `0x80030388` | `GetWeaponPickupDialog(long)` | 0 |
| `0x80032348` | `CalculateFallDamage(long)` | 0 |
| `0x80034338` | `GetWeaponFinalBlowDialog(long)` | 0 |

#### PUSHABLE.CPP (C:\CHAN\GAME\SRC\AI\PUSHABLE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80017D08` | `_._8Pushable` | 32 |

#### SLIPPERY.CPP (C:\CHAN\GAME\SRC\AI\SLIPPERY.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80012304` | `_._13SlipperyFloor` | 32 |

#### TABLE.CPP (C:\CHAN\GAME\SRC\AI\TABLE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80013FD0` | `_._15DynamicObstacle` | 24 |
| `0x800151C8` | `_._5Table` | 24 |
| `0x80015354` | `_._5Chair` | 24 |

#### TELEPORT.CPP (C:\CHAN\GAME\SRC\AI\TELEPORT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AA4D8` | `_._10Teleporter` | 24 |

#### THING.CPP (C:\CHAN\GAME\SRC\AI\THING.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80061640` | `_._5Thing` | 32 |
| `0x80061DE0` | `_._12DynamicThing` | 32 |
| `0x80062844` | `_._6Ticket` | 24 |

#### TRAPDOOR.CPP (C:\CHAN\GAME\SRC\AI\TRAPDOOR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001702C` | `_._8TrapDoor` | 24 |

#### TRIGGER.CPP (C:\CHAN\GAME\SRC\AI\TRIGGER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A999C` | `_._12TriggerThing` | 32 |

#### UNTOUCH.CPP (C:\CHAN\GAME\SRC\AI\UNTOUCH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A6380` | `_._11Untouchable` | 32 |

#### FEMNUMGR.CPP (C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80010F04` | `_._9feMenuMgr` | 24 |
| `0x80011C38` | `_._13hdMemCardMenu` | 32 |

#### GAMEMENU.CPP (C:\CHAN\GAME\SRC\FE\GAMEMENU.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8003789C` | `_SetControllerShock(hdMenuItem*)` | 32 |
| `0x80037CE4` | `_._8gameMenu` | 24 |

#### GAMESTOR.CPP (C:\CHAN\GAME\SRC\FE\GAMESTOR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800149F0` | `static_init(gGameStorage)` | 0 |

#### HDITEM.CPP (C:\CHAN\GAME\SRC\FE\HDITEM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008ED30` | `_._8hdHealth` | 24 |
| `0x8008F2C4` | `_._9hdTextOvl` | 24 |
| `0x8008FD80` | `_._6hdHits` | 24 |
| `0x8008FE20` | `_._7hdTally` | 32 |

#### HDMENU.CPP (C:\CHAN\GAME\SRC\FE\HDMENU.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8005CB4C` | `MenuColorStart(xcColour1555&)` | 32 |
| `0x8005CC44` | `CalcNextColor(xcColour1555&)` | 8 |
| `0x8005CD10` | `MenuColorNext(xcColour1555&)` | 24 |
| `0x8005CDEC` | `_._6hdMenu` | 32 |
| `0x8005DD7C` | `_._10hdItemGoto` | 24 |
| `0x8005DE70` | `_._10hdMenuItem` | 24 |
| `0x8005E350` | `static_init(gMenuR0)` | 24 |

#### LINEFILE.CPP (C:\CHAN\GAME\SRC\FE\LINEFILE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80017F68` | `_._8LineFile` | 32 |

#### LOADANIM.CPP (C:\CHAN\GAME\SRC\FE\LOADANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800476B8` | `_._10VBlankLogo` | 32 |
| `0x80047AB0` | `static_init(_10VBlankLogo.Active)` | 24 |

#### MENUMGR.CPP (C:\CHAN\GAME\SRC\FE\MENUMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8005F50C` | `_._7MenuMgr` | 32 |

#### OXOVL.CPP (C:\CHAN\GAME\SRC\FE\OXOVL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80090EF4` | `GetPrimPosA(xcPrimObj*, short&, xcPrimObj*)` | 0 |
| `0x80090F6C` | `SetPrimPosA(xcPrimObj*, short, short)` | 0 |
| `0x80091064` | `_._5oxOvl` | 24 |

#### OXSCREEN.CPP (C:\CHAN\GAME\SRC\FE\OXSCREEN.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80091198` | `_._8oxScreen` | 24 |

#### OXSCRMGR.CPP (C:\CHAN\GAME\SRC\FE\OXSCRMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80040458` | `_._15oxScreenManager` | 32 |

#### ANIMMAT.CPP (C:\CHAN\GAME\SRC\GEN\ANIMMAT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80078464` | `HeadTrack(Humanoid*, tSJoint*)` | 40 |
| `0x8007857C` | `AM_HeadCallback(tSJoint*, int)` | 32 |
| `0x800785D8` | `AM_LHandCallback(tSJoint*, int)` | 24 |
| `0x8007860C` | `AM_RHandCallback(tSJoint*, int)` | 24 |
| `0x80078640` | `AM_LFootCallback(tSJoint*, int)` | 24 |
| `0x80078674` | `AM_RFootCallback(tSJoint*, int)` | 24 |
| `0x800786A8` | `AM_PelvisCallback(tSJoint*, int)` | 24 |
| `0x800786DC` | `AM_LUpperArm(tSJoint*, int)` | 24 |
| `0x8007871C` | `AM_RUpperArm(tSJoint*, int)` | 24 |
| `0x8007875C` | `AM_LThigh(tSJoint*, int)` | 24 |
| `0x8007879C` | `AM_RThigh(tSJoint*, int)` | 24 |
| `0x800787DC` | `AM_Bip_O_One_Callback(tSJoint*, int)` | 24 |

#### ANIMMGR.CPP (C:\CHAN\GAME\SRC\GEN\ANIMMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80057174` | `_._16AnimationManager` | 32 |

#### BLKMGR.CPP (C:\CHAN\GAME\SRC\GEN\BLKMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004FF1C` | `_._12BlockManager` | 32 |

#### BLOCK.CPP (C:\CHAN\GAME\SRC\GEN\BLOCK.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80052B88` | `_._5Block` | 24 |

#### CAMERA.CPP (C:\CHAN\GAME\SRC\GEN\CAMERA.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80047B40` | `_._6Camera` | 32 |
| `0x8004A29C` | `static_destroy(splatClosestDistance)` | 24 |
| `0x8004A2D4` | `static_init(splatClosestDistance)` | 24 |
| `0x8004A368` | `_._17AsyncAnimCallback` | 24 |

#### CAMMGR.CPP (C:\CHAN\GAME\SRC\GEN\CAMMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004A580` | `_._13CameraManager` | 24 |
| `0x8004A5B0` | `cameraLoadFunc(Callback*)` | 24 |
| `0x8004A7F8` | `_._12CameraAnchor` | 32 |
| `0x8004ABE8` | `_._12DBCameraPath` | 32 |
| `0x8004B4B4` | `_._16DBCameraPathNode` | 24 |

#### CCFILE.CPP (C:\CHAN\GAME\SRC\GEN\CCFILE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004C068` | `_._6ccFile` | 32 |

#### CCLIST.CPP (C:\CHAN\GAME\SRC\GEN\CCLIST.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80037324` | `_._9ccMinNode` | 24 |
| `0x8003739C` | `_._6ccNode` | 32 |
| `0x800377FC` | `CheckPriReverse(ccNode*, ccNode*)` | 0 |

#### CHARMGR.CPP (C:\CHAN\GAME\SRC\GEN\CHARMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800395F8` | `FreeAnimMemory(void*)` | 24 |
| `0x80039624` | `GetCompositeAnimationNameHash(const char*)` | 64 |
| `0x800396BC` | `GetPlayerMeshType(void)` | 0 |
| `0x80039794` | `_._16CharacterManager` | 24 |
| `0x80039AE4` | `DeleteAndRemoveCompositeAnimation(tCompositeAnim*)` | 48 |
| `0x8003A3B0` | `CallbackHelper(_RTASK*)` | 24 |
| `0x8003B2C4` | `P3DLoadCallback(tEntity*)` | 0 |
| `0x8003B2D8` | `P3DLoadCallbackParam(tEntity*)` | 0 |
| `0x8003B798` | `_._8CharFile` | 32 |
| `0x8003BA4C` | `_._12AnimCallback` | 24 |

#### CMNEFFCT.CPP (C:\CHAN\GAME\SRC\GEN\CMNEFFCT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004DA80` | `DrawQuickReminder(void)` | 0 |
| `0x8004DA88` | `DrawQuickReminder2(void)` | 0 |
| `0x8004DB9C` | `checkForAndFreeSequenceAnims(tAnimation*)` | 40 |
| `0x8004DC90` | `_._9ComEffect` | 32 |

#### COLFIGHT.CPP (C:\CHAN\GAME\SRC\GEN\COLFIGHT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800729CC` | `CheckAttack(const Humanoid*, const Humanoid*, const FightingCollisionAttackType*)` | 24 |
| `0x800729EC` | `CheckAttackCylinder(const Humanoid*, const Humanoid*, const FightingCollisionAttackType*)` | 136 |
| `0x80072CE4` | `CheckAttackCylinder(const Humanoid*, const Humanoid*, const tagLVector&, const Humanoid*, long)` | 120 |

#### COLMGR.CPP (C:\CHAN\GAME\SRC\GEN\COLMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A7B80` | `ExtendRange(long&, long, long)` | 0 |
| `0x800A7BBC` | `HTW_FillWallArray(long, long, long)` | 136 |
| `0x800A7E38` | `HTW_HandleWallCollisions(DynamicThing*, long, long, long)` | 72 |
| `0x800A7FAC` | `HTW_HandleHandFootCollisions(DynamicThing*)` | 136 |
| `0x800A8290` | `HandleThingWall(DynamicThing*, long, long, long)` | 152 |
| `0x800A8614` | `HandleThingFloor(DynamicThing*, long, long, long)` | 408 |
| `0x800A9284` | `ClearThingFloorHeights(ccList&)` | 24 |
| `0x800A92C4` | `HandleThingEnvironmentCollisions(ccList&)` | 56 |
| `0x800A96EC` | `HandleHumanoidObstacleCollisions(ccList&)` | 24 |
| `0x800A9740` | `HandlePickupObstacleCollisions(ccList&)` | 24 |
| `0x800A9794` | `HandleHumanoidPickupCollisions(ccList&, ccList&)` | 72 |

#### COLPHYS.CPP (C:\CHAN\GAME\SRC\GEN\COLPHYS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C4AC4` | `CorrectCollision(const tagLVector&, const tagLVector&, long, G9_RMVECT16R10tagLVectorT4)` | 40 |

#### COLSECT.CPP (C:\CHAN\GAME\SRC\GEN\COLSECT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80040BC4` | `MakeBox(tagLVector&, tagLVector&, const tagLVector&, tagLVector&, long, long, long)` | 0 |
| `0x80042454` | `DrawCopy(_RMVECT16&, const tagLVector&)` | 0 |
| `0x80042478` | `MyDrawQuad(const tagLVector&, N30RC9_RMVECT16)` | 104 |
| `0x80042554` | `DrawCollisionWall(const Wall&, const _RMVECT16&)` | 112 |
| `0x800425C8` | `DrawCollisionFloor(const Floor&)` | 112 |
| `0x800427AC` | `static_init(COLLISION_SECTOR_INDEX_X_MIN)` | 32 |

#### COLVOL.CPP (C:\CHAN\GAME\SRC\GEN\COLVOL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A9D60` | `InvertCollisionBox(tagCollisionBox&)` | 0 |
| `0x800A9D8C` | `FillCollisionBox(tagCollisionBox&, const tagLVector&, const tagLVector*, unsigned long)` | 16 |
| `0x800A9F38` | `FillCollisionBox(tagCollisionBox&, const DBVolume&)` | 0 |
| `0x800A9FB4` | `FillCollisionBox(tagCollisionBox&, const OriginalGeo&)` | 32 |
| `0x800AA05C` | `SetCollisionBoxExtent(tagCollisionBox&)` | 0 |
| `0x800AA0D4` | `CheckStaticHorizontalBoxPointCollision(const tagLVector&, const tagCollisionBox&, long, long)` | 72 |
| `0x800AA22C` | `CheckStaticBoxSphereCollision(const tagLVector&, const tagCollisionBox&, long, long, const tagCollisionSphere&)` | 88 |
| `0x800AA3E0` | `CheckStaticCylinderSphereCollision(const tagLVector&, const tagCollisionCylinder&, const tagCollisionCylinder&, const tagCollisionSphere&)` | 48 |

#### CONTROL.CPP (C:\CHAN\GAME\SRC\GEN\CONTROL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002D52C` | `ClearActuator(void)` | 0 |
| `0x8002D540` | `SetActuator(unsigned char, unsigned char, unsigned int)` | 0 |
| `0x8002D564` | `UpdateActuator(int)` | 24 |
| `0x8002D588` | `StepActuator(void)` | 24 |
| `0x8002D5F4` | `InitController(int)` | 24 |
| `0x8002D650` | `GetShock(void)` | 0 |
| `0x8002D670` | `SetShock(int)` | 24 |
| `0x8002D6B4` | `IsDualShock(void)` | 0 |
| `0x8002D6C0` | `Shock(ShockEnum)` | 24 |
| `0x8002D830` | `ReadSonyPads(void)` | 0 |
| `0x8002D90C` | `_._6Button` | 24 |
| `0x8002DBEC` | `_._7Control` | 40 |
| `0x8002DF50` | `inputPrivHandler(Handler*)` | 24 |
| `0x8002E048` | `_._12InputManager` | 40 |
| `0x8002E81C` | `static_init(ACTUATOR_DURATION)` | 24 |

#### DATABASE.CPP (C:\CHAN\GAME\SRC\GEN\DATABASE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800380D0` | `(nw__FUiPPv)` | 24 |
| `0x8003812C` | `(void, n__FUiPPv)` | 24 |
| `0x8003819C` | `_._8DBAttrib` | 32 |
| `0x80038A30` | `_._8Database` | 32 |
| `0x80039290` | `static_destroy(__nw__FUiPPv)` | 24 |
| `0x800392D0` | `static_init(__nw__FUiPPv)` | 0 |

#### DEADPOOL.CPP (C:\CHAN\GAME\SRC\GEN\DEADPOOL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008D47C` | `static_destroy(theDeadPool)` | 16 |
| `0x8008D4AC` | `static_init(theDeadPool)` | 0 |

#### DIRECTOR.CPP (C:\CHAN\GAME\SRC\GEN\DIRECTOR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8003BB0C` | `runDirector(Handler*)` | 24 |
| `0x8003BB34` | `DrawDirectorOverlays(Handler*)` | 24 |
| `0x8003BE10` | `_._8Director` | 40 |
| `0x8003F138` | `setJackieCheckpoint(unsigned long)` | 24 |
| `0x8003F184` | `checkPoint(tagLVector&, long)` | 32 |

#### DISPLAY.CPP (C:\CHAN\GAME\SRC\GEN\DISPLAY.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004D86C` | `dispBeginFrameHandler(Handler*)` | 24 |
| `0x8004D890` | `dispEndFrameHandler(Handler*)` | 24 |
| `0x8004D8B4` | `_._7Display` | 32 |
| `0x8004D948` | `static_destroy(__7Display)` | 24 |
| `0x8004D980` | `static_init(__7Display)` | 24 |

#### EASTER.CPP (C:\CHAN\GAME\SRC\GEN\EASTER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004FBE4` | `ClearEasterEggs(void)` | 0 |
| `0x8004FC10` | `RecordEasterButtonPresses(unsigned long)` | 0 |
| `0x8004FC94` | `PrintEasterEggs(void)` | 24 |
| `0x8004FD3C` | `CheckEasterButtonPresses(SonyVButtons*)` | 0 |

#### EFFECTS.CPP (C:\CHAN\GAME\SRC\GEN\EFFECTS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004CA98` | `_._7Effects` | 24 |
| `0x8004CAC0` | `static_destroy(Die__7Effectsii)` | 24 |
| `0x8004CB00` | `static_init(Die__7Effectsii)` | 0 |

#### ENVMGR.CPP (C:\CHAN\GAME\SRC\GEN\ENVMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80053E84` | `_._18EnvironmentManager` | 32 |
| `0x80053ED4` | `environmentLoadFunc(Callback*)` | 24 |
| `0x80053EF8` | `environmentUnloadFunc(Callback*)` | 24 |

#### FXP.CPP (C:\CHAN\GAME\SRC\GEN\FXP.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009DBB8` | `EvalCubic(long*, long*, long, long, long)` | 16 |
| `0x8009DD1C` | `IsPointInFieldOf(const tagLVector&, const tagLVector&, long, long, long)` | 32 |
| `0x8009DD88` | `ClipAngle(long&)` | 0 |
| `0x8009DDE8` | `IsAngleInFieldOf(long, long, long, long)` | 48 |

#### GAME.CPP (C:\CHAN\GAME\SRC\GEN\GAME.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002900C` | `FreeDynamicPrimBuffers(void)` | 24 |
| `0x80029080` | `AllocateDynamicPrimBuffers(bool)` | 32 |
| `0x800291D4` | `ResizeDynamicPrimBuffers(bool)` | 24 |
| `0x80029200` | `DisplayTIM(const char*)` | 56 |
| `0x80029330` | `ButtonCheckCallback(void*)` | 32 |
| `0x80029404` | `SetupEnv(void)` | 56 |
| `0x80029D68` | `MenuRender(MenuMgr*)` | 24 |
| `0x80029DB8` | `MenuDraw(MenuMgr*)` | 32 |
| `0x80029E34` | `MenuFade(void)` | 24 |
| `0x8002A184` | `chanp3dClipCode(_RMVECT16&)` | 0 |
| `0x8002A1F8` | `vecLengthSquared(long, long, long)` | 0 |
| `0x8002A238` | `computeBlockToPointDistances(const Block*, const tagLVector&, int*, const tagLVector&)` | 112 |
| `0x8002A98C` | `DrawEverythingHandler(Handler*)` | 88 |
| `0x8002AF88` | `OffsetToPreventSeams(tagLVector&, const tagLVector&)` | 48 |
| `0x8002B224` | `DrawLoop(ccList*, unsigned long)` | 32 |
| `0x8002B290` | `AnimateLoop(ccList*)` | 24 |
| `0x8002B2F0` | `AnimateEverythingHandler(Handler*)` | 24 |
| `0x8002B368` | `animLoopDSTACK(void)` | 24 |
| `0x8002B408` | `BeginFrameHandler(Handler*)` | 0 |
| `0x8002B420` | `EndFrameHandler(Handler*)` | 0 |
| `0x8002B428` | `_._4Game` | 40 |
| `0x8002B9D8` | `_gameControlChanged(Control*, short)` | 24 |

#### GEFFECT.CPP (C:\CHAN\GAME\SRC\GEN\GEFFECT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008E1CC` | `_._7GEffect` | 32 |
| `0x8008E78C` | `static_destroy(_7GEffect.gNumComEffects)` | 24 |
| `0x8008E7CC` | `static_init(_7GEffect.gNumComEffects)` | 0 |

#### LENSFLRE.CPP (C:\CHAN\GAME\SRC\GEN\LENSFLRE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BEE90` | `_._9LensFlare` | 32 |

#### LEVELMGR.CPP (C:\CHAN\GAME\SRC\GEN\LEVELMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80058BD4` | `_._12LevelManager` | 40 |

#### LIGHTS.CPP (C:\CHAN\GAME\SRC\GEN\LIGHTS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A27A4` | `_._13LightingClass` | 40 |
| `0x800A34A0` | `computeLightDir(_RMVECT16*)` | 0 |
| `0x800A3A34` | `_._11LightAnchor` | 32 |
| `0x800A4298` | `_._12AmbientLight` | 24 |
| `0x800A4350` | `_._13HardwareLight` | 24 |

#### LOADERS.CPP (C:\CHAN\GAME\SRC\GEN\LOADERS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80087704` | `SetTransparentTim(unsigned long, unsigned short*)` | 0 |

#### MANAGER.CPP (C:\CHAN\GAME\SRC\GEN\MANAGER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002ED5C` | `_._7Manager` | 24 |

#### MEMSTAT.CPP (C:\CHAN\GAME\SRC\GEN\MEMSTAT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002F16C` | `MEMSTATTHING(unsigned short)` | 24 |
| `0x8002F1A0` | `MEMSTAT(MemoryStatEnum, MemoryStatModifierEnum)` | 40 |
| `0x8002F2E8` | `MEMSTAT_CLEAR(void)` | 0 |
| `0x8002F320` | `MEMSTAT_PRINT(void)` | 32 |
| `0x8002F3FC` | `MEMSTAT_NEW_RESET(void)` | 0 |
| `0x8002F410` | `MEMSTAT_NEW(void)` | 24 |
| `0x8002F478` | `MEMSTAT_NEW_PRINT(void)` | 24 |
| `0x8002F4AC` | `MEMSTAT_MIN_CLEAR(void)` | 0 |
| `0x8002F4BC` | `MEMSTAT_MIN_PRINT(void)` | 24 |
| `0x8002F4F4` | `MEMSTAT_OBJECTSIZEOF_PRINT(void)` | 24 |
| `0x8002F760` | `MEMSTAT_PRINT_HELPER(MemoryStatEnum)` | 24 |
| `0x8002F7B8` | `MEMSTAT_OBJECTSIZEOF_HELPER(const char*, unsigned long)` | 24 |
| `0x8002F7E8` | `MEMSTATSTRING(MemoryStatEnum)` | 0 |
| `0x8002F9D8` | `MEMSTATMODSTRING(MemoryStatModifierEnum)` | 0 |

#### MEMTRACK.CPP (C:\CHAN\GAME\SRC\GEN\MEMTRACK.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002EEA4` | `SetMemoryState(MemoryStateEnum)` | 0 |
| `0x8002EEDC` | `(bool, uiltin_new)` | 24 |
| `0x8002EF60` | `(bool, uiltin_vec_new)` | 24 |
| `0x8002EFE4` | `(nw__FUii)` | 24 |
| `0x8002F034` | `(void, n__FUii)` | 24 |
| `0x8002F084` | `(bool, uiltin_delete)` | 24 |
| `0x8002F0E0` | `(bool, uiltin_vec_delete)` | 24 |

#### MHUMAN.CPP (C:\CHAN\GAME\SRC\GEN\MHUMAN.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8006E0C8` | `_._13HumanoidModel` | 32 |

#### MODEL.CPP (C:\CHAN\GAME\SRC\GEN\MODEL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8006E4E4` | `RotMatrixZYXAndLights(unsigned short, unsigned short, unsigned short)` | 32 |
| `0x8006E5A8` | `RotMatrixZYXNoLights(unsigned short, unsigned short, unsigned short)` | 32 |
| `0x8006E6CC` | `_._5Model` | 32 |
| `0x8006E8F8` | `_._6GModel` | 24 |
| `0x8006EDAC` | `_._6SModel` | 24 |
| `0x8006F264` | `AnimBlender(long, tPose*, tTree*)` | 56 |
| `0x8006FB84` | `_._6EModel` | 24 |
| `0x800702E8` | `MYrmCartesianToSpherical(_RMVECT16*, RMVECTS16*)` | 48 |
| `0x80070434` | `MYrmSphericalToCartesian(RMVECTS16*, _RMVECT16*)` | 24 |
| `0x80070468` | `headTrackCallback(tSJoint*, _RMVECT16*)` | 248 |
| `0x80070AB8` | `_._13AnimStructure` | 32 |
| `0x800714E0` | `_._12DrawableTree` | 24 |
| `0x8007164C` | `_._13DrawableSTree` | 32 |
| `0x80071928` | `_._13DrawableETree` | 32 |
| `0x80071A18` | `_._11DrawableGeo` | 32 |
| `0x80071AF4` | `MakeBillboardMatrix(const tagLVector&, MATRIX*, int)` | 80 |
| `0x80071BCC` | `MakeBillboardMatrixFlip(const tagLVector&, MATRIX*, int)` | 80 |
| `0x80071CEC` | `_._13OriginalBasic` | 32 |
| `0x80071D80` | `_._11OriginalGeo` | 24 |
| `0x80071E44` | `_._12OriginalTree` | 32 |
| `0x80071F00` | `_._13OriginalETree` | 24 |
| `0x80071FC0` | `_._13OriginalSTree` | 32 |
| `0x800721B0` | `RedirectCompositeSuitAnimation(tCompositeAnim*, const tPrimGeom*)` | 48 |

#### MPLAYER.CPP (C:\CHAN\GAME\SRC\GEN\MPLAYER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80077A94` | `_._11PlayerModel` | 24 |
| `0x800782A0` | `_._18nisCharMgrCallback` | 24 |

#### PALDATA.CPP (C:\CHAN\GAME\SRC\GEN\PALDATA.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009CE44` | `_._11PaletteData` | 32 |

#### PATH.CPP (C:\CHAN\GAME\SRC\GEN\PATH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A44F4` | `Swap(NodeAttribs&, NodeAttribs&)` | 0 |
| `0x800A4530` | `(as__11NodeAttribsRC11NodeAttribs)` | 32 |
| `0x800A5EDC` | `_._10SubDivNode` | 32 |

#### PATHINFO.CPP (C:\CHAN\GAME\SRC\GEN\PATHINFO.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BE78C` | `_._8PathInfo` | 32 |

#### PROFILE.CPP (C:\CHAN\GAME\SRC\GEN\PROFILE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80047200` | `HTickerToGameLoopPercentage(unsigned long)` | 0 |
| `0x8004750C` | `static_init(PROFILE_CODE_TYPE_NAME_ARRAY)` | 0 |

#### PWEFFECT.CPP (C:\CHAN\GAME\SRC\GEN\PWEFFECT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009B748` | `_._8PWEffect` | 32 |
| `0x8009C52C` | `_._9FPWEffect` | 24 |
| `0x8009C554` | `static_destroy(InitPWorldEffects__8PWEffectP7DBPoint)` | 24 |
| `0x8009C594` | `static_init(InitPWorldEffects__8PWEffectP7DBPoint)` | 0 |

#### SCALEDAT.CPP (C:\CHAN\GAME\SRC\GEN\SCALEDAT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009CAEC` | `_._9ScaleData` | 32 |

#### SCOREMGR.CPP (C:\CHAN\GAME\SRC\GEN\SCOREMGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004CC38` | `scoreMgrPrivHandler(Handler*)` | 24 |
| `0x8004CCA8` | `_._12ScoreManager` | 24 |

#### SPOTLIGHT.CPP (C:\CHAN\GAME\SRC\GEN\SPOTLIGHT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BE2A4` | `_._9SpotLight` | 24 |

#### STREAM.CPP (C:\CHAN\GAME\SRC\GEN\STREAM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009911C` | `ConvertEndian(unsigned long)` | 8 |
| `0x80099160` | `_._16StreamHeaderNode` | 24 |
| `0x80099490` | `_._6Stream` | 32 |
| `0x80099648` | `myGeoLoaderCallback(tEntity*)` | 24 |
| `0x800996C0` | `myETreeLoaderCallback(tEntity*)` | 32 |
| `0x8009976C` | `mySTreeLoaderCallback(tEntity*)` | 32 |
| `0x800998EC` | `AnimLoaderCallback(tEntity*)` | 32 |
| `0x80099988` | `CompAnimLoaderCallback(tEntity*)` | 32 |
| `0x8009ABF4` | `MemoryStats(const char*, char*, unsigned long)` | 0 |

#### SUBS.CPP (C:\CHAN\GAME\SRC\GEN\SUBS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80084274` | `fdn_Sscanf(const char*, char*, ...)` | 56 |
| `0x800843F4` | `strcmpi` | 0 |
| `0x80084480` | `GetNextAlphaNumToken(char*, char*)` | 0 |
| `0x8008454C` | `alphatoulong(char*)` | 0 |

#### SWITCH.CPP (C:\CHAN\GAME\SRC\GEN\SWITCH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80093E68` | `_gfrsEnterAmbiantSpace(Thing*, unsigned long, const char**)` | 24 |
| `0x80093EEC` | `_gfPlayerDeathVol(Thing*, unsigned long, const char**)` | 32 |
| `0x80093FF0` | `MakeThingDeathVolSound(unsigned long)` | 24 |
| `0x8009406C` | `_gfEnemyObstDeathVol(Thing*, unsigned long, const char**)` | 24 |
| `0x800940D4` | `_gfDirectorVol(Thing*, unsigned long, const char**)` | 24 |
| `0x80094128` | `_gfGoToVol(Thing*, unsigned long, const char**)` | 64 |
| `0x800941D4` | `_gfExitTest(Thing*, unsigned long, const char**)` | 0 |
| `0x800941DC` | `_gfResetPlayer(Thing*, unsigned long, const char**)` | 24 |
| `0x80094238` | `_gfSetDeathState(Thing*, unsigned long, const char**)` | 40 |
| `0x800942B0` | `_gfBehaviorTrigger(Thing*, unsigned long, const char**)` | 0 |
| `0x800942B8` | `_gfGateCleanupVol(Thing*, unsigned long, const char**)` | 40 |
| `0x800943B4` | `gPurgeLoadGroups(void)` | 32 |
| `0x80094418` | `gSyncLoadGroup(int)` | 32 |
| `0x80094518` | `_gfAsyncLoadGroup(Thing*, unsigned long, const char**)` | 40 |
| `0x80094684` | `_gfAsyncLoadNIS(Thing*, unsigned long, const char**)` | 40 |
| `0x80094714` | `_gfAsyncLoadNISGOTO(Thing*, unsigned long, const char**)` | 40 |
| `0x800947B0` | `gfUnloadLoadChar(Q22AI10ThingTypesT0)` | 40 |
| `0x8009484C` | `UnloadHelper(_RTASK*)` | 24 |
| `0x80094878` | `_gfCharModelLoad(Thing*, unsigned long, const char**)` | 48 |
| `0x80094964` | `_gfLevelComplete(Thing*, unsigned long, const char**)` | 48 |
| `0x80094B0C` | `_gfCheckpoint(Thing*, unsigned long, const char**)` | 40 |
| `0x80094BBC` | `_gfBossVol(Thing*, unsigned long, const char**)` | 0 |
| `0x80094BC4` | `_gfLoadDialog(Thing*, unsigned long, const char**)` | 32 |
| `0x80094C54` | `_gfPlayDialog(Thing*, unsigned long, const char**)` | 32 |
| `0x80094D68` | `_._9WDBSwitch` | 40 |

#### TAGS.C (C:\CHAN\GAME\SRC\GEN\TAGS.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800AEA90` | `NextTagItem` | 0 |
| `0x800AEAFC` | `FindTagItem` | 32 |

#### TIME.CPP (C:\CHAN\GAME\SRC\GEN\TIME.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004492C` | `timePrivHandler(Handler*)` | 24 |
| `0x800449E8` | `_._4Time` | 24 |

#### TRAIL.CPP (C:\CHAN\GAME\SRC\GEN\TRAIL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80079778` | `_._6Trails` | 40 |
| `0x8007A598` | `static_destroy(SPL_TRAIL_RATIO)` | 24 |
| `0x8007A5D8` | `static_init(SPL_TRAIL_RATIO)` | 0 |

#### UVDATA.CPP (C:\CHAN\GAME\SRC\GEN\UVDATA.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800985B0` | `_._10UVPrimData` | 24 |
| `0x8009891C` | `_._11CBVPrimData` | 32 |

#### WEFFECT.CPP (C:\CHAN\GAME\SRC\GEN\WEFFECT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008B5B0` | `_._7WEffect` | 32 |
| `0x8008BE88` | `_._8FWEffect` | 32 |
| `0x8008CE70` | `_._9CBVEffect` | 24 |
| `0x8008D010` | `static_destroy(_7WEffect.gNumComEffects)` | 24 |
| `0x8008D050` | `static_init(_7WEffect.gNumComEffects)` | 0 |

#### WORLD.CPP (C:\CHAN\GAME\SRC\GEN\WORLD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800451D0` | `mAtoi(const char*)` | 0 |
| `0x80045340` | `_._5World` | 48 |
| `0x80047014` | `DeletePlayerBlendAndAnimData(void)` | 32 |

#### WORLDPTS.CPP (C:\CHAN\GAME\SRC\GEN\WORLDPTS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008D8F8` | `static_destroy(theWorldPoints)` | 24 |
| `0x8008D944` | `static_init(theWorldPoints)` | 0 |

#### BACKG.CPP (C:\CHAN\GAME\SRC\PSX\BACKG.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8005805C` | `_._5BackG` | 32 |

#### EXPAND.CPP (C:\CHAN\GAME\SRC\PSX\EXPAND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80093DB4` | `SquExpandData(unsigned char*, unsigned char*)` | 0 |

#### HUD.CPP (C:\CHAN\GAME\SRC\PSX\HUD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8003F53C` | `_._3HUD` | 40 |

#### MAIN.CPP (C:\CHAN\GAME\SRC\PSX\MAIN.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80026130` | `MyVBL(...)` | 24 |
| `0x80026220` | `SetupVBL(...(*)()*, long)` | 24 |
| `0x8002627C` | `SetupPSXStuff(void)` | 32 |
| `0x8002635C` | `main` | 24 |
| `0x800264C8` | `LoadOverlay(OverlayId)` | 48 |
| `0x800265F0` | `LoadBossAIOverlay(BossAIOverlayEnum)` | 40 |
| `0x800266C8` | `EarlyLoadConfigFile(void)` | 0 |

#### MEMCARD.CPP (C:\CHAN\GAME\SRC\PSX\MEMCARD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001334C` | `MCInitialize` | 24 |
| `0x80013390` | `MCTerminate` | 32 |
| `0x80013420` | `MCLoadOverlay` | 24 |
| `0x80013448` | `MCUnloadOverlay` | 24 |
| `0x80013470` | `MCSetFileType` | 40 |
| `0x80013554` | `MCGetFullState` | 24 |
| `0x80013598` | `MCGetState` | 24 |
| `0x800135DC` | `MCGetDirectory` | 256 |
| `0x80013820` | `MCGetFreeBlocks` | 96 |
| `0x80013900` | `MCCreateFile` | 224 |
| `0x80013B64` | `MCDeleteFile` | 72 |
| `0x80013BEC` | `MCLoadFile` | 80 |
| `0x80013CAC` | `MCSaveFile` | 80 |
| `0x80013D6C` | `MCFormatCard` | 24 |
| `0x80013D94` | `GetDirName(long, unsigned long, const char*, unsigned long*, char*, const char*)` | 320 |
| `0x80013FF4` | `WaitForCompletion(void)` | 32 |
| `0x800140B8` | `GetChannel(unsigned int)` | 0 |
| `0x800140D4` | `SetTitle(unsigned short*, const char*)` | 24 |
| `0x80014228` | `GetTitle(char*, const unsigned short*)` | 0 |

#### MOVIES.CPP (C:\CHAN\GAME\SRC\PSX\MOVIES.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001434C` | `_._11MoviePlayer` | 32 |
| `0x800146F8` | `_._11MovieRandom` | 32 |

#### PARTICLE.CPP (C:\CHAN\GAME\SRC\PSX\PARTICLE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80097B98` | `_._14ParticleSystem` | 32 |
| `0x80097C8C` | `_._12ParticleInfo` | 24 |
| `0x80097CC8` | `_._13ParticleStats` | 32 |
| `0x80097DA4` | `_._17ParticleSystemMgr` | 32 |
| `0x80098020` | `SphereToCart(long, long, _RMVECT16*)` | 40 |
| `0x800980E0` | `static_destroy(SPL_PARTICLE_SPEED_AVERAGE)` | 24 |
| `0x8009814C` | `static_init(SPL_PARTICLE_SPEED_AVERAGE)` | 0 |

#### PSXSUBS.CPP (C:\CHAN\GAME\SRC\PSX\PSXSUBS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004B6C8` | `MyDrawLine(const _RMVECT16&, N30)` | 72 |
| `0x8004B8DC` | `MyDrawBoxCenterRadius(const _RMVECT16&, long, long)` | 112 |
| `0x8004B9F4` | `MyDrawBox(const _RMVECT16&, N20)` | 264 |
| `0x8004BE44` | `GetScreenCoordinates(const tagLVector&, tagLVector&)` | 96 |

#### RAMTEXANIM.CPP (C:\CHAN\GAME\SRC\PSX\RAMTEXANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80089358` | `_._11tRAMTexAnim` | 32 |
| `0x80089468` | `_._11tRAMTexFlip` | 24 |

#### RCDMAIN.C (C:\CHAN\GAME\SRC\PSX\RCDMAIN.C)

| Address | Function | Size |
|---------|----------|------|
| `0x80034818` | `rCDGetMode` | 0 |
| `0x80034824` | `rCDAlignDown` | 0 |
| `0x8003483C` | `rCDFileSize` | 0 |
| `0x80034848` | `rCDSize` | 24 |
| `0x8003488C` | `rCDSetCallBack` | 24 |
| `0x800348AC` | `rCDFreeQ` | 24 |
| `0x800348D4` | `rCDMemcpyQ` | 24 |
| `0x800348FC` | `rCDFatalStub` | 0 |
| `0x80034904` | `rCDInit` | 24 |
| `0x80034A98` | `rCDOpen` | 152 |
| `0x80034C3C` | `rCDCacheInit` | 32 |
| `0x80034C90` | `rCDCacheTerm` | 24 |
| `0x80034CD0` | `rCDReadA` | 40 |
| `0x80034EAC` | `rCDSeekQ` | 0 |
| `0x80034EF0` | `rCDSeekA` | 24 |
| `0x80034F28` | `rCDCloseA` | 24 |

#### RCDQ.C (C:\CHAN\GAME\SRC\PSX\RCDQ.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8007F0AC` | `rCDInitQueue` | 0 |
| `0x8007F0E0` | `rCDAddQInternal` | 24 |
| `0x8007F1AC` | `rCDAddQ` | 32 |
| `0x8007F1D0` | `rCDAddQSingle` | 32 |
| `0x8007F1F4` | `rCDService` | 24 |
| `0x8007F214` | `rCDWaitUntilDone` | 24 |
| `0x8007F25C` | `rCDTaskService` | 24 |
| `0x8007F2B8` | `rCDDone` | 40 |
| `0x8007F41C` | `rCDSetPriority` | 0 |

#### SHADOW.CPP (C:\CHAN\GAME\SRC\PSX\SHADOW.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AED60` | `_._6Shadow` | 24 |
| `0x800AED94` | `ShadowShow(const tagLVector&, tagLVector*, int)` | 104 |
| `0x800AF014` | `_._10TreeShadow` | 24 |
| `0x800AF2AC` | `_._12SimpleShadow` | 24 |

#### SONYDUMP.CPP (C:\CHAN\GAME\SRC\PSX\SONYDUMP.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800538B8` | `OpenTIM` | 0 |
| `0x80053924` | `ReadTIM` | 0 |

#### SOUND.CPP (C:\CHAN\GAME\SRC\PSX\SOUND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8005950C` | `_StereoOnOff(hdMenuItem*)` | 32 |
| `0x80059594` | `_SetMusicVolume(hdMenuItem*)` | 24 |
| `0x80059600` | `_SetEffectsVolume(hdMenuItem*)` | 24 |
| `0x80059698` | `_SetDialogVolume(hdMenuItem*)` | 24 |
| `0x80059704` | `soundLoadFunc(Callback*)` | 24 |
| `0x80059760` | `soundUnLoadFunc(Callback*)` | 24 |
| `0x800597E8` | `_._5Sound` | 24 |
| `0x80059EB4` | `static_init(soundLoadFunc__FP8Callback)` | 24 |

#### BASESND.CPP (C:\CHAN\GAME\SRC\SND\BASESND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A1E5C` | `_._6CSound` | 32 |
| `0x800A1EAC` | `(nw__6CSoundUi)` | 40 |
| `0x800A1F00` | `(double, long, __6CSoundPv)` | 32 |

#### DRCTRSND.CPP (C:\CHAN\GAME\SRC\SND\DRCTRSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008EBE4` | `_._14CDirectorSound` | 32 |

#### DSTRSND.CPP (C:\CHAN\GAME\SRC\SND\DSTRSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AC650` | `_._18CDestructibleSound` | 32 |

#### ESOUND.CPP (C:\CHAN\GAME\SRC\SND\ESOUND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800ACCDC` | `_._20CParticleEffectSound` | 32 |
| `0x800ACDF8` | `_._17CWorldEffectSound` | 32 |

#### FESND.CPP (C:\CHAN\GAME\SRC\SND\FESND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80053640` | `_._14CFrontEndSound` | 32 |

#### HMNDSND.CPP (C:\CHAN\GAME\SRC\SND\HMNDSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800606E0` | `_._14CHumanoidSound` | 32 |

#### JCSDLG.CPP (C:\CHAN\GAME\SRC\SND\JCSDLG.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80042810` | `jcsDialogCleanUp(void)` | 24 |
| `0x800428B8` | `jcsGetDlgStatus(void)` | 0 |
| `0x800428C4` | `jcsInitializeDialog(void)` | 24 |
| `0x80042908` | `jcsIsPlayable(long)` | 24 |
| `0x800429A4` | `jcsIsPlaying(void)` | 24 |
| `0x800429CC` | `jcsIsPlaying(long)` | 24 |
| `0x80042A30` | `jcsKillDialogByHandle(long)` | 24 |
| `0x80042C78` | `jcsLoadDialog(rsCharacter, rsDialog, long)` | 64 |
| `0x80042FF4` | `jcsPauseDialog(void)` | 24 |
| `0x80043044` | `jcsPlayDialog(long, const tagLVector*, unsigned long)` | 40 |
| `0x80043218` | `jcsQueryDialogPriority(void)` | 0 |
| `0x80043254` | `jcsQueryDialogPriority(long)` | 32 |
| `0x800432B8` | `jcsResumeDialog(void)` | 24 |
| `0x800432E0` | `jcsSetConfigurationDialog(const jcsSoundParams*)` | 24 |
| `0x80043350` | `jcsSetLevelDialog(unsigned long)` | 24 |
| `0x80043420` | `jcsSetListenerDialog(const tagLVector*, const long*)` | 24 |
| `0x8004344C` | `jcsStartDialog(void)` | 0 |
| `0x8004345C` | `jcsStopDialog(void)` | 24 |
| `0x8004347C` | `jcsTerminateDialog(void)` | 24 |
| `0x800434D8` | `jcsValidateHandle(long)` | 24 |
| `0x80043568` | `CDDoneCallback(long, long, long)` | 24 |
| `0x80043684` | `CDDoneDefer(Queue, State, Queue)` | 40 |
| `0x80043700` | `CheckFlushCount(Queue)` | 40 |
| `0x800437BC` | `ConflictWithOtherQueue(unsigned short, Queue)` | 0 |
| `0x800437D4` | `CreateHandle(rsDialog, long, Queue)` | 0 |
| `0x80043808` | `DialogTask(_RTASK*)` | 24 |
| `0x80043978` | `FreeTransferBuffer(void)` | 24 |
| `0x800439B0` | `GetHeader(unsigned long, unsigned int)` | 0 |
| `0x80043A00` | `GetHeaderInfo(rsCharacter, unsigned short*, N21)` | 40 |
| `0x80043A90` | `GetUnused(unsigned char, unsigned short)` | 40 |
| `0x80043B08` | `GetVagSize(unsigned short)` | 24 |
| `0x80043B30` | `IfCanLoad(rsCharacter, rsDialog, unsigned long*, rsDialog)` | 48 |
| `0x80043C10` | `IsCurrentHandle(long)` | 24 |
| `0x80043C80` | `IsEitherHandle(long)` | 32 |
| `0x80043CD0` | `IsLoadableDialog(rsCharacter, rsDialog)` | 32 |
| `0x80043D1C` | `IsPrimaryHandle(long)` | 0 |
| `0x80043D58` | `IsSecondaryHandle(long)` | 0 |
| `0x80043D94` | `IsVoicePlaying(void)` | 24 |
| `0x80043DF0` | `KillAllDialog(void)` | 24 |
| `0x80043E2C` | `LoadAllReady(rsCharacter, rsDialog, long)` | 64 |
| `0x80043F5C` | `LoadToAudioMemory(void)` | 40 |
| `0x80043FF4` | `MarkAsTooLarge(unsigned short)` | 24 |
| `0x80044030` | `MarkAsUsed(unsigned short)` | 24 |
| `0x8004406C` | `PlayLoaded(const tagLVector*, State)` | 48 |
| `0x8004416C` | `PrepareDefer(rsCharacter, rsDialog, long, State, Queue)` | 24 |
| `0x800441B4` | `PauseTimeOut(Queue)` | 24 |
| `0x80044234` | `ReclaimUsed(unsigned char, unsigned short)` | 32 |
| `0x800442AC` | `RequestLoad(long, rsCharacter, rsDialog, long, Queue, State)` | 64 |
| `0x80044368` | `ResetDialogInfo(Queue, State)` | 24 |
| `0x800443BC` | `ResumeTimeOut(Queue)` | 24 |
| `0x80044420` | `SelectDialog(rsCharacter, rsDialog, unsigned long*, rsDialog)` | 120 |
| `0x8004466C` | `SetDlgStatus(DlgStatus)` | 0 |
| `0x80044678` | `SetHeader(unsigned long, unsigned char)` | 0 |
| `0x8004468C` | `StartLoad(long, rsDialog, long, unsigned long, unsigned short, unsigned long, Queue, State)` | 48 |
| `0x800447E8` | `StopVoice(void)` | 24 |
| `0x8004481C` | `UpdateState(State)` | 0 |
| `0x80044828` | `UpdateTimeOut(unsigned long, const tagLVector*, Queue)` | 24 |
| `0x80044884` | `UpgradeDlgInfo(State)` | 24 |
| `0x800448F4` | `UpgradeToPrimary(void)` | 24 |

#### JCSOUND.CPP (C:\CHAN\GAME\SRC\SND\JCSOUND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8003509C` | `jcsInitialize(void)` | 24 |
| `0x80035190` | `jcsTerminate(void)` | 24 |
| `0x800352C8` | `jcsSetConfiguration(jcsSoundParams*)` | 24 |
| `0x80035454` | `jcsGetConfiguration(jcsSoundParams*)` | 0 |
| `0x8003547C` | `jcsSetListener(const tagLVector*, const long*)` | 32 |
| `0x800354C4` | `jcsUnloadLevel(void)` | 24 |
| `0x80035564` | `jcsSetSoundLocation(rsSoundLocation)` | 144 |
| `0x80035664` | `jcsStartSound(void)` | 24 |
| `0x800356D0` | `jcsStopSound(void)` | 24 |
| `0x80035760` | `jcsFadeOutEngine(unsigned long)` | 24 |
| `0x8003582C` | `jcsFadeInEngine(unsigned long)` | 24 |
| `0x800358F0` | `jcsCdYield(unsigned long)` | 24 |
| `0x80035990` | `jcsCdAccess(unsigned long)` | 24 |
| `0x80035A30` | `jcsSetAmbienceSpace(unsigned long)` | 24 |
| `0x80035AA0` | `jcsSetAmbienceCrossFade(long)` | 24 |
| `0x80035B00` | `jcsHandleControlEvent(rsSoundEvent, long, long, long)` | 40 |
| `0x80035DC0` | `LoadFile(const char*)` | 136 |
| `0x80035E6C` | `LoadLevel(long, long)` | 40 |
| `0x80035EDC` | `jcsGetCurrentLocationInfo(void)` | 0 |
| `0x80035F10` | `jcsGetLocationInfo(rsSoundLocation)` | 0 |

#### KICKSND.CPP (C:\CHAN\GAME\SRC\SND\KICKSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AD2F0` | `_._15CKickNRollSound` | 32 |

#### KNDNSND.CPP (C:\CHAN\GAME\SRC\SND\KNDNSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AD51C` | `_._15CKnockDownSound` | 32 |

#### PHSMNGR.CPP (C:\CHAN\GAME\SRC\SND\PHSMNGR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80082C1C` | `_._13CPhaseManager` | 32 |
| `0x80082D60` | `(Q213CPhaseManager16CPhaseTableEntry)` | 0 |

#### PLATSND.CPP (C:\CHAN\GAME\SRC\SND\PLATSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AC8E0` | `_._14CPlatformSound` | 32 |

#### PNDLMSND.CPP (C:\CHAN\GAME\SRC\SND\PNDLMSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800ACC28` | `_._14CPendulumSound` | 32 |

#### PRSTSND.CPP (C:\CHAN\GAME\SRC\SND\PRSTSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AC2CC` | `_._23CGenericPersistentSound` | 32 |
| `0x800AC510` | `(nw__23CGenericPersistentSoundUi)` | 40 |

#### PUSHSND.CPP (C:\CHAN\GAME\SRC\SND\PUSHSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AC7C8` | `_._14CPushableSound` | 32 |

#### RSDAMBCE.CPP (C:\CHAN\GAME\SRC\SND\RSDAMBCE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80080DD0` | `_._11rsdAmbiance` | 24 |

#### RSDBACH.CPP (C:\CHAN\GAME\SRC\SND\RSDBACH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80082D74` | `rsdInit(void)` | 24 |
| `0x80082E50` | `rsdTerm(void)` | 24 |
| `0x80082F10` | `rsdLoadData(const unsigned char*, unsigned long)` | 56 |
| `0x80083140` | `rsdCreateFileList(int, int, unsigned long*)` | 56 |
| `0x8008330C` | `rsdFreeFileList(const char**, unsigned long)` | 40 |
| `0x80083388` | `rsdAllocVoice(unsigned char)` | 0 |
| `0x800834B4` | `rsdSetVoice(long, long)` | 32 |
| `0x80083550` | `rsdSetVolume(long, unsigned short, unsigned short)` | 0 |
| `0x80083588` | `rsdGetVolume(long, unsigned short*, long)` | 0 |
| `0x800835DC` | `rsdSetPitch(long, unsigned short)` | 0 |
| `0x80083600` | `rsdSetADSR(long, unsigned long)` | 0 |
| `0x8008362C` | `rsdSetLoopPoint(long, unsigned long)` | 0 |
| `0x80083654` | `rsdSetStartAddr(long, unsigned long)` | 0 |
| `0x8008367C` | `rsdLockVoice(long)` | 0 |
| `0x80083698` | `rsdReleaseVoice(long)` | 0 |
| `0x800836B4` | `rsdGetVoice(long)` | 32 |
| `0x8008371C` | `rsdGetPitch(long)` | 0 |
| `0x80083748` | `rsdSetVolumeMain(unsigned short)` | 64 |
| `0x8008379C` | `rsdAllocPhonograph(long, unsigned long*, long, bool*)` | 0 |
| `0x800837E0` | `rsdFreePhonograph(long, unsigned long, bool)` | 0 |
| `0x8008380C` | `rsdSetReverb(rsdReverbMode)` | 56 |
| `0x80083908` | `rsdGetReverb(void)` | 0 |
| `0x80083914` | `rsdSetReverbDepth(short)` | 0 |
| `0x80083920` | `rsdReverbOnVoice(long)` | 24 |
| `0x80083968` | `rsdReverbOffVoice(long)` | 24 |
| `0x800839B4` | `rsdVoiceOn(long)` | 0 |
| `0x80083A08` | `rsdVoiceOff(long)` | 0 |
| `0x80083A5C` | `rsdIsVoicePending(long)` | 0 |
| `0x80083A90` | `rsdIsVoiceOn(long)` | 0 |
| `0x80083ABC` | `WriteOnOffBitsTask(_RTASK*)` | 0 |

#### RSDCLIP.CPP (C:\CHAN\GAME\SRC\SND\RSDCLIP.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80083B58` | `_._7rsdClip` | 32 |

#### RSDLOAD.CPP (C:\CHAN\GAME\SRC\SND\RSDLOAD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800936C0` | `(nw__7rsdLoadUi)` | 24 |
| `0x800936F8` | `(double, long, __7rsdLoadPv)` | 24 |
| `0x800937F0` | `_._7rsdLoad` | 24 |

#### RSDSTRM.CPP (C:\CHAN\GAME\SRC\SND\RSDSTRM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B91E0` | `_._9rsdStream` | 24 |
| `0x800B97CC` | `Silence(unsigned char*, unsigned long, unsigned long, unsigned long)` | 64 |

#### RSDUTIL.CPP (C:\CHAN\GAME\SRC\SND\RSDUTIL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8007FDD8` | `_._8rsdWorld` | 24 |
| `0x8008057C` | `(nw__13rsdPersistentUi)` | 24 |
| `0x800805A0` | `(double, long, __13rsdPersistentPv)` | 24 |
| `0x8008068C` | `_._13rsdPersistent` | 32 |

#### RSEVENT.CPP (C:\CHAN\GAME\SRC\SND\RSEVENT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800346B0` | `rsEvent(rsSoundEvent, long, long, long)` | 24 |
| `0x8003470C` | `rsDialogEvent(rsSoundEvent, long, long, long)` | 24 |

#### RSMPLR.CPP (C:\CHAN\GAME\SRC\SND\RSMPLR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8007F4F0` | `_._14rsdMusicPlayer` | 32 |

#### SNDFACT.CPP (C:\CHAN\GAME\SRC\SND\SNDFACT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80057890` | `static_destroy(_13CSoundFactory.g_pSoundMemoryPoolBuffer)` | 24 |
| `0x800578C8` | `static_init(_13CSoundFactory.g_pSoundMemoryPoolBuffer)` | 24 |

#### SNDFDB.CPP (C:\CHAN\GAME\SRC\SND\SNDFDB.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AADC4` | `_._21CSoundFactoryDatabase` | 24 |

#### SNDMATH.CPP (C:\CHAN\GAME\SRC\SND\SNDMATH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BA0F0` | `Decibel100(unsigned long)` | 0 |
| `0x800BA118` | `PsxVol100(unsigned long)` | 24 |
| `0x800BA150` | `PsxPitch200(unsigned long)` | 0 |

#### TRNSSND.CPP (C:\CHAN\GAME\SRC\SND\TRNSSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AA84C` | `_._22CGenericTransientSound` | 32 |
| `0x800AAD68` | `(nw__22CGenericTransientSoundUi)` | 40 |

#### WPNSND.CPP (C:\CHAN\GAME\SRC\SND\WPNSND.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AD184` | `_._12CWeaponSound` | 32 |

#### PETLATL.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\PETLATL.C)

| Address | Function | Size |
|---------|----------|------|
| `0x80085D18` | `rrLoadHeaderOnly` | 2080 |
| `0x80085DE8` | `rrSize` | 0 |
| `0x80085DFC` | `rrOffset` | 0 |

#### RDEBUG.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RDEBUG.C)

| Address | Function | Size |
|---------|----------|------|
| `0x80083F4C` | `rPutChar` | 24 |
| `0x80083F94` | `rPutCharDebug` | 32 |
| `0x80083FF8` | `rPutString` | 24 |
| `0x80084044` | `rAssertFail` | 24 |
| `0x8008415C` | `rWarningFail` | 24 |
| `0x80084194` | `rValidFail` | 24 |
| `0x800841D4` | `rValidPointer` | 24 |
| `0x800841F8` | `rValidPointer32` | 0 |

#### RENTRYX.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RENTRYX.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800266D8` | `rInitMemX` | 24 |

#### RMAIN.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RMAIN.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8002671C` | `rInit` | 24 |
| `0x80026770` | `rIsPlatGik` | 24 |
| `0x800267B8` | `rIsPlatEuroGik` | 24 |
| `0x800267FC` | `rIsPlatPsycho` | 24 |
| `0x8002683C` | `rIsPlatPsychoProfiler` | 0 |
| `0x80026890` | `rIsPlatRealPSX` | 24 |
| `0x800268DC` | `rIsPlatBonk` | 0 |
| `0x800268E4` | `rIsPlatSatPsycho` | 0 |
| `0x800268EC` | `rIsPlatRealSat` | 0 |
| `0x800268F4` | `rIsPlatKrak` | 0 |
| `0x800268FC` | `rIsPlatPTUI` | 32 |

#### RPRINTF.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RPRINTF.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800368F8` | `printf_CharOut` | 24 |
| `0x80036940` | `printf_IntOut` | 72 |
| `0x80036C0C` | `printf_FixedOut` | 56 |
| `0x80036D4C` | `printf_VectOut` | 56 |
| `0x80036EBC` | `rVSPrintf` | 56 |
| `0x800372A0` | `rprintf` | 24 |
| `0x800372D8` | `rSPrintf` | 24 |

#### RSTREXT.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RSTREXT.C)

| Address | Function | Size |
|---------|----------|------|
| `0x80058A04` | `rStrNCopy` | 32 |
| `0x80058AA8` | `rStrChr` | 0 |

#### RSTRING.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RSTRING.C)

| Address | Function | Size |
|---------|----------|------|
| `0x80057B2C` | `rStrCompare` | 0 |
| `0x80057B60` | `rStrCompareNoCase` | 32 |
| `0x80057BD8` | `rStrCopy` | 0 |
| `0x80057C1C` | `rStrCat` | 0 |
| `0x80057C8C` | `rStrLength` | 0 |
| `0x80057CC8` | `rToUpper` | 0 |
| `0x80057CE8` | `rToLower` | 0 |

#### RTASK.CPP (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RTASK.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002E8DC` | `rTaskInit` | 24 |
| `0x8002E910` | `rInitTaskList` | 0 |
| `0x8002E960` | `rTaskSuicide` | 0 |
| `0x8002E968` | `rDelTask` | 0 |
| `0x8002E978` | `rInsertTask(_RTASKLIST*, _RTASK*)` | 0 |
| `0x8002E9D0` | `rExecuteTaskBucket(_RTASKLIST*, _RTASK_BUCKET*)` | 32 |
| `0x8002EA6C` | `rReInsertTaskBucket(_RTASKLIST*, _RTASK_BUCKET*)` | 0 |
| `0x8002EAFC` | `rDoTaskList` | 32 |
| `0x8002EB90` | `rNewTask` | 32 |
| `0x8002EBEC` | `rNewTaskP` | 24 |
| `0x8002EC14` | `rNewTaskPP` | 24 |

#### RTIMEF.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\RTIMEF.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8009D338` | `rTickerDifference` | 0 |

#### STRTO.C (C:\chan\devsys\psx\radlib\SOURCE\MAIN\STRTO.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800C480C` | `rStrToFixed` | 0 |

#### HASH.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\FUNC\HASH.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800B8FF4` | `rmStringHash` | 0 |

#### MAG2.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\FUNC\MAG2.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009D358` | `rmMag2(long, long)` | 24 |

#### MAG2FF.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\FUNC\MAG2FF.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AEA54` | `rmMag2ff(long, long)` | 0 |

#### MAG3.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\FUNC\MAG3.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009D6EC` | `rmMag3(long, long, long)` | 24 |

#### MAG3FF.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\FUNC\MAG3FF.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AE904` | `rmMag3ffu(unsigned long, unsigned long, unsigned long)` | 0 |
| `0x800AEA14` | `rmMag3ff(long, long, long)` | 24 |

#### RANDOM0.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\FUNC\RANDOM0.C)

| Address | Function | Size |
|---------|----------|------|
| `0x80089C1C` | `rmRandom0` | 24 |
| `0x80089C6C` | `rmRangedRandom0` | 24 |

#### DIVIDE.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\MULTDIV\DIVIDE.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8007D8B4` | `rmDiv16i` | 0 |

#### INVERSE.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\MULTDIV\INVERSE.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800AECE8` | `rmInverse16` | 0 |

#### ASIN.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\TRIG\ASIN.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800B0C48` | `rmASin16` | 0 |
| `0x800B0CA4` | `rmACos16` | 0 |

#### ATAN16.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\TRIG\ATAN16.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800C05CC` | `rmATan16` | 0 |

#### ATAN216.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\TRIG\ATAN216.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8009DAE0` | `rmATan216` | 24 |

#### POLAR.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\TRIG\POLAR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AD660` | `rmCartesianToPolar(long*, long*, long, long)` | 40 |

#### SIN.C (C:\chan\devsys\psx\radlib\SOURCE\MATH\TRIG\SIN.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800743EC` | `rmSin16` | 0 |

#### SPHERE.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\TRIG\SPHERE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AF5C8` | `rmSphericalToCartesian(RMVECTS16*, _RMVECT16*)` | 48 |

#### VECT2D.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\VECTOR\VECT2D.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C4A5C` | `rmV2Dot(_RMVECT216*, _RMVECT216*)` | 8 |

#### VECT3D.CPP (C:\chan\devsys\psx\radlib\SOURCE\MATH\VECTOR\VECT3D.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009DA64` | `rmV3Normalize(_RMVECT16*, _RMVECT16*)` | 32 |

#### RCDCACHE.C (C:\chan\devsys\psx\radlib\SOURCE\RADCD\RCDCACHE.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800888A0` | `rCDOpenC` | 32 |
| `0x800888E4` | `rCDReadC` | 40 |
| `0x80088A3C` | `rCDSeekC` | 0 |

#### RCDDBG.C (C:\chan\devsys\psx\radlib\SOURCE\RADCD\RCDDBG.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8007E1F4` | `rCDPTUIOpen` | 24 |
| `0x8007E268` | `rCDPTUIReadQ` | 32 |
| `0x8007E494` | `rCDPSYQOpen` | 40 |
| `0x8007E548` | `rCDPSYQReadQ` | 48 |

#### RCDGETF.CPP (C:\chan\devsys\psx\radlib\SOURCE\RADCD\RCDGETF.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80034FB4` | `rCDGetFileA` | 40 |
| `0x80035060` | `rCDGetFile` | 24 |

#### RCDREALX.C (C:\chan\devsys\psx\radlib\SOURCE\RADCD\RCDREALX.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8007E660` | `rcd_add_temp_dir` | 32 |
| `0x8007E710` | `rcd_transfer_dir` | 32 |
| `0x8007E7D4` | `rcd_read_long` | 0 |
| `0x8007E800` | `rcd_recurse_dir` | 48 |
| `0x8007EA3C` | `rcd_get_sector` | 64 |
| `0x8007EA8C` | `rcd_init_cache` | 24 |
| `0x8007EAC0` | `rcd_get_cached_sector` | 32 |
| `0x8007EB5C` | `rcd_close_cache` | 24 |
| `0x8007EB88` | `rcd_load_directory_onstack` | 2088 |
| `0x8007EC50` | `rcd_load_directory` | 32 |
| `0x8007ECB8` | `rCDRealCloseQ` | 0 |
| `0x8007ECC0` | `rCDRealInit` | 24 |
| `0x8007ECE8` | `rCDRealOpen` | 120 |
| `0x8007EDF8` | `rCDRealReadQ` | 48 |

#### RCDWRITE.C (C:\chan\devsys\psx\radlib\SOURCE\RADCD\RCDWRITE.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8009DEF8` | `rCDWrite` | 32 |

#### FREEMEM.C (C:\chan\devsys\psx\radlib\SOURCE\RADMEM\FREEMEM.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8004FDC8` | `rPTraversePool` | 0 |
| `0x8004FE48` | `rPCountFree` | 32 |
| `0x8004FE70` | `rPLargestBlock` | 32 |

#### MALLOC.C (C:\chan\devsys\psx\radlib\SOURCE\RADMEM\MALLOC.C)

| Address | Function | Size |
|---------|----------|------|
| `0x80083F00` | `free` | 24 |

#### RADDMEM.C (C:\chan\devsys\psx\radlib\SOURCE\RADMEM\RADDMEM.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800BDED0` | `rPDMalloc` | 24 |

#### RADFMEM.CPP (C:\chan\devsys\psx\radlib\SOURCE\RADMEM\RADFMEM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8005AAA4` | `rNewFPool` | 32 |
| `0x8005AAFC` | `rNewFPoolBuf` | 0 |
| `0x8005AB80` | `rFOMallocChain` | 24 |
| `0x8005ABB4` | `rFFree` | 0 |
| `0x8005ABC8` | `rDeleteFPool` | 24 |
| `0x8005AC08` | `rFSetOverflowSize` | 0 |
| `0x8005AC10` | `rFExpand` | 32 |

#### RADMEM.CPP (C:\chan\devsys\psx\radlib\SOURCE\RADMEM\RADMEM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80035F8C` | `rMakePuddle` | 0 |
| `0x80035FE0` | `rRemovePuddle(void**, _MEMPUDDLE*)` | 24 |
| `0x80036050` | `rCreateMemPool` | 24 |
| `0x800360A0` | `rDeleteMemPool` | 32 |
| `0x800360F8` | `_rPMalloc` | 24 |
| `0x80036138` | `rPFree` | 24 |
| `0x80036160` | `_rPSMalloc` | 40 |
| `0x800363C4` | `rJoinFreeNodes(void**, _FREENODE*)` | 0 |
| `0x80036418` | `rPSFree` | 32 |

#### RADSMEM.C (C:\chan\devsys\psx\radlib\SOURCE\RADMEM\RADSMEM.C)

| Address | Function | Size |
|---------|----------|------|
| `0x800AE898` | `rPMallocShrink` | 24 |

#### RADZMEM.C (C:\chan\devsys\psx\radlib\SOURCE\RADMEM\RADZMEM.C)

| Address | Function | Size |
|---------|----------|------|
| `0x8008458C` | `rNewZPoolBuf` | 0 |
| `0x800845AC` | `rZMalloc` | 0 |
| `0x800845D8` | `rZCountFree` | 0 |

#### XC3X3MAT.H (C:\devsys\psx\xclib\INDEP\INC\XC3X3MAT.H)

| Address | Function | Size |
|---------|----------|------|
| `0x800C55FC` | `(void, char, __11xc3x3Matrixi)` | 0 |
| `0x800C6098` | `(void, char, __11xc3x3Matrixi)` | 0 |
| `0x800C96D0` | `(void, char, __11xc3x3Matrixi)` | 0 |

#### XCCHAR.CPP (C:\devsys\psx\xclib\INDEP\SRC\XCCHAR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C96E4` | `Is2ByteASCII(const char*)` | 0 |
| `0x800C96EC` | `FindNextChar(const char*, long*, xcUint16Union*)` | 32 |

#### XCFILE.CPP (C:\devsys\psx\xclib\INDEP\SRC\XCFILE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800911C0` | `xcReadFileLow(const char*, void**, unsigned long*)` | 32 |
| `0x80091268` | `xcReadFileHigh(const char*, void**, unsigned long*)` | 32 |

#### XCHASH.CPP (C:\devsys\psx\xclib\INDEP\SRC\XCHASH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80091310` | `xcHash(const char*)` | 0 |

#### XCSORT.CPP (C:\devsys\psx\xclib\INDEP\SRC\XCSORT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BFE64` | `GenFlip32(void*, void*, unsigned long)` | 0 |
| `0x800BFEA0` | `GenShakerSort(void*, unsigned long, unsigned long, void*(*)(void*)*, unsigned long, unsigned long(*)(void*, void*)*, void)` | 56 |

#### XCCIMAGE.CPP (C:\devsys\psx\xclib\psx\SRC\XCCIMAGE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80044DAC` | `_._11xcCellImage` | 32 |

#### XCDO.CPP (C:\devsys\psx\xclib\psx\SRC\XCDO.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AE1CC` | `xcLongWordMemCopy(unsigned long*, unsigned long*, long)` | 0 |
| `0x800AE1F8` | `XCon_DrawResetPrim(void)` | 24 |
| `0x800AE4D8` | `Stub(xcPrimObj*)` | 0 |
| `0x800AE854` | `static_init(XCon_CheckPrimBuffer__FPUcUl)` | 0 |

#### XCFONT.CPP (C:\devsys\psx\xclib\psx\SRC\XCFONT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80091590` | `SortByLetterCode(void*, void*)` | 0 |
| `0x800919BC` | `_._6xcFont` | 32 |
| `0x80091A1C` | `FindItemInTable(unsigned long, const xcSpriteLetter*, int)` | 0 |

#### XCINV.CPP (C:\devsys\psx\xclib\psx\SRC\XCINV.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80091360` | `SortHash(void*, void*)` | 0 |

#### XCSOS.CPP (C:\devsys\psx\xclib\psx\SRC\XCSOS.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8005EB0C` | `_._9xcSection` | 40 |

#### XCVRAM.CPP (C:\devsys\psx\xclib\psx\SRC\XCVRAM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80093B6C` | `_._15xcVRAMAllocator` | 32 |

#### 2PTCAMFLIP.HPP (C:\v11.3\INCLUDE\2PTCAMFLIP.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009DA44` | `_._14t2PointCamFlip` | 24 |

#### CBVANIM.HPP (C:\v11.3\INCLUDE\CBVANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C39CC` | `_._14tCBVAnimLoader` | 24 |

#### CBVPARAM.HPP (C:\v11.3\INCLUDE\CBVPARAM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C3304` | `_._19tCBVParamAnimLoader` | 24 |

#### CHANNEL.HPP (C:\v11.3\INCLUDE\CHANNEL.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B0B3C` | `(thunk_32_Update__15tTransformFlip2P5tTree)` | 24 |
| `0x800B0B68` | `_._9tTreeFlip` | 24 |
| `0x800B0BE0` | `_._18tStatic3DOFKeyList` | 24 |
| `0x800B0C14` | `_._8tKeyList` | 24 |

#### CLUTANIM.HPP (C:\v11.3\INCLUDE\CLUTANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80086858` | `_._15tClutAnimLoader` | 24 |

#### COMPANIM.HPP (C:\v11.3\INCLUDE\COMPANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80086504` | `_._15tCompAnimLoader` | 24 |

#### ETREE.HPP (C:\v11.3\INCLUDE\ETREE.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C0B34` | `_._7tEJoint` | 24 |
| `0x800C0B68` | `_._12tETreeLoader` | 24 |
| `0x800C92E0` | `_._6tETree` | 32 |
| `0x800C93AC` | `_._7tEJoint` | 24 |

#### LITD.HPP (C:\v11.3\INCLUDE\LITD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B7B7C` | `_._9tLitTable` | 24 |

#### LITFARD.HPP (C:\v11.3\INCLUDE\LITFARD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B3660` | `_._12tLitFarTable` | 24 |

#### MTREE.HPP (C:\v11.3\INCLUDE\MTREE.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C451C` | `_._6tMTree` | 32 |

#### PARAMANIM.HPP (C:\v11.3\INCLUDE\PARAMANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80088838` | `_._16tParamAnimLoader` | 24 |

#### SEQUENCE.HPP (C:\v11.3\INCLUDE\SEQUENCE.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80087E40` | `_._19tSequenceAnimLoader` | 24 |

#### STREE.HPP (C:\v11.3\INCLUDE\STREE.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80085A3C` | `_._6tSTree` | 32 |
| `0x80085B00` | `_._7tSJoint` | 24 |
| `0x800892BC` | `_._12tSTreeLoader` | 24 |

#### STREEUNLIT.HPP (C:\v11.3\INCLUDE\STREEUNLIT.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BCCE4` | `_._11tSTreeUnLit` | 32 |

#### TDLIGHT.HPP (C:\v11.3\INCLUDE\TDLIGHT.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C47E4` | `_._17tDirectionalLight` | 24 |

#### TDYNGEOM.HPP (C:\v11.3\INCLUDE\TDYNGEOM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BB30C` | `_._8tDynGeom` | 24 |

#### TEXANIM.HPP (C:\v11.3\INCLUDE\TEXANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80089BB4` | `_._14tTexAnimLoader` | 24 |

#### TGEOLOAD.HPP (C:\v11.3\INCLUDE\TGEOLOAD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80084934` | `_._10tGeoLoader` | 24 |

#### TGEOMTRY.INL (C:\v11.3\INCLUDE\TGEOMTRY.INL)

| Address | Function | Size |
|---------|----------|------|
| `0x800A18CC` | `_._9tGeometry` | 24 |
| `0x800BB368` | `_._9tGeometry` | 24 |

#### TLOADER.HPP (C:\v11.3\INCLUDE\TLOADER.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80084968` | `_._7tLoader` | 24 |
| `0x80086538` | `_._7tLoader` | 24 |
| `0x8008688C` | `_._7tLoader` | 24 |
| `0x80086A64` | `_._7tLoader` | 24 |
| `0x80087E74` | `_._7tLoader` | 24 |
| `0x800881B0` | `_._7tLoader` | 24 |
| `0x8008886C` | `_._7tLoader` | 24 |
| `0x80088DE4` | `_._7tLoader` | 24 |
| `0x800892F0` | `_._7tLoader` | 24 |
| `0x80089BE8` | `_._7tLoader` | 24 |
| `0x800C0BD0` | `_._7tLoader` | 24 |
| `0x800C1F8C` | `_._7tLoader` | 24 |
| `0x800C2620` | `_._7tLoader` | 24 |
| `0x800C3350` | `_._7tLoader` | 24 |
| `0x800C3A24` | `_._7tLoader` | 24 |
| `0x800C4360` | `_._7tLoader` | 24 |

#### TMAT.HPP (C:\v11.3\INCLUDE\TMAT.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80086A3C` | `_._9tMaterial` | 24 |

#### TMATLOAD.HPP (C:\v11.3\INCLUDE\TMATLOAD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80086A08` | `_._10tMatLoader` | 24 |

#### TPRMLOAD.HPP (C:\v11.3\INCLUDE\TPRMLOAD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80088DB0` | `_._11tPrimLoader` | 24 |

#### TRANLOAD.HPP (C:\v11.3\INCLUDE\TRANLOAD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008817C` | `_._16tTranAnimLoader2` | 24 |

#### TREE.HPP (C:\v11.3\INCLUDE\TREE.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80085B34` | `_._10tTreeJoint` | 24 |
| `0x800BB494` | `_._5tTree` | 32 |
| `0x800C0B9C` | `_._10tTreeJoint` | 24 |
| `0x800C93E0` | `_._10tTreeJoint` | 24 |

#### UVANIM.HPP (C:\v11.3\INCLUDE\UVANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C25C8` | `_._13tUVAnimLoader` | 24 |

#### VERTANIM.HPP (C:\v11.3\INCLUDE\VERTANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C1F58` | `_._15tVertAnimLoader` | 24 |

#### VIZANIM.HPP (C:\v11.3\INCLUDE\VIZANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C42F4` | `_._8tVizFlip` | 24 |
| `0x800C432C` | `_._14tVizAnimLoader` | 24 |

#### ZFARD.HPP (C:\v11.3\INCLUDE\ZFARD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B4914` | `_._10tZFarTable` | 24 |

#### ZSORTD.HPP (C:\v11.3\INCLUDE\ZSORTD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B5BA8` | `_._11tZSortTable` | 24 |

#### ANIMATE.CPP (C:\v11.3\SOURCE\ANIMATE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008D260` | `_._10tAnimation` | 24 |
| `0x8008D2C4` | `_._7tPuppet` | 24 |
| `0x8008D32C` | `_._9tFlipbook` | 24 |

#### CBVANIM.CPP (C:\v11.3\SOURCE\CBVANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C33D0` | `_._8tCBVAnim` | 32 |
| `0x800C3578` | `_._8tCBVFlip` | 24 |

#### CBVPARAM.CPP (C:\v11.3\SOURCE\CBVPARAM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C2ADC` | `_._13tCBVParamAnim` | 24 |
| `0x800C2BB0` | `_._13tCBVParamFlip` | 24 |

#### CHANNEL.CPP (C:\v11.3\SOURCE\CHANNEL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AF848` | `_._15tDynamicKeyList` | 32 |
| `0x800AF9B4` | `_._15tJoint3DOFangle` | 32 |
| `0x800AFC80` | `_._15tJoint1DOFangle` | 32 |
| `0x800AFEB4` | `_._15tJoint3DOFlpPSX` | 32 |
| `0x800B0168` | `_._14tTransformAnim` | 32 |
| `0x800B03F8` | `_._15tTransformFlip2` | 32 |

#### CLUTANIM.CPP (C:\v11.3\SOURCE\CLUTANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BB540` | `_._9tClutList` | 32 |
| `0x800BB7D4` | `_._9tClutFlip` | 24 |

#### COMPANIM.CPP (C:\v11.3\SOURCE\COMPANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80085EA4` | `LoadCompositeAnim(tReadChunk&)` | 304 |
| `0x80086028` | `_._18tCompositeAnimPart` | 32 |
| `0x800860C8` | `_._14tCompositeAnim` | 32 |
| `0x80086308` | `_._14tCompositeFlip` | 24 |

#### CYCLE.CPP (C:\v11.3\SOURCE\CYCLE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008D50C` | `p3dFwdCycle(tFlipbook*)` | 32 |

#### ERROR.CPP (C:\v11.3\SOURCE\ERROR.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BB4E8` | `P3DVERIFY(int, char*, N41)` | 0 |

#### ETLOAD.CPP (C:\v11.3\SOURCE\ETLOAD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C0724` | `AddJoint(tReadChunk&, tFile*, tEJoint*)` | 560 |

#### FRUSTRUM.CPP (C:\v11.3\SOURCE\FRUSTRUM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009E218` | `P3DClipCode(unsigned long, unsigned long)` | 0 |
| `0x8009E27C` | `P3DClipCodeSphere(tSphere*)` | 64 |

#### HASH.CPP (C:\v11.3\SOURCE\HASH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80078314` | `p3dHash(const char*)` | 0 |

#### KEYNDOF.CPP (C:\v11.3\SOURCE\KEYNDOF.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BC264` | `_._10tJoint3DOF` | 32 |
| `0x800BC5DC` | `_._10tJoint1DOF` | 32 |

#### LITD.CPP (C:\v11.3\SOURCE\LITD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B5CA4` | `LitDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B5E60` | `LitDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B6068` | `LitDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B624C` | `LitDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B648C` | `LitDisplayG3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B66D4` | `LitDisplayG4(tDynGeom*, tPolygon*, unsigned long)` | 32 |
| `0x800B69B4` | `LitDisplayGT3(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B6C2C` | `LitDisplayGT4(tDynGeom*, tPolygon*, unsigned long)` | 40 |
| `0x800B6F44` | `LitDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B71E4` | `LitDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | 40 |
| `0x800B7530` | `LitDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | 32 |
| `0x800B7800` | `LitDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | 48 |

#### LITFARD.CPP (C:\v11.3\SOURCE\LITFARD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B1570` | `LitFarDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B1750` | `LitFarDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B1998` | `LitFarDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B1BA0` | `LitFarDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B1E18` | `LitFarDisplayG3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B2080` | `LitFarDisplayG4(tDynGeom*, tPolygon*, unsigned long)` | 32 |
| `0x800B239C` | `LitFarDisplayGT3(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B2634` | `LitFarDisplayGT4(tDynGeom*, tPolygon*, unsigned long)` | 40 |
| `0x800B2980` | `LitFarDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B2C40` | `LitFarDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | 40 |
| `0x800B2FC0` | `LitFarDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | 32 |
| `0x800B32B0` | `LitFarDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | 48 |

#### P3DGBL.CPP (C:\v11.3\SOURCE\P3DGBL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80086AE8` | `static_destroy(_3P3D.FrameCount)` | 24 |
| `0x80086B28` | `static_init(_3P3D.FrameCount)` | 24 |

#### P3DINV.CPP (C:\v11.3\SOURCE\P3DINV.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80086BF0` | `_._13tP3Dinventory` | 40 |

#### P3DMATH.CPP (C:\v11.3\SOURCE\P3DMATH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80073018` | `p3dGetElement(unsigned long, unsigned long, const MATRIX*)` | 0 |
| `0x80073054` | `p3dBuildIdentityMatrix(MATRIX*)` | 0 |
| `0x8007307C` | `p3dBuildRotMatrixZ(unsigned short, MATRIX*)` | 32 |
| `0x800730F8` | `p3dBuildRotMatrixXYZ(unsigned short, unsigned short, unsigned short, MATRIX*)` | 96 |
| `0x8007344C` | `p3dBuildRotMatrixZYX(unsigned short, unsigned short, unsigned short, MATRIX*)` | 88 |
| `0x800737A4` | `p3dBuildRotMatrixYZX(unsigned short, unsigned short, unsigned short, MATRIX*)` | 40 |
| `0x80073A00` | `p3dBuildTransMatrix(const _RMVECT16*, MATRIX*)` | 0 |
| `0x80073A3C` | `p3dBuildScaleMatrix(const _RMVECT16*, MATRIX*)` | 0 |
| `0x80073A88` | `p3dBuildScaleMatrix(long, long, long, MATRIX*)` | 0 |
| `0x80073AC8` | `p3dCopyMatrix(const MATRIX*, MATRIX*)` | 0 |
| `0x80073B0C` | `p3dFillTransMatrix(const _RMVECT16*, MATRIX*)` | 0 |
| `0x80073B30` | `p3dFillTransMatrix(long, long, long, MATRIX*)` | 0 |
| `0x80073B40` | `p3dGetTransMatrix(const MATRIX*, _RMVECT16*)` | 0 |
| `0x80073B64` | `p3dInverseOrthMatrix(const MATRIX*, MATRIX*)` | 40 |
| `0x80073C8C` | `p3dFillHeadingMatrix(const _RMVECT16*, const _RMVECT16*, MATRIX*)` | 80 |
| `0x80073D9C` | `p3dMultMatrix(const MATRIX*, const MATRIX*, MATRIX*)` | 32 |
| `0x80073E78` | `p3dPreMultMatrix(const MATRIX*, MATRIX*)` | 32 |
| `0x80073F50` | `p3dPosMultMatrix(const MATRIX*, MATRIX*)` | 24 |
| `0x8007401C` | `p3dVecTimesMatrix(const _RMVECT16*, const MATRIX*, _RMVECT16*)` | 0 |
| `0x8007411C` | `p3dVecTimesMatrix(_RMVECT16*, const MATRIX*)` | 0 |
| `0x8007421C` | `p3dVecTimesRotMatrix(const _RMVECT16*, const MATRIX*, _RMVECT16*)` | 0 |
| `0x80074304` | `p3dVecTimesRotMatrix(_RMVECT16*, const MATRIX*)` | 0 |

#### PARAMANIM.CPP (C:\v11.3\SOURCE\PARAMANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BC118` | `_._10tParamAnim` | 32 |

#### PARAMFLIP.CPP (C:\v11.3\SOURCE\PARAMFLIP.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AF718` | `_._10tParamFlip` | 32 |

#### PARAMLOAD.CPP (C:\v11.3\SOURCE\PARAMLOAD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800881E4` | `ParseParam(tFile*, tReadChunk&, int*)` | 208 |
| `0x80088570` | `LoadParamAnim(tReadChunk&, tParamAnim*(*)()*, tFlipbook*)` | 344 |

#### PORTLITE.CPP (C:\v11.3\SOURCE\PORTLITE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009F97C` | `PopMatrixAndLights(void)` | 24 |
| `0x8009F9C0` | `PushMatrixAndLights(void)` | 24 |
| `0x8009FA08` | `MultMatrixAndLights(MATRIX*)` | 24 |
| `0x8009FA38` | `RotMatrixXYZAndLights(unsigned short, unsigned short, unsigned short)` | 32 |
| `0x8009FAFC` | `RotMatrixXAndLights(unsigned short)` | 24 |
| `0x8009FB44` | `RotMatrixYAndLights(unsigned short)` | 24 |
| `0x8009FB8C` | `RotMatrixZAndLights(unsigned short)` | 24 |
| `0x8009FBD4` | `RotMatrixYZXAndLights(unsigned short, unsigned short, unsigned short)` | 32 |

#### PORTMATH.CPP (C:\v11.3\SOURCE\PORTMATH.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80053B1C` | `PopMatrixNoLights(void)` | 24 |
| `0x80053B50` | `PushMatrixNoLights(void)` | 24 |
| `0x80053B88` | `MultMatrixNoLights(MATRIX*)` | 24 |
| `0x80053BB0` | `RotMatrixXYZNoLights(unsigned short, unsigned short, unsigned short)` | 32 |
| `0x80053C5C` | `RotMatrixYZXNoLights(unsigned short, unsigned short, unsigned short)` | 32 |
| `0x80053D08` | `RotMatrixXNoLights(unsigned short)` | 24 |
| `0x80053D48` | `RotMatrixYNoLights(unsigned short)` | 24 |
| `0x80053D88` | `RotMatrixZNoLights(unsigned short)` | 24 |

#### POSE.CPP (C:\v11.3\SOURCE\POSE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B0D24` | `_._5tPose` | 32 |

#### RPSTREE.CPP (C:\v11.3\SOURCE\RPSTREE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C2654` | `RP_XformVerts(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | 0 |
| `0x800C26BC` | `RP_FixUpPolys(tPrimGeom*, void*, unsigned long, unsigned long)` | 0 |

#### RPSTREECOL.CPP (C:\v11.3\SOURCE\RPSTREECOL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80084F14` | `RP_XformVertsLitCBF_CL(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | 0 |
| `0x8008500C` | `RP_FixUpPolysCBF_CL(tPrimGeom*, void*, unsigned long, unsigned long)` | 64 |

#### RPSTREEFLAT.CPP (C:\v11.3\SOURCE\RPSTREEFLAT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C3A58` | `RP_FixUpPolysFlat(tPrimGeom*, void*, unsigned long, unsigned long)` | 0 |

#### RPSTREELIT.CPP (C:\v11.3\SOURCE\RPSTREELIT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C0C04` | `RP_XformVertsLit(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | 8 |
| `0x800C0D58` | `RP_XformVertsLitCBF(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | 0 |
| `0x800C0E20` | `RP_FixUpPolysCBF(tPrimGeom*, void*, unsigned long, unsigned long)` | 32 |

#### RPSTREENLT.CPP (C:\v11.3\SOURCE\RPSTREENLT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C1450` | `RP_XformVertsNoLit(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | 0 |
| `0x800C14BC` | `RP_FixUpPolysNoLit(tPrimGeom*, void*, unsigned long, unsigned long)` | 0 |
| `0x800C1940` | `RP_FixUpPolysNoLitFlat(tPrimGeom*, void*, unsigned long, unsigned long)` | 0 |

#### RPZCULL.CPP (C:\v11.3\SOURCE\RPZCULL.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A0E14` | `RP_ZCullGClip(tGeometry*)` | 24 |

#### RPZFOG.CPP (C:\v11.3\SOURCE\RPZFOG.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A18F4` | `RP_ZCullGMFog(tGeometry*)` | 24 |

#### RQUEUE.CPP (C:\v11.3\SOURCE\RQUEUE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A003C` | `DSCallback(...)` | 32 |
| `0x800A0140` | `VSCallback(...)` | 32 |

#### SEQUENCE.CPP (C:\v11.3\SOURCE\SEQUENCE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80087B9C` | `_._13tSequenceAnim` | 32 |
| `0x80087D1C` | `_._13tSequenceFlip` | 24 |

#### STLOAD.CPP (C:\v11.3\SOURCE\STLOAD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80088E18` | `AddJoint(tReadChunk&, tFile*, tSJoint*, void**)` | 320 |

#### T2POINTCAM.CPP (C:\v11.3\SOURCE\T2POINTCAM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009D438` | `_._19t2PointMatrixCamera` | 24 |

#### TCACHE.CPP (C:\v11.3\SOURCE\TCACHE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BBDA0` | `_._6tCache` | 24 |
| `0x800BBE68` | `_._9tInvCache` | 24 |

#### TCAMERA.CPP (C:\v11.3\SOURCE\TCAMERA.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008EC98` | `_._7tCamera` | 24 |

#### TCHUNK.CPP (C:\v11.3\SOURCE\TCHUNK.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B128C` | `_._6tChunk` | 24 |
| `0x800B1324` | `_._10tReadChunk` | 24 |

#### TDTABLE.CPP (C:\v11.3\SOURCE\TDTABLE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B49A0` | `_._10tDrawTable` | 24 |

#### TENTITY.CPP (C:\v11.3\SOURCE\TENTITY.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008D1A4` | `_._7tEntity` | 24 |

#### TEXANIM.CPP (C:\v11.3\SOURCE\TEXANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BCDE4` | `_._8tTexFlip` | 24 |
| `0x800BCFCC` | `_._8tTexList` | 32 |

#### TFILE.CPP (C:\v11.3\SOURCE\TFILE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B7BD8` | `_._5tFile` | 32 |
| `0x800B7EF8` | `_._11tByteStream` | 24 |
| `0x800B7F90` | `_._14tMemByteStream` | 32 |

#### TIDXLIST.CPP (C:\v11.3\SOURCE\TIDXLIST.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BB9E8` | `_._10tIndexList` | 32 |

#### TINVNTRY.CPP (C:\v11.3\SOURCE\TINVNTRY.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008E960` | `_._10tInventory` | 32 |

#### TLAYER.CPP (C:\v11.3\SOURCE\TLAYER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A031C` | `_._6tLayer` | 32 |
| `0x800A053C` | `_._12tDoubleLayer` | 32 |
| `0x800A0728` | `get_first_ot_prim(void*)` | 0 |
| `0x800A0750` | `get_next_ot_prim(unsigned long*)` | 0 |
| `0x800A0778` | `DrawFrame(unsigned long*)` | 0 |

#### TLIGHT.CPP (C:\v11.3\SOURCE\TLIGHT.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C4A04` | `_._6tLight` | 24 |

#### TLOADER.CPP (C:\v11.3\SOURCE\TLOADER.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80085B68` | `P3DLoad(tLoader**, void*, tLoader**)` | 392 |

#### TMATRIXCAM.CPP (C:\v11.3\SOURCE\TMATRIXCAM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009D648` | `_._13tMatrixCamera` | 24 |

#### TPRIMGEO.CPP (C:\v11.3\SOURCE\TPRIMGEO.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A14C4` | `_._9tPrimGeom` | 32 |

#### TTEXTURE.CPP (C:\v11.3\SOURCE\TTEXTURE.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800BC05C` | `_._8tTexture` | 32 |

#### TVIEW.CPP (C:\v11.3\SOURCE\TVIEW.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800522AC` | `_._5tView` | 32 |

#### UVANIM.CPP (C:\v11.3\SOURCE\UVANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C200C` | `_._7tUVAnim` | 32 |
| `0x800C21E0` | `_._7tUVFlip` | 24 |

#### VERTANIM.CPP (C:\v11.3\SOURCE\VERTANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8009DFA4` | `_._10tFrameList` | 24 |
| `0x8009E0E4` | `_._11tVertexFlip` | 24 |

#### VIZANIM.CPP (C:\v11.3\SOURCE\VIZANIM.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800C3E44` | `LoadVizAnim(tReadChunk&, unsigned short)` | 208 |
| `0x800C40DC` | `_._8tVizAnim` | 32 |

#### ZFARD.CPP (C:\v11.3\SOURCE\ZFARD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B373C` | `ZFarDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B390C` | `ZFarDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B3B04` | `ZFarDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B3D3C` | `ZFarDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B3F94` | `ZFarDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B4198` | `ZFarDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B43CC` | `ZFarDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B4654` | `ZFarDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | 32 |

#### ZSORTD.CPP (C:\v11.3\SOURCE\ZSORTD.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800B4B18` | `ZSortDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B4CC8` | `ZSortDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B4ECC` | `ZSortDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B50A4` | `ZSortDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B52C8` | `ZSortDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B54BC` | `ZSortDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | 24 |
| `0x800B5708` | `ZSortDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | 16 |
| `0x800B591C` | `ZSortDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | 32 |

#### RADMOVX.CPP (L:\RTOOLS\RadMovie2\Src\RadMovie\RADMOVX.CPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80016488` | `rMvInit` | 24 |
| `0x80016510` | `rMvTerm` | 24 |
| `0x80016588` | `rMvOpenMovie` | 32 |
| `0x80016770` | `rMvCloseMovie` | 24 |
| `0x80016808` | `rMvPlay` | 56 |
| `0x80016E84` | `rMvStop` | 40 |
| `0x800170E0` | `rMvGetState` | 32 |
| `0x80017164` | `rMvSetAudio` | 40 |
| `0x800172E0` | `FrameReadyCallback(...)` | 24 |
| `0x80017310` | `DCTCallback(...)` | 24 |
| `0x8001735C` | `CDReadyCallback(unsigned char, unsigned char*)` | 32 |
| `0x800173D0` | `VSynchCallback(...)` | 24 |
| `0x80017418` | `DrawSynchCallback(...)` | 24 |
| `0x8001747C` | `MovieEventHandler(void)` | 72 |
| `0x80017D2C` | `KickStartHdFrame(void)` | 24 |
| `0x80017D70` | `GetFrame(unsigned long**, StHEADER**)` | 64 |
| `0x80017F24` | `FreeFrame(unsigned long*)` | 24 |

#### TLOADER.HPP (\CHAN\DEVSYS\PSX\PURE3D\INCLUDE\TLOADER.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8003BAD8` | `_._7tLoader` | 24 |
| `0x80087A10` | `_._7tLoader` | 24 |
| `0x80089890` | `_._7tLoader` | 24 |
| `0x8009AC24` | `_._7tLoader` | 24 |

#### TTEXLOAD.HPP (\CHAN\DEVSYS\PSX\PURE3D\INCLUDE\TTEXLOAD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80087974` | `_._10tTexLoader` | 24 |

#### GENERATOR.HPP (\CHAN\GAME\INC\AI\GENERATOR.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001224C` | `_._17ThrowingGenerator` | 24 |
| `0x80012274` | `_._14EnemyGenerator` | 24 |

#### FEMNUMGR.H (\CHAN\GAME\INC\FE\FEMNUMGR.H)

| Address | Function | Size |
|---------|----------|------|
| `0x80013264` | `_._21hdControllerSelection` | 24 |
| `0x80013284` | `_._14GameOverScreen` | 24 |
| `0x800132A4` | `_._11TitleScreen` | 24 |

#### HDITEM.H (\CHAN\GAME\INC\FE\HDITEM.H)

| Address | Function | Size |
|---------|----------|------|
| `0x80090DFC` | `_._8hdDragon` | 24 |
| `0x80090E24` | `_._13hdAnimTextOvl` | 24 |
| `0x80090E4C` | `_._8hdTtlive` | 24 |

#### HDMENU.H (\CHAN\GAME\INC\FE\HDMENU.H)

| Address | Function | Size |
|---------|----------|------|
| `0x8005E3BC` | `_._9hdDynMenu` | 24 |
| `0x8005E3DC` | `_._13hdDynItemMenu` | 24 |
| `0x8005E40C` | `_._16hdShockSelection` | 24 |
| `0x8005E434` | `_._18hdDynItemSelection` | 24 |
| `0x8005E474` | `_._18hdSndItemSelection` | 24 |
| `0x8005E49C` | `_._15hdItemSelection` | 24 |
| `0x8005E4BC` | `_._16hdAlphaSelection` | 24 |
| `0x8005E4DC` | `_._18hdNumericSelection` | 24 |
| `0x8005E4FC` | `_._13hdDynItemGoto` | 24 |
| `0x8005E51C` | `_._15hdDynItemButton` | 24 |
| `0x8005E53C` | `_._12hdItemButton` | 24 |

#### OXSCRMGR.H (\CHAN\GAME\INC\FE\OXSCRMGR.H)

| Address | Function | Size |
|---------|----------|------|
| `0x8002CB98` | `_._10oxFontFile` | 24 |

#### ANCHOR.HPP (\CHAN\GAME\INC\GEN\ANCHOR.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004B57C` | `_._10DataAnchor` | 32 |
| `0x8005A008` | `_._10DataAnchor` | 32 |

#### BLKMGR.HPP (\CHAN\GAME\INC\GEN\BLKMGR.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80051950` | `_._9BlockList` | 32 |
| `0x800519A0` | `_._9BlockNode` | 32 |

#### CALLBACK.HPP (\CHAN\GAME\INC\GEN\CALLBACK.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004B4DC` | `_._8Callback` | 32 |
| `0x800518FC` | `_._8Callback` | 32 |
| `0x8005408C` | `_._8Callback` | 32 |
| `0x80059EF0` | `_._8Callback` | 32 |

#### CCLIST.HPP (\CHAN\GAME\INC\GEN\CCLIST.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002CCF4` | `_._6ccList` | 32 |
| `0x8002CD98` | `_._9ccMinList` | 32 |
| `0x80039504` | `_._6ccList` | 32 |
| `0x800395A8` | `_._9ccMinList` | 32 |
| `0x8003F31C` | `_._6ccList` | 32 |
| `0x8003F3C0` | `_._9ccMinList` | 32 |
| `0x8004710C` | `_._6ccList` | 32 |
| `0x800471B0` | `_._9ccMinList` | 32 |
| `0x8004B5D4` | `_._6ccList` | 32 |
| `0x8004B678` | `_._9ccMinList` | 32 |
| `0x8004CB44` | `_._6ccList` | 32 |
| `0x8004CBE8` | `_._9ccMinList` | 32 |
| `0x80051A48` | `_._9ccMinList` | 32 |
| `0x80056FE8` | `_._6ccList` | 32 |
| `0x8005708C` | `_._9ccMinList` | 32 |
| `0x80057344` | `_._6ccList` | 32 |
| `0x800573E8` | `_._9ccMinList` | 32 |
| `0x80059398` | `_._6ccList` | 32 |
| `0x8005943C` | `_._9ccMinList` | 32 |
| `0x8005A060` | `_._6ccList` | 32 |
| `0x8005A104` | `_._9ccMinList` | 32 |
| `0x8005E6A4` | `_._9ccMinList` | 32 |
| `0x80060608` | `_._9ccMinList` | 32 |
| `0x80062940` | `_._6ccList` | 32 |
| `0x800629E4` | `_._9ccMinList` | 32 |
| `0x8007A64C` | `_._6ccList` | 32 |
| `0x8007A6F0` | `_._9ccMinList` | 32 |
| `0x8008D09C` | `_._6ccList` | 32 |
| `0x8008D140` | `_._9ccMinList` | 32 |
| `0x8008DA38` | `_._6ccList` | 32 |
| `0x8008DADC` | `_._9ccMinList` | 32 |
| `0x8008E810` | `_._6ccList` | 32 |
| `0x8008E8B4` | `_._9ccMinList` | 32 |
| `0x800981CC` | `_._6ccList` | 32 |
| `0x80098270` | `_._9ccMinList` | 32 |
| `0x8009C5D8` | `_._6ccList` | 32 |
| `0x8009C67C` | `_._9ccMinList` | 32 |
| `0x800A44A4` | `_._9ccMinList` | 32 |
| `0x800A61FC` | `_._9ccMinList` | 32 |
| `0x800A7A8C` | `_._6ccList` | 32 |
| `0x800A7B30` | `_._9ccMinList` | 32 |

#### CHARMGR.HPP (\CHAN\GAME\INC\GEN\CHARMGR.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8003463C` | `_._15CharMgrCallback` | 24 |
| `0x8003BA98` | `_._15CharMgrCallback` | 24 |
| `0x800470CC` | `_._15CharMgrCallback` | 24 |
| `0x8004A508` | `_._15CharMgrCallback` | 24 |
| `0x80056EC4` | `_._15CharMgrCallback` | 24 |
| `0x800782D4` | `_._15CharMgrCallback` | 24 |
| `0x80095404` | `_._15CharMgrCallback` | 24 |

#### DATABASE.HPP (\CHAN\GAME\INC\GEN\DATABASE.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80010D2C` | `_._6DBRoot` | 24 |
| `0x8001229C` | `_._6DBRoot` | 24 |
| `0x8001F67C` | `_._6DBRoot` | 24 |
| `0x80039314` | `_._6DBMesh` | 32 |
| `0x80039374` | `_._6DBPath` | 32 |
| `0x800393D8` | `_._6DBLine` | 32 |
| `0x8003943C` | `_._12DBLineVertex` | 24 |
| `0x80039464` | `_._8DBVolume` | 24 |
| `0x8003948C` | `_._8DBSphere` | 24 |
| `0x800394B4` | `_._7DBPoint` | 24 |
| `0x800394DC` | `_._6DBRoot` | 24 |
| `0x800A4428` | `_._6DBRoot` | 24 |

#### DEADPOOL.HPP (\CHAN\GAME\INC\GEN\DEADPOOL.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008D4D8` | `_._8DeadPool` | 24 |

#### DRAWTABL.HPP (\CHAN\GAME\INC\GEN\DRAWTABL.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8007D70C` | `_._16tChanLitFarTable` | 24 |
| `0x8007D77C` | `_._13tChanLitTable` | 24 |
| `0x8007D7FC` | `_._14tChanZFarTable` | 24 |
| `0x8007D87C` | `_._15tChanZSortTable` | 24 |

#### HNDLRSET.HPP (\CHAN\GAME\INC\GEN\HNDLRSET.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8002CC50` | `_._10HandlerSet` | 32 |
| `0x8002CCAC` | `_._7Handler` | 32 |
| `0x8002E858` | `_._7Handler` | 32 |
| `0x8003F278` | `_._10HandlerSet` | 32 |
| `0x8003F2D4` | `_._7Handler` | 32 |
| `0x800403D8` | `_._7Handler` | 32 |
| `0x80044A54` | `_._7Handler` | 32 |
| `0x8004D7E8` | `_._7Handler` | 32 |
| `0x8004D9FC` | `_._7Handler` | 32 |
| `0x80056E7C` | `_._7Handler` | 32 |

#### HUD.HPP (\CHAN\GAME\INC\GEN\HUD.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8004033C` | `(thunk_48__._3HUD)` | 24 |
| `0x8004035C` | `(thunk_48_UpdateScreen__3HUDP15oxScreenManager)` | 24 |
| `0x8004037C` | `_._11ErrorScreen` | 24 |

#### ITEMNODE.HPP (\CHAN\GAME\INC\GEN\ITEMNODE.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800AD76C` | `_._8ItemNode` | 24 |

#### LIGHTS.HPP (\CHAN\GAME\INC\GEN\LIGHTS.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800A43A4` | `_._7DBLight` | 24 |
| `0x800A43D8` | `_._14DBHLightVolume` | 24 |
| `0x800A4400` | `_._14DBColourVolume` | 24 |

#### LOADERS.HPP (\CHAN\GAME\INC\GEN\LOADERS.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800879A8` | `_._23tChanSequenceAnimLoader` | 24 |
| `0x800879DC` | `_._11tGameLoader` | 24 |

#### MODEL.HPP (\CHAN\GAME\INC\GEN\MODEL.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80072450` | `_._18AnimStructureBasic` | 24 |
| `0x80072478` | `_._13DrawableBasic` | 24 |
| `0x8009ABFC` | `_._18AnimStructureBasic` | 24 |

#### PATH.HPP (\CHAN\GAME\INC\GEN\PATH.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8001CA3C` | `_._4Path` | 40 |
| `0x80024D0C` | `_._4Path` | 40 |
| `0x80056F04` | `_._4Path` | 40 |
| `0x8006D124` | `_._4Path` | 40 |
| `0x800A5F94` | `_._10SplinePath` | 40 |
| `0x800A60C4` | `_._10LinearPath` | 40 |
| `0x800A624C` | `_._4Path` | 40 |
| `0x800BED58` | `_._4Path` | 40 |

#### SCOREMGR.HPP (\CHAN\GAME\INC\GEN\SCOREMGR.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8003467C` | `_._14CheckpointInfo` | 24 |

#### SOUND.HPP (\CHAN\GAME\INC\GEN\SOUND.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80059F44` | `_._11SoundAnchor` | 32 |

#### SWITCH.HPP (\CHAN\GAME\INC\GEN\SWITCH.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80095444` | `_._15WDBSphereSwitch` | 24 |
| `0x8009546C` | `_._15WDBVolumeSwitch` | 24 |

#### TRAIL.HPP (\CHAN\GAME\INC\GEN\TRAIL.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8007A624` | `_._9TrailInfo` | 24 |

#### WORLDPTS.HPP (\CHAN\GAME\INC\GEN\WORLDPTS.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x8008D994` | `_._11WorldPoints` | 32 |
| `0x8008D9F0` | `_._13WorldParPoint` | 24 |
| `0x8008DA10` | `_._12World3DPoint` | 24 |

#### MOVIES.HPP (\CHAN\GAME\INC\PSX\MOVIES.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x80014854` | `_._9MoviePlay` | 24 |
| `0x80014888` | `_._11MovieAction` | 24 |

#### RAMTEXANIM.HPP (\CHAN\GAME\INC\PSX\RAMTEXANIM.HPP)

| Address | Function | Size |
|---------|----------|------|
| `0x800897DC` | `_._17tRAMTexAnimLoader` | 24 |

---

## VTable Addresses

Useful for identifying object types at runtime.

| Class | VTable Address |
|-------|---------------|
| AI | `0x800CD158` |
| ActiveZone | `0x800D3E94` |
| AmbientLight | `0x800D3C34` |
| AnimCallback | `0x800CC2A0` |
| AnimStructure | `0x800CE3AC` |
| AnimationManager | `0x800CD18C` |
| Arrow | `0x80025B94` |
| Behaviour | `0x800CE4AC` |
| BehaviourAttrib | `0x800CE4BC` |
| Blast | `0x80019838` |
| BlockManager | `0x800CCFEC` |
| Boss | `0x800215A0` |
| Butch | `0x80021488` |
| CBVEffect | `0x800D33A4` |
| CDestructibleSound | `0x800D4324` |
| CDirectorSound | `0x800D3538` |
| CFrontEndSound | `0x800CD040` |
| CGenericPersistentSound | `0x800D4310` |
| CGenericTransientSound | `0x800D40C8` |
| CHumanoidSound | `0x800CD900` |
| CKickNRollSound | `0x800D43B4` |
| CKnockDownSound | `0x800D43C8` |
| CParticleEffectSound | `0x800D438C` |
| CPendulumSound | `0x800D4360` |
| CPlatformSound | `0x800D434C` |
| CPushableSound | `0x800D4338` |
| CSound | `0x800D3BA8` |
| CWeaponSound | `0x800D43A0` |
| CWorldEffectSound | `0x800D4374` |
| Camera | `0x800CCCB8` |
| CameraAnchor | `0x800CCD44` |
| CameraManager | `0x800CCD2C` |
| Chair | `0x80019644` |
| CharacterManager | `0x800CC2B0` |
| Collectible | `0x80019528` |
| Control | `0x800CB630` |
| Conveyor | `0x80025C0C` |
| Crusher | `0x80025E18` |
| DBCameraPath | `0x800CCD6C` |
| DBCameraPathNode | `0x800CCD5C` |
| Dante | `0x80021250` |
| Database | `0x800CC1B8` |
| DestructibleThing | `0x800192A0` |
| Director | `0x800CC318` |
| Display | `0x800CCF34` |
| Door | `0x80025AFC` |
| DrawableETree | `0x800CE3DC` |
| DrawableGeo | `0x800CE3CC` |
| DrawableSTree | `0x800CE3EC` |
| DrawableTree | `0x800CE3FC` |
| DynamicObstacle | `0x80019744` |
| DynamicThing | `0x800CD99C` |
| EModel | `0x800CE26C` |
| Effects | `0x800CCEA8` |
| EnemyGenerator | `0x800193A4` |
| EnvironmentManager | `0x800CD0C8` |
| ErrorScreen | `0x800CC6B4` |
| Explosive | `0x800195A0` |
| FPWEffect | `0x800D3908` |
| FWEffect | `0x800D33C4` |
| FrontEndVolume | `0x80025A84` |
| GEffect | `0x800D34E0` |
| GModel | `0x800CE2BC` |
| Game | `0x800CB4D0` |
| GameOverScreen | `0x80010238` |
| Generator | `0x80019424` |
| Grontar | `0x80021370` |
| HUD | `0x800CC6EC` |
| HardwareLight | `0x800D3C24` |
| HorizontalPole | `0x800197C0` |
| Humanoid | `0x800CDA80` |
| HumanoidModel | `0x800CE180` |
| InputManager | `0x800CB618` |
| KickNRoll | `0x80025C94` |
| KnockDown | `0x80025D94` |
| Ladder | `0x800D332C` |
| Launcher | `0x80025E90` |
| LensFlare | `0x800D4B48` |
| LevelManager | `0x800CD478` |
| LightAnchor | `0x800D3BE4` |
| LightingClass | `0x800D3BD4` |
| LineFile | `0x800108FC` |
| LinearPath | `0x800D3CC0` |
| Manager | `0x800CB68C` |
| MenuMgr | `0x800CD864` |
| Model | `0x800CE30C` |
| MovieRandom | `0x8001037C` |
| Obstacle | `0x800CE7A0` |
| OriginalBasic | `0x800CE39C` |
| OriginalETree | `0x800CE36C` |
| OriginalGeo | `0x800CE38C` |
| OriginalSTree | `0x800CE35C` |
| OriginalTree | `0x800CE37C` |
| Oscar | `0x80021020` |
| PWEffect | `0x800D3928` |
| ParticleInfo | `0x800D386C` |
| ParticleSystem | `0x800D385C` |
| PathInfo | `0x800D4B14` |
| Paul | `0x80021138` |
| Pendulum | `0x80025FA4` |
| Pickup | `0x800CE08C` |
| Platform | `0x80025F08` |
| Player | `0x800CBB08` |
| PlayerModel | `0x800CE5E4` |
| Pushable | `0x8001993C` |
| SModel | `0x800CE20C` |
| ScoreManager | `0x800CCEF4` |
| Shadow | `0x800D4408` |
| SimpleShadow | `0x800D43E8` |
| SlipperyFloor | `0x800194B0` |
| Sound | `0x800CD4D4` |
| SplinePath | `0x800D3C98` |
| SpotLight | `0x800D4AF8` |
| Stack | `0x80025D1C` |
| SubZoneVolume | `0x800D3EA4` |
| Table | `0x800196C4` |
| Teleporter | `0x800D4050` |
| Thing | `0x800CD9F4` |
| ThrowingGenerator | `0x80019324` |
| Ticket | `0x800CDA44` |
| Time | `0x800CCA24` |
| TitleScreen | `0x80010260` |
| Trails | `0x800CE6F4` |
| TrapDoor | `0x800198C4` |
| TreeShadow | `0x800D43F8` |
| TriggerThing | `0x800D3FD8` |
| Untouchable | `0x800D3D34` |
| WDBSphereSwitch | `0x800D37E4` |
| WDBSwitch | `0x800D3814` |
| WDBVolumeSwitch | `0x800D37FC` |
| WEffect | `0x800D33E4` |
| World | `0x800CCA8C` |
| ccFile | `0x800CCE58` |
| ccMinNode | `0x800CC0D8` |
| ccNode | `0x800CC0C8` |
| feMenuMgr | `0x800101E0` |
| gameMenu | `0x800CC0F4` |
| hdAlphaSelection | `0x800CD6A0` |
| hdAnimTextOvl | `0x800D35B4` |
| hdControllerSelection | `0x80010188` |
| hdDragon | `0x800D359C` |
| hdDynItemButton | `0x800CD770` |
| hdDynItemGoto | `0x800CD710` |
| hdDynItemMenu | `0x800CD590` |
| hdDynMenu | `0x800CD568` |
| hdHealth | `0x800D35E4` |
| hdHits | `0x800D358C` |
| hdItemButton | `0x800CD7A0` |
| hdItemGoto | `0x800CD740` |
| hdItemSelection | `0x800CD670` |
| hdMemCardMenu | `0x800101B8` |
| hdMenu | `0x800CD5B8` |
| hdMenuItem | `0x800CD7D0` |
| hdNumericSelection | `0x800CD6D8` |
| hdShockSelection | `0x800CD5E0` |
| hdSndItemSelection | `0x800CD640` |
| hdTextOvl | `0x800D35CC` |
| hdTtlive | `0x800D35FC` |
| nisCharMgrCallback | `0x800CE5C4` |
| oxOvl | `0x800D363C` |
| oxScreenManager | `0x800CC744` |
| rsdAmbiance | `0x800D2D08` |
| rsdClip | `0x800D2DCC` |
| rsdMusicPlayer | `0x800D2CF8` |
| rsdStream | `0x800D4894` |
| t2PointCamFlip | `0x800D3994` |
| t2PointMatrixCamera | `0x800D3964` |
| tAnimation | `0x800D3470` |
| tCBVAnim | `0x800D4CC4` |
| tCBVAnimLoader | `0x800D4C8C` |
| tCBVFlip | `0x800D4C9C` |
| tCBVParamAnim | `0x800D4C64` |
| tCBVParamAnimLoader | `0x800D4C2C` |
| tCBVParamFlip | `0x800D4C3C` |
| tCache | `0x800D49F4` |
| tCamera | `0x800D354C` |
| tChanSequenceAnimLoader | `0x800D307C` |
| tChunk | `0x800D4700` |
| tClutAnimLoader | `0x800D2FEC` |
| tClutFlip | `0x800D4974` |
| tClutList | `0x800D499C` |
| tCompAnimLoader | `0x800D2F6C` |
| tCompositeAnim | `0x800D2F4C` |
| tCompositeFlip | `0x800D2F24` |
| tDirectionalLight | `0x800D4D60` |
| tDoubleLayer | `0x800D3A90` |
| tDrawTable | `0x800D473C` |
| tDynGeom | `0x800D48F0` |
| tDynamicKeyList | `0x800D4560` |
| tETree | `0x800D5068` |
| tETreeLoader | `0x800D4B74` |
| tEntity | `0x800D3420` |
| tFile | `0x800D47AC` |
| tFlipbook | `0x800D3430` |
| tFrameList | `0x800D3A68` |
| tGameLoader | `0x800D308C` |
| tGeoLoader | `0x800D2EB8` |
| tIndexList | `0x800D49CC` |
| tInvCache | `0x800D49DC` |
| tInventory | `0x800D351C` |
| tJoint1DOF | `0x800D4A28` |
| tJoint1DOFangle | `0x800D4530` |
| tJoint3DOF | `0x800D4A40` |
| tJoint3DOFangle | `0x800D4548` |
| tJoint3DOFlpPSX | `0x800D4518` |
| tLayer | `0x800D3AB8` |
| tLight | `0x800D4D74` |
| tLitFarTable | `0x800D470C` |
| tLitTable | `0x800D476C` |
| tMTree | `0x800D4D4C` |
| tMatLoader | `0x800D300C` |
| tMatrixCamera | `0x800D397C` |
| tMemByteStream | `0x800D4784` |
| tP3Dinventory | `0x800D3050` |
| tParamAnim | `0x800D4A10` |
| tParamAnimLoader | `0x800D3134` |
| tParamFlip | `0x800D4460` |
| tPose | `0x800D45A4` |
| tPrimGeom | `0x800D3B4C` |
| tPrimLoader | `0x800D31DC` |
| tPuppet | `0x800D3458` |
| tRAMTexAnim | `0x800D325C` |
| tRAMTexAnimLoader | `0x800D3224` |
| tRAMTexFlip | `0x800D3234` |
| tReadChunk | `0x800D46F0` |
| tSTree | `0x800D2ED8` |
| tSTreeLoader | `0x800D31FC` |
| tSTreeUnLit | `0x800D4A54` |
| tSequenceAnim | `0x800D30D4` |
| tSequenceAnimLoader | `0x800D30F4` |
| tSequenceFlip | `0x800D30AC` |
| tStatic3DOFKeyList | `0x800D4578` |
| tTexAnimLoader | `0x800D330C` |
| tTexFlip | `0x800D4AB0` |
| tTexList | `0x800D4A80` |
| tTexLoader | `0x800D306C` |
| tTexture | `0x800D4A00` |
| tTranAnimLoader2 | `0x800D3114` |
| tTransformAnim | `0x800D4500` |
| tTransformFlip2 | `0x800D44D0` |
| tTree | `0x800D494C` |
| tTreeFlip | `0x800D4488` |
| tUVAnim | `0x800D4C04` |
| tUVAnimLoader | `0x800D4BCC` |
| tUVFlip | `0x800D4BDC` |
| tVertAnimLoader | `0x800D4BAC` |
| tVertexFlip | `0x800D3A40` |
| tView | `0x800CD030` |
| tVizAnim | `0x800D4D14` |
| tVizAnimLoader | `0x800D4D2C` |
| tVizFlip | `0x800D4CEC` |
| tZFarTable | `0x800D4724` |
| tZSortTable | `0x800D4754` |

---

## Original Source Tree

Reconstructed from debug symbol source file references.

```
C:\CHAN\GAME\SRC\AI/
    ACTIVEZN.CPP
    AI.CPP
    ARROW.CPP
    BEHAVE.CPP
    BEHAVEB.CPP
    BLAST.CPP
    BOSS.CPP
    COLLECT.CPP
    COMINTER.CPP
    CONVEYOR.CPP
    CRUSHER.CPP
    DESTROY.CPP
    DOOR.CPP
    EXPLODE.CPP
    FEVOLUME.CPP
    FIGHTANI.CPP
    GENERATOR.CPP
    HPOLE.CPP
    HUMANOID.CPP
    HUMNDATA.CPP
    KICK.CPP
    KNOCKDWN.CPP
    LADDER.CPP
    LAUNCHER.CPP
    OBSTACLE.CPP
    PENDULUM.CPP
    PICKUP.CPP
    PLATFORM.CPP
    PLAYER.CPP
    PUSHABLE.CPP
    SIMPLBOX.CPP
    SLIPPERY.CPP
    TABLE.CPP
    TELEPORT.CPP
    THING.CPP
    TRAPDOOR.CPP
    TRIGGER.CPP
    UNTOUCH.CPP
C:\CHAN\GAME\SRC\FE/
    FEMNUMGR.CPP
    GAMEMENU.CPP
    GAMESTOR.CPP
    HDITEM.CPP
    HDMENU.CPP
    LINEFILE.CPP
    LOADANIM.CPP
    MENUMGR.CPP
    OXOVL.CPP
    OXSCREEN.CPP
    OXSCRMGR.CPP
C:\CHAN\GAME\SRC\GEN/
    ANIMMAT.CPP
    ANIMMGR.CPP
    BLKMGR.CPP
    BLOCK.CPP
    CAMERA.CPP
    CAMMGR.CPP
    CCFILE.CPP
    CCLIST.CPP
    CHARMGR.CPP
    CMNEFFCT.CPP
    COLFIGHT.CPP
    COLFLOOR.CPP
    COLLINE.CPP
    COLMGR.CPP
    COLPHYS.CPP
    COLSECT.CPP
    COLVOL.CPP
    COLWALL.CPP
    CONTROL.CPP
    DATABASE.CPP
    DEADPOOL.CPP
    DIRECTOR.CPP
    DISPLAY.CPP
    EASTER.CPP
    EFFECTS.CPP
    ENVMGR.CPP
    FXP.CPP
    GAME.CPP
    GEFFECT.CPP
    ITEMNODE.CPP
    LENSFLRE.CPP
    LEVELMGR.CPP
    LIGHTS.CPP
    LOADERS.CPP
    MANAGER.CPP
    MEMSTAT.CPP
    MEMTRACK.CPP
    MHUMAN.CPP
    MODEL.CPP
    MPLAYER.CPP
    PALDATA.CPP
    PATH.CPP
    PATHINFO.CPP
    PROFILE.CPP
    PWEFFECT.CPP
    SCALEDAT.CPP
    SCOREMGR.CPP
    SPOTLIGHT.CPP
    STREAM.CPP
    SUBS.CPP
    SWITCH.CPP
    TAGS.C
    TIME.CPP
    TRAIL.CPP
    UVDATA.CPP
    WEFFECT.CPP
    WORLD.CPP
    WORLDPTS.CPP
C:\CHAN\GAME\SRC\PSX/
    BACKG.CPP
    EXPAND.CPP
    HUD.CPP
    MAIN.CPP
    MEMCARD.CPP
    MOVIES.CPP
    PARTICLE.CPP
    PSXDISP.CPP
    PSXSUBS.CPP
    RAMTEXANIM.CPP
    RCDMAIN.C
    RCDQ.C
    SHADOW.CPP
    SONYDUMP.CPP
    SOUND.CPP
C:\CHAN\GAME\SRC\SND/
    BASESND.CPP
    DRCTRSND.CPP
    DSTRSND.CPP
    ESOUND.CPP
    FESND.CPP
    HMNDSND.CPP
    JCSDLG.CPP
    JCSOUND.CPP
    KICKSND.CPP
    KNDNSND.CPP
    MSCCTRLR.CPP
    PHSMNGR.CPP
    PLATSND.CPP
    PNDLMSND.CPP
    PRSTSND.CPP
    PUSHSND.CPP
    RSDAMBCE.CPP
    RSDBACH.CPP
    RSDCLIP.CPP
    RSDLOAD.CPP
    RSDSTRM.CPP
    RSDUTIL.CPP
    RSEVENT.CPP
    RSMPLR.CPP
    SNDDRCT.CPP
    SNDFACT.CPP
    SNDFDB.CPP
    SNDMATH.CPP
    TRNSSND.CPP
    WPNSND.CPP
C:\chan\devsys\psx\radlib\SOURCE\MAIN/
    PETLATL.C
    RDEBUG.C
    RENTRYX.C
    RMAIN.C
    RPRINTF.C
    RSTREXT.C
    RSTRING.C
    RTASK.CPP
    RTIMEF.C
    STRTO.C
C:\chan\devsys\psx\radlib\SOURCE\MATH\FUNC/
    HASH.C
    MAG2.CPP
    MAG2FF.CPP
    MAG3.CPP
    MAG3FF.CPP
    RANDOM0.C
C:\chan\devsys\psx\radlib\SOURCE\MATH\MULTDIV/
    DIVIDE.C
    INVERSE.C
C:\chan\devsys\psx\radlib\SOURCE\MATH\TRIG/
    ASIN.C
    ATAN16.C
    ATAN216.C
    POLAR.CPP
    SIN.C
    SPHERE.CPP
C:\chan\devsys\psx\radlib\SOURCE\MATH\VECTOR/
    VECT2D.CPP
    VECT3D.CPP
C:\chan\devsys\psx\radlib\SOURCE\RADCD/
    RCDCACHE.C
    RCDDBG.C
    RCDGETF.CPP
    RCDREALX.C
    RCDWRITE.C
C:\chan\devsys\psx\radlib\SOURCE\RADMEM/
    FREEMEM.C
    MALLOC.C
    RADDMEM.C
    RADFMEM.CPP
    RADMEM.CPP
    RADSMEM.C
    RADZMEM.C
C:\devsys\psx\xclib\INDEP\INC/
    XC3X3MAT.H
C:\devsys\psx\xclib\INDEP\SRC/
    XC3X3MAT.CPP
    XCCHAR.CPP
    XCFILE.CPP
    XCHASH.CPP
    XCSORT.CPP
C:\devsys\psx\xclib\psx\INC/
    XCINV.H
C:\devsys\psx\xclib\psx\SRC/
    XCCIMAGE.CPP
    XCDO.CPP
    XCFONT.CPP
    XCFONTDC.CPP
    XCIDC.CPP
    XCINV.CPP
    XCSOS.CPP
    XCVRAM.CPP
C:\v11.3\INCLUDE/
    2PTCAMFLIP.HPP
    CBVANIM.HPP
    CBVPARAM.HPP
    CHANNEL.HPP
    CLUTANIM.HPP
    COMPANIM.HPP
    ETREE.HPP
    GTEMATRIX.HPP
    KEYNDOF.HPP
    LITD.HPP
    LITFARD.HPP
    MTREE.HPP
    PARAMANIM.HPP
    SEQUENCE.HPP
    STREE.HPP
    STREEUNLIT.HPP
    TDLIGHT.HPP
    TDYNGEOM.HPP
    TENTITY.HPP
    TEXANIM.HPP
    TFILE.HPP
    TGEOLOAD.HPP
    TGEOMTRY.INL
    TLOADER.HPP
    TMAT.HPP
    TMATLOAD.HPP
    TPRIMGEO.HPP
    TPRMLOAD.HPP
    TRANLOAD.HPP
    TREE.HPP
    UVANIM.HPP
    VERTANIM.HPP
    VIZANIM.HPP
    ZFARD.HPP
    ZSORTD.HPP
C:\v11.3\SOURCE/
    2PTCAMFLIP.CPP
    ANIMATE.CPP
    CBVANIM.CPP
    CBVPARAM.CPP
    CHANNEL.CPP
    CLUTANIM.CPP
    COMPANIM.CPP
    CYCLE.CPP
    ERROR.CPP
    ETLOAD.CPP
    ETREE.CPP
    FRUSTRUM.CPP
    GTEMATRIX.CPP
    HASH.CPP
    KEYNDOF.CPP
    LITD.CPP
    LITFARD.CPP
    MTREE.CPP
    P3DGBL.CPP
    P3DINV.CPP
    P3DMATH.CPP
    PARAMANIM.CPP
    PARAMFLIP.CPP
    PARAMLOAD.CPP
    PORTLITE.CPP
    PORTMATH.CPP
    POSE.CPP
    RPSTREE.CPP
    RPSTREECOL.CPP
    RPSTREEFLAT.CPP
    RPSTREELIT.CPP
    RPSTREENLT.CPP
    RPZCULL.CPP
    RPZFOG.CPP
    RQUEUE.CPP
    SEQUENCE.CPP
    STLOAD.CPP
    STREE.CPP
    STREEUNLIT.CPP
    T2POINTCAM.CPP
    TCACHE.CPP
    TCAMERA.CPP
    TCHUNK.CPP
    TCLTLOAD.CPP
    TDLIGHT.CPP
    TDTABLE.CPP
    TDYNGEOM.CPP
    TENTITY.CPP
    TEXANIM.CPP
    TFILE.CPP
    TGEOLOAD.CPP
    TIDXLIST.CPP
    TINVNTRY.CPP
    TLAYER.CPP
    TLIGHT.CPP
    TLOADER.CPP
    TMATLOAD.CPP
    TMATRIXCAM.CPP
    TPORT.CPP
    TPRIMGEO.CPP
    TPRMLOAD.CPP
    TRANLOAD.CPP
    TREE.CPP
    TTEXTURE.CPP
    TTXTLOAD.CPP
    TVIEW.CPP
    TVRTLOAD.CPP
    UVANIM.CPP
    VERTANIM.CPP
    VIZANIM.CPP
    ZFARD.CPP
    ZSORTD.CPP
L:\RTOOLS\RadMovie2\Src\RadMovie/
    RADMOVX.CPP
\CHAN\DEVSYS\PSX\PURE3D\INCLUDE/
    GTEMATRIX.HPP
    TLOADER.HPP
    TTEXLOAD.HPP
\CHAN\DEVSYS\PSX\XCLIB\INCLUDE/
    XCCOLOUR.H
\CHAN\GAME\INC\AI/
    ARROW.HPP
    BOSS.HPP
    EXPLODE.HPP
    GENERATOR.HPP
    HUMANOID.HPP
    KICK.HPP
    KNOCKDWN.HPP
    OBSTACLE.HPP
    PLATFORM.HPP
    PLAYER.HPP
    PUSHABLE.HPP
    TABLE.HPP
    THING.HPP
\CHAN\GAME\INC\FE/
    FEMNUMGR.H
    HDITEM.H
    HDMENU.H
    OXSCRMGR.H
\CHAN\GAME\INC\GEN/
    ANCHOR.HPP
    BLKMGR.HPP
    CALLBACK.HPP
    CCFILE.HPP
    CCLIST.HPP
    CHARMGR.HPP
    DATABASE.HPP
    DEADPOOL.HPP
    DRAWTABL.HPP
    HNDLRSET.HPP
    HUD.HPP
    ITEMNODE.HPP
    LEVELMGR.HPP
    LIGHTS.HPP
    LOADERS.HPP
    MODEL.HPP
    PATH.HPP
    SCOREMGR.HPP
    SOUND.HPP
    SWITCH.HPP
    TRAIL.HPP
    WEFFECT.HPP
    WORLDPTS.HPP
\CHAN\GAME\INC\PSX/
    MOVIES.HPP
    RAMTEXANIM.HPP
\CHAN\GAME\INC\SND/
    RSDLOAD.HPP
```

---

## Address-to-Function Index

Complete mapping of all addresses to demangled function names.

| Address | Demangled Name | Source |
|---------|---------------|--------|
| `0x80010000` | `DestructibleThing::Destroy()` | DESTROY.CPP:245 |
| `0x800100F0` | `DestructibleThing::GenerateItem()` | DESTROY.CPP:298 |
| `0x800102B8` | `DestructibleThing::DestructibleThing(const tagLVector*, unsigned short)` | DESTROY.CPP:355 |
| `0x80010308` | `_._17DestructibleThing` | DESTROY.CPP:375 |
| `0x80010388` | `DestructibleThing::AnalyzeMesh(DBRoot*)` | DESTROY.CPP:398 |
| `0x80010658` | `DestructibleThing::CreateModel(const char*)` | DESTROY.CPP:495 |
| `0x800106AC` | `DestructibleThing::DeleteModel()` | DESTROY.CPP:527 |
| `0x800106FC` | `DestructibleThing::Reset()` | DESTROY.CPP:543 |
| `0x80010710` | `DestructibleThing::Think()` | DESTROY.CPP:555 |
| `0x80010770` | `DestructibleThing::UpdatePosition()` | DESTROY.CPP:572 |
| `0x80010778` | `DestructibleThing::Draw()` | DESTROY.CPP:581 |
| `0x800107B8` | `DestructibleThing::MovePassengers()` | DESTROY.CPP:594 |
| `0x800107D8` | `DestructibleThing::HandlePickupCollision(Pickup*)` | DESTROY.CPP:608 |
| `0x80010834` | `DestructibleThing::HandleHumanoidCollision(Humanoid*)` | DESTROY.CPP:628 |
| `0x80010988` | `feMenuMgr::_ResumeGame(hdMenuItem*)` | FEMNUMGR.CPP:269 |
| `0x80010990` | `feMenuMgr::_NewGame(hdMenuItem*)` | FEMNUMGR.CPP:273 |
| `0x80010A04` | `feMenuMgr::_LoadGame(hdMenuItem*)` | FEMNUMGR.CPP:295 |
| `0x80010A2C` | `feMenuMgr::_SaveGame(hdMenuItem*)` | FEMNUMGR.CPP:300 |
| `0x80010A54` | `feMenuMgr::_ShowCredits(hdMenuItem*)` | FEMNUMGR.CPP:305 |
| `0x80010A7C` | `feMenuMgr::InputItemPush()` | FEMNUMGR.CPP:313 |
| `0x80010B40` | `feMenuMgr::InputPadUp()` | FEMNUMGR.CPP:329 |
| `0x80010B64` | `DestructibleThing::CareAboutAttack() const` | DESTROY.CPP:753 |
| `0x80010B6C` | `DestructibleThing::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | DESTROY.CPP:763 |
| `0x80010BC0` | `feMenuMgr::InputPadDown()` | FEMNUMGR.CPP:345 |
| `0x80010BFC` | `DestructibleThing::HandleObstacleDestructibleThingCollision(Obstacle*)` | DESTROY.CPP:789 |
| `0x80010C40` | `feMenuMgr::InputPadLeft()` | FEMNUMGR.CPP:362 |
| `0x80010CA4` | `feMenuMgr::InputPadRight()` | FEMNUMGR.CPP:375 |
| `0x80010CF8` | `DestructibleThing::GetFloorMaterial() const` | DESTROY.CPP:845 |
| `0x80010D08` | `feMenuMgr::PushMenu(hdMenu*)` | FEMNUMGR.CPP:388 |
| `0x80010D2C` | `_._6DBRoot` | DATABASE.HPP:196 |
| `0x80010D54` | `Generator::Generator(const tagLVector*, unsigned short)` | GENERATOR.CPP:163 |
| `0x80010DE8` | `_._9Generator` | GENERATOR.CPP:180 |
| `0x80010E30` | `feMenuMgr::PopMenu()` | FEMNUMGR.CPP:414 |
| `0x80010EB4` | `feMenuMgr::feMenuMgr()` | FEMNUMGR.CPP:425 |
| `0x80010EEC` | `Generator::GenerateObject(int)` | GENERATOR.CPP:204 |
| `0x80010F04` | `_._9feMenuMgr` | FEMNUMGR.CPP:435 |
| `0x80010F2C` | `feMenuMgr::HandleInputChange()` | FEMNUMGR.CPP:443 |
| `0x80011018` | `Generator::AnalyzeMesh(DBRoot*)` | GENERATOR.CPP:236 |
| `0x8001103C` | `feMenuMgr::SelfInit()` | FEMNUMGR.CPP:469 |
| `0x80011188` | `feMenuMgr::LevelValid(int, long)` | FEMNUMGR.CPP:492 |
| `0x80011218` | `feMenuMgr::ShowLevel(FrontEndVolume*, Humanoid*)` | FEMNUMGR.CPP:513 |
| `0x8001121C` | `Generator::CreateModel(const char*)` | GENERATOR.CPP:298 |
| `0x80011230` | `Generator::DeleteModel()` | GENERATOR.CPP:306 |
| `0x80011250` | `Generator::Reset()` | GENERATOR.CPP:312 |
| `0x80011260` | `feMenuMgr::InitLevelMenu()` | FEMNUMGR.CPP:523 |
| `0x80011300` | `Generator::Think()` | GENERATOR.CPP:329 |
| `0x800113F4` | `Generator::UpdatePosition()` | GENERATOR.CPP:358 |
| `0x800113FC` | `Generator::Trigger(Thing*, const char*, Thing*)` | GENERATOR.CPP:363 |
| `0x80011404` | `Generator::HandlePickupCollision(Pickup*)` | GENERATOR.CPP:371 |
| `0x8001140C` | `Generator::HandleHumanoidCollision(Humanoid*)` | GENERATOR.CPP:377 |
| `0x80011414` | `EnemyGenerator::GenerateObject()` | GENERATOR.CPP:383 |
| `0x80011540` | `feMenuMgr::Deactivate()` | FEMNUMGR.CPP:610 |
| `0x8001157C` | `EnemyGenerator::Reset()` | GENERATOR.CPP:424 |
| `0x8001158C` | `EnemyGenerator::AnalyzeMesh(DBRoot*)` | GENERATOR.CPP:431 |
| `0x800115BC` | `feMenuMgr::GotoStartScreen()` | FEMNUMGR.CPP:626 |
| `0x800115F0` | `feMenuMgr::ShowNewGameMenu()` | FEMNUMGR.CPP:634 |
| `0x80011618` | `feMenuMgr::PushLoadSaveMenu(int)` | FEMNUMGR.CPP:641 |
| `0x80011680` | `feMenuMgr::OpenDoors()` | FEMNUMGR.CPP:649 |
| `0x80011774` | `feMenuMgr::QueryInput(bool)` | FEMNUMGR.CPP:701 |
| `0x800117D8` | `EnemyGenerator::Think()` | GENERATOR.CPP:500 |
| `0x800118F0` | `TitleScreen::TitleScreen()` | FEMNUMGR.CPP:726 |
| `0x80011914` | `EnemyGenerator::SetupTargets(const char*)` | GENERATOR.CPP:542 |
| `0x8001192C` | `TitleScreen::SelfUpdate()` | FEMNUMGR.CPP:731 |
| `0x800119A8` | `TitleScreen::SelfInit()` | FEMNUMGR.CPP:738 |
| `0x80011A1C` | `TitleScreen::GetScreenNames()` | FEMNUMGR.CPP:750 |
| `0x80011A28` | `GameOverScreen::GameOverScreen()` | FEMNUMGR.CPP:758 |
| `0x80011A64` | `GameOverScreen::SelfUpdate()` | FEMNUMGR.CPP:763 |
| `0x80011A80` | `ThrowingGenerator::GenerateObject(int)` | GENERATOR.CPP:587 |
| `0x80011AE0` | `GameOverScreen::SelfInit()` | FEMNUMGR.CPP:770 |
| `0x80011B54` | `GameOverScreen::GetScreenNames()` | FEMNUMGR.CPP:782 |
| `0x80011B60` | `hdMemCardMenu::hdMemCardMenu(MenuMgr*, xcOverlay*, xcOverlay*)` | FEMNUMGR.CPP:858 |
| `0x80011C38` | `_._13hdMemCardMenu` | FEMNUMGR.CPP:876 |
| `0x80011CA4` | `hdMemCardMenu::TerminateMemCard()` | FEMNUMGR.CPP:887 |
| `0x80011CD0` | `hdMemCardMenu::StateStart(int)` | FEMNUMGR.CPP:896 |
| `0x80011D58` | `hdMemCardMenu::InitMemCard()` | FEMNUMGR.CPP:939 |
| `0x80011DC8` | `ThrowingGenerator::TargetInFOF()` | GENERATOR.CPP:644 |
| `0x80011DD4` | `hdMemCardMenu::Update()` | FEMNUMGR.CPP:965 |
| `0x80011E34` | `ThrowingGenerator::AnalyzeMesh(DBRoot*)` | GENERATOR.CPP:655 |
| `0x800120F4` | `ThrowingGenerator::Reset()` | GENERATOR.CPP:713 |
| `0x80012104` | `ThrowingGenerator::Think()` | GENERATOR.CPP:720 |
| `0x8001224C` | `_._17ThrowingGenerator` | GENERATOR.HPP:177 |
| `0x80012274` | `_._14EnemyGenerator` | GENERATOR.HPP:153 |
| `0x8001229C` | `_._6DBRoot` | DATABASE.HPP:196 |
| `0x800122C4` | `SlipperyFloor::SlipperyFloor(const tagLVector*, unsigned short)` | SLIPPERY.CPP:78 |
| `0x80012304` | `_._13SlipperyFloor` | SLIPPERY.CPP:90 |
| `0x80012390` | `SlipperyFloor::AnalyzeMesh(DBRoot*)` | SLIPPERY.CPP:108 |
| `0x80012434` | `hdMemCardMenu::SaveYes()` | FEMNUMGR.CPP:1280 |
| `0x80012454` | `SlipperyFloor::CreateModel(const char*)` | SLIPPERY.CPP:136 |
| `0x80012468` | `SlipperyFloor::DeleteModel()` | SLIPPERY.CPP:149 |
| `0x80012474` | `hdMemCardMenu::SaveOkOrNo()` | FEMNUMGR.CPP:1291 |
| `0x80012488` | `SlipperyFloor::Reset()` | SLIPPERY.CPP:159 |
| `0x80012490` | `SlipperyFloor::Think()` | SLIPPERY.CPP:168 |
| `0x80012498` | `SlipperyFloor::UpdatePosition()` | SLIPPERY.CPP:177 |
| `0x800124A0` | `SlipperyFloor::HandlePickupCollision(Pickup*)` | SLIPPERY.CPP:189 |
| `0x800124AC` | `SlipperyFloor::HandleHumanoidCollision(Humanoid*)` | SLIPPERY.CPP:202 |
| `0x800124BC` | `hdMemCardMenu::PromptYesNo(int, int)` | FEMNUMGR.CPP:1301 |
| `0x80012544` | `hdMemCardMenu::DynSetup()` | FEMNUMGR.CPP:1316 |
| `0x800125A4` | `hdMemCardMenu::InputNextItem()` | FEMNUMGR.CPP:1328 |
| `0x800125A4` | `SlipperyFloor::DoTrailEffect(Humanoid*)` | SLIPPERY.CPP:229 |
| `0x800125C4` | `hdMemCardMenu::InputPrevItem()` | FEMNUMGR.CPP:1334 |
| `0x800125E4` | `hdMemCardMenu::_Yes(hdMenuItem*)` | FEMNUMGR.CPP:1340 |
| `0x80012608` | `hdMemCardMenu::_OkorNo(hdMenuItem*)` | FEMNUMGR.CPP:1347 |
| `0x8001262C` | `hdMemCardMenu::GameSave()` | FEMNUMGR.CPP:1353 |
| `0x80012744` | `Collectible::Collectible(const tagLVector*, unsigned short)` | COLLECT.CPP:199 |
| `0x800127A4` | `_._11Collectible` | COLLECT.CPP:216 |
| `0x800127CC` | `Collectible::AnalyzeMesh(DBRoot*)` | COLLECT.CPP:225 |
| `0x80012970` | `Collectible::CreateModel(const char*)` | COLLECT.CPP:286 |
| `0x80012A7C` | `hdMemCardMenu::GameLoad()` | FEMNUMGR.CPP:1393 |
| `0x80012C5C` | `Collectible::DeleteModel()` | COLLECT.CPP:406 |
| `0x80012C7C` | `Collectible::Reset()` | COLLECT.CPP:416 |
| `0x80012C84` | `Collectible::Think()` | COLLECT.CPP:425 |
| `0x80012EC8` | `Collectible::UpdatePosition()` | COLLECT.CPP:494 |
| `0x80012ED0` | `Collectible::HandlePickupCollision(Pickup*)` | COLLECT.CPP:507 |
| `0x80012ED8` | `Collectible::HandleHumanoidCollision(Humanoid*)` | COLLECT.CPP:520 |
| `0x80012F70` | `hdMemCardMenu::CalcChecksum()` | FEMNUMGR.CPP:1460 |
| `0x80012FAC` | `hdMemCardMenu::SetChecksum()` | FEMNUMGR.CPP:1473 |
| `0x80012FE0` | `hdMemCardMenu::TestChecksum()` | FEMNUMGR.CPP:1481 |
| `0x8001301C` | `hdMemCardMenu::HasMenu()` | FEMNUMGR.CPP:1494 |
| `0x80013030` | `hdMemCardMenu::PromptOk(int)` | FEMNUMGR.CPP:1500 |
| `0x8001304C` | `Explosive::Explosive(const tagLVector*, unsigned short)` | EXPLODE.CPP:154 |
| `0x800130AC` | `hdMemCardMenu::CanAbortNow()` | FEMNUMGR.CPP:1516 |
| `0x800130C0` | `_._9Explosive` | EXPLODE.CPP:164 |
| `0x800130E8` | `Explosive::AnalyzeMesh(DBRoot*)` | EXPLODE.CPP:169 |
| `0x800130F4` | `hdMemCardMenu::Cleanup()` | FEMNUMGR.CPP:1533 |
| `0x80013134` | `hdControllerSelection::hdControllerSelection(xcOverlay*, char*, xcOverlay*)` | FEMNUMGR.CPP:1542 |
| `0x80013178` | `hdControllerSelection::IncItem()` | FEMNUMGR.CPP:1548 |
| `0x800131A4` | `hdControllerSelection::DecItem()` | FEMNUMGR.CPP:1557 |
| `0x800131D0` | `hdControllerSelection::SetControlDescription()` | FEMNUMGR.CPP:1563 |
| `0x80013264` | `_._21hdControllerSelection` | FEMNUMGR.H:269 |
| `0x80013284` | `_._14GameOverScreen` | FEMNUMGR.H:132 |
| `0x800132A4` | `_._11TitleScreen` | FEMNUMGR.H:118 |
| `0x800132C4` | `xcColour1555::GetAlpha8() const` | XCCOLOUR.H:217 |
| `0x800132C8` | `Explosive::CreateModel(const char*)` | EXPLODE.CPP:204 |
| `0x800132E4` | `xcColour1555::GetBlue8() const` | XCCOLOUR.H:216 |
| `0x800132E8` | `Explosive::DeleteModel()` | EXPLODE.CPP:210 |
| `0x80013308` | `xcColour1555::GetGreen8() const` | XCCOLOUR.H:215 |
| `0x80013308` | `Explosive::Reset()` | EXPLODE.CPP:216 |
| `0x8001332C` | `xcColour1555::GetRed8() const` | XCCOLOUR.H:214 |
| `0x8001333C` | `Explosive::CheckObstacleCollisions()` | EXPLODE.CPP:224 |
| `0x8001334C` | `MCInitialize` | MEMCARD.CPP:176 |
| `0x80013390` | `MCTerminate` | MEMCARD.CPP:209 |
| `0x80013420` | `MCLoadOverlay` | MEMCARD.CPP:230 |
| `0x80013448` | `MCUnloadOverlay` | MEMCARD.CPP:239 |
| `0x80013470` | `MCSetFileType` | MEMCARD.CPP:288 |
| `0x800134BC` | `Explosive::AdjustCollisionBox()` | EXPLODE.CPP:265 |
| `0x80013554` | `MCGetFullState` | MEMCARD.CPP:378 |
| `0x80013554` | `Explosive::ExplodeThing(Thing*)` | EXPLODE.CPP:279 |
| `0x80013598` | `MCGetState` | MEMCARD.CPP:422 |
| `0x800135DC` | `MCGetDirectory` | MEMCARD.CPP:473 |
| `0x80013820` | `MCGetFreeBlocks` | MEMCARD.CPP:696 |
| `0x80013900` | `MCCreateFile` | MEMCARD.CPP:791 |
| `0x80013A18` | `Explosive::Draw()` | EXPLODE.CPP:372 |
| `0x80013A48` | `Explosive::Think()` | EXPLODE.CPP:389 |
| `0x80013B64` | `MCDeleteFile` | MEMCARD.CPP:960 |
| `0x80013BAC` | `Explosive::Trigger(Thing*, const char*, Thing*)` | EXPLODE.CPP:463 |
| `0x80013BD0` | `Explosive::ExplosiveTrigger(int, const char*)` | EXPLODE.CPP:477 |
| `0x80013BEC` | `MCLoadFile` | MEMCARD.CPP:1045 |
| `0x80013C0C` | `Explosive::MovePassengers()` | EXPLODE.CPP:498 |
| `0x80013C2C` | `Explosive::HandlePickupCollision(Pickup*)` | EXPLODE.CPP:505 |
| `0x80013CA8` | `Explosive::HandleHumanoidCollision(Humanoid*)` | EXPLODE.CPP:520 |
| `0x80013CAC` | `MCSaveFile` | MEMCARD.CPP:1142 |
| `0x80013D6C` | `MCFormatCard` | MEMCARD.CPP:1227 |
| `0x80013D94` | `GetDirName(long, unsigned long, const char*, unsigned long*, char*, const char*)` | MEMCARD.CPP:1284 |
| `0x80013E40` | `Explosive::HandleObstacleCollision(Obstacle*)` | EXPLODE.CPP:577 |
| `0x80013ECC` | `Explosive::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | EXPLODE.CPP:605 |
| `0x80013F1C` | `Explosive::CareAboutAttack() const` | EXPLODE.HPP:115 |
| `0x80013F24` | `DynamicObstacle::DynamicObstacle(const tagLVector*, unsigned short)` | TABLE.CPP:162 |
| `0x80013FD0` | `_._15DynamicObstacle` | TABLE.CPP:182 |
| `0x80013FF4` | `WaitForCompletion(void)` | MEMCARD.CPP:1457 |
| `0x80013FFC` | `DynamicObstacle::AnalyzeMesh(DBRoot*)` | TABLE.CPP:195 |
| `0x800140B8` | `GetChannel(unsigned int)` | MEMCARD.CPP:1509 |
| `0x800140D4` | `SetTitle(unsigned short*, const char*)` | MEMCARD.CPP:1541 |
| `0x80014120` | `DynamicObstacle::CreateModel(const char*)` | TABLE.CPP:244 |
| `0x80014140` | `DynamicObstacle::DeleteModel()` | TABLE.CPP:254 |
| `0x80014148` | `DynamicObstacle::Reset()` | TABLE.CPP:263 |
| `0x80014158` | `DynamicObstacle::Think()` | TABLE.CPP:273 |
| `0x80014198` | `DynamicObstacle::Move()` | TABLE.CPP:286 |
| `0x80014228` | `GetTitle(char*, const unsigned short*)` | MEMCARD.CPP:1617 |
| `0x80014338` | `MoviePlayer::MoviePlayer()` | MOVIES.CPP:41 |
| `0x8001434C` | `_._11MoviePlayer` | MOVIES.CPP:61 |
| `0x800143D0` | `MoviePlayer::AddAction(MovieAction*)` | MOVIES.CPP:84 |
| `0x800143F4` | `MoviePlayer::MakePath(const char*)` | MOVIES.CPP:112 |
| `0x80014434` | `MoviePlayer::SetPath(const char*)` | MOVIES.CPP:129 |
| `0x80014484` | `DynamicObstacle::Draw()` | TABLE.CPP:325 |
| `0x800144B0` | `MoviePlayer::AddPlayMovie(const char*)` | MOVIES.CPP:159 |
| `0x800144B4` | `DynamicObstacle::AddForce(long, const _RMVECT16*)` | TABLE.CPP:338 |
| `0x80014534` | `MoviePlayer::Play(void*(*)()*, int, void*)` | MOVIES.CPP:226 |
| `0x8001456C` | `DynamicObstacle::AddMomentVector(const _RMVECT16&, const tagLVector&)` | TABLE.CPP:359 |
| `0x8001468C` | `MovieAction::MovieAction(MovieActionType)` | MOVIES.CPP:315 |
| `0x800146A8` | `MoviePlay::MoviePlay(const char*)` | MOVIES.CPP:333 |
| `0x800146F8` | `_._11MovieRandom` | MOVIES.CPP:372 |
| `0x80014738` | `DynamicObstacle::MovePassengers()` | TABLE.CPP:388 |
| `0x80014758` | `DynamicObstacle::Throw(long, long, const _RMVECT16&, const tagLVector&)` | TABLE.CPP:404 |
| `0x80014790` | `MovieRandom::AddMovie(const char*)` | MOVIES.CPP:393 |
| `0x800147EC` | `MovieRandom::GetMovie() const` | MOVIES.CPP:413 |
| `0x80014818` | `DynamicObstacle::UpdatePosition()` | TABLE.CPP:423 |
| `0x80014820` | `DynamicObstacle::HandlePickupCollision(Pickup*)` | TABLE.CPP:435 |
| `0x80014828` | `DynamicObstacle::HandleHumanoidCollision(Humanoid*)` | TABLE.CPP:452 |
| `0x8001484C` | `MoviePlay::GetMovie() const` | MOVIES.HPP:121 |
| `0x80014854` | `_._9MoviePlay` | MOVIES.HPP:122 |
| `0x80014888` | `_._11MovieAction` | MOVIES.HPP:110 |
| `0x800148BC` | `GameStorage::InitForSave()` | GAMESTOR.CPP:81 |
| `0x80014934` | `GameStorage::InitForRestore()` | GAMESTOR.CPP:109 |
| `0x80014934` | `DynamicObstacle::HandleObjectInterAction(Humanoid*)` | TABLE.CPP:503 |
| `0x800149A8` | `GameStorage::FreeBuffer()` | GAMESTOR.CPP:135 |
| `0x800149F0` | `static_init(gGameStorage)` | GAMESTOR.CPP:144 |
| `0x80014BF8` | `DynamicObstacle::Destroy()` | TABLE.CPP:594 |
| `0x80014CA8` | `DynamicObstacle::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | TABLE.CPP:633 |
| `0x80014CB0` | `DynamicObstacle::HandleEnvironmentCollision(const tagLVector&)` | TABLE.CPP:654 |
| `0x80015190` | `Table::Table(const tagLVector*, unsigned short)` | TABLE.CPP:792 |
| `0x800151C8` | `_._5Table` | TABLE.CPP:802 |
| `0x800151F0` | `Table::AnalyzeMesh(DBRoot*)` | TABLE.CPP:814 |
| `0x80015210` | `Table::CreateModel(const char*)` | TABLE.CPP:827 |
| `0x80015230` | `Table::DeleteModel()` | TABLE.CPP:837 |
| `0x80015238` | `Table::Think()` | TABLE.CPP:857 |
| `0x80015258` | `Table::UpdatePosition()` | TABLE.CPP:868 |
| `0x80015260` | `Table::Throw(long, long, const _RMVECT16&, const tagLVector&)` | TABLE.CPP:883 |
| `0x800152A0` | `Table::HandlePickupCollision(Pickup*)` | TABLE.CPP:906 |
| `0x800152A8` | `Table::HandleHumanoidCollision(Humanoid*)` | TABLE.CPP:921 |
| `0x8001531C` | `Chair::Chair(const tagLVector*, unsigned short)` | TABLE.CPP:952 |
| `0x80015354` | `_._5Chair` | TABLE.CPP:962 |
| `0x8001537C` | `Chair::AnalyzeMesh(DBRoot*)` | TABLE.CPP:974 |
| `0x8001539C` | `Chair::CreateModel(const char*)` | TABLE.CPP:987 |
| `0x800153BC` | `Chair::DeleteModel()` | TABLE.CPP:997 |
| `0x800153C4` | `Chair::Think()` | TABLE.CPP:1017 |
| `0x800153E4` | `Chair::UpdatePosition()` | TABLE.CPP:1028 |
| `0x800153EC` | `Chair::HandlePickupCollision(Pickup*)` | TABLE.CPP:1040 |
| `0x800153F4` | `Chair::Throw(long, long, const _RMVECT16&, const tagLVector&)` | TABLE.CPP:1055 |
| `0x80015434` | `Chair::HandleHumanoidCollision(Humanoid*)` | TABLE.CPP:1078 |
| `0x80015470` | `DynamicObstacle::CareAboutAttack() const` | TABLE.HPP:95 |
| `0x80015478` | `HorizontalPole::HorizontalPole(const tagLVector*, unsigned short)` | HPOLE.CPP:72 |
| `0x800154D4` | `_._14HorizontalPole` | HPOLE.CPP:93 |
| `0x800154FC` | `HorizontalPole::AnalyzeMesh(DBRoot*)` | HPOLE.CPP:125 |
| `0x80015750` | `HorizontalPole::CreateModel(const char*)` | HPOLE.CPP:194 |
| `0x80015764` | `HorizontalPole::DeleteModel()` | HPOLE.CPP:209 |
| `0x8001576C` | `HorizontalPole::Reset()` | HPOLE.CPP:218 |
| `0x80015774` | `HorizontalPole::Think()` | HPOLE.CPP:227 |
| `0x8001577C` | `HorizontalPole::UpdatePosition()` | HPOLE.CPP:236 |
| `0x80015784` | `HorizontalPole::HandlePickupCollision(Pickup*)` | HPOLE.CPP:246 |
| `0x8001578C` | `HorizontalPole::HandleHumanoidCollision(Humanoid*)` | HPOLE.CPP:258 |
| `0x80015AB0` | `Blast::Blast(const tagLVector*, unsigned short)` | BLAST.CPP:149 |
| `0x80015AF8` | `_._5Blast` | BLAST.CPP:159 |
| `0x80015B54` | `Blast::AnalyzeMesh(DBRoot*)` | BLAST.CPP:168 |
| `0x80016284` | `Blast::CreateSound()` | BLAST.CPP:323 |
| `0x800162E4` | `Blast::UpdateSound()` | BLAST.CPP:353 |
| `0x8001631C` | `Blast::ReleaseSound()` | BLAST.CPP:361 |
| `0x80016368` | `Blast::CreateModel(const char*)` | BLAST.CPP:372 |
| `0x8001639C` | `Blast::DeleteModel()` | BLAST.CPP:384 |
| `0x800163C8` | `Blast::Reset()` | BLAST.CPP:392 |
| `0x80016488` | `rMvInit` | RADMOVX.CPP:306 |
| `0x800164A4` | `Blast::Activate()` | BLAST.CPP:429 |
| `0x800164C4` | `Blast::Deactivate()` | BLAST.CPP:435 |
| `0x80016510` | `rMvTerm` | RADMOVX.CPP:368 |
| `0x80016528` | `Blast::Think()` | BLAST.CPP:455 |
| `0x80016588` | `rMvOpenMovie` | RADMOVX.CPP:422 |
| `0x80016770` | `rMvCloseMovie` | RADMOVX.CPP:545 |
| `0x80016808` | `rMvPlay` | RADMOVX.CPP:602 |
| `0x80016A10` | `Blast::Trigger(Thing*, const char*, Thing*)` | BLAST.CPP:658 |
| `0x80016ACC` | `Blast::Draw()` | BLAST.CPP:684 |
| `0x80016B3C` | `Blast::HandlePickupCollision(Pickup*)` | BLAST.CPP:701 |
| `0x80016B44` | `Blast::HandleHumanoidCollision(Humanoid*)` | BLAST.CPP:707 |
| `0x80016E84` | `rMvStop` | RADMOVX.CPP:997 |
| `0x80016FB8` | `TrapDoor::TrapDoor(const tagLVector*, unsigned short)` | TRAPDOOR.CPP:97 |
| `0x8001702C` | `_._8TrapDoor` | TRAPDOOR.CPP:110 |
| `0x80017054` | `TrapDoor::AnalyzeMesh(DBRoot*)` | TRAPDOOR.CPP:123 |
| `0x800170E0` | `rMvGetState` | RADMOVX.CPP:1189 |
| `0x80017164` | `rMvSetAudio` | RADMOVX.CPP:1224 |
| `0x800172E0` | `FrameReadyCallback(...)` | RADMOVX.CPP:1668 |
| `0x800172EC` | `TrapDoor::CreateModel(const char*)` | TRAPDOOR.CPP:184 |
| `0x8001730C` | `TrapDoor::DeleteModel()` | TRAPDOOR.CPP:196 |
| `0x80017310` | `DCTCallback(...)` | RADMOVX.CPP:1690 |
| `0x8001732C` | `TrapDoor::Reset()` | TRAPDOOR.CPP:206 |
| `0x80017334` | `TrapDoor::Trigger(Thing*, const char*, Thing*)` | TRAPDOOR.CPP:215 |
| `0x8001735C` | `CDReadyCallback(unsigned char, unsigned char*)` | RADMOVX.CPP:1729 |
| `0x80017360` | `TrapDoor::Think()` | TRAPDOOR.CPP:232 |
| `0x800173D0` | `VSynchCallback(...)` | RADMOVX.CPP:1768 |
| `0x80017418` | `DrawSynchCallback(...)` | RADMOVX.CPP:1796 |
| `0x8001747C` | `MovieEventHandler(void)` | RADMOVX.CPP:1842 |
| `0x80017484` | `TrapDoor::UpdatePosition()` | TRAPDOOR.CPP:276 |
| `0x8001748C` | `TrapDoor::Draw()` | TRAPDOOR.CPP:285 |
| `0x800174C8` | `TrapDoor::Move()` | TRAPDOOR.CPP:311 |
| `0x800175F4` | `TrapDoor::HandlePickupCollision(Pickup*)` | TRAPDOOR.CPP:360 |
| `0x80017628` | `TrapDoor::HandleHumanoidCollision(Humanoid*)` | TRAPDOOR.CPP:374 |
| `0x80017AE4` | `TrapDoor::SetupCollisionBox()` | TRAPDOOR.CPP:489 |
| `0x80017CC4` | `TrapDoor::GetFloorMaterial() const` | TRAPDOOR.CPP:504 |
| `0x80017CCC` | `Pushable::Pushable(const tagLVector*, unsigned short)` | PUSHABLE.CPP:205 |
| `0x80017D08` | `_._8Pushable` | PUSHABLE.CPP:212 |
| `0x80017D2C` | `KickStartHdFrame(void)` | RADMOVX.CPP:2285 |
| `0x80017D70` | `GetFrame(unsigned long**, StHEADER**)` | RADMOVX.CPP:2316 |
| `0x80017D70` | `Pushable::AnalyzeMesh(DBRoot*)` | PUSHABLE.CPP:222 |
| `0x80017EE0` | `Pushable::CreateModel(const char*)` | PUSHABLE.CPP:248 |
| `0x80017F24` | `FreeFrame(unsigned long*)` | RADMOVX.CPP:2466 |
| `0x80017F34` | `Pushable::DeleteModel()` | PUSHABLE.CPP:269 |
| `0x80017F50` | `LineFile::LineFile()` | LINEFILE.CPP:18 |
| `0x80017F68` | `_._8LineFile` | LINEFILE.CPP:24 |
| `0x80017F84` | `Pushable::Reset()` | PUSHABLE.CPP:281 |
| `0x80017FAC` | `Pushable::Think()` | PUSHABLE.CPP:294 |
| `0x80017FD4` | `LineFile::Open(char*)` | LINEFILE.CPP:31 |
| `0x80018078` | `LineFile::Next()` | LINEFILE.CPP:50 |
| `0x8001811C` | `Pushable::Move()` | PUSHABLE.CPP:345 |
| `0x800181E0` | `LineFile::Word(int)` | LINEFILE.CPP:97 |
| `0x80018244` | `Pushable::MovePassengers()` | PUSHABLE.CPP:379 |
| `0x800184A8` | `Pushable::UpdatePosition()` | PUSHABLE.CPP:449 |
| `0x800184B0` | `Pushable::HandleEnvironmentCollision(const tagLVector&)` | PUSHABLE.CPP:456 |
| `0x800188E8` | `Pushable::HandlePickupCollision(Pickup*)` | PUSHABLE.CPP:551 |
| `0x80018928` | `Pushable::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | PUSHABLE.CPP:564 |
| `0x80018AF8` | `Pushable::HandleHumanoidCollision(Humanoid*)` | PUSHABLE.CPP:656 |
| `0x80019264` | `Pushable::GetFloorMaterial() const` | PUSHABLE.CPP:852 |
| `0x80019298` | `Pushable::CareAboutAttack() const` | PUSHABLE.HPP:97 |
| `0x8001A758` | `FrontEndVolume::FrontEndVolume(const tagLVector*, unsigned short)` | FEVOLUME.CPP:83 |
| `0x8001A758` | `Boss::Boss(const tagLVector*, unsigned short)` | BOSS.CPP:148 |
| `0x8001A79C` | `_._14FrontEndVolume` | FEVOLUME.CPP:96 |
| `0x8001A79C` | `_._4Boss` | BOSS.CPP:155 |
| `0x8001A7C4` | `FrontEndVolume::AnalyzeMesh(DBRoot*)` | FEVOLUME.CPP:110 |
| `0x8001A7C4` | `Boss::CreateModel(const char*)` | BOSS.CPP:158 |
| `0x8001A8D8` | `FrontEndVolume::CreateModel(const char*)` | FEVOLUME.CPP:154 |
| `0x8001A8EC` | `FrontEndVolume::DeleteModel()` | FEVOLUME.CPP:168 |
| `0x8001A8EC` | `Boss::SetActionState(unsigned long, long)` | BOSS.CPP:210 |
| `0x8001A900` | `FrontEndVolume::Reset()` | FEVOLUME.CPP:178 |
| `0x8001A908` | `FrontEndVolume::Think()` | FEVOLUME.CPP:187 |
| `0x8001A90C` | `Boss::_Collapse()` | BOSS.CPP:238 |
| `0x8001A910` | `FrontEndVolume::UpdatePosition()` | FEVOLUME.CPP:196 |
| `0x8001A918` | `FrontEndVolume::HandlePickupCollision(Pickup*)` | FEVOLUME.CPP:208 |
| `0x8001A920` | `FrontEndVolume::HandleHumanoidCollision(Humanoid*)` | FEVOLUME.CPP:221 |
| `0x8001A9CC` | `FrontEndVolume::HandleVolumeExit(Humanoid*)` | FEVOLUME.CPP:252 |
| `0x8001AAF4` | `Boss::_CrouchUp()` | BOSS.CPP:304 |
| `0x8001AB0C` | `Door::Door(const tagLVector*, unsigned short)` | DOOR.CPP:202 |
| `0x8001AB68` | `Boss::TestAndSetBackGrab()` | BOSS.CPP:325 |
| `0x8001AB70` | `Butch::Butch(const tagLVector*)` | BOSS.CPP:335 |
| `0x8001AB8C` | `_._4Door` | DOOR.CPP:217 |
| `0x8001ABB4` | `Door::AnalyzeMesh(DBRoot*)` | DOOR.CPP:226 |
| `0x8001ABD0` | `_._5Butch` | BOSS.CPP:350 |
| `0x8001AC00` | `Butch::SetActionState(unsigned long, long)` | BOSS.CPP:355 |
| `0x8001AD30` | `Butch::_Stomp()` | BOSS.CPP:419 |
| `0x8001AE88` | `Door::CreateModel(const char*)` | DOOR.CPP:284 |
| `0x8001AEA8` | `Door::DeleteModel()` | DOOR.CPP:295 |
| `0x8001AEC8` | `Door::Reset()` | DOOR.CPP:306 |
| `0x8001AEF0` | `Butch::_Charge()` | BOSS.CPP:491 |
| `0x8001AF5C` | `Door::Think()` | DOOR.CPP:342 |
| `0x8001B058` | `Door::UpdatePosition()` | DOOR.CPP:394 |
| `0x8001B058` | `Butch::_ThrowPot()` | BOSS.CPP:542 |
| `0x8001B060` | `Door::Draw()` | DOOR.CPP:404 |
| `0x8001B0BC` | `Grontar::Grontar(const tagLVector*)` | BOSS.CPP:559 |
| `0x8001B0D4` | `Door::Trigger()` | DOOR.CPP:433 |
| `0x8001B118` | `_._7Grontar` | BOSS.CPP:572 |
| `0x8001B148` | `Grontar::SetActionState(unsigned long, long)` | BOSS.CPP:577 |
| `0x8001B1D8` | `Door::Open()` | DOOR.CPP:477 |
| `0x8001B20C` | `Grontar::_Stand()` | BOSS.CPP:619 |
| `0x8001B210` | `Door::Move()` | DOOR.CPP:490 |
| `0x8001B274` | `Grontar::_Run()` | BOSS.CPP:637 |
| `0x8001B2DC` | `Grontar::_Taunt()` | BOSS.CPP:655 |
| `0x8001B2FC` | `Door::DeathCheck()` | DOOR.CPP:535 |
| `0x8001B344` | `Grontar::_Straif()` | BOSS.CPP:673 |
| `0x8001B3AC` | `Grontar::_DiveRoll()` | BOSS.CPP:694 |
| `0x8001B43C` | `Door::HandlePickupCollision(Pickup*)` | DOOR.CPP:611 |
| `0x8001B47C` | `Door::HandleHumanoidCollision(Humanoid*)` | DOOR.CPP:623 |
| `0x8001B47C` | `Grontar::_GotHitHigh()` | BOSS.CPP:731 |
| `0x8001B4F0` | `Grontar::_GotHitMed()` | BOSS.CPP:747 |
| `0x8001B564` | `Grontar::FindFoe(unsigned long, long, int)` | BOSS.CPP:767 |
| `0x8001B5A0` | `Grontar::GetTargetingFrame(const StrikeFightingMove&)` | BOSS.CPP:795 |
| `0x8001B60C` | `Dante::Dante(const tagLVector*)` | BOSS.CPP:844 |
| `0x8001B624` | `Door::TeleportPlayer()` | DOOR.CPP:710 |
| `0x8001B688` | `Arrow::Arrow(const tagLVector*, unsigned short)` | ARROW.CPP:42 |
| `0x8001B6C0` | `_._5Arrow` | ARROW.CPP:52 |
| `0x8001B6E8` | `Arrow::AnalyzeMesh(DBRoot*)` | ARROW.CPP:65 |
| `0x8001B718` | `_._5Dante` | BOSS.CPP:878 |
| `0x8001B8CC` | `Arrow::CreateModel(const char*)` | ARROW.CPP:107 |
| `0x8001B8D0` | `Dante::AnalyzeMesh(DBRoot*)` | BOSS.CPP:910 |
| `0x8001B8EC` | `Arrow::DeleteModel()` | ARROW.CPP:119 |
| `0x8001B90C` | `Arrow::Reset()` | ARROW.CPP:129 |
| `0x8001BA80` | `Dante::Think()` | BOSS.CPP:938 |
| `0x8001BAA0` | `Dante::SetActionState(unsigned long, long)` | BOSS.CPP:943 |
| `0x8001BBA4` | `Dante::_Stand()` | BOSS.CPP:999 |
| `0x8001BBD8` | `Arrow::Think()` | ARROW.CPP:216 |
| `0x8001BC18` | `Dante::_Taunt()` | BOSS.CPP:1021 |
| `0x8001BC70` | `Dante::_GotHitHigh()` | BOSS.CPP:1032 |
| `0x8001BCC4` | `Dante::_GotHitMed()` | BOSS.CPP:1042 |
| `0x8001BD18` | `Dante::_GotHitFreeForm()` | BOSS.CPP:1053 |
| `0x8001BD38` | `Dante::_ThrowFreeFall()` | BOSS.CPP:1067 |
| `0x8001BD58` | `Dante::_MissilePrepare()` | BOSS.CPP:1080 |
| `0x8001BDDC` | `Arrow::UpdatePosition()` | ARROW.CPP:252 |
| `0x8001BDE4` | `Arrow::Draw()` | ARROW.CPP:261 |
| `0x8001BDE4` | `Dante::_MissileAttack()` | BOSS.CPP:1113 |
| `0x8001BE24` | `Arrow::HandleHumanoidCollision(Humanoid*)` | ARROW.HPP:41 |
| `0x8001BE2C` | `Arrow::HandlePickupCollision(Pickup*)` | ARROW.HPP:40 |
| `0x8001BE34` | `Conveyor::Conveyor(const tagLVector*, unsigned short)` | CONVEYOR.CPP:62 |
| `0x8001BE98` | `_._8Conveyor` | CONVEYOR.CPP:77 |
| `0x8001BEE4` | `Conveyor::AnalyzeMesh(DBRoot*)` | CONVEYOR.CPP:83 |
| `0x8001C1DC` | `Conveyor::CreateModel(const char*)` | CONVEYOR.CPP:175 |
| `0x8001C214` | `Conveyor::DeleteModel()` | CONVEYOR.CPP:190 |
| `0x8001C240` | `Conveyor::Reset()` | CONVEYOR.CPP:197 |
| `0x8001C260` | `Conveyor::Think()` | CONVEYOR.CPP:203 |
| `0x8001C2BC` | `Conveyor::UpdatePosition()` | CONVEYOR.CPP:217 |
| `0x8001C2C4` | `Conveyor::HandlePickupCollision(Pickup*)` | CONVEYOR.CPP:222 |
| `0x8001C2CC` | `Conveyor::HandleHumanoidCollision(Humanoid*)` | CONVEYOR.CPP:228 |
| `0x8001C358` | `Dante::_TargetMissileAttack()` | BOSS.CPP:1238 |
| `0x8001C398` | `KickNRoll::KickNRoll(const tagLVector*, unsigned short)` | KICK.CPP:209 |
| `0x8001C3DC` | `_._9KickNRoll` | KICK.CPP:217 |
| `0x8001C444` | `KickNRoll::AnalyzeMesh(DBRoot*)` | KICK.CPP:228 |
| `0x8001C6FC` | `KickNRoll::CreateModel(const char*)` | KICK.CPP:289 |
| `0x8001C70C` | `Dante::LoadCombatDialog()` | BOSS.CPP:1349 |
| `0x8001C72C` | `Dante::PlayCombatKnockDownDialog(DamageTypesTags)` | BOSS.CPP:1374 |
| `0x8001C750` | `KickNRoll::DeleteModel()` | KICK.CPP:315 |
| `0x8001C77C` | `Paul::Paul(const tagLVector*)` | BOSS.CPP:1405 |
| `0x8001C7A0` | `KickNRoll::Reset()` | KICK.CPP:327 |
| `0x8001C7B8` | `KickNRoll::Think()` | KICK.CPP:334 |
| `0x8001C7D8` | `_._4Paul` | BOSS.CPP:1418 |
| `0x8001C808` | `Paul::_Straif()` | BOSS.CPP:1423 |
| `0x8001C834` | `Paul::_GotHitHigh()` | BOSS.CPP:1430 |
| `0x8001C850` | `KickNRoll::Move()` | KICK.CPP:362 |
| `0x8001C888` | `Paul::_GotHitMed()` | BOSS.CPP:1441 |
| `0x8001C8DC` | `Paul::SetActionState(unsigned long, long)` | BOSS.CPP:1461 |
| `0x8001C8FC` | `Oscar::Oscar(const tagLVector*)` | BOSS.CPP:1485 |
| `0x8001C964` | `_._5Oscar` | BOSS.CPP:1499 |
| `0x8001C994` | `Oscar::SetActionState(unsigned long, long)` | BOSS.CPP:1504 |
| `0x8001C9B4` | `Oscar::_Straif()` | BOSS.CPP:1537 |
| `0x8001CA10` | `Dante::GetTargetingFrame(const StrikeFightingMove&)` | BOSS.CPP:1588 |
| `0x8001CA18` | `Dante::PlayCombatThrowDialog()` | BOSS.HPP:112 |
| `0x8001CA3C` | `_._4Path` | PATH.HPP:108 |
| `0x8001CB20` | `Behaviour::_ButchDMS()` | BEHAVEB.CPP:284 |
| `0x8001CB60` | `KickNRoll::HandleEnvironmentCollision(tagLVector&)` | KICK.CPP:434 |
| `0x8001CEEC` | `KickNRoll::Destroy()` | KICK.CPP:617 |
| `0x8001CF74` | `KickNRoll::UpdatePosition()` | KICK.CPP:637 |
| `0x8001CF7C` | `KickNRoll::Draw()` | KICK.CPP:642 |
| `0x8001CFAC` | `KickNRoll::HandlePickupCollision(Pickup*)` | KICK.CPP:651 |
| `0x8001CFEC` | `KickNRoll::MovePassengers()` | KICK.CPP:659 |
| `0x8001D178` | `KickNRoll::HandleHumanoidCollision(Humanoid*)` | KICK.CPP:717 |
| `0x8001D178` | `Behaviour::_ButchDMS_Charge()` | BEHAVEB.CPP:630 |
| `0x8001D314` | `Behaviour::_GrontarDMS()` | BEHAVEB.CPP:717 |
| `0x8001D45C` | `KickNRoll::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | KICK.CPP:802 |
| `0x8001D5CC` | `KickNRoll::CareAboutAttack() const` | KICK.HPP:111 |
| `0x8001D5D4` | `KnockDown::KnockDown(const tagLVector*, unsigned short)` | KNOCKDWN.CPP:258 |
| `0x8001D650` | `_._9KnockDown` | KNOCKDWN.CPP:270 |
| `0x8001D6B8` | `KnockDown::AnalyzeMesh(DBRoot*)` | KNOCKDWN.CPP:280 |
| `0x8001D7CC` | `Behaviour::_PaulDMS()` | BEHAVEB.CPP:950 |
| `0x8001D8D8` | `KnockDown::CreateModel(const char*)` | KNOCKDWN.CPP:329 |
| `0x8001D92C` | `KnockDown::Draw()` | KNOCKDWN.CPP:351 |
| `0x8001D9A8` | `KnockDown::DeleteModel()` | KNOCKDWN.CPP:373 |
| `0x8001D9F8` | `KnockDown::Reset()` | KNOCKDWN.CPP:385 |
| `0x8001DA48` | `KnockDown::Think()` | KNOCKDWN.CPP:398 |
| `0x8001DB8C` | `KnockDown::Move()` | KNOCKDWN.CPP:458 |
| `0x8001DCA0` | `KnockDown::UpdateCollisionBox()` | KNOCKDWN.CPP:506 |
| `0x8001E05C` | `wallCheck(Humanoid*, long)` | BEHAVEB.CPP:1274 |
| `0x8001E0B0` | `Behaviour::_OscarDMS()` | BEHAVEB.CPP:1304 |
| `0x8001E120` | `KnockDown::HandlePickupCollision(Pickup*)` | KNOCKDWN.CPP:537 |
| `0x8001E16C` | `KnockDown::HandleHumanoidCollision(Humanoid*)` | KNOCKDWN.CPP:549 |
| `0x8001E4EC` | `KnockDown::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | KNOCKDWN.CPP:642 |
| `0x8001E6E0` | `Stack::LoadDialog(unsigned long, long)` | KNOCKDWN.CPP:720 |
| `0x8001E768` | `Stack::Stack(const tagLVector*, unsigned short)` | KNOCKDWN.CPP:767 |
| `0x8001E7B8` | `_._5Stack` | KNOCKDWN.CPP:781 |
| `0x8001E7DC` | `Behaviour::_OscarHenchmanDMS()` | BEHAVEB.CPP:1683 |
| `0x8001E820` | `Stack::Draw()` | KNOCKDWN.CPP:790 |
| `0x8001E850` | `Stack::AnalyzeMesh(DBRoot*)` | KNOCKDWN.CPP:807 |
| `0x8001EA18` | `Stack::CreateModel(const char*)` | KNOCKDWN.CPP:855 |
| `0x8001EB0C` | `Stack::DeleteModel()` | KNOCKDWN.CPP:940 |
| `0x8001EB5C` | `Stack::Reset()` | KNOCKDWN.CPP:952 |
| `0x8001EB68` | `Stack::UpdatePosition()` | KNOCKDWN.CPP:959 |
| `0x8001EB70` | `Stack::Wobble()` | KNOCKDWN.CPP:964 |
| `0x8001EBE8` | `Stack::Fall()` | KNOCKDWN.CPP:988 |
| `0x8001EC24` | `Stack::FinishStack()` | KNOCKDWN.CPP:998 |
| `0x8001EDB4` | `Behaviour::CounterAttack()` | BEHAVEB.CPP:1965 |
| `0x8001EE28` | `Stack::Think()` | KNOCKDWN.CPP:1058 |
| `0x8001EEE0` | `Behaviour::_DanteDMS_Phase1()` | BEHAVEB.CPP:2062 |
| `0x8001EEE8` | `Stack::HandlePickupCollision(Pickup*)` | KNOCKDWN.CPP:1088 |
| `0x8001EF1C` | `Stack::HandleHumanoidCollision(Humanoid*)` | KNOCKDWN.CPP:1094 |
| `0x8001F2B4` | `Behaviour::_DanteDMS_Phase2()` | BEHAVEB.CPP:2219 |
| `0x8001F320` | `Stack::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | KNOCKDWN.CPP:1205 |
| `0x8001F370` | `Stack::UpdateCollisionBox()` | KNOCKDWN.CPP:1224 |
| `0x8001F3E8` | `Stack::TriggerStackAnimation()` | KNOCKDWN.CPP:1235 |
| `0x8001F3F4` | `Stack::SetupCallbacks()` | KNOCKDWN.CPP:1240 |
| `0x8001F55C` | `Stack::SetupJointPosition(int, G10tagLVector)` | KNOCKDWN.CPP:1317 |
| `0x8001F5A0` | `StackEJointCallback(tEJoint*, int)` | KNOCKDWN.CPP:1337 |
| `0x8001F664` | `Stack::CareAboutAttack() const` | KNOCKDWN.HPP:209 |
| `0x8001F66C` | `KnockDown::CareAboutAttack() const` | KNOCKDWN.HPP:149 |
| `0x8001F674` | `KnockDown::UpdatePosition()` | KNOCKDWN.HPP:143 |
| `0x8001F678` | `Behaviour::_DanteDMS_Phase3()` | BEHAVEB.CPP:2372 |
| `0x8001F67C` | `_._6DBRoot` | DATABASE.HPP:196 |
| `0x8001F6A4` | `Crusher::Crusher(const tagLVector*, unsigned short)` | CRUSHER.CPP:95 |
| `0x8001F6F8` | `_._7Crusher` | CRUSHER.CPP:112 |
| `0x8001F760` | `Crusher::AnalyzeMesh(DBRoot*)` | CRUSHER.CPP:130 |
| `0x8001F924` | `Crusher::CreateModel(const char*)` | CRUSHER.CPP:172 |
| `0x8001F980` | `Crusher::DeleteModel()` | CRUSHER.CPP:204 |
| `0x8001F9D0` | `Crusher::Reset()` | CRUSHER.CPP:220 |
| `0x8001F9F0` | `Crusher::Think()` | CRUSHER.CPP:235 |
| `0x8001FAE0` | `Crusher::UpdatePosition()` | CRUSHER.CPP:270 |
| `0x8001FAE8` | `Crusher::Draw()` | CRUSHER.CPP:279 |
| `0x8001FB08` | `Crusher::Move()` | CRUSHER.CPP:289 |
| `0x8001FC58` | `Crusher::HandlePickupCollision(Pickup*)` | CRUSHER.CPP:350 |
| `0x8001FC98` | `Crusher::HandleHumanoidCollision(Humanoid*)` | CRUSHER.CPP:364 |
| `0x8001FE34` | `Launcher::Launcher(const tagLVector*, unsigned short)` | LAUNCHER.CPP:311 |
| `0x8001FE7C` | `_._8Launcher` | LAUNCHER.CPP:326 |
| `0x8001FEA4` | `Launcher::AnalyzeMesh(DBRoot*)` | LAUNCHER.CPP:346 |
| `0x8001FFB0` | `Launcher::CreateModel(const char*)` | LAUNCHER.CPP:390 |
| `0x800200F0` | `Launcher::Draw()` | LAUNCHER.CPP:481 |
| `0x80020164` | `Launcher::DeleteModel()` | LAUNCHER.CPP:503 |
| `0x8002018C` | `Launcher::Reset()` | LAUNCHER.CPP:515 |
| `0x800201A4` | `Launcher::Think()` | LAUNCHER.CPP:529 |
| `0x80020290` | `Launcher::UpdatePosition()` | LAUNCHER.CPP:555 |
| `0x80020298` | `Launcher::HandlePickupCollision(Pickup*)` | LAUNCHER.CPP:567 |
| `0x80020328` | `Launcher::HandleHumanoidCollision(Humanoid*)` | LAUNCHER.CPP:587 |
| `0x80020730` | `Launcher::HandleHumanoidDefaultLaunch(Humanoid*)` | LAUNCHER.CPP:776 |
| `0x80020844` | `Platform::Platform(const tagLVector*, unsigned short)` | PLATFORM.CPP:505 |
| `0x800208E4` | `_._8Platform` | PLATFORM.CPP:526 |
| `0x800209BC` | `Platform::Draw()` | PLATFORM.CPP:554 |
| `0x80020A24` | `Platform::AnalyzeMesh(DBRoot*)` | PLATFORM.CPP:576 |
| `0x80021578` | `Platform::CreateModel(const char*)` | PLATFORM.CPP:878 |
| `0x80021698` | `Platform::DeleteModel()` | PLATFORM.CPP:936 |
| `0x800216E8` | `Platform::Reset()` | PLATFORM.CPP:948 |
| `0x80021918` | `Platform::Think()` | PLATFORM.CPP:1043 |
| `0x80021E54` | `Platform::OnNewPathNode()` | PLATFORM.CPP:1219 |
| `0x800222F4` | `Platform::OnXorZRot()` | PLATFORM.CPP:1367 |
| `0x80022788` | `Platform::Move()` | PLATFORM.CPP:1440 |
| `0x80022B90` | `Platform::Teeter()` | PLATFORM.CPP:1537 |
| `0x80023190` | `Platform::Bob()` | PLATFORM.CPP:1660 |
| `0x8002337C` | `Platform::MovePassengers()` | PLATFORM.CPP:1695 |
| `0x80023830` | `Platform::SetPlatformToPathNode(const char*)` | PLATFORM.CPP:1776 |
| `0x80023A80` | `Platform::Trigger(Thing*, const char*, Thing*)` | PLATFORM.CPP:1815 |
| `0x80023CE4` | `Platform::HandlePickupCollision(Pickup*)` | PLATFORM.CPP:1897 |
| `0x80023D38` | `Platform::HandleEnvironmentCollision(tagLVector&)` | PLATFORM.CPP:1908 |
| `0x80023FC4` | `Platform::CheckForSquash(const tagLVector&, const _RMVECT16&, const tagCollisionCylinder&)` | PLATFORM.CPP:1995 |
| `0x800240C4` | `Platform::HandleHumanoidCollision(Humanoid*)` | PLATFORM.CPP:2026 |
| `0x80024B7C` | `Platform::GetDeltaVelocity() const` | PLATFORM.CPP:2346 |
| `0x80024BA0` | `Platform::AtEndOfPath()` | PLATFORM.CPP:2354 |
| `0x80024BE0` | `Platform::DeathCheck()` | PLATFORM.CPP:2365 |
| `0x80024C70` | `Platform::FillSphere(tSphere&) const` | PLATFORM.CPP:2412 |
| `0x80024CD0` | `Platform::GetFloorMaterial() const` | PLATFORM.CPP:2437 |
| `0x80024D04` | `Platform::GetInitialPos()` | PLATFORM.HPP:346 |
| `0x80024D0C` | `_._4Path` | PATH.HPP:108 |
| `0x80024DF0` | `Pendulum::Pendulum(const tagLVector*, unsigned short)` | PENDULUM.CPP:102 |
| `0x80024E38` | `_._8Pendulum` | PENDULUM.CPP:116 |
| `0x80024EA0` | `Pendulum::AnalyzeMesh(DBRoot*)` | PENDULUM.CPP:134 |
| `0x8002520C` | `Pendulum::CreateModel(const char*)` | PENDULUM.CPP:200 |
| `0x8002525C` | `Pendulum::DeleteModel()` | PENDULUM.CPP:230 |
| `0x800252AC` | `Pendulum::Reset()` | PENDULUM.CPP:246 |
| `0x800252B4` | `Pendulum::Think()` | PENDULUM.CPP:255 |
| `0x800256C4` | `Pendulum::UpdatePosition()` | PENDULUM.CPP:332 |
| `0x800256CC` | `Pendulum::Draw()` | PENDULUM.CPP:341 |
| `0x800256EC` | `Pendulum::HandlePickupCollision(Pickup*)` | PENDULUM.CPP:354 |
| `0x80025720` | `Pendulum::HandleHumanoidCollision(Humanoid*)` | PENDULUM.CPP:367 |
| `0x80026130` | `MyVBL(...)` | MAIN.CPP:370 |
| `0x80026220` | `SetupVBL(...(*)()*, long)` | MAIN.CPP:407 |
| `0x8002627C` | `SetupPSXStuff(void)` | MAIN.CPP:422 |
| `0x8002635C` | `main` | MAIN.CPP:519 |
| `0x800264C8` | `LoadOverlay(OverlayId)` | MAIN.CPP:728 |
| `0x800265F0` | `LoadBossAIOverlay(BossAIOverlayEnum)` | MAIN.CPP:851 |
| `0x800266C8` | `EarlyLoadConfigFile(void)` | MAIN.CPP:957 |
| `0x800266D8` | `rInitMemX` | RENTRYX.C:21 |
| `0x8002671C` | `rInit` | RMAIN.C:52 |
| `0x80026770` | `rIsPlatGik` | RMAIN.C:96 |
| `0x800267B8` | `rIsPlatEuroGik` | RMAIN.C:104 |
| `0x800267FC` | `rIsPlatPsycho` | RMAIN.C:109 |
| `0x8002683C` | `rIsPlatPsychoProfiler` | RMAIN.C:116 |
| `0x80026890` | `rIsPlatRealPSX` | RMAIN.C:128 |
| `0x800268DC` | `rIsPlatBonk` | RMAIN.C:203 |
| `0x800268E4` | `rIsPlatSatPsycho` | RMAIN.C:205 |
| `0x800268EC` | `rIsPlatRealSat` | RMAIN.C:206 |
| `0x800268F4` | `rIsPlatKrak` | RMAIN.C:217 |
| `0x800268FC` | `rIsPlatPTUI` | RMAIN.C:227 |
| `0x8002900C` | `FreeDynamicPrimBuffers(void)` | GAME.CPP:769 |
| `0x80029080` | `AllocateDynamicPrimBuffers(bool)` | GAME.CPP:785 |
| `0x800291D4` | `ResizeDynamicPrimBuffers(bool)` | GAME.CPP:846 |
| `0x80029200` | `DisplayTIM(const char*)` | GAME.CPP:862 |
| `0x80029328` | `Game::gsNullState(Game*)` | GAME.CPP:964 |
| `0x80029330` | `ButtonCheckCallback(void*)` | GAME.CPP:1027 |
| `0x80029404` | `SetupEnv(void)` | GAME.CPP:1061 |
| `0x80029460` | `Game::gsInitState(Game*)` | GAME.CPP:1161 |
| `0x80029574` | `Game::gsQueueLevelLoad(Game*)` | GAME.CPP:1230 |
| `0x8002977C` | `Game::gsQueuePetalLoad(Game*)` | GAME.CPP:1349 |
| `0x8002986C` | `Game::gsQueueLevelPetalLoad(Game*)` | GAME.CPP:1378 |
| `0x80029924` | `Game::gsDetermineNextGameState(Game*)` | GAME.CPP:1414 |
| `0x800299B0` | `Game::gsDetermineGameOverState(Game*)` | GAME.CPP:1453 |
| `0x800299B8` | `Game::gsOpenFEState(Game*)` | GAME.CPP:1470 |
| `0x80029A48` | `Game::gsFEState(Game*)` | GAME.CPP:1499 |
| `0x80029A70` | `Game::gsOpenLocationState(Game*)` | GAME.CPP:1506 |
| `0x80029AC0` | `Game::gsPrePlayState(Game*)` | GAME.CPP:1529 |
| `0x80029C6C` | `Game::gsPlayState(Game*)` | GAME.CPP:1629 |
| `0x80029D68` | `MenuRender(MenuMgr*)` | GAME.CPP:1714 |
| `0x80029DB8` | `MenuDraw(MenuMgr*)` | GAME.CPP:1735 |
| `0x80029E34` | `MenuFade(void)` | GAME.CPP:1757 |
| `0x80029EF8` | `Game::gsMenuState(Game*)` | GAME.CPP:1785 |
| `0x80029F64` | `Game::gsErrorState(Game*)` | GAME.CPP:1807 |
| `0x8002A004` | `Game::gsErrorExitState(Game*)` | GAME.CPP:1832 |
| `0x8002A064` | `Game::gsErrorLoopState(Game*)` | GAME.CPP:1851 |
| `0x8002A128` | `Game::gsLocationMenuState(Game*)` | GAME.CPP:1883 |
| `0x8002A174` | `Game::gsDbgMenuState(Game*)` | GAME.CPP:1896 |
| `0x8002A17C` | `Game::gsEndState(Game*)` | GAME.CPP:1917 |
| `0x8002A184` | `chanp3dClipCode(_RMVECT16&)` | GAME.CPP:1953 |
| `0x8002A1F8` | `vecLengthSquared(long, long, long)` | GAME.CPP:1968 |
| `0x8002A238` | `computeBlockToPointDistances(const Block*, const tagLVector&, int*, const tagLVector&)` | GAME.CPP:1976 |
| `0x8002A98C` | `DrawEverythingHandler(Handler*)` | GAME.CPP:2211 |
| `0x8002AF88` | `OffsetToPreventSeams(tagLVector&, const tagLVector&)` | GAME.CPP:2482 |
| `0x8002B224` | `DrawLoop(ccList*, unsigned long)` | GAME.CPP:2540 |
| `0x8002B290` | `AnimateLoop(ccList*)` | GAME.CPP:2565 |
| `0x8002B2F0` | `AnimateEverythingHandler(Handler*)` | GAME.CPP:2620 |
| `0x8002B368` | `animLoopDSTACK(void)` | GAME.CPP:2634 |
| `0x8002B408` | `BeginFrameHandler(Handler*)` | GAME.CPP:2665 |
| `0x8002B420` | `EndFrameHandler(Handler*)` | GAME.CPP:2687 |
| `0x8002B428` | `_._4Game` | GAME.CPP:2750 |
| `0x8002B4F0` | `Game::ProcessHandlers()` | GAME.CPP:2756 |
| `0x8002B588` | `Game::InternalClose()` | GAME.CPP:3003 |
| `0x8002B61C` | `Game::InternalReset()` | GAME.CPP:3026 |
| `0x8002B65C` | `Game::Step()` | GAME.CPP:3040 |
| `0x8002B688` | `Game::gsEndLevelState(Game*)` | GAME.CPP:3051 |
| `0x8002B6B0` | `Game::gsEndLevelLoopState(Game*)` | GAME.CPP:3061 |
| `0x8002B744` | `Game::gsEndLevelExitState(Game*)` | GAME.CPP:3086 |
| `0x8002B9D8` | `_gameControlChanged(Control*, short)` | GAME.CPP:3170 |
| `0x8002BAB8` | `Game::GetNextToken(char*, char**, char*)` | GAME.CPP:3206 |
| `0x8002BBF0` | `Game::PlayMovie(const char*, int, int)` | GAME.CPP:3309 |
| `0x8002BE0C` | `Game::gsTitleLoopState(Game*)` | GAME.CPP:3393 |
| `0x8002C22C` | `Game::gsEndGameLoopState(Game*)` | GAME.CPP:3571 |
| `0x8002C3B4` | `Game::gsEndGameState(Game*)` | GAME.CPP:3624 |
| `0x8002C474` | `Game::gsTitleState(Game*)` | GAME.CPP:3656 |
| `0x8002C5AC` | `Game::SetState(Q24Game9GameState)` | GAME.CPP:3754 |
| `0x8002C648` | `Game::LoadXconFE()` | GAME.CPP:3789 |
| `0x8002C7A4` | `Game::FreeXconFE()` | GAME.CPP:3812 |
| `0x8002C838` | `Game::InitXconFSImage()` | GAME.CPP:3826 |
| `0x8002C998` | `Game::FreeXconFSImage()` | GAME.CPP:3859 |
| `0x8002C9A0` | `Game::FadeBegin()` | GAME.CPP:3869 |
| `0x8002C9B4` | `Game::FadeEnd()` | GAME.CPP:3875 |
| `0x8002C9BC` | `Game::FadeUpdate()` | GAME.CPP:3879 |
| `0x8002C9F8` | `Game::FadeRender()` | GAME.CPP:3893 |
| `0x8002CB28` | `Game::gsPlayMovieCredits(Game*)` | GAME.CPP:3924 |
| `0x8002CB98` | `_._10oxFontFile` | OXSCRMGR.H:73 |
| `0x8002CBB8` | `HandlerSet::PurgeHandlers()` | HNDLRSET.HPP:110 |
| `0x8002CC50` | `_._10HandlerSet` | HNDLRSET.HPP:88 |
| `0x8002CCAC` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x8002CCF4` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8002CD44` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8002CD98` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8002CDE8` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x8002D52C` | `ClearActuator(void)` | CONTROL.CPP:242 |
| `0x8002D540` | `SetActuator(unsigned char, unsigned char, unsigned int)` | CONTROL.CPP:250 |
| `0x8002D564` | `UpdateActuator(int)` | CONTROL.CPP:262 |
| `0x8002D588` | `StepActuator(void)` | CONTROL.CPP:270 |
| `0x8002D5F4` | `InitController(int)` | CONTROL.CPP:297 |
| `0x8002D650` | `GetShock(void)` | CONTROL.CPP:321 |
| `0x8002D670` | `SetShock(int)` | CONTROL.CPP:332 |
| `0x8002D6B4` | `IsDualShock(void)` | CONTROL.CPP:348 |
| `0x8002D6C0` | `Shock(ShockEnum)` | CONTROL.CPP:365 |
| `0x8002D830` | `ReadSonyPads(void)` | CONTROL.CPP:519 |
| `0x8002D898` | `Button::Button()` | CONTROL.CPP:687 |
| `0x8002D90C` | `_._6Button` | CONTROL.CPP:699 |
| `0x8002D934` | `Button::Reset()` | CONTROL.CPP:703 |
| `0x8002D960` | `Button::Default()` | CONTROL.CPP:711 |
| `0x8002D980` | `Button::GetState()` | CONTROL.CPP:723 |
| `0x8002D9B0` | `Button::_RawHandler(long)` | CONTROL.CPP:745 |
| `0x8002D9FC` | `Button::_OneshotHandler(long)` | CONTROL.CPP:773 |
| `0x8002DA58` | `Button::_DeadHandler(long)` | CONTROL.CPP:795 |
| `0x8002DA60` | `Button::_AnalogHandler(long)` | CONTROL.CPP:801 |
| `0x8002DA68` | `Button::_AutoRepeatHandler(long)` | CONTROL.CPP:810 |
| `0x8002DB40` | `Button::SetMode(short)` | CONTROL.CPP:864 |
| `0x8002DB6C` | `Control::Control()` | CONTROL.CPP:874 |
| `0x8002DBEC` | `_._7Control` | CONTROL.CPP:883 |
| `0x8002DC6C` | `Control::Reset()` | CONTROL.CPP:887 |
| `0x8002DCF0` | `Control::Input(unsigned long)` | CONTROL.CPP:902 |
| `0x8002DDEC` | `Control::GetMask()` | CONTROL.CPP:928 |
| `0x8002DE5C` | `Control::SetControlMapArray(char*)` | CONTROL.CPP:985 |
| `0x8002DE88` | `Control::ApplyCurrentModeMap()` | CONTROL.CPP:998 |
| `0x8002DEEC` | `Control::GetButton(char) const` | CONTROL.CPP:1012 |
| `0x8002DF14` | `Control::GetMappedButton(char) const` | CONTROL.CPP:1016 |
| `0x8002DF50` | `inputPrivHandler(Handler*)` | CONTROL.CPP:1023 |
| `0x8002DF74` | `InputManager::InputManager()` | CONTROL.CPP:1029 |
| `0x8002E048` | `_._12InputManager` | CONTROL.CPP:1050 |
| `0x8002E0D4` | `InputManager::ServiceInput(unsigned long, unsigned short)` | CONTROL.CPP:1055 |
| `0x8002E2F0` | `InputManager::SetControlMapArray(short, char*)` | CONTROL.CPP:1135 |
| `0x8002E33C` | `InputManager::SetControlModeArray(short, short*)` | CONTROL.CPP:1149 |
| `0x8002E3F0` | `InputManager::SetPlayerConfig(char)` | CONTROL.CPP:1162 |
| `0x8002E414` | `InputManager::UpdateReverseMap()` | CONTROL.CPP:1168 |
| `0x8002E460` | `InputManager::DefaultMapArray()` | CONTROL.CPP:1180 |
| `0x8002E46C` | `InputManager::SetButtonCallback(short, short, ButtonHook*(*)()*, void, void*)` | CONTROL.CPP:1197 |
| `0x8002E4D0` | `InputManager::ClearButtonCallback(short, short)` | CONTROL.CPP:1208 |
| `0x8002E528` | `InputManager::InternalOpen()` | CONTROL.CPP:1229 |
| `0x8002E530` | `InputManager::InternalClose()` | CONTROL.CPP:1234 |
| `0x8002E550` | `InputManager::InternalReset()` | CONTROL.CPP:1240 |
| `0x8002E5BC` | `InputManager::GetControlVal(unsigned short)` | CONTROL.CPP:1250 |
| `0x8002E6E8` | `InputManager::Step()` | CONTROL.CPP:1328 |
| `0x8002E73C` | `InputManager::PlayerMapArray()` | CONTROL.CPP:1337 |
| `0x8002E754` | `InputManager::FindButtonMapping(char)` | CONTROL.CPP:1343 |
| `0x8002E7A8` | `ControlState::ControlState()` | CONTROL.CPP:1359 |
| `0x8002E7B0` | `ControlState::Save()` | CONTROL.CPP:1363 |
| `0x8002E7E0` | `ControlState::Restore()` | CONTROL.CPP:1370 |
| `0x8002E81C` | `static_init(ACTUATOR_DURATION)` | CONTROL.CPP:1391 |
| `0x8002E858` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x8002E8A0` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x8002E8DC` | `rTaskInit` | RTASK.CPP:25 |
| `0x8002E910` | `rInitTaskList` | RTASK.CPP:38 |
| `0x8002E960` | `rTaskSuicide` | RTASK.CPP:62 |
| `0x8002E968` | `rDelTask` | RTASK.CPP:67 |
| `0x8002E978` | `rInsertTask(_RTASKLIST*, _RTASK*)` | RTASK.CPP:72 |
| `0x8002E9D0` | `rExecuteTaskBucket(_RTASKLIST*, _RTASK_BUCKET*)` | RTASK.CPP:115 |
| `0x8002EA6C` | `rReInsertTaskBucket(_RTASKLIST*, _RTASK_BUCKET*)` | RTASK.CPP:149 |
| `0x8002EAFC` | `rDoTaskList` | RTASK.CPP:180 |
| `0x8002EB90` | `rNewTask` | RTASK.CPP:210 |
| `0x8002EBEC` | `rNewTaskP` | RTASK.CPP:223 |
| `0x8002EC14` | `rNewTaskPP` | RTASK.CPP:231 |
| `0x8002EC40` | `Manager::InternalClose()` | MANAGER.CPP:22 |
| `0x8002EC48` | `Manager::InternalOpen()` | MANAGER.CPP:26 |
| `0x8002EC50` | `Manager::InternalReset()` | MANAGER.CPP:30 |
| `0x8002EC58` | `Manager::Close()` | MANAGER.CPP:34 |
| `0x8002ECA4` | `Manager::Open()` | MANAGER.CPP:51 |
| `0x8002ECF4` | `Manager::Reset()` | MANAGER.CPP:65 |
| `0x8002ED24` | `Manager::Manager()` | MANAGER.CPP:72 |
| `0x8002ED5C` | `_._7Manager` | MANAGER.CPP:77 |
| `0x8002EEA4` | `SetMemoryState(MemoryStateEnum)` | MEMTRACK.CPP:92 |
| `0x8002EEDC` | `(bool, uiltin_new)` | MEMTRACK.CPP:195 |
| `0x8002EF60` | `(bool, uiltin_vec_new)` | MEMTRACK.CPP:207 |
| `0x8002EFE4` | `(nw__FUii)` | MEMTRACK.CPP:220 |
| `0x8002F034` | `(void, n__FUii)` | MEMTRACK.CPP:233 |
| `0x8002F084` | `(bool, uiltin_delete)` | MEMTRACK.CPP:250 |
| `0x8002F0E0` | `(bool, uiltin_vec_delete)` | MEMTRACK.CPP:268 |
| `0x8002F13C` | `MemBankMgr::SetBankNo(int)` | MEMTRACK.CPP:329 |
| `0x8002F14C` | `MemBankMgr::ResetBankNo()` | MEMTRACK.CPP:336 |
| `0x8002F16C` | `MEMSTATTHING(unsigned short)` | MEMSTAT.CPP:128 |
| `0x8002F1A0` | `MEMSTAT(MemoryStatEnum, MemoryStatModifierEnum)` | MEMSTAT.CPP:136 |
| `0x8002F2E8` | `MEMSTAT_CLEAR(void)` | MEMSTAT.CPP:188 |
| `0x8002F320` | `MEMSTAT_PRINT(void)` | MEMSTAT.CPP:198 |
| `0x8002F3FC` | `MEMSTAT_NEW_RESET(void)` | MEMSTAT.CPP:230 |
| `0x8002F410` | `MEMSTAT_NEW(void)` | MEMSTAT.CPP:236 |
| `0x8002F478` | `MEMSTAT_NEW_PRINT(void)` | MEMSTAT.CPP:251 |
| `0x8002F4AC` | `MEMSTAT_MIN_CLEAR(void)` | MEMSTAT.CPP:259 |
| `0x8002F4BC` | `MEMSTAT_MIN_PRINT(void)` | MEMSTAT.CPP:264 |
| `0x8002F4F4` | `MEMSTAT_OBJECTSIZEOF_PRINT(void)` | MEMSTAT.CPP:275 |
| `0x8002F760` | `MEMSTAT_PRINT_HELPER(MemoryStatEnum)` | MEMSTAT.CPP:326 |
| `0x8002F7B8` | `MEMSTAT_OBJECTSIZEOF_HELPER(const char*, unsigned long)` | MEMSTAT.CPP:336 |
| `0x8002F7E8` | `MEMSTATSTRING(MemoryStatEnum)` | MEMSTAT.CPP:341 |
| `0x8002F9D8` | `MEMSTATMODSTRING(MemoryStatModifierEnum)` | MEMSTAT.CPP:541 |
| `0x8002FA80` | `Player::Player(const tagLVector*)` | PLAYER.CPP:1014 |
| `0x8002FBC0` | `_._6Player` | PLAYER.CPP:1050 |
| `0x8002FC24` | `Player::Reset()` | PLAYER.CPP:1056 |
| `0x8002FD34` | `Player::CreateModel(const char*)` | PLAYER.CPP:1111 |
| `0x8002FE30` | `Player::Think()` | PLAYER.CPP:1155 |
| `0x800300B0` | `Player::SignalEnemyGetUp()` | PLAYER.CPP:1382 |
| `0x80030100` | `Player::Move()` | PLAYER.CPP:1408 |
| `0x80030120` | `Player::DoJump()` | PLAYER.CPP:1424 |
| `0x800301D8` | `Player::DoJump(long)` | PLAYER.CPP:1437 |
| `0x8003027C` | `Player::GetViewSpot(tagLVector*, tagLVector*)` | PLAYER.CPP:1460 |
| `0x80030388` | `GetWeaponPickupDialog(long)` | PLAYER.CPP:1548 |
| `0x800303BC` | `Player::SetActionState(unsigned long, long)` | PLAYER.CPP:1579 |
| `0x8003123C` | `Player::_InactiveIdle()` | PLAYER.CPP:2331 |
| `0x80031350` | `Player::_Stand()` | PLAYER.CPP:2378 |
| `0x80031A78` | `Player::_Flip()` | PLAYER.CPP:2683 |
| `0x80031C68` | `Player::_Jump()` | PLAYER.CPP:2830 |
| `0x80032348` | `CalculateFallDamage(long)` | PLAYER.CPP:3161 |
| `0x80032368` | `Player::FallingPhysics()` | PLAYER.CPP:3187 |
| `0x80032444` | `Player::_Fall()` | PLAYER.CPP:3226 |
| `0x800324E8` | `Player::_HardFall()` | PLAYER.CPP:3343 |
| `0x80032560` | `Player::_HardLand()` | PLAYER.CPP:3365 |
| `0x800325CC` | `Player::_Run()` | PLAYER.CPP:3397 |
| `0x80032A48` | `Player::_Push()` | PLAYER.CPP:3550 |
| `0x80032B80` | `Player::_PushObject()` | PLAYER.CPP:3604 |
| `0x80032C70` | `Player::_Teetering()` | PLAYER.CPP:3637 |
| `0x80032C78` | `Player::DoWallJump()` | PLAYER.CPP:3664 |
| `0x80032D8C` | `Player::_WallJump()` | PLAYER.CPP:3688 |
| `0x80032EB0` | `Player::_Collapse()` | PLAYER.CPP:3732 |
| `0x80032F48` | `Player::_DoStand()` | PLAYER.CPP:3757 |
| `0x80032F8C` | `Player::_HorizontalPoleSwing()` | PLAYER.CPP:3802 |
| `0x8003352C` | `Player::_LedgeLatch()` | PLAYER.CPP:4099 |
| `0x800337A8` | `Player::_LedgePullup()` | PLAYER.CPP:4214 |
| `0x80033858` | `Player::_Dead()` | PLAYER.CPP:4256 |
| `0x8003389C` | `Player::_SlopeSlide()` | PLAYER.CPP:4274 |
| `0x80033C00` | `Player::CheckForLanding()` | PLAYER.CPP:4366 |
| `0x80033D0C` | `Player::OnCheckpoint()` | PLAYER.CPP:4424 |
| `0x80033D9C` | `Player::SetLivesLeft(long)` | PLAYER.CPP:4457 |
| `0x80033DB8` | `Player::_LadderDismount()` | PLAYER.CPP:4477 |
| `0x80033DD8` | `Player::_ClimbLadder()` | PLAYER.CPP:4488 |
| `0x80033DF8` | `Player::_TableRoll()` | PLAYER.CPP:4509 |
| `0x80033FF8` | `Player::_Straif()` | PLAYER.CPP:4606 |
| `0x80034140` | `Player::PlayerSingleEncounterCheak()` | PLAYER.CPP:4660 |
| `0x80034210` | `Player::LoadPlayerTauntResponse(Humanoid*)` | PLAYER.CPP:4712 |
| `0x80034290` | `Player::PlayPlayerTauntResponse()` | PLAYER.CPP:4786 |
| `0x8003431C` | `Player::SignalEnemyDead(Humanoid*)` | PLAYER.CPP:4828 |
| `0x80034338` | `GetWeaponFinalBlowDialog(long)` | PLAYER.CPP:4863 |
| `0x800343D4` | `Player::EnterCombatCombo()` | PLAYER.CPP:4967 |
| `0x800343F4` | `Player::LoadCombatDialog()` | PLAYER.CPP:5000 |
| `0x80034510` | `Player::PlayCombatKnockDownDialog(DamageTypesTags)` | PLAYER.CPP:5094 |
| `0x800345B8` | `Player::HandleHitShock(DamageTypesTags)` | PLAYER.CPP:5155 |
| `0x80034618` | `Player::PlayCombatThrowDialog()` | PLAYER.HPP:496 |
| `0x8003463C` | `_._15CharMgrCallback` | CHARMGR.HPP:83 |
| `0x80034670` | `CharMgrCallback::Callback()` | CHARMGR.HPP:82 |
| `0x8003467C` | `_._14CheckpointInfo` | SCOREMGR.HPP:200 |
| `0x800346B0` | `rsEvent(rsSoundEvent, long, long, long)` | RSEVENT.CPP:48 |
| `0x8003470C` | `rsDialogEvent(rsSoundEvent, long, long, long)` | RSEVENT.CPP:82 |
| `0x80034818` | `rCDGetMode` | RCDMAIN.C:37 |
| `0x80034824` | `rCDAlignDown` | RCDMAIN.C:52 |
| `0x8003483C` | `rCDFileSize` | RCDMAIN.C:61 |
| `0x80034848` | `rCDSize` | RCDMAIN.C:68 |
| `0x8003488C` | `rCDSetCallBack` | RCDMAIN.C:82 |
| `0x800348AC` | `rCDFreeQ` | RCDMAIN.C:87 |
| `0x800348D4` | `rCDMemcpyQ` | RCDMAIN.C:93 |
| `0x800348FC` | `rCDFatalStub` | RCDMAIN.C:99 |
| `0x80034904` | `rCDInit` | RCDMAIN.C:104 |
| `0x80034A98` | `rCDOpen` | RCDMAIN.C:162 |
| `0x80034C3C` | `rCDCacheInit` | RCDMAIN.C:254 |
| `0x80034C90` | `rCDCacheTerm` | RCDMAIN.C:263 |
| `0x80034CD0` | `rCDReadA` | RCDMAIN.C:280 |
| `0x80034EAC` | `rCDSeekQ` | RCDMAIN.C:344 |
| `0x80034EF0` | `rCDSeekA` | RCDMAIN.C:356 |
| `0x80034F28` | `rCDCloseA` | RCDMAIN.C:362 |
| `0x80034FB4` | `rCDGetFileA` | RCDGETF.CPP:12 |
| `0x80035060` | `rCDGetFile` | RCDGETF.CPP:44 |
| `0x8003509C` | `jcsInitialize(void)` | JCSOUND.CPP:174 |
| `0x80035190` | `jcsTerminate(void)` | JCSOUND.CPP:273 |
| `0x800352C8` | `jcsSetConfiguration(jcsSoundParams*)` | JCSOUND.CPP:372 |
| `0x80035454` | `jcsGetConfiguration(jcsSoundParams*)` | JCSOUND.CPP:447 |
| `0x8003547C` | `jcsSetListener(const tagLVector*, const long*)` | JCSOUND.CPP:473 |
| `0x800354C4` | `jcsUnloadLevel(void)` | JCSOUND.CPP:494 |
| `0x80035564` | `jcsSetSoundLocation(rsSoundLocation)` | JCSOUND.CPP:547 |
| `0x80035664` | `jcsStartSound(void)` | JCSOUND.CPP:629 |
| `0x800356D0` | `jcsStopSound(void)` | JCSOUND.CPP:669 |
| `0x80035760` | `jcsFadeOutEngine(unsigned long)` | JCSOUND.CPP:721 |
| `0x8003582C` | `jcsFadeInEngine(unsigned long)` | JCSOUND.CPP:764 |
| `0x800358F0` | `jcsCdYield(unsigned long)` | JCSOUND.CPP:804 |
| `0x80035990` | `jcsCdAccess(unsigned long)` | JCSOUND.CPP:837 |
| `0x80035A30` | `jcsSetAmbienceSpace(unsigned long)` | JCSOUND.CPP:877 |
| `0x80035AA0` | `jcsSetAmbienceCrossFade(long)` | JCSOUND.CPP:914 |
| `0x80035B00` | `jcsHandleControlEvent(rsSoundEvent, long, long, long)` | JCSOUND.CPP:947 |
| `0x80035DC0` | `LoadFile(const char*)` | JCSOUND.CPP:1208 |
| `0x80035E6C` | `LoadLevel(long, long)` | JCSOUND.CPP:1261 |
| `0x80035EDC` | `jcsGetCurrentLocationInfo(void)` | JCSOUND.CPP:1282 |
| `0x80035F10` | `jcsGetLocationInfo(rsSoundLocation)` | JCSOUND.CPP:1294 |
| `0x80035F8C` | `rMakePuddle` | RADMEM.CPP:86 |
| `0x80035FE0` | `rRemovePuddle(void**, _MEMPUDDLE*)` | RADMEM.CPP:118 |
| `0x80036050` | `rCreateMemPool` | RADMEM.CPP:141 |
| `0x800360A0` | `rDeleteMemPool` | RADMEM.CPP:162 |
| `0x800360F8` | `_rPMalloc` | RADMEM.CPP:200 |
| `0x80036138` | `rPFree` | RADMEM.CPP:218 |
| `0x80036160` | `_rPSMalloc` | RADMEM.CPP:230 |
| `0x800363C4` | `rJoinFreeNodes(void**, _FREENODE*)` | RADMEM.CPP:382 |
| `0x80036418` | `rPSFree` | RADMEM.CPP:398 |
| `0x800368F8` | `printf_CharOut` | RPRINTF.C:54 |
| `0x80036940` | `printf_IntOut` | RPRINTF.C:70 |
| `0x80036C0C` | `printf_FixedOut` | RPRINTF.C:154 |
| `0x80036D4C` | `printf_VectOut` | RPRINTF.C:233 |
| `0x80036EBC` | `rVSPrintf` | RPRINTF.C:269 |
| `0x800372A0` | `rprintf` | RPRINTF.C:439 |
| `0x800372D8` | `rSPrintf` | RPRINTF.C:449 |
| `0x80037310` | `ccMinNode::ccMinNode()` | CCLIST.CPP:245 |
| `0x80037324` | `_._9ccMinNode` | CCLIST.CPP:253 |
| `0x80037358` | `ccNode::ccNode()` | CCLIST.CPP:259 |
| `0x8003739C` | `_._6ccNode` | CCLIST.CPP:276 |
| `0x800373F0` | `ccNode::SetName(const char*, int)` | CCLIST.CPP:296 |
| `0x80037494` | `ccNode::SetNameNoAlloc(const char*)` | CCLIST.CPP:341 |
| `0x800374EC` | `ccMinList::GetNumElements() const` | CCLIST.CPP:369 |
| `0x80037510` | `ccMinList::AddNode(ccMinNode*, ccMinNode*)` | CCLIST.CPP:389 |
| `0x80037570` | `ccMinList::RemNode(ccMinNode*)` | CCLIST.CPP:447 |
| `0x800375E8` | `ccMinList::RemHead()` | CCLIST.CPP:486 |
| `0x80037620` | `ccList::FindNode(const char*, ccNode*) const` | CCLIST.CPP:559 |
| `0x80037664` | `ccList::FindNodeCRC(unsigned long, ccNode*) const` | CCLIST.CPP:565 |
| `0x800376A4` | `ccList::Sort(ccNode*(*)(ccNode*)*, int)` | CCLIST.CPP:590 |
| `0x800377FC` | `CheckPriReverse(ccNode*, ccNode*)` | CCLIST.CPP:715 |
| `0x8003780C` | `ccList::SortPriReverse()` | CCLIST.CPP:740 |
| `0x80037830` | `ccList::AddNodePri(ccNode*)` | CCLIST.CPP:764 |
| `0x8003789C` | `_SetControllerShock(hdMenuItem*)` | GAMEMENU.CPP:126 |
| `0x8003791C` | `gameMenu::_ResumeGame(hdMenuItem*)` | GAMEMENU.CPP:140 |
| `0x80037924` | `gameMenu::_ExitGame(hdMenuItem*)` | GAMEMENU.CPP:145 |
| `0x80037970` | `gameMenu::PushMenu(hdMenu*)` | GAMEMENU.CPP:164 |
| `0x80037A3C` | `gameMenu::PopMenu()` | GAMEMENU.CPP:180 |
| `0x80037A88` | `gameMenu::Activate()` | GAMEMENU.CPP:190 |
| `0x80037B6C` | `gameMenu::InputItemPush()` | GAMEMENU.CPP:213 |
| `0x80037BEC` | `gameMenu::InputPadUp()` | GAMEMENU.CPP:225 |
| `0x80037C40` | `gameMenu::InputPadDown()` | GAMEMENU.CPP:236 |
| `0x80037C94` | `gameMenu::gameMenu()` | GAMEMENU.CPP:252 |
| `0x80037CE4` | `_._8gameMenu` | GAMEMENU.CPP:260 |
| `0x80037D0C` | `gameMenu::SelfInit()` | GAMEMENU.CPP:264 |
| `0x80037DA0` | `gameMenu::GotoStartScreen()` | GAMEMENU.CPP:277 |
| `0x80037E14` | `gameMenu::Deactivate()` | GAMEMENU.CPP:290 |
| `0x80037E64` | `gameMenu::ShowPauseMenu()` | GAMEMENU.CPP:302 |
| `0x80037E88` | `gameMenu::ShowLoadingScreenText(unsigned long, unsigned long)` | GAMEMENU.CPP:308 |
| `0x80037FC0` | `gameMenu::HandleInputChange()` | GAMEMENU.CPP:369 |
| `0x800380D0` | `(nw__FUiPPv)` | DATABASE.CPP:192 |
| `0x8003812C` | `(void, n__FUiPPv)` | DATABASE.CPP:216 |
| `0x80038188` | `DBAttrib::DBAttrib()` | DATABASE.CPP:248 |
| `0x8003819C` | `_._8DBAttrib` | DATABASE.CPP:259 |
| `0x80038200` | `DBAttrib::GetAttribString() const` | DATABASE.CPP:269 |
| `0x80038254` | `DBAttrib::GetAttribValue() const` | DATABASE.CPP:284 |
| `0x80038260` | `DBAttrib::SetAttribString(unsigned long, const char*)` | DATABASE.CPP:292 |
| `0x800382C8` | `DBAttrib::SetAttribValue(unsigned long, unsigned long)` | DATABASE.CPP:311 |
| `0x800382DC` | `DBRoot::GetAttribByIndex(unsigned int) const` | DATABASE.CPP:348 |
| `0x80038304` | `DBRoot::FindAttrib(unsigned long) const` | DATABASE.CPP:361 |
| `0x80038354` | `DBRoot::FindAttribValue(unsigned long, unsigned long*) const` | DATABASE.CPP:381 |
| `0x800383AC` | `DBRoot::AllocatePermanentAttributeArray(unsigned int)` | DATABASE.CPP:403 |
| `0x80038434` | `DBRoot::DeallocatePermanentAttributeArray()` | DATABASE.CPP:410 |
| `0x800384B4` | `DBRoot::AddAttribNumber(unsigned int, unsigned long, unsigned long)` | DATABASE.CPP:425 |
| `0x800384F8` | `DBRoot::AddAttribString(unsigned int, unsigned long, const char*)` | DATABASE.CPP:436 |
| `0x8003853C` | `DBRoot::AddPermanentAttribString(unsigned int, unsigned long, const char*)` | DATABASE.CPP:445 |
| `0x8003856C` | `DBRoot::AddPermanentAttribNumber(unsigned int, unsigned long, unsigned long)` | DATABASE.CPP:454 |
| `0x8003859C` | `DBRoot::Process(unsigned long*)` | DATABASE.CPP:463 |
| `0x80038774` | `DBLine::AddVertex(long, long, long)` | DATABASE.CPP:522 |
| `0x80038804` | `DBMesh::SetFileName(const char*)` | DATABASE.CPP:532 |
| `0x80038854` | `DBVolume::IsInside(const tagLVector&) const` | DATABASE.CPP:540 |
| `0x800388E4` | `Database::AnalyzeMesh(DBRoot*)` | DATABASE.CPP:561 |
| `0x8003895C` | `Database::Database()` | DATABASE.CPP:584 |
| `0x80038A30` | `_._8Database` | DATABASE.CPP:590 |
| `0x80038AD8` | `Database::InternalOpen()` | DATABASE.CPP:595 |
| `0x80038AE0` | `Database::InternalClose()` | DATABASE.CPP:599 |
| `0x80038B00` | `Database::PreScan()` | DATABASE.CPP:608 |
| `0x80038B48` | `Database::Scan(char*, unsigned long)` | DATABASE.CPP:622 |
| `0x800390C8` | `Database::Close()` | DATABASE.CPP:821 |
| `0x80039154` | `Database::GetFirstSphere()` | DATABASE.CPP:854 |
| `0x80039160` | `Database::GetFirstLine()` | DATABASE.CPP:860 |
| `0x8003916C` | `Database::GetFirstPath()` | DATABASE.CPP:866 |
| `0x80039178` | `Database::GetFirstPoint()` | DATABASE.CPP:872 |
| `0x80039184` | `Database::GetFirstVolume()` | DATABASE.CPP:878 |
| `0x80039190` | `Database::GetFirstMesh()` | DATABASE.CPP:884 |
| `0x8003919C` | `Database::GetFirstBlock()` | DATABASE.CPP:891 |
| `0x800391A8` | `Database::FindSphere(const char*, DBSphere*)` | DATABASE.CPP:901 |
| `0x800391C8` | `Database::FindSphere(const char*)` | DATABASE.CPP:938 |
| `0x800391EC` | `Database::FindLine(const char*)` | DATABASE.CPP:944 |
| `0x80039210` | `Database::FindPath(const char*)` | DATABASE.CPP:950 |
| `0x80039234` | `Database::FindPoint(const char*)` | DATABASE.CPP:956 |
| `0x80039258` | `Database::FindPath(unsigned long)` | DATABASE.CPP:978 |
| `0x80039288` | `Database::GetPointsList()` | DATABASE.CPP:992 |
| `0x80039290` | `static_destroy(__nw__FUiPPv)` | DATABASE.CPP:1042 |
| `0x800392D0` | `static_init(__nw__FUiPPv)` | DATABASE.CPP:1042 |
| `0x80039314` | `_._6DBMesh` | DATABASE.HPP:317 |
| `0x80039374` | `_._6DBPath` | DATABASE.HPP:301 |
| `0x800393D8` | `_._6DBLine` | DATABASE.HPP:282 |
| `0x8003943C` | `_._12DBLineVertex` | DATABASE.HPP:270 |
| `0x80039464` | `_._8DBVolume` | DATABASE.HPP:255 |
| `0x8003948C` | `_._8DBSphere` | DATABASE.HPP:245 |
| `0x800394B4` | `_._7DBPoint` | DATABASE.HPP:234 |
| `0x800394DC` | `_._6DBRoot` | DATABASE.HPP:196 |
| `0x80039504` | `_._6ccList` | CCLIST.HPP:237 |
| `0x80039554` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x800395A8` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x800395F8` | `FreeAnimMemory(void*)` | CHARMGR.CPP:201 |
| `0x80039624` | `GetCompositeAnimationNameHash(const char*)` | CHARMGR.CPP:267 |
| `0x800396BC` | `GetPlayerMeshType(void)` | CHARMGR.CPP:301 |
| `0x800396C8` | `CharacterManager::CharacterManager()` | CHARMGR.CPP:334 |
| `0x80039794` | `_._16CharacterManager` | CHARMGR.CPP:388 |
| `0x800397C4` | `CharacterManager::OpenCharacter(Q22AI10ThingTypes)` | CHARMGR.CPP:425 |
| `0x80039808` | `CharacterManager::CloseCharacter(Q22AI10ThingTypes)` | CHARMGR.CPP:467 |
| `0x80039830` | `CharacterManager::LoadCharacter(Q22AI10ThingTypesP15CharMgrCallback)` | CHARMGR.CPP:496 |
| `0x80039AE4` | `DeleteAndRemoveCompositeAnimation(tCompositeAnim*)` | CHARMGR.CPP:626 |
| `0x80039C3C` | `CharacterManager::UnloadCharacter(Q22AI10ThingTypes)` | CHARMGR.CPP:693 |
| `0x80039DC4` | `CharacterManager::ReloadCharacter(Q22AI10ThingTypesQ216CharacterManager8MeshTypeP15CharMgrCallback)` | CHARMGR.CPP:792 |
| `0x8003A078` | `CharacterManager::LoadCharTexture(Q22AI10ThingTypes)` | CHARMGR.CPP:931 |
| `0x8003A1A4` | `CharacterManager::IsCharacterLoaded(Q22AI10ThingTypes)` | CHARMGR.CPP:1010 |
| `0x8003A1D4` | `CharacterManager::GetNumberCharactersLoaded()` | CHARMGR.CPP:1038 |
| `0x8003A20C` | `CharacterManager::EnableCache(Q22AI10ThingTypesb)` | CHARMGR.CPP:1073 |
| `0x8003A240` | `CharacterManager::LoadAnimation(Q22AI10ThingTypesUlP15CharMgrCallback)` | CHARMGR.CPP:1309 |
| `0x8003A328` | `CharacterManager::LoadAnimation(Q22AI10ThingTypes9AnimEnumsUlP15CharMgrCallback)` | CHARMGR.CPP:1383 |
| `0x8003A3B0` | `CallbackHelper(_RTASK*)` | CHARMGR.CPP:1414 |
| `0x8003A3EC` | `CharacterManager::LoadAnimation(Q22AI10ThingTypes9AnimEnumsP15CharMgrCallback)` | CHARMGR.CPP:1448 |
| `0x8003A7E8` | `CharacterManager::UnloadAnimation(Q22AI10ThingTypesUl)` | CHARMGR.CPP:1740 |
| `0x8003A8C0` | `CharacterManager::UnloadAnimation(Q22AI10ThingTypes9AnimEnumsUl)` | CHARMGR.CPP:1813 |
| `0x8003A930` | `CharacterManager::UnloadAnimation(Q22AI10ThingTypes9AnimEnums)` | CHARMGR.CPP:1838 |
| `0x8003AC44` | `CharacterManager::GetAnimation(Q22AI10ThingTypes9AnimEnums)` | CHARMGR.CPP:1968 |
| `0x8003ACBC` | `CharacterManager::LookUpAnimation(Q22AI10ThingTypesPCc)` | CHARMGR.CPP:2023 |
| `0x8003AD44` | `CharacterManager::PurgeLevel()` | CHARMGR.CPP:2054 |
| `0x8003AE5C` | `CharacterManager::CharDataLoadCallback(long, long, long)` | CHARMGR.CPP:2204 |
| `0x8003B2C4` | `P3DLoadCallback(tEntity*)` | CHARMGR.CPP:2414 |
| `0x8003B2D8` | `P3DLoadCallbackParam(tEntity*)` | CHARMGR.CPP:2427 |
| `0x8003B2EC` | `CharacterManager::AnimLoadCallback(long, long, long)` | CHARMGR.CPP:2459 |
| `0x8003B660` | `CharacterManager::AnimLoadCacheCallback(_RTASK*)` | CHARMGR.CPP:2655 |
| `0x8003B694` | `CharFile::CharFile(Q22AI10ThingTypes)` | CHARMGR.CPP:2682 |
| `0x8003B798` | `_._8CharFile` | CHARMGR.CPP:2751 |
| `0x8003B83C` | `CharFile::AddRef()` | CHARMGR.CPP:2787 |
| `0x8003B850` | `CharFile::DeleteRef()` | CHARMGR.CPP:2805 |
| `0x8003B88C` | `CharFile::Find(Q22AI10ThingTypes)` | CHARMGR.CPP:2831 |
| `0x8003B8C4` | `CharFile::FindAnim(unsigned long)` | CHARMGR.CPP:2861 |
| `0x8003B914` | `CharFile::EnableCache(bool)` | CHARMGR.CPP:2893 |
| `0x8003B964` | `AnimCallback::AnimCallback(Q22AI10ThingTypesiUlP15CharMgrCallback)` | CHARMGR.CPP:2935 |
| `0x8003B99C` | `AnimCallback::Callback()` | CHARMGR.CPP:2963 |
| `0x8003BA4C` | `_._12AnimCallback` | CHARMGR.CPP:170 |
| `0x8003BA80` | `CharacterManager::InternalReset()` | CHARMGR.HPP:204 |
| `0x8003BA88` | `CharacterManager::InternalOpen()` | CHARMGR.HPP:203 |
| `0x8003BA90` | `CharacterManager::InternalClose()` | CHARMGR.HPP:202 |
| `0x8003BA98` | `_._15CharMgrCallback` | CHARMGR.HPP:83 |
| `0x8003BACC` | `CharMgrCallback::Callback()` | CHARMGR.HPP:82 |
| `0x8003BAD8` | `_._7tLoader` | TLOADER.HPP:76 |
| `0x8003BB0C` | `runDirector(Handler*)` | DIRECTOR.CPP:2570 |
| `0x8003BB34` | `DrawDirectorOverlays(Handler*)` | DIRECTOR.CPP:2578 |
| `0x8003BB90` | `Director::updateVramAnims()` | DIRECTOR.CPP:2597 |
| `0x8003BBE0` | `Director::cleanUpTexAnim()` | DIRECTOR.CPP:2611 |
| `0x8003BE10` | `_._8Director` | DIRECTOR.CPP:2681 |
| `0x8003C044` | `Director::LevelReset()` | DIRECTOR.CPP:2731 |
| `0x8003C04C` | `Director::InternalReset()` | DIRECTOR.CPP:2736 |
| `0x8003C11C` | `Director::InternalClose()` | DIRECTOR.CPP:2757 |
| `0x8003C234` | `Director::SetScript()` | DIRECTOR.CPP:2765 |
| `0x8003C268` | `Director::SetCodeSnip(long*, Thing*)` | DIRECTOR.CPP:2782 |
| `0x8003C298` | `Director::Process()` | DIRECTOR.CPP:2806 |
| `0x8003D5A4` | `Director::ProcessSoundScript()` | DIRECTOR.CPP:3576 |
| `0x8003D634` | `Director::Timer()` | DIRECTOR.CPP:3611 |
| `0x8003D6CC` | `Director::Loop()` | DIRECTOR.CPP:3637 |
| `0x8003D6D4` | `Director::SetDesiredWideScreen()` | DIRECTOR.CPP:3642 |
| `0x8003D800` | `Director::ProcessEdison()` | DIRECTOR.CPP:3689 |
| `0x8003D87C` | `Director::ProcessModelFunc()` | DIRECTOR.CPP:3711 |
| `0x8003D884` | `Director::ProcessCameraFunc()` | DIRECTOR.CPP:3716 |
| `0x8003DC44` | `Director::ProcessHudFunc()` | DIRECTOR.CPP:3845 |
| `0x8003DD10` | `Director::ProcessHumanoidFunc()` | DIRECTOR.CPP:3894 |
| `0x8003E0D4` | `Director::ProcessLadderFunc()` | DIRECTOR.CPP:4003 |
| `0x8003E378` | `Director::ProcessDoorFunc()` | DIRECTOR.CPP:4069 |
| `0x8003E71C` | `Director::DetermineVictoryIdle()` | DIRECTOR.CPP:4170 |
| `0x8003E864` | `Director::DetermineLevelIntro()` | DIRECTOR.CPP:4260 |
| `0x8003EA4C` | `Director::DetermineDeath()` | DIRECTOR.CPP:4382 |
| `0x8003EB14` | `Director::WaitAnimationDone()` | DIRECTOR.CPP:4443 |
| `0x8003EB88` | `Director::ProcessDynamicAnimFunc()` | DIRECTOR.CPP:4469 |
| `0x8003ECD4` | `Director::HandleWideScreen()` | DIRECTOR.CPP:4539 |
| `0x8003ED90` | `Director::DrawWideScreenPolys()` | DIRECTOR.CPP:4582 |
| `0x8003F0E4` | `Director::PurgeAnims()` | DIRECTOR.CPP:4662 |
| `0x8003F104` | `Director::DoesLevelHaveExtraMem(long)` | DIRECTOR.CPP:4669 |
| `0x8003F138` | `setJackieCheckpoint(unsigned long)` | DIRECTOR.CPP:4696 |
| `0x8003F184` | `checkPoint(tagLVector&, long)` | DIRECTOR.CPP:4711 |
| `0x8003F1E0` | `HandlerSet::PurgeHandlers()` | HNDLRSET.HPP:110 |
| `0x8003F278` | `_._10HandlerSet` | HNDLRSET.HPP:88 |
| `0x8003F2D4` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x8003F31C` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8003F36C` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8003F3C0` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8003F410` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x8003F44C` | `HUD::HUD()` | HUD.CPP:318 |
| `0x8003F53C` | `_._3HUD` | HUD.CPP:331 |
| `0x8003F650` | `HUD::InternalReset()` | HUD.CPP:345 |
| `0x8003F658` | `HUD::DisplayXHUD(Handler*)` | HUD.CPP:352 |
| `0x8003F67C` | `HUD::Display()` | HUD.CPP:357 |
| `0x8003F6F8` | `HUD::SetHUDVisible(int, int)` | HUD.CPP:367 |
| `0x8003F88C` | `HUD::DisplayTally(int)` | HUD.CPP:411 |
| `0x8003F8D0` | `HUD::ShowDestLevel()` | HUD.CPP:418 |
| `0x8003F918` | `HUD::SelfInit()` | HUD.CPP:431 |
| `0x8003FAB4` | `HUD::EnableInput(int)` | HUD.CPP:473 |
| `0x8003FB34` | `HUD::DebugDisplay(int)` | HUD.CPP:487 |
| `0x8003FC08` | `HUD::UpdateScreen(oxScreenManager*)` | HUD.CPP:599 |
| `0x8003FC10` | `HUD::SelfUpdate()` | HUD.CPP:611 |
| `0x8003FCA0` | `HUD::FindScreen(unsigned long)` | HUD.CPP:635 |
| `0x8003FCB4` | `HUD::GetScreenNames()` | HUD.CPP:641 |
| `0x8003FCC0` | `HUD::GetGameData()` | HUD.CPP:648 |
| `0x8003FD84` | `HUD::UpdateBonusScore(long, long, const tagLVector&)` | HUD.CPP:697 |
| `0x8003FDA4` | `HUD::TriggerBonusUpdate()` | HUD.CPP:706 |
| `0x8003FDC4` | `HUD::DisplayTake(int, bool)` | HUD.CPP:727 |
| `0x8003FE14` | `HUD::DisplayExtraTake(const tagLVector&)` | HUD.CPP:736 |
| `0x8003FE54` | `HUD::UpdateHealth(long, long)` | HUD.CPP:751 |
| `0x8003FF98` | `HUD::SetFoe(Humanoid*)` | HUD.CPP:809 |
| `0x80040050` | `HUD::UpdateFoe(Humanoid*)` | HUD.CPP:820 |
| `0x8004007C` | `HUD::TriggerButtonCallback(ButtonHook*)` | HUD.CPP:831 |
| `0x800400DC` | `HUD::ToggleShowAll()` | HUD.CPP:842 |
| `0x80040184` | `HUD::ShowBossHealth(const char*)` | HUD.CPP:858 |
| `0x8004028C` | `HUD::OnLoadLevel()` | HUD.CPP:887 |
| `0x80040294` | `HUD::OnUnloadLevel()` | HUD.CPP:891 |
| `0x800402EC` | `ErrorScreen::GetScreenNames()` | HUD.CPP:900 |
| `0x800402F8` | `ErrorScreen::SetErrorMessage(int)` | HUD.CPP:912 |
| `0x8004033C` | `(thunk_48__._3HUD)` | HUD.HPP:264 |
| `0x8004035C` | `(thunk_48_UpdateScreen__3HUDP15oxScreenManager)` | HUD.HPP:264 |
| `0x8004037C` | `_._11ErrorScreen` | HUD.HPP:272 |
| `0x8004039C` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x800403D8` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x80040420` | `oxScreenManager::oxScreenManager()` | OXSCRMGR.CPP:60 |
| `0x80040458` | `_._15oxScreenManager` | OXSCRMGR.CPP:72 |
| `0x80040514` | `oxScreenManager::Update()` | OXSCRMGR.CPP:85 |
| `0x8004059C` | `oxScreenManager::Render()` | OXSCRMGR.CPP:98 |
| `0x800405F0` | `oxScreenManager::FindScreen(unsigned long)` | OXSCRMGR.CPP:105 |
| `0x800405F8` | `oxScreenManager::FindOverlay(char*)` | OXSCRMGR.CPP:111 |
| `0x80040634` | `oxScreenManager::GetScreenNames()` | OXSCRMGR.CPP:132 |
| `0x8004063C` | `oxScreenManager::GotoScreen(unsigned long)` | OXSCRMGR.CPP:140 |
| `0x8004064C` | `oxScreenManager::ScreenOperation()` | OXSCRMGR.CPP:148 |
| `0x80040784` | `oxScreenManager::FindOverlay(unsigned long)` | OXSCRMGR.CPP:192 |
| `0x800407B4` | `oxScreenManager::Init(char*, oxScreenManager*)` | OXSCRMGR.CPP:201 |
| `0x80040910` | `oxScreenManager::SelfUpdate()` | OXSCRMGR.CPP:238 |
| `0x80040918` | `oxScreenManager::GetScreenHash(unsigned long)` | OXSCRMGR.CPP:243 |
| `0x80040948` | `oxScreenManager::GotoStartScreen()` | OXSCRMGR.CPP:248 |
| `0x80040968` | `oxScreenManager::SelfInit()` | OXSCRMGR.CPP:255 |
| `0x80040970` | `oxScreenManager::PushScreen(unsigned long)` | OXSCRMGR.CPP:262 |
| `0x80040980` | `oxScreenManager::PopScreen()` | OXSCRMGR.CPP:271 |
| `0x8004098C` | `oxScreenManager::GetSection()` | OXSCRMGR.CPP:279 |
| `0x80040998` | `oxFontFile::FontInit(char*)` | OXSCRMGR.CPP:284 |
| `0x80040A8C` | `oxFontFile::ReloadFont(char*)` | OXSCRMGR.CPP:317 |
| `0x80040BA0` | `oxFontFile::FindFont(char*)` | OXSCRMGR.CPP:357 |
| `0x80040BC4` | `MakeBox(tagLVector&, tagLVector&, const tagLVector&, tagLVector&, long, long, long)` | COLSECT.CPP:384 |
| `0x80040C94` | `CollisionSector::Zero()` | COLSECT.CPP:423 |
| `0x80040CD0` | `CollisionSector::Reset()` | COLSECT.CPP:450 |
| `0x80040D18` | `CollisionSector::AsynchLoad(int, unsigned long*)` | COLSECT.CPP:471 |
| `0x80040D8C` | `CollisionSector::GetBlockNumber(const tagLVector&)` | COLSECT.CPP:511 |
| `0x80040E44` | `CollisionSector::CheckWorldWallCollision(const tagLVector&, const tagLVector&, long, long, long, int, long&, _RMVECT16&, tagLVector&, int&, long&, N27)` | COLSECT.CPP:550 |
| `0x80041038` | `CollisionSector::CheckWorldWallCollision(const tagLVector&, const tagLVector&, long, long, long, long&, _RMVECT16&, tagLVector&, long&)` | COLSECT.CPP:657 |
| `0x800411F8` | `CollisionSector::FillWorldWallArray(const tagLVector&, const tagLVector&, const Wall**, int)` | COLSECT.CPP:863 |
| `0x80041384` | `CollisionSector::CheckArrayWallCollision(const Wall**, int, const tagLVector&, const tagLVector&, long, long, long, int)` | COLSECT.CPP:931 |
| `0x800415C0` | `CollisionSector::CheckArrayWallIntersection(const Wall**, int, tagLVector&, const tagLVector&, long, long, long, int)` | COLSECT.CPP:1011 |
| `0x800417B8` | `CollisionSector::GetWorldFloorHeight(const tagLVector&, long)` | COLSECT.CPP:1105 |
| `0x800417F0` | `CollisionSector::GetWorldFloorAndCeilingHeight(long&, long&, _RMVECT16&, long&, const tagLVector&, long)` | COLSECT.CPP:1142 |
| `0x80041980` | `CollisionSector::FillWorldFloorArray(const tagLVector&, const tagLVector&, const Floor**, int)` | COLSECT.CPP:1207 |
| `0x80041ADC` | `CollisionSector::GetArrayFloorAndCeilingHeight(const Floor**, int, long&, long&, _RMVECT16&, long&, int&, tagLVector&, const tagLVector&, long)` | COLSECT.CPP:1271 |
| `0x80041DC4` | `CollisionSector::LedgePrototype(const tagLVector&, const tagLVector&, long, long, _RMVECT16&, tagLVector&, long&, long)` | COLSECT.CPP:1403 |
| `0x80042278` | `CollisionSector::CollisionSector()` | COLSECT.CPP:1577 |
| `0x800422A0` | `CollisionSector::Unload()` | COLSECT.CPP:1587 |
| `0x800422C0` | `CollisionSector::Load(unsigned long*)` | COLSECT.CPP:1601 |
| `0x80042340` | `CollisionSector::DebugDrawSector(const tagLVector&)` | COLSECT.CPP:1645 |
| `0x80042454` | `DrawCopy(_RMVECT16&, const tagLVector&)` | COLSECT.CPP:1671 |
| `0x80042478` | `MyDrawQuad(const tagLVector&, N30RC9_RMVECT16)` | COLSECT.CPP:1687 |
| `0x80042554` | `DrawCollisionWall(const Wall&, const _RMVECT16&)` | COLSECT.CPP:1719 |
| `0x800425C8` | `DrawCollisionFloor(const Floor&)` | COLSECT.CPP:1747 |
| `0x80042670` | `CollisionSector::DebugDrawSector() const` | COLSECT.CPP:1769 |
| `0x800427AC` | `static_init(COLLISION_SECTOR_INDEX_X_MIN)` | COLSECT.CPP:1814 |
| `0x80042810` | `jcsDialogCleanUp(void)` | JCSDLG.CPP:309 |
| `0x800428B8` | `jcsGetDlgStatus(void)` | JCSDLG.CPP:368 |
| `0x800428C4` | `jcsInitializeDialog(void)` | JCSDLG.CPP:387 |
| `0x80042908` | `jcsIsPlayable(long)` | JCSDLG.CPP:428 |
| `0x800429A4` | `jcsIsPlaying(void)` | JCSDLG.CPP:488 |
| `0x800429CC` | `jcsIsPlaying(long)` | JCSDLG.CPP:506 |
| `0x80042A30` | `jcsKillDialogByHandle(long)` | JCSDLG.CPP:557 |
| `0x80042C78` | `jcsLoadDialog(rsCharacter, rsDialog, long)` | JCSDLG.CPP:772 |
| `0x80042FF4` | `jcsPauseDialog(void)` | JCSDLG.CPP:1064 |
| `0x80043044` | `jcsPlayDialog(long, const tagLVector*, unsigned long)` | JCSDLG.CPP:1121 |
| `0x80043218` | `jcsQueryDialogPriority(void)` | JCSDLG.CPP:1285 |
| `0x80043254` | `jcsQueryDialogPriority(long)` | JCSDLG.CPP:1312 |
| `0x800432B8` | `jcsResumeDialog(void)` | JCSDLG.CPP:1342 |
| `0x800432E0` | `jcsSetConfigurationDialog(const jcsSoundParams*)` | JCSDLG.CPP:1360 |
| `0x80043350` | `jcsSetLevelDialog(unsigned long)` | JCSDLG.CPP:1399 |
| `0x80043420` | `jcsSetListenerDialog(const tagLVector*, const long*)` | JCSDLG.CPP:1547 |
| `0x8004344C` | `jcsStartDialog(void)` | JCSDLG.CPP:1567 |
| `0x8004345C` | `jcsStopDialog(void)` | JCSDLG.CPP:1586 |
| `0x8004347C` | `jcsTerminateDialog(void)` | JCSDLG.CPP:1607 |
| `0x800434D8` | `jcsValidateHandle(long)` | JCSDLG.CPP:1639 |
| `0x80043568` | `CDDoneCallback(long, long, long)` | JCSDLG.CPP:1700 |
| `0x80043684` | `CDDoneDefer(Queue, State, Queue)` | JCSDLG.CPP:1819 |
| `0x80043700` | `CheckFlushCount(Queue)` | JCSDLG.CPP:1859 |
| `0x800437BC` | `ConflictWithOtherQueue(unsigned short, Queue)` | JCSDLG.CPP:1903 |
| `0x800437D4` | `CreateHandle(rsDialog, long, Queue)` | JCSDLG.CPP:1930 |
| `0x80043808` | `DialogTask(_RTASK*)` | JCSDLG.CPP:1957 |
| `0x80043978` | `FreeTransferBuffer(void)` | JCSDLG.CPP:2091 |
| `0x800439B0` | `GetHeader(unsigned long, unsigned int)` | JCSDLG.CPP:2113 |
| `0x80043A00` | `GetHeaderInfo(rsCharacter, unsigned short*, N21)` | JCSDLG.CPP:2158 |
| `0x80043A90` | `GetUnused(unsigned char, unsigned short)` | JCSDLG.CPP:2182 |
| `0x80043B08` | `GetVagSize(unsigned short)` | JCSDLG.CPP:2211 |
| `0x80043B30` | `IfCanLoad(rsCharacter, rsDialog, unsigned long*, rsDialog)` | JCSDLG.CPP:2239 |
| `0x80043C10` | `IsCurrentHandle(long)` | JCSDLG.CPP:2290 |
| `0x80043C80` | `IsEitherHandle(long)` | JCSDLG.CPP:2342 |
| `0x80043CD0` | `IsLoadableDialog(rsCharacter, rsDialog)` | JCSDLG.CPP:2362 |
| `0x80043D1C` | `IsPrimaryHandle(long)` | JCSDLG.CPP:2389 |
| `0x80043D58` | `IsSecondaryHandle(long)` | JCSDLG.CPP:2418 |
| `0x80043D94` | `IsVoicePlaying(void)` | JCSDLG.CPP:2446 |
| `0x80043DF0` | `KillAllDialog(void)` | JCSDLG.CPP:2469 |
| `0x80043E2C` | `LoadAllReady(rsCharacter, rsDialog, long)` | JCSDLG.CPP:2491 |
| `0x80043F5C` | `LoadToAudioMemory(void)` | JCSDLG.CPP:2577 |
| `0x80043FF4` | `MarkAsTooLarge(unsigned short)` | JCSDLG.CPP:2614 |
| `0x80044030` | `MarkAsUsed(unsigned short)` | JCSDLG.CPP:2637 |
| `0x8004406C` | `PlayLoaded(const tagLVector*, State)` | JCSDLG.CPP:2664 |
| `0x8004416C` | `PrepareDefer(rsCharacter, rsDialog, long, State, Queue)` | JCSDLG.CPP:2729 |
| `0x800441B4` | `PauseTimeOut(Queue)` | JCSDLG.CPP:2810 |
| `0x80044234` | `ReclaimUsed(unsigned char, unsigned short)` | JCSDLG.CPP:2847 |
| `0x800442AC` | `RequestLoad(long, rsCharacter, rsDialog, long, Queue, State)` | JCSDLG.CPP:2883 |
| `0x80044368` | `ResetDialogInfo(Queue, State)` | JCSDLG.CPP:2927 |
| `0x800443BC` | `ResumeTimeOut(Queue)` | JCSDLG.CPP:2955 |
| `0x80044420` | `SelectDialog(rsCharacter, rsDialog, unsigned long*, rsDialog)` | JCSDLG.CPP:2992 |
| `0x8004466C` | `SetDlgStatus(DlgStatus)` | JCSDLG.CPP:3137 |
| `0x80044678` | `SetHeader(unsigned long, unsigned char)` | JCSDLG.CPP:3179 |
| `0x8004468C` | `StartLoad(long, rsDialog, long, unsigned long, unsigned short, unsigned long, Queue, State)` | JCSDLG.CPP:3208 |
| `0x800447E8` | `StopVoice(void)` | JCSDLG.CPP:3266 |
| `0x8004481C` | `UpdateState(State)` | JCSDLG.CPP:3289 |
| `0x80044828` | `UpdateTimeOut(unsigned long, const tagLVector*, Queue)` | JCSDLG.CPP:3338 |
| `0x80044884` | `UpgradeDlgInfo(State)` | JCSDLG.CPP:3367 |
| `0x800448F4` | `UpgradeToPrimary(void)` | JCSDLG.CPP:3387 |
| `0x8004491C` | `rsdLoadCallback::Callback(rsdLoad&)` | RSDLOAD.HPP:53 |
| `0x8004492C` | `timePrivHandler(Handler*)` | TIME.CPP:51 |
| `0x80044950` | `Time::Time()` | TIME.CPP:59 |
| `0x800449E8` | `_._4Time` | TIME.CPP:80 |
| `0x80044A10` | `Time::InternalOpen()` | TIME.CPP:85 |
| `0x80044A18` | `Time::InternalClose()` | TIME.CPP:90 |
| `0x80044A38` | `Time::InternalReset()` | TIME.CPP:96 |
| `0x80044A40` | `Time::Step()` | TIME.CPP:102 |
| `0x80044A54` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x80044A9C` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x80044AD8` | `tCellAlligator::tCellAlligator()` | XCCIMAGE.CPP:20 |
| `0x80044AEC` | `tCellAlligator::InitCellArea(const xcRectSint16&)` | XCCIMAGE.CPP:29 |
| `0x80044B4C` | `tCellAlligator::InitPal4Area(const xcRectSint16&)` | XCCIMAGE.CPP:36 |
| `0x80044BAC` | `tCellAlligator::InitPal8Area(const xcRectSint16&)` | XCCIMAGE.CPP:43 |
| `0x80044C0C` | `tCellAlligator::DeleteAllocators()` | XCCIMAGE.CPP:50 |
| `0x80044C80` | `tCellAlligator::AllocCells(xcCellList*, unsigned long)` | XCCIMAGE.CPP:72 |
| `0x80044CA4` | `tCellAlligator::AllocPalettes4(xcCellList*, unsigned long)` | XCCIMAGE.CPP:82 |
| `0x80044CC8` | `tCellAlligator::AllocPalettes8(xcCellList*, unsigned long)` | XCCIMAGE.CPP:92 |
| `0x80044CEC` | `xcCellImage::xcCellImage(void*, xcCellImageMemoryTypeEnum)` | XCCIMAGE.CPP:132 |
| `0x80044DAC` | `_._11xcCellImage` | XCCIMAGE.CPP:173 |
| `0x80044E1C` | `xcCellImage::FreeRamIfOwner()` | XCCIMAGE.CPP:185 |
| `0x80044E50` | `xcCellImage::FreeVram()` | XCCIMAGE.CPP:195 |
| `0x80044ECC` | `xcCellImage::FreeRam()` | XCCIMAGE.CPP:220 |
| `0x80044F0C` | `xcCellImage::LoadToVram()` | XCCIMAGE.CPP:230 |
| `0x800451D0` | `mAtoi(const char*)` | WORLD.CPP:737 |
| `0x8004521C` | `World::PackLevelName(unsigned long, unsigned long)` | WORLD.CPP:757 |
| `0x8004522C` | `World::UnpackLevelName(unsigned long, unsigned long&, unsigned long&)` | WORLD.CPP:763 |
| `0x80045244` | `World::World()` | WORLD.CPP:772 |
| `0x80045340` | `_._5World` | WORLD.CPP:794 |
| `0x800455BC` | `World::InternalOpen()` | WORLD.CPP:834 |
| `0x800455C4` | `World::InternalClose()` | WORLD.CPP:842 |
| `0x800455FC` | `World::InternalReset()` | WORLD.CPP:855 |
| `0x80045634` | `World::_LevelMenuExecute(hdMenuItem*)` | WORLD.CPP:868 |
| `0x800456D8` | `World::GetCurLevelPetals()` | WORLD.CPP:901 |
| `0x800456F4` | `World::GetCurLevelID()` | WORLD.CPP:907 |
| `0x80045710` | `World::LevelIDToIndex(int)` | WORLD.CPP:913 |
| `0x80045768` | `World::LoadLevelNames()` | WORLD.CPP:947 |
| `0x80045D6C` | `World::LoadPermanent()` | WORLD.CPP:1062 |
| `0x80045F34` | `World::UnloadPetal()` | WORLD.CPP:1176 |
| `0x8004604C` | `World::LoadPetal(unsigned long)` | WORLD.CPP:1222 |
| `0x80046170` | `World::EstimateLoadTime(unsigned long, unsigned long, bool)` | WORLD.CPP:1285 |
| `0x80046208` | `World::UnloadLevelPart2()` | WORLD.CPP:1355 |
| `0x8004624C` | `World::LoadLevel(unsigned long)` | WORLD.CPP:1389 |
| `0x800463F0` | `World::PopulateWEffects()` | WORLD.CPP:1471 |
| `0x80046464` | `World::UnPopulateWEffects(unsigned long)` | WORLD.CPP:1520 |
| `0x800464D8` | `World::SwitchSetup(WDBSwitch*, DBRoot*)` | WORLD.CPP:1553 |
| `0x8004657C` | `World::ProcessSwitches()` | WORLD.CPP:1599 |
| `0x80046648` | `World::CheckSwitches(ccList*, Thing*)` | WORLD.CPP:1641 |
| `0x80046724` | `World::Construct()` | WORLD.CPP:1675 |
| `0x80046CB0` | `World::UnloadPermanent()` | WORLD.CPP:1886 |
| `0x80046CB8` | `World::Destruct()` | WORLD.CPP:1895 |
| `0x80046DE0` | `World::ResetLevel()` | WORLD.CPP:1918 |
| `0x80046E2C` | `World::UnloadLevel()` | WORLD.CPP:1937 |
| `0x80046F74` | `World::ExecuteLoadCallbacks()` | WORLD.CPP:1988 |
| `0x80046FC4` | `World::ExecuteUnloadCallbacks()` | WORLD.CPP:1999 |
| `0x80047014` | `DeletePlayerBlendAndAnimData(void)` | WORLD.CPP:2059 |
| `0x800470CC` | `_._15CharMgrCallback` | CHARMGR.HPP:83 |
| `0x80047100` | `CharMgrCallback::Callback()` | CHARMGR.HPP:82 |
| `0x8004710C` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8004715C` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x800471B0` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x80047200` | `HTickerToGameLoopPercentage(unsigned long)` | PROFILE.CPP:211 |
| `0x8004723C` | `Profile::Update()` | PROFILE.CPP:261 |
| `0x800472BC` | `Profile::Begin(ProfileCodeEnum)` | PROFILE.CPP:290 |
| `0x800472FC` | `Profile::End(ProfileCodeEnum)` | PROFILE.CPP:301 |
| `0x80047360` | `Profile::Sum()` | PROFILE.CPP:321 |
| `0x800473A4` | `Profile::Print()` | PROFILE.CPP:339 |
| `0x800474E0` | `Profile::Clear()` | PROFILE.CPP:386 |
| `0x8004750C` | `static_init(PROFILE_CODE_TYPE_NAME_ARRAY)` | PROFILE.CPP:398 |
| `0x80047560` | `VBlankLogo::VBlankLogo(long)` | LOADANIM.CPP:28 |
| `0x800476B8` | `_._10VBlankLogo` | LOADANIM.CPP:48 |
| `0x80047710` | `VBlankLogo::SetActive(int)` | LOADANIM.CPP:54 |
| `0x80047770` | `VBlankLogo::ClearVram()` | LOADANIM.CPP:72 |
| `0x80047778` | `VBlankLogo::Update(_RTASK*)` | LOADANIM.CPP:85 |
| `0x80047968` | `VBlankLogo::StartLogo(long)` | LOADANIM.CPP:167 |
| `0x800479BC` | `VBlankLogo::StopLogo()` | LOADANIM.CPP:183 |
| `0x80047A68` | `VBlankLogo::FillMeter(unsigned char)` | LOADANIM.CPP:207 |
| `0x80047AB0` | `static_init(_10VBlankLogo.Active)` | LOADANIM.CPP:218 |
| `0x80047AD4` | `Camera::Camera(const tagLVector*)` | CAMERA.CPP:470 |
| `0x80047B40` | `_._6Camera` | CAMERA.CPP:482 |
| `0x80047BE0` | `Camera::PurgeAnims()` | CAMERA.CPP:499 |
| `0x80047C5C` | `Camera::Reset()` | CAMERA.CPP:516 |
| `0x80047EAC` | `Camera::UpdateAnim()` | CAMERA.CPP:573 |
| `0x80047F28` | `Camera::Think()` | CAMERA.CPP:588 |
| `0x80047FD4` | `Camera::Move()` | CAMERA.CPP:619 |
| `0x800482DC` | `Camera::Update()` | CAMERA.CPP:665 |
| `0x8004850C` | `Camera::LookAtTarget(tagLVector*)` | CAMERA.CPP:781 |
| `0x80048718` | `Camera::_DebugCam()` | CAMERA.CPP:865 |
| `0x8004897C` | `Camera::_RigidCam()` | CAMERA.CPP:936 |
| `0x80048AC0` | `Camera::_FollowPath()` | CAMERA.CPP:974 |
| `0x80049C44` | `Camera::SetMode(long)` | CAMERA.CPP:1237 |
| `0x80049DC0` | `Camera::SetLookAtTarget(Thing*, unsigned short)` | CAMERA.CPP:1307 |
| `0x80049DE4` | `Camera::ShakeCamera(long)` | CAMERA.CPP:1368 |
| `0x80049DEC` | `Camera::CameraShake()` | CAMERA.CPP:1377 |
| `0x80049F9C` | `Camera::GetCameraVector()` | CAMERA.CPP:1418 |
| `0x8004A054` | `Camera::LoadAsyncAnim(long)` | CAMERA.CPP:1471 |
| `0x8004A0F0` | `Camera::PlayAsyncAnim()` | CAMERA.CPP:1499 |
| `0x8004A220` | `Camera::DeleteAsyncAnim()` | CAMERA.CPP:1530 |
| `0x8004A29C` | `static_destroy(splatClosestDistance)` | CAMERA.CPP:1544 |
| `0x8004A2D4` | `static_init(splatClosestDistance)` | CAMERA.CPP:1544 |
| `0x8004A310` | `AsyncAnimCallback::Callback()` | CAMERA.CPP:1451 |
| `0x8004A368` | `_._17AsyncAnimCallback` | CAMERA.CPP:1460 |
| `0x8004A39C` | `Camera::SetTrackingTime(const _RMVECT16&)` | CAMERA.CPP:1351 |
| `0x8004A400` | `Camera::SetMovementTime(const _RMVECT16&)` | CAMERA.CPP:1338 |
| `0x8004A464` | `Camera::SetCurFOV(long)` | CAMERA.CPP:1298 |
| `0x8004A500` | `Camera::SetFOV(long)` | CAMERA.CPP:1293 |
| `0x8004A508` | `_._15CharMgrCallback` | CHARMGR.HPP:83 |
| `0x8004A53C` | `CharMgrCallback::Callback()` | CHARMGR.HPP:82 |
| `0x8004A548` | `CameraManager::CameraManager()` | CAMMGR.CPP:343 |
| `0x8004A580` | `_._13CameraManager` | CAMMGR.CPP:349 |
| `0x8004A5B0` | `cameraLoadFunc(Callback*)` | CAMMGR.CPP:354 |
| `0x8004A5F4` | `CameraManager::InternalOpen()` | CAMMGR.CPP:362 |
| `0x8004A668` | `CameraManager::SetupPaths()` | CAMMGR.CPP:389 |
| `0x8004A780` | `CameraAnchor::CameraAnchor()` | CAMMGR.CPP:428 |
| `0x8004A7F8` | `_._12CameraAnchor` | CAMMGR.CPP:432 |
| `0x8004A870` | `CameraAnchor::AddCameraSourcePath(DBPath*)` | CAMMGR.CPP:437 |
| `0x8004A968` | `CameraAnchor::AddCameraTargetPath(DBPath*)` | CAMMGR.CPP:485 |
| `0x8004AA30` | `CameraAnchor::GetPathWithID(unsigned long)` | CAMMGR.CPP:515 |
| `0x8004AA6C` | `CameraAnchor::FindClosestNodes(G10tagLVectorPP16DBCameraPathNodeT2)` | CAMMGR.CPP:532 |
| `0x8004AB6C` | `DBCameraPath::DBCameraPath()` | CAMMGR.CPP:680 |
| `0x8004ABE8` | `_._12DBCameraPath` | CAMMGR.CPP:686 |
| `0x8004AC40` | `DBCameraPath::AddSourceNode(DBPoint*)` | CAMMGR.CPP:690 |
| `0x8004ADD0` | `DBCameraPath::AddTargetNode(DBPoint*, int)` | CAMMGR.CPP:743 |
| `0x8004AED0` | `DBCameraPath::FinalizeBoundaries(long)` | CAMMGR.CPP:900 |
| `0x8004AF1C` | `DBCameraPath::InRange(G10tagLVector)` | CAMMGR.CPP:912 |
| `0x8004AFB0` | `DBCameraPath::FindClosestNodes(G10tagLVectorPP16DBCameraPathNodeT2)` | CAMMGR.CPP:926 |
| `0x8004B480` | `DBCameraPathNode::DBCameraPathNode()` | CAMMGR.CPP:1061 |
| `0x8004B4B4` | `_._16DBCameraPathNode` | CAMMGR.CPP:1065 |
| `0x8004B4DC` | `_._8Callback` | CALLBACK.HPP:41 |
| `0x8004B530` | `DataAnchor::RemElement(ccNode*)` | ANCHOR.HPP:53 |
| `0x8004B550` | `DataAnchor::AddElement(ccNode*)` | ANCHOR.HPP:52 |
| `0x8004B57C` | `_._10DataAnchor` | ANCHOR.HPP:43 |
| `0x8004B5D4` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8004B624` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8004B678` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8004B6C8` | `MyDrawLine(const _RMVECT16&, N30)` | PSXSUBS.CPP:169 |
| `0x8004B8DC` | `MyDrawBoxCenterRadius(const _RMVECT16&, long, long)` | PSXSUBS.CPP:277 |
| `0x8004B9F4` | `MyDrawBox(const _RMVECT16&, N20)` | PSXSUBS.CPP:290 |
| `0x8004BE44` | `GetScreenCoordinates(const tagLVector&, tagLVector&)` | PSXSUBS.CPP:347 |
| `0x8004BF88` | `ccFile::ConvertLong(unsigned long)` | CCFILE.CPP:196 |
| `0x8004BFD4` | `ccFile::ConvertWord(unsigned short)` | CCFILE.CPP:214 |
| `0x8004C000` | `ccFile::SetDebug(short)` | CCFILE.CPP:228 |
| `0x8004C008` | `ccFile::ccFile()` | CCFILE.CPP:236 |
| `0x8004C068` | `_._6ccFile` | CCFILE.CPP:252 |
| `0x8004C0BC` | `ccFile::OpenMem(unsigned char*, unsigned long)` | CCFILE.CPP:271 |
| `0x8004C0D4` | `ccFile::Open(const char*, unsigned short)` | CCFILE.CPP:279 |
| `0x8004C18C` | `ccFile::Close()` | CCFILE.CPP:378 |
| `0x8004C1F0` | `ccFile::ReadString(void*, unsigned long)` | CCFILE.CPP:417 |
| `0x8004C2B4` | `ccFile::Read(void*, unsigned long)` | CCFILE.CPP:464 |
| `0x8004C374` | `ccFile::Write(void*, unsigned long)` | CCFILE.CPP:513 |
| `0x8004C3BC` | `ccFile::Seek(unsigned long, unsigned short)` | CCFILE.CPP:552 |
| `0x8004C4A0` | `ccFile::WriteLong(unsigned long)` | CCFILE.CPP:618 |
| `0x8004C4E0` | `ccFile::WriteWord(unsigned short)` | CCFILE.CPP:624 |
| `0x8004C524` | `ccFile::WriteByte(unsigned char)` | CCFILE.CPP:630 |
| `0x8004C558` | `ccFile::ReadLong(unsigned long*)` | CCFILE.CPP:635 |
| `0x8004C5D0` | `ccFile::ReadWord(unsigned short*)` | CCFILE.CPP:650 |
| `0x8004C648` | `ccFile::ReadByte(unsigned char*)` | CCFILE.CPP:668 |
| `0x8004C6A4` | `ccFile::GetError()` | CCFILE.HPP:177 |
| `0x8004C6B0` | `ccFile::GetPosition()` | CCFILE.HPP:176 |
| `0x8004C6BC` | `ccFile::GetLength()` | CCFILE.HPP:175 |
| `0x8004C6C8` | `Effects::Die(int, int)` | EFFECTS.CPP:60 |
| `0x8004C778` | `Effects::Find(long, unsigned long)` | EFFECTS.CPP:106 |
| `0x8004C7D8` | `Effects::UnloadAll()` | EFFECTS.CPP:137 |
| `0x8004C854` | `Effects::UpdateAll()` | EFFECTS.CPP:174 |
| `0x8004C8BC` | `Effects::DrawEffects(int)` | EFFECTS.CPP:207 |
| `0x8004C9D8` | `Effects::AddEffect(int)` | EFFECTS.CPP:279 |
| `0x8004CA28` | `Effects::RemoveEffect()` | EFFECTS.CPP:299 |
| `0x8004CA5C` | `Effects::Effects()` | EFFECTS.CPP:314 |
| `0x8004CA98` | `_._7Effects` | EFFECTS.CPP:328 |
| `0x8004CAC0` | `static_destroy(Die__7Effectsii)` | EFFECTS.CPP:331 |
| `0x8004CB00` | `static_init(Die__7Effectsii)` | EFFECTS.CPP:331 |
| `0x8004CB44` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8004CB94` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8004CBE8` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8004CC38` | `scoreMgrPrivHandler(Handler*)` | SCOREMGR.CPP:233 |
| `0x8004CC5C` | `ScoreManager::ScoreManager()` | SCOREMGR.CPP:241 |
| `0x8004CCA8` | `_._12ScoreManager` | SCOREMGR.CPP:254 |
| `0x8004CCD8` | `ScoreManager::InternalOpen()` | SCOREMGR.CPP:268 |
| `0x8004CD64` | `ScoreManager::InternalClose()` | SCOREMGR.CPP:284 |
| `0x8004CD84` | `ScoreManager::InternalReset()` | SCOREMGR.CPP:294 |
| `0x8004CD8C` | `ScoreManager::InitGameStats()` | SCOREMGR.CPP:303 |
| `0x8004CDEC` | `ScoreManager::InitLevelStats()` | SCOREMGR.CPP:326 |
| `0x8004CEA0` | `ScoreManager::SetPar()` | SCOREMGR.CPP:364 |
| `0x8004CEE0` | `ScoreManager::OpenAllLevels()` | SCOREMGR.CPP:376 |
| `0x8004CF24` | `ScoreManager::GiveAllDragons()` | SCOREMGR.CPP:392 |
| `0x8004CF84` | `ScoreManager::Step()` | SCOREMGR.CPP:418 |
| `0x8004CFA4` | `ScoreManager::HandleLevelBegin()` | SCOREMGR.CPP:429 |
| `0x8004CFC4` | `ScoreManager::HandleLevelEnd()` | SCOREMGR.CPP:442 |
| `0x8004D0D4` | `ScoreManager::HandleLevelAbort()` | SCOREMGR.CPP:478 |
| `0x8004D0DC` | `ScoreManager::GetLevelEndRating()` | SCOREMGR.CPP:491 |
| `0x8004D144` | `ScoreManager::OpenPetal(unsigned long, unsigned long)` | SCOREMGR.CPP:527 |
| `0x8004D184` | `ScoreManager::HandleCheckpoint()` | SCOREMGR.CPP:542 |
| `0x8004D1DC` | `ScoreManager::HandleCheckpointBegin()` | SCOREMGR.CPP:562 |
| `0x8004D234` | `ScoreManager::Print() const` | SCOREMGR.CPP:584 |
| `0x8004D260` | `ScoreManager::RegisterCollectible(const Collectible*, int)` | SCOREMGR.CPP:611 |
| `0x8004D2E0` | `ScoreManager::RegisterGotCollectible(const Collectible*, int)` | SCOREMGR.CPP:649 |
| `0x8004D388` | `ScoreManager::AddFightPoints(long)` | SCOREMGR.CPP:693 |
| `0x8004D39C` | `ScoreManager::AddComboPoints(long)` | SCOREMGR.CPP:698 |
| `0x8004D3B0` | `ScoreManager::AddStylePoints(long)` | SCOREMGR.CPP:703 |
| `0x8004D3C4` | `ScoreManager::StepFighting()` | SCOREMGR.CPP:735 |
| `0x8004D408` | `ScoreManager::BreakFightingChain()` | SCOREMGR.CPP:762 |
| `0x8004D45C` | `ScoreManager::AddFightingPoints(long)` | SCOREMGR.CPP:780 |
| `0x8004D4C8` | `ScoreManager::HandleGameBegin()` | SCOREMGR.CPP:798 |
| `0x8004D518` | `ScoreManager::CalcGrade()` | SCOREMGR.CPP:809 |
| `0x8004D5AC` | `ScoreManager::CalcGradeXTakes(unsigned char)` | SCOREMGR.CPP:839 |
| `0x8004D5C0` | `ScoreManager::CalcGDrags(int)` | SCOREMGR.CPP:845 |
| `0x8004D5CC` | `ScoreManager::GetTotalGoldDragon()` | SCOREMGR.CPP:851 |
| `0x8004D69C` | `ScoreManager::IsDrunkenMasterSuitEnabled()` | SCOREMGR.CPP:883 |
| `0x8004D72C` | `CheckpointInfo::IsValid()` | SCOREMGR.CPP:910 |
| `0x8004D798` | `CheckpointInfo::SetValidState(int)` | SCOREMGR.CPP:935 |
| `0x8004D7E8` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x8004D830` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x8004D86C` | `dispBeginFrameHandler(Handler*)` | DISPLAY.CPP:105 |
| `0x8004D890` | `dispEndFrameHandler(Handler*)` | DISPLAY.CPP:112 |
| `0x8004D8B4` | `_._7Display` | DISPLAY.CPP:131 |
| `0x8004D8FC` | `Display::InternalClose()` | DISPLAY.CPP:156 |
| `0x8004D928` | `Display::InternalReset()` | DISPLAY.CPP:164 |
| `0x8004D948` | `static_destroy(__7Display)` | DISPLAY.CPP:168 |
| `0x8004D980` | `static_init(__7Display)` | DISPLAY.CPP:168 |
| `0x8004D9FC` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x8004DA44` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x8004DA80` | `DrawQuickReminder(void)` | CMNEFFCT.CPP:84 |
| `0x8004DA88` | `DrawQuickReminder2(void)` | CMNEFFCT.CPP:124 |
| `0x8004DA90` | `ComEffect::SetPrimMargin(unsigned long)` | CMNEFFCT.CPP:271 |
| `0x8004DA9C` | `ComEffect::PrimMarginSafe()` | CMNEFFCT.CPP:277 |
| `0x8004DAF0` | `ComEffect::BlownPrimMargin()` | CMNEFFCT.CPP:288 |
| `0x8004DB38` | `ComEffect::SetUpFirstGeo()` | CMNEFFCT.CPP:305 |
| `0x8004DB60` | `ComEffect::ComEffect()` | CMNEFFCT.CPP:352 |
| `0x8004DB9C` | `checkForAndFreeSequenceAnims(tAnimation*)` | CMNEFFCT.CPP:382 |
| `0x8004DC90` | `_._9ComEffect` | CMNEFFCT.CPP:421 |
| `0x8004DE40` | `ComEffect::GetClut(int)` | CMNEFFCT.CPP:499 |
| `0x8004DEE8` | `ComEffect::SetUpUVlists()` | CMNEFFCT.CPP:531 |
| `0x8004E028` | `ComEffect::AddUV(unsigned short*, short, short)` | CMNEFFCT.CPP:588 |
| `0x8004E140` | `ComEffect::SetUpVertexlists()` | CMNEFFCT.CPP:657 |
| `0x8004E3C8` | `ComEffect::SetVertexInfo(int, long)` | CMNEFFCT.CPP:732 |
| `0x8004E4D4` | `ComEffect::SetZFar()` | CMNEFFCT.CPP:777 |
| `0x8004E528` | `ComEffect::LoadGeo(int)` | CMNEFFCT.CPP:795 |
| `0x8004E580` | `ComEffect::LoadETree(int, int)` | CMNEFFCT.CPP:817 |
| `0x8004E6D4` | `ComEffect::LoadSTree(int, int)` | CMNEFFCT.CPP:861 |
| `0x8004E7F8` | `ComEffect::SetFrame(int)` | CMNEFFCT.CPP:897 |
| `0x8004E89C` | `ComEffect::EndOfFrame(int)` | CMNEFFCT.CPP:932 |
| `0x8004E8D4` | `ComEffect::PointInView(tagLVector&, long)` | CMNEFFCT.CPP:946 |
| `0x8004E950` | `ComEffect::Render(const tagLVector&, const _RMVECT16&, const _RMVECT16&, unsigned long)` | CMNEFFCT.CPP:1117 |
| `0x8004EE48` | `ComEffect::Render(MATRIX*, unsigned long)` | CMNEFFCT.CPP:1418 |
| `0x8004F040` | `ComEffect::GetGeo()` | CMNEFFCT.CPP:1551 |
| `0x8004F04C` | `ComEffect::GetGeo(int)` | CMNEFFCT.CPP:1556 |
| `0x8004F0AC` | `ComEffect::FindFirstGeo()` | CMNEFFCT.CPP:1582 |
| `0x8004F0CC` | `ComEffect::FindFirstGeo(int*)` | CMNEFFCT.CPP:1596 |
| `0x8004F204` | `ComEffect::FindNextGeo()` | CMNEFFCT.CPP:1657 |
| `0x8004F288` | `ComEffect::InitFastRender(tGeometry*)` | CMNEFFCT.CPP:1772 |
| `0x8004F318` | `ComEffect::DoFastRender()` | CMNEFFCT.CPP:1806 |
| `0x8004F350` | `ComEffect::FastRender()` | CMNEFFCT.CPP:1815 |
| `0x8004F538` | `ComEffect::FastPushMultMatrix(MATRIX*, MATRIX*)` | CMNEFFCT.CPP:1891 |
| `0x8004F5A0` | `ComEffect::FastPopMatrix(MATRIX*)` | CMNEFFCT.CPP:1899 |
| `0x8004F5E8` | `ComEffect::FastZSortDisplayGCT3(unsigned long)` | CMNEFFCT.CPP:1906 |
| `0x8004F89C` | `ComEffect::FastZSortDisplayGCT4(unsigned long)` | CMNEFFCT.CPP:2004 |
| `0x8004FBE4` | `ClearEasterEggs(void)` | EASTER.CPP:104 |
| `0x8004FC10` | `RecordEasterButtonPresses(unsigned long)` | EASTER.CPP:114 |
| `0x8004FC94` | `PrintEasterEggs(void)` | EASTER.CPP:131 |
| `0x8004FD3C` | `CheckEasterButtonPresses(SonyVButtons*)` | EASTER.CPP:164 |
| `0x8004FDC8` | `rPTraversePool` | FREEMEM.C:23 |
| `0x8004FE48` | `rPCountFree` | FREEMEM.C:56 |
| `0x8004FE70` | `rPLargestBlock` | FREEMEM.C:63 |
| `0x8004FE98` | `BlockManager::BlockManager()` | BLKMGR.CPP:148 |
| `0x8004FF1C` | `_._12BlockManager` | BLKMGR.CPP:169 |
| `0x80050014` | `BlockManager::AllocBlockPool()` | BLKMGR.CPP:178 |
| `0x800500CC` | `BlockManager::FreeBlockPool()` | BLKMGR.CPP:193 |
| `0x8005010C` | `BlockManager::_LoadBlocksFunc(Callback*)` | BLKMGR.CPP:207 |
| `0x80050220` | `BlockManager::_UnloadBlocksFunc(Callback*)` | BLKMGR.CPP:245 |
| `0x800502BC` | `BlockManager::InternalOpen()` | BLKMGR.CPP:258 |
| `0x80050384` | `BlockManager::InternalClose()` | BLKMGR.CPP:280 |
| `0x800503A4` | `BlockManager::InternalReset()` | BLKMGR.CPP:289 |
| `0x800503AC` | `BlockManager::RemoveBlock()` | BLKMGR.CPP:299 |
| `0x80050480` | `BlockManager::RemoveBlockHelper()` | BLKMGR.CPP:322 |
| `0x80050624` | `BlockManager::AddBlock(BlockNode*)` | BLKMGR.CPP:407 |
| `0x800506BC` | `BlockManager::CrossedBoundary()` | BLKMGR.CPP:486 |
| `0x800506E8` | `BlockManager::DemandLoading()` | BLKMGR.CPP:519 |
| `0x8005085C` | `BlockManager::LoadBlock(unsigned long, Block*)` | BLKMGR.CPP:591 |
| `0x800508CC` | `BlockManager::UnloadBlocks()` | BLKMGR.CPP:638 |
| `0x800509D4` | `BlockManager::LoadSingleBlockAndParse(unsigned long, char*)` | BLKMGR.CPP:662 |
| `0x80050A98` | `BlockManager::LoadBlocks(unsigned long)` | BLKMGR.CPP:695 |
| `0x80050C04` | `BlockManager::GetBlockNumber(const tagLVector&)` | BLKMGR.CPP:749 |
| `0x80050C70` | `BlockManager::IsValidBlockNumber(unsigned long)` | BLKMGR.CPP:769 |
| `0x80050CB4` | `BlockManager::InLoadList(unsigned long) const` | BLKMGR.CPP:785 |
| `0x80050CF4` | `BlockManager::InDrawList(unsigned long) const` | BLKMGR.CPP:807 |
| `0x80050D44` | `BlockManager::InActiveList(unsigned long) const` | BLKMGR.CPP:825 |
| `0x80050D94` | `BlockManager::UpdateToBeLoadedList(unsigned long)` | BLKMGR.CPP:852 |
| `0x80051750` | `BlockManager::UpdateAlreadyLoadedList()` | BLKMGR.CPP:1323 |
| `0x800518C4` | `BlockManager::GetBlock(unsigned long)` | BLKMGR.CPP:1374 |
| `0x800518FC` | `_._8Callback` | CALLBACK.HPP:41 |
| `0x80051950` | `_._9BlockList` | BLKMGR.HPP:193 |
| `0x800519A0` | `_._9BlockNode` | BLKMGR.HPP:165 |
| `0x800519F4` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x80051A48` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x80051A98` | `tPort::SetView(tView*)` | TPORT.CPP:229 |
| `0x80051B9C` | `tPort::GetWorldMatrix(MATRIX*)` | TPORT.CPP:271 |
| `0x80051BF4` | `tPort::TransMatrix(_RMVECT16*)` | TPORT.CPP:305 |
| `0x80051C24` | `tPort::TransMatrix(long, long, long)` | TPORT.CPP:310 |
| `0x80051C44` | `tPort::ScaleMatrix(long, long, long)` | TPORT.CPP:315 |
| `0x80051C6C` | `tPort::ScaleMatrix(_RMVECT16*)` | TPORT.CPP:321 |
| `0x80051C94` | `tPort::ScaleMatrix(long)` | TPORT.CPP:327 |
| `0x80051CBC` | `tPort::SyncCamView()` | TPORT.CPP:333 |
| `0x80052024` | `tPort::CVMToCTM()` | TPORT.CPP:388 |
| `0x80052058` | `tPort::SwapBuffers()` | TPORT.CPP:396 |
| `0x8005207C` | `tPort::FrameInit()` | TPORT.CPP:401 |
| `0x800520C0` | `tPort::SetLighting(unsigned long)` | TPORT.CPP:416 |
| `0x800521C0` | `tView::tView(unsigned long)` | TVIEW.CPP:56 |
| `0x800522AC` | `_._5tView` | TVIEW.CPP:80 |
| `0x8005233C` | `tView::SetCamera(tCamera*)` | TVIEW.CPP:88 |
| `0x80052344` | `tView::SetFog(tFog*)` | TVIEW.CPP:95 |
| `0x8005234C` | `tView::SetViewPort(long, long, long, long)` | TVIEW.CPP:100 |
| `0x80052364` | `tView::SetAmbientLight(unsigned long)` | TVIEW.CPP:112 |
| `0x8005236C` | `tView::SetBackgroundColour(unsigned long)` | TVIEW.CPP:117 |
| `0x80052374` | `tView::GetState()` | TVIEW.CPP:122 |
| `0x8005237C` | `tView::AddLight(tLight*)` | TVIEW.CPP:127 |
| `0x800523CC` | `tView::RemoveLight(unsigned char)` | TVIEW.CPP:140 |
| `0x800523F0` | `tView::BeginRender()` | TVIEW.CPP:161 |
| `0x80052490` | `tView::EndRender()` | TVIEW.CPP:231 |
| `0x800524A0` | `tView::SetupLayer(unsigned long, unsigned long, unsigned long, unsigned long)` | TVIEW.CPP:236 |
| `0x80052580` | `tView::SetLayer(unsigned long)` | TVIEW.CPP:256 |
| `0x80052598` | `tView::DeleteLayer(unsigned long)` | TVIEW.CPP:261 |
| `0x80052604` | `tView::CheckLayer(unsigned long)` | TVIEW.CPP:267 |
| `0x80052648` | `tView::StartLayer(unsigned long)` | TVIEW.CPP:273 |
| `0x8005268C` | `tView::EndLayer(unsigned long)` | TVIEW.CPP:279 |
| `0x800526D0` | `tView::EnterLayer(unsigned long)` | TVIEW.CPP:297 |
| `0x80052720` | `tView::ExitLayer(unsigned long)` | TVIEW.CPP:305 |
| `0x80052754` | `tView::SetupViewPort()` | TVIEW.CPP:312 |
| `0x80052780` | `tView::ClearViewPort()` | TVIEW.CPP:321 |
| `0x800528A0` | `tView::ClipView()` | TVIEW.CPP:337 |
| `0x80052B24` | `Block::Block()` | BLOCK.CPP:249 |
| `0x80052B88` | `_._5Block` | BLOCK.CPP:282 |
| `0x80052BB0` | `Block::Init(const DBVolume*)` | BLOCK.CPP:288 |
| `0x80052EA0` | `Block::SetDimension(const tagLVector&, const tagLVector&)` | BLOCK.CPP:442 |
| `0x80052F80` | `Block::Parse(unsigned long, char*)` | BLOCK.CPP:482 |
| `0x80052FF0` | `Block::Unload()` | BLOCK.CPP:514 |
| `0x80053024` | `Block::PointInBlock(const tagLVector&) const` | BLOCK.CPP:524 |
| `0x800530B0` | `Block::GetNextBlockNumber() const` | BLOCK.CPP:531 |
| `0x800530D4` | `Block::GetPrevBlockNumber() const` | BLOCK.CPP:549 |
| `0x800530F8` | `Block::Draw(const tagLVector&)` | BLOCK.CPP:600 |
| `0x8005328C` | `Block::LoadPrim(void*)` | BLOCK.CPP:668 |
| `0x800534C8` | `CFrontEndSound::ProcessSoundEvent(Q214CFrontEndSound18FrontEndSoundEvent)` | FESND.CPP:18 |
| `0x800535FC` | `CFrontEndSound::CFrontEndSound()` | FESND.CPP:121 |
| `0x80053640` | `_._14CFrontEndSound` | FESND.CPP:132 |
| `0x800536B8` | `CFrontEndSound::Initialize()` | FESND.CPP:139 |
| `0x800536D8` | `CFrontEndSound::Load(const char*)` | FESND.CPP:145 |
| `0x800536E0` | `CFrontEndSound::ProcessLocationSpecificSound(Q214CFrontEndSound18FrontEndSoundEvent)` | FESND.CPP:152 |
| `0x80053850` | `CFrontEndSound::HandleCursorEvent(Q214CFrontEndSound18FrontEndSoundEvent)` | FESND.CPP:250 |
| `0x800538B8` | `OpenTIM` | SONYDUMP.CPP:63 |
| `0x80053924` | `ReadTIM` | SONYDUMP.CPP:104 |
| `0x800539B4` | `tPort::TransformVector(_RMVECT16*, _RMVECT16*)` | PORTMATH.CPP:102 |
| `0x80053A78` | `tPort::ProjectVector(_RMVECT16*)` | PORTMATH.CPP:147 |
| `0x80053B1C` | `PopMatrixNoLights(void)` | PORTMATH.CPP:173 |
| `0x80053B50` | `PushMatrixNoLights(void)` | PORTMATH.CPP:182 |
| `0x80053B88` | `MultMatrixNoLights(MATRIX*)` | PORTMATH.CPP:189 |
| `0x80053BB0` | `RotMatrixXYZNoLights(unsigned short, unsigned short, unsigned short)` | PORTMATH.CPP:225 |
| `0x80053C5C` | `RotMatrixYZXNoLights(unsigned short, unsigned short, unsigned short)` | PORTMATH.CPP:232 |
| `0x80053D08` | `RotMatrixXNoLights(unsigned short)` | PORTMATH.CPP:239 |
| `0x80053D48` | `RotMatrixYNoLights(unsigned short)` | PORTMATH.CPP:244 |
| `0x80053D88` | `RotMatrixZNoLights(unsigned short)` | PORTMATH.CPP:249 |
| `0x80053DC8` | `GTEVXMatrix::FillRotZAfterSinCos()` | GTEMATRIX.HPP:163 |
| `0x80053DFC` | `GTEVXMatrix::FillRotYAfterSinCos()` | GTEMATRIX.HPP:142 |
| `0x80053E18` | `GTEVXMatrix::FillRotXAfterSinCos()` | GTEMATRIX.HPP:119 |
| `0x80053E3C` | `EnvironmentManager::EnvironmentManager()` | ENVMGR.CPP:64 |
| `0x80053E84` | `_._18EnvironmentManager` | ENVMGR.CPP:71 |
| `0x80053ED4` | `environmentLoadFunc(Callback*)` | ENVMGR.CPP:75 |
| `0x80053EF8` | `environmentUnloadFunc(Callback*)` | ENVMGR.CPP:85 |
| `0x80053F1C` | `EnvironmentManager::InternalOpen()` | ENVMGR.CPP:94 |
| `0x80053FFC` | `EnvironmentManager::Reset()` | ENVMGR.CPP:115 |
| `0x8005401C` | `EnvironmentManager::SetupEnvironment()` | ENVMGR.CPP:120 |
| `0x80054024` | `EnvironmentManager::SetupModelAmbientLighting(ccList*)` | ENVMGR.CPP:127 |
| `0x8005408C` | `_._8Callback` | CALLBACK.HPP:41 |
| `0x800540E0` | `aiPrivHandler(Handler*)` | AI.CPP:257 |
| `0x80054180` | `AI::AI()` | AI.CPP:297 |
| `0x800542C4` | `_._2AI` | AI.CPP:317 |
| `0x8005436C` | `AI::InternalOpen()` | AI.CPP:322 |
| `0x8005438C` | `AI::InternalClose()` | AI.CPP:327 |
| `0x800543AC` | `AI::AddActiveZone(DBVolume*)` | AI.CPP:338 |
| `0x80054404` | `AI::AddThingNoTagList(const char*, unsigned short, const tagLVector*, const _RMVECT16*, const char*, const DBRoot*)` | AI.CPP:365 |
| `0x800553A4` | `AI::InternalReset()` | AI.CPP:874 |
| `0x800553DC` | `AI::privMoveList(ccList&)` | AI.CPP:886 |
| `0x800554D0` | `HandleHumanoidHumanoidCollision(void)` | AI.CPP:951 |
| `0x80055584` | `HandleHumanoidHumanoidCollision(Humanoid*, Humanoid*)` | AI.CPP:986 |
| `0x80055BE4` | `AI::UpdatePositions(ccList&)` | AI.CPP:1197 |
| `0x80055C30` | `KillThingsInList(ccList&, long)` | AI.CPP:1222 |
| `0x80055CB4` | `AI::KillThings(long)` | AI.CPP:1252 |
| `0x80055D10` | `AI::MoveThings()` | AI.CPP:1268 |
| `0x80055F14` | `AI::MoveThingsObstacleCollisions()` | AI.CPP:1407 |
| `0x80055F44` | `AI::MoveThingsPickupCollisions()` | AI.CPP:1413 |
| `0x80055F6C` | `AI::MoveCamera()` | AI.CPP:1442 |
| `0x80055FC8` | `AI::PopulateActiveZones()` | AI.CPP:1449 |
| `0x80056038` | `AI::PopulateActiveZonesPaths()` | AI.CPP:1471 |
| `0x80056164` | `AI::PopulateActiveZonesSubZones()` | AI.CPP:1538 |
| `0x80056214` | `AI::Populate()` | AI.CPP:1575 |
| `0x800567CC` | `AI::UnPopulate(short)` | AI.CPP:1906 |
| `0x800569E4` | `PopulateBlockHelper(ccList&)` | AI.CPP:1948 |
| `0x80056A34` | `AI::PopulateBlock()` | AI.CPP:1958 |
| `0x80056A74` | `UnpopulateBlockHelper(ccList&)` | AI.CPP:1968 |
| `0x80056AC4` | `AI::UnpopulateBlock()` | AI.CPP:1979 |
| `0x80056B04` | `AI::GetPickupWithinReach(Humanoid*)` | AI.CPP:1999 |
| `0x80056BFC` | `AI::CheckObstacleAttack(Obstacle**, int, const Humanoid*, const FightingCollisionAttackType*)` | AI.CPP:2043 |
| `0x80056DE4` | `AI::FindThing(unsigned long)` | AI.CPP:2321 |
| `0x80056E7C` | `_._7Handler` | HNDLRSET.HPP:70 |
| `0x80056EC4` | `_._15CharMgrCallback` | CHARMGR.HPP:83 |
| `0x80056EF8` | `CharMgrCallback::Callback()` | CHARMGR.HPP:82 |
| `0x80056F04` | `_._4Path` | PATH.HPP:108 |
| `0x80056FE8` | `_._6ccList` | CCLIST.HPP:237 |
| `0x80057038` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8005708C` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x800570DC` | `Handler::RemoveFromList()` | HNDLRSET.HPP:72 |
| `0x80057118` | `AnimationManager::AnimationManager()` | ANIMMGR.CPP:142 |
| `0x80057174` | `_._16AnimationManager` | ANIMMGR.CPP:147 |
| `0x800571CC` | `AnimationManager::InternalOpen()` | ANIMMGR.CPP:152 |
| `0x800571D4` | `AnimationManager::InternalClose()` | ANIMMGR.CPP:157 |
| `0x80057228` | `AnimationManager::InternalReset()` | ANIMMGR.CPP:162 |
| `0x80057230` | `AnimationManager::PurgePetal()` | ANIMMGR.CPP:168 |
| `0x800572B4` | `AnimationManager::PurgeLevel()` | ANIMMGR.CPP:185 |
| `0x80057308` | `AnimationManager::GetMiscAnim(unsigned long)` | ANIMMGR.CPP:191 |
| `0x80057344` | `_._6ccList` | CCLIST.HPP:237 |
| `0x80057394` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x800573E8` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x80057468` | `CSoundFactory::Initialize(unsigned long)` | SNDFACT.CPP:49 |
| `0x8005752C` | `CSoundFactory::Destroy()` | SNDFACT.CPP:104 |
| `0x8005759C` | `CSoundFactory::CreateObject(unsigned long, CSound**, unsigned long)` | SNDFACT.CPP:178 |
| `0x800577E0` | `CSoundFactory::GetMemoryPoolPtr(void***)` | SNDFACT.CPP:469 |
| `0x80057808` | `CSoundFactory::ObjectCreated()` | SNDFACT.CPP:497 |
| `0x80057820` | `CSoundFactory::ObjectDestroyed()` | SNDFACT.CPP:519 |
| `0x80057838` | `CSoundFactory::LoadDatabase(const char*)` | SNDFACT.CPP:541 |
| `0x80057840` | `CSoundFactory::MaintenanceTask(_RTASK*)` | SNDFACT.CPP:563 |
| `0x80057890` | `static_destroy(_13CSoundFactory.g_pSoundMemoryPoolBuffer)` | SNDFACT.CPP:581 |
| `0x800578C8` | `static_init(_13CSoundFactory.g_pSoundMemoryPoolBuffer)` | SNDFACT.CPP:581 |
| `0x80057904` | `Display::platConstructor()` | PSXDISP.CPP:251 |
| `0x80057958` | `Display::platDestructor()` | PSXDISP.CPP:258 |
| `0x80057978` | `Display::platClose()` | PSXDISP.CPP:380 |
| `0x800579B8` | `Display::platReset()` | PSXDISP.CPP:386 |
| `0x800579C0` | `Display::BeginFrame()` | PSXDISP.CPP:390 |
| `0x80057A00` | `Display::EndFrame()` | PSXDISP.CPP:414 |
| `0x80057B2C` | `rStrCompare` | RSTRING.C:13 |
| `0x80057B60` | `rStrCompareNoCase` | RSTRING.C:25 |
| `0x80057BD8` | `rStrCopy` | RSTRING.C:37 |
| `0x80057C1C` | `rStrCat` | RSTRING.C:47 |
| `0x80057C8C` | `rStrLength` | RSTRING.C:57 |
| `0x80057CC8` | `rToUpper` | RSTRING.C:64 |
| `0x80057CE8` | `rToLower` | RSTRING.C:70 |
| `0x80057D4C` | `BackG::BackG()` | BACKG.CPP:164 |
| `0x8005805C` | `_._5BackG` | BACKG.CPP:253 |
| `0x800580B4` | `BackG::InitBG()` | BACKG.CPP:263 |
| `0x800580DC` | `BackG::LoadBG()` | BACKG.CPP:269 |
| `0x80058370` | `BackG::DeleteBG()` | BACKG.CPP:379 |
| `0x8005839C` | `BackG::GetScrollY(long)` | BACKG.CPP:392 |
| `0x800583E4` | `BackG::DrawBG()` | BACKG.CPP:415 |
| `0x80058448` | `BackG::Draw()` | BACKG.CPP:429 |
| `0x80058660` | `BackG::DrawSprite(BGGEO*, int, int)` | BACKG.CPP:480 |
| `0x8005878C` | `BackG::DrawPolyG4(BGGEO*, short, short)` | BACKG.CPP:511 |
| `0x800588B4` | `BackG::UpdateSplat()` | BACKG.CPP:540 |
| `0x80058938` | `BackG::UpdateBG()` | BACKG.CPP:567 |
| `0x8005896C` | `BackG::GetCamVect()` | BACKG.CPP:576 |
| `0x80058A04` | `rStrNCopy` | RSTREXT.C:15 |
| `0x80058AA8` | `rStrChr` | RSTREXT.C:113 |
| `0x80058AE4` | `LevelManager::LevelManager()` | LEVELMGR.CPP:287 |
| `0x80058BD4` | `_._12LevelManager` | LEVELMGR.CPP:292 |
| `0x80058CB8` | `LevelManager::LoadLevel()` | LEVELMGR.CPP:323 |
| `0x80058CC0` | `LevelManager::PurgeLevelP3DInventory()` | LEVELMGR.CPP:329 |
| `0x80058CC8` | `LevelManager::PurgeLevel()` | LEVELMGR.CPP:334 |
| `0x80058DB4` | `LevelManager::PurgePetal()` | LEVELMGR.CPP:385 |
| `0x80058E68` | `LevelManager::LoadPetal()` | LEVELMGR.CPP:423 |
| `0x80058E70` | `LevelManager::DeleteOriginalModelsByID(long)` | LEVELMGR.CPP:461 |
| `0x80058F30` | `LevelManager::DeleteInventoryByID(long)` | LEVELMGR.CPP:483 |
| `0x80058F84` | `LevelManager::AddOriginal(OriginalBasic*, long)` | LEVELMGR.CPP:498 |
| `0x80058FF0` | `LevelManager::DeleteOriginal(OriginalBasic*)` | LEVELMGR.CPP:540 |
| `0x800590D8` | `LevelManager::AddPermMemory(char*, long)` | LEVELMGR.CPP:588 |
| `0x8005913C` | `LevelManager::DeleteAllPermMem()` | LEVELMGR.CPP:594 |
| `0x800591C8` | `LevelManager::DeletePermMemID(long)` | LEVELMGR.CPP:608 |
| `0x80059268` | `LevelManager::FindModel(Q212LevelManager13ModelListEnuml)` | LEVELMGR.CPP:628 |
| `0x800592A0` | `LevelManager::FindModel(long)` | LEVELMGR.CPP:639 |
| `0x80059314` | `LevelManager::FindGeo(long)` | LEVELMGR.CPP:651 |
| `0x80059338` | `LevelManager::FindSTree(long)` | LEVELMGR.CPP:656 |
| `0x8005935C` | `LevelManager::FindETree(long)` | LEVELMGR.CPP:661 |
| `0x80059380` | `LevelManager::InternalReset()` | LEVELMGR.HPP:143 |
| `0x80059388` | `LevelManager::InternalOpen()` | LEVELMGR.HPP:142 |
| `0x80059390` | `LevelManager::InternalClose()` | LEVELMGR.HPP:141 |
| `0x80059398` | `_._6ccList` | CCLIST.HPP:237 |
| `0x800593E8` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8005943C` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8005950C` | `_StereoOnOff(hdMenuItem*)` | SOUND.CPP:129 |
| `0x80059594` | `_SetMusicVolume(hdMenuItem*)` | SOUND.CPP:148 |
| `0x80059600` | `_SetEffectsVolume(hdMenuItem*)` | SOUND.CPP:159 |
| `0x80059698` | `_SetDialogVolume(hdMenuItem*)` | SOUND.CPP:170 |
| `0x80059704` | `soundLoadFunc(Callback*)` | SOUND.CPP:187 |
| `0x80059760` | `soundUnLoadFunc(Callback*)` | SOUND.CPP:200 |
| `0x80059794` | `Sound::Sound()` | SOUND.CPP:213 |
| `0x800597E8` | `_._5Sound` | SOUND.CPP:230 |
| `0x80059818` | `Sound::InternalOpen()` | SOUND.CPP:237 |
| `0x800598D8` | `Sound::SetupSound()` | SOUND.CPP:257 |
| `0x800599B0` | `Sound::CleanupSound()` | SOUND.CPP:267 |
| `0x80059A4C` | `Sound::InstallMenu(MenuMgr*)` | SOUND.CPP:272 |
| `0x80059B84` | `Sound::OnMenuSelect(hdMenu*)` | SOUND.CPP:297 |
| `0x80059C38` | `SoundAnchor::SetupSoundSphere()` | SOUND.CPP:319 |
| `0x80059DB4` | `SoundMenuState::SoundMenuState()` | SOUND.CPP:511 |
| `0x80059DBC` | `SoundMenuState::Restore()` | SOUND.CPP:515 |
| `0x80059E7C` | `SoundMenuState::Save()` | SOUND.CPP:535 |
| `0x80059EB4` | `static_init(soundLoadFunc__FP8Callback)` | SOUND.CPP:545 |
| `0x80059EF0` | `_._8Callback` | CALLBACK.HPP:41 |
| `0x80059F44` | `_._11SoundAnchor` | SOUND.HPP:90 |
| `0x80059FBC` | `DataAnchor::RemElement(ccNode*)` | ANCHOR.HPP:53 |
| `0x80059FDC` | `DataAnchor::AddElement(ccNode*)` | ANCHOR.HPP:52 |
| `0x8005A008` | `_._10DataAnchor` | ANCHOR.HPP:43 |
| `0x8005A060` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8005A0B0` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8005A104` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8005AAA4` | `rNewFPool` | RADFMEM.CPP:22 |
| `0x8005AAFC` | `rNewFPoolBuf` | RADFMEM.CPP:30 |
| `0x8005AB80` | `rFOMallocChain` | RADFMEM.CPP:63 |
| `0x8005ABB4` | `rFFree` | RADFMEM.CPP:89 |
| `0x8005ABC8` | `rDeleteFPool` | RADFMEM.CPP:96 |
| `0x8005AC08` | `rFSetOverflowSize` | RADFMEM.CPP:108 |
| `0x8005AC10` | `rFExpand` | RADFMEM.CPP:137 |
| `0x8005CB4C` | `MenuColorStart(xcColour1555&)` | HDMENU.CPP:112 |
| `0x8005CC44` | `CalcNextColor(xcColour1555&)` | HDMENU.CPP:125 |
| `0x8005CD10` | `MenuColorNext(xcColour1555&)` | HDMENU.CPP:143 |
| `0x8005CD94` | `hdMenu::hdMenu()` | HDMENU.CPP:159 |
| `0x8005CDEC` | `_._6hdMenu` | HDMENU.CPP:170 |
| `0x8005CE44` | `hdMenu::PostFlight(MenuMgr*)` | HDMENU.CPP:179 |
| `0x8005CEB8` | `hdMenu::UpdateScreen(oxScreenManager*)` | HDMENU.CPP:192 |
| `0x8005CEE8` | `hdMenu::SetCallback(unsigned long, hdMenuItem*(*)()*, int)` | HDMENU.CPP:197 |
| `0x8005CF2C` | `hdMenu::FindItem(unsigned long)` | HDMENU.CPP:205 |
| `0x8005CF68` | `hdMenu::InputPush(MenuMgr*)` | HDMENU.CPP:222 |
| `0x8005CFD0` | `hdMenu::ClearItem()` | HDMENU.CPP:235 |
| `0x8005D010` | `hdMenu::DynSetup()` | HDMENU.CPP:244 |
| `0x8005D0BC` | `hdMenu::AddItem(hdMenuItem*)` | HDMENU.CPP:278 |
| `0x8005D0E8` | `hdMenu::Update()` | HDMENU.CPP:283 |
| `0x8005D15C` | `hdMenu::SetItem(hdMenuItem*)` | HDMENU.CPP:294 |
| `0x8005D1B0` | `hdMenu::InputNextItem()` | HDMENU.CPP:305 |
| `0x8005D238` | `hdMenu::InputPrevItem()` | HDMENU.CPP:342 |
| `0x8005D2CC` | `hdMenu::SetID(const char*)` | HDMENU.CPP:378 |
| `0x8005D2F8` | `hdDynItemMenu::hdDynItemMenu(int)` | HDMENU.CPP:383 |
| `0x8005D340` | `hdDynItemMenu::DynSetup()` | HDMENU.CPP:418 |
| `0x8005D3B4` | `hdDynItemMenu::InputNextItem()` | HDMENU.CPP:434 |
| `0x8005D428` | `hdDynItemMenu::InputPrevItem()` | HDMENU.CPP:450 |
| `0x8005D49C` | `hdNumericSelection::hdNumericSelection(xcOverlay*, char*, int, int)` | HDMENU.CPP:467 |
| `0x8005D55C` | `hdNumericSelection::GetValue()` | HDMENU.CPP:482 |
| `0x8005D568` | `hdNumericSelection::ChangeValueText()` | HDMENU.CPP:488 |
| `0x8005D598` | `hdNumericSelection::IncItem()` | HDMENU.CPP:494 |
| `0x8005D634` | `hdNumericSelection::DecItem()` | HDMENU.CPP:514 |
| `0x8005D6D0` | `hdNumericSelection::SetValue(unsigned long)` | HDMENU.CPP:534 |
| `0x8005D728` | `hdAlphaSelection::hdAlphaSelection(xcOverlay*, char*, int, int)` | HDMENU.CPP:543 |
| `0x8005D764` | `hdAlphaSelection::ChangeValueText()` | HDMENU.CPP:548 |
| `0x8005D778` | `hdItemSelection::hdItemSelection(xcOverlay*, char*)` | HDMENU.CPP:554 |
| `0x8005D800` | `hdItemSelection::IncItem()` | HDMENU.CPP:562 |
| `0x8005D8A8` | `hdItemSelection::DecItem()` | HDMENU.CPP:581 |
| `0x8005D954` | `hdItemSelection::SetValue(unsigned long)` | HDMENU.CPP:601 |
| `0x8005D97C` | `hdItemSelection::GetValue()` | HDMENU.CPP:608 |
| `0x8005D990` | `hdItemSelection::SetColour(xcColour1555&, bool)` | HDMENU.CPP:614 |
| `0x8005DA28` | `hdSndItemSelection::hdSndItemSelection(xcOverlay*, char*, int, int)` | HDMENU.CPP:622 |
| `0x8005DABC` | `hdSndItemSelection::DecItem()` | HDMENU.CPP:636 |
| `0x8005DB54` | `hdSndItemSelection::GetValue()` | HDMENU.CPP:659 |
| `0x8005DB60` | `hdSndItemSelection::UpdateShown()` | HDMENU.CPP:665 |
| `0x8005DBD4` | `hdSndItemSelection::SetValue(unsigned long)` | HDMENU.CPP:680 |
| `0x8005DC14` | `hdSndItemSelection::IncItem()` | HDMENU.CPP:689 |
| `0x8005DCD0` | `hdDynItemSelection::hdDynItemSelection(xcOverlay*, char*)` | HDMENU.CPP:714 |
| `0x8005DD14` | `hdItemGoto::hdItemGoto(xcTextObj*, char*)` | HDMENU.CPP:730 |
| `0x8005DD7C` | `_._10hdItemGoto` | HDMENU.CPP:740 |
| `0x8005DDA4` | `hdItemGoto::PostFlight(MenuMgr*)` | HDMENU.CPP:744 |
| `0x8005DDD8` | `hdDynItemGoto::DynSetup()` | HDMENU.CPP:756 |
| `0x8005DDF8` | `hdItemGoto::SelectItem(MenuMgr*)` | HDMENU.CPP:762 |
| `0x8005DE2C` | `hdMenuItem::hdMenuItem()` | HDMENU.CPP:769 |
| `0x8005DE70` | `_._10hdMenuItem` | HDMENU.CPP:778 |
| `0x8005DE98` | `hdMenuItem::PostFlight(MenuMgr*)` | HDMENU.CPP:782 |
| `0x8005DEA0` | `hdMenuItem::SetColour(xcColour1555&, bool)` | HDMENU.CPP:787 |
| `0x8005DF10` | `hdMenuItem::SelectItem(MenuMgr*)` | HDMENU.CPP:793 |
| `0x8005DF18` | `hdMenuItem::IncItem()` | HDMENU.CPP:798 |
| `0x8005DF20` | `hdMenuItem::DecItem()` | HDMENU.CPP:804 |
| `0x8005DF28` | `hdMenuItem::SetValue(unsigned long)` | HDMENU.CPP:809 |
| `0x8005DF30` | `hdMenuItem::SetCallback(hdMenuItem*(*)()*, int)` | HDMENU.CPP:814 |
| `0x8005DF38` | `hdMenuItem::GetValue()` | HDMENU.CPP:819 |
| `0x8005DF40` | `hdDynMenu::hdDynMenu(MenuMgr*, xcOverlay*, int)` | HDMENU.CPP:826 |
| `0x8005DFC0` | `hdDynMenu::DynSetup()` | HDMENU.CPP:848 |
| `0x8005E0D8` | `hdDynMenu::GetTextObj(int)` | HDMENU.CPP:874 |
| `0x8005E118` | `hdItemButton::hdItemButton(xcTextObj*, char*)` | HDMENU.CPP:927 |
| `0x8005E174` | `hdItemButton::SelectItem(MenuMgr*)` | HDMENU.CPP:933 |
| `0x8005E1AC` | `hdDynItemButton::DynSetup()` | HDMENU.CPP:950 |
| `0x8005E1CC` | `hdShockSelection::hdShockSelection(xcOverlay*, char*)` | HDMENU.CPP:957 |
| `0x8005E200` | `hdShockSelection::SetColour(xcColour1555&, bool)` | HDMENU.CPP:967 |
| `0x8005E330` | `hdShockSelection::CanBeSelected()` | HDMENU.CPP:982 |
| `0x8005E350` | `static_init(gMenuR0)` | HDMENU.CPP:984 |
| `0x8005E3BC` | `_._9hdDynMenu` | HDMENU.H:315 |
| `0x8005E3DC` | `_._13hdDynItemMenu` | HDMENU.H:287 |
| `0x8005E3FC` | `hdMenu::Cleanup()` | HDMENU.H:267 |
| `0x8005E404` | `hdMenu::CanAbortNow()` | HDMENU.H:265 |
| `0x8005E40C` | `_._16hdShockSelection` | HDMENU.H:227 |
| `0x8005E434` | `_._18hdDynItemSelection` | HDMENU.H:217 |
| `0x8005E454` | `hdSndItemSelection::SetColour(xcColour1555&, bool)` | HDMENU.H:195 |
| `0x8005E474` | `_._18hdSndItemSelection` | HDMENU.H:203 |
| `0x8005E494` | `hdItemSelection::CanBeSelected()` | HDMENU.H:177 |
| `0x8005E49C` | `_._15hdItemSelection` | HDMENU.H:182 |
| `0x8005E4BC` | `_._16hdAlphaSelection` | HDMENU.H:165 |
| `0x8005E4DC` | `_._18hdNumericSelection` | HDMENU.H:158 |
| `0x8005E4FC` | `_._13hdDynItemGoto` | HDMENU.H:141 |
| `0x8005E51C` | `_._15hdDynItemButton` | HDMENU.H:119 |
| `0x8005E53C` | `_._12hdItemButton` | HDMENU.H:111 |
| `0x8005E55C` | `hdMenuItem::DynSetup()` | HDMENU.H:97 |
| `0x8005E564` | `hdMenuItem::CanBeSelected()` | HDMENU.H:94 |
| `0x8005E56C` | `xcColour1555::GetAlpha8() const` | XCCOLOUR.H:217 |
| `0x8005E58C` | `xcColour1555::GetBlue8() const` | XCCOLOUR.H:216 |
| `0x8005E5B0` | `xcColour1555::GetGreen8() const` | XCCOLOUR.H:215 |
| `0x8005E5D4` | `xcColour1555::GetRed8() const` | XCCOLOUR.H:214 |
| `0x8005E5F4` | `xcColour1555::Set8(unsigned char, unsigned char, unsigned char)` | XCCOLOUR.H:197 |
| `0x8005E650` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8005E6A4` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8005E6F4` | `xcOverlay::LoadAll()` | XCSOS.CPP:22 |
| `0x8005E754` | `xcOverlay::UnloadAll()` | XCSOS.CPP:31 |
| `0x8005E7B4` | `xcOverlay::SetVisible(unsigned long)` | XCSOS.CPP:40 |
| `0x8005E810` | `xcOverlay::FixDataPointers(unsigned long)` | XCSOS.CPP:55 |
| `0x8005E848` | `xcOverlay::FindNamedData(xcSectionMan*)` | XCSOS.CPP:66 |
| `0x8005E8B8` | `xcOverlay::DrawAll()` | XCSOS.CPP:78 |
| `0x8005E90C` | `xcOverlay::FindDOsh(unsigned long)` | XCSOS.CPP:90 |
| `0x8005E954` | `xcOverlay::GetPrimObj(unsigned long, xcChunkEnum)` | XCSOS.CPP:104 |
| `0x8005E978` | `xcOverlay::GetPrimObj(const char*, xcChunkEnum)` | XCSOS.CPP:118 |
| `0x8005E9B0` | `xcOverlay::GetSprite(unsigned long)` | XCSOS.CPP:136 |
| `0x8005E9D0` | `xcOverlay::GetTextObj(unsigned long)` | XCSOS.CPP:148 |
| `0x8005E9F0` | `xcOverlay::GetTextObj(const char*)` | XCSOS.CPP:153 |
| `0x8005EA10` | `xcOverlay::GetPolyG4(unsigned long)` | XCSOS.CPP:232 |
| `0x8005EA30` | `xcScreen::FindNamedData(xcSectionMan*)` | XCSOS.CPP:250 |
| `0x8005EAA4` | `xcScreen::LoadOverlays()` | XCSOS.CPP:259 |
| `0x8005EB0C` | `_._9xcSection` | XCSOS.CPP:279 |
| `0x8005EBD8` | `xcSection::Init(unsigned char*, xcSectionMan*, unsigned long)` | XCSOS.CPP:295 |
| `0x8005EC04` | `xcSection::FixUpPointers()` | XCSOS.CPP:307 |
| `0x8005EC40` | `xcSection::GotoScreen(xcScreen*)` | XCSOS.CPP:332 |
| `0x8005EC8C` | `xcSection::UnloadOverlays()` | XCSOS.CPP:346 |
| `0x8005ED04` | `xcSection::Draw()` | XCSOS.CPP:355 |
| `0x8005ED9C` | `xcSection::FindImage(unsigned long)` | XCSOS.CPP:373 |
| `0x8005EDC4` | `xcSection::FindOverlay(unsigned long)` | XCSOS.CPP:389 |
| `0x8005EE00` | `xcSection::FindScreen(unsigned long)` | XCSOS.CPP:407 |
| `0x8005EE28` | `xcSection::FindString(unsigned long)` | XCSOS.CPP:423 |
| `0x8005EE50` | `xcSection::FixInventories()` | XCSOS.CPP:446 |
| `0x8005EFA4` | `xcSection::LoadCells()` | XCSOS.CPP:500 |
| `0x8005F07C` | `xcSection::FreeDiscardableData()` | XCSOS.CPP:526 |
| `0x8005F0B0` | `xcSection::FixScreenAndOverlayandDO()` | XCSOS.CPP:545 |
| `0x8005F180` | `xcSectionMan::xcSectionMan()` | XCSOS.CPP:572 |
| `0x8005F190` | `xcSectionMan::FindFont(unsigned long)` | XCSOS.CPP:580 |
| `0x8005F1B8` | `xcSectionMan::FindFont(const char*)` | XCSOS.CPP:591 |
| `0x8005F1EC` | `xcSectionMan::FreeSection()` | XCSOS.CPP:598 |
| `0x8005F228` | `xcSectionMan::CreateNewSection()` | XCSOS.CPP:609 |
| `0x8005F274` | `xcSectionMan::LoadSection(const char*, unsigned long)` | XCSOS.CPP:621 |
| `0x8005F300` | `xcSectionMan::DeleteFonts()` | XCSOS.CPP:653 |
| `0x8005F3B4` | `xcSectionMan::LoadFonts(xcInventory*)` | XCSOS.CPP:676 |
| `0x8005F498` | `xcInventory::GetSizeInBytes() const` | XCINV.H:69 |
| `0x8005F4A4` | `MenuMgr::MenuMgr()` | MENUMGR.CPP:58 |
| `0x8005F50C` | `_._7MenuMgr` | MENUMGR.CPP:68 |
| `0x8005F564` | `MenuMgr::FindMenu(unsigned long)` | MENUMGR.CPP:72 |
| `0x8005F5A0` | `MenuMgr::FindScreen(unsigned long)` | MENUMGR.CPP:86 |
| `0x8005F5AC` | `MenuMgr::InputItemPop()` | MENUMGR.CPP:119 |
| `0x8005F660` | `MenuMgr::InputPadUp()` | MENUMGR.CPP:137 |
| `0x8005F6B0` | `MenuMgr::InputPadRight()` | MENUMGR.CPP:145 |
| `0x8005F6F8` | `MenuMgr::InputPadLeft()` | MENUMGR.CPP:152 |
| `0x8005F740` | `MenuMgr::InputPadDown()` | MENUMGR.CPP:159 |
| `0x8005F790` | `MenuMgr::InputItemPush()` | MENUMGR.CPP:167 |
| `0x8005F7DC` | `MenuMgr::SetTopMenu(unsigned long)` | MENUMGR.CPP:176 |
| `0x8005F830` | `MenuMgr::PushMenu(hdMenu*)` | MENUMGR.CPP:185 |
| `0x8005F894` | `MenuMgr::PostFlightDef()` | MENUMGR.CPP:203 |
| `0x8005F8E0` | `MenuMgr::ParseDefFile(char*)` | MENUMGR.CPP:215 |
| `0x8005FB00` | `MenuMgr::Invoke()` | MENUMGR.CPP:262 |
| `0x8005FBA4` | `MenuMgr::Activate()` | MENUMGR.CPP:279 |
| `0x8005FD30` | `MenuMgr::Deactivate()` | MENUMGR.CPP:327 |
| `0x8005FDF4` | `MenuMgr::QueryInput(bool)` | MENUMGR.CPP:351 |
| `0x8005FF38` | `MenuMgr::GetScreenHash(unsigned long)` | MENUMGR.CPP:373 |
| `0x8005FF40` | `MenuMgr::GetScreenNames()` | MENUMGR.CPP:380 |
| `0x8005FF4C` | `MenuMgr::PopMenu()` | MENUMGR.CPP:386 |
| `0x8005FFD0` | `MenuMgr::ParseMenu(LineFile&, hdMenu*)` | MENUMGR.CPP:416 |
| `0x800605B4` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x80060608` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x80060658` | `CHumanoidSound::Initialize(const tagLVector*, const Humanoid*)` | HMNDSND.CPP:36 |
| `0x80060678` | `CHumanoidSound::CHumanoidSound()` | HMNDSND.CPP:54 |
| `0x800606E0` | `_._14CHumanoidSound` | HMNDSND.CPP:83 |
| `0x8006074C` | `CHumanoidSound::Footstep(CSoundMaterial)` | HMNDSND.CPP:102 |
| `0x80060790` | `CHumanoidSound::Strafe(CSoundMaterial)` | HMNDSND.CPP:126 |
| `0x800607B8` | `CHumanoidSound::Fall()` | HMNDSND.CPP:145 |
| `0x800607E0` | `CHumanoidSound::Land(CSoundMaterial)` | HMNDSND.CPP:162 |
| `0x80060850` | `CHumanoidSound::DiveRoll(CSoundMaterial)` | HMNDSND.CPP:189 |
| `0x80060878` | `CHumanoidSound::HitWorldStructure(CSoundMaterial)` | HMNDSND.CPP:208 |
| `0x800608A0` | `CHumanoidSound::HandPlant()` | HMNDSND.CPP:215 |
| `0x800608C8` | `CHumanoidSound::Grab(CSoundMaterial)` | HMNDSND.CPP:232 |
| `0x8006090C` | `CHumanoidSound::GrabHumanoid()` | HMNDSND.CPP:256 |
| `0x80060934` | `CHumanoidSound::FrontFlip()` | HMNDSND.CPP:273 |
| `0x8006095C` | `CHumanoidSound::WallJump()` | HMNDSND.CPP:290 |
| `0x8006097C` | `CHumanoidSound::PoleSwing()` | HMNDSND.CPP:307 |
| `0x800609A4` | `CHumanoidSound::PunchMiss()` | HMNDSND.CPP:324 |
| `0x800609C8` | `CHumanoidSound::KickMiss()` | HMNDSND.CPP:341 |
| `0x800609EC` | `CHumanoidSound::PunchHit()` | HMNDSND.CPP:358 |
| `0x80060A28` | `CHumanoidSound::KickHit()` | HMNDSND.CPP:379 |
| `0x80060A64` | `CHumanoidSound::SuperPunch()` | HMNDSND.CPP:399 |
| `0x80060AB4` | `CHumanoidSound::SuperKick()` | HMNDSND.CPP:423 |
| `0x80060B04` | `CHumanoidSound::Collapse(CSoundMaterial)` | HMNDSND.CPP:447 |
| `0x80060B48` | `CHumanoidSound::FlyThroughAir()` | HMNDSND.CPP:471 |
| `0x80060B70` | `CHumanoidSound::BeginStun()` | HMNDSND.CPP:488 |
| `0x80060B94` | `CHumanoidSound::EndStun()` | HMNDSND.CPP:509 |
| `0x80060BB4` | `CHumanoidSound::BeginSlideOnSurface(CSoundMaterial)` | HMNDSND.CPP:526 |
| `0x80060BD8` | `CHumanoidSound::EndSlideOnSurface()` | HMNDSND.CPP:549 |
| `0x80060BF8` | `CHumanoidSound::BeginSlideDownLadder()` | HMNDSND.CPP:566 |
| `0x80060C1C` | `CHumanoidSound::EndSlideDownLadder()` | HMNDSND.CPP:587 |
| `0x80060C3C` | `CHumanoidSound::EndAllSounds()` | HMNDSND.CPP:604 |
| `0x80060C74` | `CHumanoidSound::Load(const char*)` | HMNDSND.CPP:625 |
| `0x80060D1C` | `CHumanoidSound::FXDialogHit()` | HMNDSND.CPP:644 |
| `0x80060D7C` | `CHumanoidSound::FXDialogAttack()` | HMNDSND.CPP:670 |
| `0x80060DDC` | `CHumanoidSound::PlayAttack(unsigned short)` | HMNDSND.CPP:696 |
| `0x80060E40` | `CHumanoidSound::PlayHit(unsigned short)` | HMNDSND.CPP:720 |
| `0x80060EA4` | `CHumanoidSound::Think()` | HMNDSND.CPP:744 |
| `0x80060EE8` | `CHumanoidSound::MapSoundScriptEvent(SSHumanoid)` | HMNDSND.CPP:774 |
| `0x80061170` | `CHumanoidSound::GrabWeapon()` | HMNDSND.CPP:1001 |
| `0x800611C0` | `CHumanoidSound::WeaponMiss()` | HMNDSND.CPP:1033 |
| `0x80061210` | `CHumanoidSound::WeaponHit()` | HMNDSND.CPP:1065 |
| `0x80061260` | `CHumanoidSound::Breath()` | HMNDSND.CPP:1097 |
| `0x80061288` | `CHumanoidSound::Grunt()` | HMNDSND.CPP:1114 |
| `0x800612D4` | `CHumanoidSound::HitByFireBlast()` | HMNDSND.CPP:1136 |
| `0x80061314` | `CHumanoidSound::LoadHumanoidSoundScripts()` | HMNDSND.CPP:1159 |
| `0x800613C0` | `CHumanoidSound::UnloadHumanoidSoundScripts()` | HMNDSND.CPP:1203 |
| `0x800613EC` | `CHumanoidSound::ProcessSoundScript(unsigned long, unsigned long)` | HMNDSND.CPP:1265 |
| `0x80061558` | `Thing::Thing(const tagLVector*, unsigned short)` | THING.CPP:428 |
| `0x80061640` | `_._5Thing` | THING.CPP:458 |
| `0x800616BC` | `Thing::Think()` | THING.CPP:478 |
| `0x800616EC` | `Thing::Draw()` | THING.CPP:487 |
| `0x80061760` | `Thing::Reset()` | THING.CPP:502 |
| `0x80061790` | `Thing::Activate()` | THING.CPP:521 |
| `0x8006182C` | `Thing::Deactivate()` | THING.CPP:546 |
| `0x800618E0` | `Thing::CreateModel(const char*)` | THING.CPP:585 |
| `0x80061AAC` | `Thing::DeleteModel()` | THING.CPP:689 |
| `0x80061B08` | `Thing::HandleCollision(Thing*, long, ...)` | THING.CPP:713 |
| `0x80061B44` | `DynamicThing::AddForce(long, const _RMVECT16*)` | THING.CPP:753 |
| `0x80061BFC` | `Thing::ClearFloorHeight()` | THING.CPP:765 |
| `0x80061C38` | `Thing::SetFloorHeight(long)` | THING.CPP:777 |
| `0x80061C78` | `DynamicThing::Land()` | THING.CPP:794 |
| `0x80061CC4` | `DynamicThing::DisembarkObstacle(const tagLVector&)` | THING.CPP:810 |
| `0x80061D60` | `Thing::Move()` | THING.CPP:835 |
| `0x80061D68` | `DynamicThing::DynamicThing(const tagLVector*, unsigned short)` | THING.CPP:840 |
| `0x80061DE0` | `_._12DynamicThing` | THING.CPP:851 |
| `0x80061E38` | `DynamicThing::Reset()` | THING.CPP:865 |
| `0x80061EC4` | `DynamicThing::Move()` | THING.CPP:891 |
| `0x80062400` | `Thing::AddPassenger(DynamicThing*)` | THING.CPP:1079 |
| `0x8006247C` | `Thing::RemPassenger(Ticket*)` | THING.CPP:1104 |
| `0x800624C4` | `DynamicThing::Disembark()` | THING.CPP:1126 |
| `0x80062504` | `Thing::RemAllPassengers()` | THING.CPP:1144 |
| `0x80062550` | `DynamicThing::GetTicketIssuer()` | THING.CPP:1162 |
| `0x80062574` | `Thing::GetThingHandle()` | THING.CPP:1170 |
| `0x800625C0` | `Thing::DistanceFromPointXZ(const tagLVector&) const` | THING.CPP:1180 |
| `0x800625F4` | `Thing::DistanceFromPoint(const tagLVector&) const` | THING.CPP:1189 |
| `0x80062638` | `Thing::GetViewSpot(tagLVector*, tagLVector*)` | THING.CPP:1210 |
| `0x80062680` | `Thing::AnalyzeMesh(DBRoot*)` | THING.CPP:1224 |
| `0x8006272C` | `DynamicThing::UpdatePosition()` | THING.CPP:1280 |
| `0x800627F0` | `Ticket::Ticket(Thing*, DynamicThing*)` | THING.CPP:1312 |
| `0x80062844` | `_._6Ticket` | THING.CPP:1318 |
| `0x8006286C` | `Thing::FillSphere(tSphere&) const` | THING.CPP:1329 |
| `0x80062874` | `Thing::GetObjectToWorldSpaceVector(const _RMVECT16&, _RMVECT16&)` | THING.CPP:1352 |
| `0x800628C8` | `DynamicThing::HandleLand(long)` | THING.CPP:1360 |
| `0x800628D0` | `Thing::Kill()` | THING.HPP:518 |
| `0x800628E4` | `Thing::GetSoundPosPtr()` | THING.HPP:516 |
| `0x800628EC` | `Thing::GetInitialPos()` | THING.HPP:512 |
| `0x800628F4` | `Thing::UpdatePosition()` | THING.HPP:440 |
| `0x800628FC` | `ThingHandle::Close()` | THING.HPP:379 |
| `0x80062940` | `_._6ccList` | CCLIST.HPP:237 |
| `0x80062990` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x800629E4` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x80062A34` | `Humanoid::Humanoid(const tagLVector*, unsigned short)` | HUMANOID.CPP:350 |
| `0x80062C58` | `_._8Humanoid` | HUMANOID.CPP:490 |
| `0x80062DC0` | `Humanoid::Reset()` | HUMANOID.CPP:513 |
| `0x80062E54` | `Humanoid::AnalyzeMesh(DBRoot*)` | HUMANOID.CPP:535 |
| `0x80063210` | `Humanoid::Activate()` | HUMANOID.CPP:760 |
| `0x80063270` | `Humanoid::Deactivate()` | HUMANOID.CPP:776 |
| `0x800632B4` | `Humanoid::CreateModel(const char*)` | HUMANOID.CPP:795 |
| `0x800634C4` | `Humanoid::CreateSound()` | HUMANOID.CPP:888 |
| `0x80063514` | `Humanoid::DeleteModel()` | HUMANOID.CPP:910 |
| `0x80063614` | `Humanoid::ReleaseSound()` | HUMANOID.CPP:952 |
| `0x80063660` | `Humanoid::ProcessControl()` | HUMANOID.CPP:961 |
| `0x80063690` | `Humanoid::LoadEnemyTaunts()` | HUMANOID.CPP:991 |
| `0x80063808` | `Humanoid::Think()` | HUMANOID.CPP:1133 |
| `0x80063A88` | `Humanoid::Draw()` | HUMANOID.CPP:1280 |
| `0x80064100` | `Humanoid::Move()` | HUMANOID.CPP:1544 |
| `0x80064194` | `Humanoid::HandleAnimationControl()` | HUMANOID.CPP:1590 |
| `0x800643B8` | `Humanoid::RestorePositionFromBip01()` | HUMANOID.CPP:1681 |
| `0x80064528` | `Humanoid::HandleCollisionReactionStates(long, long)` | HUMANOID.CPP:1734 |
| `0x8006475C` | `Humanoid::HandleCollisionSound(long)` | HUMANOID.CPP:1933 |
| `0x80064808` | `Humanoid::HandleCollision(Thing*, long, ...)` | HUMANOID.CPP:1997 |
| `0x80064B98` | `Humanoid::FaceThing(Thing*, int)` | HUMANOID.CPP:2252 |
| `0x80064BD0` | `Humanoid::FacePoint(const tagLVector&, int)` | HUMANOID.CPP:2260 |
| `0x80064D7C` | `Humanoid::FaceThingDesired(Thing*)` | HUMANOID.CPP:2333 |
| `0x80064DB4` | `Humanoid::FacePointDesired(const tagLVector&)` | HUMANOID.CPP:2352 |
| `0x80064EA8` | `Humanoid::SetDesiredMoveDirection(long)` | HUMANOID.CPP:2391 |
| `0x80064EB0` | `Humanoid::FaceAngleY(long, int)` | HUMANOID.CPP:2402 |
| `0x80064F94` | `Humanoid::FindFoe(unsigned long, long, int)` | HUMANOID.CPP:2446 |
| `0x8006511C` | `Humanoid::SetTarget(Humanoid*)` | HUMANOID.CPP:2502 |
| `0x800651B4` | `Humanoid::SetHumanoidTarget(Humanoid*)` | HUMANOID.CPP:2522 |
| `0x80065200` | `Humanoid::ReleaseTarget()` | HUMANOID.CPP:2553 |
| `0x80065230` | `Humanoid::IsInActiveZone()` | HUMANOID.CPP:2612 |
| `0x80065290` | `Humanoid::IsTargetInActiveZone()` | HUMANOID.CPP:2630 |
| `0x800652F4` | `Humanoid::IsInMyFieldOf(Humanoid*, long, long)` | HUMANOID.CPP:2645 |
| `0x80065340` | `Humanoid::IsInMyDesiredFieldOf(Humanoid*, long, long)` | HUMANOID.CPP:2652 |
| `0x8006538C` | `Humanoid::ProcessAction()` | HUMANOID.CPP:2659 |
| `0x800653F4` | `Humanoid::SetTauntAnim(long)` | HUMANOID.CPP:2666 |
| `0x80065420` | `GetWeaponTransitionIdle(Pickup*)` | HUMANOID.CPP:2671 |
| `0x800654C4` | `Humanoid::SetIdleAnimation(long, int)` | HUMANOID.CPP:2717 |
| `0x800655B4` | `Humanoid::StitchIdleAnimation()` | HUMANOID.CPP:2741 |
| `0x80065618` | `Humanoid::TestIdleAnimation()` | HUMANOID.CPP:2763 |
| `0x80065680` | `Humanoid::SetActionState(unsigned long, long)` | HUMANOID.CPP:2792 |
| `0x80066C4C` | `ReturnMostSignificant32BitNumber(unsigned long)` | HUMANOID.CPP:3826 |
| `0x80066CA0` | `Humanoid::_Stand()` | HUMANOID.CPP:3859 |
| `0x80066E3C` | `Humanoid::_DiveRoll()` | HUMANOID.CPP:3977 |
| `0x8006710C` | `Humanoid::_Taunt()` | HUMANOID.CPP:4069 |
| `0x80067288` | `Humanoid::_Pause()` | HUMANOID.CPP:4153 |
| `0x800672EC` | `Humanoid::_Run()` | HUMANOID.CPP:4172 |
| `0x800675C0` | `ClipAngle360(long)` | HUMANOID.CPP:4283 |
| `0x80067610` | `Humanoid::_Straif()` | HUMANOID.CPP:4307 |
| `0x80067DBC` | `Humanoid::_Jump()` | HUMANOID.CPP:4569 |
| `0x80067F2C` | `Humanoid::_Fall()` | HUMANOID.CPP:4620 |
| `0x80067F34` | `Humanoid::_FindLatch()` | HUMANOID.CPP:4624 |
| `0x80067F3C` | `Humanoid::_Push()` | HUMANOID.CPP:4628 |
| `0x80067F44` | `Humanoid::_Teetering()` | HUMANOID.CPP:4633 |
| `0x80067F4C` | `Humanoid::_WallJump()` | HUMANOID.CPP:4638 |
| `0x80067F54` | `Humanoid::_Hotfoot()` | HUMANOID.CPP:4644 |
| `0x800680B8` | `Humanoid::GetImpactRegion(const tagLVector&)` | HUMANOID.CPP:4704 |
| `0x80068264` | `Humanoid::_JumpKick()` | HUMANOID.CPP:4759 |
| `0x8006826C` | `Humanoid::_BackGrabCharacter()` | HUMANOID.CPP:4777 |
| `0x80068338` | `Humanoid::_BackGrabCharacterRelease()` | HUMANOID.CPP:4828 |
| `0x800683C4` | `Humanoid::_BackGrabCharacterReceivePreLatch()` | HUMANOID.CPP:4872 |
| `0x80068410` | `Humanoid::_BackGrabCharacterReceiveLatch()` | HUMANOID.CPP:4894 |
| `0x80068460` | `Humanoid::_BackGrabCharacterReceive()` | HUMANOID.CPP:4915 |
| `0x80068508` | `Humanoid::_Pickup()` | HUMANOID.CPP:4959 |
| `0x800685A8` | `Humanoid::_Throw()` | HUMANOID.CPP:4998 |
| `0x80068718` | `Humanoid::_TableThrow()` | HUMANOID.CPP:5061 |
| `0x8006882C` | `Humanoid::_GotHitHigh()` | HUMANOID.CPP:5114 |
| `0x800688B4` | `Humanoid::_GotHitMed()` | HUMANOID.CPP:5161 |
| `0x80068914` | `Humanoid::_GotHitBackGrab()` | HUMANOID.CPP:5200 |
| `0x800689B4` | `Humanoid::_GotHitLow()` | HUMANOID.CPP:5232 |
| `0x80068A14` | `Humanoid::_GotHitCrusher()` | HUMANOID.CPP:5295 |
| `0x80068A64` | `Humanoid::_GotHitFire()` | HUMANOID.CPP:5313 |
| `0x80068AB4` | `Humanoid::_Stunned()` | HUMANOID.CPP:5333 |
| `0x80068B78` | `Humanoid::_SpinBack()` | HUMANOID.CPP:5373 |
| `0x80068BC8` | `Humanoid::_FlyingBack()` | HUMANOID.CPP:5397 |
| `0x80068C9C` | `Humanoid::_Floating()` | HUMANOID.CPP:5425 |
| `0x80068D38` | `Humanoid::TestAndSetRisingAttack()` | HUMANOID.CPP:5438 |
| `0x80068DD4` | `Humanoid::_Collapse()` | HUMANOID.CPP:5476 |
| `0x80068EF8` | `Humanoid::_ThrowCharacterReceive()` | HUMANOID.CPP:5541 |
| `0x80068FFC` | `Humanoid::_ThrowFreeFall()` | HUMANOID.CPP:5622 |
| `0x8006909C` | `Humanoid::BodyThrowAttack(long)` | HUMANOID.CPP:5666 |
| `0x800691DC` | `Humanoid::_Dead()` | HUMANOID.CPP:5723 |
| `0x8006934C` | `Humanoid::_CrouchUp()` | HUMANOID.CPP:5822 |
| `0x80069420` | `Humanoid::_CounterAttack()` | HUMANOID.CPP:5861 |
| `0x80069518` | `Humanoid::_CounterAttackPreLatch()` | HUMANOID.CPP:5928 |
| `0x800695A8` | `Humanoid::_CounterAttackLatch()` | HUMANOID.CPP:5963 |
| `0x80069638` | `Humanoid::_CounterAttackRecovery()` | HUMANOID.CPP:6005 |
| `0x80069688` | `Humanoid::CheckForLanding()` | HUMANOID.CPP:6021 |
| `0x800697C4` | `Humanoid::CheckForPickup()` | HUMANOID.CPP:6081 |
| `0x80069894` | `Humanoid::CheckforPickup(unsigned long)` | HUMANOID.CPP:6138 |
| `0x80069968` | `Humanoid::DoJump()` | HUMANOID.CPP:6267 |
| `0x80069A04` | `Humanoid::_DoStand()` | HUMANOID.CPP:6274 |
| `0x80069A34` | `Humanoid::_DoRun()` | HUMANOID.CPP:6280 |
| `0x80069A70` | `Humanoid::_CallNextAction()` | HUMANOID.CPP:6296 |
| `0x80069AB4` | `Humanoid::HandleLand(long)` | HUMANOID.CPP:6311 |
| `0x80069B94` | `Humanoid::_LadderLatchTop()` | HUMANOID.CPP:6362 |
| `0x80069C2C` | `Humanoid::_LadderLatch()` | HUMANOID.CPP:6393 |
| `0x80069CC8` | `Humanoid::_LadderDismount()` | HUMANOID.CPP:6426 |
| `0x80069CF8` | `Humanoid::_ClimbLadder()` | HUMANOID.CPP:6448 |
| `0x80069FEC` | `Humanoid::PrepareLedgeLatch(const tagLVector*, const _RMVECT16*)` | HUMANOID.CPP:6581 |
| `0x8006A1D8` | `Humanoid::CheckForLedges()` | HUMANOID.CPP:6644 |
| `0x8006A3B0` | `Humanoid::CheckForLedges2(_RMVECT16&, tagLVector&, long)` | HUMANOID.CPP:6730 |
| `0x8006A4C4` | `Humanoid::_LedgeLatch()` | HUMANOID.CPP:6753 |
| `0x8006A538` | `Humanoid::_LedgePullup()` | HUMANOID.CPP:6764 |
| `0x8006A5D4` | `Humanoid::_SlopeSlide()` | HUMANOID.CPP:6780 |
| `0x8006A5DC` | `Humanoid::_HorizontalPoleSwing()` | HUMANOID.CPP:6804 |
| `0x8006A5E4` | `Humanoid::FillSphere(tSphere&) const` | HUMANOID.CPP:6903 |
| `0x8006A650` | `Humanoid::ProcessSoundEvent(long, long)` | HUMANOID.CPP:6920 |
| `0x8006A6D4` | `Humanoid::ProcessFightingMove(const FightingMove&, long)` | HUMANOID.CPP:6975 |
| `0x8006A714` | `Humanoid::ProcessFightingMoveStrikeJoint(const FightingJoint&, long, long, long, int, int)` | HUMANOID.CPP:7017 |
| `0x8006ADC8` | `Humanoid::GetTargetingFrame(const StrikeFightingMove&)` | HUMANOID.CPP:7350 |
| `0x8006AE0C` | `Humanoid::ProcessGenericFightingMove(const StrikeFightingMove&, long)` | HUMANOID.CPP:7404 |
| `0x8006B0A0` | `Humanoid::ProcessBodyThrow(const ThrowFightingMove&, long)` | HUMANOID.CPP:7504 |
| `0x8006B5A8` | `Humanoid::FindSiblingWithRequestedCommand(const FightingComboNode*, long)` | HUMANOID.CPP:7713 |
| `0x8006B5EC` | `Humanoid::FindSiblingWithRequestedCommand(const FightingComboNode*, long, long)` | HUMANOID.CPP:7741 |
| `0x8006B658` | `Humanoid::FindChildWithRequestedCommand(const FightingComboNode*, long)` | HUMANOID.CPP:7776 |
| `0x8006B67C` | `Humanoid::FindChildWithRequestedCommand(const FightingComboNode*, long, long)` | HUMANOID.CPP:7797 |
| `0x8006B6A0` | `Humanoid::DropPickup(int, int)` | HUMANOID.CPP:7819 |
| `0x8006B778` | `Humanoid::FightTargetAndThrowLatch(FightingType)` | HUMANOID.CPP:7856 |
| `0x8006B8C8` | `Humanoid::EnterCombatCombo()` | HUMANOID.CPP:7968 |
| `0x8006BA30` | `Humanoid::SetCurrentFightingNode()` | HUMANOID.CPP:8049 |
| `0x8006BC40` | `Humanoid::DoTrailCallbacks(const FightingJoint&)` | HUMANOID.CPP:8148 |
| `0x8006BEB4` | `Humanoid::DisableTrailCallbacks()` | HUMANOID.CPP:8284 |
| `0x8006BF04` | `Humanoid::ReSyncOrientation(const FightingMove&)` | HUMANOID.CPP:8311 |
| `0x8006BFE0` | `Humanoid::_ProcessFightingComboNode()` | HUMANOID.CPP:8363 |
| `0x8006C2C8` | `Humanoid::TestAndSetWeaponKungFU()` | HUMANOID.CPP:8569 |
| `0x8006C31C` | `Humanoid::TestWallContextFightingRequestRemap()` | HUMANOID.CPP:8607 |
| `0x8006C3EC` | `Humanoid::_GotHitFreeForm()` | HUMANOID.CPP:8682 |
| `0x8006C42C` | `Humanoid::LetGoOfLedge()` | HUMANOID.CPP:8734 |
| `0x8006C564` | `Humanoid::_NISMode()` | HUMANOID.CPP:8770 |
| `0x8006C59C` | `Humanoid::QuickCheckWallCollision(long, long, long, long)` | HUMANOID.CPP:8789 |
| `0x8006C5E8` | `Humanoid::CheckWallCollision(long, long, long, long, long&, _RMVECT16&, tagLVector&, long&, long&)` | HUMANOID.CPP:8815 |
| `0x8006C750` | `Humanoid::CheckDWOCollision(long, long)` | HUMANOID.CPP:8852 |
| `0x8006C9B8` | `Humanoid::CheckWallConstraint(unsigned long, unsigned long, long, long&, tagLVector&)` | HUMANOID.CPP:8937 |
| `0x8006CB4C` | `Humanoid::LoadDialog(unsigned long, long)` | HUMANOID.CPP:9027 |
| `0x8006CBA0` | `Humanoid::PlayDialog(unsigned long, unsigned long)` | HUMANOID.CPP:9085 |
| `0x8006CC38` | `Humanoid::PlayDialogBasedOnPriority(long, long)` | HUMANOID.CPP:9150 |
| `0x8006CCF8` | `Humanoid::KillDialog(int, long, long)` | HUMANOID.CPP:9220 |
| `0x8006CDC0` | `Humanoid::KillDialogBasedOnID(int, long)` | HUMANOID.CPP:9281 |
| `0x8006CE5C` | `Humanoid::HasEnemyTauntDialog()` | HUMANOID.CPP:9350 |
| `0x8006CEB4` | `Humanoid::SubtractHitPoints(unsigned short)` | HUMANOID.CPP:9388 |
| `0x8006CF00` | `Humanoid::Kill()` | HUMANOID.CPP:9405 |
| `0x8006CF50` | `Humanoid::_Killed()` | HUMANOID.CPP:9440 |
| `0x8006CF7C` | `Humanoid::GetStraifPhase()` | HUMANOID.CPP:9471 |
| `0x8006CFFC` | `Humanoid::RequestAction(unsigned long)` | HUMANOID.CPP:9557 |
| `0x8006D014` | `Humanoid::DeleteLeftHandObj()` | HUMANOID.CPP:6225 |
| `0x8006D070` | `Humanoid::DeleteRightHandObj()` | HUMANOID.CPP:6202 |
| `0x8006D0CC` | `Humanoid::SetRightHandObj(Pickup*)` | HUMANOID.CPP:6190 |
| `0x8006D104` | `Humanoid::PlayCombatThrowDialog()` | HUMANOID.HPP:1388 |
| `0x8006D10C` | `Humanoid::PlayCombatKnockDownDialog(DamageTypesTags)` | HUMANOID.HPP:1387 |
| `0x8006D114` | `Humanoid::LoadCombatDialog()` | HUMANOID.HPP:1386 |
| `0x8006D11C` | `Humanoid::HandleHitShock(DamageTypesTags)` | HUMANOID.HPP:1052 |
| `0x8006D124` | `_._4Path` | PATH.HPP:108 |
| `0x8006D208` | `SetDefaultCollisionPoint(const DBRoot&, int, tagLVector&, int)` | PICKUP.CPP:335 |
| `0x8006D3F4` | `GetMoveStruct(unsigned short)` | PICKUP.CPP:409 |
| `0x8006D45C` | `Pickup::Pickup(const tagLVector*, unsigned short)` | PICKUP.CPP:436 |
| `0x8006D5B0` | `_._6Pickup` | PICKUP.CPP:538 |
| `0x8006D618` | `Pickup::Reset()` | PICKUP.CPP:547 |
| `0x8006D688` | `Pickup::CreateModel(const char*)` | PICKUP.CPP:564 |
| `0x8006D710` | `Pickup::AnalyzeMesh(DBRoot*)` | PICKUP.CPP:595 |
| `0x8006D984` | `Pickup::Think()` | PICKUP.CPP:708 |
| `0x8006D9D0` | `Pickup::SetupPickup(Thing*, unsigned long)` | PICKUP.CPP:719 |
| `0x8006DA00` | `Pickup::UpdatePosition()` | PICKUP.CPP:747 |
| `0x8006DBC0` | `Pickup::Release(Thing*, ccList*, _RMVECT16*, long)` | PICKUP.CPP:812 |
| `0x8006DD00` | `Pickup::Move()` | PICKUP.CPP:869 |
| `0x8006DD30` | `Pickup::HandleCollision(Thing*, long, ...)` | PICKUP.CPP:877 |
| `0x8006DD5C` | `Pickup::DamageExtra()` | PICKUP.CPP:885 |
| `0x8006DD64` | `Pickup::PlayEffect()` | PICKUP.CPP:899 |
| `0x8006DDDC` | `Pickup::PickupDeactivate() const` | PICKUP.CPP:939 |
| `0x8006DE54` | `Pickup::FillSphere(tSphere&) const` | PICKUP.CPP:974 |
| `0x8006DEDC` | `Pickup::GetCollisionYMin() const` | PICKUP.CPP:997 |
| `0x8006DF9C` | `Pickup::GetWeaponSoundPtr()` | PICKUP.CPP:1025 |
| `0x8006DFA8` | `Pickup::SetPickupMove(long)` | PICKUP.CPP:1043 |
| `0x8006DFD8` | `Pickup::GetPickupMove()` | PICKUP.CPP:1063 |
| `0x8006DFEC` | `Pickup::GetPickupMoveGrabFrame()` | PICKUP.CPP:1068 |
| `0x8006E008` | `Pickup::GetThrowMove()` | PICKUP.CPP:1073 |
| `0x8006E01C` | `Pickup::GetThrowMoveThrowFrame()` | PICKUP.CPP:1078 |
| `0x8006E038` | `HumanoidModel::HumanoidModel()` | MHUMAN.CPP:33 |
| `0x8006E0C8` | `_._13HumanoidModel` | MHUMAN.CPP:56 |
| `0x8006E114` | `HumanoidModel::SetupModelCallbacks()` | MHUMAN.CPP:69 |
| `0x8006E1B0` | `HumanoidModel::SetAnim(long, long, int, long)` | MHUMAN.CPP:119 |
| `0x8006E3E8` | `HumanoidModel::_Loop(AnimStructure*)` | MHUMAN.CPP:196 |
| `0x8006E418` | `HumanoidModel::Animate()` | MHUMAN.CPP:207 |
| `0x8006E46C` | `HumanoidModel::SetTransitionAnim(long, long)` | MHUMAN.CPP:224 |
| `0x8006E4E4` | `RotMatrixZYXAndLights(unsigned short, unsigned short, unsigned short)` | MODEL.CPP:614 |
| `0x8006E5A8` | `RotMatrixZYXNoLights(unsigned short, unsigned short, unsigned short)` | MODEL.CPP:644 |
| `0x8006E654` | `Model::Model()` | MODEL.CPP:671 |
| `0x8006E6CC` | `_._5Model` | MODEL.CPP:697 |
| `0x8006E758` | `Model::DeleteDrawable()` | MODEL.CPP:720 |
| `0x8006E83C` | `Model::DeleteAnimStructures()` | MODEL.CPP:768 |
| `0x8006E888` | `Model::Reset()` | MODEL.CPP:777 |
| `0x8006E8C4` | `GModel::GModel()` | MODEL.CPP:791 |
| `0x8006E8F8` | `_._6GModel` | MODEL.CPP:795 |
| `0x8006E920` | `GModel::SetOriginalGeo(OriginalGeo*)` | MODEL.CPP:806 |
| `0x8006E96C` | `GModel::ApplyAnimToModel(long, long, long, long, long)` | MODEL.CPP:814 |
| `0x8006E974` | `GModel::Animate()` | MODEL.CPP:826 |
| `0x8006E97C` | `GModel::Show(unsigned long)` | MODEL.CPP:831 |
| `0x8006ED68` | `SModel::SModel()` | MODEL.CPP:1013 |
| `0x8006EDAC` | `_._6SModel` | MODEL.CPP:1022 |
| `0x8006EDD4` | `SModel::SetOriginalSTree(OriginalSTree*, tAnimation*)` | MODEL.CPP:1026 |
| `0x8006EE20` | `SModel::InitSemiTransMode()` | MODEL.CPP:1045 |
| `0x8006EE4C` | `SModel::IsAnimationLoaded(long)` | MODEL.CPP:1062 |
| `0x8006EEAC` | `SModel::ApplyAnimToModel(long, long, long, long, long)` | MODEL.CPP:1103 |
| `0x8006EF98` | `SModel::ApplyAnimToModel(tAnimation*, long, long, long)` | MODEL.CPP:1157 |
| `0x8006F068` | `SModel::ApplyAnimToModelBasic(tAnimation*)` | MODEL.CPP:1201 |
| `0x8006F264` | `AnimBlender(long, tPose*, tTree*)` | MODEL.CPP:1265 |
| `0x8006F438` | `SModel::InitBlendPose()` | MODEL.CPP:1317 |
| `0x8006F4A0` | `SModel::ApplyBlending(tAnimation*, long, long)` | MODEL.CPP:1346 |
| `0x8006F640` | `SModel::Animate()` | MODEL.CPP:1416 |
| `0x8006F68C` | `SModel::Show(unsigned long)` | MODEL.CPP:1454 |
| `0x8006FAD4` | `SModel::MirrorTree()` | MODEL.CPP:1643 |
| `0x8006FAFC` | `SModel::PlayDynamicAnim(int)` | MODEL.CPP:1658 |
| `0x8006FB50` | `EModel::EModel()` | MODEL.CPP:1675 |
| `0x8006FB84` | `_._6EModel` | MODEL.CPP:1679 |
| `0x8006FBAC` | `EModel::SetOriginalETree(OriginalETree*, tAnimation*)` | MODEL.CPP:1692 |
| `0x8006FC34` | `EModel::ApplyAnimToModel(long, long, long, long, long)` | MODEL.CPP:1736 |
| `0x8006FCAC` | `EModel::ApplyAnimToModel(tAnimation*, long, long, long)` | MODEL.CPP:1752 |
| `0x8006FD10` | `EModel::Animate()` | MODEL.CPP:1761 |
| `0x8006FD44` | `EModel::Show(unsigned long)` | MODEL.CPP:1771 |
| `0x80070034` | `Model::SetAnim(long, long, int, long)` | MODEL.CPP:1907 |
| `0x8007003C` | `Model::_Loop(AnimStructure*)` | MODEL.CPP:1916 |
| `0x8007005C` | `Model::_LoopReverse(AnimStructure*)` | MODEL.CPP:1921 |
| `0x8007007C` | `Model::_HoldFirst(AnimStructure*)` | MODEL.CPP:1926 |
| `0x8007009C` | `Model::_HoldLast(AnimStructure*)` | MODEL.CPP:1931 |
| `0x800700BC` | `Model::_HoldFrame(AnimStructure*)` | MODEL.CPP:1936 |
| `0x800700C4` | `Model::_RunToLast(AnimStructure*)` | MODEL.CPP:1948 |
| `0x800700E4` | `Model::_RunToFrame(AnimStructure*)` | MODEL.CPP:1953 |
| `0x800700EC` | `Model::_LoopDesired(AnimStructure*)` | MODEL.CPP:1975 |
| `0x800700F4` | `Model::_IncFrame(AnimStructure*)` | MODEL.CPP:1998 |
| `0x800700FC` | `Model::_DecFrame(AnimStructure*)` | MODEL.CPP:2004 |
| `0x8007011C` | `Model::_RunToLastBlend(AnimStructure*)` | MODEL.CPP:2009 |
| `0x8007013C` | `Model::AllocateAmbientLight()` | MODEL.CPP:2024 |
| `0x80070170` | `Model::DeleteAmbientLight()` | MODEL.CPP:2030 |
| `0x800701BC` | `Model::AllocateHardwareLights(unsigned long)` | MODEL.CPP:2039 |
| `0x80070254` | `Model::DeleteHardwareLights()` | MODEL.CPP:2046 |
| `0x800702E8` | `MYrmCartesianToSpherical(_RMVECT16*, RMVECTS16*)` | MODEL.CPP:2057 |
| `0x80070434` | `MYrmSphericalToCartesian(RMVECTS16*, _RMVECT16*)` | MODEL.CPP:2100 |
| `0x80070468` | `headTrackCallback(tSJoint*, _RMVECT16*)` | MODEL.CPP:2132 |
| `0x80070740` | `AnimStructure::AnimStructure(long, tAnimation*, long, Model*, DrawableBasic*)` | MODEL.CPP:2335 |
| `0x80070AB8` | `_._13AnimStructure` | MODEL.CPP:2436 |
| `0x80070B6C` | `AnimStructure::ReAttachTree(long, long)` | MODEL.CPP:2455 |
| `0x80070C20` | `AnimStructure::SetLoopType(long, int)` | MODEL.CPP:2484 |
| `0x80070D30` | `AnimStructure::ResetCountsToAnim()` | MODEL.CPP:2554 |
| `0x80070DB8` | `AnimStructure::ForceFrame(long)` | MODEL.CPP:2587 |
| `0x80070E1C` | `AnimStructure::ExecuteHandler(int)` | MODEL.CPP:2594 |
| `0x80071108` | `AnimStructure::ProcessHumanoidCB()` | MODEL.CPP:2749 |
| `0x8007119C` | `AnimStructure::Loop()` | MODEL.CPP:2772 |
| `0x80071200` | `AnimStructure::LoopReverse()` | MODEL.CPP:2781 |
| `0x80071234` | `AnimStructure::HoldFirst()` | MODEL.CPP:2795 |
| `0x80071278` | `AnimStructure::HoldLast()` | MODEL.CPP:2813 |
| `0x800712BC` | `AnimStructure::RunToLast()` | MODEL.CPP:2831 |
| `0x80071300` | `AnimStructure::IncFrame()` | MODEL.CPP:2852 |
| `0x800713D8` | `AnimStructure::DecFrame()` | MODEL.CPP:2882 |
| `0x80071410` | `AnimStructure::RunToLastBlend()` | MODEL.CPP:2901 |
| `0x8007149C` | `DrawableTree::DrawableTree(OriginalTree*)` | MODEL.CPP:2945 |
| `0x800714E0` | `_._12DrawableTree` | MODEL.CPP:2951 |
| `0x8007152C` | `DrawableSTree::DrawableSTree(OriginalSTree*)` | MODEL.CPP:2974 |
| `0x8007164C` | `_._13DrawableSTree` | MODEL.CPP:3010 |
| `0x8007170C` | `DrawableSTree::MirrorTree(SModel*)` | MODEL.CPP:3030 |
| `0x80071830` | `DrawableETree::DrawableETree(OriginalETree*)` | MODEL.CPP:3078 |
| `0x80071928` | `_._13DrawableETree` | MODEL.CPP:3104 |
| `0x800719B0` | `DrawableGeo::DrawableGeo(OriginalGeo*)` | MODEL.CPP:3126 |
| `0x80071A18` | `_._11DrawableGeo` | MODEL.CPP:3155 |
| `0x80071AF4` | `MakeBillboardMatrix(const tagLVector&, MATRIX*, int)` | MODEL.CPP:3199 |
| `0x80071BCC` | `MakeBillboardMatrixFlip(const tagLVector&, MATRIX*, int)` | MODEL.CPP:3230 |
| `0x80071CA8` | `OriginalBasic::OriginalBasic()` | MODEL.CPP:3265 |
| `0x80071CEC` | `_._13OriginalBasic` | MODEL.CPP:3274 |
| `0x80071D44` | `OriginalGeo::OriginalGeo()` | MODEL.CPP:3286 |
| `0x80071D80` | `_._11OriginalGeo` | MODEL.CPP:3294 |
| `0x80071DA8` | `OriginalGeo::Draw()` | MODEL.CPP:3298 |
| `0x80071DFC` | `OriginalTree::OriginalTree()` | MODEL.CPP:3321 |
| `0x80071E44` | `_._12OriginalTree` | MODEL.CPP:3336 |
| `0x80071EC4` | `OriginalETree::OriginalETree()` | MODEL.CPP:3389 |
| `0x80071F00` | `_._13OriginalETree` | MODEL.CPP:3394 |
| `0x80071F28` | `OriginalETree::Draw()` | MODEL.CPP:3405 |
| `0x80071F7C` | `OriginalSTree::OriginalSTree()` | MODEL.CPP:3427 |
| `0x80071FC0` | `_._13OriginalSTree` | MODEL.CPP:3434 |
| `0x80072024` | `OriginalSTree::Draw()` | MODEL.CPP:3448 |
| `0x8007205C` | `OriginalSTree::SetSemiMode(int)` | MODEL.CPP:3459 |
| `0x800721B0` | `RedirectCompositeSuitAnimation(tCompositeAnim*, const tPrimGeom*)` | MODEL.CPP:3521 |
| `0x800722A8` | `OriginalSTree::ChangeSuit(DrawableSTree*, short)` | MODEL.CPP:3588 |
| `0x80072350` | `DrawableSTree::ChangeSuit(short)` | MODEL.CPP:3630 |
| `0x80072380` | `DrawableGeo::Draw(unsigned long)` | MODEL.CPP:3178 |
| `0x800723C0` | `DrawableTree::Draw(unsigned long)` | MODEL.CPP:2962 |
| `0x80072400` | `Model::DrawShadow(unsigned long)` | MODEL.CPP:1898 |
| `0x80072440` | `SModel::SetupModelCallbacks()` | MODEL.HPP:1097 |
| `0x80072448` | `OriginalTree::Draw()` | MODEL.HPP:709 |
| `0x80072450` | `_._18AnimStructureBasic` | MODEL.HPP:483 |
| `0x80072478` | `_._13DrawableBasic` | MODEL.HPP:357 |
| `0x800724A0` | `GTEVXMatrix::FillRotZAfterSinCos()` | GTEMATRIX.HPP:163 |
| `0x800724D4` | `GTEVXMatrix::FillRotYAfterSinCos()` | GTEMATRIX.HPP:142 |
| `0x800724F0` | `GTEVXMatrix::FillRotXAfterSinCos()` | GTEMATRIX.HPP:119 |
| `0x80072514` | `FightingCollision::FindHumanoid(const Humanoid*)` | COLFIGHT.CPP:116 |
| `0x80072550` | `FightingCollision::Print()` | COLFIGHT.CPP:134 |
| `0x800725FC` | `FightingCollision::Init()` | COLFIGHT.CPP:159 |
| `0x80072668` | `FightingCollision::InsertHumanoid(Humanoid*)` | COLFIGHT.CPP:181 |
| `0x800726E0` | `FightingCollision::RemoveHumanoid(const Humanoid*)` | COLFIGHT.CPP:224 |
| `0x80072768` | `FightingCollision::GetHumanoidArray()` | COLFIGHT.CPP:258 |
| `0x80072774` | `FightingCollision::ClearAttack(const Humanoid*)` | COLFIGHT.CPP:266 |
| `0x800727D8` | `FightingCollision::CheckAttack(Humanoid**, int, const Humanoid*, const FightingCollisionAttackType*)` | COLFIGHT.CPP:306 |
| `0x800729CC` | `CheckAttack(const Humanoid*, const Humanoid*, const FightingCollisionAttackType*)` | COLFIGHT.CPP:488 |
| `0x800729EC` | `CheckAttackCylinder(const Humanoid*, const Humanoid*, const FightingCollisionAttackType*)` | COLFIGHT.CPP:501 |
| `0x80072CE4` | `CheckAttackCylinder(const Humanoid*, const Humanoid*, const tagLVector&, const Humanoid*, long)` | COLFIGHT.CPP:564 |
| `0x80072FB4` | `FightingCollision::Set(const Humanoid*, const Humanoid*)` | COLFIGHT.CPP:849 |
| `0x80073018` | `p3dGetElement(unsigned long, unsigned long, const MATRIX*)` | P3DMATH.CPP:156 |
| `0x80073054` | `p3dBuildIdentityMatrix(MATRIX*)` | P3DMATH.CPP:175 |
| `0x8007307C` | `p3dBuildRotMatrixZ(unsigned short, MATRIX*)` | P3DMATH.CPP:229 |
| `0x800730F8` | `p3dBuildRotMatrixXYZ(unsigned short, unsigned short, unsigned short, MATRIX*)` | P3DMATH.CPP:250 |
| `0x8007344C` | `p3dBuildRotMatrixZYX(unsigned short, unsigned short, unsigned short, MATRIX*)` | P3DMATH.CPP:276 |
| `0x800737A4` | `p3dBuildRotMatrixYZX(unsigned short, unsigned short, unsigned short, MATRIX*)` | P3DMATH.CPP:301 |
| `0x80073A00` | `p3dBuildTransMatrix(const _RMVECT16*, MATRIX*)` | P3DMATH.CPP:335 |
| `0x80073A3C` | `p3dBuildScaleMatrix(const _RMVECT16*, MATRIX*)` | P3DMATH.CPP:356 |
| `0x80073A88` | `p3dBuildScaleMatrix(long, long, long, MATRIX*)` | P3DMATH.CPP:365 |
| `0x80073AC8` | `p3dCopyMatrix(const MATRIX*, MATRIX*)` | P3DMATH.CPP:374 |
| `0x80073B0C` | `p3dFillTransMatrix(const _RMVECT16*, MATRIX*)` | P3DMATH.CPP:549 |
| `0x80073B30` | `p3dFillTransMatrix(long, long, long, MATRIX*)` | P3DMATH.CPP:554 |
| `0x80073B40` | `p3dGetTransMatrix(const MATRIX*, _RMVECT16*)` | P3DMATH.CPP:591 |
| `0x80073B64` | `p3dInverseOrthMatrix(const MATRIX*, MATRIX*)` | P3DMATH.CPP:676 |
| `0x80073C8C` | `p3dFillHeadingMatrix(const _RMVECT16*, const _RMVECT16*, MATRIX*)` | P3DMATH.CPP:858 |
| `0x80073D9C` | `p3dMultMatrix(const MATRIX*, const MATRIX*, MATRIX*)` | P3DMATH.CPP:874 |
| `0x80073E78` | `p3dPreMultMatrix(const MATRIX*, MATRIX*)` | P3DMATH.CPP:911 |
| `0x80073F50` | `p3dPosMultMatrix(const MATRIX*, MATRIX*)` | P3DMATH.CPP:946 |
| `0x8007401C` | `p3dVecTimesMatrix(const _RMVECT16*, const MATRIX*, _RMVECT16*)` | P3DMATH.CPP:1023 |
| `0x8007411C` | `p3dVecTimesMatrix(_RMVECT16*, const MATRIX*)` | P3DMATH.CPP:1045 |
| `0x8007421C` | `p3dVecTimesRotMatrix(const _RMVECT16*, const MATRIX*, _RMVECT16*)` | P3DMATH.CPP:1067 |
| `0x80074304` | `p3dVecTimesRotMatrix(_RMVECT16*, const MATRIX*)` | P3DMATH.CPP:1090 |
| `0x800743EC` | `rmSin16` | SIN.C:15 |
| `0x80074434` | `BehaviourAttrib::BehaviourAttrib()` | BEHAVE.CPP:690 |
| `0x800744C0` | `_._15BehaviourAttrib` | BEHAVE.CPP:726 |
| `0x8007458C` | `Behaviour::Behaviour(Humanoid*, unsigned long, long)` | BEHAVE.CPP:807 |
| `0x800746B4` | `_._9Behaviour` | BEHAVE.CPP:874 |
| `0x800746DC` | `Behaviour::SetAIHandler(unsigned long)` | BEHAVE.CPP:888 |
| `0x80074888` | `Behaviour::InActiveZone()` | BEHAVE.CPP:975 |
| `0x800748AC` | `Behaviour::Process()` | BEHAVE.CPP:991 |
| `0x80074914` | `Behaviour::ComplexAttack()` | BEHAVE.CPP:1015 |
| `0x80074BA0` | `Behaviour::BreakOffPathAndFight()` | BEHAVE.CPP:1201 |
| `0x80074C80` | `Behaviour::_AiFollowPath()` | BEHAVE.CPP:1248 |
| `0x80074F5C` | `Behaviour::_PlayerUserControl()` | BEHAVE.CPP:1474 |
| `0x800753C4` | `Behaviour::_NisControl()` | BEHAVE.CPP:1774 |
| `0x80075408` | `Behaviour::MoveToDestinationPoint(unsigned long)` | BEHAVE.CPP:1827 |
| `0x800754DC` | `Behaviour::_Idle()` | BEHAVE.CPP:1877 |
| `0x800757EC` | `Behaviour::_BackoffAndTaunt()` | BEHAVE.CPP:2033 |
| `0x80075A68` | `Behaviour::_BackOutOfTheFight()` | BEHAVE.CPP:2133 |
| `0x80075BE4` | `Behaviour::_NDMS()` | BEHAVE.CPP:2214 |
| `0x80076870` | `Behaviour::NavigateWorld(long&)` | BEHAVE.CPP:3317 |
| `0x80076C84` | `Behaviour::NavigateEnemies(int)` | BEHAVE.CPP:3549 |
| `0x80077004` | `Behaviour::_SubwayDodgeRight()` | BEHAVE.CPP:3813 |
| `0x80077200` | `Behaviour::_SubwayDodgeLeft()` | BEHAVE.CPP:3911 |
| `0x800773FC` | `Behaviour::_SubwayDodgeJump()` | BEHAVE.CPP:4001 |
| `0x800774BC` | `Behaviour::_Jumping()` | BEHAVE.CPP:4057 |
| `0x8007762C` | `Behaviour::_DieWhenWeHitTheGround()` | BEHAVE.CPP:4119 |
| `0x800776FC` | `Behaviour::_GetBackIntoActiveZone()` | BEHAVE.CPP:4174 |
| `0x800778C4` | `Behaviour::InitPathAIState(LinearPath*)` | BEHAVE.CPP:4299 |
| `0x800778F8` | `Behaviour::LookAheadFloorCheck(long, long, long)` | BEHAVE.CPP:4325 |
| `0x80077A14` | `Behaviour::LookAheadWallCheck(long, long, long)` | BEHAVE.CPP:4355 |
| `0x80077A4C` | `Behaviour::DisableInputProcessing()` | BEHAVE.CPP:1792 |
| `0x80077A60` | `PlayerModel::PlayerModel()` | MPLAYER.CPP:88 |
| `0x80077A94` | `_._11PlayerModel` | MPLAYER.CPP:92 |
| `0x80077ABC` | `PlayerModel::SetAnim(long, long, int, long)` | MPLAYER.CPP:117 |
| `0x80077DF8` | `PlayerModel::SetupModelCallbacks()` | MPLAYER.CPP:361 |
| `0x80077E18` | `PlayerModel::_RunToLast(AnimStructure*)` | MPLAYER.CPP:374 |
| `0x80077F44` | `PlayerModel::_Loop(AnimStructure*)` | MPLAYER.CPP:573 |
| `0x80077F64` | `PlayerModel::_IncFrame(AnimStructure*)` | MPLAYER.CPP:578 |
| `0x80077F84` | `PlayerModel::MirrorTree()` | MPLAYER.CPP:600 |
| `0x80077FA4` | `nisCharMgrCallback::nisCharMgrCallback(int, Q22AI10ThingTypesP9AnimEnums*)` | MPLAYER.CPP:646 |
| `0x80078008` | `nisCharMgrCallback::Callback()` | MPLAYER.CPP:658 |
| `0x80078088` | `PlayerModel::LoadNIS(unsigned long, const char**, int, int)` | MPLAYER.CPP:676 |
| `0x800782A0` | `_._18nisCharMgrCallback` | MPLAYER.CPP:644 |
| `0x800782D4` | `_._15CharMgrCallback` | CHARMGR.HPP:83 |
| `0x80078308` | `CharMgrCallback::Callback()` | CHARMGR.HPP:82 |
| `0x80078314` | `p3dHash(const char*)` | HASH.CPP:10 |
| `0x80078464` | `HeadTrack(Humanoid*, tSJoint*)` | ANIMMAT.CPP:187 |
| `0x8007857C` | `AM_HeadCallback(tSJoint*, int)` | ANIMMAT.CPP:251 |
| `0x800785D8` | `AM_LHandCallback(tSJoint*, int)` | ANIMMAT.CPP:279 |
| `0x8007860C` | `AM_RHandCallback(tSJoint*, int)` | ANIMMAT.CPP:303 |
| `0x80078640` | `AM_LFootCallback(tSJoint*, int)` | ANIMMAT.CPP:327 |
| `0x80078674` | `AM_RFootCallback(tSJoint*, int)` | ANIMMAT.CPP:351 |
| `0x800786A8` | `AM_PelvisCallback(tSJoint*, int)` | ANIMMAT.CPP:375 |
| `0x800786DC` | `AM_LUpperArm(tSJoint*, int)` | ANIMMAT.CPP:400 |
| `0x8007871C` | `AM_RUpperArm(tSJoint*, int)` | ANIMMAT.CPP:425 |
| `0x8007875C` | `AM_LThigh(tSJoint*, int)` | ANIMMAT.CPP:451 |
| `0x8007879C` | `AM_RThigh(tSJoint*, int)` | ANIMMAT.CPP:477 |
| `0x800787DC` | `AM_Bip_O_One_Callback(tSJoint*, int)` | ANIMMAT.CPP:511 |
| `0x80078880` | `AnimationMatrices::AnimationMatrices()` | ANIMMAT.CPP:550 |
| `0x80078908` | `AnimationMatrices::SetHumanoid(Humanoid*)` | ANIMMAT.CPP:577 |
| `0x80078910` | `AnimationMatrices::GetHumanoid()` | ANIMMAT.CPP:587 |
| `0x8007891C` | `AnimationMatrices::Copy() const` | ANIMMAT.CPP:597 |
| `0x80078928` | `AnimationMatrices::SetupCallbacks(Model*, const char**)` | ANIMMAT.CPP:614 |
| `0x80078BD0` | `AnimationMatrices::SetupExtraCallbacks(Model*, const char**)` | ANIMMAT.CPP:707 |
| `0x80078CAC` | `AnimationMatrices::SetExtraCallbacks(Q217AnimationMatrices14AM_MatrixTypesi)` | ANIMMAT.CPP:740 |
| `0x80078E0C` | `AnimationMatrices::CopyMatrix(unsigned long, tSJoint*)` | ANIMMAT.CPP:793 |
| `0x80078E80` | `AnimationMatrices::Swap()` | ANIMMAT.CPP:831 |
| `0x80078E98` | `AnimationMatrices::GetMatrix(unsigned long) const` | ANIMMAT.CPP:849 |
| `0x80078EB8` | `AnimationMatrices::GetAttack(unsigned long, tagLVector&, tagLVector&) const` | ANIMMAT.CPP:878 |
| `0x80078F64` | `AnimationMatrices::GetWeaponAttack(unsigned long, const tagLVector&, tagLVector&, tagLVector&) const` | ANIMMAT.CPP:913 |
| `0x800791B8` | `TrailInfo::SetupDecrements(int)` | TRAIL.CPP:86 |
| `0x800793C4` | `TrailInfo::SetVelocity(tagLVector*)` | TRAIL.CPP:127 |
| `0x80079400` | `TrailInfo::Update()` | TRAIL.CPP:153 |
| `0x80079614` | `Trails::Trails(int)` | TRAIL.CPP:220 |
| `0x80079778` | `_._6Trails` | TRAIL.CPP:265 |
| `0x80079894` | `Trails::Add(tagLVector*, tagLVector*, unsigned long, int, tagLVector*)` | TRAIL.CPP:297 |
| `0x80079AB4` | `Trails::PutBackEffect()` | TRAIL.CPP:479 |
| `0x80079AF4` | `Trails::Flush()` | TRAIL.CPP:499 |
| `0x80079B54` | `Trails::FindDoneTrail(int)` | TRAIL.CPP:517 |
| `0x80079B88` | `Trails::Update()` | TRAIL.CPP:548 |
| `0x80079C44` | `Trails::SetCurrentPos(tagLVector*)` | TRAIL.CPP:586 |
| `0x80079C4C` | `Trails::Display(int)` | TRAIL.CPP:599 |
| `0x80079D1C` | `Trails::ChanZSortDisplayNonTexture(int)` | TRAIL.CPP:629 |
| `0x8007A134` | `Trails::ChanZSortDisplayTexture(int)` | TRAIL.CPP:826 |
| `0x8007A598` | `static_destroy(SPL_TRAIL_RATIO)` | TRAIL.CPP:1071 |
| `0x8007A5D8` | `static_init(SPL_TRAIL_RATIO)` | TRAIL.CPP:1071 |
| `0x8007A61C` | `Trails::Create()` | TRAIL.HPP:103 |
| `0x8007A624` | `_._9TrailInfo` | TRAIL.HPP:43 |
| `0x8007A64C` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8007A69C` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8007A6F0` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8007A740` | `CheckXZStaticBoxCylinderCollision(const tagLVector&, const tagCollisionBox&, long, long, const tagCollisionCylinder&)` | OBSTACLE.CPP:195 |
| `0x8007A970` | `GetXZStaticBoxCylinderCollisionSortDistance(const tagLVector&, const tagCollisionBox&, long, long)` | OBSTACLE.CPP:241 |
| `0x8007AB90` | `CheckStaticBoxCylinderCollision_Obstacle(const tagLVector&, const tagCollisionBox&, long, long, const tagCollisionCylinder&)` | OBSTACLE.CPP:328 |
| `0x8007AE04` | `Obstacle::Draw()` | OBSTACLE.CPP:366 |
| `0x8007AEF0` | `Obstacle::FillVectorArray(tagLVector*, const DBLine&)` | OBSTACLE.CPP:447 |
| `0x8007AF14` | `Obstacle::FillVectorArray(tagLVector*, unsigned long, const DBLine&)` | OBSTACLE.CPP:472 |
| `0x8007AF6C` | `Obstacle::FillCollisionBox(tagCollisionBox&, const DBRoot&, unsigned long)` | OBSTACLE.CPP:530 |
| `0x8007AFD0` | `IncludeNumberInRange(short&, short, short)` | OBSTACLE.CPP:555 |
| `0x8007B010` | `IncludeVertexInBox(tagCollisionBox&, const SVECTOR&)` | OBSTACLE.CPP:578 |
| `0x8007B068` | `Obstacle::FillVehicleCollisionBoxes(tagCollisionBox&, tagCollisionBox&, const DBRoot&, unsigned long, long)` | OBSTACLE.CPP:601 |
| `0x8007B184` | `Obstacle::FillBoxCentre(tagLVector&, const tagLVector&, const _RMVECT16&, const tagCollisionBox&)` | OBSTACLE.CPP:651 |
| `0x8007B2F0` | `Obstacle::GetWorldFloorHeight(const tagLVector&)` | OBSTACLE.CPP:680 |
| `0x8007B328` | `Obstacle::GetYRotation(long, long)` | OBSTACLE.CPP:749 |
| `0x8007B398` | `Obstacle::CorrectThingPosition(const tagLVector&, const tagLVector&, long, long, const tagCollisionBox&, const tagLVector&, const tagLVector&, long, long, long, tagLVector&, _RMVECT16&, tagLVector&, _)` | OBSTACLE.CPP:842 |
| `0x8007BE24` | `Obstacle::SetCollisionBox(const tagCollisionBox&)` | OBSTACLE.CPP:1087 |
| `0x8007BE84` | `Obstacle::LedgeCheck(const tagCollisionBox&, const _RMVECT16&, const tagLVector&, Humanoid*)` | OBSTACLE.CPP:1124 |
| `0x8007C034` | `Obstacle::HandlePickupObstacleCollision(Pickup*)` | OBSTACLE.CPP:1212 |
| `0x8007C178` | `Obstacle::HandleHumanoidObstacleCollision(Humanoid*)` | OBSTACLE.CPP:1301 |
| `0x8007C6DC` | `Obstacle::DetectObstacleAboveLedge(const _RMVECT16&, const tagLVector&)` | OBSTACLE.CPP:1576 |
| `0x8007C7B0` | `Obstacle::DetectObstacle(const tagLVector&, const tagLVector&, long)` | OBSTACLE.CPP:1611 |
| `0x8007CA08` | `Obstacle::Obstacle(const tagLVector*, unsigned short)` | OBSTACLE.CPP:1719 |
| `0x8007CA7C` | `_._8Obstacle` | OBSTACLE.CPP:1733 |
| `0x8007CAA4` | `Obstacle::Load(tReadChunk&, void**)` | OBSTACLE.CPP:1749 |
| `0x8007CB5C` | `Obstacle::ClearPetalAnimList()` | OBSTACLE.CPP:1797 |
| `0x8007CB88` | `Obstacle::GetAnimation(long)` | OBSTACLE.CPP:1814 |
| `0x8007CBC4` | `Obstacle::AnalyzeMesh(DBRoot*)` | OBSTACLE.CPP:1833 |
| `0x8007CC64` | `Obstacle::CreateModel(const char*)` | OBSTACLE.CPP:1877 |
| `0x8007CC6C` | `Obstacle::AllocateAndCreateModel(const char*)` | OBSTACLE.CPP:1891 |
| `0x8007CD2C` | `Obstacle::AllocateAndCreateShadow()` | OBSTACLE.CPP:1941 |
| `0x8007CD94` | `Obstacle::DeleteModel()` | OBSTACLE.CPP:1960 |
| `0x8007CD9C` | `Obstacle::Reset()` | OBSTACLE.CPP:1969 |
| `0x8007CDA4` | `Obstacle::Think()` | OBSTACLE.CPP:1978 |
| `0x8007CDD4` | `Obstacle::Move()` | OBSTACLE.CPP:1988 |
| `0x8007CDDC` | `Obstacle::UpdatePosition()` | OBSTACLE.CPP:1997 |
| `0x8007CDE4` | `Obstacle::Trigger()` | OBSTACLE.CPP:2007 |
| `0x8007CDEC` | `Obstacle::ExplosiveTrigger(int, const char*)` | OBSTACLE.CPP:2017 |
| `0x8007CDF4` | `Obstacle::GetDeltaVelocity() const` | OBSTACLE.CPP:2029 |
| `0x8007CE00` | `Obstacle::Trigger(Thing*, const char*, Thing*)` | OBSTACLE.CPP:2045 |
| `0x8007CE08` | `Obstacle::FillSphere(tSphere&) const` | OBSTACLE.CPP:2066 |
| `0x8007D034` | `Obstacle::GetPhysical() const` | OBSTACLE.CPP:2152 |
| `0x8007D078` | `YRotate(tagLVector&, long, const tagLVector&)` | OBSTACLE.CPP:2193 |
| `0x8007D180` | `Obstacle::GetFloorMaterial() const` | OBSTACLE.CPP:2208 |
| `0x8007D188` | `Obstacle::GetObstacleFloorHeight(const tagLVector&) const` | OBSTACLE.CPP:2218 |
| `0x8007D198` | `Obstacle::StaticGetObstacleFloorHeight(const tagLVector&)` | OBSTACLE.CPP:2232 |
| `0x8007D354` | `Obstacle::UpdateShadowFloorHeight()` | OBSTACLE.CPP:2300 |
| `0x8007D40C` | `Obstacle::MovePassengersBasic()` | OBSTACLE.CPP:2348 |
| `0x8007D50C` | `static_destroy(PICKUP_OBSTACLE_CHECK_COUNT)` | OBSTACLE.CPP:2387 |
| `0x8007D5E0` | `static_init(PICKUP_OBSTACLE_CHECK_COUNT)` | OBSTACLE.CPP:2387 |
| `0x8007D6C4` | `tChanLitFarTable::Install()` | DRAWTABL.HPP:105 |
| `0x8007D70C` | `_._16tChanLitFarTable` | DRAWTABL.HPP:133 |
| `0x8007D734` | `tChanLitTable::Install()` | DRAWTABL.HPP:72 |
| `0x8007D77C` | `_._13tChanLitTable` | DRAWTABL.HPP:99 |
| `0x8007D7A4` | `tChanZFarTable::Install()` | DRAWTABL.HPP:42 |
| `0x8007D7FC` | `_._14tChanZFarTable` | DRAWTABL.HPP:66 |
| `0x8007D824` | `tChanZSortTable::Install()` | DRAWTABL.HPP:13 |
| `0x8007D87C` | `_._15tChanZSortTable` | DRAWTABL.HPP:36 |
| `0x8007D8A4` | `Obstacle::HandleAttack(Humanoid*, DamageTypesTags, long, short)` | OBSTACLE.HPP:391 |
| `0x8007D8AC` | `Obstacle::CareAboutAttack() const` | OBSTACLE.HPP:389 |
| `0x8007D8B4` | `rmDiv16i` | DIVIDE.C:30 |
| `0x8007D980` | `GetEnumFromHashTable(Hash_Enum*, unsigned long, long)` | HUMNDATA.CPP:71 |
| `0x8007D9B8` | `GetPreActiveIdle(long)` | HUMNDATA.CPP:88 |
| `0x8007D9F8` | `GetTauntAnim(long)` | HUMNDATA.CPP:106 |
| `0x8007DA38` | `GetCharSubTypeEnumFromHashID(long)` | HUMNDATA.CPP:144 |
| `0x8007DA78` | `CharSubTypeDataTableElement(Q28Humanoid17CharacterSubTypes)` | HUMNDATA.CPP:166 |
| `0x8007DAC4` | `GetCharSubTypeScale(Q28Humanoid17CharacterSubTypes)` | HUMNDATA.CPP:192 |
| `0x8007DAF8` | `GetCharSubTypeClut(Q28Humanoid17CharacterSubTypes)` | HUMNDATA.CPP:214 |
| `0x8007DB2C` | `GetCharSubTypeHitPoints(Q28Humanoid17CharacterSubTypes)` | HUMNDATA.CPP:237 |
| `0x8007DB60` | `GetBehaviourNameHash(Q28Humanoid17CharacterSubTypes)` | HUMNDATA.CPP:260 |
| `0x8007DB94` | `GetHumanoidData(Q22AI10ThingTypes)` | HUMNDATA.CPP:297 |
| `0x8007DBE0` | `FindFightingSystem(unsigned long)` | FIGHTANI.CPP:213 |
| `0x8007DC34` | `FindBossFightingSystem(unsigned long)` | FIGHTANI.CPP:233 |
| `0x8007DC8C` | `FindTypeFightingSystem(unsigned short, TypeFightingSystem*, unsigned long)` | FIGHTANI.CPP:269 |
| `0x8007DD20` | `GetFightingSystem(unsigned short)` | FIGHTANI.CPP:316 |
| `0x8007DD5C` | `GetPickupFighting(unsigned short)` | FIGHTANI.CPP:351 |
| `0x8007DD88` | `GetPickupFightingHighPickup(unsigned short)` | FIGHTANI.CPP:393 |
| `0x8007DDDC` | `GetPickupFightingLowPickup(unsigned short)` | FIGHTANI.CPP:408 |
| `0x8007DE30` | `GetPickupFightingThrow(unsigned short)` | FIGHTANI.CPP:435 |
| `0x8007DE84` | `GetPickupFightingIdle(unsigned short)` | FIGHTANI.CPP:465 |
| `0x8007DED4` | `GetrelativeAngle(long, long)` | FIGHTANI.CPP:489 |
| `0x8007DF28` | `Humanoid::_BackGrabCharacterLatch()` | FIGHTANI.CPP:518 |
| `0x8007E014` | `Humanoid::TestAndSetBackGrab()` | FIGHTANI.CPP:565 |
| `0x8007E1F4` | `rCDPTUIOpen` | RCDDBG.C:20 |
| `0x8007E268` | `rCDPTUIReadQ` | RCDDBG.C:46 |
| `0x8007E494` | `rCDPSYQOpen` | RCDDBG.C:153 |
| `0x8007E548` | `rCDPSYQReadQ` | RCDDBG.C:176 |
| `0x8007E660` | `rcd_add_temp_dir` | RCDREALX.C:64 |
| `0x8007E710` | `rcd_transfer_dir` | RCDREALX.C:79 |
| `0x8007E7D4` | `rcd_read_long` | RCDREALX.C:103 |
| `0x8007E800` | `rcd_recurse_dir` | RCDREALX.C:108 |
| `0x8007EA3C` | `rcd_get_sector` | RCDREALX.C:167 |
| `0x8007EA8C` | `rcd_init_cache` | RCDREALX.C:193 |
| `0x8007EAC0` | `rcd_get_cached_sector` | RCDREALX.C:200 |
| `0x8007EB5C` | `rcd_close_cache` | RCDREALX.C:211 |
| `0x8007EB88` | `rcd_load_directory_onstack` | RCDREALX.C:217 |
| `0x8007EC50` | `rcd_load_directory` | RCDREALX.C:259 |
| `0x8007ECB8` | `rCDRealCloseQ` | RCDREALX.C:268 |
| `0x8007ECC0` | `rCDRealInit` | RCDREALX.C:273 |
| `0x8007ECE8` | `rCDRealOpen` | RCDREALX.C:280 |
| `0x8007EDF8` | `rCDRealReadQ` | RCDREALX.C:320 |
| `0x8007F0AC` | `rCDInitQueue` | RCDQ.C:51 |
| `0x8007F0E0` | `rCDAddQInternal` | RCDQ.C:69 |
| `0x8007F1AC` | `rCDAddQ` | RCDQ.C:101 |
| `0x8007F1D0` | `rCDAddQSingle` | RCDQ.C:106 |
| `0x8007F1F4` | `rCDService` | RCDQ.C:111 |
| `0x8007F214` | `rCDWaitUntilDone` | RCDQ.C:116 |
| `0x8007F25C` | `rCDTaskService` | RCDQ.C:125 |
| `0x8007F2B8` | `rCDDone` | RCDQ.C:148 |
| `0x8007F41C` | `rCDSetPriority` | RCDQ.C:244 |
| `0x8007F458` | `rsdMusicPlayer::rsdMusicPlayer()` | RSMPLR.CPP:44 |
| `0x8007F4F0` | `_._14rsdMusicPlayer` | RSMPLR.CPP:85 |
| `0x8007F54C` | `rsdMusicPlayer::Open(const char*, long, bool, bool, unsigned long)` | RSMPLR.CPP:119 |
| `0x8007F6E4` | `rsdMusicPlayer::Close()` | RSMPLR.CPP:220 |
| `0x8007F754` | `rsdMusicPlayer::Start(unsigned long)` | RSMPLR.CPP:275 |
| `0x8007F7E4` | `rsdMusicPlayer::Stop()` | RSMPLR.CPP:332 |
| `0x8007F850` | `rsdMusicPlayer::CdYield()` | RSMPLR.CPP:377 |
| `0x8007F880` | `rsdMusicPlayer::CdAccess()` | RSMPLR.CPP:399 |
| `0x8007F8C0` | `rsdMusicPlayer::SetVolume(bool, unsigned short, unsigned short)` | RSMPLR.CPP:432 |
| `0x8007F958` | `rsdMusicPlayer::FadeIn()` | RSMPLR.CPP:464 |
| `0x8007FA08` | `rsdMusicPlayer::CueNextSong(unsigned long)` | RSMPLR.CPP:493 |
| `0x8007FA50` | `rsdMusicPlayer::FadeOut(bool)` | RSMPLR.CPP:526 |
| `0x8007FB0C` | `rsdMusicPlayer::Callback(unsigned long*, unsigned long*)` | RSMPLR.CPP:586 |
| `0x8007FB70` | `rsdMusicPlayer::FadeTask(_RTASK*)` | RSMPLR.CPP:623 |
| `0x8007FD74` | `rsdMusicPlayer::GetLastMusicWakeUp()` | RSMPLR.CPP:736 |
| `0x8007FD94` | `rsdMusicPlayer::IsCdYielded()` | RSMPLR.CPP:757 |
| `0x8007FDB4` | `rsdWorld::rsdWorld()` | RSDUTIL.CPP:194 |
| `0x8007FDD8` | `_._8rsdWorld` | RSDUTIL.CPP:219 |
| `0x8007FE00` | `rsdWorld::SetMicrophone(const tagLVector*, const long*)` | RSDUTIL.CPP:242 |
| `0x8007FE0C` | `rsdWorld::SetVolumeScale(unsigned long)` | RSDUTIL.CPP:308 |
| `0x8007FE14` | `rsdWorld::SetStereo(bool)` | RSDUTIL.CPP:331 |
| `0x8007FE30` | `rsdWorld::GetObjectVolumes(unsigned short, unsigned short*, unsigned short*)` | RSDUTIL.CPP:371 |
| `0x8007FE60` | `rsdWorld::GetObjectVolumes(unsigned short, const tagLVector*, unsigned short*, unsigned short*, unsigned long)` | RSDUTIL.CPP:409 |
| `0x800801B4` | `rsdWorld::IsObjectAtPosAudible(const tagLVector*, unsigned long)` | RSDUTIL.CPP:531 |
| `0x80080220` | `rsdWorld::PlayTransient(long, const tagLVector*, unsigned short, unsigned short, unsigned short, unsigned long)` | RSDUTIL.CPP:558 |
| `0x80080334` | `rsdWorld::PlayTransient(long, unsigned short, unsigned short, unsigned short, unsigned int)` | RSDUTIL.CPP:615 |
| `0x800804D4` | `rsdWorld::PlayTransientDelayedTask(_RTASK*)` | RSDUTIL.CPP:673 |
| `0x80080508` | `rsdPersistent::Initialize(unsigned long, unsigned long, rsdWorld*)` | RSDUTIL.CPP:710 |
| `0x80080558` | `rsdPersistent::Terminate()` | RSDUTIL.CPP:755 |
| `0x8008057C` | `(nw__13rsdPersistentUi)` | RSDUTIL.CPP:786 |
| `0x800805A0` | `(double, long, __13rsdPersistentPv)` | RSDUTIL.CPP:807 |
| `0x800805C8` | `rsdPersistent::rsdPersistent(long, const tagLVector*, unsigned long, unsigned short, unsigned short, unsigned long)` | RSDUTIL.CPP:839 |
| `0x8008068C` | `_._13rsdPersistent` | RSDUTIL.CPP:894 |
| `0x80080758` | `rsdPersistent::ObjectExists(rsdPersistent*)` | RSDUTIL.CPP:949 |
| `0x80080790` | `rsdPersistent::DeleteAll()` | RSDUTIL.CPP:980 |
| `0x800807E0` | `rsdPersistent::FadeOutAll(unsigned long)` | RSDUTIL.CPP:1024 |
| `0x80080824` | `rsdPersistent::FadeInAll(unsigned long)` | RSDUTIL.CPP:1047 |
| `0x80080864` | `rsdPersistent::UnloadQuietest(rsdPersistent*)` | RSDUTIL.CPP:1076 |
| `0x80080998` | `rsdPersistent::CaptureVoice()` | RSDUTIL.CPP:1150 |
| `0x80080A38` | `rsdPersistent::UnloadVoice()` | RSDUTIL.CPP:1193 |
| `0x80080A98` | `rsdPersistent::Think()` | RSDUTIL.CPP:1222 |
| `0x80080B34` | `rsdPersistent::ApplyVolume(bool)` | RSDUTIL.CPP:1270 |
| `0x80080C5C` | `rsdPersistent::UpdateTask(_RTASK*)` | RSDUTIL.CPP:1358 |
| `0x80080D08` | `rsdPersistent::DelayTask(_RTASK*)` | RSDUTIL.CPP:1432 |
| `0x80080D74` | `rsdAmbiance::rsdAmbiance()` | RSDAMBCE.CPP:109 |
| `0x80080DD0` | `_._11rsdAmbiance` | RSDAMBCE.CPP:140 |
| `0x80080E0C` | `rsdAmbiance::Open(const char*, long, long, long)` | RSDAMBCE.CPP:173 |
| `0x8008132C` | `rsdAmbiance::Close()` | RSDAMBCE.CPP:523 |
| `0x80081464` | `rsdAmbiance::Start(unsigned long)` | RSDAMBCE.CPP:628 |
| `0x8008158C` | `rsdAmbiance::Stop()` | RSDAMBCE.CPP:728 |
| `0x8008167C` | `rsdAmbiance::CdYield()` | RSDAMBCE.CPP:811 |
| `0x80081694` | `rsdAmbiance::CdAccess()` | RSDAMBCE.CPP:830 |
| `0x800816C0` | `rsdAmbiance::SetSpace(unsigned long)` | RSDAMBCE.CPP:858 |
| `0x80081780` | `rsdAmbiance::GetSpace()` | RSDAMBCE.CPP:927 |
| `0x8008178C` | `rsdAmbiance::SetVolume(bool, unsigned short, unsigned short)` | RSDAMBCE.CPP:952 |
| `0x80081828` | `rsdAmbiance::SetCrossFadeDuration(long)` | RSDAMBCE.CPP:998 |
| `0x800818C8` | `rsdAmbiance::FadeIn(unsigned long, bool, unsigned long)` | RSDAMBCE.CPP:1041 |
| `0x800819C0` | `rsdAmbiance::FadeOut(unsigned long, bool)` | RSDAMBCE.CPP:1125 |
| `0x80081A74` | `rsdAmbiance::FadeTask(_RTASK*)` | RSDAMBCE.CPP:1183 |
| `0x80081BF4` | `rsdAmbiance::GetForegroundSample(unsigned long*, N21)` | RSDAMBCE.CPP:1307 |
| `0x80081D0C` | `rsdAmbiance::GetBackgroundSample(unsigned long*)` | RSDAMBCE.CPP:1365 |
| `0x80081DCC` | `rsdAmbiance::SetVoiceVol(long, unsigned long, unsigned long)` | RSDAMBCE.CPP:1424 |
| `0x80081EB0` | `rsdAmbiance::AmbianceTaskStub(_RTASK*)` | RSDAMBCE.CPP:1471 |
| `0x80081ED4` | `rsdAmbiance::AmbianceTask()` | RSDAMBCE.CPP:1485 |
| `0x80082898` | `rsdAmbiance::CDDoneCallback(long, long, long)` | RSDAMBCE.CPP:1972 |
| `0x80082924` | `rsdAmbiance::CDDoneFreeMemory(long, long, long)` | RSDAMBCE.CPP:2010 |
| `0x80082950` | `rsdLoadCallback::Callback(rsdLoad&)` | RSDLOAD.HPP:53 |
| `0x80082960` | `CInteractiveMusicController::Think()` | MSCCTRLR.CPP:56 |
| `0x80082A60` | `CInteractiveMusicController::UnloadLevel()` | MSCCTRLR.CPP:170 |
| `0x80082A9C` | `CInteractiveMusicController::LoadLevel(rsSoundLocation)` | MSCCTRLR.CPP:191 |
| `0x80082ADC` | `CInteractiveMusicController::LevelBegin()` | MSCCTRLR.CPP:215 |
| `0x80082B30` | `CInteractiveMusicController::OpenPlayer(char*)` | MSCCTRLR.CPP:240 |
| `0x80082B98` | `CPhaseManager::CPhaseManager(unsigned char, unsigned char)` | PHSMNGR.CPP:21 |
| `0x80082C1C` | `_._13CPhaseManager` | PHSMNGR.CPP:34 |
| `0x80082C70` | `CPhaseManager::PlayRequest(unsigned short)` | PHSMNGR.CPP:39 |
| `0x80082CF0` | `CPhaseManager::Think()` | PHSMNGR.CPP:64 |
| `0x80082D60` | `(Q213CPhaseManager16CPhaseTableEntry)` | PHSMNGR.CPP:80 |
| `0x80082D74` | `rsdInit(void)` | RSDBACH.CPP:221 |
| `0x80082E50` | `rsdTerm(void)` | RSDBACH.CPP:306 |
| `0x80082F10` | `rsdLoadData(const unsigned char*, unsigned long)` | RSDBACH.CPP:372 |
| `0x80083140` | `rsdCreateFileList(int, int, unsigned long*)` | RSDBACH.CPP:551 |
| `0x8008330C` | `rsdFreeFileList(const char**, unsigned long)` | RSDBACH.CPP:640 |
| `0x80083388` | `rsdAllocVoice(unsigned char)` | RSDBACH.CPP:710 |
| `0x800834B4` | `rsdSetVoice(long, long)` | RSDBACH.CPP:817 |
| `0x80083550` | `rsdSetVolume(long, unsigned short, unsigned short)` | RSDBACH.CPP:867 |
| `0x80083588` | `rsdGetVolume(long, unsigned short*, long)` | RSDBACH.CPP:887 |
| `0x800835DC` | `rsdSetPitch(long, unsigned short)` | RSDBACH.CPP:923 |
| `0x80083600` | `rsdSetADSR(long, unsigned long)` | RSDBACH.CPP:954 |
| `0x8008362C` | `rsdSetLoopPoint(long, unsigned long)` | RSDBACH.CPP:986 |
| `0x80083654` | `rsdSetStartAddr(long, unsigned long)` | RSDBACH.CPP:1017 |
| `0x8008367C` | `rsdLockVoice(long)` | RSDBACH.CPP:1113 |
| `0x80083698` | `rsdReleaseVoice(long)` | RSDBACH.CPP:1136 |
| `0x800836B4` | `rsdGetVoice(long)` | RSDBACH.CPP:1262 |
| `0x8008371C` | `rsdGetPitch(long)` | RSDBACH.CPP:1353 |
| `0x80083748` | `rsdSetVolumeMain(unsigned short)` | RSDBACH.CPP:1380 |
| `0x8008379C` | `rsdAllocPhonograph(long, unsigned long*, long, bool*)` | RSDBACH.CPP:1417 |
| `0x800837E0` | `rsdFreePhonograph(long, unsigned long, bool)` | RSDBACH.CPP:1453 |
| `0x8008380C` | `rsdSetReverb(rsdReverbMode)` | RSDBACH.CPP:1481 |
| `0x80083908` | `rsdGetReverb(void)` | RSDBACH.CPP:1567 |
| `0x80083914` | `rsdSetReverbDepth(short)` | RSDBACH.CPP:1587 |
| `0x80083920` | `rsdReverbOnVoice(long)` | RSDBACH.CPP:1629 |
| `0x80083968` | `rsdReverbOffVoice(long)` | RSDBACH.CPP:1667 |
| `0x800839B4` | `rsdVoiceOn(long)` | RSDBACH.CPP:1705 |
| `0x80083A08` | `rsdVoiceOff(long)` | RSDBACH.CPP:1753 |
| `0x80083A5C` | `rsdIsVoicePending(long)` | RSDBACH.CPP:1801 |
| `0x80083A90` | `rsdIsVoiceOn(long)` | RSDBACH.CPP:1835 |
| `0x80083ABC` | `WriteOnOffBitsTask(_RTASK*)` | RSDBACH.CPP:1868 |
| `0x80083B48` | `rsdLoadCallback::Callback(rsdLoad&)` | RSDLOAD.HPP:53 |
| `0x80083B58` | `_._7rsdClip` | RSDCLIP.CPP:119 |
| `0x80083C10` | `rsdClip::ReadDialog(void**, unsigned long, unsigned long)` | RSDCLIP.CPP:163 |
| `0x80083D38` | `rsdClip::SetWorld(rsdWorld*)` | RSDCLIP.CPP:523 |
| `0x80083D44` | `rsdClip::CDDoneCallback(long, long, long)` | RSDCLIP.CPP:542 |
| `0x80083E50` | `rsdClip::FreeTransferBuffer()` | RSDCLIP.CPP:613 |
| `0x80083E98` | `rsdClip::IsVoicePlaying()` | RSDCLIP.CPP:643 |
| `0x80083EF0` | `rsdLoadCallback::Callback(rsdLoad&)` | RSDLOAD.HPP:53 |
| `0x80083F00` | `free` | MALLOC.C:20 |
| `0x80083F4C` | `rPutChar` | RDEBUG.C:33 |
| `0x80083F94` | `rPutCharDebug` | RDEBUG.C:40 |
| `0x80083FF8` | `rPutString` | RDEBUG.C:108 |
| `0x80084044` | `rAssertFail` | RDEBUG.C:129 |
| `0x8008415C` | `rWarningFail` | RDEBUG.C:155 |
| `0x80084194` | `rValidFail` | RDEBUG.C:161 |
| `0x800841D4` | `rValidPointer` | RDEBUG.C:167 |
| `0x800841F8` | `rValidPointer32` | RDEBUG.C:184 |
| `0x80084274` | `fdn_Sscanf(const char*, char*, ...)` | SUBS.CPP:91 |
| `0x800843F4` | `strcmpi` | SUBS.CPP:156 |
| `0x80084480` | `GetNextAlphaNumToken(char*, char*)` | SUBS.CPP:201 |
| `0x8008454C` | `alphatoulong(char*)` | SUBS.CPP:221 |
| `0x8008458C` | `rNewZPoolBuf` | RADZMEM.C:31 |
| `0x800845AC` | `rZMalloc` | RADZMEM.C:62 |
| `0x800845D8` | `rZCountFree` | RADZMEM.C:91 |
| `0x8008461C` | `tGeoLoader::tGeoLoader()` | TGEOLOAD.CPP:61 |
| `0x800846F0` | `tGeoLoader::Load(tReadChunk&, void**)` | TGEOLOAD.CPP:73 |
| `0x80084934` | `_._10tGeoLoader` | TGEOLOAD.HPP:64 |
| `0x80084968` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x8008499C` | `tSTree::tSTree()` | STREE.CPP:36 |
| `0x800849F4` | `tSTree::DeepCopy()` | STREE.CPP:73 |
| `0x80084B44` | `tSTree::SetNumJoints(int)` | STREE.CPP:102 |
| `0x80084C08` | `tSTree::Display()` | STREE.CPP:110 |
| `0x80084F14` | `RP_XformVertsLitCBF_CL(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | RPSTREECOL.CPP:41 |
| `0x8008500C` | `RP_FixUpPolysCBF_CL(tPrimGeom*, void*, unsigned long, unsigned long)` | RPSTREECOL.CPP:169 |
| `0x800859F8` | `tSTree::GetJointAbsolute(int)` | STREE.HPP:163 |
| `0x80085A10` | `tSTree::GetJoint(int)` | STREE.HPP:162 |
| `0x80085A3C` | `_._6tSTree` | STREE.HPP:146 |
| `0x80085B00` | `_._7tSJoint` | STREE.HPP:138 |
| `0x80085B34` | `_._10tTreeJoint` | TREE.HPP:80 |
| `0x80085B68` | `P3DLoad(tLoader**, void*, tLoader**)` | TLOADER.CPP:63 |
| `0x80085D18` | `rrLoadHeaderOnly` | PETLATL.C:16 |
| `0x80085DE8` | `rrSize` | PETLATL.C:67 |
| `0x80085DFC` | `rrOffset` | PETLATL.C:77 |
| `0x80085E10` | `tCompAnimLoader::Load(tReadChunk&, void**)` | COMPANIM.CPP:78 |
| `0x80085EA4` | `LoadCompositeAnim(tReadChunk&)` | COMPANIM.CPP:103 |
| `0x80086010` | `tCompositeAnimPart::tCompositeAnimPart()` | COMPANIM.CPP:142 |
| `0x80086028` | `_._18tCompositeAnimPart` | COMPANIM.CPP:149 |
| `0x8008608C` | `tCompositeAnim::tCompositeAnim()` | COMPANIM.CPP:157 |
| `0x800860C8` | `_._14tCompositeAnim` | COMPANIM.CPP:163 |
| `0x80086110` | `tCompositeAnim::DeleteAll()` | COMPANIM.CPP:168 |
| `0x80086190` | `tCompositeAnim::SetNumParts(int)` | COMPANIM.CPP:174 |
| `0x8008624C` | `tCompositeAnim::MakePuppet()` | COMPANIM.CPP:185 |
| `0x800862BC` | `tCompositeAnim::GetPart(int)` | COMPANIM.CPP:193 |
| `0x800862D4` | `tCompositeFlip::tCompositeFlip()` | COMPANIM.CPP:211 |
| `0x80086308` | `_._14tCompositeFlip` | COMPANIM.CPP:216 |
| `0x80086330` | `tCompositeFlip::Reset()` | COMPANIM.CPP:221 |
| `0x80086380` | `tCompositeFlip::Update()` | COMPANIM.CPP:227 |
| `0x800864E0` | `tCompositeFlip::GetEntityType()` | COMPANIM.HPP:103 |
| `0x800864EC` | `tCompositeAnim::GetNumFrames()` | COMPANIM.HPP:85 |
| `0x800864F8` | `tCompositeAnim::GetEntityType()` | COMPANIM.HPP:74 |
| `0x80086504` | `_._15tCompAnimLoader` | COMPANIM.HPP:52 |
| `0x80086538` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x8008656C` | `tClutAnimLoader::Load(tReadChunk&, void**)` | TCLTLOAD.CPP:29 |
| `0x80086858` | `_._15tClutAnimLoader` | CLUTANIM.HPP:43 |
| `0x8008688C` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800868C0` | `tMatLoader::Load(tReadChunk&, void**)` | TMATLOAD.CPP:66 |
| `0x80086A08` | `_._10tMatLoader` | TMATLOAD.HPP:72 |
| `0x80086A3C` | `_._9tMaterial` | TMAT.HPP:47 |
| `0x80086A64` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x80086A98` | `P3D::BeginFrame()` | P3DGBL.CPP:114 |
| `0x80086AC4` | `P3D::EndFrame(int)` | P3DGBL.CPP:135 |
| `0x80086AE8` | `static_destroy(_3P3D.FrameCount)` | P3DGBL.CPP:165 |
| `0x80086B28` | `static_init(_3P3D.FrameCount)` | P3DGBL.CPP:165 |
| `0x80086B70` | `tP3Dinventory::tP3Dinventory()` | P3DINV.CPP:192 |
| `0x80086BF0` | `_._13tP3Dinventory` | P3DINV.CPP:199 |
| `0x80086C8C` | `tP3Dinventory::Init()` | P3DINV.CPP:204 |
| `0x80086D68` | `tP3Dinventory::DeleteAllListsID(unsigned short)` | P3DINV.CPP:257 |
| `0x80086DFC` | `tP3Dinventory::DeleteListID(unsigned short, unsigned short)` | P3DINV.CPP:271 |
| `0x80086F24` | `tP3Dinventory::DeleteList(unsigned short)` | P3DINV.CPP:303 |
| `0x80087064` | `tP3Dinventory::DeleteObject(unsigned short, unsigned short)` | P3DINV.CPP:342 |
| `0x80087114` | `tP3Dinventory::SetListSize(unsigned short, unsigned long)` | P3DINV.CPP:377 |
| `0x8008716C` | `tP3Dinventory::StoreObject(unsigned short, tEntity*, int)` | P3DINV.CPP:410 |
| `0x800871D0` | `tP3Dinventory::FindObjectHandle(unsigned short, unsigned long)` | P3DINV.CPP:433 |
| `0x800872CC` | `tP3Dinventory::Find(unsigned short, unsigned long)` | P3DINV.CPP:458 |
| `0x8008733C` | `tP3Dinventory::FindCache(unsigned short)` | P3DINV.CPP:465 |
| `0x8008735C` | `tGameLoader::Load(tReadChunk&, void**)` | LOADERS.CPP:255 |
| `0x800874F8` | `tTexLoader::Load(tReadChunk&, void**)` | LOADERS.CPP:337 |
| `0x80087704` | `SetTransparentTim(unsigned long, unsigned short*)` | LOADERS.CPP:460 |
| `0x80087774` | `tChanSequenceAnimLoader::Load(tReadChunk&, void**)` | LOADERS.CPP:484 |
| `0x80087974` | `_._10tTexLoader` | TTEXLOAD.HPP:41 |
| `0x800879A8` | `_._23tChanSequenceAnimLoader` | LOADERS.HPP:182 |
| `0x800879DC` | `_._11tGameLoader` | LOADERS.HPP:174 |
| `0x80087A10` | `_._7tLoader` | TLOADER.HPP:76 |
| `0x80087A44` | `tSequenceAnimLoader::Load(tReadChunk&, void**)` | SEQUENCE.CPP:26 |
| `0x80087B68` | `tSequenceAnim::tSequenceAnim()` | SEQUENCE.CPP:62 |
| `0x80087B9C` | `_._13tSequenceAnim` | SEQUENCE.CPP:67 |
| `0x80087BE4` | `tSequenceAnim::DeleteAll()` | SEQUENCE.CPP:72 |
| `0x80087C64` | `tSequenceAnim::MakePuppet()` | SEQUENCE.CPP:82 |
| `0x80087CD4` | `tSequenceAnim::GetPart(int)` | SEQUENCE.CPP:90 |
| `0x80087CE8` | `tSequenceFlip::tSequenceFlip()` | SEQUENCE.CPP:108 |
| `0x80087D1C` | `_._13tSequenceFlip` | SEQUENCE.CPP:113 |
| `0x80087D44` | `tSequenceFlip::Reset()` | SEQUENCE.CPP:118 |
| `0x80087D94` | `tSequenceFlip::Update()` | SEQUENCE.CPP:124 |
| `0x80087E1C` | `tSequenceFlip::GetEntityType()` | SEQUENCE.HPP:69 |
| `0x80087E28` | `tSequenceAnim::GetNumFrames()` | SEQUENCE.HPP:52 |
| `0x80087E34` | `tSequenceAnim::GetEntityType()` | SEQUENCE.HPP:43 |
| `0x80087E40` | `_._19tSequenceAnimLoader` | SEQUENCE.HPP:30 |
| `0x80087E74` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x80087EA8` | `tTranAnimLoader2::Load(tReadChunk&, void**)` | TRANLOAD.CPP:98 |
| `0x8008817C` | `_._16tTranAnimLoader2` | TRANLOAD.HPP:42 |
| `0x800881B0` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800881E4` | `ParseParam(tFile*, tReadChunk&, int*)` | PARAMLOAD.CPP:40 |
| `0x80088570` | `LoadParamAnim(tReadChunk&, tParamAnim*(*)()*, tFlipbook*)` | PARAMLOAD.CPP:156 |
| `0x8008879C` | `tParamAnimLoader::Load(tReadChunk&, void**)` | PARAMLOAD.CPP:279 |
| `0x80088838` | `_._16tParamAnimLoader` | PARAMANIM.HPP:37 |
| `0x8008886C` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800888A0` | `rCDOpenC` | RCDCACHE.C:14 |
| `0x800888E4` | `rCDReadC` | RCDCACHE.C:22 |
| `0x80088A3C` | `rCDSeekC` | RCDCACHE.C:50 |
| `0x80088A80` | `tPrimLoader::Load(tReadChunk&, void**)` | TPRMLOAD.CPP:61 |
| `0x80088DB0` | `_._11tPrimLoader` | TPRMLOAD.HPP:71 |
| `0x80088DE4` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x80088E18` | `AddJoint(tReadChunk&, tFile*, tSJoint*, void**)` | STLOAD.CPP:121 |
| `0x80088F54` | `tSTreeLoader::tSTreeLoader()` | STLOAD.CPP:167 |
| `0x80088F80` | `tSTreeLoader::LoadInternal(tReadChunk&, void**)` | STLOAD.CPP:172 |
| `0x8008920C` | `tSTreeLoader::Load(tReadChunk&, void**)` | STLOAD.CPP:284 |
| `0x800892BC` | `_._12tSTreeLoader` | STREE.HPP:103 |
| `0x800892F0` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x80089324` | `tRAMTexAnim::tRAMTexAnim()` | RAMTEXANIM.CPP:28 |
| `0x80089358` | `_._11tRAMTexAnim` | RAMTEXANIM.CPP:33 |
| `0x800893C4` | `tRAMTexAnim::MakePuppet()` | RAMTEXANIM.CPP:47 |
| `0x80089434` | `tRAMTexFlip::tRAMTexFlip()` | RAMTEXANIM.CPP:57 |
| `0x80089468` | `_._11tRAMTexFlip` | RAMTEXANIM.CPP:62 |
| `0x80089490` | `tRAMTexFlip::Reset()` | RAMTEXANIM.CPP:67 |
| `0x800894D0` | `tRAMTexFlip::Update()` | RAMTEXANIM.CPP:73 |
| `0x800895A8` | `tRAMTexAnimLoader::Load(tReadChunk&, void**)` | RAMTEXANIM.CPP:112 |
| `0x800897DC` | `_._17tRAMTexAnimLoader` | RAMTEXANIM.HPP:92 |
| `0x80089810` | `tRAMTexFlip::GetEntityType()` | RAMTEXANIM.HPP:74 |
| `0x8008981C` | `tRAMTexAnim::SetTextureData(tTexture**)` | RAMTEXANIM.HPP:53 |
| `0x80089824` | `tRAMTexAnim::SetNumTextures(int)` | RAMTEXANIM.HPP:52 |
| `0x8008982C` | `tRAMTexAnim::SetFrameData(unsigned char*)` | RAMTEXANIM.HPP:51 |
| `0x80089834` | `tRAMTexAnim::SetNumFrames(int)` | RAMTEXANIM.HPP:50 |
| `0x8008983C` | `tRAMTexAnim::GetTexture(int)` | RAMTEXANIM.HPP:48 |
| `0x80089854` | `tRAMTexAnim::GetNumTextures()` | RAMTEXANIM.HPP:47 |
| `0x80089860` | `tRAMTexAnim::GetFrame(int)` | RAMTEXANIM.HPP:46 |
| `0x80089878` | `tRAMTexAnim::GetNumFrames()` | RAMTEXANIM.HPP:44 |
| `0x80089884` | `tRAMTexAnim::GetEntityType()` | RAMTEXANIM.HPP:39 |
| `0x80089890` | `_._7tLoader` | TLOADER.HPP:76 |
| `0x800898C8` | `tTexAnimLoader::Load(tReadChunk&, void**)` | TTXTLOAD.CPP:29 |
| `0x80089BB4` | `_._14tTexAnimLoader` | TEXANIM.HPP:65 |
| `0x80089BE8` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x80089C1C` | `rmRandom0` | RANDOM0.C:38 |
| `0x80089C6C` | `rmRangedRandom0` | RANDOM0.C:48 |
| `0x80089CA4` | `Ladder::Ladder(const tagLVector*, unsigned short)` | LADDER.CPP:142 |
| `0x80089D18` | `_._6Ladder` | LADDER.CPP:165 |
| `0x80089D40` | `Ladder::AnalyzeMesh(DBRoot*)` | LADDER.CPP:178 |
| `0x80089F48` | `Ladder::CreateModel(const char*)` | LADDER.CPP:240 |
| `0x80089F5C` | `Ladder::DeleteModel()` | LADDER.CPP:254 |
| `0x80089F70` | `Ladder::Reset()` | LADDER.CPP:264 |
| `0x80089FD4` | `Ladder::Think()` | LADDER.CPP:283 |
| `0x8008A068` | `Ladder::Move()` | LADDER.CPP:308 |
| `0x8008A070` | `Ladder::DeathCheck()` | LADDER.CPP:318 |
| `0x8008A10C` | `Ladder::UpdatePosition()` | LADDER.CPP:349 |
| `0x8008A114` | `Ladder::Draw()` | LADDER.CPP:362 |
| `0x8008A11C` | `Ladder::Trigger()` | LADDER.CPP:380 |
| `0x8008A19C` | `Ladder::TeleportPlayer()` | LADDER.CPP:403 |
| `0x8008A1F4` | `Ladder::CloseHatch()` | LADDER.CPP:418 |
| `0x8008A258` | `Ladder::HandlePickupCollision(Pickup*)` | LADDER.CPP:434 |
| `0x8008A260` | `Ladder::HandleHumanoidCollision(Humanoid*)` | LADDER.CPP:448 |
| `0x8008A570` | `Ladder::PutHumanoidOnLadder(Humanoid*)` | LADDER.CPP:635 |
| `0x8008A5DC` | `Ladder::CheckForLedges(_RMVECT16&, tagLVector&)` | LADDER.CPP:661 |
| `0x8008A708` | `WEffect::Load(tReadChunk&, void**)` | WEFFECT.CPP:100 |
| `0x8008A848` | `WEffect::Unload()` | WEFFECT.CPP:156 |
| `0x8008A8E0` | `WEffect::Find(unsigned long)` | WEFFECT.CPP:183 |
| `0x8008A954` | `WEffect::SetupPaletteData(unsigned long, unsigned long, unsigned long)` | WEFFECT.CPP:217 |
| `0x8008A9C0` | `WEffect::InitWorldEffects(DBPoint*)` | WEFFECT.CPP:245 |
| `0x8008B538` | `WEffect::WEffect()` | WEFFECT.CPP:750 |
| `0x8008B5B0` | `_._7WEffect` | WEFFECT.CPP:782 |
| `0x8008B67C` | `WEffect::CreateSound(tagLVector*)` | WEFFECT.CPP:803 |
| `0x8008B6F0` | `WEffect::UpdateSound()` | WEFFECT.CPP:831 |
| `0x8008B728` | `WEffect::ReleaseSound()` | WEFFECT.CPP:839 |
| `0x8008B774` | `WEffect::Create(int)` | WEFFECT.CPP:857 |
| `0x8008B850` | `WEffect::Create()` | WEFFECT.CPP:899 |
| `0x8008B924` | `WEffect::Purge()` | WEFFECT.CPP:950 |
| `0x8008B988` | `WEffect::PutBackEffect()` | WEFFECT.CPP:970 |
| `0x8008B9C4` | `WEffect::IsDone(int&)` | WEFFECT.CPP:992 |
| `0x8008BA08` | `WEffect::EnablePath(int)` | WEFFECT.CPP:1001 |
| `0x8008BA24` | `WEffect::Update()` | WEFFECT.CPP:1016 |
| `0x8008BD28` | `WEffect::Display(int)` | WEFFECT.CPP:1116 |
| `0x8008BE08` | `WEffect::NISRemoveEffect()` | WEFFECT.CPP:1152 |
| `0x8008BE44` | `FWEffect::FWEffect()` | WEFFECT.CPP:1169 |
| `0x8008BE88` | `_._8FWEffect` | WEFFECT.CPP:1187 |
| `0x8008BEDC` | `FWEffect::Find(unsigned long)` | WEFFECT.CPP:1208 |
| `0x8008BF50` | `FWEffect::Create(int)` | WEFFECT.CPP:1244 |
| `0x8008C024` | `FWEffect::Create()` | WEFFECT.CPP:1285 |
| `0x8008C078` | `FWEffect::Create2(unsigned long, tagLVector*, _RMVECT16*, _RMVECT16*, int)` | WEFFECT.CPP:1326 |
| `0x8008C13C` | `FWEffect::Create2(tagLVector*, _RMVECT16*, _RMVECT16*, int)` | WEFFECT.CPP:1375 |
| `0x8008C2F0` | `FWEffect::SetMentor()` | WEFFECT.CPP:1447 |
| `0x8008C3E0` | `FWEffect::SetScaleRoll(long, long)` | WEFFECT.CPP:1490 |
| `0x8008C434` | `FWEffect::Continue()` | WEFFECT.CPP:1512 |
| `0x8008C47C` | `FWEffect::Update()` | WEFFECT.CPP:1546 |
| `0x8008C9D0` | `FWEffect::Display(int)` | WEFFECT.CPP:1791 |
| `0x8008CD74` | `CBVEffect::Create2(unsigned long, int)` | WEFFECT.CPP:1914 |
| `0x8008CE18` | `CBVEffect::CBVEffect()` | WEFFECT.CPP:1955 |
| `0x8008CE70` | `_._9CBVEffect` | WEFFECT.CPP:1977 |
| `0x8008CE98` | `CBVEffect::Create()` | WEFFECT.CPP:1991 |
| `0x8008CF24` | `CBVEffect::Create2(int)` | WEFFECT.CPP:2017 |
| `0x8008CF94` | `CBVEffect::Update()` | WEFFECT.CPP:2039 |
| `0x8008CFC4` | `CBVEffect::PutBackEffect()` | WEFFECT.CPP:2058 |
| `0x8008D010` | `static_destroy(_7WEffect.gNumComEffects)` | WEFFECT.CPP:2067 |
| `0x8008D050` | `static_init(_7WEffect.gNumComEffects)` | WEFFECT.CPP:2067 |
| `0x8008D094` | `CBVEffect::Display(int)` | WEFFECT.HPP:300 |
| `0x8008D09C` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8008D0EC` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8008D140` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8008D190` | `tEntity::tEntity()` | TENTITY.CPP:92 |
| `0x8008D1A4` | `_._7tEntity` | TENTITY.CPP:98 |
| `0x8008D1D8` | `tEntity::SetName(const char*)` | TENTITY.CPP:104 |
| `0x8008D204` | `tEntity::MakeUID(const char*)` | TENTITY.CPP:114 |
| `0x8008D224` | `tEntity::GetEntityType()` | TENTITY.HPP:119 |
| `0x8008D22C` | `tAnimation::tAnimation()` | ANIMATE.CPP:66 |
| `0x8008D260` | `_._10tAnimation` | ANIMATE.CPP:71 |
| `0x8008D288` | `tPuppet::tPuppet()` | ANIMATE.CPP:77 |
| `0x8008D2C4` | `_._7tPuppet` | ANIMATE.CPP:83 |
| `0x8008D2EC` | `tFlipbook::tFlipbook()` | ANIMATE.CPP:89 |
| `0x8008D32C` | `_._9tFlipbook` | ANIMATE.CPP:96 |
| `0x8008D354` | `tFlipbook::SetFrame(int)` | ANIMATE.CPP:101 |
| `0x8008D35C` | `tFlipbook::SetFrameReal(long)` | ANIMATE.CPP:107 |
| `0x8008D38C` | `tFlipbook::UpdateReal()` | ANIMATE.CPP:113 |
| `0x8008D3BC` | `tFlipbook::SetAnimation(tAnimation*)` | ANIMATE.CPP:118 |
| `0x8008D3C4` | `tFlipbook::AdvanceFrame()` | ANIMATE.CPP:124 |
| `0x8008D40C` | `DeadPool::InternalReset()` | DEADPOOL.CPP:13 |
| `0x8008D414` | `DeadPool::AddUID(unsigned long)` | DEADPOOL.CPP:23 |
| `0x8008D43C` | `DeadPool::IsUIDInDeadPool(unsigned long)` | DEADPOOL.CPP:35 |
| `0x8008D47C` | `static_destroy(theDeadPool)` | DEADPOOL.CPP:45 |
| `0x8008D4AC` | `static_init(theDeadPool)` | DEADPOOL.CPP:45 |
| `0x8008D4D8` | `_._8DeadPool` | DEADPOOL.HPP:18 |
| `0x8008D50C` | `p3dFwdCycle(tFlipbook*)` | CYCLE.CPP:8 |
| `0x8008D5B0` | `CSoundDirect::PlayTransient(unsigned short, const tagLVector*, unsigned short, unsigned long)` | SNDDRCT.CPP:18 |
| `0x8008D650` | `CSoundDirect::BeginPersistent(unsigned char, CGenericPersistentSound**, const tagLVector*)` | SNDDRCT.CPP:45 |
| `0x8008D6C4` | `CSoundDirect::EndPersistent(CGenericPersistentSound**)` | SNDDRCT.CPP:71 |
| `0x8008D714` | `WorldPoints::InternalReset()` | WORLDPTS.CPP:15 |
| `0x8008D768` | `WorldPoints::AddPoint(DBPoint*)` | WORLDPTS.CPP:26 |
| `0x8008D8A4` | `WorldPoints::GetNISPoint(unsigned long)` | WORLDPTS.CPP:74 |
| `0x8008D8C4` | `WorldPoints::GetParPointValue()` | WORLDPTS.CPP:80 |
| `0x8008D8F8` | `static_destroy(theWorldPoints)` | WORLDPTS.CPP:84 |
| `0x8008D944` | `static_init(theWorldPoints)` | WORLDPTS.CPP:84 |
| `0x8008D994` | `_._11WorldPoints` | WORLDPTS.HPP:37 |
| `0x8008D9F0` | `_._13WorldParPoint` | WORLDPTS.HPP:28 |
| `0x8008DA10` | `_._12World3DPoint` | WORLDPTS.HPP:17 |
| `0x8008DA38` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8008DA88` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8008DADC` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8008DB2C` | `GEffect::Load(tReadChunk&, void**)` | GEFFECT.CPP:85 |
| `0x8008DCE8` | `GEffect::Unload()` | GEFFECT.CPP:163 |
| `0x8008DDD0` | `GEffect::FindEffect(unsigned long)` | GEFFECT.CPP:193 |
| `0x8008DE18` | `GEffect::Create(unsigned long, tagLVector*, _RMVECT16*, _RMVECT16*, int, int, unsigned long)` | GEFFECT.CPP:226 |
| `0x8008E184` | `GEffect::GEffect()` | GEFFECT.CPP:452 |
| `0x8008E1CC` | `_._7GEffect` | GEFFECT.CPP:472 |
| `0x8008E228` | `GEffect::PutBackEffect()` | GEFFECT.CPP:491 |
| `0x8008E264` | `GEffect::Create()` | GEFFECT.CPP:513 |
| `0x8008E26C` | `GEffect::Update()` | GEFFECT.CPP:527 |
| `0x8008E3DC` | `GEffect::Display(int)` | GEFFECT.CPP:605 |
| `0x8008E608` | `GEffect::CreateSound()` | GEFFECT.CPP:674 |
| `0x8008E6A0` | `GEffect::UpdateSound()` | GEFFECT.CPP:707 |
| `0x8008E714` | `GEffect::ReleaseSound()` | GEFFECT.CPP:719 |
| `0x8008E78C` | `static_destroy(_7GEffect.gNumComEffects)` | GEFFECT.CPP:733 |
| `0x8008E7CC` | `static_init(_7GEffect.gNumComEffects)` | GEFFECT.CPP:733 |
| `0x8008E810` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8008E860` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8008E8B4` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8008E924` | `tInventory::tInventory()` | TINVNTRY.CPP:106 |
| `0x8008E960` | `_._10tInventory` | TINVNTRY.CPP:111 |
| `0x8008E9B0` | `tInventory::ApplyFunction(unsigned short, tEntity*(*)()*, int)` | TINVNTRY.CPP:116 |
| `0x8008EA50` | `tInventory::FindHandle(unsigned short, unsigned long)` | TINVNTRY.CPP:136 |
| `0x8008EA90` | `tInventory::FindListHandle(const char*)` | TINVNTRY.CPP:147 |
| `0x8008EACC` | `tInventory::SearchList(tIndexList*, unsigned long)` | TINVNTRY.CPP:154 |
| `0x8008EB24` | `CDirectorSound::Initialize()` | DRCTRSND.CPP:12 |
| `0x8008EB44` | `CDirectorSound::ProcessNISEvent(unsigned long, unsigned long)` | DRCTRSND.CPP:21 |
| `0x8008EBAC` | `CDirectorSound::CDirectorSound()` | DRCTRSND.CPP:40 |
| `0x8008EBE4` | `_._14CDirectorSound` | DRCTRSND.CPP:47 |
| `0x8008EC44` | `tCamera::tCamera()` | TCAMERA.CPP:106 |
| `0x8008EC98` | `_._7tCamera` | TCAMERA.CPP:114 |
| `0x8008ECC0` | `tCamera::SetFOV(long, long)` | TCAMERA.CPP:118 |
| `0x8008ECCC` | `tCamera::GetFOV(long*, long*)` | TCAMERA.CPP:124 |
| `0x8008ECE4` | `tCamera::GetClipPlanes(unsigned short*, unsigned short*)` | TCAMERA.CPP:130 |
| `0x8008ECFC` | `hdHealth::hdHealth()` | HDITEM.CPP:200 |
| `0x8008ED30` | `_._8hdHealth` | HDITEM.CPP:205 |
| `0x8008ED58` | `hdHealth::SelfInit()` | HDITEM.CPP:209 |
| `0x8008EDB0` | `hdHealth::SetMax(long)` | HDITEM.CPP:227 |
| `0x8008EDB8` | `hdHealth::SetValue(long)` | HDITEM.CPP:233 |
| `0x8008EE50` | `hdHealth::SetText(char*)` | HDITEM.CPP:255 |
| `0x8008EE98` | `hdHealth::Update()` | HDITEM.CPP:263 |
| `0x8008EF0C` | `hdTtlive::Update()` | HDITEM.CPP:283 |
| `0x8008EF44` | `hdTtlive::hdTtlive()` | HDITEM.CPP:295 |
| `0x8008EF80` | `hdTtlive::SetTtlive(long)` | HDITEM.CPP:301 |
| `0x8008EFA4` | `hdDestSelect::hdDestSelect()` | HDITEM.CPP:308 |
| `0x8008EFD0` | `hdDestSelect::Start(long)` | HDITEM.CPP:312 |
| `0x8008F05C` | `hdDestSelect::Init(oxScreenManager*)` | HDITEM.CPP:327 |
| `0x8008F0F4` | `hdDestSelect::Hide()` | HDITEM.CPP:342 |
| `0x8008F138` | `hdDestSelect::ShowLevel(int)` | HDITEM.CPP:349 |
| `0x8008F26C` | `hdDestSelect::Update()` | HDITEM.CPP:390 |
| `0x8008F28C` | `hdTextOvl::hdTextOvl()` | HDITEM.CPP:397 |
| `0x8008F2C4` | `_._9hdTextOvl` | HDITEM.CPP:403 |
| `0x8008F2EC` | `hdTextOvl::SelfInit()` | HDITEM.CPP:413 |
| `0x8008F334` | `hdTextOvl::SetNumber(long)` | HDITEM.CPP:420 |
| `0x8008F360` | `hdAnimTextOvl::hdAnimTextOvl()` | HDITEM.CPP:431 |
| `0x8008F3B0` | `hdAnimTextOvl::SelfInit()` | HDITEM.CPP:436 |
| `0x8008F454` | `hdAnimTextOvl::SetPos(int, int)` | HDITEM.CPP:457 |
| `0x8008F508` | `hdAnimTextOvl::GoToMinPos()` | HDITEM.CPP:469 |
| `0x8008F550` | `hdAnimTextOvl::SetAnimInfo(int, int, int, int)` | HDITEM.CPP:479 |
| `0x8008F65C` | `hdAnimTextOvl::Reset()` | HDITEM.CPP:496 |
| `0x8008F664` | `hdAnimTextOvl::Update()` | HDITEM.CPP:501 |
| `0x8008F790` | `hdAnimTextOvl::Play()` | HDITEM.CPP:536 |
| `0x8008F79C` | `hdAnimTextOvl::SetPauseState(int, int)` | HDITEM.CPP:541 |
| `0x8008F81C` | `hdDragon::SetNum(int)` | HDITEM.CPP:582 |
| `0x8008F84C` | `hdDragon::hdDragon()` | HDITEM.CPP:588 |
| `0x8008F884` | `hdDragon::SelfInit()` | HDITEM.CPP:593 |
| `0x8008F8C4` | `hdDragon::SetGoldDragons(short)` | HDITEM.CPP:601 |
| `0x8008F93C` | `hdHits::Init(oxScreenManager*)` | HDITEM.CPP:620 |
| `0x8008FA2C` | `hdHits::Update()` | HDITEM.CPP:647 |
| `0x8008FBC4` | `hdHits::TriggerUpdate()` | HDITEM.CPP:674 |
| `0x8008FBEC` | `hdHits::IncrementHits()` | HDITEM.CPP:680 |
| `0x8008FD80` | `_._6hdHits` | HDITEM.CPP:733 |
| `0x8008FDB4` | `hdTally::hdTally()` | HDITEM.CPP:759 |
| `0x8008FE20` | `_._7hdTally` | HDITEM.CPP:766 |
| `0x8008FEC0` | `hdTally::Init(oxScreenManager*)` | HDITEM.CPP:771 |
| `0x800900A0` | `hdTally::DoScoreTally(char*, bool)` | HDITEM.CPP:794 |
| `0x8009013C` | `hdTally::UpdateCombo(bool)` | HDITEM.CPP:831 |
| `0x80090220` | `hdTally::UpdateFight(bool)` | HDITEM.CPP:867 |
| `0x80090308` | `hdTally::UpdateStyle(bool)` | HDITEM.CPP:902 |
| `0x8009041C` | `hdTally::UpdateGrade(bool)` | HDITEM.CPP:948 |
| `0x800905DC` | `hdTally::UpdateRdragon(bool)` | HDITEM.CPP:1013 |
| `0x80090718` | `hdTally::UpdateRdragonBonus(bool)` | HDITEM.CPP:1069 |
| `0x800907F0` | `hdTally::UpdateGdragon(bool)` | HDITEM.CPP:1106 |
| `0x80090914` | `hdTally::UpdateMovieBonus(bool)` | HDITEM.CPP:1144 |
| `0x800909D0` | `hdTally::DoDoneStuff()` | HDITEM.CPP:1176 |
| `0x80090AC0` | `hdTally::Update()` | HDITEM.CPP:1204 |
| `0x80090C58` | `hdTally::Start(int)` | HDITEM.CPP:1280 |
| `0x80090D2C` | `hdTally::Show(int)` | HDITEM.CPP:1306 |
| `0x80090DC4` | `hdHits::hdHits()` | HDITEM.CPP:1321 |
| `0x80090DD8` | `hdHits::SetVisible(int)` | HDITEM.CPP:1327 |
| `0x80090DFC` | `_._8hdDragon` | HDITEM.H:160 |
| `0x80090E24` | `_._13hdAnimTextOvl` | HDITEM.H:129 |
| `0x80090E4C` | `_._8hdTtlive` | HDITEM.H:35 |
| `0x80090E6C` | `xcColour1555::GetAlpha8() const` | XCCOLOUR.H:217 |
| `0x80090E8C` | `xcColour1555::GetBlue8() const` | XCCOLOUR.H:216 |
| `0x80090EB0` | `xcColour1555::GetGreen8() const` | XCCOLOUR.H:215 |
| `0x80090ED4` | `xcColour1555::GetRed8() const` | XCCOLOUR.H:214 |
| `0x80090EF4` | `GetPrimPosA(xcPrimObj*, short&, xcPrimObj*)` | OXOVL.CPP:12 |
| `0x80090F6C` | `SetPrimPosA(xcPrimObj*, short, short)` | OXOVL.CPP:35 |
| `0x80091050` | `oxOvl::oxOvl()` | OXOVL.CPP:98 |
| `0x80091064` | `_._5oxOvl` | OXOVL.CPP:103 |
| `0x80091098` | `oxOvl::Init(xcOverlay*)` | OXOVL.CPP:107 |
| `0x800910C8` | `oxOvl::SelfInit()` | OXOVL.CPP:119 |
| `0x800910D0` | `oxOvl::SetVisible(short)` | OXOVL.CPP:125 |
| `0x800910F8` | `oxOvl::IsVisible()` | OXOVL.CPP:130 |
| `0x8009110C` | `oxOvl::GetPrimPos(xcPrimObj*, short&, short&)` | OXOVL.CPP:135 |
| `0x80091134` | `oxOvl::SetPrimPos(xcPrimObj*, short, short)` | OXOVL.CPP:141 |
| `0x80091164` | `oxScreen::oxScreen()` | OXSCREEN.CPP:4 |
| `0x80091198` | `_._8oxScreen` | OXSCREEN.CPP:8 |
| `0x800911C0` | `xcReadFileLow(const char*, void**, unsigned long*)` | XCFILE.CPP:76 |
| `0x80091268` | `xcReadFileHigh(const char*, void**, unsigned long*)` | XCFILE.CPP:105 |
| `0x80091310` | `xcHash(const char*)` | XCHASH.CPP:11 |
| `0x8009134C` | `xcInventoryItem::FixDataPointers(unsigned long)` | XCINV.CPP:12 |
| `0x80091360` | `SortHash(void*, void*)` | XCINV.CPP:19 |
| `0x80091370` | `xcInventory::Sort()` | XCINV.CPP:27 |
| `0x800913B0` | `xcInventory::FixDataPointers(unsigned long)` | XCINV.CPP:34 |
| `0x80091420` | `xcInventory::FindItem(unsigned long)` | XCINV.CPP:44 |
| `0x80091468` | `xcSpriteLetter::Init(const xciSpriteLetter*)` | XCFONT.CPP:48 |
| `0x80091590` | `SortByLetterCode(void*, void*)` | XCFONT.CPP:78 |
| `0x800915A0` | `xcFont::xcFont(void*)` | XCFONT.CPP:92 |
| `0x800919BC` | `_._6xcFont` | XCFONT.CPP:204 |
| `0x80091A1C` | `FindItemInTable(unsigned long, const xcSpriteLetter*, int)` | XCFONT.CPP:212 |
| `0x80091AC8` | `xcFont::FindLetter(unsigned short) const` | XCFONT.CPP:254 |
| `0x80091B20` | `xcFont::ReloadData(void*)` | XCFONT.CPP:268 |
| `0x80091C64` | `xcFont::FindLetter(unsigned char) const` | XCFONT.CPP:319 |
| `0x80091C90` | `Wall::CheckWallIntersection(tagLVector&, const tagLVector&, long, long, long, int) const` | COLWALL.CPP:132 |
| `0x80091EAC` | `Wall::CheckWallCollision(const tagLVector&, const tagLVector&, long, long, long, int, long&, _RMVECT16&, tagLVector&) const` | COLWALL.CPP:194 |
| `0x80092144` | `Wall::IsCurb() const` | COLWALL.CPP:269 |
| `0x80092250` | `Wall::CheckWallBounds(const tagLVector&, long, long, long, int) const` | COLWALL.CPP:312 |
| `0x800923D8` | `Wall::Get(tagLVector&, N31) const` | COLWALL.CPP:387 |
| `0x800926BC` | `Floor::GetFloorHeight(const tagLVector&) const` | COLFLOOR.CPP:119 |
| `0x8009272C` | `Floor::GetFloorNormal(_RMVECT16&) const` | COLFLOOR.CPP:139 |
| `0x800927A0` | `Floor::CheckFloorBounds(const tagLVector&, long) const` | COLFLOOR.CPP:167 |
| `0x8009296C` | `Floor::GetRailingCorrection(tagLVector&, const tagLVector&) const` | COLFLOOR.CPP:189 |
| `0x80092B24` | `Floor::LedgePrototype(const tagLVector&, const tagLVector&, long, long, _RMVECT16&, tagLVector&) const` | COLFLOOR.CPP:244 |
| `0x80093244` | `Floor::BoundNumber() const` | COLFLOOR.CPP:340 |
| `0x80093278` | `Floor::Get(tagLVector&, N31) const` | COLFLOOR.CPP:369 |
| `0x8009361C` | `rsdLoad::Initialize(unsigned long)` | RSDLOAD.CPP:78 |
| `0x80093660` | `rsdLoad::Terminate()` | RSDLOAD.CPP:120 |
| `0x800936A8` | `rsdLoad::IsBusy()` | RSDLOAD.CPP:158 |
| `0x800936B4` | `rsdLoad::SetHighMemoryLimit(unsigned long)` | RSDLOAD.CPP:176 |
| `0x800936C0` | `(nw__7rsdLoadUi)` | RSDLOAD.CPP:197 |
| `0x800936F8` | `(double, long, __7rsdLoadPv)` | RSDLOAD.CPP:222 |
| `0x80093730` | `rsdLoad::rsdLoad(unsigned long, const unsigned char*, unsigned long, rsdLoadCallback*)` | RSDLOAD.CPP:252 |
| `0x800937F0` | `_._7rsdLoad` | RSDLOAD.CPP:326 |
| `0x80093818` | `rsdLoad::Destroy()` | RSDLOAD.CPP:346 |
| `0x800938A4` | `rsdLoad::TransferComplete()` | RSDLOAD.CPP:417 |
| `0x8009394C` | `xcVRAMAllocator::xcVRAMAllocator(const xcRectSint16&, unsigned long, unsigned long)` | XCVRAM.CPP:15 |
| `0x80093B6C` | `_._15xcVRAMAllocator` | XCVRAM.CPP:47 |
| `0x80093BE8` | `xcVRAMAllocator::FreeAllVRAM()` | XCVRAM.CPP:54 |
| `0x80093C84` | `xcVRAMAllocator::AllocCells(xcCellList*, unsigned long)` | XCVRAM.CPP:69 |
| `0x80093D18` | `xcVRAMAllocator::FreeCells(xcCellList*)` | XCVRAM.CPP:93 |
| `0x80093D50` | `xcCellList::AddTail(xcCellNode*)` | XCVRAM.CPP:108 |
| `0x80093D78` | `xcCellList::AppendListTail(xcCellNode*, xcCellNode*)` | XCVRAM.CPP:121 |
| `0x80093DA0` | `xcCellList::AppendListHead(xcCellNode*, xcCellNode*)` | XCVRAM.CPP:136 |
| `0x80093DB4` | `SquExpandData(unsigned char*, unsigned char*)` | EXPAND.CPP:67 |
| `0x80093E68` | `_gfrsEnterAmbiantSpace(Thing*, unsigned long, const char**)` | SWITCH.CPP:348 |
| `0x80093EEC` | `_gfPlayerDeathVol(Thing*, unsigned long, const char**)` | SWITCH.CPP:370 |
| `0x80093FF0` | `MakeThingDeathVolSound(unsigned long)` | SWITCH.CPP:424 |
| `0x8009406C` | `_gfEnemyObstDeathVol(Thing*, unsigned long, const char**)` | SWITCH.CPP:473 |
| `0x800940D4` | `_gfDirectorVol(Thing*, unsigned long, const char**)` | SWITCH.CPP:500 |
| `0x80094128` | `_gfGoToVol(Thing*, unsigned long, const char**)` | SWITCH.CPP:510 |
| `0x800941D4` | `_gfExitTest(Thing*, unsigned long, const char**)` | SWITCH.CPP:526 |
| `0x800941DC` | `_gfResetPlayer(Thing*, unsigned long, const char**)` | SWITCH.CPP:535 |
| `0x80094238` | `_gfSetDeathState(Thing*, unsigned long, const char**)` | SWITCH.CPP:546 |
| `0x800942B0` | `_gfBehaviorTrigger(Thing*, unsigned long, const char**)` | SWITCH.CPP:586 |
| `0x800942B8` | `_gfGateCleanupVol(Thing*, unsigned long, const char**)` | SWITCH.CPP:607 |
| `0x800943B4` | `gPurgeLoadGroups(void)` | SWITCH.CPP:740 |
| `0x80094418` | `gSyncLoadGroup(int)` | SWITCH.CPP:788 |
| `0x80094518` | `_gfAsyncLoadGroup(Thing*, unsigned long, const char**)` | SWITCH.CPP:819 |
| `0x80094684` | `_gfAsyncLoadNIS(Thing*, unsigned long, const char**)` | SWITCH.CPP:884 |
| `0x80094714` | `_gfAsyncLoadNISGOTO(Thing*, unsigned long, const char**)` | SWITCH.CPP:912 |
| `0x800947B0` | `gfUnloadLoadChar(Q22AI10ThingTypesT0)` | SWITCH.CPP:942 |
| `0x8009484C` | `UnloadHelper(_RTASK*)` | SWITCH.CPP:990 |
| `0x80094878` | `_gfCharModelLoad(Thing*, unsigned long, const char**)` | SWITCH.CPP:1005 |
| `0x80094964` | `_gfLevelComplete(Thing*, unsigned long, const char**)` | SWITCH.CPP:1096 |
| `0x80094B0C` | `_gfCheckpoint(Thing*, unsigned long, const char**)` | SWITCH.CPP:1173 |
| `0x80094BBC` | `_gfBossVol(Thing*, unsigned long, const char**)` | SWITCH.CPP:1223 |
| `0x80094BC4` | `_gfLoadDialog(Thing*, unsigned long, const char**)` | SWITCH.CPP:1253 |
| `0x80094C54` | `_gfPlayDialog(Thing*, unsigned long, const char**)` | SWITCH.CPP:1308 |
| `0x80094CE4` | `WDBSwitch::WDBSwitch()` | SWITCH.CPP:1379 |
| `0x80094D68` | `_._9WDBSwitch` | SWITCH.CPP:1392 |
| `0x80094E10` | `WDBSwitch::Setup(DBRoot*)` | SWITCH.CPP:1423 |
| `0x80095064` | `WDBSwitch::Bind()` | SWITCH.CPP:1512 |
| `0x80095204` | `WDBSwitch::Execute(Thing*)` | SWITCH.CPP:1588 |
| `0x8009525C` | `WDBSwitch::Reject(Thing*)` | SWITCH.CPP:1601 |
| `0x800952B4` | `WDBSphereSwitch::IsInside(const tagLVector&)` | SWITCH.CPP:1611 |
| `0x800952BC` | `WDBVolumeSwitch::IsInside(const tagLVector&)` | SWITCH.CPP:1618 |
| `0x80095384` | `WDBVolumeSwitch::SetVolume(DBVolume*)` | SWITCH.CPP:1632 |
| `0x80095404` | `_._15CharMgrCallback` | CHARMGR.HPP:83 |
| `0x80095438` | `CharMgrCallback::Callback()` | CHARMGR.HPP:82 |
| `0x80095444` | `_._15WDBSphereSwitch` | SWITCH.HPP:173 |
| `0x8009546C` | `_._15WDBVolumeSwitch` | SWITCH.HPP:154 |
| `0x80095494` | `ParticleSystem::Load(tReadChunk&, void**)` | PARTICLE.CPP:205 |
| `0x80095610` | `ParticleSystem::InitParticleInfoMemory()` | PARTICLE.CPP:312 |
| `0x800956D0` | `ParticleSystem::Unload()` | PARTICLE.CPP:346 |
| `0x80095740` | `ParticleSystem::UnloadLevel()` | PARTICLE.CPP:370 |
| `0x80095854` | `ParticleSystem::CommonParticles(int)` | PARTICLE.CPP:408 |
| `0x80095860` | `ParticleSystem::ParseData(tReadChunk&)` | PARTICLE.CPP:425 |
| `0x80095F78` | `ParticleSystem::AnalyzeMesh()` | PARTICLE.CPP:782 |
| `0x80096318` | `ParticleSystem::Find(unsigned long)` | PARTICLE.CPP:908 |
| `0x800963A4` | `ParticleSystem::SetParticleDirection(_RMVECT16*)` | PARTICLE.CPP:995 |
| `0x800963C4` | `ParticleSystem::ResetParticleDirection()` | PARTICLE.CPP:1010 |
| `0x800963EC` | `ParticleSystem::PurgeParticles()` | PARTICLE.CPP:1048 |
| `0x8009648C` | `ParticleSystem::ActiveParticles()` | PARTICLE.CPP:1082 |
| `0x800964C0` | `ParticleSystem::CreateParticles(const _RMVECT16&, ParticleStats*)` | PARTICLE.CPP:1125 |
| `0x80096698` | `ParticleSystem::InitParticles(const _RMVECT16&)` | PARTICLE.CPP:1321 |
| `0x80096EF0` | `ParticleSystem::Update()` | PARTICLE.CPP:1542 |
| `0x80097540` | `ParticleSystem::Display()` | PARTICLE.CPP:1737 |
| `0x80097B18` | `ParticleSystem::ParticleSystem()` | PARTICLE.CPP:1995 |
| `0x80097B98` | `_._14ParticleSystem` | PARTICLE.CPP:2024 |
| `0x80097C4C` | `ParticleInfo::ParticleInfo()` | PARTICLE.CPP:2053 |
| `0x80097C8C` | `_._12ParticleInfo` | PARTICLE.CPP:2070 |
| `0x80097CB4` | `ParticleStats::ParticleStats()` | PARTICLE.CPP:2082 |
| `0x80097CC8` | `_._13ParticleStats` | PARTICLE.CPP:2099 |
| `0x80097D1C` | `ParticleSystemMgr::ParticleSystemMgr(ParticleSystem*)` | PARTICLE.CPP:2108 |
| `0x80097D60` | `ParticleSystemMgr::ParticleSystemMgr()` | PARTICLE.CPP:2117 |
| `0x80097DA4` | `_._17ParticleSystemMgr` | PARTICLE.CPP:2126 |
| `0x80097E00` | `ParticleSystemMgr::InitMgr(ParticleSystem*)` | PARTICLE.CPP:2133 |
| `0x80097E1C` | `ParticleSystemMgr::CreateParticles(const _RMVECT16&, ParticleStats*)` | PARTICLE.CPP:2145 |
| `0x80097ECC` | `ParticleSystemMgr::SetParticleDirection(_RMVECT16*)` | PARTICLE.CPP:2170 |
| `0x80097F30` | `ParticleSystemMgr::ResetParticleDirection()` | PARTICLE.CPP:2179 |
| `0x80097F60` | `ParticleSystemMgr::Update()` | PARTICLE.CPP:2186 |
| `0x80097F90` | `ParticleSystemMgr::Display()` | PARTICLE.CPP:2192 |
| `0x80097FC0` | `ParticleSystemMgr::ActiveParticles()` | PARTICLE.CPP:2199 |
| `0x80097FF0` | `ParticleSystemMgr::PurgeParticles()` | PARTICLE.CPP:2205 |
| `0x80098020` | `SphereToCart(long, long, _RMVECT16*)` | PARTICLE.CPP:2379 |
| `0x800980E0` | `static_destroy(SPL_PARTICLE_SPEED_AVERAGE)` | PARTICLE.CPP:2530 |
| `0x8009814C` | `static_init(SPL_PARTICLE_SPEED_AVERAGE)` | PARTICLE.CPP:2530 |
| `0x800981CC` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8009821C` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x80098270` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x800982C0` | `GTEVXMatrix::FillRotZAfterSinCos()` | GTEMATRIX.HPP:163 |
| `0x800982F4` | `GTEVXMatrix::FillRotYAfterSinCos()` | GTEMATRIX.HPP:142 |
| `0x80098310` | `GTEVXMatrix::FillRotXAfterSinCos()` | GTEMATRIX.HPP:119 |
| `0x80098334` | `UVPrimData::Load(tReadChunk&, void**)` | UVDATA.CPP:81 |
| `0x8009842C` | `UVPrimData::FindUVPrimInfo(unsigned long)` | UVDATA.CPP:129 |
| `0x80098478` | `UVPrimData::Unload()` | UVDATA.CPP:154 |
| `0x800984E4` | `UVPrimData::Update(int, tPrimGeom*)` | UVDATA.CPP:174 |
| `0x800985A4` | `UVPrimData::UVPrimData()` | UVDATA.CPP:223 |
| `0x800985B0` | `_._10UVPrimData` | UVDATA.CPP:238 |
| `0x800985D8` | `UVPrimData::Init(int, int, int, int)` | UVDATA.CPP:251 |
| `0x80098604` | `UVPrimData::Update()` | UVDATA.CPP:287 |
| `0x8009864C` | `CBVPrimData::Load(tReadChunk&, void**)` | UVDATA.CPP:339 |
| `0x80098744` | `CBVPrimData::FindCBVPrimInfo(unsigned long)` | UVDATA.CPP:387 |
| `0x80098790` | `CBVPrimData::Unload()` | UVDATA.CPP:412 |
| `0x800987FC` | `CBVPrimData::Update(int, tPrimGeom*)` | UVDATA.CPP:434 |
| `0x8009890C` | `CBVPrimData::CBVPrimData()` | UVDATA.CPP:479 |
| `0x8009891C` | `_._11CBVPrimData` | UVDATA.CPP:495 |
| `0x80098960` | `CBVPrimData::Init(int, int, unsigned long, int, int, int)` | UVDATA.CPP:517 |
| `0x80098B2C` | `CBVPrimData::Release()` | UVDATA.CPP:607 |
| `0x80098B4C` | `CBVPrimData::FreeColourInfo()` | UVDATA.CPP:621 |
| `0x80098BE0` | `CBVPrimData::Update()` | UVDATA.CPP:644 |
| `0x80098F00` | `ColourInfo::Init(unsigned long, unsigned long, int)` | UVDATA.CPP:791 |
| `0x80099044` | `ColourInfo::Reset(int)` | UVDATA.CPP:812 |
| `0x80099080` | `ColourInfo::Update()` | UVDATA.CPP:831 |
| `0x800990F4` | `ColourInfo::GetColour()` | UVDATA.CPP:847 |
| `0x8009911C` | `ConvertEndian(unsigned long)` | STREAM.CPP:367 |
| `0x80099148` | `StreamHeaderNode::StreamHeaderNode(unsigned long, unsigned long)` | STREAM.CPP:384 |
| `0x80099160` | `_._16StreamHeaderNode` | STREAM.CPP:393 |
| `0x80099188` | `Stream::Stream()` | STREAM.CPP:397 |
| `0x800991A0` | `Stream::Open(const char*, int)` | STREAM.CPP:404 |
| `0x800993E8` | `Stream::Close()` | STREAM.CPP:491 |
| `0x80099490` | `_._6Stream` | STREAM.CPP:510 |
| `0x800994D4` | `Stream::LoadPermChunk()` | STREAM.CPP:515 |
| `0x80099570` | `Stream::HandleTPGChunk()` | STREAM.CPP:534 |
| `0x80099648` | `myGeoLoaderCallback(tEntity*)` | STREAM.CPP:623 |
| `0x800996C0` | `myETreeLoaderCallback(tEntity*)` | STREAM.CPP:678 |
| `0x8009976C` | `mySTreeLoaderCallback(tEntity*)` | STREAM.CPP:704 |
| `0x800998EC` | `AnimLoaderCallback(tEntity*)` | STREAM.CPP:775 |
| `0x80099988` | `CompAnimLoaderCallback(tEntity*)` | STREAM.CPP:799 |
| `0x80099A24` | `Stream::HandleRCBChunk()` | STREAM.CPP:823 |
| `0x80099EA4` | `Stream::HandlePCBChunk()` | STREAM.CPP:934 |
| `0x8009A318` | `Stream::HandleWDBChunk()` | STREAM.CPP:1042 |
| `0x8009A3EC` | `Stream::HandleLLNChunk()` | STREAM.CPP:1064 |
| `0x8009A5A4` | `Stream::Read()` | STREAM.CPP:1128 |
| `0x8009A74C` | `Stream::AsyncLoad(unsigned long, long, void*, int, int)` | STREAM.CPP:1255 |
| `0x8009A860` | `Stream::Load(void*, long)` | STREAM.CPP:1287 |
| `0x8009A900` | `Stream::_TPGLoadCallback(long, long, long)` | STREAM.CPP:1301 |
| `0x8009A98C` | `Stream::_BLKLoadCallback(long, long, long)` | STREAM.CPP:1325 |
| `0x8009AA74` | `Stream::LoadPetal(long)` | STREAM.CPP:1363 |
| `0x8009ABF4` | `MemoryStats(const char*, char*, unsigned long)` | STREAM.CPP:1588 |
| `0x8009ABFC` | `_._18AnimStructureBasic` | MODEL.HPP:483 |
| `0x8009AC24` | `_._7tLoader` | TLOADER.HPP:76 |
| `0x8009AC58` | `PWEffect::InitPWorldEffects(DBPoint*)` | PWEFFECT.CPP:80 |
| `0x8009B238` | `PWEffect::AddFragTemps(unsigned long, unsigned long, unsigned long)` | PWEFFECT.CPP:368 |
| `0x8009B3C4` | `PWEffect::CreateSound()` | PWEFFECT.CPP:434 |
| `0x8009B424` | `PWEffect::UpdateSound()` | PWEFFECT.CPP:461 |
| `0x8009B454` | `PWEffect::ReleaseSound()` | PWEFFECT.CPP:469 |
| `0x8009B4A0` | `PWEffect::PutBackEffect()` | PWEFFECT.CPP:490 |
| `0x8009B534` | `PWEffect::Unload()` | PWEFFECT.CPP:528 |
| `0x8009B554` | `PWEffect::Create(int)` | PWEFFECT.CPP:542 |
| `0x8009B618` | `PWEffect::Create()` | PWEFFECT.CPP:579 |
| `0x8009B6EC` | `PWEffect::PWEffect()` | PWEFFECT.CPP:625 |
| `0x8009B748` | `_._8PWEffect` | PWEFFECT.CPP:651 |
| `0x8009B7FC` | `PWEffect::Purge()` | PWEFFECT.CPP:680 |
| `0x8009B860` | `PWEffect::Update()` | PWEFFECT.CPP:699 |
| `0x8009B994` | `PWEffect::Display(int)` | PWEFFECT.CPP:745 |
| `0x8009BA80` | `FPWEffect::FPWEffect()` | PWEFFECT.CPP:802 |
| `0x8009BACC` | `FPWEffect::Create(int)` | PWEFFECT.CPP:822 |
| `0x8009BB90` | `FPWEffect::Create()` | PWEFFECT.CPP:860 |
| `0x8009BC38` | `FPWEffect::Create2(unsigned long, tagLVector*, _RMVECT16*, int, int)` | PWEFFECT.CPP:911 |
| `0x8009BD60` | `FPWEffect::Create2(tagLVector*, _RMVECT16*, int)` | PWEFFECT.CPP:966 |
| `0x8009BEE0` | `PWEffect::IsDone(int&)` | PWEFFECT.CPP:1035 |
| `0x8009BF24` | `FPWEffect::SetMentor()` | PWEFFECT.CPP:1053 |
| `0x8009C004` | `FPWEffect::Update()` | PWEFFECT.CPP:1092 |
| `0x8009C3D8` | `FPWEffect::Display(int)` | PWEFFECT.CPP:1283 |
| `0x8009C52C` | `_._9FPWEffect` | PWEFFECT.CPP:1350 |
| `0x8009C554` | `static_destroy(InitPWorldEffects__8PWEffectP7DBPoint)` | PWEFFECT.CPP:1352 |
| `0x8009C594` | `static_init(InitPWorldEffects__8PWEffectP7DBPoint)` | PWEFFECT.CPP:1352 |
| `0x8009C5D8` | `_._6ccList` | CCLIST.HPP:237 |
| `0x8009C628` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x8009C67C` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x8009C6CC` | `ScaleData::Load(tReadChunk&, void**)` | SCALEDAT.CPP:56 |
| `0x8009C918` | `ScaleData::FindScaleInfo(int)` | SCALEDAT.CPP:179 |
| `0x8009C9B8` | `ScaleData::ScaleECallback(tEJoint*, int)` | SCALEDAT.CPP:217 |
| `0x8009CA4C` | `ScaleData::ScaleMCallback(tMJoint*, int)` | SCALEDAT.CPP:261 |
| `0x8009CAEC` | `_._9ScaleData` | SCALEDAT.CPP:296 |
| `0x8009CB40` | `ScaleData::Unload()` | SCALEDAT.CPP:312 |
| `0x8009CBB4` | `ScaleData::UnloadLevel()` | SCALEDAT.CPP:340 |
| `0x8009CC20` | `ScaleData::CommonScaleData(int)` | SCALEDAT.CPP:365 |
| `0x8009CC2C` | `ScaleData::SetFrame(int)` | SCALEDAT.CPP:379 |
| `0x8009CC34` | `ScaleData::GetScale(_RMVECT16*, ScaleKeyFrames*)` | SCALEDAT.CPP:398 |
| `0x8009CE34` | `PaletteData::PaletteData()` | PALDATA.CPP:78 |
| `0x8009CE44` | `_._11PaletteData` | PALDATA.CPP:94 |
| `0x8009CEA4` | `PaletteData::Load(tReadChunk&, void**)` | PALDATA.CPP:117 |
| `0x8009CFEC` | `PaletteData::FindPaletteInfo(unsigned long)` | PALDATA.CPP:186 |
| `0x8009D038` | `PaletteData::Clone()` | PALDATA.CPP:209 |
| `0x8009D0F4` | `PaletteData::SetupClut(ComEffect*, int, unsigned long)` | PALDATA.CPP:254 |
| `0x8009D134` | `PaletteData::Unload()` | PALDATA.CPP:270 |
| `0x8009D1A0` | `PaletteData::InitPalette()` | PALDATA.CPP:291 |
| `0x8009D1B8` | `PaletteData::NextFrame()` | PALDATA.CPP:313 |
| `0x8009D208` | `PaletteData::Update()` | PALDATA.CPP:338 |
| `0x8009D2E0` | `PaletteData::TransferVram()` | PALDATA.CPP:498 |
| `0x8009D338` | `rTickerDifference` | RTIMEF.C:36 |
| `0x8009D358` | `rmMag2(long, long)` | MAG2.CPP:19 |
| `0x8009D3DC` | `t2PointMatrixCamera::t2PointMatrixCamera()` | T2POINTCAM.CPP:26 |
| `0x8009D438` | `_._19t2PointMatrixCamera` | T2POINTCAM.CPP:33 |
| `0x8009D460` | `t2PointMatrixCamera::SetTarget(_RMVECT16*)` | T2POINTCAM.CPP:38 |
| `0x8009D480` | `t2PointMatrixCamera::SetPosition(_RMVECT16*)` | T2POINTCAM.CPP:43 |
| `0x8009D4A0` | `t2PointMatrixCamera::SetTwist(unsigned short)` | T2POINTCAM.CPP:48 |
| `0x8009D4A8` | `t2PointMatrixCamera::GetTarget(_RMVECT16*)` | T2POINTCAM.CPP:53 |
| `0x8009D4C8` | `t2PointMatrixCamera::GetPosition(_RMVECT16*)` | T2POINTCAM.CPP:58 |
| `0x8009D4E8` | `t2PointMatrixCamera::GetTwist()` | T2POINTCAM.CPP:63 |
| `0x8009D4F4` | `t2PointMatrixCamera::UpdateMatrix()` | T2POINTCAM.CPP:68 |
| `0x8009D5E8` | `tMatrixCamera::tMatrixCamera(MATRIX*)` | TMATRIXCAM.CPP:28 |
| `0x8009D648` | `_._13tMatrixCamera` | TMATRIXCAM.CPP:40 |
| `0x8009D670` | `tMatrixCamera::SetCameraMatrix(MATRIX*)` | TMATRIXCAM.CPP:45 |
| `0x8009D698` | `tMatrixCamera::GetCameraMatrix()` | TMATRIXCAM.CPP:50 |
| `0x8009D6A0` | `tMatrixCamera::UpdateMatrix()` | TMATRIXCAM.CPP:55 |
| `0x8009D6C8` | `tMatrixCamera::GetPosition(_RMVECT16*)` | TMATRIXCAM.CPP:70 |
| `0x8009D6EC` | `rmMag3(long, long, long)` | MAG3.CPP:17 |
| `0x8009D798` | `t2PointCamFlip::Update()` | 2PTCAMFLIP.CPP:16 |
| `0x8009D9A8` | `t2PointCamFlip::Attach(t2PointMatrixCamera*, tParamAnim*)` | 2PTCAMFLIP.CPP:79 |
| `0x8009DA38` | `t2PointCamFlip::GetEntityType()` | 2PTCAMFLIP.HPP:24 |
| `0x8009DA44` | `_._14t2PointCamFlip` | 2PTCAMFLIP.HPP:32 |
| `0x8009DA64` | `rmV3Normalize(_RMVECT16*, _RMVECT16*)` | VECT3D.CPP:22 |
| `0x8009DAE0` | `rmATan216` | ATAN216.C:12 |
| `0x8009DBB8` | `EvalCubic(long*, long*, long, long, long)` | FXP.CPP:81 |
| `0x8009DD1C` | `IsPointInFieldOf(const tagLVector&, const tagLVector&, long, long, long)` | FXP.CPP:147 |
| `0x8009DD88` | `ClipAngle(long&)` | FXP.CPP:164 |
| `0x8009DDE8` | `IsAngleInFieldOf(long, long, long, long)` | FXP.CPP:184 |
| `0x8009DEF8` | `rCDWrite` | RCDWRITE.C:14 |
| `0x8009DF70` | `tFrameList::tFrameList()` | VERTANIM.CPP:94 |
| `0x8009DFA4` | `_._10tFrameList` | VERTANIM.CPP:104 |
| `0x8009DFCC` | `tFrameList::MakePuppet()` | VERTANIM.CPP:112 |
| `0x8009E02C` | `tFrameList::GetNumFrames()` | VERTANIM.CPP:120 |
| `0x8009E038` | `tFrameList::GetFrame(int)` | VERTANIM.CPP:125 |
| `0x8009E050` | `tFrameList::DeleteFrames()` | VERTANIM.CPP:131 |
| `0x8009E090` | `tFrameList::SetNumFrames(int)` | VERTANIM.CPP:138 |
| `0x8009E098` | `tFrameList::SetFrame(int, SVECTOR*)` | VERTANIM.CPP:159 |
| `0x8009E0AC` | `tVertexFlip::tVertexFlip()` | VERTANIM.CPP:169 |
| `0x8009E0E4` | `_._11tVertexFlip` | VERTANIM.CPP:174 |
| `0x8009E10C` | `tVertexFlip::Reset()` | VERTANIM.CPP:179 |
| `0x8009E15C` | `tVertexFlip::Update()` | VERTANIM.CPP:187 |
| `0x8009E1C8` | `tVertexFlip::GetVertexFrame(int)` | VERTANIM.CPP:193 |
| `0x8009E200` | `tVertexFlip::GetEntityType()` | VERTANIM.HPP:124 |
| `0x8009E20C` | `tFrameList::GetEntityType()` | VERTANIM.HPP:93 |
| `0x8009E218` | `P3DClipCode(unsigned long, unsigned long)` | FRUSTRUM.CPP:37 |
| `0x8009E27C` | `P3DClipCodeSphere(tSphere*)` | FRUSTRUM.CPP:442 |
| `0x8009F760` | `GTEVXMatrix::FillRotScale(const _RMVECT16&)` | GTEMATRIX.CPP:114 |
| `0x8009F790` | `GTEVXMatrix::FillRotScale(long, long, long)` | GTEMATRIX.CPP:119 |
| `0x8009F7C8` | `GTEVXMatrix::FillRotScale(long)` | GTEMATRIX.CPP:135 |
| `0x8009F7F4` | `GTERTMatrix::GetMatrix(MATRIX*)` | GTEMATRIX.CPP:210 |
| `0x8009F83C` | `GTERTMatrix::FillMatrix(const MATRIX&)` | GTEMATRIX.CPP:223 |
| `0x8009F884` | `GTELDMatrix::GetRot(MATRIX*)` | GTEMATRIX.CPP:309 |
| `0x8009F8B4` | `GTELDMatrix::FillRot(const MATRIX&)` | GTEMATRIX.CPP:321 |
| `0x8009F8E4` | `GTELDMatrix::FillRotZero()` | GTEMATRIX.CPP:370 |
| `0x8009F900` | `GTELCMatrix::GetRot(MATRIX*)` | GTEMATRIX.CPP:402 |
| `0x8009F930` | `GTELCMatrix::FillRot(const MATRIX&)` | GTEMATRIX.CPP:413 |
| `0x8009F960` | `GTELCMatrix::FillRotZero()` | GTEMATRIX.CPP:471 |
| `0x8009F97C` | `PopMatrixAndLights(void)` | PORTLITE.CPP:111 |
| `0x8009F9C0` | `PushMatrixAndLights(void)` | PORTLITE.CPP:122 |
| `0x8009FA08` | `MultMatrixAndLights(MATRIX*)` | PORTLITE.CPP:133 |
| `0x8009FA38` | `RotMatrixXYZAndLights(unsigned short, unsigned short, unsigned short)` | PORTLITE.CPP:140 |
| `0x8009FAFC` | `RotMatrixXAndLights(unsigned short)` | PORTLITE.CPP:167 |
| `0x8009FB44` | `RotMatrixYAndLights(unsigned short)` | PORTLITE.CPP:178 |
| `0x8009FB8C` | `RotMatrixZAndLights(unsigned short)` | PORTLITE.CPP:189 |
| `0x8009FBD4` | `RotMatrixYZXAndLights(unsigned short, unsigned short, unsigned short)` | PORTLITE.CPP:200 |
| `0x8009FC98` | `GTEVXMatrix::FillRotZAfterSinCos()` | GTEMATRIX.HPP:163 |
| `0x8009FCCC` | `GTEVXMatrix::FillRotYAfterSinCos()` | GTEMATRIX.HPP:142 |
| `0x8009FCE8` | `GTEVXMatrix::FillRotXAfterSinCos()` | GTEMATRIX.HPP:119 |
| `0x8009FD3C` | `RenderQueue::RenderQueue(unsigned long)` | RQUEUE.CPP:28 |
| `0x8009FDAC` | `RenderQueue::WaitForLayer(unsigned long)` | RQUEUE.CPP:52 |
| `0x8009FE64` | `RenderQueue::QueueLayer(unsigned long)` | RQUEUE.CPP:81 |
| `0x8009FF7C` | `RenderQueue::QueueSwap()` | RQUEUE.CPP:131 |
| `0x800A003C` | `DSCallback(...)` | RQUEUE.CPP:166 |
| `0x800A0140` | `VSCallback(...)` | RQUEUE.CPP:214 |
| `0x800A0288` | `tLayer::tLayer(unsigned long, unsigned long)` | TLAYER.CPP:52 |
| `0x800A031C` | `_._6tLayer` | TLAYER.CPP:67 |
| `0x800A0370` | `tLayer::Draw()` | TLAYER.CPP:72 |
| `0x800A039C` | `tLayer::Flip()` | TLAYER.CPP:77 |
| `0x800A03A4` | `tLayer::FlipToDrawOT()` | TLAYER.CPP:82 |
| `0x800A03AC` | `tLayer::Check()` | TLAYER.CPP:96 |
| `0x800A03B8` | `tLayer::Start()` | TLAYER.CPP:101 |
| `0x800A0428` | `tLayer::End()` | TLAYER.CPP:114 |
| `0x800A04C0` | `tLayer::Free()` | TLAYER.CPP:131 |
| `0x800A04D8` | `tDoubleLayer::tDoubleLayer(unsigned long, unsigned long)` | TLAYER.CPP:147 |
| `0x800A053C` | `_._12tDoubleLayer` | TLAYER.CPP:155 |
| `0x800A0588` | `tDoubleLayer::Draw()` | TLAYER.CPP:160 |
| `0x800A05C4` | `tDoubleLayer::Flip()` | TLAYER.CPP:165 |
| `0x800A05E8` | `tDoubleLayer::FlipToDrawOT()` | TLAYER.CPP:171 |
| `0x800A05FC` | `tDoubleLayer::Check()` | TLAYER.CPP:177 |
| `0x800A0608` | `tDoubleLayer::Start()` | TLAYER.CPP:182 |
| `0x800A068C` | `tDoubleLayer::End()` | TLAYER.CPP:196 |
| `0x800A06F0` | `tDoubleLayer::Free()` | TLAYER.CPP:207 |
| `0x800A0728` | `get_first_ot_prim(void*)` | TLAYER.CPP:227 |
| `0x800A0750` | `get_next_ot_prim(unsigned long*)` | TLAYER.CPP:240 |
| `0x800A0778` | `DrawFrame(unsigned long*)` | TLAYER.CPP:280 |
| `0x800A089C` | `tLayer::ScaleOT()` | TLAYER.CPP:318 |
| `0x800A0C3C` | `tLayer::DumpOT()` | TLAYER.CPP:523 |
| `0x800A0E14` | `RP_ZCullGClip(tGeometry*)` | RPZCULL.CPP:308 |
| `0x800A14C4` | `_._9tPrimGeom` | TPRIMGEO.CPP:70 |
| `0x800A1548` | `tPrimGeom::Clone()` | TPRIMGEO.CPP:81 |
| `0x800A1860` | `tPrimGeom::Display()` | TPRIMGEO.CPP:158 |
| `0x800A1888` | `tPrimGeom::GetGeoType()` | TPRIMGEO.HPP:55 |
| `0x800A1894` | `tPrimGeom::GetEntityType()` | TPRIMGEO.HPP:25 |
| `0x800A18A0` | `tGeometry::Clone()` | TGEOMTRY.INL:57 |
| `0x800A18A8` | `tGeometry::GetVertexList()` | TGEOMTRY.INL:42 |
| `0x800A18B4` | `tGeometry::SetVertexList(SVECTOR*)` | TGEOMTRY.INL:37 |
| `0x800A18BC` | `tGeometry::GetBoundingSphere()` | TGEOMTRY.INL:32 |
| `0x800A18C4` | `tGeometry::GetBoundingBox()` | TGEOMTRY.INL:27 |
| `0x800A18CC` | `_._9tGeometry` | TGEOMTRY.INL:23 |
| `0x800A18F4` | `RP_ZCullGMFog(tGeometry*)` | RPZFOG.CPP:646 |
| `0x800A1E18` | `CSound::CSound()` | BASESND.CPP:31 |
| `0x800A1E5C` | `_._6CSound` | BASESND.CPP:54 |
| `0x800A1EAC` | `(nw__6CSoundUi)` | BASESND.CPP:74 |
| `0x800A1F00` | `(double, long, __6CSoundPv)` | BASESND.CPP:101 |
| `0x800A1F40` | `CSound::Load(const char*)` | BASESND.CPP:127 |
| `0x800A1F48` | `CSound::Initialize(const tagLVector*)` | BASESND.CPP:152 |
| `0x800A1F54` | `CSound::Release()` | BASESND.CPP:174 |
| `0x800A1FAC` | `CSound::GetPosPtr() const` | BASESND.CPP:202 |
| `0x800A1FB8` | `CSound::BeginPersistent(unsigned char, CGenericPersistentSound**)` | BASESND.CPP:229 |
| `0x800A2038` | `CSound::EndPersistent(CGenericPersistentSound**)` | BASESND.CPP:275 |
| `0x800A2088` | `CSound::PlayTransient(unsigned short, unsigned long, unsigned short)` | BASESND.CPP:309 |
| `0x800A2164` | `CSound::PlayTransientStereo(unsigned short, unsigned short)` | BASESND.CPP:360 |
| `0x800A23A8` | `AnimLight::CalcLight(long&, N21)` | LIGHTS.CPP:425 |
| `0x800A24F8` | `LightingClass::InternalOpen()` | LIGHTS.CPP:455 |
| `0x800A2538` | `LightingClass::AnalyzeShpere(DBPoint*)` | LIGHTS.CPP:689 |
| `0x800A2720` | `LightingClass::LightingClass()` | LIGHTS.CPP:782 |
| `0x800A27A4` | `_._13LightingClass` | LIGHTS.CPP:790 |
| `0x800A2864` | `LightingClass::Reset()` | LIGHTS.CPP:807 |
| `0x800A2928` | `LightingClass::AllocHLight(unsigned long, unsigned long, long, long, long)` | LIGHTS.CPP:851 |
| `0x800A2A0C` | `LightingClass::DeallocHLight(long)` | LIGHTS.CPP:881 |
| `0x800A2A88` | `LightingClass::AddLightToPort(long, _RMVECT16*, unsigned long)` | LIGHTS.CPP:901 |
| `0x800A2AEC` | `LightingClass::RemoveLightFromPort(long)` | LIGHTS.CPP:915 |
| `0x800A2B44` | `LightingClass::SetupHLight(long, _RMVECT16*, unsigned long)` | LIGHTS.CPP:924 |
| `0x800A2B98` | `LightingClass::SetHLightToOriginal(long)` | LIGHTS.CPP:945 |
| `0x800A2BFC` | `LightingClass::SetupLighting()` | LIGHTS.CPP:957 |
| `0x800A2D8C` | `LightingClass::ClampWithinRGBLimit(unsigned long*, N21)` | LIGHTS.CPP:1013 |
| `0x800A2DDC` | `LightingClass::ClampWithinNormalLimit(long*, N21)` | LIGHTS.CPP:1023 |
| `0x800A2E5C` | `LightingClass::SetupStageAttributes(DBVolume*)` | LIGHTS.CPP:1040 |
| `0x800A3110` | `LightingClass::DoModelLighting(Thing*)` | LIGHTS.CPP:1124 |
| `0x800A314C` | `LightingClass::FindAmbientVolumes(Thing*)` | LIGHTS.CPP:1131 |
| `0x800A34A0` | `computeLightDir(_RMVECT16*)` | LIGHTS.CPP:1305 |
| `0x800A3550` | `LightingClass::FindHardwareVolumes(Thing*)` | LIGHTS.CPP:1344 |
| `0x800A39E8` | `LightAnchor::LightAnchor()` | LIGHTS.CPP:1543 |
| `0x800A3A34` | `_._11LightAnchor` | LIGHTS.CPP:1554 |
| `0x800A3B10` | `LightAnchor::SetupLightMemory()` | LIGHTS.CPP:1565 |
| `0x800A3B98` | `LightAnchor::AddColourVolume(DBVolume*)` | LIGHTS.CPP:1573 |
| `0x800A3E08` | `LightAnchor::AddHardwareLightVolume(DBVolume*)` | LIGHTS.CPP:1626 |
| `0x800A400C` | `LightAnchor::SetupLight(DBLight*, DBSphere*)` | LIGHTS.CPP:1689 |
| `0x800A4194` | `LightAnchor::AddRadialHardwareLight(DBSphere*, int)` | LIGHTS.CPP:1773 |
| `0x800A4264` | `AmbientLight::AmbientLight()` | LIGHTS.CPP:1854 |
| `0x800A4298` | `_._12AmbientLight` | LIGHTS.CPP:1867 |
| `0x800A42CC` | `AmbientLight::SetToWorldAmbient()` | LIGHTS.CPP:1871 |
| `0x800A42E4` | `AmbientLight::SetDesired(unsigned long)` | LIGHTS.CPP:1891 |
| `0x800A42EC` | `AmbientLight::SetPortToLight()` | LIGHTS.CPP:1932 |
| `0x800A4328` | `HardwareLight::HardwareLight()` | LIGHTS.CPP:1943 |
| `0x800A4350` | `_._13HardwareLight` | LIGHTS.CPP:1950 |
| `0x800A4384` | `HardwareLight::SetLight(unsigned long, _RMVECT16*)` | LIGHTS.CPP:1954 |
| `0x800A43A4` | `_._7DBLight` | LIGHTS.HPP:185 |
| `0x800A43D8` | `_._14DBHLightVolume` | LIGHTS.HPP:163 |
| `0x800A4400` | `_._14DBColourVolume` | LIGHTS.HPP:152 |
| `0x800A4428` | `_._6DBRoot` | DATABASE.HPP:196 |
| `0x800A4450` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x800A44A4` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x800A44F4` | `Swap(NodeAttribs&, NodeAttribs&)` | PATH.CPP:71 |
| `0x800A4530` | `(as__11NodeAttribsRC11NodeAttribs)` | PATH.CPP:107 |
| `0x800A45D8` | `NodeAttribs::Init(DBPoint*)` | PATH.CPP:127 |
| `0x800A46F4` | `NodeAttribs::GetAttrib(int)` | PATH.CPP:166 |
| `0x800A4760` | `Path::Flip()` | PATH.CPP:182 |
| `0x800A4894` | `Path::Draw()` | PATH.CPP:198 |
| `0x800A4958` | `LinearPath::Subdivide(long)` | PATH.CPP:224 |
| `0x800A4D9C` | `LinearPath::Init(const DBPath*)` | PATH.CPP:276 |
| `0x800A4F8C` | `LinearPath::Init(const DBLine*)` | PATH.CPP:299 |
| `0x800A513C` | `LinearPath::Move(long)` | PATH.CPP:319 |
| `0x800A54BC` | `SplinePath::Subdivide(long)` | PATH.CPP:391 |
| `0x800A54C4` | `SplinePath::CalcCMRCoefficiants(long&, N31llll)` | PATH.CPP:398 |
| `0x800A556C` | `SplinePath::Init(const DBPath*)` | PATH.CPP:417 |
| `0x800A57BC` | `SplinePath::Init(const DBLine*)` | PATH.CPP:442 |
| `0x800A59CC` | `SplinePath::Move(long)` | PATH.CPP:464 |
| `0x800A5EDC` | `_._10SubDivNode` | PATH.CPP:67 |
| `0x800A5F4C` | `SplinePath::EndOfPath()` | PATH.HPP:210 |
| `0x800A5F64` | `SplinePath::Reset()` | PATH.HPP:204 |
| `0x800A5F94` | `_._10SplinePath` | PATH.HPP:187 |
| `0x800A6078` | `LinearPath::EndOfPath()` | PATH.HPP:162 |
| `0x800A6090` | `LinearPath::Reset()` | PATH.HPP:155 |
| `0x800A60C4` | `_._10LinearPath` | PATH.HPP:151 |
| `0x800A61A8` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x800A61FC` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x800A624C` | `_._4Path` | PATH.HPP:108 |
| `0x800A6330` | `Untouchable::Untouchable(const tagLVector*, unsigned short)` | UNTOUCH.CPP:99 |
| `0x800A6380` | `_._11Untouchable` | UNTOUCH.CPP:107 |
| `0x800A63DC` | `Untouchable::AnalyzeMesh(DBRoot*)` | UNTOUCH.CPP:117 |
| `0x800A64C8` | `Untouchable::CreateModel(const char*)` | UNTOUCH.CPP:140 |
| `0x800A64DC` | `Untouchable::DeleteModel()` | UNTOUCH.CPP:148 |
| `0x800A64F0` | `Untouchable::Reset()` | UNTOUCH.CPP:154 |
| `0x800A6540` | `Untouchable::Think()` | UNTOUCH.CPP:169 |
| `0x800A6604` | `Untouchable::Draw()` | UNTOUCH.CPP:203 |
| `0x800A6684` | `Untouchable::UpdatePosition()` | UNTOUCH.CPP:221 |
| `0x800A668C` | `Untouchable::HandlePickupCollision(Pickup*)` | UNTOUCH.CPP:226 |
| `0x800A6694` | `Untouchable::HandleHumanoidCollision(Humanoid*)` | UNTOUCH.CPP:232 |
| `0x800A68FC` | `Untouchable::CreateSound()` | UNTOUCH.CPP:328 |
| `0x800A695C` | `Untouchable::UpdateSound()` | UNTOUCH.CPP:356 |
| `0x800A698C` | `Untouchable::ReleaseSound()` | UNTOUCH.CPP:364 |
| `0x800A69D8` | `SubZoneVolume::SubZoneVolume(DBVolume*)` | ACTIVEZN.CPP:169 |
| `0x800A6A48` | `_._13SubZoneVolume` | ACTIVEZN.CPP:176 |
| `0x800A6A70` | `SubZoneVolume::IsInSubZoneVolume(Thing*)` | ACTIVEZN.CPP:183 |
| `0x800A6ADC` | `SubZoneVolume::Draw(const _RMVECT16&)` | ACTIVEZN.CPP:199 |
| `0x800A6B40` | `ActiveZone::GetActiveZoneCenterPoint()` | ACTIVEZN.CPP:217 |
| `0x800A6BC4` | `ActiveZone::ActiveZone(DBVolume*, unsigned long)` | ACTIVEZN.CPP:228 |
| `0x800A6D00` | `_._10ActiveZone` | ACTIVEZN.CPP:380 |
| `0x800A6DA4` | `ActiveZone::AddHumanoidToOverlordMembers(Humanoid*)` | ACTIVEZN.CPP:390 |
| `0x800A6DEC` | `ActiveZone::RemoveHumanoidFromOverlordMembers(Humanoid*)` | ACTIVEZN.CPP:417 |
| `0x800A6E30` | `ActiveZone::GetNumberOfThinkingMembers()` | ACTIVEZN.CPP:433 |
| `0x800A6E7C` | `ActiveZone::AllowedToMoveIn(Humanoid*)` | ACTIVEZN.CPP:456 |
| `0x800A6F40` | `ActiveZone::AddLinearPath(LinearPath&)` | ACTIVEZN.CPP:505 |
| `0x800A6F6C` | `ActiveZone::AddSubZoneVolume(SubZoneVolume&)` | ACTIVEZN.CPP:512 |
| `0x800A6F98` | `ActiveZone::FindFirstValidPath(Humanoid*)` | ACTIVEZN.CPP:590 |
| `0x800A701C` | `ActiveZone::IsInActiveZone(Thing*)` | ACTIVEZN.CPP:680 |
| `0x800A7040` | `ActiveZone::DoAICheck(LinearPath*, long, Humanoid*)` | ACTIVEZN.CPP:999 |
| `0x800A76AC` | `ActiveZone::IsPathNodeTerminator(LinearPath*, long)` | ACTIVEZN.CPP:1318 |
| `0x800A7718` | `ActiveZone::AllowBreakoffOfDestinationNode(LinearPath*, long)` | ACTIVEZN.CPP:1352 |
| `0x800A7784` | `ActiveZone::DoActionsAtNode(LinearPath*, long, Humanoid*)` | ACTIVEZN.CPP:1378 |
| `0x800A7A8C` | `_._6ccList` | CCLIST.HPP:237 |
| `0x800A7ADC` | `ccMinList::Purge()` | CCLIST.HPP:216 |
| `0x800A7B30` | `_._9ccMinList` | CCLIST.HPP:201 |
| `0x800A7B80` | `ExtendRange(long&, long, long)` | COLMGR.CPP:195 |
| `0x800A7BBC` | `HTW_FillWallArray(long, long, long)` | COLMGR.CPP:240 |
| `0x800A7E38` | `HTW_HandleWallCollisions(DynamicThing*, long, long, long)` | COLMGR.CPP:319 |
| `0x800A7FAC` | `HTW_HandleHandFootCollisions(DynamicThing*)` | COLMGR.CPP:409 |
| `0x800A8290` | `HandleThingWall(DynamicThing*, long, long, long)` | COLMGR.CPP:510 |
| `0x800A8614` | `HandleThingFloor(DynamicThing*, long, long, long)` | COLMGR.CPP:795 |
| `0x800A9284` | `ClearThingFloorHeights(ccList&)` | COLMGR.CPP:1352 |
| `0x800A92C4` | `HandleThingEnvironmentCollisions(ccList&)` | COLMGR.CPP:1395 |
| `0x800A96EC` | `HandleHumanoidObstacleCollisions(ccList&)` | COLMGR.CPP:1667 |
| `0x800A9740` | `HandlePickupObstacleCollisions(ccList&)` | COLMGR.CPP:1699 |
| `0x800A9794` | `HandleHumanoidPickupCollisions(ccList&, ccList&)` | COLMGR.CPP:1732 |
| `0x800A9958` | `TriggerThing::TriggerThing(const tagLVector*, unsigned short)` | TRIGGER.CPP:75 |
| `0x800A999C` | `_._12TriggerThing` | TRIGGER.CPP:88 |
| `0x800A9A20` | `TriggerThing::AnalyzeMesh(DBRoot*)` | TRIGGER.CPP:105 |
| `0x800A9BE8` | `TriggerThing::CreateModel(const char*)` | TRIGGER.CPP:164 |
| `0x800A9BFC` | `TriggerThing::DeleteModel()` | TRIGGER.CPP:177 |
| `0x800A9C1C` | `TriggerThing::Reset()` | TRIGGER.CPP:187 |
| `0x800A9C24` | `TriggerThing::Think()` | TRIGGER.CPP:196 |
| `0x800A9C2C` | `TriggerThing::UpdatePosition()` | TRIGGER.CPP:205 |
| `0x800A9C34` | `TriggerThing::HandleCollision(Thing*)` | TRIGGER.CPP:217 |
| `0x800A9D20` | `TriggerThing::HandlePickupCollision(Pickup*)` | TRIGGER.CPP:257 |
| `0x800A9D40` | `TriggerThing::HandleHumanoidCollision(Humanoid*)` | TRIGGER.CPP:270 |
| `0x800A9D60` | `InvertCollisionBox(tagCollisionBox&)` | COLVOL.CPP:174 |
| `0x800A9D8C` | `FillCollisionBox(tagCollisionBox&, const tagLVector&, const tagLVector*, unsigned long)` | COLVOL.CPP:200 |
| `0x800A9F38` | `FillCollisionBox(tagCollisionBox&, const DBVolume&)` | COLVOL.CPP:267 |
| `0x800A9FB4` | `FillCollisionBox(tagCollisionBox&, const OriginalGeo&)` | COLVOL.CPP:307 |
| `0x800AA05C` | `SetCollisionBoxExtent(tagCollisionBox&)` | COLVOL.CPP:340 |
| `0x800AA0D4` | `CheckStaticHorizontalBoxPointCollision(const tagLVector&, const tagCollisionBox&, long, long)` | COLVOL.CPP:374 |
| `0x800AA22C` | `CheckStaticBoxSphereCollision(const tagLVector&, const tagCollisionBox&, long, long, const tagCollisionSphere&)` | COLVOL.CPP:461 |
| `0x800AA3E0` | `CheckStaticCylinderSphereCollision(const tagLVector&, const tagCollisionCylinder&, const tagCollisionCylinder&, const tagCollisionSphere&)` | COLVOL.CPP:549 |
| `0x800AA48C` | `Teleporter::Teleporter(const tagLVector*, unsigned short)` | TELEPORT.CPP:109 |
| `0x800AA4D8` | `_._10Teleporter` | TELEPORT.CPP:124 |
| `0x800AA500` | `Teleporter::AnalyzeMesh(DBRoot*)` | TELEPORT.CPP:138 |
| `0x800AA618` | `Teleporter::CreateModel(const char*)` | TELEPORT.CPP:186 |
| `0x800AA62C` | `Teleporter::DeleteModel()` | TELEPORT.CPP:200 |
| `0x800AA640` | `Teleporter::Reset()` | TELEPORT.CPP:210 |
| `0x800AA648` | `Teleporter::Think()` | TELEPORT.CPP:219 |
| `0x800AA650` | `Teleporter::UpdatePosition()` | TELEPORT.CPP:228 |
| `0x800AA658` | `Teleporter::HandlePickupCollision(Pickup*)` | TELEPORT.CPP:240 |
| `0x800AA660` | `Teleporter::HandleHumanoidCollision(Humanoid*)` | TELEPORT.CPP:253 |
| `0x800AA804` | `CGenericTransientSound::CGenericTransientSound()` | TRNSSND.CPP:18 |
| `0x800AA84C` | `_._22CGenericTransientSound` | TRNSSND.CPP:27 |
| `0x800AA8A0` | `CGenericTransientSound::Initialize(const tagLVector*, unsigned short)` | TRNSSND.CPP:43 |
| `0x800AA8C0` | `CGenericTransientSound::InitializeStereo(unsigned char, unsigned char)` | TRNSSND.CPP:54 |
| `0x800AA8E8` | `CGenericTransientSound::Trigger(unsigned short)` | TRNSSND.CPP:62 |
| `0x800AA938` | `CGenericTransientSound::TriggerDialogWorld(unsigned short)` | TRNSSND.CPP:74 |
| `0x800AAA68` | `CGenericTransientSound::TriggerPositional(unsigned short)` | TRNSSND.CPP:104 |
| `0x800AAB9C` | `CGenericTransientSound::TriggerNotPositional(unsigned short)` | TRNSSND.CPP:134 |
| `0x800AAD4C` | `CGenericTransientSound::Load(const char*)` | TRNSSND.CPP:169 |
| `0x800AAD68` | `(nw__22CGenericTransientSoundUi)` | TRNSSND.CPP:176 |
| `0x800AADBC` | `CSoundFactoryDatabase::CSoundFactoryDatabase()` | SNDFDB.CPP:28 |
| `0x800AADC4` | `_._21CSoundFactoryDatabase` | SNDFDB.CPP:32 |
| `0x800AADEC` | `CSoundFactoryDatabase::LoadObject(unsigned long, CSound*, unsigned long)` | SNDFDB.CPP:41 |
| `0x800AAF2C` | `CSoundFactoryDatabase::CreateParticleEffectSound(CSound*, unsigned long)` | SNDFDB.CPP:165 |
| `0x800AB040` | `CSoundFactoryDatabase::CreateWorldEffectSound(CSound*, unsigned long)` | SNDFDB.CPP:216 |
| `0x800AB234` | `CSoundFactoryDatabase::CreatePushableSound(CSound*, unsigned long)` | SNDFDB.CPP:299 |
| `0x800AB370` | `CSoundFactoryDatabase::CreateKickNRollSound(CSound*, unsigned long)` | SNDFDB.CPP:356 |
| `0x800AB3B4` | `CSoundFactoryDatabase::CreatePlatformSound(CSound*, unsigned long)` | SNDFDB.CPP:372 |
| `0x800AB900` | `CSoundFactoryDatabase::CreateWeaponSound(CSound*, unsigned long)` | SNDFDB.CPP:589 |
| `0x800AB9BC` | `CSoundFactoryDatabase::CreateHumanoidSound(CSound*, unsigned long)` | SNDFDB.CPP:634 |
| `0x800ABB64` | `CSoundFactoryDatabase::CreateDestructibleSound(CSound*, unsigned long)` | SNDFDB.CPP:718 |
| `0x800ABF54` | `CSoundFactoryDatabase::CreatePendulumSound(CSound*, unsigned long)` | SNDFDB.CPP:859 |
| `0x800AC074` | `CSoundFactoryDatabase::CreateGenericTransientSound(CSound*, unsigned long)` | SNDFDB.CPP:915 |
| `0x800AC100` | `CSoundFactoryDatabase::CreateGenericPersistentSound(CSound*, unsigned long)` | SNDFDB.CPP:946 |
| `0x800AC18C` | `CSoundFactoryDatabase::CreateKnockDownSound(CSound*, unsigned long)` | SNDFDB.CPP:976 |
| `0x800AC1D8` | `CSoundFactoryDatabase::CreateFrontEndSound(CSound*, unsigned long)` | SNDFDB.CPP:993 |
| `0x800AC20C` | `CSoundFactoryDatabase::CreateDirectorSound(CSound*, unsigned long)` | SNDFDB.CPP:1004 |
| `0x800AC240` | `CSoundFactoryDatabase::IsBasicSoundLoaded(unsigned long)` | SNDFDB.CPP:1011 |
| `0x800AC28C` | `CGenericPersistentSound::CGenericPersistentSound()` | PRSTSND.CPP:15 |
| `0x800AC2CC` | `_._23CGenericPersistentSound` | PRSTSND.CPP:22 |
| `0x800AC328` | `CGenericPersistentSound::SetVol(unsigned char)` | PRSTSND.CPP:27 |
| `0x800AC3AC` | `CGenericPersistentSound::Initialize(const tagLVector*, unsigned short)` | PRSTSND.CPP:45 |
| `0x800AC3CC` | `CGenericPersistentSound::Begin()` | PRSTSND.CPP:51 |
| `0x800AC498` | `CGenericPersistentSound::End()` | PRSTSND.CPP:77 |
| `0x800AC4F8` | `CGenericPersistentSound::Load(const char*)` | PRSTSND.CPP:93 |
| `0x800AC510` | `(nw__23CGenericPersistentSoundUi)` | PRSTSND.CPP:101 |
| `0x800AC564` | `CGenericPersistentSound::Initialize(const tagLVector*)` | PRSTSND.CPP:117 |
| `0x800AC584` | `CDestructibleSound::Initialize(const tagLVector*)` | DSTRSND.CPP:34 |
| `0x800AC5A4` | `CDestructibleSound::Smash()` | DSTRSND.CPP:52 |
| `0x800AC5F4` | `CDestructibleSound::Think()` | DSTRSND.CPP:77 |
| `0x800AC610` | `CDestructibleSound::CDestructibleSound()` | DSTRSND.CPP:100 |
| `0x800AC650` | `_._18CDestructibleSound` | DSTRSND.CPP:121 |
| `0x800AC6A4` | `CDestructibleSound::Load(const char*)` | DSTRSND.CPP:140 |
| `0x800AC6BC` | `CDestructibleSound::GetMaterial(CSoundMaterial*)` | DSTRSND.CPP:161 |
| `0x800AC6D0` | `CPushableSound::BeginPush()` | PUSHSND.CPP:14 |
| `0x800AC6F4` | `CPushableSound::EndPush()` | PUSHSND.CPP:19 |
| `0x800AC714` | `CPushableSound::Kick()` | PUSHSND.CPP:24 |
| `0x800AC75C` | `CPushableSound::HitHumanoid()` | PUSHSND.CPP:35 |
| `0x800AC77C` | `CPushableSound::CPushableSound()` | PUSHSND.CPP:40 |
| `0x800AC7C8` | `_._14CPushableSound` | PUSHSND.CPP:50 |
| `0x800AC824` | `CPushableSound::Load(const char*)` | PUSHSND.CPP:55 |
| `0x800AC844` | `CPushableSound::Initialize(const tagLVector*)` | PUSHSND.CPP:62 |
| `0x800AC864` | `CPushableSound::Think()` | PUSHSND.CPP:67 |
| `0x800AC880` | `CPushableSound::GetMaterial(CSoundMaterial*)` | PUSHSND.CPP:77 |
| `0x800AC894` | `CPlatformSound::CPlatformSound()` | PLATSND.CPP:15 |
| `0x800AC8E0` | `_._14CPlatformSound` | PLATSND.CPP:26 |
| `0x800AC94C` | `CPlatformSound::Initialize(const tagLVector*)` | PLATSND.CPP:32 |
| `0x800AC984` | `CPlatformSound::BeginMove()` | PLATSND.CPP:48 |
| `0x800AC9E0` | `CPlatformSound::EndMove()` | PLATSND.CPP:69 |
| `0x800ACA2C` | `CPlatformSound::HitPathNode(long, long, long)` | PLATSND.CPP:86 |
| `0x800ACA6C` | `CPlatformSound::Tilt()` | PLATSND.CPP:100 |
| `0x800ACA94` | `CPlatformSound::Impact()` | PLATSND.CPP:105 |
| `0x800ACABC` | `CPlatformSound::HitHumanoid()` | PLATSND.CPP:110 |
| `0x800ACAFC` | `CPlatformSound::Think()` | PLATSND.CPP:121 |
| `0x800ACB2C` | `CPlatformSound::Load(const char*)` | PLATSND.CPP:134 |
| `0x800ACB70` | `CPlatformSound::GetMaterial(CSoundMaterial*)` | PLATSND.CPP:142 |
| `0x800ACB84` | `CPendulumSound::Initialize(const tagLVector*)` | PNDLMSND.CPP:12 |
| `0x800ACBA4` | `CPendulumSound::Swing()` | PNDLMSND.CPP:18 |
| `0x800ACBCC` | `CPendulumSound::HitHumanoid()` | PNDLMSND.CPP:23 |
| `0x800ACBF4` | `CPendulumSound::CPendulumSound()` | PNDLMSND.CPP:28 |
| `0x800ACC28` | `_._14CPendulumSound` | PNDLMSND.CPP:31 |
| `0x800ACC7C` | `CPendulumSound::Load(const char*)` | PNDLMSND.CPP:36 |
| `0x800ACC9C` | `CParticleEffectSound::CParticleEffectSound()` | ESOUND.CPP:23 |
| `0x800ACCDC` | `_._20CParticleEffectSound` | ESOUND.CPP:43 |
| `0x800ACD38` | `CParticleEffectSound::Initialize(const tagLVector*)` | ESOUND.CPP:61 |
| `0x800ACD58` | `CParticleEffectSound::Load(const char*)` | ESOUND.CPP:84 |
| `0x800ACD70` | `CParticleEffectSound::StartAnimating()` | ESOUND.CPP:105 |
| `0x800ACD94` | `CParticleEffectSound::StopAnimating()` | ESOUND.CPP:123 |
| `0x800ACDB4` | `CWorldEffectSound::CWorldEffectSound()` | ESOUND.CPP:141 |
| `0x800ACDF8` | `_._17CWorldEffectSound` | ESOUND.CPP:163 |
| `0x800ACE54` | `CWorldEffectSound::Initialize(const tagLVector*)` | ESOUND.CPP:181 |
| `0x800ACE74` | `CWorldEffectSound::Load(const char*)` | ESOUND.CPP:204 |
| `0x800ACEB0` | `CWorldEffectSound::StartAnimating()` | ESOUND.CPP:224 |
| `0x800ACED4` | `CWorldEffectSound::Update(unsigned long)` | ESOUND.CPP:246 |
| `0x800ACF90` | `CWorldEffectSound::StopAnimating()` | ESOUND.CPP:278 |
| `0x800ACFB0` | `CWorldEffectSound::VolRiseAndFall(unsigned long)` | ESOUND.CPP:297 |
| `0x800AD074` | `CWeaponSound::Initialize(const tagLVector*)` | WPNSND.CPP:12 |
| `0x800AD094` | `CWeaponSound::HitHumanoid()` | WPNSND.CPP:17 |
| `0x800AD0BC` | `CWeaponSound::Grab()` | WPNSND.CPP:27 |
| `0x800AD0E4` | `CWeaponSound::Explode()` | WPNSND.CPP:32 |
| `0x800AD128` | `CWeaponSound::Miss()` | WPNSND.CPP:38 |
| `0x800AD150` | `CWeaponSound::CWeaponSound()` | WPNSND.CPP:43 |
| `0x800AD184` | `_._12CWeaponSound` | WPNSND.CPP:47 |
| `0x800AD1D8` | `CWeaponSound::Load(const char*)` | WPNSND.CPP:51 |
| `0x800AD210` | `CKickNRollSound::BeginRoll()` | KICKSND.CPP:14 |
| `0x800AD234` | `CKickNRollSound::EndRoll()` | KICKSND.CPP:23 |
| `0x800AD254` | `CKickNRollSound::Kick()` | KICKSND.CPP:30 |
| `0x800AD294` | `CKickNRollSound::HitHumanoid()` | KICKSND.CPP:42 |
| `0x800AD2B4` | `CKickNRollSound::CKickNRollSound()` | KICKSND.CPP:48 |
| `0x800AD2F0` | `_._15CKickNRollSound` | KICKSND.CPP:54 |
| `0x800AD34C` | `CKickNRollSound::Load(const char*)` | KICKSND.CPP:59 |
| `0x800AD36C` | `CKickNRollSound::Initialize(const tagLVector*)` | KICKSND.CPP:67 |
| `0x800AD38C` | `CKickNRollSound::Think()` | KICKSND.CPP:72 |
| `0x800AD3A8` | `CKnockDownSound::Initialize(const tagLVector*)` | KNDNSND.CPP:21 |
| `0x800AD3C8` | `CKnockDownSound::BeginFall()` | KNDNSND.CPP:26 |
| `0x800AD3EC` | `CKnockDownSound::EndFall()` | KNDNSND.CPP:35 |
| `0x800AD40C` | `CKnockDownSound::Kick()` | KNDNSND.CPP:40 |
| `0x800AD44C` | `CKnockDownSound::Impact()` | KNDNSND.CPP:52 |
| `0x800AD48C` | `CKnockDownSound::HitHumanoid()` | KNDNSND.CPP:64 |
| `0x800AD4AC` | `CKnockDownSound::Think()` | KNDNSND.CPP:69 |
| `0x800AD4DC` | `CKnockDownSound::CKnockDownSound()` | KNDNSND.CPP:83 |
| `0x800AD51C` | `_._15CKnockDownSound` | KNDNSND.CPP:91 |
| `0x800AD578` | `CKnockDownSound::Load(const char*)` | KNDNSND.CPP:96 |
| `0x800AD660` | `rmCartesianToPolar(long*, long*, long, long)` | POLAR.CPP:62 |
| `0x800AD6C4` | `ItemNode::ItemNode(char*, long)` | ITEMNODE.CPP:24 |
| `0x800AD718` | `ItemNode::GetStoreID()` | ITEMNODE.CPP:36 |
| `0x800AD724` | `ItemNode::DeletePermMem()` | ITEMNODE.CPP:46 |
| `0x800AD760` | `ItemNode::GetNext()` | ITEMNODE.CPP:52 |
| `0x800AD76C` | `_._8ItemNode` | ITEMNODE.HPP:54 |
| `0x800AE1CC` | `xcLongWordMemCopy(unsigned long*, unsigned long*, long)` | XCDO.CPP:30 |
| `0x800AE1F8` | `XCon_DrawResetPrim(void)` | XCDO.CPP:38 |
| `0x800AE2A8` | `xcPrimObj::sDraw(xcPrimObj*)` | XCDO.CPP:68 |
| `0x800AE3E8` | `xcPrimObj::FindNamedData(xcSectionMan*)` | XCDO.CPP:100 |
| `0x800AE430` | `xcPrimObj::Draw()` | XCDO.CPP:110 |
| `0x800AE478` | `xcPrimObj::Load()` | XCDO.CPP:118 |
| `0x800AE4A8` | `xcPrimObj::Unload()` | XCDO.CPP:126 |
| `0x800AE4D8` | `Stub(xcPrimObj*)` | XCDO.CPP:134 |
| `0x800AE4E0` | `xcClipObj::sDraw(xcPrimObj*)` | XCDO.CPP:162 |
| `0x800AE5F4` | `xcSprite::Load()` | XCDO.CPP:187 |
| `0x800AE670` | `xcSprite::Unload()` | XCDO.CPP:198 |
| `0x800AE6F8` | `xcSprite::FindNamedData(xcSectionMan*)` | XCDO.CPP:209 |
| `0x800AE76C` | `xcSprite::sDraw(xcPrimObj*)` | XCDO.CPP:221 |
| `0x800AE798` | `xcTextObj::FindNamedData(xcSectionMan*)` | XCDO.CPP:229 |
| `0x800AE828` | `xcTextObj::sDraw(xcPrimObj*)` | XCDO.CPP:241 |
| `0x800AE854` | `static_init(XCon_CheckPrimBuffer__FPUcUl)` | XCDO.CPP:281 |
| `0x800AE898` | `rPMallocShrink` | RADSMEM.C:17 |
| `0x800AE904` | `rmMag3ffu(unsigned long, unsigned long, unsigned long)` | MAG3FF.CPP:57 |
| `0x800AEA14` | `rmMag3ff(long, long, long)` | MAG3FF.CPP:76 |
| `0x800AEA54` | `rmMag2ff(long, long)` | MAG2FF.CPP:27 |
| `0x800AEA90` | `NextTagItem` | TAGS.C:47 |
| `0x800AEAFC` | `FindTagItem` | TAGS.C:81 |
| `0x800AEB64` | `SimpleBox::SimpleBox()` | SIMPLBOX.CPP:21 |
| `0x800AEB84` | `SimpleBox::SetBox(DBVolume*)` | SIMPLBOX.CPP:31 |
| `0x800AEBDC` | `SimpleBox::IsValid() const` | SIMPLBOX.CPP:44 |
| `0x800AEBF4` | `SimpleBox::IsInside(const tagLVector&) const` | SIMPLBOX.CPP:52 |
| `0x800AEC88` | `SimpleBox::IsInside(long, long) const` | SIMPLBOX.CPP:63 |
| `0x800AECE8` | `rmInverse16` | INVERSE.C:20 |
| `0x800AED38` | `Shadow::Shadow(Model*)` | SHADOW.CPP:167 |
| `0x800AED60` | `_._6Shadow` | SHADOW.CPP:176 |
| `0x800AED94` | `ShadowShow(const tagLVector&, tagLVector*, int)` | SHADOW.CPP:192 |
| `0x800AEFE0` | `TreeShadow::TreeShadow(Model*)` | SHADOW.CPP:249 |
| `0x800AF014` | `_._10TreeShadow` | SHADOW.CPP:255 |
| `0x800AF03C` | `TreeShadow::Place(tagLVector&, tagLVector*)` | SHADOW.CPP:273 |
| `0x800AF214` | `TreeShadow::Show(void*)` | SHADOW.CPP:342 |
| `0x800AF258` | `SimpleShadow::SimpleShadow(Model*)` | SHADOW.CPP:357 |
| `0x800AF2AC` | `_._12SimpleShadow` | SHADOW.CPP:367 |
| `0x800AF2D4` | `SimpleShadow::Place(tagLVector&, tagLVector*)` | SHADOW.CPP:385 |
| `0x800AF4A4` | `SimpleShadow::Show(void*)` | SHADOW.CPP:457 |
| `0x800AF5C8` | `rmSphericalToCartesian(RMVECTS16*, _RMVECT16*)` | SPHERE.CPP:78 |
| `0x800AF6D0` | `tParamFlip::tParamFlip()` | PARAMFLIP.CPP:22 |
| `0x800AF718` | `_._10tParamFlip` | PARAMFLIP.CPP:30 |
| `0x800AF76C` | `tParamFlip::Reset()` | PARAMFLIP.CPP:37 |
| `0x800AF7C0` | `tParamFlip::SetFrame(int)` | PARAMFLIP.CPP:45 |
| `0x800AF7D0` | `tParamFlip::SetFrameReal(long)` | PARAMFLIP.CPP:52 |
| `0x800AF7E0` | `tParamFlip::SetAnimation(tAnimation*)` | PARAMFLIP.CPP:59 |
| `0x800AF7F0` | `tStatic3DOFKeyList::GetValue(long, long*, tJointCache*, int)` | CHANNEL.CPP:242 |
| `0x800AF814` | `tDynamicKeyList::tDynamicKeyList()` | CHANNEL.CPP:251 |
| `0x800AF834` | `tDynamicKeyList::tDynamicKeyList(int)` | CHANNEL.CPP:258 |
| `0x800AF848` | `_._15tDynamicKeyList` | CHANNEL.CPP:263 |
| `0x800AF8B0` | `tDynamicKeyList::FindFirstKey(int)` | CHANNEL.CPP:268 |
| `0x800AF948` | `tJoint3DOFangle::tJoint3DOFangle()` | CHANNEL.CPP:299 |
| `0x800AF980` | `tJoint3DOFangle::tJoint3DOFangle(int)` | CHANNEL.CPP:303 |
| `0x800AF9B4` | `_._15tJoint3DOFangle` | CHANNEL.CPP:309 |
| `0x800AFA08` | `tJoint3DOFangle::GetValue(long, long*, tJointCache*, int)` | CHANNEL.CPP:314 |
| `0x800AFC10` | `tJoint1DOFangle::tJoint1DOFangle()` | CHANNEL.CPP:445 |
| `0x800AFC4C` | `tJoint1DOFangle::tJoint1DOFangle(int)` | CHANNEL.CPP:450 |
| `0x800AFC80` | `_._15tJoint1DOFangle` | CHANNEL.CPP:456 |
| `0x800AFCD4` | `tJoint1DOFangle::GetValue(long, long*, tJointCache*, int)` | CHANNEL.CPP:461 |
| `0x800AFE80` | `tJoint3DOFlpPSX::tJoint3DOFlpPSX(int)` | CHANNEL.CPP:577 |
| `0x800AFEB4` | `_._15tJoint3DOFlpPSX` | CHANNEL.CPP:582 |
| `0x800AFF08` | `tJoint3DOFlpPSX::GetValue(long, long*, tJointCache*, int)` | CHANNEL.CPP:588 |
| `0x800B0134` | `tTransformAnim::tTransformAnim(int)` | CHANNEL.CPP:644 |
| `0x800B0168` | `_._14tTransformAnim` | CHANNEL.CPP:649 |
| `0x800B0290` | `tTransformAnim::MakePuppet()` | CHANNEL.CPP:674 |
| `0x800B0398` | `tTransformFlip2::tTransformFlip2()` | CHANNEL.CPP:712 |
| `0x800B03F8` | `_._15tTransformFlip2` | CHANNEL.CPP:723 |
| `0x800B0470` | `tTransformFlip2::SetFrame(int)` | CHANNEL.CPP:730 |
| `0x800B0480` | `tTransformFlip2::SetFrameReal(long)` | CHANNEL.CPP:737 |
| `0x800B0490` | `tTransformFlip2::Reset()` | CHANNEL.CPP:744 |
| `0x800B04E4` | `tTransformFlip2::Update()` | CHANNEL.CPP:752 |
| `0x800B0514` | `tTransformFlip2::Update(tTree*)` | CHANNEL.CPP:758 |
| `0x800B0578` | `tTransformFlip2::UpdateJoints(tTree*)` | CHANNEL.CPP:778 |
| `0x800B0748` | `tTransformFlip2::UpdateJointsMirrored(tTree*)` | CHANNEL.CPP:832 |
| `0x800B0A70` | `tTransformFlip2::SetAnimation(tAnimation*)` | CHANNEL.CPP:944 |
| `0x800B0A80` | `tTreeFlip::Attach(tTree*, tTransformAnim*)` | CHANNEL.CPP:953 |
| `0x800B0B3C` | `(thunk_32_Update__15tTransformFlip2P5tTree)` | CHANNEL.HPP:582 |
| `0x800B0B5C` | `tTreeFlip::GetEntityType()` | CHANNEL.HPP:591 |
| `0x800B0B68` | `_._9tTreeFlip` | CHANNEL.HPP:592 |
| `0x800B0B9C` | `tTransformFlip2::GetEntityType()` | CHANNEL.HPP:550 |
| `0x800B0BA8` | `tTransformAnim::GetNumFrames()` | CHANNEL.HPP:514 |
| `0x800B0BB4` | `tTransformAnim::GetEntityType()` | CHANNEL.HPP:509 |
| `0x800B0BC0` | `tJoint3DOFlpPSX::GetKeyType()` | CHANNEL.HPP:490 |
| `0x800B0BC8` | `tJoint1DOFangle::GetKeyType()` | CHANNEL.HPP:445 |
| `0x800B0BD0` | `tJoint3DOFangle::GetKeyType()` | CHANNEL.HPP:404 |
| `0x800B0BD8` | `tStatic3DOFKeyList::GetKeyType()` | CHANNEL.HPP:361 |
| `0x800B0BE0` | `_._18tStatic3DOFKeyList` | CHANNEL.HPP:366 |
| `0x800B0C14` | `_._8tKeyList` | CHANNEL.HPP:329 |
| `0x800B0C48` | `rmASin16` | ASIN.C:18 |
| `0x800B0CA4` | `rmACos16` | ASIN.C:30 |
| `0x800B0D08` | `tPose::tPose()` | POSE.CPP:40 |
| `0x800B0D24` | `_._5tPose` | POSE.CPP:46 |
| `0x800B0D80` | `tPose::Init(int)` | POSE.CPP:51 |
| `0x800B0E18` | `FindActionRequest(unsigned long&, unsigned long, unsigned long, const Control*)` | COMINTER.CPP:84 |
| `0x800B1268` | `tChunk::tChunk(tFile*)` | TCHUNK.CPP:59 |
| `0x800B128C` | `_._6tChunk` | TCHUNK.CPP:67 |
| `0x800B12C0` | `tChunk::SetFile(tFile*)` | TCHUNK.CPP:71 |
| `0x800B12C8` | `tChunk::GetFile()` | TCHUNK.CPP:76 |
| `0x800B12D4` | `tReadChunk::tReadChunk(tFile*)` | TCHUNK.CPP:82 |
| `0x800B1324` | `_._10tReadChunk` | TCHUNK.CPP:87 |
| `0x800B134C` | `tReadChunk::GetDataSize()` | TCHUNK.CPP:91 |
| `0x800B1358` | `tReadChunk::Read()` | TCHUNK.CPP:96 |
| `0x800B13B4` | `tReadChunk::Skip()` | TCHUNK.CPP:103 |
| `0x800B13F0` | `tReadChunk::ReadNext()` | TCHUNK.CPP:108 |
| `0x800B1448` | `tReadChunk::EndOfChunk()` | TCHUNK.CPP:115 |
| `0x800B149C` | `tLitFarTable::tLitFarTable()` | LITFARD.CPP:34 |
| `0x800B14D8` | `tLitFarTable::Install()` | LITFARD.CPP:39 |
| `0x800B1570` | `LitFarDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:59 |
| `0x800B1750` | `LitFarDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:121 |
| `0x800B1998` | `LitFarDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:187 |
| `0x800B1BA0` | `LitFarDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:255 |
| `0x800B1E18` | `LitFarDisplayG3(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:335 |
| `0x800B2080` | `LitFarDisplayG4(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:413 |
| `0x800B239C` | `LitFarDisplayGT3(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:511 |
| `0x800B2634` | `LitFarDisplayGT4(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:599 |
| `0x800B2980` | `LitFarDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:716 |
| `0x800B2C40` | `LitFarDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:804 |
| `0x800B2FC0` | `LitFarDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:914 |
| `0x800B32B0` | `LitFarDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | LITFARD.CPP:1013 |
| `0x800B3660` | `_._12tLitFarTable` | LITFARD.HPP:25 |
| `0x800B3688` | `tZFarTable::tZFarTable()` | ZFARD.CPP:34 |
| `0x800B36C4` | `tZFarTable::Install()` | ZFARD.CPP:39 |
| `0x800B373C` | `ZFarDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:59 |
| `0x800B390C` | `ZFarDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:119 |
| `0x800B3B04` | `ZFarDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:183 |
| `0x800B3D3C` | `ZFarDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:246 |
| `0x800B3F94` | `ZFarDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:324 |
| `0x800B4198` | `ZFarDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:384 |
| `0x800B43CC` | `ZFarDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:458 |
| `0x800B4654` | `ZFarDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | ZFARD.CPP:530 |
| `0x800B4914` | `_._10tZFarTable` | ZFARD.HPP:19 |
| `0x800B493C` | `tDrawTable::tDrawTable()` | TDTABLE.CPP:43 |
| `0x800B49A0` | `_._10tDrawTable` | TDTABLE.CPP:59 |
| `0x800B49C8` | `tDrawTable::DrawGeometry(const tGeometry*)` | TDTABLE.CPP:90 |
| `0x800B4A64` | `tZSortTable::tZSortTable()` | ZSORTD.CPP:34 |
| `0x800B4AA0` | `tZSortTable::Install()` | ZSORTD.CPP:39 |
| `0x800B4B18` | `ZSortDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:59 |
| `0x800B4CC8` | `ZSortDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:118 |
| `0x800B4ECC` | `ZSortDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:180 |
| `0x800B50A4` | `ZSortDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:245 |
| `0x800B52C8` | `ZSortDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:322 |
| `0x800B54BC` | `ZSortDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:382 |
| `0x800B5708` | `ZSortDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:453 |
| `0x800B591C` | `ZSortDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | ZSORTD.CPP:527 |
| `0x800B5BA8` | `_._11tZSortTable` | ZSORTD.HPP:24 |
| `0x800B5BD0` | `tLitTable::tLitTable()` | LITD.CPP:34 |
| `0x800B5C0C` | `tLitTable::Install()` | LITD.CPP:39 |
| `0x800B5CA4` | `LitDisplayF3(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:59 |
| `0x800B5E60` | `LitDisplayF4(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:120 |
| `0x800B6068` | `LitDisplayFT3(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:184 |
| `0x800B624C` | `LitDisplayFT4(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:251 |
| `0x800B648C` | `LitDisplayG3(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:329 |
| `0x800B66D4` | `LitDisplayG4(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:406 |
| `0x800B69B4` | `LitDisplayGT3(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:503 |
| `0x800B6C2C` | `LitDisplayGT4(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:591 |
| `0x800B6F44` | `LitDisplayGC3(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:707 |
| `0x800B71E4` | `LitDisplayGC4(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:794 |
| `0x800B7530` | `LitDisplayGCT3(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:903 |
| `0x800B7800` | `LitDisplayGCT4(tDynGeom*, tPolygon*, unsigned long)` | LITD.CPP:1002 |
| `0x800B7B7C` | `_._9tLitTable` | LITD.HPP:25 |
| `0x800B7BA4` | `tFile::tFile(tByteStream*, int)` | TFILE.CPP:32 |
| `0x800B7BD8` | `_._5tFile` | TFILE.CPP:37 |
| `0x800B7C28` | `tFile::AttachStream(tByteStream*, int)` | TFILE.CPP:42 |
| `0x800B7C34` | `tFile::DetachStream()` | TFILE.CPP:48 |
| `0x800B7C90` | `tFile::Eof()` | TFILE.CPP:55 |
| `0x800B7CC8` | `tFile::GetBytes(void*, unsigned long)` | TFILE.CPP:60 |
| `0x800B7D00` | `tFile::GetWord()` | TFILE.CPP:83 |
| `0x800B7D3C` | `tFile::GetLong()` | TFILE.CPP:90 |
| `0x800B7D78` | `tFile::GetPString(char*)` | TFILE.CPP:97 |
| `0x800B7DD4` | `tFile::GetChar()` | TFILE.CPP:112 |
| `0x800B7E10` | `tFile::IsOpen()` | TFILE.CPP:119 |
| `0x800B7E50` | `tFile::SetPosition(long)` | TFILE.CPP:126 |
| `0x800B7E88` | `tFile::GetPosition()` | TFILE.CPP:131 |
| `0x800B7EC0` | `tFile::GetFileSize()` | TFILE.CPP:136 |
| `0x800B7EF8` | `_._11tByteStream` | TFILE.CPP:141 |
| `0x800B7F2C` | `tMemByteStream::tMemByteStream(unsigned char*, unsigned long)` | TFILE.CPP:156 |
| `0x800B7F90` | `_._14tMemByteStream` | TFILE.CPP:173 |
| `0x800B7FE8` | `tMemByteStream::GetLength()` | TFILE.CPP:179 |
| `0x800B7FF4` | `tMemByteStream::GetPosition()` | TFILE.CPP:184 |
| `0x800B8004` | `tMemByteStream::SetPosition(long)` | TFILE.CPP:189 |
| `0x800B801C` | `tMemByteStream::GetBytes(void*, unsigned long)` | TFILE.CPP:196 |
| `0x800B8068` | `tMemByteStream::Eof()` | TFILE.CPP:227 |
| `0x800B8084` | `tMemByteStream::IsOpen()` | TFILE.CPP:232 |
| `0x800B808C` | `tMemByteStream::GetMemPosition()` | TFILE.HPP:131 |
| `0x800B8098` | `tByteStream::GetMemPosition()` | TFILE.HPP:72 |
| `0x800B8FF4` | `rmStringHash` | HASH.C:15 |
| `0x800B91A4` | `rsdStream::rsdStream()` | RSDSTRM.CPP:92 |
| `0x800B91E0` | `_._9rsdStream` | RSDSTRM.CPP:117 |
| `0x800B921C` | `rsdStream::Open(void**, unsigned long, long, bool, bool, rsdStreamCallback*)` | RSDSTRM.CPP:151 |
| `0x800B93F0` | `rsdStream::Close()` | RSDSTRM.CPP:287 |
| `0x800B9520` | `rsdStream::StartCallback(long, long, long)` | RSDSTRM.CPP:365 |
| `0x800B9540` | `rsdStream::Start(unsigned long, unsigned long)` | RSDSTRM.CPP:391 |
| `0x800B9578` | `rsdStream::StartCDQueueISEmpty(unsigned long, unsigned long)` | RSDSTRM.CPP:418 |
| `0x800B96F8` | `rsdStream::Stop()` | RSDSTRM.CPP:525 |
| `0x800B97CC` | `Silence(unsigned char*, unsigned long, unsigned long, unsigned long)` | RSDSTRM.CPP:601 |
| `0x800B98E4` | `rsdStream::CdYield()` | RSDSTRM.CPP:653 |
| `0x800B98F0` | `rsdStream::CdAccess()` | RSDSTRM.CPP:673 |
| `0x800B98F8` | `rsdStream::SetVolume(bool, unsigned short, unsigned short)` | RSDSTRM.CPP:701 |
| `0x800B999C` | `rsdStream::GetLastMusicWakeUp()` | RSDSTRM.CPP:755 |
| `0x800B99A8` | `rsdStream::IsCdYielded()` | RSDSTRM.CPP:775 |
| `0x800B99B4` | `rsdStream::SPUAddrCallback()` | RSDSTRM.CPP:794 |
| `0x800B9B60` | `rsdStream::Callback(rsdLoad&)` | RSDSTRM.CPP:921 |
| `0x800B9C44` | `rsdStream::LoadTaskStub(_RTASK*)` | RSDSTRM.CPP:991 |
| `0x800B9C68` | `rsdStream::LoadTask()` | RSDSTRM.CPP:997 |
| `0x800B9F54` | `rsdStream::CDDoneCallback(long, long, long)` | RSDSTRM.CPP:1217 |
| `0x800BA04C` | `rsdStream::CDDoneFreeMemory(long, long, long)` | RSDSTRM.CPP:1278 |
| `0x800BA078` | `rsdStream::VoiceOffTask(_RTASK*)` | RSDSTRM.CPP:1304 |
| `0x800BA0D8` | `rsdStream::EnableSeamlessStitching(bool)` | RSDSTRM.CPP:1331 |
| `0x800BA0E0` | `rsdLoadCallback::Callback(rsdLoad&)` | RSDLOAD.HPP:53 |
| `0x800BA0F0` | `Decibel100(unsigned long)` | SNDMATH.CPP:24 |
| `0x800BA118` | `PsxVol100(unsigned long)` | SNDMATH.CPP:47 |
| `0x800BA150` | `PsxPitch200(unsigned long)` | SNDMATH.CPP:52 |
| `0x800BB168` | `tDynGeom::Clone()` | TDYNGEOM.CPP:35 |
| `0x800BB2C8` | `tDynGeom::Display()` | TDYNGEOM.CPP:64 |
| `0x800BB304` | `tDynGeom::GetGeoType()` | TDYNGEOM.HPP:113 |
| `0x800BB30C` | `_._8tDynGeom` | TDYNGEOM.HPP:84 |
| `0x800BB334` | `tDynGeom::GetEntityType()` | TDYNGEOM.HPP:58 |
| `0x800BB33C` | `tGeometry::GetVertexList()` | TGEOMTRY.INL:42 |
| `0x800BB348` | `tGeometry::SetVertexList(SVECTOR*)` | TGEOMTRY.INL:37 |
| `0x800BB350` | `tGeometry::GetBoundingSphere()` | TGEOMTRY.INL:32 |
| `0x800BB358` | `tGeometry::GetBoundingBox()` | TGEOMTRY.INL:27 |
| `0x800BB360` | `tGeometry::Clone()` | TGEOMTRY.INL:57 |
| `0x800BB368` | `_._9tGeometry` | TGEOMTRY.INL:23 |
| `0x800BB390` | `tTree::FindJoint(unsigned long)` | TREE.CPP:36 |
| `0x800BB414` | `tTree::FindJointIndex(unsigned long)` | TREE.CPP:50 |
| `0x800BB494` | `_._5tTree` | TREE.HPP:101 |
| `0x800BB4E8` | `P3DVERIFY(int, char*, N41)` | ERROR.CPP:109 |
| `0x800BB4F0` | `tClutList::tClutList()` | CLUTANIM.CPP:31 |
| `0x800BB540` | `_._9tClutList` | CLUTANIM.CPP:42 |
| `0x800BB5AC` | `tClutList::MakePuppet()` | CLUTANIM.CPP:48 |
| `0x800BB638` | `tClutList::GetNumFrames()` | CLUTANIM.CPP:60 |
| `0x800BB644` | `tClutList::GetFrame(int)` | CLUTANIM.CPP:65 |
| `0x800BB65C` | `tClutList::DeleteFrames()` | CLUTANIM.CPP:71 |
| `0x800BB69C` | `tClutList::SetNumFrames(int)` | CLUTANIM.CPP:78 |
| `0x800BB700` | `tClutList::SetNumOffsets(int)` | CLUTANIM.CPP:94 |
| `0x800BB764` | `tClutList::SetFrame(int, unsigned short)` | CLUTANIM.CPP:110 |
| `0x800BB778` | `tClutList::SetOffset(int, unsigned short)` | CLUTANIM.CPP:116 |
| `0x800BB78C` | `tClutFlip::tClutFlip()` | CLUTANIM.CPP:125 |
| `0x800BB7D4` | `_._9tClutFlip` | CLUTANIM.CPP:134 |
| `0x800BB7FC` | `tClutFlip::Reset()` | CLUTANIM.CPP:139 |
| `0x800BB84C` | `tClutFlip::Update()` | CLUTANIM.CPP:147 |
| `0x800BB94C` | `tClutFlip::GetClutFrame(int)` | CLUTANIM.CPP:189 |
| `0x800BB984` | `tClutFlip::GetEntityType()` | CLUTANIM.HPP:90 |
| `0x800BB990` | `tClutList::GetEntityType()` | CLUTANIM.HPP:54 |
| `0x800BB99C` | `tIndexList::tIndexList()` | TIDXLIST.CPP:125 |
| `0x800BB9E8` | `_._10tIndexList` | TIDXLIST.CPP:134 |
| `0x800BBA3C` | `tIndexList::Insert(tEntity*)` | TIDXLIST.CPP:140 |
| `0x800BBB24` | `tIndexList::Remove(unsigned short)` | TIDXLIST.CPP:193 |
| `0x800BBBF4` | `tIndexList::Empty()` | TIDXLIST.CPP:237 |
| `0x800BBC08` | `tIndexList::CreateStore(int)` | TIDXLIST.CPP:245 |
| `0x800BBC88` | `tIndexList::DeleteStore()` | TIDXLIST.CPP:262 |
| `0x800BBCD8` | `tIndexList::GrowBy(int)` | TIDXLIST.CPP:272 |
| `0x800BBD7C` | `tCache::tCache()` | TCACHE.CPP:72 |
| `0x800BBDA0` | `_._6tCache` | TCACHE.CPP:80 |
| `0x800BBDD4` | `InvCacheElem::InvCacheElem()` | TCACHE.CPP:86 |
| `0x800BBDEC` | `tInvCache::tInvCache()` | TCACHE.CPP:95 |
| `0x800BBE68` | `_._9tInvCache` | TCACHE.CPP:101 |
| `0x800BBE90` | `tInvCache::Flush()` | TCACHE.CPP:107 |
| `0x800BBEF4` | `tInvCache::ResetElem(InvCacheElem*)` | TCACHE.CPP:115 |
| `0x800BBF08` | `tInvCache::Insert(unsigned long, unsigned short)` | TCACHE.CPP:123 |
| `0x800BBF98` | `tInvCache::Search(unsigned long)` | TCACHE.CPP:154 |
| `0x800BC024` | `tTexture::tTexture()` | TTEXTURE.CPP:48 |
| `0x800BC05C` | `_._8tTexture` | TTEXTURE.CPP:53 |
| `0x800BC0B0` | `tTexture::GetTextureData()` | TTEXTURE.CPP:69 |
| `0x800BC0BC` | `tTexture::SetTextureData(unsigned long*)` | TTEXTURE.CPP:74 |
| `0x800BC0C4` | `tTexture::GetTextureRect()` | TTEXTURE.CPP:79 |
| `0x800BC0CC` | `tParamAnim::tParamAnim()` | PARAMANIM.CPP:21 |
| `0x800BC118` | `_._10tParamAnim` | PARAMANIM.CPP:31 |
| `0x800BC1D0` | `tParamAnim::SetParam(int, tKeyList**)` | PARAMANIM.CPP:39 |
| `0x800BC1DC` | `tParamAnim::MakePuppet()` | PARAMANIM.CPP:46 |
| `0x800BC214` | `tParamAnim::GetNumFrames()` | PARAMANIM.HPP:51 |
| `0x800BC220` | `tParamAnim::GetEntityType()` | PARAMANIM.HPP:49 |
| `0x800BC22C` | `tJoint3DOF::tJoint3DOF()` | KEYNDOF.CPP:11 |
| `0x800BC264` | `_._10tJoint3DOF` | KEYNDOF.CPP:21 |
| `0x800BC2B8` | `tJoint3DOF::GetValue(long, long*, tJointCache*, int)` | KEYNDOF.CPP:26 |
| `0x800BC5A0` | `tJoint1DOF::tJoint1DOF()` | KEYNDOF.CPP:74 |
| `0x800BC5DC` | `_._10tJoint1DOF` | KEYNDOF.CPP:85 |
| `0x800BC630` | `tJoint1DOF::GetValue(long, long*, tJointCache*, int)` | KEYNDOF.CPP:90 |
| `0x800BC840` | `tJoint1DOF::GetKeyType()` | KEYNDOF.HPP:67 |
| `0x800BC848` | `tJoint3DOF::GetKeyType()` | KEYNDOF.HPP:47 |
| `0x800BC850` | `tSTreeUnLit::DeepCopy()` | STREEUNLIT.CPP:33 |
| `0x800BC9D0` | `tSTreeUnLit::Display()` | STREEUNLIT.CPP:67 |
| `0x800BCCE4` | `_._11tSTreeUnLit` | STREEUNLIT.HPP:49 |
| `0x800BCDA8` | `tTexFlip::tTexFlip()` | TEXANIM.CPP:25 |
| `0x800BCDE4` | `_._8tTexFlip` | TEXANIM.CPP:31 |
| `0x800BCE0C` | `tTexFlip::Reset()` | TEXANIM.CPP:36 |
| `0x800BCE5C` | `tTexFlip::Update()` | TEXANIM.CPP:44 |
| `0x800BCF44` | `tTexFlip::GetTexFrame(int)` | TEXANIM.CPP:94 |
| `0x800BCF7C` | `tTexList::tTexList()` | TEXANIM.CPP:106 |
| `0x800BCFCC` | `_._8tTexList` | TEXANIM.CPP:117 |
| `0x800BD038` | `tTexList::MakePuppet()` | TEXANIM.CPP:123 |
| `0x800BD0A0` | `tTexList::DeleteFrames()` | TEXANIM.CPP:132 |
| `0x800BD0E0` | `tTexList::SetNumFrames(int)` | TEXANIM.CPP:139 |
| `0x800BD140` | `tTexList::SetNumOffsets(unsigned short)` | TEXANIM.CPP:156 |
| `0x800BD1A4` | `tTexList::SetOffset(unsigned short, unsigned short)` | TEXANIM.CPP:173 |
| `0x800BD1BC` | `tTexList::GetTexFrame(int)` | TEXANIM.CPP:179 |
| `0x800BD1F8` | `tTexList::SetTexFrame(int, unsigned short)` | TEXANIM.CPP:188 |
| `0x800BD22C` | `tTexList::GetNumFrames()` | TEXANIM.HPP:113 |
| `0x800BD238` | `tTexList::GetEntityType()` | TEXANIM.HPP:106 |
| `0x800BD244` | `tTexFlip::GetEntityType()` | TEXANIM.HPP:76 |
| `0x800BDED0` | `rPDMalloc` | RADDMEM.C:22 |
| `0x800BE270` | `SpotLight::SpotLight()` | SPOTLIGHT.CPP:79 |
| `0x800BE2A4` | `_._9SpotLight` | SPOTLIGHT.CPP:94 |
| `0x800BE2CC` | `SpotLight::Create()` | SPOTLIGHT.CPP:111 |
| `0x800BE344` | `SpotLight::Update()` | SPOTLIGHT.CPP:145 |
| `0x800BE450` | `SpotLight::Display(int)` | SPOTLIGHT.CPP:222 |
| `0x800BE510` | `SpotLight::SetUp(unsigned long, unsigned long)` | SPOTLIGHT.CPP:254 |
| `0x800BE51C` | `SpotLight::PutBackEffect()` | SPOTLIGHT.CPP:271 |
| `0x800BE54C` | `PathInfo::Init(DBPath*, DBPoint*)` | PATHINFO.CPP:69 |
| `0x800BE73C` | `PathInfo::PathInfo()` | PATHINFO.CPP:134 |
| `0x800BE78C` | `_._8PathInfo` | PATHINFO.CPP:167 |
| `0x800BE7F8` | `PathInfo::Reset()` | PATHINFO.CPP:183 |
| `0x800BE844` | `PathInfo::Update()` | PATHINFO.CPP:202 |
| `0x800BEA44` | `PathInfo::OnNewPathNode(int)` | PATHINFO.CPP:272 |
| `0x800BED44` | `PathInfo::GetPosition()` | PATHINFO.CPP:423 |
| `0x800BED50` | `PathInfo::GetRotation()` | PATHINFO.CPP:428 |
| `0x800BED58` | `_._4Path` | PATH.HPP:108 |
| `0x800BEE3C` | `LensFlare::LensFlare()` | LENSFLRE.CPP:171 |
| `0x800BEE90` | `_._9LensFlare` | LENSFLRE.CPP:193 |
| `0x800BEF44` | `LensFlare::Create()` | LENSFLRE.CPP:214 |
| `0x800BF000` | `LensFlare::InitLensFlare(int, DBPath*)` | LENSFLRE.CPP:258 |
| `0x800BF244` | `LensFlare::BigScreenGlow()` | LENSFLRE.CPP:359 |
| `0x800BF45C` | `LensFlare::ComputeTracking(tagLVector&, tagLVector&)` | LENSFLRE.CPP:418 |
| `0x800BF578` | `LensFlare::Update()` | LENSFLRE.CPP:481 |
| `0x800BFB34` | `LensFlare::Display(int)` | LENSFLRE.CPP:631 |
| `0x800BFE64` | `GenFlip32(void*, void*, unsigned long)` | XCSORT.CPP:27 |
| `0x800BFEA0` | `GenShakerSort(void*, unsigned long, unsigned long, void*(*)(void*)*, unsigned long, unsigned long(*)(void*, void*)*, void)` | XCSORT.CPP:44 |
| `0x800BFFD4` | `Line::GetXOnLine(long) const` | COLLINE.CPP:84 |
| `0x800C003C` | `Line::GetZOnLine(long) const` | COLLINE.CPP:112 |
| `0x800C00A4` | `Line::Equal(const Line&, const Line&)` | COLLINE.CPP:141 |
| `0x800C0160` | `Line::Intersection(const Line&, const Line&, long, long&, long&)` | COLLINE.CPP:166 |
| `0x800C05CC` | `rmATan16` | ATAN16.C:23 |
| `0x800C0724` | `AddJoint(tReadChunk&, tFile*, tEJoint*)` | ETLOAD.CPP:71 |
| `0x800C0880` | `tETreeLoader::LoadInternal(tReadChunk&, void**)` | ETLOAD.CPP:111 |
| `0x800C0A90` | `tETreeLoader::Load(tReadChunk&, void**)` | ETLOAD.CPP:161 |
| `0x800C0B34` | `_._7tEJoint` | ETREE.HPP:116 |
| `0x800C0B68` | `_._12tETreeLoader` | ETREE.HPP:89 |
| `0x800C0B9C` | `_._10tTreeJoint` | TREE.HPP:80 |
| `0x800C0BD0` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800C0C04` | `RP_XformVertsLit(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | RPSTREELIT.CPP:19 |
| `0x800C0D58` | `RP_XformVertsLitCBF(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | RPSTREELIT.CPP:92 |
| `0x800C0E20` | `RP_FixUpPolysCBF(tPrimGeom*, void*, unsigned long, unsigned long)` | RPSTREELIT.CPP:138 |
| `0x800C1450` | `RP_XformVertsNoLit(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | RPSTREENLT.CPP:20 |
| `0x800C14BC` | `RP_FixUpPolysNoLit(tPrimGeom*, void*, unsigned long, unsigned long)` | RPSTREENLT.CPP:301 |
| `0x800C1940` | `RP_FixUpPolysNoLitFlat(tPrimGeom*, void*, unsigned long, unsigned long)` | RPSTREENLT.CPP:502 |
| `0x800C1DC4` | `tVertAnimLoader::Load(tReadChunk&, void**)` | TVRTLOAD.CPP:32 |
| `0x800C1F58` | `_._15tVertAnimLoader` | VERTANIM.HPP:81 |
| `0x800C1F8C` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800C1FC0` | `tUVAnim::tUVAnim()` | UVANIM.CPP:68 |
| `0x800C200C` | `_._7tUVAnim` | UVANIM.CPP:78 |
| `0x800C2078` | `tUVAnim::Init(int, int, int, unsigned short*, unsigned long*)` | UVANIM.CPP:85 |
| `0x800C210C` | `tUVAnim::GetUVFrame(int, int)` | UVANIM.CPP:96 |
| `0x800C2138` | `tUVAnim::MakePuppet()` | UVANIM.CPP:101 |
| `0x800C21AC` | `tUVFlip::tUVFlip()` | UVANIM.CPP:111 |
| `0x800C21E0` | `_._7tUVFlip` | UVANIM.CPP:116 |
| `0x800C2208` | `tUVFlip::Reset()` | UVANIM.CPP:121 |
| `0x800C2238` | `tUVFlip::Update()` | UVANIM.CPP:126 |
| `0x800C233C` | `tUVAnimLoader::Load(tReadChunk&, void**)` | UVANIM.CPP:197 |
| `0x800C25C8` | `_._13tUVAnimLoader` | UVANIM.HPP:118 |
| `0x800C25FC` | `tUVFlip::GetEntityType()` | UVANIM.HPP:100 |
| `0x800C2608` | `tUVAnim::GetNumFrames()` | UVANIM.HPP:74 |
| `0x800C2614` | `tUVAnim::GetEntityType()` | UVANIM.HPP:61 |
| `0x800C2620` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800C2654` | `RP_XformVerts(tPrimGeom*, tSJoint*, unsigned long*, unsigned short*)` | RPSTREE.CPP:20 |
| `0x800C26BC` | `RP_FixUpPolys(tPrimGeom*, void*, unsigned long, unsigned long)` | RPSTREE.CPP:49 |
| `0x800C2AA8` | `tCBVParamAnim::tCBVParamAnim()` | CBVPARAM.CPP:56 |
| `0x800C2ADC` | `_._13tCBVParamAnim` | CBVPARAM.CPP:61 |
| `0x800C2B04` | `tCBVParamAnim::SetBlendAnim(tParamAnim*)` | CBVPARAM.CPP:66 |
| `0x800C2B0C` | `tCBVParamAnim::MakePuppet()` | CBVPARAM.CPP:71 |
| `0x800C2B7C` | `tCBVParamFlip::tCBVParamFlip()` | CBVPARAM.CPP:80 |
| `0x800C2BB0` | `_._13tCBVParamFlip` | CBVPARAM.CPP:85 |
| `0x800C2BD8` | `tCBVParamFlip::Reset()` | CBVPARAM.CPP:90 |
| `0x800C2C08` | `tCBVParamFlip::Update()` | CBVPARAM.CPP:95 |
| `0x800C2F84` | `tCBVParamFlip::Attach(tCBVParamAnim*, tParamAnim*)` | CBVPARAM.CPP:183 |
| `0x800C301C` | `tCBVParamAnimLoader::Load(tReadChunk&, void**)` | CBVPARAM.CPP:243 |
| `0x800C3304` | `_._19tCBVParamAnimLoader` | CBVPARAM.HPP:98 |
| `0x800C3338` | `tCBVParamFlip::GetEntityType()` | CBVPARAM.HPP:75 |
| `0x800C3344` | `tCBVParamAnim::GetEntityType()` | CBVPARAM.HPP:58 |
| `0x800C3350` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800C3384` | `tCBVAnim::tCBVAnim()` | CBVANIM.CPP:68 |
| `0x800C33D0` | `_._8tCBVAnim` | CBVANIM.CPP:78 |
| `0x800C343C` | `tCBVAnim::Init(int, int, int, unsigned long*, unsigned long*)` | CBVANIM.CPP:85 |
| `0x800C34D0` | `tCBVAnim::MakePuppet()` | CBVANIM.CPP:97 |
| `0x800C3544` | `tCBVFlip::tCBVFlip()` | CBVANIM.CPP:107 |
| `0x800C3578` | `_._8tCBVFlip` | CBVANIM.CPP:112 |
| `0x800C35A0` | `tCBVFlip::Reset()` | CBVANIM.CPP:117 |
| `0x800C35D0` | `tCBVFlip::Update()` | CBVANIM.CPP:122 |
| `0x800C3728` | `tCBVAnimLoader::Load(tReadChunk&, void**)` | CBVANIM.CPP:218 |
| `0x800C39CC` | `_._14tCBVAnimLoader` | CBVANIM.HPP:122 |
| `0x800C3A00` | `tCBVFlip::GetEntityType()` | CBVANIM.HPP:103 |
| `0x800C3A0C` | `tCBVAnim::GetNumFrames()` | CBVANIM.HPP:78 |
| `0x800C3A18` | `tCBVAnim::GetEntityType()` | CBVANIM.HPP:65 |
| `0x800C3A24` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800C3A58` | `RP_FixUpPolysFlat(tPrimGeom*, void*, unsigned long, unsigned long)` | RPSTREEFLAT.CPP:18 |
| `0x800C3E44` | `LoadVizAnim(tReadChunk&, unsigned short)` | VIZANIM.CPP:62 |
| `0x800C3FF8` | `tVizAnimLoader::Load(tReadChunk&, void**)` | VIZANIM.CPP:116 |
| `0x800C409C` | `tVizAnim::tVizAnim()` | VIZANIM.CPP:136 |
| `0x800C40DC` | `_._8tVizAnim` | VIZANIM.CPP:143 |
| `0x800C4130` | `tVizAnim::MakePuppet()` | VIZANIM.CPP:148 |
| `0x800C41A8` | `tVizAnim::Init(int, int, tVizNode*)` | VIZANIM.CPP:158 |
| `0x800C41B8` | `tVizFlip::Reset()` | VIZANIM.CPP:165 |
| `0x800C4208` | `tVizFlip::Update()` | VIZANIM.CPP:171 |
| `0x800C4278` | `tVizFlip::SetViz(tVizNode*)` | VIZANIM.CPP:181 |
| `0x800C42F4` | `_._8tVizFlip` | VIZANIM.HPP:114 |
| `0x800C4314` | `tVizAnim::GetNumFrames()` | VIZANIM.HPP:93 |
| `0x800C4320` | `tVizAnim::GetEntityType()` | VIZANIM.HPP:85 |
| `0x800C432C` | `_._14tVizAnimLoader` | VIZANIM.HPP:64 |
| `0x800C4360` | `_._7tLoader` | TLOADER.HPP:74 |
| `0x800C4394` | `tMTree::FindJoint(unsigned long)` | MTREE.CPP:40 |
| `0x800C43E0` | `tMTree::Display()` | MTREE.CPP:54 |
| `0x800C451C` | `_._6tMTree` | MTREE.HPP:138 |
| `0x800C4604` | `tDirectionalLight::tDirectionalLight(tLightHardwareSlot)` | TDLIGHT.CPP:33 |
| `0x800C4644` | `tDirectionalLight::SetDirection(long, long, long)` | TDLIGHT.CPP:41 |
| `0x800C467C` | `tDirectionalLight::Update()` | TDLIGHT.CPP:49 |
| `0x800C47E4` | `_._17tDirectionalLight` | TDLIGHT.HPP:41 |
| `0x800C480C` | `rStrToFixed` | STRTO.C:51 |
| `0x800C49AC` | `tLight::tLight(tLightHardwareSlot)` | TLIGHT.CPP:31 |
| `0x800C4A04` | `_._6tLight` | TLIGHT.CPP:39 |
| `0x800C4A2C` | `tLight::SetColour(unsigned long)` | TLIGHT.CPP:45 |
| `0x800C4A5C` | `rmV2Dot(_RMVECT216*, _RMVECT216*)` | VECT2D.CPP:64 |
| `0x800C4AC4` | `CorrectCollision(const tagLVector&, const tagLVector&, long, G9_RMVECT16R10tagLVectorT4)` | COLPHYS.CPP:47 |
| `0x800C4D8C` | `xcPolyHandleFT4::AddPolysToOT()` | XCFONTDC.CPP:20 |
| `0x800C4E20` | `xcFontDC::xcFontDC(xcTextObj&)` | XCFONTDC.CPP:47 |
| `0x800C4ED4` | `xcFontDC::Draw()` | XCFONTDC.CPP:60 |
| `0x800C4F44` | `xcFontDC::PushJustTrans(short, short)` | XCFONTDC.CPP:76 |
| `0x800C505C` | `xcFontDC::MakePolys(POLY_FT4*, xcPolyHandleFT4*)` | XCFONTDC.CPP:111 |
| `0x800C53D0` | `xcFontDC::GetSize(short*)` | XCFONTDC.CPP:326 |
| `0x800C5514` | `xcFontDC::GetWidthLine(long)` | XCFONTDC.CPP:368 |
| `0x800C55FC` | `(void, char, __11xc3x3Matrixi)` | XC3X3MAT.H:27 |
| `0x800C5610` | `xcImageDC::xcImageDC(xcSprite&)` | XCIDC.CPP:27 |
| `0x800C56AC` | `xcImageDC::Draw()` | XCIDC.CPP:37 |
| `0x800C5744` | `xcImageDC::FindWalkingVectors(unsigned long, unsigned long, _RMVECT216*)` | XCIDC.CPP:61 |
| `0x800C57C4` | `xcImageDC::FindJust(unsigned long, unsigned long, _RMVECT216*)` | XCIDC.CPP:82 |
| `0x800C5840` | `xcImageDC::DrawPolys(POLY_FT4*)` | XCIDC.CPP:101 |
| `0x800C5CC4` | `xcImageDC::DrawSprite(SPRT*)` | XCIDC.CPP:246 |
| `0x800C6098` | `(void, char, __11xc3x3Matrixi)` | XC3X3MAT.H:27 |
| `0x800C8F58` | `tETree::DeepCopy()` | ETREE.CPP:66 |
| `0x800C90E8` | `tETree::Display()` | ETREE.CPP:84 |
| `0x800C9280` | `tETree::GetJointAbsolute(int)` | ETREE.HPP:130 |
| `0x800C92A0` | `tETree::GetJoint(int)` | ETREE.HPP:129 |
| `0x800C92D4` | `tETree::GetJointList()` | ETREE.HPP:127 |
| `0x800C92E0` | `_._6tETree` | ETREE.HPP:123 |
| `0x800C93AC` | `_._7tEJoint` | ETREE.HPP:116 |
| `0x800C93E0` | `_._10tTreeJoint` | TREE.HPP:80 |
| `0x800C9414` | `xc3x3Matrix::SetUnit()` | XC3X3MAT.CPP:51 |
| `0x800C9450` | `xc3x3Matrix::SetTrans(long, long)` | XC3X3MAT.CPP:61 |
| `0x800C9494` | `xc3x3Matrix::Mult(const xc3x3Matrix&)` | XC3X3MAT.CPP:111 |
| `0x800C95A8` | `xc3x3Matrix::Mult(_RMVECT216*, _RMVECT216*, unsigned long)` | XC3X3MAT.CPP:130 |
| `0x800C96A8` | `xc3x3MatrixStack::Inc()` | XC3X3MAT.CPP:163 |
| `0x800C96BC` | `xc3x3MatrixStack::Dec()` | XC3X3MAT.CPP:179 |
| `0x800C96D0` | `(void, char, __11xc3x3Matrixi)` | XC3X3MAT.H:27 |
| `0x800C96E4` | `Is2ByteASCII(const char*)` | XCCHAR.CPP:13 |
| `0x800C96EC` | `FindNextChar(const char*, long*, xcUint16Union*)` | XCCHAR.CPP:65 |
| `0x800C99A0` | `Game::gsIntroState(Game*)` | GAME.CPP:1088 |
| `0x800C9AEC` | `Game::Game()` | GAME.CPP:2694 |
| `0x800C9CF8` | `Game::InternalOpen()` | GAME.CPP:2888 |
| `0x800C9F5C` | `Game::LoadConfigFile()` | GAME.CPP:3232 |
| `0x800CA1B8` | `Director::Director()` | DIRECTOR.CPP:2658 |
| `0x800CA2AC` | `Director::InternalOpen()` | DIRECTOR.CPP:2704 |
| `0x800CA39C` | `Display::Display()` | DISPLAY.CPP:119 |
| `0x800CA3E4` | `Display::InternalOpen()` | DISPLAY.CPP:137 |
| `0x800CA4B4` | `tPort::Init(tPortInitData*)` | TPORT.CPP:152 |
| `0x800CA650` | `AI::ParseBehaviourAttribScript()` | AI.CPP:2168 |
| `0x800CAAF8` | `Display::platOpen()` | PSXDISP.CPP:263 |
| `0x800CAEA8` | `P3D::Init()` | P3DGBL.CPP:44 |

