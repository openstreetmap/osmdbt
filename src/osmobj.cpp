
#include "osmobj.hpp"
#include "db.hpp"
#include "exception.hpp"

#include <osmium/util/string.hpp>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <vector>

osmobj::osmobj(std::string const &obj, std::string const &version,
               std::string const &changeset, changeset_user_lookup *cucache)
{
    if (obj.size() < 2 || version.size() < 2 || changeset.size() < 2) {
        throw std::runtime_error{"Log file has wrong format: entry too short"};
    }

    m_type = osmium::char_to_item_type(obj[0]);
    if (m_type != osmium::item_type::node && m_type != osmium::item_type::way &&
        m_type != osmium::item_type::relation) {
        throw std::runtime_error{
            "Log file has wrong format: type must be 'n', 'w', or 'r'"};
    }
    m_id = std::strtoll(&obj[1], nullptr, 10);

    if (version[0] != 'v') {
        throw std::runtime_error{"Log file has wrong format: expected version"};
    }
    m_version = std::strtoll(&version[1], nullptr, 10);

    if (changeset[0] != 'c') {
        throw std::runtime_error{
            "Log file has wrong format: expected changeset"};
    }
    m_cid = std::strtoll(&changeset[1], nullptr, 10);

    if (cucache) {
        (*cucache)[m_cid] = {};
    }
}

void osmobj::get_data(pqxx::work &txn, osmium::memory::Buffer &buffer,
                      changeset_user_lookup const &cucache) const
{
    pqxx::result const result =
        txn.prepared(osmium::item_type_to_name(m_type))(m_id)(m_version).exec();

    assert(result.size() == 1);
    if (result.size() != 1) {
        throw database_error{"Expected exactly one result (get_data)."};
    }
    auto const &row = result[0];

    if (!row["redaction_id"].is_null()) {
        std::cerr << "Ignored redacted " << osmium::item_type_to_name(m_type)
                  << ' ' << m_id << " version " << m_version
                  << " (redaction_id=" << row["redaction_id"].c_str() << ")\n";
        return;
    }

    auto const cid = row["changeset_id"].as<osmium::changeset_id_type>();
    bool const visible = row["visible"].c_str()[0] == 't';
    auto const &user = cucache.at(cid);
    char const *const timestamp = row["timestamp"].c_str();

    switch (m_type) {
    case osmium::item_type::node: {
        osmium::builder::NodeBuilder builder{buffer};
        set_attributes(builder, cid, visible, user.id, timestamp);
        osmium::Location loc{row["longitude"].as<int64_t>(),
                             row["latitude"].as<int64_t>()};
        builder.set_location(loc).set_user(user.username);
        add_tags(txn, builder);
    } break;
    case osmium::item_type::way: {
        osmium::builder::WayBuilder builder{buffer};
        set_attributes(builder, cid, visible, user.id, timestamp);
        builder.set_user(user.username);
        add_nodes(txn, builder);
        add_tags(txn, builder);
    } break;
    case osmium::item_type::relation: {
        osmium::builder::RelationBuilder builder{buffer};
        set_attributes(builder, cid, visible, user.id, timestamp);
        builder.set_user(user.username);
        add_members(txn, builder);
        add_tags(txn, builder);
    } break;
    default:
        assert(false);
    }

    buffer.commit();
}

void osmobj::add_nodes(pqxx::work &txn,
                       osmium::builder::WayBuilder &builder) const
{
    osmium::builder::WayNodeListBuilder wnbuilder{builder};
    pqxx::result const result =
        txn.prepared("way_nodes")(m_id)(m_version).exec();

    for (auto const &row : result) {
        wnbuilder.add_node_ref(row[0].as<osmium::object_id_type>());
    }
}

void osmobj::add_members(pqxx::work &txn,
                         osmium::builder::RelationBuilder &builder) const
{
    osmium::builder::RelationMemberListBuilder mbuilder{builder};
    pqxx::result const result = txn.prepared("members")(m_id)(m_version).exec();

    for (auto const &row : result) {
        osmium::item_type type;
        switch (*row[0].c_str()) {
        case 'N':
            type = osmium::item_type::node;
            break;
        case 'W':
            type = osmium::item_type::way;
            break;
        case 'R':
            type = osmium::item_type::relation;
            break;
        default:
            assert(false);
        }
        mbuilder.add_member(type, row[1].as<osmium::object_id_type>(),
                            row[2].c_str());
    }
}

void read_log(std::vector<osmobj> &objects_todo, std::string const &dir_name,
              std::string const &file_name, changeset_user_lookup *cucache)
{
    std::ifstream logfile{dir_name + file_name};
    if (!logfile.is_open()) {
        throw std::system_error{errno, std::system_category(),
                                "Could not open log file '" + dir_name +
                                    file_name + "'"};
    }

    for (std::string line; std::getline(logfile, line);) {
        auto const parts = osmium::split_string(line, ' ');
        if (parts.size() < 3) {
            std::cerr << "Warning: Ignored log line due to wrong formatting: "
                      << line << '\n';
            continue;
        }

        if (parts[2] == "N") {
            if (parts.size() != 6) {
                std::cerr
                    << "Warning: Ignored log line due to wrong formatting: "
                    << line << '\n';
                continue;
            }
            objects_todo.emplace_back(parts[3], parts[4], parts[5], cucache);
        } else if (parts[2] == "X") {
            std::cerr << "Error found in logfile: " << line << '\n';
        }
    }
}
