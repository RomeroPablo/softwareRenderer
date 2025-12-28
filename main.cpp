#include <SDL2/SDL_video.h>
#include <cassert>
#include <cmath>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>
#include <SDL2/SDL.h>
#include "tgaimage.h"

#define PATH "../assets/demon.obj"
#define WIDTH 640
#define HEIGHT 480

constexpr TGAColor 
    white  = {255, 255, 255, 255}, 
    green  = {  0, 255,   0, 255}, 
    red    = {  0,   0, 255, 255}, 
    blue   = {255, 128,  64, 255}, 
    yellow = {  0, 200, 255, 255};

struct vCoord { float x{}, y{}, z{};  } typedef vCoord_t;
struct face   { size_t a{}, b{}, c{}; } typedef face_t;
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
                vertices.emplace_back((vCoord_t){x, y, z});
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
    std::vector<vCoord_t> vertices;
    std::vector<face_t> faces;
} typedef model_t;

struct state{
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;
    bool running = true;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    struct{
        int x;
    } mvp ;
}typedef state_t;

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs (ax-bx) < std::abs(ay-by);
    if(steep) <% std::swap(ax, ay); std::swap(bx, by); %>
    if(ax>bx) <% std::swap(ax, bx); std::swap(ay, by); %>

    float y = ay;
    const float f = (by-ay) / static_cast<float>(bx-ax);

    if(steep)<% for(size_t x=ax; x<=bx; x++, y+=f) framebuffer.set(y, x, color); %>
    else     <% for(size_t x=ax; x<=bx; x++, y+=f) framebuffer.set(x, y, color); %>
}

inline double hP(double x){ return (x + 1.) * WIDTH  /2; } // if a -0.1, if d + 0.1
inline double vP(double y){ return (1. - y) * HEIGHT /2; } // if w -0.1, if s + 0.1

void drawModel(state_t& state, TGAImage& framebuffer, const model_t& model){
    for(const auto& f : model.faces){
        double ax = hP(model.vertices[f.a-1].x), ay = vP(model.vertices[f.a-1].y);
        double bx = hP(model.vertices[f.b-1].x), by = vP(model.vertices[f.b-1].y);
        double cx = hP(model.vertices[f.c-1].x), cy = vP(model.vertices[f.c-1].y);
        line(ax, ay, bx, by, framebuffer, blue);
        line(cx, cy, bx, by, framebuffer, green);
        line(ax, ay, cx, cy, framebuffer, red);
    }
}

void getInput(state_t& state){
    SDL_Event e; SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) state.running = false;
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    state.up    = keys[SDL_SCANCODE_W];
    state.down  = keys[SDL_SCANCODE_S];
    state.left  = keys[SDL_SCANCODE_A];
    state.right = keys[SDL_SCANCODE_D];
}

void showFramebuffer(state_t& state, const TGAImage& fb) {
    SDL_UpdateTexture(state.sdlTexture, nullptr, fb.getData(), fb.width() * fb.bytesPerPixel());
    SDL_RenderClear(state.sdlRenderer);
    SDL_RenderCopy(state.sdlRenderer, state.sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(state.sdlRenderer);
    SDL_Delay(7); // ~60 FPS
}

int main(int argc, char** argv) {
    model_t model(PATH);
    TGAImage framebuffer(WIDTH, HEIGHT, TGAImage::RGB);
    state_t state;
    SDL_Init(SDL_INIT_VIDEO);
    state.sdlWindow    = SDL_CreateWindow("trr", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    state.sdlRenderer  = SDL_CreateRenderer(state.sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    state.sdlTexture   = SDL_CreateTexture(state.sdlRenderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, framebuffer.width(), framebuffer.height());

    std::chrono::time_point<std::chrono::high_resolution_clock> s<%%>, e<%%>;
    while(state.running){
        std::cout << '\r' << "ft: " << std::chrono::duration_cast<std::chrono::microseconds>(e-s).count() << " μS" << std::flush;
        s = std::chrono::high_resolution_clock::now();

        framebuffer.clear_fb();
        getInput(state);
        drawModel(state, framebuffer, model);
        showFramebuffer(state, framebuffer);

        e = std::chrono::high_resolution_clock::now();
    }

    framebuffer.flip_vertically();
    framebuffer.write_tga_file("framebuffer.tga");

    SDL_DestroyTexture(state.sdlTexture);
    SDL_DestroyRenderer(state.sdlRenderer);
    SDL_DestroyWindow(state.sdlWindow);
    SDL_Quit();

    return 0;
}
