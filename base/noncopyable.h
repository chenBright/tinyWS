#ifndef TINYWS_NONCOPYABLE_H
#define TINYWS_NONCOPYABLE_H

namespace tinyWS {
    class noncopyable {
    protected:
        // 默认的构造函数和析构函数是保护的
        noncopyable() = default;
        ~noncopyable() = default;

    private:
        // 使用delete关键字禁止编译器自动产生复制构造函数和复制赋值操作符
        noncopyable(const noncopyable&) = delete;
        const noncopyable& operator=(const noncopyable&) = delete;
    };
}

#endif //TINYWS_NONCOPYABLE_H
