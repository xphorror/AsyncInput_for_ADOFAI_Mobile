#ifndef ADOFAI_ASYNC_INGRESS_QUEUE_H
#define ADOFAI_ASYNC_INGRESS_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#define ADO_ASYNC_INGRESS_EVENT_CAPACITY 512
#define ADO_ASYNC_INGRESS_CONTROL_CAPACITY 16

typedef enum AdoAsyncIngressKind {
    ADO_ASYNC_INGRESS_EVENT = 1,
    ADO_ASYNC_INGRESS_RESET = 2,
    ADO_ASYNC_INGRESS_SOFT_PAUSE = 3,
    ADO_ASYNC_INGRESS_SOFT_RESUME = 4
} AdoAsyncIngressKind;

typedef struct AdoAsyncIngressRecord {
    int kind;
    int event_type;
    int source_id;
    uint64_t raw_ns;
    uint64_t seq;
} AdoAsyncIngressRecord;

typedef enum AdoAsyncIngressPushResult {
    ADO_ASYNC_INGRESS_REJECTED = 0,
    ADO_ASYNC_INGRESS_PUSHED = 1
} AdoAsyncIngressPushResult;

typedef struct AdoAsyncIngressQueue {
    AdoAsyncIngressRecord events[ADO_ASYNC_INGRESS_EVENT_CAPACITY];
    int event_head;
    int event_count;
    AdoAsyncIngressRecord controls[ADO_ASYNC_INGRESS_CONTROL_CAPACITY];
    int control_head;
    int control_count;
    uint64_t dropped_event_count;
} AdoAsyncIngressQueue;

static inline int ado_async_ingress_push_event(
    AdoAsyncIngressQueue *queue,
    AdoAsyncIngressRecord record,
    AdoAsyncIngressRecord *dropped_out) {
    if (queue == NULL) {
        return ADO_ASYNC_INGRESS_REJECTED;
    }
    if (queue->event_count == ADO_ASYNC_INGRESS_EVENT_CAPACITY) {
        if (dropped_out != NULL) {
            *dropped_out = queue->events[queue->event_head];
        }
        queue->event_head =
            (queue->event_head + 1) % ADO_ASYNC_INGRESS_EVENT_CAPACITY;
        queue->event_count--;
        queue->dropped_event_count++;
    }
    int index = (queue->event_head + queue->event_count) %
        ADO_ASYNC_INGRESS_EVENT_CAPACITY;
    queue->events[index] = record;
    queue->event_count++;
    return ADO_ASYNC_INGRESS_PUSHED;
}

static inline int ado_async_ingress_push_control(
    AdoAsyncIngressQueue *queue,
    AdoAsyncIngressRecord record) {
    if (queue == NULL ||
        queue->control_count == ADO_ASYNC_INGRESS_CONTROL_CAPACITY) {
        return ADO_ASYNC_INGRESS_REJECTED;
    }
    int index = (queue->control_head + queue->control_count) %
        ADO_ASYNC_INGRESS_CONTROL_CAPACITY;
    queue->controls[index] = record;
    queue->control_count++;
    return ADO_ASYNC_INGRESS_PUSHED;
}

static inline int ado_async_ingress_pop(
    AdoAsyncIngressQueue *queue,
    AdoAsyncIngressRecord *out) {
    if (queue == NULL || out == NULL ||
        (queue->event_count == 0 && queue->control_count == 0)) {
        return 0;
    }

    int use_control = queue->event_count == 0;
    if (!use_control && queue->control_count != 0) {
        const AdoAsyncIngressRecord *event =
            &queue->events[queue->event_head];
        const AdoAsyncIngressRecord *control =
            &queue->controls[queue->control_head];
        use_control = control->seq < event->seq;
    }

    if (use_control) {
        *out = queue->controls[queue->control_head];
        queue->control_head =
            (queue->control_head + 1) % ADO_ASYNC_INGRESS_CONTROL_CAPACITY;
        queue->control_count--;
    } else {
        *out = queue->events[queue->event_head];
        queue->event_head =
            (queue->event_head + 1) % ADO_ASYNC_INGRESS_EVENT_CAPACITY;
        queue->event_count--;
    }
    return 1;
}

static inline int ado_async_ingress_count(const AdoAsyncIngressQueue *queue) {
    return queue == NULL ? 0 : queue->event_count + queue->control_count;
}

static inline int ado_async_ingress_control_has_capacity(
    const AdoAsyncIngressQueue *queue) {
    return queue != NULL &&
        queue->control_count < ADO_ASYNC_INGRESS_CONTROL_CAPACITY;
}

#endif
