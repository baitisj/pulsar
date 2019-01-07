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

#include <memory>
#include <iostream>

#include "domain.h"
#include "jackaudio.h"
#include "ladspa.h"

using namespace std;
using namespace std::chrono_literals;

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024
#define NUM_THREADS 4

int main(void)
{
    auto domain = make_shared<pulsar::domain>("main", SAMPLE_RATE, BUFFER_SIZE);
    auto instance = pulsar::ladspa::make_instance("/usr/lib/ladspa/amp.so", 1048, SAMPLE_RATE);
    auto node = domain->make_node<pulsar::ladspa::node>("ladspa", instance);
    auto node2 = domain->make_node<pulsar::ladspa::node>("ladspa2", "/usr/lib/ladspa/amp.so", 1048);
}
