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

#include <cassert>

#include "config.h"
#include "domain.h"
#include "library.h"
#include "logging.h"
#include "system.h"
#include "util.h"

namespace pulsar {

namespace config {

std::shared_ptr<pulsar::domain> make_domain(std::shared_ptr<pulsar::config::domain> domain_info_in)
{
    auto domain_config = domain_info_in->get_config();

    assert(domain_config["sample_rate"]);
    assert(domain_config["buffer_size"]);

    auto domain_sample_rate = domain_config["sample_rate"].as<pulsar::size_type>();
    auto domain_buffer_size = domain_config["buffer_size"].as<pulsar::size_type>();

    auto domain = pulsar::domain::make(domain_info_in->name, domain_sample_rate, domain_buffer_size);
    return domain;
}

static void apply_node_template(YAML::Node& dest_in, const YAML::Node& src_in)
{
    if (dest_in.Type() != src_in.Type()) {
        system_fault("YAML node type difference during template import");
    }

    if (src_in.IsMap()) {
        for(auto&& i : src_in) {
            auto key_name = i.first.as<std::string>();

            if (dest_in[key_name]) {
                if (src_in[key_name].IsSequence() || src_in[key_name].IsMap()) {
                    // FIXME is that how this is supposed to work?
                    auto new_node = dest_in[key_name];
                    apply_node_template(new_node, src_in[key_name]);
                }
            } else {
              dest_in[key_name] = i.second;
            }
        }
    } else if (src_in.IsSequence()) {
        // FIXME does it make sense to try to merge a sequence?
        // Does not seem so
        // auto size = src_in.size();
        // for(size_type i = 0; i < size; i++) {
        //     if (dest_in[i]) {
        //         if (src_in[i].IsSequence() || src_in[i].IsMap()) {
        //             // FIXME is that how this is supposed to work?
        //             auto new_node = dest_in[i];
        //             apply_node_template(new_node, src_in[i]);
        //         }
        //     } else {
        //         dest_in[i] = src_in[i];
        //     }
        // }
    }
}

std::map<std::string, pulsar::node::base::node *> make_nodes(std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in) {
    auto node_map = std::map<std::string, pulsar::node::base::node *>();

    for (YAML::Node node_yaml : config_in->get_nodes()) {
        auto template_node = node_yaml["template"];
        if (template_node) {
            assert(template_node.IsScalar());
            auto template_name = template_node.as<std::string>();
            auto template_src = config_in->get_parent()->get_template(template_name);
            apply_node_template(node_yaml, template_src);
        }

        auto node_name = node_yaml["name"].as<std::string>();
        auto class_name = node_yaml["class"].as<std::string>();
        auto node_config = node_yaml["config"];
        auto node_plugin = node_yaml["plugin"];
        auto node_inputs = node_yaml["inputs"];
        auto node_outputs = node_yaml["outputs"];
        auto new_node = pulsar::library::make_node(class_name, node_name, domain_in);

        if (node_plugin) {
            for(auto&& i : node_plugin) {
                auto config_name = i.first.as<std::string>();
                auto config_node = i.second;
                auto property_name = std::string("plugin:") + config_name;
                new_node->get_property(property_name).set(config_node);
            }
        }

        new_node->init();

        if (node_config) {
            for(auto&& i : node_config) {
                auto config_name = i.first.as<std::string>();
                auto config_node = i.second;
                auto property_name = std::string("config:") + config_name;
                new_node->get_property(property_name).set(config_node);
            }
        }

        if (node_inputs) {
            for(auto&& i : node_inputs) {
                auto input_name = i.as<std::string>();
                new_node->audio.add_input(input_name);
            }
        }

        if (node_outputs) {
            for(auto&& i : node_outputs) {
                auto output_name = i.as<std::string>();
                new_node->audio.add_output(output_name);
            }
        }

        auto found = node_map.find(node_name);
        if (found != node_map.end()) {
            system_fault("duplicate node name: ", node_name);
        }
        node_map[node_name] = new_node;
    }

    for (auto&& node_yaml : config_in->get_nodes()) {
        auto node_name = node_yaml["name"].as<std::string>();
        auto source_node = node_map[node_name];
        auto connections = node_yaml["connect"];

        if (! connections) {
            continue;
        }

        // if the number of inputs and outputs is the same
        // connect them together in order
        if (connections.IsScalar()) {
            auto output_names = source_node->audio.get_output_names();
            auto sink_node_name = connections.as<std::string>();
            auto sink_node = node_map[sink_node_name];
            auto input_names = sink_node->audio.get_input_names();

            auto num_outputs = output_names.size();
            if (input_names.size() != num_outputs) {
                system_fault("number of inputs and outputs was not the same");
            }

            for(size_type i = 0; i < num_outputs; i++) {
                auto sink_channel = sink_node->audio.get_input(input_names[i]);
                source_node->audio.get_output(output_names[i])->connect(sink_channel);
            }
        } else {
            for(auto&& i : connections) {
                auto source_channel = i.first.as<std::string>();
                auto target_node = i.second;

                if (target_node.IsSequence()) {
                    system_fault("can't handle a sequence yet");
                }

                auto target_string = i.second.as<std::string>();
                auto target_split = util::string::split(target_string, ':');
                auto split_size = target_split.size();
                std::string sink_node_name, sink_channel_name;
                node::base::node * sink_node;

                if (split_size == 2) {
                    sink_node_name = target_split[0];
                    sink_node = node_map[sink_node_name];
                    sink_channel_name = target_split[1];
                } else if (split_size == 1) {
                    sink_node_name = target_string;
                    sink_node = node_map[sink_node_name];
                    auto sink_node_inputs = sink_node->audio.get_input_names();

                    if (sink_node_inputs.size() != 1) {
                        system_fault("no input channel was specified and target node has more than 1 input");
                    }

                    sink_channel_name = sink_node_inputs[0];
                } else {
                    system_fault("invalid connection string specified: ", target_string);
                }

                if (sink_channel_name == "*") {
                    for (auto&& channel_name : sink_node->audio.get_input_names()) {
                        auto sink_channel = sink_node->audio.get_input(channel_name);
                        source_node->audio.get_output(source_channel)->connect(sink_channel);
                    }
                } else {
                    auto sink_channel = sink_node->audio.get_input(sink_channel_name);
                    source_node->audio.get_output(source_channel)->connect(sink_channel);
                }
            }

        }
    }

    return node_map;
}

file::file(const std::string& path_in)
: path(path_in)
{ }

void file::open()
{
    yaml_root = YAML::LoadFile(path);
    parse();
}

void file::parse()
{
    if (yaml_root["domain"] && yaml_root["domains"]) {
        system_fault("config file must have one domain or one domains section but not both");
    } else if (! yaml_root["domain"] && ! yaml_root["domains"]) {
        system_fault("config file must have a domain or a domains section");
    }
}

std::vector<std::string> file::get_domain_names()
{
    std::vector<std::string> retval;


    if (yaml_root["domain"]) {
        retval.push_back("main");
    } else if (yaml_root["domains"]) {
        system_fault("can't yet handle a domains section");
    }

    return retval;
}

std::shared_ptr<domain> file::get_domain(const std::string& name_in)
{
    if (name_in == "main" && yaml_root["domain"]) {
        return domain::make(name_in, yaml_root["domain"], this->shared_from_this());
    }

    auto domains = yaml_root["domains"];
    if (! domains[name_in]) {
        system_fault("could not find a domain named ", name_in);
    }

    return domain::make(name_in, domains[name_in], this->shared_from_this());
}

YAML::Node file::get_template(const std::string& name_in)
{
    auto templates_node = yaml_root["templates"];
    if (! templates_node) {
        system_fault("there was not a template section in the config file");
    }

    auto template_node = templates_node[name_in];
    if (! template_node) {
        system_fault("there was no node template named ", name_in);
    }

    if (! template_node.IsMap()) {
        system_fault("template must be a map: ", name_in);
    }

    return template_node;
}

domain::domain(const std::string name_in, const YAML::Node& yaml_in, std::shared_ptr<file> parent_in)
: yaml_root(yaml_in), parent(parent_in), name(name_in)
{ }

std::shared_ptr<file> domain::get_parent()
{
    return parent;
}

const YAML::Node domain::get_config()
{
    assert(yaml_root["config"]);
    return yaml_root["config"];
}

const YAML::Node domain::get_nodes()
{
    assert(yaml_root["nodes"]);
    return yaml_root["nodes"];
}

} // namespace configfile

} // namespace pulsar
