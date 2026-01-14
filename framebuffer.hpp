#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

#include "geometry.hpp"

using color_t = vec<uint8_t, 4>;

using framebuffer_t =
struct framebuffer {
    framebuffer(int w, int h) : w(w), h(h), data(w*h*bpp, 0) {
        for (int j=0; j<h; j++)
            for (int i=0; i<w; i++)
                set(i, j, {});
    }
    void set(int x, int y, const color_t& color){
        if (!data.size() || x<0 || y<0 || x>=w || y>=h) return;
        memcpy(data.data()+(x+y*w)*bpp, color.data.data(), bpp);
    }
    color_t get(int x, int y){
        if (!data.size() || x<0 || y<0 || x>=w || y>=h) return {0,0,0,0};
        color_t ret;
        ret[0] = data[(x+y*w)*bpp+0];
        ret[1] = data[(x+y*w)*bpp+1];
        ret[2] = data[(x+y*w)*bpp+2];
        ret[3] = data[(x+y*w)*bpp+3];
        return ret;
    }
    void clear(uint8_t c = 0){ std::fill(data.begin(), data.end(), c); }

    int w;
    int h;
    int bpp = 4; // 4 bytes per pixel R, G, B, A
    std::vector<uint8_t> data = {};
};

constexpr color_t
    white  = {255, 255, 255, 255},
    green  = {  0, 255,   0, 255},
    red    = {255,   0,   0, 255},
    blue   = { 64, 128, 255, 255},
    yellow = {255, 200,   0, 255},
    purple = { 70,  50, 150, 255},
    pink   = {175,  50, 150, 255},
    teal   = { 10, 215, 170, 255};

constexpr color_t colors[] = {red, green, blue, pink, teal, purple};
