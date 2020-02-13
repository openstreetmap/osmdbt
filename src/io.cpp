
#include "io.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
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
