#include <SDL2/SDL.h>

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

#include "framebuffer.hpp"
#include "geometry.hpp"
#include "model.hpp"

constexpr const char* PATH = "../assets/demon.obj";
//constexpr const char* PATH = "../assets/weep.obj";
//constexpr const char* PATH = "../assets/hunter.obj";
//constexpr const char* PATH = "../assets/dionysos.obj";
constexpr uint16_t WIDTH  = 640;
constexpr uint16_t HEIGHT = 480;
constexpr uint16_t DEPTH  = 128;

using state_t =
struct state { 
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;

    struct {
        bool running = true;
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
    } controls;

    vec<float, 4> camera{};

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
    //if(area<1) return; // backface & area culling

    int bbXMin = std::min(std::min(a[0], b[0]), c[0]);
    int bbXMax = std::max(std::max(a[0], b[0]), c[0]);
    int bbYMin = std::min(std::min(a[1], b[1]), c[1]);
    int bbYMax = std::max(std::max(a[1], b[1]), c[1]);

// #pragma omp parallel for // this kills perf
    for(int x = bbXMin; x < bbXMax; x++){
        for(int y = bbYMin; y < bbYMax; y++){
            double α = tArea(vec<int, 2>{x, y}, vec<int, 2>{b[0], b[1]}, vec<int, 2>{c[0], c[1]}) / area;
            double β = tArea(vec<int, 2>{x, y}, vec<int, 2>{c[0], c[1]}, vec<int, 2>{a[0], a[1]}) / area;
            double γ = tArea(vec<int, 2>{x, y}, vec<int, 2>{a[0], a[1]}, vec<int, 2>{b[0], b[1]}) / area;
            if(α<0|| β<0|| γ<0) continue;

            unsigned char z = static_cast<unsigned char>(α * a[2] + β * b[2] + γ * c[2]);
            if(z <= depthbuffer.get(x, y)[0]) continue;
            depthbuffer.set(x, y, {z});
            framebuffer.set(x, y, {z, z, z});
        }
    }
}

inline vec<int, 3> vmvp(vec<float, 3> a, vec<float, 4> camera){
    vec<float, 4> p = {a[0], a[1], a[2], 1};

    float xt = camera[0] * std::numbers::inv_pi;
    float yt = camera[2] * std::numbers::inv_pi;

    mat<float, 4, 4> rotX = {
         std::cos(xt), 0, std::sin(xt), 0,
                    0, 1,            0, 0,
        -std::sin(xt), 0, std::cos(xt), 0,
                    0, 0,            0, 1,
    };

    mat<float, 4, 4> rotY = {
        1,             0,            0, 0,
        0,  std::cos(yt), std::sin(yt), 0,
        0, -std::sin(yt), std::cos(yt), 0,
        0,             0,            0, 1,
    };

    mat<float, 4, 4> rotation = rotX * rotY;

    p = rotation * p;

    constexpr mat<float, 4, 4> viewport = {
        WIDTH/2.f,           0,          0,  WIDTH/2.f,
                0, -HEIGHT/2.f,          0, HEIGHT/2.f,
                0,           0,  DEPTH/2.f,  DEPTH/2.f,
                0,           0,          0,          1
    };
    p = viewport * p;

    vec<int, 3> b;
    b[0] = p[0];
    b[1] = p[1];
    b[2] = p[2];
    return b;
}

void drawModel(state_t& state, framebuffer_t& framebuffer, framebuffer_t& depthbuffer, const model_t& model){
    for(const auto& f : model.faces){
        vec<int, 3> a = vmvp(model.vertices[f[0]-1], state.camera);
        vec<int, 3> b = vmvp(model.vertices[f[1]-1], state.camera);
        vec<int, 3> c = vmvp(model.vertices[f[2]-1], state.camera);

        rasterOMP(a, b, c, framebuffer, depthbuffer);
    }
}

void getInput(state_t& state){
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        switch (e.type){
            case SDL_QUIT:
                state.controls.running = false;
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE) state.controls.running = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_W) state.controls.up    = true;
                if (e.key.keysym.scancode == SDL_SCANCODE_S) state.controls.down  = true;
                if (e.key.keysym.scancode == SDL_SCANCODE_A) state.controls.left  = true;
                if (e.key.keysym.scancode == SDL_SCANCODE_D) state.controls.right = true;
                break;
            case SDL_KEYUP:
                if (e.key.keysym.scancode == SDL_SCANCODE_W) state.controls.up    = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_S) state.controls.down  = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_A) state.controls.left  = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_D) state.controls.right = false;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST){
                    state.controls.up = state.controls.down = state.controls.left = state.controls.right = false; // drop stale key state when focus leaves
                }
                break;
            default: break;
        }
    }
    if(state.controls.up)    state.camera[2] += 0.1;
    if(state.controls.down)  state.camera[2] -= 0.1;
    if(state.controls.left)  state.camera[0] -= 0.1;
    if(state.controls.right) state.camera[0] += 0.1;
}

void showFramebuffer(state_t& state, const framebuffer_t& fb) {
    SDL_UpdateTexture(state.sdlTexture, nullptr, fb.data.data(), fb.w * fb.bpp);
    SDL_RenderClear(state.sdlRenderer);
    SDL_RenderCopy(state.sdlRenderer, state.sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(state.sdlRenderer);
    state.time.end = std::chrono::high_resolution_clock::now();
    state.time.delta = std::chrono::duration_cast<std::chrono::milliseconds>(state.time.end - state.time.start);
    SDL_Delay(std::max(state.time.frameTime - state.time.delta.count(), 0.0));
}

void benchmark(){
    framebuffer_t framebuffer(WIDTH, HEIGHT);
    std::chrono::time_point<std::chrono::high_resolution_clock> s<%%>, e<%%>;
    s = std::chrono::high_resolution_clock::now();

    std::srand(std::time({}));
    for (int i={}; i<(1<<24); i++) {
        vec<int, 2> a = {rand()%WIDTH, rand()%HEIGHT};
        vec<int, 2> b = {rand()%WIDTH, rand()%HEIGHT};
        line(a, b, framebuffer,
        {static_cast<uint8_t>(rand()%255),  static_cast<uint8_t>(rand()%255), static_cast<uint8_t>(rand()%255), static_cast<uint8_t>(rand()%255)});
    }
    e = std::chrono::high_resolution_clock::now();
    std::cout << "benchmark: " << std::chrono::duration_cast<std::chrono::microseconds>(e-s).count() << " μS" << std::endl;
    std::cout << "terminating" << std::endl;
    assert(0);
}

void initWindow(state_t& state, framebuffer_t& framebuffer){
    SDL_Init(SDL_INIT_VIDEO);
    state.sdlWindow    = SDL_CreateWindow("trr", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    state.sdlRenderer  = SDL_CreateRenderer(state.sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    state.sdlTexture   = SDL_CreateTexture(state.sdlRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, framebuffer.w, framebuffer.h);
};

int main(int argc, char** argv) {
    model_t model(PATH);
    framebuffer_t framebuffer(WIDTH, HEIGHT);
    framebuffer_t depthbuffer(WIDTH, HEIGHT);
    state_t state;

    int t = omp_get_max_threads();
    std::cout << "system has " << t << " threads" << std::endl;

    initWindow(state, framebuffer);

    while(state.controls.running){
        std::cout << '\r' << "ft: " << state.time.delta.count() << " mS" << std::flush;
        state.time.start = std::chrono::high_resolution_clock::now();
        depthbuffer.clear();
        framebuffer.clear();
        getInput(state);
        drawModel(state, framebuffer, depthbuffer, model);
        showFramebuffer(state, framebuffer);
    } std::cout << std::endl << std::flush;

    SDL_DestroyTexture(state.sdlTexture);
    SDL_DestroyRenderer(state.sdlRenderer);
    SDL_DestroyWindow(state.sdlWindow);
    SDL_Quit();

    return 0;
}
