#include <stdint.h>
#include <stdio.h>

#include "../include/async_auto_replay.h"
#include "../include/async_capture_gate.h"
#include "../include/async_ingress_queue.h"
#include "../include/async_runtime_gate.h"

static int g_failures;

#define EXPECT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s\n", message); \
            g_failures++; \
        } \
    } while (0)

static AdoAsyncIngressRecord make_record(int kind, uint64_t seq) {
    AdoAsyncIngressRecord record = {0};
    record.kind = kind;
    record.seq = seq;
    return record;
}

static void test_control_survives_event_overflow(void) {
    AdoAsyncIngressQueue queue = {0};
    AdoAsyncIngressRecord dropped = {0};
    AdoAsyncIngressRecord out = {0};

    EXPECT_TRUE(
        ado_async_ingress_push_event(
            &queue, make_record(ADO_ASYNC_INGRESS_EVENT, 1), &dropped) ==
            ADO_ASYNC_INGRESS_PUSHED,
        "first event should be queued");
    EXPECT_TRUE(
        ado_async_ingress_push_control(
            &queue, make_record(ADO_ASYNC_INGRESS_RESET, 2)) ==
            ADO_ASYNC_INGRESS_PUSHED,
        "reset should be queued independently");

    for (uint64_t seq = 3;
         seq <= ADO_ASYNC_INGRESS_EVENT_CAPACITY + 2;
         ++seq) {
        (void)ado_async_ingress_push_event(
            &queue,
            make_record(ADO_ASYNC_INGRESS_EVENT, seq),
            &dropped);
    }

    EXPECT_TRUE(queue.dropped_event_count == 1,
                "event overflow should drop exactly one event");
    EXPECT_TRUE(dropped.seq == 1,
                "event overflow should only drop the oldest event");
    EXPECT_TRUE(ado_async_ingress_pop(&queue, &out) == 1,
                "queue should contain the reset");
    EXPECT_TRUE(out.kind == ADO_ASYNC_INGRESS_RESET && out.seq == 2,
                "reset must survive and retain global sequence order");

    uint64_t previous_seq = out.seq;
    while (ado_async_ingress_pop(&queue, &out)) {
        EXPECT_TRUE(out.kind == ADO_ASYNC_INGRESS_EVENT,
                    "only events should remain after reset");
        EXPECT_TRUE(out.seq > previous_seq,
                    "merged queues must preserve global sequence order");
        previous_seq = out.seq;
    }
}

static void test_control_overflow_is_explicit(void) {
    AdoAsyncIngressQueue queue = {0};
    AdoAsyncIngressRecord out = {0};

    for (uint64_t seq = 1;
         seq <= ADO_ASYNC_INGRESS_CONTROL_CAPACITY;
         ++seq) {
        EXPECT_TRUE(
            ado_async_ingress_push_control(
                &queue, make_record(ADO_ASYNC_INGRESS_SOFT_PAUSE, seq)) ==
                ADO_ASYNC_INGRESS_PUSHED,
            "control queue should accept its advertised capacity");
    }
    EXPECT_TRUE(
        ado_async_ingress_push_control(
            &queue,
            make_record(
                ADO_ASYNC_INGRESS_SOFT_RESUME,
                ADO_ASYNC_INGRESS_CONTROL_CAPACITY + 1)) ==
            ADO_ASYNC_INGRESS_REJECTED,
        "full control queue must reject instead of dropping a command");

    EXPECT_TRUE(ado_async_ingress_pop(&queue, &out) == 1,
                "one control should be available for processing");
    EXPECT_TRUE(
        ado_async_ingress_push_control(
            &queue,
            make_record(
                ADO_ASYNC_INGRESS_SOFT_RESUME,
                ADO_ASYNC_INGRESS_CONTROL_CAPACITY + 1)) ==
            ADO_ASYNC_INGRESS_PUSHED,
        "rejected command should be retryable with the same sequence");
}

static void test_runtime_gate_recovers_requested_state(void) {
    AdoAsyncRuntimeGateState state = {0};
    AdoAsyncRuntimeGateTransition transition;

    transition = ado_async_runtime_gate_set_requested(&state, 1, 1);
    EXPECT_TRUE(state.requested == 1 && state.active == 1,
                "enabled request should activate while the gate is valid");
    EXPECT_TRUE(transition.active_changed == 1 && transition.active == 1,
                "activation should be observable");

    transition = ado_async_runtime_gate_observe(&state, 0);
    EXPECT_TRUE(state.requested == 1 && state.active == 0,
                "gate failure must suspend without erasing user intent");
    EXPECT_TRUE(transition.active_changed == 1 && transition.active == 0,
                "gate failure should immediately deactivate runtime input");

    transition = ado_async_runtime_gate_observe(&state, 0);
    EXPECT_TRUE(transition.active_changed == 0 && state.active == 0,
                "continued gate failure should remain fail-closed");

    transition = ado_async_runtime_gate_observe(&state, 1);
    EXPECT_TRUE(state.requested == 1 && state.active == 1,
                "gate recovery should restore a still-requested runtime");
    EXPECT_TRUE(transition.active_changed == 1 && transition.active == 1,
                "gate recovery should publish one activation transition");
}

static void test_runtime_gate_activates_after_delayed_readiness(void) {
    AdoAsyncRuntimeGateState state = {0};

    AdoAsyncRuntimeGateTransition transition =
        ado_async_runtime_gate_set_requested(&state, 1, 0);
    EXPECT_TRUE(state.requested == 1 && state.active == 0,
                "blocked startup should retain the enabled request");
    EXPECT_TRUE(transition.active_changed == 0,
                "blocked startup must remain fail-closed");

    transition = ado_async_runtime_gate_observe(&state, 1);
    EXPECT_TRUE(state.requested == 1 && state.active == 1,
                "delayed gate readiness should activate retained intent");
    EXPECT_TRUE(transition.active_changed == 1 && transition.active == 1,
                "delayed readiness should publish one activation");
}

static void test_explicit_disable_does_not_auto_restore(void) {
    AdoAsyncRuntimeGateState state = {1, 1};

    (void)ado_async_runtime_gate_set_requested(&state, 0, 1);
    EXPECT_TRUE(state.requested == 0 && state.active == 0,
                "explicit disable should clear intent and active state");
    (void)ado_async_runtime_gate_observe(&state, 1);
    EXPECT_TRUE(state.requested == 0 && state.active == 0,
                "a valid gate must not override explicit disable");
}

static void test_capture_gate_rejects_transition_out_of_player_control(void) {
    const int player_control = 4;

    EXPECT_TRUE(
        ado_async_controller_allows_capture(
            1, player_control, 0, -1, player_control) == 1,
        "capture should tolerate an unavailable destination state");
    EXPECT_TRUE(
        ado_async_controller_allows_capture(
            1, player_control, 1, player_control, player_control) == 1,
        "capture should remain open in a stable PlayerControl state");
    EXPECT_TRUE(
        ado_async_controller_allows_capture(
            1, player_control, 1, 7, player_control) == 0,
        "capture must close when PlayerControl starts transitioning to Won");
    EXPECT_TRUE(
        ado_async_controller_allows_capture(
            1, player_control, 1, 5, player_control) == 0,
        "capture must close when PlayerControl starts transitioning to Fail");
    EXPECT_TRUE(
        ado_async_controller_allows_capture(
            1, 7, 1, 7, player_control) == 0,
        "capture must remain closed after the transition completes");
    EXPECT_TRUE(
        ado_async_controller_allows_capture(
            0, player_control, 1, player_control, player_control) == 0,
        "capture must close immediately when gameplay enters freeroam");
}

static void test_auto_replay_commits_exactly_once(void) {
    AdoAsyncAutoReplayTransaction transaction;
    ado_async_auto_replay_transaction_init(&transaction);

    EXPECT_TRUE(
        ado_async_auto_replay_try_begin_commit(&transaction) == 1,
        "first nested auto hit must own the replay commit");
    EXPECT_TRUE(
        ado_async_auto_replay_try_begin_commit(&transaction) == 0,
        "one auto replay transaction must not commit twice");

    ado_async_auto_replay_finish_commit(&transaction, 1);
    EXPECT_TRUE(
        ado_async_auto_replay_has_commit(&transaction) == 1,
        "completed original Hit(isAuto) must be visible to the outer hook");
    EXPECT_TRUE(
        transaction.commit_result == 1,
        "outer hook must propagate the original auto hit result");
}

static void test_auto_replay_preserves_official_auto_inside_transaction(void) {
    EXPECT_TRUE(
        ado_async_auto_replay_player_auto_value(0, 1, 0) == 0,
        "unowned synthetic auto replay must not recursively enable auto");
    EXPECT_TRUE(
        ado_async_auto_replay_player_auto_value(0, 1, 1) == 1,
        "owned auto transaction must reach the nested official auto path");
    EXPECT_TRUE(
        ado_async_auto_replay_player_auto_value(1, 0, 0) == 1,
        "ordinary official auto state must remain unchanged");
}

int main(void) {
    test_control_survives_event_overflow();
    test_control_overflow_is_explicit();
    test_runtime_gate_recovers_requested_state();
    test_runtime_gate_activates_after_delayed_readiness();
    test_explicit_disable_does_not_auto_restore();
    test_capture_gate_rejects_transition_out_of_player_control();
    test_auto_replay_commits_exactly_once();
    test_auto_replay_preserves_official_auto_inside_transaction();

    if (g_failures != 0) {
        fprintf(stderr, "async input core tests failed: %d\n", g_failures);
        return 1;
    }
    puts("async input core tests passed");
    return 0;
}
