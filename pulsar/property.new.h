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

#include <string>

#include <boost/variant.hpp>

#include <pulsar/property.h>


#include <pulsar/config.h>
#include <pulsar/node.forward.h>
#include <pulsar/system.h>
#include <pulsar/types.h>

namespace pulsar {

namespace property {

using variant_type = boost::variant<size_type, string_type>;

struct storage {
    struct size_converter : boost::static_visitor<size_type> {
        size_type operator() (const size_type& value_in) const
        { return value_in; }

        size_type operator() (const string_type& value_in) const
        { return std::strtoul(value_in.c_str(), nullptr, 0); }
    };

    struct string_converter : boost::static_visitor<string_type> {
        string_type operator() (const size_type& value_in) const
        { return std::to_string(value_in); }

        string_type operator() (const string_type& value_in) const
        { return value_in; }
    };

    variant_type value;

    string_type get();
    void set(const double& value_in);
    void set(const string_type& value_in);
};

} // namespace property

} // namespace pulsar
