#ifndef IMPALA_COMPILER_H
#define IMPALA_COMPILER_H

#include "impala/sema/world.h"

namespace impala {

using thorin::Location;

class Compiler {
public:
    Compiler(const Compiler&) = delete;
    Compiler(Compiler&&) = delete;
    Compiler& operator=(Compiler) = delete;
    Compiler() = default;

    int num_warnings() const { return num_warnings_; }
    int num_errors() const { return num_errors_; }

    template<class... Args>
    std::ostream& error(Location location, const char* fmt, Args... args) {
        ++num_errors_;
        thorin::errf("{}: error: ", location);
        return thorin::errln(fmt, std::forward<Args>(args)...);
    }
    template<class... Args>
    std::ostream& warn(Location location, const char* fmt, Args... args) {
        ++num_warnings_;
        thorin::errf("{}: warning: ", location);
        return thorin::errln(fmt, std::forward<Args>(args)...);
    }
    template<class... Args>
    std::ostream& note(Location location, const char* fmt, Args... args) {
        thorin::errf("{}: note: ", location);
        return thorin::errln(fmt, std::forward<Args>(args)...);
    }

    World world;

private:
    int num_warnings_ = 0;
    int num_errors_ = 0;
};

}

#endif
