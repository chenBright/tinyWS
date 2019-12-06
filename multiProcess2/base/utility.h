#ifndef TINYWS_UTILITY_H
#define TINYWS_UTILITY_H

#include <memory>
#include <utility>

namespace tinyWS_process2 {

    // FIXME 不能用，会报错
    template <class T, class... Arg>
    std::unique_ptr<T> make_unique(Arg&&... arg) {
        return std::unique_ptr<T>(std::forward<T>(arg)...);
    }
}

#endif //TINYWS_UTILITY_H
