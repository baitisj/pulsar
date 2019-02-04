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

#include <memory>
#include <string>

#include <pulsar/config.h>
#include <pulsar/node.forward.h>
#include <pulsar/system.h>

namespace pulsar {

namespace property {

enum class value_type { unknown, null, size, integer, real, string, plist, pmap };

struct generic;

using plist_type = std::vector<generic>;
using pmap_type = std::map<string_type, std::shared_ptr<generic>>;

union value_container {
    size_type size;
    integer_type integer;
    real_type real;
    string_type * string;
    plist_type * plist;
    pmap_type * pmap;
};

class generic : public std::enable_shared_from_this<generic> {
    generic(const generic&) = delete;

    protected:
    value_container value;
    node::base * parent;

    public:
    const string_type name;
    const value_type type = value_type::unknown;
    generic(node::base * parent_in, const string_type& name_in, const value_type type_in);
    virtual ~generic();
    operator bool();
    string_type get();
    void set(const double& value_in);
    void set(const string_type& value_in);
    void set(const YAML::Node& value_in);
    node::base * get_parent();
    size_type& get_size();
    void set_size(const size_type& size_in);
    integer_type& get_integer();
    void set_integer(const integer_type& integer_in);
    void set_real(const real_type& real_in);
    real_type& get_real();
    string_type& get_string();
    void set_string(const string_type& string_in);
    const plist_type& get_plist();
    pmap_type& get_pmap();
    std::shared_ptr<generic> lookup(const string_type& key_in);
    std::shared_ptr<generic> add(const string_type& name_in, const value_type type_in);
    std::shared_ptr<generic> add(const string_type& name_in, std::shared_ptr<generic> property_in);
    pmap_type::iterator begin();
    pmap_type::iterator end();
};

} // namespace property

} // namespace pulsar
