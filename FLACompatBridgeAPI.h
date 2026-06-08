#pragma once

#include <stdint.h>

// Optional public API exported by scripts\FLACompatBridge.asi.
// Load with GetModuleHandleA("FLACompatBridge.asi") and GetProcAddress.

enum FLACompatBridgeRelocatedAddress : uint32_t {
    FLA_BRIDGE_ADDR_CMODELINFO_PTRS = 1,
    FLA_BRIDGE_ADDR_CSTREAMING_INFO = 2,
    FLA_BRIDGE_ADDR_ANIM_BLOCKS = 3,
    FLA_BRIDGE_ADDR_VEHICLE_RECORDING_STREAMING_ARRAY = 4,
    FLA_BRIDGE_ADDR_STREAMED_SCRIPTS = 5,
    FLA_BRIDGE_ADDR_HANDLING_MANAGER = 6,
    FLA_BRIDGE_ADDR_RADAR_TRACE = 7,
};

enum FLACompatBridgePoolId : uint32_t {
    FLA_BRIDGE_POOL_PEDS = 1,
    FLA_BRIDGE_POOL_VEHICLES = 2,
    FLA_BRIDGE_POOL_OBJECTS = 3,
};

enum FLACompatBridgeRuntimeSource : uint32_t {
    FLA_BRIDGE_RUNTIME_SOURCE_UNKNOWN = 0,
    FLA_BRIDGE_RUNTIME_SOURCE_FLA_ABI = 1,
    FLA_BRIDGE_RUNTIME_SOURCE_FLA_LOG = 2,
    FLA_BRIDGE_RUNTIME_SOURCE_INI_OR_VANILLA = 3,
};

enum FLACompatBridgeCompatFlags : uint32_t {
    FLA_BRIDGE_COMPAT_FLAG_ID_LIMITS_KNOWN = 1 << 0,
    FLA_BRIDGE_COMPAT_FLAG_ID_LIMIT_PATCH_ENABLED = 1 << 1,
    FLA_BRIDGE_COMPAT_FLAG_ANY_ID_LIMIT_INCREASED = 1 << 2,
    FLA_BRIDGE_COMPAT_FLAG_ID_UNSIGNED = 1 << 3,
    FLA_BRIDGE_COMPAT_FLAG_ID_BASE_INT32 = 1 << 4,
    FLA_BRIDGE_COMPAT_FLAG_DIFFICULT_FILE_TYPES_INT32 = 1 << 5,
    FLA_BRIDGE_COMPAT_FLAG_COL_ID_SIZE_INCREASED = 1 << 6,
    FLA_BRIDGE_COMPAT_FLAG_IPL_ID_SIZE_INCREASED = 1 << 7,
    FLA_BRIDGE_COMPAT_FLAG_MODEL_INFO_PTRS_MOVED = 1 << 8,
    FLA_BRIDGE_COMPAT_FLAG_STREAMING_INFO_MOVED = 1 << 9,
    FLA_BRIDGE_COMPAT_FLAG_ANIM_BLOCKS_MOVED = 1 << 10,
    FLA_BRIDGE_COMPAT_FLAG_REGISTERED_KILLS_MOVED = 1 << 11,
};

extern "C" {
uint32_t __stdcall FLACompatBridge_GetApiVersion();
uint32_t __stdcall FLACompatBridge_GetFileIdCapacity();
uint32_t __stdcall FLACompatBridge_GetCompatFlags();
uint32_t __stdcall FLACompatBridge_GetRuntimeSource();
uintptr_t __stdcall FLACompatBridge_GetRelocatedAddress(uint32_t id);
int __stdcall FLACompatBridge_GetRelocatedAddressByName(const char* name, uintptr_t* out);
uintptr_t __stdcall FLACompatBridge_GetModelInfoEntry(uint32_t modelId);
uintptr_t __stdcall FLACompatBridge_GetModelInfo(uint32_t modelId);
uintptr_t __stdcall FLACompatBridge_GetStreamingInfo(uint32_t modelId);
int __stdcall FLACompatBridge_ReadStreamingInfo(uint32_t modelId, void* out, uint32_t outSize);
int __stdcall FLACompatBridge_GetPoolInfo(uint32_t poolId, uintptr_t* poolPtr, uint32_t* capacity);
}
