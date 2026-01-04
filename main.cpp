#include <SDL2/SDL.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <omp.h>
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
    yellow = {  0, 200, 255, 255},
    purple = {150,  50,  70, 255},
    pink   = {150,  50, 175, 255},
    teal   = {170, 215,  10, 255};

constexpr color_t colors[] = {red, green, blue, pink, teal, purple};

struct framebuffer{
    framebuffer(int w, int h) : w(w), h(h), data(w*h*bpp, 0) {
        for (int j=0; j<h; j++)
            for (int i=0; i<w; i++)
                set(i, j, {});
    }
    void set(int x, int y, const color_t& color){
        if (!data.size() || x<0 || y<0 || x>=w || y>=h) return;
        memcpy(data.data()+(x+y*w)*bpp, color.bgra, bpp);
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
    } mvp;
    struct {
        std::chrono::time_point<std::chrono::high_resolution_clock> s<%%>, e<%%>;
        std::chrono::milliseconds d<%%>;
        double frameTime = 1000.0/60.0; // 60 fps
    } time;
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

void raster(int ax, int ay, int bx, int by, int cx, int cy, framebuffer_t &framebuffer, color_t color) {
    // gtl, where g -> a, b, c <- l
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

    // other triangle
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

void rasterOMP(int ax, int ay, int bx, int by, int cx, int cy, framebuffer_t &framebuffer, color_t color) {
    int bbXMin = std::min(std::min(ax, bx), cx);
    int bbXMax = std::max(std::max(ax, bx), cx);
    int bbYMin = std::min(std::min(ay, by), cy);
    int bbYMax = std::max(std::max(ay, by), cy);

    double area = tArea(ax, ay, bx, by, cx, cy);
    if(area<1) return; // backface culling


//#pragma omp parallel for // this kills perf
    for(int x = bbXMin; x < bbXMax; x++){
        for(int y = bbYMin; y < bbYMax; y++){
            double α = tArea(x, y, bx, by, cx, cy) / area;
            double β = tArea(x, y, cx, cy, ax, ay) / area;
            double γ = tArea(x, y, ax, ay, bx, by) / area;
            if(α<0|| β<0|| γ<0) continue;
            framebuffer.set(x, y, color);
        }
    }
}

void drawTriangles(framebuffer_t& framebuffer){
    rasterOMP(  7, 45, 35, 100, 45,  60, framebuffer, red);
    rasterOMP(120, 35, 90,   5, 45, 110, framebuffer, white);
    rasterOMP(115, 83, 80,  90, 85, 120, framebuffer, green);
}

inline double hP(double x){ return std::clamp((x + 1.) * WIDTH /2, 0.0, double(WIDTH ));}
inline double vP(double y){ return std::clamp((1. - y) * HEIGHT/2, 0.0, double(HEIGHT));}

void drawModel(state_t& state, framebuffer_t& framebuffer, const model_t& model){
    int j = 0;
    for(const auto& f : model.faces){
        double ax = hP(model.vertices[f.a-1].x + state.mvp.x), ay = vP(model.vertices[f.a-1].y + state.mvp.y);
        double bx = hP(model.vertices[f.b-1].x + state.mvp.x), by = vP(model.vertices[f.b-1].y + state.mvp.y);
        double cx = hP(model.vertices[f.c-1].x + state.mvp.x), cy = vP(model.vertices[f.c-1].y + state.mvp.y);
        rasterOMP(ax, ay, bx, by, cx, cy, framebuffer, colors[j]);
        j = (j+1)%6; // this is a bug but looks cool
    }
}

void getInput(state_t& state){
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        switch (e.type){
            case SDL_QUIT:
                state.running = false;
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE) state.running = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_W) state.up    = true;
                if (e.key.keysym.scancode == SDL_SCANCODE_S) state.down  = true;
                if (e.key.keysym.scancode == SDL_SCANCODE_A) state.left  = true;
                if (e.key.keysym.scancode == SDL_SCANCODE_D) state.right = true;
                break;
            case SDL_KEYUP:
                if (e.key.keysym.scancode == SDL_SCANCODE_W) state.up    = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_S) state.down  = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_A) state.left  = false;
                if (e.key.keysym.scancode == SDL_SCANCODE_D) state.right = false;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST){
                    state.up = state.down = state.left = state.right = false; // drop stale key state when focus leaves
                }
                break;
            default: break;
        }
    }
    if(state.up)    state.mvp.y += 0.1;
    if(state.down)  state.mvp.y -= 0.1;
    if(state.left)  state.mvp.x -= 0.1;
    if(state.right) state.mvp.x += 0.1;
}

void showFramebuffer(state_t& state, const framebuffer_t& fb) {
    SDL_UpdateTexture(state.sdlTexture, nullptr, fb.data.data(), fb.w * fb.bpp);
    SDL_RenderClear(state.sdlRenderer);
    SDL_RenderCopy(state.sdlRenderer, state.sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(state.sdlRenderer);
    state.time.e = std::chrono::high_resolution_clock::now();
    state.time.d = std::chrono::duration_cast<std::chrono::milliseconds>(state.time.e - state.time.s);
    SDL_Delay(std::max(state.time.frameTime - state.time.d.count(), 0.0));
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
    state_t state;
    SDL_Init(SDL_INIT_VIDEO);
    state.sdlWindow    = SDL_CreateWindow("trr", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    state.sdlRenderer  = SDL_CreateRenderer(state.sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    state.sdlTexture   = SDL_CreateTexture(state.sdlRenderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, framebuffer.w, framebuffer.h);

    int t = omp_get_max_threads();
    std::cout << "system has " << t << " threads" << std::endl;

//  benchmark();
    while(state.running){
        std::cout << '\r' << "ft: " << state.time.d.count() << " mS" << std::flush;
        state.time.s = std::chrono::high_resolution_clock::now();

        framebuffer.clear();
        getInput(state);
        drawModel(state, framebuffer, model);
        //drawTriangles(framebuffer);
        showFramebuffer(state, framebuffer);

    } std::cout << std::endl << std::flush;

    SDL_DestroyTexture(state.sdlTexture);
    SDL_DestroyRenderer(state.sdlRenderer);
    SDL_DestroyWindow(state.sdlWindow);
    SDL_Quit();

    return 0;
}
