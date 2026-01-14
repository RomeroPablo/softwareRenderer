#include <SDL2/SDL.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <omp.h>
#include <string>
#include <utility>
#include <vector>

#include "framebuffer.hpp"
#include "geometry.hpp"
#include "model.hpp"

constexpr const char* PATH = "../assets/demon.obj";
//constexpr const char* PATH = "../assets/hunter.obj";
//constexpr const char* PATH = "../assets/weep.obj";
//constexpr const char* PATH = "../assets/dionysos.obj";
constexpr uint16_t WIDTH  = 640;
constexpr uint16_t HEIGHT = 480;
constexpr uint16_t DEPTH  = 255;

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
    struct{
        double x = 0.;
        double y = 0.;
        double z = 0.;
    } camera;
    struct {
        std::chrono::time_point<std::chrono::high_resolution_clock> start<%%>, end<%%>;
        std::chrono::milliseconds delta<%%>;
        double frameTime = 1000.0/60.0; // 60 fps
    } time;
};

void line(int ax, int ay, int bx, int by, framebuffer_t &framebuffer, color_t color) {
    bool steep = std::abs (ax-bx) < std::abs(ay-by);
    if(steep) <% std::swap(ax, ay); std::swap(bx, by); %>
    if(ax>bx) <% std::swap(ax, bx); std::swap(ay, by); %>

    float y = ay;
    const float f = (by-ay) / static_cast<float>(bx-ax);

    if(steep)<% for(size_t x=ax; x<=bx; x++, y+=f) framebuffer.set(y, x, color); %>
    else     <% for(size_t x=ax; x<=bx; x++, y+=f) framebuffer.set(x, y, color); %>
}

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
            int x1 = cx + ((ax - cx)*(y - cy)) / total_height; // edge line lerp
            int x2 = cx + ((bx - cx)*(y - cy)) / base_height;
            for(int x=std::min(x1,x2); x<std::max(x1,x2); x++) // draw horizontal line edge-to-edge
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

double inline tArea(int ax, int ay, int bx, int by, int cx, int cy){
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

void rasterOMP(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, framebuffer_t &framebuffer, framebuffer_t &depthbuffer) {
    double area = tArea(ax, ay, bx, by, cx, cy);
    //if(area<1) return; // backface & area culling

    int bbXMin = std::min(std::min(ax, bx), cx);
    int bbXMax = std::max(std::max(ax, bx), cx);
    int bbYMin = std::min(std::min(ay, by), cy);
    int bbYMax = std::max(std::max(ay, by), cy);

// #pragma omp parallel for // this kills perf
    for(int x = bbXMin; x < bbXMax; x++){
        for(int y = bbYMin; y < bbYMax; y++){
            double α = tArea(x, y, bx, by, cx, cy) / area;
            double β = tArea(x, y, cx, cy, ax, ay) / area;
            double γ = tArea(x, y, ax, ay, bx, by) / area;
            if(α<0|| β<0|| γ<0) continue;

            unsigned char z = static_cast<unsigned char>(α * az + β * bz + γ * cz);
            if(z <= depthbuffer.get(x, y)[0]) continue;
            depthbuffer.set(x, y, {z});
            framebuffer.set(x, y, {z, z, z});
        }
    }
}

inline double hP(double x){ return std::clamp((x + 1.) * WIDTH /2, 0.0, double(WIDTH ));}
inline double vP(double y){ return std::clamp((1. - y) * HEIGHT/2, 0.0, double(HEIGHT));}
inline double dP(double z){ return std::clamp((z + 1.) * DEPTH /2, 0.0, double(DEPTH));}

void drawModel(state_t& state, framebuffer_t& framebuffer, framebuffer_t& depthbuffer, const model_t& model){
    // instead of passing state, we should simply push the MVP matrix
    int j = 0;
    for(const auto& f : model.faces){
        double ax = hP(model.vertices[f.a-1].x + state.camera.x), ay = vP(model.vertices[f.a-1].y + state.camera.y);
        double bx = hP(model.vertices[f.b-1].x + state.camera.x), by = vP(model.vertices[f.b-1].y + state.camera.y);
        double cx = hP(model.vertices[f.c-1].x + state.camera.x), cy = vP(model.vertices[f.c-1].y + state.camera.y);
        double az = dP(model.vertices[f.a-1].z + state.camera.z);
        double bz = dP(model.vertices[f.b-1].z + state.camera.z);
        double cz = dP(model.vertices[f.c-1].z + state.camera.z);

        rasterOMP(ax, ay, az, bx, by, bz, cx, cy, cz, framebuffer, depthbuffer);
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
    if(state.controls.up)    state.camera.z += 0.1;
    if(state.controls.down)  state.camera.z -= 0.1;
    if(state.controls.left)  state.camera.x -= 0.1;
    if(state.controls.right) state.camera.x += 0.1;
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
        int ax = rand()%WIDTH, ay = rand()%HEIGHT;
        int bx = rand()%WIDTH, by = rand()%HEIGHT;
        line(ax, ay, bx, by, framebuffer,
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
