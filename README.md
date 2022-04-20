# uDaemon
uDaemon - Micro Daemon is a small header only daemon solution.

```cpp

    #include "daemon/udaemon.hpp"

    #include <syslog.h>
    #include <sstream>

    constexpr int forty_and_two{42};
    
    void init_syslog()
    {
        ::setlogmask (LOG_UPTO (LOG_INFO));
        ::openlog ("answer-udaemon-core", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    }
    
    bool print_to_syslog()
    {
            std::stringstream log_message;
            log_message << "The answer to life, the universe, and everything is " <<  forty_and_two << ".";
            syslog (LOG_INFO, "%s", log_message.str().c_str());
            
            return false;
    }
    
    void close_syslog()
    {
        ::closelog();
    }

    int main()
    {
        using marklar::detail::daemon::uDaemon;
    
        uDaemon answer({"answer-udaemon", "/tmp/answer.pid", "/", 0});
        
        answer.run(print_to_syslog, init_syslog, close_syslog);
    
        return 0;
    }

```
