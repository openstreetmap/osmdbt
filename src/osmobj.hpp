#pragma once

#include "db.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm/types.hpp>

#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

struct userinfo
{
    osmium::user_id_type id = 0;
    std::string username;
};

using changeset_user_lookup =
    std::unordered_map<osmium::changeset_id_type, userinfo>;

class osmobj
{
public:
    explicit osmobj(std::string const &obj, std::string const &version,
                    std::string const &changeset,
                    changeset_user_lookup *cucache = nullptr);

    osmium::item_type type() const noexcept { return m_type; }
    osmium::object_id_type id() const noexcept { return m_id; }
    osmium::object_version_type version() const noexcept { return m_version; }
    osmium::changeset_id_type cid() const noexcept { return m_cid; }

    void add_nodes(pqxx::work &txn, osmium::builder::WayBuilder &builder) const;
    void add_members(pqxx::work &txn,
                     osmium::builder::RelationBuilder &builder) const;
    void get_data(pqxx::work &txn, osmium::memory::Buffer &buffer,
                  changeset_user_lookup const &cucache) const;

    template <typename TBuilder>
    void set_attributes(TBuilder &builder, osmium::changeset_id_type const cid,
                        bool const visible, osmium::user_id_type const uid,
                        char const *const timestamp) const
    {
        builder.set_id(m_id)
            .set_version(m_version)
            .set_changeset(cid)
            .set_visible(visible)
            .set_uid(uid)
            .set_timestamp(timestamp);
    }

    template <typename TBuilder>
    void add_tags(pqxx::work &txn, TBuilder &builder) const
    {
        osmium::builder::TagListBuilder tbuilder{builder};
        pqxx::result const result =
#if PQXX_VERSION_MAJOR >= 6
            txn.exec_prepared(osmium::item_type_to_name(m_type) +
                                  std::string{"_tag"},
                              m_id, m_version);
#else
            txn.prepared(osmium::item_type_to_name(m_type) +
                         std::string{"_tag"})(m_id)(m_version)
                .exec();
#endif

        for (auto const &row : result) {
            tbuilder.add_tag(row[0].c_str(), row[1].c_str());
        }
    }

    using osmobj_tuple = std::tuple<unsigned int, osmium::object_id_type,
                                    osmium::object_version_type>;

    friend bool operator<(osmobj const &a, osmobj const &b) noexcept
    {
        return osmobj_tuple{osmium::item_type_to_nwr_index(a.type()), a.id(),
                            a.version()} <
               osmobj_tuple{osmium::item_type_to_nwr_index(b.type()), b.id(),
                            b.version()};
    }

    friend bool operator>(osmobj const &a, osmobj const &b) noexcept
    {
        return osmobj_tuple{osmium::item_type_to_nwr_index(a.type()), a.id(),
                            a.version()} >
               osmobj_tuple{osmium::item_type_to_nwr_index(b.type()), b.id(),
                            b.version()};
    }

    friend bool operator<=(osmobj const &a, osmobj const &b) noexcept
    {
        return !(a > b);
    }

    friend bool operator>=(osmobj const &a, osmobj const &b) noexcept
    {
        return !(a < b);
    }

private:
    osmium::item_type m_type;
    osmium::object_id_type m_id;
    osmium::object_version_type m_version;
    osmium::changeset_id_type m_cid;

}; // class osmobj

void read_log(std::vector<osmobj> &objects_todo, std::string const &dir_name,
              std::string const &file_name,
              changeset_user_lookup *cucache = nullptr);
