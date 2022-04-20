/*
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */
#ifndef MARKLAR_INCLUDE_DETAIL_DAEMON_FILE_HELPER_HPP
#define MARKLAR_INCLUDE_DETAIL_DAEMON_FILE_HELPER_HPP

// Marklar headers
#include <detail/daemon/common.hpp>
// External library headers
// C/C++ standard library headers
#include <sys/file.h>

#include <sys/resource.h>
#include <unistd.h>

#include <filesystem>

namespace marklar::detail::daemon
{

constexpr int number_of_standard_file_descriptors{3};
constexpr const char * path_of_dev_null{"/dev/null"};

[[nodiscard("Unclosed file descriptors may remain open for an indefinite amount of time.")]] bool
close_non_standard_file_descriptors()
{
    struct rlimit reslimit
    {
    };
    auto num_of_file_descriptors{::getrlimit(RLIMIT_NOFILE, &reslimit)};

    if (returned_with_error == num_of_file_descriptors)
    {
        return false;
    }

    // stdin, stdout and stderr file descriptors identifiers are always 0,1,2
    for (auto file_descriptor{number_of_standard_file_descriptors}; file_descriptor < num_of_file_descriptors; ++file_descriptor)
    {
        ::close(file_descriptor);
    }

    return true;
}

[[nodiscard("Parent process could therefore fill the file descriptor table with bogus files.")]] bool
attach_standard_file_descriptors_to_null()
{
    auto const read_file_descriptor = ::open(path_of_dev_null, O_RDONLY);
    if (returned_with_error == read_file_descriptor)
    {
        return false;
    }
    if (returned_with_error == ::dup2(read_file_descriptor, STDIN_FILENO))
    {
        return false;
    }

    auto const write_file_descriptor = ::open(path_of_dev_null, O_WRONLY);
    if (returned_with_error == write_file_descriptor)
    {
        return false;
    }
    if (returned_with_error == ::dup2(write_file_descriptor, STDOUT_FILENO))
    {
        return false;
    }
    if (returned_with_error == ::dup2(write_file_descriptor, STDERR_FILENO))
    {
        return false;
    }

    return true;
}

} // namespace marklar::detail::daemon

#endif // MARKLAR_INCLUDE_DETAIL_DAEMON_FILE_HELPER_HPP
