#pragma once

#include <string>

void rename_file(std::string const &old_name, std::string const &new_name);
void sync_dir(std::string const &dir_name);
int excl_write_open(std::string const &file_name);

/**
 * Process-ID file also used as lock file.
 */
class PIDFile
{
public:
    PIDFile(std::string const &dir, std::string const &name);

    PIDFile(PIDFile const &) = delete;
    PIDFile(PIDFile &&) = delete;

    PIDFile &operator=(PIDFile const &) = delete;
    PIDFile &operator=(PIDFile &&) = delete;

    ~PIDFile();

private:
    std::string m_path;

}; // class PIDFile
