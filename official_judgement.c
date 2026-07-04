#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef ADO_OFFICIAL_JUDGEMENT_SELFTEST
#include <stdio.h>
#endif

#define ADO_OJ_KEY_SLOTS 64
#define ADO_OJ_PI 3.1415927410125732
#define ADO_OJ_ASYNC_PI 3.141592653598793
#define ADO_OJ_TWO_PI 6.2831854820251465
#define ADO_OJ_RAD2DEG 57.295780181884766

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

typedef enum AdoOfficialHitMarginGeneral {
    ADO_MARGIN_COUNTED = 0,
    ADO_MARGIN_PERFECT = 1,
    ADO_MARGIN_PURE = 2
} AdoOfficialHitMarginGeneral;

typedef enum AdoOfficialHitMarginLimit {
    ADO_MARGIN_LIMIT_NONE = 0,
    ADO_MARGIN_LIMIT_PERFECTS_ONLY = 1,
    ADO_MARGIN_LIMIT_PURE_PERFECT_ONLY = 2
} AdoOfficialHitMarginLimit;

typedef enum AdoOfficialConditionalEffect {
    ADO_CONDITIONAL_NONE = 0,
    ADO_CONDITIONAL_PERFECT = 1,
    ADO_CONDITIONAL_EARLY_PERFECT = 2,
    ADO_CONDITIONAL_LATE_PERFECT = 3,
    ADO_CONDITIONAL_VERY_EARLY = 4,
    ADO_CONDITIONAL_VERY_LATE = 5,
    ADO_CONDITIONAL_TOO_EARLY = 6,
    ADO_CONDITIONAL_TOO_LATE = 7,
    ADO_CONDITIONAL_LOSS = 8,
    ADO_CONDITIONAL_ON_CHECKPOINT = 9
} AdoOfficialConditionalEffect;

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

typedef struct AdoOfficialFloor {
    int seq_id;
    int taps_needed;
    int taps_so_far;
    int num_planets;
    int hold_length;
    int mid_spin;
    int is_ccw;
    int is_safe;
    int auto_floor;
    int freeroam;
    int hide_judgment;
    int has_conditional_change;
    int conditional_perfect_count;
    int conditional_early_perfect_count;
    int conditional_late_perfect_count;
    int conditional_very_early_count;
    int conditional_very_late_count;
    int conditional_too_early_count;
    int conditional_too_late_count;
    int conditional_loss_count;
    int conditional_on_checkpoint_count;
    int freeroam_generated;
    int unstable;
    int is_swirl;
    int is_portal;
    int is_landable;
    int is_landable_set;
    int collider_enabled;
    int hidden_by_unstable;
    int freeroam_region;
    int angle_correction_type;
    double speed;
    double margin_scale;
    double entryangle;
    double exitangle;
    double angle_length;
    double entry_time;
    double extra_beats;
    double hold_completion;
    double hold_margin_override;
    struct AdoOfficialFloor *prev;
    struct AdoOfficialFloor *next;
} AdoOfficialFloor;

typedef struct AdoOfficialController {
    int gameworld;
    int paused;
    int is_cutscene;
    int is_level_editor;
    int exiting_to_main_menu;
    int is_fail_state;
    int is_player_control_state;
    int no_fail;
    int no_fail_infinite_margin;
    int strict_holds;
    int require_holding;
    int benchmark_mode;
    int unlock_key_limiter;
    int floor_count;
    int current_floor_id;
    int current_seq_id;
    int checkpoint_seq_id;
    int multipress_penalty;
    int multipress_and_has_pressed_first_press;
    int maximum_used_keys;
    int is_official_level;
    int is_expo;
    int is_puzzle_room;
    int independent_players;
    int win_time_set;
    int lives;
    double percent_complete;
    double average_frame_time;
    double booth_mode_debounce_counter;
    AdoOfficialFloor *curr_floor;
} AdoOfficialController;

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
    double down_key_duration_time[ADO_OJ_KEY_SLOTS];
    double failbar_overload_counter;
    double failbar_multipress_counter;
    double failbar_multipress_reset_counter;
    double miss_cooldown_remaining;
    double lock_input;
    double last_hit;
    double actual_last_hit;
} AdoOfficialPlayer;

typedef struct AdoOfficialPlanet {
    AdoOfficialFloor *currfloor;
    double snapped_last_angle;
    double target_exit_angle;
    double angle;
    double cached_angle;
    double speed;
    int is_cw;
} AdoOfficialPlanet;

typedef struct AdoOfficialRuntime {
    AdoOfficialController controller;
    AdoOfficialPlayer player;
    AdoOfficialPlanet planet;
    int next_tile_is_hold_cached;
    int current_frame_count;
    uint64_t current_event_tick;
    double current_song_position;
    double lock_time;
    AdoOfficialFloor *freeroam_target_floor;
    double freeroam_exit_angle;
    int freeroam_selection_ready;
    int freeroam_candidate_occupied_by_player;
    int freeroam_landing_checked;
    AdoOfficialFloor *freeroam_landing_floor;
    int revive_bubble_intercept;
    int freeroam_invalid_after_move;
    AdoOfficialFloor *conditional_floor;
} AdoOfficialRuntime;

typedef struct AdoOfficialInputEvent {
    uint64_t event_tick;
    int went_down_count;
    int went_up_count;
    int held_count;
    int trigger_valid_key_count;
    int trigger_key_limiter_over_count;
    int enqueue_valid_key_count;
    int enqueue_key_limiter_over_count;
    int valid_key_count;
    int key_limiter_over_count;
    int active_down_key_count;
    int pressed_held_key_count;
    int held_key_count;
    int release_valid_key_count;
    int force_valid_input_released;
    int touch_hold_clear_release;
    int hit_input_ignore_add_count;
    int release_input_ignore_add_count;
    uint8_t hit_input_slot_mask;
    uint8_t release_input_slot_mask;
    uint8_t hit_input_ignore_slot_mask;
    uint8_t release_input_ignore_slot_mask;
    int frame_count;
    uint64_t pressed_keys_mask;
    uint64_t held_keys_mask;
    uint64_t released_keys_mask;
    double delta_unscaled_time;
    double delta_song_pos;
} AdoOfficialInputEvent;

typedef struct AdoOfficialStepResult {
    int processed;
    int moved_to_next_floor;
    int hit_returned_true;
    int damaged;
    int died;
    int overpress;
    int multipress;
    int auto_hit;
    int hold_fail;
    int hold_cancelled;
    int post_hold_fail;
    int skipped_by_hold_guard;
    int partial_multitap;
    int input_ignored;
    int no_fail_recovered;
    int hit_input_event_only;
    int release_input_event_only;
    int damage_failed;
    int death_suppressed;
    int death_suppressed_by_auto_or_debug;
    int death_suppressed_by_no_fail;
    int death_suppressed_by_safe_floor;
    int death_suppressed_by_miss_cooldown;
    int failbar_damaged;
    int failbar_failed;
    int miss_marked;
    int fail_marked;
    int miss_cooldown_started;
    int midspin_key_appended;
    int midspin_second_hit;
    int life_lost;
    int lives_remaining_after;
    int revive_bubble_intercepted;
    int conditional_floor_set;
    int conditional_effect_triggered;
    int puzzle_iframes_applied;
    int portal_landed;
    int player_made_unresponsive;
    int freeroam_target_selected;
    int freeroam_candidate_rejected;
    int freeroam_entered;
    int freeroam_colliders_disabled;
    int error_meter_hidden;
    int error_meter_shown;
    int freeroam_unstable_consumed;
    int freeroam_swirl_flipped;
    int freeroam_landed_on_floor;
    int planet_count_changed;
    int floor_lit;
    int ffx_triggered;
    int invalid_freeroam;
    int drum_debounce_returned;
    int drum_debounce_set;
    int pause_handled;
    int input_locked;
    int input_unlocked;
    int hit_input_event_frame_gate_open;
    int release_input_event_frame_gate_open;
    uint8_t hit_input_slots_handled;
    uint8_t release_input_slots_handled;
    uint8_t hit_input_ignore_slots_handled;
    uint8_t release_input_ignore_slots_handled;
    int ending_started;
    int percent_updated;
    int level_hit_emitted;
    int tracker_hit_added;
    int hit_text_shown;
    int floor_grade_set;
    int error_meter_hit_added;
    int camera_pulse_allowed;
    int avatar_hit_emitted;
    int key_total_added;
    int key_limiter_over_count;
    int active_down_key_count;
    int maximum_used_keys_after;
    int pending_key_times_after;
    int pending_midspin_key_times_after;
    int hold_key_count_after;
    int holding_after;
    int curr_seq_id_after;
    int controller_seq_id_after;
    int death_registered_floor_id;
    int target_seq_id;
    int moving_to_seq_id;
    int freeroam_landing_seq_id;
    AdoOfficialHitMargin raw_margin;
    AdoOfficialHitMargin effective_margin;
    AdoOfficialHitMargin display_margin;
    AdoOfficialHitMargin floor_grade_margin;
    AdoOfficialHitMargin tracker_margin;
    AdoOfficialHitMargin hit_text_margin;
    AdoOfficialHitMargin conditional_effect_margin;
    AdoOfficialConditionalEffect conditional_effect;
    double hit_angle;
    double target_angle;
    double signed_angle_delta_deg;
    double signed_time_offset_ms;
    double song_position;
} AdoOfficialStepResult;

typedef struct AdoOfficialJudgementOnly {
    AdoOfficialHitMargin margin;
    double signed_angle_delta_deg;
    double signed_time_offset_ms;
} AdoOfficialJudgementOnly;

static void ado_oj_die_or_recover(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    int overload,
    int hitbox,
    AdoOfficialStepResult *result);
static int ado_oj_popcount64(uint64_t mask);
double ado_official_get_angle(
    const AdoOfficialConductor *conductor,
    const AdoOfficialPlayer *player,
    const AdoOfficialPlanet *planet,
    double snapped_last_angle,
    uint64_t now_tick);

static AdoOfficialInputEvent ado_oj_empty_input(void) {
    AdoOfficialInputEvent input;
    memset(&input, 0, sizeof(input));
    return input;
}

const char *ado_official_judgement_version(void) {
    return "ado_official_judgement_level0_freeroam_select";
}

static double ado_oj_max(double a, double b) {
    return a > b ? a : b;
}

static double ado_oj_mod(double x, double m) {
    double r = fmod(x, m);
    if (r < 0.0) {
        r += m;
    }
    return r;
}

static double ado_oj_bpm_to_crotchet(double bpm) {
    if (bpm <= 0.0) {
        return 0.0;
    }
    return 60.0 / bpm;
}

static double ado_oj_time_to_angle_rad(double seconds, double bpm_times_speed, double conductor_pitch) {
    double crotchet = ado_oj_bpm_to_crotchet(bpm_times_speed);
    if (crotchet <= 0.0) {
        return 0.0;
    }
    return seconds * conductor_pitch * ADO_OJ_PI / crotchet;
}

static double ado_oj_get_angle_moved(double entry_angle, double exit_angle, int is_cw) {
    double sign = is_cw ? 1.0 : -1.0;
    return ado_oj_mod((exit_angle - entry_angle) * sign, ADO_OJ_TWO_PI);
}

static double ado_oj_angle_to_time(double angle, double bpm) {
    double crotchet = ado_oj_bpm_to_crotchet(bpm);
    return ado_oj_mod(angle, ADO_OJ_TWO_PI) / ADO_OJ_PI * crotchet;
}

static double ado_oj_inverse_angle_per_beat_multiplanet(double planets) {
    return 3.1415926 * (planets - 2.0) / planets;
}

static void ado_oj_sync_controller_floor(AdoOfficialRuntime *runtime, AdoOfficialFloor *floor, AdoOfficialStepResult *result) {
    if (runtime == NULL || floor == NULL) {
        return;
    }
    runtime->controller.curr_floor = floor;
    runtime->controller.current_seq_id = floor->seq_id;
    if (floor->seq_id > runtime->controller.current_floor_id) {
        runtime->controller.current_floor_id = floor->seq_id;
        if (result != NULL) {
            result->level_hit_emitted = 1;
        }
    }
    if (runtime->controller.floor_count > 0) {
        runtime->controller.percent_complete =
            (double)(runtime->controller.current_seq_id + 1) /
            (double)runtime->controller.floor_count;
        if (result != NULL) {
            result->percent_updated = 1;
        }
    }
}

void ado_official_config_defaults(AdoOfficialConfig *config) {
    if (config == NULL) {
        return;
    }
    memset(config, 0, sizeof(*config));
    config->difficulty = ADO_DIFFICULTY_NORMAL;
    config->hit_margin_limit = ADO_MARGIN_LIMIT_NONE;
    config->current_speed_trial = 1.0;
    config->hitmargin_counted = 60.0;
    config->quantize_hitmargin_float = 1;
}

void ado_official_floor_defaults(AdoOfficialFloor *floor) {
    if (floor == NULL) {
        return;
    }
    memset(floor, 0, sizeof(*floor));
    floor->taps_needed = 1;
    floor->num_planets = 2;
    floor->hold_length = -1;
    floor->speed = 1.0;
    floor->margin_scale = 1.0;
    floor->is_landable = 1;
    floor->is_landable_set = 1;
    floor->collider_enabled = 1;
}

void ado_official_runtime_defaults(AdoOfficialRuntime *runtime) {
    if (runtime == NULL) {
        return;
    }
    memset(runtime, 0, sizeof(*runtime));
    runtime->controller.gameworld = 1;
    runtime->controller.is_player_control_state = 1;
    runtime->player.alive = 1;
    runtime->player.responsive = 1;
    runtime->planet.speed = 1.0;
    runtime->planet.is_cw = 1;
    runtime->freeroam_selection_ready = 1;
}

double ado_official_get_adjusted_angle_boundary_deg(
    const AdoOfficialConfig *config,
    AdoOfficialHitMarginGeneral margin_type,
    double bpm_times_speed,
    double conductor_pitch,
    double margin_mult) {
    AdoOfficialConfig fallback;
    if (config == NULL) {
        ado_official_config_defaults(&fallback);
        config = &fallback;
    }

    double speed_trial = config->current_speed_trial;
    if (speed_trial <= 0.0) {
        speed_trial = 1.0;
    }

    float speed_trial_f = (float)speed_trial;
    float counted_seconds_f = 0.065f;
    if (config->difficulty == ADO_DIFFICULTY_LENIENT) {
        counted_seconds_f = 0.091f;
    } else if (config->difficulty == ADO_DIFFICULTY_STRICT) {
        counted_seconds_f = 0.04f;
    }

    counted_seconds_f = config->is_mobile ? 0.09f : counted_seconds_f / speed_trial_f;
    float perfect_seconds_f = config->is_mobile ? 0.07f : 0.03f / speed_trial_f;
    float pure_seconds_f = config->is_mobile ? 0.05f : 0.02f / speed_trial_f;

    counted_seconds_f = (float)ado_oj_max((double)counted_seconds_f, (double)0.025f);
    perfect_seconds_f = (float)ado_oj_max((double)perfect_seconds_f, (double)0.025f);
    pure_seconds_f = (float)ado_oj_max((double)pure_seconds_f, (double)0.025f);

    double counted_by_time = ado_oj_time_to_angle_rad((double)counted_seconds_f, bpm_times_speed, conductor_pitch) * ADO_OJ_RAD2DEG;
    double perfect_by_time = ado_oj_time_to_angle_rad((double)perfect_seconds_f, bpm_times_speed, conductor_pitch) * ADO_OJ_RAD2DEG;
    double pure_by_time = ado_oj_time_to_angle_rad((double)pure_seconds_f, bpm_times_speed, conductor_pitch) * ADO_OJ_RAD2DEG;

    double counted = ado_oj_max(config->hitmargin_counted * margin_mult, counted_by_time);
    double perfect = ado_oj_max(45.0 * margin_mult, perfect_by_time);
    double pure = ado_oj_max(30.0 * margin_mult, pure_by_time);

    if (margin_type == ADO_MARGIN_PERFECT) {
        return perfect;
    }
    if (margin_type == ADO_MARGIN_PURE) {
        return pure;
    }
    return counted;
}

AdoOfficialHitMargin ado_official_get_hit_margin(
    const AdoOfficialConfig *config,
    double hit_angle,
    double ref_angle,
    int is_cw,
    double bpm_times_speed,
    double conductor_pitch,
    double margin_scale) {
    if (config != NULL && config->quantize_hitmargin_float) {
        hit_angle = (double)(float)hit_angle;
        ref_angle = (double)(float)ref_angle;
        bpm_times_speed = (double)(float)bpm_times_speed;
        conductor_pitch = (double)(float)conductor_pitch;
    }
    double delta = (hit_angle - ref_angle) * (is_cw ? 1.0 : -1.0);
    double delta_deg = config != NULL && config->quantize_hitmargin_float
                           ? (double)(57.29578f * (float)delta)
                           : delta * ADO_OJ_RAD2DEG;
    double counted = ado_official_get_adjusted_angle_boundary_deg(
        config, ADO_MARGIN_COUNTED, bpm_times_speed, conductor_pitch, margin_scale);
    double perfect = ado_official_get_adjusted_angle_boundary_deg(
        config, ADO_MARGIN_PERFECT, bpm_times_speed, conductor_pitch, margin_scale);
    double pure = ado_official_get_adjusted_angle_boundary_deg(
        config, ADO_MARGIN_PURE, bpm_times_speed, conductor_pitch, margin_scale);

    AdoOfficialHitMargin result = ADO_HIT_TOO_EARLY;
    if (delta_deg > -counted) {
        result = ADO_HIT_VERY_EARLY;
    }
    if (delta_deg > -perfect) {
        result = ADO_HIT_EARLY_PERFECT;
    }
    if (delta_deg > -pure) {
        result = ADO_HIT_PERFECT;
    }
    if (delta_deg > pure) {
        result = ADO_HIT_LATE_PERFECT;
    }
    if (delta_deg > perfect) {
        result = ADO_HIT_VERY_LATE;
    }
    if (delta_deg > counted) {
        result = ADO_HIT_TOO_LATE;
    }
    return result;
}

AdoOfficialJudgementOnly ado_official_judge_angle_only(
    const AdoOfficialConfig *config,
    double hit_angle,
    double ref_angle,
    int is_cw,
    double bpm_times_speed,
    double conductor_pitch,
    double margin_scale) {
    AdoOfficialJudgementOnly result;
    memset(&result, 0, sizeof(result));
    result.margin = ado_official_get_hit_margin(
        config,
        hit_angle,
        ref_angle,
        is_cw,
        bpm_times_speed,
        conductor_pitch,
        margin_scale);

    float hit_f = (float)hit_angle;
    float ref_f = (float)ref_angle;
    float delta = (hit_f - ref_f) * (float)(is_cw ? 1 : -1);
    result.signed_angle_delta_deg = (double)(57.29578f * delta);

    double crotchet = ado_oj_bpm_to_crotchet(bpm_times_speed);
    if (conductor_pitch != 0.0) {
        result.signed_time_offset_ms = delta / ADO_OJ_PI * crotchet / conductor_pitch * 1000.0;
    }
    return result;
}

AdoOfficialJudgementOnly ado_official_judge_event_tick_only(
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    const AdoOfficialPlayer *player,
    const AdoOfficialPlanet *planet,
    uint64_t event_tick,
    double margin_scale) {
    AdoOfficialJudgementOnly result;
    memset(&result, 0, sizeof(result));
    result.margin = ADO_HIT_PERFECT;
    if (conductor == NULL || player == NULL || planet == NULL) {
        return result;
    }
    double hit_angle = ado_official_get_angle(
        conductor,
        player,
        planet,
        planet->snapped_last_angle,
        event_tick);
    return ado_official_judge_angle_only(
        config,
        hit_angle,
        planet->target_exit_angle,
        planet->is_cw,
        conductor->bpm * planet->speed,
        conductor->pitch,
        margin_scale);
}

int ado_official_is_valid_hit(const AdoOfficialConfig *config, AdoOfficialHitMargin margin) {
    AdoOfficialHitMarginLimit limit = ADO_MARGIN_LIMIT_NONE;
    if (margin == ADO_HIT_AUTO) {
        return 1;
    }
    if (config != NULL) {
        limit = config->hit_margin_limit;
    }
    if (limit == ADO_MARGIN_LIMIT_PERFECTS_ONLY) {
        return margin >= ADO_HIT_EARLY_PERFECT && margin <= ADO_HIT_LATE_PERFECT;
    }
    if (limit == ADO_MARGIN_LIMIT_PURE_PERFECT_ONLY) {
        return margin == ADO_HIT_PERFECT;
    }
    return margin >= ADO_HIT_VERY_EARLY && margin <= ADO_HIT_VERY_LATE;
}

double ado_official_get_song_position(const AdoOfficialConductor *conductor, uint64_t now_tick) {
    if (conductor == NULL) {
        return 0.0;
    }
    if (conductor->use_song_time || conductor->song_time != 0.0) {
        return (conductor->song_time - conductor->calibration_i) - conductor->add_offset / conductor->pitch;
    }
    return (((double)now_tick / 10000000.0) - conductor->dsp_time_song - conductor->calibration_i) *
               conductor->pitch -
           conductor->add_offset;
}

double ado_official_get_angle(
    const AdoOfficialConductor *conductor,
    const AdoOfficialPlayer *player,
    const AdoOfficialPlanet *planet,
    double snapped_last_angle,
    uint64_t now_tick) {
    if (conductor == NULL || player == NULL || planet == NULL || conductor->crotchet_at_start == 0.0) {
        return snapped_last_angle;
    }
    return snapped_last_angle +
           (ado_official_get_song_position(conductor, now_tick) - player->last_hit) /
               conductor->crotchet_at_start *
               ADO_OJ_ASYNC_PI *
               planet->speed *
               (planet->is_cw ? 1.0 : -1.0);
}

static double ado_oj_min_angle_margin(
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    const AdoOfficialPlanet *planet) {
    if (conductor == NULL || planet == NULL) {
        return 0.0;
    }
    return ado_official_get_adjusted_angle_boundary_deg(
               config,
               ADO_MARGIN_COUNTED,
               conductor->bpm * planet->speed,
               conductor->pitch,
               1.0) /
           ADO_OJ_RAD2DEG;
}

static double ado_oj_hold_margin(
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    const AdoOfficialRuntime *runtime) {
    if (runtime == NULL || runtime->planet.currfloor == NULL) {
        return 1.0;
    }
    AdoOfficialFloor *floor = runtime->planet.currfloor;
    if (floor->hold_margin_override != 0.0) {
        return floor->hold_margin_override;
    }
    double min_angle_margin = ado_oj_min_angle_margin(config, conductor, &runtime->planet);
    double margin_scale = (floor->next == NULL) ? 1.0 : floor->next->margin_scale;
    if (floor->angle_length == 0.0) {
        return 1.0;
    }
    return 1.0 - min_angle_margin * margin_scale / floor->angle_length;
}

static AdoOfficialFloor *ado_oj_next_non_midspin_floor(const AdoOfficialFloor *floor) {
    AdoOfficialFloor *next = floor == NULL ? NULL : floor->next;
    while (next != NULL && next->mid_spin && next->next != NULL) {
        next = next->next;
    }
    return next;
}

static int ado_oj_next_tile_is_hold(const AdoOfficialRuntime *runtime) {
    if (runtime == NULL) {
        return 0;
    }
    if (runtime->next_tile_is_hold_cached) {
        return 1;
    }
    AdoOfficialFloor *floor = runtime->planet.currfloor;
    const AdoOfficialFloor *next = floor == NULL ? NULL : floor->next;
    return next != NULL && next->hold_length > -1;
}

static int ado_oj_next_tile_is_auto(const AdoOfficialRuntime *runtime) {
    const AdoOfficialFloor *floor = runtime == NULL ? NULL : runtime->planet.currfloor;
    const AdoOfficialFloor *next = floor == NULL ? NULL : floor->next;
    return next != NULL && next->auto_floor;
}

static int ado_oj_auto_path_active(
    const AdoOfficialRuntime *runtime,
    int next_auto,
    int is_auto_call) {
    if (runtime == NULL) {
        return 0;
    }
    return runtime->player.auto_player ||
           runtime->player.invincible_auto ||
           next_auto ||
           is_auto_call;
}

static AdoOfficialConditionalEffect ado_oj_conditional_effect_for_margin(AdoOfficialHitMargin margin) {
    switch (margin) {
        case ADO_HIT_PERFECT:
            return ADO_CONDITIONAL_PERFECT;
        case ADO_HIT_EARLY_PERFECT:
            return ADO_CONDITIONAL_EARLY_PERFECT;
        case ADO_HIT_LATE_PERFECT:
            return ADO_CONDITIONAL_LATE_PERFECT;
        case ADO_HIT_VERY_EARLY:
            return ADO_CONDITIONAL_VERY_EARLY;
        case ADO_HIT_VERY_LATE:
            return ADO_CONDITIONAL_VERY_LATE;
        case ADO_HIT_TOO_EARLY:
            return ADO_CONDITIONAL_TOO_EARLY;
        case ADO_HIT_TOO_LATE:
            return ADO_CONDITIONAL_TOO_LATE;
        default:
            return ADO_CONDITIONAL_NONE;
    }
}

static int ado_oj_conditional_effect_count(
    const AdoOfficialFloor *floor,
    AdoOfficialConditionalEffect effect) {
    if (floor == NULL) {
        return 0;
    }
    switch (effect) {
        case ADO_CONDITIONAL_PERFECT:
            return floor->conditional_perfect_count;
        case ADO_CONDITIONAL_EARLY_PERFECT:
            return floor->conditional_early_perfect_count;
        case ADO_CONDITIONAL_LATE_PERFECT:
            return floor->conditional_late_perfect_count;
        case ADO_CONDITIONAL_VERY_EARLY:
            return floor->conditional_very_early_count;
        case ADO_CONDITIONAL_VERY_LATE:
            return floor->conditional_very_late_count;
        case ADO_CONDITIONAL_TOO_EARLY:
            return floor->conditional_too_early_count;
        case ADO_CONDITIONAL_TOO_LATE:
            return floor->conditional_too_late_count;
        case ADO_CONDITIONAL_LOSS:
            return floor->conditional_loss_count;
        case ADO_CONDITIONAL_ON_CHECKPOINT:
            return floor->conditional_on_checkpoint_count;
        case ADO_CONDITIONAL_NONE:
        default:
            return 0;
    }
}

static void ado_oj_mark_conditional_effect(
    AdoOfficialRuntime *runtime,
    AdoOfficialStepResult *result,
    AdoOfficialConditionalEffect effect,
    AdoOfficialHitMargin margin) {
    if (runtime == NULL || result == NULL || effect == ADO_CONDITIONAL_NONE) {
        return;
    }
    if (ado_oj_conditional_effect_count(runtime->conditional_floor, effect) <= 0) {
        return;
    }
    result->conditional_effect_triggered = 1;
    result->conditional_effect = effect;
    result->conditional_effect_margin = margin;
}

static int ado_oj_floor_is_landable(const AdoOfficialFloor *floor) {
    if (floor == NULL) {
        return 0;
    }
    return !floor->is_landable_set || floor->is_landable;
}

static int ado_oj_freeroam_candidate_combo_is_valid(const AdoOfficialFloor *floor) {
    if (floor == NULL) {
        return 0;
    }
    return floor->freeroam == floor->freeroam_generated;
}

static AdoOfficialFloor *ado_oj_select_freeroam_target(
    AdoOfficialRuntime *runtime,
    double *exit_angle,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || exit_angle == NULL) {
        return NULL;
    }
    AdoOfficialFloor *candidate = runtime->freeroam_target_floor;
    if (candidate == NULL) {
        return NULL;
    }
    if (!runtime->freeroam_selection_ready ||
        (runtime->controller.independent_players && runtime->freeroam_candidate_occupied_by_player) ||
        !ado_oj_floor_is_landable(candidate) ||
        !ado_oj_freeroam_candidate_combo_is_valid(candidate)) {
        if (result != NULL) {
            result->freeroam_candidate_rejected = 1;
        }
        return NULL;
    }

    AdoOfficialFloor *curr = runtime->planet.currfloor;
    if (curr != NULL && curr->unstable && ado_oj_floor_is_landable(curr)) {
        curr->is_landable = 0;
        curr->is_landable_set = 1;
        curr->collider_enabled = 0;
        curr->hidden_by_unstable = 1;
        if (result != NULL) {
            result->freeroam_unstable_consumed = 1;
        }
    }

    *exit_angle = runtime->freeroam_exit_angle;
    if (result != NULL) {
        result->freeroam_target_selected = 1;
    }
    return candidate;
}

static int ado_oj_auto_should_hit_now_with_config(
    const AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config) {
    if (runtime == NULL || runtime->planet.currfloor == NULL) {
        return 0;
    }
    if (!runtime->controller.gameworld ||
        (runtime->controller.floor_count > 0 &&
         runtime->planet.currfloor->seq_id >= runtime->controller.floor_count - 1)) {
        return 0;
    }
    double margin_deg = (config != NULL && config->use_old_auto) ? 10.0 : 0.5;
    double margin_rad = margin_deg * (ADO_OJ_PI / 180.0);
    if (runtime->planet.is_cw) {
        return runtime->planet.angle > runtime->planet.target_exit_angle - margin_rad;
    }
    return runtime->planet.angle < runtime->planet.target_exit_angle + margin_rad;
}

static void ado_oj_lock_input(AdoOfficialRuntime *runtime, double seconds, AdoOfficialStepResult *result) {
    if (runtime == NULL || seconds <= 0.0) {
        return;
    }
    runtime->player.lock_input = seconds;
    runtime->player.responsive = 0;
    runtime->lock_time = seconds;
    if (result != NULL) {
        result->input_locked = 1;
    }
}

static void ado_oj_unlock_input(AdoOfficialRuntime *runtime, AdoOfficialStepResult *result) {
    if (runtime == NULL) {
        return;
    }
    runtime->player.lock_input = 0.0;
    runtime->player.responsive = 1;
    if (result != NULL) {
        result->input_unlocked = 1;
    }
}

static void ado_oj_update_lock_input(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConductor *conductor,
    double delta_song_pos,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || conductor == NULL) {
        return;
    }
    if (runtime->player.lock_input <= 0.0 || runtime->player.responsive) {
        return;
    }

    double delta = delta_song_pos;
    if (delta < 0.0) {
        delta = -delta;
    }
    if (delta > 0.0 && conductor->pitch != 0.0) {
        delta /= fabs(conductor->pitch);
    }
    if (delta <= 0.0) {
        return;
    }

    runtime->player.lock_input -= delta * conductor->pitch;
    if (runtime->player.lock_input <= 0.0) {
        ado_oj_unlock_input(runtime, result);
    }
}

static double ado_oj_unscaled_delta_from_input(
    const AdoOfficialConductor *conductor,
    AdoOfficialInputEvent input) {
    if (input.delta_unscaled_time > 0.0) {
        return input.delta_unscaled_time;
    }
    if (conductor != NULL && conductor->pitch != 0.0 && input.delta_song_pos != 0.0) {
        double pitch = fabs(conductor->pitch);
        if (pitch > 0.0) {
            return fabs(input.delta_song_pos) / pitch;
        }
    }
    return 0.0;
}

static void ado_oj_update_miss_cooldown(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConductor *conductor,
    AdoOfficialInputEvent input) {
    if (runtime == NULL || !runtime->player.miss_cooldown_active) {
        return;
    }
    double delta = ado_oj_unscaled_delta_from_input(conductor, input);
    if (delta <= 0.0) {
        return;
    }
    runtime->player.miss_cooldown_remaining -= delta;
    if (runtime->player.miss_cooldown_remaining <= 0.0) {
        runtime->player.miss_cooldown_remaining = 0.0;
        runtime->player.miss_cooldown_active = 0;
    }
}

static int ado_oj_hit_input_event(
    AdoOfficialRuntime *runtime,
    int is_auto,
    int is_release,
    AdoOfficialInputEvent input,
    AdoOfficialStepResult *result) {
    if (is_auto) {
        return 1;
    }
    if (runtime == NULL) {
        return 0;
    }
    int *last_frame = is_release ? &runtime->player.last_hit_frame_up : &runtime->player.last_hit_frame_down;
    int frame_count = input.frame_count != 0 ? input.frame_count : runtime->current_frame_count;
    int frame_gate_open = *last_frame != frame_count;
    *last_frame = frame_count;
    if (result != NULL) {
        if (is_release) {
            result->release_input_event_frame_gate_open |= frame_gate_open;
        } else {
            result->hit_input_event_frame_gate_open |= frame_gate_open;
        }
    }
    if (!is_release && frame_gate_open) {
        runtime->player.ignore_input_count = 0;
    }
    if (frame_gate_open) {
        int ignore_add_count = is_release ? input.release_input_ignore_add_count : input.hit_input_ignore_add_count;
        uint8_t active_slots = is_release ? input.release_input_slot_mask : input.hit_input_slot_mask;
        uint8_t ignore_slots = is_release ? input.release_input_ignore_slot_mask : input.hit_input_ignore_slot_mask;
        if (result != NULL) {
            if (is_release) {
                result->release_input_slots_handled |= active_slots;
                result->release_input_ignore_slots_handled |= active_slots & ignore_slots;
            } else {
                result->hit_input_slots_handled |= active_slots;
                result->hit_input_ignore_slots_handled |= active_slots & ignore_slots;
            }
        }
        if (active_slots != 0 && ignore_slots != 0) {
            ignore_add_count += ado_oj_popcount64((uint64_t)(active_slots & ignore_slots));
        }
        if (ignore_add_count > 0) {
            runtime->player.ignore_input_count += ignore_add_count;
        }
    }
    if (!is_release && runtime->player.ignore_input_count > 0) {
        runtime->player.ignore_input_count--;
        return 0;
    }
    return 1;
}

static int ado_oj_popcount64(uint64_t mask) {
    int count = 0;
    while (mask != 0) {
        count += (int)(mask & 1u);
        mask >>= 1;
    }
    return count;
}

static int ado_oj_mask_count_or_fallback(uint64_t mask, int fallback) {
    int count = ado_oj_popcount64(mask);
    int fallback_count = fallback > 0 ? fallback : 0;
    return count > fallback_count ? count : fallback_count;
}

static double ado_oj_tick_to_seconds(uint64_t tick) {
    return (double)tick / 10000000.0;
}

static void ado_oj_expire_down_keys(AdoOfficialRuntime *runtime, double now_seconds) {
    if (runtime == NULL || runtime->player.down_keys_duration_mask == 0) {
        return;
    }
    for (int i = 0; i < ADO_OJ_KEY_SLOTS; i++) {
        uint64_t bit = UINT64_C(1) << i;
        if ((runtime->player.down_keys_duration_mask & bit) == 0) {
            continue;
        }
        double down_time = runtime->player.down_key_duration_time[i];
        if (now_seconds - down_time >= 0.5) {
            runtime->player.down_keys_duration_mask &= ~bit;
            runtime->player.down_key_duration_time[i] = 0.0;
        }
    }
    runtime->player.down_key_duration_count = ado_oj_popcount64(runtime->player.down_keys_duration_mask);
}

static void ado_oj_set_down_key_time(AdoOfficialRuntime *runtime, uint64_t key, double now_seconds) {
    if (runtime == NULL || key == 0) {
        return;
    }
    for (int i = 0; i < ADO_OJ_KEY_SLOTS; i++) {
        if (key == (UINT64_C(1) << i)) {
            runtime->player.down_key_duration_time[i] = now_seconds;
            return;
        }
    }
}

static int ado_oj_count_valid_keys_pressed(
    AdoOfficialRuntime *runtime,
    int count,
    uint64_t pressed_keys_mask,
    uint64_t held_keys_mask,
    int key_limiter_over_count,
    int active_down_key_count,
    AdoOfficialStepResult *result) {
    if (runtime == NULL) {
        return count < 0 ? 0 : count;
    }
    int pressed_key_count = ado_oj_popcount64(pressed_keys_mask);
    int valid_count = ado_oj_mask_count_or_fallback(pressed_keys_mask, count);
    int key_total_delta = pressed_keys_mask != 0 ? pressed_key_count : valid_count;
    runtime->player.key_limiter_over_counter = 0;

    if (runtime->controller.is_player_control_state) {
        double now_seconds = ado_oj_tick_to_seconds(runtime->current_event_tick);
        ado_oj_expire_down_keys(runtime, now_seconds);
        if (pressed_keys_mask != 0) {
            int limit = runtime->controller.unlock_key_limiter ? 16 : 1000;
            uint64_t known = runtime->player.down_keys_duration_mask;
            uint64_t accepted = 0;
            uint64_t over = 0;
            uint64_t mask = pressed_keys_mask;
            while (mask != 0) {
                uint64_t key = mask & (~mask + 1u);
                if ((known & key) != 0 || ado_oj_popcount64(known | accepted) < limit) {
                    accepted |= key;
                    ado_oj_set_down_key_time(runtime, key, now_seconds);
                } else {
                    over |= key;
                }
                mask &= ~key;
            }
            runtime->player.down_keys_duration_mask = (known | accepted | (held_keys_mask & known));
            runtime->player.down_key_duration_count = ado_oj_popcount64(runtime->player.down_keys_duration_mask);
            runtime->player.key_limiter_over_counter = ado_oj_popcount64(over);
        } else if (key_limiter_over_count > 0) {
            runtime->player.key_limiter_over_counter = key_limiter_over_count;
        } else if (active_down_key_count > 0) {
            int limit = runtime->controller.unlock_key_limiter ? 16 : 1000;
            int over = active_down_key_count + valid_count - limit;
            runtime->player.key_limiter_over_counter = over > 0 ? over : 0;
            runtime->player.down_key_duration_count = active_down_key_count + valid_count;
            if (runtime->player.down_key_duration_count > limit) {
                runtime->player.down_key_duration_count = limit;
            }
        }
        runtime->player.key_total += key_total_delta;
    } else {
        runtime->player.down_key_duration_count = 0;
        runtime->player.down_keys_duration_mask = 0;
    }

    if (result != NULL) {
        if (runtime->controller.is_player_control_state) {
            result->key_total_added += key_total_delta;
        }
        result->key_limiter_over_count += runtime->player.key_limiter_over_counter;
        result->active_down_key_count = runtime->player.down_key_duration_count;
        if (runtime->controller.is_player_control_state) {
            if (runtime->player.down_key_duration_count > runtime->controller.maximum_used_keys) {
                runtime->controller.maximum_used_keys = runtime->player.down_key_duration_count;
            }
        } else {
            runtime->controller.maximum_used_keys = 0;
        }
        result->maximum_used_keys_after = runtime->controller.maximum_used_keys;
    }
    return valid_count;
}

static int ado_oj_input_count_or_fallback(int primary, int secondary, int fallback) {
    if (primary > 0) {
        return primary;
    }
    if (secondary > 0) {
        return secondary;
    }
    return fallback;
}

static void ado_oj_merge_input_observation(AdoOfficialStepResult *dst, const AdoOfficialStepResult *src) {
    if (dst == NULL || src == NULL) {
        return;
    }
    dst->processed |= src->processed;
    dst->moved_to_next_floor |= src->moved_to_next_floor;
    dst->hit_returned_true |= src->hit_returned_true;
    dst->damaged |= src->damaged;
    dst->died |= src->died;
    dst->overpress |= src->overpress;
    dst->multipress |= src->multipress;
    dst->auto_hit |= src->auto_hit;
    dst->hold_fail |= src->hold_fail;
    dst->hold_cancelled |= src->hold_cancelled;
    dst->post_hold_fail |= src->post_hold_fail;
    dst->skipped_by_hold_guard |= src->skipped_by_hold_guard;
    dst->partial_multitap |= src->partial_multitap;
    dst->input_ignored |= src->input_ignored;
    dst->no_fail_recovered |= src->no_fail_recovered;
    dst->hit_input_event_only |= src->hit_input_event_only;
    dst->release_input_event_only |= src->release_input_event_only;
    dst->damage_failed |= src->damage_failed;
    dst->death_suppressed |= src->death_suppressed;
    dst->death_suppressed_by_auto_or_debug |= src->death_suppressed_by_auto_or_debug;
    dst->death_suppressed_by_no_fail |= src->death_suppressed_by_no_fail;
    dst->death_suppressed_by_safe_floor |= src->death_suppressed_by_safe_floor;
    dst->death_suppressed_by_miss_cooldown |= src->death_suppressed_by_miss_cooldown;
    dst->failbar_damaged |= src->failbar_damaged;
    dst->failbar_failed |= src->failbar_failed;
    dst->miss_marked |= src->miss_marked;
    dst->fail_marked |= src->fail_marked;
    dst->miss_cooldown_started |= src->miss_cooldown_started;
    dst->midspin_key_appended |= src->midspin_key_appended;
    dst->midspin_second_hit |= src->midspin_second_hit;
    dst->life_lost |= src->life_lost;
    if (src->lives_remaining_after != 0 || dst->lives_remaining_after == 0) {
        dst->lives_remaining_after = src->lives_remaining_after;
    }
    dst->revive_bubble_intercepted |= src->revive_bubble_intercepted;
    dst->conditional_floor_set |= src->conditional_floor_set;
    dst->conditional_effect_triggered |= src->conditional_effect_triggered;
    if (src->conditional_effect != ADO_CONDITIONAL_NONE) {
        dst->conditional_effect = src->conditional_effect;
    }
    dst->puzzle_iframes_applied |= src->puzzle_iframes_applied;
    dst->portal_landed |= src->portal_landed;
    dst->player_made_unresponsive |= src->player_made_unresponsive;
    dst->freeroam_target_selected |= src->freeroam_target_selected;
    dst->freeroam_candidate_rejected |= src->freeroam_candidate_rejected;
    dst->freeroam_entered |= src->freeroam_entered;
    dst->freeroam_colliders_disabled |= src->freeroam_colliders_disabled;
    dst->error_meter_hidden |= src->error_meter_hidden;
    dst->error_meter_shown |= src->error_meter_shown;
    dst->freeroam_unstable_consumed |= src->freeroam_unstable_consumed;
    dst->freeroam_swirl_flipped |= src->freeroam_swirl_flipped;
    dst->freeroam_landed_on_floor |= src->freeroam_landed_on_floor;
    dst->planet_count_changed |= src->planet_count_changed;
    dst->floor_lit |= src->floor_lit;
    dst->ffx_triggered |= src->ffx_triggered;
    dst->invalid_freeroam |= src->invalid_freeroam;
    dst->drum_debounce_returned |= src->drum_debounce_returned;
    dst->drum_debounce_set |= src->drum_debounce_set;
    dst->pause_handled |= src->pause_handled;
    dst->input_locked |= src->input_locked;
    dst->input_unlocked |= src->input_unlocked;
    dst->hit_input_event_frame_gate_open |= src->hit_input_event_frame_gate_open;
    dst->release_input_event_frame_gate_open |= src->release_input_event_frame_gate_open;
    dst->hit_input_slots_handled |= src->hit_input_slots_handled;
    dst->release_input_slots_handled |= src->release_input_slots_handled;
    dst->hit_input_ignore_slots_handled |= src->hit_input_ignore_slots_handled;
    dst->release_input_ignore_slots_handled |= src->release_input_ignore_slots_handled;
    dst->ending_started |= src->ending_started;
    dst->percent_updated |= src->percent_updated;
    dst->level_hit_emitted |= src->level_hit_emitted;
    dst->tracker_hit_added |= src->tracker_hit_added;
    dst->hit_text_shown |= src->hit_text_shown;
    dst->floor_grade_set |= src->floor_grade_set;
    dst->error_meter_hit_added |= src->error_meter_hit_added;
    dst->camera_pulse_allowed |= src->camera_pulse_allowed;
    dst->avatar_hit_emitted |= src->avatar_hit_emitted;
    dst->key_total_added += src->key_total_added;
    dst->key_limiter_over_count += src->key_limiter_over_count;
    if (src->active_down_key_count > dst->active_down_key_count) {
        dst->active_down_key_count = src->active_down_key_count;
    }
    if (dst->death_registered_floor_id < 0 && src->death_registered_floor_id >= 0) {
        dst->death_registered_floor_id = src->death_registered_floor_id;
    }
    if (dst->freeroam_landing_seq_id < 0 && src->freeroam_landing_seq_id >= 0) {
        dst->freeroam_landing_seq_id = src->freeroam_landing_seq_id;
    }
}

static void ado_oj_capture_step_exit_state(
    const AdoOfficialRuntime *runtime,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || result == NULL) {
        return;
    }
    result->pending_key_times_after = runtime->player.pending_key_times;
    result->pending_midspin_key_times_after = runtime->player.pending_midspin_key_times;
    result->hold_key_count_after = runtime->player.hold_key_count;
    result->holding_after = runtime->player.holding;
    result->maximum_used_keys_after = runtime->controller.maximum_used_keys;
    result->curr_seq_id_after =
        runtime->planet.currfloor != NULL ? runtime->planet.currfloor->seq_id : -1;
    result->controller_seq_id_after = runtime->controller.current_seq_id;
}

static void ado_oj_set_hold_keys(AdoOfficialRuntime *runtime, uint64_t mask, int fallback_count) {
    if (runtime == NULL) {
        return;
    }
    runtime->player.hold_keys_mask = mask;
    runtime->player.hold_key_count = ado_oj_mask_count_or_fallback(mask, fallback_count);
    runtime->player.holding = runtime->player.hold_key_count > 0;
}

static void ado_oj_set_hold_key_count(AdoOfficialRuntime *runtime, int count) {
    ado_oj_set_hold_keys(runtime, 0, count);
}

static int ado_oj_get_current_held_count(AdoOfficialInputEvent input) {
    return ado_oj_mask_count_or_fallback(
        input.held_keys_mask,
        ado_oj_input_count_or_fallback(input.held_key_count, input.held_count, 0));
}

static int ado_oj_valid_input_was_released(AdoOfficialRuntime *runtime, AdoOfficialInputEvent input) {
    if (runtime == NULL) {
        return 0;
    }
    if (input.touch_hold_clear_release && runtime->player.hold_key_count > 0) {
        ado_oj_set_hold_keys(runtime, 0, 0);
        return 1;
    }
    if (runtime->player.hold_keys_mask != 0 || input.held_keys_mask != 0 || input.released_keys_mask != 0) {
        uint64_t previous = runtime->player.hold_keys_mask;
        uint64_t current = input.held_keys_mask;
        uint64_t still_valid = previous & current;
        uint64_t removed = previous & ~current;
        if (input.released_keys_mask != 0) {
            removed |= previous & input.released_keys_mask;
            still_valid &= ~input.released_keys_mask;
        }
        runtime->player.hold_keys_mask = still_valid;
        runtime->player.hold_key_count = ado_oj_popcount64(still_valid);
        runtime->player.holding = runtime->player.hold_key_count > 0;
        if (input.force_valid_input_released) {
            ado_oj_set_hold_keys(runtime, current, input.held_key_count);
            return 1;
        }
        return removed != 0;
    }
    int previous = runtime->player.hold_key_count;
    int current = ado_oj_get_current_held_count(input);
    if (input.force_valid_input_released) {
        ado_oj_set_hold_keys(runtime, input.held_keys_mask, current);
        return 1;
    }
    if (input.release_valid_key_count > 0) {
        int remaining = previous - input.release_valid_key_count;
        if (remaining < 0 || remaining > current) {
            remaining = current;
        }
        ado_oj_set_hold_key_count(runtime, remaining);
        return 1;
    }
    if (previous > 0 && current < previous) {
        ado_oj_set_hold_key_count(runtime, current);
        return 1;
    }
    return 0;
}

static void ado_oj_refresh_hold_keys_after_hit(
    AdoOfficialRuntime *runtime,
    AdoOfficialInputEvent input,
    const AdoOfficialStepResult *hit_result) {
    if (runtime == NULL || hit_result == NULL || runtime->planet.currfloor == NULL) {
        return;
    }
    if (!hit_result->hit_returned_true || runtime->planet.currfloor->hold_length <= -1) {
        return;
    }
    if (input.pressed_keys_mask != 0) {
        ado_oj_set_hold_keys(runtime, input.pressed_keys_mask & (input.held_keys_mask | input.pressed_keys_mask), 0);
        return;
    }
    int count = ado_oj_input_count_or_fallback(
        input.pressed_held_key_count,
        input.held_key_count,
        input.held_count);
    ado_oj_set_hold_keys(runtime, 0, count);
}

static void ado_oj_cancel_hold(AdoOfficialRuntime *runtime, AdoOfficialStepResult *result) {
    if (runtime == NULL || result == NULL || runtime->planet.currfloor == NULL) {
        return;
    }
    AdoOfficialFloor *floor = runtime->planet.currfloor;
    floor->hold_completion = 0.0;
    if (floor->prev != NULL) {
        runtime->planet.currfloor = floor->prev;
        ado_oj_sync_controller_floor(runtime, floor->prev, result);
        result->target_seq_id = floor->prev->seq_id;
    }
    ado_oj_lock_input(runtime, 0.4, result);
    result->hold_cancelled = 1;
}

static int ado_oj_get_multipress_penalty(const AdoOfficialController *controller, const AdoOfficialFloor *floor) {
    if (controller == NULL || !controller->gameworld || floor == NULL) {
        return 0;
    }
    if (floor->mid_spin) {
        floor = floor->next;
    }
    if (floor == NULL) {
        return 0;
    }
    const AdoOfficialFloor *next1 = floor->next;
    if (next1 != NULL && next1->mid_spin) {
        next1 = next1->next;
    }
    if (next1 == NULL) {
        return 0;
    }
    const AdoOfficialFloor *next2 = next1->next;
    if (next2 != NULL && next2->mid_spin) {
        next2 = next2->next;
    }
    if (next2 == NULL) {
        return 0;
    }
    const AdoOfficialFloor *next3 = next2->next;
    if (next3 != NULL && next3->mid_spin) {
        next3 = next3->next;
    }
    if (next3 == NULL) {
        return 0;
    }

    double wide = 0.5253441007807851;
    double tight = 0.2635447150096297;
    double a0 = ado_oj_get_angle_moved(floor->entryangle, floor->exitangle, !floor->is_ccw);
    double a1 = ado_oj_get_angle_moved(next1->entryangle, next1->exitangle, !next1->is_ccw);
    double a2 = ado_oj_get_angle_moved(next2->entryangle, next2->exitangle, !next2->is_ccw);
    double a3 = ado_oj_get_angle_moved(next3->entryangle, next3->exitangle, !next3->is_ccw);

    if (a0 < wide && a1 > wide) {
        return 0;
    }
    if (a0 < wide && a1 < wide && a2 > wide) {
        return 0;
    }
    if (a0 < tight && a1 < tight && a2 < tight && a3 > tight) {
        return 0;
    }
    return 1;
}

static double ado_oj_abs_double(double x) {
    return x < 0.0 ? -x : x;
}

static void ado_oj_handle_pause(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConductor *conductor,
    AdoOfficialFloor *floor,
    double angle_diff,
    double time_diff,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || conductor == NULL || floor == NULL || result == NULL ||
        floor->next == NULL || conductor->crotchet_at_start == 0.0 || floor->speed == 0.0) {
        return;
    }
    if (floor->freeroam || floor->extra_beats <= 0.0) {
        return;
    }

    double lock_time = (conductor->crotchet_at_start / floor->speed) * (floor->extra_beats + 0.2) + time_diff;
    if (floor->next->entry_time - floor->entry_time - lock_time < 0.05000000074505806) {
        lock_time = 0.0;
    }
    if (lock_time > 0.0 && runtime->controller.is_player_control_state) {
        ado_oj_lock_input(runtime, lock_time, result);
    } else {
        runtime->lock_time = lock_time;
    }

    runtime->planet.angle =
        runtime->planet.snapped_last_angle - angle_diff +
        (runtime->current_song_position - runtime->player.last_hit) /
            conductor->crotchet_at_start *
            ADO_OJ_PI *
            runtime->planet.speed *
            (runtime->planet.is_cw ? 1.0 : -1.0);
    double before_angle = ado_oj_mod(runtime->planet.angle, ADO_OJ_TWO_PI);

    int dir = !floor->is_ccw ? 1 : -1;
    runtime->player.last_hit = floor->entry_time;
    double correction = (ADO_OJ_PI - floor->angle_length) * (double)dir;
    if (ado_oj_abs_double(floor->angle_length) < 0.002 ||
        ado_oj_abs_double(floor->angle_length - ADO_OJ_TWO_PI) < 0.002) {
        correction = 0.0;
    }

    double target_exit = floor->next->entryangle - ADO_OJ_PI * (double)dir;
    double snapped =
        target_exit -
        ADO_OJ_PI * (floor->extra_beats + 1.0) * (double)dir -
        angle_diff +
        correction;
    runtime->planet.target_exit_angle = target_exit;
    runtime->planet.snapped_last_angle = snapped;
    runtime->planet.angle =
        runtime->planet.snapped_last_angle +
        (runtime->current_song_position - runtime->player.last_hit) /
            conductor->crotchet_at_start *
            ADO_OJ_PI *
            0.5 *
            runtime->planet.speed *
            (runtime->planet.is_cw ? 1.0 : -1.0);
    double after_angle = ado_oj_mod(runtime->planet.angle, ADO_OJ_TWO_PI);
    double tween_correction =
        (double)floor->angle_correction_type *
        (before_angle - after_angle) *
        (!floor->is_ccw ? 1.0 : -1.0) *
        (after_angle < before_angle ? 1.0 : -1.0);
    runtime->planet.snapped_last_angle = snapped - tween_correction;
    result->pause_handled = 1;
}

static void ado_oj_reset_player_runtime(AdoOfficialRuntime *runtime) {
    if (runtime == NULL) {
        return;
    }
    runtime->player.alive = 1;
    runtime->player.responsive = 1;
    runtime->player.is_reunifying = 0;
    runtime->player.holding = 0;
    runtime->player.hold_key_count = 0;
    runtime->player.midspin_infinite_margin = 0;
    runtime->player.taps_on_this_floor = 0;
    runtime->player.consec_multipress_counter = 0;
    runtime->player.key_limiter_over_counter = 0;
    runtime->player.ignore_input_count = 0;
    runtime->player.miss_cooldown_active = 0;
    runtime->player.pending_key_times = 0;
    runtime->player.pending_midspin_key_times = 0;
    runtime->player.down_key_duration_count = 0;
    runtime->player.key_total = 0;
    runtime->player.last_hit_frame_down = 0;
    runtime->player.last_hit_frame_up = 0;
    runtime->player.hold_keys_mask = 0;
    runtime->player.down_keys_duration_mask = 0;
    memset(runtime->player.down_key_duration_time, 0, sizeof(runtime->player.down_key_duration_time));
    runtime->player.failbar_overload_counter = 0.0;
    runtime->player.failbar_multipress_counter = 0.0;
    runtime->player.failbar_multipress_reset_counter = 0.0;
    runtime->player.miss_cooldown_remaining = 0.0;
    runtime->player.lock_input = 0.0;
}

void ado_official_rewind(AdoOfficialRuntime *runtime) {
    if (runtime == NULL) {
        return;
    }
    AdoOfficialFloor *first = runtime->planet.currfloor;
    AdoOfficialFloor *conditional = runtime->conditional_floor;
    int floor_count = runtime->controller.floor_count;
    int is_player_control = runtime->controller.is_player_control_state;
    int no_fail = runtime->controller.no_fail;
    int strict_holds = runtime->controller.strict_holds;
    int require_holding = runtime->controller.require_holding;
    int benchmark = runtime->controller.benchmark_mode;
    int official = runtime->controller.is_official_level;
    int expo = runtime->controller.is_expo;
    int puzzle = runtime->controller.is_puzzle_room;
    int independent = runtime->controller.independent_players;

    ado_official_runtime_defaults(runtime);
    runtime->planet.currfloor = first;
    runtime->controller.curr_floor = first;
    runtime->conditional_floor = conditional;
    runtime->controller.floor_count = floor_count;
    runtime->controller.is_player_control_state = is_player_control;
    runtime->controller.no_fail = no_fail;
    runtime->controller.strict_holds = strict_holds;
    runtime->controller.require_holding = require_holding;
    runtime->controller.benchmark_mode = benchmark;
    runtime->controller.is_official_level = official;
    runtime->controller.is_expo = expo;
    runtime->controller.is_puzzle_room = puzzle;
    runtime->controller.independent_players = independent;
    ado_oj_reset_player_runtime(runtime);
    runtime->planet.snapped_last_angle = 0.0;
    runtime->planet.target_exit_angle = 0.0;
    runtime->planet.angle = 0.0;
    runtime->planet.cached_angle = 0.0;
    runtime->planet.speed = 1.0;
    runtime->planet.is_cw = 1;
}

void ado_official_scrub_to_floor(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConductor *conductor,
    AdoOfficialFloor *floor,
    double windback_time,
    int has_windback_time,
    int doing_revive) {
    if (runtime == NULL || conductor == NULL || floor == NULL) {
        return;
    }

    runtime->planet.currfloor = floor;
    runtime->controller.curr_floor = floor;
    if (!doing_revive) {
        runtime->controller.current_seq_id = floor->seq_id;
        runtime->controller.current_floor_id = floor->seq_id;
    }

    double inv = ado_oj_inverse_angle_per_beat_multiplanet((double)floor->num_planets) *
                 (!floor->is_ccw ? 1.0 : -1.0);
    if (floor->prev != NULL && floor->prev->mid_spin && floor->num_planets > 2) {
        inv -= ado_oj_inverse_angle_per_beat_multiplanet((double)floor->prev->num_planets) *
               (!floor->prev->is_ccw ? 1.0 : -1.0);
    }
    runtime->planet.snapped_last_angle = floor->entryangle + inv;
    runtime->planet.is_cw = !floor->is_ccw;
    runtime->planet.speed = floor->speed;
    runtime->planet.target_exit_angle =
        runtime->planet.snapped_last_angle + floor->angle_length * (!floor->is_ccw ? 1.0 : -1.0);
    runtime->player.last_hit = floor->entry_time;

    double angle_diff = 0.0;
    double time_diff = 0.0;
    if (has_windback_time) {
        double start = floor->entry_time - windback_time;
        time_diff = 0.0;
        angle_diff = ado_oj_time_to_angle_rad(
            floor->entry_time - start,
            conductor->bpm * floor->speed,
            conductor->pitch);
        runtime->planet.snapped_last_angle -= angle_diff * (runtime->planet.is_cw ? 1.0 : -1.0);
    }

    runtime->planet.angle = ado_official_get_angle(
        conductor,
        &runtime->player,
        &runtime->planet,
        runtime->planet.snapped_last_angle,
        runtime->current_event_tick);

    AdoOfficialStepResult ignored;
    memset(&ignored, 0, sizeof(ignored));
    ignored.death_registered_floor_id = -1;
    ignored.target_seq_id = -1;
    ignored.moving_to_seq_id = -1;
    ignored.freeroam_landing_seq_id = -1;
    ado_oj_handle_pause(
        runtime,
        conductor,
        floor,
        angle_diff * (runtime->planet.is_cw ? 1.0 : -1.0),
        time_diff,
        &ignored);
}

void ado_official_scrub_to_floor_with_history(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConductor *conductor,
    AdoOfficialFloor **floors,
    int floor_count,
    int floor_index,
    double windback_time,
    int has_windback_time,
    int doing_revive) {
    if (runtime == NULL || conductor == NULL || floors == NULL ||
        floor_index < 0 || floor_index >= floor_count || floors[floor_index] == NULL) {
        return;
    }
    ado_official_scrub_to_floor(
        runtime,
        conductor,
        floors[floor_index],
        windback_time,
        has_windback_time,
        doing_revive);
    if (!doing_revive) {
        runtime->conditional_floor = NULL;
        for (int i = floor_index; i > 0; i--) {
            if (floors[i] != NULL && floors[i]->has_conditional_change) {
                runtime->conditional_floor = floors[i];
                break;
            }
        }
    }
}

void ado_official_revive_at_floor(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConductor *conductor,
    AdoOfficialFloor *helper_floor,
    AdoOfficialFloor *revive_floor) {
    if (runtime == NULL || conductor == NULL || revive_floor == NULL) {
        return;
    }
    AdoOfficialFloor *last_floor = helper_floor != NULL ? helper_floor : runtime->planet.currfloor;
    ado_official_rewind(runtime);
    double windback = 0.0;
    if (last_floor != NULL && conductor->pitch != 0.0) {
        windback = (revive_floor->entry_time - (last_floor->entry_time + last_floor->extra_beats)) /
                   conductor->pitch;
    }
    ado_official_scrub_to_floor(runtime, conductor, revive_floor, windback, 1, 1);
    if (last_floor != NULL) {
        runtime->player.last_hit = last_floor->entry_time;
    }
    runtime->player.alive = 1;
    runtime->player.responsive = 1;
}

static int ado_oj_failbar_did_fail(const AdoOfficialRuntime *runtime) {
    if (runtime == NULL) {
        return 0;
    }
    int overloaded = runtime->player.failbar_overload_counter > 1.0 ||
                     runtime->player.failbar_multipress_counter > 1.0;
    int official_end_guard =
        runtime->controller.is_official_level &&
        (!runtime->controller.gameworld || runtime->controller.percent_complete >= 0.96);
    return overloaded && !official_end_guard;
}

static int ado_oj_failbar_damage(AdoOfficialRuntime *runtime, const AdoOfficialConfig *config, int multipress) {
    if (runtime == NULL) {
        return 0;
    }
    if (multipress) {
        runtime->player.failbar_multipress_counter += 0.35;
        runtime->player.failbar_multipress_reset_counter = 0.0;
    } else {
        double amount = 0.5;
        if (config != NULL && config->drum_controller) {
            amount /= 1.7;
        }
        runtime->player.failbar_overload_counter += amount;
    }
    return ado_oj_failbar_did_fail(runtime);
}

static void ado_oj_failbar_update(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    double delta_song_pos,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || config == NULL || conductor == NULL || result == NULL ||
        conductor->crotchet_at_start == 0.0) {
        return;
    }
    if (ado_oj_failbar_did_fail(runtime)) {
        ado_oj_die_or_recover(runtime, config, conductor, 1, 0, result);
        if (runtime->player.failbar_overload_counter > 0.5) {
            runtime->player.failbar_overload_counter = 0.5;
        }
        if (runtime->player.failbar_multipress_counter > 0.5) {
            runtime->player.failbar_multipress_counter = 0.5;
        }
    }
    double beat_delta = delta_song_pos / conductor->crotchet_at_start;
    runtime->player.failbar_overload_counter -= 0.4000000059604645 * beat_delta;
    if (runtime->player.failbar_overload_counter < 0.0) {
        runtime->player.failbar_overload_counter = 0.0;
    }
    runtime->player.failbar_multipress_counter -= 0.20000000298023224 * beat_delta;
    if (runtime->player.failbar_multipress_counter < 0.0) {
        runtime->player.failbar_multipress_counter = 0.0;
    }
    runtime->player.failbar_multipress_reset_counter += beat_delta;
    if (runtime->player.failbar_multipress_reset_counter > 6.0) {
        runtime->player.failbar_multipress_counter = 0.0;
        runtime->player.failbar_multipress_reset_counter = 0.0;
    }
}

static int ado_oj_on_damage(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    int multipress,
    int apply_multipress_damage,
    int skip_damage,
    AdoOfficialHitMargin hit_margin,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || conductor == NULL || result == NULL || runtime->planet.currfloor == NULL) {
        return 0;
    }

    const AdoOfficialFloor *floor = runtime->planet.currfloor;
    double bpm = conductor->bpm * runtime->planet.speed * conductor->pitch;
    double floor_time = ado_oj_angle_to_time(
        ado_oj_get_angle_moved(floor->entryangle, floor->exitangle, !floor->is_ccw), bpm);

    int failed = 0;
    if (multipress && !apply_multipress_damage && floor_time > 0.019999999552965164) {
        runtime->player.consec_multipress_counter++;
    }
    if (runtime->player.consec_multipress_counter > 8) {
        runtime->player.consec_multipress_counter = 4;
        ado_oj_die_or_recover(runtime, config, conductor, 1, 0, result);
        failed = 1;
    }
    if (multipress) {
        result->multipress = 1;
    }
    if ((!multipress || apply_multipress_damage) && !skip_damage) {
        result->damaged = 1;
        result->failbar_damaged = 1;
        int failbar_failed = ado_oj_failbar_damage(runtime, config, multipress);
        result->failbar_failed |= failbar_failed;
        if (failbar_failed) {
            ado_oj_die_or_recover(runtime, config, conductor, 1, 0, result);
            failed = 1;
        }
        if (runtime->controller.gameworld &&
            (!runtime->controller.no_fail || !failbar_failed) &&
            !floor->hide_judgment) {
            result->miss_marked = 1;
        }
    }
    if (config != NULL &&
        config->hit_margin_limit == ADO_MARGIN_LIMIT_PURE_PERFECT_ONLY &&
        hit_margin != ADO_HIT_PERFECT &&
        !runtime->controller.no_fail &&
        !floor->freeroam) {
        ado_oj_die_or_recover(runtime, config, conductor, 0, 0, result);
        failed = 1;
    }
    result->damage_failed |= failed;
    return failed;
}

static void ado_oj_move_to_next_floor(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialFloor *floor,
    double exit_angle,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || conductor == NULL || floor == NULL || result == NULL) {
        return;
    }

    AdoOfficialFloor *old_floor = runtime->planet.currfloor;
    double old_snapped = runtime->planet.snapped_last_angle;
    double old_speed = runtime->planet.speed;
    int old_is_cw = runtime->planet.is_cw;

    if (old_floor != NULL && old_floor->hold_length > -1) {
        old_floor->hold_completion = 1.0;
    }

    int entering_freeroam = old_floor != NULL && !old_floor->freeroam && floor->freeroam;
    int was_in_freeroam = old_floor != NULL && old_floor->freeroam;
    runtime->planet.currfloor = floor;
    ado_oj_sync_controller_floor(runtime, floor, result);
    if (entering_freeroam && runtime->player.lock_input < 0.05) {
        ado_oj_lock_input(runtime, 0.05, result);
    }
    if (floor->freeroam) {
        runtime->controller.gameworld = 0;
        result->freeroam_entered = 1;
        if (entering_freeroam) {
            result->freeroam_colliders_disabled = 1;
            result->error_meter_hidden = 1;
            floor->collider_enabled = 0;
        }
        if (was_in_freeroam && floor->is_swirl) {
            result->freeroam_swirl_flipped = 1;
        }
    }
    if (old_floor != NULL &&
        runtime->controller.gameworld &&
        old_floor->freeroam_generated &&
        !floor->freeroam_generated) {
        result->error_meter_shown = 1;
    }

    double inv = ado_oj_inverse_angle_per_beat_multiplanet((double)floor->num_planets) *
                 (!floor->is_ccw ? 1.0 : -1.0);
    if (floor->prev != NULL && floor->prev->mid_spin && floor->num_planets > 2) {
        inv -= 2.0 *
               ado_oj_inverse_angle_per_beat_multiplanet((double)floor->prev->num_planets) *
               (!floor->prev->is_ccw ? 1.0 : -1.0);
    }
    runtime->planet.snapped_last_angle = ado_oj_mod(exit_angle + ADO_OJ_PI, ADO_OJ_TWO_PI) + inv;
    runtime->planet.target_exit_angle =
        runtime->planet.snapped_last_angle + floor->angle_length * (!floor->is_ccw ? 1.0 : -1.0);
    if (config != NULL && config->stationary) {
        runtime->planet.angle = runtime->planet.snapped_last_angle;
    }
    if (old_floor != NULL && floor->num_planets != old_floor->num_planets) {
        result->planet_count_changed = 1;
    }

    runtime->player.actual_last_hit = runtime->current_song_position;
    if (old_speed != 0.0) {
        runtime->player.last_hit +=
            (exit_angle - old_snapped) * (old_is_cw ? 1.0 : -1.0) /
            ADO_OJ_PI * conductor->crotchet_at_start / old_speed;
    }

    if (runtime->controller.gameworld || floor->freeroam || floor->freeroam_generated) {
        runtime->planet.speed = floor->speed;
    }
    runtime->planet.is_cw = !floor->is_ccw;
    if (!floor->freeroam || (floor->freeroam && floor->freeroam_generated && !floor->unstable)) {
        result->floor_lit = 1;
    }
    result->ffx_triggered = 1;

    result->moved_to_next_floor = old_floor != floor;
    result->target_seq_id = floor->seq_id;
    if (floor->is_portal) {
        result->portal_landed = 1;
        if (!runtime->controller.win_time_set) {
            runtime->controller.win_time_set = 1;
            if (runtime->controller.gameworld ||
                (!runtime->controller.gameworld &&
                 !runtime->controller.is_puzzle_room &&
                 floor->freeroam_generated)) {
                runtime->player.responsive = 0;
                result->player_made_unresponsive = 1;
            }
        }
    }
    if (floor->freeroam &&
        !floor->freeroam_generated &&
        runtime->freeroam_landing_checked &&
        runtime->freeroam_landing_floor != NULL) {
        AdoOfficialFloor *landing = runtime->freeroam_landing_floor;
        result->freeroam_landed_on_floor = 1;
        result->freeroam_landing_seq_id = landing->seq_id;
        if (landing->is_swirl) {
            result->freeroam_swirl_flipped = 1;
            runtime->planet.is_cw = !landing->is_ccw;
        }
        if (ado_oj_floor_is_landable(landing)) {
            runtime->planet.currfloor = landing;
            runtime->controller.curr_floor = landing;
        }
    }
    if (floor->freeroam &&
        !floor->freeroam_generated &&
        (runtime->freeroam_invalid_after_move ||
         (runtime->freeroam_landing_checked && runtime->freeroam_landing_floor == NULL))) {
        runtime->controller.gameworld = 1;
        result->invalid_freeroam = 1;
        ado_oj_die_or_recover(runtime, config, conductor, 0, 1, result);
        runtime->freeroam_invalid_after_move = 0;
    }
    if (floor->mid_spin && floor->num_planets > 2 && floor->prev != NULL) {
        result->moving_to_seq_id = floor->prev->seq_id;
    } else {
        result->moving_to_seq_id = floor->seq_id;
    }
    ado_oj_handle_pause(runtime, conductor, floor, 0.0, 0.0, result);
    if (floor->next == NULL) {
        result->ending_started = 1;
    }
    runtime->freeroam_target_floor = NULL;
    runtime->freeroam_exit_angle = 0.0;
    runtime->freeroam_candidate_occupied_by_player = 0;
    runtime->freeroam_landing_checked = 0;
    runtime->freeroam_landing_floor = NULL;
    runtime->controller.multipress_penalty = ado_oj_get_multipress_penalty(&runtime->controller, floor);
    runtime->controller.multipress_and_has_pressed_first_press = runtime->player.pending_key_times != 0;
}

static AdoOfficialStepResult ado_oj_hit(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialInputEvent input,
    int is_auto) {
    AdoOfficialStepResult result;
    memset(&result, 0, sizeof(result));
    result.raw_margin = ADO_HIT_PERFECT;
    result.effective_margin = ADO_HIT_PERFECT;
    result.display_margin = ADO_HIT_PERFECT;
    result.floor_grade_margin = ADO_HIT_PERFECT;
    result.tracker_margin = ADO_HIT_PERFECT;
    result.hit_text_margin = ADO_HIT_PERFECT;
    result.conditional_effect_margin = ADO_HIT_PERFECT;
    result.death_registered_floor_id = -1;
    result.target_seq_id = -1;
    result.moving_to_seq_id = -1;
    result.freeroam_landing_seq_id = -1;

    if (runtime == NULL || config == NULL || conductor == NULL) {
        return result;
    }
    if (!runtime->player.responsive ||
        runtime->player.is_reunifying ||
        (runtime->controller.is_level_editor && runtime->controller.paused) ||
        runtime->planet.currfloor == NULL) {
        return result;
    }

    AdoOfficialFloor *curr = runtime->planet.currfloor;
    AdoOfficialFloor *next = curr->next;
    int next_auto = next != NULL && next->auto_floor;
    runtime->planet.cached_angle = runtime->planet.angle;
    if (!ado_oj_hit_input_event(runtime, is_auto, 0, input, &result)) {
        result.input_ignored = 1;
        return result;
    }

    double margin_scale = next == NULL ? 1.0 : next->margin_scale;
    AdoOfficialHitMargin hit_margin = ado_official_get_hit_margin(
        config,
        runtime->planet.cached_angle,
        runtime->planet.target_exit_angle,
        runtime->planet.is_cw,
        conductor->bpm * runtime->planet.speed,
        conductor->pitch,
        margin_scale);

    result.processed = 1;
    result.raw_margin = hit_margin;
    result.effective_margin = hit_margin;
    result.display_margin = hit_margin;
    result.hit_angle = runtime->planet.cached_angle;
    result.target_angle = runtime->planet.target_exit_angle;
    result.signed_angle_delta_deg =
        (runtime->planet.cached_angle - runtime->planet.target_exit_angle) *
        (runtime->planet.is_cw ? 1.0 : -1.0) *
        ADO_OJ_RAD2DEG;
    if (conductor->pitch != 0.0) {
        double crotchet = ado_oj_bpm_to_crotchet(conductor->bpm * runtime->planet.speed);
        result.signed_time_offset_ms =
            (runtime->planet.cached_angle - runtime->planet.target_exit_angle) *
            (runtime->planet.is_cw ? 1.0 : -1.0) /
            ADO_OJ_PI *
            crotchet /
            conductor->pitch *
            1000.0;
    }
    result.song_position = runtime->current_song_position;

    AdoOfficialFloor *target_floor = NULL;
    double exit_angle = 0.0;
    if (runtime->controller.gameworld) {
        if (ado_official_is_valid_hit(config, hit_margin) ||
            config->debug ||
            (ado_oj_auto_path_active(runtime, next_auto, is_auto) && !config->use_old_auto) ||
            runtime->player.midspin_infinite_margin ||
            runtime->controller.no_fail_infinite_margin) {
            target_floor = next;
            exit_angle = runtime->planet.target_exit_angle;
            if (runtime->controller.no_fail_infinite_margin) {
                hit_margin = ADO_HIT_FAIL_MISS;
            }
        }
    } else {
        target_floor = ado_oj_select_freeroam_target(runtime, &exit_angle, &result);
    }

    if (target_floor != NULL && ado_oj_failbar_did_fail(runtime) && !runtime->controller.no_fail) {
        target_floor = NULL;
    }

    int non_perfect_family =
        hit_margin != ADO_HIT_PERFECT &&
        hit_margin != ADO_HIT_LATE_PERFECT &&
        hit_margin != ADO_HIT_EARLY_PERFECT;
    if (runtime->revive_bubble_intercept &&
        (target_floor == NULL || non_perfect_family)) {
        result.revive_bubble_intercepted = 1;
        runtime->revive_bubble_intercept = 0;
        return result;
    }

    if (target_floor == NULL || next == NULL) {
        if (config->drum_controller && runtime->controller.booth_mode_debounce_counter > 0.0) {
            result.drum_debounce_returned = 1;
            return result;
        }
        if (config->drum_controller && runtime->controller.gameworld) {
            runtime->controller.booth_mode_debounce_counter = 0.1;
            result.drum_debounce_set = 1;
        }
        if (curr->seq_id > 0 &&
            runtime->controller.gameworld &&
            curr->prev != NULL &&
            curr->prev->hold_length > -1 &&
            fabs(runtime->planet.snapped_last_angle - runtime->planet.angle) < ADO_OJ_PI / 2.0 &&
            !curr->freeroam) {
            if (runtime->controller.strict_holds && next != NULL && next->hold_length < 0) {
                (void)ado_oj_on_damage(runtime, config, conductor, 0, 0, 0, ADO_HIT_TOO_EARLY, &result);
                result.effective_margin = ADO_HIT_TOO_EARLY;
                result.tracker_margin = ADO_HIT_TOO_EARLY;
                result.tracker_hit_added = 1;
            }
            result.skipped_by_hold_guard = 1;
            return result;
        }
        int should_damage =
            ((!curr->freeroam &&
              curr->hold_length > -1 &&
              runtime->controller.checkpoint_seq_id != curr->seq_id) ||
             curr->hold_length == -1);
        if (!should_damage) {
            return result;
        }
        int skip_damage = hit_margin == ADO_HIT_TOO_LATE || curr->freeroam;
        int failed = ado_oj_on_damage(runtime, config, conductor, 0, 0, skip_damage, hit_margin, &result);
        AdoOfficialHitMargin fail_effective = runtime->player.midspin_infinite_margin
                                                  ? ADO_HIT_PERFECT
                                                  : hit_margin;
        if (failed &&
            (config->hit_margin_limit != ADO_MARGIN_LIMIT_PURE_PERFECT_ONLY ||
             runtime->controller.no_fail)) {
            fail_effective = ADO_HIT_FAIL_OVERLOAD;
        }
        if (runtime->player.midspin_infinite_margin) {
            fail_effective = ADO_HIT_PERFECT;
        }
        result.effective_margin = fail_effective;
        result.display_margin = fail_effective;
        result.tracker_margin = fail_effective;
        result.hit_text_margin = fail_effective;
        if (runtime->controller.gameworld) {
            result.tracker_hit_added = 1;
            if (!curr->hide_judgment) {
                result.hit_text_shown = 1;
            }
            if (runtime->conditional_floor != NULL &&
                (hit_margin == ADO_HIT_TOO_EARLY || hit_margin == ADO_HIT_TOO_LATE)) {
                ado_oj_mark_conditional_effect(
                    runtime,
                    &result,
                    ado_oj_conditional_effect_for_margin(hit_margin),
                    hit_margin);
            }
        }
        runtime->controller.multipress_penalty = 0;
        runtime->controller.multipress_and_has_pressed_first_press = 0;
        return result;
    }

    int multipress_hit_text = 0;
    if (runtime->controller.multipress_and_has_pressed_first_press &&
        runtime->controller.multipress_penalty &&
        !runtime->player.midspin_infinite_margin &&
        !curr->freeroam) {
        int apply_damage = 0;
        double bpm = conductor->bpm * runtime->planet.speed * conductor->pitch;
        double moved = ado_oj_get_angle_moved(curr->entryangle, curr->exitangle, !curr->is_ccw);
        double floor_time = ado_oj_angle_to_time(moved, bpm);
        apply_damage = moved > 1.5690509853884578 &&
                       floor_time > runtime->controller.average_frame_time * 3.5 &&
                       floor_time > 0.03999999910593033;
        (void)ado_oj_on_damage(runtime, config, conductor, 1, apply_damage, 0, hit_margin, &result);
        runtime->controller.multipress_penalty = 0;
        runtime->controller.multipress_and_has_pressed_first_press = 0;
        if (runtime->player.consec_multipress_counter > 5 || apply_damage) {
            multipress_hit_text = 1;
        }
    }

    if (runtime->player.key_limiter_over_counter > 0) {
        runtime->player.key_limiter_over_counter--;
        result.overpress = 1;
        result.effective_margin = ADO_HIT_OVERPRESS;
        result.display_margin = ADO_HIT_OVERPRESS;
        result.hit_text_margin = ADO_HIT_OVERPRESS;
        result.hit_text_shown = 1;
        return result;
    }

    if (config->drum_controller && runtime->controller.gameworld) {
        runtime->controller.booth_mode_debounce_counter = 0.1;
        result.drum_debounce_set = 1;
    }

    runtime->player.taps_on_this_floor++;
    int taps_needed = target_floor->taps_needed > 0 ? target_floor->taps_needed : 1;
    if (target_floor->taps_so_far < runtime->player.taps_on_this_floor) {
        target_floor->taps_so_far = runtime->player.taps_on_this_floor;
    }
    if (runtime->player.taps_on_this_floor < taps_needed) {
        result.partial_multitap = 1;
        if (!config->multitap_hit_once) {
            return result;
        }
    }

    AdoOfficialHitMargin floor_grade = hit_margin;
    AdoOfficialHitMargin tracker = hit_margin;
    if (runtime->player.midspin_infinite_margin ||
        (ado_oj_auto_path_active(runtime, next_auto, is_auto) && !config->use_old_auto)) {
        tracker = ADO_HIT_PERFECT;
    }
    if (next_auto) {
        result.auto_hit = 1;
        tracker = ADO_HIT_AUTO;
    }
    AdoOfficialHitMargin post_tracker_margin = tracker;
    if (next_auto) {
        post_tracker_margin = ADO_HIT_PERFECT;
    }

    if (runtime->controller.gameworld) {
        result.floor_grade_margin = floor_grade;
        result.tracker_margin = tracker;
        result.floor_grade_set = 1;
        result.tracker_hit_added = 1;
        result.effective_margin = post_tracker_margin;
        result.display_margin = post_tracker_margin;
        runtime->player.taps_on_this_floor = 0;
        if (target_floor->has_conditional_change) {
            runtime->conditional_floor = target_floor;
            result.conditional_floor_set = 1;
        }
        if (runtime->conditional_floor != NULL) {
            if (post_tracker_margin == ADO_HIT_PERFECT ||
                post_tracker_margin == ADO_HIT_EARLY_PERFECT ||
                post_tracker_margin == ADO_HIT_LATE_PERFECT ||
                post_tracker_margin == ADO_HIT_VERY_EARLY ||
                post_tracker_margin == ADO_HIT_VERY_LATE) {
                ado_oj_mark_conditional_effect(
                    runtime,
                    &result,
                    ado_oj_conditional_effect_for_margin(post_tracker_margin),
                    post_tracker_margin);
            }
        }
    } else {
        result.effective_margin = ADO_HIT_PERFECT;
        result.display_margin = ADO_HIT_PERFECT;
        result.hit_text_margin = ADO_HIT_PERFECT;
    }
    if (runtime->controller.is_puzzle_room) {
        result.puzzle_iframes_applied = 1;
    }
    ado_oj_move_to_next_floor(runtime, config, conductor, target_floor, exit_angle, &result);
    result.hit_returned_true = 1;
    if (next_auto) {
        result.effective_margin = ADO_HIT_PERFECT;
        result.display_margin = ADO_HIT_PERFECT;
        result.hit_text_margin = ADO_HIT_PERFECT;
    } else {
        result.hit_text_margin = result.display_margin;
    }
    if (runtime->controller.gameworld && !target_floor->hide_judgment) {
        result.hit_text_shown = runtime->player.midspin_infinite_margin ? 0 : 1;
        if (multipress_hit_text) {
            result.hit_text_margin = ADO_HIT_MULTIPRESS;
            result.display_margin = ADO_HIT_MULTIPRESS;
        }
    }
    if (runtime->controller.gameworld && !runtime->player.midspin_infinite_margin) {
        result.error_meter_hit_added = 1;
    }
    int pulse_allowed = 1;
    if (runtime->controller.gameworld && runtime->planet.currfloor != NULL) {
        AdoOfficialFloor *new_curr = runtime->planet.currfloor;
        if (new_curr->mid_spin ||
            (new_curr->seq_id > 0 && new_curr->prev != NULL && new_curr->prev->hold_length > -1) ||
            (new_curr->seq_id > 1 &&
             new_curr->prev != NULL &&
             new_curr->prev->mid_spin &&
             new_curr->prev->prev != NULL &&
             new_curr->prev->prev->hold_length > -1)) {
            pulse_allowed = 0;
        }
    }
    result.camera_pulse_allowed = pulse_allowed;
    result.avatar_hit_emitted = pulse_allowed;
    if (runtime->planet.currfloor != NULL && runtime->planet.currfloor->mid_spin) {
        runtime->player.midspin_infinite_margin = 1;
        runtime->player.pending_key_times++;
        runtime->player.pending_midspin_key_times++;
        result.midspin_key_appended = 1;
    } else {
        runtime->player.midspin_infinite_margin = 0;
    }
    return result;
}

static void ado_oj_die_or_recover(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    int overload,
    int hitbox,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || config == NULL || conductor == NULL || result == NULL ||
        runtime->planet.currfloor == NULL) {
        return;
    }
    AdoOfficialFloor *floor = runtime->planet.currfloor;
    int next_auto = floor->next != NULL && floor->next->auto_floor;
    if (!hitbox) {
        if ((ado_oj_auto_path_active(runtime, next_auto, 0) && !config->use_old_auto) ||
            (config->debug && !overload) ||
            (!runtime->controller.gameworld && !floor->freeroam)) {
            result->death_suppressed = 1;
            result->death_suppressed_by_auto_or_debug = 1;
            return;
        }
        int suppress =
            runtime->controller.no_fail ||
            (floor->is_safe && config->hit_margin_limit == ADO_MARGIN_LIMIT_NONE) ||
            runtime->player.miss_cooldown_active;
        if (runtime->controller.no_fail) {
            result->death_suppressed_by_no_fail = 1;
        }
        if (floor->is_safe && config->hit_margin_limit == ADO_MARGIN_LIMIT_NONE) {
            result->death_suppressed_by_safe_floor = 1;
        }
        if (runtime->player.miss_cooldown_active) {
            result->death_suppressed_by_miss_cooldown = 1;
        }
        int can_mark_fail = !runtime->player.miss_cooldown_active;
        if (runtime->controller.is_expo && !suppress && runtime->controller.lives > 0) {
            runtime->controller.lives--;
            result->life_lost = 1;
            result->lives_remaining_after = runtime->controller.lives;
            if (runtime->controller.lives > 0) {
                runtime->player.miss_cooldown_active = 1;
                runtime->player.miss_cooldown_remaining = 2.0;
                result->miss_cooldown_started = 1;
                suppress = 1;
                result->death_suppressed = 1;
            }
        }
        if (suppress) {
            result->death_suppressed = 1;
            if (!floor->hide_judgment && can_mark_fail) {
                if (overload) {
                    result->fail_marked = 1;
                } else {
                    const AdoOfficialFloor *next = floor->next;
                    int taps_needed = next != NULL && next->taps_needed > 0 ? next->taps_needed : 1;
                    if (next == NULL || runtime->player.taps_on_this_floor + 1 == taps_needed) {
                        result->fail_marked = 1;
                    }
                }
            }
            if (!overload && !runtime->player.midspin_infinite_margin) {
                runtime->controller.no_fail_infinite_margin = 1;
                AdoOfficialStepResult before = *result;
                AdoOfficialStepResult recovered = ado_oj_hit(runtime, config, conductor, ado_oj_empty_input(), 0);
                runtime->controller.no_fail_infinite_margin = 0;
                if (recovered.processed) {
                    *result = recovered;
                }
                result->damaged |= before.damaged;
                result->damage_failed |= before.damage_failed;
                result->hold_fail |= before.hold_fail;
                result->post_hold_fail |= before.post_hold_fail;
                result->multipress |= before.multipress;
                result->overpress |= before.overpress;
                result->input_ignored |= before.input_ignored;
                result->hit_input_event_only |= before.hit_input_event_only;
                result->release_input_event_only |= before.release_input_event_only;
                result->life_lost |= before.life_lost;
                result->failbar_damaged |= before.failbar_damaged;
                result->failbar_failed |= before.failbar_failed;
                result->miss_marked |= before.miss_marked;
                result->fail_marked |= before.fail_marked;
                result->miss_cooldown_started |= before.miss_cooldown_started;
                result->key_total_added += before.key_total_added;
                result->key_limiter_over_count += before.key_limiter_over_count;
                if (before.active_down_key_count > result->active_down_key_count) {
                    result->active_down_key_count = before.active_down_key_count;
                }
                result->no_fail_recovered = 1;
            }
            return;
        }
    }
    result->died = 1;
    runtime->player.alive = 0;
    result->death_registered_floor_id = runtime->controller.current_floor_id;
    ado_oj_mark_conditional_effect(
        runtime,
        result,
        ADO_CONDITIONAL_LOSS,
        ADO_HIT_FAIL_OVERLOAD);
}

static void ado_oj_adjust_angle(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConductor *conductor,
    uint64_t event_tick) {
    if (runtime == NULL || conductor == NULL) {
        return;
    }
    runtime->planet.angle = ado_official_get_angle(
        conductor,
        &runtime->player,
        &runtime->planet,
        runtime->planet.snapped_last_angle,
        event_tick);
    runtime->current_event_tick = event_tick;
    runtime->current_song_position = ado_official_get_song_position(conductor, event_tick);
}

static void ado_oj_check_post_hold_fail(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || result == NULL || runtime->planet.currfloor == NULL) {
        return;
    }
    const AdoOfficialFloor *floor = runtime->planet.currfloor;
    double min_angle_margin = ado_oj_min_angle_margin(config, conductor, &runtime->planet);
    double limit = ado_oj_max(ADO_OJ_PI, min_angle_margin * 2.0);
    if (runtime->controller.no_fail || floor->is_safe) {
        limit = min_angle_margin * 1.01;
    }
    double delta = runtime->planet.angle - runtime->planet.target_exit_angle;
    if (!runtime->planet.is_cw) {
        delta *= -1.0;
    }
    if (runtime->controller.gameworld &&
        runtime->controller.floor_count > floor->seq_id + 1 &&
        delta > limit) {
        ado_oj_die_or_recover(runtime, config, conductor, 0, 0, result);
        result->post_hold_fail = 1;
    }
}

static void ado_oj_otto_hold_hit(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || config == NULL || conductor == NULL || result == NULL ||
        runtime->planet.currfloor == NULL) {
        return;
    }
    const AdoOfficialFloor *floor = runtime->planet.currfloor;
    int next_auto = ado_oj_next_tile_is_auto(runtime);
    if ((!(ado_oj_auto_path_active(runtime, next_auto, 0) || runtime->controller.benchmark_mode) &&
         (floor->hold_length <= -1 || !floor->auto_floor))) {
        return;
    }
    int count = config->use_old_auto ? 1 : 5;
    while (count-- > 0 && ado_oj_auto_should_hit_now_with_config(runtime, config)) {
        runtime->player.pending_key_times = 0;
        runtime->player.pending_midspin_key_times = 0;
        AdoOfficialStepResult before_hit = *result;
        *result = ado_oj_hit(runtime, config, conductor, ado_oj_empty_input(), 1);
        ado_oj_merge_input_observation(result, &before_hit);
        ado_oj_adjust_angle(runtime, conductor, runtime->current_event_tick);
    }
}

static void ado_oj_update_hold_behavior(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialInputEvent input,
    int valid_input_was_released,
    int valid_input_was_triggered,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || runtime->planet.currfloor == NULL || result == NULL) {
        return;
    }
    AdoOfficialFloor *floor = runtime->planet.currfloor;
    double hold_margin = ado_oj_hold_margin(config, conductor, runtime);
    int next_auto = ado_oj_next_tile_is_auto(runtime);
    int next_hold = ado_oj_next_tile_is_hold(runtime);

    if (!runtime->controller.gameworld && floor->hold_length > -1) {
        double min_angle_margin = ado_oj_min_angle_margin(config, conductor, &runtime->planet);
        double hold_angle = ADO_OJ_PI * (double)(floor->hold_length * 2 + 1);
        hold_margin = 1.0 - min_angle_margin / hold_angle;
        int hold_behavior_triggered = valid_input_was_triggered;
        if (!valid_input_was_released) {
            hold_behavior_triggered = 0;
            if (!runtime->controller.exiting_to_main_menu && !runtime->controller.paused) {
                int trigger_count = ado_oj_input_count_or_fallback(
                    input.trigger_valid_key_count,
                    input.valid_key_count,
                    input.went_down_count);
                int trigger_over_count = ado_oj_input_count_or_fallback(
                    input.trigger_key_limiter_over_count,
                    input.key_limiter_over_count,
                    0);
                hold_behavior_triggered =
                    ado_oj_count_valid_keys_pressed(
                        runtime,
                        trigger_count,
                        input.pressed_keys_mask,
                        input.held_keys_mask,
                        trigger_over_count,
                        input.active_down_key_count,
                        result) > 0;
            }
        }
        if (valid_input_was_released ||
            (hold_behavior_triggered && !runtime->controller.strict_holds)) {
            if (floor->hold_completion > hold_margin) {
                floor->hold_completion = 0.0;
                AdoOfficialStepResult before_hit = *result;
                *result = ado_oj_hit(runtime, config, conductor, ado_oj_empty_input(), 0);
                ado_oj_merge_input_observation(result, &before_hit);
            } else {
                ado_oj_cancel_hold(runtime, result);
                return;
            }
        }
        if (floor->hold_completion > 2.0 - hold_margin || floor->hold_completion < -0.3) {
            ado_oj_cancel_hold(runtime, result);
            return;
        }
    }

    if (!runtime->controller.gameworld ||
        !valid_input_was_released ||
        next_auto ||
        floor->auto_floor ||
        ado_oj_auto_path_active(runtime, 0, 0) ||
        runtime->controller.benchmark_mode ||
        floor->hold_length <= -1) {
        return;
    }

    if (floor->hold_completion < hold_margin) {
        if (runtime->controller.checkpoint_seq_id != floor->seq_id &&
            runtime->controller.require_holding) {
            ado_oj_die_or_recover(runtime, config, conductor, 0, 0, result);
            result->hold_fail = 1;
        }
    } else if (!next_hold) {
        AdoOfficialStepResult before_hit = *result;
        *result = ado_oj_hit(runtime, config, conductor, ado_oj_empty_input(), 0);
        ado_oj_merge_input_observation(result, &before_hit);
    }
}

static void ado_oj_hit_hold_floors_if_started_at_hold(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || runtime->planet.currfloor == NULL || result == NULL) {
        return;
    }
    const AdoOfficialFloor *floor = runtime->planet.currfloor;
    if (!runtime->player.auto_player &&
        !runtime->player.invincible_auto &&
        !runtime->controller.benchmark_mode &&
        ado_oj_auto_should_hit_now_with_config(runtime, config) &&
        floor->hold_length > -1 &&
        runtime->controller.checkpoint_seq_id == floor->seq_id &&
        !runtime->controller.is_fail_state) {
        AdoOfficialStepResult before_hit = *result;
        *result = ado_oj_hit(runtime, config, conductor, ado_oj_empty_input(), 0);
        ado_oj_merge_input_observation(result, &before_hit);
    }
}

static void ado_oj_check_pre_hold_fail(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialStepResult *result) {
    if (runtime == NULL || runtime->planet.currfloor == NULL || result == NULL) {
        return;
    }
    const AdoOfficialFloor *floor = runtime->planet.currfloor;
    int next_auto = ado_oj_next_tile_is_auto(runtime);
    double hold_margin = ado_oj_hold_margin(config, conductor, runtime);
    if (runtime->controller.gameworld &&
        !next_auto &&
        !floor->auto_floor &&
        floor->hold_completion > hold_margin &&
        floor->hold_completion < 1.0 - hold_margin &&
        !ado_oj_auto_path_active(runtime, 0, 0) &&
        !runtime->player.holding &&
        runtime->controller.require_holding &&
        runtime->controller.checkpoint_seq_id != floor->seq_id &&
        !runtime->controller.benchmark_mode &&
        floor->hold_length > -1) {
        ado_oj_die_or_recover(runtime, config, conductor, 0, 0, result);
        result->hold_fail = 1;
    }
}

static int ado_oj_floor_seq(const AdoOfficialRuntime *runtime) {
    if (runtime == NULL || runtime->planet.currfloor == NULL) {
        return -1;
    }
    return runtime->planet.currfloor->seq_id;
}

AdoOfficialStepResult ado_official_playercontrol_step(
    AdoOfficialRuntime *runtime,
    const AdoOfficialConfig *config,
    const AdoOfficialConductor *conductor,
    AdoOfficialInputEvent input) {
    AdoOfficialStepResult result;
    memset(&result, 0, sizeof(result));
    result.raw_margin = ADO_HIT_PERFECT;
    result.effective_margin = ADO_HIT_PERFECT;
    result.display_margin = ADO_HIT_PERFECT;
    result.floor_grade_margin = ADO_HIT_PERFECT;
    result.tracker_margin = ADO_HIT_PERFECT;
    result.hit_text_margin = ADO_HIT_PERFECT;
    result.conditional_effect_margin = ADO_HIT_PERFECT;
    result.death_registered_floor_id = -1;
    result.target_seq_id = -1;
    result.moving_to_seq_id = -1;
    result.freeroam_landing_seq_id = -1;

    if (runtime == NULL || config == NULL || conductor == NULL) {
        return result;
    }
    ado_oj_update_miss_cooldown(runtime, conductor, input);
    if (input.frame_count != 0) {
        runtime->current_frame_count = input.frame_count;
    } else {
        runtime->current_frame_count++;
    }
    if (!runtime->player.alive ||
        runtime->controller.paused ||
        runtime->controller.is_cutscene ||
        runtime->planet.currfloor == NULL) {
        ado_oj_capture_step_exit_state(runtime, &result);
        return result;
    }
    if (runtime->controller.curr_floor == NULL) {
        ado_oj_sync_controller_floor(runtime, runtime->planet.currfloor, &result);
    }

    ado_oj_adjust_angle(runtime, conductor, input.event_tick);
    result.song_position = runtime->current_song_position;
    runtime->next_tile_is_hold_cached = 0;
    AdoOfficialFloor *cached_next = ado_oj_next_non_midspin_floor(runtime->planet.currfloor);
    if (cached_next != NULL && cached_next->hold_length > -1) {
        runtime->next_tile_is_hold_cached = 1;
    }
    if (input.delta_song_pos != 0.0) {
        ado_oj_failbar_update(runtime, config, conductor, input.delta_song_pos, &result);
        if (result.died) {
            ado_oj_capture_step_exit_state(runtime, &result);
            return result;
        }
    }

    int valid_input_was_released = ado_oj_valid_input_was_released(runtime, input);
    int valid_input_was_triggered = 0;

    for (int prev_seq = -1, guard = 0; guard < 16 && prev_seq != ado_oj_floor_seq(runtime); guard++) {
        prev_seq = ado_oj_floor_seq(runtime);
        ado_oj_adjust_angle(runtime, conductor, input.event_tick);
        ado_oj_check_post_hold_fail(runtime, config, conductor, &result);
    }
    if (result.died) {
        ado_oj_capture_step_exit_state(runtime, &result);
        return result;
    }

    for (int prev_seq = -1, guard = 0; guard < 16 && prev_seq != ado_oj_floor_seq(runtime); guard++) {
        prev_seq = ado_oj_floor_seq(runtime);
        ado_oj_adjust_angle(runtime, conductor, input.event_tick);
        ado_oj_otto_hold_hit(runtime, config, conductor, &result);
    }

    if (!runtime->controller.exiting_to_main_menu && !runtime->controller.paused) {
        int trigger_count = ado_oj_input_count_or_fallback(
            input.trigger_valid_key_count,
            input.valid_key_count,
            input.went_down_count);
        int trigger_over_count = ado_oj_input_count_or_fallback(
            input.trigger_key_limiter_over_count,
            input.key_limiter_over_count,
            0);
        valid_input_was_triggered =
            ado_oj_count_valid_keys_pressed(
                runtime,
                trigger_count,
                input.pressed_keys_mask,
                input.held_keys_mask,
                trigger_over_count,
                input.active_down_key_count,
                &result) > 0;
    }

    int enqueue_count = 0;
    if (valid_input_was_triggered) {
        enqueue_count = ado_oj_input_count_or_fallback(
            input.enqueue_valid_key_count,
            input.valid_key_count,
            input.went_down_count);
        int enqueue_over_count = ado_oj_input_count_or_fallback(
            input.enqueue_key_limiter_over_count,
            input.key_limiter_over_count,
            0);
        enqueue_count = ado_oj_count_valid_keys_pressed(
            runtime,
            enqueue_count,
            input.pressed_keys_mask,
            input.held_keys_mask,
            enqueue_over_count,
            input.active_down_key_count,
            &result);
    }
    if (enqueue_count > 0) {
        runtime->player.pending_key_times += enqueue_count;
        if (enqueue_count == 1) {
            runtime->player.consec_multipress_counter = 0;
        }
        runtime->controller.multipress_and_has_pressed_first_press = 1;
    }

    ado_oj_adjust_angle(runtime, conductor, input.event_tick);
    ado_oj_update_hold_behavior(
        runtime,
        config,
        conductor,
        input,
        valid_input_was_released,
        valid_input_was_triggered,
        &result);
    ado_oj_adjust_angle(runtime, conductor, input.event_tick);

    for (int prev_seq = -1, guard = 0; guard < 16 && prev_seq != ado_oj_floor_seq(runtime); guard++) {
        prev_seq = ado_oj_floor_seq(runtime);
        ado_oj_adjust_angle(runtime, conductor, input.event_tick);
        ado_oj_hit_hold_floors_if_started_at_hold(runtime, config, conductor, &result);
    }

    for (int prev_seq = -1, guard = 0; guard < 16 && prev_seq != ado_oj_floor_seq(runtime); guard++) {
        prev_seq = ado_oj_floor_seq(runtime);
        ado_oj_adjust_angle(runtime, conductor, input.event_tick);
        ado_oj_check_pre_hold_fail(runtime, config, conductor, &result);
    }
    if (result.died) {
        ado_oj_capture_step_exit_state(runtime, &result);
        return result;
    }

    for (int prev_seq = -1, guard = 0; guard < 16 && prev_seq != ado_oj_floor_seq(runtime); guard++) {
        prev_seq = ado_oj_floor_seq(runtime);
        ado_oj_adjust_angle(runtime, conductor, input.event_tick);

        int next_hold = ado_oj_next_tile_is_hold(runtime);
        double hold_margin = ado_oj_hold_margin(config, conductor, runtime);
        const AdoOfficialFloor *floor = runtime->planet.currfloor;
        int can_consume_key_time =
            floor != NULL &&
            runtime->player.pending_key_times > 0 &&
            !config->stationary &&
            (((floor->hold_length > -1 && !runtime->controller.strict_holds) || next_hold) ||
             floor->hold_length == -1 ||
             floor->hold_completion < hold_margin) &&
            (!runtime->controller.gameworld ||
             runtime->controller.floor_count <= 0 ||
             floor->seq_id < runtime->controller.floor_count - 1) &&
            !runtime->controller.benchmark_mode;

        if (!can_consume_key_time) {
            break;
        }

        runtime->player.pending_key_times--;
        if (((runtime->player.auto_player || runtime->player.invincible_auto) && runtime->controller.gameworld) ||
            (ado_oj_next_tile_is_auto(runtime) && floor != NULL && !floor->freeroam)) {
            result.processed = 1;
            result.hit_input_event_only = 1;
            if (!ado_oj_hit_input_event(runtime, 0, 0, input, &result)) {
                result.input_ignored = 1;
            }
            break;
        } else {
            AdoOfficialStepResult before_hit = result;
            result = ado_oj_hit(runtime, config, conductor, input, 0);
            ado_oj_merge_input_observation(&result, &before_hit);
            result.song_position = runtime->current_song_position;
            ado_oj_refresh_hold_keys_after_hit(runtime, input, &result);
        }
        if (runtime->player.midspin_infinite_margin &&
            runtime->player.pending_key_times > 0 &&
            runtime->player.pending_midspin_key_times > 0) {
            runtime->player.pending_key_times--;
            runtime->player.pending_midspin_key_times--;
            result.midspin_second_hit = 1;
            AdoOfficialStepResult before_hit = result;
            result = ado_oj_hit(runtime, config, conductor, input, 0);
            ado_oj_merge_input_observation(&result, &before_hit);
            result.midspin_second_hit = 1;
            result.song_position = runtime->current_song_position;
            ado_oj_refresh_hold_keys_after_hit(runtime, input, &result);
        }
    }

    if (input.went_up_count > 0 || input.release_input_ignore_add_count > 0) {
        result.release_input_event_only = 1;
        if (!ado_oj_hit_input_event(runtime, 0, 1, input, &result)) {
            result.input_ignored = 1;
        }
    }

    if (input.delta_song_pos != 0.0) {
        ado_oj_update_lock_input(runtime, conductor, input.delta_song_pos, &result);
    }

    ado_oj_capture_step_exit_state(runtime, &result);
    return result;
}

#ifdef ADO_OFFICIAL_JUDGEMENT_SELFTEST
static AdoOfficialStepResult ado_oj_selftest_result(void) {
    AdoOfficialStepResult result;
    memset(&result, 0, sizeof(result));
    result.raw_margin = ADO_HIT_PERFECT;
    result.effective_margin = ADO_HIT_PERFECT;
    result.display_margin = ADO_HIT_PERFECT;
    result.floor_grade_margin = ADO_HIT_PERFECT;
    result.tracker_margin = ADO_HIT_PERFECT;
    result.hit_text_margin = ADO_HIT_PERFECT;
    result.conditional_effect_margin = ADO_HIT_PERFECT;
    result.death_registered_floor_id = -1;
    result.target_seq_id = -1;
    result.moving_to_seq_id = -1;
    result.freeroam_landing_seq_id = -1;
    return result;
}

static AdoOfficialConductor ado_oj_selftest_conductor(void) {
    AdoOfficialConductor conductor;
    memset(&conductor, 0, sizeof(conductor));
    conductor.bpm = 166.25;
    conductor.pitch = 1.0;
    conductor.crotchet_at_start = 60.0 / 166.25;
    return conductor;
}

static void ado_oj_selftest_runtime(
    AdoOfficialRuntime *runtime,
    AdoOfficialFloor *floor,
    AdoOfficialFloor *next,
    int seq_id) {
    ado_official_runtime_defaults(runtime);
    ado_official_floor_defaults(floor);
    ado_official_floor_defaults(next);

    floor->seq_id = seq_id;
    floor->next = next;
    next->seq_id = seq_id + 1;
    next->prev = floor;

    runtime->planet.currfloor = floor;
    runtime->controller.curr_floor = floor;
    runtime->controller.current_seq_id = seq_id;
    runtime->controller.current_floor_id = seq_id;
    runtime->controller.floor_count = seq_id + 2;
    runtime->controller.checkpoint_seq_id = 0;
    runtime->controller.strict_holds = 1;
    runtime->controller.require_holding = 1;
    runtime->planet.speed = 1.0;
    runtime->planet.is_cw = 1;
}

static int ado_oj_selftest_expect(int condition, const char *name) {
    if (condition) {
        return 0;
    }
    fprintf(stderr, "official_judgement selftest failed: %s\n", name);
    return 1;
}

static int ado_oj_selftest_update_hold_behavior_release_fail(void) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();
    AdoOfficialStepResult result = ado_oj_selftest_result();

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, 5);
    floor.hold_length = 1;
    floor.hold_completion = 0.5091506;
    floor.angle_length = ADO_OJ_PI * 3.0;
    runtime.planet.angle = 8.81093216054937;
    runtime.planet.target_exit_angle = 14.1371668018401;
    runtime.player.holding = 0;
    runtime.player.hold_key_count = 0;

    ado_oj_update_hold_behavior(
        &runtime,
        &config,
        &conductor,
        ado_oj_empty_input(),
        1,
        0,
        &result);

    int failures = 0;
    failures += ado_oj_selftest_expect(result.died, "trace frame 24544 should die");
    failures += ado_oj_selftest_expect(result.hold_fail, "trace frame 24544 should set hold_fail");
    failures += ado_oj_selftest_expect(!runtime.player.alive, "trace frame 24544 should kill player");
    failures += ado_oj_selftest_expect(result.death_registered_floor_id == 5, "trace frame 24544 death floor");
    return failures;
}

static int ado_oj_selftest_update_hold_behavior_release_fail_one(
    int seq_id,
    int hold_length,
    double hold_completion,
    double angle,
    double target_exit_angle,
    double angle_length,
    int strict_holds,
    const char *label) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();
    AdoOfficialStepResult result = ado_oj_selftest_result();

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, seq_id);
    runtime.controller.strict_holds = strict_holds;
    floor.hold_length = hold_length;
    floor.hold_completion = hold_completion;
    floor.angle_length = angle_length;
    runtime.planet.angle = angle;
    runtime.planet.target_exit_angle = target_exit_angle;
    runtime.player.holding = 0;
    runtime.player.hold_key_count = 0;

    ado_oj_update_hold_behavior(
        &runtime,
        &config,
        &conductor,
        ado_oj_empty_input(),
        1,
        0,
        &result);

    int failures = 0;
    failures += ado_oj_selftest_expect(result.died, label);
    failures += ado_oj_selftest_expect(result.hold_fail, "release fail should set hold_fail");
    failures += ado_oj_selftest_expect(!runtime.player.alive, "release fail should kill player");
    failures += ado_oj_selftest_expect(result.death_registered_floor_id == seq_id, "release fail death floor");
    return failures;
}

static int ado_oj_selftest_update_hold_behavior_release_survives_one(
    int seq_id,
    int hold_length,
    double hold_completion,
    double angle,
    double target_exit_angle,
    double angle_length,
    int strict_holds,
    int require_holding,
    const char *label) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();
    AdoOfficialStepResult result = ado_oj_selftest_result();

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, seq_id);
    runtime.controller.strict_holds = strict_holds;
    runtime.controller.require_holding = require_holding;
    floor.hold_length = hold_length;
    floor.hold_completion = hold_completion;
    floor.angle_length = angle_length;
    runtime.planet.angle = angle;
    runtime.planet.target_exit_angle = target_exit_angle;
    runtime.player.holding = 0;
    runtime.player.hold_key_count = 0;

    ado_oj_update_hold_behavior(
        &runtime,
        &config,
        &conductor,
        ado_oj_empty_input(),
        1,
        0,
        &result);

    int failures = 0;
    failures += ado_oj_selftest_expect(!result.died, label);
    failures += ado_oj_selftest_expect(!result.hold_fail, "release survive should not set hold_fail");
    failures += ado_oj_selftest_expect(runtime.player.alive, "release survive should keep player alive");
    return failures;
}

static int ado_oj_selftest_post_hold_fail_one(
    int seq_id,
    int hold_length,
    double hold_completion,
    double angle,
    double target_exit_angle,
    int strict_holds,
    int require_holding,
    const char *label) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();
    AdoOfficialStepResult result = ado_oj_selftest_result();

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, seq_id);
    runtime.controller.strict_holds = strict_holds;
    runtime.controller.require_holding = require_holding;
    floor.hold_length = hold_length;
    floor.hold_completion = hold_completion;
    runtime.planet.angle = angle;
    runtime.planet.target_exit_angle = target_exit_angle;

    ado_oj_check_post_hold_fail(&runtime, &config, &conductor, &result);

    int failures = 0;
    failures += ado_oj_selftest_expect(result.died, label);
    failures += ado_oj_selftest_expect(result.post_hold_fail, "post hold fail flag");
    failures += ado_oj_selftest_expect(!runtime.player.alive, "post hold fail should kill player");
    failures += ado_oj_selftest_expect(result.death_registered_floor_id == seq_id, "post hold fail death floor");
    return failures;
}

static AdoOfficialInputEvent ado_oj_selftest_press_input(int frame_count) {
    AdoOfficialInputEvent input = ado_oj_empty_input();
    input.frame_count = frame_count;
    input.went_down_count = 1;
    input.trigger_valid_key_count = 1;
    input.enqueue_valid_key_count = 1;
    input.valid_key_count = 1;
    input.active_down_key_count = 1;
    input.hit_input_slot_mask = 1;
    input.pressed_keys_mask = 1;
    return input;
}

static int ado_oj_selftest_multitap_normal_one(
    int seq_id,
    int taps_needed,
    const char *label) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();

    ado_official_config_defaults(&config);
    config.multitap_hit_once = 0;
    ado_oj_selftest_runtime(&runtime, &floor, &next, seq_id);
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    runtime.planet.angle = 0.0;
    runtime.planet.target_exit_angle = 0.0;
    next.taps_needed = taps_needed;

    int failures = 0;
    for (int i = 1; i <= taps_needed; i++) {
        AdoOfficialStepResult result = ado_oj_selftest_result();
        result = ado_oj_hit(
            &runtime,
            &config,
            &conductor,
            ado_oj_selftest_press_input(1000 + i),
            0);

        failures += ado_oj_selftest_expect(result.processed, label);
        failures += ado_oj_selftest_expect(result.raw_margin == ADO_HIT_PERFECT, "multitap hit should be perfect");
        if (i < taps_needed) {
            failures += ado_oj_selftest_expect(result.partial_multitap, "normal multitap partial flag");
            failures += ado_oj_selftest_expect(!result.hit_returned_true, "normal multitap partial should not return hit");
            failures += ado_oj_selftest_expect(!result.moved_to_next_floor, "normal multitap partial should not move");
            failures += ado_oj_selftest_expect(runtime.player.taps_on_this_floor == i, "normal multitap partial player tap count");
            failures += ado_oj_selftest_expect(next.taps_so_far == i, "normal multitap partial floor tap count");
            failures += ado_oj_selftest_expect(runtime.planet.currfloor == &floor, "normal multitap partial current floor");
        } else {
            failures += ado_oj_selftest_expect(!result.partial_multitap, "normal multitap complete should not be partial");
            failures += ado_oj_selftest_expect(result.hit_returned_true, "normal multitap complete should return hit");
            failures += ado_oj_selftest_expect(result.moved_to_next_floor, "normal multitap complete should move");
            failures += ado_oj_selftest_expect(runtime.player.taps_on_this_floor == 0, "normal multitap complete resets player tap count");
            failures += ado_oj_selftest_expect(next.taps_so_far == taps_needed, "normal multitap complete floor tap count");
            failures += ado_oj_selftest_expect(runtime.planet.currfloor == &next, "normal multitap complete current floor");
        }
    }
    return failures;
}

static int ado_oj_selftest_multitap_hit_once_one(void) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();

    ado_official_config_defaults(&config);
    config.multitap_hit_once = 1;
    ado_oj_selftest_runtime(&runtime, &floor, &next, 4);
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    runtime.planet.angle = 0.0;
    runtime.planet.target_exit_angle = 0.0;
    next.taps_needed = 3;

    AdoOfficialStepResult result = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1200),
        0);

    int failures = 0;
    failures += ado_oj_selftest_expect(result.processed, "HitOnce multitap should process first hit");
    failures += ado_oj_selftest_expect(result.partial_multitap, "HitOnce multitap first hit keeps partial flag");
    failures += ado_oj_selftest_expect(result.hit_returned_true, "HitOnce multitap first hit should return hit");
    failures += ado_oj_selftest_expect(result.moved_to_next_floor, "HitOnce multitap first hit should move");
    failures += ado_oj_selftest_expect(runtime.player.taps_on_this_floor == 0, "HitOnce multitap first hit resets player tap count");
    failures += ado_oj_selftest_expect(next.taps_so_far == 1, "HitOnce multitap first hit records one tap");
    failures += ado_oj_selftest_expect(runtime.planet.currfloor == &next, "HitOnce multitap first hit current floor");
    return failures;
}

static int ado_oj_selftest_multitap_normal_nofail_invalid_margin(void) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();

    ado_official_config_defaults(&config);
    config.multitap_hit_once = 0;
    ado_oj_selftest_runtime(&runtime, &floor, &next, 4);
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    runtime.controller.no_fail = 1;
    runtime.controller.no_fail_infinite_margin = 1;
    runtime.planet.angle = 2.0;
    runtime.planet.target_exit_angle = 0.0;
    next.taps_needed = 3;
    runtime.player.taps_on_this_floor = 1;
    next.taps_so_far = 1;

    AdoOfficialStepResult partial = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1300),
        0);

    int failures = 0;
    failures += ado_oj_selftest_expect(partial.processed, "noFail multitap invalid partial should process");
    failures += ado_oj_selftest_expect(partial.raw_margin == ADO_HIT_TOO_LATE, "noFail multitap invalid partial raw margin");
    failures += ado_oj_selftest_expect(partial.partial_multitap, "noFail multitap invalid partial flag");
    failures += ado_oj_selftest_expect(!partial.hit_returned_true, "noFail multitap invalid partial should not return hit");
    failures += ado_oj_selftest_expect(!partial.moved_to_next_floor, "noFail multitap invalid partial should not move");
    failures += ado_oj_selftest_expect(runtime.player.taps_on_this_floor == 2, "noFail multitap invalid partial tap count");
    failures += ado_oj_selftest_expect(next.taps_so_far == 2, "noFail multitap invalid partial floor count");
    failures += ado_oj_selftest_expect(runtime.planet.currfloor == &floor, "noFail multitap invalid partial current floor");

    AdoOfficialStepResult complete = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1301),
        0);

    failures += ado_oj_selftest_expect(complete.processed, "noFail multitap invalid complete should process");
    failures += ado_oj_selftest_expect(complete.raw_margin == ADO_HIT_TOO_LATE, "noFail multitap invalid complete raw margin");
    failures += ado_oj_selftest_expect(!complete.partial_multitap, "noFail multitap invalid complete should not be partial");
    failures += ado_oj_selftest_expect(complete.hit_returned_true, "noFail multitap invalid complete should return hit");
    failures += ado_oj_selftest_expect(complete.moved_to_next_floor, "noFail multitap invalid complete should move");
    failures += ado_oj_selftest_expect(complete.floor_grade_margin == ADO_HIT_FAIL_MISS, "noFail multitap invalid complete grade");
    failures += ado_oj_selftest_expect(runtime.player.taps_on_this_floor == 0, "noFail multitap invalid complete resets tap count");
    failures += ado_oj_selftest_expect(next.taps_so_far == 3, "noFail multitap invalid complete floor count");
    failures += ado_oj_selftest_expect(runtime.planet.currfloor == &next, "noFail multitap invalid complete current floor");
    return failures;
}

static int ado_oj_selftest_new_auto_tile_hit(void) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();

    ado_official_config_defaults(&config);
    config.use_old_auto = 0;
    ado_oj_selftest_runtime(&runtime, &floor, &next, 3);
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    runtime.player.auto_player = 1;
    runtime.planet.angle = 0.0;
    runtime.planet.target_exit_angle = 0.0;
    next.auto_floor = 1;

    AdoOfficialStepResult result = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1400),
        1);

    int failures = 0;
    failures += ado_oj_selftest_expect(result.processed, "new auto tile should process");
    failures += ado_oj_selftest_expect(result.raw_margin == ADO_HIT_PERFECT, "new auto tile raw margin");
    failures += ado_oj_selftest_expect(result.auto_hit, "new auto tile auto flag");
    failures += ado_oj_selftest_expect(result.hit_returned_true, "new auto tile should return hit");
    failures += ado_oj_selftest_expect(result.moved_to_next_floor, "new auto tile should move");
    failures += ado_oj_selftest_expect(result.floor_grade_margin == ADO_HIT_PERFECT, "new auto tile floor grade");
    failures += ado_oj_selftest_expect(result.tracker_margin == ADO_HIT_AUTO, "new auto tile tracker margin");
    failures += ado_oj_selftest_expect(result.effective_margin == ADO_HIT_PERFECT, "new auto tile effective margin");
    failures += ado_oj_selftest_expect(result.display_margin == ADO_HIT_PERFECT, "new auto tile display margin");
    failures += ado_oj_selftest_expect(result.hit_text_margin == ADO_HIT_PERFECT, "new auto tile hit text margin");
    failures += ado_oj_selftest_expect(runtime.planet.currfloor == &next, "new auto tile current floor");
    return failures;
}

static int ado_oj_selftest_conditional_effect_counts(void) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();
    int failures = 0;

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, 4);
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    runtime.conditional_floor = &floor;
    runtime.planet.angle = -10.0;
    runtime.planet.cached_angle = -10.0;
    runtime.planet.target_exit_angle = 0.0;

    AdoOfficialStepResult miss_without_effect = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1500),
        0);
    failures += ado_oj_selftest_expect(
        miss_without_effect.raw_margin == ADO_HIT_TOO_EARLY,
        "conditional count zero miss raw margin");
    failures += ado_oj_selftest_expect(
        !miss_without_effect.conditional_effect_triggered,
        "conditional count zero should not trigger TooEarly effect");

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, 4);
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    runtime.conditional_floor = &floor;
    floor.conditional_too_early_count = 1;
    runtime.planet.angle = -10.0;
    runtime.planet.cached_angle = -10.0;
    runtime.planet.target_exit_angle = 0.0;

    AdoOfficialStepResult miss_with_effect = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1501),
        0);
    failures += ado_oj_selftest_expect(
        miss_with_effect.conditional_effect_triggered,
        "conditional TooEarly count one should trigger");
    failures += ado_oj_selftest_expect(
        miss_with_effect.conditional_effect == ADO_CONDITIONAL_TOO_EARLY,
        "conditional TooEarly effect kind");

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, 4);
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    next.has_conditional_change = 1;
    next.conditional_perfect_count = 1;
    runtime.planet.angle = 0.0;
    runtime.planet.cached_angle = 0.0;
    runtime.planet.target_exit_angle = 0.0;

    AdoOfficialStepResult perfect_with_effect = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1502),
        0);
    failures += ado_oj_selftest_expect(
        perfect_with_effect.conditional_floor_set,
        "conditional success should set conditional floor");
    failures += ado_oj_selftest_expect(
        perfect_with_effect.conditional_effect_triggered,
        "conditional Perfect count one should trigger");
    failures += ado_oj_selftest_expect(
        perfect_with_effect.conditional_effect == ADO_CONDITIONAL_PERFECT,
        "conditional Perfect effect kind");

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, 4);
    runtime.conditional_floor = &floor;
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    floor.conditional_loss_count = 0;
    ado_oj_die_or_recover(&runtime, &config, &conductor, 1, 0, &miss_without_effect);
    failures += ado_oj_selftest_expect(
        !miss_without_effect.conditional_effect_triggered,
        "conditional loss count zero should not trigger");

    AdoOfficialStepResult loss_with_effect = ado_oj_selftest_result();
    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, 4);
    runtime.conditional_floor = &floor;
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    floor.conditional_loss_count = 1;
    ado_oj_die_or_recover(&runtime, &config, &conductor, 1, 0, &loss_with_effect);
    failures += ado_oj_selftest_expect(
        loss_with_effect.conditional_effect_triggered,
        "conditional loss count one should trigger");
    failures += ado_oj_selftest_expect(
        loss_with_effect.conditional_effect == ADO_CONDITIONAL_LOSS,
        "conditional loss effect kind");

    return failures;
}

static int ado_oj_selftest_freeroam_success_does_not_grade(void) {
    AdoOfficialConfig config;
    AdoOfficialRuntime runtime;
    AdoOfficialFloor floor;
    AdoOfficialFloor next;
    AdoOfficialConductor conductor = ado_oj_selftest_conductor();

    ado_official_config_defaults(&config);
    ado_oj_selftest_runtime(&runtime, &floor, &next, 0);
    runtime.controller.gameworld = 0;
    runtime.controller.strict_holds = 0;
    runtime.controller.require_holding = 0;
    runtime.freeroam_selection_ready = 1;
    runtime.freeroam_target_floor = &next;
    runtime.freeroam_exit_angle = 1.570796251297;
    runtime.player.taps_on_this_floor = 4;
    runtime.planet.angle = 4.48692092792076;
    runtime.planet.cached_angle = 4.48692092792076;
    runtime.planet.target_exit_angle = 1.570796251297;

    AdoOfficialStepResult result = ado_oj_hit(
        &runtime,
        &config,
        &conductor,
        ado_oj_selftest_press_input(1600),
        0);

    int failures = 0;
    failures += ado_oj_selftest_expect(result.processed, "freeroam hit should process");
    failures += ado_oj_selftest_expect(result.raw_margin == ADO_HIT_TOO_LATE, "freeroam raw margin should be TooLate");
    failures += ado_oj_selftest_expect(result.hit_returned_true, "freeroam hit should return true");
    failures += ado_oj_selftest_expect(result.moved_to_next_floor, "freeroam hit should move");
    failures += ado_oj_selftest_expect(!result.floor_grade_set, "freeroam hit should not set floor grade");
    failures += ado_oj_selftest_expect(!result.tracker_hit_added, "freeroam hit should not add tracker hit");
    failures += ado_oj_selftest_expect(runtime.player.taps_on_this_floor == 5, "freeroam hit should not reset tapsOnThisFloor");
    failures += ado_oj_selftest_expect(result.effective_margin == ADO_HIT_PERFECT, "freeroam move margin should remain Perfect");
    return failures;
}

int ado_official_judgement_selftest(void) {
    int failures = 0;
    failures += ado_oj_selftest_update_hold_behavior_release_fail();
    failures += ado_oj_selftest_update_hold_behavior_release_fail_one(
        5,
        1,
        0.4520324,
        8.22356186050341,
        14.1371668018401,
        ADO_OJ_PI * 3.0,
        0,
        "trace frame 10907 CanHitEnd early release should die");
    failures += ado_oj_selftest_update_hold_behavior_release_fail_one(
        5,
        1,
        0.145788,
        4.79288860125671,
        14.1371668018401,
        ADO_OJ_PI * 3.0,
        0,
        "trace frame 16871 CanHitEnd early release should die");
    failures += ado_oj_selftest_update_hold_behavior_release_fail_one(
        19,
        0,
        0.02945898,
        4.85853865651139,
        7.85398149490356,
        ADO_OJ_PI,
        0,
        "trace frame 20309 CanHitEnd short hold release should die");
    failures += ado_oj_selftest_update_hold_behavior_release_survives_one(
        5,
        1,
        0.04506975,
        3.78315797620512,
        14.1371668018401,
        ADO_OJ_PI * 3.0,
        0,
        0,
        "trace frame 197339 NoHoldNeeded early release should survive");
    failures += ado_oj_selftest_post_hold_fail_one(
        12,
        -1,
        0.0,
        11.1491841912659,
        7.85398149490356,
        1,
        1,
        "trace frame 27834 should die by CheckPostHoldFail");
    failures += ado_oj_selftest_post_hold_fail_one(
        6,
        -1,
        0.0,
        11.0535019556474,
        7.85398149490356,
        1,
        1,
        "trace frame 30455 should die by CheckPostHoldFail");
    failures += ado_oj_selftest_post_hold_fail_one(
        5,
        1,
        1.269479,
        17.2805662080494,
        14.1371668018401,
        0,
        1,
        "trace frame 13970 CanHitEnd overheld hold should die by CheckPostHoldFail");
    failures += ado_oj_selftest_post_hold_fail_one(
        11,
        1,
        1.276467,
        17.3664070499791,
        14.1371668018401,
        0,
        0,
        "trace frame 197565 NoHoldNeeded overheld hold should die by CheckPostHoldFail");
    failures += ado_oj_selftest_post_hold_fail_one(
        17,
        5,
        1.08411,
        42.5004633764129,
        39.2699087299407,
        0,
        0,
        "trace frame 201078 NoHoldNeeded long hold overrun should die by CheckPostHoldFail");
    failures += ado_oj_selftest_multitap_normal_one(
        4,
        3,
        "trace multitap Normal tapsNeeded=3 should require three valid hits");
    failures += ado_oj_selftest_multitap_normal_one(
        12,
        2,
        "trace multitap Normal tapsNeeded=2 should require two valid hits");
    failures += ado_oj_selftest_multitap_hit_once_one();
    failures += ado_oj_selftest_multitap_normal_nofail_invalid_margin();
    failures += ado_oj_selftest_new_auto_tile_hit();
    failures += ado_oj_selftest_conditional_effect_counts();
    failures += ado_oj_selftest_freeroam_success_does_not_grade();
    return failures;
}

#ifdef ADO_OFFICIAL_JUDGEMENT_SELFTEST_MAIN
int main(void) {
    int failures = ado_official_judgement_selftest();
    if (failures == 0) {
        printf("official_judgement selftest passed\n");
        return 0;
    }
    return 1;
}
#endif
#endif
