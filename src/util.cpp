
#include "util.hpp"
#include "io.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/osm/timestamp.hpp>

/**
 * Replace a suffix (anything after last dot) on filename by the new_suffix.
 * If there is no suffix, append the new one. The new_suffix must begin with
 * a '.'.
 */
std::string replace_suffix(std::string filename, char const *new_suffix)
{
    auto const dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        filename.resize(dot);
    }

    filename += new_suffix;
    return filename;
}

std::string dirname(std::string file_name)
{
    auto const pos = file_name.find_last_of('/');
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return "/";
    }

    file_name.resize(pos);

    return file_name;
}

std::string create_replication_log_name(std::string const &name,
                                        std::time_t time)
{
    std::string file_name = "/osm-repl-";

    osmium::Timestamp now{time};

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
    std::string const file_name_new{file_name_final + ".new"};

    int const fd = excl_write_open(file_name_new);

    osmium::io::detail::reliable_write(fd, data.data(), data.size());
    osmium::io::detail::reliable_fsync(fd);
    osmium::io::detail::reliable_close(fd);

    rename_file(file_name_new, file_name_final);
    sync_dir(dir_name);
}
