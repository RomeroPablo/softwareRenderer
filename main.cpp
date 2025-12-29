#include <SDL2/SDL.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define PATH "../assets/demon.obj"
#define WIDTH 640
#define HEIGHT 480

struct color {
    std::uint8_t bgra[4] = {0,0,0,0};
}typedef color_t;

constexpr color_t
    white  = {255, 255, 255, 255}, 
    green  = {  0, 255,   0, 255}, 
    red    = {  0,   0, 255, 255}, 
    blue   = {255, 128,  64, 255}, 
    yellow = {  0, 200, 255, 255};

struct framebuffer{
    framebuffer(int w, int h) : w(w), h(h), data(w*h*bpp, 0) {
        for (int j=0; j<h; j++)
            for (int i=0; i<w; i++)
                set(i, j, {});
    }
    void set(int x, int y, const color_t& c){
        if (!data.size() || x<0 || y<0 || x>=w || y>=h) return;
        memcpy(data.data()+(x+y*w)*bpp, c.bgra, bpp);
    }
    void clear(){ std::fill(data.begin(), data.end(), 0); }

    int w;
    int h;
    int bpp = 3; // 3 bytes per pixel B,G,R : No Alpha Channel ATM
    std::vector<uint8_t> data = {};
} typedef framebuffer_t;

struct vertex { float x{}, y{}, z{};  } typedef vertex_t;
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
        double x = 0.;
        double y = 0.;
    } mvp ;
}typedef state_t;

void line(int ax, int ay, int bx, int by, framebuffer_t &framebuffer, color_t color) {
    bool steep = std::abs (ax-bx) < std::abs(ay-by);
    if(steep) <% std::swap(ax, ay); std::swap(bx, by); %>
    if(ax>bx) <% std::swap(ax, bx); std::swap(ay, by); %>

    float y = ay;
    const float f = (by-ay) / static_cast<float>(bx-ax);

    if(steep)<% for(size_t x=ax; x<=bx; x++, y+=f) framebuffer.set(y, x, color); %>
    else     <% for(size_t x=ax; x<=bx; x++, y+=f) framebuffer.set(x, y, color); %>
}

inline double hP(double x){ return std::clamp((x + 1.) * WIDTH /2, 0.0, double(WIDTH  - 1));}
inline double vP(double y){ return std::clamp((1. - y) * HEIGHT/2, 0.0, double(HEIGHT- 1)); }

void drawModel(state_t& state, framebuffer_t& framebuffer, const model_t& model){
    for(const auto& f : model.faces){
        double ax = hP(model.vertices[f.a-1].x + state.mvp.x), ay = vP(model.vertices[f.a-1].y + state.mvp.y);
        double bx = hP(model.vertices[f.b-1].x + state.mvp.x), by = vP(model.vertices[f.b-1].y + state.mvp.y);
        double cx = hP(model.vertices[f.c-1].x + state.mvp.x), cy = vP(model.vertices[f.c-1].y + state.mvp.y);
        line(ax, ay, bx, by, framebuffer, blue);
        line(cx, cy, bx, by, framebuffer, green);
        line(ax, ay, cx, cy, framebuffer, red);
    }
}

void getInput(state_t& state){
    SDL_Event e; SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) state.running = false;
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    if(keys[SDL_SCANCODE_W]) state.mvp.y += 0.1;
    if(keys[SDL_SCANCODE_S]) state.mvp.y -= 0.1;
    if(keys[SDL_SCANCODE_A]) state.mvp.x -= 0.1;
    if(keys[SDL_SCANCODE_D]) state.mvp.x += 0.1;
}

void showFramebuffer(state_t& state, const framebuffer_t& fb) {
    SDL_UpdateTexture(state.sdlTexture, nullptr, fb.data.data(), fb.w * fb.bpp);
    SDL_RenderClear(state.sdlRenderer);
    SDL_RenderCopy(state.sdlRenderer, state.sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(state.sdlRenderer);
    SDL_Delay(7);
}

int main(int argc, char** argv) {
    model_t model(PATH);
    framebuffer_t framebuffer(WIDTH, HEIGHT);
    state_t state;
    SDL_Init(SDL_INIT_VIDEO);
    state.sdlWindow    = SDL_CreateWindow("trr", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    state.sdlRenderer  = SDL_CreateRenderer(state.sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    state.sdlTexture   = SDL_CreateTexture(state.sdlRenderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, framebuffer.w, framebuffer.h);

    std::chrono::time_point<std::chrono::high_resolution_clock> s<%%>, e<%%>;
    while(state.running){
        std::cout << '\r' << "ft: " << std::chrono::duration_cast<std::chrono::microseconds>(e-s).count() << " μS" << std::flush;
        s = std::chrono::high_resolution_clock::now();

        framebuffer.clear();
        getInput(state);
        drawModel(state, framebuffer, model);
        showFramebuffer(state, framebuffer);

        e = std::chrono::high_resolution_clock::now();
    } std::cout << std::endl << std::flush;

    SDL_DestroyTexture(state.sdlTexture);
    SDL_DestroyRenderer(state.sdlRenderer);
    SDL_DestroyWindow(state.sdlWindow);
    SDL_Quit();

    return 0;
}
