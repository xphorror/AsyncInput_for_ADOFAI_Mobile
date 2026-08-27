#ifndef ADOFAI_ASYNC_AUTO_REPLAY_H
#define ADOFAI_ASYNC_AUTO_REPLAY_H

typedef struct AdoAsyncAutoReplayTransaction {
    int commit_started;
    int commit_finished;
    int commit_result;
} AdoAsyncAutoReplayTransaction;

static inline void ado_async_auto_replay_transaction_init(
    AdoAsyncAutoReplayTransaction *transaction) {
    if (transaction == 0) {
        return;
    }
    transaction->commit_started = 0;
    transaction->commit_finished = 0;
    transaction->commit_result = 0;
}

static inline int ado_async_auto_replay_try_begin_commit(
    AdoAsyncAutoReplayTransaction *transaction) {
    if (transaction == 0 || transaction->commit_started) {
        return 0;
    }
    transaction->commit_started = 1;
    return 1;
}

static inline void ado_async_auto_replay_finish_commit(
    AdoAsyncAutoReplayTransaction *transaction,
    int result) {
    if (transaction == 0 || !transaction->commit_started) {
        return;
    }
    transaction->commit_result = result;
    transaction->commit_finished = 1;
}

static inline int ado_async_auto_replay_has_commit(
    const AdoAsyncAutoReplayTransaction *transaction) {
    return transaction != 0 && transaction->commit_finished;
}

static inline int ado_async_auto_replay_player_auto_value(
    int original_value,
    int synthetic_auto_replay,
    int auto_transaction_active) {
    if (!synthetic_auto_replay) {
        return original_value ? 1 : 0;
    }
    return auto_transaction_active ? 1 : 0;
}

#endif
