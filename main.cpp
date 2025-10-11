#include <functional>
#include <iostream>
#include <type_traits>
#include <memory>
#include <chrono>
#include "smartptr_te.h"

template<class F, class... Args>
auto time_func(F&& func, Args&&... args) {
    const auto start{std::chrono::steady_clock::now()};
    auto _{func(std::forward<Args>(args)...)};
    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> ret{finish - start};
    return ret;
}

int test1() noexcept {
    struct d : decltype([](void* ptr) {
        delete static_cast<int*>(ptr);
    }) {};
    for(int i = 0; i < 1'000'000; i++) {
        if(std::rand() % 2)
            lbo_optimized::smartptr_te<int> p1(new int{}, std::default_delete<int>{});
        else
            lbo_optimized::smartptr_te<int> p1(new int{}, d{});
    }
    return 0;
}
int test2() noexcept {
    struct d : decltype([](void* ptr) {
        delete static_cast<int*>(ptr);
    }) {};
    for(int i = 0; i < 1'000'000; i++) {
        if(std::rand() % 2)
            lbo_original::smartptr_te<int> p1(new int{}, std::default_delete<int>{});
        else
            lbo_original::smartptr_te<int> p1(new int{}, d{});
    }
    return 0;
}

int main() {
    /*auto my_deleter = [](void* ptr) {
        delete static_cast<int*>(ptr);
        std::cout << "destroy called!";
    };*/
    struct my_deleter : decltype([](void* ptr) {
        delete static_cast<int*>(ptr);
        std::cout << "destroy called!\n";
    }) {};


    std::cout << time_func(test1) << '\n';
    std::cout << time_func(test2) << '\n';

    return 0;
}
