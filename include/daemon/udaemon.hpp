/*
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */
#ifndef MARKLAR_INCLUDE_DAEMON_UDAEMON_HPP
#define MARKLAR_INCLUDE_DAEMON_UDAEMON_HPP

// Marklar headers
#include <detail/daemon/file_handler.hpp>
#include <detail/daemon/signal_helper.hpp>
// External library headers
// C/C++ standard library headers
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>

/*
 * Small header only linux daemon solution
 */
namespace marklar::daemon
{

constexpr int default_process_id{0};
constexpr int default_session_id{0};

class uDaemon {
public:
    typedef enum
    {
        SUCCESS = 0,
        CANNOT_CLOSE_NON_STD_FILE_DESCRIPTORS,
        CANNOT_CLEAR_SIGNAL_MASK,
        CANNOT_FORK_DAEMON_PROCESS,
        CANNOT_ATTACH_STD_FILE_DESCRIPTORS_TO_NULL,
        CANNOT_CREATE_SESSION,
        CANNOT_CHANGE_DIR,
        CANNOT_CREATE_PID_FILE,
        CANNOT_DELETE_PID_FILE,
    } Status;

    struct Settings
    {
        std::string_view const name_of_daemon_{"uDameon-service"};
        std::filesystem::path const path_of_pid_file_{"/tmp/uDaemon.pid"};
        std::filesystem::path const path_of_daemon_root_{"/"};
        // TODO :: not handling when umask is ignored
        mode_t const umask_mode_{S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH};
    };

protected:
    typedef enum
    {
        NOT_CLOSE_SYSLOG = 0,
        CLOSE_SYSLOG,
    } SysLogHandling;

public:
    uDaemon(Settings const & settings)
        : settings_{settings}
    {
    }

    uDaemon(Settings && settings)
        : settings_{std::move(settings)}
    {
    }

    virtual ~uDaemon() = default;

    // Should be non-copiable and movable
    uDaemon(uDaemon const &) = delete;
    uDaemon & operator=(uDaemon const &) = delete;

    uDaemon(uDaemon &&) = delete;
    uDaemon & operator=(uDaemon &&) = delete;

    Status run(
        std::function<bool(void)> loop, std::function<void(void)> prework = []() {}, std::function<void(void)> postwork = []() {})
    {
        using marklar::detail::daemon::attach_standard_file_descriptors_to_null;
        using marklar::detail::daemon::clear_signal_mask;
        using marklar::detail::daemon::close_non_standard_file_descriptors;
        using marklar::detail::daemon::reset_signal_handlers_to_default;

        // Closing all, but the standard file descriptors, because the daemon process inherits all open files from the calling process.
        if (!close_non_standard_file_descriptors())
            return CANNOT_CLOSE_NON_STD_FILE_DESCRIPTORS;

        // Reset signal handlers to default, because the daemon process also inherits the signal handler configuration from the caller
        // process.
        reset_signal_handlers_to_default();

        // Reset signal mask to default, because the signal mask also gets inherited by the daemon from the calling process.
        if (!clear_signal_mask())
            return CANNOT_CLEAR_SIGNAL_MASK;

        // After cleaned up, forking the child process
        process_id_ = ::fork();

        if (process_id_ < default_process_id)
        {
            return CANNOT_FORK_DAEMON_PROCESS;
        }

        if (process_id_ > default_process_id)
        {
            std::cout << "Process ID of child process = " << process_id_ << "\n";

            return SUCCESS;
        }

        /*
         * Code of Forked Process
         */

        ::setlogmask(LOG_UPTO(LOG_INFO));
        ::openlog(settings_.name_of_daemon_.data(), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);

        if (!attach_standard_file_descriptors_to_null())
        {
            return error_handling(CANNOT_ATTACH_STD_FILE_DESCRIPTORS_TO_NULL, CLOSE_SYSLOG);
        }

        ::umask(settings_.umask_mode_);

        session_id_ = ::setsid();
        if (session_id_ < default_session_id)
        {
            return error_handling(Status::CANNOT_CREATE_SESSION, CLOSE_SYSLOG);
        }

        if (!std::filesystem::is_directory(settings_.path_of_daemon_root_)
            || detail::daemon::returned_with_error == ::chdir(settings_.path_of_daemon_root_.c_str()))
        {
            return error_handling(CANNOT_CHANGE_DIR, CLOSE_SYSLOG);
        }

        if (!create_pid_file())
        {
            return error_handling(CANNOT_CREATE_PID_FILE, CLOSE_SYSLOG);
        }

        prework();
        while (loop())
            ;
        postwork();

        if (!delete_pid_file())
        {
            return error_handling(CANNOT_DELETE_PID_FILE, CLOSE_SYSLOG);
        }

        ::syslog(LOG_INFO, "%s", status_to_string(SUCCESS).c_str());
        ::closelog();

        return SUCCESS;
    }

    std::string status_to_string(Status status) const
    {
        switch (status)
        {
            case SUCCESS: return {"Successfully completed the run!"};

            case CANNOT_CLOSE_NON_STD_FILE_DESCRIPTORS: return {"Cannot close the non standard file descriptors!"};

            case CANNOT_CLEAR_SIGNAL_MASK: return {"Cannot clear signal mask!"};

            case CANNOT_FORK_DAEMON_PROCESS: return {"Cannot fork daemon process!"};

            case CANNOT_ATTACH_STD_FILE_DESCRIPTORS_TO_NULL: return {"Cannot attach standard file descriptors to /dev/null !"};

            case CANNOT_CREATE_SESSION: return {"Cannot create new session!"};

            case CANNOT_CHANGE_DIR: return {"Cannot change current working directory to " + settings_.path_of_daemon_root_.string() + " !"};

            case CANNOT_CREATE_PID_FILE: return {"Cannot create " + settings_.path_of_pid_file_.string() + " PID file!"};

            case CANNOT_DELETE_PID_FILE: return {"Cannot unlink " + settings_.path_of_pid_file_.string() + " PID file!"};
        }

        return {"Unknown daemon error!"};
    }

protected:
    bool create_pid_file()
    {
        if (std::filesystem::is_directory(settings_.path_of_pid_file_) || std::filesystem::exists(settings_.path_of_pid_file_))
        {
            return false;
        }

        std::ofstream pid_file{settings_.path_of_pid_file_};

        if (pid_file.is_open())
        {
            pid_file << ::getpid();
        } else
        {
            return false;
        }

        return true;
    }

    bool delete_pid_file()
    {
        if (std::filesystem::exists(settings_.path_of_pid_file_))
        {
            return std::filesystem::remove(settings_.path_of_pid_file_);
        }

        return true;
    }

    Status error_handling(Status status, SysLogHandling syslog_handling)
    {
        ::syslog(LOG_ERR, "%s", status_to_string(status).c_str());

        if (CLOSE_SYSLOG == syslog_handling)
        {
            ::closelog();
        }

        return status;
    }

protected:
    Settings const settings_{};

    pid_t process_id_{default_process_id};
    pid_t session_id_{default_session_id};
};

} // namespace marklar::daemon

#endif // MARKLAR_INCLUDE_DAEMON_UDAEMON_HPP
