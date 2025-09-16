#ifndef EVENT_H_
#define EVENT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_DEFAULT_MAX_EVENTS 256

typedef enum {
    EVENT_SUCCESS = 0,
    EVENT_ERROR_INVALID_ARGUMENT = -1,
    EVENT_ERROR_OUT_OF_MEMORY = -2,
    EVENT_ERROR_NOT_FOUND = -3
} event_result_t;

typedef struct {
    const char* name;
    void* data;
    size_t data_size;
    uint64_t timestamp;
    uint32_t event_hash;
} event_t;

typedef void (*event_callback_t)(const event_t* event, void* user_data);

typedef struct event_subscriber {
    event_callback_t callback;
    void* user_data;
    struct event_subscriber* next;
} event_subscriber_t;

typedef struct event_type {
    char* name;
    uint32_t hash;
    event_subscriber_t* subscribers;
    size_t subscriber_count;
} event_type_t;

typedef struct event_subscription {
    uint32_t event_hash;
    event_subscriber_t* subscriber;
} event_subscription_t;

typedef struct event_context {
    event_type_t* events;
    size_t event_count;
    size_t max_events;
} event_context_t;

static uint32_t event_hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static uint64_t event_get_timestamp() {
    return (uint64_t)time(NULL) * 1000;
}

static event_type_t* event_find_by_name(event_context_t* context, const char* name) {
    uint32_t hash = event_hash_string(name);
    for (size_t i = 0; i < context->event_count; i++) {
        if (context->events[i].hash == hash &&
            strcmp(context->events[i].name, name) == 0) {
            return &context->events[i];
        }
    }
    return NULL;
}

static event_type_t* event_find_by_hash(event_context_t* context, uint32_t hash) {
    for (size_t i = 0; i < context->event_count; i++) {
        if (context->events[i].hash == hash) {
            return &context->events[i];
        }
    }
    return NULL;
}

static event_result_t event_create_context(event_context_t** context, size_t max_events) {
    if (context == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }

    *context = (event_context_t*)malloc(sizeof(event_context_t));
    if (*context == NULL) {
        return EVENT_ERROR_OUT_OF_MEMORY;
    }

    (*context)->max_events = max_events > 0 ? max_events : EVENT_DEFAULT_MAX_EVENTS;
    (*context)->events = (event_type_t*)malloc(sizeof(event_type_t) * (*context)->max_events);
    if ((*context)->events == NULL) {
        free(*context);
        *context = NULL;
        return EVENT_ERROR_OUT_OF_MEMORY;
    }

    (*context)->event_count = 0;

    return EVENT_SUCCESS;
}

static event_result_t event_destroy_context(event_context_t* context) {
    if (context == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < context->event_count; i++) {
        event_type_t* event_type = &context->events[i];

        if (event_type->name != NULL) {
            free(event_type->name);
        }

        event_subscriber_t* subscriber = event_type->subscribers;
        while (subscriber != NULL) {
            event_subscriber_t* next = subscriber->next;
            free(subscriber);
            subscriber = next;
        }
    }

    free(context->events);
    free(context);

    return EVENT_SUCCESS;
}

static event_result_t event_subscribe(
    event_context_t* context,
    const char* event_name,
    event_callback_t callback,
    void* user_data
) {
    if (context == NULL || event_name == NULL || callback == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }

    event_type_t* event_type = event_find_by_name(context, event_name);
    if (event_type == NULL) {
        if (context->event_count >= context->max_events) {
            return EVENT_ERROR_OUT_OF_MEMORY;
        }

        event_type = &context->events[context->event_count];

        size_t name_len = strlen(event_name) + 1;
        event_type->name = (char*)malloc(name_len);
        if (event_type->name == NULL) {
            return EVENT_ERROR_OUT_OF_MEMORY;
        }
        strcpy(event_type->name, event_name);

        event_type->hash = event_hash_string(event_name);
        event_type->subscribers = NULL;
        event_type->subscriber_count = 0;

        context->event_count++;
    }

    event_subscriber_t* subscriber = (event_subscriber_t*)malloc(sizeof(event_subscriber_t));
    if (subscriber == NULL) {
        return EVENT_ERROR_OUT_OF_MEMORY;
    }

    subscriber->callback = callback;
    subscriber->user_data = user_data;
    subscriber->next = event_type->subscribers;
    event_type->subscribers = subscriber;
    event_type->subscriber_count++;

    return EVENT_SUCCESS;
}

static event_result_t event_dispatch(
    event_context_t* context,
    const char* event_name,
    const void* data,
    size_t data_size
) {
    if (context == NULL || event_name == NULL || (data == NULL && data_size > 0)) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }

    event_type_t* event_type = event_find_by_name(context, event_name);
    if (event_type == NULL) {
        return EVENT_ERROR_NOT_FOUND;
    }

    event_t event;
    event.name = event_type->name;
    event.event_hash = event_type->hash;
    event.timestamp = event_get_timestamp();

    void* event_data = NULL;
    if (data != NULL && data_size > 0) {
        event_data = malloc(data_size);
        if (event_data == NULL) {
            return EVENT_ERROR_OUT_OF_MEMORY;
        }
        memcpy(event_data, data, data_size);
    }

    event.data = event_data;
    event.data_size = data_size;

    event_subscriber_t* subscriber = event_type->subscribers;
    while (subscriber != NULL) {
        subscriber->callback(&event, subscriber->user_data);
        subscriber = subscriber->next;
    }

    free(event_data);

    return EVENT_SUCCESS;
}

static const char* event_error_string(event_result_t result) {
    switch (result) {
        case EVENT_SUCCESS:
            return "Success";
        case EVENT_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case EVENT_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case EVENT_ERROR_NOT_FOUND:
            return "Not found";
        default:
            return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif

#endif