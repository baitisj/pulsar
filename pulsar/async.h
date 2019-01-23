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
#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>
#include <chrono>
#include <functional>
#include <memory>

#include <pulsar/system.h>
#include <pulsar/thread.h>

#define PULSAR_WATCHDOG_DEFAULT_MESSAGE "watchdog hit timeout"

namespace pulsar {

namespace async {

using boost_timer_type = boost::asio::system_timer;

void init();
bool is_online();

struct base_timer {
    using handler_type = std::function<void (base_timer&)>;

    protected:
    mutex_type mutex;
    duration_type initial;
    duration_type repeat;
    boost_timer_type boost_timer;
    virtual void boost_handler(const boost::system::error_code& error_in);
    virtual void run() = 0;

    public:
    base_timer(const duration_type& initial_in, const duration_type& repeat_in = duration_type(0));
    virtual ~base_timer();
    virtual void start();
    virtual void reset();
    virtual void stop();
};

class timer : public base_timer, public std::enable_shared_from_this<timer> {

    protected:
    virtual void run() override;
    std::vector<handler_type> watchers;

    public:
    timer(const duration_type& initial_in, const duration_type& repeat_in = duration_type(0));
    timer(const duration_type& initial_in, const duration_type& repeat_in, handler_type handler_in);
    timer(const duration_type& initial_in, handler_type handler_in);
    template <typename... Args>
    static std::shared_ptr<timer> make(Args&&... args)
    {
        return std::make_shared<timer>(args...);
    }
    virtual void watch(handler_type handler_in);
};

class watchdog : public base_timer, public std::enable_shared_from_this<watchdog> {
    protected:
    virtual void run() override;

    public:
    const std::string message = PULSAR_WATCHDOG_DEFAULT_MESSAGE;
    watchdog(const duration_type& timeout_in);
    watchdog(const duration_type& timeout_in, const std::string& message_in);

    template <typename... Args>
    static std::shared_ptr<watchdog> make(Args&&... args)
    {
        return std::make_shared<watchdog>(args...);
    }
};

} // namespace async

} // namespace pulsar
