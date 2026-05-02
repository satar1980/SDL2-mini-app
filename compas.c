#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// SSD1306 6x8 font - each character is 6 bytes (6 columns x 8 rows)
const uint8_t font6x8[] = {
    0x00, 0x06, 0x08, 0x20,  // header: version, width, height, first char
    // Space to '~' (95 characters, each 6 bytes = 570 bytes)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sp
    0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, // !
    0x00, 0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14, // #
    0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12, // $
    0x00, 0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x00, 0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x00, 0x1c, 0x22, 0x41, 0x00, // (
    0x00, 0x00, 0x41, 0x22, 0x1c, 0x00, // )
    0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x00, 0x00, 0xA0, 0x60, 0x00, // ,
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x00, 0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x00, 0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x00, 0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x00, 0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x00, 0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x00, 0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x00, 0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x00, 0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x00, 0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x00, 0x32, 0x49, 0x59, 0x51, 0x3E, // @
    0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C, // A
    0x00, 0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x00, 0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x00, 0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x00, 0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x00, 0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x00, 0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x00, 0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x00, 0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x00, 0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x00, 0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x00, 0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x00, 0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x00, 0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x00, 0x55, 0x2A, 0x55, 0x2A, 0x55, // 55
    0x00, 0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x00, 0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x00, 0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x00, 0x01, 0x02, 0x04, 0x00, // '
    0x00, 0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x00, 0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x00, 0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x00, 0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x00, 0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x00, 0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C, // g
    0x00, 0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x00, 0x40, 0x80, 0x84, 0x7D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x00, 0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x00, 0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x00, 0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x00, 0xFC, 0x24, 0x24, 0x24, 0x18, // p
    0x00, 0x18, 0x24, 0x24, 0x18, 0xFC, // q
    0x00, 0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x00, 0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x00, 0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x00, 0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C, // y
    0x00, 0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x00, 0x08, 0x77, 0x00, 0x00, // {
    0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x00, 0x77, 0x08, 0x00, 0x00, // }
    0x00, 0x10, 0x08, 0x10, 0x08, 0x00, // ~
    0x14, 0x14, 0x14, 0x14, 0x14, 0x14, // horiz lines // DEL
    0x00 /* This byte is required for italic type of font */
};

// 16-bit framebuffer emulation (RGB 5-6-5 format)
typedef struct {
    Uint16* pixels;
    int width;
    int height;
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    SDL_Window* window;
} Framebuffer16;

// Compass structure
typedef struct {
    float angle;
    float target_angle;
    float transition_speed;
    int cx, cy;
    int radius;
    float* history;
    int history_size;
    int history_index;
} Compass;

int auto_mode = 0;  // Global for render function

// Color conversion for 16-bit RGB 5-6-5
Uint16 rgb565(Uint8 r, Uint8 g, Uint8 b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Initialize framebuffer
Framebuffer16* init_framebuffer(SDL_Window* window, SDL_Renderer* renderer, int width, int height) {
    Framebuffer16* fb = (Framebuffer16*)malloc(sizeof(Framebuffer16));
    if (!fb) return NULL;
    
    fb->pixels = (Uint16*)calloc(width * height, sizeof(Uint16));
    fb->width = width;
    fb->height = height;
    fb->renderer = renderer;
    fb->window = window;
    
    fb->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, 
                                    SDL_TEXTUREACCESS_STREAMING, width, height);
    return fb;
}

void fb_clear(Framebuffer16* fb, Uint16 color) {
    for (int i = 0; i < fb->width * fb->height; i++) {
        fb->pixels[i] = color;
    }
}

void fb_pixel(Framebuffer16* fb, int x, int y, Uint16 color) {
    if (x >= 0 && x < fb->width && y >= 0 && y < fb->height) {
        fb->pixels[y * fb->width + x] = color;
    }
}

void fb_line(Framebuffer16* fb, int x1, int y1, int x2, int y2, Uint16 color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;
    
    while (1) {
        fb_pixel(fb, x, y, color);
        if (x == x2 && y == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

void fb_circle(Framebuffer16* fb, int cx, int cy, int radius, Uint16 color) {
    if (radius <= 0) return;
    
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        fb_pixel(fb, cx + x, cy + y, color);
        fb_pixel(fb, cx + y, cy + x, color);
        fb_pixel(fb, cx - y, cy + x, color);
        fb_pixel(fb, cx - x, cy + y, color);
        fb_pixel(fb, cx - x, cy - y, color);
        fb_pixel(fb, cx - y, cy - x, color);
        fb_pixel(fb, cx + y, cy - x, color);
        fb_pixel(fb, cx + x, cy - y, color);
        
        if (err <= 0) {
            y++;
            err += 2*y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2*x + 1;
        }
    }
}

void fb_filled_circle(Framebuffer16* fb, int cx, int cy, int radius, Uint16 color) {
    if (radius <= 0) return;
    
    for (int y = -radius; y <= radius; y++) {
        int dx = (int)sqrt(radius * radius - y * y);
        for (int x = -dx; x <= dx; x++) {
            fb_pixel(fb, cx + x, cy + y, color);
        }
    }
}

void fb_filled_triangle(Framebuffer16* fb, int x1, int y1, int x2, int y2, int x3, int y3, Uint16 color) {
    if (y1 > y2) { int t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }
    if (y1 > y3) { int t = x1; x1 = x3; x3 = t; t = y1; y1 = y3; y3 = t; }
    if (y2 > y3) { int t = x2; x2 = x3; x3 = t; t = y2; y2 = y3; y3 = t; }
    
    if (y1 == y3) return;
    
    float inv_slope1 = (float)(x2 - x1) / (y2 - y1);
    float inv_slope2 = (float)(x3 - x1) / (y3 - y1);
    
    if (y2 != y1) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (int)((y - y1) * inv_slope1);
            int x_end = x1 + (int)((y - y1) * inv_slope2);
            if (x_start > x_end) { int t = x_start; x_start = x_end; x_end = t; }
            for (int x = x_start; x <= x_end; x++) fb_pixel(fb, x, y, color);
        }
    }
    
    if (y3 != y2) {
        inv_slope1 = (float)(x3 - x2) / (y3 - y2);
        for (int y = y2; y <= y3; y++) {
            int x_start = x2 + (int)((y - y2) * inv_slope1);
            int x_end = x1 + (int)((y - y1) * inv_slope2);
            if (x_start > x_end) { int t = x_start; x_start = x_end; x_end = t; }
            for (int x = x_start; x <= x_end; x++) fb_pixel(fb, x, y, color);
        }
    }
}

// Fixed character drawing for SSD1306 font
void fb_draw_char(Framebuffer16* fb, int x, int y, char c, Uint16 color) {
    if (c < 32 || c > 126) c = 32;
    
    // Font format: header at bytes 0-3, then each character is 6 bytes (6 columns)
    // Byte 0: version (0x00), Byte 1: width (6), Byte 2: height (8), Byte 3: first char (32)
    int font_width = font6x8[1];   // Should be 6
    int font_height = font6x8[2];  // Should be 8
    int first_char = font6x8[3];   // Should be 32
    
    int char_index = (c - first_char) * font_width;
    int font_start = 4; // Skip 4-byte header
    
    // Draw each column of the character
    for (int col = 0; col < font_width; col++) {
        uint8_t column_bits = font6x8[font_start + char_index + col];
        
        // For each row in this column (bits are vertical, LSB is top row)
        for (int row = 0; row < font_height; row++) {
            if (column_bits & (1 << row)) {
                fb_pixel(fb, x + col, y + row, color);
            }
        }
    }
}

void fb_text(Framebuffer16* fb, int x, int y, const char* text, Uint16 color) {
    int font_width = font6x8[1];
    int cur_x = x;
    
    while (*text) {
        if (*text == '\n') {
            cur_x = x;
            y += font6x8[2] + 1;
        } else {
            fb_draw_char(fb, cur_x, y, *text, color);
            cur_x += font_width + 1; // Add 1 pixel spacing
        }
        text++;
    }
}

int fb_text_width(const char* text) {
    int font_width = font6x8[1];
    return strlen(text) * (font_width + 1);
}

void draw_needle(Framebuffer16* fb, Compass* compass, Uint16 color_north, Uint16 color_south) {
    float rad = compass->angle * M_PI / 180.0f;
    int needle_length = compass->radius - 20;
    
    int nx = compass->cx + (int)(needle_length * sin(rad));
    int ny = compass->cy - (int)(needle_length * cos(rad));
    int sx = compass->cx - (int)(needle_length * sin(rad));
    int sy = compass->cy + (int)(needle_length * cos(rad));
    
    float perp_rad = rad + M_PI/2;
    int half_width = 12;
    
    int n_base1_x = compass->cx + (int)(half_width * sin(perp_rad));
    int n_base1_y = compass->cy - (int)(half_width * cos(perp_rad));
    int n_base2_x = compass->cx - (int)(half_width * sin(perp_rad));
    int n_base2_y = compass->cy + (int)(half_width * cos(perp_rad));
    
    fb_filled_triangle(fb, nx, ny, n_base1_x, n_base1_y, n_base2_x, n_base2_y, color_north);
    
    half_width = 8;
    int s_base1_x = compass->cx + (int)(half_width * sin(perp_rad));
    int s_base1_y = compass->cy - (int)(half_width * cos(perp_rad));
    int s_base2_x = compass->cx - (int)(half_width * sin(perp_rad));
    int s_base2_y = compass->cy + (int)(half_width * cos(perp_rad));
    
    fb_filled_triangle(fb, sx, sy, s_base1_x, s_base1_y, s_base2_x, s_base2_y, color_south);
    
    fb_filled_circle(fb, compass->cx, compass->cy, 10, rgb565(200, 200, 200));
    fb_circle(fb, compass->cx, compass->cy, 10, rgb565(0, 0, 0));
}

void draw_markings(Framebuffer16* fb, Compass* compass) {
    Uint16 color_mark = rgb565(255, 255, 255);
    Uint16 color_cardinal = rgb565(255, 100, 100);
    
    for (int angle = 0; angle < 360; angle += 15) {
        float rad = angle * M_PI / 180.0f;
        int inner_r = compass->radius - 15;
        int outer_r = compass->radius;
        
        if (angle % 90 == 0) {
            outer_r = compass->radius + 5;
            fb_line(fb, 
                   compass->cx + (int)(inner_r * sin(rad)),
                   compass->cy - (int)(inner_r * cos(rad)),
                   compass->cx + (int)(outer_r * sin(rad)),
                   compass->cy - (int)(outer_r * cos(rad)),
                   color_cardinal);
        } else if (angle % 45 == 0) {
            fb_line(fb,
                   compass->cx + (int)(inner_r * sin(rad)),
                   compass->cy - (int)(inner_r * cos(rad)),
                   compass->cx + (int)(outer_r * sin(rad)),
                   compass->cy - (int)(outer_r * cos(rad)),
                   color_mark);
        } else {
            fb_line(fb,
                   compass->cx + (int)((inner_r + 5) * sin(rad)),
                   compass->cy - (int)((inner_r + 5) * cos(rad)),
                   compass->cx + (int)(outer_r * sin(rad)),
                   compass->cy - (int)(outer_r * cos(rad)),
                   color_mark);
        }
    }
    
    const char* directions[] = {"N", "E", "S", "W"};
    int angles[] = {0, 90, 180, 270};
    int font_width = font6x8[1];
    int font_height = font6x8[2];
    
    for (int i = 0; i < 4; i++) {
        float rad = angles[i] * M_PI / 180.0f;
        int x = compass->cx + (int)((compass->radius - 20) * sin(rad)) - (font_width / 2);
        int y = compass->cy - (int)((compass->radius - 20) * cos(rad)) - (font_height / 2);
        fb_text(fb, x, y, directions[i], color_cardinal);
    }
}

void draw_history_trail(Framebuffer16* fb, Compass* compass) {
    if (compass->history_size < 2) return;
    
    Uint16 color_trail = rgb565(100, 100, 200);
    int trail_length = compass->radius - 30;
    
    for (int i = 1; i < compass->history_size; i++) {
        int idx = (compass->history_index - i + compass->history_size) % compass->history_size;
        int prev_idx = (idx - 1 + compass->history_size) % compass->history_size;
        
        if (compass->history[idx] >= 0 && compass->history[prev_idx] >= 0) {
            float rad1 = compass->history[idx] * M_PI / 180.0f;
            float rad2 = compass->history[prev_idx] * M_PI / 180.0f;
            
            int x1 = compass->cx + (int)(trail_length * sin(rad1));
            int y1 = compass->cy - (int)(trail_length * cos(rad1));
            int x2 = compass->cx + (int)(trail_length * sin(rad2));
            int y2 = compass->cy - (int)(trail_length * cos(rad2));
            
            fb_line(fb, x1, y1, x2, y2, color_trail);
        }
    }
}

void update_history(Compass* compass, float angle) {
    compass->history[compass->history_index] = angle;
    compass->history_index = (compass->history_index + 1) % compass->history_size;
}

void draw_digital_readout(Framebuffer16* fb, Compass* compass) {
    char buffer[64];
    int font_height = font6x8[2];
    int y_offset = compass->cy + compass->radius + 15;
    
    sprintf(buffer, "Angle: %.1f deg", compass->angle);
    int text_width = fb_text_width(buffer);
    fb_text(fb, compass->cx - text_width/2, y_offset, buffer, rgb565(200, 255, 200));
    
    const char* direction;
    if (compass->angle >= 337.5 || compass->angle < 22.5) direction = "North";
    else if (compass->angle >= 22.5 && compass->angle < 67.5) direction = "Northeast";
    else if (compass->angle >= 67.5 && compass->angle < 112.5) direction = "East";
    else if (compass->angle >= 112.5 && compass->angle < 157.5) direction = "Southeast";
    else if (compass->angle >= 157.5 && compass->angle < 202.5) direction = "South";
    else if (compass->angle >= 202.5 && compass->angle < 247.5) direction = "Southwest";
    else if (compass->angle >= 247.5 && compass->angle < 292.5) direction = "West";
    else direction = "Northwest";
    
    sprintf(buffer, "Direction: %s", direction);
    text_width = fb_text_width(buffer);
    fb_text(fb, compass->cx - text_width/2, y_offset + font_height + 5, buffer, rgb565(200, 255, 200));
    
    sprintf(buffer, "Speed: %.2f", compass->transition_speed);
    text_width = fb_text_width(buffer);
    fb_text(fb, compass->cx - text_width/2, y_offset + (font_height + 5) * 2, buffer, rgb565(200, 200, 100));
    
    sprintf(buffer, "Mode: %s", auto_mode ? "AUTO" : "MANUAL");
    text_width = fb_text_width(buffer);
    fb_text(fb, compass->cx - text_width/2, y_offset + (font_height + 5) * 3, buffer, 
            auto_mode ? rgb565(100, 255, 100) : rgb565(255, 200, 100));
}

void render_compass(Framebuffer16* fb, Compass* compass) {
    fb_clear(fb, rgb565(10, 10, 30));
    
    fb_circle(fb, compass->cx, compass->cy, compass->radius, rgb565(200, 200, 200));
    fb_circle(fb, compass->cx, compass->cy, compass->radius - 2, rgb565(150, 150, 150));
    fb_circle(fb, compass->cx, compass->cy, compass->radius - 35, rgb565(50, 50, 80));
    
    draw_markings(fb, compass);
    draw_history_trail(fb, compass);
    draw_needle(fb, compass, rgb565(255, 50, 50), rgb565(100, 100, 255));
    draw_digital_readout(fb, compass);
    
    int title_width = fb_text_width("Advanced Compass");
    fb_text(fb, compass->cx - title_width/2, 20, "Advanced Compass", rgb565(255, 255, 100));
    
    int controls_y = fb->height - 80;
    fb_text(fb, 10, controls_y, "Controls:", rgb565(150, 150, 150));
    fb_text(fb, 10, controls_y + 12, "Mouse - Control direction", rgb565(150, 150, 150));
    fb_text(fb, 10, controls_y + 24, "Space - Toggle auto-rotate", rgb565(150, 150, 150));
    fb_text(fb, 10, controls_y + 36, "Up/Down - Adjust speed", rgb565(150, 150, 150));
    fb_text(fb, 10, controls_y + 48, "R - Reset", rgb565(150, 150, 150));
}

void fb_present(Framebuffer16* fb) {
    SDL_UpdateTexture(fb->texture, NULL, fb->pixels, fb->width * sizeof(Uint16));
    SDL_RenderClear(fb->renderer);
    SDL_RenderCopy(fb->renderer, fb->texture, NULL, NULL);
    SDL_RenderPresent(fb->renderer);
}

void cleanup(Framebuffer16* fb, Compass* compass) {
    if (compass) {
        if (compass->history) free(compass->history);
        free(compass);
    }
    if (fb) {
        if (fb->pixels) free(fb->pixels);
        if (fb->texture) SDL_DestroyTexture(fb->texture);
        free(fb);
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    int width = 800;
    int height = 600;
    SDL_Window* window = SDL_CreateWindow("Advanced Compass - SSD1306 6x8 Font",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          width, height,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    Framebuffer16* fb = init_framebuffer(window, renderer, width, height);
    if (!fb) {
        printf("Framebuffer initialization failed\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    Compass* compass = (Compass*)calloc(1, sizeof(Compass));
    if (!compass) {
        printf("Compass initialization failed\n");
        cleanup(fb, NULL);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    compass->angle = 0.0f;
    compass->target_angle = 0.0f;
    compass->transition_speed = 0.1f;
    compass->cx = width / 2;
    compass->cy = height / 2 - 30;
    compass->radius = 170;
    compass->history_size = 30;
    compass->history_index = 0;
    compass->history = (float*)malloc(compass->history_size * sizeof(float));
    
    if (!compass->history) {
        printf("History initialization failed\n");
        cleanup(fb, compass);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    for (int i = 0; i < compass->history_size; i++) {
        compass->history[i] = -1.0f;
    }
    
    int running = 1;
    SDL_Event event;
    int mouse_x = 0, mouse_y = 0;
    Uint32 last_time = SDL_GetTicks();
    float auto_rotation = 0.0f;
    
    while (running) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        if (delta_time > 0.1f) delta_time = 0.1f;
        last_time = current_time;
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEMOTION) {
                if (!auto_mode) {
                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                    
                    float dx = mouse_x - compass->cx;
                    float dy = mouse_y - compass->cy;
                    if (dx != 0 || dy != 0) {
                        compass->target_angle = atan2(dx, -dy) * 180.0f / M_PI;
                        if (compass->target_angle < 0) compass->target_angle += 360.0f;
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        auto_mode = !auto_mode;
                        break;
                    case SDLK_r:

                        auto_rotation = 0.0f;
                        compass->target_angle = 0.0f;
                        compass->angle = 0.0f;
                        break;
                    case SDLK_UP:
                        compass->transition_speed += 0.05f;
                        if (compass->transition_speed > 0.5f) compass->transition_speed = 0.5f;
                        break;
                    case SDLK_DOWN:
                        compass->transition_speed -= 0.05f;
                        if (compass->transition_speed < 0.02f) compass->transition_speed = 0.02f;
                        break;
                }
            }
        }
        
        if (auto_mode) {
            auto_rotation += 60.0f * delta_time;
            if (auto_rotation >= 360.0f) auto_rotation -= 360.0f;
            compass->target_angle = auto_rotation;
        }
        
        float angle_diff = compass->target_angle - compass->angle;
        if (angle_diff > 180.0f) angle_diff -= 360.0f;
        if (angle_diff < -180.0f) angle_diff += 360.0f;
        compass->angle += angle_diff * compass->transition_speed;
        
        if (compass->angle >= 360.0f) compass->angle -= 360.0f;
        if (compass->angle < 0.0f) compass->angle += 360.0f;
        
        update_history(compass, compass->angle);
        render_compass(fb, compass);
        fb_present(fb);
        
        SDL_Delay(16);
    }
    
    cleanup(fb, compass);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
