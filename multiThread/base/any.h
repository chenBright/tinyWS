#ifndef TINYWS_ANY_H
#define TINYWS_ANY_H

#include <memory>
#include <utility>
#include <algorithm>
#include <typeinfo>     // typeinfo

namespace tinyWS_thread {

    // std::any、any_cast（C++ 17）的简单实现。
    // 参考
    // https://www.cnblogs.com/joeylee97/p/8877248.html
    // https://www.cnblogs.com/qicosmos/p/3420095.html
    // https://github.com/qicosmos/cosmos/blob/3cfa3cc189b49809e4fcd45644c83bc22e6b1444/Any.hpp
    // https://www.cnblogs.com/qicosmos/p/3256022.html （类型擦除）
    // https://www.cnblogs.com/feixue/p/boost-any.html
    class any {
    private:
        class holder;
        using holderPtr = std::shared_ptr<holder>;

        // 数据基类，用于类型擦除。
        class holder {
        public:
            virtual ~holder() {}
            virtual holderPtr clone() const = 0;
            virtual const std::type_info& type() const = 0;
        };

        // 持有数据的派生类。
        template <class T>
        class data_holder : public holder {
        public:
            T data_;

            template <class U>
            explicit data_holder(U&& data) : data_(std::forward<U>(data)) {}

            holderPtr clone() const override {
                return static_cast<holderPtr>(new data_holder(data_));
            }

            const std::type_info& type() const override {
                return typeid(data_);
            }
        };

        holderPtr data_;

    public:
        // 友元函数 any_cast
        template <class T>
        friend T& any_cast(const any& anyObj);

        template <class T>
        friend T& any_cast(any& anyObj);

        template <class T>
        friend T* any_cast(const any* anyPtr);

        template <class T>
        friend T* any_cast(any* anyPtr);

        any() : data_(nullptr) {}

        template <class T>
        any(const T& data) : data_(new data_holder<T>(data)) {}

        any(const any& other) : data_(other.data_) {}

        any(any&& other) : data_(std::move(other.data_)) {}

        ~any() = default;

        any& operator=(const any& other) {
            any tmp(other);

            using std::swap;
            swap(*this, tmp);

            return *this;
        }

        any& operator=(any&& other) noexcept {
            if (this != &other) {
                data_ = std::move(other.data_);
            }

            return *this;
        }

        const std::type_info& type() const {
            return data_ == nullptr ? typeid(void) : data_->type();
        }
    };

    template <class T>
    T& any_cast(any& anyObj) {
        return static_cast<any::data_holder<T>*>(anyObj.data_.get())->data_;
    }

    template <class T>
    T& any_cast(const any& anyObj) {
        return static_cast<any::data_holder<T>*>(anyObj.data_.get())->data_;
    }

    template <class T>
    T* any_cast(const any* anyPtr) {
        return &(static_cast<any::data_holder<T>*>(anyPtr->data_.get())->data_);
    }

    template <class T>
    T* any_cast(any* anyPtr) {
        return &(static_cast<any::data_holder<T>*>(anyPtr->data_.get())->data_);
    }
}


#endif //TINYWS_ANY_H
