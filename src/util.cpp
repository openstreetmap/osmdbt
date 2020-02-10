
#include "util.hpp"

/**
 * Replace a suffix (anything after last dot) on filename by the new_suffix.
 * If there is no suffix, append the new one.
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
