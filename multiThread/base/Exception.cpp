#include "Exception.h"

#include <execinfo.h>
#include <cxxabi.h>     // The header provides an interface to the C++ ABI.
#include <cstdlib>

using  namespace tinyWS_thread;

Exception::Exception(const char* what) : message_(what) {
    const int length = 200;
    void* buffer[length];
    int nptrs = ::backtrace(buffer, length);
    char** strings = ::backtrace_symbols(buffer, nptrs);
    if (strings) {
        for (int i = 0; i < nptrs; ++i) {
            stack_.append(strings[i]);
            stack_.push_back('\n');
        }
        ::free(strings);
    }
}


const char* Exception::what() const throw() {
    return message_.c_str();
}

const char * Exception::stackTrace() const throw() {
    return stack_.c_str();
}
