#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <errno.h>
#include <elf.h>
#include <inttypes.h>
#include <link.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dobby.h"

#define LOG_TAG "ADOFAI_ASYNC_INPUT"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define PACKAGE_NAME "com.fizzd.connectedworlds.leveleditor.debug"
#define CFG_PATH "/data/data/" PACKAGE_NAME "/files/adofai_async_input.cfg"
#define AUTO_REPLAY_CFG_PATH "/data/data/" PACKAGE_NAME "/files/adofai_async_auto_replay.cfg"
#define TRACE_CFG_PATH "/data/data/" PACKAGE_NAME "/files/adofai_async_trace.cfg"

#define OFFSET_SCRCONTROLLER_CURRENT_STATE 0xD8u
#define OFFSET_SCRCONTROLLER_PAUSED 0x1E8u
#define OFFSET_SCRCONTROLLER_GAMEWORLD 0x78u
#define OFFSET_STATEBEHAVIOUR_STATE_MACHINE 0x20u
#define OFFSET_STATEENGINE_CURRENT_STATE 0x28u
#define OFFSET_STATEENGINE_DESTINATION_STATE 0x30u
#define OFFSET_STATEMAPPING_STATE 0x10u
#define OFFSET_BOXED_ENUM_VALUE 0x10u
#define OFFSET_RDINPUTTYPE_IS_ACTIVE 0x14u
#define OFFSET_SCNEDITOR_PAUSED_IN_PLAY_MODE 0x9BDu
#define OFFSET_SCNEDITOR_IN_STRICTLY_EDITING_MODE 0xA14u
#define IL2CPP_IMAGE_ASSEMBLY_CSHARP "Assembly-CSharp.dll"
#define IL2CPP_IMAGE_AUDIO_MODULE "UnityEngine.AudioModule.dll"
#define IL2CPP_IMAGE_CORE_MODULE "UnityEngine.CoreModule.dll"
#define STATE_COUNTDOWN 2
#define STATE_CHECKPOINT 3
#define STATE_PLAYER_CONTROL 4
#define ASYNC_KEY_RAW_NONE 0xffffu
#define ASYNC_KEY_RAW_BASE 0xff00u
#define ASYNC_KEY_LABEL_SPACE 81
#define ASYNC_KEY_SLOT_COUNT 16

#define ACTION_DOWN 0
#define ACTION_UP 1
#define ACTION_MOVE 2
#define ACTION_CANCEL 3
#define ACTION_POINTER_DOWN 5
#define ACTION_POINTER_UP 6
#define ACTION_MASK 0xff

#define KEY_ACTION_DOWN 0
#define KEY_ACTION_UP 1

#define SOURCE_TOUCHSCREEN 0x00001002
#define SOURCE_MOUSE 0x00002002
#define SOURCE_KEYBOARD 0x00000101

#define KEYCODE_BACK 4
#define KEYCODE_ALT_LEFT 57
#define KEYCODE_ALT_RIGHT 58
#define KEYCODE_ESCAPE 111
#define KEYCODE_SYSRQ 120
#define KEYCODE_META_LEFT 117
#define KEYCODE_META_RIGHT 118
#define KEYCODE_F12 142

#define BUTTON_WENT_DOWN 0
#define BUTTON_WENT_UP 1
#define BUTTON_IS_DOWN 2
#define BUTTON_IS_UP 3

#define MAX_EVENTS 256
#define MAX_INGRESS_RECORDS 512
#define MAX_HELD_SOURCES 64
#define CAPTURE_START_SUPPRESS_NS 250000000ULL
#define MULTITAP_BURST_NS 8000000ULL
#define CAPTURE_STALE_TICKS 10000000ULL
#define REPLAY_TICK_STEP_TICKS 10000ULL
#define REPLAY_MAX_STEPS_PER_FRAME MAX_EVENTS
#define IL2CPP_BASE_WAIT_ATTEMPTS 120
#define IL2CPP_BASE_POLL_US (500 * 1000)
#define IL2CPP_METADATA_POLL_US (500 * 1000)
#define EVENT_DOWN 1
#define EVENT_UP 2
#define INGRESS_EVENT 1
#define INGRESS_RESET 2
#define INGRESS_SOFT_PAUSE 3
#define INGRESS_SOFT_RESUME 4
#define RESET_BARRIER_TIMEOUT_MS 5
#define REPLAY_MODE_NONE 0
#define REPLAY_MODE_LEGACY 1
#define REPLAY_MODE_MASK 2
#define SYNTHETIC_AUTO_SOURCE_ID 0x4155544f
#define SYNTHETIC_AUTO_UP_DELAY_NS 100000ULL
#define SYNTHETIC_AUTO_NEXT_DOWN_GAP_NS 100000ULL

typedef enum AdoOfficialHitMargin {
    ADO_HIT_TOO_EARLY = 0,
    ADO_HIT_VERY_EARLY = 1,
    ADO_HIT_EARLY_PERFECT = 2,
    ADO_HIT_PERFECT = 3,
    ADO_HIT_LATE_PERFECT = 4,
    ADO_HIT_VERY_LATE = 5,
    ADO_HIT_TOO_LATE = 6,
    ADO_HIT_MULTIPRESS = 7,
    ADO_HIT_FAIL_MISS = 8,
    ADO_HIT_FAIL_OVERLOAD = 9,
    ADO_HIT_AUTO = 10,
    ADO_HIT_OVERPRESS = 11
} AdoOfficialHitMargin;

typedef enum AdoOfficialHitMarginLimit {
    ADO_MARGIN_LIMIT_NONE = 0,
    ADO_MARGIN_LIMIT_PERFECTS_ONLY = 1,
    ADO_MARGIN_LIMIT_PURE_PERFECT_ONLY = 2
} AdoOfficialHitMarginLimit;

typedef enum AdoOfficialDifficulty {
    ADO_DIFFICULTY_LENIENT = 0,
    ADO_DIFFICULTY_NORMAL = 1,
    ADO_DIFFICULTY_STRICT = 2
} AdoOfficialDifficulty;

typedef struct AdoOfficialConfig {
    int is_mobile;
    int use_old_auto;
    int use_song_time;
    int debug;
    int stationary;
    int multitap_hit_once;
    int quantize_hitmargin_float;
    int drum_controller;
    AdoOfficialDifficulty difficulty;
    AdoOfficialHitMarginLimit hit_margin_limit;
    double current_speed_trial;
    double hitmargin_counted;
} AdoOfficialConfig;

typedef struct AdoOfficialConductor {
    int use_song_time;
    double bpm;
    double pitch;
    double crotchet_at_start;
    double dsp_time_song;
    double calibration_i;
    double add_offset;
    double song_time;
} AdoOfficialConductor;

typedef struct AdoOfficialPlayer {
    int alive;
    int responsive;
    int is_reunifying;
    int auto_player;
    int invincible_auto;
    int holding;
    int hold_key_count;
    int midspin_infinite_margin;
    int taps_on_this_floor;
    int consec_multipress_counter;
    int key_limiter_over_counter;
    int ignore_input_count;
    int miss_cooldown_active;
    int pending_key_times;
    int pending_midspin_key_times;
    int down_key_duration_count;
    int key_total;
    int last_hit_frame_down;
    int last_hit_frame_up;
    uint64_t hold_keys_mask;
    uint64_t down_keys_duration_mask;
    double down_key_duration_time[64];
    double failbar_overload_counter;
    double failbar_multipress_counter;
    double failbar_multipress_reset_counter;
    double miss_cooldown_remaining;
    double lock_input;
    double last_hit;
    double actual_last_hit;
} AdoOfficialPlayer;

typedef struct AdoOfficialPlanet {
    void *currfloor;
    double snapped_last_angle;
    double target_exit_angle;
    double angle;
    double cached_angle;
    double speed;
    int is_cw;
} AdoOfficialPlanet;

typedef struct AdoOfficialJudgementOnly {
    AdoOfficialHitMargin margin;
    double signed_angle_delta_deg;
    double signed_time_offset_ms;
} AdoOfficialJudgementOnly;

void ado_official_config_defaults(AdoOfficialConfig *config);
AdoOfficialJudgementOnly ado_official_judge_event_tick_only(
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    const AdoOfficialPlayer *player,
    const AdoOfficialPlanet *planet,
    uint64_t event_tick,
    double margin_scale);
AdoOfficialJudgementOnly ado_official_judge_angle_only(
    const AdoOfficialConfig *config,
    double hit_angle,
    double ref_angle,
    int is_cw,
    double bpm_times_speed,
    double conductor_pitch,
    double margin_scale);

typedef int (*GetIsActiveFn)(void *method);
typedef void (*SimulatedUpdateFn)(void *self, uint64_t has_value, uint64_t tick, void *method);
typedef void (*UpdateOffsetTimeFn)(int64_t fix_divider, void *method);
typedef void (*ProcessKeyInputsFn)(void *self, uint64_t event_tick, void *method);
typedef int (*BoolSelfFn)(void *self, void *method);
typedef int (*IntSelfFn)(void *self, void *method);
typedef int (*IntStaticFn)(void *method);
typedef float (*FloatSelfFn)(void *self, void *method);
typedef float (*FloatStaticFn)(void *method);
typedef double (*DoubleSelfFn)(void *self, void *method);
typedef int (*RDInputGetMainFn)(int state, void *method);
typedef int (*GetHitMarginFn)(float hitangle, float refangle, int is_cw, float bpm_times_speed, float conductor_pitch, double margin_scale, void *method);
typedef int (*HitSelfBoolFn)(void *self, int is_auto, void *method);
typedef void (*VoidSelfFn)(void *self, void *method);
typedef void *(*GetObjectSelfFn)(void *self, void *method);
typedef void *(*SwitchChosenFn)(void *self, void *method);
typedef void (*AdjustAngleFn)(void *player, uint64_t target_tick, void *method);
typedef void (*CameraUpdateFollowCamFn)(void *self, int force, void *method);
typedef void (*SetPausedFn)(void *self, int value, void *method);
typedef void (*SetBoolStaticFn)(int value, void *method);
typedef double (*GetDoubleStaticFn)(void *method);
typedef void *(*GetObjectStaticFn)(void *method);
typedef const void *(*InputEventFromJavaFn)(JNIEnv *env, jobject event);
typedef int64_t (*InputEventGetTimeFn)(const void *event);
typedef void (*InputEventReleaseFn)(const void *event);
typedef int32_t (*InputEventGetSourceFn)(const void *event);
typedef int32_t (*MotionEventGetActionFn)(const void *event);
typedef size_t (*MotionEventGetPointerCountFn)(const void *event);
typedef int32_t (*MotionEventGetPointerIdFn)(const void *event, size_t pointer_index);
typedef float (*MotionEventGetCoordFn)(const void *event, size_t pointer_index);
typedef int32_t (*KeyEventGetIntFn)(const void *event);
typedef int (*Il2CppInitFn)(const char *domain_name);
typedef int (*Il2CppInitUtf16Fn)(const uint16_t *domain_name);
typedef void *(*Il2CppGetCorlibFn)(void);
typedef void *(*Il2CppDomainGetFn)(void);
typedef void *(*Il2CppThreadAttachFn)(void *domain);
typedef const void **(*Il2CppDomainGetAssembliesFn)(void *domain, size_t *size);
typedef void *(*Il2CppAssemblyGetImageFn)(const void *assembly);
typedef const char *(*Il2CppImageGetNameFn)(const void *image);
typedef void *(*Il2CppClassFromNameFn)(const void *image, const char *namespaze, const char *name);
typedef const void *(*Il2CppClassGetMethodFromNameFn)(void *klass, const char *name, int args_count);
typedef void *(*Il2CppClassGetFieldFromNameFn)(void *klass, const char *name);
typedef size_t (*Il2CppFieldGetOffsetFn)(void *field);
typedef void (*Il2CppFieldStaticGetValueFn)(void *field, void *value);
typedef void (*Il2CppFieldStaticSetValueFn)(void *field, const void *value);
typedef void *(*Il2CppObjectGetClassFn)(void *object);
typedef void (*Il2CppRuntimeClassInitFn)(void *klass);
typedef void *(*Il2CppRuntimeInvokeFn)(void *method, void *object, void **params, void **exc);
typedef const uint16_t *(*Il2CppStringCharsFn)(void *str);
typedef int32_t (*Il2CppStringLengthFn)(void *str);

typedef struct AsyncKeyCodeValue {
    uint16_t key;
    uint16_t padding;
    int32_t label;
} AsyncKeyCodeValue;

typedef struct AsyncEvent {
    uint64_t tick;
    int type;
    int source_id;
    int synthetic_auto;
} AsyncEvent;

typedef struct Il2CppApi {
    void *handle;
    Il2CppGetCorlibFn get_corlib;
    Il2CppDomainGetFn domain_get;
    Il2CppThreadAttachFn thread_attach;
    Il2CppDomainGetAssembliesFn domain_get_assemblies;
    Il2CppAssemblyGetImageFn assembly_get_image;
    Il2CppImageGetNameFn image_get_name;
    Il2CppClassFromNameFn class_from_name;
    Il2CppClassGetMethodFromNameFn class_get_method_from_name;
    Il2CppClassGetFieldFromNameFn class_get_field_from_name;
    Il2CppFieldGetOffsetFn field_get_offset;
    Il2CppFieldStaticGetValueFn field_static_get_value;
    Il2CppFieldStaticSetValueFn field_static_set_value;
    Il2CppObjectGetClassFn object_get_class;
    Il2CppRuntimeClassInitFn runtime_class_init;
    Il2CppRuntimeInvokeFn runtime_invoke;
    Il2CppStringCharsFn string_chars;
    Il2CppStringLengthFn string_length;
    void *assembly_csharp_image;
    void *audio_module_image;
    void *core_module_image;
    int ready;
} Il2CppApi;

typedef struct Il2CppMethodCache {
    void *method_info;
    uintptr_t method_pointer;
} Il2CppMethodCache;

typedef struct MethodHookSpec {
    const char *namespaze;
    const char *klass;
    const char *method;
    int args_count;
    void *replacement;
    void **original_out;
    const char *label;
} MethodHookSpec;

typedef struct MethodInfoHead {
    uintptr_t method_pointer;
} MethodInfoHead;

typedef struct MotionEventSnapshot {
    int action;
    int index;
    int pointer_id;
    int source;
    int has_xy;
    float x;
    float y;
    uint64_t raw_ns;
} MotionEventSnapshot;

typedef struct KeyEventSnapshot {
    int action;
    int key_code;
    int meta_state;
    int repeat_count;
    uint64_t raw_ns;
} KeyEventSnapshot;

typedef struct IngressRecord {
    int kind;
    int event_type;
    int source_id;
    uint64_t raw_ns;
    uint64_t seq;
} IngressRecord;

static Il2CppApi g_il2cpp_api;
static volatile int g_enabled = 0;
static volatile int g_auto_replay_enabled = 1;
static volatile int g_trace_enabled = 0;
static volatile int g_hooks_installed = 0;
static volatile int g_in_async_replay = 0;
static volatile int g_current_replay_is_synthetic_auto = 0;
static volatile int g_in_auto_state_machine_replay = 0;
static volatile int g_in_conductor_frame __attribute__((unused)) = 0;
static volatile int g_in_playercontrol_original = 0;
static volatile int g_in_playercontrol_frame = 0;
static volatile int g_force_async_active_for_original = 0;
static volatile int g_forced_hit_margin_active = 0;
static volatile int g_forced_hit_margin_value = 3;
static volatile uint64_t g_current_replay_trace_id = 0;
static volatile uint64_t g_current_replay_tick = 0;
static volatile int g_current_replay_down = 0;
static volatile int g_current_replay_up = 0;
static volatile int g_current_replay_held = 0;
static volatile int g_capture_ready = 0;
static uintptr_t g_offset_scrcontroller_current_state = OFFSET_SCRCONTROLLER_CURRENT_STATE;
static uintptr_t g_offset_scrcontroller_paused = OFFSET_SCRCONTROLLER_PAUSED;
static uintptr_t g_offset_scrcontroller_gameworld = OFFSET_SCRCONTROLLER_GAMEWORLD;
static uintptr_t g_offset_statebehaviour_state_machine = OFFSET_STATEBEHAVIOUR_STATE_MACHINE;
static uintptr_t g_offset_stateengine_current_state = OFFSET_STATEENGINE_CURRENT_STATE;
static uintptr_t g_offset_stateengine_destination_state = OFFSET_STATEENGINE_DESTINATION_STATE;
static uintptr_t g_offset_statemapping_state = OFFSET_STATEMAPPING_STATE;
static uintptr_t g_offset_boxed_enum_value = OFFSET_BOXED_ENUM_VALUE;
static uintptr_t g_offset_rdinputtype_is_active = OFFSET_RDINPUTTYPE_IS_ACTIVE;
static uintptr_t g_offset_scneditor_paused_in_play_mode = OFFSET_SCNEDITOR_PAUSED_IN_PLAY_MODE;
static uintptr_t g_offset_scneditor_in_strictly_editing_mode = OFFSET_SCNEDITOR_IN_STRICTLY_EDITING_MODE;
static void *g_current_controller_self = NULL;
static pthread_mutex_t g_scene_state_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_scene_state_ready = 0;
static int g_cached_editor_present = 0;
static int g_cached_editor_play_mode = 0;
static int g_cached_is_scn_game = 0;
static int g_cached_is_level_editor = 0;
static int g_cached_is_dlc_level = 0;
static int g_cached_gameworld = 0;
static int g_cached_paused_in_play_mode = 0;
static int g_cached_strict_edit = 0;
static int g_cached_controller_state = -1;
static int g_cached_capture = 0;
static int g_cached_async_blocks = 0;
static int g_last_session_boundary_state = -1;
static char g_cached_scene_name[128] = {0};
static uint64_t g_capture_floor_raw_ns = 0;
static void *g_field_async_curr_frame_tick = NULL;
static void *g_field_async_prev_frame_tick = NULL;
static void *g_field_async_target_song_tick = NULL;
static void *g_field_async_offset_tick = NULL;
static void *g_field_async_offset_tick_updated = NULL;
static void *g_field_async_last_reported_target_tick = NULL;
static void *g_field_async_key_mask = NULL;
static void *g_field_async_key_down_mask = NULL;
static void *g_field_async_key_up_mask = NULL;
static void *g_field_async_frame_key_mask = NULL;
static void *g_field_async_frame_key_down_mask = NULL;
static void *g_field_async_frame_key_up_mask = NULL;
static void *g_field_scrplayer_should_replace_camy_to_pos = NULL;
static void *g_field_rdinput_keyboard_input = NULL;
static void *g_field_rdinput_keyboard_left = NULL;
static void *g_field_rdinput_keyboard_right = NULL;
static void *g_field_rdinput_async_keyboard = NULL;
static void *g_field_rdinput_async_keyboard_left = NULL;
static void *g_field_rdinput_async_keyboard_right = NULL;
static void *g_field_gcs_current_speed_trial = NULL;
static void *g_field_gcs_hitmargin_counted = NULL;
static void *g_field_gcs_difficulty = NULL;
static void *g_field_gcs_hit_margin_limit = NULL;
static uintptr_t g_offset_scrplanet_player = 0;
static uintptr_t g_offset_scrplanet_currfloor = 0;
static uintptr_t g_offset_scrplanet_planetary_system = 0;
static uintptr_t g_offset_scrplanet_angle = 0;
static uintptr_t g_offset_scrplanet_cached_angle = 0;
static uintptr_t g_offset_scrplayer_planetary_system = 0;
static uintptr_t g_offset_scrplayer_last_hit = 0;
static uintptr_t g_offset_scrplayer_actual_last_hit = 0;
static uintptr_t g_offset_scrplayer_key_times = 0;
static uintptr_t g_offset_scrplayer_hold_keys = 0;
static uintptr_t g_offset_scrplayer_key_total = 0;
static uintptr_t g_offset_scrplayer_consec_multipress_counter = 0;
static uintptr_t g_offset_scrplayer_key_limiter_over_counter = 0;
static uintptr_t g_offset_scrplayer_taps_on_this_floor = 0;
static uintptr_t g_offset_scrplayer_alive = 0;
static uintptr_t g_offset_scrplayer_responsive = 0;
static uintptr_t g_offset_planetarysystem_chosen_planet = 0;
static uintptr_t g_offset_planetarysystem_speed = 0;
static uintptr_t g_offset_planetarysystem_is_cw = 0;
static uintptr_t g_offset_scrfloor_seq_id = 0;
static uintptr_t g_offset_scrfloor_nextfloor = 0;
static uintptr_t g_offset_scrfloor_margin_scale = 0;
static uintptr_t g_offset_scrfloor_auto = 0;
static uintptr_t g_offset_scrfloor_taps_needed = 0;
static uintptr_t g_offset_scrfloor_taps_so_far = 0;
static uintptr_t g_offset_scrfloor_hold_length = 0;
static uintptr_t g_offset_scrfloor_hold_completion = 0;
static uintptr_t g_offset_scrfloor_entry_time = 0;
static uintptr_t g_offset_scrfloor_entry_time_pitch_adj = 0;
static uintptr_t g_offset_scrconductor_song = 0;
static uintptr_t g_offset_scrconductor_bpm = 0;
static uintptr_t g_offset_scrconductor_addoffset = 0;
static uintptr_t g_offset_scrconductor_crotchet_at_start = 0;
static uintptr_t g_offset_scrconductor_dsp_time_song = 0;
static void *g_async_key_mask = NULL;
static void *g_async_key_down_mask = NULL;
static void *g_async_key_up_mask = NULL;
static void *g_async_frame_key_mask = NULL;
static void *g_async_frame_key_down_mask = NULL;
static void *g_async_frame_key_up_mask = NULL;
static Il2CppMethodCache g_hashset_add_method;
static Il2CppMethodCache g_hashset_remove_method;
static Il2CppMethodCache g_hashset_clear_method;
static Il2CppMethodCache g_hashset_get_count_method;
static Il2CppMethodCache g_audio_get_dsptime_method;
static Il2CppMethodCache g_time_get_frame_count_method;
static Il2CppMethodCache g_process_key_inputs_method;
static Il2CppMethodCache g_scrcontroller_get_chosen_planet_method;
static Il2CppMethodCache g_scrplanet_async_refresh_angles_method;
static Il2CppMethodCache g_scrplanet_update_refresh_angles_method;
static Il2CppMethodCache g_scrplanet_get_snapped_last_angle_method;
static Il2CppMethodCache g_scrplanet_get_target_exit_angle_method;
static Il2CppMethodCache g_scrconductor_get_instance_method;
static Il2CppMethodCache g_scrconductor_get_calibration_i_method;
static Il2CppMethodCache g_adobase_get_is_mobile_method;
static Il2CppMethodCache g_rdc_get_auto_method;
static Il2CppMethodCache g_rdc_set_auto_method;
static Il2CppMethodCache g_rdc_get_use_old_auto_method;
static Il2CppMethodCache g_audiosource_get_pitch_method;
static Il2CppMethodCache g_adobase_get_controller_method;
static Il2CppMethodCache g_adobase_get_editor_method;
static Il2CppMethodCache g_adobase_get_is_scn_game_method;
static Il2CppMethodCache g_adobase_get_is_level_editor_method;
static Il2CppMethodCache g_adobase_get_is_dlc_level_method;
static Il2CppMethodCache g_adobase_get_scene_name_method;
static Il2CppMethodCache g_editor_get_play_mode_method;
static int g_mask_api_ready = 0;
static int g_metadata_init_ready = 0;
static int g_metadata_init_attempts = 0;
static int g_last_update_input_frame = -1;
static void *g_last_update_input_controller = NULL;
static pthread_mutex_t g_il2cpp_state_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_il2cpp_state_cond = PTHREAD_COND_INITIALIZER;
static volatile int g_il2cpp_init_completed = 0;
static volatile int g_il2cpp_state_hooks_installed = 0;
static volatile int g_il2cpp_init_hook_installed = 0;
static volatile int g_il2cpp_init_utf16_hook_installed = 0;
static Il2CppInitFn g_original_il2cpp_init = NULL;
static Il2CppInitUtf16Fn g_original_il2cpp_init_utf16 = NULL;

static void ensure_metadata_ready_lazy(void);
static void mark_il2cpp_init_completed(const char *reason);
static int capture_gate_open(void);
static void enqueue_event(int type, int source_id, uint64_t raw_ns);
static void clear_ui_sources(void);
static int ensure_mask_api_ready(void);
static int cache_method_from_class(void *class_ptr, const char *method_name, int args_count, Il2CppMethodCache *out);
static int hashset_count_if_ready(void *set);
static int ensure_process_key_inputs_ready(void);
static int read_static_bool_method(const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache, int fallback);
static double read_static_float_method(const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache, double fallback);
static void *read_static_object_method(const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache);
static double read_self_double_method(void *self, const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache, double fallback);
static double read_audio_source_pitch(void *audio_source);
static int read_rdc_auto(int *out);
static int write_rdc_auto(int value);
static void async_masks_clear_all(void);
static void async_masks_clear_all_if_ready(void);
static void async_masks_clear_frame_edges(void);
static void async_masks_clear_tick_edges(void);
static void apply_primary_key_masks(int down, int up, int held_count);
static void restore_regular_input_types(void);
static void force_async_input_types(void);
static int consume_auto_hit_via_async_state_machine(void *player_self);
static void restore_async_angle_to_tick(void *controller_self, uint64_t tick);
static uint64_t current_async_frame_tick_or_now(void);
static void suppress_async_camera_override_marker(void);
static void close_async_capture(void);
static void disable_async_for_dlc_if_needed(const char *reason);
static void stop_capture_and_clear_queue(void);
static int capture_accepts_raw_ns(uint64_t raw_ns);
static void *read_adobase_controller(void);
static int controller_current_state(void *controller_self);
static int controller_cached_state(void *controller_self);
static int controller_is_paused(void *controller_self);
static int try_controller_destination_state(void *controller_self, int *out_state);
static int controller_allows_async_capture(void *controller_self);
static int controller_allows_async_replay(void *controller_self);
static void reset_capture_for_new_session(void *controller_self, const char *reason);
static int ensure_method_cache(const char *namespaze, const char *klass, const char *method, int args_count, Il2CppMethodCache *cache);
static int ensure_method_cache_in_image(const char *image_name, const char *namespaze, const char *klass, const char *method, int args_count, Il2CppMethodCache *cache);
static const char *shadow_margin_name(AdoOfficialHitMargin margin);
static void shadow_judgement_before_process(void *controller_self, uint64_t tick, int replay_mode);

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_metadata_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_virtual_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_async_clock_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_ingress_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_ingress_cond = PTHREAD_COND_INITIALIZER;
static AsyncEvent g_queue[MAX_EVENTS];
static int g_queue_head = 0;
static int g_queue_count = 0;
static int g_down_count = 0;
static int g_up_count = 0;
static int g_held_sources[MAX_HELD_SOURCES];
static int g_held_slots[MAX_HELD_SOURCES];
static int g_held_count = 0;
static uint64_t g_down_slot_mask = 0;
static uint64_t g_up_slot_mask = 0;
static uint64_t g_held_slot_mask = 0;
static int g_physical_sources[MAX_HELD_SOURCES];
static int g_physical_count = 0;
static int g_ui_sources[MAX_HELD_SOURCES];
static int g_ui_count = 0;
static int g_suppressed_sources[MAX_HELD_SOURCES];
static int g_suppressed_count = 0;
static uint64_t g_capture_suppress_until_raw_ns = 0;
static uint64_t g_last_processed_tick = 0;
static uint64_t g_replay_cursor_tick = 0;
static int g_replay_cursor_ready = 0;
static uint64_t g_current_frame_tick = 0;
static int g_current_frame_tick_ready = 0;
static uint64_t g_wall_minus_uptime_ticks = 0;
static uint64_t g_last_playercontrol_wall_tick = 0;
static uint64_t g_last_auto_synthetic_down_raw_ns = 0;
static IngressRecord g_ingress_queue[MAX_INGRESS_RECORDS];
static int g_ingress_head = 0;
static int g_ingress_count = 0;
static uint64_t g_ingress_seq = 0;
static uint64_t g_ingress_processed_seq = 0;
static int g_input_thread_started = 0;
static int g_input_thread_stop = 0;
static int g_soft_pause_had_capture = 0;
static int g_virtual_running = 1;
static uint64_t g_virtual_base_raw_ns = 0;
static uint64_t g_virtual_base_tick = 0;
static uint64_t g_virtual_pause_tick = 0;
static int g_virtual_resumed_from_pause = 0;
static uint64_t g_last_async_frame_tick = 0;
static volatile int g_managed_masks_need_clear = 0;
static int g_replay_mode = REPLAY_MODE_NONE;
static int g_logged_update_offset_gate_closed = 0;
static uint64_t g_last_input_latency_log_ns = 0;
static uint64_t g_max_input_dispatch_latency_us = 0;
static uint64_t g_last_touch_gate_log_ns = 0;
static uint64_t g_last_update_input_log_ns = 0;
static uint64_t g_last_legacy_fallback_log_ns = 0;
static uint64_t g_last_replay_empty_log_ns = 0;
static uint64_t g_verify_mask_edge_count = 0;
static uint64_t g_verify_rdinput_count = 0;
static uint64_t g_verify_simulated_count = 0;
static uint64_t g_ingress_event_posted_count = 0;
static uint64_t g_ingress_event_sealed_count = 0;
static uint64_t g_replay_event_count = 0;
static pthread_once_t g_input_api_once = PTHREAD_ONCE_INIT;
static InputEventFromJavaFn g_motion_event_from_java = NULL;
static InputEventFromJavaFn g_key_event_from_java = NULL;
static InputEventGetTimeFn g_motion_event_get_time = NULL;
static InputEventGetTimeFn g_key_event_get_time = NULL;
static InputEventReleaseFn g_input_event_release = NULL;
static InputEventGetSourceFn g_input_event_get_source = NULL;
static MotionEventGetActionFn g_motion_event_get_action = NULL;
static MotionEventGetPointerCountFn g_motion_event_get_pointer_count = NULL;
static MotionEventGetPointerIdFn g_motion_event_get_pointer_id = NULL;
static MotionEventGetCoordFn g_motion_event_get_x = NULL;
static MotionEventGetCoordFn g_motion_event_get_y = NULL;
static KeyEventGetIntFn g_key_event_get_action = NULL;
static KeyEventGetIntFn g_key_event_get_key_code = NULL;
static KeyEventGetIntFn g_key_event_get_meta_state = NULL;
static KeyEventGetIntFn g_key_event_get_repeat_count = NULL;

typedef struct MotionMethodCache {
    int ready;
    jclass cls;
    jmethodID getActionMasked;
    jmethodID getActionIndex;
    jmethodID getPointerId;
    jmethodID getSource;
    jmethodID getX;
    jmethodID getY;
} MotionMethodCache;

typedef struct KeyMethodCache {
    int ready;
    jclass cls;
    jmethodID getAction;
    jmethodID getKeyCode;
    jmethodID getMetaState;
    jmethodID getRepeatCount;
} KeyMethodCache;

static pthread_mutex_t g_jni_cache_lock = PTHREAD_MUTEX_INITIALIZER;
static MotionMethodCache g_motion_methods;
static KeyMethodCache g_key_methods;

static GetIsActiveFn g_original_is_active = NULL;
static UpdateOffsetTimeFn g_original_update_offset_time = NULL;
static VoidSelfFn __attribute__((unused)) g_original_conductor_update = NULL;
static VoidSelfFn g_original_update_input = NULL;
static SimulatedUpdateFn g_original_simulated_update = NULL;
static BoolSelfFn g_original_touch_enabled = NULL;
static BoolSelfFn g_original_valid_triggered = NULL;
static BoolSelfFn g_original_valid_released = NULL;
static IntSelfFn g_original_count_valid_keys = NULL;
static BoolSelfFn g_original_get_holding = NULL;
static RDInputGetMainFn g_original_rdinput_get_main = NULL;
static IntStaticFn g_original_rdinput_get_main_press_count = NULL;
static GetHitMarginFn g_original_get_hit_margin = NULL;
static VoidSelfFn g_original_playercontrol_update = NULL;
static VoidSelfFn g_original_editor_update = NULL;
static SetPausedFn __attribute__((unused)) g_original_set_paused = NULL;
static BoolSelfFn g_original_scrplayer_get_auto = NULL;
static HitSelfBoolFn g_original_scrplayer_hit = NULL;
static SwitchChosenFn g_original_scrplanet_switch_chosen = NULL;
static AdjustAngleFn g_original_adjust_angle = NULL;
static CameraUpdateFollowCamFn g_original_camera_update_follow_cam = NULL;

static uint64_t wall_ticks_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((uint64_t)ts.tv_sec + 62135596800ULL) * 10000000ULL + (uint64_t)(ts.tv_nsec / 100);
}

static uint64_t uptime_ticks_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 10000000ULL + (uint64_t)(ts.tv_nsec / 100);
}

static uint64_t monotonic_ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void update_time_origin(void) {
    uint64_t wall = wall_ticks_now();
    uint64_t up = uptime_ticks_now();
    g_wall_minus_uptime_ticks = wall - up;
}

static uint64_t uptime_ms_to_wall_ticks(int64_t ms) {
    if (g_wall_minus_uptime_ticks == 0) {
        update_time_origin();
    }
    return g_wall_minus_uptime_ticks + (uint64_t)ms * 10000ULL;
}

static uint64_t uptime_ns_to_wall_ticks(int64_t ns) {
    if (g_wall_minus_uptime_ticks == 0) {
        update_time_origin();
    }
    return g_wall_minus_uptime_ticks + (uint64_t)(ns / 100);
}

static uint64_t raw_ns_to_virtual_tick(uint64_t raw_ns) {
    pthread_mutex_lock(&g_virtual_lock);
    if (g_virtual_base_tick == 0 || g_virtual_base_raw_ns == 0) {
        g_virtual_base_raw_ns = raw_ns ? raw_ns : monotonic_ns_now();
        g_virtual_base_tick = wall_ticks_now();
        g_virtual_pause_tick = g_virtual_base_tick;
        g_virtual_running = 1;
    }

    if (!g_virtual_running) {
        uint64_t tick = g_virtual_pause_tick;
        pthread_mutex_unlock(&g_virtual_lock);
        return tick;
    }

    if (raw_ns < g_virtual_base_raw_ns) {
        uint64_t tick = g_virtual_base_tick;
        pthread_mutex_unlock(&g_virtual_lock);
        return tick;
    }
    uint64_t tick = g_virtual_base_tick + (raw_ns - g_virtual_base_raw_ns) / 100ULL;
    pthread_mutex_unlock(&g_virtual_lock);
    return tick;
}

static void virtual_clock_reset(uint64_t raw_ns) {
    pthread_mutex_lock(&g_virtual_lock);
    g_virtual_base_raw_ns = raw_ns ? raw_ns : monotonic_ns_now();
    g_virtual_base_tick = wall_ticks_now();
    g_virtual_pause_tick = g_virtual_base_tick;
    g_virtual_running = 1;
    g_virtual_resumed_from_pause = 0;
    pthread_mutex_unlock(&g_virtual_lock);

    pthread_mutex_lock(&g_async_clock_lock);
    g_last_async_frame_tick = 0;
    pthread_mutex_unlock(&g_async_clock_lock);

    LOGI("virtual clock reset raw_ns=%" PRIu64, raw_ns ? raw_ns : g_virtual_base_raw_ns);
}

static void virtual_clock_pause(uint64_t raw_ns) {
    pthread_mutex_lock(&g_virtual_lock);
    uint64_t pause_raw_ns = raw_ns ? raw_ns : monotonic_ns_now();
    if (g_virtual_base_tick == 0 || g_virtual_base_raw_ns == 0) {
        g_virtual_base_raw_ns = pause_raw_ns;
        g_virtual_base_tick = wall_ticks_now();
        g_virtual_pause_tick = g_virtual_base_tick;
    } else if (g_virtual_running) {
        if (pause_raw_ns < g_virtual_base_raw_ns) {
            g_virtual_pause_tick = g_virtual_base_tick;
        } else {
            g_virtual_pause_tick = g_virtual_base_tick + (pause_raw_ns - g_virtual_base_raw_ns) / 100ULL;
        }
    }
    g_virtual_running = 0;
    g_virtual_resumed_from_pause = 0;
    pthread_mutex_unlock(&g_virtual_lock);

    LOGI("virtual clock paused raw_ns=%" PRIu64, pause_raw_ns);
}

static void virtual_clock_resume(uint64_t raw_ns) {
    pthread_mutex_lock(&g_virtual_lock);
    uint64_t resume_raw_ns = raw_ns ? raw_ns : monotonic_ns_now();
    if (g_virtual_base_tick == 0 || g_virtual_base_raw_ns == 0) {
        g_virtual_base_raw_ns = resume_raw_ns;
        g_virtual_base_tick = wall_ticks_now();
        g_virtual_pause_tick = g_virtual_base_tick;
    } else if (!g_virtual_running) {
        g_virtual_base_raw_ns = resume_raw_ns;
        g_virtual_base_tick = g_virtual_pause_tick;
    }
    g_virtual_running = 1;
    g_virtual_resumed_from_pause = 1;
    pthread_mutex_unlock(&g_virtual_lock);

    pthread_mutex_lock(&g_async_clock_lock);
    g_last_async_frame_tick = 0;
    pthread_mutex_unlock(&g_async_clock_lock);

    LOGI("virtual clock resumed raw_ns=%" PRIu64, resume_raw_ns);
}

static uint64_t virtual_tick_now(void) {
    return raw_ns_to_virtual_tick(monotonic_ns_now());
}

static int virtual_consume_resume_marker(void) {
    pthread_mutex_lock(&g_virtual_lock);
    int value = g_virtual_resumed_from_pause;
    g_virtual_resumed_from_pause = 0;
    pthread_mutex_unlock(&g_virtual_lock);
    return value;
}

static int load_bool_file(const char *path, int default_value) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return default_value;
    }
    int c = fgetc(fp);
    fclose(fp);
    return (c == '1') ? 1 : 0;
}

static void save_bool_file(const char *path, int value) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        LOGW("save state failed path=%s errno=%d", path, errno);
        return;
    }
    fputc(value ? '1' : '0', fp);
    fclose(fp);
}

static int source_index_in_locked(const int *sources, int count, int source_id) {
    for (int i = 0; i < count; ++i) {
        if (sources[i] == source_id) {
            return i;
        }
    }
    return -1;
}

static int logical_source_index_locked(int source_id) {
    return source_index_in_locked(g_held_sources, g_held_count, source_id);
}

static int held_slot_used_locked(int slot) {
    for (int i = 0; i < g_held_count; ++i) {
        if (g_held_slots[i] == slot) {
            return 1;
        }
    }
    return 0;
}

static int allocate_held_slot_locked(void) {
    for (int slot = 0; slot < ASYNC_KEY_SLOT_COUNT; ++slot) {
        if (!held_slot_used_locked(slot)) {
            return slot;
        }
    }
    return -1;
}

static int physical_source_index_locked(int source_id) {
    return source_index_in_locked(g_physical_sources, g_physical_count, source_id);
}

static int ui_source_index_locked(int source_id) {
    return source_index_in_locked(g_ui_sources, g_ui_count, source_id);
}

static int suppressed_source_index_locked(int source_id) {
    return source_index_in_locked(g_suppressed_sources, g_suppressed_count, source_id);
}

static int queue_active_count_locked(void) {
    return g_queue_count - g_queue_head;
}

static int queue_pending_count_snapshot(void) {
    pthread_mutex_lock(&g_lock);
    int count = queue_active_count_locked();
    pthread_mutex_unlock(&g_lock);
    return count;
}

static int __attribute__((unused)) synthetic_auto_pending_locked(void) {
    for (int i = g_queue_head; i < g_queue_count; ++i) {
        if (g_queue[i].synthetic_auto) {
            return 1;
        }
    }
    return 0;
}

static void compact_queue_locked(void) {
    int active_count = queue_active_count_locked();
    if (active_count > 0 && g_queue_head > 0) {
        memmove(&g_queue[0], &g_queue[g_queue_head], sizeof(g_queue[0]) * active_count);
    }
    g_queue_head = 0;
    g_queue_count = active_count;
}

static void remove_source_at_locked(int *sources, int *count, int index) {
    if (index >= 0 && index < *count) {
        sources[index] = sources[--(*count)];
    }
}

static void remove_held_source_at_locked(int index) {
    if (index >= 0 && index < g_held_count) {
        int last = --g_held_count;
        g_held_sources[index] = g_held_sources[last];
        g_held_slots[index] = g_held_slots[last];
    }
}

static uint64_t held_slot_mask_locked(void) {
    uint64_t mask = 0;
    for (int i = 0; i < g_held_count; ++i) {
        int slot = g_held_slots[i];
        if (slot >= 0 && slot < ASYNC_KEY_SLOT_COUNT) {
            mask |= 1ULL << slot;
        }
    }
    return mask;
}

static void clear_runtime_state_locked(void) {
    g_queue_head = 0;
    g_queue_count = 0;
    g_down_count = 0;
    g_up_count = 0;
    g_held_count = 0;
    g_down_slot_mask = 0;
    g_up_slot_mask = 0;
    g_held_slot_mask = 0;
    g_physical_count = 0;
    g_ui_count = 0;
    g_suppressed_count = 0;
    g_capture_suppress_until_raw_ns = 0;
    g_last_processed_tick = 0;
    g_replay_cursor_tick = 0;
    g_replay_cursor_ready = 0;
    g_current_frame_tick = 0;
    g_current_frame_tick_ready = 0;
    g_last_auto_synthetic_down_raw_ns = 0;
    g_managed_masks_need_clear = 1;
}

static void set_capture_gate_locked(int ready, uint64_t tick) {
    g_capture_ready = ready ? 1 : 0;
    g_last_playercontrol_wall_tick = ready ? tick : 0;
}

static int ingress_push_locked(const IngressRecord *record) {
    if (record == NULL) {
        return 0;
    }
    if (g_ingress_count == MAX_INGRESS_RECORDS) {
        uint64_t dropped_seq = g_ingress_queue[g_ingress_head].seq;
        g_ingress_head = (g_ingress_head + 1) % MAX_INGRESS_RECORDS;
        g_ingress_count--;
        if (g_ingress_processed_seq < dropped_seq) {
            g_ingress_processed_seq = dropped_seq;
        }
        pthread_cond_broadcast(&g_ingress_cond);
        LOGW("ingress queue overflow, dropped oldest");
    }
    int index = (g_ingress_head + g_ingress_count) % MAX_INGRESS_RECORDS;
    g_ingress_queue[index] = *record;
    g_ingress_count++;
    pthread_cond_signal(&g_ingress_cond);
    return 1;
}

static int ingress_post_event(int event_type, int source_id, uint64_t raw_ns) {
    IngressRecord record;
    memset(&record, 0, sizeof(record));
    record.kind = INGRESS_EVENT;
    record.event_type = event_type;
    record.source_id = source_id;
    record.raw_ns = raw_ns ? raw_ns : monotonic_ns_now();

    uint64_t now_ns = monotonic_ns_now();
    if (now_ns > record.raw_ns) {
        uint64_t latency_us = (now_ns - record.raw_ns) / 1000ULL;
        if (latency_us > g_max_input_dispatch_latency_us) {
            g_max_input_dispatch_latency_us = latency_us;
        }
        if (latency_us >= 5000ULL && now_ns - g_last_input_latency_log_ns >= 1000000000ULL) {
            g_last_input_latency_log_ns = now_ns;
            LOGW("input dispatch latency high current_us=%" PRIu64 " max_us=%" PRIu64,
                 latency_us, g_max_input_dispatch_latency_us);
        }
    }

    pthread_mutex_lock(&g_ingress_lock);
    record.seq = ++g_ingress_seq;
    int ok = ingress_push_locked(&record);
    pthread_mutex_unlock(&g_ingress_lock);
    if (ok && g_trace_enabled) {
        uint64_t count = __atomic_add_fetch(&g_ingress_event_posted_count, 1, __ATOMIC_RELAXED);
        if (count <= 8 || (count % 64ULL) == 0) {
            LOGI("ingress event posted count=%" PRIu64 " type=%d source=%d raw=%" PRIu64,
                 count, event_type, source_id, record.raw_ns);
        }
    } else if (ok) {
        __atomic_add_fetch(&g_ingress_event_posted_count, 1, __ATOMIC_RELAXED);
    }
    return ok;
}

static void ingress_post_command(int kind, int wait_for_ack) {
    IngressRecord record;
    memset(&record, 0, sizeof(record));
    record.kind = kind;
    record.raw_ns = monotonic_ns_now();

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_nsec += RESET_BARRIER_TIMEOUT_MS * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec += deadline.tv_nsec / 1000000000L;
        deadline.tv_nsec %= 1000000000L;
    }

    pthread_mutex_lock(&g_ingress_lock);
    uint64_t seq = ++g_ingress_seq;
    record.seq = seq;
    ingress_push_locked(&record);
    if (wait_for_ack) {
        while (g_ingress_processed_seq < seq) {
            int rc = pthread_cond_timedwait(&g_ingress_cond, &g_ingress_lock, &deadline);
            if (rc != 0) {
                LOGW("reset barrier timed out");
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_ingress_lock);
}

static void ingress_post_reset(int wait_for_ack) {
    ingress_post_command(INGRESS_RESET, wait_for_ack);
}

static void ingress_post_soft_pause(int wait_for_ack) {
    ingress_post_command(INGRESS_SOFT_PAUSE, wait_for_ack);
}

static void ingress_post_soft_resume(void) {
    ingress_post_command(INGRESS_SOFT_RESUME, 0);
}

static void ingress_mark_processed(uint64_t seq) {
    pthread_mutex_lock(&g_ingress_lock);
    if (g_ingress_processed_seq < seq) {
        g_ingress_processed_seq = seq;
    }
    pthread_cond_broadcast(&g_ingress_cond);
    pthread_mutex_unlock(&g_ingress_lock);
}

static int ingress_pop(IngressRecord *out) {
    if (out == NULL) {
        return 0;
    }
    pthread_mutex_lock(&g_ingress_lock);
    while (g_ingress_count == 0 && !g_input_thread_stop) {
        pthread_cond_wait(&g_ingress_cond, &g_ingress_lock);
    }
    if (g_input_thread_stop) {
        pthread_mutex_unlock(&g_ingress_lock);
        return 0;
    }
    *out = g_ingress_queue[g_ingress_head];
    g_ingress_head = (g_ingress_head + 1) % MAX_INGRESS_RECORDS;
    g_ingress_count--;
    pthread_cond_broadcast(&g_ingress_cond);
    pthread_mutex_unlock(&g_ingress_lock);
    return 1;
}

static void *input_thread_main(void *arg) {
    (void)arg;
    LOGI("input thread started");

    IngressRecord record;
    while (ingress_pop(&record)) {
        if (record.kind == INGRESS_RESET) {
            pthread_mutex_lock(&g_lock);
            clear_runtime_state_locked();
            set_capture_gate_locked(0, 0);
            g_soft_pause_had_capture = 0;
            pthread_mutex_unlock(&g_lock);
            g_managed_masks_need_clear = 1;
            if (g_mask_api_ready) {
                async_masks_clear_all_if_ready();
            }
            virtual_clock_reset(record.raw_ns);
            ingress_mark_processed(record.seq);
            LOGI("runtime state cleared by ingress reset");
            continue;
        }
        if (record.kind == INGRESS_SOFT_PAUSE) {
            pthread_mutex_lock(&g_lock);
            g_soft_pause_had_capture = g_capture_ready && g_last_playercontrol_wall_tick != 0;
            clear_runtime_state_locked();
            set_capture_gate_locked(0, 0);
            pthread_mutex_unlock(&g_lock);
            g_managed_masks_need_clear = 1;
            if (g_mask_api_ready) {
                async_masks_clear_all_if_ready();
            }
            virtual_clock_pause(record.raw_ns);
            ingress_mark_processed(record.seq);
            LOGI("runtime state soft-paused");
            continue;
        }
        if (record.kind == INGRESS_SOFT_RESUME) {
            int resume_capture = 0;
            pthread_mutex_lock(&g_lock);
            resume_capture = g_soft_pause_had_capture;
            g_soft_pause_had_capture = 0;
            pthread_mutex_unlock(&g_lock);
            if (resume_capture) {
                virtual_clock_resume(record.raw_ns);
            } else {
                virtual_clock_reset(record.raw_ns);
            }
            ingress_mark_processed(record.seq);
            LOGI("runtime state soft-resumed");
            continue;
        }
        if (record.kind != INGRESS_EVENT) {
            ingress_mark_processed(record.seq);
            continue;
        }
        if (record.event_type == EVENT_DOWN || record.event_type == EVENT_UP) {
            enqueue_event(record.event_type, record.source_id, record.raw_ns);
            uint64_t count = __atomic_add_fetch(&g_ingress_event_sealed_count, 1, __ATOMIC_RELAXED);
            if (g_trace_enabled && (count <= 8 || (count % 64ULL) == 0)) {
                LOGI("ingress event sealed count=%" PRIu64 " type=%d source=%d raw_ns=%" PRIu64,
                     count, record.event_type, record.source_id, record.raw_ns);
            }
        }
        ingress_mark_processed(record.seq);
    }

    LOGI("input thread stopped");
    return NULL;
}

static void ensure_input_thread_started(void) {
    pthread_mutex_lock(&g_ingress_lock);
    if (g_input_thread_started) {
        pthread_mutex_unlock(&g_ingress_lock);
        return;
    }
    g_input_thread_started = 1;
    g_input_thread_stop = 0;
    pthread_mutex_unlock(&g_ingress_lock);

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, input_thread_main, NULL);
    if (rc == 0) {
        pthread_detach(thread);
    } else {
        LOGE("input pthread_create failed rc=%d", rc);
        pthread_mutex_lock(&g_ingress_lock);
        g_input_thread_started = 0;
        pthread_mutex_unlock(&g_ingress_lock);
    }
}

static int capture_gate_open(void) {
    pthread_mutex_lock(&g_scene_state_lock);
    int dlc_disabled = g_scene_state_ready && g_cached_is_dlc_level;
    pthread_mutex_unlock(&g_scene_state_lock);
    if (dlc_disabled) {
        return 0;
    }

    pthread_mutex_lock(&g_lock);
    int open = g_capture_ready && g_last_playercontrol_wall_tick != 0;
    if (open) {
        uint64_t now = wall_ticks_now();
        if (now >= g_last_playercontrol_wall_tick &&
            now - g_last_playercontrol_wall_tick > CAPTURE_STALE_TICKS) {
            clear_runtime_state_locked();
            set_capture_gate_locked(0, 0);
            g_capture_floor_raw_ns = 0;
            open = 0;
            if (g_trace_enabled) {
                LOGI("capture gate stale closed");
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
    return open ? 1 : 0;
}

static int __attribute__((unused)) capture_gate_active(void) {
    return capture_gate_open();
}

static void update_current_frame_tick(uint64_t tick) {
    if (tick == 0) {
        return;
    }

    pthread_mutex_lock(&g_lock);
    g_current_frame_tick = tick;
    g_current_frame_tick_ready = 1;
    pthread_mutex_unlock(&g_lock);
}

static uint64_t replay_target_tick_now(void) {
    pthread_mutex_lock(&g_lock);
    int ready = g_current_frame_tick_ready;
    uint64_t tick = g_current_frame_tick;
    pthread_mutex_unlock(&g_lock);
    if (ready && tick != 0) {
        return tick;
    }
    return virtual_tick_now();
}

static void write_static_u64_field(void *field, uint64_t value) {
    if (field != NULL && g_il2cpp_api.field_static_set_value != NULL) {
        g_il2cpp_api.field_static_set_value(field, &value);
    }
}

static void write_static_bool_field(void *field, int value) {
    if (field != NULL && g_il2cpp_api.field_static_set_value != NULL) {
        uint8_t bool_value = value ? 1 : 0;
        g_il2cpp_api.field_static_set_value(field, &bool_value);
    }
}

static int read_static_bool_field(void *field, int *out) {
    if (field == NULL || out == NULL || g_il2cpp_api.field_static_get_value == NULL) {
        return 0;
    }
    uint8_t value = 0;
    g_il2cpp_api.field_static_get_value(field, &value);
    *out = value ? 1 : 0;
    return 1;
}

static int read_static_u64_field(void *field, uint64_t *out) {
    if (field == NULL || out == NULL || g_il2cpp_api.field_static_get_value == NULL) {
        return 0;
    }
    uint64_t value = 0;
    g_il2cpp_api.field_static_get_value(field, &value);
    *out = value;
    return 1;
}

static int read_static_object_field(void *field, void **out) {
    if (field == NULL || out == NULL || g_il2cpp_api.field_static_get_value == NULL) {
        return 0;
    }
    void *value = NULL;
    g_il2cpp_api.field_static_get_value(field, &value);
    *out = value;
    return value != NULL;
}

static int read_static_int_field(void *field, int *out) {
    if (field == NULL || out == NULL || g_il2cpp_api.field_static_get_value == NULL) {
        return 0;
    }
    int32_t value = 0;
    g_il2cpp_api.field_static_get_value(field, &value);
    *out = value;
    return 1;
}

static int read_static_float_field(void *field, double *out) {
    if (field == NULL || out == NULL || g_il2cpp_api.field_static_get_value == NULL) {
        return 0;
    }
    float value = 0.0f;
    g_il2cpp_api.field_static_get_value(field, &value);
    *out = (double)value;
    return 1;
}

static int read_instance_object_field(void *object, uintptr_t offset, void **out) {
    if (object == NULL || offset == 0 || out == NULL) {
        return 0;
    }
    void *value = *(void **)((uintptr_t)object + offset);
    *out = value;
    return value != NULL;
}

static int read_instance_bool_field(void *object, uintptr_t offset, int *out) {
    if (object == NULL || offset == 0 || out == NULL) {
        return 0;
    }
    *out = *(uint8_t *)((uintptr_t)object + offset) ? 1 : 0;
    return 1;
}

static int read_instance_int_field(void *object, uintptr_t offset, int *out) {
    if (object == NULL || offset == 0 || out == NULL) {
        return 0;
    }
    *out = *(int *)((uintptr_t)object + offset);
    return 1;
}

static int read_instance_float_field(void *object, uintptr_t offset, double *out) {
    if (object == NULL || offset == 0 || out == NULL) {
        return 0;
    }
    *out = (double)(*(float *)((uintptr_t)object + offset));
    return 1;
}

static int read_instance_double_field(void *object, uintptr_t offset, double *out) {
    if (object == NULL || offset == 0 || out == NULL) {
        return 0;
    }
    *out = *(double *)((uintptr_t)object + offset);
    return 1;
}

static int write_instance_double_field(void *object, uintptr_t offset, double value) {
    if (object == NULL || offset == 0) {
        return 0;
    }
    *(double *)((uintptr_t)object + offset) = value;
    return 1;
}

static int object_count_if_ready(void *object) {
    if (object == NULL || !g_il2cpp_api.ready || g_il2cpp_api.object_get_class == NULL ||
        g_il2cpp_api.class_get_method_from_name == NULL) {
        return -1;
    }
    void *klass = g_il2cpp_api.object_get_class(object);
    Il2CppMethodCache get_count;
    memset(&get_count, 0, sizeof(get_count));
    if (!cache_method_from_class(klass, "get_Count", 0, &get_count)) {
        return -1;
    }
    IntSelfFn fn = (IntSelfFn)get_count.method_pointer;
    return fn(object, get_count.method_info);
}

static AsyncKeyCodeValue primary_async_key(void) {
    AsyncKeyCodeValue key;
    memset(&key, 0, sizeof(key));
    key.key = (uint16_t)ASYNC_KEY_RAW_NONE;
    key.label = ASYNC_KEY_LABEL_SPACE;
    return key;
}

static AsyncKeyCodeValue async_key_for_slot(int slot) {
    static const int labels[ASYNC_KEY_SLOT_COUNT] = {
        81, 65, 113, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 41, 42, 44,
    };
    AsyncKeyCodeValue key;
    memset(&key, 0, sizeof(key));
    if (slot < 0 || slot >= ASYNC_KEY_SLOT_COUNT) {
        return primary_async_key();
    }
    key.key = (uint16_t)(ASYNC_KEY_RAW_BASE + (uint16_t)slot);
    key.label = labels[slot];
    return key;
}

static double read_dsp_time_or_zero(void) {
    if (!ensure_method_cache_in_image(
            IL2CPP_IMAGE_AUDIO_MODULE,
            "UnityEngine",
            "AudioSettings",
            "get_dspTime",
            0,
            &g_audio_get_dsptime_method)) {
        return 0.0;
    }

    GetDoubleStaticFn get_dsptime = (GetDoubleStaticFn)g_audio_get_dsptime_method.method_pointer;
    return get_dsptime(g_audio_get_dsptime_method.method_info);
}

static uint64_t __attribute__((unused)) dsp_tick_now_or_virtual(void) {
    double dsp_time = read_dsp_time_or_zero();
    if (dsp_time > 0.0) {
        return (uint64_t)(dsp_time * 10000000.0);
    }
    return virtual_tick_now();
}

static uint64_t wall_tick_now_or_virtual(void) {
    uint64_t tick = wall_ticks_now();
    return tick != 0 ? tick : virtual_tick_now();
}

static uint64_t __attribute__((unused)) raw_event_ns_to_dsp_tick(uint64_t event_raw_ns, uint64_t now_raw_ns, double current_dsp_time) {
    if (event_raw_ns == 0) {
        event_raw_ns = now_raw_ns;
    }
    if (current_dsp_time > 0.0) {
        uint64_t age_ns = now_raw_ns > event_raw_ns ? (now_raw_ns - event_raw_ns) : 0;
        double event_dsp_time = current_dsp_time - ((double)age_ns / 1000000000.0);
        if (event_dsp_time > 0.0) {
            return (uint64_t)(event_dsp_time * 10000000.0);
        }
    }
    return raw_ns_to_virtual_tick(event_raw_ns);
}

static int read_unity_frame_count_or_negative(void) {
    if (!ensure_method_cache_in_image(
            IL2CPP_IMAGE_CORE_MODULE,
            "UnityEngine",
            "Time",
            "get_frameCount",
            0,
            &g_time_get_frame_count_method)) {
        return -1;
    }

    IntStaticFn get_frame_count = (IntStaticFn)g_time_get_frame_count_method.method_pointer;
    return get_frame_count(g_time_get_frame_count_method.method_info);
}

static void *read_adobase_controller(void) {
    if (!ensure_method_cache("", "ADOBase", "get_controller", 0, &g_adobase_get_controller_method)) {
        return NULL;
    }
    GetObjectStaticFn get_controller = (GetObjectStaticFn)g_adobase_get_controller_method.method_pointer;
    return get_controller(g_adobase_get_controller_method.method_info);
}

static void *read_adobase_editor(void) {
    if (!ensure_method_cache("", "ADOBase", "get_editor", 0, &g_adobase_get_editor_method)) {
        return NULL;
    }
    GetObjectStaticFn get_editor = (GetObjectStaticFn)g_adobase_get_editor_method.method_pointer;
    return get_editor(g_adobase_get_editor_method.method_info);
}

static int read_adobase_bool_method(const char *method_name, Il2CppMethodCache *cache) {
    if (!ensure_method_cache("", "ADOBase", method_name, 0, cache)) {
        return 0;
    }
    IntStaticFn get_bool = (IntStaticFn)cache->method_pointer;
    return get_bool(cache->method_info) ? 1 : 0;
}

static int try_read_adobase_bool_method_safe(const char *method_name, Il2CppMethodCache *cache, int *out) {
    if (out == NULL || g_il2cpp_api.runtime_invoke == NULL) {
        return 0;
    }
    *out = 0;
    if (!ensure_method_cache("", "ADOBase", method_name, 0, cache)) {
        return 0;
    }

    void *exception = NULL;
    void *result = g_il2cpp_api.runtime_invoke(cache->method_info, NULL, NULL, &exception);
    if (exception != NULL || result == NULL) {
        static uint64_t last_exception_log_ns = 0;
        uint64_t now_ns = monotonic_ns_now();
        if (now_ns - last_exception_log_ns >= 1000000000ULL) {
            last_exception_log_ns = now_ns;
            LOGW("ADOBase.%s safe read failed exception=%p result=%p", method_name, exception, result);
        }
        return 0;
    }

    *out = (*(uint8_t *)((uintptr_t)result + OFFSET_BOXED_ENUM_VALUE)) ? 1 : 0;
    return 1;
}

static int level_editor_active(void) {
    if (!g_metadata_init_ready || !g_il2cpp_api.ready) {
        return 0;
    }
    return read_adobase_editor() != NULL;
}

static int read_editor_play_mode(void *editor) {
    if (editor == NULL) {
        return 0;
    }
    if (!ensure_method_cache("", "scnEditor", "get_playMode", 0, &g_editor_get_play_mode_method)) {
        return 0;
    }
    BoolSelfFn get_play_mode = (BoolSelfFn)g_editor_get_play_mode_method.method_pointer;
    return get_play_mode(editor, g_editor_get_play_mode_method.method_info) ? 1 : 0;
}

static int read_editor_bool_field(void *editor, uintptr_t offset) {
    if (editor == NULL || offset == 0) {
        return 0;
    }
    return *(uint8_t *)((uintptr_t)editor + offset) ? 1 : 0;
}

static int read_controller_bool_field(void *controller, uintptr_t offset) {
    if (controller == NULL || offset == 0) {
        return 0;
    }
    return *(uint8_t *)((uintptr_t)controller + offset) ? 1 : 0;
}

static int __attribute__((unused)) editor_blocks_async_live(void) {
    void *editor = read_adobase_editor();
    if (editor == NULL) {
        return 0;
    }
    return read_editor_play_mode(editor) ? 0 : 1;
}

static int editor_blocks_async_cached(void) {
    pthread_mutex_lock(&g_scene_state_lock);
    int ready = g_scene_state_ready;
    int blocks = g_cached_async_blocks;
    pthread_mutex_unlock(&g_scene_state_lock);
    return ready ? blocks : 0;
}

static int editor_blocks_async(void) {
    return editor_blocks_async_cached();
}

static int dlc_async_disabled_cached(void) {
    pthread_mutex_lock(&g_scene_state_lock);
    int disabled = g_scene_state_ready && g_cached_is_dlc_level;
    pthread_mutex_unlock(&g_scene_state_lock);
    return disabled ? 1 : 0;
}

static int scene_allows_async_gameplay_cached(void) {
    pthread_mutex_lock(&g_scene_state_lock);
    int ready = g_scene_state_ready;
    int gameworld = g_cached_gameworld;
    int is_level_editor = g_cached_is_level_editor;
    int editor_play = g_cached_editor_play_mode;
    int is_dlc = g_cached_is_dlc_level;
    pthread_mutex_unlock(&g_scene_state_lock);
    return ready && !is_dlc && gameworld && (!is_level_editor || editor_play);
}

static void scene_async_gate_snapshot(int *ready,
                                      int *gameworld,
                                      int *is_scn_game,
                                      int *editor_play,
                                      int *is_dlc,
                                      int *controller_state,
                                      char *scene,
                                      size_t scene_size) {
    pthread_mutex_lock(&g_scene_state_lock);
    if (ready != NULL) {
        *ready = g_scene_state_ready;
    }
    if (gameworld != NULL) {
        *gameworld = g_cached_gameworld;
    }
    if (is_scn_game != NULL) {
        *is_scn_game = g_cached_is_scn_game;
    }
    if (editor_play != NULL) {
        *editor_play = g_cached_editor_play_mode;
    }
    if (is_dlc != NULL) {
        *is_dlc = g_cached_is_dlc_level;
    }
    if (controller_state != NULL) {
        *controller_state = g_cached_controller_state;
    }
    if (scene != NULL && scene_size > 0) {
        snprintf(scene, scene_size, "%s", g_cached_scene_name[0] ? g_cached_scene_name : "<unknown>");
    }
    pthread_mutex_unlock(&g_scene_state_lock);
}

static int async_is_active_gate_allows(void) {
    if (!g_enabled) {
        return 0;
    }
    if (g_in_conductor_frame ||
        (g_in_async_replay && g_replay_mode == REPLAY_MODE_LEGACY)) {
        return 0;
    }
    if (editor_blocks_async()) {
        return 0;
    }
    if (!capture_gate_open()) {
        return 0;
    }
    int controller_state = -999;
    scene_async_gate_snapshot(NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &controller_state,
                              NULL,
                              0);
    return scene_allows_async_gameplay_cached() &&
           controller_state == STATE_PLAYER_CONTROL;
}

static void il2cpp_string_to_utf8(void *str, char *out, size_t out_size) {
    if (out == NULL || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (str == NULL || g_il2cpp_api.string_chars == NULL || g_il2cpp_api.string_length == NULL) {
        snprintf(out, out_size, "<unavailable>");
        return;
    }
    const uint16_t *chars = g_il2cpp_api.string_chars(str);
    int32_t len = g_il2cpp_api.string_length(str);
    if (chars == NULL || len <= 0) {
        snprintf(out, out_size, "");
        return;
    }
    size_t w = 0;
    for (int32_t i = 0; i < len && w + 1 < out_size; ++i) {
        uint16_t c = chars[i];
        if (c < 0x80) {
            out[w++] = (char)c;
        } else if (c < 0x800 && w + 2 < out_size) {
            out[w++] = (char)(0xC0 | (c >> 6));
            out[w++] = (char)(0x80 | (c & 0x3F));
        } else if (w + 3 < out_size) {
            out[w++] = (char)(0xE0 | (c >> 12));
            out[w++] = (char)(0x80 | ((c >> 6) & 0x3F));
            out[w++] = (char)(0x80 | (c & 0x3F));
        } else {
            break;
        }
    }
    out[w] = '\0';
}

static void refresh_scene_state_on_main_thread(const char *reason, void *controller_self) {
    static int last_editor_present = -1;
    static int last_editor_play_mode = -1;
    static int last_is_dlc_level = -1;
    static int last_paused_in_play_mode = -1;
    static int last_strict_edit = -1;
    static int last_controller_state = -999;
    static int last_cached_controller_state = -999;
    static int last_destination_state = -999;
    static int last_capture = -1;
    static char last_scene[128] = {0};

    char scene[128];
    snprintf(scene, sizeof(scene), "<unknown>");
    if (ensure_method_cache("", "ADOBase", "get_sceneName", 0, &g_adobase_get_scene_name_method)) {
        GetObjectStaticFn get_scene_name = (GetObjectStaticFn)g_adobase_get_scene_name_method.method_pointer;
        void *scene_str = get_scene_name(g_adobase_get_scene_name_method.method_info);
        il2cpp_string_to_utf8(scene_str, scene, sizeof(scene));
    }

    void *editor = level_editor_active() ? read_adobase_editor() : NULL;
    int editor_present = editor != NULL;
    int editor_play_mode = editor_present ? read_editor_play_mode(editor) : 0;
    int is_scn_game = read_adobase_bool_method("get_isScnGame", &g_adobase_get_is_scn_game_method);
    int is_level_editor = read_adobase_bool_method("get_isLevelEditor", &g_adobase_get_is_level_editor_method);
    int gameworld = read_controller_bool_field(controller_self, g_offset_scrcontroller_gameworld);
    int is_dlc_level = 0;
    if (controller_self != NULL && gameworld && is_scn_game) {
        int dlc_value = 0;
        if (try_read_adobase_bool_method_safe("get_isDLCLevel", &g_adobase_get_is_dlc_level_method, &dlc_value)) {
            is_dlc_level = dlc_value;
        }
    }
    int paused_in_play_mode = editor_present ? read_editor_bool_field(editor, g_offset_scneditor_paused_in_play_mode) : 0;
    int strict_edit = editor_present ? read_editor_bool_field(editor, g_offset_scneditor_in_strictly_editing_mode) : 0;
    int controller_state = controller_self ? controller_current_state(controller_self) : -1;
    int cached_controller_state = controller_self ? controller_cached_state(controller_self) : -1;
    int destination_state = -1;
    if (controller_self != NULL) {
        (void)try_controller_destination_state(controller_self, &destination_state);
    }
    int capture = capture_gate_open();
    int async_blocks = is_dlc_level || !gameworld || (is_level_editor && !editor_play_mode);

    pthread_mutex_lock(&g_scene_state_lock);
    g_scene_state_ready = 1;
    g_cached_editor_present = editor_present;
    g_cached_editor_play_mode = editor_play_mode;
    g_cached_is_scn_game = is_scn_game;
    g_cached_is_level_editor = is_level_editor;
    g_cached_is_dlc_level = is_dlc_level;
    g_cached_gameworld = gameworld;
    g_cached_paused_in_play_mode = paused_in_play_mode;
    g_cached_strict_edit = strict_edit;
    g_cached_controller_state = controller_state;
    g_cached_capture = capture;
    g_cached_async_blocks = async_blocks;
    snprintf(g_cached_scene_name, sizeof(g_cached_scene_name), "%s", scene);
    pthread_mutex_unlock(&g_scene_state_lock);

    if (strcmp(scene, last_scene) != 0 ||
        editor_present != last_editor_present ||
        editor_play_mode != last_editor_play_mode ||
        is_dlc_level != last_is_dlc_level ||
        paused_in_play_mode != last_paused_in_play_mode ||
        strict_edit != last_strict_edit ||
        controller_state != last_controller_state ||
        cached_controller_state != last_cached_controller_state ||
        destination_state != last_destination_state ||
        capture != last_capture) {
        snprintf(last_scene, sizeof(last_scene), "%s", scene);
        last_editor_present = editor_present;
        last_editor_play_mode = editor_play_mode;
        last_is_dlc_level = is_dlc_level;
        last_paused_in_play_mode = paused_in_play_mode;
        last_strict_edit = strict_edit;
        last_controller_state = controller_state;
        last_cached_controller_state = cached_controller_state;
        last_destination_state = destination_state;
        last_capture = capture;
        LOGI("SCENE state reason=%s scene=%s isScnGame=%d isLevelEditor=%d isDLC=%d gameworld=%d editor=%d editorPlay=%d pausedInPlay=%d strictEdit=%d controllerState=%d cachedState=%d destState=%d capture=%d asyncBlocks=%d",
             reason ? reason : "unknown",
             scene,
             is_scn_game,
             is_level_editor,
             is_dlc_level,
             gameworld,
             editor_present,
             editor_play_mode,
             paused_in_play_mode,
             strict_edit,
             controller_state,
             cached_controller_state,
             destination_state,
             capture,
             async_blocks);
    }
}

static void __attribute__((unused)) log_scene_state_if_changed(const char *reason, void *controller_self) {
    (void)controller_self;
    pthread_mutex_lock(&g_scene_state_lock);
    int ready = g_scene_state_ready;
    int editor_present = g_cached_editor_present;
    int editor_play_mode = g_cached_editor_play_mode;
    int is_scn_game = g_cached_is_scn_game;
    int is_level_editor = g_cached_is_level_editor;
    int is_dlc_level = g_cached_is_dlc_level;
    int gameworld = g_cached_gameworld;
    int paused_in_play_mode = g_cached_paused_in_play_mode;
    int strict_edit = g_cached_strict_edit;
    int controller_state = g_cached_controller_state;
    int capture = g_cached_capture;
    int async_blocks = g_cached_async_blocks;
    char scene[128];
    snprintf(scene, sizeof(scene), "%s", g_cached_scene_name[0] ? g_cached_scene_name : "<unknown>");
    pthread_mutex_unlock(&g_scene_state_lock);

    if (!ready) {
        return;
    }

    static int last_editor_present = -1;
    static int last_editor_play_mode = -1;
    static int last_is_dlc_level = -1;
    static int last_paused_in_play_mode = -1;
    static int last_strict_edit = -1;
    static int last_controller_state = -999;
    static int last_capture = -1;
    static int last_async_blocks = -1;
    static char last_scene[128] = {0};

    if (strcmp(scene, last_scene) != 0 ||
        editor_present != last_editor_present ||
        editor_play_mode != last_editor_play_mode ||
        is_dlc_level != last_is_dlc_level ||
        paused_in_play_mode != last_paused_in_play_mode ||
        strict_edit != last_strict_edit ||
        controller_state != last_controller_state ||
        capture != last_capture ||
        async_blocks != last_async_blocks) {
        snprintf(last_scene, sizeof(last_scene), "%s", scene);
        last_editor_present = editor_present;
        last_editor_play_mode = editor_play_mode;
        last_is_dlc_level = is_dlc_level;
        last_paused_in_play_mode = paused_in_play_mode;
        last_strict_edit = strict_edit;
        last_controller_state = controller_state;
        last_capture = capture;
        last_async_blocks = async_blocks;
        LOGI("SCENE cached reason=%s scene=%s isScnGame=%d isLevelEditor=%d isDLC=%d gameworld=%d editor=%d editorPlay=%d pausedInPlay=%d strictEdit=%d controllerState=%d capture=%d asyncBlocks=%d",
             reason ? reason : "unknown",
             scene,
             is_scn_game,
             is_level_editor,
             is_dlc_level,
             gameworld,
             editor_present,
             editor_play_mode,
             paused_in_play_mode,
             strict_edit,
             controller_state,
             capture,
             async_blocks);
    }
}

static void ensure_async_clock_fields(uint64_t tick) {
    if (!g_enabled || tick == 0) {
        return;
    }
    if (g_field_async_curr_frame_tick == NULL ||
        g_field_async_prev_frame_tick == NULL ||
        g_field_async_offset_tick == NULL ||
        g_field_async_offset_tick_updated == NULL) {
        ensure_metadata_ready_lazy();
    }

    double dsp_time = read_dsp_time_or_zero();
    uint64_t offset_tick = 0;
    if (dsp_time > 0.0) {
        uint64_t dsp_tick = (uint64_t)(dsp_time * 10000000.0);
        if (tick > dsp_tick) {
            offset_tick = tick - dsp_tick;
        }
    }

    pthread_mutex_lock(&g_async_clock_lock);
    uint64_t prev = g_last_async_frame_tick ? g_last_async_frame_tick : tick;
    g_last_async_frame_tick = tick;
    pthread_mutex_unlock(&g_async_clock_lock);

    write_static_u64_field(g_field_async_prev_frame_tick, prev);
    write_static_u64_field(g_field_async_curr_frame_tick, tick);
    write_static_u64_field(g_field_async_offset_tick, offset_tick);
    write_static_bool_field(g_field_async_offset_tick_updated, 1);
    update_current_frame_tick(tick);
}

static void enqueue_event(int type, int source_id, uint64_t raw_ns) {
    if (!g_enabled || !capture_gate_open()) {
        return;
    }

    pthread_mutex_lock(&g_lock);

    if (!g_enabled || !g_capture_ready || g_last_playercontrol_wall_tick == 0) {
        pthread_mutex_unlock(&g_lock);
        return;
    }

    int synthetic_auto = (source_id == SYNTHETIC_AUTO_SOURCE_ID);
    if (!synthetic_auto) {
        int physical_index = physical_source_index_locked(source_id);
        if (type == EVENT_DOWN) {
            if (physical_index >= 0) {
                LOGW("duplicate DOWN source=%d, resetting async input state", source_id);
                clear_runtime_state_locked();
            }
            if (g_physical_count < MAX_HELD_SOURCES) {
                g_physical_sources[g_physical_count++] = source_id;
            } else {
                pthread_mutex_unlock(&g_lock);
                return;
            }
        } else {
            if (physical_index < 0) {
                pthread_mutex_unlock(&g_lock);
                return;
            }
            remove_source_at_locked(g_physical_sources, &g_physical_count, physical_index);
        }
    }

    if (queue_active_count_locked() == 0) {
        g_queue_head = 0;
        g_queue_count = 0;
    } else if (g_queue_count == MAX_EVENTS && g_queue_head > 0) {
        compact_queue_locked();
    }

    if (g_queue_count == MAX_EVENTS) {
        LOGW("event queue overflow, resetting async input state");
        clear_runtime_state_locked();
        if (type == EVENT_DOWN) {
            if (!synthetic_auto && g_physical_count < MAX_HELD_SOURCES) {
                g_physical_sources[g_physical_count++] = source_id;
            }
        } else {
            pthread_mutex_unlock(&g_lock);
            return;
        }
    }

    int pos = g_queue_count++;
    while (pos > g_queue_head && g_queue[pos - 1].tick > raw_ns) {
        g_queue[pos] = g_queue[pos - 1];
        pos--;
    }
    g_queue[pos].tick = raw_ns;
    g_queue[pos].type = type;
    g_queue[pos].source_id = source_id;
    g_queue[pos].synthetic_auto = synthetic_auto;
    int queue_count_after = g_queue_count;
    pthread_mutex_unlock(&g_lock);

    if (g_trace_enabled && type == EVENT_DOWN) {
        LOGI("enqueue %sDOWN raw_ns=%" PRIu64 " src=%d queue=%d",
             synthetic_auto ? "AUTO " : "", raw_ns, source_id, queue_count_after);
    } else if (g_trace_enabled && type == EVENT_UP) {
        LOGI("enqueue %sUP raw_ns=%" PRIu64 " src=%d queue=%d",
             synthetic_auto ? "AUTO " : "", raw_ns, source_id, queue_count_after);
    }
}

static int prepare_replay_step(uint64_t target_tick, uint64_t *step_limit) {
    if (step_limit == NULL) {
        return 0;
    }

    pthread_mutex_lock(&g_lock);
    if (queue_active_count_locked() <= 0) {
        g_queue_head = 0;
        g_queue_count = 0;
        g_replay_cursor_ready = 0;
        g_replay_cursor_tick = 0;
        pthread_mutex_unlock(&g_lock);
        return 0;
    }

    uint64_t next_tick = g_queue[g_queue_head].tick;
    if (!g_replay_cursor_ready || g_replay_cursor_tick < next_tick) {
        g_replay_cursor_tick = next_tick;
        g_replay_cursor_ready = 1;
    }

    if (g_replay_cursor_tick > target_tick) {
        pthread_mutex_unlock(&g_lock);
        return 0;
    }

    uint64_t limit = g_replay_cursor_tick + REPLAY_TICK_STEP_TICKS - 1ULL;
    if (limit < g_replay_cursor_tick || limit > target_tick) {
        limit = target_tick;
    }

    g_replay_cursor_tick = (limit == UINT64_MAX) ? UINT64_MAX : limit + 1ULL;
    *step_limit = limit;
    pthread_mutex_unlock(&g_lock);
    return 1;
}

static int pop_events_for_tick(uint64_t max_tick, uint64_t *tick, int *down_count, int *up_count, int *held_count, int *synthetic_auto_count) {
    pthread_mutex_lock(&g_lock);
    if (queue_active_count_locked() <= 0) {
        g_queue_head = 0;
        g_queue_count = 0;
        g_replay_cursor_ready = 0;
        g_replay_cursor_tick = 0;
        pthread_mutex_unlock(&g_lock);
        return 0;
    }

    uint64_t t = g_queue[g_queue_head].tick;
    if (t > max_tick) {
        int qcount_dbg = queue_active_count_locked();
        pthread_mutex_unlock(&g_lock);
        static uint64_t last_block_log_ns = 0;
        uint64_t now_ns_dbg = monotonic_ns_now();
        if (g_trace_enabled && now_ns_dbg - last_block_log_ns >= 500000000ULL) {
            last_block_log_ns = now_ns_dbg;
            LOGI("pop blocked next_tick=%" PRIu64 " max_tick=%" PRIu64 " diff_ticks=%" PRId64 " queue=%d",
                 t, max_tick, (int64_t)(t - max_tick), qcount_dbg);
        }
        return 0;
    }

    int first_type = g_queue[g_queue_head].type;
    int allow_down_burst = first_type == EVENT_DOWN && !g_queue[g_queue_head].synthetic_auto;
    uint64_t burst_limit = t;
    if (allow_down_burst) {
        burst_limit = t + MULTITAP_BURST_NS;
        if (burst_limit < t) {
            burst_limit = UINT64_MAX;
        }
        if (burst_limit > max_tick) {
            burst_limit = max_tick;
        }
    }

    int down = 0;
    int up = 0;
    int synthetic_auto = 0;
    uint64_t down_slot_mask = 0;
    uint64_t up_slot_mask = 0;
    int consume = g_queue_head;
    while (consume < g_queue_count) {
        int same_tick = g_queue[consume].tick == t;
        int same_burst_down = allow_down_burst &&
                              g_queue[consume].type == EVENT_DOWN &&
                              !g_queue[consume].synthetic_auto &&
                              g_queue[consume].tick <= burst_limit;
        if (!same_tick && !same_burst_down) {
            break;
        }
        int source_id = g_queue[consume].source_id;
        if (g_queue[consume].synthetic_auto) {
            synthetic_auto++;
        }
        int logical_index = logical_source_index_locked(source_id);
        if (g_queue[consume].type == EVENT_DOWN) {
            if (logical_index < 0 && g_held_count < MAX_HELD_SOURCES) {
                int slot = allocate_held_slot_locked();
                if (slot >= 0) {
                    g_held_sources[g_held_count] = source_id;
                    g_held_slots[g_held_count] = slot;
                    g_held_count++;
                    down++;
                    down_slot_mask |= 1ULL << slot;
                }
            }
        } else if (g_queue[consume].type == EVENT_UP) {
            if (logical_index >= 0) {
                int slot = g_held_slots[logical_index];
                remove_held_source_at_locked(logical_index);
                up++;
                if (slot >= 0 && slot < ASYNC_KEY_SLOT_COUNT) {
                    up_slot_mask |= 1ULL << slot;
                }
            }
        }
        consume++;
    }

    g_queue_head = consume;
    if (g_queue_head == g_queue_count) {
        g_queue_head = 0;
        g_queue_count = 0;
    }
    g_down_count = down;
    g_up_count = up;
    g_down_slot_mask = down_slot_mask;
    g_up_slot_mask = up_slot_mask;
    g_held_slot_mask = held_slot_mask_locked();
    g_last_processed_tick = t;

    *tick = t;
    *down_count = down;
    *up_count = up;
    if (held_count != NULL) {
        *held_count = g_held_count;
    }
    if (synthetic_auto_count != NULL) {
        *synthetic_auto_count = synthetic_auto;
    }
    pthread_mutex_unlock(&g_lock);
    return 1;
}

static int snapshot_down(void) {
    pthread_mutex_lock(&g_lock);
    int value = g_down_count;
    pthread_mutex_unlock(&g_lock);
    return value;
}

static int snapshot_up(void) {
    pthread_mutex_lock(&g_lock);
    int value = g_up_count;
    pthread_mutex_unlock(&g_lock);
    return value;
}

static int snapshot_held(void) {
    pthread_mutex_lock(&g_lock);
    int value = g_held_count;
    pthread_mutex_unlock(&g_lock);
    return value;
}

static void snapshot_slot_masks(uint64_t *down_mask, uint64_t *up_mask, uint64_t *held_mask) {
    pthread_mutex_lock(&g_lock);
    if (down_mask != NULL) {
        *down_mask = g_down_slot_mask;
    }
    if (up_mask != NULL) {
        *up_mask = g_up_slot_mask;
    }
    if (held_mask != NULL) {
        *held_mask = g_held_slot_mask;
    }
    pthread_mutex_unlock(&g_lock);
}

static void clear_frame_edges(void) {
    pthread_mutex_lock(&g_lock);
    g_down_count = 0;
    g_up_count = 0;
    g_down_slot_mask = 0;
    g_up_slot_mask = 0;
    pthread_mutex_unlock(&g_lock);
}

static void sync_last_reported_target_tick(uint64_t tick) {
    if (g_field_async_last_reported_target_tick == NULL) {
        ensure_metadata_ready_lazy();
    }

    void *field = g_field_async_last_reported_target_tick;
    if (field != NULL && g_il2cpp_api.field_static_set_value != NULL) {
        g_il2cpp_api.field_static_set_value(field, &tick);
    }
}

static void hooked_update_offset_time(int64_t fix_divider, void *method) {
    if (!g_enabled) {
        if (g_original_update_offset_time != NULL) {
            g_original_update_offset_time(fix_divider, method);
        }
        return;
    }

    if (!capture_gate_open()) {
        if (!g_logged_update_offset_gate_closed) {
            g_logged_update_offset_gate_closed = 1;
            LOGI("UpdateOffsetTime passthrough while capture gate closed");
        }
        if (g_original_update_offset_time != NULL) {
            g_original_update_offset_time(fix_divider, method);
        }
        return;
    }
    g_logged_update_offset_gate_closed = 0;
    uint64_t tick = wall_tick_now_or_virtual();
    ensure_async_clock_fields(tick);
}

static void set_enabled_internal(int enabled, int persist) {
    int normalized = enabled ? 1 : 0;
    int changed = (g_enabled != normalized);
    g_enabled = normalized;
    if (!normalized) {
        pthread_mutex_lock(&g_lock);
        set_capture_gate_locked(0, 0);
        clear_runtime_state_locked();
        pthread_mutex_unlock(&g_lock);
        restore_regular_input_types();
        g_managed_masks_need_clear = 1;
        if (g_mask_api_ready) {
            async_masks_clear_all_if_ready();
        }
    } else {
        update_time_origin();
        ensure_input_thread_started();
        ingress_post_reset(0);
        g_last_update_input_frame = -1;
        g_last_update_input_controller = NULL;
    }
    if (persist) {
        save_bool_file(CFG_PATH, normalized);
    }
    if (changed) {
        LOGI("async input %s", normalized ? "ON" : "OFF");
    }
}

__attribute__((visibility("default")))
void ADOFAIAsyncInput_SetEnabled(int enabled) {
    set_enabled_internal(enabled, 1);
}

__attribute__((visibility("default")))
int ADOFAIAsyncInput_IsEnabled(void) {
    return g_enabled ? 1 : 0;
}

__attribute__((visibility("default")))
void ADOFAIAsyncInput_SetAutoReplayEnabled(int enabled) {
    g_auto_replay_enabled = enabled ? 1 : 0;
    save_bool_file(AUTO_REPLAY_CFG_PATH, g_auto_replay_enabled);
    LOGI("async auto replay %s", g_auto_replay_enabled ? "ON" : "OFF");
}

__attribute__((visibility("default")))
int ADOFAIAsyncInput_IsAutoReplayEnabled(void) {
    return g_auto_replay_enabled ? 1 : 0;
}

__attribute__((visibility("default")))
void ADOFAIAsyncInput_SetTraceEnabled(int enabled) {
    g_trace_enabled = enabled ? 1 : 0;
    save_bool_file(TRACE_CFG_PATH, g_trace_enabled);
    LOGI("async trace log %s", g_trace_enabled ? "ON" : "OFF");
}

__attribute__((visibility("default")))
int ADOFAIAsyncInput_IsTraceEnabled(void) {
    return g_trace_enabled ? 1 : 0;
}

static int is_special_key(int key_code, int meta_state) {
    if (key_code == KEYCODE_BACK || key_code == KEYCODE_ESCAPE || key_code == KEYCODE_SYSRQ || key_code == KEYCODE_F12) {
        return 1;
    }
    if (key_code == KEYCODE_ALT_LEFT || key_code == KEYCODE_ALT_RIGHT ||
        key_code == KEYCODE_META_LEFT || key_code == KEYCODE_META_RIGHT) {
        return 1;
    }
    if ((meta_state & 0x70000) != 0) { // META_CTRL_ON / LEFT / RIGHT
        return 1;
    }
    return 0;
}

static int should_accept_key(int key_code, int meta_state, int repeat_count) {
    if (repeat_count != 0) {
        return 0;
    }
    if (is_special_key(key_code, meta_state)) {
        return 0;
    }
    return 1;
}

static void init_input_api_once(void) {
    void *libandroid = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (libandroid == NULL) {
        LOGW("libandroid.so unavailable for native input timestamps: %s", dlerror());
        return;
    }

    g_motion_event_from_java = (InputEventFromJavaFn)dlsym(libandroid, "AMotionEvent_fromJava");
    g_key_event_from_java = (InputEventFromJavaFn)dlsym(libandroid, "AKeyEvent_fromJava");
    g_motion_event_get_time = (InputEventGetTimeFn)dlsym(libandroid, "AMotionEvent_getEventTime");
    g_key_event_get_time = (InputEventGetTimeFn)dlsym(libandroid, "AKeyEvent_getEventTime");
    g_input_event_release = (InputEventReleaseFn)dlsym(libandroid, "AInputEvent_release");
    g_input_event_get_source = (InputEventGetSourceFn)dlsym(libandroid, "AInputEvent_getSource");
    g_motion_event_get_action = (MotionEventGetActionFn)dlsym(libandroid, "AMotionEvent_getAction");
    g_motion_event_get_pointer_count = (MotionEventGetPointerCountFn)dlsym(libandroid, "AMotionEvent_getPointerCount");
    g_motion_event_get_pointer_id = (MotionEventGetPointerIdFn)dlsym(libandroid, "AMotionEvent_getPointerId");
    g_motion_event_get_x = (MotionEventGetCoordFn)dlsym(libandroid, "AMotionEvent_getX");
    g_motion_event_get_y = (MotionEventGetCoordFn)dlsym(libandroid, "AMotionEvent_getY");
    g_key_event_get_action = (KeyEventGetIntFn)dlsym(libandroid, "AKeyEvent_getAction");
    g_key_event_get_key_code = (KeyEventGetIntFn)dlsym(libandroid, "AKeyEvent_getKeyCode");
    g_key_event_get_meta_state = (KeyEventGetIntFn)dlsym(libandroid, "AKeyEvent_getMetaState");
    g_key_event_get_repeat_count = (KeyEventGetIntFn)dlsym(libandroid, "AKeyEvent_getRepeatCount");

    if (g_motion_event_from_java != NULL && g_key_event_from_java != NULL &&
        g_motion_event_get_time != NULL && g_key_event_get_time != NULL &&
        g_input_event_release != NULL) {
        LOGI("native input event timestamps enabled");
    } else {
        LOGW("native input event timestamps unavailable, falling back to Java getEventTime");
    }
}

static jmethodID safe_get_method(JNIEnv *env, jclass cls, const char *name, const char *sig) {
    jmethodID method = (*env)->GetMethodID(env, cls, name, sig);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return NULL;
    }
    return method;
}

static int ensure_motion_methods(JNIEnv *env, jobject event) {
    if (g_motion_methods.ready) {
        return 1;
    }

    pthread_mutex_lock(&g_jni_cache_lock);
    if (g_motion_methods.ready) {
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 1;
    }

    jclass cls = (*env)->GetObjectClass(env, event);
    if (cls == NULL) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 0;
    }

    g_motion_methods.getActionMasked = safe_get_method(env, cls, "getActionMasked", "()I");
    g_motion_methods.getActionIndex = safe_get_method(env, cls, "getActionIndex", "()I");
    g_motion_methods.getPointerId = safe_get_method(env, cls, "getPointerId", "(I)I");
    g_motion_methods.getSource = safe_get_method(env, cls, "getSource", "()I");
    g_motion_methods.getX = safe_get_method(env, cls, "getX", "(I)F");
    g_motion_methods.getY = safe_get_method(env, cls, "getY", "(I)F");

    if (g_motion_methods.getActionMasked == NULL ||
        g_motion_methods.getActionIndex == NULL ||
        g_motion_methods.getPointerId == NULL) {
        (*env)->DeleteLocalRef(env, cls);
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 0;
    }

    g_motion_methods.cls = (jclass)(*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (g_motion_methods.cls == NULL) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 0;
    }

    g_motion_methods.ready = 1;
    pthread_mutex_unlock(&g_jni_cache_lock);
    return 1;
}

static int ensure_key_methods(JNIEnv *env, jobject event) {
    if (g_key_methods.ready) {
        return 1;
    }

    pthread_mutex_lock(&g_jni_cache_lock);
    if (g_key_methods.ready) {
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 1;
    }

    jclass cls = (*env)->GetObjectClass(env, event);
    if (cls == NULL) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 0;
    }

    g_key_methods.getAction = safe_get_method(env, cls, "getAction", "()I");
    g_key_methods.getKeyCode = safe_get_method(env, cls, "getKeyCode", "()I");
    g_key_methods.getMetaState = safe_get_method(env, cls, "getMetaState", "()I");
    g_key_methods.getRepeatCount = safe_get_method(env, cls, "getRepeatCount", "()I");

    if (g_key_methods.getAction == NULL || g_key_methods.getKeyCode == NULL) {
        (*env)->DeleteLocalRef(env, cls);
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 0;
    }

    g_key_methods.cls = (jclass)(*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (g_key_methods.cls == NULL) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        pthread_mutex_unlock(&g_jni_cache_lock);
        return 0;
    }

    g_key_methods.ready = 1;
    pthread_mutex_unlock(&g_jni_cache_lock);
    return 1;
}

static uint64_t __attribute__((unused)) java_event_time_tick(JNIEnv *env, jobject event) {
    jclass cls = (*env)->GetObjectClass(env, event);
    if (cls == NULL) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        return wall_ticks_now();
    }

    jmethodID getEventTime = safe_get_method(env, cls, "getEventTime", "()J");
    if (getEventTime == NULL) {
        (*env)->DeleteLocalRef(env, cls);
        return wall_ticks_now();
    }

    jlong ms = (*env)->CallLongMethod(env, event, getEventTime);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return wall_ticks_now();
    }
    return uptime_ms_to_wall_ticks(ms);
}

static uint64_t java_event_time_ns(JNIEnv *env, jobject event) {
    jclass cls = (*env)->GetObjectClass(env, event);
    if (cls == NULL) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        return monotonic_ns_now();
    }

    jmethodID getEventTime = safe_get_method(env, cls, "getEventTime", "()J");
    if (getEventTime == NULL) {
        (*env)->DeleteLocalRef(env, cls);
        return monotonic_ns_now();
    }

    jlong ms = (*env)->CallLongMethod(env, event, getEventTime);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return monotonic_ns_now();
    }
    return (uint64_t)ms * 1000000ULL;
}

static uint64_t __attribute__((unused)) native_motion_tick(JNIEnv *env, jobject event, int *ok) {
    *ok = 0;
    pthread_once(&g_input_api_once, init_input_api_once);
    if (g_motion_event_from_java == NULL || g_motion_event_get_time == NULL || g_input_event_release == NULL) {
        return 0;
    }

    const void *native_event = g_motion_event_from_java(env, event);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return 0;
    }
    if (native_event == NULL) {
        return 0;
    }

    int64_t ns = g_motion_event_get_time(native_event);
    g_input_event_release(native_event);
    *ok = 1;
    return uptime_ns_to_wall_ticks(ns);
}

static uint64_t __attribute__((unused)) native_key_tick(JNIEnv *env, jobject event, int *ok) {
    *ok = 0;
    pthread_once(&g_input_api_once, init_input_api_once);
    if (g_key_event_from_java == NULL || g_key_event_get_time == NULL || g_input_event_release == NULL) {
        return 0;
    }

    const void *native_event = g_key_event_from_java(env, event);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return 0;
    }
    if (native_event == NULL) {
        return 0;
    }

    int64_t ns = g_key_event_get_time(native_event);
    g_input_event_release(native_event);
    *ok = 1;
    return uptime_ns_to_wall_ticks(ns);
}

static uint64_t __attribute__((unused)) motion_tick(JNIEnv *env, jobject event) {
    int ok = 0;
    uint64_t tick = native_motion_tick(env, event, &ok);
    return ok ? tick : java_event_time_tick(env, event);
}

static uint64_t __attribute__((unused)) key_tick(JNIEnv *env, jobject event) {
    int ok = 0;
    uint64_t tick = native_key_tick(env, event, &ok);
    return ok ? tick : java_event_time_tick(env, event);
}

static int native_motion_snapshot(JNIEnv *env, jobject event, MotionEventSnapshot *out) {
    if (out == NULL) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    pthread_once(&g_input_api_once, init_input_api_once);
    if (g_motion_event_from_java == NULL || g_motion_event_get_time == NULL ||
        g_input_event_release == NULL || g_motion_event_get_action == NULL ||
        g_motion_event_get_pointer_id == NULL) {
        return 0;
    }

    const void *native_event = g_motion_event_from_java(env, event);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return 0;
    }
    if (native_event == NULL) {
        return 0;
    }

    int packed_action = g_motion_event_get_action(native_event);
    int index = (packed_action >> 8) & 0xff;
    if (g_motion_event_get_pointer_count != NULL) {
        size_t pointer_count = g_motion_event_get_pointer_count(native_event);
        if (index < 0 || (size_t)index >= pointer_count) {
            g_input_event_release(native_event);
            return 0;
        }
    }

    out->action = packed_action & ACTION_MASK;
    out->index = index;
    out->pointer_id = g_motion_event_get_pointer_id(native_event, (size_t)index);
    out->source = g_input_event_get_source ? g_input_event_get_source(native_event) : SOURCE_TOUCHSCREEN;
    if (g_motion_event_get_x != NULL && g_motion_event_get_y != NULL) {
        out->x = g_motion_event_get_x(native_event, (size_t)index);
        out->y = g_motion_event_get_y(native_event, (size_t)index);
        out->has_xy = 1;
    }
    out->raw_ns = (uint64_t)g_motion_event_get_time(native_event);
    g_input_event_release(native_event);
    return 1;
}

static int native_key_snapshot(JNIEnv *env, jobject event, KeyEventSnapshot *out) {
    if (out == NULL) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    pthread_once(&g_input_api_once, init_input_api_once);
    if (g_key_event_from_java == NULL || g_key_event_get_time == NULL ||
        g_input_event_release == NULL || g_key_event_get_action == NULL ||
        g_key_event_get_key_code == NULL) {
        return 0;
    }

    const void *native_event = g_key_event_from_java(env, event);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return 0;
    }
    if (native_event == NULL) {
        return 0;
    }

    out->action = g_key_event_get_action(native_event);
    out->key_code = g_key_event_get_key_code(native_event);
    out->meta_state = g_key_event_get_meta_state ? g_key_event_get_meta_state(native_event) : 0;
    out->repeat_count = g_key_event_get_repeat_count ? g_key_event_get_repeat_count(native_event) : 0;
    out->raw_ns = (uint64_t)g_key_event_get_time(native_event);
    g_input_event_release(native_event);
    return 1;
}

static int is_mobile_ui_touch(float x, float y, int width, int height) {
    if (width <= 0 || height <= 0) {
        return 0;
    }

    float scale = (float)height / 1000.0f;
    float pause_area = scale * 200.0f;
    return x > (float)width - pause_area && y < pause_area;
}

static void add_ui_source(int source_id) {
    pthread_mutex_lock(&g_lock);
    if (ui_source_index_locked(source_id) < 0 && g_ui_count < MAX_HELD_SOURCES) {
        g_ui_sources[g_ui_count++] = source_id;
    }
    pthread_mutex_unlock(&g_lock);
}

static int remove_ui_source(int source_id) {
    int removed = 0;
    pthread_mutex_lock(&g_lock);
    int index = ui_source_index_locked(source_id);
    if (index >= 0) {
        remove_source_at_locked(g_ui_sources, &g_ui_count, index);
        removed = 1;
    }
    pthread_mutex_unlock(&g_lock);
    return removed;
}

static void __attribute__((unused)) add_suppressed_source(int source_id) {
    pthread_mutex_lock(&g_lock);
    if (suppressed_source_index_locked(source_id) < 0 && g_suppressed_count < MAX_HELD_SOURCES) {
        g_suppressed_sources[g_suppressed_count++] = source_id;
    }
    pthread_mutex_unlock(&g_lock);
}

static int remove_suppressed_source(int source_id) {
    int removed = 0;
    pthread_mutex_lock(&g_lock);
    int index = suppressed_source_index_locked(source_id);
    if (index >= 0) {
        remove_source_at_locked(g_suppressed_sources, &g_suppressed_count, index);
        removed = 1;
    }
    pthread_mutex_unlock(&g_lock);
    return removed;
}

static int has_suppressed_motion_sources(void) {
    int has_sources = 0;
    pthread_mutex_lock(&g_lock);
    for (int i = 0; i < g_suppressed_count; ++i) {
        int source = (g_suppressed_sources[i] >> 16) & 0xffff;
        if (source == SOURCE_TOUCHSCREEN || source == SOURCE_MOUSE) {
            has_sources = 1;
            break;
        }
    }
    pthread_mutex_unlock(&g_lock);
    return has_sources;
}

static int __attribute__((unused)) capture_start_suppresses_raw_ns(uint64_t raw_ns) {
    pthread_mutex_lock(&g_lock);
    uint64_t suppress_until = g_capture_suppress_until_raw_ns;
    pthread_mutex_unlock(&g_lock);
    return suppress_until != 0 && raw_ns <= suppress_until;
}

static void __attribute__((unused)) restore_regular_input_fallback(void) {
    restore_regular_input_types();
    g_managed_masks_need_clear = 1;
    if (g_mask_api_ready) {
        async_masks_clear_all_if_ready();
    }
}

static void log_touch_gate_limited(const char *reason, int action, int source_id, uint64_t raw_ns) {
    if (!g_trace_enabled) {
        return;
    }
    uint64_t now_ns = monotonic_ns_now();
    if (now_ns - g_last_touch_gate_log_ns < 500000000ULL) {
        return;
    }
    g_last_touch_gate_log_ns = now_ns;
    LOGI("touch gate %s action=%d source=%d raw=%" PRIu64 " enabled=%d capture=%d",
         reason ? reason : "unknown", action, source_id, raw_ns, g_enabled, capture_gate_open());
}

static void clear_ui_sources(void) {
    pthread_mutex_lock(&g_lock);
    g_ui_count = 0;
    pthread_mutex_unlock(&g_lock);
}

static int close_capture_if_controller_paused_now(void) {
    void *controller = read_adobase_controller();
    if (controller != NULL && controller_is_paused(controller)) {
        if (capture_gate_open()) {
            close_async_capture();
        }
        return 1;
    }
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_fizzd_connectedworlds_editorport_ExtraMenuUnityPlayerActivity_nativeOnTouchEvent(JNIEnv *env, jclass clazz, jobject event, jint view_width, jint view_height) {
    (void)clazz;
    if (event == NULL || !g_enabled) {
        return JNI_FALSE;
    }
    ensure_input_thread_started();
    if (close_capture_if_controller_paused_now()) {
        return JNI_FALSE;
    }

    MotionEventSnapshot native_event;
    int native_ok = native_motion_snapshot(env, event, &native_event);
    int action = 0;
    int index = 0;
    int pointer_id = 0;
    int source = SOURCE_TOUCHSCREEN;
    uint64_t raw_ns = 0;

    if (native_ok) {
        action = native_event.action;
        index = native_event.index;
        pointer_id = native_event.pointer_id;
        source = native_event.source;
        raw_ns = native_event.raw_ns;
    } else {
        if (!ensure_motion_methods(env, event)) {
            return JNI_FALSE;
        }
        action = (*env)->CallIntMethod(env, event, g_motion_methods.getActionMasked);
        index = (*env)->CallIntMethod(env, event, g_motion_methods.getActionIndex);
        pointer_id = (*env)->CallIntMethod(env, event, g_motion_methods.getPointerId, index);
        source = g_motion_methods.getSource ? (*env)->CallIntMethod(env, event, g_motion_methods.getSource) : SOURCE_TOUCHSCREEN;
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            return JNI_FALSE;
        }
    }
    int source_id = (source << 16) ^ (pointer_id & 0xffff);
    int was_suppressed = 0;

    if (action == ACTION_DOWN || action == ACTION_POINTER_DOWN) {
        float x = 0.0f;
        float y = 0.0f;
        int has_xy = 0;
        if (native_ok && native_event.has_xy) {
            x = native_event.x;
            y = native_event.y;
            has_xy = 1;
        } else if (!native_ok && g_motion_methods.getX != NULL && g_motion_methods.getY != NULL) {
            x = (*env)->CallFloatMethod(env, event, g_motion_methods.getX, index);
            y = (*env)->CallFloatMethod(env, event, g_motion_methods.getY, index);
            if ((*env)->ExceptionCheck(env)) {
                (*env)->ExceptionClear(env);
                return JNI_FALSE;
            }
            has_xy = 1;
        }
        if (has_xy) {
            if (is_mobile_ui_touch(x, y, (int)view_width, (int)view_height)) {
                add_ui_source(source_id);
                return JNI_FALSE;
            }
        }
    } else if (action == ACTION_CANCEL) {
        int consume_cancel = has_suppressed_motion_sources();
        clear_ui_sources();
        pthread_mutex_lock(&g_lock);
        clear_runtime_state_locked();
        pthread_mutex_unlock(&g_lock);
        g_managed_masks_need_clear = 1;
        if (g_mask_api_ready) {
            async_masks_clear_all_if_ready();
        }
        return consume_cancel ? JNI_TRUE : JNI_FALSE;
    } else if (action == ACTION_UP || action == ACTION_POINTER_UP) {
        was_suppressed = remove_suppressed_source(source_id);
        if (remove_ui_source(source_id)) {
            return JNI_FALSE;
        }
    } else if (action == ACTION_MOVE && has_suppressed_motion_sources()) {
        if (capture_gate_open()) {
            return JNI_TRUE;
        }
        pthread_mutex_lock(&g_lock);
        clear_runtime_state_locked();
        pthread_mutex_unlock(&g_lock);
        g_managed_masks_need_clear = 1;
        if (g_mask_api_ready) {
            async_masks_clear_all_if_ready();
        }
        return JNI_TRUE;
    }

    if (!capture_gate_open()) {
        log_touch_gate_limited("capture_closed", action, source_id, raw_ns);
        restore_regular_input_types();
        return was_suppressed ? JNI_TRUE : JNI_FALSE;
    }

    if (!native_ok) {
        raw_ns = java_event_time_ns(env, event);
    }
    if (!capture_accepts_raw_ns(raw_ns)) {
        return JNI_FALSE;
    }
    if (action == ACTION_DOWN || action == ACTION_POINTER_DOWN) {
        log_touch_gate_limited("post_down", action, source_id, raw_ns);
        if (ingress_post_event(EVENT_DOWN, source_id, raw_ns)) {
            add_suppressed_source(source_id);
            return JNI_TRUE;
        }
        return JNI_FALSE;
    } else if (action == ACTION_UP || action == ACTION_POINTER_UP) {
        if (!was_suppressed) {
            return JNI_FALSE;
        }
        log_touch_gate_limited("post_up", action, source_id, raw_ns);
        return ingress_post_event(EVENT_UP, source_id, raw_ns) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_fizzd_connectedworlds_editorport_ExtraMenuUnityPlayerActivity_nativeOnKeyEvent(JNIEnv *env, jclass clazz, jobject event) {
    (void)clazz;
    if (event == NULL || !g_enabled) {
        return JNI_FALSE;
    }
    ensure_input_thread_started();
    if (close_capture_if_controller_paused_now()) {
        return JNI_FALSE;
    }

    KeyEventSnapshot native_event;
    int native_ok = native_key_snapshot(env, event, &native_event);
    int action = 0;
    int key_code = 0;
    int meta_state = 0;
    int repeat_count = 0;
    uint64_t raw_ns = 0;

    if (native_ok) {
        action = native_event.action;
        key_code = native_event.key_code;
        meta_state = native_event.meta_state;
        repeat_count = native_event.repeat_count;
        raw_ns = native_event.raw_ns;
    } else {
        if (!ensure_key_methods(env, event)) {
            return JNI_FALSE;
        }
        action = (*env)->CallIntMethod(env, event, g_key_methods.getAction);
        key_code = (*env)->CallIntMethod(env, event, g_key_methods.getKeyCode);
        meta_state = g_key_methods.getMetaState ? (*env)->CallIntMethod(env, event, g_key_methods.getMetaState) : 0;
        repeat_count = g_key_methods.getRepeatCount ? (*env)->CallIntMethod(env, event, g_key_methods.getRepeatCount) : 0;
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            return JNI_FALSE;
        }
    }
    if (!should_accept_key(key_code, meta_state, repeat_count)) {
        return JNI_FALSE;
    }
    if (!capture_gate_open()) {
        restore_regular_input_types();
        return JNI_FALSE;
    }

    if (!native_ok) {
        raw_ns = java_event_time_ns(env, event);
    }
    int source_id = (SOURCE_KEYBOARD << 16) ^ (key_code & 0xffff);
    if (!capture_accepts_raw_ns(raw_ns)) {
        return JNI_FALSE;
    }
    int was_suppressed = 0;
    if (action == KEY_ACTION_DOWN) {
        if (ingress_post_event(EVENT_DOWN, source_id, raw_ns)) {
            add_suppressed_source(source_id);
            return JNI_TRUE;
        }
        return JNI_FALSE;
    } else if (action == KEY_ACTION_UP) {
        was_suppressed = remove_suppressed_source(source_id);
        if (!was_suppressed) {
            return JNI_FALSE;
        }
        return ingress_post_event(EVENT_UP, source_id, raw_ns) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_fizzd_connectedworlds_editorport_ExtraMenuUnityPlayerActivity_nativeOnLifecycleReset(JNIEnv *env, jclass clazz) {
    (void)env;
    (void)clazz;
    ensure_input_thread_started();
    ingress_post_reset(1);
}

JNIEXPORT void JNICALL
Java_com_fizzd_connectedworlds_editorport_ExtraMenuUnityPlayerActivity_nativeOnLifecyclePause(JNIEnv *env, jclass clazz) {
    (void)env;
    (void)clazz;
    ensure_input_thread_started();
    ingress_post_soft_pause(1);
}

JNIEXPORT void JNICALL
Java_com_fizzd_connectedworlds_editorport_ExtraMenuUnityPlayerActivity_nativeOnLifecycleResume(JNIEnv *env, jclass clazz) {
    (void)env;
    (void)clazz;
    ensure_input_thread_started();
    ingress_post_soft_resume();
}

static int hooked_async_is_active(void *method) {
    (void)method;
    int active = (g_enabled &&
                  g_force_async_active_for_original &&
                  g_in_playercontrol_original &&
                  capture_gate_open()) ? 1 : async_is_active_gate_allows();
    if (!active && g_enabled && !capture_gate_open()) {
        restore_regular_input_types();
    }
    static int last_active = -1;
    static uint64_t true_count = 0;
    if (active != last_active) {
        int ready = 0;
        int gameworld = 0;
        int is_scn_game = 0;
        int editor_play = 0;
        int is_dlc = 0;
        int controller_state = -999;
        char scene[128];
        scene_async_gate_snapshot(&ready,
                                  &gameworld,
                                  &is_scn_game,
                                  &editor_play,
                                  &is_dlc,
                                  &controller_state,
                                  scene,
                                  sizeof(scene));
        LOGI("AsyncInputManager.isActive gate -> %d scene=%s ready=%d gameworld=%d isScnGame=%d editorPlay=%d isDLC=%d controllerState=%d conductor=%d replay=%d mode=%d",
             active,
             scene,
             ready,
             gameworld,
             is_scn_game,
             editor_play,
             is_dlc,
             controller_state,
             g_in_conductor_frame,
             g_in_async_replay,
             g_replay_mode);
        last_active = active;
    } else if (active && g_trace_enabled) {
        uint64_t c = __atomic_add_fetch(&true_count, 1, __ATOMIC_RELAXED);
        if (c <= 8 || (c % 512ULL) == 0) {
            int ready = 0;
            int gameworld = 0;
            int is_scn_game = 0;
            int editor_play = 0;
            int is_dlc = 0;
            int controller_state = -999;
            char scene[128];
            scene_async_gate_snapshot(&ready,
                                      &gameworld,
                                      &is_scn_game,
                                      &editor_play,
                                      &is_dlc,
                                      &controller_state,
                                      scene,
                                      sizeof(scene));
            LOGI("AsyncInputManager.isActive true count=%" PRIu64 " scene=%s ready=%d gameworld=%d isScnGame=%d editorPlay=%d isDLC=%d controllerState=%d",
                 c,
                 scene,
                 ready,
                 gameworld,
                 is_scn_game,
                 editor_play,
                 is_dlc,
                 controller_state);
        }
    }
    return active;
}

static int async_query_active(void) {
    return g_enabled && g_in_async_replay &&
           g_replay_mode == REPLAY_MODE_LEGACY;
}

static int mask_replay_active(void) {
    return g_enabled && g_in_async_replay &&
           g_replay_mode == REPLAY_MODE_MASK;
}

static void enter_playercontrol_original(void) {
    __atomic_add_fetch(&g_in_playercontrol_original, 1, __ATOMIC_RELAXED);
}

static void leave_playercontrol_original(void) {
    int value = __atomic_sub_fetch(&g_in_playercontrol_original, 1, __ATOMIC_RELAXED);
    if (value < 0) {
        __atomic_store_n(&g_in_playercontrol_original, 0, __ATOMIC_RELAXED);
    }
}

static int __attribute__((unused)) hooked_valid_triggered(void *self, void *method) {
    if (mask_replay_active()) {
        return snapshot_down() > 0;
    }
    if (async_query_active()) {
        return snapshot_down() > 0;
    }
    return g_original_valid_triggered(self, method);
}

static int __attribute__((unused)) hooked_valid_released(void *self, void *method) {
    if (mask_replay_active()) {
        return snapshot_up() > 0;
    }
    if (async_query_active()) {
        return snapshot_up() > 0;
    }
    return g_original_valid_released(self, method);
}

static int __attribute__((unused)) hooked_count_valid_keys(void *self, void *method) {
    if (mask_replay_active() || async_query_active()) {
        int count = g_original_count_valid_keys(self, method);
        int replay_count = snapshot_down();
        return count > replay_count ? count : replay_count;
    }
    return g_original_count_valid_keys(self, method);
}

static int __attribute__((unused)) hooked_get_holding(void *self, void *method) {
    if (mask_replay_active() || async_query_active()) {
        return snapshot_held() > 0;
    }
    return g_original_get_holding(self, method);
}

static int __attribute__((unused)) hooked_rdinput_get_main(int state, void *method) {
    if (g_enabled && g_in_async_replay && g_replay_mode == REPLAY_MODE_LEGACY) {
        if (state == BUTTON_WENT_DOWN) {
            return snapshot_down();
        }
        if (state == BUTTON_WENT_UP) {
            return snapshot_up();
        }
        if (state == BUTTON_IS_DOWN) {
            return snapshot_held();
        }
        if (state == BUTTON_IS_UP) {
            return snapshot_held() == 0 ? 1 : 0;
        }
    }
    int result = g_original_rdinput_get_main(state, method);
    if (g_trace_enabled && g_enabled && g_in_async_replay && g_replay_mode == REPLAY_MODE_MASK) {
        uint64_t count = __atomic_add_fetch(&g_verify_rdinput_count, 1, __ATOMIC_RELAXED);
        if (count <= 32 || state == BUTTON_WENT_DOWN || (count % 128ULL) == 0) {
            LOGI("VERIFY RDInput.GetMain state=%d result=%d replay_mode=%d count=%" PRIu64 " maskCounts held=%d down=%d up=%d",
                 state,
                 result,
                 g_replay_mode,
                 count,
                 hashset_count_if_ready(g_async_key_mask),
                 hashset_count_if_ready(g_async_key_down_mask),
                 hashset_count_if_ready(g_async_key_up_mask));
        }
    }
    return result;
}

static int __attribute__((unused)) hooked_rdinput_get_main_press_count(void *method) {
    if (async_query_active()) {
        return snapshot_down();
    }
    return g_original_rdinput_get_main_press_count(method);
}

static int auto_replay_can_intercept(void) {
    return g_enabled &&
           g_auto_replay_enabled &&
           g_current_controller_self != NULL &&
           controller_allows_async_replay(g_current_controller_self);
}

typedef struct JudgementStateTrace {
    void *player;
    void *planetary_system;
    void *chosen_planet;
    void *currfloor;
    int floor_seq;
    int next_seq;
    int taps_needed;
    int taps_so_far;
    int hold_length;
    double hold_completion;
    double entry_time;
    double entry_time_pitch_adj;
    int key_times_count;
    int hold_keys_count;
    int key_total;
    int key_limiter_over_counter;
    int consec_multipress_counter;
    int taps_on_this_floor;
    int alive;
    int responsive;
    double last_hit;
    double actual_last_hit;
    double angle;
    double cached_angle;
    double snapped_last_angle;
    double target_exit_angle;
    double speed;
    int is_cw;
} JudgementStateTrace;

static void capture_judgement_state(void *player_self, JudgementStateTrace *out) {
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->floor_seq = -1;
    out->next_seq = -1;
    out->key_times_count = -1;
    out->hold_keys_count = -1;
    out->player = player_self;

    if (player_self == NULL) {
        return;
    }

    void *planetary_system = NULL;
    if (!read_instance_object_field(player_self, g_offset_scrplayer_planetary_system, &planetary_system)) {
        return;
    }
    out->planetary_system = planetary_system;
    (void)read_instance_double_field(planetary_system, g_offset_planetarysystem_speed, &out->speed);
    (void)read_instance_bool_field(planetary_system, g_offset_planetarysystem_is_cw, &out->is_cw);

    void *chosen_planet = NULL;
    if (!read_instance_object_field(planetary_system, g_offset_planetarysystem_chosen_planet, &chosen_planet)) {
        return;
    }
    out->chosen_planet = chosen_planet;
    (void)read_instance_object_field(chosen_planet, g_offset_scrplanet_currfloor, &out->currfloor);
    (void)read_instance_double_field(chosen_planet, g_offset_scrplanet_angle, &out->angle);
    (void)read_instance_double_field(chosen_planet, g_offset_scrplanet_cached_angle, &out->cached_angle);
    out->snapped_last_angle =
        read_self_double_method(chosen_planet, "", "scrPlanet", "get_snappedLastAngle", &g_scrplanet_get_snapped_last_angle_method, 0.0);
    out->target_exit_angle =
        read_self_double_method(chosen_planet, "", "scrPlanet", "get_targetExitAngle", &g_scrplanet_get_target_exit_angle_method, 0.0);

    if (out->currfloor != NULL) {
        void *nextfloor = NULL;
        (void)read_instance_int_field(out->currfloor, g_offset_scrfloor_seq_id, &out->floor_seq);
        (void)read_instance_int_field(out->currfloor, g_offset_scrfloor_taps_needed, &out->taps_needed);
        (void)read_instance_int_field(out->currfloor, g_offset_scrfloor_taps_so_far, &out->taps_so_far);
        (void)read_instance_int_field(out->currfloor, g_offset_scrfloor_hold_length, &out->hold_length);
        (void)read_instance_float_field(out->currfloor, g_offset_scrfloor_hold_completion, &out->hold_completion);
        (void)read_instance_double_field(out->currfloor, g_offset_scrfloor_entry_time, &out->entry_time);
        (void)read_instance_double_field(out->currfloor, g_offset_scrfloor_entry_time_pitch_adj, &out->entry_time_pitch_adj);
        if (read_instance_object_field(out->currfloor, g_offset_scrfloor_nextfloor, &nextfloor)) {
            (void)read_instance_int_field(nextfloor, g_offset_scrfloor_seq_id, &out->next_seq);
        }
    }

    (void)read_instance_bool_field(player_self, g_offset_scrplayer_alive, &out->alive);
    (void)read_instance_bool_field(player_self, g_offset_scrplayer_responsive, &out->responsive);
    (void)read_instance_double_field(player_self, g_offset_scrplayer_last_hit, &out->last_hit);
    (void)read_instance_double_field(player_self, g_offset_scrplayer_actual_last_hit, &out->actual_last_hit);
    (void)read_instance_int_field(player_self, g_offset_scrplayer_key_total, &out->key_total);
    (void)read_instance_int_field(player_self, g_offset_scrplayer_key_limiter_over_counter, &out->key_limiter_over_counter);
    (void)read_instance_int_field(player_self, g_offset_scrplayer_consec_multipress_counter, &out->consec_multipress_counter);
    (void)read_instance_int_field(player_self, g_offset_scrplayer_taps_on_this_floor, &out->taps_on_this_floor);

    void *key_times = NULL;
    if (read_instance_object_field(player_self, g_offset_scrplayer_key_times, &key_times)) {
        out->key_times_count = object_count_if_ready(key_times);
    }
    void *hold_keys = NULL;
    if (read_instance_object_field(player_self, g_offset_scrplayer_hold_keys, &hold_keys)) {
        out->hold_keys_count = object_count_if_ready(hold_keys);
    }
}

static void trace_judgement_state(const char *stage, void *player_self, int is_auto, int result_value) {
    if (!g_trace_enabled || !g_enabled || !g_in_async_replay || g_replay_mode != REPLAY_MODE_MASK) {
        return;
    }
    uint64_t trace_id = __atomic_load_n(&g_current_replay_trace_id, __ATOMIC_ACQUIRE);
    if (trace_id == 0) {
        return;
    }
    JudgementStateTrace state;
    capture_judgement_state(player_self, &state);
    uint64_t tick = __atomic_load_n(&g_current_replay_tick, __ATOMIC_ACQUIRE);
    int down = __atomic_load_n(&g_current_replay_down, __ATOMIC_ACQUIRE);
    int up = __atomic_load_n(&g_current_replay_up, __ATOMIC_ACQUIRE);
    int held = __atomic_load_n(&g_current_replay_held, __ATOMIC_ACQUIRE);
    int synthetic_auto = __atomic_load_n(&g_current_replay_is_synthetic_auto, __ATOMIC_ACQUIRE);
    int controller_state = controller_current_state(g_current_controller_self);
    LOGI("TRACE state stage=%s id=%" PRIu64
         " tick=%" PRIu64
         " down=%d up=%d held=%d isAuto=%d syntheticAuto=%d result=%d"
         " ctrlState=%d floor=%d next=%d taps=%d/%d holdLength=%d holdCompletion=%.6f"
         " keyTimes=%d holdKeys=%d keyTotal=%d limiter=%d consecMulti=%d tapsOnFloor=%d"
         " alive=%d responsive=%d lastHit=%.9f actualLastHit=%.9f"
         " angle=%.9f cached=%.9f snapped=%.9f target=%.9f speed=%.6f isCW=%d"
         " entryTime=%.9f entryTimePitchAdj=%.9f",
         stage ? stage : "?",
         trace_id,
         tick,
         down,
         up,
         held,
         is_auto,
         synthetic_auto,
         result_value,
         controller_state,
         state.floor_seq,
         state.next_seq,
         state.taps_so_far,
         state.taps_needed,
         state.hold_length,
         state.hold_completion,
         state.key_times_count,
         state.hold_keys_count,
         state.key_total,
         state.key_limiter_over_counter,
         state.consec_multipress_counter,
         state.taps_on_this_floor,
         state.alive,
         state.responsive,
         state.last_hit,
         state.actual_last_hit,
         state.angle,
         state.cached_angle,
         state.snapped_last_angle,
         state.target_exit_angle,
         state.speed,
         state.is_cw,
         state.entry_time,
         state.entry_time_pitch_adj);
}

static int hooked_scrplayer_get_auto(void *self, void *method) {
    if (g_enabled &&
        g_auto_replay_enabled &&
        g_in_async_replay &&
        __atomic_load_n(&g_current_replay_is_synthetic_auto, __ATOMIC_ACQUIRE)) {
        return 0;
    }
    return g_original_scrplayer_get_auto(self, method);
}

static int hooked_scrplayer_hit(void *self, int is_auto, void *method) {
    if (is_auto &&
        g_enabled &&
        g_auto_replay_enabled &&
        (__atomic_load_n(&g_in_auto_state_machine_replay, __ATOMIC_ACQUIRE) ||
         (g_in_async_replay &&
          __atomic_load_n(&g_current_replay_is_synthetic_auto, __ATOMIC_ACQUIRE)))) {
        return 0;
    }
    if (is_auto && auto_replay_can_intercept()) {
        if (consume_auto_hit_via_async_state_machine(self)) {
            return 0;
        }
        LOGW("AUTO replay state-machine injection failed; falling back to original Hit(isAuto)");
        trace_judgement_state("Hit.before.fallbackAuto", self, is_auto, -1);
        int fallback_result = g_original_scrplayer_hit(self, is_auto, method);
        trace_judgement_state("Hit.after.fallbackAuto", self, is_auto, fallback_result);
        return fallback_result;
    }
    if (is_auto &&
        g_enabled &&
        g_auto_replay_enabled &&
        g_in_async_replay &&
        __atomic_load_n(&g_current_replay_is_synthetic_auto, __ATOMIC_ACQUIRE)) {
        return 0;
    }
    trace_judgement_state("Hit.before", self, is_auto, -1);
    int result = g_original_scrplayer_hit(self, is_auto, method);
    trace_judgement_state("Hit.after", self, is_auto, result);
    return result;
}

static int project_planet_angle_from_tick(void *player, void *planet, uint64_t target_tick, int allow_restore_projection) {
    if (!g_enabled || player == NULL || target_tick == 0) {
        return 0;
    }
    if (g_in_async_replay && g_replay_mode != REPLAY_MODE_MASK) {
        return 0;
    }
    if (!g_in_async_replay && !allow_restore_projection) {
        return 0;
    }
    if (g_field_async_target_song_tick == NULL ||
        g_field_async_offset_tick == NULL ||
        g_field_async_offset_tick_updated == NULL ||
        g_offset_scrplayer_planetary_system == 0 ||
        g_offset_planetarysystem_chosen_planet == 0 ||
        g_offset_scrplanet_angle == 0) {
        ensure_metadata_ready_lazy();
    }

    int offset_updated = 0;
    if (!read_static_bool_field(g_field_async_offset_tick_updated, &offset_updated) ||
        !offset_updated) {
        return 0;
    }

    uint64_t offset_tick = 0;
    (void)read_static_u64_field(g_field_async_offset_tick, &offset_tick);
    uint64_t target_song_tick = target_tick > offset_tick ? target_tick - offset_tick : target_tick;
    write_static_u64_field(g_field_async_target_song_tick, target_song_tick);

    void *planetary_system = NULL;
    if (!read_instance_object_field(player, g_offset_scrplayer_planetary_system, &planetary_system)) {
        return 0;
    }
    if (planet == NULL &&
        !read_instance_object_field(planetary_system, g_offset_planetarysystem_chosen_planet, &planet)) {
        return 0;
    }
    if (planet == NULL) {
        return 0;
    }

    void *conductor = read_static_object_method("", "scrConductor", "get_instance", &g_scrconductor_get_instance_method);
    if (conductor == NULL) {
        return 0;
    }

    void *song = NULL;
    (void)read_instance_object_field(conductor, g_offset_scrconductor_song, &song);

    double last_hit = 0.0;
    double snapped_last_angle = read_self_double_method(
        planet,
        "",
        "scrPlanet",
        "get_snappedLastAngle",
        &g_scrplanet_get_snapped_last_angle_method,
        0.0);
    double speed = 1.0;
    double bpm = 0.0;
    double crotchet_at_start = 0.0;
    double dsp_time_song = 0.0;
    double add_offset = 0.0;
    int is_cw = 1;
    if (!read_instance_double_field(player, g_offset_scrplayer_last_hit, &last_hit) ||
        !read_instance_float_field(conductor, g_offset_scrconductor_bpm, &bpm) ||
        !read_instance_double_field(conductor, g_offset_scrconductor_dsp_time_song, &dsp_time_song) ||
        !read_instance_double_field(conductor, g_offset_scrconductor_addoffset, &add_offset)) {
        return 0;
    }
    (void)read_instance_double_field(conductor, g_offset_scrconductor_crotchet_at_start, &crotchet_at_start);
    if (crotchet_at_start == 0.0 && bpm > 0.0) {
        crotchet_at_start = 60.0 / bpm;
    }
    if (crotchet_at_start == 0.0) {
        return 0;
    }
    (void)read_instance_double_field(planetary_system, g_offset_planetarysystem_speed, &speed);
    (void)read_instance_bool_field(planetary_system, g_offset_planetarysystem_is_cw, &is_cw);

    double pitch = read_audio_source_pitch(song);
    if (pitch == 0.0) {
        pitch = 1.0;
    }
    double calibration_i = read_static_float_method(
        "",
        "scrConductor",
        "get_calibration_i",
        &g_scrconductor_get_calibration_i_method,
        0.0);
    double song_position = (((double)target_song_tick / 10000000.0) -
                            dsp_time_song -
                            calibration_i) *
                               pitch -
                           add_offset;
    double forced_angle =
        snapped_last_angle +
        (song_position - last_hit) / crotchet_at_start *
            3.141592653598793 *
            speed *
            (is_cw ? 1.0 : -1.0);

    double old_angle = 0.0;
    (void)read_instance_double_field(planet, g_offset_scrplanet_angle, &old_angle);
    if (!write_instance_double_field(planet, g_offset_scrplanet_angle, forced_angle)) {
        return 0;
    }

    static uint64_t force_count = 0;
    uint64_t count = __atomic_add_fetch(&force_count, 1, __ATOMIC_RELAXED);
    if (g_trace_enabled && (count <= 8 || (count % 512ULL) == 0)) {
        LOGI("FORCE AdjustAngle count=%" PRIu64
             " tick=%" PRIu64
             " targetSongTick=%" PRIu64
             " oldAngle=%.9f forcedAngle=%.9f songPos=%.9f lastHit=%.9f",
             count,
             target_tick,
             target_song_tick,
             old_angle,
             forced_angle,
             song_position,
             last_hit);
    }
    return 1;
}

static int force_adjust_angle_from_tick(void *player, uint64_t target_tick, int allow_restore_projection) {
    return project_planet_angle_from_tick(player, NULL, target_tick, allow_restore_projection);
}

static void hooked_adjust_angle(void *player, uint64_t target_tick, void *method) {
    trace_judgement_state("AdjustAngle.before", player, 0, -1);
    if (g_original_adjust_angle != NULL) {
        g_original_adjust_angle(player, target_tick, method);
    }
    uint64_t replay_tick = __atomic_load_n(&g_current_replay_tick, __ATOMIC_ACQUIRE);
    if (replay_tick != 0 && target_tick == replay_tick) {
        (void)force_adjust_angle_from_tick(player, target_tick, 0);
    }
    trace_judgement_state("AdjustAngle.after", player, 0, -1);
}

static void *hooked_scrplanet_switch_chosen(void *self, void *method) {
    void *player = NULL;
    (void)read_instance_object_field(self, g_offset_scrplanet_player, &player);
    trace_judgement_state("SwitchChosen.before", player, 0, -1);
    void *result = g_original_scrplanet_switch_chosen(self, method);
    uint64_t replay_tick = __atomic_load_n(&g_current_replay_tick, __ATOMIC_ACQUIRE);
    if (g_in_async_replay && g_replay_mode == REPLAY_MODE_MASK && replay_tick != 0 && player != NULL) {
        uint64_t frame_tick = current_async_frame_tick_or_now();
        (void)project_planet_angle_from_tick(player, result != NULL ? result : self, frame_tick, 1);
    }
    trace_judgement_state("SwitchChosen.after", player, 0, result != self ? 1 : 0);
    return result;
}

static void hooked_camera_update_follow_cam(void *self, int force, void *method) {
    uint64_t replay_tick = __atomic_load_n(&g_current_replay_tick, __ATOMIC_ACQUIRE);
    if (g_enabled && g_in_async_replay && g_replay_mode == REPLAY_MODE_MASK && replay_tick != 0) {
        void *controller_self = g_current_controller_self != NULL ? g_current_controller_self : read_adobase_controller();
        uint64_t frame_tick = current_async_frame_tick_or_now();
        restore_async_angle_to_tick(controller_self, frame_tick);
    }
    if (g_original_camera_update_follow_cam != NULL) {
        g_original_camera_update_follow_cam(self, force, method);
    }
}

static int hooked_get_hit_margin(float hitangle,
                                 float refangle,
                                 int is_cw,
                                 float bpm_times_speed,
                                 float conductor_pitch,
                                 double margin_scale,
                                 void *method) {
    if (__atomic_load_n(&g_forced_hit_margin_active, __ATOMIC_ACQUIRE)) {
        return __atomic_load_n(&g_forced_hit_margin_value, __ATOMIC_RELAXED);
    }
    int result = g_original_get_hit_margin(hitangle,
                                           refangle,
                                           is_cw,
                                           bpm_times_speed,
                                           conductor_pitch,
                                           margin_scale,
                                           method);
    if (g_trace_enabled && g_enabled && g_in_async_replay && g_replay_mode == REPLAY_MODE_MASK) {
        uint64_t trace_id = __atomic_load_n(&g_current_replay_trace_id, __ATOMIC_ACQUIRE);
        uint64_t tick = __atomic_load_n(&g_current_replay_tick, __ATOMIC_ACQUIRE);
        int down = __atomic_load_n(&g_current_replay_down, __ATOMIC_ACQUIRE);
        int up = __atomic_load_n(&g_current_replay_up, __ATOMIC_ACQUIRE);
        int held = __atomic_load_n(&g_current_replay_held, __ATOMIC_ACQUIRE);
        if (trace_id != 0 && down > 0) {
            double angle_delta_deg =
                (double)((hitangle - refangle) * (float)(is_cw ? 1 : -1) * 57.29578f);
            double offset_ms = 0.0;
            if (bpm_times_speed > 0.0f && conductor_pitch != 0.0f) {
                double crotchet = 60.0 / (double)bpm_times_speed;
                offset_ms =
                    (double)((hitangle - refangle) * (float)(is_cw ? 1 : -1)) /
                    3.1415927410125732 *
                    crotchet /
                    (double)conductor_pitch *
                    1000.0;
            }
            AdoOfficialConfig config;
            ado_official_config_defaults(&config);
            config.is_mobile = read_static_bool_method("", "ADOBase", "get_isMobile", &g_adobase_get_is_mobile_method, 1);
            config.use_old_auto = read_static_bool_method("", "RDC", "get_useOldAuto", &g_rdc_get_use_old_auto_method, 0);
            (void)read_static_int_field(g_field_gcs_difficulty, (int *)&config.difficulty);
            (void)read_static_int_field(g_field_gcs_hit_margin_limit, (int *)&config.hit_margin_limit);
            (void)read_static_float_field(g_field_gcs_current_speed_trial, &config.current_speed_trial);
            (void)read_static_float_field(g_field_gcs_hitmargin_counted, &config.hitmargin_counted);
            if (config.current_speed_trial <= 0.0) {
                config.current_speed_trial = 1.0;
            }
            if (config.hitmargin_counted <= 0.0) {
                config.hitmargin_counted = 60.0;
            }
            AdoOfficialJudgementOnly model_actual = ado_official_judge_angle_only(
                &config,
                (double)hitangle,
                (double)refangle,
                is_cw,
                (double)bpm_times_speed,
                (double)conductor_pitch,
                margin_scale);
            LOGI("TRACE official id=%" PRIu64
                 " tick=%" PRIu64
                 " mode=%d down=%d up=%d held=%d margin=%s(%d) offset_ms=%.3f angle_delta_deg=%.3f"
                 " modelMargin=%s(%d) modelOffset_ms=%.3f modelAngleDelta_deg=%.3f"
                 " hit=%.9f target=%.9f isCW=%d bpmTimesSpeed=%.6f pitch=%.6f marginScale=%.6f",
                 trace_id,
                 tick,
                 g_replay_mode,
                 down,
                 up,
                 held,
                 shadow_margin_name((AdoOfficialHitMargin)result),
                 result,
                 offset_ms,
                 angle_delta_deg,
                 shadow_margin_name(model_actual.margin),
                 (int)model_actual.margin,
                 model_actual.signed_time_offset_ms,
                 model_actual.signed_angle_delta_deg,
                 (double)hitangle,
                 (double)refangle,
                 is_cw,
                 (double)bpm_times_speed,
                 (double)conductor_pitch,
                 margin_scale);
        }
    }
    return result;
}

static int __attribute__((unused)) hooked_touch_enabled(void *self, void *method) {
    if (g_enabled && g_in_async_replay && g_replay_mode == REPLAY_MODE_MASK) {
        return 0;
    }
    return g_original_touch_enabled(self, method);
}

static int controller_current_state(void *controller_self) {
    if (controller_self == NULL) {
        return STATE_PLAYER_CONTROL;
    }
    int state = -1;
    void *state_machine = *(void **)((uintptr_t)controller_self + g_offset_statebehaviour_state_machine);
    if (state_machine != NULL) {
        void *current_mapping = *(void **)((uintptr_t)state_machine + g_offset_stateengine_current_state);
        if (current_mapping != NULL) {
            void *boxed_state = *(void **)((uintptr_t)current_mapping + g_offset_statemapping_state);
            if (boxed_state != NULL) {
                state = *(int *)((uintptr_t)boxed_state + g_offset_boxed_enum_value);
                return state;
            }
        }
    }
    return *(int *)((uintptr_t)controller_self + g_offset_scrcontroller_current_state);
}

static int controller_cached_state(void *controller_self) {
    if (controller_self == NULL) {
        return -1;
    }
    return *(int *)((uintptr_t)controller_self + g_offset_scrcontroller_current_state);
}

static int __attribute__((unused)) controller_is_paused(void *controller_self) {
    if (controller_self == NULL) {
        return 1;
    }
    return *(uint8_t *)((uintptr_t)controller_self + g_offset_scrcontroller_paused) ? 1 : 0;
}

static int try_controller_destination_state(void *controller_self, int *out_state) {
    if (controller_self == NULL || out_state == NULL) {
        return 0;
    }

    void *state_machine = *(void **)((uintptr_t)controller_self + g_offset_statebehaviour_state_machine);
    if (state_machine == NULL) {
        return 0;
    }

    void *destination_mapping = *(void **)((uintptr_t)state_machine + g_offset_stateengine_destination_state);
    if (destination_mapping == NULL) {
        return 0;
    }

    void *boxed_state = *(void **)((uintptr_t)destination_mapping + g_offset_statemapping_state);
    if (boxed_state == NULL) {
        return 0;
    }

    *out_state = *(int *)((uintptr_t)boxed_state + g_offset_boxed_enum_value);
    return 1;
}

static int controller_allows_async_capture(void *controller_self) {
    if (controller_self == NULL) {
        return 0;
    }
    if (controller_is_paused(controller_self)) {
        return 0;
    }
    if (!scene_allows_async_gameplay_cached()) {
        return 0;
    }

    return controller_current_state(controller_self) == STATE_PLAYER_CONTROL;
}

static int controller_allows_async_replay(void *controller_self) {
    if (controller_self == NULL) {
        return 0;
    }

    if (!controller_allows_async_capture(controller_self)) {
        return 0;
    }

    if (controller_current_state(controller_self) != STATE_PLAYER_CONTROL) {
        return 0;
    }

    return 1;
}

static void open_capture_for_controller(void *controller_self) {
    if (!g_enabled || !controller_allows_async_capture(controller_self)) {
        return;
    }
    if (dlc_async_disabled_cached()) {
        disable_async_for_dlc_if_needed("open_capture");
        return;
    }

    uint64_t now = wall_ticks_now();
    int starting_capture = 0;
    pthread_mutex_lock(&g_lock);
    if (!g_capture_ready) {
        clear_runtime_state_locked();
        starting_capture = 1;
    }
    set_capture_gate_locked(1, now);
    pthread_mutex_unlock(&g_lock);

    if (starting_capture) {
        g_capture_floor_raw_ns = monotonic_ns_now();
        pthread_mutex_lock(&g_lock);
        g_suppressed_count = 0;
        g_capture_suppress_until_raw_ns = g_capture_floor_raw_ns + CAPTURE_START_SUPPRESS_NS;
        pthread_mutex_unlock(&g_lock);
        int state = controller_current_state(controller_self);
        if (virtual_consume_resume_marker()) {
            LOGI("capture reopened after soft resume state=%d", state);
        } else {
            LOGI("capture opened, rebuilding virtual clock base state=%d", state);
            virtual_clock_reset(monotonic_ns_now());
        }
    }
}

static void reset_capture_for_new_session(void *controller_self, const char *reason) {
    if (!g_enabled || !controller_allows_async_capture(controller_self)) {
        return;
    }

    int state = controller_current_state(controller_self);
    if (state == g_last_session_boundary_state) {
        return;
    }
    g_last_session_boundary_state = state;

    uint64_t now = wall_ticks_now();
    pthread_mutex_lock(&g_lock);
    int dropped = queue_active_count_locked();
    clear_runtime_state_locked();
    set_capture_gate_locked(1, now);
    pthread_mutex_unlock(&g_lock);

    g_capture_floor_raw_ns = monotonic_ns_now();
    pthread_mutex_lock(&g_lock);
    g_capture_suppress_until_raw_ns = g_capture_floor_raw_ns + CAPTURE_START_SUPPRESS_NS;
    pthread_mutex_unlock(&g_lock);

    virtual_clock_reset(monotonic_ns_now());
    LOGI("capture reset for new session reason=%s state=%d dropped=%d",
         reason ? reason : "unknown",
         state,
         dropped);
}

static int __attribute__((unused)) capture_accepts_raw_ns(uint64_t raw_ns) {
    if (!capture_gate_open()) {
        return 0;
    }
    if (g_capture_floor_raw_ns == 0) {
        return 1;
    }
    return raw_ns > g_capture_floor_raw_ns;
}

static void call_process_key_inputs(void *controller_self, uint64_t tick, int replay_mode, int synthetic_auto) {
    if (controller_self == NULL || !ensure_process_key_inputs_ready()) {
        return;
    }

    int prev_in_async_replay = g_in_async_replay;
    int prev_replay_mode = g_replay_mode;
    uint64_t prev_trace_id = __atomic_load_n(&g_current_replay_trace_id, __ATOMIC_ACQUIRE);
    uint64_t prev_replay_tick = __atomic_load_n(&g_current_replay_tick, __ATOMIC_ACQUIRE);
    int prev_replay_down = __atomic_load_n(&g_current_replay_down, __ATOMIC_ACQUIRE);
    int prev_replay_up = __atomic_load_n(&g_current_replay_up, __ATOMIC_ACQUIRE);
    int prev_replay_held = __atomic_load_n(&g_current_replay_held, __ATOMIC_ACQUIRE);
    int prev_synthetic_auto = __atomic_load_n(&g_current_replay_is_synthetic_auto, __ATOMIC_ACQUIRE);

    uint64_t reported_tick = tick ? tick : replay_target_tick_now();
    uint64_t trace_id = 0;
    int trace_down = snapshot_down();
    int trace_up = snapshot_up();
    int trace_held = snapshot_held();
    if (tick != 0) {
        trace_id = __atomic_add_fetch(&g_replay_event_count, 1, __ATOMIC_RELAXED);
        if (g_trace_enabled && (trace_id <= 8 || (trace_id % 64ULL) == 0)) {
            LOGI("TRACE replay id=%" PRIu64 " tick=%" PRIu64 " mode=%d down=%d up=%d held=%d syntheticAuto=%d",
                 trace_id, tick, replay_mode, trace_down, trace_up, trace_held, synthetic_auto ? 1 : 0);
        }
        if (g_trace_enabled) {
            LOGI("TRACE call id=%" PRIu64 " tick=%" PRIu64 " mode=%d down=%d up=%d held=%d syntheticAuto=%d",
                 trace_id, tick, replay_mode, trace_down, trace_up, trace_held, synthetic_auto ? 1 : 0);
        }
    }
    ProcessKeyInputsFn fn = (ProcessKeyInputsFn)g_process_key_inputs_method.method_pointer;
    __atomic_store_n(&g_current_replay_trace_id, trace_id, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_tick, tick, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_down, trace_down, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_up, trace_up, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_held, trace_held, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_is_synthetic_auto, synthetic_auto ? 1 : 0, __ATOMIC_RELEASE);
    g_in_async_replay = 1;
    g_replay_mode = replay_mode;
    if (g_trace_enabled) {
        shadow_judgement_before_process(controller_self, tick, replay_mode);
    }
    fn(controller_self, tick, g_process_key_inputs_method.method_info);
    g_replay_mode = prev_replay_mode;
    g_in_async_replay = prev_in_async_replay;
    __atomic_store_n(&g_current_replay_trace_id, prev_trace_id, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_tick, prev_replay_tick, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_down, prev_replay_down, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_up, prev_replay_up, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_held, prev_replay_held, __ATOMIC_RELEASE);
    __atomic_store_n(&g_current_replay_is_synthetic_auto, prev_synthetic_auto, __ATOMIC_RELEASE);
    sync_last_reported_target_tick(reported_tick);
}

static uint64_t current_async_frame_tick_or_now(void) {
    uint64_t tick = 0;
    if (g_field_async_curr_frame_tick == NULL) {
        ensure_metadata_ready_lazy();
    }
    if (g_field_async_curr_frame_tick != NULL && read_static_u64_field(g_field_async_curr_frame_tick, &tick) && tick != 0) {
        return tick;
    }
    return replay_target_tick_now();
}

static uint64_t auto_target_tick_for_player(void *player_self, uint64_t fallback_tick) {
    if (player_self == NULL) {
        return fallback_tick;
    }
    if (g_field_async_offset_tick == NULL ||
        g_offset_scrplayer_planetary_system == 0 ||
        g_offset_planetarysystem_chosen_planet == 0) {
        ensure_metadata_ready_lazy();
    }

    void *planetary_system = NULL;
    void *chosen_planet = NULL;
    if (!read_instance_object_field(player_self, g_offset_scrplayer_planetary_system, &planetary_system) ||
        !read_instance_object_field(planetary_system, g_offset_planetarysystem_chosen_planet, &chosen_planet)) {
        return fallback_tick;
    }

    void *conductor = read_static_object_method("", "scrConductor", "get_instance", &g_scrconductor_get_instance_method);
    if (conductor == NULL) {
        return fallback_tick;
    }

    void *song = NULL;
    (void)read_instance_object_field(conductor, g_offset_scrconductor_song, &song);

    double last_hit = 0.0;
    double snapped_last_angle = read_self_double_method(
        chosen_planet,
        "",
        "scrPlanet",
        "get_snappedLastAngle",
        &g_scrplanet_get_snapped_last_angle_method,
        0.0);
    double target_exit_angle = read_self_double_method(
        chosen_planet,
        "",
        "scrPlanet",
        "get_targetExitAngle",
        &g_scrplanet_get_target_exit_angle_method,
        0.0);
    double speed = 1.0;
    double bpm = 0.0;
    double crotchet_at_start = 0.0;
    double dsp_time_song = 0.0;
    double add_offset = 0.0;
    int is_cw = 1;
    if (!read_instance_double_field(player_self, g_offset_scrplayer_last_hit, &last_hit) ||
        !read_instance_float_field(conductor, g_offset_scrconductor_bpm, &bpm) ||
        !read_instance_double_field(conductor, g_offset_scrconductor_dsp_time_song, &dsp_time_song) ||
        !read_instance_double_field(conductor, g_offset_scrconductor_addoffset, &add_offset)) {
        return fallback_tick;
    }
    (void)read_instance_double_field(conductor, g_offset_scrconductor_crotchet_at_start, &crotchet_at_start);
    if (crotchet_at_start == 0.0 && bpm > 0.0) {
        crotchet_at_start = 60.0 / bpm;
    }
    if (crotchet_at_start == 0.0) {
        return fallback_tick;
    }
    (void)read_instance_double_field(planetary_system, g_offset_planetarysystem_speed, &speed);
    (void)read_instance_bool_field(planetary_system, g_offset_planetarysystem_is_cw, &is_cw);
    if (speed == 0.0) {
        return fallback_tick;
    }

    double pitch = read_audio_source_pitch(song);
    if (pitch == 0.0) {
        pitch = 1.0;
    }
    double calibration_i = read_static_float_method(
        "",
        "scrConductor",
        "get_calibration_i",
        &g_scrconductor_get_calibration_i_method,
        0.0);
    int use_old_auto = read_static_bool_method("", "RDC", "get_useOldAuto", &g_rdc_get_use_old_auto_method, 0);
    double trigger_angle = target_exit_angle;
    if (use_old_auto) {
        const double old_auto_margin_rad = 10.0 * 3.141592653598793 / 180.0;
        trigger_angle = target_exit_angle + (is_cw ? -old_auto_margin_rad : old_auto_margin_rad);
    }
    double signed_angle_delta = (trigger_angle - snapped_last_angle) * (is_cw ? 1.0 : -1.0);
    double target_song_position =
        last_hit +
        signed_angle_delta / 3.141592653598793 / speed * crotchet_at_start;
    double target_song_tick_d =
        ((target_song_position + add_offset) / pitch + dsp_time_song + calibration_i) *
        10000000.0;
    if (!isfinite(target_song_tick_d) || target_song_tick_d <= 0.0) {
        return fallback_tick;
    }

    uint64_t offset_tick = 0;
    (void)read_static_u64_field(g_field_async_offset_tick, &offset_tick);
    uint64_t target_song_tick = (uint64_t)(target_song_tick_d + 0.5);
    uint64_t event_tick = offset_tick + target_song_tick;
    if (event_tick == 0) {
        return fallback_tick;
    }

    static uint64_t auto_tick_count = 0;
    uint64_t count = __atomic_add_fetch(&auto_tick_count, 1, __ATOMIC_RELAXED);
    if (g_trace_enabled && (count <= 16 || (count % 64ULL) == 0)) {
        LOGI("AUTO replay target tick count=%" PRIu64
             " fallback=%" PRIu64
             " exact=%" PRIu64
             " targetSongTick=%" PRIu64
             " targetSongPos=%.9f lastHit=%.9f snapped=%.9f target=%.9f trigger=%.9f oldAuto=%d",
             count,
             fallback_tick,
             event_tick,
             target_song_tick,
             target_song_position,
             last_hit,
             snapped_last_angle,
             target_exit_angle,
             trigger_angle,
             use_old_auto);
    }
    return event_tick;
}

static int consume_auto_hit_via_async_state_machine(void *player_self) {
    void *controller_self = g_current_controller_self != NULL ? g_current_controller_self : read_adobase_controller();
    if (controller_self == NULL || !controller_allows_async_replay(controller_self)) {
        return 0;
    }
    if (!ensure_mask_api_ready()) {
        return 0;
    }

    uint64_t frame_tick = current_async_frame_tick_or_now();
    ensure_async_clock_fields(frame_tick);
    uint64_t tick = auto_target_tick_for_player(player_self, frame_tick);
    force_async_input_types();
    async_masks_clear_tick_edges();

    int old_rdc_auto = 0;
    if (!read_rdc_auto(&old_rdc_auto) || !write_rdc_auto(0)) {
        LOGW("AUTO replay state-machine injection skipped; RDC.auto access unavailable");
        return 0;
    }

    int prev_down = 0;
    int prev_up = 0;
    int prev_held = 0;
    uint64_t prev_down_slot_mask = 0;
    uint64_t prev_up_slot_mask = 0;
    uint64_t prev_held_slot_mask = 0;
    pthread_mutex_lock(&g_lock);
    prev_down = g_down_count;
    prev_up = g_up_count;
    prev_held = g_held_count;
    prev_down_slot_mask = g_down_slot_mask;
    prev_up_slot_mask = g_up_slot_mask;
    prev_held_slot_mask = g_held_slot_mask;
    g_down_count = 1;
    g_up_count = 0;
    g_held_count = 1;
    g_down_slot_mask = 1ULL;
    g_up_slot_mask = 0;
    g_held_slot_mask = 1ULL;
    pthread_mutex_unlock(&g_lock);

    apply_primary_key_masks(1, 0, 1);
    __atomic_store_n(&g_in_auto_state_machine_replay, 1, __ATOMIC_RELEASE);
    call_process_key_inputs(controller_self, tick, REPLAY_MODE_MASK, 1);
    __atomic_store_n(&g_in_auto_state_machine_replay, 0, __ATOMIC_RELEASE);
    restore_async_angle_to_tick(controller_self, frame_tick);
    write_rdc_auto(old_rdc_auto);

    clear_frame_edges();
    async_masks_clear_tick_edges();
    pthread_mutex_lock(&g_lock);
    g_down_count = prev_down;
    g_up_count = prev_up;
    g_held_count = prev_held;
    g_down_slot_mask = prev_down_slot_mask;
    g_up_slot_mask = prev_up_slot_mask;
    g_held_slot_mask = prev_held_slot_mask;
    pthread_mutex_unlock(&g_lock);
    if (prev_down || prev_up || prev_held) {
        apply_primary_key_masks(prev_down, prev_up, prev_held);
    } else {
        apply_primary_key_masks(0, 0, 0);
    }
    suppress_async_camera_override_marker();

    static uint64_t auto_state_machine_count = 0;
    uint64_t count = __atomic_add_fetch(&auto_state_machine_count, 1, __ATOMIC_RELAXED);
    if (g_trace_enabled && (count <= 16 || (count % 64ULL) == 0)) {
        LOGI("AUTO replay consumed direct Hit(isAuto) via async state machine tick=%" PRIu64
             " count=%" PRIu64,
             tick,
             count);
    }
    return 1;
}

static void restore_async_angle_to_tick(void *controller_self, uint64_t tick) {
    if (controller_self == NULL || tick == 0) {
        return;
    }
    if (g_field_async_target_song_tick == NULL ||
        g_field_async_offset_tick == NULL ||
        !ensure_method_cache("", "scrController", "get_chosenPlanet", 0, &g_scrcontroller_get_chosen_planet_method)) {
        return;
    }

    uint64_t offset_tick = 0;
    (void)read_static_u64_field(g_field_async_offset_tick, &offset_tick);
    uint64_t target_song_tick = tick > offset_tick ? tick - offset_tick : tick;
    write_static_u64_field(g_field_async_target_song_tick, target_song_tick);

    GetObjectSelfFn get_chosen_planet = (GetObjectSelfFn)g_scrcontroller_get_chosen_planet_method.method_pointer;
    void *chosen_planet = get_chosen_planet(controller_self, g_scrcontroller_get_chosen_planet_method.method_info);
    if (chosen_planet == NULL) {
        return;
    }

    if (ensure_method_cache("", "scrPlanet", "AsyncRefreshAngles", 0, &g_scrplanet_async_refresh_angles_method)) {
        VoidSelfFn async_refresh_angles = (VoidSelfFn)g_scrplanet_async_refresh_angles_method.method_pointer;
        async_refresh_angles(chosen_planet, g_scrplanet_async_refresh_angles_method.method_info);
    }
    if (ensure_method_cache("", "scrPlanet", "Update_RefreshAngles", 0, &g_scrplanet_update_refresh_angles_method)) {
        VoidSelfFn update_refresh_angles = (VoidSelfFn)g_scrplanet_update_refresh_angles_method.method_pointer;
        update_refresh_angles(chosen_planet, g_scrplanet_update_refresh_angles_method.method_info);
    }

    void *player = NULL;
    if (read_instance_object_field(chosen_planet, g_offset_scrplanet_player, &player)) {
        (void)force_adjust_angle_from_tick(player, tick, 1);
    }
}

static void suppress_async_camera_override_marker(void) {
    if (g_field_scrplayer_should_replace_camy_to_pos == NULL) {
        return;
    }
    write_static_bool_field(g_field_scrplayer_should_replace_camy_to_pos, 0);
}

static int replay_pending_events_via_process_key_inputs(void *controller_self, uint64_t target_tick, int replay_mode) {
    if (!g_enabled || controller_self == NULL) {
        return 0;
    }
    if (dlc_async_disabled_cached()) {
        disable_async_for_dlc_if_needed("replay");
        return 0;
    }
    if (!controller_allows_async_replay(controller_self)) {
        if (!controller_allows_async_capture(controller_self)) {
            stop_capture_and_clear_queue();
        }
        return 0;
    }

    uint64_t event_raw_ns = 0;
    int down = 0;
    int up = 0;
    int held = 0;
    int synthetic_auto = 0;
    int replayed = 0;
    int had_pending = 0;
    uint64_t now_raw_ns = monotonic_ns_now();
    pthread_mutex_lock(&g_lock);
    had_pending = queue_active_count_locked() > 0;
    pthread_mutex_unlock(&g_lock);
    while (replayed < MAX_EVENTS && pop_events_for_tick(now_raw_ns, &event_raw_ns, &down, &up, &held, &synthetic_auto)) {
        if (!controller_allows_async_replay(controller_self)) {
            if (!controller_allows_async_capture(controller_self)) {
                stop_capture_and_clear_queue();
            }
            return replayed;
        }

        if (replay_mode == REPLAY_MODE_MASK) {
            if (replayed > 0) {
                async_masks_clear_tick_edges();
            }
            apply_primary_key_masks(down, up, held);
        }

        uint64_t event_tick = raw_ns_to_virtual_tick(event_raw_ns);
        call_process_key_inputs(controller_self, event_tick, replay_mode, synthetic_auto > 0);
        clear_frame_edges();
        replayed++;
    }
    if (replay_mode == REPLAY_MODE_MASK && replayed == 0 && !had_pending) {
        call_process_key_inputs(controller_self, 0, replay_mode, 0);
    } else if (replay_mode == REPLAY_MODE_MASK && replayed > 0) {
        restore_async_angle_to_tick(controller_self, target_tick);
    }
    if (replay_mode == REPLAY_MODE_MASK) {
        suppress_async_camera_override_marker();
    }
    if (had_pending && replayed == 0) {
        uint64_t now_ns = monotonic_ns_now();
        if (g_trace_enabled && now_ns - g_last_replay_empty_log_ns >= 500000000ULL) {
            g_last_replay_empty_log_ns = now_ns;
            LOGI("replay pending not due raw_now=%" PRIu64 " mode=%d", now_raw_ns, replay_mode);
        }
    }
    return replayed;
}

static void close_async_capture(void) {
    stop_capture_and_clear_queue();
    restore_regular_input_types();
    if (g_mask_api_ready) {
        async_masks_clear_all_if_ready();
    }
}

static void disable_async_for_dlc_if_needed(const char *reason) {
    if (!dlc_async_disabled_cached()) {
        return;
    }

    static int logged_disabled = 0;
    if (!logged_disabled) {
        logged_disabled = 1;
        LOGW("DLC async fuse active reason=%s; capture/masks/clock/replay disabled", reason ? reason : "unknown");
    }
    close_async_capture();
}

static void hooked_update_input(void *self, void *method) {
    if (!g_enabled) {
        if (g_original_update_input != NULL) {
            g_original_update_input(self, method);
        }
        return;
    }

    {
        static uint64_t g_hook_entry_count = 0;
        static uint64_t g_last_hook_count_log_ns = 0;
        uint64_t entries = __atomic_add_fetch(&g_hook_entry_count, 1, __ATOMIC_RELAXED);
        uint64_t now_count_ns = monotonic_ns_now();
        if (g_trace_enabled && g_last_hook_count_log_ns == 0) {
            g_last_hook_count_log_ns = now_count_ns;
        }
        if (g_trace_enabled && now_count_ns - g_last_hook_count_log_ns >= 1000000000ULL) {
            uint64_t prev_entries_ns = g_last_hook_count_log_ns;
            g_last_hook_count_log_ns = now_count_ns;
            static uint64_t g_last_hook_entries = 0;
            uint64_t delta = entries - g_last_hook_entries;
            g_last_hook_entries = entries;
            LOGI("hook entry rate %" PRIu64 "/s total=%" PRIu64 " elapsed_ms=%" PRIu64,
                 delta, entries, (uint64_t)((now_count_ns - prev_entries_ns) / 1000000ULL));
        }
    }

    g_current_controller_self = self;
    refresh_scene_state_on_main_thread("UpdateInput", self);
    if (dlc_async_disabled_cached()) {
        disable_async_for_dlc_if_needed("UpdateInput");
        g_current_controller_self = NULL;
        if (g_original_update_input != NULL) {
            g_original_update_input(self, method);
        }
        return;
    }
    if (!controller_allows_async_replay(self)) {
        if (controller_allows_async_capture(self)) {
            int state = controller_current_state(self);
            if (state == STATE_COUNTDOWN || state == STATE_CHECKPOINT) {
                reset_capture_for_new_session(self, "state boundary");
            } else {
                open_capture_for_controller(self);
            }
            ensure_async_clock_fields(wall_tick_now_or_virtual());
        } else if (capture_gate_open()) {
            close_async_capture();
        } else if (editor_blocks_async()) {
            restore_regular_input_types();
        }
        g_current_controller_self = NULL;
        return;
    }

    int frame_count = read_unity_frame_count_or_negative();
    int pending_before_frame_guard = queue_pending_count_snapshot();
    if (frame_count >= 0 &&
        pending_before_frame_guard <= 0 &&
        g_last_update_input_frame == frame_count &&
        g_last_update_input_controller == self) {
        g_current_controller_self = NULL;
        return;
    }
    if (frame_count >= 0) {
        g_last_update_input_frame = frame_count;
        g_last_update_input_controller = self;
    }

    open_capture_for_controller(self);
    uint64_t target_tick = wall_tick_now_or_virtual();
    ensure_async_clock_fields(target_tick);

    uint64_t now_ns = monotonic_ns_now();
    if (g_trace_enabled && now_ns - g_last_update_input_log_ns >= 1000000000ULL) {
        g_last_update_input_log_ns = now_ns;
        LOGI("UpdateInput async replay state=%d target=%" PRIu64 " queue_posted=%" PRIu64 " sealed=%" PRIu64 " replayed=%" PRIu64,
             controller_current_state(self),
             target_tick,
             g_ingress_event_posted_count,
             g_ingress_event_sealed_count,
             g_replay_event_count);
    }

    if (!ensure_mask_api_ready()) {
        if (now_ns - g_last_legacy_fallback_log_ns >= 1000000000ULL) {
            g_last_legacy_fallback_log_ns = now_ns;
            LOGW("mask API unavailable, using legacy replay fallback for this frame");
        }
        replay_pending_events_via_process_key_inputs(self, target_tick, REPLAY_MODE_LEGACY);
        call_process_key_inputs(self, 0, REPLAY_MODE_LEGACY, 0);
        g_current_controller_self = NULL;
        return;
    }

    force_async_input_types();

    if (g_managed_masks_need_clear) {
        async_masks_clear_all();
    }
    async_masks_clear_frame_edges();

    int replayed = replay_pending_events_via_process_key_inputs(self, target_tick, REPLAY_MODE_MASK);
    int pending_after_replay = 0;
    pthread_mutex_lock(&g_lock);
    pending_after_replay = queue_active_count_locked();
    pthread_mutex_unlock(&g_lock);
    if (replayed == 0 && pending_after_replay > 0) {
        uint64_t now_wait_ns = monotonic_ns_now();
        if (g_trace_enabled && now_wait_ns - g_last_replay_empty_log_ns >= 500000000ULL) {
            g_last_replay_empty_log_ns = now_wait_ns;
            LOGI("no due async event this frame; ProcessKeyInputs skipped target=%" PRIu64,
                 target_tick);
        }
    }

    g_current_controller_self = NULL;
}

static void stop_capture_and_clear_queue(void) {
    pthread_mutex_lock(&g_lock);
    set_capture_gate_locked(0, 0);
    g_capture_floor_raw_ns = 0;
    g_last_session_boundary_state = -1;
    clear_runtime_state_locked();
    pthread_mutex_unlock(&g_lock);
    g_managed_masks_need_clear = 1;
    if (g_mask_api_ready) {
        async_masks_clear_all_if_ready();
    }
}

static int __attribute__((unused)) replay_pending_events(void *player_self) {
    if (!g_enabled || player_self == NULL || g_original_simulated_update == NULL) {
        return 0;
    }
    if (g_in_async_replay) {
        return 0;
    }
    if (!controller_allows_async_replay(g_current_controller_self)) {
        if (!controller_allows_async_capture(g_current_controller_self)) {
            stop_capture_and_clear_queue();
        }
        return 0;
    }

    uint64_t tick = 0;
    int down = 0;
    int up = 0;
    int replayed = 0;
    int steps = 0;
    uint64_t target_tick = replay_target_tick_now();
    uint64_t step_limit = 0;
    while (steps < REPLAY_MAX_STEPS_PER_FRAME && prepare_replay_step(target_tick, &step_limit)) {
        steps++;
        while (pop_events_for_tick(step_limit, &tick, &down, &up, NULL, NULL)) {
            (void)down;
            (void)up;
            if (!controller_allows_async_replay(g_current_controller_self)) {
                if (!controller_allows_async_capture(g_current_controller_self)) {
                    stop_capture_and_clear_queue();
                }
                return replayed;
            }
            g_in_async_replay = 1;
            g_original_simulated_update(player_self, 1, tick, NULL);
            g_in_async_replay = 0;
            sync_last_reported_target_tick(tick);
            clear_frame_edges();
            replayed++;
            if (!controller_allows_async_replay(g_current_controller_self)) {
                if (!controller_allows_async_capture(g_current_controller_self)) {
                    stop_capture_and_clear_queue();
                }
                return replayed;
            }
            if (replayed >= MAX_EVENTS) {
                return replayed;
            }
        }
        if (replayed >= MAX_EVENTS) {
            break;
        }
    }
    return replayed;
}

static void __attribute__((unused)) hooked_simulated_update(void *self, uint64_t has_value, uint64_t tick, void *method) {
    if (g_trace_enabled && g_enabled && g_in_async_replay && g_replay_mode == REPLAY_MODE_MASK) {
        uint64_t count = __atomic_add_fetch(&g_verify_simulated_count, 1, __ATOMIC_RELAXED);
        if (count <= 32 || (count % 128ULL) == 0) {
            LOGI("VERIFY Simulated_PlayerControl_Update entered count=%" PRIu64 " has_value=%" PRIu64 " tick=%" PRIu64,
                 count, has_value, tick);
        }
    }
    if (g_enabled && g_in_playercontrol_frame && !g_in_async_replay) {
        if (controller_allows_async_replay(g_current_controller_self)) {
            return;
        }
        if (!controller_allows_async_capture(g_current_controller_self)) {
            stop_capture_and_clear_queue();
        }
        return;
    }
    if (g_enabled && g_in_async_replay && !controller_allows_async_replay(g_current_controller_self)) {
        if (!controller_allows_async_capture(g_current_controller_self)) {
            stop_capture_and_clear_queue();
        }
        return;
    }
    g_original_simulated_update(self, has_value, tick, method);
}

static void __attribute__((unused)) hooked_conductor_update(void *self, void *method) {
    g_in_conductor_frame = 1;
    if (g_enabled) {
        void *controller = read_adobase_controller();
        if (controller != NULL) {
            refresh_scene_state_on_main_thread("scrConductor.Update.pre", controller);
            disable_async_for_dlc_if_needed("scrConductor.Update.pre");
        }
    }
    g_original_conductor_update(self, method);
    g_in_conductor_frame = 0;
}

static void drive_editor_async_update(const char *reason) {
    void *controller = read_adobase_controller();
    refresh_scene_state_on_main_thread(reason, controller);

    if (controller_allows_async_replay(controller)) {
        hooked_update_input(controller, NULL);
        return;
    }

    if (editor_blocks_async() && capture_gate_open()) {
        close_async_capture();
    } else if (editor_blocks_async()) {
        restore_regular_input_types();
    }
}

static void hooked_editor_update(void *self, void *method) {
    if (!g_enabled) {
        g_original_editor_update(self, method);
        return;
    }

    void *controller = read_adobase_controller();
    refresh_scene_state_on_main_thread("scnEditor.Update.pre", controller);

    g_original_editor_update(self, method);

    drive_editor_async_update("scnEditor.Update.after");
}

static void hooked_playercontrol_update(void *self, void *method) {
    if (!g_enabled) {
        stop_capture_and_clear_queue();
        g_original_playercontrol_update(self, method);
        return;
    }

    g_current_controller_self = self;
    refresh_scene_state_on_main_thread("PlayerControl.Update", self);
    if (dlc_async_disabled_cached()) {
        disable_async_for_dlc_if_needed("PlayerControl.Update");
        enter_playercontrol_original();
        g_original_playercontrol_update(self, method);
        leave_playercontrol_original();
        g_current_controller_self = NULL;
        clear_frame_edges();
        return;
    }
    if (!controller_allows_async_replay(self)) {
        if (controller_allows_async_capture(self)) {
            open_capture_for_controller(self);
            ensure_async_clock_fields(wall_tick_now_or_virtual());
        } else if (capture_gate_open()) {
            close_async_capture();
        } else if (editor_blocks_async()) {
            restore_regular_input_types();
        }
        enter_playercontrol_original();
        g_original_playercontrol_update(self, method);
        leave_playercontrol_original();
        if (controller_allows_async_replay(self)) {
            hooked_update_input(self, NULL);
        } else if (!controller_allows_async_capture(self) && capture_gate_open()) {
            close_async_capture();
        } else if (editor_blocks_async()) {
            restore_regular_input_types();
        }
        g_current_controller_self = NULL;
        clear_frame_edges();
        return;
    }

    open_capture_for_controller(self);
    g_last_session_boundary_state = -1;
    ensure_async_clock_fields(wall_tick_now_or_virtual());

    /*
     * Android/editor paths do not always reach scrConductor's UpdateInput call
     * even while PlayerControl_Update is running. Since isActive=true makes the
     * original PlayerControl_Update skip the frame-bound Simulated update, this
     * hook must drive the official async mask replay from the PlayerControl
     * state-machine tick as well.
     */
    hooked_update_input(self, NULL);

    g_in_playercontrol_frame = 1;
    g_force_async_active_for_original = 1;
    enter_playercontrol_original();
    g_original_playercontrol_update(self, method);
    leave_playercontrol_original();
    g_force_async_active_for_original = 0;
    g_in_playercontrol_frame = 0;
    g_current_controller_self = NULL;
    if (!controller_allows_async_capture(self)) {
        close_async_capture();
    }
    clear_frame_edges();
}

static uintptr_t find_module_base_from_maps(const char *soname) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp == NULL) {
        return 0;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, soname) == NULL) {
            continue;
        }

        uintptr_t start = 0;
        if (sscanf(line, "%" SCNxPTR "-", &start) == 1) {
            fclose(fp);
            return start;
        }
    }

    fclose(fp);
    return 0;
}

static int maps_contains_path_fragment(const char *fragment) {
    if (fragment == NULL) {
        return 0;
    }
    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp == NULL) {
        return 0;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, fragment) != NULL) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static int string_equals_ci(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }
    while (*a != '\0' && *b != '\0') {
        char ca = *a++;
        char cb = *b++;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca + ('a' - 'A'));
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb + ('a' - 'A'));
        }
        if (ca != cb) {
            return 0;
        }
    }
    return *a == '\0' && *b == '\0';
}

typedef struct DynsymLookup {
    const char *soname;
    const char *symbol;
    void *address;
} DynsymLookup;

static uintptr_t dynamic_entry_addr(const struct dl_phdr_info *info, uintptr_t value) {
    if (value == 0) {
        return 0;
    }
    uintptr_t base = (uintptr_t)info->dlpi_addr;
    if (base != 0 && value >= base && value < base + 0x100000000ULL) {
        return value;
    }
    return base + value;
}

static size_t gnu_hash_symbol_count(const uint32_t *gnu_hash) {
    if (gnu_hash == NULL) {
        return 0;
    }

    uint32_t nbuckets = gnu_hash[0];
    uint32_t symoffset = gnu_hash[1];
    uint32_t bloom_size = gnu_hash[2];
    const uintptr_t *bloom = (const uintptr_t *)(gnu_hash + 4);
    const uint32_t *buckets = (const uint32_t *)(bloom + bloom_size);
    const uint32_t *chains = buckets + nbuckets;

    uint32_t max_symbol = 0;
    for (uint32_t i = 0; i < nbuckets; ++i) {
        if (buckets[i] > max_symbol) {
            max_symbol = buckets[i];
        }
    }
    if (max_symbol < symoffset) {
        return symoffset;
    }

    uint32_t chain_index = max_symbol - symoffset;
    while ((chains[chain_index] & 1u) == 0) {
        chain_index++;
    }
    return (size_t)(symoffset + chain_index + 1);
}

static int resolve_dynsym_callback(struct dl_phdr_info *info, size_t size, void *data) {
    (void)size;
    DynsymLookup *lookup = (DynsymLookup *)data;
    if (lookup == NULL || lookup->address != NULL || lookup->soname == NULL || lookup->symbol == NULL) {
        return 0;
    }
    if (info == NULL || info->dlpi_name == NULL || strstr(info->dlpi_name, lookup->soname) == NULL) {
        return 0;
    }

    const ElfW(Dyn) *dynamic = NULL;
    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
            dynamic = (const ElfW(Dyn) *)((uintptr_t)info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
            break;
        }
    }
    if (dynamic == NULL) {
        return 0;
    }

    const ElfW(Sym) *symtab = NULL;
    const char *strtab = NULL;
    const uint32_t *sysv_hash = NULL;
    const uint32_t *gnu_hash = NULL;
    size_t syment = sizeof(ElfW(Sym));
    size_t symbol_count = 0;

    for (const ElfW(Dyn) *dyn = dynamic; dyn->d_tag != DT_NULL; ++dyn) {
        if (dyn->d_tag == DT_SYMTAB) {
            symtab = (const ElfW(Sym) *)dynamic_entry_addr(info, (uintptr_t)dyn->d_un.d_ptr);
        } else if (dyn->d_tag == DT_STRTAB) {
            strtab = (const char *)dynamic_entry_addr(info, (uintptr_t)dyn->d_un.d_ptr);
        } else if (dyn->d_tag == DT_SYMENT) {
            syment = (size_t)dyn->d_un.d_val;
        } else if (dyn->d_tag == DT_HASH) {
            sysv_hash = (const uint32_t *)dynamic_entry_addr(info, (uintptr_t)dyn->d_un.d_ptr);
        } else if (dyn->d_tag == DT_GNU_HASH) {
            gnu_hash = (const uint32_t *)dynamic_entry_addr(info, (uintptr_t)dyn->d_un.d_ptr);
        }
    }

    if (symtab == NULL || strtab == NULL || syment == 0) {
        return 0;
    }
    if (sysv_hash != NULL) {
        symbol_count = sysv_hash[1];
    } else if (gnu_hash != NULL) {
        symbol_count = gnu_hash_symbol_count(gnu_hash);
    }
    if (symbol_count == 0) {
        return 0;
    }

    for (size_t i = 0; i < symbol_count; ++i) {
        const ElfW(Sym) *sym = (const ElfW(Sym) *)((const char *)symtab + i * syment);
        if (ELF_ST_TYPE(sym->st_info) != STT_FUNC || sym->st_name == 0 || sym->st_value == 0) {
            continue;
        }
        const char *name = strtab + sym->st_name;
        if (strcmp(name, lookup->symbol) == 0) {
            lookup->address = (void *)((uintptr_t)info->dlpi_addr + (uintptr_t)sym->st_value);
            return 1;
        }
    }
    return 0;
}

static void *resolve_dynsym(const char *soname, const char *symbol) {
    DynsymLookup lookup;
    memset(&lookup, 0, sizeof(lookup));
    lookup.soname = soname;
    lookup.symbol = symbol;
    dl_iterate_phdr(resolve_dynsym_callback, &lookup);
    return lookup.address;
}

static void *load_symbol(void *handle, const char *name) {
    (void)handle;
    void *symbol = resolve_dynsym("libil2cpp.so", name);
    if (symbol == NULL) {
        LOGW("missing il2cpp symbol %s", name);
    }
    return symbol;
}

static int load_il2cpp_symbols(void) {
    if (g_il2cpp_api.domain_get != NULL &&
        g_il2cpp_api.thread_attach != NULL &&
        g_il2cpp_api.domain_get_assemblies != NULL) {
        return 1;
    }

    memset(&g_il2cpp_api, 0, sizeof(g_il2cpp_api));
    g_il2cpp_api.handle = dlopen("libil2cpp.so", RTLD_NOW | RTLD_LOCAL);
    if (g_il2cpp_api.handle == NULL) {
        LOGW("dlopen libil2cpp.so failed: %s", dlerror());
        return 0;
    }

    g_il2cpp_api.get_corlib = (Il2CppGetCorlibFn)load_symbol(g_il2cpp_api.handle, "il2cpp_get_corlib");
    g_il2cpp_api.domain_get = (Il2CppDomainGetFn)load_symbol(g_il2cpp_api.handle, "il2cpp_domain_get");
    g_il2cpp_api.thread_attach = (Il2CppThreadAttachFn)load_symbol(g_il2cpp_api.handle, "il2cpp_thread_attach");
    g_il2cpp_api.domain_get_assemblies = (Il2CppDomainGetAssembliesFn)load_symbol(g_il2cpp_api.handle, "il2cpp_domain_get_assemblies");
    g_il2cpp_api.assembly_get_image = (Il2CppAssemblyGetImageFn)load_symbol(g_il2cpp_api.handle, "il2cpp_assembly_get_image");
    g_il2cpp_api.image_get_name = (Il2CppImageGetNameFn)load_symbol(g_il2cpp_api.handle, "il2cpp_image_get_name");
    g_il2cpp_api.class_from_name = (Il2CppClassFromNameFn)load_symbol(g_il2cpp_api.handle, "il2cpp_class_from_name");
    g_il2cpp_api.class_get_method_from_name = (Il2CppClassGetMethodFromNameFn)load_symbol(g_il2cpp_api.handle, "il2cpp_class_get_method_from_name");
    g_il2cpp_api.class_get_field_from_name = (Il2CppClassGetFieldFromNameFn)load_symbol(g_il2cpp_api.handle, "il2cpp_class_get_field_from_name");
    g_il2cpp_api.field_get_offset = (Il2CppFieldGetOffsetFn)load_symbol(g_il2cpp_api.handle, "il2cpp_field_get_offset");
    g_il2cpp_api.field_static_get_value = (Il2CppFieldStaticGetValueFn)load_symbol(g_il2cpp_api.handle, "il2cpp_field_static_get_value");
    if (g_il2cpp_api.field_static_get_value == NULL) {
        LOGW("missing il2cpp symbol il2cpp_field_static_get_value");
    }
    g_il2cpp_api.field_static_set_value = (Il2CppFieldStaticSetValueFn)load_symbol(g_il2cpp_api.handle, "il2cpp_field_static_set_value");
    g_il2cpp_api.object_get_class = (Il2CppObjectGetClassFn)load_symbol(g_il2cpp_api.handle, "il2cpp_object_get_class");
    g_il2cpp_api.runtime_class_init = (Il2CppRuntimeClassInitFn)load_symbol(g_il2cpp_api.handle, "il2cpp_runtime_class_init");
    g_il2cpp_api.runtime_invoke = (Il2CppRuntimeInvokeFn)load_symbol(g_il2cpp_api.handle, "il2cpp_runtime_invoke");
    g_il2cpp_api.string_chars = (Il2CppStringCharsFn)load_symbol(g_il2cpp_api.handle, "il2cpp_string_chars");
    g_il2cpp_api.string_length = (Il2CppStringLengthFn)load_symbol(g_il2cpp_api.handle, "il2cpp_string_length");

    if (g_il2cpp_api.domain_get == NULL ||
        g_il2cpp_api.thread_attach == NULL ||
        g_il2cpp_api.domain_get_assemblies == NULL ||
        g_il2cpp_api.assembly_get_image == NULL ||
        g_il2cpp_api.image_get_name == NULL ||
        g_il2cpp_api.class_from_name == NULL ||
        g_il2cpp_api.class_get_method_from_name == NULL ||
        g_il2cpp_api.class_get_field_from_name == NULL ||
        g_il2cpp_api.field_get_offset == NULL ||
        g_il2cpp_api.field_static_set_value == NULL ||
        g_il2cpp_api.object_get_class == NULL ||
        g_il2cpp_api.runtime_class_init == NULL ||
        g_il2cpp_api.runtime_invoke == NULL) {
        LOGW("il2cpp metadata API incomplete, metadata hooks unavailable");
        return 0;
    }
    return 1;
}

static int il2cpp_runtime_ready_for_metadata(void) {
    if (!load_il2cpp_symbols()) {
        return 0;
    }
    if (!maps_contains_path_fragment("libil2cpp.so") ||
        !maps_contains_path_fragment("global-metadata.dat")) {
        return 0;
    }
    if (g_il2cpp_init_completed) {
        return 1;
    }
    if (g_il2cpp_api.get_corlib != NULL && g_il2cpp_api.get_corlib() != NULL) {
        mark_il2cpp_init_completed("il2cpp_get_corlib");
        return 1;
    }
    return 0;
}

static int init_il2cpp_api(void) {
    if (!il2cpp_runtime_ready_for_metadata()) {
        return 0;
    }

    void *domain = g_il2cpp_api.domain_get();
    if (domain == NULL) {
        LOGW("il2cpp_domain_get returned null");
        return 0;
    }
    g_il2cpp_api.thread_attach(domain);

    size_t assembly_count = 0;
    const void **assemblies = g_il2cpp_api.domain_get_assemblies(domain, &assembly_count);
    if (assemblies == NULL || assembly_count == 0) {
        LOGW("il2cpp_domain_get_assemblies returned no assemblies");
        return 0;
    }

    for (size_t i = 0; i < assembly_count; ++i) {
        void *image = g_il2cpp_api.assembly_get_image(assemblies[i]);
        const char *image_name = image ? g_il2cpp_api.image_get_name(image) : NULL;
        if (string_equals_ci(image_name, IL2CPP_IMAGE_ASSEMBLY_CSHARP)) {
            g_il2cpp_api.assembly_csharp_image = image;
        } else if (string_equals_ci(image_name, IL2CPP_IMAGE_AUDIO_MODULE)) {
            g_il2cpp_api.audio_module_image = image;
        } else if (string_equals_ci(image_name, IL2CPP_IMAGE_CORE_MODULE)) {
            g_il2cpp_api.core_module_image = image;
        }
    }

    if (g_il2cpp_api.assembly_csharp_image == NULL) {
        LOGW("Assembly-CSharp image not found, metadata unavailable");
        return 0;
    }
    if (g_il2cpp_api.audio_module_image == NULL) {
        LOGW("UnityEngine.AudioModule image not found; dspTime metadata unavailable");
    }
    if (g_il2cpp_api.core_module_image == NULL) {
        LOGW("UnityEngine.CoreModule image not found; frameCount metadata unavailable");
    }

    g_il2cpp_api.ready = 1;
    LOGI("il2cpp metadata ready assembly_csharp=%p audio_module=%p core_module=%p assemblies=%zu",
         g_il2cpp_api.assembly_csharp_image,
         g_il2cpp_api.audio_module_image,
         g_il2cpp_api.core_module_image,
         assembly_count);
    return 1;
}

static void *resolve_image_by_name(const char *image_name) {
    if (!g_il2cpp_api.ready) {
        return NULL;
    }
    if (image_name == NULL || string_equals_ci(image_name, IL2CPP_IMAGE_ASSEMBLY_CSHARP)) {
        return g_il2cpp_api.assembly_csharp_image;
    }
    if (string_equals_ci(image_name, IL2CPP_IMAGE_AUDIO_MODULE)) {
        return g_il2cpp_api.audio_module_image;
    }
    if (string_equals_ci(image_name, IL2CPP_IMAGE_CORE_MODULE)) {
        return g_il2cpp_api.core_module_image;
    }
    return NULL;
}

static void *resolve_class_in_image(const char *image_name, const char *namespaze, const char *klass) {
    void *image = resolve_image_by_name(image_name);
    if (image == NULL) {
        return NULL;
    }
    return g_il2cpp_api.class_from_name(image, namespaze ? namespaze : "", klass);
}

static void *resolve_class(const char *namespaze, const char *klass) {
    return resolve_class_in_image(IL2CPP_IMAGE_ASSEMBLY_CSHARP, namespaze, klass);
}

static uintptr_t resolve_method_pointer_internal(const char *image_name, const char *namespaze, const char *klass, const char *method, int args_count, int log_failures) {
    void *class_ptr = resolve_class_in_image(image_name, namespaze, klass);
    if (class_ptr == NULL) {
        if (log_failures) {
            LOGW("metadata class missing %s %s.%s", image_name ? image_name : IL2CPP_IMAGE_ASSEMBLY_CSHARP, namespaze ? namespaze : "", klass);
        }
        return 0;
    }

    const void *method_info = g_il2cpp_api.class_get_method_from_name(class_ptr, method, args_count);
    if (method_info == NULL) {
        if (log_failures) {
            LOGW("metadata method missing %s.%s/%d", klass, method, args_count);
        }
        return 0;
    }

    uintptr_t pointer = ((const MethodInfoHead *)method_info)->method_pointer;
    if (pointer == 0 && log_failures) {
        LOGW("metadata method pointer null %s.%s/%d", klass, method, args_count);
    }
    return pointer;
}

static uintptr_t resolve_method_pointer(const char *namespaze, const char *klass, const char *method, int args_count) {
    return resolve_method_pointer_internal(IL2CPP_IMAGE_ASSEMBLY_CSHARP, namespaze, klass, method, args_count, 1);
}

static uintptr_t resolve_method_pointer_quiet(const char *namespaze, const char *klass, const char *method, int args_count) {
    return resolve_method_pointer_internal(IL2CPP_IMAGE_ASSEMBLY_CSHARP, namespaze, klass, method, args_count, 0);
}

static void *resolve_method_info_in_image(const char *image_name, const char *namespaze, const char *klass, const char *method, int args_count) {
    void *class_ptr = resolve_class_in_image(image_name, namespaze, klass);
    if (class_ptr == NULL) {
        LOGW("metadata class missing for method info %s %s.%s", image_name ? image_name : IL2CPP_IMAGE_ASSEMBLY_CSHARP, namespaze ? namespaze : "", klass);
        return NULL;
    }

    void *method_info = (void *)g_il2cpp_api.class_get_method_from_name(class_ptr, method, args_count);
    if (method_info == NULL) {
        LOGW("metadata method info missing %s.%s/%d", klass, method, args_count);
    }
    return method_info;
}

static int ensure_method_cache_in_image(const char *image_name, const char *namespaze, const char *klass, const char *method, int args_count, Il2CppMethodCache *cache) {
    if (cache == NULL) {
        return 0;
    }
    if (cache->method_pointer != 0) {
        return 1;
    }
    ensure_metadata_ready_lazy();
    void *method_info = resolve_method_info_in_image(image_name, namespaze, klass, method, args_count);
    if (method_info == NULL) {
        return 0;
    }
    uintptr_t pointer = ((const MethodInfoHead *)method_info)->method_pointer;
    if (pointer == 0) {
        LOGW("metadata method pointer null %s.%s/%d", klass, method, args_count);
        return 0;
    }
    cache->method_info = method_info;
    cache->method_pointer = pointer;
    return 1;
}

static int ensure_method_cache(const char *namespaze, const char *klass, const char *method, int args_count, Il2CppMethodCache *cache) {
    return ensure_method_cache_in_image(IL2CPP_IMAGE_ASSEMBLY_CSHARP, namespaze, klass, method, args_count, cache);
}

static int cache_method_from_class(void *class_ptr, const char *method_name, int args_count, Il2CppMethodCache *out) {
    if (class_ptr == NULL || method_name == NULL || out == NULL || g_il2cpp_api.class_get_method_from_name == NULL) {
        return 0;
    }
    void *method_info = (void *)g_il2cpp_api.class_get_method_from_name(class_ptr, method_name, args_count);
    if (method_info == NULL) {
        LOGW("method missing on runtime class %s/%d", method_name, args_count);
        return 0;
    }
    uintptr_t pointer = ((const MethodInfoHead *)method_info)->method_pointer;
    if (pointer == 0) {
        LOGW("method pointer null on runtime class %s/%d", method_name, args_count);
        return 0;
    }
    out->method_info = method_info;
    out->method_pointer = pointer;
    return 1;
}

static int hashset_count_if_ready(void *set) {
    if (set == NULL || !g_mask_api_ready || g_hashset_get_count_method.method_pointer == 0) {
        return -1;
    }
    IntSelfFn get_count = (IntSelfFn)g_hashset_get_count_method.method_pointer;
    return get_count(set, g_hashset_get_count_method.method_info);
}

static uintptr_t resolve_field_offset_or_default(const char *namespaze, const char *klass, const char *field_name, uintptr_t fallback) {
    void *class_ptr = resolve_class(namespaze, klass);
    if (class_ptr == NULL) {
        LOGW("metadata class missing for field %s.%s, fallback=0x%lx", klass, field_name, (unsigned long)fallback);
        return fallback;
    }

    void *field = g_il2cpp_api.class_get_field_from_name(class_ptr, field_name);
    if (field == NULL) {
        LOGW("metadata field missing %s.%s, fallback=0x%lx", klass, field_name, (unsigned long)fallback);
        return fallback;
    }

    uintptr_t offset = (uintptr_t)g_il2cpp_api.field_get_offset(field);
    LOGI("field resolved %s.%s offset=0x%lx", klass, field_name, (unsigned long)offset);
    return offset;
}

static void *resolve_field_or_null(const char *namespaze, const char *klass, const char *field_name) {
    void *class_ptr = resolve_class(namespaze, klass);
    if (class_ptr == NULL) {
        LOGW("metadata class missing for field %s.%s", klass, field_name);
        return NULL;
    }

    void *field = g_il2cpp_api.class_get_field_from_name(class_ptr, field_name);
    if (field == NULL) {
        LOGW("metadata field missing %s.%s", klass, field_name);
        return NULL;
    }

    LOGI("field resolved %s.%s", klass, field_name);
    return field;
}

static const char *shadow_margin_name(AdoOfficialHitMargin margin) {
    switch (margin) {
        case ADO_HIT_TOO_EARLY:
            return "TooEarly";
        case ADO_HIT_VERY_EARLY:
            return "VeryEarly";
        case ADO_HIT_EARLY_PERFECT:
            return "EarlyPerfect";
        case ADO_HIT_PERFECT:
            return "Perfect";
        case ADO_HIT_LATE_PERFECT:
            return "LatePerfect";
        case ADO_HIT_VERY_LATE:
            return "VeryLate";
        case ADO_HIT_TOO_LATE:
            return "TooLate";
        case ADO_HIT_MULTIPRESS:
            return "Multipress";
        case ADO_HIT_FAIL_MISS:
            return "FailMiss";
        case ADO_HIT_FAIL_OVERLOAD:
            return "FailOverload";
        case ADO_HIT_AUTO:
            return "Auto";
        case ADO_HIT_OVERPRESS:
            return "OverPress";
        default:
            return "Unknown";
    }
}

static int read_static_bool_method(const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache, int fallback) {
    if (!ensure_method_cache(namespaze, klass, method, 0, cache)) {
        return fallback;
    }
    IntStaticFn fn = (IntStaticFn)cache->method_pointer;
    return fn(cache->method_info) ? 1 : 0;
}

static int read_rdc_auto(int *out) {
    if (out == NULL || !ensure_method_cache("", "RDC", "get_auto", 0, &g_rdc_get_auto_method)) {
        return 0;
    }
    IntStaticFn fn = (IntStaticFn)g_rdc_get_auto_method.method_pointer;
    *out = fn(g_rdc_get_auto_method.method_info) ? 1 : 0;
    return 1;
}

static int write_rdc_auto(int value) {
    if (!ensure_method_cache("", "RDC", "set_auto", 1, &g_rdc_set_auto_method)) {
        return 0;
    }
    SetBoolStaticFn fn = (SetBoolStaticFn)g_rdc_set_auto_method.method_pointer;
    fn(value ? 1 : 0, g_rdc_set_auto_method.method_info);
    return 1;
}

static double read_static_float_method(const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache, double fallback) {
    if (!ensure_method_cache(namespaze, klass, method, 0, cache)) {
        return fallback;
    }
    FloatStaticFn fn = (FloatStaticFn)cache->method_pointer;
    return (double)fn(cache->method_info);
}

static void *read_static_object_method(const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache) {
    if (!ensure_method_cache(namespaze, klass, method, 0, cache)) {
        return NULL;
    }
    GetObjectStaticFn fn = (GetObjectStaticFn)cache->method_pointer;
    return fn(cache->method_info);
}

static double read_self_double_method(void *self, const char *namespaze, const char *klass, const char *method, Il2CppMethodCache *cache, double fallback) {
    if (self == NULL || !ensure_method_cache(namespaze, klass, method, 0, cache)) {
        return fallback;
    }
    DoubleSelfFn fn = (DoubleSelfFn)cache->method_pointer;
    return fn(self, cache->method_info);
}

static double read_audio_source_pitch(void *audio_source) {
    if (audio_source == NULL ||
        !ensure_method_cache_in_image(
            IL2CPP_IMAGE_AUDIO_MODULE,
            "UnityEngine",
            "AudioSource",
            "get_pitch",
            0,
            &g_audiosource_get_pitch_method)) {
        return 1.0;
    }
    FloatSelfFn get_pitch = (FloatSelfFn)g_audiosource_get_pitch_method.method_pointer;
    double pitch = (double)get_pitch(audio_source, g_audiosource_get_pitch_method.method_info);
    return pitch > 0.0 ? pitch : 1.0;
}

static int shadow_should_log(uint64_t *count_out) {
    static uint64_t shadow_count = 0;
    static uint64_t last_log_ns = 0;
    uint64_t count = __atomic_add_fetch(&shadow_count, 1, __ATOMIC_RELAXED);
    if (count_out != NULL) {
        *count_out = count;
    }
    uint64_t now_ns = monotonic_ns_now();
    if (count <= 16 || now_ns - last_log_ns >= 500000000ULL) {
        last_log_ns = now_ns;
        return 1;
    }
    return 0;
}

static void shadow_log_skip(const char *reason) {
    static uint64_t last_skip_log_ns = 0;
    uint64_t now_ns = monotonic_ns_now();
    if (now_ns - last_skip_log_ns >= 2000000000ULL) {
        last_skip_log_ns = now_ns;
        LOGW("shadow judgement skipped: %s", reason ? reason : "unknown");
    }
}

static void shadow_judgement_before_process(void *controller_self, uint64_t tick, int replay_mode) {
    if (!g_trace_enabled || !g_enabled || tick == 0) {
        return;
    }

    uint64_t shadow_count = 0;
    if (!shadow_should_log(&shadow_count)) {
        return;
    }

    int down = snapshot_down();
    int up = snapshot_up();
    int held = snapshot_held();
    uint64_t trace_id = __atomic_load_n(&g_current_replay_trace_id, __ATOMIC_ACQUIRE);
    if (down <= 0) {
        LOGI("SHADOW release count=%" PRIu64
             " id=%" PRIu64
             " tick=%" PRIu64
             " mode=%d down=%d up=%d held=%d no_hit_prediction=1",
             shadow_count,
             trace_id,
             tick,
             replay_mode,
             down,
             up,
             held);
        return;
    }

    ensure_metadata_ready_lazy();
    if (!g_metadata_init_ready || !g_il2cpp_api.ready) {
        shadow_log_skip("metadata not ready");
        return;
    }

    void *chosen_planet = NULL;
    if (!ensure_method_cache("", "scrController", "get_chosenPlanet", 0, &g_scrcontroller_get_chosen_planet_method)) {
        shadow_log_skip("scrController.get_chosenPlanet missing");
        return;
    }
    GetObjectSelfFn get_chosen_planet = (GetObjectSelfFn)g_scrcontroller_get_chosen_planet_method.method_pointer;
    chosen_planet = get_chosen_planet(controller_self, g_scrcontroller_get_chosen_planet_method.method_info);
    if (chosen_planet == NULL) {
        shadow_log_skip("chosen planet null");
        return;
    }

    void *player = NULL;
    void *planetary_system = NULL;
    void *currfloor = NULL;
    void *nextfloor = NULL;
    if (!read_instance_object_field(chosen_planet, g_offset_scrplanet_player, &player) ||
        !read_instance_object_field(chosen_planet, g_offset_scrplanet_planetary_system, &planetary_system) ||
        !read_instance_object_field(chosen_planet, g_offset_scrplanet_currfloor, &currfloor)) {
        shadow_log_skip("planet field unavailable");
        return;
    }
    (void)read_instance_object_field(currfloor, g_offset_scrfloor_nextfloor, &nextfloor);

    void *conductor_object = read_static_object_method("", "scrConductor", "get_instance", &g_scrconductor_get_instance_method);
    if (conductor_object == NULL) {
        shadow_log_skip("scrConductor.instance null");
        return;
    }

    void *song = NULL;
    (void)read_instance_object_field(conductor_object, g_offset_scrconductor_song, &song);

    AdoOfficialConfig config;
    ado_official_config_defaults(&config);
    config.is_mobile = read_static_bool_method("", "ADOBase", "get_isMobile", &g_adobase_get_is_mobile_method, 1);
    config.use_old_auto = read_static_bool_method("", "RDC", "get_useOldAuto", &g_rdc_get_use_old_auto_method, 0);
    (void)read_static_int_field(g_field_gcs_difficulty, (int *)&config.difficulty);
    (void)read_static_int_field(g_field_gcs_hit_margin_limit, (int *)&config.hit_margin_limit);
    (void)read_static_float_field(g_field_gcs_current_speed_trial, &config.current_speed_trial);
    (void)read_static_float_field(g_field_gcs_hitmargin_counted, &config.hitmargin_counted);
    if (config.current_speed_trial <= 0.0) {
        config.current_speed_trial = 1.0;
    }
    if (config.hitmargin_counted <= 0.0) {
        config.hitmargin_counted = 60.0;
    }

    AdoOfficialConductor conductor;
    memset(&conductor, 0, sizeof(conductor));
    if (!read_instance_float_field(conductor_object, g_offset_scrconductor_bpm, &conductor.bpm) ||
        !read_instance_double_field(conductor_object, g_offset_scrconductor_dsp_time_song, &conductor.dsp_time_song) ||
        !read_instance_double_field(conductor_object, g_offset_scrconductor_addoffset, &conductor.add_offset)) {
        shadow_log_skip("conductor field unavailable");
        return;
    }
    conductor.pitch = read_audio_source_pitch(song);
    conductor.calibration_i = read_static_float_method("", "scrConductor", "get_calibration_i", &g_scrconductor_get_calibration_i_method, 0.0);
    (void)read_instance_double_field(conductor_object, g_offset_scrconductor_crotchet_at_start, &conductor.crotchet_at_start);
    if (conductor.crotchet_at_start == 0.0 && conductor.bpm > 0.0) {
        conductor.crotchet_at_start = 60.0 / conductor.bpm;
    }
    if (conductor.bpm <= 0.0 || conductor.crotchet_at_start == 0.0) {
        shadow_log_skip("invalid conductor timing");
        return;
    }

    AdoOfficialPlayer oj_player;
    memset(&oj_player, 0, sizeof(oj_player));
    if (!read_instance_double_field(player, g_offset_scrplayer_last_hit, &oj_player.last_hit)) {
        shadow_log_skip("player.lastHit unavailable");
        return;
    }
    oj_player.alive = 1;
    oj_player.responsive = 1;

    AdoOfficialPlanet planet;
    memset(&planet, 0, sizeof(planet));
    planet.currfloor = currfloor;
    planet.snapped_last_angle =
        read_self_double_method(chosen_planet, "", "scrPlanet", "get_snappedLastAngle", &g_scrplanet_get_snapped_last_angle_method, 0.0);
    planet.target_exit_angle =
        read_self_double_method(chosen_planet, "", "scrPlanet", "get_targetExitAngle", &g_scrplanet_get_target_exit_angle_method, 0.0);
    (void)read_instance_double_field(chosen_planet, g_offset_scrplanet_angle, &planet.angle);
    (void)read_instance_double_field(chosen_planet, g_offset_scrplanet_cached_angle, &planet.cached_angle);
    if (!read_instance_double_field(planetary_system, g_offset_planetarysystem_speed, &planet.speed)) {
        planet.speed = 1.0;
    }
    if (!read_instance_bool_field(planetary_system, g_offset_planetarysystem_is_cw, &planet.is_cw)) {
        planet.is_cw = 1;
    }

    double margin_scale = 1.0;
    int next_auto = 0;
    if (nextfloor != NULL) {
        (void)read_instance_double_field(nextfloor, g_offset_scrfloor_margin_scale, &margin_scale);
        (void)read_instance_bool_field(nextfloor, g_offset_scrfloor_auto, &next_auto);
    }

    uint64_t offset_tick = 0;
    if (g_field_async_offset_tick != NULL) {
        (void)read_static_u64_field(g_field_async_offset_tick, &offset_tick);
    }
    uint64_t target_song_tick = tick;
    if (offset_tick != 0 && tick > offset_tick) {
        target_song_tick = tick - offset_tick;
    }

    AdoOfficialJudgementOnly prediction_raw_tick = ado_official_judge_event_tick_only(
        &config,
        &conductor,
        &oj_player,
        &planet,
        tick,
        margin_scale);
    AdoOfficialJudgementOnly prediction_target_tick = ado_official_judge_event_tick_only(
        &config,
        &conductor,
        &oj_player,
        &planet,
        target_song_tick,
        margin_scale);
    AdoOfficialJudgementOnly prediction_live_angle = ado_official_judge_angle_only(
        &config,
        planet.angle,
        planet.target_exit_angle,
        planet.is_cw,
        conductor.bpm * planet.speed,
        conductor.pitch,
        margin_scale);
    AdoOfficialJudgementOnly prediction_cached_angle = ado_official_judge_angle_only(
        &config,
        planet.cached_angle,
        planet.target_exit_angle,
        planet.is_cw,
        conductor.bpm * planet.speed,
        conductor.pitch,
        margin_scale);

    double raw_song_position = (((double)tick / 10000000.0) -
                               conductor.dsp_time_song -
                               conductor.calibration_i) *
                              conductor.pitch -
                          conductor.add_offset;
    double target_song_position = (((double)target_song_tick / 10000000.0) -
                                      conductor.dsp_time_song -
                                      conductor.calibration_i) *
                                     conductor.pitch -
                                 conductor.add_offset;
    double raw_since_last_ms = (raw_song_position - oj_player.last_hit) * 1000.0;
    double target_since_last_ms = (target_song_position - oj_player.last_hit) * 1000.0;
    double raw_hit_angle = planet.snapped_last_angle +
                           (raw_song_position - oj_player.last_hit) /
                               conductor.crotchet_at_start *
                               3.141592653598793 *
                               planet.speed *
                               (planet.is_cw ? 1.0 : -1.0);
    double target_hit_angle = planet.snapped_last_angle +
                              (target_song_position - oj_player.last_hit) /
                                  conductor.crotchet_at_start *
                                  3.141592653598793 *
                                  planet.speed *
                                  (planet.is_cw ? 1.0 : -1.0);

    LOGI("SHADOW hit count=%" PRIu64
         " id=%" PRIu64
         " tick=%" PRIu64
         " targetSongTick=%" PRIu64
         " offsetTick=%" PRIu64
         " mode=%d down=%d up=%d held=%d rawMargin=%s(%d) rawOffset_ms=%.3f rawAngleDelta_deg=%.3f"
         " targetMargin=%s(%d) targetOffset_ms=%.3f targetAngleDelta_deg=%.3f"
         " liveAngleMargin=%s(%d) liveOffset_ms=%.3f cachedAngleMargin=%s(%d) cachedOffset_ms=%.3f"
         " rawHit=%.9f targetHit=%.9f liveAngle=%.9f cachedAngle=%.9f target=%.9f"
         " rawSongPos=%.9f targetSongPos=%.9f rawSinceLast_ms=%.3f targetSinceLast_ms=%.3f"
         " bpm=%.3f pitch=%.6f speed=%.6f marginScale=%.6f isCW=%d mobile=%d speedTrial=%.3f counted=%.3f oldAuto=%d nextAuto=%d"
         " snapped=%.9f target=%.9f lastHit=%.9f dspSong=%.9f addOffset=%.9f calib=%.9f",
         shadow_count,
         trace_id,
         tick,
         target_song_tick,
         offset_tick,
         replay_mode,
         down,
         up,
         held,
         shadow_margin_name(prediction_raw_tick.margin),
         (int)prediction_raw_tick.margin,
         prediction_raw_tick.signed_time_offset_ms,
         prediction_raw_tick.signed_angle_delta_deg,
         shadow_margin_name(prediction_target_tick.margin),
         (int)prediction_target_tick.margin,
         prediction_target_tick.signed_time_offset_ms,
         prediction_target_tick.signed_angle_delta_deg,
         shadow_margin_name(prediction_live_angle.margin),
         (int)prediction_live_angle.margin,
         prediction_live_angle.signed_time_offset_ms,
         shadow_margin_name(prediction_cached_angle.margin),
         (int)prediction_cached_angle.margin,
         prediction_cached_angle.signed_time_offset_ms,
         raw_hit_angle,
         target_hit_angle,
         planet.angle,
         planet.cached_angle,
         planet.target_exit_angle,
         raw_song_position,
         target_song_position,
         raw_since_last_ms,
         target_since_last_ms,
         conductor.bpm,
         conductor.pitch,
         planet.speed,
         margin_scale,
         planet.is_cw,
         config.is_mobile,
         config.current_speed_trial,
         config.hitmargin_counted,
         config.use_old_auto,
         next_auto,
         planet.snapped_last_angle,
         planet.target_exit_angle,
         oj_player.last_hit,
         conductor.dsp_time_song,
         conductor.add_offset,
         conductor.calibration_i);
}

static int refresh_async_mask_objects(void) {
    int ok = 1;
    ok &= read_static_object_field(g_field_async_key_mask, &g_async_key_mask);
    ok &= read_static_object_field(g_field_async_key_down_mask, &g_async_key_down_mask);
    ok &= read_static_object_field(g_field_async_key_up_mask, &g_async_key_up_mask);
    ok &= read_static_object_field(g_field_async_frame_key_mask, &g_async_frame_key_mask);
    ok &= read_static_object_field(g_field_async_frame_key_down_mask, &g_async_frame_key_down_mask);
    ok &= read_static_object_field(g_field_async_frame_key_up_mask, &g_async_frame_key_up_mask);
    return ok;
}

static int ensure_process_key_inputs_ready(void) {
    if (g_process_key_inputs_method.method_pointer != 0) {
        return 1;
    }
    if (ensure_method_cache("", "scrController", "ProcessKeyInputs", 1, &g_process_key_inputs_method)) {
        LOGI("ProcessKeyInputs resolved by metadata target=0x%lx", (unsigned long)g_process_key_inputs_method.method_pointer);
        return 1;
    }
    return 0;
}

static int ensure_mask_api_ready(void) {
    if (g_mask_api_ready && refresh_async_mask_objects()) {
        return 1;
    }

    ensure_metadata_ready_lazy();
    if (!g_il2cpp_api.ready || g_il2cpp_api.field_static_get_value == NULL ||
        g_il2cpp_api.object_get_class == NULL || g_il2cpp_api.runtime_invoke == NULL) {
        return 0;
    }

    if (!refresh_async_mask_objects()) {
        LOGW("AsyncInputManager mask fields are not initialized yet");
        return 0;
    }
    LOGI("mask API static mask objects resolved key=%p frame=%p", g_async_key_mask, g_async_frame_key_mask);

    void *hashset_class = g_il2cpp_api.object_get_class(g_async_key_mask);
    if (hashset_class == NULL) {
        LOGW("HashSet<AsyncKeyCode> runtime class unavailable");
        return 0;
    }
    LOGI("mask API hashset class=%p", hashset_class);

    if (!cache_method_from_class(hashset_class, "Add", 1, &g_hashset_add_method) ||
        !cache_method_from_class(hashset_class, "Remove", 1, &g_hashset_remove_method) ||
        !cache_method_from_class(hashset_class, "Clear", 0, &g_hashset_clear_method) ||
        !cache_method_from_class(hashset_class, "get_Count", 0, &g_hashset_get_count_method) ||
        !ensure_process_key_inputs_ready()) {
        return 0;
    }

    g_mask_api_ready = 1;
    LOGI("AsyncInput mask API ready: AsyncKeyCode Space label=%d", ASYNC_KEY_LABEL_SPACE);
    return 1;
}

static int invoke_hashset_key_method(const Il2CppMethodCache *method, void *set, AsyncKeyCodeValue key) {
    if (method == NULL || method->method_info == NULL || set == NULL || !ensure_mask_api_ready()) {
        return 0;
    }
    void *params[1];
    void *exception = NULL;
    params[0] = &key;
    g_il2cpp_api.runtime_invoke(method->method_info, set, params, &exception);
    if (exception != NULL) {
        LOGW("HashSet<AsyncKeyCode> key method raised exception");
        return 0;
    }
    return 1;
}

static int hashset_add_key(void *set, AsyncKeyCodeValue key) {
    return invoke_hashset_key_method(&g_hashset_add_method, set, key);
}

static int __attribute__((unused)) hashset_remove_key(void *set, AsyncKeyCodeValue key) {
    return invoke_hashset_key_method(&g_hashset_remove_method, set, key);
}

static void hashset_clear(void *set) {
    if (set == NULL || !ensure_mask_api_ready()) {
        return;
    }
    void *exception = NULL;
    g_il2cpp_api.runtime_invoke(g_hashset_clear_method.method_info, set, NULL, &exception);
    if (exception != NULL) {
        LOGW("HashSet<AsyncKeyCode>.Clear raised exception");
    }
}

static void hashset_clear_if_ready(void *set) {
    if (set == NULL || !g_mask_api_ready || g_il2cpp_api.runtime_invoke == NULL ||
        g_hashset_clear_method.method_info == NULL) {
        return;
    }
    void *exception = NULL;
    g_il2cpp_api.runtime_invoke(g_hashset_clear_method.method_info, set, NULL, &exception);
    if (exception != NULL) {
        LOGW("HashSet<AsyncKeyCode>.Clear raised exception");
    }
}

static void async_masks_clear_frame_edges(void) {
    if (!ensure_mask_api_ready()) {
        return;
    }
    hashset_clear(g_async_key_down_mask);
    hashset_clear(g_async_key_up_mask);
    hashset_clear(g_async_frame_key_down_mask);
    hashset_clear(g_async_frame_key_up_mask);
}

static void async_masks_clear_tick_edges(void) {
    if (!ensure_mask_api_ready()) {
        return;
    }
    hashset_clear(g_async_key_down_mask);
    hashset_clear(g_async_key_up_mask);
}

static void async_masks_clear_all(void) {
    if (!ensure_mask_api_ready()) {
        return;
    }
    hashset_clear(g_async_key_mask);
    hashset_clear(g_async_key_down_mask);
    hashset_clear(g_async_key_up_mask);
    hashset_clear(g_async_frame_key_mask);
    hashset_clear(g_async_frame_key_down_mask);
    hashset_clear(g_async_frame_key_up_mask);
    g_managed_masks_need_clear = 0;
}

static void async_masks_clear_all_if_ready(void) {
    if (!g_mask_api_ready || !refresh_async_mask_objects()) {
        return;
    }
    hashset_clear_if_ready(g_async_key_mask);
    hashset_clear_if_ready(g_async_key_down_mask);
    hashset_clear_if_ready(g_async_key_up_mask);
    hashset_clear_if_ready(g_async_frame_key_mask);
    hashset_clear_if_ready(g_async_frame_key_down_mask);
    hashset_clear_if_ready(g_async_frame_key_up_mask);
    g_managed_masks_need_clear = 0;
}

static void apply_primary_key_masks(int down, int up, int held_count) {
    if (!ensure_mask_api_ready()) {
        return;
    }

    uint64_t down_mask = 0;
    uint64_t up_mask = 0;
    uint64_t held_mask = 0;
    snapshot_slot_masks(&down_mask, &up_mask, &held_mask);
    if (down > 0 && down_mask == 0) {
        int n = down < ASYNC_KEY_SLOT_COUNT ? down : ASYNC_KEY_SLOT_COUNT;
        down_mask = n >= 64 ? UINT64_MAX : ((1ULL << n) - 1ULL);
    }
    if (up > 0 && up_mask == 0) {
        int n = up < ASYNC_KEY_SLOT_COUNT ? up : ASYNC_KEY_SLOT_COUNT;
        up_mask = n >= 64 ? UINT64_MAX : ((1ULL << n) - 1ULL);
    }
    if (held_count > 0 && held_mask == 0) {
        int n = held_count < ASYNC_KEY_SLOT_COUNT ? held_count : ASYNC_KEY_SLOT_COUNT;
        held_mask = n >= 64 ? UINT64_MAX : ((1ULL << n) - 1ULL);
    }

    hashset_clear(g_async_key_mask);
    hashset_clear(g_async_frame_key_mask);
    for (int slot = 0; slot < ASYNC_KEY_SLOT_COUNT; ++slot) {
        uint64_t bit = 1ULL << slot;
        AsyncKeyCodeValue key = async_key_for_slot(slot);
        if ((held_mask & bit) != 0) {
            hashset_add_key(g_async_key_mask, key);
            hashset_add_key(g_async_frame_key_mask, key);
        }
        if ((down_mask & bit) != 0) {
            hashset_add_key(g_async_key_down_mask, key);
            hashset_add_key(g_async_frame_key_down_mask, key);
        }
        if ((up_mask & bit) != 0) {
            hashset_add_key(g_async_key_up_mask, key);
            hashset_add_key(g_async_frame_key_up_mask, key);
        }
    }

    if (g_trace_enabled && (down || up)) {
        uint64_t count = __atomic_add_fetch(&g_verify_mask_edge_count, 1, __ATOMIC_RELAXED);
        LOGI("VERIFY mask edge count=%" PRIu64 " down=%d up=%d held=%d downSlots=0x%" PRIx64 " upSlots=0x%" PRIx64 " heldSlots=0x%" PRIx64 " keyMask=%p keyDownMask=%p keyUpMask=%p maskCounts held=%d down=%d up=%d",
             count,
             down,
             up,
             held_count,
             down_mask,
             up_mask,
             held_mask,
             g_async_key_mask,
             g_async_key_down_mask,
             g_async_key_up_mask,
             hashset_count_if_ready(g_async_key_mask),
             hashset_count_if_ready(g_async_key_down_mask),
             hashset_count_if_ready(g_async_key_up_mask));
    }
}

static void set_input_type_active_object(void *object, int active) {
    if (object == NULL) {
        return;
    }
    uint8_t *field = (uint8_t *)((uintptr_t)object + g_offset_rdinputtype_is_active);
    *field = active ? 1 : 0;
}

static void set_input_type_active_field(void *field, int active) {
    void *object = NULL;
    if (read_static_object_field(field, &object)) {
        set_input_type_active_object(object, active);
    }
}

static void force_async_input_types(void) {
    ensure_metadata_ready_lazy();
    if (!g_il2cpp_api.ready || g_il2cpp_api.field_static_get_value == NULL) {
        return;
    }

    set_input_type_active_field(g_field_rdinput_keyboard_input, 0);
    set_input_type_active_field(g_field_rdinput_keyboard_left, 0);
    set_input_type_active_field(g_field_rdinput_keyboard_right, 0);
    set_input_type_active_field(g_field_rdinput_async_keyboard, 1);
    set_input_type_active_field(g_field_rdinput_async_keyboard_left, 1);
    set_input_type_active_field(g_field_rdinput_async_keyboard_right, 1);
    static uint64_t last_verify_active_log_ns = 0;
    uint64_t now_ns = monotonic_ns_now();
    if (g_trace_enabled && now_ns - last_verify_active_log_ns >= 1000000000ULL) {
        last_verify_active_log_ns = now_ns;
        void *keyboard = NULL;
        void *async_keyboard = NULL;
        void *async_left = NULL;
        void *async_right = NULL;
        read_static_object_field(g_field_rdinput_keyboard_input, &keyboard);
        read_static_object_field(g_field_rdinput_async_keyboard, &async_keyboard);
        read_static_object_field(g_field_rdinput_async_keyboard_left, &async_left);
        read_static_object_field(g_field_rdinput_async_keyboard_right, &async_right);
        int keyboard_active = keyboard ? *(uint8_t *)((uintptr_t)keyboard + g_offset_rdinputtype_is_active) : -1;
        int async_active = async_keyboard ? *(uint8_t *)((uintptr_t)async_keyboard + g_offset_rdinputtype_is_active) : -1;
        int async_left_active = async_left ? *(uint8_t *)((uintptr_t)async_left + g_offset_rdinputtype_is_active) : -1;
        int async_right_active = async_right ? *(uint8_t *)((uintptr_t)async_right + g_offset_rdinputtype_is_active) : -1;
        LOGI("VERIFY input types keyboard=%d async=%d asyncLeft=%d asyncRight=%d",
             keyboard_active, async_active, async_left_active, async_right_active);
    }
}

static void restore_regular_input_types(void) {
    if (!g_metadata_init_ready || !g_il2cpp_api.ready || g_il2cpp_api.field_static_get_value == NULL) {
        return;
    }

    set_input_type_active_field(g_field_rdinput_keyboard_input, 1);
    set_input_type_active_field(g_field_rdinput_keyboard_left, 1);
    set_input_type_active_field(g_field_rdinput_keyboard_right, 1);
    set_input_type_active_field(g_field_rdinput_async_keyboard, 0);
    set_input_type_active_field(g_field_rdinput_async_keyboard_left, 0);
    set_input_type_active_field(g_field_rdinput_async_keyboard_right, 0);
}

static void reset_metadata_state(void) {
    memset(&g_il2cpp_api, 0, sizeof(g_il2cpp_api));
    g_metadata_init_ready = 0;
    g_metadata_init_attempts = 0;
    g_mask_api_ready = 0;
    g_field_async_curr_frame_tick = NULL;
    g_field_async_prev_frame_tick = NULL;
    g_field_async_target_song_tick = NULL;
    g_field_async_offset_tick = NULL;
    g_field_async_offset_tick_updated = NULL;
    g_field_async_last_reported_target_tick = NULL;
    g_field_async_key_mask = NULL;
    g_field_async_key_down_mask = NULL;
    g_field_async_key_up_mask = NULL;
    g_field_async_frame_key_mask = NULL;
    g_field_async_frame_key_down_mask = NULL;
    g_field_async_frame_key_up_mask = NULL;
    g_field_rdinput_keyboard_input = NULL;
    g_field_rdinput_keyboard_left = NULL;
    g_field_rdinput_keyboard_right = NULL;
    g_field_rdinput_async_keyboard = NULL;
    g_field_rdinput_async_keyboard_left = NULL;
    g_field_rdinput_async_keyboard_right = NULL;
    g_field_gcs_current_speed_trial = NULL;
    g_field_gcs_hitmargin_counted = NULL;
    g_field_gcs_difficulty = NULL;
    g_field_gcs_hit_margin_limit = NULL;
    g_offset_scrplanet_player = 0;
    g_offset_scrplanet_currfloor = 0;
    g_offset_scrplanet_planetary_system = 0;
    g_offset_scrplanet_angle = 0;
    g_offset_scrplanet_cached_angle = 0;
    g_offset_scrplayer_planetary_system = 0;
    g_offset_scrplayer_last_hit = 0;
    g_offset_scrplayer_actual_last_hit = 0;
    g_offset_scrplayer_key_times = 0;
    g_offset_scrplayer_hold_keys = 0;
    g_offset_scrplayer_key_total = 0;
    g_offset_scrplayer_consec_multipress_counter = 0;
    g_offset_scrplayer_key_limiter_over_counter = 0;
    g_offset_scrplayer_taps_on_this_floor = 0;
    g_offset_scrplayer_alive = 0;
    g_offset_scrplayer_responsive = 0;
    g_offset_planetarysystem_chosen_planet = 0;
    g_offset_planetarysystem_speed = 0;
    g_offset_planetarysystem_is_cw = 0;
    g_offset_scrfloor_seq_id = 0;
    g_offset_scrfloor_nextfloor = 0;
    g_offset_scrfloor_margin_scale = 0;
    g_offset_scrfloor_auto = 0;
    g_offset_scrfloor_taps_needed = 0;
    g_offset_scrfloor_taps_so_far = 0;
    g_offset_scrfloor_hold_length = 0;
    g_offset_scrfloor_hold_completion = 0;
    g_offset_scrfloor_entry_time = 0;
    g_offset_scrfloor_entry_time_pitch_adj = 0;
    g_offset_scrconductor_song = 0;
    g_offset_scrconductor_bpm = 0;
    g_offset_scrconductor_addoffset = 0;
    g_offset_scrconductor_crotchet_at_start = 0;
    g_offset_scrconductor_dsp_time_song = 0;
    g_async_key_mask = NULL;
    g_async_key_down_mask = NULL;
    g_async_key_up_mask = NULL;
    g_async_frame_key_mask = NULL;
    g_async_frame_key_down_mask = NULL;
    g_async_frame_key_up_mask = NULL;
    g_field_scrplayer_should_replace_camy_to_pos = NULL;
    memset(&g_hashset_add_method, 0, sizeof(g_hashset_add_method));
    memset(&g_hashset_remove_method, 0, sizeof(g_hashset_remove_method));
    memset(&g_hashset_clear_method, 0, sizeof(g_hashset_clear_method));
    memset(&g_hashset_get_count_method, 0, sizeof(g_hashset_get_count_method));
    memset(&g_audio_get_dsptime_method, 0, sizeof(g_audio_get_dsptime_method));
    memset(&g_time_get_frame_count_method, 0, sizeof(g_time_get_frame_count_method));
    memset(&g_process_key_inputs_method, 0, sizeof(g_process_key_inputs_method));
    memset(&g_scrcontroller_get_chosen_planet_method, 0, sizeof(g_scrcontroller_get_chosen_planet_method));
    memset(&g_scrplanet_async_refresh_angles_method, 0, sizeof(g_scrplanet_async_refresh_angles_method));
    memset(&g_scrplanet_update_refresh_angles_method, 0, sizeof(g_scrplanet_update_refresh_angles_method));
    memset(&g_scrplanet_get_snapped_last_angle_method, 0, sizeof(g_scrplanet_get_snapped_last_angle_method));
    memset(&g_scrplanet_get_target_exit_angle_method, 0, sizeof(g_scrplanet_get_target_exit_angle_method));
    memset(&g_scrconductor_get_instance_method, 0, sizeof(g_scrconductor_get_instance_method));
    memset(&g_scrconductor_get_calibration_i_method, 0, sizeof(g_scrconductor_get_calibration_i_method));
    memset(&g_adobase_get_is_mobile_method, 0, sizeof(g_adobase_get_is_mobile_method));
    memset(&g_rdc_get_auto_method, 0, sizeof(g_rdc_get_auto_method));
    memset(&g_rdc_set_auto_method, 0, sizeof(g_rdc_set_auto_method));
    memset(&g_rdc_get_use_old_auto_method, 0, sizeof(g_rdc_get_use_old_auto_method));
    memset(&g_audiosource_get_pitch_method, 0, sizeof(g_audiosource_get_pitch_method));
    memset(&g_adobase_get_controller_method, 0, sizeof(g_adobase_get_controller_method));
    memset(&g_adobase_get_editor_method, 0, sizeof(g_adobase_get_editor_method));
    memset(&g_adobase_get_is_scn_game_method, 0, sizeof(g_adobase_get_is_scn_game_method));
    memset(&g_adobase_get_is_level_editor_method, 0, sizeof(g_adobase_get_is_level_editor_method));
    memset(&g_adobase_get_is_dlc_level_method, 0, sizeof(g_adobase_get_is_dlc_level_method));
    memset(&g_adobase_get_scene_name_method, 0, sizeof(g_adobase_get_scene_name_method));
    memset(&g_editor_get_play_mode_method, 0, sizeof(g_editor_get_play_mode_method));
}

static void resolve_async_metadata_fields(void) {
    if (!g_il2cpp_api.ready) {
        return;
    }

    g_offset_scrcontroller_current_state =
        resolve_field_offset_or_default("", "scrController", "currentState", OFFSET_SCRCONTROLLER_CURRENT_STATE);
    g_offset_scrcontroller_paused =
        resolve_field_offset_or_default("", "scrController", "_paused", OFFSET_SCRCONTROLLER_PAUSED);
    g_offset_scrcontroller_gameworld =
        resolve_field_offset_or_default("", "scrController", "gameworld", OFFSET_SCRCONTROLLER_GAMEWORLD);
    g_offset_statebehaviour_state_machine =
        resolve_field_offset_or_default("MonsterLove.StateMachine", "StateBehaviour", "_stateMachine", OFFSET_STATEBEHAVIOUR_STATE_MACHINE);
    g_offset_stateengine_current_state =
        resolve_field_offset_or_default("MonsterLove.StateMachine", "StateEngine", "currentState", OFFSET_STATEENGINE_CURRENT_STATE);
    g_offset_stateengine_destination_state =
        resolve_field_offset_or_default("MonsterLove.StateMachine", "StateEngine", "destinationState", OFFSET_STATEENGINE_DESTINATION_STATE);
    g_offset_statemapping_state =
        resolve_field_offset_or_default("MonsterLove.StateMachine", "StateMapping", "state", OFFSET_STATEMAPPING_STATE);
    g_offset_rdinputtype_is_active =
        resolve_field_offset_or_default("", "RDInputType", "_isActive", OFFSET_RDINPUTTYPE_IS_ACTIVE);
    g_offset_scneditor_paused_in_play_mode =
        resolve_field_offset_or_default("", "scnEditor", "pausedInPlayMode", OFFSET_SCNEDITOR_PAUSED_IN_PLAY_MODE);
    g_offset_scneditor_in_strictly_editing_mode =
        resolve_field_offset_or_default("", "scnEditor", "inStrictlyEditingMode", OFFSET_SCNEDITOR_IN_STRICTLY_EDITING_MODE);
    g_offset_scrplanet_player =
        resolve_field_offset_or_default("", "scrPlanet", "player", 0);
    g_offset_scrplanet_currfloor =
        resolve_field_offset_or_default("", "scrPlanet", "currfloor", 0);
    g_offset_scrplanet_planetary_system =
        resolve_field_offset_or_default("", "scrPlanet", "planetarySystem", 0);
    g_offset_scrplanet_angle =
        resolve_field_offset_or_default("", "scrPlanet", "angle", 0);
    g_offset_scrplanet_cached_angle =
        resolve_field_offset_or_default("", "scrPlanet", "cachedAngle", 0);
    g_offset_scrplayer_planetary_system =
        resolve_field_offset_or_default("", "scrPlayer", "planetarySystem", 0);
    g_offset_scrplayer_last_hit =
        resolve_field_offset_or_default("", "scrPlayer", "lastHit", 0);
    g_offset_scrplayer_actual_last_hit =
        resolve_field_offset_or_default("", "scrPlayer", "actualLastHit", 0);
    g_offset_scrplayer_key_times =
        resolve_field_offset_or_default("", "scrPlayer", "keyTimes", 0);
    g_offset_scrplayer_hold_keys =
        resolve_field_offset_or_default("", "scrPlayer", "holdKeys", 0);
    g_offset_scrplayer_key_total =
        resolve_field_offset_or_default("", "scrPlayer", "keyTotal", 0);
    g_offset_scrplayer_consec_multipress_counter =
        resolve_field_offset_or_default("", "scrPlayer", "consecMultipressCounter", 0);
    g_offset_scrplayer_key_limiter_over_counter =
        resolve_field_offset_or_default("", "scrPlayer", "keyLimiterOverCounter", 0);
    g_offset_scrplayer_taps_on_this_floor =
        resolve_field_offset_or_default("", "scrPlayer", "tapsOnThisFloor", 0);
    g_offset_scrplayer_alive =
        resolve_field_offset_or_default("", "scrPlayer", "alive", 0);
    g_offset_scrplayer_responsive =
        resolve_field_offset_or_default("", "scrPlayer", "responsive", 0);
    g_offset_planetarysystem_chosen_planet =
        resolve_field_offset_or_default("", "PlanetarySystem", "chosenPlanet", 0);
    g_offset_planetarysystem_speed =
        resolve_field_offset_or_default("", "PlanetarySystem", "speed", 0);
    g_offset_planetarysystem_is_cw =
        resolve_field_offset_or_default("", "PlanetarySystem", "isCW", 0);
    g_offset_scrfloor_seq_id =
        resolve_field_offset_or_default("", "scrFloor", "seqID", 0);
    g_offset_scrfloor_nextfloor =
        resolve_field_offset_or_default("", "scrFloor", "nextfloor", 0);
    g_offset_scrfloor_margin_scale =
        resolve_field_offset_or_default("", "scrFloor", "marginScale", 0);
    g_offset_scrfloor_auto =
        resolve_field_offset_or_default("", "scrFloor", "auto", 0);
    g_offset_scrfloor_taps_needed =
        resolve_field_offset_or_default("", "scrFloor", "tapsNeeded", 0);
    g_offset_scrfloor_taps_so_far =
        resolve_field_offset_or_default("", "scrFloor", "tapsSoFar", 0);
    g_offset_scrfloor_hold_length =
        resolve_field_offset_or_default("", "scrFloor", "holdLength", 0);
    g_offset_scrfloor_hold_completion =
        resolve_field_offset_or_default("", "scrFloor", "holdCompletion", 0);
    g_offset_scrfloor_entry_time =
        resolve_field_offset_or_default("", "scrFloor", "entryTime", 0);
    g_offset_scrfloor_entry_time_pitch_adj =
        resolve_field_offset_or_default("", "scrFloor", "entryTimePitchAdj", 0);
    g_offset_scrconductor_song =
        resolve_field_offset_or_default("", "scrConductor", "song", 0);
    g_offset_scrconductor_bpm =
        resolve_field_offset_or_default("", "scrConductor", "bpm", 0);
    g_offset_scrconductor_addoffset =
        resolve_field_offset_or_default("", "scrConductor", "addoffset", 0);
    g_offset_scrconductor_crotchet_at_start =
        resolve_field_offset_or_default("", "scrConductor", "crotchetAtStart", 0);
    g_offset_scrconductor_dsp_time_song =
        resolve_field_offset_or_default("", "scrConductor", "dspTimeSong", 0);
    g_field_async_last_reported_target_tick =
        resolve_field_or_null("", "AsyncInputManager", "lastReportedTargetTick");
    g_field_async_curr_frame_tick =
        resolve_field_or_null("", "AsyncInputManager", "currFrameTick");
    g_field_async_prev_frame_tick =
        resolve_field_or_null("", "AsyncInputManager", "prevFrameTick");
    g_field_async_target_song_tick =
        resolve_field_or_null("", "AsyncInputManager", "targetSongTick");
    g_field_async_offset_tick =
        resolve_field_or_null("", "AsyncInputManager", "offsetTick");
    g_field_async_offset_tick_updated =
        resolve_field_or_null("", "AsyncInputManager", "offsetTickUpdated");
    g_field_async_key_mask =
        resolve_field_or_null("", "AsyncInputManager", "keyMask");
    g_field_async_key_down_mask =
        resolve_field_or_null("", "AsyncInputManager", "keyDownMask");
    g_field_async_key_up_mask =
        resolve_field_or_null("", "AsyncInputManager", "keyUpMask");
    g_field_async_frame_key_mask =
        resolve_field_or_null("", "AsyncInputManager", "frameDependentKeyMask");
    g_field_async_frame_key_down_mask =
        resolve_field_or_null("", "AsyncInputManager", "frameDependentKeyDownMask");
    g_field_async_frame_key_up_mask =
        resolve_field_or_null("", "AsyncInputManager", "frameDependentKeyUpMask");
    g_field_scrplayer_should_replace_camy_to_pos =
        resolve_field_or_null("", "scrPlayer", "shouldReplaceCamyToPos");
    g_field_rdinput_keyboard_input =
        resolve_field_or_null("", "RDInput", "keyboardInput");
    g_field_rdinput_keyboard_left =
        resolve_field_or_null("", "RDInput", "keyboardLeft");
    g_field_rdinput_keyboard_right =
        resolve_field_or_null("", "RDInput", "keyboardRight");
    g_field_rdinput_async_keyboard =
        resolve_field_or_null("", "RDInput", "asyncKeyboard");
    g_field_rdinput_async_keyboard_left =
        resolve_field_or_null("", "RDInput", "asyncKeyboardLeft");
    g_field_rdinput_async_keyboard_right =
        resolve_field_or_null("", "RDInput", "asyncKeyboardRight");
    g_field_gcs_current_speed_trial =
        resolve_field_or_null("", "GCS", "currentSpeedTrial");
    g_field_gcs_hitmargin_counted =
        resolve_field_or_null("", "GCS", "HITMARGIN_COUNTED");
    g_field_gcs_difficulty =
        resolve_field_or_null("", "GCS", "difficulty");
    g_field_gcs_hit_margin_limit =
        resolve_field_or_null("", "GCS", "hitMarginLimit");
}

static void ensure_metadata_ready_lazy(void) {
    pthread_mutex_lock(&g_metadata_lock);
    if (g_metadata_init_ready) {
        pthread_mutex_unlock(&g_metadata_lock);
        return;
    }

    g_metadata_init_attempts++;
    if (!init_il2cpp_api()) {
        if (!il2cpp_runtime_ready_for_metadata()) {
            g_metadata_init_attempts = 0;
        }
        pthread_mutex_unlock(&g_metadata_lock);
        return;
    }

    resolve_async_metadata_fields();
    g_metadata_init_ready = 1;
    pthread_mutex_unlock(&g_metadata_lock);
}

static int install_hook(uintptr_t target, uintptr_t replacement, void **original_out, const char *name) {
    if (target == 0 || replacement == 0 || original_out == NULL) {
        return 0;
    }
    int rc = DobbyHook((void *)target, (void *)replacement, original_out);
    if (rc != 0 || *original_out == NULL) {
        LOGE("%s DobbyHook failed target=0x%lx rc=%d original=%p", name, (unsigned long)target, rc, *original_out);
        return 0;
    }
    LOGI("%s Dobby hook installed target=0x%lx original=%p", name, (unsigned long)target, *original_out);
    return 1;
}

static void mark_il2cpp_init_completed(const char *reason) {
    pthread_mutex_lock(&g_il2cpp_state_lock);
    if (!g_il2cpp_init_completed) {
        g_il2cpp_init_completed = 1;
        LOGI("IL2CPP init completed by %s", reason ? reason : "unknown");
    }
    pthread_cond_broadcast(&g_il2cpp_state_cond);
    pthread_mutex_unlock(&g_il2cpp_state_lock);
}

static int hooked_il2cpp_init(const char *domain_name) {
    int result = 0;
    if (g_original_il2cpp_init != NULL) {
        result = g_original_il2cpp_init(domain_name);
    }
    mark_il2cpp_init_completed("il2cpp_init");
    return result;
}

static int hooked_il2cpp_init_utf16(const uint16_t *domain_name) {
    int result = 0;
    if (g_original_il2cpp_init_utf16 != NULL) {
        result = g_original_il2cpp_init_utf16(domain_name);
    }
    mark_il2cpp_init_completed("il2cpp_init_utf16");
    return result;
}

static void install_il2cpp_state_hook_symbol(const char *symbol, uintptr_t replacement, void **original_out, volatile int *installed_flag) {
    if (symbol == NULL || replacement == 0 || original_out == NULL || installed_flag == NULL || *installed_flag) {
        return;
    }

    void *target = resolve_dynsym("libil2cpp.so", symbol);
    if (target == NULL) {
        return;
    }
    if (install_hook((uintptr_t)target, replacement, original_out, symbol)) {
        *installed_flag = 1;
    }
}

static void install_il2cpp_state_hooks(void) {
    if (g_il2cpp_state_hooks_installed) {
        return;
    }

    install_il2cpp_state_hook_symbol(
        "il2cpp_init",
        (uintptr_t)&hooked_il2cpp_init,
        (void **)&g_original_il2cpp_init,
        &g_il2cpp_init_hook_installed);
    install_il2cpp_state_hook_symbol(
        "il2cpp_init_utf16",
        (uintptr_t)&hooked_il2cpp_init_utf16,
        (void **)&g_original_il2cpp_init_utf16,
        &g_il2cpp_init_utf16_hook_installed);
    g_il2cpp_state_hooks_installed =
        g_il2cpp_init_hook_installed ||
        g_il2cpp_init_utf16_hook_installed;

    if (g_il2cpp_state_hooks_installed) {
        LOGI("IL2CPP state hooks installed init=%d init_utf16=%d",
             g_il2cpp_init_hook_installed,
             g_il2cpp_init_utf16_hook_installed);
    } else {
        LOGW("no IL2CPP state hook symbols resolved yet");
    }
}

static int __attribute__((unused)) hook_targets_resolved_by_metadata(const MethodHookSpec *hooks, size_t hook_count) {
    if (!g_il2cpp_api.ready || hooks == NULL || hook_count == 0) {
        return 0;
    }

    for (size_t i = 0; i < hook_count; ++i) {
        uintptr_t target = resolve_method_pointer_quiet(
            hooks[i].namespaze,
            hooks[i].klass,
            hooks[i].method,
            hooks[i].args_count);
        if (target == 0) {
            return 0;
        }
    }
    return 1;
}

static void wait_for_il2cpp_runtime_state(void) {
    pthread_mutex_lock(&g_il2cpp_state_lock);
    while (!g_il2cpp_init_completed) {
        pthread_mutex_unlock(&g_il2cpp_state_lock);
        if (il2cpp_runtime_ready_for_metadata()) {
            return;
        }
        pthread_mutex_lock(&g_il2cpp_state_lock);
        if (!g_il2cpp_init_completed) {
            struct timespec deadline;
            clock_gettime(CLOCK_REALTIME, &deadline);
            deadline.tv_nsec += IL2CPP_METADATA_POLL_US * 1000L;
            if (deadline.tv_nsec >= 1000000000L) {
                deadline.tv_sec += deadline.tv_nsec / 1000000000L;
                deadline.tv_nsec %= 1000000000L;
            }
            pthread_cond_timedwait(&g_il2cpp_state_cond, &g_il2cpp_state_lock, &deadline);
        }
    }
    pthread_mutex_unlock(&g_il2cpp_state_lock);
}

static int __attribute__((unused)) wait_for_metadata_hook_targets(const MethodHookSpec *hooks, size_t hook_count) {
    LOGI("waiting for IL2CPP runtime state before installing metadata hooks");

    for (;;) {
        wait_for_il2cpp_runtime_state();

        pthread_mutex_lock(&g_metadata_lock);
        if (init_il2cpp_api()) {
            resolve_async_metadata_fields();
            if (hook_targets_resolved_by_metadata(hooks, hook_count)) {
                g_metadata_init_ready = 1;
                g_metadata_init_attempts = 0;
                pthread_mutex_unlock(&g_metadata_lock);
                LOGI("IL2CPP metadata hook targets ready");
                return 1;
            }
            LOGW("IL2CPP runtime ready but hook targets not resolved yet");
        }
        pthread_mutex_unlock(&g_metadata_lock);
        usleep(IL2CPP_METADATA_POLL_US);
    }
}

static int install_method_hook(const MethodHookSpec *spec) {
    uintptr_t target = resolve_method_pointer(spec->namespaze, spec->klass, spec->method, spec->args_count);
    if (target == 0) {
        LOGE("%s metadata target unavailable; hook not installed", spec->label);
        return 0;
    }
    LOGI("%s resolved by metadata target=0x%lx", spec->label, (unsigned long)target);
    return install_hook(target, (uintptr_t)spec->replacement, spec->original_out, spec->label);
}

static void *patch_thread_main(void *arg) {
    (void)arg;
    LOGI("patch thread started, waiting for libil2cpp.so");

    for (int i = 0; i < IL2CPP_BASE_WAIT_ATTEMPTS; ++i) {
        uintptr_t base = find_module_base_from_maps("libil2cpp.so");
        if (base == 0) {
            usleep(IL2CPP_BASE_POLL_US);
            continue;
        }

        LOGI("libil2cpp.so found base=0x%lx", (unsigned long)base);
        install_il2cpp_state_hooks();
        update_time_origin();
        set_enabled_internal(load_bool_file(CFG_PATH, 0), 0);
        g_auto_replay_enabled = load_bool_file(AUTO_REPLAY_CFG_PATH, 1);
        g_trace_enabled = load_bool_file(TRACE_CFG_PATH, 0);
        LOGI("async auto replay default=%d", g_auto_replay_enabled);
        LOGI("async trace log default=%d", g_trace_enabled);

        reset_metadata_state();
        g_offset_scrcontroller_current_state = OFFSET_SCRCONTROLLER_CURRENT_STATE;
        g_offset_scrcontroller_paused = OFFSET_SCRCONTROLLER_PAUSED;
        g_offset_scrcontroller_gameworld = OFFSET_SCRCONTROLLER_GAMEWORLD;
        g_offset_statebehaviour_state_machine = OFFSET_STATEBEHAVIOUR_STATE_MACHINE;
        g_offset_stateengine_current_state = OFFSET_STATEENGINE_CURRENT_STATE;
        g_offset_stateengine_destination_state = OFFSET_STATEENGINE_DESTINATION_STATE;
        g_offset_statemapping_state = OFFSET_STATEMAPPING_STATE;
        g_offset_rdinputtype_is_active = OFFSET_RDINPUTTYPE_IS_ACTIVE;

        MethodHookSpec hooks[] = {
            {"", "AsyncInputManager", "get_isActive", 0, (void *)&hooked_async_is_active, (void **)&g_original_is_active, "AsyncInputManager.get_isActive"},
            {"", "AsyncInputUtils", "UpdateOffsetTime", 1, (void *)&hooked_update_offset_time, (void **)&g_original_update_offset_time, "AsyncInputUtils.UpdateOffsetTime"},
            {"", "AsyncInputUtils", "AdjustAngle", 2, (void *)&hooked_adjust_angle, (void **)&g_original_adjust_angle, "AsyncInputUtils.AdjustAngle"},
            {"", "scrMisc", "GetHitMargin", 6, (void *)&hooked_get_hit_margin, (void **)&g_original_get_hit_margin, "scrMisc.GetHitMargin"},
            {"", "scrPlayer", "ValidInputWasTriggered", 0, (void *)&hooked_valid_triggered, (void **)&g_original_valid_triggered, "scrPlayer.ValidInputWasTriggered"},
            {"", "scrPlayer", "ValidInputWasReleased", 0, (void *)&hooked_valid_released, (void **)&g_original_valid_released, "scrPlayer.ValidInputWasReleased"},
            {"", "scrPlayer", "CountValidKeysPressed", 0, (void *)&hooked_count_valid_keys, (void **)&g_original_count_valid_keys, "scrPlayer.CountValidKeysPressed"},
            {"", "scrPlayer", "get_touchEnabled", 0, (void *)&hooked_touch_enabled, (void **)&g_original_touch_enabled, "scrPlayer.get_touchEnabled"},
            {"", "scrPlayer", "get_holding", 0, (void *)&hooked_get_holding, (void **)&g_original_get_holding, "scrPlayer.get_holding"},
            {"", "scrPlayer", "get_auto", 0, (void *)&hooked_scrplayer_get_auto, (void **)&g_original_scrplayer_get_auto, "scrPlayer.get_auto"},
            {"", "scrPlayer", "Hit", 1, (void *)&hooked_scrplayer_hit, (void **)&g_original_scrplayer_hit, "scrPlayer.Hit"},
            {"", "scrPlanet", "SwitchChosen", 0, (void *)&hooked_scrplanet_switch_chosen, (void **)&g_original_scrplanet_switch_chosen, "scrPlanet.SwitchChosen"},
            {"", "scrCamera", "UpdateFollowCam", 1, (void *)&hooked_camera_update_follow_cam, (void **)&g_original_camera_update_follow_cam, "scrCamera.UpdateFollowCam"},
            {"", "scnEditor", "Update", 0, (void *)&hooked_editor_update, (void **)&g_original_editor_update, "scnEditor.Update"},
            {"", "scrController", "PlayerControl_Update", 0, (void *)&hooked_playercontrol_update, (void **)&g_original_playercontrol_update, "scrController.PlayerControl_Update"},
        };

        /*
         * Do not Dobby-hook scrController.UpdateInput in this Android IL2CPP
         * build. Its method pointer resolves to a single ret at 0x24b7660,
         * immediately followed by scrConductor.PlayWithEndTime at 0x24b7664.
         * An inline hook must overwrite more than one AArch64 instruction and
         * corrupts PlayWithEndTime's prologue, which makes DLC hold-loop sound
         * scheduling crash even when async input is disabled.
         *
         * PlayerControl_Update remains the safe required hook point and calls
         * hooked_update_input() as an internal replay driver.
         */

        size_t hook_count = sizeof(hooks) / sizeof(hooks[0]);
        int metadata_ready = wait_for_metadata_hook_targets(hooks, hook_count);
        if (!metadata_ready) {
            LOGE("metadata hook targets unavailable; aborting hook install");
            return NULL;
        }

        int ok = 1;
        for (size_t j = 0; j < hook_count; ++j) {
            ok &= install_method_hook(&hooks[j]);
        }

        if (!ok) {
            LOGE("one or more hooks failed");
            return NULL;
        }

        g_hooks_installed = 1;
        LOGI("all hooks installed, enabled=%d, metadata_ready=%d", g_enabled, metadata_ready);
        return NULL;
    }

    LOGW("timed out waiting for libil2cpp.so");
    return NULL;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)vm;
    (void)reserved;
    LOGI("JNI_OnLoad entered");

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, patch_thread_main, NULL);
    if (rc != 0) {
        LOGE("pthread_create failed rc=%d", rc);
        return JNI_VERSION_1_6;
    }
    pthread_detach(thread);

    LOGI("JNI_OnLoad finished");
    return JNI_VERSION_1_6;
}
