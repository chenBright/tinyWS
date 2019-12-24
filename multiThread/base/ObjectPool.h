#ifndef TINYWS_OBJECTPOOL_H
#define TINYWS_OBJECTPOOL_H

#include <memory>
#include <vector>
#include <functional>
#include <iostream>

namespace tinyWS_thread {

    // 借助 C++ 智能指针的自定义删除器，
    // 在智能指针释放的时候会调用删除器，
    // 在删除器中将用完的对象重新放回对象池。
    // 参考
    // https://www.cnblogs.com/qicosmos/p/4995248.html
    // https://www.cnblogs.com/qicosmos/p/3673723.html
    // https://zhuanlan.zhihu.com/p/73066435

    // unique_ptr 版本的对象池
    template <class T>
    class ObjectPool_unique {
    public:
        using DeleterType = std::function<void(T*)>;
        using ObjectType = std::unique_ptr<T, DeleterType>;

    private:
        std::vector<std::unique_ptr<T>> pool_;

    public:
        void add(std::unique_ptr<T> obj) {
            pool_.emplace_back(std::move(obj));
        }

        ObjectType get() {
            if (pool_.empty()) {
                throw std::logic_error("no more object");
            }

            // 释放原生指针，并将其返回给用户
            ObjectType obj(pool_.back().release(), [this](T* t) {
                // 将对象放回对象池
                this->pool_.emplace_back(std::unique_ptr<T>(t));
            });

            pool_.pop_back();
            return std::move(obj);
        }

        bool empty() const {
            return pool_.empty();
        }

        size_t size() const {
            return pool_.size();
        }
    };


    // shared_ptr 版本的对象池
    template <class T>
    class ObjectPool_shared {
    public:
        using DeleterType = std::function<void(T*)>;
        using ObjectType = std::shared_ptr<T>;

    private:
        std::vector<std::unique_ptr<T>> pool_;

    public:
        void add(std::unique_ptr<T> obj) {
            pool_.emplace_back(std::move(obj));
        }

        ObjectType get() {
            if (pool_.empty()) {
                throw std::logic_error("no more object");
            }


            ObjectType obj = pool_.back();
            // 需要拷贝构造 T 对象
            ObjectType newObj = ObjectType(new T(*obj.get()), [this](T* t) {
                // 将对象放回对象池
                pool_.emplace_back(std::shared_ptr<T>(t));
            });

            pool_.pop_back();
            return std::move(obj);
        }

        bool empty() const {
            return pool_.empty();
        }

        size_t size() const {
            return pool_.size();
        }
    };
}

#endif //TINYWS_OBJECTPOOL_H
