#pragma once
#include "defines.h"

typedef struct native_clock {
    f64 start_time;
    f64 elapsed;
} native_clock;

// Updates the provided clock. Should be called just before checking elapsed time.
// Has no effect on non-started clocks.
YAPI void clock_update(native_clock* clock);

// Starts the provided clock. Resets elapsed time.
YAPI void clock_start(native_clock* clock);

// Stops the provided clock. Does not reset elapsed time.
YAPI void clock_stop(native_clock* clock);
