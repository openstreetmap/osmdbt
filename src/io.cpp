
#include "io.hpp"

#include <osmium/io/detail/read_write.hpp>

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

void rename_file(std::string const &old_name, std::string const &new_name)
{
    if (rename(old_name.c_str(), new_name.c_str()) != 0) {
        std::string msg{"Renaming '"};
        msg += old_name;
        msg += "' to '";
        msg += new_name;
        msg += "' failed.";
        throw std::system_error{errno, std::system_category(), msg};
    }
}

void sync_dir(std::string const &dir_name)
{
    int const dir_fd =
        ::open(dir_name.c_str(),
               O_DIRECTORY | O_CLOEXEC); // NOLINT(hicpp-signed-bitwise)

    if (dir_fd == -1) {
        std::string msg{"Opening output directory failed for '"};
        msg += dir_name;
        msg += "'";
        throw std::system_error{errno, std::system_category(), msg};
    }

    if (::fsync(dir_fd) != 0) {
        std::string msg{"Syncing output directory failed for '"};
        msg += dir_name;
        msg += "'";
        throw std::system_error{errno, std::system_category(), msg};
    }

    if (::close(dir_fd) != 0) {
        std::string msg{"Closing output directory failed for '"};
        msg += dir_name;
        msg += "'";
        throw std::system_error{errno, std::system_category(), msg};
    }
}

int excl_write_open(std::string const &file_name)
{
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    int const flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC;

    return ::open(file_name.c_str(), flags, 0666);
}

PIDFile::PIDFile(std::string const &dir, std::string const &name)
{
    if (dir.empty()) {
        return;
    }

    std::string const path{dir + "/" + name + ".pid"};

    int const fd = excl_write_open(path);

    if (fd < 0) {
        if (errno == EEXIST) {
            throw std::runtime_error{"pid file '" + path +
                                     "' exists. Is another " + name +
                                     " process running?"};
        }
        throw std::runtime_error{"Can not create pid file '" + path + "'."};
    }

    auto pid = std::to_string(::getpid());
    pid += '\n';
    osmium::io::detail::reliable_write(fd, pid.data(), pid.size());
    osmium::io::detail::reliable_close(fd);

    m_path = path;
}

PIDFile::~PIDFile()
{
    if (!m_path.empty()) {
        ::unlink(m_path.c_str());
    }
}
