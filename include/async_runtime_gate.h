#ifndef ADOFAI_ASYNC_RUNTIME_GATE_H
#define ADOFAI_ASYNC_RUNTIME_GATE_H

typedef struct AdoAsyncRuntimeGateState {
    int requested;
    int active;
} AdoAsyncRuntimeGateState;

typedef struct AdoAsyncRuntimeGateTransition {
    int active;
    int active_changed;
} AdoAsyncRuntimeGateTransition;

static inline AdoAsyncRuntimeGateTransition ado_async_runtime_gate_apply(
    AdoAsyncRuntimeGateState *state,
    int requested,
    int gate_open) {
    AdoAsyncRuntimeGateTransition transition = {0, 0};
    if (state == 0) {
        return transition;
    }
    int next_requested = requested ? 1 : 0;
    int next_active = next_requested && gate_open ? 1 : 0;
    transition.active = next_active;
    transition.active_changed = state->active != next_active;
    state->requested = next_requested;
    state->active = next_active;
    return transition;
}

static inline AdoAsyncRuntimeGateTransition ado_async_runtime_gate_set_requested(
    AdoAsyncRuntimeGateState *state,
    int requested,
    int gate_open) {
    return ado_async_runtime_gate_apply(state, requested, gate_open);
}

static inline AdoAsyncRuntimeGateTransition ado_async_runtime_gate_observe(
    AdoAsyncRuntimeGateState *state,
    int gate_open) {
    if (state == 0) {
        AdoAsyncRuntimeGateTransition transition = {0, 0};
        return transition;
    }
    return ado_async_runtime_gate_apply(state, state->requested, gate_open);
}

#endif
