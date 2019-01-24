// Pulsar Audio Engine
// Copyright 2019 Tyler Riddle <kg7oem@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

#include <pthread.h>

#include <pulsar/system.h>
#include <pulsar/thread.h>
#include <pulsar/types.h>

namespace pulsar {

namespace thread {

// from https://stackoverflow.com/a/31652324
void set_realtime_priority(thread_type& thread_in, const size_type priority_in)
{
    sched_param sch_params;
    sch_params.sched_priority = priority_in;

    if (pthread_setschedparam(thread_in.native_handle(), SCHED_RR, &sch_params)) {
        system_fault("could not set realtime priority for thread");
    }
}

} // namespace thread

} // namespace pulsar