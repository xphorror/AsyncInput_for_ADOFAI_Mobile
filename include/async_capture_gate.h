#ifndef ADOFAI_ASYNC_CAPTURE_GATE_H
#define ADOFAI_ASYNC_CAPTURE_GATE_H

static inline int ado_async_controller_allows_capture(
    int live_gameworld,
    int current_state,
    int destination_state_known,
    int destination_state,
    int player_control_state) {
    if (!live_gameworld || current_state != player_control_state) {
        return 0;
    }
    return !destination_state_known ||
        destination_state == player_control_state;
}

#endif
