#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>

template<typename T>
concept integer =
    (std::integral<T> &&
    !std::same_as<T, bool> &&
    !std::same_as<T, char> &&
    !std::same_as<T, signed char> &&
    !std::same_as<T, unsigned char>)
    || std::floating_point<T>;

template<integer T, std::size_t N> struct vec{
    std::array<T, N> data;

    constexpr const T& operator[](std::size_t n) const noexcept {
        return data[n];
    }

    constexpr T& operator[](std::size_t n) noexcept {
        return data[n];
    }

    constexpr vec<T, N> operator+(const vec<T, N>& w) const noexcept {
        vec<T, N> u{};
        for(std::size_t i{0uz}; i < N; i++)
            u[i] = (*this)[i] + w[i];
        return u;
    }

    constexpr vec<T, N> operator-(const vec<T, N>& w) const noexcept {
        vec<T, N> u{};
        for(std::size_t i{0uz}; i < N; i++)
            u[i] = (*this)[i] - w[i];
        return u;
    }

    constexpr vec<T, N> operator*(const T s) const noexcept {
        vec<T, N> u{};
        for(std::size_t i{0uz}; i < N; i++)
            u[i] = (*this)[i] * s;
        return u;
    }

    double length() const {
        T sum{};
        for(std::size_t i{0uz}; i < N; i++){
            T x = (*this)[i];
            sum += (x * x);
        }
        return std::sqrt(sum);
    }

    constexpr T dot(const vec<T,N>& w) const noexcept {
        T dp{};
        for(std::size_t i{0uz}; i < N; i++){
            T s = (*this)[i] * w[i];
            dp += s;
        }
        return dp;
    }

    double angle(const vec<T,N>& w) const {
        return std::acos( std::clamp(this->dot(w) / (this->length() * w.length()), -1., 1.) );
    }

};

template<integer T, std::size_t m, std::size_t n> struct mat{
    std::array<vec<T, n>, m> data;

    constexpr const vec<T, n>& operator[](std::size_t i) const noexcept{
        return data[i];
    }

    constexpr vec<T, n>& operator[](std::size_t i) noexcept{
        return data[i];
    }

    constexpr mat<T, m, n> operator+(const mat<T, m, n>& b) const noexcept{
        mat<T, m, n> c{};
        for(std::size_t i = {0uz}; i < m; i++){
            c[i] = (*this)[i] + b[i];
        }
        return c;
    }

    constexpr mat<T, m, n> operator-(const mat<T, m, n>& b) const noexcept{
        mat<T, m, n> c;
        for(std::size_t i = {0uz}; i < m; i++){
            c[i] = (*this)[i] - b[i];
        }
        return c;
    }

    constexpr vec<T, m> operator*(const vec<T, n>& v) const noexcept{
        vec<T, m> u{};
        for(std::size_t i{0uz}; i < m; i++){
            u[i] = (*this)[i].dot(v);
        }
        return u;
    }

    constexpr mat<T, m, n> operator*(const mat<T, n, n>& b) const noexcept{
        mat<T, m, n> c;
        for(std::size_t i = {0uz}; i < m; i++){
            for(std::size_t j = {0uz}; j < n; j++)
                c[i][j] = (*this)[i][j].dot(b[i][j]);
        }
        return c;
    }
};
