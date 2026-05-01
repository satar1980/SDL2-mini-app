// 15puzzle.c
// Compile: gcc -o 15puzzle 15puzzle.c -lSDL2 -lSDL2_ttf -lm
// Run: ./15puzzle

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Constants
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 480
#define BOARD_SIZE 4
#define TILE_SIZE 80
#define TILE_MARGIN 8
#define BOARD_OFFSET_X 25
#define BOARD_OFFSET_Y 120
#define ANIMATION_SPEED 12.0f

// Colors
#define COLOR_BACKGROUND 52, 73, 94
#define COLOR_TILE 236, 240, 241
#define COLOR_TILE_HOVER 189, 195, 199
#define COLOR_TEXT 44, 62, 80
#define COLOR_EMPTY 26, 37, 50
#define COLOR_WIN_TEXT 46, 204, 113
#define COLOR_TITLE 255, 255, 255

typedef struct {
    int x, y;
    int value;
    int target_x, target_y;
    bool animating;
    float anim_progress;
} Tile;

typedef struct {
    Tile tiles[BOARD_SIZE][BOARD_SIZE];
    int empty_x, empty_y;
    bool solved;
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* tile_textures[BOARD_SIZE * BOARD_SIZE];
    SDL_Texture* win_texture;
    int hover_tile_x, hover_tile_y;
    bool mouse_pressed;
    int animating_count;
} Game;

// Function prototypes
void init_game(Game* game, SDL_Renderer* renderer, TTF_Font* font);
void shuffle_board(Game* game, int moves);
bool is_solved(Game* game);
bool is_move_valid(Game* game, int tile_x, int tile_y);
void swap_tiles(Game* game, int x1, int y1, int x2, int y2);
bool try_move_tile(Game* game, int tile_x, int tile_y);
void update_animation(Game* game);
void render_tile(Game* game, int x, int y);
void render(Game* game);
void handle_events(Game* game, bool* running);
void create_tile_textures(Game* game);
void free_tile_textures(Game* game);
void update_win_texture(Game* game);

SDL_Texture* create_text_texture(SDL_Renderer* renderer, TTF_Font* font, 
                                  const char* text, SDL_Color color) {
    if (!font) return NULL;
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) {
        printf("TTF_RenderUTF8_Blended error: %s\n", TTF_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void init_game(Game* game, SDL_Renderer* renderer, TTF_Font* font) {
    game->renderer = renderer;
    game->font = font;
    game->solved = false;
    game->hover_tile_x = -1;
    game->hover_tile_y = -1;
    game->mouse_pressed = false;
    game->animating_count = 0;
    game->win_texture = NULL;
    
    // Initialize tiles
    int value = 1;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            game->tiles[y][x].x = x;
            game->tiles[y][x].y = y;
            game->tiles[y][x].target_x = x;
            game->tiles[y][x].target_y = y;
            game->tiles[y][x].value = value;
            game->tiles[y][x].animating = false;
            game->tiles[y][x].anim_progress = 1.0f;
            value++;
        }
    }
    game->tiles[BOARD_SIZE-1][BOARD_SIZE-1].value = 0;
    game->empty_x = BOARD_SIZE - 1;
    game->empty_y = BOARD_SIZE - 1;
    
    create_tile_textures(game);
    shuffle_board(game, 200);
}

void create_tile_textures(Game* game) {
    SDL_Color text_color = {COLOR_TEXT};
    
    // Clear old textures
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
        if (game->tile_textures[i]) {
            SDL_DestroyTexture(game->tile_textures[i]);
        }
    }
    
    // Create new textures
    for (int i = 1; i < BOARD_SIZE * BOARD_SIZE; i++) {
        char num_str[4];
        snprintf(num_str, sizeof(num_str), "%d", i);
        
        game->tile_textures[i] = create_text_texture(
            game->renderer, game->font, num_str, text_color
        );
        
        if (!game->tile_textures[i]) {
            printf("Failed to create texture for %d\n", i);
        }
    }
    game->tile_textures[0] = NULL;
}

void free_tile_textures(Game* game) {
    for (int i = 1; i < BOARD_SIZE * BOARD_SIZE; i++) {
        if (game->tile_textures[i]) {
            SDL_DestroyTexture(game->tile_textures[i]);
            game->tile_textures[i] = NULL;
        }
    }
    if (game->win_texture) {
        SDL_DestroyTexture(game->win_texture);
        game->win_texture = NULL;
    }
}

void update_win_texture(Game* game) {
    if (game->win_texture) {
        SDL_DestroyTexture(game->win_texture);
        game->win_texture = NULL;
    }
    
    if (game->solved && game->font) {
        SDL_Color win_color = {COLOR_WIN_TEXT};
        game->win_texture = create_text_texture(
            game->renderer, game->font, "YOU WIN!", win_color
        );
    }
}

void shuffle_board(Game* game, int moves) {
    srand(time(NULL));
    int last_move = -1;
    
    for (int i = 0; i < moves; i++) {
        int possible_moves[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        int valid_moves[4];
        int valid_count = 0;
        
        for (int m = 0; m < 4; m++) {
            int new_x = game->empty_x + possible_moves[m][0];
            int new_y = game->empty_y + possible_moves[m][1];
            
            if (new_x >= 0 && new_x < BOARD_SIZE && 
                new_y >= 0 && new_y < BOARD_SIZE) {
                bool is_undo = false;
                if (last_move != -1) {
                    int undo_x = game->empty_x - possible_moves[last_move][0];
                    int undo_y = game->empty_y - possible_moves[last_move][1];
                    if (new_x == undo_x && new_y == undo_y) {
                        is_undo = true;
                    }
                }
                if (!is_undo) {
                    valid_moves[valid_count++] = m;
                }
            }
        }
        
        if (valid_count > 0) {
            int move = valid_moves[rand() % valid_count];
            int tile_x = game->empty_x + possible_moves[move][0];
            int tile_y = game->empty_y + possible_moves[move][1];
            
            swap_tiles(game, tile_x, tile_y, game->empty_x, game->empty_y);
            game->empty_x = tile_x;
            game->empty_y = tile_y;
            last_move = move;
        }
    }
    
    game->solved = is_solved(game);
    update_win_texture(game);
}

bool is_solved(Game* game) {
    int expected = 1;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (game->tiles[y][x].value != expected) {
                if (y == BOARD_SIZE-1 && x == BOARD_SIZE-1) {
                    return game->tiles[y][x].value == 0;
                }
                return false;
            }
            expected++;
            if (expected == BOARD_SIZE * BOARD_SIZE) expected = 0;
        }
    }
    return true;
}

bool is_move_valid(Game* game, int tile_x, int tile_y) {
    if (tile_x < 0 || tile_x >= BOARD_SIZE || 
        tile_y < 0 || tile_y >= BOARD_SIZE) {
        return false;
    }
    
    int dx = abs(tile_x - game->empty_x);
    int dy = abs(tile_y - game->empty_y);
    return (dx + dy) == 1;
}

void swap_tiles(Game* game, int x1, int y1, int x2, int y2) {
    Tile temp = game->tiles[y1][x1];
    game->tiles[y1][x1] = game->tiles[y2][x2];
    game->tiles[y2][x2] = temp;
    
    game->tiles[y1][x1].x = x1;
    game->tiles[y1][x1].y = y1;
    game->tiles[y2][x2].x = x2;
    game->tiles[y2][x2].y = y2;
}

bool try_move_tile(Game* game, int tile_x, int tile_y) {
    if (game->solved) return false;
    if (game->tiles[tile_y][tile_x].value == 0) return false;
    if (!is_move_valid(game, tile_x, tile_y)) return false;
    
    Tile* tile = &game->tiles[tile_y][tile_x];
    tile->target_x = game->empty_x;
    tile->target_y = game->empty_y;
    tile->animating = true;
    tile->anim_progress = 0.0f;
    game->animating_count++;
    
    game->empty_x = tile_x;
    game->empty_y = tile_y;
    
    return true;
}

void update_animation(Game* game) {
    bool any_animating = false;
    
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Tile* tile = &game->tiles[y][x];
            if (tile->animating) {
                tile->anim_progress += ANIMATION_SPEED / 60.0f;
                if (tile->anim_progress >= 1.0f) {
                    tile->anim_progress = 1.0f;
                    tile->animating = false;
                    
                    swap_tiles(game, x, y, tile->target_x, tile->target_y);
                    game->animating_count--;
                } else {
                    any_animating = true;
                }
            }
        }
    }
    
    if (!any_animating && game->animating_count == 0) {
        bool was_solved = game->solved;
        game->solved = is_solved(game);
        if (game->solved != was_solved) {
            update_win_texture(game);
        }
    }
}

int get_tile_at_pos(Game* game, int mouse_x, int mouse_y) {
    int board_start_x = BOARD_OFFSET_X;
    int board_start_y = BOARD_OFFSET_Y;
    
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            int tile_x = board_start_x + x * (TILE_SIZE + TILE_MARGIN);
            int tile_y = board_start_y + y * (TILE_SIZE + TILE_MARGIN);
            
            if (mouse_x >= tile_x && mouse_x <= tile_x + TILE_SIZE &&
                mouse_y >= tile_y && mouse_y <= tile_y + TILE_SIZE) {
                return y * BOARD_SIZE + x;
            }
        }
    }
    return -1;
}

void render_tile(Game* game, int x, int y) {
    Tile* tile = &game->tiles[y][x];
    
    float draw_x = BOARD_OFFSET_X + x * (TILE_SIZE + TILE_MARGIN);
    float draw_y = BOARD_OFFSET_Y + y * (TILE_SIZE + TILE_MARGIN);
    
    if (tile->animating) {
        float progress = tile->anim_progress;
        float start_x = draw_x;
        float start_y = draw_y;
        float end_x = BOARD_OFFSET_X + tile->target_x * (TILE_SIZE + TILE_MARGIN);
        float end_y = BOARD_OFFSET_Y + tile->target_y * (TILE_SIZE + TILE_MARGIN);
        
        draw_x = start_x + (end_x - start_x) * progress;
        draw_y = start_y + (end_y - start_y) * progress;
    }
    
    SDL_Rect rect = {(int)draw_x, (int)draw_y, TILE_SIZE, TILE_SIZE};
    
    // Choose color
    if (tile->value == 0) {
        SDL_SetRenderDrawColor(game->renderer, COLOR_EMPTY, 255);
    } else if (x == game->hover_tile_x && y == game->hover_tile_y && 
               !game->solved && is_move_valid(game, x, y)) {
        SDL_SetRenderDrawColor(game->renderer, COLOR_TILE_HOVER, 255);
    } else {
        SDL_SetRenderDrawColor(game->renderer, COLOR_TILE, 255);
    }
    SDL_RenderFillRect(game->renderer, &rect);
    
    // Draw border
    SDL_SetRenderDrawColor(game->renderer, 127, 140, 141, 255);
    SDL_RenderDrawRect(game->renderer, &rect);
    
    // Draw text
    if (tile->value > 0 && tile->value < BOARD_SIZE * BOARD_SIZE) {
        if (game->tile_textures[tile->value]) {
            int text_w, text_h;
            SDL_QueryTexture(game->tile_textures[tile->value], NULL, NULL, &text_w, &text_h);
            SDL_Rect text_rect = {
                rect.x + (TILE_SIZE - text_w) / 2,
                rect.y + (TILE_SIZE - text_h) / 2,
                text_w, text_h
            };
            SDL_RenderCopy(game->renderer, game->tile_textures[tile->value], NULL, &text_rect);
        } else {
            // Fallback: draw number directly
            char num[4];
            snprintf(num, sizeof(num), "%d", tile->value);
            SDL_SetRenderDrawColor(game->renderer, COLOR_TEXT, 255);
            // Simple text rendering fallback (just draw rectangle)
            SDL_Rect inner = {rect.x + 5, rect.y + 5, TILE_SIZE - 10, TILE_SIZE - 10};
            SDL_RenderDrawRect(game->renderer, &inner);
        }
    }
}

void render(Game* game) {
    // Clear screen with background
    SDL_SetRenderDrawColor(game->renderer, COLOR_BACKGROUND, 255);
    SDL_RenderClear(game->renderer);
    
    // Draw title
    if (game->font) {
        SDL_Color title_color = {COLOR_TITLE};
        SDL_Texture* title_text = create_text_texture(
            game->renderer, game->font, "15 PUZZLE", title_color
        );
        if (title_text) {
            int text_w, text_h;
            SDL_QueryTexture(title_text, NULL, NULL, &text_w, &text_h);
            SDL_Rect text_rect = {(WINDOW_WIDTH - text_w) / 2, 15, text_w, text_h};
            SDL_RenderCopy(game->renderer, title_text, NULL, &text_rect);
            SDL_DestroyTexture(title_text);
        }
    }
    
    // Draw instruction
    if (game->font && !game->solved) {
        SDL_Color instr_color = {COLOR_TILE};
        SDL_Texture* instr_text = create_text_texture(
            game->renderer, game->font, "Press R to shuffle", instr_color
        );
        if (instr_text) {
            int text_w, text_h;
            SDL_QueryTexture(instr_text, NULL, NULL, &text_w, &text_h);
            SDL_Rect text_rect = {(WINDOW_WIDTH - text_w) / 2, 
                                   BOARD_OFFSET_Y - 60, text_w, text_h};
            SDL_RenderCopy(game->renderer, instr_text, NULL, &text_rect);
            SDL_DestroyTexture(instr_text);
        }
    }
    
    // Draw board background
    SDL_Rect board_bg = {
        BOARD_OFFSET_X - 5, BOARD_OFFSET_Y - 5,
        BOARD_SIZE * (TILE_SIZE + TILE_MARGIN) + TILE_MARGIN + 5,
        BOARD_SIZE * (TILE_SIZE + TILE_MARGIN) + TILE_MARGIN + 5
    };
    SDL_SetRenderDrawColor(game->renderer, 52, 73, 94, 255);
    SDL_RenderFillRect(game->renderer, &board_bg);
    
    // Draw tiles
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            render_tile(game, x, y);
        }
    }
    
    // Draw win message with background
    if (game->solved && game->win_texture) {
        int text_w, text_h;
        SDL_QueryTexture(game->win_texture, NULL, NULL, &text_w, &text_h);
        
        // Draw background for win text
        SDL_Rect bg_rect = {
            (WINDOW_WIDTH - text_w - 40) / 2,
            (WINDOW_HEIGHT - text_h - 40) / 2,
            text_w + 40, text_h + 40
        };
        SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 200);
        SDL_RenderFillRect(game->renderer, &bg_rect);
        
        // Draw border
        SDL_SetRenderDrawColor(game->renderer, COLOR_WIN_TEXT, 255);
        SDL_RenderDrawRect(game->renderer, &bg_rect);
        
        // Draw win text
        SDL_Rect text_rect = {
            (WINDOW_WIDTH - text_w) / 2,
            (WINDOW_HEIGHT - text_h) / 2,
            text_w, text_h
        };
        SDL_RenderCopy(game->renderer, game->win_texture, NULL, &text_rect);
    }
    
    SDL_RenderPresent(game->renderer);
}

void handle_events(Game* game, bool* running) {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                *running = false;
                break;
                
            case SDL_MOUSEMOTION:
                if (!game->solved && game->animating_count == 0) {
                    int index = get_tile_at_pos(game, event.motion.x, event.motion.y);
                    if (index >= 0) {
                        int x = index % BOARD_SIZE;
                        int y = index / BOARD_SIZE;
                        if (game->tiles[y][x].value != 0) {
                            game->hover_tile_x = x;
                            game->hover_tile_y = y;
                        } else {
                            game->hover_tile_x = -1;
                            game->hover_tile_y = -1;
                        }
                    } else {
                        game->hover_tile_x = -1;
                        game->hover_tile_y = -1;
                    }
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    game->mouse_pressed = true;
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT && game->mouse_pressed) {
                    if (!game->solved && game->animating_count == 0) {
                        int index = get_tile_at_pos(game, event.button.x, event.button.y);
                        if (index >= 0) {
                            int tile_x = index % BOARD_SIZE;
                            int tile_y = index / BOARD_SIZE;
                            if (game->tiles[tile_y][tile_x].value != 0) {
                                try_move_tile(game, tile_x, tile_y);
                            }
                        }
                    }
                    game->mouse_pressed = false;
                }
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_r) {
                    shuffle_board(game, 200);
                    game->solved = false;
                    game->hover_tile_x = -1;
                    game->hover_tile_y = -1;
                    update_win_texture(game);
                }
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        printf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "15 Puzzle - Sliding Puzzle",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Try to find and load font
    TTF_Font* font = NULL;
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "C:\\Windows\\Fonts\\Arial.ttf",
        "C:\\Windows\\Fonts\\SegoeUI.ttf"
    };
    
    for (int i = 0; i < 8; i++) {
        font = TTF_OpenFont(font_paths[i], 40);
        if (font) {
            printf("Loaded font: %s\n", font_paths[i]);
            break;
        }
    }
    
    if (!font) {
        printf("Warning: No font found. Text will not display.\n");
        printf("Install a font or adjust font path in the code.\n");
    }
    
    // Initialize game
    Game game;
    memset(&game, 0, sizeof(Game));
    init_game(&game, renderer, font);
    
    // Main loop
    bool running = true;
    Uint32 last_time = SDL_GetTicks();
    
    while (running) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        handle_events(&game, &running);
        update_animation(&game);
        render(&game);
        
        SDL_Delay(16);
    }
    
    // Cleanup
    free_tile_textures(&game);
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}
