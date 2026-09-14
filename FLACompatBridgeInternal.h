#pragma once
// FLA++ internal shared header: types, constants, globals, and internal
// function declarations for the split translation units. Mirrors the
// former single-TU layout; definitions live in Bridge*.cpp / FLACompatBridge.cpp.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <intrin.h>
#pragma comment(lib, "Psapi.lib")
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr const char* kDisplayName = "FLA++";
constexpr const char* kProductVersion = "1.10c2";
constexpr uint32_t kApiVersion = 6;
constexpr const char* kLogRelativePath = "scripts\\FLACompatBridge.log";
constexpr const char* kConfigRelativePath = "scripts\\FLACompatBridge.ini";
constexpr const char* kFlaLogRelativePath = "fastman92limitAdjuster.log";
constexpr const char* kFlaIniRelativePath = "fastman92limitAdjuster_GTASA.ini";
struct BridgeConfig {
    bool enableBridge = true;
    bool enableDiagnosticsGroup = true;
    bool enableFLACompat = true;
    bool enableModCompat = true;
    bool enableCLEOPlusCompat = true;
    bool enableUrbanizeCompat = true;
    bool enableTaxi77Compat = true;
    bool enableSanPabloCompat = true;
    bool enableMixSetsCompat = true;

    bool enableVectoredExceptionHandler = true;
    bool enableExceptionDiagnostics = true;
    bool enableStackScanDiagnostics = false;
    bool enableMemoryRegionDiagnostics = false;
    bool enableBoundCentreVehRecovery = true;
    bool enablePtrNodeExhaustionGuard = true;
    bool enableExceptionLoopBreaker = true;
    bool exceptionLoopBreakerTerminate = true;
    int maxExceptionLogs = 16;
    int exceptionLoopBreakerThreshold = 32;

    bool enableModuleSnapshot = true;
    bool enableRiskConstantScan = true;
    bool enableRelocatedAddressDiagnostics = true;
    bool enablePoolPointerDiagnostics = true;
    bool enablePopulationPoolDiagnostics = false;
    int populationPoolDiagStartDelayMs = 15000;
    int populationPoolDiagIntervalMs = 5000;
    int populationPoolDiagIterations = 24;
    bool enableGangOnlyPopulationGuard = true;
    int gangOnlyPopulationGuardStartDelayMs = 15000;
    int gangOnlyPopulationGuardIntervalMs = 2000;
    int gangOnlyPopulationGuardIterations = 180;
    bool gangOnlyPopulationGuardClearCheatFlag = true;
    bool enablePedStreamingZoneRepair = false;
    bool pedStreamingZoneRepairCallOriginal = false;
    int pedStreamingZoneRepairStartDelayMs = 20000;
    int pedStreamingZoneRepairIntervalMs = 2000;
    int pedStreamingZoneRepairIterations = 180;
    int pedStreamingZoneRepairMaxCalls = 12;
    int pedStreamingZoneRepairMaxLogs = 64;
    bool enableStreamingBusyThresholdPatch = true;
    int streamingBusyThreshold = 128;
    bool enablePopulationUpdateBudgetPatch = false;
    int populationUpdateBudgetMs = 4;
    bool enableCrashClassification = true;
    bool enableRuntimeRewriteAudit = true;
    bool enableOpenLimitAdjusterOverlapAudit = true;
    bool enableFlaPathNodeDiagnostics = true;
    bool enableOpenLimitAdjusterSaLimitGuard = true;
    bool enableOpenLimitAdjusterModuleGuard = false;
    char openLimitAdjusterSaLimitAllowlist[512]{};
    bool enableModulePolicy = true;
    char legacyModuleAllowlist[512]{};
    char modernModuleDenylist[512]{};
    char forceNoRuntimeRewrite[512]{};
    char forceNoAutoPoolGuard[512]{};
    bool enableProperShadersCompat = true;
    bool enableRuntimeRewrite = false;
    bool enableRuntimeRewriteRescan = false;
    bool enableRuntimeRewriteRuleTable = true;
    int runtimeRewriteStartDelayMs = 1000;
    int runtimeRewriteIterations = 60;
    int runtimeRewriteIntervalMs = 1000;
    int runtimeRewriteRuleCount = 0;
    int runtimeRewriteDefaultMaxPatchesPerModule = 256;
    char runtimeRewriteAllowlist[512]{};
    char runtimeRewriteDenylist[512]{};

    bool enableLegacyModelInfoShadow = true;
    bool enableLegacyStreamingInfoShadow = true;
    bool enableFlaExtendedIdApi = true;
    int legacyShadowStartDelayMs = 500;
    int legacyShadowIterations = 300;
    int legacyShadowIntervalMs = 200;

    bool enableCObjectCreateBridge = true;
    bool enableCleoObjectCreateInlineRestore = true;
    bool enableCleoDispatchGuard = true;
    bool enableCleoDispatchLazyPoolRecovery = false;
    bool enableCleoThunk26720Guard = true;
    bool enableCleoPlusExtendedObjectVarGuard = false;
    bool enableCleoPlusPoolAllocateGuard = true;
    bool enableMixSetsPoolAllocateGuard = true;
    bool enableUrbanizePoolAllocateGuard = true;
    bool enableVehFuncsPoolAllocateGuard = true;
    bool enableAutoPoolAllocateGuard = false;
    bool enableDeferredPoolAllocateReplay = true;
    int autoPoolAllocateGuardMaxPatches = 256;
    int deferredPoolAllocateReplayIterations = 600;
    int deferredPoolAllocateReplayIntervalMs = 100;
    char autoPoolAllocateGuardAllowlist[512]{};
    char autoPoolAllocateGuardDenylist[512]{};
    bool enableAnimUncompressGuard = true;
    bool enableAnimStaticAssocGuard = true;
    bool enableAnimFrameUpdateGuard = false;
    bool enableAnimEmptyUpdateGuard = true;
    bool enableAnimLifecycleDiagnostics = true;
    bool enableRpAnimBlendClumpInitGuard = true;
    bool enableRwClumpForAllAtomicsGuard = true;
    bool enableShouldModelBeStreamedGuard = true;
    bool enableGetBoundCentreInlineGuard = true;
    bool enableGetBoundRectColModelGuard = true;
    bool enablePlaceableRemoveMatrixGuard = true;
    bool enablePlaceableRemoveMatrixSkipGuard = false;
    bool enablePlaceableStaticMatrixAllocGuard = true;
    bool matrixGuardSignatureCheck = true;
    bool matrixGuardRecoverStaticList = true;
    uint32_t matrixGuardListBase = 0x00B74288;
    uint32_t matrixGuardRemoveMatrixEntry = 0x0054F3B3;
    uint32_t matrixGuardRemoveMatrixNullLoad = 0x0054F3B6;
    uint32_t matrixGuardAllocateStaticOwnerWrite = 0x0054F557;
    uint32_t matrixGuardAllocateStaticResume = 0x0054F55D;
    uint32_t matrixGuardAllocateStaticMin = 0x0054F4C0;
    uint32_t matrixGuardAllocateStaticMax = 0x0054F640;
    bool enableUrbanizeProblemPedPreload = true;
    bool enableFlaNoCollisionErrorRestore = true;
    bool enableBridgeCheatStringLoader = true;
    bool enableFlaTrainInitHookRepair = true;
    bool enableFlaObjectInitCollisionRestore = false;
    bool enablePickupModelLoadGuard = true;
    bool pickupModelLoadGuardProtectHighIds = true;
    bool pickupModelLoadGuardLoadNow = true;
    int pickupModelLoadGuardFlags = 0x1E;
    int pickupModelLoadGuardMaxLogs = 64;
    char pickupModelLoadGuardAllowlist[512]{};
    bool enableColAccelStartCachePoolGuard = true;
    bool enableColModelPoolNewGuard = true;
    bool enableLazyCPoolRegistry = true;
    bool enableBatchLazyCPoolInitialise = false;
    bool enableReplayPoolReadSkipGuard = false;
    bool enableEarlyCPoolsInitialiseRecovery = true;
    bool enableCPoolsInitialiseRecovery = false;
    bool enableRadarBlipHandleGuard = true;
    bool enableCleoTargetBlipCoordsBridge = true;
    bool enableRadarTraceRuntimeRewrite = true;
    bool enableClosestCarNode03D3Fallback = true;
    bool enableTaxi77SetCarCoordinatesGuard = true;
    bool enableTaxi77StateWatchdog = true;
    bool enableTaxi77StateWatchdogRecovery = true;
    bool enableSanPabloSpecialActorBridge = true;
    bool specialActorAutoDetectExtendedSlots = true;
    bool specialActorAutoScanImgArchives = true;
    bool specialActorAutoScanModloader = true;
    bool specialActorAllowMissingTxd = false;
    bool specialActorRequireModelInfoSlot = true;
    bool specialActorLogCatalog = true;
    int specialActorMinModelId = 290;
    int specialActorMaxModelId = 299;
    int specialActorAutoMaxModelId = 20000;
    int specialActorAutoCatalogFirstCode = 100;
    int specialActorMaxAutoCatalogNames = 512;
    int specialActorCatalogBuildDelayMs = 8000;
    char specialActorAutoScanFilter[512]{};
    int taxi77WatchdogPollMs = 1000;
    int taxi77MainStuckSeconds = 75;
    int taxi77StartLabelOffset = 0x099B;
    int taxi77ActiveMinOffset = 0x0A70;
    int taxi77ActiveMaxOffset = 0x21B2;
};
struct FakeColModelForBounds {
    float bboxMinX;
    float bboxMinY;
    float bboxMinZ;
    float bboxMaxX;
    float bboxMaxY;
    float bboxMaxZ;
    float sphereCenterX;
    float sphereCenterY;
    float sphereCenterZ;
    float sphereRadius;
    uint32_t flagsAndSlot;
    void* colData;
};
struct BridgeRect {
    float left;
    float top;
    float right;
    float bottom;
};
enum RuntimeStateSource : uint32_t {
    RUNTIME_SOURCE_UNKNOWN = 0,
    RUNTIME_SOURCE_FLA_ABI = 1,
    RUNTIME_SOURCE_FLA_LOG = 2,
    RUNTIME_SOURCE_INI_OR_VANILLA = 3,
};
enum FLACompatRuntimeAddress : uint32_t {
    FLA_RUNTIME_ADDRESS_CMODELINFO_MODEL_INFO_PTRS = 1,
    FLA_RUNTIME_ADDRESS_CSTREAMING_INFO_FOR_MODEL = 2,
    FLA_RUNTIME_ADDRESS_CSTREAMING_INFO_EXTENSION = 3,
    FLA_RUNTIME_ADDRESS_CANIMMANAGER_ANIM_BLOCKS = 4,
    FLA_RUNTIME_ADDRESS_CDARKEL_REGISTERED_KILLS = 5,
};
enum FLACompatFlags : uint32_t {
    FLA_COMPAT_FLAG_ID_LIMITS_KNOWN = 1 << 0,
    FLA_COMPAT_FLAG_ID_LIMIT_PATCH_ENABLED = 1 << 1,
    FLA_COMPAT_FLAG_ANY_ID_LIMIT_INCREASED = 1 << 2,
    FLA_COMPAT_FLAG_ID_UNSIGNED = 1 << 3,
    FLA_COMPAT_FLAG_ID_BASE_INT32 = 1 << 4,
    FLA_COMPAT_FLAG_DIFFICULT_FILE_TYPES_INT32 = 1 << 5,
    FLA_COMPAT_FLAG_COL_ID_SIZE_INCREASED = 1 << 6,
    FLA_COMPAT_FLAG_IPL_ID_SIZE_INCREASED = 1 << 7,
    FLA_COMPAT_FLAG_MODEL_INFO_PTRS_MOVED = 1 << 8,
    FLA_COMPAT_FLAG_STREAMING_INFO_MOVED = 1 << 9,
    FLA_COMPAT_FLAG_ANIM_BLOCKS_MOVED = 1 << 10,
    FLA_COMPAT_FLAG_REGISTERED_KILLS_MOVED = 1 << 11,
};
struct FLACompatibilityInfo {
    uint32_t dwSize;
    uint32_t abiVersion;
    int32_t gameVersion;
    uint32_t flags;
    int32_t countOfAllFileIDs;
    int32_t defaultCountOfAllFileIDs;
    int32_t maxCountOfFileIDs;
    int32_t killableModelIDs;
    uintptr_t modelInfoPtrs;
    uintptr_t streamingInfoForModel;
    uintptr_t streamingInfoExtension;
    uintptr_t animBlocks;
    uintptr_t registeredKills;
};
using FlaAreDifficultIDsExtendedFn = bool(__cdecl*)();
using FlaGetNumberOfFileIDsFn = int32_t(__cdecl*)();
using FlaGetExtendedIDFrom16BitBeforeFn = int32_t(__cdecl*)(const void*);
using FlaSetExtendedIDFrom16BitBeforeFn = void(__cdecl*)(void*, int32_t);
struct FlaExtendedIdApi {
    HMODULE module = nullptr;
    FlaAreDifficultIDsExtendedFn areDifficultIDsExtended = nullptr;
    FlaGetNumberOfFileIDsFn getNumberOfFileIDs = nullptr;
    FlaGetExtendedIDFrom16BitBeforeFn getExtendedIDFrom16BitBefore = nullptr;
    FlaSetExtendedIDFrom16BitBeforeFn setExtendedIDFrom16BitBefore = nullptr;
    bool resolved = false;
    bool logged = false;
};
constexpr size_t kRwClumpCallbackCacheSize = 64;
constexpr size_t kMaxNeutralizedAnimAssociations = 64;
constexpr size_t kAnimLifecycleHistorySize = 64;
struct AnimLifecycleRecord {
    uintptr_t group = 0;
    uintptr_t associations = 0;
    uintptr_t caller = 0;
    uintptr_t animBlock = 0;
    uint32_t associationCount = 0;
    uint32_t idOffset = 0;
    uint32_t groupIndex = UINT32_MAX;
    uint32_t threadId = 0;
    uint32_t tick = 0;
    volatile LONG sequence = 0;
};
constexpr size_t kMaxDeferredPoolAllocates = 256;
struct DeferredPoolAllocate {
    uintptr_t poolPtrAddress = 0;
    uintptr_t continueAddress = 0;
    uintptr_t thisPtr = 0;
    LONG completed = 0;
    char moduleName[64]{};
};
struct ScriptParamLite {
    union {
        uint32_t uParam;
        int32_t iParam;
        float fParam;
        void* pParam;
    };
};
struct RunningScriptLite {
    void* next;
    void* prev;
    char name[8];
    uint8_t* baseIP;
    uint8_t* currentIP;
};
using ScriptCommandHandlerFn = unsigned char(__thiscall*)(RunningScriptLite*, unsigned short);
using ScriptCollectParametersFn = void(__thiscall*)(RunningScriptLite*, short);
using ScriptStoreParametersFn = void(__thiscall*)(RunningScriptLite*, short);
using ProcessCommandsGroupFn = unsigned char(__thiscall*)(RunningScriptLite*, int);
struct CleoExports {
    using OpcodeResult = signed char;
    using CustomOpcodeHandlerFn = OpcodeResult(__stdcall*)(RunningScriptLite*);
    using ScriptOpcodeBeforeCallbackFn = OpcodeResult(WINAPI*)(RunningScriptLite*, DWORD);
    using RegisterOpcodeFn = BOOL(WINAPI*)(WORD, CustomOpcodeHandlerFn);
    using CallNativeOpcodeFn = OpcodeResult(WINAPI*)(RunningScriptLite*, WORD);
    using GetScriptByNameFn = RunningScriptLite* (WINAPI*)(const char*, BOOL, BOOL, DWORD);
    using IsValidScriptPtrFn = BOOL(WINAPI*)(const RunningScriptLite*);
    using IsScriptRunningFn = BOOL(WINAPI*)(const RunningScriptLite*);
    using GetScriptBaseRelativeOffsetFn = DWORD(WINAPI*)(const RunningScriptLite*, const uint8_t*);
    using ThreadJumpAtLabelPtrFn = void(WINAPI*)(RunningScriptLite*, int);
    using RegisterCallbackFn = void(WINAPI*)(DWORD, void*);
    using GetOperandTypeFn = int(WINAPI*)(const RunningScriptLite*);
    using PeekPointerToScriptVariableFn = ScriptParamLite*(WINAPI*)(RunningScriptLite*);
    using SkipOpcodeParamsFn = void(WINAPI*)(RunningScriptLite*, int);
    using SetThreadCondResultFn = void(WINAPI*)(RunningScriptLite*, BOOL);

    HMODULE module = nullptr;
    GetScriptByNameFn getScriptByName = nullptr;
    IsValidScriptPtrFn isValidScriptPtr = nullptr;
    IsScriptRunningFn isScriptRunning = nullptr;
    GetScriptBaseRelativeOffsetFn getScriptBaseRelativeOffset = nullptr;
    ThreadJumpAtLabelPtrFn threadJumpAtLabelPtr = nullptr;
    RegisterOpcodeFn registerOpcode = nullptr;
    CallNativeOpcodeFn callNativeOpcode = nullptr;
    RegisterCallbackFn registerCallback = nullptr;
    GetOperandTypeFn getOperandType = nullptr;
    PeekPointerToScriptVariableFn peekPointerToScriptVariable = nullptr;
    SkipOpcodeParamsFn skipOpcodeParams = nullptr;
    SetThreadCondResultFn setThreadCondResult = nullptr;
};
constexpr size_t kMaxRuntimeSpecialActorNames = 768;
constexpr size_t kSpecialActorNameLength = 32;
struct RuntimeSpecialActorName {
    uint32_t code;
    char name[kSpecialActorNameLength];
    char source[32];
};
constexpr size_t kMaxRuntimeRewriteRules = 128;
enum RuntimeRewriteTarget : uint32_t {
    RUNTIME_REWRITE_TARGET_STATIC = 0,
    RUNTIME_REWRITE_TARGET_MODEL_INFO = 1,
    RUNTIME_REWRITE_TARGET_STREAMING_INFO = 2,
    RUNTIME_REWRITE_TARGET_RADAR_TRACE = 3,
    RUNTIME_REWRITE_TARGET_ANIM_BLOCKS = 4,
    RUNTIME_REWRITE_TARGET_STREAMED_SCRIPTS = 5,
    RUNTIME_REWRITE_TARGET_HANDLING_MANAGER = 6,
    RUNTIME_REWRITE_TARGET_VEHICLE_RECORDING = 7,
};
struct RuntimeRewriteRule {
    bool enabled = false;
    bool auditOnly = false;
    bool align4 = false;
    bool executableOnly = false;
    uint32_t oldValue = 0;
    uint32_t staticNewValue = 0;
    uint32_t maxPatchesPerModule = 0;
    RuntimeRewriteTarget target = RUNTIME_REWRITE_TARGET_STATIC;
    char name[64]{};
    char allowlist[256]{};
    char denylist[256]{};
};
constexpr uintptr_t kOriginalCModelInfoPtrs = 0x00A9B0C8;
constexpr size_t kOriginalCModelInfoCount = 20000;
constexpr uintptr_t kOriginalStreamingInfo = 0x008E4CC0;
constexpr size_t kOriginalStreamingInfoCount = 20000;
constexpr size_t kStreamingInfoSize = 0x14;
constexpr uintptr_t kOriginalAnimBlocks = 0x00B5D4A0;
constexpr uintptr_t kPtrNodeSinglePoolPtr = 0x00B74484;
constexpr uintptr_t kPtrNodeDoublePoolPtr = 0x00B74488;
constexpr uintptr_t kEntryInfoNodePoolPtr = 0x00B7448C;
constexpr uintptr_t kOriginalPedPoolPtr = 0x00B74490;
constexpr uintptr_t kOriginalVehiclePoolPtr = 0x00B74494;
constexpr uintptr_t kOriginalBuildingPoolPtr = 0x00B74498;
constexpr uintptr_t kOriginalObjectPoolPtr = 0x00B7449C;
constexpr uintptr_t kOriginalDummyPoolPtr = 0x00B744A0;
constexpr uintptr_t kOriginalColModelPoolPtr = 0x00B744A4;
constexpr uintptr_t kTasksPoolPtr = 0x00B744A8;
constexpr uintptr_t kEventsPoolPtr = 0x00B744AC;
constexpr uintptr_t kPointRoutePoolPtr = 0x00B744B0;
constexpr uintptr_t kPatrolRoutePoolPtr = 0x00B744B4;
constexpr uintptr_t kNodeRoutePoolPtr = 0x00B744B8;
constexpr uintptr_t kTaskAllocatorPoolPtr = 0x00B744BC;
constexpr uintptr_t kCheatHashKeys = 0x008A5CC8;
constexpr size_t kCheatHashKeyCount = 92;
constexpr uintptr_t kPedModelInfoVtable = 0x0085BDC0;
constexpr uintptr_t kFlaNoCollisionErrorPatch = 0x00534134;
constexpr uintptr_t kFlaObjectInitCollisionPatch = 0x0059F8BE;
constexpr uintptr_t kCColAccelStartCachePoolRead = 0x005B31A5;
constexpr uintptr_t kCBuildingPoolNewEntry = 0x00403FA0;
constexpr uintptr_t kCDummyPoolNewEntry = 0x00532630;
constexpr uintptr_t kCIplStoreRemoveIplObjectPoolRead = 0x00404BB4;
constexpr uintptr_t kCReplayMarkEverythingAsNewPedPoolRead = 0x0045D436;
constexpr uintptr_t kCReplayMarkEverythingAsNewVehiclePoolRead = 0x0045D474;
constexpr uintptr_t kCEntryInfoNodePoolNewEntry = 0x00536D10;
constexpr uintptr_t kCColModelPoolNewEntry = 0x0040FB80;
constexpr uintptr_t kCEventPoolNewEntry = 0x004B5570;
constexpr uintptr_t kCPointRoutePoolNewEntry = 0x0041B5B0;
constexpr uintptr_t kCNodeRoutePoolNewEntry = 0x0041B710;
constexpr uintptr_t kCTaskAllocatorPoolNewEntry = 0x0069D8E0;
constexpr uintptr_t kCPedAttractorPoolNewEntry = 0x005EA9F0;
constexpr uintptr_t kCPedIntelligencePoolNewEntry = 0x00605EC0;
constexpr uintptr_t kGenericPoolNewEntry = 0x0061A500;
constexpr uintptr_t kCPoolsInitialise = 0x00550F10;
constexpr uintptr_t kCPtrNodeSingleLinkPoolNewEntry = 0x00552240;
constexpr uintptr_t kCPtrNodeDoubleLinkPoolNewEntry = 0x005522E0;
constexpr uintptr_t kCPtrListSingleAddItemNullWrite = 0x00533606;
constexpr uintptr_t kCPtrListDoubleAddItemNullWrite = 0x0053368B;
constexpr uintptr_t kCQuadTreeNodeAddItemNullWrite = 0x00552D06;
constexpr uintptr_t kRpClumpForAllAtomics = 0x00749B70;
constexpr uintptr_t kRpClumpForAllAtomicsNullClumpRead = 0x00749B7B;
constexpr uintptr_t kRpAnimBlendAllocateData = 0x004D5F50;
constexpr uintptr_t kRpAnimBlendAllocateDataWrite = 0x004D5F6F;
constexpr uintptr_t kRpAnimBlendClumpFillFrameArray = 0x004D64A0;
constexpr uintptr_t kRpAnimBlendClumpFillFrameArrayRead = 0x004D64AB;
constexpr uintptr_t kRpAnimBlendClumpInit = 0x004D6720;
constexpr uintptr_t kRwClumpAnimPluginOffset = 0x00B5F878;
constexpr uintptr_t kDefaultMatrixLinkList = 0x00B74288;
constexpr size_t kMatrixListHeadOffset = 0x00;
constexpr size_t kMatrixListTailOffset = 0x54;
constexpr size_t kMatrixListAllocatedHeadOffset = 0xA8;
constexpr size_t kMatrixListAllocatedTailOffset = 0xFC;
constexpr size_t kMatrixListFreeHeadOffset = 0x150;
constexpr size_t kMatrixListFreeTailOffset = 0x1A4;
constexpr size_t kMatrixLinkOwnerOffset = 0x48;
constexpr size_t kMatrixLinkPrevOffset = 0x4C;
constexpr size_t kMatrixLinkNextOffset = 0x50;
constexpr uintptr_t kCColStorePoolPtr = 0x00965560;
constexpr uintptr_t kPedIntelligencePoolPtr = 0x00B744C0;
constexpr uintptr_t kPedAttractorsPoolPtr = 0x00B744C4;
constexpr uintptr_t kGameOperatorNew = 0x0082119A;
constexpr uintptr_t kCObjectCreatePatch1 = 0x005A1FA1;
constexpr uintptr_t kCObjectCreatePatch2 = 0x005A2016;
constexpr uintptr_t kCPickupGiveUsAPickUpObject = 0x004567E0;
constexpr uintptr_t kCleoObjectCreateInlinePatch = 0x0059FB1E;
constexpr uintptr_t kOriginalRadarTrace = 0x00BA86F0;
constexpr uintptr_t kCRadarGetActualBlipArrayIndex = 0x00582870;
constexpr uintptr_t kFrontEndTargetBlipIndex = 0x00BA6774;
constexpr size_t kPedPoolSlotSize = 0x7C4; // CPools::ms_pPedPool stores CPool<CPed, CCopPed>.
constexpr uintptr_t kCWorldPlayers = 0x00B7CD98;
constexpr uintptr_t kCGameCurrentArea = 0x00B72914;
constexpr size_t kCPlayerInfoSize = 0x190;
constexpr size_t kCPlayerInfoPedOffset = 0x00;
constexpr size_t kCPlaceablePlacementPosOffset = 0x04;
constexpr size_t kCPlaceableMatrixOffset = 0x14;
constexpr size_t kCMatrixPositionOffset = 0x30;
constexpr size_t kCEntityRwObjectOffset = 0x18;
constexpr size_t kCEntityFlagsOffset = 0x1C;
constexpr size_t kCEntityScanCodeOffset = 0x2C;
constexpr size_t kCEntityAreaCodeOffset = 0x2F;
constexpr size_t kCEntityInfoOffset = 0x36;
constexpr size_t kCPhysicalFlagsOffset = 0x40;
constexpr size_t kCPhysicalCollisionListOffset = 0xB0;
constexpr size_t kCPhysicalMovingListOffset = 0xB4;
constexpr size_t kCPedFlagsOffset = 0x46C;
constexpr size_t kCPedCreatedByOffset = 0x484;
constexpr size_t kCPedStateOffset = 0x534;
constexpr size_t kCPedMoveStateOffset = 0x538;
constexpr size_t kCPedHealthOffset = 0x540;
constexpr size_t kCPedTypeOffset = 0x598;
constexpr uintptr_t kPopulationDontCreateRandomGangMembers = 0x00C0FCB2;
constexpr uintptr_t kPopulationOnlyCreateRandomGangMembers = 0x00C0FCB3;
constexpr uintptr_t kPopulationDontCreateRandomCops = 0x00C0FCB4;
constexpr uintptr_t kPopulationPedDensityMultiplier = 0x008D2530;
constexpr uintptr_t kPopulationMaxNumberOfPedsInUse = 0x008D2538;
constexpr uintptr_t kPopulationTotalMissionPeds = 0x00C0EC24;
constexpr uintptr_t kPopulationTotalPeds = 0x00C0EC28;
constexpr uintptr_t kPopulationTotalGangPeds = 0x00C0EC2C;
constexpr uintptr_t kPopulationTotalCivPeds = 0x00C0EC30;
constexpr uintptr_t kPopulationNumDealers = 0x00C0EC38;
constexpr uintptr_t kPopulationNumCops = 0x00C0EC68;
constexpr uintptr_t kPopulationNumCivFemale = 0x00C0EC6C;
constexpr uintptr_t kPopulationNumCivMale = 0x00C0EC70;
constexpr uintptr_t kPopulationCurrentWorldZone = 0x00C0FCBC;
constexpr uintptr_t kStreamingLoadedGangs = 0x008E4BAC;
constexpr uintptr_t kStreamingNumPriorityRequests = 0x008E4BA0;
constexpr uintptr_t kStreamingNumPedsLoaded = 0x008E4BB0;
constexpr uintptr_t kStreamingPedsLoaded = 0x008E4C00;
constexpr uintptr_t kStreamingCurrentZoneType = 0x008E4C20;
constexpr uintptr_t kStreamingNumModelsRequested = 0x008E4CB8;
constexpr uintptr_t kStreamingDisableStreaming = 0x009654B0;
constexpr uintptr_t kStreamingLoadingBigModel = 0x008E4A58;
constexpr uintptr_t kRendererLoadingPriority = 0x00B76850;
constexpr uintptr_t kCStreamingStreamZoneModels = 0x0040A560;
constexpr uintptr_t kCStreamingStreamZoneModelsGangs = 0x0040AA10;
constexpr uintptr_t kCStreamingStreamVehiclesAndPedsAlways = 0x0040B650;
constexpr uintptr_t kCStreamingStreamVehiclesAndPeds = 0x0040B700;
constexpr uintptr_t kCStreamingUpdate = 0x0040E670;
constexpr uintptr_t kCStreamingIsVeryBusy = 0x004076A0;
constexpr uintptr_t kHoodlumCStreamingIsVeryBusy = 0x0156D7A0;
constexpr uintptr_t kCGamePopulationUpdateBudgetCmp = 0x0053C00F;
constexpr uintptr_t kCGamePopulationUpdateBudgetImmediate = 0x0053C011;
constexpr uintptr_t kCCutsceneMgrCutsceneProcessing = 0x00B5F852;
constexpr uintptr_t kCReplayMode = 0x00A43088;
constexpr uintptr_t kPopCycleNumOtherPeds = 0x00C0BC40;
constexpr uintptr_t kPopCycleNumCopsPeds = 0x00C0BC44;
constexpr uintptr_t kPopCycleNumGangsPeds = 0x00C0BC48;
constexpr uintptr_t kPopCyclePercOther = 0x00C0BC4C;
constexpr uintptr_t kPopCyclePercCops = 0x00C0BC50;
constexpr uintptr_t kPopCyclePercGangs = 0x00C0BC54;
constexpr uintptr_t kPopCycleCurrentZoneInfo = 0x00C0BC68;
constexpr uintptr_t kPopCycleCurrentZoneType = 0x00C0BC6C;
constexpr uintptr_t kPopCycleCurrentTimeOfWeek = 0x00C0BC70;
constexpr uintptr_t kPopCycleCurrentTimeIndex = 0x00C0BC74;
constexpr uintptr_t kPopCyclePercTypeGroup = 0x00C0BC78;
constexpr uintptr_t kPopCycleNumDealersPeds = 0x00C0E978;
constexpr uintptr_t kCheatsActive = 0x00969130;
constexpr size_t kCheatElvisIsEverywhere = 39;
constexpr size_t kCheatPedsAttackWithRockets = 40;
constexpr size_t kCheatBeachParty = 41;
constexpr size_t kCheatGangMembersEverywhere = 42;
constexpr size_t kCheatGangsControlStreets = 43;
constexpr size_t kCheatNinjaTheme = 44;
constexpr size_t kCheatSlutMagnet = 45;
constexpr size_t kCheatFunhouseTheme = 74;
constexpr size_t kCheatCountryTraffic = 79;
constexpr uintptr_t kScriptCommandHandlerTable = 0x008A6168;
constexpr uintptr_t kProcessCommands600To699 = 0x0047F370;
constexpr uintptr_t kProcessCommands900To999 = 0x00483BD0;
constexpr uintptr_t kProcessCommands100To199 = 0x00466DE0;
constexpr uint32_t kCommandHandlerTableCount = 27;
constexpr uint32_t kCommandHandlerTableIndex600To699 = 6;
constexpr uint32_t kCommandHandlerTableIndex900To999 = 9;
constexpr uintptr_t kScriptParams = 0x00A43C78;
constexpr uintptr_t kCRunningScriptCollectParameters = 0x00464080;
constexpr uintptr_t kCRunningScriptStoreParameters = 0x00464370;
constexpr uintptr_t kCRunningScriptUpdateCompareFlag = 0x004859D0;
constexpr uintptr_t kCStreamingRequestSpecialModel = 0x00409D10;
constexpr uintptr_t kCStreamingSetMissionDoesntRequireModel = 0x00409C90;
constexpr uintptr_t kCStreamingRemoveModel = 0x004089A0;
constexpr size_t kRunningScriptLocalVarsOffset = 0x3C;
constexpr uintptr_t kThePaths = 0x0096F050;
constexpr uintptr_t kCPathFindSetPathsNeededAtPosition = 0x0044DCD0;
constexpr uintptr_t kCWorldFindGroundZForCoord = 0x00569660;
constexpr uintptr_t kCPoolsGetVehicle = 0x0054FFF0;
constexpr uintptr_t kCCarCtrlSetCoordsOfScriptCar = 0x004342A0;
constexpr uint16_t kCommandGetClosestCarNodeWithHeading = 0x03D3;
constexpr uint16_t kCommandSetCarCoordinates = 0x00AB;
constexpr uint16_t kCommandUnloadSpecialCharacter = 296;
constexpr uint16_t kCommandUnloadSpecialCharacterHexAlias = 0x0296;
constexpr DWORD kCleoCallbackScriptOpcodeProcessBefore = 8;
constexpr signed char kCleoOpcodeResultNone = -2;
constexpr signed char kCleoOpcodeResultContinue = 0;
constexpr int kScriptParamGlobalNumberVariable = 2;
constexpr int kScriptParamLocalNumberVariable = 3;
constexpr int kScriptParamGlobalNumberArray = 7;
constexpr int kScriptParamLocalNumberArray = 8;
constexpr size_t kRadarTraceSize = 0x28;
constexpr size_t kRadarTracePositionOffset = 0x08;
constexpr size_t kRadarTraceEntityHandleOffset = 0x04;
constexpr size_t kRadarTraceCounterOffset = 0x14;
constexpr size_t kRadarTraceSpriteOffset = 0x24;
constexpr size_t kRadarTraceFlagOffset = 0x25;
constexpr uint8_t kRadarTraceTrackingFlag = 0x02;
constexpr uint8_t kRadarSpriteWaypoint = 41;
constexpr uint32_t kOriginalRadarTraceCount = 175;
constexpr uint32_t kLegacyCleoRadarTraceShadowSlot = kOriginalRadarTraceCount - 1;
struct OlaSaOverlapSpec {
    const char* key;
    const char* flaKey;
    const char* owner;
};
struct RuntimeVec3 {
    float x;
    float y;
    float z;
};
struct ZoneStreamingCheatFlag {
    size_t index;
    uint32_t mask;
};
constexpr ZoneStreamingCheatFlag kZoneStreamingCheatFlags[] = {
    { kCheatElvisIsEverywhere, 0x0001 },
    { kCheatPedsAttackWithRockets, 0x0002 },
    { kCheatBeachParty, 0x0004 },
    { kCheatGangMembersEverywhere, 0x0008 },
    { kCheatNinjaTheme, 0x0010 },
    { kCheatSlutMagnet, 0x0020 },
    { kCheatFunhouseTheme, 0x0040 },
    { kCheatCountryTraffic, 0x0080 },
};
struct PedModelCounter {
    int32_t modelId;
    uint32_t count;
};
using CleoOpcodeTrampolineFn = CleoExports::OpcodeResult(__stdcall*)(RunningScriptLite*);
struct LazyCPoolSpec {
    const char* label;
    uintptr_t poolPtr;
    uintptr_t ctor;
    uintptr_t namePtr;
    const char* iniKey;
    uint32_t defaultCapacity;
    LONG state;
};
struct SanPabloSpecialActor {
    uint32_t modelId;
    const char* name;
};
constexpr SanPabloSpecialActor kSanPabloSpecialActors[] = {
    { 290, "SWEET" },
    { 291, "RYDER" },
    { 292, "SMOKEV" },
    { 293, "ZERO" },
    { 294, "CESAR" },
};
struct SpecialActorCatalogEntry {
    uint32_t code;
    const char* name;
};
constexpr SpecialActorCatalogEntry kBuiltInSpecialActorCatalog[] = {
    { 1, "SWEET" },
    { 2, "RYDER" },
    { 3, "SMOKE" },
    { 4, "SMOKEV" },
    { 5, "CESAR" },
    { 6, "ZERO" },
    { 7, "TENPEN" },
    { 8, "PULASKI" },
    { 9, "TRUTH" },
    { 10, "OGLOC" },
    { 11, "KENDL" },
    { 12, "JIZZY" },
    { 13, "MADDOGG" },
    { 14, "MACCER" },
    { 15, "WUZIMU" },
    { 16, "EMMET" },
    { 17, "JETHRO" },
    { 18, "JANITOR" },
    { 19, "CLAUDE" },
    { 20, "FORELLI" },
    { 21, "ANDRE" },
    { 22, "BB" },
    { 23, "BBTHIN" },
    { 24, "CAT" },
    { 25, "CROGRL1" },
    { 26, "CROGRL2" },
    { 27, "CROGRL3" },
    { 28, "DNB1" },
    { 29, "DNB2" },
    { 30, "DNB3" },
    { 31, "ROSE" },
    { 32, "SUZIE" },
    { 33, "TBONE" },
    { 34, "TORINO" },
    { 35, "HMOGAR" },
    { 36, "RYDER2" },
    { 37, "RYDER3" },
};
template <typename T>
T ResolveProcByNameOrOrdinal(HMODULE module, const char* name, WORD ordinal)
{
    FARPROC proc = GetProcAddress(module, name);
    if (!proc) {
        proc = GetProcAddress(module, MAKEINTRESOURCEA(ordinal));
    }
    return reinterpret_cast<T>(proc);
}
struct KnownCleoPlusVersion {
    uint32_t textHash;
    int32_t  objectOffset;
    int32_t  vehicleOffset;
    int32_t  pedOffset;
};
// Batch 5: re-audited against the 2026-07-03 ProperShaders build
// (1,534,592 bytes, .text CRC32). Matches the bridge-side gate so the
// CStreaming rewrite, fixed patch and AddTxdSlot verified-rva re-engage.
constexpr uint32_t kSupportedProperShadersTextHash = 0x526DBD01;

// ---- globals (defined in BridgeGlobals.cpp) ----
extern char g_logPath[MAX_PATH];
extern char g_configPath[MAX_PATH];
extern char g_flaLogPath[MAX_PATH];
extern char g_flaIniPath[MAX_PATH];
extern LONG g_bridgeConfigOpenRetries;
extern LONG g_flaRuntimeRecoveryStarted;
extern BridgeConfig g_config;
extern FakeColModelForBounds g_fakeColModelForBounds;
extern LONG g_exceptionLogCount;
extern uintptr_t g_relocatedCModelInfoPtrs;
extern uintptr_t g_relocatedStreamingInfo;
extern uintptr_t g_relocatedStreamingInfoExtension;
extern uintptr_t g_relocatedAnimBlocks;
extern uintptr_t g_relocatedVehicleRecordingStreamingArray;
extern uintptr_t g_relocatedStreamedScripts;
extern uintptr_t g_relocatedHandlingManager;
extern uintptr_t g_relocatedRegisteredKills;
extern uint32_t g_fileIdCapacity;
extern uint32_t g_flaCompatFlags;
extern uint32_t g_flaAbiVersion;
extern uint32_t g_runtimeStateSource;
extern uint32_t g_pedPoolCapacity;
extern uint32_t g_vehiclePoolCapacity;
extern uint32_t g_objectPoolCapacity;
extern uint32_t g_buildingPoolCapacity;
extern uint32_t g_dummyPoolCapacity;
extern uint32_t g_colModelPoolCapacity;
extern uint32_t g_collisionStoreCapacity;
extern uint32_t g_colAccelFakePool[16];
extern LONG g_colAccelStartCachePoolGuardLogs;
extern LONG g_colModelPoolNewGuardLogs;
extern LONG g_cPoolsInitialiseRecoveryLogs;
extern LONG g_cPoolsInitialiseRecoveryState;
extern LONG g_cPoolsInitialiseDispatchState;
extern LONG g_batchLazyCPoolInitialiseLogs;
extern LONG g_batchLazyCPoolInitialiseState;
extern LONG g_lazyCPoolBatchDepth;
extern LONG g_ptrNodeExhaustionGuardLogs;
extern LONG g_rwClumpForAllAtomicsGuardLogs;
extern LONG g_rpAnimBlendClumpInitGuardLogs;
extern PVOID g_vectoredExceptionHandlerHandle;
extern DWORD g_gameThreadId;
extern uintptr_t g_chainedStreamingBusyTarget;
extern uintptr_t g_cPoolsInitialiseReplayTrampoline;
extern FlaExtendedIdApi g_flaExtendedIdApi;
extern uintptr_t g_cleoDispatchNullGuardReturn;
extern uintptr_t g_cleoThunk26720Original;
extern uintptr_t g_cleoPlusInitExtendedObjectVarsTrampoline;
extern uintptr_t g_cleoPlusSetExtendedObjectVarTrampoline;
extern uintptr_t g_cleoPlusGetExtendedObjectVarTrampoline;
extern uintptr_t g_cleoPlusObjectAllocateBlocksContinue;
extern uintptr_t g_cleoPlusVehicleAllocateBlocksContinue;
extern uintptr_t g_cleoPlusPedAllocateBlocksContinue;
extern uintptr_t g_mixSetsPedAllocateBlocksContinue;
extern uintptr_t g_urbanizePedAllocateBlocksContinue;
extern uintptr_t g_vehFuncsVehicleAllocateBlocksContinue;
extern LONG g_vehFuncsPoolGuardInstallerStarted;
extern uintptr_t g_animUncompressContinue;
extern uintptr_t g_animStaticAssocInitContinue;
extern uintptr_t g_animUpdateBlendContinue;
extern uintptr_t g_animFrameUpdateSkinnedContinue;
extern uintptr_t g_animFrameUpdateSkinnedVelocityContinue;
extern uintptr_t g_animBlendGroupContinue;
extern uintptr_t g_animClumpFinalizeNodesContinue;
extern uintptr_t g_animClumpFinalizeNodesEmpty;
extern uintptr_t g_animDestroyAssociationsContinue;
extern uintptr_t g_animRemoveBlockContinue;
extern uintptr_t g_rwClumpForAllAtomicsContinue;
extern uintptr_t g_rpAnimBlendClumpInitContinue;
extern std::atomic<uintptr_t> g_rwClumpExecutableCallbacks[kRwClumpCallbackCacheSize];
extern uintptr_t g_shouldModelBeStreamedContinue;
extern uintptr_t g_shouldModelBeStreamedReturnFalse;
extern uintptr_t g_flaTrainTypeCarriagesLoaderThis;
extern uintptr_t g_flaTrainTypeCarriagesLoadFunc;
extern uintptr_t g_widescreenFixSpriteNameGuardContinue;
extern uintptr_t g_widescreenFixSpriteNameGuardSkip;
extern uintptr_t g_flaAddTxdSlotThunk;
extern uintptr_t g_properShadersAddTxdSlotThunk;
extern uintptr_t g_lastValidAnimHierarchy;
extern uintptr_t g_neutralizedAnimAssociations[kMaxNeutralizedAnimAssociations];
extern LONG g_neutralizedAnimAssociationCursor;
extern AnimLifecycleRecord g_animLifecycleHistory[kAnimLifecycleHistorySize];
extern LONG g_animLifecycleCursor;
extern uintptr_t g_radarTraceBase;
extern uint32_t g_radarTraceLimit;
extern alignas(16) uint32_t g_boundCentreScratch[4];
extern CRITICAL_SECTION g_deferredPoolAllocateLock;
extern DeferredPoolAllocate g_deferredPoolAllocates[kMaxDeferredPoolAllocates];
extern uint32_t g_deferredPoolAllocateCount;
extern LONG g_deferredPoolAllocateLogs;
extern LONG g_deferredPoolAllocateReplayActive;
extern ScriptCommandHandlerFn g_originalCommands900To999;
extern ScriptCommandHandlerFn g_originalCommands600To699;
extern uintptr_t g_commands900To999TableSlot;
extern uintptr_t g_commands600To699TableSlot;
extern LONG g_closestCarNode03D3FallbackLogs;
extern LONG g_sanPabloSpecialActorBridgeLogs;
extern LONG g_sanPabloSpecialActorBridgeLastLogState;
extern LONG g_specialActorSlotRangeLogs;
extern LONG g_forwardingCommands600To699;
extern LONG g_forwardingCommands900To999;
extern LONG g_sanPabloCleoOpcodeCallbackInstalled;
extern LONG g_sanPabloCleoOpcodeRegistered;
extern ProcessCommandsGroupFn g_originalProcessCommands100To199;
extern uintptr_t g_processCommands100To199Trampoline;
extern LONG g_taxi77SetCarCoordinatesLogs;
extern float g_lastTargetBlipX;
extern float g_lastTargetBlipY;
extern float g_lastTargetBlipZ;
extern LONG g_hasLastTargetBlipCoords;
extern CleoExports g_cleoExports;
extern uintptr_t g_taxi77WatchdogMainPtr;
extern uint32_t g_taxi77WatchdogActiveSince;
extern uint32_t g_taxi77WatchdogLastLog;
extern uint32_t g_taxi77WatchdogLastOffset;
extern LONG g_taxi77WatchdogRecoveries;
extern RuntimeSpecialActorName g_runtimeSpecialActorNames[kMaxRuntimeSpecialActorNames];
extern uint32_t g_runtimeSpecialActorNameCount;
extern RuntimeRewriteRule g_runtimeRewriteRules[kMaxRuntimeRewriteRules];
extern uint32_t g_runtimeRewriteRuleCount;
extern const OlaSaOverlapSpec kOlaSaOverlapSpecs[38];
extern LazyCPoolSpec g_lazyCPoolSpecs[18];
extern const KnownCleoPlusVersion kKnownCleoPlusVersions[2];
extern const size_t kKnownCleoPlusVersionCount;

// ---- internal functions ----
bool ShouldLogSanPabloSpecialActorBridge(long count, int mode, int result);
#if defined(_M_IX86)
extern "C" void Bridge_CObject_Create_5A1FA1();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_CObject_Create_5A2016();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_Cleo_Dispatch_NullGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_Cleo_Thunk26720_NullGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_CleoPlus_ObjectAllocateBlocks_PoolGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_CleoPlus_VehicleAllocateBlocks_PoolGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_CleoPlus_PedAllocateBlocks_PoolGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_MixSets_PedAllocateBlocks_PoolGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_CPoolsInitialise_ReplayHook();
#endif
#if defined(_M_IX86)
extern "C" void __stdcall Bridge_InvokePoolAllocateContinue(uintptr_t continueAddress, uintptr_t thisPtr, uintptr_t poolPtr);
#endif
#if defined(_M_IX86)
extern "C" void Bridge_ProperShadersAddTxdSlot_Combined();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimUncompress_NullGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimStaticAssocInit_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimUpdateBlend_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimBlendGroup_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimFrameUpdateSkinnedVelocity_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimFrameUpdateSkinned_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimClumpFinalizeNodes_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimDestroyAssociations_Diagnostic();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_AnimRemoveBlock_Diagnostic();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_RpAnimBlendClumpInit_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_RpClumpForAllAtomics_Guard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_ShouldModelBeStreamed_ColModelGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_GetBoundCentre_NullGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_GetBoundRect_ColModelGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_FlaTrainInit_LoadPreserveEax();
#endif
#if defined(_M_IX86)
extern "C" int __stdcall Bridge_IsReadableMemory(uintptr_t address, size_t size);
#endif
#if defined(_M_IX86)
extern "C" void Bridge_TxdLoadDictionaryWriteGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_RwTexDictionaryFindNamedTexture_DictGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_RwTexDictionaryFindNamedTexture_NameGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_RwTexDictionaryFindNamedTexture_LinkGuard();
#endif
#if defined(_M_IX86)
extern "C" void Bridge_WidescreenFixSpriteNameGuard();
#endif
void ResolveGameRelativePath(const char* relativePath, char* out, size_t outSize);
void ResolveGamePath(const char* path, char* out, size_t outSize);
void InitializeBridgeFilePaths();
void Log(const char* fmt, ...);
const char* BaseName(const char* path);
bool ReadSmallTextValue(const char* filePath, const char* key, char* out, size_t outSize);
bool ReadSectionTextValue(const char* filePath, const char* section, const char* key, char* out, size_t outSize);
void WriteDefaultBridgeConfigIfMissing();
bool ReadBridgeBool(const char* key, bool defaultValue);
int ReadBridgeInt(const char* key, int defaultValue, int minValue, int maxValue);
uint32_t ReadBridgeU32(const char* key, uint32_t defaultValue);
void ReadBridgeText(const char* key, char* out, size_t outSize, const char* defaultValue = "");
int ReadFlaInt(const char* key, int defaultValue, int minValue, int maxValue);
void LoadBridgeConfig();
void LogBridgeConfig();
void LogIniValue(const char* key);
bool FileNameLooksLikeNodeDat(const char* name, uint32_t* slotOut);
void ScanLooseNodeDatFiles(const char* root, int depth, uint32_t* count, uint32_t* maxSlot);
void LogFlaPathNodeDiagnostics();
bool IniValueLooksEnabled(const char* value);
bool FlaOwnsOlaSaLimit(const OlaSaOverlapSpec& spec);
const OlaSaOverlapSpec* FindOlaSaOverlapSpec(const char* key);
bool TokenListContainsExact(const char* rawList, const char* value);
bool ExtractIniSectionName(const char* line, size_t len, char* out, size_t outSize);
bool ExtractIniKeyName(const char* line, size_t len, char* out, size_t outSize);
bool AppendText(char* output, size_t capacity, size_t* used, const char* text, size_t len);
bool ExtractModloaderIgnoreEntry(const char* line, size_t len, char* out, size_t outSize);
void GuardOpenLimitAdjusterModuleLoad(const char* phase);
void GuardOpenLimitAdjusterSaLimitsAtPath(const char* path, const char* phase);
void GuardOpenLimitAdjusterSaLimits(const char* phase);
void AuditOpenLimitAdjusterSaOverlapsAtPath(const char* path);
void AuditOpenLimitAdjusterSaOverlaps();
bool IsOpenLimitAdjusterAddress(uintptr_t address);
bool PatchEntryLooksOpenLimitAdjusterOwned(uintptr_t address);
bool RestoreOpenLimitAdjusterPatchIfOwned(const char* label, uintptr_t address, const uint8_t* originalBytes, size_t size, const char* phase);
int RepairOpenLimitAdjusterSaPoolHooks(const char* phase);
DWORD WINAPI OpenLimitAdjusterRepairThread(void*);
bool ReadLogAddress(const char* prefix, uintptr_t* out);
bool SyncShadowTable(const char* label, const char* logPrefix, uintptr_t originalAddress, size_t count, size_t elementSize);
bool SyncLegacyModelInfoPointers();
bool SyncLegacyStreamingInfo();
DWORD WINAPI LegacyShadowThread(void*);
bool FileContainsPattern(const char* path, const uint8_t* pattern, size_t patternSize);
bool ReadLogAddress_Old(const char* prefix, uintptr_t* out);
DWORD FindProcessMainThreadId();
uintptr_t ModuleBaseFromAddress(uintptr_t address, char* moduleName, size_t moduleNameSize);
const char* ProtectName(DWORD protect);
const char* StateName(DWORD state);
const char* TypeName(DWORD type);
void LogMemoryRegion(const char* label, uintptr_t address);
void LogStackModules(uintptr_t esp);
bool SafeReadU32(uintptr_t address, uint32_t* out);
bool SafeReadU8(uintptr_t address, uint8_t* out);
bool SafeReadF32(uintptr_t address, float* out);
void LogDwords(const char* label, uintptr_t address, int count);
void LogBytes(const char* label, uintptr_t address, size_t count);
void LogOneRelocatedAddress(const char* label, const char* logPrefix, uintptr_t originalAddress);
void LogRelocatedAddressDiagnostics();
void LogOnePoolPointer(const char* label, uintptr_t pointerAddress);
void LogPoolPointerDiagnostics();
bool ReadPoolByteMap(uintptr_t pool, uintptr_t* objects, uintptr_t* byteMap, uint32_t* size, uint32_t* firstFree);
void LogPopulationPoolUsage(const char* label, uintptr_t poolPtrAddress);
int ReadRuntimeByteForLog(uintptr_t address);
uint32_t ReadRuntimeU32ForLog(uintptr_t address);
float ReadRuntimeFloatForLog(uintptr_t address);
bool IsReasonableRuntimeFloat(float value);
bool IsReasonableRuntimeVec3(const RuntimeVec3& value);
bool SafeReadVec3(uintptr_t address, RuntimeVec3* out);
bool ReadEntityPosition(uintptr_t entity, RuntimeVec3* out, uintptr_t* matrixOut, bool* fromMatrixOut);
uintptr_t ReadFocusedPlayerPed();
float DistanceSquared2D(const RuntimeVec3& a, const RuntimeVec3& b);
uint32_t ReadZoneStreamingCheatMaskForLog();
bool ClearZoneStreamingCheatsForMask(uint32_t mask);
void LogPopulationRuntimeState(const char* reason, int sampleIndex);
void AddPedModelCounter(PedModelCounter* counters, size_t count, int32_t modelId);
void AppendTopPedModels(char* output, size_t outputSize, PedModelCounter* counters, size_t counterCount);
bool IsPedRaceAllowedByCurrentZone(uint32_t race, uint8_t zoneRaces, bool hasZoneInfo);
void AppendCurrentPopcycleGroupRow(char* output, size_t outputSize);
void LogPopulationStreamingPedSlots(const char* reason, int sampleIndex);
void LogPedPoolModelTypeSummary(const char* reason, int sampleIndex);
DWORD WINAPI PopulationPoolDiagnosticsThread(void*);
DWORD WINAPI GangOnlyPopulationGuardThread(void*);
void LogOneStreamingPedFunctionEntry(const char* reason, const char* label, uintptr_t address);
void LogStreamingPedFunctionEntryDiagnostics(const char* reason);
DWORD WINAPI PedStreamingZoneRepairThread(void*);
uint32_t ReadIniU32(const char* key, uint32_t defaultValue);
uint32_t CalculateFileIdCapacityFromIni();
HMODULE FindFlaModule();
bool ResolveFlaExtendedIdApi();
bool IsFlaDifficultHighIdMode();
int32_t ReadExtendedIdFrom16BitField(const void* field);
bool RefreshFlaRuntimeStateFromAbi();
void RefreshFlaRuntimeState(bool logState = true);
bool TryRecoverCriticalFlaRuntimeAddresses();
DWORD WINAPI FlaRuntimeStateRecoveryThread(void*);
void StartFlaRuntimeStateRecovery();
uintptr_t SafeModelInfoEntryAddress(uint32_t modelId);
uintptr_t SafeStreamingInfoEntryAddress(uint32_t modelId);
void DumpModelContext(uint32_t modelId);
bool IsModelRwObjectLoaded(uint32_t modelId);
bool TokenMatchesPickupModel(uint32_t modelId, const char* token);
bool PickupModelGuardAllows(uint32_t modelId);
bool TryLoadPickupModelNow(uint32_t modelId);
bool ShadowPickupModelToLegacyTables(uint32_t modelId);
extern "C" int __stdcall Bridge_ShouldCreatePickupObject(uintptr_t pickup);
extern "C" void Bridge_CPickup_GiveUsAPickUpObject_Guard();
extern "C" bool __stdcall Bridge_HasValidBoundRectColModel(uintptr_t entity);
extern "C" void __stdcall Bridge_FillFallbackBoundRect(uintptr_t entity, BridgeRect* outRect);
extern "C" void __stdcall Bridge_LogInvalidShouldModelBeStreamedColModel(uintptr_t entity, uintptr_t modelInfo, uintptr_t colModel, uintptr_t stack);
uint8_t GetStreamingLoadState(uint32_t modelId);
bool IsStreamingModelDefined(uint32_t modelId);
bool IsPedModelInfo(uint32_t modelId);
DWORD WINAPI PreloadUrbanizePedModelsThread(void*);
bool IsReadableCommitted(uintptr_t address, size_t size);
bool IsWritableCommitted(uintptr_t address, size_t size);
uintptr_t MatrixListBase();
uintptr_t MatrixListHead();
uintptr_t MatrixListTail();
uintptr_t MatrixListAllocatedHead();
uintptr_t MatrixListAllocatedTail();
uintptr_t MatrixListFreeHead();
uintptr_t MatrixListFreeTail();
uintptr_t MatrixListDynamicTailPrevAddress();
uintptr_t MatrixListStaticTailPrevAddress();
bool IsMatrixListSentinel(uintptr_t link);
bool BytePatternMatches(uintptr_t address, const uint8_t* bytes, size_t size);
bool MatrixGuardCodeLooksCompatible(uintptr_t eip);
bool MoveMatrixLinkToFreeList(uintptr_t link, uintptr_t expectedOwner, uintptr_t* recoveredLink, uintptr_t* recoveredOwner);
bool RecoverOldestMatrixLinkToFreeList(uintptr_t expectedOwner, uintptr_t* recoveredLink, uintptr_t* recoveredOwner, const char** sourceList);
bool AllocateStaticMatrixLinkFromFreeList(uintptr_t owner, uintptr_t* allocatedLink);
uintptr_t ResolvePlaceableRemoveMatrixReturnAddress(uintptr_t esp);
extern "C" bool __stdcall Bridge_IsWritableMemory(uintptr_t address, size_t size);
extern "C" bool __stdcall Bridge_IsExecutableMemory(uintptr_t address);
uintptr_t ResolveCleoThunk26720FinalTarget(uintptr_t dispatchObject, uintptr_t thunkTarget, bool logInvalid);
bool IsWritableVectorArray(uintptr_t address, size_t count);
bool LooksLikeStdVectorTriplet(uintptr_t first, uintptr_t last, uintptr_t end);
bool ClearCleoPlusScriptEventsAt(uintptr_t eventsBase, const char* reason);
void ClearCleoPlusScriptEvents(const char* reason);
extern "C" uintptr_t __stdcall Bridge_GetSafeCleoThunkTarget(uintptr_t target);
extern "C" uintptr_t __stdcall Bridge_GetSafeCleoDispatchTarget(uintptr_t dispatchObject);
int __stdcall Bridge_CleoDispatchExceptionFilter(EXCEPTION_POINTERS* info, uintptr_t target, uintptr_t dispatchObject);
extern "C" void __stdcall Bridge_CallCleoDispatchTargetSafely(uintptr_t target, uintptr_t dispatchObject);
CleoExports::OpcodeResult CallCleoPlusExtendedObjectVarSafely( const char* name, uintptr_t trampoline, RunningScriptLite* thread);
extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_InitExtendedObjectVars_Guard(RunningScriptLite* thread);
extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_SetExtendedObjectVar_Guard(RunningScriptLite* thread);
extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_GetExtendedObjectVar_Guard(RunningScriptLite* thread);
extern "C" void __stdcall Bridge_LogInvalidTxdLoadDictionaryWrite(uintptr_t slot, uintptr_t dictionary, uintptr_t index);
extern "C" void __stdcall Bridge_LogInvalidBoundCentreOut(uintptr_t entity, uintptr_t outVec, uintptr_t returnAddress, uintptr_t stack);
extern "C" bool __stdcall Bridge_IsValidAnimHierarchy(uintptr_t hierarchy);
extern "C" void __stdcall Bridge_LogInvalidAnimHierarchy(uintptr_t hierarchy, uintptr_t returnAddress, uintptr_t stack);
extern "C" bool __stdcall Bridge_IsValidStaticAssociation(uintptr_t staticAssociation);
void MarkNeutralizedAnimAssociation(uintptr_t association);
bool ConsumeNeutralizedAnimAssociation(uintptr_t association);
bool IsNeutralizedAnimAssociation(uintptr_t association);
bool LooksLikeNeutralizedAnimAssociation(uintptr_t association);
bool ReadAnimGroupSnapshot( uintptr_t group, uintptr_t* associationsOut, uint32_t* countOut, uint32_t* idOffsetOut, uintptr_t* animBlockOut, uint32_t* groupIndexOut);
void LogInvalidStaticAssociationOwnership(const char* reason, uintptr_t staticAssociation);
extern "C" void __stdcall Bridge_LogAnimGroupDestroy(uintptr_t group, uintptr_t returnAddress);
extern "C" void __stdcall Bridge_LogAnimBlockRemove(uint32_t blockIndex, uintptr_t returnAddress);
extern "C" void __stdcall Bridge_LogEmptyAnimUpdate(uintptr_t updateData, uintptr_t clump);
extern "C" bool __stdcall Bridge_ShouldBlockGroupBlendAnimation(uint32_t groupId, uint32_t animId);
extern "C" void __stdcall Bridge_RepairInvalidStaticAssociation(uintptr_t runtimeAssociation, uintptr_t staticAssociation);
extern "C" bool __stdcall Bridge_PrepareAnimAssociationUpdate(uintptr_t association);
extern "C" bool __stdcall Bridge_PrepareAnimFrameUpdateData(uintptr_t updateData);
extern "C" bool __stdcall Bridge_ShouldSkipRpAnimBlendClumpInit(uintptr_t clump, uintptr_t returnAddress, uintptr_t stack);
bool IsCachedExecutableRwClumpCallback(uintptr_t callback);
extern "C" bool __stdcall Bridge_IsSafeRpClumpForAllAtomicsCall( uintptr_t clump, uintptr_t callback, uintptr_t data, uintptr_t returnAddress, uintptr_t stack);
bool CopyMemoryWithProtect(uintptr_t destination, uintptr_t source, size_t size);
bool WriteBytesWithProtect(uintptr_t destination, const uint8_t* bytes, size_t size);
uint32_t GetUppercaseKeyViaGame(const char* text);
void ReverseString(const char* input, char* output, size_t outputSize);
void StripLineCommentAndCommas(char* line);
void LoadBridgeCheatStrings();
void InstallFlaTrainInitHookRepair();
uintptr_t DecodeRel32JumpTarget(uintptr_t address);
bool IsExecutableCommitted(uintptr_t address);
bool IsValidCPool(uintptr_t pool);
bool ReadCPoolHeader(uintptr_t pool, uint32_t* size, uint32_t* firstFree);
bool IsPoolReadyForDeferredReplay(uintptr_t pool);
bool ReadCorePoolPointers(uint32_t* pedPool, uint32_t* vehiclePool, uint32_t* objectPool, uint32_t* colModelPool);
bool AreCorePoolsReadyForDeferredReplay(uint32_t* pedOut, uint32_t* vehicleOut, uint32_t* objectOut, uint32_t* colModelOut);
bool TryEnsureCPoolsInitialised(const char* reason, bool allowEarlyRecovery = false);
LazyCPoolSpec* FindLazyCPoolSpec(uintptr_t poolPtr);
bool EnsureLazyCPoolReady(uintptr_t poolPtr, const char* reason);
void PumpDeferredPoolAllocatesOnGameThread(uint32_t maxCount);
extern "C" void __cdecl Bridge_PumpDeferredPoolAllocatesAfterCorePoolInit();
bool EnsureLazyCoreCPoolsReady(const char* reason);
bool EnsureBatchLazyCPoolsInitialised(const char* reason, bool forceRetry = false);
bool WriteRel32Jump(uintptr_t source, uintptr_t target);
extern "C" bool __cdecl Bridge_CStreaming_IsVeryBusy();
void InstallStreamingBusyThresholdPatch();
void InstallPopulationUpdateBudgetPatch();
void InstallRwClumpForAllAtomicsGuard();
void InstallShouldModelBeStreamedGuard();
void InstallRpAnimBlendClumpInitGuard();
uintptr_t CreateRel32Trampoline(uintptr_t source, size_t stolenBytes);
void InstallCPoolsInitialiseReplayHook();
float AbsFloat(float value);
bool IsReasonableWorldCoord(float value);
bool IsNearWorldOrigin2D(float x, float y);
bool IsNearWorldOrigin2DLoose(float x, float y);
bool IsValidScriptCoord3D(float x, float y, float z);
void RequestPathStreamingForCoord(float x, float y, float z);
float SafeFindGroundZForCoord(float x, float y, float fallback);
bool IsScriptNamed(const RunningScriptLite* script, const char* expected);
void UpdateScriptCompareFlag(RunningScriptLite* script, bool state);
bool RequestSanPabloSpecialActors();
const char* GetBuiltInSpecialActorName(uint32_t modelId);
const char* GetBuiltInSpecialActorCatalogName(uint32_t actorCode);
bool IsBuiltInSpecialActorCatalogName(const char* name);
bool NormalizeSpecialActorName(const char* input, char* out, size_t outSize);
bool AutoScanFilterAllowsName(const char* name);
bool AddRuntimeSpecialActorName(const char* rawName, const char* source);
const char* GetRuntimeSpecialActorCatalogName(uint32_t actorCode);
bool ExtractImgEntryBaseName(const uint8_t* bytes, size_t size, const char* extension, char* out, size_t outSize);
bool ImgArchiveHasTxdName(const char* imgPath, const char* wantedName);
void ScanSpecialActorsFromImgArchive(const char* imgPath, const char* sourceLabel);
bool HasSiblingTxdForDff(const char* dffPath, const char* baseName);
void ScanSpecialActorsFromDirectory(const char* root, const char* sourceLabel, int depth = 0);
void BuildSpecialActorRuntimeCatalog();
DWORD WINAPI SpecialActorRuntimeCatalogThread(void*);
bool IsSpecialActorSlotUsable(uint32_t modelId);
bool ReadConfiguredSpecialActorName(uint32_t modelId, uint32_t actorCode, char* out, size_t outSize);
bool RequestSpecialActorName(uint32_t modelId, const char* name);
bool RequestConfiguredSpecialActor(uint32_t modelId, uint32_t actorCode);
bool ReleaseConfiguredSpecialActor(uint32_t modelId);
bool DecodeGenericSpecialActorMode(int mode, int* op, uint32_t* modelId, uint32_t* actorCode);
bool AreSanPabloSpecialActorsLoaded();
bool ReleaseSanPabloSpecialActors();
bool IsNumberVariableOperandType(int operandType);
bool HandleSpecialActorBridgeMode(RunningScriptLite* script, int mode, int* outResult, int* outGenericOp, uint32_t* outGenericModelId, uint32_t* outGenericActorCode);
signed char WINAPI Bridge_CleoScriptOpcodeProcessBefore(RunningScriptLite* script, DWORD opcode);
signed char CallNativeCleoOpcode0296(RunningScriptLite* script);
signed char __stdcall Bridge_CleoOpcode0296(RunningScriptLite* script);
unsigned char __fastcall Bridge_ProcessCommands600To699(RunningScriptLite* script, void*, unsigned short commandID);
void InstallSanPabloSpecialActorBridge();
unsigned char __fastcall Bridge_ProcessCommands900To999(RunningScriptLite* script, void*, unsigned short commandID);
void InstallClosestCarNode03D3Fallback();
bool IsTaxi77Script(const RunningScriptLite* script);
bool SetLocalFloatFromParam(RunningScriptLite* script, const uint8_t* param, float value);
bool SetScriptCarCoordinatesSafe(uint32_t carHandle, float x, float y, float z);
unsigned char __fastcall Bridge_ProcessCommands100To199(RunningScriptLite* script, void*, int commandID);
void InstallTaxi77SetCarCoordinatesGuard();
DWORD ElapsedMs(uint32_t now, uint32_t then);
bool ResolveCleoExports();
bool IsCleoScriptUsable(RunningScriptLite* script);
RunningScriptLite* FindCleoScriptByName(const char* name);
RunningScriptLite* FindTaxi77MainScript();
RunningScriptLite* FindTaxi77MeterScript();
uint32_t GetCleoScriptOffset(RunningScriptLite* script);
bool ReadTaxi77Local(RunningScriptLite* script, uint32_t index, ScriptParamLite* out);
bool WriteTaxi77LocalDword(RunningScriptLite* script, uint32_t index, uint32_t value);
bool IsTaxi77ScriptProcessing(RunningScriptLite* script);
bool ResetTaxi77MainScriptToStart(RunningScriptLite* script, uint32_t offset, uint32_t local0);
DWORD WINAPI Taxi77StateWatchdogThread(void*);
uint32_t ReadPatchedRadarTraceLimit();
uintptr_t ReadPatchedRadarTraceBase();
void RefreshRadarTraceRuntimeState();
bool ReadU32(uintptr_t address, uint32_t* out);
bool IsCleoGameEntitiesCaller(uintptr_t returnAddress, char* moduleName, size_t moduleNameSize);
bool FindModuleRangeFromAddress(uintptr_t address, char* moduleName, size_t moduleNameSize, uintptr_t* moduleBase, uintptr_t* moduleEnd);
bool ModuleMemoryContainsU32(uintptr_t moduleBase, uintptr_t moduleEnd, uint32_t needle);
bool CallerAlreadyUsesRelocatedRadarTrace(uintptr_t returnAddress, const char* moduleName);
bool ReadRadarTraceFields(uintptr_t trace, float* outX, float* outY, float* outZ, uint16_t* outCounter, uint8_t* outSprite, uint8_t* outFlags);
void RememberTargetBlipCoords(float x, float y, float z);
bool IsSaneRadarCoord(float x, float y, float z);
bool FindRelocatedWaypointTrace(uint16_t wantedCounter, uint32_t* outIndex, uintptr_t* outTrace);
bool ShadowRadarTraceForLegacyCleo(uint32_t blip, uint32_t traceIndex, uintptr_t trace, uintptr_t returnAddress, int* outIndex);
bool ResolveTargetBlipByWaypointScan(uint32_t blip, uint16_t counter, uintptr_t returnAddress, int* outIndex);
extern "C" int __cdecl Bridge_CRadar_GetActualBlipArrayIndex(uint32_t blip);
void InstallRadarBlipHandleGuard();
void InstallOneCObjectCreateBridgeStub(const char* label, uintptr_t patchAddress, void* bridgeTarget);
void InstallPickupModelLoadGuard();
void InstallCObjectCreateBridgeStubs();
void RestoreCleoObjectCreateInlinePatch();
void InstallCleoDispatchNullGuard();
void InstallCleoThunk26720NullGuard();
bool InstallOneCleoPlusOpcodeFunctionGuard( const char* name, uintptr_t patchAddress, uintptr_t guardTarget, uintptr_t* trampolineOut);
void InstallCleoPlusExtendedObjectVarGuard();
extern "C" void __stdcall Bridge_DeferPoolAllocate(uintptr_t poolPtrAddress, uintptr_t continueAddress, uintptr_t thisPtr);
uintptr_t FindPatternInModuleText(HMODULE module, const uint8_t* pattern, const char* mask, size_t patternLen);
uint32_t CalculateModuleTextHash(HMODULE module);
uint32_t CalculateModuleFileTextHash(HMODULE module);
uintptr_t FindAllocateBlocksByPattern(HMODULE module, const char* typeName, uintptr_t expectedPoolPtr);
uintptr_t CreatePoolAllocateGuardStub(uintptr_t poolPtrAddress, uintptr_t continueAddress);
bool InstallOneCleoPlusPoolAllocateGuard( const char* name, uintptr_t patchAddress, uintptr_t expectedPoolPtr, uintptr_t guardTarget, uintptr_t* continueOut);
void InstallCleoPlusPoolAllocateGuard();
void InstallMixSetsPoolAllocateGuard();
HMODULE FindLoadedModuleBySubstring(const char* needle);
DWORD WINAPI MixSetsPoolAllocateGuardInstallThread(void*);
void InstallUrbanizePoolAllocateGuard();
DWORD WINAPI UrbanizePoolAllocateGuardInstallThread(void*);
uintptr_t FindVehFuncsVehicleAllocateBlocks(HMODULE vehFuncs);
void InstallVehFuncsPoolAllocateGuard();
DWORD WINAPI VehFuncsPoolAllocateGuardInstallThread(void*);
void StartVehFuncsPoolAllocateGuardInstaller();
void InstallAnimUncompressNullGuard();
void InstallAnimStaticAssocInitGuard();
void InstallAnimUpdateBlendGuard();
void InstallAnimBlendGroupGuard();
void InstallAnimFrameUpdateSkinnedVelocityGuard();
void InstallAnimFrameUpdateSkinnedGuard();
void InstallAnimEmptyUpdateGuard();
void InstallAnimLifecycleDiagnostics();
void InstallGetBoundCentreNullGuard();
void InstallGetBoundRectColModelGuard();
void RestoreFlaNoCollisionErrorPatch();
void RestoreFlaObjectInitCollisionPatch();
void ScanRiskyAddressConstants(const char* modulePath);
bool ContainsCaseInsensitive(const char* haystack, const char* needle);
bool ModuleMatchesRewriteList(const char* moduleName, const char* modulePath, const char* rawList);
bool ModuleDeniedByCompatPolicy(const char* moduleName, const char* modulePath);
bool ModuleAllowedByLegacyPolicy(const char* moduleName, const char* modulePath);
bool ModuleAllowedForAutoPoolGuard(const char* moduleName, const char* modulePath);
bool MemoryMatches(uintptr_t address, const uint8_t* bytes, size_t size);
bool IsExecutableRegion(const MEMORY_BASIC_INFORMATION& mbi);
bool BytesMatchInKnownReadableRegion(uintptr_t address, const uint8_t* bytes, size_t size);
bool HasBytesInRange(uintptr_t address, size_t rangeSize, const uint8_t* bytes, size_t bytesSize);
bool LooksLikePluginSdkPoolAllocateBlocks(uintptr_t address, uintptr_t expectedPoolPtr);
uint32_t ScanOneModuleForPoolAllocateGuards(const MODULEENTRY32& me, uint32_t remainingPatchBudget);
void InstallAutoPoolAllocateGuards();
bool ModuleDeniedForRuntimeRewrite(const char* moduleName, const char* modulePath);
bool ModuleAllowedForRuntimeRewrite(const char* moduleName, const char* modulePath);
RuntimeRewriteTarget ParseRuntimeRewriteTarget(const char* raw);
const char* RuntimeRewriteTargetName(RuntimeRewriteTarget target);
uint32_t ResolveRuntimeRewriteNewValue(const RuntimeRewriteRule& rule);
void AddRuntimeRewriteRule( const char* name, uint32_t oldValue, RuntimeRewriteTarget target, uint32_t staticNewValue, bool align4, bool executableOnly, bool auditOnly, uint32_t maxPatchesPerModule, const char* allowlist, const char* denylist);
void AddBuiltInRuntimeRewriteRules();
void LoadRuntimeRewriteRules();
bool IsExecutableProtect(DWORD protect);
bool RuleDeniedForRuntimeRewrite(const RuntimeRewriteRule& rule, const char* moduleName, const char* modulePath);
bool RuleAllowedForRuntimeRewrite(const RuntimeRewriteRule& rule, const char* moduleName, const char* modulePath);
bool ModuleInterestingForRuntimeRewrite(const char* moduleName, const char* modulePath);
void RewriteOneModuleConstants(const MODULEENTRY32& me, bool logDetails);
void RuntimeRewriteHardcodedAddressConstants(bool logDetails = true);
DWORD WINAPI RuntimeRewriteRescanThread(void*);
bool StackMentionsModule(uintptr_t esp, const char* needle);
bool FindStackValue(uintptr_t esp, uintptr_t needle, int maxDwords, uintptr_t* valueAddress);
bool IsFreeOrNonExecutableRegion(uintptr_t address, DWORD* stateOut, DWORD* protectOut = nullptr);
bool StackMentionsGtaScriptOpcodePath(uintptr_t esp);
bool BreakRepeatedNonExecutableExceptionLoop(EXCEPTION_POINTERS* info);
bool SkipReplayMarkEverythingAsNew(CONTEXT* ctx, const char* reason, bool restoreSavedEsi);
const char* KnownGameAddressName(uintptr_t eip);
void LogCrashClassification(EXCEPTION_POINTERS* info);
bool SkipFailedPtrNodeListAdd( CONTEXT* ctx, const char* label, uintptr_t poolPtrAddress, uintptr_t fault, uintptr_t returnAddressOffset, uintptr_t itemOffset, uintptr_t finalEspAdvance);
void LogModuleSnapshot();
void LogFLAExports();
LONG CALLBACK BridgeVectoredExceptionHandler(EXCEPTION_POINTERS* info);
bool IsSupportedProperShadersTextHash(uint32_t hash);
bool IsProperShadersAddTxdSlotThunk(HMODULE psAsi, uintptr_t target);
uintptr_t FindFlaAddTxdSlotThunk();
void InstallProperShadersVtableGuard();
void ApplyProperShadersCompat();
void InstallRwTexDictionaryFindNamedTextureGuard();
void InstallTxdLoadDictionaryWriteGuard();
void InstallWidescreenFixSpriteNameGuard();
bool IsProperShadersAddTxdSlotConflictPresent();
void InstallEarlyProperShadersCompatOnce(bool includeTextureGuards);
DWORD WINAPI EarlyProperShadersCompatThread(void*);
DWORD WINAPI BridgeThread(void*);

