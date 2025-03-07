#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL3_ttf/SDL_ttf.h>

#define RAD_TO_DEG(X) (X * 180 / pi)
#define DEG_TO_RAD(X) (X * pi / 180)

const float g = 9.81;
const float pi = 3.14159;
double lastTick = 0;

char *ftocstr(float value) {
    char *BUF = malloc(sizeof(char)*100);
    snprintf(BUF, 100, "%f", value);
    return BUF;
}

char *concat_str(const char* str1, const char *str2) {
    char *BUF = malloc(sizeof(char)*100);
    snprintf(BUF, 100, str1, str2);
    return BUF;
}

//Структуры и функции касающиеся панели управления маятником
typedef struct Controls {
    float initial_angle;
    float initial_speed;
    float delta_time;
    float *current_damping;
} Controls;

typedef struct Message {
    const char *text;
    int font_size;
    float x, y;
    SDL_Texture *textTexture;
    TTF_Font *font;
    SDL_Color color;
} Message;

Message* createTextBar(SDL_Renderer *renderer, TTF_Font *font, char *text, float x, float y, int size) {
    SDL_Color black = {0, 0, 0, 255};

    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, strlen(text), black);

    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    Message *message = malloc(sizeof(Message));
    message->text = text;
    message->font_size = size;
    message->x = x; message->y = y;
    message->textTexture = textTexture;
    message->font = font;
    message->color = black;

    SDL_DestroySurface(textSurface);

    return message;
}

void drawUpdateText(SDL_Renderer *renderer, Message *message, Controls *ctrls, char *text, float w) {
    SDL_FRect area;
    area.h = message->font_size; area.w = w;
    area.x = message->x; area.y = message->y;

    message->text = text;

    SDL_Surface *textSurface = TTF_RenderText_Solid(message->font, message->text, strlen(message->text), message->color);
    message->textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    
    SDL_RenderTexture(renderer, message->textTexture, NULL, &area);
}

//Структуры и функции касающиеся маятника
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
    float cease_coeff;
} Pendulum;

Ball *createBall(SDL_Renderer *renderer, const char *file_name, int W, int H) {
    SDL_Surface *surface = SDL_LoadBMP(file_name);
    
    Ball *ball = malloc(sizeof(Ball));
    ball->texture = SDL_CreateTextureFromSurface(renderer, surface);
    ball->rect.w = W; ball->rect.h = H;

    return ball;
}

void drawBall(SDL_Renderer *renderer, Pendulum* pendulum, Ball *ball) {
    ball->rect.x = (pendulum->x1)-ball->rect.w/2; ball->rect.y = (pendulum->y1)-ball->rect.h/2;
    SDL_RenderTexture(renderer, ball->texture, NULL, &(ball->rect));
}

float CalculateAngles(float delta_t, Pendulum *pendulum) {
    pendulum->dd_fi_c = -(g/pendulum->l) * sin(pendulum->fi) - pendulum->cease_coeff * pendulum->d_fi;
    pendulum->fi += pendulum->d_fi * delta_t + 0.5 * pendulum->dd_fi_c * delta_t * delta_t;
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
    
    drawBall(renderer, pendulum, ball);
}  

void destroyPendulum(Ball *ball) {
    SDL_DestroyTexture(ball->texture);
    free(ball);
}

void handleInputs(SDL_Event *event, bool *running, Pendulum *pendulum, Controls *controls) {
    while(SDL_PollEvent(event)) {
        switch(event->type) {
            case SDL_EVENT_QUIT:
                *(running) = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                switch (event->key.key) {
                    case SDLK_UP:
                        controls->initial_angle += 1;
                        break;
                    case SDLK_DOWN:
                        controls->initial_angle -= 1;
                        break;
                    case SDLK_RIGHT:
                        controls->initial_speed += 1; 
                        break;
                    case SDLK_LEFT:
                        controls->initial_speed -= 1;
                        break;
                    case SDLK_1:
                        controls->delta_time -= 0.001f;
                        break;
                    case SDLK_2:
                        controls->delta_time += 0.001f; 
                        break;
                    case SDLK_SPACE:
                        pendulum->fi = DEG_TO_RAD(controls->initial_angle);
                        pendulum->d_fi = DEG_TO_RAD(controls->initial_speed);
                        pendulum->dd_fi_c = 0;
                        pendulum->dd_fi_n = 0;
                        break;
                    case SDLK_3:
                        if (pendulum->cease_coeff >= 0) {
                            pendulum->cease_coeff -= 0.001;
                        } 
                        else {
                            pendulum->cease_coeff = 0;
                        }
                        break;
                    case SDLK_4:
                        pendulum->cease_coeff += 0.001;
                }
            break;
        }
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    TTF_Font *Hack_Bold = TTF_OpenFont("src/Hack-Bold.ttf", 24);

    SDL_Window *Window = SDL_CreateWindow("Pendulum(Math Model)", 1000, 800, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *Renderer = SDL_CreateRenderer(Window, NULL);
    SDL_SetRenderVSync(Renderer, 1);

    bool running = true;

    Pendulum pendulum = {500, 100, 500, 400, (pi/4), 0, 0};
    pendulum.l = sqrt(pow((pendulum.x1-pendulum.x0), 2) + pow((pendulum.y1-pendulum.y0), 2));
    pendulum.cease_coeff = 0;

    Ball *ball = createBall(Renderer, "src/blue_ball_DGU.bmp", 40, 40);

    SDL_FRect Ceil = {pendulum.x0 - 10, pendulum.y0-5, 20, 5};

    Controls ctrls = {RAD_TO_DEG(pendulum.fi), RAD_TO_DEG(pendulum.d_fi), 0.015, &(pendulum.cease_coeff)};

    Message *messages[4] = {
        createTextBar(Renderer, Hack_Bold, concat_str("Initial angle: %s", ftocstr(ctrls.initial_angle)), 10, 10, 24),
        createTextBar(Renderer, Hack_Bold, concat_str("Initial angular speed: %s", ftocstr(ctrls.initial_speed)), 10, 34, 24),
        createTextBar(Renderer, Hack_Bold, concat_str("Current integration step: %s", ftocstr(ctrls.delta_time)), 10, 58, 24),
        createTextBar(Renderer, Hack_Bold, concat_str("Current damping: %s", ftocstr(pendulum.cease_coeff)), 10, 82, 24)
    };

    while (running) {
        char *str_ctrls[4] = {
            concat_str("Initial angle: %s", ftocstr(ctrls.initial_angle)),
            concat_str("Initial angular speed: %s", ftocstr(ctrls.initial_speed)),
            concat_str("Current integration step: %s", ftocstr(ctrls.delta_time)),
            concat_str("Current damping: %s", ftocstr(pendulum.cease_coeff))
        };

        SDL_Event *event = malloc(sizeof(SDL_Event));

        handleInputs(event, &running, &pendulum, &ctrls);

        SDL_SetRenderDrawColor(Renderer, 255, 255, 255, 255);
        SDL_RenderClear(Renderer);

        drawPendulum(Renderer, &pendulum, ball, ctrls.delta_time);
        SDL_RenderFillRect(Renderer, &Ceil);

        for(int i = 0; i < sizeof(messages) / sizeof(void*); i++) {
            drawUpdateText(Renderer, messages[i], &ctrls, str_ctrls[i], 400);
            free(str_ctrls[i]);
        }

        SDL_RenderPresent(Renderer);
    }
    TTF_CloseFont(Hack_Bold);
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);

    destroyPendulum(ball);
    
    SDL_Quit();

    for(int i = 0; i < sizeof(messages) / sizeof(void*); i++) {
        free(messages[i]);
    }
    return 0;
}