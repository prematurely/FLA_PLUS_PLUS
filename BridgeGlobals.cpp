#include "FLACompatBridgeInternal.h"

char g_logPath[MAX_PATH] = "scripts\\FLACompatBridge.log";

char g_configPath[MAX_PATH] = "scripts\\FLACompatBridge.ini";

char g_flaLogPath[MAX_PATH] = "fastman92limitAdjuster.log";

char g_flaIniPath[MAX_PATH] = "fastman92limitAdjuster_GTASA.ini";

LONG g_bridgeConfigOpenRetries = 0;

LONG g_flaRuntimeRecoveryStarted = 0;

BridgeConfig g_config{};

FakeColModelForBounds g_fakeColModelForBounds = {
    -0.5f, -0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.0f,  0.0f,  0.0f,
     0.75f,
     0x00000100u,
     nullptr
};

LONG g_exceptionLogCount = 0;

uintptr_t g_relocatedCModelInfoPtrs = 0;

uintptr_t g_relocatedStreamingInfo = 0;

uintptr_t g_relocatedStreamingInfoExtension = 0;

uintptr_t g_relocatedAnimBlocks = 0;

uintptr_t g_relocatedVehicleRecordingStreamingArray = 0;

uintptr_t g_relocatedStreamedScripts = 0;

uintptr_t g_relocatedHandlingManager = 0;

uintptr_t g_relocatedRegisteredKills = 0;

uint32_t g_fileIdCapacity = 0;

uint32_t g_flaCompatFlags = 0;

uint32_t g_flaAbiVersion = 0;

uint32_t g_runtimeStateSource = RUNTIME_SOURCE_UNKNOWN;

uint32_t g_pedPoolCapacity = 0;

uint32_t g_vehiclePoolCapacity = 0;

uint32_t g_objectPoolCapacity = 0;

uint32_t g_buildingPoolCapacity = 0;

uint32_t g_dummyPoolCapacity = 0;

uint32_t g_colModelPoolCapacity = 0;

uint32_t g_collisionStoreCapacity = 0;

uint32_t g_colAccelFakePool[16]{};

LONG g_colAccelStartCachePoolGuardLogs = 0;

LONG g_colModelPoolNewGuardLogs = 0;

LONG g_cPoolsInitialiseRecoveryLogs = 0;

LONG g_cPoolsInitialiseRecoveryState = 0;

LONG g_cPoolsInitialiseDispatchState = 0;

LONG g_batchLazyCPoolInitialiseLogs = 0;

LONG g_batchLazyCPoolInitialiseState = 0;

LONG g_lazyCPoolBatchDepth = 0;

LONG g_ptrNodeExhaustionGuardLogs = 0;

LONG g_rwClumpForAllAtomicsGuardLogs = 0;

LONG g_rpAnimBlendClumpInitGuardLogs = 0;

PVOID g_vectoredExceptionHandlerHandle = nullptr;

DWORD g_gameThreadId = 0;

uintptr_t g_chainedStreamingBusyTarget = 0;

uintptr_t g_cPoolsInitialiseReplayTrampoline = 0;

FlaExtendedIdApi g_flaExtendedIdApi{};

uintptr_t g_cleoDispatchNullGuardReturn = 0;

uintptr_t g_cleoThunk26720Original = 0;

uintptr_t g_cleoPlusInitExtendedObjectVarsTrampoline = 0;

uintptr_t g_cleoPlusSetExtendedObjectVarTrampoline = 0;

uintptr_t g_cleoPlusGetExtendedObjectVarTrampoline = 0;

uintptr_t g_cleoPlusObjectAllocateBlocksContinue = 0;

uintptr_t g_cleoPlusVehicleAllocateBlocksContinue = 0;

uintptr_t g_cleoPlusPedAllocateBlocksContinue = 0;

uintptr_t g_mixSetsPedAllocateBlocksContinue = 0;

uintptr_t g_urbanizePedAllocateBlocksContinue = 0;

uintptr_t g_vehFuncsVehicleAllocateBlocksContinue = 0;

LONG g_vehFuncsPoolGuardInstallerStarted = 0;

uintptr_t g_animUncompressContinue = 0;

uintptr_t g_animStaticAssocInitContinue = 0;

uintptr_t g_animUpdateBlendContinue = 0;

uintptr_t g_animFrameUpdateSkinnedContinue = 0;

uintptr_t g_animFrameUpdateSkinnedVelocityContinue = 0;

uintptr_t g_animBlendGroupContinue = 0;

uintptr_t g_animClumpFinalizeNodesContinue = 0;

uintptr_t g_animClumpFinalizeNodesEmpty = 0;

uintptr_t g_animDestroyAssociationsContinue = 0;

uintptr_t g_animRemoveBlockContinue = 0;

uintptr_t g_rwClumpForAllAtomicsContinue = 0;

uintptr_t g_rpAnimBlendClumpInitContinue = 0;

std::atomic<uintptr_t> g_rwClumpExecutableCallbacks[kRwClumpCallbackCacheSize]{};

uintptr_t g_shouldModelBeStreamedContinue = 0;

uintptr_t g_shouldModelBeStreamedReturnFalse = 0;

uintptr_t g_flaTrainTypeCarriagesLoaderThis = 0;

uintptr_t g_flaTrainTypeCarriagesLoadFunc = 0;

uintptr_t g_widescreenFixSpriteNameGuardContinue = 0;

uintptr_t g_widescreenFixSpriteNameGuardSkip = 0;

uintptr_t g_flaAddTxdSlotThunk = 0;

uintptr_t g_properShadersAddTxdSlotThunk = 0;

uintptr_t g_lastValidAnimHierarchy = 0;

uintptr_t g_neutralizedAnimAssociations[kMaxNeutralizedAnimAssociations]{};

LONG g_neutralizedAnimAssociationCursor = 0;

AnimLifecycleRecord g_animLifecycleHistory[kAnimLifecycleHistorySize]{};

LONG g_animLifecycleCursor = 0;

uintptr_t g_radarTraceBase = 0;

uint32_t g_radarTraceLimit = 175;

alignas(16) uint32_t g_boundCentreScratch[4]{};

CRITICAL_SECTION g_deferredPoolAllocateLock{};

DeferredPoolAllocate g_deferredPoolAllocates[kMaxDeferredPoolAllocates]{};

uint32_t g_deferredPoolAllocateCount = 0;

LONG g_deferredPoolAllocateLogs = 0;

LONG g_deferredPoolAllocateReplayActive = 0;

ScriptCommandHandlerFn g_originalCommands900To999 = nullptr;

ScriptCommandHandlerFn g_originalCommands600To699 = nullptr;

uintptr_t g_commands900To999TableSlot = 0;

uintptr_t g_commands600To699TableSlot = 0;

LONG g_closestCarNode03D3FallbackLogs = 0;

LONG g_sanPabloSpecialActorBridgeLogs = 0;

LONG g_sanPabloSpecialActorBridgeLastLogState = (-2147483647L - 1L);

LONG g_specialActorSlotRangeLogs = 0;

LONG g_forwardingCommands600To699 = 0;

LONG g_forwardingCommands900To999 = 0;

LONG g_sanPabloCleoOpcodeCallbackInstalled = 0;

LONG g_sanPabloCleoOpcodeRegistered = 0;

ProcessCommandsGroupFn g_originalProcessCommands100To199 = nullptr;

uintptr_t g_processCommands100To199Trampoline = 0;

LONG g_taxi77SetCarCoordinatesLogs = 0;

float g_lastTargetBlipX = 0.0f;

float g_lastTargetBlipY = 0.0f;

float g_lastTargetBlipZ = 0.0f;

LONG g_hasLastTargetBlipCoords = 0;

CleoExports g_cleoExports{};

uintptr_t g_taxi77WatchdogMainPtr = 0;

uint32_t g_taxi77WatchdogActiveSince = 0;

uint32_t g_taxi77WatchdogLastLog = 0;

uint32_t g_taxi77WatchdogLastOffset = 0xFFFFFFFF;

LONG g_taxi77WatchdogRecoveries = 0;

RuntimeSpecialActorName g_runtimeSpecialActorNames[kMaxRuntimeSpecialActorNames]{};

uint32_t g_runtimeSpecialActorNameCount = 0;

RuntimeRewriteRule g_runtimeRewriteRules[kMaxRuntimeRewriteRules]{};

uint32_t g_runtimeRewriteRuleCount = 0;

const OlaSaOverlapSpec kOlaSaOverlapSpecs[] = {
    {"PtrNodeSingle", "PtrNode Singles", "FLA PtrNode Singles"},
    {"PtrNodeDouble", "PtrNode Doubles", "FLA PtrNode Doubles"},
    {"EntryInfoNode", "EntryInfoNodes", "FLA EntryInfoNodes"},
    {"Peds", "Peds", "FLA CPools Peds"},
    {"PedIntelligence", "PedIntelligence", "FLA CPools PedIntelligence"},
    {"Vehicles", "Vehicles", "FLA CPools Vehicles"},
    {"Buildings", "Buildings", "FLA CPools Buildings"},
    {"Objects", "Objects", "FLA CPools Objects"},
    {"Dummys", "Dummies", "FLA CPools Dummies"},
    {"ColModel", "ColModels", "FLA CPools ColModels"},
    {"Task", "Tasks", "FLA task allocator / old-mod guards"},
    {"Event", "Events", "FLA task/event limits"},
    {"PointRoute", "PointRoute", "FLA CPools PointRoute"},
    {"PatrolRoute", "PatrolRoute", "FLA CPools PatrolRoute"},
    {"NodeRoute", "NodeRoute", "FLA CPools NodeRoute"},
    {"TaskAllocator", "TaskAllocator", "FLA CPools TaskAllocator"},
    {"PedAttractors", "PedAttractors", "FLA CPools PedAttractors"},
    {"VehicleStructs", "VehicleStructs", "FLA vehicle/model limits"},
    {"MatrixList", "Matrices", "FLA matrix/physical object limits"},
    {"OutsideWorldWaterBlocks", "Blocks to be rendered outside world", "FLA map/render limits"},
    {"AlphaEntityList", "Alpha entity list limit", "FLA visibility limits"},
    {"VisibleEntityPtrs", "Visible entity pointers", "FLA visibility limits"},
    {"VisibleLodPtrs", "Visible LOD pointers", "FLA visibility limits"},
    {"StreamingObjectInstancesList", "rwObjectInstances", "FLA streaming/render limits"},
    {"AtomicModels", nullptr, "FLA model ID limits"},
    {"DamageAtomicModels", nullptr, "FLA model ID limits"},
    {"TimeModels", nullptr, "FLA model ID limits"},
    {"ClumpModels", nullptr, "FLA model ID limits"},
    {"VehicleModels", "Vehicle Models", "FLA model ID limits"},
    {"PedModels", "Ped Models", "FLA model ID limits"},
    {"WeaponModels", "Weapon Models", "FLA model ID limits"},
    {"EntitiesPerIpl", "Inst entries per file", "FLA map/IPL limits"},
    {"EntityIpl", "Entity index array", "FLA map/IPL limits"},
    {"StaticShadows", "Static shadows", "FLA shadow limits"},
    {"Coronas", "Coronas", "FLA corona limits"},
    {"ScriptSearchLights", nullptr, "FLA script/searchlight limits"},
    {"FrameLimit", nullptr, "external FPS plugin / FLA timing policy"},
    {"MemoryAvailable", "Memory available", "FLA streaming memory"}
};

LazyCPoolSpec g_lazyCPoolSpecs[] = {
    { "PtrNode Singles", kPtrNodeSinglePoolPtr, 0x00550180, 0x00863D10, "PtrNode Singles", 300000, 0 },
    { "PtrNode Doubles", kPtrNodeDoublePoolPtr, 0x00550250, 0x00863D00, "PtrNode Doubles", 150000, 0 },
    { "EntryInfoNodes", kEntryInfoNodePoolPtr, 0x00550320, 0x00863CF0, "EntryInfoNodes", 150000, 0 },
    { "Peds", kOriginalPedPoolPtr, 0x005503F0, 0x00863CE8, "Peds", 140, 0 },
    { "Vehicles", kOriginalVehiclePoolPtr, 0x005504C0, 0x00863CDC, "Vehicles", 110, 0 },
    { "Buildings", kOriginalBuildingPoolPtr, 0x00550570, 0x00863CD0, "Buildings", 13000, 0 },
    { "Objects", kOriginalObjectPoolPtr, 0x00550640, 0x00863CC8, "Objects", 350, 0 },
    { "Dummies", kOriginalDummyPoolPtr, 0x005506F0, 0x00863CC0, "Dummies", 2500, 0 },
    { "ColModels", kOriginalColModelPoolPtr, 0x005507C0, 0x00863CB4, "ColModels", 10150, 0 },
    { "Tasks", kTasksPoolPtr, 0x00550890, 0x00863CAC, "Tasks", 500, 0 },
    { "Events", kEventsPoolPtr, 0x00550960, 0x00863CA4, "Events", 200, 0 },
    { "PointRoute", kPointRoutePoolPtr, 0x00550A30, 0x00863C98, "PointRoute", 64, 0 },
    { "PatrolRoute", kPatrolRoutePoolPtr, 0x00550B00, 0x00863C8C, "PatrolRoute", 32, 0 },
    { "NodeRoute", kNodeRoutePoolPtr, 0x00550BD0, 0x00863C80, "NodeRoute", 64, 0 },
    { "TaskAllocator", kTaskAllocatorPoolPtr, 0x00550CA0, 0x00863C70, "TaskAllocator", 16, 0 },
    { "PedIntelligence", kPedIntelligencePoolPtr, 0x00550D70, 0x00863C60, "PedIntelligence", 140, 0 },
    { "PedAttractors", kPedAttractorsPoolPtr, 0x00550E40, 0x00863C50, "PedAttractors", 64, 0 },
};

const KnownCleoPlusVersion kKnownCleoPlusVersions[] = {
    // CLEO+ build measured 2026-06-09
    { 0x11DFE4B9, 0x30590, 0x30700, 0x30870 },
};

const size_t kKnownCleoPlusVersionCount =
    sizeof(kKnownCleoPlusVersions) / sizeof(kKnownCleoPlusVersions[0]);


