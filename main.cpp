#include <SDL2/SDL.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "geometry.hpp"

constexpr const char* PATH = "../assets/demon.obj";
//constexpr const char* PATH = "../assets/hunter.obj";
//constexpr const char* PATH = "../assets/weep.obj";
//constexpr const char* PATH = "../assets/dionysos.obj";
constexpr uint16_t WIDTH  = 640;
constexpr uint16_t HEIGHT = 480;
constexpr uint16_t DEPTH  = 255;

using color_t = 
struct color {
    uint8_t rgba[4] = {0,0,0,0};
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

using framebuffer_t =
struct framebuffer {
    framebuffer(int w, int h) : w(w), h(h), data(w*h*bpp, 0) {
        for (int j=0; j<h; j++)
            for (int i=0; i<w; i++)
                set(i, j, {});
    }
    void set(int x, int y, const color_t& color){
        if (!data.size() || x<0 || y<0 || x>=w || y>=h) return;
        memcpy(data.data()+(x+y*w)*bpp, color.rgba, bpp);
    }
    color_t get(int x, int y){
        if (!data.size() || x<0 || y<0 || x>=w || y>=h) return {0,0,0,0};
        color_t ret;
        ret.rgba[0] = data[(x+y*w)*bpp+0];
        ret.rgba[1] = data[(x+y*w)*bpp+1];
        ret.rgba[2] = data[(x+y*w)*bpp+2];
        ret.rgba[3] = data[(x+y*w)*bpp+3];
        return ret;
    }
    void clear(uint8_t c = 0){ std::fill(data.begin(), data.end(), c); }

    int w;
    int h;
    int bpp = 4; // 4 bytes per pixel R, G, B, A
    std::vector<uint8_t> data = {};
};

using vertex_t = struct vertex { float  x{}, y{}, z{}; };
using face_t   = struct face   { size_t a{}, b{}, c{}; };

using model_t =
struct model {
    model(std::string path){
        std::ifstream file(path);
        if(!file) std::cout << "Model Not Found" << std::endl;
        std::string line;
        while(std::getline(file, line)){
            std::istringstream iss(line);
            std::string tag;
            iss >> tag;

            if(tag == "v"){
                float x, y, z;
                iss >> x >> y >> z;
                vertices.emplace_back((vertex_t){x, y, z});
            } else 

            if (tag == "f"){
                size_t a, b, c;
                iss >> a;
                iss.ignore(std::numeric_limits<std::streamsize>::max(), ' '); iss >> b;
                iss.ignore(std::numeric_limits<std::streamsize>::max(), ' '); iss >> c;
                faces.emplace_back((face_t){a, b, c});
            }
        }
    }
    std::vector<vertex_t> vertices;
    std::vector<face_t> faces;
};

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
        std::chrono::time_point<std::chrono::high_resolution_clock> s<%%>, e<%%>;
        std::chrono::milliseconds d<%%>;
        double ft = 1000.0/60.0; // 60 fps
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
            if(z <= depthbuffer.get(x, y).rgba[0]) continue;
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
    state.time.e = std::chrono::high_resolution_clock::now();
    state.time.d = std::chrono::duration_cast<std::chrono::milliseconds>(state.time.e - state.time.s);
    SDL_Delay(std::max(state.time.ft - state.time.d.count(), 0.0));
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

int main(int argc, char** argv) {
    model_t model(PATH);
    framebuffer_t framebuffer(WIDTH, HEIGHT);
    framebuffer_t depthbuffer(WIDTH, HEIGHT);
    state_t state;

    vec<int, 2> v = {12, 13};
    mat<int, 2, 2> matrix;
    matrix[0] = {1,0};
    matrix[1] = {0,1};
    v = matrix * v;

    std::cout << v[0] << ' ' << v[1] << std::endl;

    SDL_Init(SDL_INIT_VIDEO);
    state.sdlWindow    = SDL_CreateWindow("trr", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    state.sdlRenderer  = SDL_CreateRenderer(state.sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    state.sdlTexture   = SDL_CreateTexture(state.sdlRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, framebuffer.w, framebuffer.h);

    int t = omp_get_max_threads();
    std::cout << "system has " << t << " threads" << std::endl;

    while(state.controls.running){
        std::cout << '\r' << "ft: " << state.time.d.count() << " mS" << std::flush;
        state.time.s = std::chrono::high_resolution_clock::now();

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
