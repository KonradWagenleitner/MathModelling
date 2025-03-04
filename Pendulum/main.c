#include <SDL3/SDL.h>
#include <stdlib.h>
#include <math.h>
#include "include/rope.h"

const float g = 9.81;
const float pi = 3.14159;
double lastTick = 0;

typedef struct Ball {
    SDL_FRect rect;
    SDL_Texture *texture;
} Ball;

typedef struct Pendulum{
    double x0, y0, x1, y1;
    double fi, d_fi;
    double dd_fi_c, dd_fi_n; //dd_fi_c - угловое ускорение в настоящий момент
                             //dd_fi_n - угловое ускорение в на следующей итерации
    double l;
} Pendulum;

Ball *createBall(SDL_Renderer *renderer, const char *file_name, int W, int H) {
    SDL_Surface *surface = SDL_LoadBMP(file_name);
    
    Ball *ball = malloc(sizeof(Ball));
    ball->texture = SDL_CreateTextureFromSurface(renderer, surface);
    ball->rect.w = W; ball->rect.h = H;

    return ball;
}

void drawBall(SDL_Renderer *renderer, Pendulum* pendulum, Ball *ball) {
    ball->rect.x = (pendulum->x1)-20; ball->rect.y = (pendulum->y1)-20;
    SDL_RenderTexture(renderer, ball->texture, NULL, &(ball->rect));
}

float CalculateAngles(float delta_t, Pendulum *pendulum) {
    pendulum->dd_fi_c = -(g/pendulum->l) * sin(pendulum->fi);
    pendulum->fi = pendulum->fi + pendulum->d_fi * delta_t + 0.5 * pendulum->dd_fi_c * delta_t * delta_t;
    pendulum->dd_fi_n = -(g/pendulum->l) * sin(pendulum->fi);

    pendulum->d_fi = pendulum->d_fi + 0.5 * (pendulum->dd_fi_c + pendulum->dd_fi_n) * delta_t;
}

void drawPendulum(SDL_Renderer *renderer, Pendulum *pendulum, Ball *ball, float dt_step) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    double currentTick = SDL_GetTicks();

    CalculateAngles((currentTick - lastTick ) * dt_step, pendulum);

    pendulum->x1 = pendulum->x0 + (pendulum->l*sin(pendulum->fi));
    pendulum->y1 = pendulum->y0 + (pendulum->l*cos(pendulum->fi));

    lastTick = currentTick;

    SDL_RenderLine(renderer, pendulum->x0, pendulum->y0, pendulum->x1, pendulum->y1);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    
    drawBall(renderer, pendulum, ball);
}  

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *Window = SDL_CreateWindow("Jopa_Slona(Math Model)", 1000, 800, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *Renderer = SDL_CreateRenderer(Window, NULL);
    SDL_SetRenderVSync(Renderer, 1);

    bool running = true;

    Pendulum pendulum = {500, 100, 500, 400, (3.14/3), 0, 0};
    pendulum.l = sqrt(pow((pendulum.x1-pendulum.x0), 2) + pow((pendulum.y1-pendulum.y0), 2));

    Ball *ball = createBall(Renderer, "src/blue_ball_DGU.bmp", 40, 40);

    SDL_FRect Ceil = {450, 290, 100, 10};

    while (running) {
        SDL_Event event;

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }
        }

        SDL_SetRenderDrawColor(Renderer, 255, 255, 255, 255);
        SDL_RenderClear(Renderer);

        drawPendulum(Renderer, &pendulum, ball, 0.015);

        SDL_RenderPresent(Renderer);
        SDL_Delay(16); 
    }
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    
    SDL_Quit();
    return 0;
}