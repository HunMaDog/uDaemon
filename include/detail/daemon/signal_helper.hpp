/*
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */
#ifndef MARKLAR_INCLUDE_DETAIL_DAEMON_SIGNAL_HELPER_HPP
#define MARKLAR_INCLUDE_DETAIL_DAEMON_SIGNAL_HELPER_HPP

// Marklar headers
#include <detail/daemon/common.hpp>
// External library headers
// C/C++ standard library headers
#include <signal.h>

namespace marklar::detail::daemon
{

constexpr int smallest_signal_id{1}; // TODO :: many signals have different numeric values on different architectures

void
reset_signal_handlers_to_default()
{
#if defined _NSIG
    // TODO :: compile dependent
    for (auto id_of_signal{smallest_signal_id}; id_of_signal < _NSIG; ++id_of_signal)
    {
        switch (id_of_signal)
        {
            // SIGKILL and SIGSTOP cannot be overridden
            case SIGKILL: [[fallthrough]];
            case SIGSTOP:
                break;

                // override
            default: ::signal(id_of_signal, SIG_DFL); break;
        }
    }
#else
#    warning "Can't reset signal handlers to default, daemon may behave in a non-standard and unpredictable way."
#endif
}

[[nodiscard("Daemon process may behave in a non-standard and unpredictable way.")]] bool
clear_signal_mask(void)
{
    sigset_t signal_set{};

    if (returned_with_error == ::sigemptyset(&signal_set))
    {
        return false;
    }

    if (returned_with_error == ::sigprocmask(SIG_SETMASK, &signal_set, nullptr))
    {
        return false;
    }

    return true;
}

} // namespace marklar::detail::daemon

#endif // MARKLAR_INCLUDE_DETAIL_DAEMON_SIGNAL_HELPER_HPP
