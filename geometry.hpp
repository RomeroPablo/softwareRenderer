#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <limits>
#include <optional>

template<typename T>
concept integer =
    (std::integral<T> &&
    !std::same_as<T, bool>)
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

    template<std::size_t p>
    constexpr mat<T, m, n> operator*(const mat<T, n, p>& b) const noexcept{
        mat<T, m, p> c{};
        for(std::size_t i{0uz}; i < m; i++)
            for(std::size_t j{0uz}; j < p; j++){
                T sum{};
                for(std::size_t k{0uz}; k < n; k++)
                    sum+= (*this)[i][k] * b[k][j];
                c[i][j] = sum;
            }
        return c;
    }

    constexpr mat<T, n, m> transpose() const noexcept{
        mat<T, n, m> t{};
        for(std::size_t i{0uz}; i < m; i++)
            for(std::size_t j{0uz}; j < n; j++)
                t[j][i] = (*this)[i][j];
        return t;
    }

    std::optional<mat<T, m, n>> inverse() const requires (m == n && std::floating_point<T>){
        mat<T, m, n> a = *this;
        mat<T, m, n> inv{};
        for(std::size_t i{0uz}; i < n; i++)
            inv[i][i] = T{1};

        const T eps = std::numeric_limits<T>::epsilon() * T{10};
        for(std::size_t i{0uz}; i < n; i++){
            std::size_t pivot = i;
            T max = std::abs(a[i][i]);
            for(std::size_t r{i + 1}; r < n; r++){
                T val = std::abs(a[r][i]);
                if(val > max){
                    max = val;
                    pivot = r;
                }
            }
            if(max <= eps)
                return std::nullopt;
            if(pivot != i){
                std::swap(a[i], a[pivot]);
                std::swap(inv[i], inv[pivot]);
            }

            T inv_diag = T{1} / a[i][i];
            for(std::size_t j{0uz}; j < n; j++){
                a[i][j] *= inv_diag;
                inv[i][j] *= inv_diag;
            }

            for(std::size_t r{0uz}; r < n; r++){
                if(r == i)
                    continue;
                T factor = a[r][i];
                if(factor == T{})
                    continue;
                for(std::size_t j{0uz}; j < n; j++){
                    a[r][j] -= factor * a[i][j];
                    inv[r][j] -= factor * inv[i][j];
                }
            }
        }
        return inv;
    }
};
