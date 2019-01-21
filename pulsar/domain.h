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

#pragma once

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "audio.h"
#include "system.h"

namespace pulsar {

namespace node {

namespace base {

struct node;

} // namespace base

} // namespace node

struct domain : public std::enable_shared_from_this<domain> {
    using mutex_type = std::mutex;
    using lock_type = std::unique_lock<mutex_type>;

    private:
    std::shared_ptr<audio::buffer> zero_buffer = std::make_shared<audio::buffer>();
    std::vector<node::base::node *> nodes;
    std::list<node::base::node *> run_queue;
    mutex_type run_queue_mutex;
    std::condition_variable run_queue_condition;
    std::vector<std::thread> threads;
    std::condition_variable step_done_condition;
    mutex_type step_done_mutex;
    bool activated = false;
    bool step_done_flag = false;
    lock_type make_step_done_lock();
    lock_type make_run_queue_lock();
    static void be_thread(domain * domain_in);

    public:
    const string_type name;
    const pulsar::size_type sample_rate;
    const pulsar::size_type buffer_size;
    template <typename... Args>
    static std::shared_ptr<domain> make(Args&&... args)
    {
        return std::make_shared<domain>(args...);
    }
    domain(const string_type& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in);
    virtual ~domain();
    std::shared_ptr<audio::buffer> get_zero_buffer();
    void activate(const size_type num_threads_in = 1);
    void step();
    void add_ready_node(node::base::node * node_in);
    template<class T, typename... Args>
    T * make_node(Args&&... args)
    {
        auto new_node = new T(args..., this->shared_from_this());

        if (activated) {
            new_node->activate();
        }

        nodes.push_back(new_node);

        return new_node;
    }
};

} // namespace pulsar
