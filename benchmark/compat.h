#pragma once
// Polyfill for older MinGW g++ (6.x) that lacks std::clamp
#ifndef __cpp_lib_clamp
#include <algorithm>
namespace std {
    template<class T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }
}
#endif
