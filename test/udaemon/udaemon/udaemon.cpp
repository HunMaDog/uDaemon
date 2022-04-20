// Marklar headers
#include "daemon/udaemon.hpp"
// External library headers
#include "../catch.hpp"
// C++ standard library headers
#include <filesystem>
#include <iostream>

constexpr int forty_and_two{42};

constexpr int umask_flag{S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH};
#if defined(__clang__)
constexpr int wrong_umask_flag{S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH + 1};
#endif

constexpr const char * name_of_service{"test-uDaemon-service"};

constexpr const char * path_of_test_file{"/tmp/udaemon_is_answering.txt"};
constexpr const char * path_of_pid_file{"/tmp/udaemon_test.pid"};
constexpr const char * path_of_root_directory{"/"};
constexpr const char * path_of_wrong_pid_file{"/tmp"};
constexpr const char * path_of_wrong_root_directory{"/dev/null"};

bool
core()
{
    std::ofstream test_file;

    test_file.open(path_of_test_file);
    test_file << "The answer to life, the universe, and everything is " << forty_and_two << ".";
    test_file.close();

    // Exit from the loop
    return false;
};

void
clean_up_test_file()
{
    if (std::filesystem::exists(path_of_test_file))
    {
        std::filesystem::remove(path_of_test_file);
    }
}

TEST_CASE("marklar::detail::daemon::uDaemon integration test")
{
    using marklar::daemon::uDaemon;

    uDaemon answer({name_of_service, path_of_pid_file, path_of_root_directory, umask_flag});

    REQUIRE_FALSE(std::filesystem::exists(path_of_test_file));
    CHECK(uDaemon::SUCCESS == answer.run(core)); // The run function only can return with the parent's process error
    sleep(3);                                    // Wait a few seconds to finish running
    REQUIRE(std::filesystem::exists(path_of_test_file));

    clean_up_test_file();
}

TEST_CASE("marklar::detail::daemon::uDaemon wrong pid file")
{
    using marklar::daemon::uDaemon;

    uDaemon answer({name_of_service, path_of_wrong_pid_file, path_of_root_directory, umask_flag});

    REQUIRE_FALSE(std::filesystem::exists(path_of_test_file));
    CHECK(uDaemon::SUCCESS == answer.run(core)); // Should be success because error will occur in the child process
    REQUIRE_FALSE(std::filesystem::exists(path_of_test_file));

    clean_up_test_file();
}

TEST_CASE("marklar::detail::daemon::uDaemon wrong directory")
{
    using marklar::daemon::uDaemon;

    uDaemon answer({name_of_service, path_of_pid_file, path_of_wrong_root_directory, umask_flag});

    REQUIRE_FALSE(std::filesystem::exists(path_of_test_file));
    CHECK(uDaemon::SUCCESS == answer.run(core)); // Should be success because error will occur in the child process
    REQUIRE_FALSE(std::filesystem::exists(path_of_test_file));

    clean_up_test_file();
}

#if defined(__clang__)
// The GCC handling differently the umask flag
TEST_CASE("marklar::detail::daemon::uDaemon wrong umask")
{
    using marklar::daemon::uDaemon;

    uDaemon answer({name_of_service, path_of_pid_file, path_of_root_directory, wrong_umask_flag});

    REQUIRE_FALSE(std::filesystem::exists(path_of_test_file));
    CHECK(uDaemon::SUCCESS == answer.run(core)); // Should be success because error will occur in the child process
    REQUIRE_FALSE(std::filesystem::exists(path_of_test_file));

    clean_up_test_file();
}
#endif
