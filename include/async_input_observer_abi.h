#ifndef ADOFAI_ASYNC_INPUT_OBSERVER_ABI_H
#define ADOFAI_ASYNC_INPUT_OBSERVER_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADOFAI_ASYNC_RAW_OBSERVER_ABI_V1 1u

typedef struct AdoAsyncRawTouchEventV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    uint64_t raw_ns;
    uint64_t producer_epoch;
    int32_t action;
    int32_t pointer_id;
    int32_t pointer_count;
    int32_t source;
    int32_t device_id;
    int32_t viewport_width;
    int32_t viewport_height;
    float x;
    float y;
    uint32_t android_flags;
} AdoAsyncRawTouchEventV1;

typedef struct AdoAsyncRawKeyEventV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    uint64_t raw_ns;
    uint64_t producer_epoch;
    int32_t action;
    int32_t key_code;
    int32_t scan_code;
    int32_t meta_state;
    int32_t device_id;
    int32_t repeat_count;
    int32_t source;
    int32_t android_flags;
    uint32_t reserved;
} AdoAsyncRawKeyEventV1;

typedef void (*AdoAsyncEnabledChangedV1)(
    void *user_data,
    int32_t enabled,
    uint64_t producer_epoch);

typedef void (*AdoAsyncRawTouchObserverV1)(
    void *user_data,
    const AdoAsyncRawTouchEventV1 *event);

typedef void (*AdoAsyncRawKeyObserverV1)(
    void *user_data,
    const AdoAsyncRawKeyEventV1 *event);

typedef struct AdoAsyncRawObserverV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    void *user_data;
    AdoAsyncEnabledChangedV1 on_enabled_changed;
    AdoAsyncRawTouchObserverV1 on_touch;
    AdoAsyncRawKeyObserverV1 on_key;
} AdoAsyncRawObserverV1;

typedef int (*ADOFAIAsyncInputRegisterRawObserverV1Fn)(
    const AdoAsyncRawObserverV1 *observer);

int ADOFAIAsyncInput_RegisterRawObserverV1(
    const AdoAsyncRawObserverV1 *observer);

#ifdef __cplusplus
}

static_assert(sizeof(AdoAsyncRawTouchEventV1) == 64);
static_assert(sizeof(AdoAsyncRawKeyEventV1) == 64);
#else
_Static_assert(sizeof(AdoAsyncRawTouchEventV1) == 64, "touch observer ABI changed");
_Static_assert(sizeof(AdoAsyncRawKeyEventV1) == 64, "key observer ABI changed");
#endif

#endif
