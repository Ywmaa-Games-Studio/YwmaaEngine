#include "core/event.h"

#include "core/ymemory.h"
#include "variants/darray.h"

typedef struct REGISTERED_EVENT {
    void* listener;
    PFN_on_event callback;
} REGISTERED_EVENT;

typedef struct EVENT_CODE_ENTRY {
    REGISTERED_EVENT* events;
} EVENT_CODE_ENTRY;

// This should be more than enough codes...
#define MAX_MESSAGE_CODES 16384

// State structure.
typedef struct EVENT_SYSTEM_STATE {
    // Lookup table for event codes.
    EVENT_CODE_ENTRY registered[MAX_MESSAGE_CODES];
} EVENT_SYSTEM_STATE;

/**
 * Event system internal state_ptr->
 */
static EVENT_SYSTEM_STATE* state_ptr;

void event_system_init(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(EVENT_SYSTEM_STATE);
    if (state == 0) {
        return;
    }
    yzero_memory(state, sizeof(state));
    state_ptr = state;
}

void event_system_shutdown(void* state) {
    if (state_ptr) {
        // Free the events arrays. And objects pointed to should be destroyed on their own.
        for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {
            if (state_ptr->registered[i].events != 0) {
                darray_destroy(state_ptr->registered[i].events);
                state_ptr->registered[i].events = 0;
            }
        }
    }
    state_ptr = 0;
}

b8 event_register(u16 code, void* listener, PFN_on_event on_event) {
    if(!state_ptr) {
        return false;
    }

    if(state_ptr->registered[code].events == 0) {
        state_ptr->registered[code].events = darray_create(REGISTERED_EVENT);
    }

    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        if(state_ptr->registered[code].events[i].listener == listener) {
            // TODO: warn
            return false;
        }
    }

    // If at this point, no duplicate was found. Proceed with registration.
    REGISTERED_EVENT event;
    event.listener = listener;
    event.callback = on_event;
    darray_push(state_ptr->registered[code].events, event);

    return true;
}

b8 event_unregister(u16 code, void* listener, PFN_on_event on_event) {
    if(!state_ptr) {
        return false;
    }

    // On nothing is registered for the code, boot out.
    if(state_ptr->registered[code].events == 0) {
        // TODO: warn
        return false;
    }

    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        REGISTERED_EVENT e = state_ptr->registered[code].events[i];
        if(e.listener == listener && e.callback == on_event) {
            // Found one, remove it
            REGISTERED_EVENT popped_event;
            darray_pop_at(state_ptr->registered[code].events, i, &popped_event);
            return true;
        }
    }

    // Not found.
    return false;
}

b8 event_fire(u16 code, void* sender, EVENT_CONTEXT context) {
    if(!state_ptr) {
        return false;
    }

    // If nothing is registered for the code, boot out.
    if(state_ptr->registered[code].events == 0) {
        return false;
    }

    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        REGISTERED_EVENT e = state_ptr->registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Message has been handled, do not send to other listeners.
            return true;
        }
    }

    // Not found.
    return false;
}