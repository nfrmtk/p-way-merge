#pragma once
#include <type_traits>
template <typename F>
class Defer{
public:
    Defer(F&& f):f_(static_cast<F&&>(f)){}
    ~Defer(){
        static_assert(std::is_nothrow_invocable_v<F>, "dont throw in Defer dtor");
        f_();
    }
private:
F f_;
};