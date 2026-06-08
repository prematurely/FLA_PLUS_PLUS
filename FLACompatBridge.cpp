#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <intrin.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_M_IX86)
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetApiVersion=_FLACompatBridge_GetApiVersion@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetFileIdCapacity=_FLACompatBridge_GetFileIdCapacity@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetCompatFlags=_FLACompatBridge_GetCompatFlags@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetRuntimeSource=_FLACompatBridge_GetRuntimeSource@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetRelocatedAddress=_FLACompatBridge_GetRelocatedAddress@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetRelocatedAddressByName=_FLACompatBridge_GetRelocatedAddressByName@8")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetModelInfoEntry=_FLACompatBridge_GetModelInfoEntry@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetModelInfo=_FLACompatBridge_GetModelInfo@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetStreamingInfo=_FLACompatBridge_GetStreamingInfo@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_ReadStreamingInfo=_FLACompatBridge_ReadStreamingInfo@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetPoolInfo=_FLACompatBridge_GetPoolInfo@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetPoolInfoByName=_FLACompatBridge_GetPoolInfoByName@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_RebuildSpecialActorCatalog=_FLACompatBridge_RebuildSpecialActorCatalog@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetSpecialActorName=_FLACompatBridge_GetSpecialActorName@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_RequestSpecialActor=_FLACompatBridge_RequestSpecialActor@8")
#pragma comment(linker, "/EXPORT:FLACompatBridge_RequestSpecialActorByCode=_FLACompatBridge_RequestSpecialActorByCode@8")
#pragma comment(linker, "/EXPORT:FLACompatBridge_ReleaseSpecialActor=_FLACompatBridge_ReleaseSpecialActor@4")
#endif

namespace {
constexpr const char* kDisplayName = "FLA++";
constexpr const char* kLogPath = "scripts\\FLACompatBridge.log";
constexpr const char* kConfigPath = "scripts\\FLACompatBridge.ini";

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
    bool enableBoundCentreVehRecovery = true;
    bool enableExceptionLoopBreaker = true;
    bool exceptionLoopBreakerTerminate = true;
    int maxExceptionLogs = 16;
    int exceptionLoopBreakerThreshold = 32;

    bool enableModuleSnapshot = true;
    bool enableRiskConstantScan = true;
    bool enableRelocatedAddressDiagnostics = true;
    bool enablePoolPointerDiagnostics = true;
    bool enableCrashClassification = true;
    bool enableRuntimeRewriteAudit = true;
    bool enableOpenLimitAdjusterOverlapAudit = true;
    bool enableModulePolicy = true;
    char legacyModuleAllowlist[512]{};
    char modernModuleDenylist[512]{};
    char forceNoRuntimeRewrite[512]{};
    char forceNoAutoPoolGuard[512]{};
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
    bool enableCleoThunk26720Guard = true;
    bool enableCleoPlusExtendedObjectVarGuard = false;
    bool enableCleoPlusPoolAllocateGuard = true;
    bool enableMixSetsPoolAllocateGuard = true;
    bool enableUrbanizePoolAllocateGuard = true;
    bool enableAutoPoolAllocateGuard = true;
    bool enableDeferredPoolAllocateReplay = true;
    int autoPoolAllocateGuardMaxPatches = 256;
    int deferredPoolAllocateReplayIterations = 600;
    int deferredPoolAllocateReplayIntervalMs = 100;
    char autoPoolAllocateGuardAllowlist[512]{};
    char autoPoolAllocateGuardDenylist[512]{};
    bool enableAnimUncompressGuard = true;
    bool enableAnimStaticAssocGuard = true;
    bool enableAnimFrameUpdateGuard = true;
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

BridgeConfig g_config{};

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

FakeColModelForBounds g_fakeColModelForBounds = {
    -0.5f, -0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.0f,  0.0f,  0.0f,
     0.75f,
     0x00000100u,
     nullptr
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
uintptr_t g_animUncompressContinue = 0;
uintptr_t g_animStaticAssocInitContinue = 0;
uintptr_t g_animFrameUpdateSkinnedVelocityContinue = 0;
uintptr_t g_flaTrainTypeCarriagesLoaderThis = 0;
uintptr_t g_flaTrainTypeCarriagesLoadFunc = 0;
uintptr_t g_lastValidAnimHierarchy = 0;
uintptr_t g_radarTraceBase = 0;
uint32_t g_radarTraceLimit = 175;
alignas(16) uint32_t g_boundCentreScratch[4]{};

constexpr size_t kMaxDeferredPoolAllocates = 256;
struct DeferredPoolAllocate {
    uintptr_t poolPtrAddress = 0;
    uintptr_t continueAddress = 0;
    uintptr_t thisPtr = 0;
    LONG completed = 0;
    char moduleName[64]{};
};

CRITICAL_SECTION g_deferredPoolAllocateLock{};
DeferredPoolAllocate g_deferredPoolAllocates[kMaxDeferredPoolAllocates]{};
uint32_t g_deferredPoolAllocateCount = 0;
LONG g_deferredPoolAllocateLogs = 0;

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

ScriptCommandHandlerFn g_originalCommands900To999 = nullptr;
ScriptCommandHandlerFn g_originalCommands600To699 = nullptr;
uintptr_t g_commands900To999TableSlot = 0;
uintptr_t g_commands600To699TableSlot = 0;
LONG g_closestCarNode03D3FallbackLogs = 0;
LONG g_sanPabloSpecialActorBridgeLogs = 0;
LONG g_specialActorSlotRangeLogs = 0;
LONG g_forwardingCommands600To699 = 0;
LONG g_forwardingCommands900To999 = 0;
LONG g_sanPabloCleoOpcodeCallbackInstalled = 0;
LONG g_sanPabloCleoOpcodeRegistered = 0;
using ProcessCommandsGroupFn = unsigned char(__thiscall*)(RunningScriptLite*, int);
ProcessCommandsGroupFn g_originalProcessCommands100To199 = nullptr;
uintptr_t g_processCommands100To199Trampoline = 0;
LONG g_taxi77SetCarCoordinatesLogs = 0;
float g_lastTargetBlipX = 0.0f;
float g_lastTargetBlipY = 0.0f;
float g_lastTargetBlipZ = 0.0f;
LONG g_hasLastTargetBlipCoords = 0;

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

CleoExports g_cleoExports{};
uintptr_t g_taxi77WatchdogMainPtr = 0;
uint32_t g_taxi77WatchdogActiveSince = 0;
uint32_t g_taxi77WatchdogLastLog = 0;
uint32_t g_taxi77WatchdogLastOffset = 0xFFFFFFFF;
LONG g_taxi77WatchdogRecoveries = 0;

constexpr size_t kMaxRuntimeSpecialActorNames = 768;
constexpr size_t kSpecialActorNameLength = 32;
struct RuntimeSpecialActorName {
    uint32_t code;
    char name[kSpecialActorNameLength];
    char source[32];
};
RuntimeSpecialActorName g_runtimeSpecialActorNames[kMaxRuntimeSpecialActorNames]{};
uint32_t g_runtimeSpecialActorNameCount = 0;

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

RuntimeRewriteRule g_runtimeRewriteRules[kMaxRuntimeRewriteRules]{};
uint32_t g_runtimeRewriteRuleCount = 0;

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
constexpr uintptr_t kCColModelPoolNewEntry = 0x0040FB80;
constexpr uintptr_t kCPedIntelligencePoolNewEntry = 0x00605EC0;
constexpr uintptr_t kGenericPoolNewEntry = 0x0061A500;
constexpr uintptr_t kCPoolsInitialise = 0x00550F10;
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

void LogMemoryRegion(const char* label, uintptr_t address);
void LoadBridgeConfig();
void LogBridgeConfig();
void LoadRuntimeRewriteRules();
bool CopyMemoryWithProtect(uintptr_t destination, uintptr_t source, size_t size);
bool IsReadableCommitted(uintptr_t address, size_t size);
bool IsWritableCommitted(uintptr_t address, size_t size);
bool IsExecutableCommitted(uintptr_t address);
bool IsValidCPool(uintptr_t pool);
bool ReadCorePoolPointers(uint32_t* pedPool, uint32_t* vehiclePool, uint32_t* objectPool, uint32_t* colModelPool);
bool EnsureLazyCPoolReady(uintptr_t poolPtr, const char* reason);
bool EnsureLazyCoreCPoolsReady(const char* reason);
bool IsReasonableWorldCoord(float value);
uintptr_t DecodeRel32JumpTarget(uintptr_t address);
uintptr_t ModuleBaseFromAddress(uintptr_t address, char* moduleName, size_t moduleNameSize);
void LogRelocatedAddressDiagnostics();
void LogPoolPointerDiagnostics();
void RefreshFlaRuntimeState();
void LogCrashClassification(EXCEPTION_POINTERS* info);
void LogBytes(const char* label, uintptr_t address, size_t count);
bool TryEnsureCPoolsInitialised(const char* reason);
HMODULE FindFlaModule();
bool ResolveFlaExtendedIdApi();
bool IsFlaDifficultHighIdMode();
int32_t ReadExtendedIdFrom16BitField(const void* field);
void RuntimeRewriteHardcodedAddressConstants(bool logDetails = true);
DWORD WINAPI RuntimeRewriteRescanThread(void*);
void LoadBridgeCheatStrings();
void InstallPickupModelLoadGuard();
bool IsSaneRadarCoord(float x, float y, float z);
void InstallFlaTrainInitHookRepair();
void RestoreFlaNoCollisionErrorPatch();
void RestoreFlaObjectInitCollisionPatch();
void InstallRadarBlipHandleGuard();
void RefreshRadarTraceRuntimeState();
void InstallClosestCarNode03D3Fallback();
void InstallTaxi77SetCarCoordinatesGuard();
void InstallSanPabloSpecialActorBridge();
bool ResolveCleoExports();
uint32_t GetCleoScriptOffset(RunningScriptLite* script);
void UpdateScriptCompareFlag(RunningScriptLite* script, bool state);
void BuildSpecialActorRuntimeCatalog();
DWORD WINAPI SpecialActorRuntimeCatalogThread(void*);
DWORD WINAPI Taxi77StateWatchdogThread(void*);
uintptr_t SafeModelInfoEntryAddress(uint32_t modelId);
uintptr_t SafeStreamingInfoEntryAddress(uint32_t modelId);
bool IsModelRwObjectLoaded(uint32_t modelId);
bool IsStreamingModelDefined(uint32_t modelId);
uint8_t GetStreamingLoadState(uint32_t modelId);
void InstallCObjectCreateBridgeStubs();
void RestoreCleoObjectCreateInlinePatch();
void InstallCleoDispatchNullGuard();
void InstallCleoThunk26720NullGuard();
void InstallCleoPlusExtendedObjectVarGuard();
void InstallCleoPlusPoolAllocateGuard();
void InstallMixSetsPoolAllocateGuard();
void InstallUrbanizePoolAllocateGuard();
void InstallAutoPoolAllocateGuards();
DWORD WINAPI DeferredPoolAllocateReplayThread(void*);
void InstallAnimUncompressNullGuard();
void InstallAnimStaticAssocInitGuard();
void InstallAnimFrameUpdateSkinnedVelocityGuard();
void InstallGetBoundCentreNullGuard();
void InstallGetBoundRectColModelGuard();
DWORD WINAPI PreloadUrbanizePedModelsThread(void*);
bool ContainsCaseInsensitive(const char* haystack, const char* needle);
extern "C" bool __stdcall Bridge_IsValidAnimHierarchy(uintptr_t hierarchy);
extern "C" bool __stdcall Bridge_IsWritableMemory(uintptr_t address, size_t size);
extern "C" bool __stdcall Bridge_IsExecutableMemory(uintptr_t address);
extern "C" uintptr_t __stdcall Bridge_GetSafeCleoThunkTarget(uintptr_t target);
extern "C" void __stdcall Bridge_CallCleoDispatchTargetSafely(uintptr_t target, uintptr_t dispatchObject);
extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_InitExtendedObjectVars_Guard(RunningScriptLite* thread);
extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_SetExtendedObjectVar_Guard(RunningScriptLite* thread);
extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_GetExtendedObjectVar_Guard(RunningScriptLite* thread);
extern "C" void __stdcall Bridge_DeferPoolAllocate(uintptr_t poolPtrAddress, uintptr_t continueAddress, uintptr_t thisPtr);
extern "C" void __stdcall Bridge_InvokePoolAllocateContinue(uintptr_t continueAddress, uintptr_t thisPtr, uintptr_t poolPtr);
extern "C" void __stdcall Bridge_LogInvalidAnimHierarchy(uintptr_t hierarchy, uintptr_t returnAddress, uintptr_t stack);
extern "C" bool __stdcall Bridge_IsValidStaticAssociation(uintptr_t staticAssociation);
extern "C" void __stdcall Bridge_RepairInvalidStaticAssociation(uintptr_t runtimeAssociation, uintptr_t staticAssociation);
extern "C" bool __stdcall Bridge_PrepareAnimFrameUpdateData(uintptr_t updateData);
extern "C" void __stdcall Bridge_LogInvalidBoundCentreOut(uintptr_t entity, uintptr_t outVec, uintptr_t returnAddress, uintptr_t stack);
extern "C" bool __stdcall Bridge_HasValidBoundRectColModel(uintptr_t entity);
extern "C" void __stdcall Bridge_FillFallbackBoundRect(uintptr_t entity, BridgeRect* outRect);
extern "C" uintptr_t __stdcall Bridge_GetSafeCleoDispatchTarget(uintptr_t dispatchObject);
extern "C" int __cdecl Bridge_CRadar_GetActualBlipArrayIndex(uint32_t blip);

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_CObject_Create_5A1FA1()
{
    __asm
    {
        push 0;       // bCreate
        push edi;     // modelID
        mov ecx, esi; // this

        push continueTheCode;
        push 0x5A1D70; // CObject::CObject(CObject *this, int modelID, char bCreate)
        retn;

    continueTheCode:
        mov eax, esi;
        push 0x5A1FCE;
        retn;
    }
}

extern "C" __declspec(naked) void Bridge_CObject_Create_5A2016()
{
    __asm
    {
        push 0;       // bCreate
        push edi;     // modelID
        mov ecx, esi; // this

        push 0x5A203D;
        push 0x5A1D70; // CObject::CObject(CObject *this, int modelID, char bCreate)
        retn;
    }
}

extern "C" __declspec(naked) void Bridge_Cleo_Dispatch_NullGuard()
{
    __asm
    {
        push ecx
        push ecx
        call Bridge_GetSafeCleoDispatchTarget
        pop ecx
        test eax, eax
        jz done
        push ecx
        push eax
        call Bridge_CallCleoDispatchTargetSafely

    done:
        push dword ptr [g_cleoDispatchNullGuardReturn]
        retn
    }
}

extern "C" __declspec(naked) void Bridge_Cleo_Thunk26720_NullGuard()
{
    __asm
    {
        mov eax, [ecx + 4]
        test eax, eax
        jz done

        mov eax, [eax + 0x18]
        test eax, eax
        jz done

        push ecx
        push edx
        push eax
        call Bridge_GetSafeCleoThunkTarget
        pop edx
        pop ecx
        test eax, eax
        jz done

        jmp eax

    done:
        xor eax, eax
        retn
    }
}

extern "C" __declspec(naked) void Bridge_CleoPlus_ObjectAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B7449C]
        test eax, eax
        jz done
        push dword ptr [g_cleoPlusObjectAllocateBlocksContinue]
        retn

    done:
        ret
    }
}

extern "C" __declspec(naked) void Bridge_CleoPlus_VehicleAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B74494]
        test eax, eax
        jz done
        push dword ptr [g_cleoPlusVehicleAllocateBlocksContinue]
        retn

    done:
        ret
    }
}

extern "C" __declspec(naked) void Bridge_CleoPlus_PedAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B74490]
        test eax, eax
        jz done
        push dword ptr [g_cleoPlusPedAllocateBlocksContinue]
        retn

    done:
        ret
    }
}

extern "C" __declspec(naked) void Bridge_MixSets_PedAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B74490]
        test eax, eax
        jz done
        push dword ptr [g_mixSetsPedAllocateBlocksContinue]
        retn

    done:
        ret
    }
}

extern "C" __declspec(naked) void __stdcall Bridge_InvokePoolAllocateContinue(uintptr_t continueAddress, uintptr_t thisPtr, uintptr_t poolPtr)
{
    __asm
    {
        mov ecx, [esp + 8]
        mov eax, [esp + 12]
        call dword ptr [esp + 4]
        ret 12
    }
}

extern "C" __declspec(naked) void Bridge_AnimUncompress_NullGuard()
{
    __asm
    {
        push edi
        mov edi, [esp + 8]

        push edi
        call Bridge_IsValidAnimHierarchy
        test al, al
        jz invalidHierarchy

        push dword ptr [g_animUncompressContinue]
        retn

    invalidHierarchy:
        mov eax, [esp + 4]
        mov ecx, [esp + 8]
        lea edx, [esp + 12]
        push edx
        push eax
        push ecx
        call Bridge_LogInvalidAnimHierarchy

        pop edi
        retn
    }
}

extern "C" __declspec(naked) void Bridge_AnimStaticAssocInit_Guard()
{
    __asm
    {
        push esi
        mov esi, ecx
        push edi
        mov edi, [esp + 0x0C]

        push edi
        call Bridge_IsValidStaticAssociation
        test al, al
        jz invalidAssociation

        push dword ptr [g_animStaticAssocInitContinue]
        retn

    invalidAssociation:
        push edi
        push esi
        call Bridge_RepairInvalidStaticAssociation

        pop edi
        pop esi
        ret 4
    }
}

extern "C" __declspec(naked) void Bridge_AnimFrameUpdateSkinnedVelocity_Guard()
{
    __asm
    {
        push eax
        push eax
        call Bridge_PrepareAnimFrameUpdateData
        mov dl, al
        pop eax
        test dl, dl
        jz skipFrameUpdate

        sub esp, 0x68
        push ebp
        push esi
        push dword ptr [g_animFrameUpdateSkinnedVelocityContinue]
        retn

    skipFrameUpdate:
        retn
    }
}

extern "C" __declspec(naked) void Bridge_GetBoundCentre_NullGuard()
{
    __asm
    {
        pushad

        mov eax, [esp + 36] // original [esp+4], hidden/out CVector*
        push 12
        push eax
        call Bridge_IsWritableMemory
        test al, al
        jz invalidOut

        popad
        movsx eax, word ptr [ecx + 0x22]
        mov edx, dword ptr [eax * 4 + 0x00A9B0C8]
        push 0x0053365B
        retn

    invalidOut:
        mov eax, [esp + 36] // out vector
        mov edx, [esp + 32] // return address
        mov ecx, [esp + 24] // this/entity
        mov ebx, [esp + 12] // original esp
        push ebx
        push edx
        push eax
        push ecx
        call Bridge_LogInvalidBoundCentreOut

        popad
        xor eax, eax
        ret 4
    }
}

extern "C" __declspec(naked) void Bridge_GetBoundRect_ColModelGuard()
{
    __asm
    {
        push esi
        mov esi, ecx

        push esi
        call Bridge_HasValidBoundRectColModel
        test al, al
        jz fallbackRect

        pop edx
        sub esp, 0x34
        push edx
        push 0x00534126
        retn

    fallbackRect:
        mov eax, [esp + 8]
        push eax
        push esi
        call Bridge_FillFallbackBoundRect
        mov eax, [esp + 8]
        pop esi
        ret 4
    }
}

extern "C" __declspec(naked) void Bridge_FlaTrainInit_LoadPreserveEax()
{
    __asm
    {
        push eax
        mov ecx, dword ptr [g_flaTrainTypeCarriagesLoaderThis]
        call dword ptr [g_flaTrainTypeCarriagesLoadFunc]
        pop eax

        push ebx
        xor ebx, ebx
        cmp eax, ebx

        push 0x006F744B
        retn
    }
}
#endif

void Log(const char* fmt, ...)
{
    FILE* file = nullptr;
    if (fopen_s(&file, kLogPath, "a") != 0 || !file) {
        return;
    }

    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::fprintf(file, "[%04u-%02u-%02u %02u:%02u:%02u.%03u] ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    va_list args;
    va_start(args, fmt);
    std::vfprintf(file, fmt, args);
    va_end(args);

    std::fputc('\n', file);
    std::fclose(file);
}

const char* BaseName(const char* path)
{
    const char* slash = std::strrchr(path, '\\');
    const char* slash2 = std::strrchr(path, '/');
    if (slash2 && (!slash || slash2 > slash)) {
        slash = slash2;
    }
    return slash ? slash + 1 : path;
}

bool ReadSmallTextValue(const char* filePath, const char* key, char* out, size_t outSize)
{
    FILE* file = nullptr;
    if (fopen_s(&file, filePath, "r") != 0 || !file) {
        return false;
    }

    char line[512];
    const size_t keyLen = std::strlen(key);
    bool found = false;
    while (std::fgets(line, sizeof(line), file)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == '#' || *p == ';') {
            continue;
        }
        if (_strnicmp(p, key, keyLen) != 0) {
            continue;
        }
        p += keyLen;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p != '=') {
            continue;
        }
        ++p;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        char* end = p + std::strlen(p);
        while (end > p && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';

        strncpy_s(out, outSize, p, _TRUNCATE);
        found = true;
        break;
    }

    std::fclose(file);
    return found;
}

bool ReadSectionTextValue(const char* filePath, const char* section, const char* key, char* out, size_t outSize)
{
    FILE* file = nullptr;
    if (fopen_s(&file, filePath, "r") != 0 || !file) {
        return false;
    }

    char line[512];
    const size_t keyLen = std::strlen(key);
    bool inSection = false;
    bool found = false;

    while (std::fgets(line, sizeof(line), file)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        if (*p == '[') {
            char* close = std::strchr(p + 1, ']');
            if (close) {
                *close = '\0';
                inSection = _stricmp(p + 1, section) == 0;
            }
            continue;
        }

        if (!inSection || *p == '#' || *p == ';' || *p == '\0' || *p == '\r' || *p == '\n') {
            continue;
        }
        if (_strnicmp(p, key, keyLen) != 0) {
            continue;
        }
        p += keyLen;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p != '=') {
            continue;
        }
        ++p;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        char* end = p + std::strlen(p);
        while (end > p && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';

        strncpy_s(out, outSize, p, _TRUNCATE);
        found = true;
        break;
    }

    std::fclose(file);
    return found;
}

void WriteDefaultBridgeConfigIfMissing()
{
    if (GetFileAttributesA(kConfigPath) != INVALID_FILE_ATTRIBUTES) {
        return;
    }

    static const char text[] =
        "; FLA++ configuration\n"
        "; Runtime file/API name remains FLACompatBridge for compatibility.\n"
        "; 1 = enabled, 0 = disabled. Defaults preserve the current tested stable profile.\n"
        "\n"
        "[General]\n"
        "; Master switch for this bridge. Set 0 to disable all bridge patches except the loader itself.\n"
        "EnableBridge = 1\n"
        "\n"
        "; Group switches. These work like MixSets-style category toggles.\n"
        "; If a group is 0, all detailed options under that group are ignored.\n"
        "EnableDiagnosticsGroup = 1\n"
        "EnableFLACompat = 1\n"
        "EnableModCompat = 1\n"
        "\n"
        "[Diagnostics]\n"
        "EnableVectoredExceptionHandler = 1\n"
        "EnableExceptionDiagnostics = 1\n"
        "EnableBoundCentreVehRecovery = 1\n"
        "EnableExceptionLoopBreaker = 1\n"
        "ExceptionLoopBreakerTerminate = 1\n"
        "ExceptionLoopBreakerThreshold = 32\n"
        "MaxExceptionLogs = 16\n"
        "EnableModuleSnapshot = 1\n"
        "EnableRiskConstantScan = 1\n"
        "EnableRelocatedAddressDiagnostics = 1\n"
        "EnablePoolPointerDiagnostics = 1\n"
        "EnableCrashClassification = 1\n"
        "EnableRuntimeRewriteAudit = 1\n"
        "EnableOpenLimitAdjusterOverlapAudit = 1\n"
        "\n"
        "[ModulePolicy]\n"
        "; Global compatibility policy shared by RuntimeRewrite and AutoPoolAllocateGuard.\n"
        "; Deny wins over every allowlist. Add modern fixes/loaders here so FLA++ only manages legacy-risk modules.\n"
        "EnableModulePolicy = 1\n"
        "LegacyModuleAllowlist = .cleo;.asi;modloader\n"
        "ModernModuleDenylist = SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;DINPUT8;vorbis;_noDEP;rundll32exefix;iii.vc.sa.limitadjuster;ImgLimitAdjuster;ProperFixes;ProperShaders\n"
        "ForceNoRuntimeRewrite = \n"
        "ForceNoAutoPoolGuard = ImprovedStreaming\n"
        "\n"
        "[RuntimeRewrite]\n"
        "; Runtime rewrite patches loaded modules in memory. Keep 0 unless testing a named module.\n"
        "EnableRuntimeRewrite = 0\n"
        "; Re-scan after startup so late-loaded CLEO/ASI/modloader DLL modules are covered.\n"
        "EnableRuntimeRewriteRescan = 0\n"
        "EnableRuntimeRewriteRuleTable = 1\n"
        "RuntimeRewriteStartDelayMs = 1000\n"
        "RuntimeRewriteIterations = 60\n"
        "RuntimeRewriteIntervalMs = 1000\n"
        "RuntimeRewriteRuleCount = 3\n"
        "RuntimeRewriteDefaultMaxPatchesPerModule = 256\n"
        "; Semicolon/comma separated substring allowlist, for example: .cleo;urbanize\n"
        "RuntimeRewriteAllowlist = \n"
        "; Semicolon/comma separated substring denylist. Denylist wins over allowlist.\n"
        "RuntimeRewriteDenylist = SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;DINPUT8;vorbis\n"
        "\n"
        "[RuntimeRewriteRules]\n"
        "; RuleNNNTarget: CModelInfo, CStreaming, RadarTrace, AnimBlocks, StreamedScripts, HandlingManager, VehicleRecording, or Static.\n"
        "; RuleNNNNew is only used when Target=Static. Per-rule allow/deny lists override the global lists when non-empty.\n"
        "Rule001Enabled = 1\n"
        "Rule001Name = CModelInfo::ms_modelInfoPtrs\n"
        "Rule001Old = 0x00A9B0C8\n"
        "Rule001Target = CModelInfo\n"
        "Rule001Align4 = 1\n"
        "Rule001ExecutableOnly = 0\n"
        "Rule001AuditOnly = 0\n"
        "Rule001MaxPatchesPerModule = 256\n"
        "Rule002Enabled = 1\n"
        "Rule002Name = CStreaming::ms_aInfoForModel\n"
        "Rule002Old = 0x008E4CC0\n"
        "Rule002Target = CStreaming\n"
        "Rule002Align4 = 1\n"
        "Rule002ExecutableOnly = 0\n"
        "Rule002AuditOnly = 0\n"
        "Rule002MaxPatchesPerModule = 256\n"
        "Rule003Enabled = 1\n"
        "Rule003Name = CRadar::ms_RadarTrace\n"
        "Rule003Old = 0x00BA86F0\n"
        "Rule003Target = RadarTrace\n"
        "Rule003Align4 = 1\n"
        "Rule003ExecutableOnly = 0\n"
        "Rule003AuditOnly = 0\n"
        "Rule003MaxPatchesPerModule = 512\n"
        "\n"
        "[FLA]\n"
        "EnableLegacyModelInfoShadow = 1\n"
        "EnableLegacyStreamingInfoShadow = 1\n"
        "EnableFlaExtendedIdApi = 1\n"
        "LegacyShadowStartDelayMs = 500\n"
        "LegacyShadowIterations = 300\n"
        "LegacyShadowIntervalMs = 200\n"
        "; Restores FLA error-reporting hook at CEntity::GetBoundRect 0x534134.\n"
        "; Keeps LOD/no-collision models from becoming fatal errors.\n"
        "EnableFlaNoCollisionErrorRestore = 1\n"
        "; Safe replacement for FLA's Enable cheat string loader.\n"
        "; Keep FLA's own Enable cheat string loader disabled.\n"
        "EnableBridgeCheatStringLoader = 1\n"
        "; Repairs FLA train type carriage loader hook at CTrain::InitTrains.\n"
        "; Preserves EAX across FLA's loader call.\n"
        "EnableFlaTrainInitHookRepair = 1\n"
        "; High-risk legacy restore. Keep 0 unless testing a CObject::Init collision-hook regression.\n"
        "EnableFlaObjectInitCollisionRestore = 0\n"
        "; Guards pickup object creation when FLA/model-streaming reports a model defined but its RwObject is not loaded yet.\n"
        "; Default only covers the save pickup model. Add IDs/names separated by semicolons, for example: 1277;pickupsave\n"
        "EnablePickupModelLoadGuard = 1\n"
        "PickupModelLoadGuardProtectHighIds = 1\n"
        "PickupModelLoadGuardAllowlist = 1277;pickupsave\n"
        "PickupModelLoadGuardFlags = 0x1E\n"
        "PickupModelLoadGuardLoadNow = 1\n"
        "PickupModelLoadGuardMaxLogs = 64\n"
        "; Guards CColAccel::startCache when an old/half-patched entry reads the original CColStore pool before it is valid.\n"
        "; Uses FLA Collision size as the fallback pool size, so expanded maps keep their capacity.\n"
        "EnableColAccelStartCachePoolGuard = 1\n"
        "; Redirects null CColModelPool::New this pointer to CPools::ms_pColModelPool when FLA/plugin loaders call collision allocation through a stale entry.\n"
        "EnableColModelPoolNewGuard = 1\n"
        "; Table-driven lazy CPool construction derived from FLA/original CPools::Initialise.\n"
        "; Builds only the missing pool requested by a guard, using capacities from fastman92limitAdjuster_GTASA.ini.\n"
        "EnableLazyCPoolRegistry = 1\n"
        "; Unsafe legacy recovery. Keep 0: calling CPools::Initialise from an exception path can leave pools half-initialized.\n"
        "; The bridge now waits for the real game initialization path and defers old plugin pool allocation until core pools are ready.\n"
        "EnableCPoolsInitialiseRecovery = 0\n"
        "\n"
        "[Mods]\n"
        "; Per-mod compatibility group switches.\n"
        "EnableCLEOPlusCompat = 1\n"
        "EnableUrbanizeCompat = 1\n"
        "EnableTaxi77Compat = 1\n"
        "EnableSanPabloCompat = 1\n"
        "EnableMixSetsCompat = 1\n"
        "\n"
        "; Shared old-mod compatibility patches.\n"
        "EnableCObjectCreateBridge = 1\n"
        "EnableAnimUncompressGuard = 1\n"
        "EnableAnimStaticAssocGuard = 1\n"
        "EnableAnimFrameUpdateGuard = 1\n"
        "EnableGetBoundCentreInlineGuard = 1\n"
        "EnableGetBoundRectColModelGuard = 1\n"
        "; Matrix guard master switch.\n"
        "EnablePlaceableRemoveMatrixGuard = 1\n"
        "; Disabled by default: skipping RemoveMatrix by guessing the caller return can corrupt the stack on heavy IPL loads.\n"
        "EnablePlaceableRemoveMatrixSkipGuard = 0\n"
        "; Keeps expanded map/model loads alive by allocating/recycling a static matrix when the original list is exhausted.\n"
        "EnablePlaceableStaticMatrixAllocGuard = 1\n"
        "; Matrix guard is address-backed because GTA SA 1.0 US exposes this only as fixed machine code.\n"
        "; Signature check prevents the bridge from touching this path when another plugin has changed the code bytes.\n"
        "MatrixGuardSignatureCheck = 1\n"
        "MatrixGuardRecoverStaticList = 1\n"
        "MatrixGuardListBase = 0x00B74288\n"
        "MatrixGuardRemoveMatrixEntry = 0x0054F3B3\n"
        "MatrixGuardRemoveMatrixNullLoad = 0x0054F3B6\n"
        "MatrixGuardAllocateStaticOwnerWrite = 0x0054F557\n"
        "MatrixGuardAllocateStaticResume = 0x0054F55D\n"
        "MatrixGuardAllocateStaticMin = 0x0054F4C0\n"
        "MatrixGuardAllocateStaticMax = 0x0054F640\n"
        "; Guards CRadar::GetActualBlipArrayIndex against invalid high blip handles.\n"
        "; Fixes old CLEO scripts that can pass stale blip handles when FLA expands Radar traces.\n"
        "EnableRadarBlipHandleGuard = 1\n"
        "; Bridges CLEO5 SA.GameEntities 0AB6 target-blip coords to FLA's relocated radar trace table.\n"
        "; Fixes old taxi/GPS scripts that otherwise fall back to world 0,0 under expanded Radar traces.\n"
        "EnableCleoTargetBlipCoordsBridge = 1\n"
        "; Rewrites hardcoded CRadar::ms_RadarTrace 0xBA86F0 constants in allowed old modules to FLA's relocated table.\n"
        "; Covers GPS/HUD/CLEO plugins that read LOWORD(targetBlip) directly instead of calling the game helper.\n"
        "EnableRadarTraceRuntimeRewrite = 1\n"
        "; Guards opcode 03D3 get_closest_car_node_with_heading from writing world 0,0 when remote path nodes are not streamed yet.\n"
        "; This prevents old taxi/GPS scripts from teleporting to the map origin on cross-city routes.\n"
        "EnableClosestCarNode03D3Fallback = 1\n"
        "\n"
        "[CLEOPlus]\n"
        "EnableCleoObjectCreateInlineRestore = 1\n"
        "EnableCleoDispatchGuard = 1\n"
        "EnableCleoThunk26720Guard = 1\n"
        "EnableCleoPlusExtendedObjectVarGuard = 0\n"
        "EnableCleoPlusPoolAllocateGuard = 1\n"
        "\n"
        "[MixSets]\n"
        "EnableMixSetsPoolAllocateGuard = 1\n"
        "\n"
        "[Urbanize]\n"
        "EnableUrbanizePoolAllocateGuard = 1\n"
        "EnableUrbanizeProblemPedPreload = 1\n"
        "\n"
        "[PoolGuards]\n"
        "; Scans all allowed modules for plugin-sdk ExtendedData AllocateBlocks patterns.\n"
        "; Pool is checked before the original function runs; if the pool is not ready, replay is deferred.\n"
        "EnableAutoPoolAllocateGuard = 1\n"
        "EnableDeferredPoolAllocateReplay = 1\n"
        "AutoPoolAllocateGuardMaxPatches = 256\n"
        "DeferredPoolAllocateReplayIterations = 600\n"
        "DeferredPoolAllocateReplayIntervalMs = 100\n"
        "AutoPoolAllocateGuardAllowlist = .asi;.cleo;modloader\n"
        "AutoPoolAllocateGuardDenylist = FLACompatBridge;fastman92;DINPUT8;vorbis;CrashInfo;SilentPatch;WidescreenFix;WindowedMode;modloader.asi;_noDEP;rundll32exefix\n"
        "\n"
        "[Taxi77]\n"
        "; Blocks Taxi77 set_car_coordinates calls when they try to write near world 0,0 and logs the script IP offset.\n"
        "EnableTaxi77SetCarCoordinatesGuard = 1\n"
        "; Watches Taxi77's main CLEO thread for long-running call/boarding states that never return to @START.\n"
        "; Recovery jumps only the Taxi77 main thread back to its own @START label; no Taxi77.cs bytecode is edited.\n"
        "EnableTaxi77StateWatchdog = 1\n"
        "EnableTaxi77StateWatchdogRecovery = 1\n"
        "Taxi77WatchdogPollMs = 1000\n"
        "Taxi77MainStuckSeconds = 75\n"
        "Taxi77StartLabelOffset = 2459\n"
        "Taxi77ActiveMinOffset = 2672\n"
        "Taxi77ActiveMaxOffset = 8626\n"
        "\n"
        "[SanPablo]\n"
        "; SANPABLO-only bridge for original special actors.\n"
        "; The SANPABLO script uses opcode 0296 with integer modes: 1=request group, 2=check group, 3=release group.\n"
        "; Generic bridge modes are also available to every script through opcode 0296:\n"
        ";   100000 + modelId = request configured actor, 200000 + modelId = check, 300000 + modelId = release.\n"
        ";   10000000 + modelId * 1000 + actorCode = request a catalog actor into a target special slot.\n"
        "; Packed high-ID mode: -2147483648 + op * 536870912 + actorCode * 262144 + modelId.\n"
        ";   op: 1=request, 2=check loaded, 3=release. modelId up to 262143, actorCode up to 2047.\n"
        "; Example: 10295012 requests SpecialActorName012 into model slot 295.\n"
        "; Example: -1584398016 requests actorCode 100 into model slot 320.\n"
        "; Example: set 30@ = 100295, then 0296: unload_special_actor 30@ requests SpecialActor295.\n"
        "; This avoids CLEO 023C string parsing crashes while keeping special actor names in the ASI/INI layer.\n"
        "EnableSanPabloSpecialActorBridge = 1\n"
        "\n"
        "[SpecialActorSlots]\n"
        "; Target model IDs used as special actor slots. Vanilla is 290-299.\n"
        "; Raise MaxModelId or AutoMaxModelId when FLA/another plugin creates usable extended special slots.\n"
        "MinModelId = 290\n"
        "MaxModelId = 299\n"
        "AutoDetectExtendedSlots = 1\n"
        "AutoMaxModelId = 20000\n"
        "RequireModelInfoSlot = 1\n"
        "\n"
        "[SpecialActors]\n"
        "; Default 290-299 special actor slots. Change or add entries as needed.\n"
        "; Common valid names in gta3.img include:\n"
        "; ANDRE, BB, BBTHIN, CAT, CESAR, CLAUDE, CROGRL1, CROGRL2, CROGRL3,\n"
        "; DNB1, DNB2, DNB3, EMMET, FORELLI, JANITOR, JETHRO, JIZZY, KENDL,\n"
        "; MACCER, MADDOGG, OGLOC, PULASKI, ROSE, RYDER, SMOKE, SMOKEV,\n"
        "; SWEET, TENPEN, TRUTH, WUZIMU, ZERO.\n"
        "SpecialActor290 = SWEET\n"
        "SpecialActor291 = RYDER\n"
        "SpecialActor292 = SMOKEV\n"
        "SpecialActor293 = ZERO\n"
        "SpecialActor294 = CESAR\n"
        "SpecialActor295 = TENPEN\n"
        "SpecialActor296 = PULASKI\n"
        "SpecialActor297 = TRUTH\n"
        "SpecialActor298 = OGLOC\n"
        "SpecialActor299 = KENDL\n"
        "\n"
        "[SpecialActorCatalog]\n"
        "; Catalog used by encoded requests: 10000000 + modelId * 1000 + actorCode.\n"
        "; INI names below are stable. Auto-scanned names start at AutoCatalogFirstCode.\n"
        "AutoScanImgArchives = 1\n"
        "AutoScanModloader = 1\n"
        "AllowMissingTxd = 0\n"
        "LogCatalog = 1\n"
        "AutoCatalogFirstCode = 100\n"
        "MaxAutoCatalogNames = 512\n"
        "CatalogBuildDelayMs = 8000\n"
        "; Semicolon-separated allow filter. Tokens match exact names or numbered variants; use NAME* for a wider prefix.\n"
        "; Use * to catalog every DFF/TXD pair, but that can include non-peds.\n"
        "AutoScanFilter = SWEET;RYDER;SMOKE;SMOKEV;CESAR;ZERO;TENPEN;PULASKI;TRUTH;OGLOC;KENDL;JIZZY;MADDOGG;MACCER;WUZIMU;EMMET;JETHRO;JANITOR;CLAUDE;FORELLI;ANDRE;BB;BBTHIN;CAT;CROGRL;DNB;ROSE;SUZIE;TBONE;TORINO;HMOGAR\n"
        "SpecialActorName001 = SWEET\n"
        "SpecialActorName002 = RYDER\n"
        "SpecialActorName003 = SMOKE\n"
        "SpecialActorName004 = SMOKEV\n"
        "SpecialActorName005 = CESAR\n"
        "SpecialActorName006 = ZERO\n"
        "SpecialActorName007 = TENPEN\n"
        "SpecialActorName008 = PULASKI\n"
        "SpecialActorName009 = TRUTH\n"
        "SpecialActorName010 = OGLOC\n"
        "SpecialActorName011 = KENDL\n"
        "SpecialActorName012 = JIZZY\n"
        "SpecialActorName013 = MADDOGG\n"
        "SpecialActorName014 = MACCER\n"
        "SpecialActorName015 = WUZIMU\n"
        "SpecialActorName016 = EMMET\n"
        "SpecialActorName017 = JETHRO\n"
        "SpecialActorName018 = JANITOR\n"
        "SpecialActorName019 = CLAUDE\n"
        "SpecialActorName020 = FORELLI\n"
        "SpecialActorName021 = ANDRE\n"
        "SpecialActorName022 = BB\n"
        "SpecialActorName023 = BBTHIN\n"
        "SpecialActorName024 = CAT\n"
        "SpecialActorName025 = CROGRL1\n"
        "SpecialActorName026 = CROGRL2\n"
        "SpecialActorName027 = CROGRL3\n"
        "SpecialActorName028 = DNB1\n"
        "SpecialActorName029 = DNB2\n"
        "SpecialActorName030 = DNB3\n"
        "SpecialActorName031 = ROSE\n"
        "SpecialActorName032 = SUZIE\n"
        "SpecialActorName033 = TBONE\n"
        "SpecialActorName034 = TORINO\n"
        "SpecialActorName035 = HMOGAR\n"
        "SpecialActorName036 = RYDER2\n"
        "SpecialActorName037 = RYDER3\n";

    HANDLE file = CreateFileA(kConfigPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD written = 0;
    WriteFile(file, text, static_cast<DWORD>(std::strlen(text)), &written, nullptr);
    CloseHandle(file);
}

bool ReadBridgeBool(const char* key, bool defaultValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(kConfigPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    if (_stricmp(value, "1") == 0 || _stricmp(value, "true") == 0 ||
        _stricmp(value, "yes") == 0 || _stricmp(value, "on") == 0) {
        return true;
    }

    if (_stricmp(value, "0") == 0 || _stricmp(value, "false") == 0 ||
        _stricmp(value, "no") == 0 || _stricmp(value, "off") == 0) {
        return false;
    }

    return defaultValue;
}

int ReadBridgeInt(const char* key, int defaultValue, int minValue, int maxValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(kConfigPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 0);
    if (end == value) {
        return defaultValue;
    }

    if (parsed < minValue) {
        return minValue;
    }
    if (parsed > maxValue) {
        return maxValue;
    }
    return static_cast<int>(parsed);
}

uint32_t ReadBridgeU32(const char* key, uint32_t defaultValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(kConfigPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    char* p = value;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(p, &end, 0);
    if (end == p) {
        return defaultValue;
    }
    return static_cast<uint32_t>(parsed);
}

void ReadBridgeText(const char* key, char* out, size_t outSize, const char* defaultValue = "")
{
    if (!out || outSize == 0) {
        return;
    }
    out[0] = '\0';
    if (!ReadSmallTextValue(kConfigPath, key, out, outSize) && defaultValue) {
        strncpy_s(out, outSize, defaultValue, _TRUNCATE);
    }
}

int ReadFlaInt(const char* key, int defaultValue, int minValue, int maxValue)
{
    char value[64]{};
    if (!ReadSmallTextValue("fastman92limitAdjuster_GTASA.ini", key, value, sizeof(value))) {
        return defaultValue;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value) {
        return defaultValue;
    }

    if (parsed < minValue) {
        return minValue;
    }
    if (parsed > maxValue) {
        return maxValue;
    }
    return static_cast<int>(parsed);
}

void LoadBridgeConfig()
{
    WriteDefaultBridgeConfigIfMissing();

    g_config.enableBridge = ReadBridgeBool("EnableBridge", g_config.enableBridge);
    g_config.enableDiagnosticsGroup = ReadBridgeBool("EnableDiagnosticsGroup", g_config.enableDiagnosticsGroup);
    g_config.enableFLACompat = ReadBridgeBool("EnableFLACompat", g_config.enableFLACompat);
    g_config.enableModCompat = ReadBridgeBool("EnableModCompat", g_config.enableModCompat);
    g_config.enableCLEOPlusCompat = ReadBridgeBool("EnableCLEOPlusCompat", g_config.enableCLEOPlusCompat);
    g_config.enableUrbanizeCompat = ReadBridgeBool("EnableUrbanizeCompat", g_config.enableUrbanizeCompat);
    g_config.enableTaxi77Compat = ReadBridgeBool("EnableTaxi77Compat", g_config.enableTaxi77Compat);
    g_config.enableSanPabloCompat = ReadBridgeBool("EnableSanPabloCompat", g_config.enableSanPabloCompat);
    g_config.enableMixSetsCompat = ReadBridgeBool("EnableMixSetsCompat", g_config.enableMixSetsCompat);

    g_config.enableVectoredExceptionHandler = ReadBridgeBool("EnableVectoredExceptionHandler", g_config.enableVectoredExceptionHandler);
    g_config.enableExceptionDiagnostics = ReadBridgeBool("EnableExceptionDiagnostics", g_config.enableExceptionDiagnostics);
    g_config.enableBoundCentreVehRecovery = ReadBridgeBool("EnableBoundCentreVehRecovery", g_config.enableBoundCentreVehRecovery);
    g_config.enableExceptionLoopBreaker = ReadBridgeBool("EnableExceptionLoopBreaker", g_config.enableExceptionLoopBreaker);
    g_config.exceptionLoopBreakerTerminate = ReadBridgeBool("ExceptionLoopBreakerTerminate", g_config.exceptionLoopBreakerTerminate);
    g_config.exceptionLoopBreakerThreshold = ReadBridgeInt("ExceptionLoopBreakerThreshold", g_config.exceptionLoopBreakerThreshold, 2, 10000);
    g_config.maxExceptionLogs = ReadBridgeInt("MaxExceptionLogs", g_config.maxExceptionLogs, 0, 256);

    g_config.enableModuleSnapshot = ReadBridgeBool("EnableModuleSnapshot", g_config.enableModuleSnapshot);
    g_config.enableRiskConstantScan = ReadBridgeBool("EnableRiskConstantScan", g_config.enableRiskConstantScan);
    g_config.enableRelocatedAddressDiagnostics = ReadBridgeBool("EnableRelocatedAddressDiagnostics", g_config.enableRelocatedAddressDiagnostics);
    g_config.enablePoolPointerDiagnostics = ReadBridgeBool("EnablePoolPointerDiagnostics", g_config.enablePoolPointerDiagnostics);
    g_config.enableCrashClassification = ReadBridgeBool("EnableCrashClassification", g_config.enableCrashClassification);
    g_config.enableRuntimeRewriteAudit = ReadBridgeBool("EnableRuntimeRewriteAudit", g_config.enableRuntimeRewriteAudit);
    g_config.enableOpenLimitAdjusterOverlapAudit = ReadBridgeBool("EnableOpenLimitAdjusterOverlapAudit", g_config.enableOpenLimitAdjusterOverlapAudit);
    g_config.enableModulePolicy = ReadBridgeBool("EnableModulePolicy", g_config.enableModulePolicy);
    ReadBridgeText("LegacyModuleAllowlist", g_config.legacyModuleAllowlist, sizeof(g_config.legacyModuleAllowlist), ".cleo;.asi;modloader");
    ReadBridgeText("ModernModuleDenylist", g_config.modernModuleDenylist, sizeof(g_config.modernModuleDenylist),
        "SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;DINPUT8;vorbis;_noDEP;rundll32exefix;iii.vc.sa.limitadjuster;ImgLimitAdjuster");
    ReadBridgeText("ForceNoRuntimeRewrite", g_config.forceNoRuntimeRewrite, sizeof(g_config.forceNoRuntimeRewrite));
    ReadBridgeText("ForceNoAutoPoolGuard", g_config.forceNoAutoPoolGuard, sizeof(g_config.forceNoAutoPoolGuard));
    g_config.enableRuntimeRewrite = ReadBridgeBool("EnableRuntimeRewrite", g_config.enableRuntimeRewrite);
    g_config.enableRuntimeRewriteRescan = ReadBridgeBool("EnableRuntimeRewriteRescan", g_config.enableRuntimeRewriteRescan);
    g_config.enableRuntimeRewriteRuleTable = ReadBridgeBool("EnableRuntimeRewriteRuleTable", g_config.enableRuntimeRewriteRuleTable);
    g_config.runtimeRewriteStartDelayMs = ReadBridgeInt("RuntimeRewriteStartDelayMs", g_config.runtimeRewriteStartDelayMs, 0, 30000);
    g_config.runtimeRewriteIterations = ReadBridgeInt("RuntimeRewriteIterations", g_config.runtimeRewriteIterations, 0, 10000);
    g_config.runtimeRewriteIntervalMs = ReadBridgeInt("RuntimeRewriteIntervalMs", g_config.runtimeRewriteIntervalMs, 100, 60000);
    g_config.runtimeRewriteRuleCount = ReadBridgeInt("RuntimeRewriteRuleCount", g_config.runtimeRewriteRuleCount, 0, static_cast<int>(kMaxRuntimeRewriteRules));
    g_config.runtimeRewriteDefaultMaxPatchesPerModule = ReadBridgeInt("RuntimeRewriteDefaultMaxPatchesPerModule", g_config.runtimeRewriteDefaultMaxPatchesPerModule, 1, 100000);
    ReadSmallTextValue(kConfigPath, "RuntimeRewriteAllowlist", g_config.runtimeRewriteAllowlist, sizeof(g_config.runtimeRewriteAllowlist));
    ReadSmallTextValue(kConfigPath, "RuntimeRewriteDenylist", g_config.runtimeRewriteDenylist, sizeof(g_config.runtimeRewriteDenylist));
    LoadRuntimeRewriteRules();

    g_config.enableLegacyModelInfoShadow = ReadBridgeBool("EnableLegacyModelInfoShadow", g_config.enableLegacyModelInfoShadow);
    g_config.enableLegacyStreamingInfoShadow = ReadBridgeBool("EnableLegacyStreamingInfoShadow", g_config.enableLegacyStreamingInfoShadow);
    g_config.enableFlaExtendedIdApi = ReadBridgeBool("EnableFlaExtendedIdApi", g_config.enableFlaExtendedIdApi);
    g_config.legacyShadowStartDelayMs = ReadBridgeInt("LegacyShadowStartDelayMs", g_config.legacyShadowStartDelayMs, 0, 30000);
    g_config.legacyShadowIterations = ReadBridgeInt("LegacyShadowIterations", g_config.legacyShadowIterations, 0, 10000);
    g_config.legacyShadowIntervalMs = ReadBridgeInt("LegacyShadowIntervalMs", g_config.legacyShadowIntervalMs, 10, 10000);

    g_config.enableCObjectCreateBridge = ReadBridgeBool("EnableCObjectCreateBridge", g_config.enableCObjectCreateBridge);
    g_config.enableCleoObjectCreateInlineRestore = ReadBridgeBool("EnableCleoObjectCreateInlineRestore", g_config.enableCleoObjectCreateInlineRestore);
    g_config.enableCleoDispatchGuard = ReadBridgeBool("EnableCleoDispatchGuard", g_config.enableCleoDispatchGuard);
    g_config.enableCleoThunk26720Guard = ReadBridgeBool("EnableCleoThunk26720Guard", g_config.enableCleoThunk26720Guard);
    g_config.enableCleoPlusExtendedObjectVarGuard = ReadBridgeBool("EnableCleoPlusExtendedObjectVarGuard", g_config.enableCleoPlusExtendedObjectVarGuard);
    g_config.enableCleoPlusPoolAllocateGuard = ReadBridgeBool("EnableCleoPlusPoolAllocateGuard", g_config.enableCleoPlusPoolAllocateGuard);
    g_config.enableMixSetsPoolAllocateGuard = ReadBridgeBool("EnableMixSetsPoolAllocateGuard", g_config.enableMixSetsPoolAllocateGuard);
    g_config.enableUrbanizePoolAllocateGuard = ReadBridgeBool("EnableUrbanizePoolAllocateGuard", g_config.enableUrbanizePoolAllocateGuard);
    g_config.enableAutoPoolAllocateGuard = ReadBridgeBool("EnableAutoPoolAllocateGuard", g_config.enableAutoPoolAllocateGuard);
    g_config.enableDeferredPoolAllocateReplay = ReadBridgeBool("EnableDeferredPoolAllocateReplay", g_config.enableDeferredPoolAllocateReplay);
    g_config.autoPoolAllocateGuardMaxPatches = ReadBridgeInt("AutoPoolAllocateGuardMaxPatches", g_config.autoPoolAllocateGuardMaxPatches, 0, 10000);
    g_config.deferredPoolAllocateReplayIterations = ReadBridgeInt("DeferredPoolAllocateReplayIterations", g_config.deferredPoolAllocateReplayIterations, 0, 100000);
    g_config.deferredPoolAllocateReplayIntervalMs = ReadBridgeInt("DeferredPoolAllocateReplayIntervalMs", g_config.deferredPoolAllocateReplayIntervalMs, 10, 10000);
    ReadSmallTextValue(kConfigPath, "AutoPoolAllocateGuardAllowlist", g_config.autoPoolAllocateGuardAllowlist, sizeof(g_config.autoPoolAllocateGuardAllowlist));
    ReadSmallTextValue(kConfigPath, "AutoPoolAllocateGuardDenylist", g_config.autoPoolAllocateGuardDenylist, sizeof(g_config.autoPoolAllocateGuardDenylist));
    g_config.enableAnimUncompressGuard = ReadBridgeBool("EnableAnimUncompressGuard", g_config.enableAnimUncompressGuard);
    g_config.enableAnimStaticAssocGuard = ReadBridgeBool("EnableAnimStaticAssocGuard", g_config.enableAnimStaticAssocGuard);
    g_config.enableAnimFrameUpdateGuard = ReadBridgeBool("EnableAnimFrameUpdateGuard", g_config.enableAnimFrameUpdateGuard);
    g_config.enableGetBoundCentreInlineGuard = ReadBridgeBool("EnableGetBoundCentreInlineGuard", g_config.enableGetBoundCentreInlineGuard);
    g_config.enableGetBoundRectColModelGuard = ReadBridgeBool("EnableGetBoundRectColModelGuard", g_config.enableGetBoundRectColModelGuard);
    g_config.enablePlaceableRemoveMatrixGuard = ReadBridgeBool("EnablePlaceableRemoveMatrixGuard", g_config.enablePlaceableRemoveMatrixGuard);
    g_config.enablePlaceableRemoveMatrixSkipGuard = ReadBridgeBool("EnablePlaceableRemoveMatrixSkipGuard", g_config.enablePlaceableRemoveMatrixSkipGuard);
    g_config.enablePlaceableStaticMatrixAllocGuard = ReadBridgeBool("EnablePlaceableStaticMatrixAllocGuard", g_config.enablePlaceableStaticMatrixAllocGuard);
    g_config.matrixGuardSignatureCheck = ReadBridgeBool("MatrixGuardSignatureCheck", g_config.matrixGuardSignatureCheck);
    g_config.matrixGuardRecoverStaticList = ReadBridgeBool("MatrixGuardRecoverStaticList", g_config.matrixGuardRecoverStaticList);
    g_config.matrixGuardListBase = ReadBridgeU32("MatrixGuardListBase", g_config.matrixGuardListBase);
    g_config.matrixGuardRemoveMatrixEntry = ReadBridgeU32("MatrixGuardRemoveMatrixEntry", g_config.matrixGuardRemoveMatrixEntry);
    g_config.matrixGuardRemoveMatrixNullLoad = ReadBridgeU32("MatrixGuardRemoveMatrixNullLoad", g_config.matrixGuardRemoveMatrixNullLoad);
    g_config.matrixGuardAllocateStaticOwnerWrite = ReadBridgeU32("MatrixGuardAllocateStaticOwnerWrite", g_config.matrixGuardAllocateStaticOwnerWrite);
    g_config.matrixGuardAllocateStaticResume = ReadBridgeU32("MatrixGuardAllocateStaticResume", g_config.matrixGuardAllocateStaticResume);
    g_config.matrixGuardAllocateStaticMin = ReadBridgeU32("MatrixGuardAllocateStaticMin", g_config.matrixGuardAllocateStaticMin);
    g_config.matrixGuardAllocateStaticMax = ReadBridgeU32("MatrixGuardAllocateStaticMax", g_config.matrixGuardAllocateStaticMax);
    g_config.enableUrbanizeProblemPedPreload = ReadBridgeBool("EnableUrbanizeProblemPedPreload", g_config.enableUrbanizeProblemPedPreload);
    g_config.enableFlaNoCollisionErrorRestore = ReadBridgeBool("EnableFlaNoCollisionErrorRestore", g_config.enableFlaNoCollisionErrorRestore);
    g_config.enableBridgeCheatStringLoader = ReadBridgeBool("EnableBridgeCheatStringLoader", g_config.enableBridgeCheatStringLoader);
    g_config.enableFlaTrainInitHookRepair = ReadBridgeBool("EnableFlaTrainInitHookRepair", g_config.enableFlaTrainInitHookRepair);
    g_config.enableFlaObjectInitCollisionRestore = ReadBridgeBool("EnableFlaObjectInitCollisionRestore", g_config.enableFlaObjectInitCollisionRestore);
    g_config.enablePickupModelLoadGuard = ReadBridgeBool("EnablePickupModelLoadGuard", g_config.enablePickupModelLoadGuard);
    g_config.pickupModelLoadGuardProtectHighIds = ReadBridgeBool("PickupModelLoadGuardProtectHighIds", g_config.pickupModelLoadGuardProtectHighIds);
    ReadBridgeText("PickupModelLoadGuardAllowlist", g_config.pickupModelLoadGuardAllowlist, sizeof(g_config.pickupModelLoadGuardAllowlist), "1277;pickupsave");
    g_config.pickupModelLoadGuardFlags = ReadBridgeInt("PickupModelLoadGuardFlags", g_config.pickupModelLoadGuardFlags, 0, 0xFF);
    g_config.pickupModelLoadGuardLoadNow = ReadBridgeBool("PickupModelLoadGuardLoadNow", g_config.pickupModelLoadGuardLoadNow);
    g_config.pickupModelLoadGuardMaxLogs = ReadBridgeInt("PickupModelLoadGuardMaxLogs", g_config.pickupModelLoadGuardMaxLogs, 0, 100000);
    g_config.enableColAccelStartCachePoolGuard = ReadBridgeBool("EnableColAccelStartCachePoolGuard", g_config.enableColAccelStartCachePoolGuard);
    g_config.enableColModelPoolNewGuard = ReadBridgeBool("EnableColModelPoolNewGuard", g_config.enableColModelPoolNewGuard);
    g_config.enableLazyCPoolRegistry = ReadBridgeBool("EnableLazyCPoolRegistry", g_config.enableLazyCPoolRegistry);
    g_config.enableCPoolsInitialiseRecovery = ReadBridgeBool("EnableCPoolsInitialiseRecovery", g_config.enableCPoolsInitialiseRecovery);
    g_config.enableRadarBlipHandleGuard = ReadBridgeBool("EnableRadarBlipHandleGuard", g_config.enableRadarBlipHandleGuard);
    g_config.enableCleoTargetBlipCoordsBridge = ReadBridgeBool("EnableCleoTargetBlipCoordsBridge", g_config.enableCleoTargetBlipCoordsBridge);
    g_config.enableRadarTraceRuntimeRewrite = ReadBridgeBool("EnableRadarTraceRuntimeRewrite", g_config.enableRadarTraceRuntimeRewrite);
    g_config.enableClosestCarNode03D3Fallback = ReadBridgeBool("EnableClosestCarNode03D3Fallback", g_config.enableClosestCarNode03D3Fallback);
    g_config.enableTaxi77SetCarCoordinatesGuard = ReadBridgeBool("EnableTaxi77SetCarCoordinatesGuard", g_config.enableTaxi77SetCarCoordinatesGuard);
    g_config.enableTaxi77StateWatchdog = ReadBridgeBool("EnableTaxi77StateWatchdog", g_config.enableTaxi77StateWatchdog);
    g_config.enableTaxi77StateWatchdogRecovery = ReadBridgeBool("EnableTaxi77StateWatchdogRecovery", g_config.enableTaxi77StateWatchdogRecovery);
    g_config.enableSanPabloSpecialActorBridge = ReadBridgeBool("EnableSanPabloSpecialActorBridge", g_config.enableSanPabloSpecialActorBridge);
    g_config.specialActorMinModelId = ReadBridgeInt("MinModelId", g_config.specialActorMinModelId, 0, 1000000);
    g_config.specialActorMaxModelId = ReadBridgeInt("MaxModelId", g_config.specialActorMaxModelId, 0, 1000000);
    g_config.specialActorAutoDetectExtendedSlots = ReadBridgeBool("AutoDetectExtendedSlots", g_config.specialActorAutoDetectExtendedSlots);
    g_config.specialActorAutoMaxModelId = ReadBridgeInt("AutoMaxModelId", g_config.specialActorAutoMaxModelId, 0, 1000000);
    g_config.specialActorAutoScanImgArchives = ReadBridgeBool("AutoScanImgArchives", g_config.specialActorAutoScanImgArchives);
    g_config.specialActorAutoScanModloader = ReadBridgeBool("AutoScanModloader", g_config.specialActorAutoScanModloader);
    g_config.specialActorAllowMissingTxd = ReadBridgeBool("AllowMissingTxd", g_config.specialActorAllowMissingTxd);
    g_config.specialActorRequireModelInfoSlot = ReadBridgeBool("RequireModelInfoSlot", g_config.specialActorRequireModelInfoSlot);
    g_config.specialActorLogCatalog = ReadBridgeBool("LogCatalog", g_config.specialActorLogCatalog);
    g_config.specialActorAutoCatalogFirstCode = ReadBridgeInt("AutoCatalogFirstCode", g_config.specialActorAutoCatalogFirstCode, 1, 999);
    g_config.specialActorMaxAutoCatalogNames = ReadBridgeInt("MaxAutoCatalogNames", g_config.specialActorMaxAutoCatalogNames, 0, static_cast<int>(kMaxRuntimeSpecialActorNames));
    g_config.specialActorCatalogBuildDelayMs = ReadBridgeInt("CatalogBuildDelayMs", g_config.specialActorCatalogBuildDelayMs, 0, 60000);
    ReadSmallTextValue(kConfigPath, "AutoScanFilter", g_config.specialActorAutoScanFilter, sizeof(g_config.specialActorAutoScanFilter));
    if (!g_config.specialActorAutoScanFilter[0]) {
        strncpy_s(g_config.specialActorAutoScanFilter, sizeof(g_config.specialActorAutoScanFilter),
            "SWEET;RYDER;SMOKE;SMOKEV;CESAR;ZERO;TENPEN;PULASKI;TRUTH;OGLOC;KENDL;JIZZY;MADDOGG;MACCER;WUZIMU;EMMET;JETHRO;JANITOR;CLAUDE;FORELLI;ANDRE;BB;BBTHIN;CAT;CROGRL;DNB;ROSE;SUZIE;TBONE;TORINO;HMOGAR",
            _TRUNCATE);
    }
    g_config.taxi77WatchdogPollMs = ReadBridgeInt("Taxi77WatchdogPollMs", g_config.taxi77WatchdogPollMs, 250, 60000);
    g_config.taxi77MainStuckSeconds = ReadBridgeInt("Taxi77MainStuckSeconds", g_config.taxi77MainStuckSeconds, 10, 3600);
    g_config.taxi77StartLabelOffset = ReadBridgeInt("Taxi77StartLabelOffset", g_config.taxi77StartLabelOffset, 0, 0xFFFF);
    g_config.taxi77ActiveMinOffset = ReadBridgeInt("Taxi77ActiveMinOffset", g_config.taxi77ActiveMinOffset, 0, 0xFFFF);
    g_config.taxi77ActiveMaxOffset = ReadBridgeInt("Taxi77ActiveMaxOffset", g_config.taxi77ActiveMaxOffset, 0, 0xFFFF);

    if (!g_config.enableBridge) {
        g_config.enableDiagnosticsGroup = false;
        g_config.enableFLACompat = false;
        g_config.enableModCompat = false;
        g_config.enableCLEOPlusCompat = false;
        g_config.enableUrbanizeCompat = false;
        g_config.enableTaxi77Compat = false;
        g_config.enableSanPabloCompat = false;
        g_config.enableMixSetsCompat = false;
    }

    if (!g_config.enableDiagnosticsGroup) {
        g_config.enableVectoredExceptionHandler = false;
        g_config.enableExceptionDiagnostics = false;
        g_config.enableBoundCentreVehRecovery = false;
        g_config.enableExceptionLoopBreaker = false;
        g_config.enableModuleSnapshot = false;
        g_config.enableRiskConstantScan = false;
        g_config.enableRelocatedAddressDiagnostics = false;
        g_config.enablePoolPointerDiagnostics = false;
        g_config.enableCrashClassification = false;
        g_config.enableRuntimeRewriteAudit = false;
        g_config.enableOpenLimitAdjusterOverlapAudit = false;
    }

    if (!g_config.enableFLACompat) {
        g_config.enableLegacyModelInfoShadow = false;
        g_config.enableLegacyStreamingInfoShadow = false;
        g_config.enableFlaExtendedIdApi = false;
        g_config.enableFlaNoCollisionErrorRestore = false;
        g_config.enableBridgeCheatStringLoader = false;
        g_config.enableFlaTrainInitHookRepair = false;
        g_config.enableFlaObjectInitCollisionRestore = false;
        g_config.enablePickupModelLoadGuard = false;
        g_config.enableColAccelStartCachePoolGuard = false;
        g_config.enableColModelPoolNewGuard = false;
        g_config.enableLazyCPoolRegistry = false;
        g_config.enableCPoolsInitialiseRecovery = false;
    }

    if (!g_config.enableModCompat) {
        g_config.enableCLEOPlusCompat = false;
        g_config.enableUrbanizeCompat = false;
        g_config.enableTaxi77Compat = false;
        g_config.enableSanPabloCompat = false;
        g_config.enableMixSetsCompat = false;
        g_config.enableRuntimeRewrite = false;
        g_config.enableRuntimeRewriteRescan = false;
        g_config.enableRuntimeRewriteRuleTable = false;
        g_config.enableCObjectCreateBridge = false;
        g_config.enableAutoPoolAllocateGuard = false;
        g_config.enableDeferredPoolAllocateReplay = false;
        g_config.enableAnimUncompressGuard = false;
        g_config.enableAnimStaticAssocGuard = false;
        g_config.enableAnimFrameUpdateGuard = false;
        g_config.enableGetBoundCentreInlineGuard = false;
        g_config.enableGetBoundRectColModelGuard = false;
        g_config.enablePlaceableRemoveMatrixGuard = false;
        g_config.enableRadarBlipHandleGuard = false;
        g_config.enableCleoTargetBlipCoordsBridge = false;
        g_config.enableRadarTraceRuntimeRewrite = false;
        g_config.enableClosestCarNode03D3Fallback = false;
        g_config.enableSanPabloSpecialActorBridge = false;
    }

    if (!g_config.enableCLEOPlusCompat) {
        g_config.enableCleoObjectCreateInlineRestore = false;
        g_config.enableCleoDispatchGuard = false;
        g_config.enableCleoThunk26720Guard = false;
        g_config.enableCleoPlusExtendedObjectVarGuard = false;
        g_config.enableCleoPlusPoolAllocateGuard = false;
    }

    if (!g_config.enableUrbanizeCompat) {
        g_config.enableUrbanizePoolAllocateGuard = false;
        g_config.enableUrbanizeProblemPedPreload = false;
    }

    if (!g_config.enableTaxi77Compat) {
        g_config.enableTaxi77SetCarCoordinatesGuard = false;
        g_config.enableTaxi77StateWatchdog = false;
        g_config.enableTaxi77StateWatchdogRecovery = false;
    }

    if (!g_config.enableSanPabloCompat) {
        g_config.enableSanPabloSpecialActorBridge = false;
    }
    if (!g_config.enableMixSetsCompat) {
        g_config.enableMixSetsPoolAllocateGuard = false;
    }

    if (g_config.specialActorMaxModelId < g_config.specialActorMinModelId) {
        g_config.specialActorMaxModelId = g_config.specialActorMinModelId;
    }
    if (g_config.specialActorAutoMaxModelId < g_config.specialActorMaxModelId) {
        g_config.specialActorAutoMaxModelId = g_config.specialActorMaxModelId;
    }
}

void LogBridgeConfig()
{
    Log("bridge config: general bridge=%d diagnosticsGroup=%d flaCompat=%d modCompat=%d cleoPlusCompat=%d urbanizeCompat=%d taxi77Compat=%d sanPabloCompat=%d",
        g_config.enableBridge ? 1 : 0,
        g_config.enableDiagnosticsGroup ? 1 : 0,
        g_config.enableFLACompat ? 1 : 0,
        g_config.enableModCompat ? 1 : 0,
        g_config.enableCLEOPlusCompat ? 1 : 0,
        g_config.enableUrbanizeCompat ? 1 : 0,
        g_config.enableTaxi77Compat ? 1 : 0,
        g_config.enableSanPabloCompat ? 1 : 0);
    Log("bridge config: modulePolicy=%d legacyAllowlist='%s' modernDenylist='%s' forceNoRuntimeRewrite='%s' forceNoAutoPoolGuard='%s'",
        g_config.enableModulePolicy ? 1 : 0,
        g_config.legacyModuleAllowlist,
        g_config.modernModuleDenylist,
        g_config.forceNoRuntimeRewrite,
        g_config.forceNoAutoPoolGuard);
    Log("bridge config: VEH=%d exceptionDiag=%d boundCentreVEH=%d loopBreaker=%d loopTerminate=%d loopThreshold=%d maxExceptionLogs=%d moduleSnapshot=%d riskScan=%d relocatedDiag=%d poolDiag=%d crashClass=%d rewriteAudit=%d olaOverlapAudit=%d rewrite=%d rewriteRescan=%d rewriteRuleTable=%d rewriteRules=%u rescanDelay=%d rescanIterations=%d rescanInterval=%d defaultMaxPatches=%d allowlist='%s' denylist='%s'",
        g_config.enableVectoredExceptionHandler ? 1 : 0,
        g_config.enableExceptionDiagnostics ? 1 : 0,
        g_config.enableBoundCentreVehRecovery ? 1 : 0,
        g_config.enableExceptionLoopBreaker ? 1 : 0,
        g_config.exceptionLoopBreakerTerminate ? 1 : 0,
        g_config.exceptionLoopBreakerThreshold,
        g_config.maxExceptionLogs,
        g_config.enableModuleSnapshot ? 1 : 0,
        g_config.enableRiskConstantScan ? 1 : 0,
        g_config.enableRelocatedAddressDiagnostics ? 1 : 0,
        g_config.enablePoolPointerDiagnostics ? 1 : 0,
        g_config.enableCrashClassification ? 1 : 0,
        g_config.enableRuntimeRewriteAudit ? 1 : 0,
        g_config.enableOpenLimitAdjusterOverlapAudit ? 1 : 0,
        g_config.enableRuntimeRewrite ? 1 : 0,
        g_config.enableRuntimeRewriteRescan ? 1 : 0,
        g_config.enableRuntimeRewriteRuleTable ? 1 : 0,
        g_runtimeRewriteRuleCount,
        g_config.runtimeRewriteStartDelayMs,
        g_config.runtimeRewriteIterations,
        g_config.runtimeRewriteIntervalMs,
        g_config.runtimeRewriteDefaultMaxPatchesPerModule,
        g_config.runtimeRewriteAllowlist,
        g_config.runtimeRewriteDenylist);
    Log("bridge config: modelShadow=%d streamingShadow=%d flaExtendedIdApi=%d shadowDelay=%d shadowIterations=%d shadowInterval=%d",
        g_config.enableLegacyModelInfoShadow ? 1 : 0,
        g_config.enableLegacyStreamingInfoShadow ? 1 : 0,
        g_config.enableFlaExtendedIdApi ? 1 : 0,
        g_config.legacyShadowStartDelayMs,
        g_config.legacyShadowIterations,
        g_config.legacyShadowIntervalMs);
    Log("bridge config: specialActorSlots=[%d,%d] autoDetect=%d autoMax=%d autoScanImg=%d autoScanModloader=%d allowMissingTxd=%d requireModelInfo=%d autoFirstCode=%d maxAutoNames=%d catalogDelayMs=%d filter='%s'",
        g_config.specialActorMinModelId,
        g_config.specialActorMaxModelId,
        g_config.specialActorAutoDetectExtendedSlots ? 1 : 0,
        g_config.specialActorAutoMaxModelId,
        g_config.specialActorAutoScanImgArchives ? 1 : 0,
        g_config.specialActorAutoScanModloader ? 1 : 0,
        g_config.specialActorAllowMissingTxd ? 1 : 0,
        g_config.specialActorRequireModelInfoSlot ? 1 : 0,
        g_config.specialActorAutoCatalogFirstCode,
        g_config.specialActorMaxAutoCatalogNames,
        g_config.specialActorCatalogBuildDelayMs,
        g_config.specialActorAutoScanFilter);
    Log("bridge config: cobjectBridge=%d cleoObjectRestore=%d cleoDispatch=%d cleoThunk=%d cleoExtObjVarGuard=%d cleoPoolAllocGuard=%d urbanizePoolAllocGuard=%d animUncompress=%d animStatic=%d animFrame=%d boundCentreInline=%d boundRectColModel=%d placeableRemoveMatrix=%d urbanizePedPreload=%d flaNoCollisionRestore=%d bridgeCheatLoader=%d trainInitRepair=%d flaObjectInitRestore=%d colAccelPoolGuard=%d colModelPoolNewGuard=%d lazyCPoolRegistry=%d cPoolsRecovery=%d radarBlipGuard=%d cleoTargetBlipBridge=%d radarTraceRewrite=%d closestCarNode03D3=%d taxi77SetCarGuard=%d taxi77Watchdog=%d taxi77Recovery=%d sanPabloSpecialActor=%d taxi77PollMs=%d taxi77StuckSeconds=%d taxi77Start=0x%X taxi77Active=[0x%X,0x%X)",
        g_config.enableCObjectCreateBridge ? 1 : 0,
        g_config.enableCleoObjectCreateInlineRestore ? 1 : 0,
        g_config.enableCleoDispatchGuard ? 1 : 0,
        g_config.enableCleoThunk26720Guard ? 1 : 0,
        g_config.enableCleoPlusExtendedObjectVarGuard ? 1 : 0,
        g_config.enableCleoPlusPoolAllocateGuard ? 1 : 0,
        g_config.enableUrbanizePoolAllocateGuard ? 1 : 0,
        g_config.enableAnimUncompressGuard ? 1 : 0,
        g_config.enableAnimStaticAssocGuard ? 1 : 0,
        g_config.enableAnimFrameUpdateGuard ? 1 : 0,
        g_config.enableGetBoundCentreInlineGuard ? 1 : 0,
        g_config.enableGetBoundRectColModelGuard ? 1 : 0,
        g_config.enablePlaceableRemoveMatrixGuard ? 1 : 0,
        g_config.enableUrbanizeProblemPedPreload ? 1 : 0,
        g_config.enableFlaNoCollisionErrorRestore ? 1 : 0,
        g_config.enableBridgeCheatStringLoader ? 1 : 0,
        g_config.enableFlaTrainInitHookRepair ? 1 : 0,
        g_config.enableFlaObjectInitCollisionRestore ? 1 : 0,
        g_config.enableColAccelStartCachePoolGuard ? 1 : 0,
        g_config.enableColModelPoolNewGuard ? 1 : 0,
        g_config.enableLazyCPoolRegistry ? 1 : 0,
        g_config.enableCPoolsInitialiseRecovery ? 1 : 0,
        g_config.enableRadarBlipHandleGuard ? 1 : 0,
        g_config.enableCleoTargetBlipCoordsBridge ? 1 : 0,
        g_config.enableRadarTraceRuntimeRewrite ? 1 : 0,
        g_config.enableClosestCarNode03D3Fallback ? 1 : 0,
        g_config.enableTaxi77SetCarCoordinatesGuard ? 1 : 0,
        g_config.enableTaxi77StateWatchdog ? 1 : 0,
        g_config.enableTaxi77StateWatchdogRecovery ? 1 : 0,
        g_config.enableSanPabloSpecialActorBridge ? 1 : 0,
        g_config.taxi77WatchdogPollMs,
        g_config.taxi77MainStuckSeconds,
        g_config.taxi77StartLabelOffset,
        g_config.taxi77ActiveMinOffset,
        g_config.taxi77ActiveMaxOffset);
    Log("bridge config: pickupModelLoadGuard=%d pickupProtectHighIds=%d pickupAllowlist='%s' pickupFlags=0x%X pickupLoadNow=%d pickupMaxLogs=%d",
        g_config.enablePickupModelLoadGuard ? 1 : 0,
        g_config.pickupModelLoadGuardProtectHighIds ? 1 : 0,
        g_config.pickupModelLoadGuardAllowlist,
        g_config.pickupModelLoadGuardFlags,
        g_config.pickupModelLoadGuardLoadNow ? 1 : 0,
        g_config.pickupModelLoadGuardMaxLogs);
}

void LogIniValue(const char* key)
{
    char value[128]{};
    if (ReadSmallTextValue("fastman92limitAdjuster_GTASA.ini", key, value, sizeof(value))) {
        Log("FLA ini: %s = %s", key, value);
    }
}

struct OlaSaOverlapSpec {
    const char* key;
    const char* owner;
};

const OlaSaOverlapSpec kOlaSaOverlapSpecs[] = {
    {"PtrNodeSingle", "FLA PtrNode Singles"},
    {"PtrNodeDouble", "FLA PtrNode Doubles"},
    {"EntryInfoNode", "FLA EntryInfoNodes"},
    {"Peds", "FLA CPools Peds"},
    {"PedIntelligence", "FLA CPools PedIntelligence"},
    {"Vehicles", "FLA CPools Vehicles"},
    {"Buildings", "FLA CPools Buildings"},
    {"Objects", "FLA CPools Objects"},
    {"Dummys", "FLA CPools Dummies"},
    {"ColModel", "FLA CPools ColModels"},
    {"Task", "FLA task allocator / old-mod guards"},
    {"Event", "FLA task/event limits"},
    {"PointRoute", "FLA CPools PointRoute"},
    {"PatrolRoute", "FLA CPools PatrolRoute"},
    {"NodeRoute", "FLA CPools NodeRoute"},
    {"TaskAllocator", "FLA CPools TaskAllocator"},
    {"PedAttractors", "FLA CPools PedAttractors"},
    {"VehicleStructs", "FLA vehicle/model limits"},
    {"MatrixList", "FLA matrix/physical object limits"},
    {"OutsideWorldWaterBlocks", "FLA map/render limits"},
    {"AlphaEntityList", "FLA visibility limits"},
    {"VisibleEntityPtrs", "FLA visibility limits"},
    {"VisibleLodPtrs", "FLA visibility limits"},
    {"StreamingObjectInstancesList", "FLA streaming/render limits"},
    {"AtomicModels", "FLA model ID limits"},
    {"DamageAtomicModels", "FLA model ID limits"},
    {"TimeModels", "FLA model ID limits"},
    {"ClumpModels", "FLA model ID limits"},
    {"VehicleModels", "FLA model ID limits"},
    {"PedModels", "FLA model ID limits"},
    {"WeaponModels", "FLA model ID limits"},
    {"EntitiesPerIpl", "FLA map/IPL limits"},
    {"EntityIpl", "FLA map/IPL limits"},
    {"StaticShadows", "FLA shadow limits"},
    {"Coronas", "FLA corona limits"},
    {"ScriptSearchLights", "FLA script/searchlight limits"},
    {"FrameLimit", "external FPS plugin / FLA timing policy"},
    {"MemoryAvailable", "FLA streaming memory"}
};

bool IniValueLooksEnabled(const char* value)
{
    if (!value) {
        return false;
    }

    while (*value == ' ' || *value == '\t') {
        ++value;
    }
    if (*value == '\0') {
        return false;
    }
    if ((*value == '0') && (value[1] == '\0' || value[1] == ' ' || value[1] == '\t')) {
        return false;
    }
    if (_stricmp(value, "false") == 0 || _stricmp(value, "off") == 0 || _stricmp(value, "disabled") == 0) {
        return false;
    }
    return true;
}

void AuditOpenLimitAdjusterSaOverlaps()
{
    if (!g_config.enableOpenLimitAdjusterOverlapAudit) {
        return;
    }

    const char* path = "modloader\\OLA\\III.VC.SA.LimitAdjuster.ini";
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
        Log("OLA overlap audit: config not found path=%s", path);
        return;
    }

    int active = 0;
    for (const OlaSaOverlapSpec& spec : kOlaSaOverlapSpecs) {
        char value[128]{};
        if (!ReadSectionTextValue(path, "SALIMITS", spec.key, value, sizeof(value))) {
            continue;
        }
        if (!IniValueLooksEnabled(value)) {
            continue;
        }

        ++active;
        Log("OLA overlap audit: active SA limit key='%s' value='%s' conflictsWith='%s' action=disable-or-keep-commented-under-FLA++",
            spec.key,
            value,
            spec.owner);
    }

    if (active == 0) {
        Log("OLA overlap audit: OK no active [SALIMITS] entries; SA limit ownership stays with FLA/FLA++");
    } else {
        Log("OLA overlap audit: WARNING activeOverlaps=%d; OLA and FLA are both trying to own SA limits", active);
    }
}

bool ReadLogAddress(const char* prefix, uintptr_t* out)
{
    if (!out) {
        return false;
    }

    HANDLE file = CreateFileA("fastman92limitAdjuster.log", GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        static DWORD lastError = 0;
        const DWORD error = GetLastError();
        if (lastError != error) {
            lastError = error;
            Log("FLA log read: open failed gle=%lu", error);
        }
        return false;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 2ll * 1024ll * 1024ll) {
        Log("FLA log read: invalid size=%lld gle=%lu", size.QuadPart, GetLastError());
        CloseHandle(file);
        return false;
    }

    const DWORD bytesToRead = static_cast<DWORD>(size.QuadPart);
    char* buffer = new char[bytesToRead + 1];
    DWORD bytesRead = 0;
    const bool readOk = ReadFile(file, buffer, bytesToRead, &bytesRead, nullptr) && bytesRead > 0;
    CloseHandle(file);
    if (!readOk) {
        Log("FLA log read: ReadFile failed gle=%lu", GetLastError());
        delete[] buffer;
        return false;
    }
    buffer[bytesRead] = '\0';

    bool found = false;
    char* search = buffer;
    while (char* match = std::strstr(search, prefix)) {
        char* p = std::strstr(match, "0x");
        if (p) {
            unsigned long value = std::strtoul(p + 2, nullptr, 16);
            if (value) {
                *out = static_cast<uintptr_t>(value);
                found = true;
            }
        }
        search = match + 1;
    }

    delete[] buffer;
    return found;
}

bool SyncShadowTable(const char* label, const char* logPrefix, uintptr_t originalAddress, size_t count, size_t elementSize)
{
    uintptr_t relocatedAddress = 0;
    if (!ReadLogAddress(logPrefix, &relocatedAddress)) {
        Log("legacy shadow: relocated %s address not found in FLA log", label);
        return false;
    }

    if (relocatedAddress == originalAddress) {
        Log("legacy shadow: %s not relocated", label);
        return true;
    }

    struct LastLog {
        const char* label;
        uintptr_t address;
    };
    static LastLog lastLogs[4]{};
    bool alreadyLogged = false;
    for (auto& entry : lastLogs) {
        if (entry.label == label) {
            if (entry.address == relocatedAddress) {
                alreadyLogged = true;
            } else {
                entry.address = relocatedAddress;
            }
            break;
        }
        if (!entry.label) {
            entry.label = label;
            entry.address = relocatedAddress;
            break;
        }
    }
    if (!alreadyLogged) {
        Log("legacy shadow: %s source from FLA log old=0x%08X new=0x%08X count=%u elem=0x%X",
            label, originalAddress, relocatedAddress, static_cast<unsigned>(count), static_cast<unsigned>(elementSize));
        LogMemoryRegion(label, relocatedAddress);
        LogMemoryRegion("legacy-shadow-destination", originalAddress);
    }

    const size_t copySize = count * elementSize;
    const bool ok = CopyMemoryWithProtect(originalAddress, relocatedAddress, copySize);
    return ok;
}

bool SyncLegacyModelInfoPointers()
{
    if (!g_config.enableLegacyModelInfoShadow) {
        return false;
    }

    const bool ok = SyncShadowTable("CModelInfo", "CModelInfo::ms_modelInfoPtrs:",
        kOriginalCModelInfoPtrs, kOriginalCModelInfoCount, sizeof(uintptr_t));
    ReadLogAddress("CModelInfo::ms_modelInfoPtrs:", &g_relocatedCModelInfoPtrs);
    return ok;
}

bool SyncLegacyStreamingInfo()
{
    if (!g_config.enableLegacyStreamingInfoShadow) {
        return false;
    }

    const bool ok = SyncShadowTable("CStreamingInfo", "CStreaming::ms_aInfoForModel:",
        kOriginalStreamingInfo, kOriginalStreamingInfoCount, kStreamingInfoSize);
    ReadLogAddress("CStreaming::ms_aInfoForModel:", &g_relocatedStreamingInfo);
    return ok;
}

DWORD WINAPI LegacyShadowThread(void*)
{
    if (!g_config.enableLegacyModelInfoShadow && !g_config.enableLegacyStreamingInfoShadow) {
        Log("legacy shadow: disabled by config");
        return 0;
    }

    Sleep(static_cast<DWORD>(g_config.legacyShadowStartDelayMs));
    for (int i = 0; i < g_config.legacyShadowIterations; ++i) {
        SyncLegacyModelInfoPointers();
        SyncLegacyStreamingInfo();
        Sleep(static_cast<DWORD>(g_config.legacyShadowIntervalMs));
    }
    return 0;
}

bool FileContainsPattern(const char* path, const uint8_t* pattern, size_t patternSize)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 64ll * 1024ll * 1024ll) {
        CloseHandle(file);
        return false;
    }

    auto* bytes = new uint8_t[static_cast<size_t>(size.QuadPart)];
    DWORD read = 0;
    const bool ok = ReadFile(file, bytes, static_cast<DWORD>(size.QuadPart), &read, nullptr) && read == size.QuadPart;
    CloseHandle(file);

    bool found = false;
    if (ok && read >= patternSize) {
        for (DWORD i = 0; i <= read - patternSize; ++i) {
            if (std::memcmp(bytes + i, pattern, patternSize) == 0) {
                found = true;
                break;
            }
        }
    }

    delete[] bytes;
    return found;
}

#if 0
bool ReadLogAddress_Old(const char* prefix, uintptr_t* out)
{
    if (!out) {
        return false;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, "fastman92limitAdjuster.log", "r") != 0 || !file) {
        return false;
    }

    char line[512];
    bool found = false;
    while (std::fgets(line, sizeof(line), file)) {
        char* match = std::strstr(line, prefix);
        if (!match) {
            continue;
        }

        char* p = std::strstr(match, "0x");
        if (!p) {
            continue;
        }

        unsigned long value = std::strtoul(p + 2, nullptr, 16);
        if (value) {
            *out = static_cast<uintptr_t>(value);
            found = true;
        }
    }

    std::fclose(file);
    return found;
}
#endif

uintptr_t ModuleBaseFromAddress(uintptr_t address, char* moduleName, size_t moduleNameSize)
{
    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        strncpy_s(moduleName, moduleNameSize, "<snapshot-failed>", _TRUNCATE);
        return 0;
    }

    uintptr_t base = 0;
    if (Module32First(snapshot, &me)) {
        do {
            const uintptr_t modBase = reinterpret_cast<uintptr_t>(me.modBaseAddr);
            const uintptr_t modEnd = modBase + me.modBaseSize;
            if (address >= modBase && address < modEnd) {
                base = modBase;
                strncpy_s(moduleName, moduleNameSize, me.szModule, _TRUNCATE);
                break;
            }
        } while (Module32Next(snapshot, &me));
    }
    CloseHandle(snapshot);

    if (!base) {
        strncpy_s(moduleName, moduleNameSize, "<unknown-module>", _TRUNCATE);
    }
    return base;
}

const char* ProtectName(DWORD protect)
{
    protect &= 0xFF;
    switch (protect) {
    case PAGE_NOACCESS: return "NOACCESS";
    case PAGE_READONLY: return "READONLY";
    case PAGE_READWRITE: return "READWRITE";
    case PAGE_WRITECOPY: return "WRITECOPY";
    case PAGE_EXECUTE: return "EXECUTE";
    case PAGE_EXECUTE_READ: return "EXECUTE_READ";
    case PAGE_EXECUTE_READWRITE: return "EXECUTE_READWRITE";
    case PAGE_EXECUTE_WRITECOPY: return "EXECUTE_WRITECOPY";
    default: return "UNKNOWN";
    }
}

const char* StateName(DWORD state)
{
    switch (state) {
    case MEM_COMMIT: return "COMMIT";
    case MEM_RESERVE: return "RESERVE";
    case MEM_FREE: return "FREE";
    default: return "UNKNOWN";
    }
}

const char* TypeName(DWORD type)
{
    switch (type) {
    case MEM_IMAGE: return "IMAGE";
    case MEM_MAPPED: return "MAPPED";
    case MEM_PRIVATE: return "PRIVATE";
    default: return "UNKNOWN";
    }
}

void LogMemoryRegion(const char* label, uintptr_t address)
{
    MEMORY_BASIC_INFORMATION mbi{};
    const SIZE_T result = VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi));
    if (!result) {
        Log("memory[%s]: address=0x%08X VirtualQuery failed gle=%lu", label, address, GetLastError());
        return;
    }

    Log("memory[%s]: address=0x%08X base=0x%08X allocBase=0x%08X size=0x%08X state=%s protect=%s rawProtect=0x%08lX type=%s",
        label,
        address,
        reinterpret_cast<uintptr_t>(mbi.BaseAddress),
        reinterpret_cast<uintptr_t>(mbi.AllocationBase),
        static_cast<uintptr_t>(mbi.RegionSize),
        StateName(mbi.State),
        ProtectName(mbi.Protect),
        mbi.Protect,
        TypeName(mbi.Type));
}

void LogStackModules(uintptr_t esp)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(esp), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT ||
        (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        Log("stack-scan: esp=0x%08X not readable", esp);
        return;
    }

    Log("stack-scan begin: esp=0x%08X", esp);
    for (int i = 0; i < 32; ++i) {
        uintptr_t value = 0;
        __try {
            value = reinterpret_cast<const uintptr_t*>(esp)[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("stack-scan: +0x%03X unreadable", i * 4);
            break;
        }

        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(value, moduleName, sizeof(moduleName));
        if (moduleBase) {
            Log("stack-scan: +0x%03X value=0x%08X -> %s+0x%X", i * 4, value, moduleName, value - moduleBase);
        } else if (value >= 0x00400000 && value < 0x80000000) {
            Log("stack-scan: +0x%03X value=0x%08X -> <no-module>", i * 4, value);
        }
    }
    Log("stack-scan end");
}

bool SafeReadU32(uintptr_t address, uint32_t* out)
{
    if (!out || !IsReadableCommitted(address, sizeof(uint32_t))) {
        return false;
    }

    __try {
        *out = *reinterpret_cast<const uint32_t*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void LogDwords(const char* label, uintptr_t address, int count)
{
    LogMemoryRegion(label, address);
    for (int i = 0; i < count; ++i) {
        uint32_t value = 0;
        const uintptr_t current = address + static_cast<uintptr_t>(i * sizeof(uint32_t));
        if (!SafeReadU32(current, &value)) {
            Log("dump[%s]: +0x%02X unreadable", label, i * 4);
            break;
        }

        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(value, moduleName, sizeof(moduleName));
        if (moduleBase) {
            Log("dump[%s]: +0x%02X = 0x%08X -> %s+0x%X", label, i * 4, value, moduleName, value - moduleBase);
        } else {
            Log("dump[%s]: +0x%02X = 0x%08X", label, i * 4, value);
        }
    }
}

void LogBytes(const char* label, uintptr_t address, size_t count)
{
    LogMemoryRegion(label, address);
    if (!count || !IsReadableCommitted(address, count)) {
        Log("bytes[%s]: address=0x%08X count=0x%X unreadable", label, address, static_cast<unsigned>(count));
        return;
    }

    uint8_t bytes[128]{};
    const size_t limited = count < sizeof(bytes) ? count : sizeof(bytes);
    __try {
        std::memcpy(bytes, reinterpret_cast<const void*>(address), limited);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bytes[%s]: address=0x%08X count=0x%X read exception", label, address, static_cast<unsigned>(limited));
        return;
    }

    for (size_t i = 0; i < limited; i += 16) {
        char line[3 * 16 + 1]{};
        size_t pos = 0;
        const size_t lineCount = (limited - i) < 16 ? (limited - i) : 16;
        for (size_t j = 0; j < lineCount && pos + 4 < sizeof(line); ++j) {
            const int written = sprintf_s(line + pos, sizeof(line) - pos, "%02X ", bytes[i + j]);
            if (written <= 0) {
                break;
            }
            pos += static_cast<size_t>(written);
        }
        Log("bytes[%s]: 0x%08X: %s", label, address + i, line);
    }
}

void LogOneRelocatedAddress(const char* label, const char* logPrefix, uintptr_t originalAddress)
{
    uintptr_t relocated = 0;
    if (!ReadLogAddress(logPrefix, &relocated)) {
        Log("relocated diag: %s not found in FLA log", label);
        return;
    }

    Log("relocated diag: %s original=0x%08X relocated=0x%08X moved=%d",
        label, originalAddress, relocated, relocated != originalAddress ? 1 : 0);
    if (relocated) {
        LogMemoryRegion(label, relocated);
    }
}

void LogRelocatedAddressDiagnostics()
{
    if (!g_config.enableRelocatedAddressDiagnostics) {
        return;
    }

    LogOneRelocatedAddress("CModelInfo::ms_modelInfoPtrs", "CModelInfo::ms_modelInfoPtrs:", kOriginalCModelInfoPtrs);
    LogOneRelocatedAddress("CStreaming::ms_aInfoForModel", "CStreaming::ms_aInfoForModel:", kOriginalStreamingInfo);
    LogOneRelocatedAddress("CAnimManager::ms_aAnimBlocks", "CAnimManager::ms_aAnimBlocks:", kOriginalAnimBlocks);
    LogOneRelocatedAddress("CVehicleRecording::StreamingArray", "CVehicleRecording::StreamingArray:", 0);
    LogOneRelocatedAddress("CTheScripts::StreamedScripts", "CTheScripts::StreamedScripts:", 0);
    LogOneRelocatedAddress("&mod_HandlingManager", "&mod_HandlingManager =", 0);
    Log("relocated diag: CRadar::ms_RadarTrace original=0x%08X relocated=0x%08X moved=%d limit=%u",
        kOriginalRadarTrace,
        g_radarTraceBase,
        g_radarTraceBase != kOriginalRadarTrace ? 1 : 0,
        g_radarTraceLimit);
    if (g_radarTraceBase) {
        LogMemoryRegion("CRadar::ms_RadarTrace", g_radarTraceBase);
    }
}

void LogOnePoolPointer(const char* label, uintptr_t pointerAddress)
{
    uint32_t value = 0;
    if (!SafeReadU32(pointerAddress, &value)) {
        Log("pool diag: %s pointerAddress=0x%08X unreadable", label, pointerAddress);
        return;
    }

    Log("pool diag: %s pointerAddress=0x%08X value=0x%08X", label, pointerAddress, value);
    if (value) {
        LogMemoryRegion(label, value);
        LogDwords(label, value, 12);
    }
}

void LogPoolPointerDiagnostics()
{
    if (!g_config.enablePoolPointerDiagnostics) {
        return;
    }

    LogOnePoolPointer("CPools::ms_pPedPool", kOriginalPedPoolPtr);
    LogOnePoolPointer("CPools::ms_pVehiclePool", kOriginalVehiclePoolPtr);
    LogOnePoolPointer("CPools::ms_pBuildingPool", kOriginalBuildingPoolPtr);
    LogOnePoolPointer("CPools::ms_pObjectPool", kOriginalObjectPoolPtr);
    LogOnePoolPointer("CPools::ms_pDummyPool", kOriginalDummyPoolPtr);
    LogOnePoolPointer("CPools::ms_pColModelPool", kOriginalColModelPoolPtr);
    LogOnePoolPointer("CColStore::ms_pColPool", kCColStorePoolPtr);
}

uint32_t ReadIniU32(const char* key, uint32_t defaultValue)
{
    char value[64]{};
    if (!ReadSmallTextValue("fastman92limitAdjuster_GTASA.ini", key, value, sizeof(value))) {
        return defaultValue;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (end == value) {
        return defaultValue;
    }
    return static_cast<uint32_t>(parsed);
}

uint32_t CalculateFileIdCapacityFromIni()
{
    const uint32_t dff = ReadIniU32("FILE_TYPE_DFF", 0);
    const uint32_t txd = ReadIniU32("FILE_TYPE_TXD", 0);
    const uint32_t col = ReadIniU32("FILE_TYPE_COL", 0);
    const uint32_t ipl = ReadIniU32("FILE_TYPE_IPL", 0);
    const uint32_t ifp = ReadIniU32("FILE_TYPE_IFP", 0);
    const uint32_t rrr = ReadIniU32("FILE_TYPE_RRR", 0);
    const uint32_t scm = ReadIniU32("FILE_TYPE_SCM", 0);
    const uint64_t sum = static_cast<uint64_t>(dff) + txd + col + ipl + ifp + rrr + scm;
    if (sum > 0x7FFFFFFFu) {
        return 0x7FFFFFFFu;
    }
    return static_cast<uint32_t>(sum);
}

HMODULE FindFlaModule()
{
    HMODULE module = GetModuleHandleA("$fastman92limitAdjuster.asi");
    if (module) {
        return module;
    }

    module = GetModuleHandleA("fastman92limitAdjuster.asi");
    if (module) {
        return module;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    if (Module32First(snapshot, &me)) {
        do {
            if (strstr(me.szModule, "fastman92") || strstr(me.szExePath, "fastman92")) {
                module = me.hModule;
                break;
            }
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    return module;
}

bool ResolveFlaExtendedIdApi()
{
    if (!g_config.enableFlaExtendedIdApi) {
        return false;
    }

    HMODULE module = FindFlaModule();
    if (!module) {
        return false;
    }

    if (g_flaExtendedIdApi.resolved && g_flaExtendedIdApi.module == module) {
        return g_flaExtendedIdApi.getExtendedIDFrom16BitBefore != nullptr;
    }

    g_flaExtendedIdApi = {};
    g_flaExtendedIdApi.module = module;
    g_flaExtendedIdApi.areDifficultIDsExtended = reinterpret_cast<FlaAreDifficultIDsExtendedFn>(
        GetProcAddress(module, "AreDifficultIDsExtended"));
    g_flaExtendedIdApi.getNumberOfFileIDs = reinterpret_cast<FlaGetNumberOfFileIDsFn>(
        GetProcAddress(module, "GetNumberOfFileIDs"));
    g_flaExtendedIdApi.getExtendedIDFrom16BitBefore = reinterpret_cast<FlaGetExtendedIDFrom16BitBeforeFn>(
        GetProcAddress(module, "GetExtendedIDfrom16bitBefore"));
    g_flaExtendedIdApi.setExtendedIDFrom16BitBefore = reinterpret_cast<FlaSetExtendedIDFrom16BitBeforeFn>(
        GetProcAddress(module, "SetExtendedIDfrom16bitBefore"));
    g_flaExtendedIdApi.resolved = true;

    if (!g_flaExtendedIdApi.logged) {
        g_flaExtendedIdApi.logged = true;
        Log("FLA extended ID API: module=0x%08X difficult=%p count=%p get16Before=%p set16Before=%p",
            reinterpret_cast<uintptr_t>(module),
            g_flaExtendedIdApi.areDifficultIDsExtended,
            g_flaExtendedIdApi.getNumberOfFileIDs,
            g_flaExtendedIdApi.getExtendedIDFrom16BitBefore,
            g_flaExtendedIdApi.setExtendedIDFrom16BitBefore);
    }

    return g_flaExtendedIdApi.getExtendedIDFrom16BitBefore != nullptr;
}

bool IsFlaDifficultHighIdMode()
{
    if (!ResolveFlaExtendedIdApi()) {
        return g_fileIdCapacity > 65532;
    }

    __try {
        if (g_flaExtendedIdApi.areDifficultIDsExtended && g_flaExtendedIdApi.areDifficultIDsExtended()) {
            return true;
        }
        if (g_flaExtendedIdApi.getNumberOfFileIDs && g_flaExtendedIdApi.getNumberOfFileIDs() > 65532) {
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("FLA extended ID API: high-ID mode query exception");
    }

    return g_fileIdCapacity > 65532;
}

int32_t ReadExtendedIdFrom16BitField(const void* field)
{
    if (!field) {
        return -1;
    }

    if (ResolveFlaExtendedIdApi()) {
        __try {
            const int32_t id = g_flaExtendedIdApi.getExtendedIDFrom16BitBefore(field);
            if (id >= 0) {
                return id;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("FLA extended ID API: GetExtendedIDfrom16bitBefore exception field=0x%08X",
                reinterpret_cast<uintptr_t>(field));
        }
    }

    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(field), sizeof(uint16_t))) {
        return -1;
    }

    uint16_t id = 0xFFFF;
    __try {
        id = *reinterpret_cast<const uint16_t*>(field);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
    return static_cast<int32_t>(id);
}

bool RefreshFlaRuntimeStateFromAbi()
{
    HMODULE fla = FindFlaModule();
    if (!fla) {
        return false;
    }

    using GetFLACompatibilityInfoFn = bool(__cdecl*)(FLACompatibilityInfo*);
    using GetFLARuntimeAddressFn = uintptr_t(__cdecl*)(uint32_t);
    using GetNumberOfFileIDsFn = int32_t(__cdecl*)();

    auto getCompatibilityInfo = reinterpret_cast<GetFLACompatibilityInfoFn>(
        GetProcAddress(fla, "GetFLACompatibilityInfo"));
    auto getRuntimeAddress = reinterpret_cast<GetFLARuntimeAddressFn>(
        GetProcAddress(fla, "GetFLARuntimeAddress"));
    auto getNumberOfFileIDs = reinterpret_cast<GetNumberOfFileIDsFn>(
        GetProcAddress(fla, "GetNumberOfFileIDs"));

    bool usedAbi = false;

    if (getCompatibilityInfo) {
        FLACompatibilityInfo info{};
        info.dwSize = sizeof(info);
        if (getCompatibilityInfo(&info)) {
            g_flaAbiVersion = info.abiVersion;
            g_flaCompatFlags = info.flags;

            if (info.countOfAllFileIDs > 0) {
                g_fileIdCapacity = static_cast<uint32_t>(info.countOfAllFileIDs);
            }
            if (info.modelInfoPtrs) {
                g_relocatedCModelInfoPtrs = info.modelInfoPtrs;
            }
            if (info.streamingInfoForModel) {
                g_relocatedStreamingInfo = info.streamingInfoForModel;
            }
            if (info.streamingInfoExtension) {
                g_relocatedStreamingInfoExtension = info.streamingInfoExtension;
            }
            if (info.animBlocks) {
                g_relocatedAnimBlocks = info.animBlocks;
            }
            if (info.registeredKills) {
                g_relocatedRegisteredKills = info.registeredKills;
            }

            usedAbi = true;
            Log("FLA ABI: compatibility info abi=%u flags=0x%08X game=%d count=%d defaultCount=%d maxCount=%d modelInfo=0x%08X streaming=0x%08X streamingExt=0x%08X animBlocks=0x%08X registeredKills=0x%08X",
                info.abiVersion,
                info.flags,
                info.gameVersion,
                info.countOfAllFileIDs,
                info.defaultCountOfAllFileIDs,
                info.maxCountOfFileIDs,
                info.modelInfoPtrs,
                info.streamingInfoForModel,
                info.streamingInfoExtension,
                info.animBlocks,
                info.registeredKills);
        } else {
            Log("FLA ABI: GetFLACompatibilityInfo returned false");
        }
    }

    if (getRuntimeAddress) {
        const uintptr_t modelInfo = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CMODELINFO_MODEL_INFO_PTRS);
        const uintptr_t streaming = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CSTREAMING_INFO_FOR_MODEL);
        const uintptr_t streamingExtension = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CSTREAMING_INFO_EXTENSION);
        const uintptr_t animBlocks = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CANIMMANAGER_ANIM_BLOCKS);
        const uintptr_t registeredKills = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CDARKEL_REGISTERED_KILLS);

        if (modelInfo) {
            g_relocatedCModelInfoPtrs = modelInfo;
        }
        if (streaming) {
            g_relocatedStreamingInfo = streaming;
        }
        if (streamingExtension) {
            g_relocatedStreamingInfoExtension = streamingExtension;
        }
        if (animBlocks) {
            g_relocatedAnimBlocks = animBlocks;
        }
        if (registeredKills) {
            g_relocatedRegisteredKills = registeredKills;
        }

        usedAbi = usedAbi || modelInfo || streaming || streamingExtension || animBlocks || registeredKills;
        Log("FLA ABI: runtime addresses modelInfo=0x%08X streaming=0x%08X streamingExt=0x%08X animBlocks=0x%08X registeredKills=0x%08X",
            modelInfo, streaming, streamingExtension, animBlocks, registeredKills);
    }

    if (!g_fileIdCapacity && getNumberOfFileIDs) {
        const int32_t count = getNumberOfFileIDs();
        if (count > 0) {
            g_fileIdCapacity = static_cast<uint32_t>(count);
            usedAbi = true;
            Log("FLA ABI: GetNumberOfFileIDs=%d", count);
        }
    }

    if (usedAbi) {
        g_runtimeStateSource = RUNTIME_SOURCE_FLA_ABI;
    }

    return usedAbi;
}

void RefreshFlaRuntimeState()
{
    const bool usedAbi = RefreshFlaRuntimeStateFromAbi();

    bool usedLog = false;
    uintptr_t logAddress = 0;
    if (!g_relocatedCModelInfoPtrs && ReadLogAddress("CModelInfo::ms_modelInfoPtrs:", &logAddress)) {
        g_relocatedCModelInfoPtrs = logAddress;
        usedLog = true;
    }
    if (!g_relocatedStreamingInfo && ReadLogAddress("CStreaming::ms_aInfoForModel:", &logAddress)) {
        g_relocatedStreamingInfo = logAddress;
        usedLog = true;
    }
    if (!g_relocatedAnimBlocks && ReadLogAddress("CAnimManager::ms_aAnimBlocks:", &logAddress)) {
        g_relocatedAnimBlocks = logAddress;
        usedLog = true;
    }
    if (!g_relocatedVehicleRecordingStreamingArray && ReadLogAddress("CVehicleRecording::StreamingArray:", &logAddress)) {
        g_relocatedVehicleRecordingStreamingArray = logAddress;
        usedLog = true;
    }
    if (!g_relocatedStreamedScripts && ReadLogAddress("CTheScripts::StreamedScripts:", &logAddress)) {
        g_relocatedStreamedScripts = logAddress;
        usedLog = true;
    }
    if (!g_relocatedHandlingManager && ReadLogAddress("&mod_HandlingManager =", &logAddress)) {
        g_relocatedHandlingManager = logAddress;
        usedLog = true;
    }

    if (!g_fileIdCapacity) {
        g_fileIdCapacity = CalculateFileIdCapacityFromIni();
        if (g_fileIdCapacity && !usedAbi && !usedLog) {
            g_runtimeStateSource = RUNTIME_SOURCE_INI_OR_VANILLA;
        }
    }
    if (!g_fileIdCapacity) {
        g_fileIdCapacity = static_cast<uint32_t>(kOriginalCModelInfoCount);
        if (!usedAbi && !usedLog) {
            g_runtimeStateSource = RUNTIME_SOURCE_INI_OR_VANILLA;
        }
    }
    if (!usedAbi && usedLog) {
        g_runtimeStateSource = RUNTIME_SOURCE_FLA_LOG;
    }

    g_pedPoolCapacity = ReadIniU32("Peds", 140);
    g_vehiclePoolCapacity = ReadIniU32("Vehicles", 110);
    g_objectPoolCapacity = ReadIniU32("Objects", 350);
    g_buildingPoolCapacity = ReadIniU32("Buildings", 13000);
    g_dummyPoolCapacity = ReadIniU32("Dummies", 2500);
    g_colModelPoolCapacity = ReadIniU32("ColModels", 10150);
    g_collisionStoreCapacity = ReadIniU32("Collision size", ReadIniU32("FILE_TYPE_COL", 500));

    Log("runtime state: source=%u abi=%u flags=0x%08X fileIdCapacity=%u CModelInfo=0x%08X CStreaming=0x%08X CStreamingExt=0x%08X AnimBlocks=0x%08X VehicleRecording=0x%08X StreamedScripts=0x%08X Handling=0x%08X RegisteredKills=0x%08X",
        g_runtimeStateSource,
        g_flaAbiVersion,
        g_flaCompatFlags,
        g_fileIdCapacity,
        g_relocatedCModelInfoPtrs,
        g_relocatedStreamingInfo,
        g_relocatedStreamingInfoExtension,
        g_relocatedAnimBlocks,
        g_relocatedVehicleRecordingStreamingArray,
        g_relocatedStreamedScripts,
        g_relocatedHandlingManager,
        g_relocatedRegisteredKills);
    Log("runtime state: poolCapacity peds=%u vehicles=%u objects=%u buildings=%u dummies=%u colModels=%u colStore=%u",
        g_pedPoolCapacity,
        g_vehiclePoolCapacity,
        g_objectPoolCapacity,
        g_buildingPoolCapacity,
        g_dummyPoolCapacity,
        g_colModelPoolCapacity,
        g_collisionStoreCapacity);
}

uintptr_t SafeModelInfoEntryAddress(uint32_t modelId)
{
    const uint32_t capacity = g_fileIdCapacity ? g_fileIdCapacity : static_cast<uint32_t>(kOriginalCModelInfoCount);
    if (modelId >= capacity) {
        return 0;
    }

    const uintptr_t table = g_relocatedCModelInfoPtrs ? g_relocatedCModelInfoPtrs : kOriginalCModelInfoPtrs;
    const uintptr_t entry = table + static_cast<uintptr_t>(modelId) * sizeof(uintptr_t);
    return IsReadableCommitted(entry, sizeof(uintptr_t)) ? entry : 0;
}

uintptr_t SafeStreamingInfoEntryAddress(uint32_t modelId)
{
    const uint32_t capacity = g_fileIdCapacity ? g_fileIdCapacity : static_cast<uint32_t>(kOriginalStreamingInfoCount);
    if (modelId >= capacity) {
        return 0;
    }

    const uintptr_t table = g_relocatedStreamingInfo ? g_relocatedStreamingInfo : kOriginalStreamingInfo;
    const uintptr_t entry = table + static_cast<uintptr_t>(modelId) * kStreamingInfoSize;
    return IsReadableCommitted(entry, kStreamingInfoSize) ? entry : 0;
}

void DumpModelContext(uint32_t modelId)
{
    Log("model-dump: modelId=%u oldModelInfoTable=0x%08X newModelInfoTable=0x%08X oldStreaming=0x%08X newStreaming=0x%08X",
        modelId, kOriginalCModelInfoPtrs, g_relocatedCModelInfoPtrs, kOriginalStreamingInfo, g_relocatedStreamingInfo);

    if (modelId >= (g_fileIdCapacity ? g_fileIdCapacity : kOriginalCModelInfoCount)) {
        Log("model-dump: modelId outside file ID capacity=%u", g_fileIdCapacity);
        return;
    }

    uint32_t oldMi = 0;
    uint32_t newMi = 0;
    if (modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &oldMi);
    }
    const uintptr_t modelInfoEntry = SafeModelInfoEntryAddress(modelId);
    if (modelInfoEntry) {
        SafeReadU32(modelInfoEntry, &newMi);
    }
    Log("model-dump: CModelInfo[%u] old=0x%08X new=0x%08X", modelId, oldMi, newMi);

    if (oldMi) {
        LogDwords("old-model-info", oldMi, 16);
        uint32_t vtable = 0;
        uint32_t rwObject = 0;
        SafeReadU32(oldMi, &vtable);
        SafeReadU32(oldMi + 0x1C, &rwObject);
        Log("model-dump: old vtable=0x%08X rwObject(+0x1C)=0x%08X", vtable, rwObject);
        if (vtable) {
            LogDwords("old-model-vtable", vtable, 16);
        }
        if (rwObject) {
            LogDwords("old-model-rwobject", rwObject, 12);
        }
    }

    if (newMi && newMi != oldMi) {
        LogDwords("new-model-info", newMi, 16);
        uint32_t vtable = 0;
        uint32_t rwObject = 0;
        SafeReadU32(newMi, &vtable);
        SafeReadU32(newMi + 0x1C, &rwObject);
        Log("model-dump: new vtable=0x%08X rwObject(+0x1C)=0x%08X", vtable, rwObject);
        if (vtable) {
            LogDwords("new-model-vtable", vtable, 16);
        }
        if (rwObject) {
            LogDwords("new-model-rwobject", rwObject, 12);
        }
    }

    if (modelId < kOriginalStreamingInfoCount) {
        LogDwords("old-streaming-entry", kOriginalStreamingInfo + modelId * kStreamingInfoSize, 5);
    }
    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (streamingEntry) {
        LogDwords("safe-streaming-entry", streamingEntry, 5);
    }
}

bool IsModelRwObjectLoaded(uint32_t modelId)
{
    const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
    if (!entry) {
        return false;
    }

    uint32_t modelInfo = 0;
    SafeReadU32(entry, &modelInfo);
    if (!modelInfo && modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &modelInfo);
    }
    if (!modelInfo) {
        return false;
    }

    uint32_t rwObject = 0;
    return SafeReadU32(modelInfo + 0x1C, &rwObject) && rwObject != 0;
}

bool TokenMatchesPickupModel(uint32_t modelId, const char* token)
{
    if (!token || !*token) {
        return false;
    }

    while (*token == ' ' || *token == '\t') {
        ++token;
    }

    if (std::strcmp(token, "*") == 0 || _stricmp(token, "all") == 0 || _stricmp(token, "highid") == 0) {
        return true;
    }

    if (_stricmp(token, "pickupsave") == 0 || _stricmp(token, "MI_PICKUP_SAVEGAME") == 0 ||
        _stricmp(token, "MODEL_PICKUPSAVE") == 0) {
        return modelId == 1277;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(token, &end, 0);
    if (end && *end == '\0') {
        return parsed == modelId;
    }

    return false;
}

bool PickupModelGuardAllows(uint32_t modelId)
{
    char list[sizeof(g_config.pickupModelLoadGuardAllowlist)]{};
    strncpy_s(list, sizeof(list), g_config.pickupModelLoadGuardAllowlist, _TRUNCATE);

    char* context = nullptr;
    for (char* token = strtok_s(list, ";, \t", &context);
         token;
         token = strtok_s(nullptr, ";, \t", &context)) {
        if (TokenMatchesPickupModel(modelId, token)) {
            return true;
        }
    }

    return false;
}

bool TryLoadPickupModelNow(uint32_t modelId)
{
#if defined(_M_IX86)
    if (!IsStreamingModelDefined(modelId)) {
        return false;
    }
    if (IsModelRwObjectLoaded(modelId)) {
        return true;
    }

    using RequestModelFn = void(__cdecl*)(int, int);
    using LoadAllRequestedModelsFn = void(__cdecl*)(bool);

    auto requestModel = reinterpret_cast<RequestModelFn>(0x004087E0);
    auto loadAllRequestedModels = reinterpret_cast<LoadAllRequestedModelsFn>(0x0040EA10);

    __try {
        requestModel(static_cast<int>(modelId), g_config.pickupModelLoadGuardFlags);
        if (g_config.pickupModelLoadGuardLoadNow) {
            loadAllRequestedModels(false);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("pickup model guard: streaming request exception model=%u flags=0x%X", modelId, g_config.pickupModelLoadGuardFlags);
        return false;
    }

    return IsModelRwObjectLoaded(modelId);
#else
    (void)modelId;
    return false;
#endif
}

bool ShadowPickupModelToLegacyTables(uint32_t modelId)
{
    bool ok = true;
    uint32_t modelInfo = 0;
    const uintptr_t modelEntry = SafeModelInfoEntryAddress(modelId);
    if (!modelEntry || !SafeReadU32(modelEntry, &modelInfo) || !modelInfo) {
        return false;
    }

    if (modelId < kOriginalCModelInfoCount) {
        ok = CopyMemoryWithProtect(
            kOriginalCModelInfoPtrs + static_cast<uintptr_t>(modelId) * sizeof(uintptr_t),
            reinterpret_cast<uintptr_t>(&modelInfo),
            sizeof(modelInfo)) && ok;
    }

    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (streamingEntry && modelId < kOriginalStreamingInfoCount) {
        ok = CopyMemoryWithProtect(
            kOriginalStreamingInfo + static_cast<uintptr_t>(modelId) * kStreamingInfoSize,
            streamingEntry,
            kStreamingInfoSize) && ok;
    }

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= g_config.pickupModelLoadGuardMaxLogs) {
        Log("pickup model guard: legacy shadow model=%u modelInfo=0x%08X streaming=0x%08X ok=%d",
            modelId,
            modelInfo,
            streamingEntry,
            ok ? 1 : 0);
    }

    return ok;
}

extern "C" int __stdcall Bridge_ShouldCreatePickupObject(uintptr_t pickup)
{
    if (!g_config.enablePickupModelLoadGuard || !pickup || !IsReadableCommitted(pickup + 0x18, sizeof(uint16_t))) {
        return 1;
    }

    const int32_t extendedModelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(pickup + 0x18));
    if (extendedModelId < 0) {
        return 1;
    }

    const uint32_t modelId = static_cast<uint32_t>(extendedModelId);
    const bool protectHighId = g_config.pickupModelLoadGuardProtectHighIds &&
        (modelId >= kOriginalCModelInfoCount || IsFlaDifficultHighIdMode());
    if (!protectHighId && !PickupModelGuardAllows(modelId)) {
        return 1;
    }

    if (IsModelRwObjectLoaded(modelId)) {
        ShadowPickupModelToLegacyTables(modelId);
        return 1;
    }

    const uint8_t beforeState = GetStreamingLoadState(modelId);
    const bool loaded = TryLoadPickupModelNow(modelId);
    const uint8_t afterState = GetStreamingLoadState(modelId);

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= g_config.pickupModelLoadGuardMaxLogs) {
        Log("pickup model guard: model=%u beforeState=%u afterState=%u loadNow=%d loaded=%d pickup=0x%08X",
            modelId,
            static_cast<unsigned>(beforeState),
            static_cast<unsigned>(afterState),
            g_config.pickupModelLoadGuardLoadNow ? 1 : 0,
            loaded ? 1 : 0,
            pickup);
        if (!loaded) {
            DumpModelContext(modelId);
        }
    }

    if (loaded) {
        ShadowPickupModelToLegacyTables(modelId);
    }

    return loaded ? 1 : 0;
}

extern "C" __declspec(naked) void Bridge_CPickup_GiveUsAPickUpObject_Guard()
{
#if defined(_M_IX86)
    __asm {
        pushad
        push ecx
        call Bridge_ShouldCreatePickupObject
        test eax, eax
        jnz allow_original
        popad
        mov eax, [esp + 4]
        test eax, eax
        jz skip_store
        mov dword ptr [eax], 0
    skip_store:
        ret 8

    allow_original:
        popad
        sub esp, 0Ch
        push esi
        mov esi, [esp + 14h]
        push 004567E8h
        ret
    }
#endif
}

extern "C" bool __stdcall Bridge_HasValidBoundRectColModel(uintptr_t entity)
{
    if (!IsReadableCommitted(entity + 0x22, sizeof(uint16_t))) {
        return false;
    }

    const int32_t extendedModelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(entity + 0x22));
    if (extendedModelId < 0) {
        return false;
    }

    const uint32_t modelId = static_cast<uint32_t>(extendedModelId);
    uintptr_t modelInfo = 0;
    if (modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + static_cast<uintptr_t>(modelId) * sizeof(uintptr_t), &modelInfo);
    }
    if (!modelInfo) {
        const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
        if (entry) {
            SafeReadU32(entry, &modelInfo);
        }
    }
    if (!modelInfo || !IsReadableCommitted(modelInfo + 0x14, sizeof(uintptr_t))) {
        return false;
    }

    uint32_t colModel = 0;
    if (!SafeReadU32(modelInfo + 0x14, &colModel)) {
        return false;
    }

    return colModel && IsReadableCommitted(colModel, 0x18);
}

extern "C" void __stdcall Bridge_FillFallbackBoundRect(uintptr_t entity, BridgeRect* outRect)
{
    if (!outRect || !IsWritableCommitted(reinterpret_cast<uintptr_t>(outRect), sizeof(*outRect))) {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (IsReadableCommitted(entity + 0x04, sizeof(float) * 3)) {
        __try {
            x = *reinterpret_cast<const float*>(entity + 0x04);
            y = *reinterpret_cast<const float*>(entity + 0x08);
            z = *reinterpret_cast<const float*>(entity + 0x0C);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }
    }

    if (!IsReasonableWorldCoord(x) || !IsReasonableWorldCoord(y)) {
        x = 0.0f;
        y = 0.0f;
    }

    int32_t modelId = -1;
    if (IsReadableCommitted(entity + 0x22, sizeof(uint16_t))) {
        modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(entity + 0x22));
    }

    constexpr float radius = 1.0f;
    outRect->left = x - radius;
    outRect->right = x + radius;
    outRect->bottom = y - radius;
    outRect->top = y + radius;

    static LONG fallbackCount = 0;
    const LONG count = InterlockedIncrement(&fallbackCount);
    if (count <= 64) {
        Log("bound rect guard: fallback rect model=%u entity=0x%08X pos=(%.3f, %.3f, %.3f) rect=(%.3f, %.3f, %.3f, %.3f)",
            modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
            entity,
            x,
            y,
            z,
            outRect->left,
            outRect->bottom,
            outRect->right,
            outRect->top);
        if (modelId < kOriginalCModelInfoCount) {
            DumpModelContext(modelId);
        }
    }
}

uint8_t GetStreamingLoadState(uint32_t modelId)
{
    const uintptr_t entry = SafeStreamingInfoEntryAddress(modelId);
    if (!entry) {
        return 0xFF;
    }

    uint32_t state = 0xFF;
    SafeReadU32(entry + 0x10, &state);
    return static_cast<uint8_t>(state & 0xFF);
}

bool IsStreamingModelDefined(uint32_t modelId)
{
    const uintptr_t modelEntry = SafeModelInfoEntryAddress(modelId);
    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (!modelEntry || !streamingEntry) {
        return false;
    }

    uint32_t modelInfo = 0;
    SafeReadU32(modelEntry, &modelInfo);
    if (!modelInfo && modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &modelInfo);
    }

    uint32_t cdSize = 0;
    SafeReadU32(streamingEntry + 0x0C, &cdSize);
    return modelInfo != 0 && cdSize != 0;
}

bool IsPedModelInfo(uint32_t modelId)
{
    const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
    if (!entry) {
        return false;
    }

    uint32_t modelInfo = 0;
    SafeReadU32(entry, &modelInfo);
    if (!modelInfo && modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &modelInfo);
    }
    if (!modelInfo) {
        return false;
    }

    uint32_t vtable = 0;
    return SafeReadU32(modelInfo, &vtable) && vtable == kPedModelInfoVtable;
}

DWORD WINAPI PreloadUrbanizePedModelsThread(void*)
{
#if defined(_M_IX86)
    using RequestModelFn = void(__cdecl*)(int, int);
    using LoadAllRequestedModelsFn = void(__cdecl*)(bool);

    constexpr uintptr_t kRequestModel = 0x004087E0;
    constexpr uintptr_t kLoadAllRequestedModels = 0x0040EA10;
    constexpr int kStreamingGameRequired = 0x2;
    constexpr int kStreamingKeepInMemory = 0x8;
    constexpr int kFlags = kStreamingGameRequired | kStreamingKeepInMemory;
    constexpr int kProblemPedModels[] = { 104, 105, 109, 110, 111 };

    auto requestModel = reinterpret_cast<RequestModelFn>(kRequestModel);
    auto loadAllRequestedModels = reinterpret_cast<LoadAllRequestedModelsFn>(kLoadAllRequestedModels);

    Log("ped preload: waiting for observed problem ped models flags=0x%X request=0x%08X loadAll=0x%08X",
        kFlags, kRequestModel, kLoadAllRequestedModels);

    Sleep(500);
    for (int attempt = 0; attempt < 60; ++attempt) {
        SyncLegacyModelInfoPointers();
        SyncLegacyStreamingInfo();

        int definedCount = 0;
        int missingRwCount = 0;
        for (int modelId : kProblemPedModels) {
            if (!IsPedModelInfo(modelId) || !IsStreamingModelDefined(modelId)) {
                continue;
            }
            ++definedCount;
            if (!IsModelRwObjectLoaded(modelId)) {
                ++missingRwCount;
            }
        }

        if (!definedCount) {
            if (attempt == 0 || attempt == 10 || attempt == 30) {
                Log("ped preload: no observed problem ped models defined yet attempt=%d", attempt);
            }
            Sleep(500);
            continue;
        }

        if (!missingRwCount) {
            Log("ped preload: all observed problem ped models already loaded attempt=%d defined=%d", attempt, definedCount);
            break;
        }

        __try {
            int requestCount = 0;
            for (int modelId : kProblemPedModels) {
                if (IsPedModelInfo(modelId) && IsStreamingModelDefined(modelId) && !IsModelRwObjectLoaded(modelId)) {
                    const uint8_t state = GetStreamingLoadState(modelId);
                    if (state == 0) {
                        Log("ped preload: request model=%d state=%u", modelId, static_cast<unsigned>(state));
                        requestModel(modelId, kFlags);
                        ++requestCount;
                    } else {
                        Log("ped preload: skip repeated request model=%d state=%u", modelId, static_cast<unsigned>(state));
                    }
                }
            }
            Log("ped preload: requested count=%d attempt=%d", requestCount, attempt);
            loadAllRequestedModels(false);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("ped preload: exception while requesting/loading observed problem ped models attempt=%d", attempt);
            Sleep(500);
            continue;
        }

        SyncLegacyModelInfoPointers();
        SyncLegacyStreamingInfo();
        bool allLoaded = true;
        int loadedCount = 0;
        int stillMissingCount = 0;
        for (int modelId : kProblemPedModels) {
            if (!IsPedModelInfo(modelId) || !IsStreamingModelDefined(modelId)) {
                continue;
            }
            if (IsModelRwObjectLoaded(modelId)) {
                ++loadedCount;
            } else {
                Log("ped preload: still missing model=%d loadState=%u", modelId, static_cast<unsigned>(GetStreamingLoadState(modelId)));
                ++stillMissingCount;
            }
        }
        allLoaded = stillMissingCount == 0;
        Log("ped preload: status attempt=%d defined=%d loaded=%d missing=%d",
            attempt, definedCount, loadedCount, stillMissingCount);

        if (allLoaded) {
            Log("ped preload: completed attempt=%d", attempt);
            break;
        }
        Sleep(500);
    }
#else
    Log("urbanize ped preload: unsupported architecture");
#endif
    return 0;
}

bool IsReadableCommitted(uintptr_t address, size_t size)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        return false;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    const uintptr_t end = base + static_cast<uintptr_t>(mbi.RegionSize);
    if (address < base || address + size > end) {
        return false;
    }

    return mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_NOACCESS) && !(mbi.Protect & PAGE_GUARD);
}

bool IsWritableCommitted(uintptr_t address, size_t size)
{
    if (!address || !size) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        return false;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    const uintptr_t end = base + static_cast<uintptr_t>(mbi.RegionSize);
    if (address < base || address + size > end) {
        return false;
    }

    if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return false;
    }

    const DWORD protect = mbi.Protect & 0xFF;
    return protect == PAGE_READWRITE || protect == PAGE_WRITECOPY ||
        protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

uintptr_t MatrixListBase()
{
    return g_config.matrixGuardListBase ? static_cast<uintptr_t>(g_config.matrixGuardListBase) : kDefaultMatrixLinkList;
}

uintptr_t MatrixListHead()
{
    return MatrixListBase() + kMatrixListHeadOffset;
}

uintptr_t MatrixListTail()
{
    return MatrixListBase() + kMatrixListTailOffset;
}

uintptr_t MatrixListAllocatedHead()
{
    return MatrixListBase() + kMatrixListAllocatedHeadOffset;
}

uintptr_t MatrixListAllocatedTail()
{
    return MatrixListBase() + kMatrixListAllocatedTailOffset;
}

uintptr_t MatrixListFreeHead()
{
    return MatrixListBase() + kMatrixListFreeHeadOffset;
}

uintptr_t MatrixListFreeTail()
{
    return MatrixListBase() + kMatrixListFreeTailOffset;
}

uintptr_t MatrixListDynamicTailPrevAddress()
{
    return MatrixListTail() + kMatrixLinkPrevOffset;
}

uintptr_t MatrixListStaticTailPrevAddress()
{
    return MatrixListAllocatedTail() + kMatrixLinkPrevOffset;
}

bool IsMatrixListSentinel(uintptr_t link)
{
    return link == MatrixListHead() ||
        link == MatrixListTail() ||
        link == MatrixListAllocatedHead() ||
        link == MatrixListAllocatedTail() ||
        link == MatrixListFreeHead() ||
        link == MatrixListFreeTail();
}

bool BytePatternMatches(uintptr_t address, const uint8_t* bytes, size_t size)
{
    if (!bytes || !size || !IsReadableCommitted(address, size)) {
        return false;
    }

    __try {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes, size) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool MatrixGuardCodeLooksCompatible(uintptr_t eip)
{
    if (!g_config.matrixGuardSignatureCheck) {
        return true;
    }

    bool matched = false;
    if (eip == g_config.matrixGuardRemoveMatrixEntry) {
        const uint8_t pattern[] = { 0x8B, 0x41, 0x14 };
        matched = BytePatternMatches(eip, pattern, sizeof(pattern));
    } else if (eip == g_config.matrixGuardRemoveMatrixNullLoad) {
        const uint8_t pattern[] = { 0x8B, 0x48, 0x04 };
        matched = BytePatternMatches(eip, pattern, sizeof(pattern));
    } else if (eip == g_config.matrixGuardAllocateStaticOwnerWrite) {
        const uint8_t pattern[] = { 0x89, 0x70, 0x48, 0x89, 0x46, 0x14 };
        matched = BytePatternMatches(eip, pattern, sizeof(pattern));
    } else {
        return false;
    }

    if (!matched) {
        static LONG mismatchLogs = 0;
        const LONG count = InterlockedIncrement(&mismatchLogs);
        if (count <= 8) {
            Log("matrix guard signature mismatch: eip=0x%08X removeEntry=0x%08X removeNull=0x%08X allocWrite=0x%08X signatureCheck=%d",
                eip,
                g_config.matrixGuardRemoveMatrixEntry,
                g_config.matrixGuardRemoveMatrixNullLoad,
                g_config.matrixGuardAllocateStaticOwnerWrite,
                g_config.matrixGuardSignatureCheck ? 1 : 0);
            LogBytes("matrix-guard-eip", eip, 16);
        }
    }

    return matched;
}

bool MoveMatrixLinkToFreeList(uintptr_t link, uintptr_t expectedOwner, uintptr_t* recoveredLink, uintptr_t* recoveredOwner)
{
    if (recoveredLink) {
        *recoveredLink = 0;
    }
    if (recoveredOwner) {
        *recoveredOwner = 0;
    }

    if (!link || IsMatrixListSentinel(link) || !IsWritableCommitted(link, 0x54)) {
        return false;
    }

    uintptr_t owner = 0;
    uintptr_t prev = 0;
    uintptr_t next = 0;
    uintptr_t freeNext = 0;
    if (!SafeReadU32(link + kMatrixLinkOwnerOffset, &owner) ||
        !SafeReadU32(link + kMatrixLinkPrevOffset, &prev) ||
        !SafeReadU32(link + kMatrixLinkNextOffset, &next) ||
        !SafeReadU32(MatrixListFreeHead() + kMatrixLinkNextOffset, &freeNext)) {
        return false;
    }

    if (expectedOwner && owner && owner != expectedOwner) {
        uintptr_t ownerMatrix = 0;
        SafeReadU32(expectedOwner + 0x14, &ownerMatrix);
        if (ownerMatrix != 0 && ownerMatrix != link) {
            return false;
        }
    }

    if (!prev || !next || !freeNext ||
        IsMatrixListSentinel(link) ||
        !IsWritableCommitted(prev + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(next + kMatrixLinkPrevOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(MatrixListFreeHead() + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(freeNext + kMatrixLinkPrevOffset, sizeof(uintptr_t))) {
        return false;
    }

    __try {
        if (owner && IsWritableCommitted(owner + 0x14, sizeof(uintptr_t))) {
            uintptr_t ownerMatrix = 0;
            if (SafeReadU32(owner + 0x14, &ownerMatrix) && ownerMatrix == link) {
                *reinterpret_cast<uint32_t*>(owner + 0x14) = 0;
            }
        }

        *reinterpret_cast<uint32_t*>(next + kMatrixLinkPrevOffset) = static_cast<uint32_t>(prev);
        *reinterpret_cast<uint32_t*>(prev + kMatrixLinkNextOffset) = static_cast<uint32_t>(next);

        *reinterpret_cast<uint32_t*>(link + kMatrixLinkOwnerOffset) = 0;
        *reinterpret_cast<uint32_t*>(link + kMatrixLinkPrevOffset) = static_cast<uint32_t>(MatrixListFreeHead());
        *reinterpret_cast<uint32_t*>(link + kMatrixLinkNextOffset) = static_cast<uint32_t>(freeNext);
        *reinterpret_cast<uint32_t*>(freeNext + kMatrixLinkPrevOffset) = static_cast<uint32_t>(link);
        *reinterpret_cast<uint32_t*>(MatrixListFreeHead() + kMatrixLinkNextOffset) = static_cast<uint32_t>(link);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (recoveredLink) {
        *recoveredLink = link;
    }
    if (recoveredOwner) {
        *recoveredOwner = owner;
    }
    return true;
}

bool RecoverOldestMatrixLinkToFreeList(uintptr_t expectedOwner, uintptr_t* recoveredLink, uintptr_t* recoveredOwner, const char** sourceList)
{
    if (sourceList) {
        *sourceList = "none";
    }

    uintptr_t oldestDynamic = 0;
    if (SafeReadU32(MatrixListDynamicTailPrevAddress(), &oldestDynamic) &&
        MoveMatrixLinkToFreeList(oldestDynamic, expectedOwner, recoveredLink, recoveredOwner)) {
        if (sourceList) {
            *sourceList = "dynamic";
        }
        return true;
    }

    uintptr_t oldestStatic = 0;
    if (g_config.matrixGuardRecoverStaticList &&
        SafeReadU32(MatrixListStaticTailPrevAddress(), &oldestStatic) &&
        MoveMatrixLinkToFreeList(oldestStatic, expectedOwner, recoveredLink, recoveredOwner)) {
        if (sourceList) {
            *sourceList = "static";
        }
        return true;
    }

    return false;
}

bool AllocateStaticMatrixLinkFromFreeList(uintptr_t owner, uintptr_t* allocatedLink)
{
    if (allocatedLink) {
        *allocatedLink = 0;
    }

    if (!owner || !IsWritableCommitted(owner + 0x14, sizeof(uintptr_t))) {
        return false;
    }

    uintptr_t freeLink = 0;
    uintptr_t freeNext = 0;
    uintptr_t list2Next = 0;
    if (!SafeReadU32(MatrixListFreeHead() + kMatrixLinkNextOffset, &freeLink) ||
        !freeLink ||
        freeLink == MatrixListFreeTail() ||
        IsMatrixListSentinel(freeLink) ||
        !IsWritableCommitted(freeLink, 0x54) ||
        !SafeReadU32(freeLink + kMatrixLinkNextOffset, &freeNext) ||
        !SafeReadU32(MatrixListAllocatedHead() + kMatrixLinkNextOffset, &list2Next)) {
        return false;
    }

    if (!freeNext || !list2Next ||
        !IsWritableCommitted(freeNext + kMatrixLinkPrevOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(MatrixListFreeHead() + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(MatrixListAllocatedHead() + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(list2Next + kMatrixLinkPrevOffset, sizeof(uintptr_t))) {
        return false;
    }

    __try {
        *reinterpret_cast<uint32_t*>(freeNext + kMatrixLinkPrevOffset) = static_cast<uint32_t>(MatrixListFreeHead());
        *reinterpret_cast<uint32_t*>(MatrixListFreeHead() + kMatrixLinkNextOffset) = static_cast<uint32_t>(freeNext);

        *reinterpret_cast<uint32_t*>(freeLink + kMatrixLinkPrevOffset) = static_cast<uint32_t>(MatrixListAllocatedHead());
        *reinterpret_cast<uint32_t*>(freeLink + kMatrixLinkNextOffset) = static_cast<uint32_t>(list2Next);
        *reinterpret_cast<uint32_t*>(list2Next + kMatrixLinkPrevOffset) = static_cast<uint32_t>(freeLink);
        *reinterpret_cast<uint32_t*>(MatrixListAllocatedHead() + kMatrixLinkNextOffset) = static_cast<uint32_t>(freeLink);

        *reinterpret_cast<uint32_t*>(freeLink + kMatrixLinkOwnerOffset) = static_cast<uint32_t>(owner);
        *reinterpret_cast<uint32_t*>(owner + 0x14) = static_cast<uint32_t>(freeLink);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (allocatedLink) {
        *allocatedLink = freeLink;
    }
    return true;
}

uintptr_t ResolvePlaceableRemoveMatrixReturnAddress(uintptr_t esp)
{
    if (!IsReadableCommitted(esp, 0x24)) {
        return 0;
    }

    uintptr_t fallback = 0;
    __try {
        for (uintptr_t offset = 0; offset <= 0x20; offset += sizeof(uintptr_t)) {
            const uintptr_t value = *reinterpret_cast<const uintptr_t*>(esp + offset);
            if (!value) {
                continue;
            }

            if (value >= g_config.matrixGuardAllocateStaticMin && value <= g_config.matrixGuardAllocateStaticMax) {
                return value;
            }

            if (!fallback && value >= 0x00400000 && value < 0x00880000 && IsExecutableCommitted(value)) {
                fallback = value;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    return fallback;
}

extern "C" bool __stdcall Bridge_IsWritableMemory(uintptr_t address, size_t size)
{
    return IsWritableCommitted(address, size);
}

extern "C" bool __stdcall Bridge_IsExecutableMemory(uintptr_t address)
{
    return IsExecutableCommitted(address);
}

uintptr_t ResolveCleoThunk26720FinalTarget(uintptr_t dispatchObject, uintptr_t thunkTarget, bool logInvalid)
{
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    const uintptr_t cleoBase = reinterpret_cast<uintptr_t>(cleoPlus);
    if (!cleoBase || thunkTarget != cleoBase + 0x26720) {
        return thunkTarget;
    }

    uintptr_t innerObject = 0;
    uintptr_t finalTarget = 0;
    if (IsReadableCommitted(dispatchObject + 0x04, sizeof(uintptr_t))) {
        __try {
            innerObject = *reinterpret_cast<const uintptr_t*>(dispatchObject + 0x04);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            innerObject = 0;
        }
    }

    if (innerObject && IsReadableCommitted(innerObject + 0x18, sizeof(uintptr_t))) {
        __try {
            finalTarget = *reinterpret_cast<const uintptr_t*>(innerObject + 0x18);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            finalTarget = 0;
        }
    }

    if (finalTarget && IsExecutableCommitted(finalTarget)) {
        return finalTarget;
    }

    if (logInvalid) {
        static LONG logCount = 0;
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 64) {
            Log("CLEO+ dispatch guard: skipped thunk26720 final target object=0x%08X inner=0x%08X thunk=0x%08X final=0x%08X executable=%d",
                dispatchObject,
                innerObject,
                thunkTarget,
                finalTarget,
                finalTarget ? (IsExecutableCommitted(finalTarget) ? 1 : 0) : 0);
            LogMemoryRegion("cleo-thunk26720-object", dispatchObject);
            LogMemoryRegion("cleo-thunk26720-inner", innerObject);
            LogMemoryRegion("cleo-thunk26720-final", finalTarget);
        }
    }

    return 0;
}

bool IsWritableVectorArray(uintptr_t address, size_t count)
{
    return IsWritableCommitted(address, count * 3 * sizeof(uintptr_t));
}

bool LooksLikeStdVectorTriplet(uintptr_t first, uintptr_t last, uintptr_t end)
{
    if (!first && !last && !end) {
        return true;
    }
    if (!first || !last || !end || first > last || last > end) {
        return false;
    }
    if ((last - first) > 4096 * sizeof(uintptr_t) || (end - first) > 8192 * sizeof(uintptr_t)) {
        return false;
    }
    return IsReadableCommitted(first, last > first ? last - first : sizeof(uintptr_t));
}

bool ClearCleoPlusScriptEventsAt(uintptr_t eventsBase, const char* reason)
{
    constexpr size_t kScriptEventLists = 17;
    if (!IsWritableVectorArray(eventsBase, kScriptEventLists)) {
        return false;
    }

    uintptr_t* vectors = reinterpret_cast<uintptr_t*>(eventsBase);
    for (size_t i = 0; i < kScriptEventLists; ++i) {
        uintptr_t first = 0;
        uintptr_t last = 0;
        uintptr_t end = 0;
        __try {
            first = vectors[i * 3 + 0];
            last = vectors[i * 3 + 1];
            end = vectors[i * 3 + 2];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        if (!LooksLikeStdVectorTriplet(first, last, end)) {
            return false;
        }
    }

    size_t cleared = 0;
    __try {
        for (size_t i = 0; i < kScriptEventLists; ++i) {
            uintptr_t first = vectors[i * 3 + 0];
            uintptr_t last = vectors[i * 3 + 1];
            if (last != first) {
                ++cleared;
            }
            vectors[i * 3 + 1] = first;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    Log("CLEO+ scriptEvents recovery: cleared=%u base=0x%08X reason=%s",
        static_cast<unsigned>(cleared),
        eventsBase,
        reason ? reason : "");
    return true;
}

void ClearCleoPlusScriptEvents(const char* reason)
{
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    if (!base) {
        return;
    }

    static LONG clearCount = 0;
    const LONG count = InterlockedIncrement(&clearCount);
    if (count > 16) {
        return;
    }

    // Known offsets for the current restored CLEO+ build and the local rebuilt variant.
    constexpr uintptr_t kCandidateOffsets[] = { 0x000B7FC8, 0x0008E7E8 };
    for (uintptr_t offset : kCandidateOffsets) {
        if (ClearCleoPlusScriptEventsAt(base + offset, reason)) {
            return;
        }
    }

    Log("CLEO+ scriptEvents recovery: no valid vector array found reason=%s", reason ? reason : "");
}

extern "C" uintptr_t __stdcall Bridge_GetSafeCleoThunkTarget(uintptr_t target)
{
    if (target && IsExecutableCommitted(target)) {
        return target;
    }

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 32) {
        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(target, moduleName, sizeof(moduleName));
        Log("CLEO+ thunk26720 guard: skipped invalid target=0x%08X module=%s+0x%X executable=%d",
            target,
            moduleName,
            moduleBase ? target - moduleBase : 0,
            target ? (IsExecutableCommitted(target) ? 1 : 0) : 0);
        LogMemoryRegion("cleo-thunk26720-target", target);
    }

    return 0;
}

extern "C" uintptr_t __stdcall Bridge_GetSafeCleoDispatchTarget(uintptr_t dispatchObject)
{
    uintptr_t vtable = 0;
    uintptr_t target = 0;

    if (IsReadableCommitted(dispatchObject, sizeof(uintptr_t))) {
        __try {
            vtable = *reinterpret_cast<const uintptr_t*>(dispatchObject);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            vtable = 0;
        }
    }

    if (vtable && IsReadableCommitted(vtable + 0x08, sizeof(uintptr_t))) {
        __try {
            target = *reinterpret_cast<const uintptr_t*>(vtable + 0x08);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            target = 0;
        }
    }

    target = ResolveCleoThunk26720FinalTarget(dispatchObject, target, true);

    if (target && IsExecutableCommitted(target)) {
        return target;
    }

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 32) {
        Log("CLEO+ dispatch guard: skipped invalid dispatch object=0x%08X vtable=0x%08X target=0x%08X executable=%d",
            dispatchObject,
            vtable,
            target,
            target ? (IsExecutableCommitted(target) ? 1 : 0) : 0);
        LogMemoryRegion("cleo-dispatch-object", dispatchObject);
        LogMemoryRegion("cleo-dispatch-vtable", vtable);
        LogMemoryRegion("cleo-dispatch-target", target);
    }

    return 0;
}

int __stdcall Bridge_CleoDispatchExceptionFilter(EXCEPTION_POINTERS* info, uintptr_t target, uintptr_t dispatchObject)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 32) {
        const uintptr_t eip = info && info->ExceptionRecord
            ? reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress)
            : 0;
        const uintptr_t fault = info && info->ExceptionRecord && info->ExceptionRecord->NumberParameters > 1
            ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1])
            : 0;

        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(eip, moduleName, sizeof(moduleName));
        Log("CLEO+ dispatch guard: swallowed callback exception target=0x%08X object=0x%08X eip=0x%08X %s+0x%X fault=0x%08X",
            target,
            dispatchObject,
            eip,
            moduleName,
            moduleBase ? eip - moduleBase : 0,
            fault);
#if defined(_M_IX86)
        if (info && info->ContextRecord) {
            Log("CLEO+ dispatch guard context: eax=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X esp=0x%08X",
                info->ContextRecord->Eax,
                info->ContextRecord->Ecx,
                info->ContextRecord->Edx,
                info->ContextRecord->Esi,
                info->ContextRecord->Edi,
                info->ContextRecord->Esp);
            LogStackModules(info->ContextRecord->Esp);
        }
#endif
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

extern "C" void __stdcall Bridge_CallCleoDispatchTargetSafely(uintptr_t target, uintptr_t dispatchObject)
{
    if (!target || !IsExecutableCommitted(target)) {
        return;
    }

    if (target == kCPoolsInitialise) {
        Log("CLEO+ dispatch guard: blocked invalid dispatch target=CPools::Initialise object=0x%08X",
            dispatchObject);
        EnsureLazyCoreCPoolsReady("blocked CLEO+ dispatch target CPools::Initialise");
        ClearCleoPlusScriptEvents("invalid dispatch target CPools::Initialise");
        return;
    }

    __try {
        using DispatchFn = void(__thiscall*)(void*);
        reinterpret_cast<DispatchFn>(target)(reinterpret_cast<void*>(dispatchObject));
    }
    __except (Bridge_CleoDispatchExceptionFilter(GetExceptionInformation(), target, dispatchObject)) {
    }
}

using CleoOpcodeTrampolineFn = CleoExports::OpcodeResult(__stdcall*)(RunningScriptLite*);

CleoExports::OpcodeResult CallCleoPlusExtendedObjectVarSafely(
    const char* name,
    uintptr_t trampoline,
    RunningScriptLite* thread)
{
    if (!trampoline || !IsExecutableCommitted(trampoline)) {
        UpdateScriptCompareFlag(thread, false);
        return kCleoOpcodeResultContinue;
    }

    __try {
        return reinterpret_cast<CleoOpcodeTrampolineFn>(trampoline)(thread);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        static LONG logCount = 0;
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 64) {
            const uint32_t offset = GetCleoScriptOffset(thread);
            Log("CLEO+ extended object var guard: swallowed exception opcode=%s script='%.*s' offset=0x%X trampoline=0x%08X",
                name ? name : "",
                thread ? 8 : 0,
                thread ? thread->name : "",
                offset,
                trampoline);
        }
        UpdateScriptCompareFlag(thread, false);
        return kCleoOpcodeResultContinue;
    }
}

extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_InitExtendedObjectVars_Guard(RunningScriptLite* thread)
{
    return CallCleoPlusExtendedObjectVarSafely(
        "INIT_EXTENDED_OBJECT_VARS",
        g_cleoPlusInitExtendedObjectVarsTrampoline,
        thread);
}

extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_SetExtendedObjectVar_Guard(RunningScriptLite* thread)
{
    return CallCleoPlusExtendedObjectVarSafely(
        "SET_EXTENDED_OBJECT_VAR",
        g_cleoPlusSetExtendedObjectVarTrampoline,
        thread);
}

extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_GetExtendedObjectVar_Guard(RunningScriptLite* thread)
{
    return CallCleoPlusExtendedObjectVarSafely(
        "GET_EXTENDED_OBJECT_VAR",
        g_cleoPlusGetExtendedObjectVarTrampoline,
        thread);
}

extern "C" void __stdcall Bridge_LogInvalidBoundCentreOut(uintptr_t entity, uintptr_t outVec, uintptr_t returnAddress, uintptr_t stack)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count > 32) {
        return;
    }

    int32_t modelId = -1;
    if (IsReadableCommitted(entity + 0x22, sizeof(uint16_t))) {
        modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(entity + 0x22));
    }

    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));
    Log("bound centre guard: skipped null/bad out entity=0x%08X out=0x%08X model=%u return=0x%08X %s+0x%X",
        entity,
        outVec,
        modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
        returnAddress,
        moduleName,
        moduleBase ? returnAddress - moduleBase : 0);
    LogMemoryRegion("bound-centre-entity", entity);
    LogMemoryRegion("bound-centre-out", outVec);
    if (modelId < kOriginalCModelInfoCount) {
        DumpModelContext(modelId);
    }
    LogStackModules(stack);
}

extern "C" bool __stdcall Bridge_IsValidAnimHierarchy(uintptr_t hierarchy)
{
    if (hierarchy < 0x10000) {
        return false;
    }

    if (!IsReadableCommitted(hierarchy, 0x20)) {
        return false;
    }

    uint8_t compressedFlags = 0;
    __try {
        compressedFlags = *reinterpret_cast<const uint8_t*>(hierarchy + 0x0B);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    (void)compressedFlags;
    g_lastValidAnimHierarchy = hierarchy;
    return true;
}

extern "C" void __stdcall Bridge_LogInvalidAnimHierarchy(uintptr_t hierarchy, uintptr_t returnAddress, uintptr_t stack)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count > 24) {
        return;
    }

    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));
    Log("anim guard: skipped invalid CAnimBlendHierarchy=0x%08X return=0x%08X %s+0x%X",
        hierarchy,
        returnAddress,
        moduleName,
        moduleBase ? returnAddress - moduleBase : 0);
    LogMemoryRegion("anim-hierarchy", hierarchy);
    LogStackModules(stack);
}

extern "C" bool __stdcall Bridge_IsValidStaticAssociation(uintptr_t staticAssociation)
{
    if (!IsReadableCommitted(staticAssociation, 0x14)) {
        return false;
    }

    uint32_t blendSeqs = 0;
    uint32_t blendHier = 0;
    uint16_t numBlendNodes = 0;
    __try {
        numBlendNodes = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x04);
        blendSeqs = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x0C);
        blendHier = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x10);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!Bridge_IsValidAnimHierarchy(blendHier)) {
        return false;
    }

    if (numBlendNodes > 512) {
        return false;
    }

    if (numBlendNodes != 0 && !IsReadableCommitted(blendSeqs, static_cast<size_t>(numBlendNodes) * sizeof(uintptr_t))) {
        return false;
    }

    return true;
}

extern "C" void __stdcall Bridge_RepairInvalidStaticAssociation(uintptr_t runtimeAssociation, uintptr_t staticAssociation)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);

    uint16_t numBlendNodes = 0;
    uint16_t animId = 0xFFFF;
    uint16_t animGroupId = 0xFFFF;
    uint16_t flags = 0;
    uint32_t blendSeqs = 0;
    uint32_t blendHier = 0;

    if (IsReadableCommitted(staticAssociation, 0x14)) {
        __try {
            numBlendNodes = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x04);
            animId = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x06);
            animGroupId = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x08);
            flags = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x0A);
            blendSeqs = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x0C);
            blendHier = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x10);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    const uintptr_t fallbackHier = Bridge_IsValidAnimHierarchy(g_lastValidAnimHierarchy) ? g_lastValidAnimHierarchy : 0;
    constexpr uint16_t kAnimationBlendAutoRemove = 0x0004;
    constexpr float kZeroBlend = 0.0f;
    constexpr float kFadeOutNow = -1.0f;
    const uint16_t neutralizedFlags = fallbackHier ? kAnimationBlendAutoRemove : static_cast<uint16_t>(kAnimationBlendAutoRemove | 0x4000);

    if (IsReadableCommitted(runtimeAssociation, 0x30)) {
        __try {
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x0C) = 0;                 // m_NumBlendNodes
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x0E) = animGroupId;       // m_AnimGroupId
            *reinterpret_cast<uint32_t*>(runtimeAssociation + 0x10) = 0;                 // m_BlendNodes
            *reinterpret_cast<uint32_t*>(runtimeAssociation + 0x14) = static_cast<uint32_t>(fallbackHier);
            *reinterpret_cast<float*>(runtimeAssociation + 0x18) = kZeroBlend;           // m_BlendAmount
            *reinterpret_cast<float*>(runtimeAssociation + 0x1C) = kFadeOutNow;          // m_BlendDelta
            *reinterpret_cast<float*>(runtimeAssociation + 0x20) = 0.0f;                 // m_CurrentTime
            *reinterpret_cast<float*>(runtimeAssociation + 0x24) = 0.0f;                 // m_Speed
            *reinterpret_cast<float*>(runtimeAssociation + 0x28) = 0.0f;                 // m_TimeStep
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x2C) = animId;            // m_AnimId
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x2E) = neutralizedFlags;  // m_Flags
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    if (count <= 32) {
        Log("static assoc guard: neutralized invalid source=0x%08X runtime=0x%08X srcNodes=%u animId=%u group=%u flags=0x%04X neutralFlags=0x%04X seqs=0x%08X hier=0x%08X fallbackHier=0x%08X",
            staticAssociation,
            runtimeAssociation,
            numBlendNodes,
            animId,
            animGroupId,
            flags,
            neutralizedFlags,
            blendSeqs,
            blendHier,
            fallbackHier);
        LogMemoryRegion("static-assoc-source", staticAssociation);
        if (staticAssociation) {
            LogDwords("static-assoc-source", staticAssociation, 5);
        }
        LogMemoryRegion("static-assoc-seqs", blendSeqs);
        LogMemoryRegion("static-assoc-hier", blendHier);
        LogMemoryRegion("static-assoc-fallback-hier", fallbackHier);
    }
}

extern "C" bool __stdcall Bridge_PrepareAnimFrameUpdateData(uintptr_t updateData)
{
    static LONG logCount = 0;

    constexpr size_t kUpdateDataSize = 4 + 12 * sizeof(uintptr_t);
    constexpr size_t kNodeSize = 0x18;
    if (!IsReadableCommitted(updateData, kUpdateDataSize)) {
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 32) {
            Log("anim frame guard: skipped unreadable update data=0x%08X", updateData);
            LogMemoryRegion("anim-frame-update-data", updateData);
        }
        return false;
    }

    uintptr_t firstNodeArray = 0;
    __try {
        firstNodeArray = *reinterpret_cast<const uintptr_t*>(updateData + 4);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!firstNodeArray) {
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 32) {
            const uint32_t includePartial = *reinterpret_cast<const uint32_t*>(updateData);
            Log("anim frame guard: skipped empty BlendNodeArrays updateData=0x%08X includePartial=%u", updateData, includePartial);
        }
        return false;
    }

    for (int i = 0; i < 12; ++i) {
        uintptr_t node = 0;
        __try {
            node = *reinterpret_cast<const uintptr_t*>(updateData + 4 + i * sizeof(uintptr_t));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            node = 0;
        }

        if (!node) {
            return true;
        }

        bool valid = IsReadableCommitted(node, kNodeSize);
        uintptr_t sequence = 0;
        uintptr_t association = 0;
        if (valid) {
            __try {
                sequence = *reinterpret_cast<const uintptr_t*>(node + 0x10);
                association = *reinterpret_cast<const uintptr_t*>(node + 0x14);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                valid = false;
            }
        }

        if (valid && sequence && !IsReadableCommitted(sequence, 0x10)) {
            valid = false;
        }

        if (valid && sequence && !IsReadableCommitted(association, 0x30)) {
            valid = false;
        }

        if (valid) {
            continue;
        }

        if (IsWritableCommitted(updateData + 4 + i * sizeof(uintptr_t), sizeof(uintptr_t))) {
            __try {
                *reinterpret_cast<uintptr_t*>(updateData + 4 + i * sizeof(uintptr_t)) = 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 32) {
            Log("anim frame guard: truncated invalid BlendNodeArrays updateData=0x%08X index=%d node=0x%08X seq=0x%08X assoc=0x%08X",
                updateData,
                i,
                node,
                sequence,
                association);
            LogMemoryRegion("anim-frame-node", node);
            LogMemoryRegion("anim-frame-seq", sequence);
            LogMemoryRegion("anim-frame-assoc", association);
        }

        return i > 0;
    }

    return true;
}

bool CopyMemoryWithProtect(uintptr_t destination, uintptr_t source, size_t size)
{
    if (!IsReadableCommitted(source, size)) {
        Log("legacy shadow: source unreadable address=0x%08X size=0x%X", source, static_cast<unsigned>(size));
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(destination), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("legacy shadow: destination protect failed address=0x%08X size=0x%X gle=%lu",
            destination, static_cast<unsigned>(size), GetLastError());
        return false;
    }

    std::memcpy(reinterpret_cast<void*>(destination), reinterpret_cast<const void*>(source), size);

    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(destination), size, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<const void*>(destination), size);
    return true;
}

bool WriteBytesWithProtect(uintptr_t destination, const uint8_t* bytes, size_t size)
{
    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(destination), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("patch: protect failed address=0x%08X size=0x%X gle=%lu",
            destination, static_cast<unsigned>(size), GetLastError());
        return false;
    }

    std::memcpy(reinterpret_cast<void*>(destination), bytes, size);

    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(destination), size, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<const void*>(destination), size);
    return true;
}

uint32_t GetUppercaseKeyViaGame(const char* text)
{
    using GetUppercaseKeyFn = uint32_t(__cdecl*)(const char*);
    auto getUppercaseKey = reinterpret_cast<GetUppercaseKeyFn>(0x0053CF30);
    return getUppercaseKey(text);
}

void ReverseString(const char* input, char* output, size_t outputSize)
{
    if (!input || !output || outputSize == 0) {
        return;
    }

    const size_t length = std::strlen(input);
    const size_t copyLength = length < outputSize - 1 ? length : outputSize - 1;
    for (size_t i = 0; i < copyLength; ++i) {
        output[i] = input[copyLength - 1 - i];
    }
    output[copyLength] = 0;
}

void StripLineCommentAndCommas(char* line)
{
    if (!line) {
        return;
    }

    for (char* c = line; *c; ++c) {
        if (*c == '#') {
            *c = 0;
            break;
        }
        if (*c == ',') {
            *c = ' ';
        }
        if (*c == '\r' || *c == '\n') {
            *c = 0;
            break;
        }
    }
}

void LoadBridgeCheatStrings()
{
    constexpr const char* cheatPath = "data\\cheatStrings.dat";
    FILE* file = nullptr;
    if (fopen_s(&file, cheatPath, "r") != 0 || !file) {
        Log("bridge cheat loader: %s not found; keeping original cheat hashes", cheatPath);
        return;
    }

    if (!IsReadableCommitted(kCheatHashKeys, kCheatHashKeyCount * sizeof(uint32_t))) {
        Log("bridge cheat loader: cheat hash table unreadable at 0x%08X", kCheatHashKeys);
        std::fclose(file);
        return;
    }

    uint32_t newHashes[kCheatHashKeyCount]{};
    __try {
        std::memcpy(newHashes, reinterpret_cast<const void*>(kCheatHashKeys), sizeof(newHashes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bridge cheat loader: failed to copy original cheat hash table");
        std::fclose(file);
        return;
    }

    char line[1024]{};
    int lineNumber = 0;
    int applied = 0;
    int skipped = 0;

    while (std::fgets(line, sizeof(line), file)) {
        ++lineNumber;
        StripLineCommentAndCommas(line);

        char* context = nullptr;
        char* idToken = strtok_s(line, " \t", &context);
        if (!idToken) {
            continue;
        }

        char* end = nullptr;
        const long id = std::strtol(idToken, &end, 10);
        if (!end || *end != 0 || id < 0 || id >= static_cast<long>(kCheatHashKeyCount)) {
            Log("bridge cheat loader: skip line=%d invalid ID token='%s'", lineNumber, idToken);
            ++skipped;
            continue;
        }

        char* cheatText = strtok_s(nullptr, " \t", &context);
        if (!cheatText) {
            cheatText = const_cast<char*>("");
        }

        const size_t length = std::strlen(cheatText);
        if (length != 0 && (length < 6 || length > 30)) {
            Log("bridge cheat loader: skip line=%d id=%ld invalid length=%u text='%s'",
                lineNumber, id, static_cast<unsigned>(length), cheatText);
            ++skipped;
            continue;
        }

        uint32_t hash = 0;
        if (length != 0) {
            char reversed[31]{};
            ReverseString(cheatText, reversed, sizeof(reversed));
            hash = GetUppercaseKeyViaGame(reversed);
        }

        newHashes[id] = hash;
        ++applied;
        Log("bridge cheat loader: id=%ld hash=0x%08X text='%s'", id, hash, cheatText);
    }

    std::fclose(file);

    if (WriteBytesWithProtect(kCheatHashKeys, reinterpret_cast<const uint8_t*>(newHashes), sizeof(newHashes))) {
        Log("bridge cheat loader: loaded applied=%d skipped=%d table=0x%08X source=%s",
            applied, skipped, kCheatHashKeys, cheatPath);
    }
}

void InstallFlaTrainInitHookRepair()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x006F7446;
    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t bridgeTarget = reinterpret_cast<uintptr_t>(Bridge_FlaTrainInit_LoadPreserveEax);

    if (currentTarget == bridgeTarget) {
        Log("train hook repair: already installed at 0x%08X", patchAddress);
        return;
    }

    if (!currentTarget) {
        Log("train hook repair: no FLA train hook detected at 0x%08X", patchAddress);
        return;
    }

    uint8_t stub[16]{};
    if (!IsReadableCommitted(currentTarget, sizeof(stub))) {
        Log("train hook repair: FLA stub unreadable target=0x%08X", currentTarget);
        return;
    }

    __try {
        std::memcpy(stub, reinterpret_cast<const void*>(currentTarget), sizeof(stub));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("train hook repair: read exception target=0x%08X", currentTarget);
        return;
    }

    if (stub[0] != 0xB9 || stub[5] != 0xE8) {
        Log("train hook repair: unexpected FLA stub target=0x%08X bytes=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
            currentTarget, stub[0], stub[1], stub[2], stub[3], stub[4],
            stub[5], stub[6], stub[7], stub[8], stub[9]);
        return;
    }

    uintptr_t loaderThis = 0;
    int32_t callRel = 0;
    std::memcpy(&loaderThis, stub + 1, sizeof(loaderThis));
    std::memcpy(&callRel, stub + 6, sizeof(callRel));
    const uintptr_t loadFunc = currentTarget + 10 + callRel;

    if (!IsExecutableCommitted(loadFunc)) {
        Log("train hook repair: loader function not executable this=0x%08X func=0x%08X target=0x%08X",
            loaderThis, loadFunc, currentTarget);
        return;
    }

    g_flaTrainTypeCarriagesLoaderThis = loaderThis;
    g_flaTrainTypeCarriagesLoadFunc = loadFunc;

    uint8_t patch[5]{};
    patch[0] = 0xE9;
    const int32_t bridgeRel = static_cast<int32_t>(bridgeTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &bridgeRel, sizeof(bridgeRel));

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("train hook repair: installed at 0x%08X oldTarget=0x%08X this=0x%08X loadFunc=0x%08X bridge=0x%08X",
            patchAddress, currentTarget, loaderThis, loadFunc, bridgeTarget);
    }
#else
    Log("train hook repair: unsupported architecture");
#endif
}

uintptr_t DecodeRel32JumpTarget(uintptr_t address)
{
    uint8_t bytes[5]{};
    if (!IsReadableCommitted(address, sizeof(bytes))) {
        return 0;
    }

    __try {
        std::memcpy(bytes, reinterpret_cast<const void*>(address), sizeof(bytes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    if (bytes[0] != 0xE9) {
        return 0;
    }

    int32_t rel = 0;
    std::memcpy(&rel, bytes + 1, sizeof(rel));
    return address + 5 + rel;
}

bool IsExecutableCommitted(uintptr_t address)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        return false;
    }
    if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return false;
    }

    const DWORD protect = mbi.Protect & 0xFF;
    return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ ||
        protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

bool IsValidCPool(uintptr_t pool)
{
    if (!pool || !IsReadableCommitted(pool, 0x14)) {
        return false;
    }

    uint32_t size = 0;
    return SafeReadU32(pool + 0x08, &size) && size > 0 && size < 10000000;
}

bool ReadCPoolHeader(uintptr_t pool, uint32_t* size, uint32_t* firstFree)
{
    uint32_t localSize = 0;
    uint32_t localFirstFree = 0xFFFFFFFF;
    if (!IsValidCPool(pool) ||
        !SafeReadU32(pool + 0x08, &localSize) ||
        !SafeReadU32(pool + 0x0C, &localFirstFree)) {
        return false;
    }

    if (size) {
        *size = localSize;
    }
    if (firstFree) {
        *firstFree = localFirstFree;
    }
    return true;
}

bool IsPoolReadyForDeferredReplay(uintptr_t pool)
{
    uint32_t size = 0;
    uint32_t firstFree = 0xFFFFFFFF;
    if (!ReadCPoolHeader(pool, &size, &firstFree)) {
        return false;
    }

    // CPool starts with m_nFirstFree == -1 for an empty pool. A valid header is enough here:
    // replay is deferred until the core pool pointers exist and their object/bitmap storage is readable.
    return true;
}

bool ReadCorePoolPointers(uint32_t* pedPool, uint32_t* vehiclePool, uint32_t* objectPool, uint32_t* colModelPool)
{
    uint32_t ped = 0;
    uint32_t vehicle = 0;
    uint32_t object = 0;
    uint32_t colModel = 0;

    const bool ok =
        SafeReadU32(kOriginalPedPoolPtr, &ped) &&
        SafeReadU32(kOriginalVehiclePoolPtr, &vehicle) &&
        SafeReadU32(kOriginalObjectPoolPtr, &object) &&
        SafeReadU32(kOriginalColModelPoolPtr, &colModel);

    if (pedPool) {
        *pedPool = ped;
    }
    if (vehiclePool) {
        *vehiclePool = vehicle;
    }
    if (objectPool) {
        *objectPool = object;
    }
    if (colModelPool) {
        *colModelPool = colModel;
    }
    return ok;
}

bool AreCorePoolsReadyForDeferredReplay(uint32_t* pedOut, uint32_t* vehicleOut, uint32_t* objectOut, uint32_t* colModelOut)
{
    uint32_t ped = 0;
    uint32_t vehicle = 0;
    uint32_t object = 0;
    uint32_t colModel = 0;

    const bool readable = ReadCorePoolPointers(&ped, &vehicle, &object, &colModel);
    if (pedOut) {
        *pedOut = ped;
    }
    if (vehicleOut) {
        *vehicleOut = vehicle;
    }
    if (objectOut) {
        *objectOut = object;
    }
    if (colModelOut) {
        *colModelOut = colModel;
    }

    return readable &&
        IsPoolReadyForDeferredReplay(ped) &&
        IsPoolReadyForDeferredReplay(vehicle) &&
        IsPoolReadyForDeferredReplay(object) &&
        IsPoolReadyForDeferredReplay(colModel);
}

bool TryEnsureCPoolsInitialised(const char* reason)
{
    if (!g_config.enableCPoolsInitialiseRecovery) {
        return false;
    }

    uint32_t pedBefore = 0;
    uint32_t vehicleBefore = 0;
    uint32_t objectBefore = 0;
    uint32_t colBefore = 0;
    if (!ReadCorePoolPointers(&pedBefore, &vehicleBefore, &objectBefore, &colBefore)) {
        Log("CPools recovery: cannot read pool pointer table reason=%s", reason ? reason : "<null>");
        return false;
    }

    if (IsValidCPool(colBefore)) {
        return true;
    }

    const bool anyCorePoolExists = pedBefore || vehicleBefore || objectBefore || colBefore;
    if (anyCorePoolExists) {
        const LONG logCount = InterlockedIncrement(&g_cPoolsInitialiseRecoveryLogs);
        if (logCount <= 16) {
            Log("CPools recovery: skipped partial pool state reason=%s ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                reason ? reason : "<null>",
                pedBefore,
                vehicleBefore,
                objectBefore,
                colBefore);
        }
        return false;
    }

    if (InterlockedCompareExchange(&g_cPoolsInitialiseRecoveryState, 1, 0) != 0) {
        uint32_t colAfterOtherAttempt = 0;
        SafeReadU32(kOriginalColModelPoolPtr, &colAfterOtherAttempt);
        return IsValidCPool(colAfterOtherAttempt);
    }

    const LONG logCount = InterlockedIncrement(&g_cPoolsInitialiseRecoveryLogs);
    if (logCount <= 16) {
        Log("CPools recovery: calling CPools::Initialise reason=%s entry=0x%08X executable=%d before ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
            reason ? reason : "<null>",
            kCPoolsInitialise,
            IsExecutableCommitted(kCPoolsInitialise) ? 1 : 0,
            pedBefore,
            vehicleBefore,
            objectBefore,
            colBefore);
        LogBytes("CPools-Initialise-entry", kCPoolsInitialise, 32);
    }

    bool called = false;
    if (IsExecutableCommitted(kCPoolsInitialise)) {
        using CPoolsInitialiseFn = void(__cdecl*)();
        __try {
            reinterpret_cast<CPoolsInitialiseFn>(kCPoolsInitialise)();
            called = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("CPools recovery: CPools::Initialise raised exception reason=%s code=0x%08X",
                reason ? reason : "<null>",
                GetExceptionCode());
        }
    }

    uint32_t pedAfter = 0;
    uint32_t vehicleAfter = 0;
    uint32_t objectAfter = 0;
    uint32_t colAfter = 0;
    ReadCorePoolPointers(&pedAfter, &vehicleAfter, &objectAfter, &colAfter);

    uint32_t colSize = 0;
    uint32_t colFirstFree = 0;
    if (IsValidCPool(colAfter)) {
        SafeReadU32(colAfter + 0x08, &colSize);
        SafeReadU32(colAfter + 0x0C, &colFirstFree);
    }

    const bool recovered =
        called &&
        IsValidCPool(pedAfter) &&
        IsValidCPool(vehicleAfter) &&
        IsValidCPool(objectAfter) &&
        IsValidCPool(colAfter);

    Log("CPools recovery: after call called=%d recovered=%d reason=%s ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X colSize=%u colFirstFree=%u",
        called ? 1 : 0,
        recovered ? 1 : 0,
        reason ? reason : "<null>",
        pedAfter,
        vehicleAfter,
        objectAfter,
        colAfter,
        colSize,
        colFirstFree);

    InterlockedExchange(&g_cPoolsInitialiseRecoveryState, recovered ? 2 : -1);
    return recovered;
}

struct LazyCPoolSpec {
    const char* label;
    uintptr_t poolPtr;
    uintptr_t ctor;
    uintptr_t namePtr;
    const char* iniKey;
    uint32_t defaultCapacity;
    LONG state;
};

LazyCPoolSpec g_lazyCPoolSpecs[] = {
    { "PtrNode Singles", kPtrNodeSinglePoolPtr, 0x00550180, 0x00863D10, "PtrNode Singles", 70000, 0 },
    { "PtrNode Doubles", kPtrNodeDoublePoolPtr, 0x00550250, 0x00863D00, "PtrNode Doubles", 3200, 0 },
    { "EntryInfoNodes", kEntryInfoNodePoolPtr, 0x00550320, 0x00863CF0, "EntryInfoNodes", 500, 0 },
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

LazyCPoolSpec* FindLazyCPoolSpec(uintptr_t poolPtr)
{
    for (auto& spec : g_lazyCPoolSpecs) {
        if (spec.poolPtr == poolPtr) {
            return &spec;
        }
    }
    return nullptr;
}

bool EnsureLazyCPoolReady(uintptr_t poolPtr, const char* reason)
{
    if (!g_config.enableLazyCPoolRegistry) {
        return false;
    }

    LazyCPoolSpec* spec = FindLazyCPoolSpec(poolPtr);
    if (!spec) {
        Log("lazy CPool registry: no spec for poolPtr=0x%08X reason=%s",
            poolPtr,
            reason ? reason : "<null>");
        return false;
    }

    uint32_t pool = 0;
    if (SafeReadU32(spec->poolPtr, &pool) && IsValidCPool(pool)) {
        return true;
    }

    const LONG oldState = InterlockedCompareExchange(&spec->state, 1, 0);
    if (oldState != 0) {
        SafeReadU32(spec->poolPtr, &pool);
        return IsValidCPool(pool);
    }

    uint32_t capacity = ReadIniU32(spec->iniKey, spec->defaultCapacity);
    if (capacity == 0) {
        capacity = spec->defaultCapacity;
    }
    if (capacity > 10000000u) {
        capacity = 10000000u;
    }

    bool created = false;
    uintptr_t newPool = 0;

    __try {
        using OperatorNewFn = void* (__cdecl*)(uint32_t);
        using PoolCtorFn = void* (__thiscall*)(void*, int, const char*);
        auto operatorNewFn = reinterpret_cast<OperatorNewFn>(kGameOperatorNew);
        auto ctorFn = reinterpret_cast<PoolCtorFn>(spec->ctor);

        if (IsExecutableCommitted(kGameOperatorNew) && IsExecutableCommitted(spec->ctor)) {
            void* rawPool = operatorNewFn(0x14);
            if (rawPool) {
                newPool = reinterpret_cast<uintptr_t>(ctorFn(rawPool, static_cast<int>(capacity), reinterpret_cast<const char*>(spec->namePtr)));
                if (newPool && IsValidCPool(newPool)) {
                    uint32_t oldValue = 0;
                    SafeReadU32(spec->poolPtr, &oldValue);
                    if (!oldValue || !IsValidCPool(oldValue)) {
                        WriteBytesWithProtect(spec->poolPtr, reinterpret_cast<const uint8_t*>(&newPool), sizeof(newPool));
                    }
                    created = true;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("lazy CPool registry: exception while creating %s reason=%s code=0x%08X",
            spec->label,
            reason ? reason : "<null>",
            GetExceptionCode());
    }

    uint32_t finalPool = 0;
    SafeReadU32(spec->poolPtr, &finalPool);
    uint32_t size = 0;
    uint32_t firstFree = 0;
    if (IsValidCPool(finalPool)) {
        SafeReadU32(finalPool + 0x08, &size);
        SafeReadU32(finalPool + 0x0C, &firstFree);
    }

    Log("lazy CPool registry: create result label=%s reason=%s created=%d poolPtr=0x%08X ptr=0x%08X new=0x%08X ctor=0x%08X name=0x%08X size=%u firstFree=%u capacity=%u iniKey=%s",
        spec->label,
        reason ? reason : "<null>",
        created ? 1 : 0,
        spec->poolPtr,
        finalPool,
        newPool,
        spec->ctor,
        spec->namePtr,
        size,
        firstFree,
        capacity,
        spec->iniKey);

    InterlockedExchange(&spec->state, IsValidCPool(finalPool) ? 2 : -1);
    return created && IsValidCPool(finalPool);
}

bool EnsureLazyCoreCPoolsReady(const char* reason)
{
    const uintptr_t corePools[] = {
        kPtrNodeSinglePoolPtr,
        kPtrNodeDoublePoolPtr,
        kEntryInfoNodePoolPtr,
        kOriginalPedPoolPtr,
        kOriginalVehiclePoolPtr,
        kOriginalBuildingPoolPtr,
        kOriginalObjectPoolPtr,
        kOriginalDummyPoolPtr,
        kOriginalColModelPoolPtr,
        kTasksPoolPtr,
        kEventsPoolPtr,
        kPointRoutePoolPtr,
        kPatrolRoutePoolPtr,
        kNodeRoutePoolPtr,
        kTaskAllocatorPoolPtr,
        kPedIntelligencePoolPtr,
        kPedAttractorsPoolPtr,
    };

    bool allReady = true;
    for (uintptr_t poolPtr : corePools) {
        if (!EnsureLazyCPoolReady(poolPtr, reason)) {
            allReady = false;
        }
    }

    uint32_t ped = 0;
    uint32_t vehicle = 0;
    uint32_t building = 0;
    uint32_t object = 0;
    uint32_t dummy = 0;
    uint32_t colModel = 0;
    uint32_t tasks = 0;
    uint32_t events = 0;
    ReadCorePoolPointers(&ped, &vehicle, &object, &colModel);
    SafeReadU32(kOriginalBuildingPoolPtr, &building);
    SafeReadU32(kOriginalDummyPoolPtr, &dummy);
    SafeReadU32(kTasksPoolPtr, &tasks);
    SafeReadU32(kEventsPoolPtr, &events);
    Log("lazy CPool registry: core ensure result reason=%s allReady=%d ped=0x%08X vehicle=0x%08X building=0x%08X object=0x%08X dummy=0x%08X colModel=0x%08X tasks=0x%08X events=0x%08X",
        reason ? reason : "<null>",
        allReady ? 1 : 0,
        ped,
        vehicle,
        building,
        object,
        dummy,
        colModel,
        tasks,
        events);
    return allReady;
}

bool WriteRel32Jump(uintptr_t source, uintptr_t target)
{
    const int64_t diff = static_cast<int64_t>(target) - static_cast<int64_t>(source + 5);
    if (diff < INT32_MIN || diff > INT32_MAX) {
        Log("bridge stub: rel32 jump out of range source=0x%08X target=0x%08X diff=%lld",
            source, target, diff);
        return false;
    }

    uint8_t bytes[5]{ 0xE9, 0, 0, 0, 0 };
    const int32_t rel = static_cast<int32_t>(diff);
    std::memcpy(bytes + 1, &rel, sizeof(rel));
    return WriteBytesWithProtect(source, bytes, sizeof(bytes));
}

uintptr_t CreateRel32Trampoline(uintptr_t source, size_t stolenBytes)
{
    if (stolenBytes < 5 || !IsReadableCommitted(source, stolenBytes)) {
        return 0;
    }

    uint8_t* gateway = reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr, stolenBytes + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!gateway) {
        Log("trampoline: VirtualAlloc failed source=0x%08X stolen=%u gle=%lu", source, static_cast<uint32_t>(stolenBytes), GetLastError());
        return 0;
    }

    __try {
        std::memcpy(gateway, reinterpret_cast<const void*>(source), stolenBytes);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        VirtualFree(gateway, 0, MEM_RELEASE);
        Log("trampoline: read exception source=0x%08X stolen=%u", source, static_cast<uint32_t>(stolenBytes));
        return 0;
    }

    const uintptr_t jumpBackSource = reinterpret_cast<uintptr_t>(gateway) + stolenBytes;
    const uintptr_t jumpBackTarget = source + stolenBytes;
    const int64_t diff = static_cast<int64_t>(jumpBackTarget) - static_cast<int64_t>(jumpBackSource + 5);
    if (diff < INT32_MIN || diff > INT32_MAX) {
        VirtualFree(gateway, 0, MEM_RELEASE);
        Log("trampoline: jump back out of range source=0x%08X target=0x%08X diff=%lld", jumpBackSource, jumpBackTarget, diff);
        return 0;
    }

    gateway[stolenBytes] = 0xE9;
    const int32_t rel = static_cast<int32_t>(diff);
    std::memcpy(gateway + stolenBytes + 1, &rel, sizeof(rel));
    return reinterpret_cast<uintptr_t>(gateway);
}

float AbsFloat(float value)
{
    return value < 0.0f ? -value : value;
}

bool IsReasonableWorldCoord(float value)
{
    return value == value && value > -20000.0f && value < 20000.0f;
}

bool IsNearWorldOrigin2D(float x, float y)
{
    return AbsFloat(x) < 5.0f && AbsFloat(y) < 5.0f;
}

bool IsNearWorldOrigin2DLoose(float x, float y)
{
    return AbsFloat(x) < 100.0f && AbsFloat(y) < 100.0f;
}

bool IsValidScriptCoord3D(float x, float y, float z)
{
    return IsReasonableWorldCoord(x) && IsReasonableWorldCoord(y) &&
        IsReasonableWorldCoord(z) && !IsNearWorldOrigin2D(x, y);
}

void RequestPathStreamingForCoord(float x, float y, float z)
{
    if (!IsValidScriptCoord3D(x, y, z)) {
        return;
    }

    using SetPathsNeededAtPositionFn = void(__thiscall*)(void*, const float*);
    auto setPathsNeededAtPosition = reinterpret_cast<SetPathsNeededAtPositionFn>(kCPathFindSetPathsNeededAtPosition);
    float pos[3]{ x, y, z };

    __try {
        setPathsNeededAtPosition(reinterpret_cast<void*>(kThePaths), pos);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        const long count = InterlockedIncrement(&g_closestCarNode03D3FallbackLogs);
        if (count <= 16) {
            Log("03D3 fallback: SetPathsNeededAtPosition exception target=(%.2f, %.2f, %.2f)", x, y, z);
        }
    }
}

float SafeFindGroundZForCoord(float x, float y, float fallback)
{
    using FindGroundZForCoordFn = float(__cdecl*)(float, float);
    auto findGroundZForCoord = reinterpret_cast<FindGroundZForCoordFn>(kCWorldFindGroundZForCoord);

    __try {
        const float z = findGroundZForCoord(x, y);
        if (IsReasonableWorldCoord(z) && AbsFloat(z) > 0.01f) {
            return z + 1.0f;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    return fallback;
}

bool IsScriptNamed(const RunningScriptLite* script, const char* expected)
{
    if (!script || !expected) {
        return false;
    }

    char name[9]{};
    __try {
        std::memcpy(name, script->name, 8);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return _strnicmp(name, expected, 8) == 0;
}

void UpdateScriptCompareFlag(RunningScriptLite* script, bool state)
{
    using UpdateCompareFlagFn = void(__thiscall*)(RunningScriptLite*, bool);
    auto updateCompareFlag = reinterpret_cast<UpdateCompareFlagFn>(kCRunningScriptUpdateCompareFlag);

    __try {
        updateCompareFlag(script, state);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

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

bool RequestSanPabloSpecialActors()
{
    using RequestSpecialModelFn = void(__cdecl*)(int, const char*, int);
    auto requestSpecialModel = reinterpret_cast<RequestSpecialModelFn>(kCStreamingRequestSpecialModel);
    constexpr int kStreamingMissionKeepPriority = 0x04 | 0x08 | 0x10;

    __try {
        for (const auto& actor : kSanPabloSpecialActors) {
            if (!SafeModelInfoEntryAddress(actor.modelId) || !SafeStreamingInfoEntryAddress(actor.modelId)) {
                Log("sanpablo special actor bridge: missing model/streaming entry model=%u name=%s",
                    actor.modelId, actor.name);
                return false;
            }
            requestSpecialModel(static_cast<int>(actor.modelId), actor.name, kStreamingMissionKeepPriority);
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: RequestSpecialModel exception");
        return false;
    }
}

const char* GetBuiltInSpecialActorName(uint32_t modelId)
{
    switch (modelId) {
    case 290: return "SWEET";
    case 291: return "RYDER";
    case 292: return "SMOKEV";
    case 293: return "ZERO";
    case 294: return "CESAR";
    case 295: return "TENPEN";
    case 296: return "PULASKI";
    case 297: return "TRUTH";
    case 298: return "OGLOC";
    case 299: return "KENDL";
    default: return nullptr;
    }
}

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

const char* GetBuiltInSpecialActorCatalogName(uint32_t actorCode)
{
    for (const auto& entry : kBuiltInSpecialActorCatalog) {
        if (entry.code == actorCode) {
            return entry.name;
        }
    }

    return nullptr;
}

bool IsBuiltInSpecialActorCatalogName(const char* name)
{
    if (!name || !name[0]) {
        return false;
    }

    for (const auto& entry : kBuiltInSpecialActorCatalog) {
        if (_stricmp(entry.name, name) == 0) {
            return true;
        }
    }

    return false;
}

bool NormalizeSpecialActorName(const char* input, char* out, size_t outSize)
{
    if (!input || !out || outSize == 0) {
        return false;
    }

    size_t w = 0;
    for (size_t r = 0; input[r] && w + 1 < outSize; ++r) {
        const unsigned char c = static_cast<unsigned char>(input[r]);
        if (c >= 'a' && c <= 'z') {
            out[w++] = static_cast<char>(c - 'a' + 'A');
        } else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            out[w++] = static_cast<char>(c);
        } else if (c == '.') {
            break;
        }
    }
    out[w] = '\0';
    return w > 0;
}

bool AutoScanFilterAllowsName(const char* name)
{
    if (!name || !name[0]) {
        return false;
    }
    if (std::strcmp(g_config.specialActorAutoScanFilter, "*") == 0) {
        return true;
    }

    char filter[sizeof(g_config.specialActorAutoScanFilter)]{};
    strncpy_s(filter, g_config.specialActorAutoScanFilter, _TRUNCATE);

    char* context = nullptr;
    for (char* token = strtok_s(filter, ";,", &context); token; token = strtok_s(nullptr, ";,", &context)) {
        while (*token == ' ' || *token == '\t') {
            ++token;
        }
        char* end = token + std::strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';
        if (!token[0]) {
            continue;
        }
        const size_t tokenLen = std::strlen(token);
        bool wildcardPrefix = false;
        if (tokenLen > 0 && token[tokenLen - 1] == '*') {
            token[tokenLen - 1] = '\0';
            wildcardPrefix = true;
        }
        const size_t matchLen = std::strlen(token);
        if (!matchLen) {
            continue;
        }

        if (_stricmp(name, token) == 0 ||
            (wildcardPrefix && _strnicmp(name, token, matchLen) == 0) ||
            (_strnicmp(name, token, matchLen) == 0 && name[matchLen] >= '0' && name[matchLen] <= '9')) {
            return true;
        }
    }

    return false;
}

bool AddRuntimeSpecialActorName(const char* rawName, const char* source)
{
    char name[kSpecialActorNameLength]{};
    if (!NormalizeSpecialActorName(rawName, name, sizeof(name)) || !AutoScanFilterAllowsName(name)) {
        return false;
    }

    if (IsBuiltInSpecialActorCatalogName(name)) {
        return false;
    }

    for (uint32_t i = 0; i < g_runtimeSpecialActorNameCount; ++i) {
        if (_stricmp(g_runtimeSpecialActorNames[i].name, name) == 0) {
            return false;
        }
    }

    if (g_runtimeSpecialActorNameCount >= kMaxRuntimeSpecialActorNames ||
        g_runtimeSpecialActorNameCount >= static_cast<uint32_t>(g_config.specialActorMaxAutoCatalogNames)) {
        return false;
    }

    RuntimeSpecialActorName& entry = g_runtimeSpecialActorNames[g_runtimeSpecialActorNameCount];
    entry.code = static_cast<uint32_t>(g_config.specialActorAutoCatalogFirstCode) + g_runtimeSpecialActorNameCount;
    strncpy_s(entry.name, name, _TRUNCATE);
    strncpy_s(entry.source, source ? source : "scan", _TRUNCATE);
    ++g_runtimeSpecialActorNameCount;
    return true;
}

const char* GetRuntimeSpecialActorCatalogName(uint32_t actorCode)
{
    for (uint32_t i = 0; i < g_runtimeSpecialActorNameCount; ++i) {
        if (g_runtimeSpecialActorNames[i].code == actorCode) {
            return g_runtimeSpecialActorNames[i].name;
        }
    }

    return nullptr;
}

bool ExtractImgEntryBaseName(const uint8_t* bytes, size_t size, const char* extension, char* out, size_t outSize)
{
    if (!bytes || !extension || !out || outSize == 0) {
        return false;
    }

    char text[32]{};
    size_t w = 0;
    for (size_t i = 0; i < size && w + 1 < sizeof(text); ++i) {
        const unsigned char c = bytes[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.') {
            text[w++] = static_cast<char>(c);
        } else {
            break;
        }
    }
    text[w] = '\0';

    const size_t extLen = std::strlen(extension);
    const size_t textLen = std::strlen(text);
    if (textLen <= extLen + 1 || text[textLen - extLen - 1] != '.' ||
        _stricmp(text + textLen - extLen, extension) != 0) {
        return false;
    }

    text[textLen - extLen - 1] = '\0';
    return NormalizeSpecialActorName(text, out, outSize);
}

bool ImgArchiveHasTxdName(const char* imgPath, const char* wantedName)
{
    if (!imgPath || !wantedName || !wantedName[0]) {
        return false;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, imgPath, "rb") != 0 || !file) {
        return false;
    }

    char magic[4]{};
    uint32_t count = 0;
    if (std::fread(magic, 1, sizeof(magic), file) != sizeof(magic) ||
        std::fread(&count, sizeof(count), 1, file) != 1 ||
        std::memcmp(magic, "VER2", 4) != 0 ||
        count > 200000) {
        std::fclose(file);
        return false;
    }

    bool found = false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t offset = 0;
        uint32_t size = 0;
        uint8_t bytes[24]{};
        if (std::fread(&offset, sizeof(offset), 1, file) != 1 ||
            std::fread(&size, sizeof(size), 1, file) != 1 ||
            std::fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
            break;
        }
        char name[kSpecialActorNameLength]{};
        if (ExtractImgEntryBaseName(bytes, sizeof(bytes), "txd", name, sizeof(name)) &&
            _stricmp(name, wantedName) == 0) {
            found = true;
            break;
        }
    }

    std::fclose(file);
    return found;
}

void ScanSpecialActorsFromImgArchive(const char* imgPath, const char* sourceLabel)
{
    if (!imgPath) {
        return;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, imgPath, "rb") != 0 || !file) {
        return;
    }

    char magic[4]{};
    uint32_t count = 0;
    if (std::fread(magic, 1, sizeof(magic), file) != sizeof(magic) ||
        std::fread(&count, sizeof(count), 1, file) != 1 ||
        std::memcmp(magic, "VER2", 4) != 0 ||
        count > 200000) {
        std::fclose(file);
        return;
    }

    uint32_t added = 0;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t offset = 0;
        uint32_t size = 0;
        uint8_t bytes[24]{};
        if (std::fread(&offset, sizeof(offset), 1, file) != 1 ||
            std::fread(&size, sizeof(size), 1, file) != 1 ||
            std::fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
            break;
        }

        char name[kSpecialActorNameLength]{};
        if (!ExtractImgEntryBaseName(bytes, sizeof(bytes), "dff", name, sizeof(name))) {
            continue;
        }
        if (!g_config.specialActorAllowMissingTxd && !ImgArchiveHasTxdName(imgPath, name)) {
            continue;
        }
        if (AddRuntimeSpecialActorName(name, sourceLabel)) {
            ++added;
        }
    }

    std::fclose(file);
    Log("special actor catalog: scanned IMG '%s' added=%u totalAuto=%u", imgPath, added, g_runtimeSpecialActorNameCount);
}

bool HasSiblingTxdForDff(const char* dffPath, const char* baseName)
{
    char txdPath[MAX_PATH]{};
    strncpy_s(txdPath, dffPath, _TRUNCATE);
    char* slash1 = std::strrchr(txdPath, '\\');
    char* slash2 = std::strrchr(txdPath, '/');
    char* slash = slash1 > slash2 ? slash1 : slash2;
    char* fileName = slash ? slash + 1 : txdPath;
    sprintf_s(fileName, MAX_PATH - (fileName - txdPath), "%s.txd", baseName);
    if (GetFileAttributesA(txdPath) != INVALID_FILE_ATTRIBUTES) {
        return true;
    }
    sprintf_s(fileName, MAX_PATH - (fileName - txdPath), "%s.TXD", baseName);
    return GetFileAttributesA(txdPath) != INVALID_FILE_ATTRIBUTES;
}

void ScanSpecialActorsFromDirectory(const char* root, const char* sourceLabel, int depth = 0)
{
    if (!root || depth > 12) {
        return;
    }

    char pattern[MAX_PATH]{};
    sprintf_s(pattern, "%s\\*", root);
    WIN32_FIND_DATAA data{};
    HANDLE find = FindFirstFileA(pattern, &data);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0) {
            continue;
        }

        char path[MAX_PATH]{};
        sprintf_s(path, "%s\\%s", root, data.cFileName);
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ScanSpecialActorsFromDirectory(path, sourceLabel, depth + 1);
            continue;
        }

        const char* dot = std::strrchr(data.cFileName, '.');
        if (!dot) {
            continue;
        }
        if (_stricmp(dot, ".img") == 0) {
            ScanSpecialActorsFromImgArchive(path, sourceLabel);
            continue;
        }
        if (_stricmp(dot, ".dff") != 0) {
            continue;
        }

        char base[kSpecialActorNameLength]{};
        char rawName[64]{};
        strncpy_s(rawName, data.cFileName, _TRUNCATE);
        char* rawDot = std::strrchr(rawName, '.');
        if (rawDot) {
            *rawDot = '\0';
        }
        if (!NormalizeSpecialActorName(rawName, base, sizeof(base))) {
            continue;
        }
        if (!g_config.specialActorAllowMissingTxd && !HasSiblingTxdForDff(path, base)) {
            continue;
        }
        AddRuntimeSpecialActorName(base, sourceLabel);
    } while (FindNextFileA(find, &data));

    FindClose(find);
}

void BuildSpecialActorRuntimeCatalog()
{
    g_runtimeSpecialActorNameCount = 0;

    if (g_config.specialActorMaxAutoCatalogNames <= 0) {
        return;
    }

    if (g_config.specialActorAutoScanImgArchives) {
        ScanSpecialActorsFromDirectory("models", "models");
    }

    if (g_config.specialActorAutoScanModloader) {
        ScanSpecialActorsFromDirectory("modloader", "modloader");
    }

    Log("special actor catalog: built builtIn=%u auto=%u firstAutoCode=%d",
        static_cast<unsigned>(sizeof(kBuiltInSpecialActorCatalog) / sizeof(kBuiltInSpecialActorCatalog[0])),
        g_runtimeSpecialActorNameCount,
        g_config.specialActorAutoCatalogFirstCode);
    if (g_config.specialActorLogCatalog) {
        for (uint32_t i = 0; i < g_runtimeSpecialActorNameCount && i < 128; ++i) {
            Log("special actor catalog: code=%u name=%s source=%s",
                g_runtimeSpecialActorNames[i].code,
                g_runtimeSpecialActorNames[i].name,
                g_runtimeSpecialActorNames[i].source);
        }
    }
}

DWORD WINAPI SpecialActorRuntimeCatalogThread(void*)
{
    Sleep(static_cast<DWORD>(g_config.specialActorCatalogBuildDelayMs));
    BuildSpecialActorRuntimeCatalog();
    return 0;
}

bool IsSpecialActorSlotUsable(uint32_t modelId)
{
    const bool configuredRange =
        modelId >= static_cast<uint32_t>(g_config.specialActorMinModelId) &&
        modelId <= static_cast<uint32_t>(g_config.specialActorMaxModelId);
    const bool autoRange =
        g_config.specialActorAutoDetectExtendedSlots &&
        modelId <= static_cast<uint32_t>(g_config.specialActorAutoMaxModelId);

    if (!configuredRange && !autoRange) {
        const long count = InterlockedIncrement(&g_specialActorSlotRangeLogs);
        if (count <= 64) {
            Log("special actor bridge: rejected slot model=%u outside configured=[%d,%d] autoMax=%d autoDetect=%d",
                modelId,
                g_config.specialActorMinModelId,
                g_config.specialActorMaxModelId,
                g_config.specialActorAutoMaxModelId,
                g_config.specialActorAutoDetectExtendedSlots ? 1 : 0);
        }
        return false;
    }

    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (!streamingEntry) {
        const long count = InterlockedIncrement(&g_specialActorSlotRangeLogs);
        if (count <= 64) {
            Log("special actor bridge: rejected slot model=%u missing streamingEntry", modelId);
        }
        return false;
    }

    const uintptr_t modelEntry = SafeModelInfoEntryAddress(modelId);
    uint32_t modelInfo = 0;
    if (g_config.specialActorRequireModelInfoSlot &&
        (!modelEntry || !SafeReadU32(modelEntry, &modelInfo) || !modelInfo)) {
        const long count = InterlockedIncrement(&g_specialActorSlotRangeLogs);
        if (count <= 64) {
            Log("special actor bridge: rejected slot model=%u modelEntry=0x%08X modelInfo=0x%08X requireModelInfo=1",
                modelId, modelEntry, modelInfo);
        }
        return false;
    }

    return true;
}

bool ReadConfiguredSpecialActorName(uint32_t modelId, uint32_t actorCode, char* out, size_t outSize)
{
    if (!out || outSize == 0) {
        return false;
    }

    out[0] = '\0';
    char key[64]{};
    if (actorCode > 0) {
        sprintf_s(key, "SpecialActorName%03u", actorCode);
        if (ReadSmallTextValue(kConfigPath, key, out, outSize) && out[0]) {
            return true;
        }

        const char* builtInCatalogName = GetBuiltInSpecialActorCatalogName(actorCode);
        if (builtInCatalogName) {
            strncpy_s(out, outSize, builtInCatalogName, _TRUNCATE);
            return true;
        }

        const char* runtimeCatalogName = GetRuntimeSpecialActorCatalogName(actorCode);
        if (runtimeCatalogName) {
            strncpy_s(out, outSize, runtimeCatalogName, _TRUNCATE);
            return true;
        }

        return false;
    }

    sprintf_s(key, "SpecialActor%u", modelId);
    if (ReadSmallTextValue(kConfigPath, key, out, outSize) && out[0]) {
        return true;
    }

    const char* builtIn = GetBuiltInSpecialActorName(modelId);
    if (!builtIn) {
        return false;
    }

    strncpy_s(out, outSize, builtIn, _TRUNCATE);
    return true;
}

bool RequestSpecialActorName(uint32_t modelId, const char* name)
{
    if (!name || !name[0]) {
        return false;
    }

    if (!IsSpecialActorSlotUsable(modelId)) {
        Log("special actor bridge: missing model/streaming entry model=%u name=%s", modelId, name);
        return false;
    }

    using RequestSpecialModelFn = void(__cdecl*)(int, const char*, int);
    auto requestSpecialModel = reinterpret_cast<RequestSpecialModelFn>(kCStreamingRequestSpecialModel);
    constexpr int kStreamingMissionKeepPriority = 0x04 | 0x08 | 0x10;

    __try {
        requestSpecialModel(static_cast<int>(modelId), name, kStreamingMissionKeepPriority);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: RequestSpecialModel exception model=%u name=%s", modelId, name);
        return false;
    }
}

bool RequestConfiguredSpecialActor(uint32_t modelId, uint32_t actorCode)
{
    if (actorCode > 0 && !g_runtimeSpecialActorNameCount) {
        BuildSpecialActorRuntimeCatalog();
    }

    char name[64]{};
    if (!ReadConfiguredSpecialActorName(modelId, actorCode, name, sizeof(name))) {
        Log("special actor bridge: no configured name for model=%u actorCode=%u; add SpecialActor%u = NAME or SpecialActorName%03u = NAME",
            modelId, actorCode, modelId, actorCode);
        return false;
    }

    return RequestSpecialActorName(modelId, name);
}

bool ReleaseConfiguredSpecialActor(uint32_t modelId)
{
    if (!SafeStreamingInfoEntryAddress(modelId)) {
        return false;
    }

    using SetMissionDoesntRequireModelFn = void(__cdecl*)(int);
    auto setMissionDoesntRequireModel = reinterpret_cast<SetMissionDoesntRequireModelFn>(kCStreamingSetMissionDoesntRequireModel);

    __try {
        setMissionDoesntRequireModel(static_cast<int>(modelId));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: SetMissionDoesntRequireModel exception model=%u", modelId);
        return false;
    }
}

bool DecodeGenericSpecialActorMode(int mode, int* op, uint32_t* modelId, uint32_t* actorCode)
{
    if (!op || !modelId || !actorCode) {
        return false;
    }

    *actorCode = 0;
    if (mode < 0) {
        const uint32_t encoded = static_cast<uint32_t>(mode);
        *op = static_cast<int>((encoded >> 29) & 0x03);
        *actorCode = (encoded >> 18) & 0x07FF;
        *modelId = encoded & 0x0003FFFF;
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    if (mode >= 10000000 && mode < 40000000) {
        *op = mode / 10000000;
        const int rest = mode % 10000000;
        *modelId = static_cast<uint32_t>(rest / 1000);
        *actorCode = static_cast<uint32_t>(rest % 1000);
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    if (mode >= 100000 && mode < 400000) {
        *op = mode / 100000;
        *modelId = static_cast<uint32_t>(mode % 100000);
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    if (mode >= 10000 && mode < 40000) {
        *op = mode / 10000;
        *modelId = static_cast<uint32_t>(mode % 10000);
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    return false;
}

bool AreSanPabloSpecialActorsLoaded()
{
    for (const auto& actor : kSanPabloSpecialActors) {
        if (GetStreamingLoadState(actor.modelId) != 1 || !IsModelRwObjectLoaded(actor.modelId)) {
            return false;
        }
    }
    return true;
}

bool ReleaseSanPabloSpecialActors()
{
    using SetMissionDoesntRequireModelFn = void(__cdecl*)(int);
    auto setMissionDoesntRequireModel = reinterpret_cast<SetMissionDoesntRequireModelFn>(kCStreamingSetMissionDoesntRequireModel);

    __try {
        for (const auto& actor : kSanPabloSpecialActors) {
            setMissionDoesntRequireModel(static_cast<int>(actor.modelId));
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: SetMissionDoesntRequireModel exception");
        return false;
    }
}

bool IsNumberVariableOperandType(int operandType)
{
    return operandType == kScriptParamGlobalNumberVariable ||
        operandType == kScriptParamLocalNumberVariable ||
        operandType == kScriptParamGlobalNumberArray ||
        operandType == kScriptParamLocalNumberArray;
}

bool HandleSpecialActorBridgeMode(RunningScriptLite* script, int mode, int* outResult, int* outGenericOp, uint32_t* outGenericModelId, uint32_t* outGenericActorCode)
{
    if (!outResult || !outGenericOp || !outGenericModelId || !outGenericActorCode) {
        return false;
    }

    *outResult = 0;
    *outGenericOp = 0;
    *outGenericModelId = 0;
    *outGenericActorCode = 0;

    const bool isSanPabloScript = IsScriptNamed(script, "SANPABLO");
    const bool isGenericBridgeMode = DecodeGenericSpecialActorMode(mode, outGenericOp, outGenericModelId, outGenericActorCode);
    if (!isSanPabloScript && !isGenericBridgeMode) {
        return false;
    }

    if (isGenericBridgeMode) {
        switch (*outGenericOp) {
        case 1:
            *outResult = RequestConfiguredSpecialActor(*outGenericModelId, *outGenericActorCode) ? 1 : 0;
            break;
        case 2:
            *outResult = (GetStreamingLoadState(*outGenericModelId) == 1 && IsModelRwObjectLoaded(*outGenericModelId)) ? 1 : 0;
            break;
        case 3:
            *outResult = ReleaseConfiguredSpecialActor(*outGenericModelId) ? 1 : 0;
            break;
        default:
            break;
        }
        return true;
    }

    switch (mode) {
    case 1:
        *outResult = RequestSanPabloSpecialActors() ? 1 : 0;
        break;
    case 2:
        *outResult = AreSanPabloSpecialActorsLoaded() ? 1 : 0;
        break;
    case 3:
        *outResult = ReleaseSanPabloSpecialActors() ? 1 : 0;
        break;
    default:
        Log("sanpablo special actor bridge: unknown mode=%d", mode);
        break;
    }
    return true;
}

signed char WINAPI Bridge_CleoScriptOpcodeProcessBefore(RunningScriptLite* script, DWORD opcode)
{
    if (opcode != kCommandUnloadSpecialCharacterHexAlias ||
        !g_config.enableSanPabloSpecialActorBridge ||
        !script ||
        !script->currentIP ||
        !g_cleoExports.getOperandType ||
        !g_cleoExports.peekPointerToScriptVariable ||
        !g_cleoExports.skipOpcodeParams) {
        return kCleoOpcodeResultNone;
    }

    int operandType = -1;
    ScriptParamLite* modeVariable = nullptr;
    __try {
        operandType = g_cleoExports.getOperandType(script);
        if (!IsNumberVariableOperandType(operandType)) {
            return kCleoOpcodeResultNone;
        }
        modeVariable = g_cleoExports.peekPointerToScriptVariable(script);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO opcode-before param peek exception script='%.*s'", 8, script->name);
        return kCleoOpcodeResultNone;
    }

    if (!modeVariable || !IsWritableCommitted(reinterpret_cast<uintptr_t>(modeVariable), sizeof(*modeVariable))) {
        return kCleoOpcodeResultNone;
    }

    int mode = 0;
    __try {
        mode = modeVariable->iParam;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return kCleoOpcodeResultNone;
    }

    int result = 0;
    int genericOp = 0;
    uint32_t genericModelId = 0;
    uint32_t genericActorCode = 0;
    if (!HandleSpecialActorBridgeMode(script, mode, &result, &genericOp, &genericModelId, &genericActorCode)) {
        return kCleoOpcodeResultNone;
    }

    __try {
        g_cleoExports.skipOpcodeParams(script, 1);
        modeVariable->iParam = result;
        if (g_cleoExports.setThreadCondResult) {
            g_cleoExports.setThreadCondResult(script, result != 0 ? TRUE : FALSE);
        } else {
            UpdateScriptCompareFlag(script, result != 0);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO opcode-before write exception script='%.*s' mode=%d result=%d",
            8, script->name, mode, result);
        return kCleoOpcodeResultNone;
    }

    const long count = InterlockedIncrement(&g_sanPabloSpecialActorBridgeLogs);
    if (count <= 96 || result == 0) {
        Log("special actor bridge: CLEO before script='%.*s' mode=%d genericOp=%d genericModel=%u genericActorCode=%u result=%d sanpabloLoaded=%d states=[%u,%u,%u,%u,%u] operand=%d var=0x%08X",
            8,
            script->name,
            mode,
            genericOp,
            genericModelId,
            genericActorCode,
            result,
            AreSanPabloSpecialActorsLoaded() ? 1 : 0,
            static_cast<unsigned>(GetStreamingLoadState(290)),
            static_cast<unsigned>(GetStreamingLoadState(291)),
            static_cast<unsigned>(GetStreamingLoadState(292)),
            static_cast<unsigned>(GetStreamingLoadState(293)),
            static_cast<unsigned>(GetStreamingLoadState(294)),
            operandType,
            reinterpret_cast<uintptr_t>(modeVariable));
    }

    return kCleoOpcodeResultContinue;
}

signed char CallNativeCleoOpcode0296(RunningScriptLite* script)
{
    if (g_cleoExports.callNativeOpcode) {
        __try {
            return g_cleoExports.callNativeOpcode(script, kCommandUnloadSpecialCharacterHexAlias);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("special actor bridge: CLEO_CallNativeOpcode(0296) exception script='%.*s'", 8, script ? script->name : "");
        }
    }
    return -1;
}

signed char __stdcall Bridge_CleoOpcode0296(RunningScriptLite* script)
{
    if (!g_config.enableSanPabloSpecialActorBridge ||
        !script ||
        !script->currentIP ||
        !g_cleoExports.getOperandType ||
        !g_cleoExports.peekPointerToScriptVariable ||
        !g_cleoExports.skipOpcodeParams ||
        !g_cleoExports.callNativeOpcode) {
        return CallNativeCleoOpcode0296(script);
    }

    int operandType = -1;
    ScriptParamLite* modeVariable = nullptr;
    __try {
        operandType = g_cleoExports.getOperandType(script);
        if (!IsNumberVariableOperandType(operandType)) {
            return CallNativeCleoOpcode0296(script);
        }
        modeVariable = g_cleoExports.peekPointerToScriptVariable(script);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO 0296 param peek exception script='%.*s'", 8, script->name);
        return CallNativeCleoOpcode0296(script);
    }

    if (!modeVariable || !IsWritableCommitted(reinterpret_cast<uintptr_t>(modeVariable), sizeof(*modeVariable))) {
        return CallNativeCleoOpcode0296(script);
    }

    int mode = 0;
    __try {
        mode = modeVariable->iParam;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return CallNativeCleoOpcode0296(script);
    }

    int result = 0;
    int genericOp = 0;
    uint32_t genericModelId = 0;
    uint32_t genericActorCode = 0;
    if (!HandleSpecialActorBridgeMode(script, mode, &result, &genericOp, &genericModelId, &genericActorCode)) {
        return CallNativeCleoOpcode0296(script);
    }

    __try {
        g_cleoExports.skipOpcodeParams(script, 1);
        modeVariable->iParam = result;
        if (g_cleoExports.setThreadCondResult) {
            g_cleoExports.setThreadCondResult(script, result != 0 ? TRUE : FALSE);
        } else {
            UpdateScriptCompareFlag(script, result != 0);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO 0296 write exception script='%.*s' mode=%d result=%d",
            8, script->name, mode, result);
        return -1;
    }

    const long count = InterlockedIncrement(&g_sanPabloSpecialActorBridgeLogs);
    if (count <= 96 || result == 0) {
        Log("special actor bridge: CLEO 0296 script='%.*s' mode=%d genericOp=%d genericModel=%u genericActorCode=%u result=%d sanpabloLoaded=%d states=[%u,%u,%u,%u,%u] operand=%d var=0x%08X",
            8,
            script->name,
            mode,
            genericOp,
            genericModelId,
            genericActorCode,
            result,
            AreSanPabloSpecialActorsLoaded() ? 1 : 0,
            static_cast<unsigned>(GetStreamingLoadState(290)),
            static_cast<unsigned>(GetStreamingLoadState(291)),
            static_cast<unsigned>(GetStreamingLoadState(292)),
            static_cast<unsigned>(GetStreamingLoadState(293)),
            static_cast<unsigned>(GetStreamingLoadState(294)),
            operandType,
            reinterpret_cast<uintptr_t>(modeVariable));
    }

    return kCleoOpcodeResultContinue;
}

unsigned char __fastcall Bridge_ProcessCommands600To699(RunningScriptLite* script, void*, unsigned short commandID)
{
    if (!g_originalCommands600To699) {
        return 0;
    }

    if (InterlockedCompareExchange(&g_forwardingCommands600To699, 0, 0) != 0) {
        auto vanilla = reinterpret_cast<ScriptCommandHandlerFn>(kProcessCommands600To699);
        return vanilla(script, commandID);
    }

    if ((commandID != kCommandUnloadSpecialCharacter && commandID != kCommandUnloadSpecialCharacterHexAlias) ||
        !g_config.enableSanPabloSpecialActorBridge ||
        !script ||
        !script->currentIP) {
        InterlockedExchange(&g_forwardingCommands600To699, 1);
        const unsigned char result = g_originalCommands600To699(script, commandID);
        InterlockedExchange(&g_forwardingCommands600To699, 0);
        return result;
    }

    auto collectParameters = reinterpret_cast<ScriptCollectParametersFn>(kCRunningScriptCollectParameters);
    auto storeParameters = reinterpret_cast<ScriptStoreParametersFn>(kCRunningScriptStoreParameters);
    auto params = reinterpret_cast<ScriptParamLite*>(kScriptParams);
    uint8_t* const commandIp = script->currentIP;

    int mode = 0;
    bool collected = false;
    __try {
        collectParameters(script, 1);
        mode = params[0].iParam;
        collected = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = commandIp;
    }

    int genericOp = 0;
    uint32_t genericModelId = 0;
    uint32_t genericActorCode = 0;
    const bool isSanPabloScript = IsScriptNamed(script, "SANPABLO");
    const bool isGenericBridgeMode = collected && DecodeGenericSpecialActorMode(mode, &genericOp, &genericModelId, &genericActorCode);
    if (!isSanPabloScript && !isGenericBridgeMode) {
        script->currentIP = commandIp;
        InterlockedExchange(&g_forwardingCommands600To699, 1);
        const unsigned char result = g_originalCommands600To699(script, commandID);
        InterlockedExchange(&g_forwardingCommands600To699, 0);
        return result;
    }

    int result = 0;
    if (isGenericBridgeMode) {
        switch (genericOp) {
        case 1:
            result = RequestConfiguredSpecialActor(genericModelId, genericActorCode) ? 1 : 0;
            break;
        case 2:
            result = (GetStreamingLoadState(genericModelId) == 1 && IsModelRwObjectLoaded(genericModelId)) ? 1 : 0;
            break;
        case 3:
            result = ReleaseConfiguredSpecialActor(genericModelId) ? 1 : 0;
            break;
        default:
            break;
        }
    } else if (collected) {
        switch (mode) {
        case 1:
            result = RequestSanPabloSpecialActors() ? 1 : 0;
            break;
        case 2:
            result = AreSanPabloSpecialActorsLoaded() ? 1 : 0;
            break;
        case 3:
            result = ReleaseSanPabloSpecialActors() ? 1 : 0;
            break;
        default:
            Log("sanpablo special actor bridge: unknown mode=%d", mode);
            break;
        }
    }

    __try {
        params[0].iParam = result;
        storeParameters(script, 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: StoreParameters exception mode=%d result=%d", mode, result);
    }

    UpdateScriptCompareFlag(script, result != 0);

    const long count = InterlockedIncrement(&g_sanPabloSpecialActorBridgeLogs);
    if (count <= 64 || result == 0) {
        Log("special actor bridge: script='%.*s' mode=%d genericOp=%d genericModel=%u genericActorCode=%u result=%d sanpabloLoaded=%d states=[%u,%u,%u,%u,%u]",
            8,
            script->name,
            mode,
            genericOp,
            genericModelId,
            genericActorCode,
            result,
            AreSanPabloSpecialActorsLoaded() ? 1 : 0,
            static_cast<unsigned>(GetStreamingLoadState(290)),
            static_cast<unsigned>(GetStreamingLoadState(291)),
            static_cast<unsigned>(GetStreamingLoadState(292)),
            static_cast<unsigned>(GetStreamingLoadState(293)),
            static_cast<unsigned>(GetStreamingLoadState(294)));
    }

    return 0;
}

void InstallSanPabloSpecialActorBridge()
{
#if defined(_M_IX86)
    if (ResolveCleoExports() &&
        g_cleoExports.registerOpcode &&
        g_cleoExports.callNativeOpcode &&
        g_cleoExports.getOperandType &&
        g_cleoExports.peekPointerToScriptVariable &&
        g_cleoExports.skipOpcodeParams &&
        InterlockedCompareExchange(&g_sanPabloCleoOpcodeRegistered, 1, 0) == 0) {
        __try {
            if (g_cleoExports.registerOpcode(kCommandUnloadSpecialCharacterHexAlias, Bridge_CleoOpcode0296)) {
                Log("sanpablo special actor bridge: registered CLEO opcode=0x%04X handler=0x%08X nativeForward=%p",
                    kCommandUnloadSpecialCharacterHexAlias,
                    reinterpret_cast<uintptr_t>(Bridge_CleoOpcode0296),
                    g_cleoExports.callNativeOpcode);
                return;
            }
            InterlockedExchange(&g_sanPabloCleoOpcodeRegistered, 0);
            Log("sanpablo special actor bridge: CLEO_RegisterOpcode failed opcode=0x%04X; falling back to handler table",
                kCommandUnloadSpecialCharacterHexAlias,
                reinterpret_cast<uintptr_t>(Bridge_CleoOpcode0296));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            InterlockedExchange(&g_sanPabloCleoOpcodeRegistered, 0);
            Log("sanpablo special actor bridge: CLEO opcode registration exception; falling back to handler table");
        }
    }

    auto table = reinterpret_cast<uintptr_t*>(kScriptCommandHandlerTable);
    if (!IsReadableCommitted(kScriptCommandHandlerTable, kCommandHandlerTableCount * sizeof(uintptr_t))) {
        Log("sanpablo special actor bridge: command handler table unreadable at 0x%08X", kScriptCommandHandlerTable);
        return;
    }

    uintptr_t currentHandler = 0;
    const uintptr_t slot = kScriptCommandHandlerTable + kCommandHandlerTableIndex600To699 * sizeof(uintptr_t);
    __try {
        currentHandler = table[kCommandHandlerTableIndex600To699];
        if (currentHandler == reinterpret_cast<uintptr_t>(Bridge_ProcessCommands600To699)) {
            Log("sanpablo special actor bridge: already installed tableIndex=%u slot=0x%08X",
                kCommandHandlerTableIndex600To699, slot);
            return;
        }
        if (table[0] == currentHandler && table[1] == currentHandler && table[2] == currentHandler &&
            table[3] == currentHandler && table[4] == currentHandler && table[5] == currentHandler) {
            Log("sanpablo special actor bridge: shared dispatcher detected handler=0x%08X; installing with recursion guard",
                currentHandler);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: command handler table scan exception");
        return;
    }

    if (!currentHandler || !IsExecutableCommitted(currentHandler)) {
        Log("sanpablo special actor bridge: current 600-699 handler not executable tableIndex=%u handler=0x%08X vanilla=0x%08X",
            kCommandHandlerTableIndex600To699, currentHandler, kProcessCommands600To699);
        return;
    }

    g_originalCommands600To699 = reinterpret_cast<ScriptCommandHandlerFn>(currentHandler);
    g_commands600To699TableSlot = slot;
    const uintptr_t bridge = reinterpret_cast<uintptr_t>(Bridge_ProcessCommands600To699);
    if (WriteBytesWithProtect(slot, reinterpret_cast<const uint8_t*>(&bridge), sizeof(bridge))) {
        Log("sanpablo special actor bridge: installed tableIndex=%u tableSlot=0x%08X old=0x%08X vanilla=0x%08X bridge=0x%08X",
            kCommandHandlerTableIndex600To699, slot, currentHandler, kProcessCommands600To699, bridge);
    }
#else
    Log("sanpablo special actor bridge: unsupported architecture");
#endif
}

unsigned char __fastcall Bridge_ProcessCommands900To999(RunningScriptLite* script, void*, unsigned short commandID)
{
    if (!g_originalCommands900To999) {
        return 0;
    }

    if (InterlockedCompareExchange(&g_forwardingCommands900To999, 0, 0) != 0) {
        auto vanilla = reinterpret_cast<ScriptCommandHandlerFn>(kProcessCommands900To999);
        return vanilla(script, commandID);
    }

    if (commandID != kCommandGetClosestCarNodeWithHeading || !script || !script->currentIP) {
        InterlockedExchange(&g_forwardingCommands900To999, 1);
        const unsigned char result = g_originalCommands900To999(script, commandID);
        InterlockedExchange(&g_forwardingCommands900To999, 0);
        return result;
    }

    auto collectParameters = reinterpret_cast<ScriptCollectParametersFn>(kCRunningScriptCollectParameters);
    auto storeParameters = reinterpret_cast<ScriptStoreParametersFn>(kCRunningScriptStoreParameters);
    auto params = reinterpret_cast<ScriptParamLite*>(kScriptParams);

    uint8_t* const commandIp = script->currentIP;
    float inX = 0.0f;
    float inY = 0.0f;
    float inZ = 0.0f;
    bool haveInput = false;

    __try {
        collectParameters(script, 3);
        inX = params[0].fParam;
        inY = params[1].fParam;
        inZ = params[2].fParam;
        haveInput = IsValidScriptCoord3D(inX, inY, inZ);
        script->currentIP = commandIp;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = commandIp;
    }

    if (haveInput) {
        RequestPathStreamingForCoord(inX, inY, inZ);
    }

    InterlockedExchange(&g_forwardingCommands900To999, 1);
    const unsigned char result = g_originalCommands900To999(script, commandID);
    InterlockedExchange(&g_forwardingCommands900To999, 0);
    uint8_t* const afterOriginalIp = script->currentIP;

    if (!haveInput) {
        return result;
    }

    const float outX = params[0].fParam;
    const float outY = params[1].fParam;
    const float outZ = params[2].fParam;
    const float outHeading = params[3].fParam;

    if (!IsNearWorldOrigin2D(outX, outY) || IsNearWorldOrigin2D(inX, inY)) {
        return result;
    }

    float fallbackZ = inZ;
    if (AbsFloat(fallbackZ) < 1.0f) {
        fallbackZ = SafeFindGroundZForCoord(inX, inY, 10.0f);
    }

    float fallbackHeading = outHeading;
    if (!IsReasonableWorldCoord(fallbackHeading)) {
        fallbackHeading = 0.0f;
    }

    __try {
        script->currentIP = commandIp;
        collectParameters(script, 3);
        params[0].fParam = inX;
        params[1].fParam = inY;
        params[2].fParam = fallbackZ;
        params[3].fParam = fallbackHeading;
        storeParameters(script, 4);
        script->currentIP = afterOriginalIp;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = afterOriginalIp;
        const long count = InterlockedIncrement(&g_closestCarNode03D3FallbackLogs);
        if (count <= 16) {
            Log("03D3 fallback: failed to store replacement input=(%.2f, %.2f, %.2f) originalOut=(%.2f, %.2f, %.2f)",
                inX, inY, inZ, outX, outY, outZ);
        }
        return result;
    }

    const long count = InterlockedIncrement(&g_closestCarNode03D3FallbackLogs);
    if (count <= 64) {
        Log("03D3 fallback: replaced origin output originalOut=(%.2f, %.2f, %.2f, %.2f) input=(%.2f, %.2f, %.2f) fallback=(%.2f, %.2f, %.2f, %.2f) script='%.*s'",
            outX, outY, outZ, outHeading,
            inX, inY, inZ,
            inX, inY, fallbackZ, fallbackHeading,
            8, script->name);
    }

    return result;
}

void InstallClosestCarNode03D3Fallback()
{
#if defined(_M_IX86)
    auto table = reinterpret_cast<uintptr_t*>(kScriptCommandHandlerTable);
    if (!IsReadableCommitted(kScriptCommandHandlerTable, kCommandHandlerTableCount * sizeof(uintptr_t))) {
        Log("03D3 fallback: command handler table unreadable at 0x%08X", kScriptCommandHandlerTable);
        return;
    }

    uintptr_t currentHandler = 0;
    const uintptr_t slot = kScriptCommandHandlerTable + kCommandHandlerTableIndex900To999 * sizeof(uintptr_t);
    __try {
        currentHandler = table[kCommandHandlerTableIndex900To999];
        if (currentHandler == reinterpret_cast<uintptr_t>(Bridge_ProcessCommands900To999)) {
            Log("03D3 fallback: already installed tableIndex=%u slot=0x%08X",
                kCommandHandlerTableIndex900To999, slot);
            return;
        }
        if (table[0] == currentHandler && table[1] == currentHandler && table[2] == currentHandler &&
            table[3] == currentHandler && table[4] == currentHandler && table[5] == currentHandler) {
            Log("03D3 fallback: shared dispatcher detected handler=0x%08X; installing with recursion guard",
                currentHandler);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("03D3 fallback: command handler table scan exception");
        return;
    }

    if (!currentHandler || !IsExecutableCommitted(currentHandler)) {
        Log("03D3 fallback: current 900-999 handler not executable tableIndex=%u handler=0x%08X vanilla=0x%08X",
            kCommandHandlerTableIndex900To999, currentHandler, kProcessCommands900To999);
        return;
    }

    g_originalCommands900To999 = reinterpret_cast<ScriptCommandHandlerFn>(currentHandler);
    g_commands900To999TableSlot = slot;
    const uintptr_t bridge = reinterpret_cast<uintptr_t>(Bridge_ProcessCommands900To999);
    if (WriteBytesWithProtect(slot, reinterpret_cast<const uint8_t*>(&bridge), sizeof(bridge))) {
        Log("03D3 fallback: installed tableIndex=%u tableSlot=0x%08X old=0x%08X vanilla=0x%08X bridge=0x%08X",
            kCommandHandlerTableIndex900To999, slot, currentHandler, kProcessCommands900To999, bridge);
    }
#else
    Log("03D3 fallback: unsupported architecture");
#endif
}

bool IsTaxi77Script(const RunningScriptLite* script)
{
    if (!script) {
        return false;
    }

    char name[9]{};
    __try {
        std::memcpy(name, script->name, 8);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return ContainsCaseInsensitive(name, "taxi77");
}

bool SetLocalFloatFromParam(RunningScriptLite* script, const uint8_t* param, float value)
{
    if (!script || !param || !IsReadableCommitted(reinterpret_cast<uintptr_t>(param), 3)) {
        return false;
    }

    uint8_t type = 0;
    uint16_t index = 0;
    __try {
        type = param[0];
        std::memcpy(&index, param + 1, sizeof(index));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (type != 0x03 || index >= 32) {
        return false;
    }

    auto locals = reinterpret_cast<ScriptParamLite*>(reinterpret_cast<uintptr_t>(script) + kRunningScriptLocalVarsOffset);
    if (!IsWritableCommitted(reinterpret_cast<uintptr_t>(&locals[index]), sizeof(ScriptParamLite))) {
        return false;
    }

    __try {
        locals[index].fParam = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SetScriptCarCoordinatesSafe(uint32_t carHandle, float x, float y, float z)
{
    using GetVehicleFn = void*(__cdecl*)(int);
    using SetCoordsOfScriptCarFn = void(__cdecl*)(void*, float, float, float, uint8_t, uint8_t);

    auto getVehicle = reinterpret_cast<GetVehicleFn>(kCPoolsGetVehicle);
    auto setCoords = reinterpret_cast<SetCoordsOfScriptCarFn>(kCCarCtrlSetCoordsOfScriptCar);

    __try {
        void* vehicle = getVehicle(static_cast<int>(carHandle));
        if (!vehicle || !IsReadableCommitted(reinterpret_cast<uintptr_t>(vehicle), 4)) {
            return false;
        }

        setCoords(vehicle, x, y, z, 0, 0);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

unsigned char __fastcall Bridge_ProcessCommands100To199(RunningScriptLite* script, void*, int commandID)
{
    if (!g_originalProcessCommands100To199) {
        return 0;
    }

    if (commandID != kCommandSetCarCoordinates || !script || !script->currentIP || !IsTaxi77Script(script)) {
        return g_originalProcessCommands100To199(script, commandID);
    }

    auto collectParameters = reinterpret_cast<ScriptCollectParametersFn>(kCRunningScriptCollectParameters);
    auto params = reinterpret_cast<ScriptParamLite*>(kScriptParams);

    uint8_t* const commandIp = script->currentIP;
    uint32_t car = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool collected = false;

    __try {
        collectParameters(script, 4);
        car = params[0].uParam;
        x = params[1].fParam;
        y = params[2].fParam;
        z = params[3].fParam;
        collected = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = commandIp;
    }

    if (!collected) {
        return g_originalProcessCommands100To199(script, commandID);
    }

    const uintptr_t baseIp = reinterpret_cast<uintptr_t>(script->baseIP);
    const uintptr_t paramsOffset = baseIp ? reinterpret_cast<uintptr_t>(commandIp) - baseIp : 0;
    const uintptr_t opcodeOffset = paramsOffset >= 2 ? paramsOffset - 2 : paramsOffset;

    const bool originWrite = IsNearWorldOrigin2DLoose(x, y);
    const LONG count = InterlockedIncrement(&g_taxi77SetCarCoordinatesLogs);
    if (count <= 128 || originWrite) {
        Log("taxi77 00AB: %s script='%.*s' opcodeOff=0x%X paramsOff=0x%X car=%u coords=(%.2f, %.2f, %.2f)",
            originWrite ? "originish" : "pass",
            8,
            script->name,
            static_cast<uint32_t>(opcodeOffset),
            static_cast<uint32_t>(paramsOffset),
            car,
            x,
            y,
            z);
    }

    if (originWrite) {
        if (InterlockedCompareExchange(&g_hasLastTargetBlipCoords, 0, 0) == 1) {
            float targetZ = g_lastTargetBlipZ;
            if (AbsFloat(targetZ) < 1.0f) {
                targetZ = SafeFindGroundZForCoord(g_lastTargetBlipX, g_lastTargetBlipY, 10.0f);
            }
            if (AbsFloat(targetZ) < 1.0f) {
                targetZ = 10.0f;
            }
            const bool moved = SetScriptCarCoordinatesSafe(car, g_lastTargetBlipX, g_lastTargetBlipY, targetZ + 2.0f);
            Log("taxi77 00AB: repaired-origin opcodeOff=0x%X old=(%.2f, %.2f, %.2f) target=(%.2f, %.2f, %.2f) moved=%d",
                static_cast<uint32_t>(opcodeOffset),
                x,
                y,
                z,
                g_lastTargetBlipX,
                g_lastTargetBlipY,
                targetZ + 2.0f,
                moved ? 1 : 0);
            return 0;
        }

        Log("taxi77 00AB: blocked-origin opcodeOff=0x%X no usable target coords", static_cast<uint32_t>(opcodeOffset));
        return 0;
    }

    script->currentIP = commandIp;
    return g_originalProcessCommands100To199(script, commandID);
}

void InstallTaxi77SetCarCoordinatesGuard()
{
#if defined(_M_IX86)
    constexpr size_t stolenBytes = 8;
    static const uint8_t expectedBytes[stolenBytes] = {
        0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, // mov eax, fs:[0]
        0x6A, 0xFF                          // push -1
    };

    const uintptr_t bridge = reinterpret_cast<uintptr_t>(Bridge_ProcessCommands100To199);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(kProcessCommands100To199);
    if (currentTarget == bridge) {
        Log("taxi77 00AB guard: already installed at 0x%08X", kProcessCommands100To199);
        return;
    }
    if (currentTarget) {
        Log("taxi77 00AB guard: skipped, ProcessCommands100To199 already hooked target=0x%08X", currentTarget);
        return;
    }

    uint8_t current[stolenBytes]{};
    if (!IsReadableCommitted(kProcessCommands100To199, sizeof(current))) {
        Log("taxi77 00AB guard: function unreadable at 0x%08X", kProcessCommands100To199);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kProcessCommands100To199), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("taxi77 00AB guard: read exception at 0x%08X", kProcessCommands100To199);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("taxi77 00AB guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X",
            kProcessCommands100To199,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7]);
        return;
    }

    g_processCommands100To199Trampoline = CreateRel32Trampoline(kProcessCommands100To199, stolenBytes);
    if (!g_processCommands100To199Trampoline) {
        return;
    }

    g_originalProcessCommands100To199 = reinterpret_cast<ProcessCommandsGroupFn>(g_processCommands100To199Trampoline);
    if (WriteRel32Jump(kProcessCommands100To199, bridge)) {
        Log("taxi77 00AB guard: installed at 0x%08X trampoline=0x%08X bridge=0x%08X",
            kProcessCommands100To199,
            g_processCommands100To199Trampoline,
            bridge);
    }
#else
    Log("taxi77 00AB guard: unsupported architecture");
#endif
}

DWORD ElapsedMs(uint32_t now, uint32_t then)
{
    return now - then;
}

template <typename T>
T ResolveProcByNameOrOrdinal(HMODULE module, const char* name, WORD ordinal)
{
    FARPROC proc = GetProcAddress(module, name);
    if (!proc) {
        proc = GetProcAddress(module, MAKEINTRESOURCEA(ordinal));
    }
    return reinterpret_cast<T>(proc);
}

bool ResolveCleoExports()
{
    HMODULE module = GetModuleHandleA("CLEO.asi");
    if (!module) {
        module = GetModuleHandleA("cleo.asi");
    }
    if (!module) {
        return false;
    }

    if (g_cleoExports.module == module &&
        g_cleoExports.getScriptByName &&
        g_cleoExports.isValidScriptPtr &&
        g_cleoExports.isScriptRunning &&
        g_cleoExports.getScriptBaseRelativeOffset &&
        g_cleoExports.threadJumpAtLabelPtr) {
        return true;
    }

    CleoExports exports{};
    exports.module = module;
    exports.getScriptByName = ResolveProcByNameOrOrdinal<CleoExports::GetScriptByNameFn>(
        module, "_CLEO_GetScriptByName@16", 47);
    exports.isValidScriptPtr = ResolveProcByNameOrOrdinal<CleoExports::IsValidScriptPtrFn>(
        module, "_CLEO_IsValidScriptPtr@4", 69);
    exports.isScriptRunning = ResolveProcByNameOrOrdinal<CleoExports::IsScriptRunningFn>(
        module, "_CLEO_IsScriptRunning@4", 53);
    exports.getScriptBaseRelativeOffset = ResolveProcByNameOrOrdinal<CleoExports::GetScriptBaseRelativeOffsetFn>(
        module, "_CLEO_GetScriptBaseRelativeOffset@8", 59);
    exports.threadJumpAtLabelPtr = ResolveProcByNameOrOrdinal<CleoExports::ThreadJumpAtLabelPtrFn>(
        module, "_CLEO_ThreadJumpAtLabelPtr@8", 14);
    exports.registerOpcode = ResolveProcByNameOrOrdinal<CleoExports::RegisterOpcodeFn>(
        module, "_CLEO_RegisterOpcode@8", 8);
    exports.callNativeOpcode = ResolveProcByNameOrOrdinal<CleoExports::CallNativeOpcodeFn>(
        module, "_CLEO_CallNativeOpcode@8", 61);
    exports.registerCallback = ResolveProcByNameOrOrdinal<CleoExports::RegisterCallbackFn>(
        module, "_CLEO_RegisterCallback@8", 27);
    exports.getOperandType = ResolveProcByNameOrOrdinal<CleoExports::GetOperandTypeFn>(
        module, "_CLEO_GetOperandType@4", 4);
    exports.peekPointerToScriptVariable = ResolveProcByNameOrOrdinal<CleoExports::PeekPointerToScriptVariableFn>(
        module, "_CLEO_PeekPointerToScriptVariable@4", 46);
    exports.skipOpcodeParams = ResolveProcByNameOrOrdinal<CleoExports::SkipOpcodeParamsFn>(
        module, "_CLEO_SkipOpcodeParams@8", 13);
    exports.setThreadCondResult = ResolveProcByNameOrOrdinal<CleoExports::SetThreadCondResultFn>(
        module, "_CLEO_SetThreadCondResult@8", 12);

    if (!exports.getScriptByName || !exports.isValidScriptPtr || !exports.isScriptRunning ||
        !exports.getScriptBaseRelativeOffset || !exports.threadJumpAtLabelPtr) {
        static LONG missingLogCount = 0;
        if (InterlockedIncrement(&missingLogCount) <= 8) {
            Log("CLEO exports incomplete module=0x%08X get=%p valid=%p running=%p offset=%p jump=%p",
                reinterpret_cast<uintptr_t>(module),
                exports.getScriptByName,
                exports.isValidScriptPtr,
                exports.isScriptRunning,
                exports.getScriptBaseRelativeOffset,
                exports.threadJumpAtLabelPtr);
        }
        return false;
    }

    g_cleoExports = exports;
    Log("CLEO exports resolved module=0x%08X registerOpcode=%p callNative=%p callback=%p operand=%p peekVar=%p skip=%p setCond=%p",
        reinterpret_cast<uintptr_t>(module),
        exports.registerOpcode,
        exports.callNativeOpcode,
        exports.registerCallback,
        exports.getOperandType,
        exports.peekPointerToScriptVariable,
        exports.skipOpcodeParams,
        exports.setThreadCondResult);
    return true;
}

bool IsCleoScriptUsable(RunningScriptLite* script)
{
    if (!script || !g_cleoExports.isValidScriptPtr || !g_cleoExports.isScriptRunning) {
        return false;
    }

    __try {
        return g_cleoExports.isValidScriptPtr(script) && g_cleoExports.isScriptRunning(script);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

RunningScriptLite* FindCleoScriptByName(const char* name)
{
    if (!g_cleoExports.getScriptByName) {
        return nullptr;
    }

    for (DWORD index = 0; index < 16; ++index) {
        RunningScriptLite* script = nullptr;
        __try {
            script = g_cleoExports.getScriptByName(name, FALSE, TRUE, index);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
        if (!script) {
            return nullptr;
        }
        if (IsCleoScriptUsable(script)) {
            return script;
        }
    }
    return nullptr;
}

RunningScriptLite* FindTaxi77MainScript()
{
    RunningScriptLite* script = FindCleoScriptByName("taxi77");
    if (script) {
        return script;
    }
    return FindCleoScriptByName("Taxi77");
}

RunningScriptLite* FindTaxi77MeterScript()
{
    RunningScriptLite* script = FindCleoScriptByName("taxi77m");
    if (script) {
        return script;
    }
    return FindCleoScriptByName("Taxi77M");
}

uint32_t GetCleoScriptOffset(RunningScriptLite* script)
{
    if (!script || !script->currentIP) {
        return 0xFFFFFFFF;
    }

    if (g_cleoExports.getScriptBaseRelativeOffset) {
        __try {
            return g_cleoExports.getScriptBaseRelativeOffset(script, script->currentIP);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    __try {
        if (script->baseIP && script->currentIP >= script->baseIP) {
            return static_cast<uint32_t>(script->currentIP - script->baseIP);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return 0xFFFFFFFF;
}

bool ReadTaxi77Local(RunningScriptLite* script, uint32_t index, ScriptParamLite* out)
{
    if (!script || !out || index >= 32) {
        return false;
    }

    auto locals = reinterpret_cast<ScriptParamLite*>(reinterpret_cast<uintptr_t>(script) + kRunningScriptLocalVarsOffset);
    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(&locals[index]), sizeof(ScriptParamLite))) {
        return false;
    }

    __try {
        *out = locals[index];
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WriteTaxi77LocalDword(RunningScriptLite* script, uint32_t index, uint32_t value)
{
    if (!script || index >= 32) {
        return false;
    }

    auto locals = reinterpret_cast<ScriptParamLite*>(reinterpret_cast<uintptr_t>(script) + kRunningScriptLocalVarsOffset);
    if (!IsWritableCommitted(reinterpret_cast<uintptr_t>(&locals[index]), sizeof(ScriptParamLite))) {
        return false;
    }

    __try {
        locals[index].uParam = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsTaxi77ScriptProcessing(RunningScriptLite* script)
{
    if (!script || !IsReadableCommitted(reinterpret_cast<uintptr_t>(script) + 0xDF, 1)) {
        return true;
    }

    __try {
        const uint8_t flags = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(script) + 0xDF);
        return (flags & 0x02) != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
}

bool ResetTaxi77MainScriptToStart(RunningScriptLite* script, uint32_t offset, uint32_t local0)
{
    if (!script || !g_cleoExports.threadJumpAtLabelPtr || g_config.taxi77StartLabelOffset <= 0) {
        return false;
    }

    constexpr uint32_t transientMask =
        (1u << 0) | // teleport flag
        (1u << 1) | // friends clear-to-go flag
        (1u << 4) |
        (1u << 7) | // set blip message flag
        (1u << 8) | // water message flag
        (1u << 9) | // cruising flag
        (1u << 10); // FPV flag

    const uint32_t cleanedLocal0 = local0 & ~transientMask;
    const bool localWritten = WriteTaxi77LocalDword(script, 0, cleanedLocal0);

    __try {
        auto timers = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(script) + 0xBC);
        if (IsWritableCommitted(reinterpret_cast<uintptr_t>(&timers[0]), sizeof(uint32_t) * 2)) {
            timers[0] = 0;
            timers[1] = 0;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    __try {
        g_cleoExports.threadJumpAtLabelPtr(script, -g_config.taxi77StartLabelOffset);
        Log("taxi77 watchdog: recovered main script ptr=0x%08X oldOffset=0x%X startOffset=0x%X local0=0x%08X->0x%08X localWritten=%d recoveryCount=%ld",
            reinterpret_cast<uintptr_t>(script),
            offset,
            g_config.taxi77StartLabelOffset,
            local0,
            cleanedLocal0,
            localWritten ? 1 : 0,
            InterlockedIncrement(&g_taxi77WatchdogRecoveries));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("taxi77 watchdog: recovery exception ptr=0x%08X oldOffset=0x%X startOffset=0x%X",
            reinterpret_cast<uintptr_t>(script),
            offset,
            g_config.taxi77StartLabelOffset);
        return false;
    }
}

DWORD WINAPI Taxi77StateWatchdogThread(void*)
{
    Sleep(3000);
    Log("taxi77 watchdog: thread started pollMs=%d stuckSeconds=%d recovery=%d",
        g_config.taxi77WatchdogPollMs,
        g_config.taxi77MainStuckSeconds,
        g_config.enableTaxi77StateWatchdogRecovery ? 1 : 0);

    for (;;) {
        Sleep(static_cast<DWORD>(g_config.taxi77WatchdogPollMs));

        if (!ResolveCleoExports()) {
            continue;
        }

        RunningScriptLite* mainScript = FindTaxi77MainScript();
        RunningScriptLite* meterScript = FindTaxi77MeterScript();
        const uint32_t now = GetTickCount();

        if (!mainScript) {
            if (g_taxi77WatchdogMainPtr != 0) {
                Log("taxi77 watchdog: main script disappeared oldPtr=0x%08X", g_taxi77WatchdogMainPtr);
            }
            g_taxi77WatchdogMainPtr = 0;
            g_taxi77WatchdogActiveSince = 0;
            g_taxi77WatchdogLastOffset = 0xFFFFFFFF;
            continue;
        }

        const uintptr_t mainPtr = reinterpret_cast<uintptr_t>(mainScript);
        if (g_taxi77WatchdogMainPtr != mainPtr) {
            Log("taxi77 watchdog: main script ptr=0x%08X", mainPtr);
            g_taxi77WatchdogMainPtr = mainPtr;
            g_taxi77WatchdogActiveSince = 0;
            g_taxi77WatchdogLastOffset = 0xFFFFFFFF;
        }

        const uint32_t offset = GetCleoScriptOffset(mainScript);
        ScriptParamLite local0{};
        ScriptParamLite car{};
        ScriptParamLite driver{};
        ReadTaxi77Local(mainScript, 0, &local0);
        ReadTaxi77Local(mainScript, 10, &car);
        ReadTaxi77Local(mainScript, 11, &driver);

        const bool hasMeter = meterScript != nullptr;
        const bool activeRange =
            offset != 0xFFFFFFFF &&
            offset >= static_cast<uint32_t>(g_config.taxi77ActiveMinOffset) &&
            offset < static_cast<uint32_t>(g_config.taxi77ActiveMaxOffset);
        const bool recoverableActive = activeRange && !hasMeter;

        if (offset != g_taxi77WatchdogLastOffset ||
            g_taxi77WatchdogLastLog == 0 ||
            ElapsedMs(now, g_taxi77WatchdogLastLog) >= 10000) {
            Log("taxi77 watchdog: state ptr=0x%08X offset=0x%X active=%d meter=%d local0=0x%08X car=%u driver=%u activeForMs=%u",
                mainPtr,
                offset,
                activeRange ? 1 : 0,
                hasMeter ? 1 : 0,
                local0.uParam,
                car.uParam,
                driver.uParam,
                g_taxi77WatchdogActiveSince ? ElapsedMs(now, g_taxi77WatchdogActiveSince) : 0);
            g_taxi77WatchdogLastLog = now;
            g_taxi77WatchdogLastOffset = offset;
        }

        if (!recoverableActive) {
            g_taxi77WatchdogActiveSince = 0;
            continue;
        }

        if (g_taxi77WatchdogActiveSince == 0) {
            g_taxi77WatchdogActiveSince = now;
            continue;
        }

        const DWORD activeMs = ElapsedMs(now, g_taxi77WatchdogActiveSince);
        if (activeMs < static_cast<DWORD>(g_config.taxi77MainStuckSeconds) * 1000) {
            continue;
        }

        if (!g_config.enableTaxi77StateWatchdogRecovery) {
            Log("taxi77 watchdog: would recover main script ptr=0x%08X offset=0x%X activeMs=%u local0=0x%08X car=%u driver=%u",
                mainPtr,
                offset,
                activeMs,
                local0.uParam,
                car.uParam,
                driver.uParam);
            g_taxi77WatchdogActiveSince = now;
            continue;
        }

        if (IsTaxi77ScriptProcessing(mainScript)) {
            Log("taxi77 watchdog: recovery delayed because script is processing ptr=0x%08X offset=0x%X activeMs=%u",
                mainPtr,
                offset,
                activeMs);
            continue;
        }

        if (ResetTaxi77MainScriptToStart(mainScript, offset, local0.uParam)) {
            g_taxi77WatchdogActiveSince = 0;
            g_taxi77WatchdogLastOffset = 0xFFFFFFFF;
            g_taxi77WatchdogLastLog = 0;
        } else {
            g_taxi77WatchdogActiveSince = now;
        }
    }
}

uint32_t ReadPatchedRadarTraceLimit()
{
    uint32_t limit = 0;
    constexpr uintptr_t patchedCountAddress = 0x0058384C + 2;
    if (IsReadableCommitted(patchedCountAddress, sizeof(limit))) {
        __try {
            std::memcpy(&limit, reinterpret_cast<const void*>(patchedCountAddress), sizeof(limit));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            limit = 0;
        }
    }

    if (limit >= 1 && limit <= 65535) {
        return limit;
    }
    return 0;
}

uintptr_t ReadPatchedRadarTraceBase()
{
    uintptr_t indexField = 0;
    constexpr uintptr_t patchedIndexPointer = 0x00582889 + 3;
    if (IsReadableCommitted(patchedIndexPointer, sizeof(indexField))) {
        __try {
            std::memcpy(&indexField, reinterpret_cast<const void*>(patchedIndexPointer), sizeof(indexField));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            indexField = 0;
        }
    }

    if (indexField >= kRadarTraceCounterOffset) {
        const uintptr_t base = indexField - kRadarTraceCounterOffset;
        if (IsReadableCommitted(base, kRadarTraceSize)) {
            return base;
        }
    }
    return kOriginalRadarTrace;
}

void RefreshRadarTraceRuntimeState()
{
    g_radarTraceBase = ReadPatchedRadarTraceBase();

    const uint32_t iniLimit = static_cast<uint32_t>(ReadFlaInt("Radar traces", 175, 1, 65535));
    const uint32_t patchedLimit = ReadPatchedRadarTraceLimit();
    g_radarTraceLimit = patchedLimit ? patchedLimit : iniLimit;
    if (iniLimit > g_radarTraceLimit) {
        g_radarTraceLimit = iniLimit;
    }
}

bool ReadU32(uintptr_t address, uint32_t* out)
{
    if (!out || !IsReadableCommitted(address, sizeof(*out))) {
        return false;
    }

    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), sizeof(*out));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsCleoGameEntitiesCaller(uintptr_t returnAddress, char* moduleName, size_t moduleNameSize)
{
    if (moduleName && moduleNameSize) {
        moduleName[0] = '\0';
    }

    char name[MAX_PATH]{};
    if (!ModuleBaseFromAddress(returnAddress, name, sizeof(name))) {
        if (moduleName && moduleNameSize) {
            strncpy_s(moduleName, moduleNameSize, name, _TRUNCATE);
        }
        return false;
    }

    if (moduleName && moduleNameSize) {
        strncpy_s(moduleName, moduleNameSize, name, _TRUNCATE);
    }

    return ContainsCaseInsensitive(name, "GameEntities") &&
        ContainsCaseInsensitive(name, ".cleo");
}

bool FindModuleRangeFromAddress(uintptr_t address, char* moduleName, size_t moduleNameSize, uintptr_t* moduleBase, uintptr_t* moduleEnd)
{
    if (moduleName && moduleNameSize) {
        moduleName[0] = '\0';
    }
    if (moduleBase) {
        *moduleBase = 0;
    }
    if (moduleEnd) {
        *moduleEnd = 0;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        if (moduleName && moduleNameSize) {
            strncpy_s(moduleName, moduleNameSize, "<snapshot-failed>", _TRUNCATE);
        }
        return false;
    }

    bool found = false;
    if (Module32First(snapshot, &me)) {
        do {
            const uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
            const uintptr_t end = base + me.modBaseSize;
            if (address >= base && address < end) {
                if (moduleName && moduleNameSize) {
                    strncpy_s(moduleName, moduleNameSize, me.szModule, _TRUNCATE);
                }
                if (moduleBase) {
                    *moduleBase = base;
                }
                if (moduleEnd) {
                    *moduleEnd = end;
                }
                found = true;
                break;
            }
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    if (!found && moduleName && moduleNameSize) {
        strncpy_s(moduleName, moduleNameSize, "<unknown-module>", _TRUNCATE);
    }
    return found;
}

bool ModuleMemoryContainsU32(uintptr_t moduleBase, uintptr_t moduleEnd, uint32_t needle)
{
    if (!needle || moduleEnd <= moduleBase) {
        return false;
    }

    uintptr_t cursor = moduleBase;
    while (cursor < moduleEnd) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<const void*>(cursor), &mbi, sizeof(mbi))) {
            cursor += 0x1000;
            continue;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t regionEnd = regionBase + mbi.RegionSize;
        cursor = regionEnd;

        if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
            continue;
        }

        const uintptr_t scanStart = regionBase < moduleBase ? moduleBase : regionBase;
        const uintptr_t scanEnd = regionEnd > moduleEnd ? moduleEnd : regionEnd;
        if (scanEnd <= scanStart || scanEnd - scanStart < sizeof(uint32_t)) {
            continue;
        }

        const size_t size = scanEnd - scanStart;
        uint8_t* buffer = new uint8_t[size];
        bool copied = false;
        __try {
            std::memcpy(buffer, reinterpret_cast<const void*>(scanStart), size);
            copied = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            copied = false;
        }

        if (!copied) {
            delete[] buffer;
            continue;
        }

        for (size_t i = 0; i + sizeof(uint32_t) <= size; ++i) {
            uint32_t value = 0;
            std::memcpy(&value, buffer + i, sizeof(value));
            if (value == needle) {
                delete[] buffer;
                return true;
            }
        }

        delete[] buffer;
    }

    return false;
}

bool CallerAlreadyUsesRelocatedRadarTrace(uintptr_t returnAddress, const char* moduleName)
{
    if (!g_radarTraceBase || g_radarTraceBase == kOriginalRadarTrace) {
        return false;
    }

    static uintptr_t cachedBase = 0;
    static uintptr_t cachedRadarBase = 0;
    static int cachedResult = -1;

    uintptr_t moduleBase = 0;
    uintptr_t moduleEnd = 0;
    char resolvedName[MAX_PATH]{};
    if (!FindModuleRangeFromAddress(returnAddress, resolvedName, sizeof(resolvedName), &moduleBase, &moduleEnd)) {
        return false;
    }

    if (cachedBase == moduleBase && cachedRadarBase == g_radarTraceBase && cachedResult != -1) {
        return cachedResult != 0;
    }

    const bool hasRelocated = ModuleMemoryContainsU32(moduleBase, moduleEnd, static_cast<uint32_t>(g_radarTraceBase));
    cachedBase = moduleBase;
    cachedRadarBase = g_radarTraceBase;
    cachedResult = hasRelocated ? 1 : 0;

    Log("target blip bridge: caller radar trace mode module=%s base=0x%08X relocatedTrace=0x%08X alreadyRelocated=%d",
        moduleName && *moduleName ? moduleName : resolvedName,
        moduleBase,
        g_radarTraceBase,
        hasRelocated ? 1 : 0);

    return hasRelocated;
}

bool ReadRadarTraceFields(uintptr_t trace, float* outX, float* outY, float* outZ, uint16_t* outCounter, uint8_t* outSprite, uint8_t* outFlags)
{
    if (!IsReadableCommitted(trace, kRadarTraceSize)) {
        return false;
    }

    __try {
        if (outX) {
            std::memcpy(outX, reinterpret_cast<const void*>(trace + kRadarTracePositionOffset + 0x00), sizeof(*outX));
        }
        if (outY) {
            std::memcpy(outY, reinterpret_cast<const void*>(trace + kRadarTracePositionOffset + 0x04), sizeof(*outY));
        }
        if (outZ) {
            std::memcpy(outZ, reinterpret_cast<const void*>(trace + kRadarTracePositionOffset + 0x08), sizeof(*outZ));
        }
        if (outCounter) {
            std::memcpy(outCounter, reinterpret_cast<const void*>(trace + kRadarTraceCounterOffset), sizeof(*outCounter));
        }
        if (outSprite) {
            std::memcpy(outSprite, reinterpret_cast<const void*>(trace + kRadarTraceSpriteOffset), sizeof(*outSprite));
        }
        if (outFlags) {
            std::memcpy(outFlags, reinterpret_cast<const void*>(trace + kRadarTraceFlagOffset), sizeof(*outFlags));
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void RememberTargetBlipCoords(float x, float y, float z)
{
    if (!IsSaneRadarCoord(x, y, z)) {
        return;
    }

    g_lastTargetBlipX = x;
    g_lastTargetBlipY = y;
    g_lastTargetBlipZ = z;
    InterlockedExchange(&g_hasLastTargetBlipCoords, 1);
}

bool IsSaneRadarCoord(float x, float y, float z)
{
    if (x != x || y != y || z != z) {
        return false;
    }
    if (x > -1.0f && x < 1.0f && y > -1.0f && y < 1.0f) {
        return false;
    }
    return x > -60000.0f && x < 60000.0f &&
        y > -60000.0f && y < 60000.0f &&
        z > -10000.0f && z < 10000.0f;
}

bool FindRelocatedWaypointTrace(uint16_t wantedCounter, uint32_t* outIndex, uintptr_t* outTrace)
{
    if (!outIndex || !outTrace || !g_radarTraceBase || !g_radarTraceLimit) {
        return false;
    }

    struct Candidate {
        uint32_t index = 0;
        uintptr_t trace = 0;
        uint16_t counter = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    Candidate fallback{};
    bool hasFallback = false;

    for (uint32_t i = 0; i < g_radarTraceLimit; ++i) {
        const uintptr_t trace = g_radarTraceBase + static_cast<uintptr_t>(i) * kRadarTraceSize;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        uint16_t actualCounter = 0;
        uint8_t sprite = 0;
        uint8_t flags = 0;
        if (!ReadRadarTraceFields(trace, &x, &y, &z, &actualCounter, &sprite, &flags)) {
            continue;
        }
        if (!(flags & kRadarTraceTrackingFlag) || sprite != kRadarSpriteWaypoint || !IsSaneRadarCoord(x, y, z)) {
            continue;
        }

        if (actualCounter == wantedCounter) {
            *outIndex = i;
            *outTrace = trace;
            static LONG exactLogs = 0;
            const LONG count = InterlockedIncrement(&exactLogs);
            if (count <= 32) {
                Log("target blip bridge: found waypoint by counter index=%u counter=%u coords=(%.2f, %.2f, %.2f)",
                    i, actualCounter, x, y, z);
            }
            return true;
        }

        if (!hasFallback || static_cast<uint16_t>(actualCounter - fallback.counter) < 0x8000u) {
            fallback.index = i;
            fallback.trace = trace;
            fallback.counter = actualCounter;
            fallback.x = x;
            fallback.y = y;
            fallback.z = z;
            hasFallback = true;
        }
    }

    if (!hasFallback) {
        for (uint32_t i = 0; i < g_radarTraceLimit; ++i) {
            const uintptr_t trace = g_radarTraceBase + static_cast<uintptr_t>(i) * kRadarTraceSize;
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            uint16_t actualCounter = 0;
            uint8_t sprite = 0;
            uint8_t flags = 0;
            uint32_t entityHandle = 0;
            if (!ReadRadarTraceFields(trace, &x, &y, &z, &actualCounter, &sprite, &flags) ||
                !ReadU32(trace + kRadarTraceEntityHandleOffset, &entityHandle)) {
                continue;
            }
            if (!(flags & kRadarTraceTrackingFlag) || entityHandle != 0 || !IsSaneRadarCoord(x, y, z)) {
                continue;
            }

            if (actualCounter == wantedCounter) {
                *outIndex = i;
                *outTrace = trace;
                static LONG relaxedExactLogs = 0;
                const LONG count = InterlockedIncrement(&relaxedExactLogs);
                if (count <= 32) {
                    Log("target blip bridge: found coord blip by counter index=%u counter=%u sprite=%u coords=(%.2f, %.2f, %.2f)",
                        i, actualCounter, sprite, x, y, z);
                }
                return true;
            }

            if (!hasFallback || static_cast<uint16_t>(actualCounter - fallback.counter) < 0x8000u) {
                fallback.index = i;
                fallback.trace = trace;
                fallback.counter = actualCounter;
                fallback.x = x;
                fallback.y = y;
                fallback.z = z;
                hasFallback = true;
            }
        }

        if (!hasFallback) {
            return false;
        }
    }

    *outIndex = fallback.index;
    *outTrace = fallback.trace;
    static LONG fallbackLogs = 0;
    const LONG count = InterlockedIncrement(&fallbackLogs);
    if (count <= 32) {
        Log("target blip bridge: found waypoint by scan index=%u counter=%u wantedCounter=%u coords=(%.2f, %.2f, %.2f)",
            fallback.index, fallback.counter, wantedCounter, fallback.x, fallback.y, fallback.z);
    }
    return true;
}

bool ShadowRadarTraceForLegacyCleo(uint32_t blip, uint32_t traceIndex, uintptr_t trace, uintptr_t returnAddress, int* outIndex)
{
    if (!g_config.enableCleoTargetBlipCoordsBridge || !outIndex) {
        return false;
    }

    uint32_t targetBlip = 0;
    if (!ReadU32(kFrontEndTargetBlipIndex, &targetBlip) || targetBlip != blip) {
        return false;
    }

    char moduleName[MAX_PATH]{};
    if (!IsCleoGameEntitiesCaller(returnAddress, moduleName, sizeof(moduleName))) {
        return false;
    }

    if (CallerAlreadyUsesRelocatedRadarTrace(returnAddress, moduleName)) {
        return false;
    }

    const uint32_t shadowIndex = traceIndex < kOriginalRadarTraceCount ?
        traceIndex : kLegacyCleoRadarTraceShadowSlot;
    const uintptr_t destination = kOriginalRadarTrace + shadowIndex * kRadarTraceSize;

    if (!CopyMemoryWithProtect(destination, trace, kRadarTraceSize)) {
        static LONG failedLogs = 0;
        const LONG count = InterlockedIncrement(&failedLogs);
        if (count <= 16) {
            Log("target blip bridge: failed shadow copy blip=0x%08X srcIndex=%u dstIndex=%u src=0x%08X dst=0x%08X caller=%s+0x%08X",
                blip, traceIndex, shadowIndex, trace, destination, moduleName, returnAddress);
        }
        return false;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    ReadRadarTraceFields(trace, &x, &y, &z, nullptr, nullptr, nullptr);

    static LONG shadowLogs = 0;
    const LONG count = InterlockedIncrement(&shadowLogs);
    if (count <= 64) {
        Log("target blip bridge: shadowed blip=0x%08X srcIndex=%u dstIndex=%u coords=(%.2f, %.2f, %.2f) caller=%s+0x%08X",
            blip, traceIndex, shadowIndex, x, y, z, moduleName, returnAddress);
    }

    *outIndex = static_cast<int>(shadowIndex);
    return true;
}

bool ResolveTargetBlipByWaypointScan(uint32_t blip, uint16_t counter, uintptr_t returnAddress, int* outIndex)
{
    if (!g_config.enableCleoTargetBlipCoordsBridge || !outIndex) {
        return false;
    }

    uint32_t targetBlip = 0;
    if (!ReadU32(kFrontEndTargetBlipIndex, &targetBlip) || targetBlip != blip) {
        return false;
    }

    uint32_t foundIndex = 0;
    uintptr_t foundTrace = 0;
    if (!FindRelocatedWaypointTrace(counter, &foundIndex, &foundTrace)) {
        static LONG missLogs = 0;
        const LONG count = InterlockedIncrement(&missLogs);
        if (count <= 32) {
            Log("target blip bridge: waypoint scan miss blip=0x%08X counter=%u base=0x%08X limit=%u",
                blip, counter, g_radarTraceBase, g_radarTraceLimit);
        }
        return false;
    }

    int shadowIndex = -1;
    if (ShadowRadarTraceForLegacyCleo(blip, foundIndex, foundTrace, returnAddress, &shadowIndex)) {
        *outIndex = shadowIndex;
        return true;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    uint16_t actualCounter = 0;
    uint8_t sprite = 0;
    uint8_t flags = 0;
    ReadRadarTraceFields(foundTrace, &x, &y, &z, &actualCounter, &sprite, &flags);

    char moduleName[MAX_PATH]{};
    ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));

    static LONG repairLogs = 0;
    const LONG count = InterlockedIncrement(&repairLogs);
    if (count <= 64) {
        Log("target blip bridge: repaired target blip=0x%08X badIndex=%u returnIndex=%u counter=%u actualCounter=%u sprite=%u flags=0x%02X coords=(%.2f, %.2f, %.2f) caller=%s+0x%08X",
            blip,
            blip & 0xFFFFu,
            foundIndex,
            counter,
            actualCounter,
            sprite,
            flags,
            x,
            y,
            z,
            moduleName,
            returnAddress);
    }

    *outIndex = static_cast<int>(foundIndex);
    return true;
}

extern "C" int __cdecl Bridge_CRadar_GetActualBlipArrayIndex(uint32_t blip)
{
    const uintptr_t returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());

    if (blip == 0xFFFFFFFFu) {
        return -1;
    }

    const uint32_t traceIndex = blip & 0xFFFFu;
    const uint16_t counter = static_cast<uint16_t>(blip >> 16);
    const uint32_t limit = g_radarTraceLimit ? g_radarTraceLimit : 175;

    if (traceIndex >= limit) {
        int repairedIndex = -1;
        if (ResolveTargetBlipByWaypointScan(blip, counter, returnAddress, &repairedIndex)) {
            return repairedIndex;
        }

        static LONG invalidHighHandleLogs = 0;
        const LONG count = InterlockedIncrement(&invalidHighHandleLogs);
        if (count <= 32) {
            Log("radar blip guard: rejected out-of-range blip=0x%08X index=%u counter=%u limit=%u base=0x%08X",
                blip, traceIndex, counter, limit, g_radarTraceBase);
        }
        return -1;
    }

    const uintptr_t trace = (g_radarTraceBase ? g_radarTraceBase : kOriginalRadarTrace) + traceIndex * kRadarTraceSize;
    if (!IsReadableCommitted(trace + kRadarTraceCounterOffset, sizeof(uint16_t)) ||
        !IsReadableCommitted(trace + kRadarTraceFlagOffset, sizeof(uint8_t))) {
        static LONG unreadableLogs = 0;
        const LONG count = InterlockedIncrement(&unreadableLogs);
        if (count <= 16) {
            Log("radar blip guard: trace unreadable blip=0x%08X index=%u trace=0x%08X limit=%u",
                blip, traceIndex, trace, limit);
        }
        return -1;
    }

    uint16_t actualCounter = 0;
    uint8_t flags = 0;
    __try {
        std::memcpy(&actualCounter, reinterpret_cast<const void*>(trace + kRadarTraceCounterOffset), sizeof(actualCounter));
        std::memcpy(&flags, reinterpret_cast<const void*>(trace + kRadarTraceFlagOffset), sizeof(flags));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }

    if (counter != actualCounter || !(flags & kRadarTraceTrackingFlag)) {
        int repairedIndex = -1;
        if (ResolveTargetBlipByWaypointScan(blip, counter, returnAddress, &repairedIndex)) {
            return repairedIndex;
        }
        return -1;
    }

    uint32_t targetBlip = 0;
    if (ReadU32(kFrontEndTargetBlipIndex, &targetBlip) && targetBlip == blip) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        uint8_t sprite = 0;
        uint8_t currentFlags = 0;
        if (ReadRadarTraceFields(trace, &x, &y, &z, nullptr, &sprite, &currentFlags)) {
            RememberTargetBlipCoords(x, y, z);
            static LONG targetTraceLogs = 0;
            const LONG count = InterlockedIncrement(&targetTraceLogs);
            if (count <= 64) {
                Log("target blip bridge: valid target trace blip=0x%08X index=%u counter=%u sprite=%u flags=0x%02X coords=(%.2f, %.2f, %.2f)",
                    blip, traceIndex, counter, sprite, currentFlags, x, y, z);
            }

            if (!IsSaneRadarCoord(x, y, z) || sprite != kRadarSpriteWaypoint) {
                int repairedIndex = -1;
                if (ResolveTargetBlipByWaypointScan(blip, counter, returnAddress, &repairedIndex)) {
                    return repairedIndex;
                }
            }
        }
    }

    int shadowIndex = -1;
    if (ShadowRadarTraceForLegacyCleo(blip, traceIndex, trace, returnAddress, &shadowIndex)) {
        return shadowIndex;
    }

    return static_cast<int>(traceIndex);
}

void InstallRadarBlipHandleGuard()
{
#if defined(_M_IX86)
    RefreshRadarTraceRuntimeState();

    const uintptr_t target = reinterpret_cast<uintptr_t>(Bridge_CRadar_GetActualBlipArrayIndex);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(kCRadarGetActualBlipArrayIndex);

    Log("radar blip guard: base=0x%08X limit=%u iniLimit=%u patchedLimit=%u currentTarget=0x%08X bridge=0x%08X",
        g_radarTraceBase,
        g_radarTraceLimit,
        static_cast<uint32_t>(ReadFlaInt("Radar traces", 175, 1, 65535)),
        ReadPatchedRadarTraceLimit(),
        currentTarget,
        target);

    if (currentTarget == target) {
        return;
    }

    if (WriteRel32Jump(kCRadarGetActualBlipArrayIndex, target)) {
        Log("radar blip guard: installed at 0x%08X", kCRadarGetActualBlipArrayIndex);
    }
#else
    Log("radar blip guard: unsupported architecture");
#endif
}

void InstallOneCObjectCreateBridgeStub(const char* label, uintptr_t patchAddress, void* bridgeTarget)
{
#if defined(_M_IX86)
    const uintptr_t target = reinterpret_cast<uintptr_t>(bridgeTarget);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);

    Log("bridge stub: %s patch=0x%08X currentTarget=0x%08X targetExecutable=%d bridgeTarget=0x%08X",
        label,
        patchAddress,
        currentTarget,
        currentTarget ? (IsExecutableCommitted(currentTarget) ? 1 : 0) : -1,
        target);

    if (currentTarget == target) {
        return;
    }

    if (!currentTarget || !IsExecutableCommitted(currentTarget)) {
        if (WriteRel32Jump(patchAddress, target)) {
            Log("bridge stub: redirected stale FLA %s hook to bridge target", label);
        }
    }
#else
    (void)label;
    (void)patchAddress;
    (void)bridgeTarget;
#endif
}

void InstallPickupModelLoadGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = kCPickupGiveUsAPickUpObject;
    static const uint8_t expectedBytes[] = {
        0x83, 0xEC, 0x0C,       // sub esp, 0Ch
        0x56,                   // push esi
        0x8B, 0x74, 0x24, 0x14  // mov esi, [esp+14h]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("pickup model guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("pickup model guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_CPickup_GiveUsAPickUpObject_Guard);
    if (currentTarget == guardTarget) {
        Log("pickup model guard: already installed at CPickup::GiveUsAPickUpObject");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("pickup model guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0],
            current[1],
            current[2],
            current[3],
            current[4],
            current[5],
            current[6],
            current[7],
            currentTarget);
        return;
    }

    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("pickup model guard: installed at CPickup::GiveUsAPickUpObject target=0x%08X allowlist='%s' flags=0x%X loadNow=%d",
            guardTarget,
            g_config.pickupModelLoadGuardAllowlist,
            g_config.pickupModelLoadGuardFlags,
            g_config.pickupModelLoadGuardLoadNow ? 1 : 0);
    }
#else
    Log("pickup model guard: unsupported architecture");
#endif
}

void InstallCObjectCreateBridgeStubs()
{
#if defined(_M_IX86)
    InstallOneCObjectCreateBridgeStub("CObject::Create 0x5A1FA1", kCObjectCreatePatch1, Bridge_CObject_Create_5A1FA1);
    InstallOneCObjectCreateBridgeStub("CObject::Create 0x5A2016", kCObjectCreatePatch2, Bridge_CObject_Create_5A2016);
#endif
}

void RestoreCleoObjectCreateInlinePatch()
{
    static bool loggedOriginal = false;
    static bool loggedPatched = false;
    static const uint8_t originalBytes[] = {
        0x89, 0x9E, 0x74, 0x01, 0x00, 0x00
    }; // mov [esi+174h], ebx

    uint8_t current[sizeof(originalBytes)]{};
    if (!IsReadableCommitted(kCleoObjectCreateInlinePatch, sizeof(current))) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("CLEO+ compat: object-create inline 0x59FB1E address unreadable");
        }
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kCleoObjectCreateInlinePatch), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("CLEO+ compat: object-create inline 0x59FB1E read exception");
        }
        return;
    }

    if (std::memcmp(current, originalBytes, sizeof(originalBytes)) == 0) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("CLEO+ compat: object-create inline 0x59FB1E already original");
        }
        return;
    }

    if (WriteBytesWithProtect(kCleoObjectCreateInlinePatch, originalBytes, sizeof(originalBytes))) {
        if (!loggedPatched) {
            loggedPatched = true;
            Log("CLEO+ compat: restored object-create inline hook at 0x%08X old=%02X %02X %02X %02X %02X %02X",
                kCleoObjectCreateInlinePatch,
                current[0], current[1], current[2], current[3], current[4], current[5]);
        }
    }
}

void InstallCleoDispatchNullGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ dispatch guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    const uintptr_t patchAddress = base + 0x18C62;
    g_cleoDispatchNullGuardReturn = base + 0x18C67;

    static const uint8_t expectedBytes[] = { 0x8B, 0x01, 0xFF, 0x50, 0x08 };
    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("CLEO+ dispatch guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("CLEO+ dispatch guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_Cleo_Dispatch_NullGuard);
    if (currentTarget == guardTarget) {
        Log("CLEO+ dispatch guard: already installed at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("CLEO+ dispatch guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X",
            patchAddress, current[0], current[1], current[2], current[3], current[4]);
        return;
    }

    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("CLEO+ dispatch guard: installed at CLEO+.cleo+0x18C62 return=0x%08X target=0x%08X",
            g_cleoDispatchNullGuardReturn, guardTarget);
    }
#endif
}

void InstallCleoThunk26720NullGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ thunk26720 guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    const uintptr_t patchAddress = base + 0x26720;

    static const uint8_t expectedBytes[] = {
        0x8B, 0x41, 0x04,       // mov eax, [ecx+4]
        0x8B, 0x40, 0x18,       // mov eax, [eax+18h]
        0xFF, 0xE0              // jmp eax
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("CLEO+ thunk26720 guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("CLEO+ thunk26720 guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_Cleo_Thunk26720_NullGuard);
    if (currentTarget == guardTarget) {
        Log("CLEO+ thunk26720 guard: already installed at CLEO+.cleo+0x26720");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("CLEO+ thunk26720 guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7]);
        return;
    }

    g_cleoThunk26720Original = patchAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("CLEO+ thunk26720 guard: installed at CLEO+.cleo+0x26720 target=0x%08X",
            guardTarget);
    }
#endif
}

bool InstallOneCleoPlusOpcodeFunctionGuard(
    const char* name,
    uintptr_t patchAddress,
    uintptr_t guardTarget,
    uintptr_t* trampolineOut)
{
#if defined(_M_IX86)
    if (!trampolineOut || !IsExecutableCommitted(patchAddress)) {
        return false;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    if (currentTarget == guardTarget) {
        Log("CLEO+ extended object var guard: %s already installed at 0x%08X",
            name ? name : "",
            patchAddress);
        return *trampolineOut != 0;
    }

    uint8_t current[5]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        return false;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (current[0] == 0xE9) {
        Log("CLEO+ extended object var guard: %s already hooked by another module at 0x%08X target=0x%08X",
            name ? name : "",
            patchAddress,
            currentTarget);
        return false;
    }

    uintptr_t trampoline = CreateRel32Trampoline(patchAddress, sizeof(current));
    if (!trampoline) {
        Log("CLEO+ extended object var guard: %s trampoline failed at 0x%08X bytes=%02X %02X %02X %02X %02X",
            name ? name : "",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4]);
        return false;
    }

    if (!WriteRel32Jump(patchAddress, guardTarget)) {
        VirtualFree(reinterpret_cast<void*>(trampoline), 0, MEM_RELEASE);
        return false;
    }

    *trampolineOut = trampoline;
    Log("CLEO+ extended object var guard: installed %s at 0x%08X trampoline=0x%08X target=0x%08X old=%02X %02X %02X %02X %02X",
        name ? name : "",
        patchAddress,
        trampoline,
        guardTarget,
        current[0], current[1], current[2], current[3], current[4]);
    return true;
#else
    (void)name;
    (void)patchAddress;
    (void)guardTarget;
    (void)trampolineOut;
    return false;
#endif
}

void InstallCleoPlusExtendedObjectVarGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ extended object var guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    struct OffsetSet {
        uintptr_t init;
        uintptr_t set;
        uintptr_t get;
        const char* label;
    };
    constexpr OffsetSet kOffsetSets[] = {
        { 0x306C0, 0x30800, 0x308E0, "restored-current" },
        { 0x177F0, 0x17940, 0x17A30, "local-rebuilt" },
    };

    for (const OffsetSet& offsets : kOffsetSets) {
        if (!InstallOneCleoPlusOpcodeFunctionGuard(
                "INIT_EXTENDED_OBJECT_VARS",
                base + offsets.init,
                reinterpret_cast<uintptr_t>(Bridge_CleoPlus_InitExtendedObjectVars_Guard),
                &g_cleoPlusInitExtendedObjectVarsTrampoline)) {
            continue;
        }
        if (!InstallOneCleoPlusOpcodeFunctionGuard(
                "SET_EXTENDED_OBJECT_VAR",
                base + offsets.set,
                reinterpret_cast<uintptr_t>(Bridge_CleoPlus_SetExtendedObjectVar_Guard),
                &g_cleoPlusSetExtendedObjectVarTrampoline)) {
            Log("CLEO+ extended object var guard: incomplete offset set %s after INIT; not trying other sets",
                offsets.label);
            return;
        }
        if (!InstallOneCleoPlusOpcodeFunctionGuard(
                "GET_EXTENDED_OBJECT_VAR",
                base + offsets.get,
                reinterpret_cast<uintptr_t>(Bridge_CleoPlus_GetExtendedObjectVar_Guard),
                &g_cleoPlusGetExtendedObjectVarTrampoline)) {
            Log("CLEO+ extended object var guard: incomplete offset set %s after INIT/SET; not trying other sets",
                offsets.label);
            return;
        }

        Log("CLEO+ extended object var guard: offset set selected %s base=0x%08X",
            offsets.label,
            base);
        return;
    }

    Log("CLEO+ extended object var guard: no supported CLEO+ offset set matched base=0x%08X", base);
#endif
}

extern "C" void __stdcall Bridge_DeferPoolAllocate(uintptr_t poolPtrAddress, uintptr_t continueAddress, uintptr_t thisPtr)
{
    if (!g_config.enableDeferredPoolAllocateReplay || !poolPtrAddress || !continueAddress || !thisPtr) {
        return;
    }

    char moduleName[MAX_PATH]{};
    ModuleBaseFromAddress(continueAddress, moduleName, sizeof(moduleName));

    EnterCriticalSection(&g_deferredPoolAllocateLock);
    for (uint32_t i = 0; i < g_deferredPoolAllocateCount; ++i) {
        const DeferredPoolAllocate& item = g_deferredPoolAllocates[i];
        if (!item.completed &&
            item.poolPtrAddress == poolPtrAddress &&
            item.continueAddress == continueAddress &&
            item.thisPtr == thisPtr) {
            LeaveCriticalSection(&g_deferredPoolAllocateLock);
            return;
        }
    }

    if (g_deferredPoolAllocateCount < kMaxDeferredPoolAllocates) {
        DeferredPoolAllocate& item = g_deferredPoolAllocates[g_deferredPoolAllocateCount++];
        item.poolPtrAddress = poolPtrAddress;
        item.continueAddress = continueAddress;
        item.thisPtr = thisPtr;
        item.completed = 0;
        strncpy_s(item.moduleName, moduleName[0] ? moduleName : "<unknown>", _TRUNCATE);

        const LONG logCount = InterlockedIncrement(&g_deferredPoolAllocateLogs);
        if (logCount <= 64) {
            Log("auto pool guard: deferred allocate module=%s poolPtr=0x%08X continue=0x%08X this=0x%08X queued=%u",
                item.moduleName,
                item.poolPtrAddress,
                item.continueAddress,
                item.thisPtr,
                g_deferredPoolAllocateCount);
        }
    } else {
        static LONG fullLogs = 0;
        if (InterlockedIncrement(&fullLogs) <= 8) {
            Log("auto pool guard: deferred queue full poolPtr=0x%08X continue=0x%08X this=0x%08X",
                poolPtrAddress,
                continueAddress,
                thisPtr);
        }
    }
    LeaveCriticalSection(&g_deferredPoolAllocateLock);
}

DWORD WINAPI DeferredPoolAllocateReplayThread(void*)
{
    for (int attempt = 0; attempt < g_config.deferredPoolAllocateReplayIterations; ++attempt) {
        Sleep(static_cast<DWORD>(g_config.deferredPoolAllocateReplayIntervalMs));

        uint32_t pedPool = 0;
        uint32_t vehiclePool = 0;
        uint32_t objectPool = 0;
        uint32_t colModelPool = 0;
        if (!AreCorePoolsReadyForDeferredReplay(&pedPool, &vehiclePool, &objectPool, &colModelPool)) {
            static LONG waitLogs = 0;
            const LONG logCount = InterlockedIncrement(&waitLogs);
            if (logCount <= 12 || (attempt % 50) == 0) {
                uint32_t colSize = 0;
                uint32_t colFirstFree = 0xFFFFFFFF;
                ReadCPoolHeader(colModelPool, &colSize, &colFirstFree);
                Log("auto pool guard: replay waiting for complete core pools attempt=%d ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X colSize=%u colFirstFree=%u",
                    attempt,
                    pedPool,
                    vehiclePool,
                    objectPool,
                    colModelPool,
                    colSize,
                    colFirstFree);
            }
            continue;
        }

        DeferredPoolAllocate toRun[kMaxDeferredPoolAllocates]{};
        uint32_t runCount = 0;

        EnterCriticalSection(&g_deferredPoolAllocateLock);
        for (uint32_t i = 0; i < g_deferredPoolAllocateCount && runCount < kMaxDeferredPoolAllocates; ++i) {
            DeferredPoolAllocate& item = g_deferredPoolAllocates[i];
            if (item.completed) {
                continue;
            }

            uintptr_t pool = 0;
            if (!SafeReadU32(item.poolPtrAddress, reinterpret_cast<uint32_t*>(&pool)) || !pool) {
                continue;
            }

            item.completed = 1;
            toRun[runCount++] = item;
        }
        LeaveCriticalSection(&g_deferredPoolAllocateLock);

        for (uint32_t i = 0; i < runCount; ++i) {
            uintptr_t pool = 0;
            if (!SafeReadU32(toRun[i].poolPtrAddress, reinterpret_cast<uint32_t*>(&pool)) || !pool) {
                continue;
            }

            __try {
                Bridge_InvokePoolAllocateContinue(toRun[i].continueAddress, toRun[i].thisPtr, pool);
                Log("auto pool guard: replayed deferred allocate module=%s pool=0x%08X continue=0x%08X this=0x%08X",
                    toRun[i].moduleName,
                    pool,
                    toRun[i].continueAddress,
                    toRun[i].thisPtr);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                Log("auto pool guard: replay exception module=%s pool=0x%08X continue=0x%08X this=0x%08X",
                    toRun[i].moduleName,
                    pool,
                    toRun[i].continueAddress,
                    toRun[i].thisPtr);
            }
        }
    }

    return 0;
}

uintptr_t CreatePoolAllocateGuardStub(uintptr_t poolPtrAddress, uintptr_t continueAddress)
{
#if defined(_M_IX86)
    uint8_t* stub = reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!stub) {
        return 0;
    }

    size_t o = 0;
    stub[o++] = 0xA1; // mov eax, [poolPtrAddress]
    std::memcpy(stub + o, &poolPtrAddress, sizeof(poolPtrAddress));
    o += sizeof(poolPtrAddress);
    stub[o++] = 0x85; stub[o++] = 0xC0; // test eax, eax
    stub[o++] = 0x74; stub[o++] = 0x06; // jz deferred
    stub[o++] = 0x68; // push continueAddress
    std::memcpy(stub + o, &continueAddress, sizeof(continueAddress));
    o += sizeof(continueAddress);
    stub[o++] = 0xC3; // ret

    stub[o++] = 0x51; // push ecx
    stub[o++] = 0x68; // push continueAddress
    std::memcpy(stub + o, &continueAddress, sizeof(continueAddress));
    o += sizeof(continueAddress);
    stub[o++] = 0x68; // push poolPtrAddress
    std::memcpy(stub + o, &poolPtrAddress, sizeof(poolPtrAddress));
    o += sizeof(poolPtrAddress);
    stub[o++] = 0xE8; // call Bridge_DeferPoolAllocate
    const uintptr_t callSite = reinterpret_cast<uintptr_t>(stub + o - 1);
    const uintptr_t callNext = callSite + 5;
    const intptr_t rel = reinterpret_cast<uintptr_t>(Bridge_DeferPoolAllocate) - callNext;
    if (rel < INT32_MIN || rel > INT32_MAX) {
        VirtualFree(stub, 0, MEM_RELEASE);
        return 0;
    }
    const int32_t rel32 = static_cast<int32_t>(rel);
    std::memcpy(stub + o, &rel32, sizeof(rel32));
    o += sizeof(rel32);
    stub[o++] = 0xC3; // ret

    return reinterpret_cast<uintptr_t>(stub);
#else
    (void)poolPtrAddress;
    (void)continueAddress;
    return 0;
#endif
}

bool InstallOneCleoPlusPoolAllocateGuard(
    const char* name,
    uintptr_t patchAddress,
    uintptr_t expectedPoolPtr,
    uintptr_t guardTarget,
    uintptr_t* continueOut)
{
#if defined(_M_IX86)
    if (!continueOut || !IsReadableCommitted(patchAddress, 5)) {
        return false;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    if (guardTarget && currentTarget == guardTarget) {
        Log("CLEO+ pool allocate guard: %s already installed at 0x%08X",
            name ? name : "",
            patchAddress);
        return true;
    }

    uint8_t current[5]{};
    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    uintptr_t poolPtr = 0;
    if (current[0] == 0xA1) {
        std::memcpy(&poolPtr, current + 1, sizeof(poolPtr));
    }
    if (current[0] != 0xA1 || poolPtr != expectedPoolPtr) {
        Log("CLEO+ pool allocate guard: %s unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X expectedPool=0x%08X decodedPool=0x%08X",
            name ? name : "",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4],
            expectedPoolPtr,
            poolPtr);
        return false;
    }

    const uintptr_t continueAddress = patchAddress + 5;
    const uintptr_t stub = CreatePoolAllocateGuardStub(expectedPoolPtr, continueAddress);
    if (!stub) {
        Log("CLEO+ pool allocate guard: %s stub allocation failed at 0x%08X poolPtr=0x%08X",
            name ? name : "",
            patchAddress,
            expectedPoolPtr);
        return false;
    }

    *continueOut = continueAddress;
    if (WriteRel32Jump(patchAddress, stub)) {
        Log("CLEO+ pool allocate guard: installed %s at 0x%08X continue=0x%08X stub=0x%08X poolPtr=0x%08X",
            name ? name : "",
            patchAddress,
            *continueOut,
            stub,
            expectedPoolPtr);
        return true;
    }

    VirtualFree(reinterpret_cast<void*>(stub), 0, MEM_RELEASE);
    *continueOut = 0;
    return false;
#else
    (void)name;
    (void)patchAddress;
    (void)expectedPoolPtr;
    (void)guardTarget;
    (void)continueOut;
    return false;
#endif
}

void InstallCleoPlusPoolAllocateGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ pool allocate guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    InstallOneCleoPlusPoolAllocateGuard(
        "ObjectExtendedData::AllocateBlocks",
        base + 0x30590,
        kOriginalObjectPoolPtr,
        reinterpret_cast<uintptr_t>(Bridge_CleoPlus_ObjectAllocateBlocks_PoolGuard),
        &g_cleoPlusObjectAllocateBlocksContinue);
    InstallOneCleoPlusPoolAllocateGuard(
        "VehicleExtendedData::AllocateBlocks",
        base + 0x30700,
        kOriginalVehiclePoolPtr,
        reinterpret_cast<uintptr_t>(Bridge_CleoPlus_VehicleAllocateBlocks_PoolGuard),
        &g_cleoPlusVehicleAllocateBlocksContinue);
    InstallOneCleoPlusPoolAllocateGuard(
        "PedExtendedData::AllocateBlocks",
        base + 0x30870,
        kOriginalPedPoolPtr,
        reinterpret_cast<uintptr_t>(Bridge_CleoPlus_PedAllocateBlocks_PoolGuard),
        &g_cleoPlusPedAllocateBlocksContinue);
#endif
}

void InstallMixSetsPoolAllocateGuard()
{
#if defined(_M_IX86)
    HMODULE mixSets = GetModuleHandleA("mixsets.asi");
    if (!mixSets) {
        mixSets = GetModuleHandleA("MixSets.asi");
    }
    if (!mixSets) {
        Log("MixSets pool allocate guard: MixSets.asi not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(mixSets);
    const uintptr_t patchAddress = base + 0x00CCB0;
    InstallOneCleoPlusPoolAllocateGuard(
        "MixSets PedExtendedData::AllocateBlocks",
        patchAddress,
        kOriginalPedPoolPtr,
        reinterpret_cast<uintptr_t>(Bridge_MixSets_PedAllocateBlocks_PoolGuard),
        &g_mixSetsPedAllocateBlocksContinue);
#endif
}

HMODULE FindLoadedModuleBySubstring(const char* needle)
{
    if (!needle || !*needle) {
        return nullptr;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    HMODULE result = nullptr;
    if (Module32First(snapshot, &me)) {
        do {
            if (ContainsCaseInsensitive(me.szModule, needle) || ContainsCaseInsensitive(me.szExePath, needle)) {
                result = me.hModule;
                break;
            }
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    return result;
}

void InstallUrbanizePoolAllocateGuard()
{
#if defined(_M_IX86)
    HMODULE urbanize = GetModuleHandleA("urbanize (junior_djjr).asi");
    if (!urbanize) {
        urbanize = FindLoadedModuleBySubstring("urbanize");
    }
    if (!urbanize) {
        Log("Urbanize pool allocate guard: urbanize module not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(urbanize);
    const uintptr_t patchAddress = base + 0x018750;
    InstallOneCleoPlusPoolAllocateGuard(
        "Urbanize PedExtendedData::AllocateBlocks",
        patchAddress,
        kOriginalPedPoolPtr,
        0,
        &g_urbanizePedAllocateBlocksContinue);
#endif
}

void InstallAnimUncompressNullGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D41C0;
    constexpr uintptr_t continueAddress = 0x004D41C5;
    static const uint8_t expectedBytes[] = {
        0x57,                   // push edi
        0x8B, 0x7C, 0x24, 0x08  // mov edi, [esp+8]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimUncompress_NullGuard);
    if (currentTarget == guardTarget) {
        Log("anim guard: already installed at CAnimManager::UncompressAnimation");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress, current[0], current[1], current[2], current[3], current[4], currentTarget);
        return;
    }

    g_animUncompressContinue = continueAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("anim guard: installed at CAnimManager::UncompressAnimation target=0x%08X continue=0x%08X",
            guardTarget, g_animUncompressContinue);
    }
#else
    Log("anim guard: unsupported architecture");
#endif
}

void InstallAnimStaticAssocInitGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004CEEC0;
    constexpr uintptr_t continueAddress = 0x004CEEC8;
    static const uint8_t expectedBytes[] = {
        0x56,                   // push esi
        0x8B, 0xF1,             // mov esi, ecx
        0x57,                   // push edi
        0x8B, 0x7C, 0x24, 0x0C  // mov edi, [esp+0Ch]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("static assoc guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("static assoc guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimStaticAssocInit_Guard);
    if (currentTarget == guardTarget) {
        Log("static assoc guard: already installed at CAnimBlendAssociation::Init(static)");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("static assoc guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7],
            currentTarget);
        return;
    }

    g_animStaticAssocInitContinue = continueAddress;
    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("static assoc guard: installed at CAnimBlendAssociation::Init(static) target=0x%08X continue=0x%08X",
            guardTarget, g_animStaticAssocInitContinue);
    }
#else
    Log("static assoc guard: unsupported architecture");
#endif
}

void InstallAnimFrameUpdateSkinnedVelocityGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D1680;
    constexpr uintptr_t continueAddress = 0x004D1685;
    static const uint8_t expectedBytes[] = {
        0x83, 0xEC, 0x68, // sub esp, 68h
        0x55,             // push ebp
        0x56              // push esi
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim frame guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim frame guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimFrameUpdateSkinnedVelocity_Guard);
    if (currentTarget == guardTarget) {
        Log("anim frame guard: already installed at FrameUpdateCallBackSkinnedWithVelocityExtraction");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim frame guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4],
            currentTarget);
        return;
    }

    g_animFrameUpdateSkinnedVelocityContinue = continueAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("anim frame guard: installed at FrameUpdateCallBackSkinnedWithVelocityExtraction target=0x%08X continue=0x%08X",
            guardTarget,
            g_animFrameUpdateSkinnedVelocityContinue);
    }
#else
    Log("anim frame guard: unsupported architecture");
#endif
}

void InstallGetBoundCentreNullGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x00533650;
    constexpr uintptr_t continueAddress = 0x0053365B;
    static const uint8_t expectedBytes[] = {
        0x0F, 0xBF, 0x41, 0x22,             // movsx eax, word ptr [ecx+22h]
        0x8B, 0x14, 0x85, 0xC8, 0xB0, 0xA9, 0x00 // mov edx, [eax*4+00A9B0C8h]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("bound centre guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bound centre guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_GetBoundCentre_NullGuard);
    if (currentTarget == guardTarget) {
        Log("bound centre guard: already installed at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("bound centre guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4], current[5],
            current[6], current[7], current[8], current[9], current[10],
            currentTarget);
        return;
    }

    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("bound centre guard: installed at 0x%08X target=0x%08X continue=0x%08X",
            patchAddress, guardTarget, continueAddress);
    }
#else
    Log("bound centre guard: unsupported architecture");
#endif
}

void InstallGetBoundRectColModelGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x00534120;
    constexpr uintptr_t continueAddress = 0x00534126;
    static const uint8_t expectedBytes[] = {
        0x83, 0xEC, 0x34, // sub esp, 34h
        0x56,             // push esi
        0x8B, 0xF1        // mov esi, ecx
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("bound rect guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bound rect guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_GetBoundRect_ColModelGuard);
    if (currentTarget == guardTarget) {
        Log("bound rect guard: already installed at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("bound rect guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4], current[5],
            currentTarget);
        return;
    }

    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("bound rect guard: installed at 0x%08X target=0x%08X continue=0x%08X",
            patchAddress, guardTarget, continueAddress);
    }
#else
    Log("bound rect guard: unsupported architecture");
#endif
}

void RestoreFlaNoCollisionErrorPatch()
{
    static bool loggedOriginal = false;
    static bool loggedPatched = false;
    static const uint8_t originalBytes[] = {
        0x8B, 0x10,                   // mov edx, [eax]
        0x89, 0x54, 0x24, 0x10        // mov [esp+10h], edx
    };

    uint8_t current[sizeof(originalBytes)]{};
    if (!IsReadableCommitted(kFlaNoCollisionErrorPatch, sizeof(current))) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: no-collision error 0x534134 address unreadable");
        }
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kFlaNoCollisionErrorPatch), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: no-collision error 0x534134 read exception");
        }
        return;
    }

    if (std::memcmp(current, originalBytes, sizeof(originalBytes)) == 0) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: no-collision error 0x534134 already original");
        }
        return;
    }

    if (current[0] != 0xE9) {
        if (!loggedPatched) {
            loggedPatched = true;
            Log("compat patch: no-collision error 0x534134 unexpected bytes old=%02X %02X %02X %02X %02X %02X",
                current[0], current[1], current[2], current[3], current[4], current[5]);
        }
        return;
    }

    if (WriteBytesWithProtect(kFlaNoCollisionErrorPatch, originalBytes, sizeof(originalBytes))) {
        Log("compat patch: restored FLA no-collision fatal hook at 0x%08X old=%02X %02X %02X %02X %02X %02X",
            kFlaNoCollisionErrorPatch,
            current[0], current[1], current[2], current[3], current[4], current[5]);
    }
}

void RestoreFlaObjectInitCollisionPatch()
{
    static bool loggedOriginal = false;
    static bool loggedPatched = false;
    static const uint8_t originalBytes[] = {
        0x0F, 0xB6, 0x47, 0x28, 0x50
    }; // movzx eax, byte ptr [edi+28h]; push eax

    uint8_t current[sizeof(originalBytes)]{};
    if (!IsReadableCommitted(kFlaObjectInitCollisionPatch, sizeof(current))) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: CObject::Init 0x59F8BE address unreadable");
        }
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kFlaObjectInitCollisionPatch), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: CObject::Init 0x59F8BE read exception");
        }
        return;
    }

    if (std::memcmp(current, originalBytes, sizeof(originalBytes)) == 0) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: CObject::Init 0x59F8BE already original");
        }
        return;
    }

    if (WriteBytesWithProtect(kFlaObjectInitCollisionPatch, originalBytes, sizeof(originalBytes))) {
        if (!loggedPatched) {
            loggedPatched = true;
            Log("compat patch: restored FLA CObject::Init collision hook at 0x%08X old=%02X %02X %02X %02X %02X",
                kFlaObjectInitCollisionPatch,
                current[0], current[1], current[2], current[3], current[4]);
        }
    }
}

void ScanRiskyAddressConstants(const char* modulePath)
{
    struct Pattern {
        const char* name;
        uint8_t bytes[4];
    };

    static const Pattern patterns[] = {
        { "CModelInfo::ms_modelInfoPtrs 0xA9B0C8", { 0xC8, 0xB0, 0xA9, 0x00 } },
        { "CStreaming::ms_aInfoForModel 0x8E4CC0", { 0xC0, 0x4C, 0x8E, 0x00 } },
        { "CAnimManager::ms_aAnimBlocks 0xB5D4A0", { 0xA0, 0xD4, 0xB5, 0x00 } },
        { "CRadar::ms_RadarTrace 0xBA86F0", { 0xF0, 0x86, 0xBA, 0x00 } },
        { "CDarkel::RegisteredKills 0x969A50", { 0x50, 0x9A, 0x96, 0x00 } },
        { "CPools::ms_pPedPool 0xB74490", { 0x90, 0x44, 0xB7, 0x00 } },
        { "CPools::ms_pVehiclePool 0xB74494", { 0x94, 0x44, 0xB7, 0x00 } },
        { "CPools::ms_pObjectPool 0xB7449C", { 0x9C, 0x44, 0xB7, 0x00 } },
    };

    bool any = false;
    for (const auto& pattern : patterns) {
        if (FileContainsPattern(modulePath, pattern.bytes, sizeof(pattern.bytes))) {
            Log("risk constant hit: %s -> %s", BaseName(modulePath), pattern.name);
            any = true;
        }
    }

    if (!any) {
        const char* base = BaseName(modulePath);
        if (strstr(base, ".asi") || strstr(base, ".cleo")) {
            Log("risk constant clean: %s", base);
        }
    }
}

bool ContainsCaseInsensitive(const char* haystack, const char* needle)
{
    if (!haystack || !needle || !*needle) {
        return false;
    }

    const size_t needleLen = std::strlen(needle);
    for (const char* p = haystack; *p; ++p) {
        if (_strnicmp(p, needle, needleLen) == 0) {
            return true;
        }
    }
    return false;
}

bool ModuleMatchesRewriteList(const char* moduleName, const char* modulePath, const char* rawList)
{
    if (!rawList || !rawList[0]) {
        return false;
    }

    char list[512]{};
    strncpy_s(list, rawList, _TRUNCATE);

    char* context = nullptr;
    for (char* token = strtok_s(list, ";,", &context); token; token = strtok_s(nullptr, ";,", &context)) {
        while (*token == ' ' || *token == '\t') {
            ++token;
        }
        char* end = token + std::strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';
        if (!*token) {
            continue;
        }
        if (ContainsCaseInsensitive(moduleName, token) || ContainsCaseInsensitive(modulePath, token)) {
            return true;
        }
    }

    return false;
}

bool ModuleDeniedByCompatPolicy(const char* moduleName, const char* modulePath)
{
    if (!g_config.enableModulePolicy) {
        return false;
    }
    return ModuleMatchesRewriteList(moduleName, modulePath, g_config.modernModuleDenylist);
}

bool ModuleAllowedByLegacyPolicy(const char* moduleName, const char* modulePath)
{
    if (!g_config.enableModulePolicy) {
        return true;
    }
    if (ModuleDeniedByCompatPolicy(moduleName, modulePath)) {
        return false;
    }
    if (!g_config.legacyModuleAllowlist[0]) {
        return true;
    }
    return ModuleMatchesRewriteList(moduleName, modulePath, g_config.legacyModuleAllowlist);
}

bool ModuleAllowedForAutoPoolGuard(const char* moduleName, const char* modulePath)
{
    if (!moduleName || !modulePath) {
        return false;
    }
    if (ModuleDeniedByCompatPolicy(moduleName, modulePath) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.forceNoAutoPoolGuard) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.autoPoolAllocateGuardDenylist)) {
        return false;
    }
    if (!ModuleAllowedByLegacyPolicy(moduleName, modulePath)) {
        return false;
    }
    if (!g_config.autoPoolAllocateGuardAllowlist[0]) {
        return false;
    }
    return ModuleMatchesRewriteList(moduleName, modulePath, g_config.autoPoolAllocateGuardAllowlist);
}

bool MemoryMatches(uintptr_t address, const uint8_t* bytes, size_t size)
{
    if (!bytes || !IsReadableCommitted(address, size)) {
        return false;
    }
    __try {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes, size) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsExecutableRegion(const MEMORY_BASIC_INFORMATION& mbi)
{
    if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return false;
    }

    const DWORD protect = mbi.Protect & 0xFF;
    return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ ||
        protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

bool BytesMatchInKnownReadableRegion(uintptr_t address, const uint8_t* bytes, size_t size)
{
    if (!bytes || !address || !size) {
        return false;
    }

    __try {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes, size) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool HasBytesInRange(uintptr_t address, size_t rangeSize, const uint8_t* bytes, size_t bytesSize)
{
    if (!bytes || !bytesSize || rangeSize < bytesSize) {
        return false;
    }

    for (size_t offset = 0; offset + bytesSize <= rangeSize; ++offset) {
        if (BytesMatchInKnownReadableRegion(address + offset, bytes, bytesSize)) {
            return true;
        }
    }
    return false;
}

bool LooksLikePluginSdkPoolAllocateBlocks(uintptr_t address, uintptr_t expectedPoolPtr)
{
    uint8_t first[5]{};
    __try {
        std::memcpy(first, reinterpret_cast<const void*>(address), sizeof(first));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    uintptr_t decodedPoolPtr = 0;
    std::memcpy(&decodedPoolPtr, first + 1, sizeof(decodedPoolPtr));
    if (first[0] != 0xA1 || decodedPoolPtr != expectedPoolPtr) {
        return false;
    }

    static const uint8_t thisSetup[] = { 0x56, 0x8B, 0xF1 };       // push esi; mov esi, ecx
    static const uint8_t readPoolCount[] = { 0x8B, 0x40, 0x08 };   // mov eax, [eax+8]
    static const uint8_t writeCount[] = { 0x89, 0x46, 0x08 };      // mov [esi+8], eax
    static const uint8_t writeArray[] = { 0x89, 0x46, 0x04 };      // mov [esi+4], eax

    return HasBytesInRange(address + 5, 20, thisSetup, sizeof(thisSetup)) &&
        HasBytesInRange(address + 5, 28, readPoolCount, sizeof(readPoolCount)) &&
        HasBytesInRange(address + 5, 48, writeCount, sizeof(writeCount)) &&
        HasBytesInRange(address + 5, 96, writeArray, sizeof(writeArray));
}

uint32_t ScanOneModuleForPoolAllocateGuards(const MODULEENTRY32& me, uint32_t remainingPatchBudget)
{
    if (!remainingPatchBudget || !ModuleAllowedForAutoPoolGuard(me.szModule, me.szExePath)) {
        return 0;
    }

    struct PoolPattern {
        const char* name;
        uintptr_t poolPtr;
    };

    const PoolPattern patterns[] = {
        { "PedExtendedData::AllocateBlocks", kOriginalPedPoolPtr },
        { "VehicleExtendedData::AllocateBlocks", kOriginalVehiclePoolPtr },
        { "ObjectExtendedData::AllocateBlocks", kOriginalObjectPoolPtr },
    };

    const uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
    const uintptr_t end = base + static_cast<uintptr_t>(me.modBaseSize);
    uint32_t patched = 0;

    for (uintptr_t regionCursor = base; regionCursor < end && patched < remainingPatchBudget;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<const void*>(regionCursor), &mbi, sizeof(mbi)) || !mbi.RegionSize) {
            break;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t regionEnd = regionBase + static_cast<uintptr_t>(mbi.RegionSize);
        const uintptr_t scanStart = regionBase > base ? regionBase : base;
        const uintptr_t scanEnd = regionEnd < end ? regionEnd : end;
        regionCursor = regionEnd > regionCursor ? regionEnd : regionCursor + 0x1000;

        if (!IsExecutableRegion(mbi) || scanEnd <= scanStart + 5) {
            continue;
        }

        for (uintptr_t address = scanStart; address + 96 <= scanEnd && patched < remainingPatchBudget; ++address) {
            for (const PoolPattern& pattern : patterns) {
                if (!LooksLikePluginSdkPoolAllocateBlocks(address, pattern.poolPtr)) {
                    continue;
                }

                uintptr_t ignoredContinue = 0;
                if (InstallOneCleoPlusPoolAllocateGuard(pattern.name, address, pattern.poolPtr, 0, &ignoredContinue)) {
                    ++patched;
                    Log("auto pool guard: patched module=%s pattern=%s rva=0x%08X path=%s",
                        me.szModule,
                        pattern.name,
                        static_cast<uint32_t>(address - base),
                        me.szExePath);
                }
                break;
            }
        }
    }

    return patched;
}

void InstallAutoPoolAllocateGuards()
{
    if (!g_config.enableAutoPoolAllocateGuard || g_config.autoPoolAllocateGuardMaxPatches <= 0) {
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        Log("auto pool guard: module snapshot failed gle=%lu", GetLastError());
        return;
    }

    uint32_t patched = 0;
    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);

    Log("auto pool guard: scan begin allowlist='%s' denylist='%s' maxPatches=%d deferredReplay=%d",
        g_config.autoPoolAllocateGuardAllowlist,
        g_config.autoPoolAllocateGuardDenylist,
        g_config.autoPoolAllocateGuardMaxPatches,
        g_config.enableDeferredPoolAllocateReplay ? 1 : 0);

    if (Module32First(snapshot, &me)) {
        do {
            if (patched >= static_cast<uint32_t>(g_config.autoPoolAllocateGuardMaxPatches)) {
                break;
            }
            patched += ScanOneModuleForPoolAllocateGuards(
                me,
                static_cast<uint32_t>(g_config.autoPoolAllocateGuardMaxPatches) - patched);
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    Log("auto pool guard: scan end patched=%u", patched);
}

bool ModuleDeniedForRuntimeRewrite(const char* moduleName, const char* modulePath)
{
    return ModuleDeniedByCompatPolicy(moduleName, modulePath) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.forceNoRuntimeRewrite) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.runtimeRewriteDenylist);
}

bool ModuleAllowedForRuntimeRewrite(const char* moduleName, const char* modulePath)
{
    return ModuleAllowedByLegacyPolicy(moduleName, modulePath) &&
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.runtimeRewriteAllowlist) &&
        !ModuleDeniedForRuntimeRewrite(moduleName, modulePath);
}

RuntimeRewriteTarget ParseRuntimeRewriteTarget(const char* raw)
{
    if (!raw || !raw[0]) {
        return RUNTIME_REWRITE_TARGET_STATIC;
    }
    if (_stricmp(raw, "CModelInfo") == 0 || _stricmp(raw, "ModelInfo") == 0 ||
        _stricmp(raw, "CModelInfo::ms_modelInfoPtrs") == 0) {
        return RUNTIME_REWRITE_TARGET_MODEL_INFO;
    }
    if (_stricmp(raw, "CStreaming") == 0 || _stricmp(raw, "StreamingInfo") == 0 ||
        _stricmp(raw, "CStreaming::ms_aInfoForModel") == 0) {
        return RUNTIME_REWRITE_TARGET_STREAMING_INFO;
    }
    if (_stricmp(raw, "RadarTrace") == 0 || _stricmp(raw, "CRadar::ms_RadarTrace") == 0) {
        return RUNTIME_REWRITE_TARGET_RADAR_TRACE;
    }
    if (_stricmp(raw, "AnimBlocks") == 0 || _stricmp(raw, "CAnimManager::ms_aAnimBlocks") == 0) {
        return RUNTIME_REWRITE_TARGET_ANIM_BLOCKS;
    }
    if (_stricmp(raw, "StreamedScripts") == 0 || _stricmp(raw, "CTheScripts::StreamedScripts") == 0) {
        return RUNTIME_REWRITE_TARGET_STREAMED_SCRIPTS;
    }
    if (_stricmp(raw, "HandlingManager") == 0 || _stricmp(raw, "mod_HandlingManager") == 0) {
        return RUNTIME_REWRITE_TARGET_HANDLING_MANAGER;
    }
    if (_stricmp(raw, "VehicleRecording") == 0 || _stricmp(raw, "CVehicleRecording::StreamingArray") == 0) {
        return RUNTIME_REWRITE_TARGET_VEHICLE_RECORDING;
    }
    return RUNTIME_REWRITE_TARGET_STATIC;
}

const char* RuntimeRewriteTargetName(RuntimeRewriteTarget target)
{
    switch (target) {
    case RUNTIME_REWRITE_TARGET_MODEL_INFO: return "CModelInfo";
    case RUNTIME_REWRITE_TARGET_STREAMING_INFO: return "CStreaming";
    case RUNTIME_REWRITE_TARGET_RADAR_TRACE: return "RadarTrace";
    case RUNTIME_REWRITE_TARGET_ANIM_BLOCKS: return "AnimBlocks";
    case RUNTIME_REWRITE_TARGET_STREAMED_SCRIPTS: return "StreamedScripts";
    case RUNTIME_REWRITE_TARGET_HANDLING_MANAGER: return "HandlingManager";
    case RUNTIME_REWRITE_TARGET_VEHICLE_RECORDING: return "VehicleRecording";
    default: return "Static";
    }
}

uint32_t ResolveRuntimeRewriteNewValue(const RuntimeRewriteRule& rule)
{
    switch (rule.target) {
    case RUNTIME_REWRITE_TARGET_MODEL_INFO:
        return static_cast<uint32_t>(g_relocatedCModelInfoPtrs);
    case RUNTIME_REWRITE_TARGET_STREAMING_INFO:
        return static_cast<uint32_t>(g_relocatedStreamingInfo);
    case RUNTIME_REWRITE_TARGET_RADAR_TRACE:
        if (!g_config.enableRadarTraceRuntimeRewrite || !g_radarTraceBase || g_radarTraceBase == kOriginalRadarTrace) {
            return 0;
        }
        return static_cast<uint32_t>(g_radarTraceBase);
    case RUNTIME_REWRITE_TARGET_ANIM_BLOCKS:
        return static_cast<uint32_t>(g_relocatedAnimBlocks);
    case RUNTIME_REWRITE_TARGET_STREAMED_SCRIPTS:
        return static_cast<uint32_t>(g_relocatedStreamedScripts);
    case RUNTIME_REWRITE_TARGET_HANDLING_MANAGER:
        return static_cast<uint32_t>(g_relocatedHandlingManager);
    case RUNTIME_REWRITE_TARGET_VEHICLE_RECORDING:
        return static_cast<uint32_t>(g_relocatedVehicleRecordingStreamingArray);
    default:
        return rule.staticNewValue;
    }
}

void AddRuntimeRewriteRule(
    const char* name,
    uint32_t oldValue,
    RuntimeRewriteTarget target,
    uint32_t staticNewValue,
    bool align4,
    bool executableOnly,
    bool auditOnly,
    uint32_t maxPatchesPerModule,
    const char* allowlist = "",
    const char* denylist = "")
{
    if (g_runtimeRewriteRuleCount >= kMaxRuntimeRewriteRules || !oldValue) {
        return;
    }

    RuntimeRewriteRule& rule = g_runtimeRewriteRules[g_runtimeRewriteRuleCount++];
    rule.enabled = true;
    rule.auditOnly = auditOnly;
    rule.align4 = align4;
    rule.executableOnly = executableOnly;
    rule.oldValue = oldValue;
    rule.target = target;
    rule.staticNewValue = staticNewValue;
    rule.maxPatchesPerModule = maxPatchesPerModule ? maxPatchesPerModule : static_cast<uint32_t>(g_config.runtimeRewriteDefaultMaxPatchesPerModule);
    strncpy_s(rule.name, name && name[0] ? name : RuntimeRewriteTargetName(target), _TRUNCATE);
    strncpy_s(rule.allowlist, allowlist ? allowlist : "", _TRUNCATE);
    strncpy_s(rule.denylist, denylist ? denylist : "", _TRUNCATE);
}

void AddBuiltInRuntimeRewriteRules()
{
    AddRuntimeRewriteRule("CModelInfo::ms_modelInfoPtrs", static_cast<uint32_t>(kOriginalCModelInfoPtrs),
        RUNTIME_REWRITE_TARGET_MODEL_INFO, 0, true, false, false, 256);
    AddRuntimeRewriteRule("CStreaming::ms_aInfoForModel", static_cast<uint32_t>(kOriginalStreamingInfo),
        RUNTIME_REWRITE_TARGET_STREAMING_INFO, 0, true, false, false, 256);
    AddRuntimeRewriteRule("CRadar::ms_RadarTrace", static_cast<uint32_t>(kOriginalRadarTrace),
        RUNTIME_REWRITE_TARGET_RADAR_TRACE, 0, true, false, false, 512);
}

void LoadRuntimeRewriteRules()
{
    std::memset(g_runtimeRewriteRules, 0, sizeof(g_runtimeRewriteRules));
    g_runtimeRewriteRuleCount = 0;

    if (!g_config.enableRuntimeRewriteRuleTable || g_config.runtimeRewriteRuleCount <= 0) {
        AddBuiltInRuntimeRewriteRules();
        return;
    }

    for (int i = 1; i <= g_config.runtimeRewriteRuleCount && g_runtimeRewriteRuleCount < kMaxRuntimeRewriteRules; ++i) {
        char prefix[32]{};
        sprintf_s(prefix, "Rule%03d", i);

        char key[96]{};
        sprintf_s(key, "%sEnabled", prefix);
        if (!ReadBridgeBool(key, true)) {
            continue;
        }

        sprintf_s(key, "%sOld", prefix);
        const uint32_t oldValue = ReadBridgeU32(key, 0);
        if (!oldValue) {
            Log("runtime rewrite rule: skipped %s missing/invalid Old", prefix);
            continue;
        }

        char name[64]{};
        sprintf_s(key, "%sName", prefix);
        ReadBridgeText(key, name, sizeof(name), prefix);

        char targetText[64]{};
        sprintf_s(key, "%sTarget", prefix);
        ReadBridgeText(key, targetText, sizeof(targetText), "Static");

        sprintf_s(key, "%sNew", prefix);
        const uint32_t staticNewValue = ReadBridgeU32(key, 0);

        sprintf_s(key, "%sAlign4", prefix);
        const bool align4 = ReadBridgeBool(key, true);
        sprintf_s(key, "%sExecutableOnly", prefix);
        const bool executableOnly = ReadBridgeBool(key, false);
        sprintf_s(key, "%sAuditOnly", prefix);
        const bool auditOnly = ReadBridgeBool(key, false);
        sprintf_s(key, "%sMaxPatchesPerModule", prefix);
        const uint32_t maxPatches = static_cast<uint32_t>(ReadBridgeInt(key, g_config.runtimeRewriteDefaultMaxPatchesPerModule, 1, 100000));

        char allowlist[256]{};
        sprintf_s(key, "%sAllowlist", prefix);
        ReadBridgeText(key, allowlist, sizeof(allowlist));
        char denylist[256]{};
        sprintf_s(key, "%sDenylist", prefix);
        ReadBridgeText(key, denylist, sizeof(denylist));

        AddRuntimeRewriteRule(name, oldValue, ParseRuntimeRewriteTarget(targetText), staticNewValue,
            align4, executableOnly, auditOnly, maxPatches, allowlist, denylist);
    }

    if (!g_runtimeRewriteRuleCount) {
        AddBuiltInRuntimeRewriteRules();
    }

    Log("runtime rewrite rules: loaded count=%u source=%s configuredCount=%d",
        g_runtimeRewriteRuleCount,
        g_config.enableRuntimeRewriteRuleTable && g_config.runtimeRewriteRuleCount > 0 ? "ini" : "built-in",
        g_config.runtimeRewriteRuleCount);
}

bool IsExecutableProtect(DWORD protect)
{
    const DWORD p = protect & 0xFF;
    return p == PAGE_EXECUTE || p == PAGE_EXECUTE_READ ||
        p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
}

bool RuleDeniedForRuntimeRewrite(const RuntimeRewriteRule& rule, const char* moduleName, const char* modulePath)
{
    return ModuleDeniedForRuntimeRewrite(moduleName, modulePath) ||
        ModuleMatchesRewriteList(moduleName, modulePath, rule.denylist);
}

bool RuleAllowedForRuntimeRewrite(const RuntimeRewriteRule& rule, const char* moduleName, const char* modulePath)
{
    const char* allow = rule.allowlist[0] ? rule.allowlist : g_config.runtimeRewriteAllowlist;
    return ModuleMatchesRewriteList(moduleName, modulePath, allow) &&
        !RuleDeniedForRuntimeRewrite(rule, moduleName, modulePath);
}

bool ModuleInterestingForRuntimeRewrite(const char* moduleName, const char* modulePath)
{
    if (!moduleName || !modulePath) {
        return false;
    }
    if (ContainsCaseInsensitive(moduleName, "FLACompatBridge") ||
        ContainsCaseInsensitive(moduleName, "fastman92") ||
        ContainsCaseInsensitive(moduleName, "DINPUT8") ||
        ContainsCaseInsensitive(moduleName, "vorbis")) {
        return false;
    }

    if (ModuleAllowedForRuntimeRewrite(moduleName, modulePath) ||
        ModuleDeniedForRuntimeRewrite(moduleName, modulePath)) {
        return true;
    }

    for (uint32_t i = 0; i < g_runtimeRewriteRuleCount; ++i) {
        const RuntimeRewriteRule& rule = g_runtimeRewriteRules[i];
        if (!rule.enabled) {
            continue;
        }
        if (RuleAllowedForRuntimeRewrite(rule, moduleName, modulePath) ||
            RuleDeniedForRuntimeRewrite(rule, moduleName, modulePath)) {
            return true;
        }
    }

    return strstr(moduleName, ".asi") || strstr(moduleName, ".cleo") || strstr(moduleName, "modloader") ||
        strstr(moduleName, "SilentPatch") || strstr(moduleName, "MixSets") || strstr(moduleName, "Urbanize") ||
        ContainsCaseInsensitive(modulePath, "modloader");
}

void RewriteOneModuleConstants(const MODULEENTRY32& me, bool logDetails)
{
    const bool denied = ModuleDeniedForRuntimeRewrite(me.szModule, me.szExePath);
    const bool allowed = ModuleAllowedForRuntimeRewrite(me.szModule, me.szExePath);
    uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
    uintptr_t end = base + me.modBaseSize;
    uintptr_t cursor = base;
    uint32_t hits = 0;
    uint32_t patched = 0;
    uint32_t rulePatched[kMaxRuntimeRewriteRules]{};

    while (cursor < end) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<const void*>(cursor), &mbi, sizeof(mbi))) {
            cursor += 0x1000;
            continue;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t regionEnd = regionBase + mbi.RegionSize;
        cursor = regionEnd;

        if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
            continue;
        }

        const bool regionExecutable = IsExecutableProtect(mbi.Protect);
        uintptr_t scanStart = regionBase < base ? base : regionBase;
        uintptr_t scanEnd = regionEnd > end ? end : regionEnd;
        if (scanEnd <= scanStart || scanEnd - scanStart < sizeof(uint32_t)) {
            continue;
        }

        const size_t size = scanEnd - scanStart;
        uint8_t* buffer = new uint8_t[size];
        bool copied = false;
        __try {
            std::memcpy(buffer, reinterpret_cast<const void*>(scanStart), size);
            copied = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            copied = false;
        }
        if (!copied) {
            delete[] buffer;
            continue;
        }

        for (size_t i = 0; i + sizeof(uint32_t) <= size; ++i) {
            uint32_t value = 0;
            std::memcpy(&value, buffer + i, sizeof(value));
            for (uint32_t ruleIndex = 0; ruleIndex < g_runtimeRewriteRuleCount; ++ruleIndex) {
                RuntimeRewriteRule& rule = g_runtimeRewriteRules[ruleIndex];
                const uint32_t newValue = ResolveRuntimeRewriteNewValue(rule);
                if (!rule.enabled || !newValue || value != rule.oldValue) {
                    continue;
                }
                const uintptr_t patchAddress = scanStart + i;
                if ((rule.align4 && (patchAddress & 3)) ||
                    (rule.executableOnly && !regionExecutable)) {
                    continue;
                }
                ++hits;
                const bool ruleDenied = RuleDeniedForRuntimeRewrite(rule, me.szModule, me.szExePath);
                const bool ruleAllowed = RuleAllowedForRuntimeRewrite(rule, me.szModule, me.szExePath);
                if (logDetails && hits <= 64) {
                    Log("runtime rewrite %s: module=%s rule=%s target=%s address=0x%08X old=0x%08X new=0x%08X allowed=%d denied=%d auditOnly=%d",
                        g_config.enableRuntimeRewrite && !rule.auditOnly ? "candidate" : "audit",
                        me.szModule,
                        rule.name,
                        RuntimeRewriteTargetName(rule.target),
                        patchAddress,
                        rule.oldValue,
                        newValue,
                        ruleAllowed ? 1 : 0,
                        ruleDenied ? 1 : 0,
                        rule.auditOnly ? 1 : 0);
                }
                if (g_config.enableRuntimeRewrite && !rule.auditOnly && ruleAllowed &&
                    rulePatched[ruleIndex] < rule.maxPatchesPerModule &&
                    WriteBytesWithProtect(patchAddress, reinterpret_cast<const uint8_t*>(&newValue), sizeof(newValue))) {
                    ++patched;
                    ++rulePatched[ruleIndex];
                    Log("runtime rewrite patched: module=%s rule=%s target=%s address=0x%08X countForRule=%u",
                        me.szModule, rule.name, RuntimeRewriteTargetName(rule.target), patchAddress, rulePatched[ruleIndex]);
                }
            }
        }

        delete[] buffer;
    }

    if ((hits || patched) && (logDetails || patched)) {
        Log("runtime rewrite summary: module=%s hits=%u patched=%u allowed=%d denied=%d",
            me.szModule, hits, patched, allowed ? 1 : 0, denied ? 1 : 0);
    }
}

void RuntimeRewriteHardcodedAddressConstants(bool logDetails)
{
    if (!g_config.enableRuntimeRewriteAudit && !g_config.enableRuntimeRewrite) {
        return;
    }

    if (g_config.enableRuntimeRewrite && !g_config.runtimeRewriteAllowlist[0]) {
        Log("runtime rewrite: enabled but RuntimeRewriteAllowlist is empty; refusing to patch modules");
    }

    bool hasResolvableRule = false;
    for (uint32_t i = 0; i < g_runtimeRewriteRuleCount; ++i) {
        if (g_runtimeRewriteRules[i].enabled && ResolveRuntimeRewriteNewValue(g_runtimeRewriteRules[i]) != 0) {
            hasResolvableRule = true;
            break;
        }
    }

    if (!hasResolvableRule) {
        Log("runtime rewrite: relocated tables unavailable");
        return;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        Log("runtime rewrite: module snapshot failed gle=%lu", GetLastError());
        return;
    }

    if (logDetails) {
        Log("runtime rewrite begin: audit=%d rewrite=%d allowlist='%s' denylist='%s'",
            g_config.enableRuntimeRewriteAudit ? 1 : 0,
            g_config.enableRuntimeRewrite ? 1 : 0,
            g_config.runtimeRewriteAllowlist,
            g_config.runtimeRewriteDenylist);
        Log("runtime rewrite state: modelInfo=0x%08X streaming=0x%08X radarTrace=0x%08X radarLimit=%u radarTraceRewrite=%d",
            g_relocatedCModelInfoPtrs,
            g_relocatedStreamingInfo,
            g_radarTraceBase,
            g_radarTraceLimit,
            g_config.enableRadarTraceRuntimeRewrite ? 1 : 0);
    }

    if (Module32First(snapshot, &me)) {
        do {
            const char* name = me.szModule;
            const char* path = me.szExePath;
            if (!ModuleInterestingForRuntimeRewrite(name, path)) {
                continue;
            }
            RewriteOneModuleConstants(me, logDetails);
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    if (logDetails) {
        Log("runtime rewrite end");
    }
}

DWORD WINAPI RuntimeRewriteRescanThread(void*)
{
    if (!g_config.enableRuntimeRewriteRescan ||
        (!g_config.enableRuntimeRewriteAudit && !g_config.enableRuntimeRewrite) ||
        g_config.runtimeRewriteIterations <= 0) {
        return 0;
    }

    Sleep(static_cast<DWORD>(g_config.runtimeRewriteStartDelayMs));
    Log("runtime rewrite rescan: begin iterations=%d intervalMs=%d",
        g_config.runtimeRewriteIterations, g_config.runtimeRewriteIntervalMs);

    for (int i = 0; i < g_config.runtimeRewriteIterations; ++i) {
        RuntimeRewriteHardcodedAddressConstants(false);
        Sleep(static_cast<DWORD>(g_config.runtimeRewriteIntervalMs));
    }

    Log("runtime rewrite rescan: end");
    return 0;
}

bool StackMentionsModule(uintptr_t esp, const char* needle)
{
    if (!needle || !*needle || !IsReadableCommitted(esp, 32 * sizeof(uintptr_t))) {
        return false;
    }

    for (int i = 0; i < 32; ++i) {
        uintptr_t value = 0;
        __try {
            value = reinterpret_cast<const uintptr_t*>(esp)[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        char moduleName[MAX_PATH]{};
        if (ModuleBaseFromAddress(value, moduleName, sizeof(moduleName)) &&
            ContainsCaseInsensitive(moduleName, needle)) {
            return true;
        }
    }
    return false;
}

bool FindStackValue(uintptr_t esp, uintptr_t needle, int maxDwords, uintptr_t* valueAddress)
{
    if (!needle || maxDwords <= 0 || !IsReadableCommitted(esp, static_cast<size_t>(maxDwords) * sizeof(uintptr_t))) {
        return false;
    }

    for (int i = 0; i < maxDwords; ++i) {
        uintptr_t value = 0;
        __try {
            value = reinterpret_cast<const uintptr_t*>(esp)[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        if (value == needle) {
            if (valueAddress) {
                *valueAddress = esp + static_cast<uintptr_t>(i) * sizeof(uintptr_t);
            }
            return true;
        }
    }

    return false;
}

bool IsFreeOrNonExecutableRegion(uintptr_t address, DWORD* stateOut = nullptr, DWORD* protectOut = nullptr)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        if (stateOut) {
            *stateOut = 0;
        }
        if (protectOut) {
            *protectOut = 0;
        }
        return true;
    }

    if (stateOut) {
        *stateOut = mbi.State;
    }
    if (protectOut) {
        *protectOut = mbi.Protect;
    }

    if (mbi.State == MEM_FREE || mbi.State != MEM_COMMIT ||
        (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return true;
    }

    return !IsExecutableCommitted(address);
}

bool StackMentionsGtaScriptOpcodePath(uintptr_t esp)
{
    static const uintptr_t kScriptPathHints[] = {
        0x00456878, // CRunningScript native opcode dispatch return path seen around 0213/0215 crashes
        0x00457332,
        0x0047E63B, // CRunningScript::ProcessCommands500To599, pickup-related command path
        0x0047DDA5, // CRunningScript::ProcessCommands400To499
        0x00469FF7, // CRunningScript::Process
        0x0046A220, // CTheScripts::Process
        0x0083D326
    };

    for (uintptr_t hint : kScriptPathHints) {
        if (FindStackValue(esp, hint, 48, nullptr)) {
            return true;
        }
    }
    return false;
}

bool BreakRepeatedNonExecutableExceptionLoop(EXCEPTION_POINTERS* info)
{
    if (!g_config.enableExceptionLoopBreaker || !info || !info->ExceptionRecord ||
        info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return false;
    }

    const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    const uintptr_t fault = info->ExceptionRecord->NumberParameters > 1
        ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1])
        : 0;

    if (!eip || IsExecutableCommitted(eip)) {
        return false;
    }

    static uintptr_t lastEip = 0;
    static uintptr_t lastFault = 0;
    static DWORD firstTick = 0;
    static LONG sameCount = 0;
    static LONG tripped = 0;

    const DWORD now = GetTickCount();
    if (eip != lastEip || fault != lastFault) {
        lastEip = eip;
        lastFault = fault;
        firstTick = now;
        sameCount = 1;
        return false;
    }

    const LONG count = ++sameCount;
    if (count < g_config.exceptionLoopBreakerThreshold || InterlockedExchange(&tripped, 1) != 0) {
        return false;
    }

    Log("exception loop breaker: repeated non-executable EIP eip=0x%08X fault=0x%08X count=%ld elapsedMs=%lu terminate=%d",
        eip,
        fault,
        count,
        now - firstTick,
        g_config.exceptionLoopBreakerTerminate ? 1 : 0);
    LogMemoryRegion("loop-breaker-eip", eip);
    LogMemoryRegion("loop-breaker-fault", fault);
#if defined(_M_IX86)
    if (info->ContextRecord) {
        Log("exception loop breaker context: eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X esp=0x%08X",
            info->ContextRecord->Eax,
            info->ContextRecord->Ebx,
            info->ContextRecord->Ecx,
            info->ContextRecord->Edx,
            info->ContextRecord->Esi,
            info->ContextRecord->Edi,
            info->ContextRecord->Esp);
        LogStackModules(info->ContextRecord->Esp);
    }
#endif

    if (g_config.exceptionLoopBreakerTerminate) {
        TerminateProcess(GetCurrentProcess(), 0xFAEE0001u);
    }

    return true;
}

const char* KnownGameAddressName(uintptr_t eip)
{
    switch (eip) {
    case 0x00533650:
        return "CEntity::GetBoundCentre prologue";
    case 0x0053368B:
        return "CEntity::GetBoundCentre / CPtrListDoubleLink::AddItem null output path";
    case 0x00533769:
        return "CEntity::GetRectAdd caller area";
    case 0x0054F3B3:
        return "CPlaceable::RemoveMatrix null matrix access";
    case 0x005A1FA1:
        return "CObject create patched callsite 1";
    case 0x005A2016:
        return "CObject create patched callsite 2";
    case 0x0059FB1E:
        return "CLEO+/object-create inline patch area";
    case 0x00582870:
        return "CRadar::GetActualBlipArrayIndex entry";
    case 0x00582889:
        return "CRadar::GetActualBlipArrayIndex counter read";
    case 0x0040FB80:
        return "CColModelPool::New / CPools::ms_pColModelPool allocation";
    case 0x005B31A5:
        return "CColAccel::startCache / CColStore pool size read";
    case 0x006F7524:
        return "CTrain::InitTrains train carriage/IPL init read";
    case 0x0061A500:
        return "CPool::New generic / Tasks pool allocation";
    case 0x0061A5A0:
        return "CTask::operator new / CPools::ms_pTaskPool";
    case 0x004D41C0:
        return "CAnimManager::UncompressAnimation";
    case 0x004CEEC0:
        return "CAnimBlendAssociation::Init(static)";
    case 0x004D1680:
    case 0x004D1710:
        return "FrameUpdateCallBackSkinnedWithVelocityExtraction";
    default:
        return "";
    }
}

void LogCrashClassification(EXCEPTION_POINTERS* info)
{
    if (!g_config.enableCrashClassification || !info || !info->ExceptionRecord) {
        return;
    }

    const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    const uintptr_t fault = info->ExceptionRecord->NumberParameters > 1
        ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1])
        : 0;

    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(eip, moduleName, sizeof(moduleName));
    const char* known = KnownGameAddressName(eip);
    const bool lowFault = fault < 0x10000;
    DWORD eipState = 0;
    DWORD eipProtect = 0;
    const bool staleExec = IsFreeOrNonExecutableRegion(eip, &eipState, &eipProtect) && moduleBase == 0;

#if defined(_M_IX86)
    CONTEXT* ctx = info->ContextRecord;
    const uintptr_t esp = ctx ? ctx->Esp : 0;
    const bool stackUrbanize = esp && StackMentionsModule(esp, "urbanize");
    const bool stackFla = esp && StackMentionsModule(esp, "fastman92");
    const bool stackCleo = esp && StackMentionsModule(esp, "cleo");
    const bool stackScriptOpcode = esp && StackMentionsGtaScriptOpcodePath(esp);
#else
    const uintptr_t esp = 0;
    const bool stackUrbanize = false;
    const bool stackFla = false;
    const bool stackCleo = false;
    const bool stackScriptOpcode = false;
#endif

    const char* category = "UNKNOWN";
    if (staleExec && stackScriptOpcode) {
        category = "SCRIPT_OR_PLUGIN_STALE_EXEC_POINTER";
    } else if (staleExec) {
        category = "STALE_EXEC_POINTER_OR_FREED_CODE";
    } else if (eip == 0x0053368B || eip == 0x00533650) {
        category = "MODEL_BOUND_CENTRE_OR_STREAMING";
    } else if (eip == kCColModelPoolNewEntry) {
        category = "COL_MODEL_POOL_ALLOCATION";
    } else if (eip >= 0x00582870 && eip <= 0x00582899) {
        category = "RADAR_BLIP_HANDLE_OR_TRACE_LIMIT";
    } else if (eip == kCColAccelStartCachePoolRead) {
        category = "COL_ACCEL_OR_RELOCATED_COL_POOL";
    } else if (eip == kGenericPoolNewEntry) {
        category = "TASK_POOL_ALLOCATION";
    } else if (eip >= 0x006F7400 && eip <= 0x006F7800) {
        category = "TRAIN_INIT_OR_FLA_CARRIAGE_LOADER";
    } else if (eip == 0x0054F3B3) {
        category = "PLACEABLE_MATRIX_LIFETIME";
    } else if (eip == 0x004D41C0 || eip == 0x004CEEC0 || (eip >= 0x004D1680 && eip <= 0x004D1A4A)) {
        category = "ANIMATION_POINTER";
    } else if (ContainsCaseInsensitive(moduleName, "CLEO")) {
        category = "CLEO_PLUGIN_OR_SCRIPT_BRIDGE";
    } else if (ContainsCaseInsensitive(moduleName, "fastman92")) {
        category = "FLA_INTERNAL_OR_FLA_HOOK";
    } else if (lowFault) {
        category = "NULL_OR_LOW_POINTER";
    }

    Log("crash-class: category=%s eip=0x%08X module=%s+0x%X known='%s' fault=0x%08X lowFault=%d staleExec=%d eipState=%s eipProtect=%s stackUrbanize=%d stackFLA=%d stackCLEO=%d stackScriptOpcode=%d",
        category,
        eip,
        moduleName,
        moduleBase ? eip - moduleBase : 0,
        known,
        fault,
        lowFault ? 1 : 0,
        staleExec ? 1 : 0,
        StateName(eipState),
        ProtectName(eipProtect),
        stackUrbanize ? 1 : 0,
        stackFla ? 1 : 0,
        stackCleo ? 1 : 0,
        stackScriptOpcode ? 1 : 0);
}

void LogModuleSnapshot()
{
    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        Log("module snapshot failed: gle=%lu", GetLastError());
        return;
    }

    Log("module snapshot begin");
    if (Module32First(snapshot, &me)) {
        do {
            const char* name = me.szModule;
            const char* path = me.szExePath;
            const bool interesting =
                strstr(name, ".asi") || strstr(name, ".cleo") || strstr(name, "DINPUT8") ||
                strstr(name, "vorbis") || strstr(name, "fastman92") || strstr(name, "cleo") ||
                strstr(name, "modloader") || strstr(name, "SilentPatch") || strstr(name, "MixSets") ||
                strstr(name, "Urbanize") || ContainsCaseInsensitive(path, "modloader");

            if (interesting) {
                Log("module: base=0x%08X size=0x%08X name=%s path=%s",
                    reinterpret_cast<uintptr_t>(me.modBaseAddr), me.modBaseSize, me.szModule, me.szExePath);
                if (g_config.enableRiskConstantScan) {
                    ScanRiskyAddressConstants(me.szExePath);
                }
            }
        } while (Module32Next(snapshot, &me));
    }
    CloseHandle(snapshot);
    Log("module snapshot end");
}

void LogFLAExports()
{
    HMODULE fla = FindFlaModule();
    if (!fla) {
        Log("FLA module not loaded yet");
        return;
    }

    using GetProjectVersionFn = const char* (__cdecl*)();
    using GetNumberOfFileIDsFn = int32_t(__cdecl*)();
    using AreDifficultIDsExtendedFn = bool(__cdecl*)();

    auto getProjectVersion = reinterpret_cast<GetProjectVersionFn>(GetProcAddress(fla, "GetProjectVersion"));
    auto getNumberOfFileIDs = reinterpret_cast<GetNumberOfFileIDsFn>(GetProcAddress(fla, "GetNumberOfFileIDs"));
    auto areDifficultIDsExtended = reinterpret_cast<AreDifficultIDsExtendedFn>(GetProcAddress(fla, "AreDifficultIDsExtended"));

    Log("FLA module: base=0x%08X GetProjectVersion=%p GetNumberOfFileIDs=%p AreDifficultIDsExtended=%p",
        reinterpret_cast<uintptr_t>(fla), getProjectVersion, getNumberOfFileIDs, areDifficultIDsExtended);

    if (getProjectVersion) {
        Log("FLA export: ProjectVersion=%s", getProjectVersion());
    }
    if (getNumberOfFileIDs) {
        Log("FLA export: NumberOfFileIDs=%d", getNumberOfFileIDs());
    }
    if (areDifficultIDsExtended) {
        Log("FLA export: DifficultIDsExtended=%d", areDifficultIDsExtended() ? 1 : 0);
    }

    Log("FLA new ABI present: GetFLACompatibilityInfo=%p GetFLARuntimeAddress=%p",
        GetProcAddress(fla, "GetFLACompatibilityInfo"),
        GetProcAddress(fla, "GetFLARuntimeAddress"));
}

LONG CALLBACK BridgeVectoredExceptionHandler(EXCEPTION_POINTERS* info)
{
    if (!info || !info->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

#if defined(_M_IX86)
    if (g_config.enablePlaceableRemoveMatrixGuard &&
        g_config.enablePlaceableRemoveMatrixSkipGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        (info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(static_cast<uintptr_t>(g_config.matrixGuardRemoveMatrixEntry)) ||
            info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(static_cast<uintptr_t>(g_config.matrixGuardRemoveMatrixNullLoad))) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        const bool nullThisAtEntry = eip == g_config.matrixGuardRemoveMatrixEntry && ctx->Ecx == 0 && fault == 0x14;
        const bool nullMatrixAfterLoad = eip == g_config.matrixGuardRemoveMatrixNullLoad && ctx->Eax == 0 && fault < 0x10000;

        if ((nullThisAtEntry || nullMatrixAfterLoad) && MatrixGuardCodeLooksCompatible(eip)) {
            const uintptr_t returnAddress = ResolvePlaceableRemoveMatrixReturnAddress(ctx->Esp);

            if (returnAddress) {
                uintptr_t recoveredLink = 0;
                uintptr_t recoveredOwner = 0;
                const char* recoveredSource = "none";
                const bool recoveredMatrix = RecoverOldestMatrixLinkToFreeList(ctx->Ecx, &recoveredLink, &recoveredOwner, &recoveredSource);

                static LONG recoverCount = 0;
                const LONG count = InterlockedIncrement(&recoverCount);
                if (count <= 32) {
                    int32_t modelId = -1;
                    if (IsReadableCommitted(ctx->Esi + 0x22, sizeof(uint16_t))) {
                        modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(ctx->Esi + 0x22));
                    }

                    Log("placeable remove-matrix guard: skipped invalid RemoveMatrix eip=0x%08X fault=0x%08X this=0x%08X matrix=0x%08X possibleEntity=0x%08X model=%u recovered=%d source=%s link=0x%08X owner=0x%08X return=0x%08X esp=0x%08X",
                        eip,
                        fault,
                        ctx->Ecx,
                        ctx->Eax,
                        ctx->Esi,
                        modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
                        recoveredMatrix ? 1 : 0,
                        recoveredSource,
                        recoveredLink,
                        recoveredOwner,
                        returnAddress,
                        ctx->Esp);
                    LogStackModules(ctx->Esp);
                }

                ctx->Eip = static_cast<DWORD>(returnAddress);
                ctx->Esp += 0x10;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }

    if (g_config.enablePlaceableRemoveMatrixGuard &&
        g_config.enablePlaceableStaticMatrixAllocGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(static_cast<uintptr_t>(g_config.matrixGuardAllocateStaticOwnerWrite)) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Eax == 0 && fault == 0x48 && ctx->Ecx == MatrixListBase() && ctx->Esi != 0 &&
            MatrixGuardCodeLooksCompatible(g_config.matrixGuardAllocateStaticOwnerWrite)) {
            uintptr_t recoveredLink = 0;
            uintptr_t recoveredOwner = 0;
            uintptr_t allocatedLink = 0;
            const char* recoveredSource = "none";

            bool allocated = AllocateStaticMatrixLinkFromFreeList(ctx->Esi, &allocatedLink);
            bool recovered = false;
            if (!allocated) {
                recovered = RecoverOldestMatrixLinkToFreeList(ctx->Esi, &recoveredLink, &recoveredOwner, &recoveredSource);
                if (recovered) {
                    allocated = AllocateStaticMatrixLinkFromFreeList(ctx->Esi, &allocatedLink);
                }
            }

            static LONG allocateRecoverCount = 0;
            const LONG count = InterlockedIncrement(&allocateRecoverCount);
            if (count <= 32) {
                int32_t modelId = -1;
                if (IsReadableCommitted(ctx->Esi + 0x22, sizeof(uint16_t))) {
                    modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(ctx->Esi + 0x22));
                }

                Log("placeable static-matrix alloc guard: eip=0x%08X fault=0x%08X entity=0x%08X model=%u allocated=%d allocLink=0x%08X recovered=%d source=%s recoveredLink=0x%08X recoveredOwner=0x%08X matrixBase=0x%08X esp=0x%08X",
                    g_config.matrixGuardAllocateStaticOwnerWrite,
                    fault,
                    ctx->Esi,
                    modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
                    allocated ? 1 : 0,
                    allocatedLink,
                    recovered ? 1 : 0,
                    recoveredSource,
                    recoveredLink,
                    recoveredOwner,
                    MatrixListBase(),
                    ctx->Esp);
                LogStackModules(ctx->Esp);
            }

            if (allocated) {
                ctx->Eax = static_cast<DWORD>(allocatedLink);
                ctx->Eip = g_config.matrixGuardAllocateStaticResume;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCBuildingPoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            if (EnsureLazyCPoolReady(kOriginalBuildingPoolPtr, "CBuildingPool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kOriginalBuildingPoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        Log("building pool guard: supplied lazy pool eip=0x%08X pool=0x%08X return=0x%08X",
                            kCBuildingPoolNewEntry,
                            pool,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableColModelPoolNewGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCColModelPoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            uint32_t colModelPool = 0;
            SafeReadU32(kOriginalColModelPoolPtr, &colModelPool);
            if (!colModelPool) {
                EnsureLazyCPoolReady(kOriginalColModelPoolPtr, "CColModelPool::New");
                SafeReadU32(kOriginalColModelPoolPtr, &colModelPool);
            }
            const bool validPool =
                colModelPool != 0 &&
                IsReadableCommitted(colModelPool, 0x14) &&
                IsReadableCommitted(colModelPool + 0x08, sizeof(uint32_t));

            const LONG logCount = InterlockedIncrement(&g_colModelPoolNewGuardLogs);
            if (logCount <= 16) {
                uint32_t poolSize = 0;
                uint32_t firstFree = 0;
                if (validPool) {
                    SafeReadU32(colModelPool + 0x08, &poolSize);
                    SafeReadU32(colModelPool + 0x0C, &firstFree);
                }
                Log("col model pool guard: null this at CColModelPool::New fault=0x%08X poolPtr[0x%08X]=0x%08X valid=%d size=%u firstFree=%u return=0x%08X",
                    fault,
                    kOriginalColModelPoolPtr,
                    colModelPool,
                    validPool ? 1 : 0,
                    poolSize,
                    firstFree,
                    IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                LogBytes("col-model-pool-new-eip", kCColModelPoolNewEntry, 64);
                LogStackModules(ctx->Esp);
            }

            if (validPool) {
                ctx->Ecx = colModelPool;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }

    if (info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCPedIntelligencePoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            if (EnsureLazyCPoolReady(kPedIntelligencePoolPtr, "CPedIntelligencePool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kPedIntelligencePoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kGenericPoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            uintptr_t returnAddress = 0;
            if (IsReadableCommitted(ctx->Esp, sizeof(returnAddress))) {
                __try {
                    returnAddress = *reinterpret_cast<const uintptr_t*>(ctx->Esp);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    returnAddress = 0;
                }
            }

            if (EnsureLazyCPoolReady(kTasksPoolPtr, "CTask pool generic CPool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kTasksPoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("task pool guard: supplied lazy Tasks pool for generic CPool::New pool=0x%08X size=%u firstFree=%u return=0x%08X esp=0x%08X",
                            pool,
                            poolSize,
                            firstFree,
                            returnAddress,
                            ctx->Esp);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableColAccelStartCachePoolGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCColAccelStartCachePoolRead) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (fault == ctx->Eax + 0x08) {
            if (ctx->Eax == 0 && EnsureLazyCPoolReady(kOriginalColModelPoolPtr, "CColAccel::startCache")) {
                uint32_t colModelPool = 0;
                SafeReadU32(kOriginalColModelPoolPtr, &colModelPool);
                if (IsValidCPool(colModelPool)) {
                    const LONG logCount = InterlockedIncrement(&g_colAccelStartCachePoolGuardLogs);
                    if (logCount <= 16) {
                        Log("col accel guard: CPools recovery produced real ColModel pool eip=0x%08X pool=0x%08X",
                            kCColAccelStartCachePoolRead,
                            colModelPool);
                    }
                    ctx->Eax = colModelPool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }

            const uint32_t fallbackSize = static_cast<uint32_t>(
                ReadFlaInt("Collision size", static_cast<int>(ReadIniU32("FILE_TYPE_COL", 500)), 1, 1000000));
            uint32_t originalColPool = 0;
            SafeReadU32(kCColStorePoolPtr, &originalColPool);

            g_colAccelFakePool[0] = 0;
            g_colAccelFakePool[1] = 0;
            g_colAccelFakePool[2] = fallbackSize;
            g_colAccelFakePool[3] = 0;

            const LONG logCount = InterlockedIncrement(&g_colAccelStartCachePoolGuardLogs);
            if (logCount <= 16) {
                Log("col accel guard: supplied fallback CColStore pool for startCache eip=0x%08X oldEax=0x%08X fault=0x%08X originalPoolPtr[0x%08X]=0x%08X fallbackSize=%u fakePool=0x%08X",
                    kCColAccelStartCachePoolRead,
                    ctx->Eax,
                    fault,
                    kCColStorePoolPtr,
                    originalColPool,
                    fallbackSize,
                    reinterpret_cast<uintptr_t>(g_colAccelFakePool));
                LogBytes("col-accel-eip", kCColAccelStartCachePoolRead - 16, 64);
                LogStackModules(ctx->Esp);
            }

            ctx->Eax = reinterpret_cast<DWORD>(g_colAccelFakePool);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    if (g_config.enableGetBoundRectColModelGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(0x00534134) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) == 0x0) {

        CONTEXT* ctx = info->ContextRecord;
        int32_t modelId = -1;
        if (IsReadableCommitted(ctx->Esi + 0x22, sizeof(uint16_t))) {
            modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(ctx->Esi + 0x22));
        }

        static LONG recoverCount = 0;
        const LONG count = InterlockedIncrement(&recoverCount);
        if (count <= 16) {
            uintptr_t returnAddress = 0;
            if (IsReadableCommitted(ctx->Esp, sizeof(returnAddress))) {
                __try {
                    returnAddress = *reinterpret_cast<const uintptr_t*>(ctx->Esp);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    returnAddress = 0;
                }
            }

            Log("bound rect VEH: replaced null CColModel at 0x00534134 model=%u entity=0x%08X fakeCol=0x%08X return=0x%08X esp=0x%08X",
                modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
                ctx->Esi,
                reinterpret_cast<uintptr_t>(&g_fakeColModelForBounds),
                returnAddress,
                ctx->Esp);
            LogStackModules(ctx->Esp);
            if (modelId < kOriginalCModelInfoCount) {
                DumpModelContext(modelId);
            }
        }

        if (IsWritableCommitted(ctx->Ecx + 0x14, sizeof(uintptr_t))) {
            const uint32_t fakeCol = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_fakeColModelForBounds));
            __try {
                *reinterpret_cast<uint32_t*>(ctx->Ecx + 0x14) = fakeCol;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        ctx->Eax = reinterpret_cast<DWORD>(&g_fakeColModelForBounds);
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (g_config.enableGetBoundRectColModelGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(0x0053414E) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) == 0x0C) {

        CONTEXT* ctx = info->ContextRecord;
        if (IsWritableCommitted(ctx->Esp + 0x10, sizeof(float) * 3)) {
            __try {
                *reinterpret_cast<float*>(ctx->Esp + 0x10) = g_fakeColModelForBounds.bboxMinX;
                *reinterpret_cast<float*>(ctx->Esp + 0x14) = g_fakeColModelForBounds.bboxMinY;
                *reinterpret_cast<float*>(ctx->Esp + 0x18) = g_fakeColModelForBounds.bboxMinZ;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        static LONG recoverMaxCount = 0;
        const LONG count = InterlockedIncrement(&recoverMaxCount);
        if (count <= 16) {
            Log("bound rect VEH: recovered bbox max read at 0x0053414E entity=0x%08X fakeCol=0x%08X esp=0x%08X",
                ctx->Esi,
                reinterpret_cast<uintptr_t>(&g_fakeColModelForBounds),
                ctx->Esp);
            LogStackModules(ctx->Esp);
        }

        ctx->Ecx = reinterpret_cast<DWORD>(&g_fakeColModelForBounds) + 0x0C;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (g_config.enableBoundCentreVehRecovery &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(0x0053368B) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) == 0x8) {

        CONTEXT* ctx = info->ContextRecord;
        static LONG recoverCount = 0;
        const LONG count = InterlockedIncrement(&recoverCount);
        if (count <= 6) {
            uintptr_t returnAddress = 0;
            if (IsReadableCommitted(ctx->Esp, sizeof(returnAddress))) {
                __try {
                    returnAddress = *reinterpret_cast<const uintptr_t*>(ctx->Esp);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    returnAddress = 0;
                }
            }

            Log("bound centre VEH: redirected null out at 0x0053368B scratch=0x%08X eax=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi/model=%u return=0x%08X esp=0x%08X",
                reinterpret_cast<uintptr_t>(g_boundCentreScratch),
                ctx->Eax,
                ctx->Ecx,
                ctx->Edx,
                ctx->Esi,
                ctx->Edi,
                returnAddress,
                ctx->Esp);
            LogStackModules(ctx->Esp);
            if (ctx->Edi < kOriginalCModelInfoCount) {
                DumpModelContext(ctx->Edi);
            }
        }

        const bool rwLoaded = ctx->Edi < kOriginalCModelInfoCount && IsModelRwObjectLoaded(ctx->Edi);
        const bool isolatedModel86 = ctx->Edi == 86;
        if (rwLoaded || isolatedModel86) {
            g_boundCentreScratch[0] = 0;
            g_boundCentreScratch[1] = 0;
            g_boundCentreScratch[2] = 0;
            g_boundCentreScratch[3] = 0;
            ctx->Eax = reinterpret_cast<DWORD>(g_boundCentreScratch);
            Log("bound centre VEH: recovered null output pointer with scratch model=%u rwLoaded=%u isolated86=%u",
                ctx->Edi, rwLoaded ? 1 : 0, isolatedModel86 ? 1 : 0);
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        Log("bound centre VEH: not recovering; model=%u is not rwLoaded, avoiding loading soft hang",
            ctx->Edi);
        return EXCEPTION_CONTINUE_SEARCH;
    }
#endif

    if (BreakRepeatedNonExecutableExceptionLoop(info)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    LogCrashClassification(info);

    if (!g_config.enableExceptionDiagnostics || g_config.maxExceptionLogs <= 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_PRIV_INSTRUCTION && code != EXCEPTION_IN_PAGE_ERROR) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const LONG count = InterlockedIncrement(&g_exceptionLogCount);
    if (count > g_config.maxExceptionLogs) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(eip, moduleName, sizeof(moduleName));

    Log("exception: code=0x%08lX address=0x%08X module=%s+0x%X info0=0x%X info1=0x%X",
        code,
        eip,
        moduleName,
        moduleBase ? eip - moduleBase : 0,
        info->ExceptionRecord->NumberParameters > 0 ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[0]) : 0,
        info->ExceptionRecord->NumberParameters > 1 ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) : 0);

    LogMemoryRegion("exception-address", eip);
    if (eip >= 16) {
        LogBytes("exception-eip", eip - 16, 64);
    }
    if (info->ExceptionRecord->NumberParameters > 1) {
        LogMemoryRegion("fault-address", static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]));
    }

#if defined(_M_IX86)
    CONTEXT* ctx = info->ContextRecord;
    if (ctx) {
        Log("context: eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X",
            ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp);
        LogMemoryRegion("eax", ctx->Eax);
        LogMemoryRegion("ecx", ctx->Ecx);
        LogMemoryRegion("edx", ctx->Edx);
        LogMemoryRegion("esi", ctx->Esi);
        LogDwords("eax-dwords", ctx->Eax, 24);
        LogDwords("ecx-dwords", ctx->Ecx, 16);
        LogDwords("edx-dwords", ctx->Edx, 16);
        LogDwords("esi-dwords", ctx->Esi, 24);
        if (ctx->Edi < 20000) {
            DumpModelContext(ctx->Edi);
        }
        LogStackModules(ctx->Esp);
    }
#endif

    return EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI BridgeThread(void*)
{
    Sleep(1500);
    LoadBridgeConfig();
    Log("%s loaded (runtime/API name: FLACompatBridge)", kDisplayName);
    LogBridgeConfig();
    Log("loader: DINPUT8=%p DINPUT8Hooked=%p vorbisFile=%p vorbisHooked=%p",
        GetModuleHandleA("DINPUT8.dll"),
        GetModuleHandleA("DINPUT8Hooked.dll"),
        GetModuleHandleA("vorbisFile.dll"),
        GetModuleHandleA("vorbisHooked.dll"));
    AuditOpenLimitAdjusterSaOverlaps();

    LogIniValue("Apply ID limit patch");
    LogIniValue("FILE_TYPE_DFF");
    LogIniValue("FILE_TYPE_TXD");
    LogIniValue("FILE_TYPE_COL");
    LogIniValue("FILE_TYPE_IPL");
    LogIniValue("FILE_TYPE_IFP");
    LogIniValue("FILE_TYPE_RRR");
    LogIniValue("FILE_TYPE_SCM");
    LogIniValue("Count of killable model IDs");

    LogFLAExports();
    RefreshFlaRuntimeState();
    RefreshRadarTraceRuntimeState();
    if (g_config.enableFlaTrainInitHookRepair) {
        InstallFlaTrainInitHookRepair();
    }
    LogRelocatedAddressDiagnostics();
    LogPoolPointerDiagnostics();
    RuntimeRewriteHardcodedAddressConstants();
    if (g_config.enableRuntimeRewriteRescan) {
        HANDLE rewriteThread = CreateThread(nullptr, 0, RuntimeRewriteRescanThread, nullptr, 0, nullptr);
        if (rewriteThread) {
            CloseHandle(rewriteThread);
        } else {
            Log("runtime rewrite rescan: thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableModuleSnapshot) {
        LogModuleSnapshot();
    }
    if (g_config.enableFlaNoCollisionErrorRestore) {
        RestoreFlaNoCollisionErrorPatch();
    }
    if (g_config.enableGetBoundRectColModelGuard) {
        InstallGetBoundRectColModelGuard();
    }
    if (g_config.enableBridgeCheatStringLoader) {
        LoadBridgeCheatStrings();
    }
    if (g_config.enableFlaObjectInitCollisionRestore) {
        RestoreFlaObjectInitCollisionPatch();
    }
    if (g_config.enablePickupModelLoadGuard) {
        InstallPickupModelLoadGuard();
    }
    if (g_config.enableRadarBlipHandleGuard) {
        InstallRadarBlipHandleGuard();
    }
    if (g_config.enableClosestCarNode03D3Fallback) {
        InstallClosestCarNode03D3Fallback();
    }
    if (g_config.enableTaxi77SetCarCoordinatesGuard) {
        InstallTaxi77SetCarCoordinatesGuard();
    }
    if (g_config.enableSanPabloSpecialActorBridge) {
        InstallSanPabloSpecialActorBridge();
    }
    if (g_config.enableTaxi77StateWatchdog) {
        HANDLE taxi77WatchdogThread = CreateThread(nullptr, 0, Taxi77StateWatchdogThread, nullptr, 0, nullptr);
        if (taxi77WatchdogThread) {
            CloseHandle(taxi77WatchdogThread);
        } else {
            Log("taxi77 watchdog: thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableCObjectCreateBridge) {
        InstallCObjectCreateBridgeStubs();
    }
    if (g_config.enableCleoObjectCreateInlineRestore) {
        RestoreCleoObjectCreateInlinePatch();
    }
    if (g_config.enableCleoDispatchGuard) {
        InstallCleoDispatchNullGuard();
    }
    if (g_config.enableCleoThunk26720Guard) {
        InstallCleoThunk26720NullGuard();
    }
    if (g_config.enableCleoPlusPoolAllocateGuard) {
        InstallCleoPlusPoolAllocateGuard();
    }
    if (g_config.enableMixSetsPoolAllocateGuard) {
        InstallMixSetsPoolAllocateGuard();
    }
    if (g_config.enableUrbanizePoolAllocateGuard) {
        InstallUrbanizePoolAllocateGuard();
    }
    if (g_config.enableAutoPoolAllocateGuard) {
        InstallAutoPoolAllocateGuards();
    }
    if (g_config.enableDeferredPoolAllocateReplay) {
        HANDLE replayThread = CreateThread(nullptr, 0, DeferredPoolAllocateReplayThread, nullptr, 0, nullptr);
        if (replayThread) {
            CloseHandle(replayThread);
        } else {
            Log("auto pool guard: deferred replay thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableCleoPlusExtendedObjectVarGuard) {
        InstallCleoPlusExtendedObjectVarGuard();
    }
    if (g_config.enableAnimUncompressGuard) {
        InstallAnimUncompressNullGuard();
    }
    if (g_config.enableAnimStaticAssocGuard) {
        InstallAnimStaticAssocInitGuard();
    }
    if (g_config.enableAnimFrameUpdateGuard) {
        InstallAnimFrameUpdateSkinnedVelocityGuard();
    }
    if (g_config.enableGetBoundCentreInlineGuard) {
        InstallGetBoundCentreNullGuard();
    }

    if (g_config.enableLegacyModelInfoShadow || g_config.enableLegacyStreamingInfoShadow) {
        HANDLE shadowThread = CreateThread(nullptr, 0, LegacyShadowThread, nullptr, 0, nullptr);
        if (shadowThread) {
            CloseHandle(shadowThread);
        }
    }
    if (g_config.enableUrbanizeProblemPedPreload) {
        HANDLE preloadThread = CreateThread(nullptr, 0, PreloadUrbanizePedModelsThread, nullptr, 0, nullptr);
        if (preloadThread) {
            CloseHandle(preloadThread);
        }
    }
    if (g_config.enableSanPabloSpecialActorBridge &&
        (g_config.specialActorAutoScanImgArchives || g_config.specialActorAutoScanModloader)) {
        HANDLE catalogThread = CreateThread(nullptr, 0, SpecialActorRuntimeCatalogThread, nullptr, 0, nullptr);
        if (catalogThread) {
            CloseHandle(catalogThread);
        } else {
            Log("special actor catalog: thread creation failed gle=%lu", GetLastError());
        }
    }
    return 0;
}
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetApiVersion()
{
    return 6;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetFileIdCapacity()
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }
    return g_fileIdCapacity;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetCompatFlags()
{
    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    return g_flaCompatFlags;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetRuntimeSource()
{
    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    return g_runtimeStateSource;
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetRelocatedAddress(uint32_t id)
{
    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    if (!g_radarTraceBase) {
        RefreshRadarTraceRuntimeState();
    }

    switch (id) {
    case 1:
        return g_relocatedCModelInfoPtrs;
    case 2:
        return g_relocatedStreamingInfo;
    case 3:
        return g_relocatedAnimBlocks;
    case 4:
        return g_relocatedVehicleRecordingStreamingArray;
    case 5:
        return g_relocatedStreamedScripts;
    case 6:
        return g_relocatedHandlingManager;
    case 7:
        return g_radarTraceBase;
    case 8:
        return g_relocatedStreamingInfoExtension;
    case 9:
        return g_relocatedRegisteredKills;
    default:
        return 0;
    }
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetRelocatedAddressByName(const char* name, uintptr_t* out)
{
    if (!name || !out || !IsWritableCommitted(reinterpret_cast<uintptr_t>(out), sizeof(*out))) {
        return 0;
    }

    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    if (!g_radarTraceBase) {
        RefreshRadarTraceRuntimeState();
    }

    uintptr_t value = 0;
    if (_stricmp(name, "CModelInfo::ms_modelInfoPtrs") == 0 || _stricmp(name, "CModelInfo") == 0) {
        value = g_relocatedCModelInfoPtrs;
    } else if (_stricmp(name, "CStreaming::ms_aInfoForModel") == 0 || _stricmp(name, "CStreaming") == 0) {
        value = g_relocatedStreamingInfo;
    } else if (_stricmp(name, "CStreaming::ms_aInfoForModelExtension") == 0 || _stricmp(name, "CStreamingExtension") == 0) {
        value = g_relocatedStreamingInfoExtension;
    } else if (_stricmp(name, "CAnimManager::ms_aAnimBlocks") == 0 || _stricmp(name, "AnimBlocks") == 0) {
        value = g_relocatedAnimBlocks;
    } else if (_stricmp(name, "CVehicleRecording::StreamingArray") == 0 || _stricmp(name, "VehicleRecording") == 0) {
        value = g_relocatedVehicleRecordingStreamingArray;
    } else if (_stricmp(name, "CTheScripts::StreamedScripts") == 0 || _stricmp(name, "StreamedScripts") == 0) {
        value = g_relocatedStreamedScripts;
    } else if (_stricmp(name, "mod_HandlingManager") == 0 || _stricmp(name, "HandlingManager") == 0) {
        value = g_relocatedHandlingManager;
    } else if (_stricmp(name, "CDarkel::RegisteredKills") == 0 || _stricmp(name, "RegisteredKills") == 0) {
        value = g_relocatedRegisteredKills;
    } else if (_stricmp(name, "CRadar::ms_RadarTrace") == 0 || _stricmp(name, "RadarTrace") == 0) {
        value = g_radarTraceBase;
    }

    *out = value;
    return value ? 1 : 0;
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetModelInfoEntry(uint32_t modelId)
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }
    return SafeModelInfoEntryAddress(modelId);
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetModelInfo(uint32_t modelId)
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }

    const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
    uint32_t modelInfo = 0;
    return entry && SafeReadU32(entry, &modelInfo) ? modelInfo : 0;
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetStreamingInfo(uint32_t modelId)
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }
    return SafeStreamingInfoEntryAddress(modelId);
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_ReadStreamingInfo(uint32_t modelId, void* out, uint32_t outSize)
{
    if (!out || outSize < kStreamingInfoSize ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(out), kStreamingInfoSize)) {
        return 0;
    }

    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }

    const uintptr_t entry = SafeStreamingInfoEntryAddress(modelId);
    if (!entry) {
        return 0;
    }

    __try {
        std::memcpy(out, reinterpret_cast<const void*>(entry), kStreamingInfoSize);
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetPoolInfo(uint32_t poolId, uintptr_t* poolPtr, uint32_t* capacity)
{
    if (!poolPtr || !capacity ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(poolPtr), sizeof(*poolPtr)) ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(capacity), sizeof(*capacity))) {
        return 0;
    }

    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }

    uintptr_t ptrAddress = 0;
    uint32_t cap = 0;
    switch (poolId) {
    case 1:
        ptrAddress = kOriginalPedPoolPtr;
        cap = g_pedPoolCapacity;
        break;
    case 2:
        ptrAddress = kOriginalVehiclePoolPtr;
        cap = g_vehiclePoolCapacity;
        break;
    case 3:
        ptrAddress = kOriginalObjectPoolPtr;
        cap = g_objectPoolCapacity;
        break;
    case 4:
        ptrAddress = kOriginalBuildingPoolPtr;
        cap = g_buildingPoolCapacity;
        break;
    case 5:
        ptrAddress = kOriginalDummyPoolPtr;
        cap = g_dummyPoolCapacity;
        break;
    case 6:
        ptrAddress = kOriginalColModelPoolPtr;
        cap = g_colModelPoolCapacity;
        break;
    case 7:
        ptrAddress = kCColStorePoolPtr;
        cap = g_collisionStoreCapacity;
        break;
    default:
        return 0;
    }

    uint32_t ptr = 0;
    if (!SafeReadU32(ptrAddress, &ptr) || !ptr) {
        return 0;
    }

    *poolPtr = ptr;
    *capacity = cap;
    return 1;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetPoolInfoByName(const char* name, uintptr_t* poolPtr, uint32_t* capacity)
{
    if (!name || !poolPtr || !capacity ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(poolPtr), sizeof(*poolPtr)) ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(capacity), sizeof(*capacity))) {
        return 0;
    }

    char localName[96]{};
    __try {
        strncpy_s(localName, name, _TRUNCATE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    uint32_t poolId = 0;
    if (_stricmp(localName, "Ped") == 0 || _stricmp(localName, "Peds") == 0 || _stricmp(localName, "CPools::ms_pPedPool") == 0) {
        poolId = 1;
    } else if (_stricmp(localName, "Vehicle") == 0 || _stricmp(localName, "Vehicles") == 0 || _stricmp(localName, "CPools::ms_pVehiclePool") == 0) {
        poolId = 2;
    } else if (_stricmp(localName, "Object") == 0 || _stricmp(localName, "Objects") == 0 || _stricmp(localName, "CPools::ms_pObjectPool") == 0) {
        poolId = 3;
    } else if (_stricmp(localName, "Building") == 0 || _stricmp(localName, "Buildings") == 0 || _stricmp(localName, "CPools::ms_pBuildingPool") == 0) {
        poolId = 4;
    } else if (_stricmp(localName, "Dummy") == 0 || _stricmp(localName, "Dummies") == 0 || _stricmp(localName, "CPools::ms_pDummyPool") == 0) {
        poolId = 5;
    } else if (_stricmp(localName, "ColModel") == 0 || _stricmp(localName, "ColModels") == 0 || _stricmp(localName, "CPools::ms_pColModelPool") == 0) {
        poolId = 6;
    } else if (_stricmp(localName, "ColStore") == 0 || _stricmp(localName, "Collision") == 0 || _stricmp(localName, "CColStore::ms_pColPool") == 0) {
        poolId = 7;
    }

    return poolId ? FLACompatBridge_GetPoolInfo(poolId, poolPtr, capacity) : 0;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_RebuildSpecialActorCatalog()
{
    LoadBridgeConfig();
    BuildSpecialActorRuntimeCatalog();
    return g_runtimeSpecialActorNameCount;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetSpecialActorName(uint32_t actorCode, char* out, uint32_t outSize)
{
    if (!out || outSize == 0 || !IsWritableCommitted(reinterpret_cast<uintptr_t>(out), outSize)) {
        return 0;
    }

    if (!g_runtimeSpecialActorNameCount) {
        BuildSpecialActorRuntimeCatalog();
    }

    out[0] = '\0';
    return ReadConfiguredSpecialActorName(0, actorCode, out, outSize) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_RequestSpecialActor(uint32_t modelId, const char* name)
{
    if (!name || !IsReadableCommitted(reinterpret_cast<uintptr_t>(name), 1)) {
        return 0;
    }

    char raw[64]{};
    __try {
        strncpy_s(raw, name, _TRUNCATE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    char normalized[64]{};
    if (!NormalizeSpecialActorName(raw, normalized, sizeof(normalized))) {
        return 0;
    }

    return RequestSpecialActorName(modelId, normalized) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_RequestSpecialActorByCode(uint32_t modelId, uint32_t actorCode)
{
    if (!g_runtimeSpecialActorNameCount) {
        BuildSpecialActorRuntimeCatalog();
    }

    return RequestConfiguredSpecialActor(modelId, actorCode) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_ReleaseSpecialActor(uint32_t modelId)
{
    return ReleaseConfiguredSpecialActor(modelId) ? 1 : 0;
}

extern "C" __declspec(dllexport) void FLACompatBridge_KeepExport()
{
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        InitializeCriticalSection(&g_deferredPoolAllocateLock);
        LoadBridgeConfig();
        if (g_config.enableVectoredExceptionHandler) {
            AddVectoredExceptionHandler(1, BridgeVectoredExceptionHandler);
        }
        HANDLE thread = CreateThread(nullptr, 0, BridgeThread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        DeleteCriticalSection(&g_deferredPoolAllocateLock);
    }
    return TRUE;
}
