#pragma once

#include <stdexcept>
#include <string>

/**
 *  Thrown when there is a problem with the command line arguments.
 */
struct argument_error : std::runtime_error
{

    explicit argument_error(const char *message) : std::runtime_error(message)
    {}

    explicit argument_error(const std::string &message)
    : std::runtime_error(message)
    {}
};

/**
 *  Thrown when there is a problem with parsing the yaml config file.
 */
struct config_error : public std::runtime_error
{

    explicit config_error(const char *message)
    : std::runtime_error(std::string{"Config error: "} + message)
    {}

    explicit config_error(const std::string &message)
    : std::runtime_error("Config error: " + message)
    {}

}; // struct config_error

/**
 *  Thrown when there is a problem with the database.
 */
struct database_error : public std::runtime_error
{

    explicit database_error(const char *message)
    : std::runtime_error(std::string{"Database error: "} + message)
    {}

    explicit database_error(const std::string &message)
    : std::runtime_error("Database error: " + message)
    {}

}; // struct database_error
