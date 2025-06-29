
#include "util.hpp"
#include "io.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/osm/timestamp.hpp>

#include <ctime>
#include <string>

std::string create_replication_log_name(std::string const &name,
                                        std::time_t time)
{
    std::string file_name = "osm-repl-";

    osmium::Timestamp const now{time};

    file_name += now.to_iso_all();
    file_name += '-';
    file_name += name;
    file_name += ".log";

    return file_name;
}

void write_data_to_file(std::string const &data, std::string const &dir_name,
                        std::string const &file_name)
{
    std::string const file_name_final{dir_name + file_name};
    std::string const file_name_tmp{file_name_final + ".new"};

    int const fd = excl_write_open(file_name_tmp);

    osmium::io::detail::reliable_write(fd, data.data(), data.size());
    osmium::io::detail::reliable_fsync(fd);
    osmium::io::detail::reliable_close(fd);

    rename_file(file_name_tmp, file_name_final);
    sync_dir(dir_name);
}
