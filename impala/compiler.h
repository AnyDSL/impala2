#ifndef IMPALA_COMPILER_H
#define IMPALA_COMPILER_H

#include "thorin/util/location.h"
#include "thorin/util/stream.h"

namespace impala {

using thorin::Location;

class Compiler {
public:
    template<typename... Args>
    std::ostream& warning(Location location, const char* fmt, Args... args) {
        ++num_warnings_;
        thorin::streamf(std::cerr, "{}: warning: ", location);
        return thorin::streamf(std::cerr, fmt, args...) << std::endl;;
    }
    template<typename... Args>
    std::ostream& error(Location location, const char* fmt, Args... args) {
        ++num_errors_;
        thorin::streamf(std::cerr, "{}: error: ", location);
        return thorin::streamf(std::cerr, fmt, args...) << std::endl;;
    }

private:
    int num_warnings_;
    int num_errors_;
};

}

#endif