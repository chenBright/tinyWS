#ifndef TINYWS_NONCOPYABLE_H
#define TINYWS_NONCOPYABLE_H

namespace tinyWS {
    // 不可拷贝类
    class noncopyable {
    protected:
        // 默认的构造函数和析构函数是 protected
        noncopyable() = default;

        ~noncopyable() = default;

    private:
        // 使用 delete 关键字禁止编译器自动产生复制构造函数和复制赋值操作符。
        noncopyable(const noncopyable&) = delete;

        const noncopyable& operator=(const noncopyable&) = delete;
    };
}

#endif //TINYWS_NONCOPYABLE_H
