#include <SDL2/SDL.h>
#include <SDL_mouse.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <omp.h>
#include <string>
#include <utility>
#include <vector>

#include "SDL_main.h"
#include "framebuffer.hpp"
#include "geometry.hpp"
#include "model.hpp"

constexpr const char* PATH = "../assets/demon.obj";
//constexpr const char* PATH = "../assets/weep.obj";
//constexpr const char* PATH = "../assets/dionysos.obj";
//constexpr const char* PATH = "../assets/hunter.obj";
constexpr double aS = 1.0/1000.0;
using state_t =
struct state { 
    uint16_t width = 640;
    uint16_t height = 480;
    uint16_t depth = 255;
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;

    struct {
        bool running = true;
        bool up = false;
        bool down = false;
        bool forward = false;
        bool back = false;
        bool left = false;
        bool right = false;
        bool leanLeft = false;
        bool leanRight = false;
        int32_t yaw = 90000;    // θE3
        int32_t pitch = -15000; // θE3
        int32_t sensitivity = 35;
    } controls;

    struct {
        vec<float, 3> position = {0.0f, -3.0f, 0.0f};
        vec<float, 3> forward = {0.0f, 1.0f, 0.0f};
        vec<float, 3> up = {0.0f, 0.0f, 1.0f};
        vec<float, 3> right = {1.0f, 0.0f, 0.0f};
        vec<float, 2> orientation{};
    } camera;

    mat<float, 4, 4> mvp;

    struct {
        std::chrono::time_point<std::chrono::high_resolution_clock> start<%%>, end<%%>;
        std::chrono::milliseconds delta<%%>;
        double frameTime = 1000.0/60.0; // 60 fps
    } time;
};

void line(vec<int, 2> a, vec<int, 2> b, framebuffer_t &framebuffer, color_t color) {
    bool steep = std::abs (a[0]-b[0]) < std::abs(a[1]-b[1]);
    if(steep) <% std::swap(a[0], a[1]); std::swap(b[0], b[1]); %>
    if(a[0]>b[0]) <% std::swap(a[0], b[0]); std::swap(a[1], b[1]); %>

    float y = a[1];
    const float f = (b[1]-a[1]) / static_cast<float>(b[0]-a[0]);

    if(steep)<% for(size_t x=a[0]; x<=b[0]; x++, y+=f) framebuffer.set(y, x, color); %>
    else     <% for(size_t x=a[0]; x<=b[0]; x++, y+=f) framebuffer.set(x, y, color); %>
}


void raster(int ax, int ay, int bx, int by, int cx, int cy, framebuffer_t &framebuffer, color_t color) {
    if(ay<by)<%std::swap(ay, by); std::swap(ax, bx);%>
    if(ay<cy)<%std::swap(ay, cy); std::swap(ax, cx);%>
    if(by<cy)<%std::swap(by, cy); std::swap(bx, cx);%>

   int total_height = ay-cy;

    if(cy != by){
        int base_height = by - cy;
        for(int y = cy; y < by; y++){
            int x1 = cx + ((ax - cx)*(y - cy)) / total_height;
            int x2 = cx + ((bx - cx)*(y - cy)) / base_height;
            for(int x=std::min(x1,x2); x<std::max(x1,x2); x++)
                framebuffer.set(x, y, color);
        }
    }

    if(ay != by){
        int base_height = ay - by;
        for(int y = by; y < ay; y++){
            int x1 = cx + ((ax - cx)*(y - cy)) / total_height;
            int x2 = bx + ((ax - bx)*(y - by)) / base_height;
            for(int x=std::min(x1,x2); x<std::max(x1,x2); x++)
                framebuffer.set(x, y, color);
        }
    }
}


double inline tArea(vec<int, 2> a, vec<int, 2> b, vec<int, 2> c){
    return .5*((b[1]-a[1])*(b[0]+a[0]) + (c[1]-b[1])*(c[0]+b[0]) + (a[1]-c[1])*(a[0]+c[0]));
}

void rasterOMP(vec<int, 3> a, vec<int, 3> b, vec<int, 3> c, framebuffer_t& framebuffer, framebuffer_t& depthbuffer){
    double area = tArea(vec<int, 2>{a[0], a[1]}, vec<int, 2>{b[0], b[1]}, vec<int, 2>{c[0], c[1]});
    if(std::abs(area) < 1e-6) return;
    //if(area<1) return; // backface & area culling

    int bbXMin = std::min(std::min(a[0], b[0]), c[0]);
    int bbXMax = std::max(std::max(a[0], b[0]), c[0]);
    int bbYMin = std::min(std::min(a[1], b[1]), c[1]);
    int bbYMax = std::max(std::max(a[1], b[1]), c[1]);
    bbXMin = std::max(bbXMin, 0);
    bbXMax = std::min(bbXMax, framebuffer.w - 1);
    bbYMin = std::max(bbYMin, 0);
    bbYMax = std::min(bbYMax, framebuffer.h - 1);
    if(bbXMin >= bbXMax || bbYMin >= bbYMax) return;

// #pragma omp parallel for // this kills perf, consider memory alignment and cache behavior
    for(int x = bbXMin; x < bbXMax; x++){
        for(int y = bbYMin; y < bbYMax; y++){
            double α = tArea(vec<int, 2>{x, y}, vec<int, 2>{b[0], b[1]}, vec<int, 2>{c[0], c[1]}) / area;
            double β = tArea(vec<int, 2>{x, y}, vec<int, 2>{c[0], c[1]}, vec<int, 2>{a[0], a[1]}) / area;
            double γ = tArea(vec<int, 2>{x, y}, vec<int, 2>{a[0], a[1]}, vec<int, 2>{b[0], b[1]}) / area;
            if(α<0|| β<0|| γ<0) continue;

            unsigned char z = static_cast<unsigned char>(α * a[2] + β * b[2] + γ * c[2]);
            if(z <= depthbuffer.get(x, y)[0]) continue;
            depthbuffer.set(x, y, {z});
            framebuffer.set(x, y, {z, z, z, 255});
        }
    }
}

inline vec<int, 3> mvpv(vec<float, 3> a, const mat<float, 4, 4>& mvp, int width, int height){
    vec<float, 4> p = {a[0], a[1], a[2], 1};
    p  = mvp * p;

    if(std::abs(p[3]) < 1e-6f) return {-1, -1, -1};
    float ndcX = p[0] / p[3];
    float ndcY = p[1] / p[3];
    float ndcZ = p[2] / p[3];

    int sx = static_cast<int>((ndcX * 0.5f + 0.5f) * (width - 1));
    int sy = static_cast<int>((1.0f - (ndcY * 0.5f + 0.5f)) * (height - 1)); // flip Y for screen coords
    int sz = static_cast<int>((ndcZ * 0.5f + 0.5f) * 255.0f); // for your depthbuffer

    return {sx, sy, sz};
}

void drawModel(state_t& state, framebuffer_t& framebuffer, framebuffer_t& depthbuffer, const model_t& model){
    for(const auto& f : model.faces){
        vec<int, 3> a = mvpv(model.vertices[f[0]-1], state.mvp, state.width, state.height);
        vec<int, 3> b = mvpv(model.vertices[f[1]-1], state.mvp, state.width, state.height);
        vec<int, 3> c = mvpv(model.vertices[f[2]-1], state.mvp, state.width, state.height);
        if(a[0] < 0 || b[0] < 0 || c[0] < 0) continue;
        rasterOMP(a, b, c, framebuffer, depthbuffer);
    }
}

void getInput(state_t& state){
    SDL_Event e;
    while(SDL_PollEvent(&e));
    const uint8_t* keys = SDL_GetKeyboardState(nullptr);
    if(e.type == SDL_QUIT)          state.controls.running = false;
    if (keys[SDLK_ESCAPE])          state.controls.running = false;
    if (keys[SDL_SCANCODE_W])       state.controls.forward = true;
    if (keys[SDL_SCANCODE_S])       state.controls.back  = true;
    if (keys[SDL_SCANCODE_A])       state.controls.left  = true;
    if (keys[SDL_SCANCODE_D])       state.controls.right = true;
    if (keys[SDL_SCANCODE_SPACE])   state.controls.up = true;
    if (keys[SDL_SCANCODE_LCTRL])   state.controls.down = true;

    int dx = 0, dy = 0;
    SDL_GetRelativeMouseState(&dx, &dy);
    state.controls.yaw   -= dx * 300;
    state.controls.pitch -= dy * 300;
    state.controls.pitch = std::clamp(state.controls.pitch, -89000, 89000);
}

void updateCamera(state_t& state){
    const float dt = std::max(0.0f, static_cast<float>(state.time.delta.count()) / 1000.0f);
    constexpr float moveSpeed = 3.0f; // world units per second

    constexpr float degToRad = std::numbers::pi_v<float> / 180.0f;
    float pitch = static_cast<float>(state.controls.pitch) * static_cast<float>(aS) * degToRad;
    float yaw = static_cast<float>(state.controls.yaw) * static_cast<float>(aS) * degToRad;
    vec<float, 3> front = {
        std::cos(pitch) * std::cos(yaw),
        std::cos(pitch) * std::sin(yaw),
        std::sin(pitch)
    };
    vec<float, 3> worldUp = {0.0f, 0.0f, 1.0f};
    state.camera.forward = front / front.length();
    state.camera.right = state.camera.forward.cross(worldUp);
    state.camera.right = state.camera.right / state.camera.right.length();
    state.camera.up = state.camera.right.cross(state.camera.forward);
    state.camera.up = state.camera.up / state.camera.up.length();

    vec<float, 3> move = {};
    if(state.controls.left){
        move = move - state.camera.right;
        state.controls.left = false;
    }
    if(state.controls.right){
        move = move + state.camera.right;
        state.controls.right = false;
    }

    if(state.controls.forward){
        move = move + state.camera.forward;
        state.controls.forward = false;
    }
    if(state.controls.back){
        move = move - state.camera.forward;
        state.controls.back = false;
    }

    if(state.controls.up){
        move = move + worldUp;
        state.controls.up = false;
    }
    if(state.controls.down){
        move = move - worldUp;
        state.controls.down = false;
    }

    const float len = move.length();
    if(len > 0.0f){
        move = move / len;
        state.camera.position = state.camera.position + (move * (moveSpeed * dt));
    }
}

void updateMVP(state_t& state){
    constexpr mat<float, 4, 4> idn = {
         1,  0,  0,  0,
         0,  1,  0,  0,
         0,  0,  1,  0,
         0,  0,  0,  1,
    };
    constexpr mat<float, 4, 4> rot = {
         0, -1,  0,  0,
         1,  0,  0,  0,
         0,  0,  1,  0,
         0,  0,  0,  1,
    };
    mat<float, 4, 4> model = idn * rot;

    vec<float, 3> f = state.camera.forward / state.camera.forward.length();
    vec<float, 3> s = state.camera.right / state.camera.right.length();
    vec<float, 3> u = state.camera.up / state.camera.up.length();
    mat<float, 4, 4> view = {
         s[0],  s[1],  s[2], -s.dot(state.camera.position),
         u[0],  u[1],  u[2], -u.dot(state.camera.position),
        -f[0], -f[1], -f[2],  f.dot(state.camera.position),
         0,     0,     0,     1,
    };

    mat<float, 4, 4> proj = {};
    constexpr float fovy = 45 * (std::numbers::pi / 180);
    float aspect = (float)state.width / (float)state.height;
    float nearZ = 0.1f;
    float farZ = 100.0f;
    float g = 1.0f /tan(fovy / 2);
    proj[0][0] = g / aspect;
    proj[1][1] = g;
    proj[2][2] = (farZ + nearZ) / (nearZ - farZ);
    proj[2][3] = (2.0f * farZ * nearZ) / (nearZ - farZ);
    proj[3][2] = -1.0f;

    state.mvp = ((proj * view) * model);
}

void showFramebuffer(state_t& state, const framebuffer_t& fb) {
    SDL_UpdateTexture(state.sdlTexture, nullptr, fb.data.data(), fb.w * fb.bpp);
    SDL_RenderClear(state.sdlRenderer);
    SDL_RenderCopy(state.sdlRenderer, state.sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(state.sdlRenderer);

    state.time.end = std::chrono::high_resolution_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(state.time.end - state.time.start).count();
    SDL_Delay(std::max(state.time.frameTime - static_cast<double>(elapsedMs), 0.0));

    // delta is the full frame time (work + optional delay), used by input/movement.
    state.time.end = std::chrono::high_resolution_clock::now();
    state.time.delta = std::chrono::duration_cast<std::chrono::milliseconds>(state.time.end - state.time.start);
}

void initWindow(state_t& state, framebuffer_t& framebuffer){
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    state.sdlWindow    = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, state.width, state.height, 0);
    state.sdlRenderer  = SDL_CreateRenderer(state.sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    state.sdlTexture   = SDL_CreateTexture(state.sdlRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, framebuffer.w, framebuffer.h);
    SDL_SetRelativeMouseMode(SDL_TRUE);
};

int main(int argc, char** argv) {
    state_t state;
    model_t model(PATH);
    framebuffer_t framebuffer(state.width, state.height);
    framebuffer_t depthbuffer(state.width, state.height);

    int t = omp_get_max_threads();
    std::cout << "system has " << t << " threads" << std::endl;

    initWindow(state, framebuffer);
    while(state.controls.running){
        std::cout << '\r' << "ft: " << state.time.delta.count() << " mS" << std::flush;
        state.time.start = std::chrono::high_resolution_clock::now();
        depthbuffer.clear();
        framebuffer.clear();
        getInput(state);
        updateCamera(state);
        updateMVP(state);
        drawModel(state, framebuffer, depthbuffer, model);
        showFramebuffer(state, framebuffer);
    } std::cout << std::endl << std::flush;

    SDL_DestroyTexture(state.sdlTexture);
    SDL_DestroyRenderer(state.sdlRenderer);
    SDL_DestroyWindow(state.sdlWindow);
    SDL_Quit();

    return 0;
}
