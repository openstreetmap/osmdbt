
#include "util.hpp"

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
    } else if (pos == 0) {
        return "/";
    }

    file_name.resize(pos);

    return file_name;
}

/**
 * Return the specified time in ISO format (UTC).
 */
std::string get_time(std::time_t now)
{
    std::string buffer(16, '\0');

    auto const num = std::strftime(&buffer[0], buffer.size(), "%Y%m%dT%H%M%S",
                                   std::gmtime(&now));

    buffer.resize(num);
    assert(num == 15);

    return buffer;
}
