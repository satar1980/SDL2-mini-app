// scientific_calculator.c
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 800
#define HEIGHT 600
#define PI 3.14159265359

typedef struct {
    double x, y;
    char expr[256];
    int expr_len;
    double memory;
} CalculatorState;

void draw_axes(SDL_Renderer *renderer, int width, int height) {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    
    // Sumbu X
    SDL_RenderDrawLine(renderer, 0, height/2, width, height/2);
    // Sumbu Y
    SDL_RenderDrawLine(renderer, width/2, 0, width/2, height);
}

void draw_function(SDL_Renderer *renderer, double (*func)(double), int width, int height) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    
    double x_scale = 4.0 / width;      // -2 to 2
    double y_scale = 4.0 / height;     // -2 to 2
    
    int prev_x = -1, prev_y = -1;
    
    for (int screen_x = 0; screen_x < width; screen_x++) {
        double world_x = (screen_x - width/2) * x_scale;
        double world_y = func(world_x);
        int screen_y = height/2 - (world_y / y_scale);
        
        if (screen_y >= 0 && screen_y < height) {
            if (prev_x != -1) {
                SDL_RenderDrawLine(renderer, prev_x, prev_y, screen_x, screen_y);
            }
            prev_x = screen_x;
            prev_y = screen_y;
        } else {
            prev_x = -1;
        }
    }
}

double f1(double x) { return sin(x); }
double f2(double x) { return cos(x); }
double f3(double x) { return tan(x); }
double f4(double x) { return x * x; }
double f5(double x) { return sqrt(fabs(x)); }

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Kalkulator Grafik", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    double (*functions[])(double) = {f1, f2, f3, f4, f5};
    const char *names[] = {"sin(x)", "cos(x)", "tan(x)", "x^2", "sqrt(|x|)"};
    int current_func = 0;
    int running = 1;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_UP: current_func = (current_func + 1) % 5; break;
                    case SDLK_DOWN: current_func = (current_func - 1 + 5) % 5; break;
                    case SDLK_ESCAPE: running = 0; break;
                }
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        draw_axes(renderer, WIDTH, HEIGHT);
        draw_function(renderer, functions[current_func], WIDTH, HEIGHT);
        
        // Tampilkan nama fungsi
        char title[256];
        snprintf(title, sizeof(title), "Fungsi: %s (Gunakan panah atas/bawah untuk ganti)", names[current_func]);
        SDL_SetWindowTitle(window, title);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
