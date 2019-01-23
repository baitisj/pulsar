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

#include <boost/bind.hpp>
#include <exception>
#include <thread>
#include <vector>

#include <pulsar/async.h>
#include <pulsar/logging.h>
#include <pulsar/system.h>

#define NUM_ASYNC_THREADS 4

namespace pulsar {

namespace async {

using namespace std::chrono_literals;

static boost::asio::io_service boost_io;
static std::vector<thread_type> async_threads;
static std::atomic<bool> is_online_flag = ATOMIC_VAR_INIT(false);

bool is_online()
{
    return is_online_flag.load();
}

static void async_thread()
{
    boost_io.run();
    system_fault("Boost io.run() returned");
}

void init()
{
    assert(! is_online_flag.load());

    size_type num_threads_in = NUM_ASYNC_THREADS;

    for(size_type i = 0; i < num_threads_in; i++) {
        async_threads.emplace_back(async_thread);
    }

    is_online_flag.store(true);
}

base_timer::base_timer(const duration_type& initial_in, const duration_type& repeat_in)
: initial(initial_in), repeat(repeat_in), boost_timer(boost_io)
{ }

base_timer::~base_timer()
{
    stop();
}

void base_timer::boost_handler(const boost::system::error_code& error_in)
{
    log_trace("timer boost handler just started");

    lock_type lock(mutex);

    if (error_in) {
        if (error_in == boost::asio::error::operation_aborted) {
            log_trace("boost indicated the timer was cancelled");
            return;
        }

        system_fault("there was an error");
    }

    log_trace("invoking timer run() method");
    run();
    log_trace("done running timer run() method");

    if (repeat == 0ms) {
        return;
    }

    auto next_time = boost_timer.expires_at() + repeat;
    boost_timer.expires_at(next_time);
    auto bound = boost::bind(&timer::boost_handler, this, boost::asio::placeholders::error);
    boost_timer.async_wait(bound);
}

void base_timer::start()
{
    lock_type lock(mutex);

    boost_timer.expires_at(std::chrono::system_clock::now() + initial);
    auto bound = boost::bind(&timer::boost_handler, this, boost::asio::placeholders::error);
    boost_timer.async_wait(bound);
}

void base_timer::reset()
{
    lock_type lock(mutex);

    auto when = std::chrono::system_clock::now() + repeat;
    boost_timer.expires_at(when);
    base_timer::start();
}

void base_timer::stop()
{
    lock_type lock(mutex);

    boost_timer.cancel();
}

timer::timer(const duration_type& initial_in, const duration_type& repeat_in)
: base_timer(initial_in, repeat_in)
{ }

timer::timer(const duration_type& initial_in, const duration_type& repeat_in, handler_type handler_in)
: base_timer(initial_in, repeat_in)
{
    watchers.push_back(handler_in);
}

timer::timer(const duration_type& initial_in, handler_type handler_in)
: base_timer(initial_in)
{
    watchers.push_back(handler_in);
}

void timer::watch(timer::handler_type handler_in)
{
    lock_type lock(mutex);
    watchers.push_back(handler_in);
}

void timer::run()
{
    for(auto&& cb : watchers) {
        cb(*this);
    }
}

watchdog::watchdog(const duration_type& timeout_in)
: base_timer(timeout_in, timeout_in)
{ }

watchdog::watchdog(const duration_type& timeout_in, const std::string& message_in)
: base_timer(timeout_in, timeout_in), message(message_in)
{ }

void watchdog::run()
{
    system_fault(message);
}

} // namespace async

} // namespace pulsar
