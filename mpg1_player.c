#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_BUFFER_SIZE 4096
#define RGB565_RED_MASK   0xF800
#define RGB565_GREEN_MASK 0x07E0
#define RGB565_BLUE_MASK  0x001F

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_AudioDeviceID audio_device;
    
    uint16_t *framebuffer;
    int framebuffer_width;
    int framebuffer_height;
    
    float *audio_buffer;
    int audio_buffer_frames;
    int audio_buffer_capacity;
    int audio_read_pos;
    int audio_write_pos;
    
    int video_width;
    int video_height;
    int quit;
    double last_time;
    int paused;
} PlayerState;

// Convert RGB (8-bit each) to RGB565 (16-bit)
static inline uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Convert YCrCb to RGB565 and write directly to framebuffer
static void ycrcb_to_rgb565_frame(plm_frame_t *frame, uint16_t *fb, int fb_width) {
    int cols = frame->width >> 1;
    int rows = frame->height >> 1;
    int yw = frame->y.width;
    int cw = frame->cb.width;
    
    for (int row = 0; row < rows; row++) {
        int c_index = row * cw;
        int y_index = row * 2 * yw;
        int fb_index = row * 2 * fb_width;
        
        for (int col = 0; col < cols; col++) {
            int y;
            int cr_val = frame->cr.data[c_index] - 128;
            int cb_val = frame->cb.data[c_index] - 128;
            
            // Calculate color components using BT.601
            int r_scale = (cr_val * 104597) >> 16;
            int g_scale = (cb_val * 25674 + cr_val * 53278) >> 16;
            int b_scale = (cb_val * 132201) >> 16;
            
            // Process 2x2 block of pixels
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    y = ((frame->y.data[y_index + dy * yw + dx] - 16) * 76309) >> 16;
                    
                    int r = y + r_scale;
                    int g = y - g_scale;
                    int b = y + b_scale;
                    
                    // Clamp to 0-255
                    if (r > 255) r = 255;
                    else if (r < 0) r = 0;
                    if (g > 255) g = 255;
                    else if (g < 0) g = 0;
                    if (b > 255) b = 255;
                    else if (b < 0) b = 0;
                    
                    fb[fb_index + dy * fb_width + dx] = rgb_to_rgb565(r, g, b);
                }
            }
            
            c_index += 1;
            y_index += 2;
            fb_index += 2;
        }
    }
}

static void video_callback(plm_t *plm, plm_frame_t *frame, void *user) {
    PlayerState *state = (PlayerState *)user;
    (void)plm;
    
    if (!state->video_width) {
        state->video_width = frame->width;
        state->video_height = frame->height;
        
        // Resize framebuffer if needed
        if (state->framebuffer) {
            free(state->framebuffer);
        }
        state->framebuffer_width = frame->width;
        state->framebuffer_height = frame->height;
        state->framebuffer = (uint16_t *)malloc(frame->width * frame->height * sizeof(uint16_t));
        
        if (!state->framebuffer) {
            fprintf(stderr, "Failed to allocate framebuffer\n");
            return;
        }
        
        // Set window size
        SDL_SetWindowSize(state->window, frame->width, frame->height);
        SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        
        // Create texture for display
        if (state->texture) {
            SDL_DestroyTexture(state->texture);
        }
        state->texture = SDL_CreateTexture(state->renderer,
            SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
            frame->width, frame->height);
    }
    
    // Render YCrCb to RGB565 framebuffer
    ycrcb_to_rgb565_frame(frame, state->framebuffer, state->framebuffer_width);
    
    // Update texture from framebuffer
    SDL_UpdateTexture(state->texture, NULL, state->framebuffer, 
                      state->framebuffer_width * sizeof(uint16_t));
    
    // Display
    SDL_RenderClear(state->renderer);
    SDL_RenderCopy(state->renderer, state->texture, NULL, NULL);
    SDL_RenderPresent(state->renderer);
}

static void audio_callback(plm_t *plm, plm_samples_t *samples, void *user) {
    PlayerState *state = (PlayerState *)user;
    (void)plm;
    
    int frames_available = PLM_AUDIO_SAMPLES_PER_FRAME;
    float *src = samples->interleaved;
    
    while (frames_available > 0) {
        int space = state->audio_buffer_capacity - state->audio_write_pos;
        int to_copy = frames_available < space ? frames_available : space;
        
        memcpy(state->audio_buffer + state->audio_write_pos * 2, src, to_copy * 2 * sizeof(float));
        
        state->audio_write_pos += to_copy;
        if (state->audio_write_pos >= state->audio_buffer_capacity) {
            state->audio_write_pos = 0;
        }
        
        frames_available -= to_copy;
        src += to_copy * 2;
        state->audio_buffer_frames += to_copy;
    }
    
    if (state->audio_buffer_frames > state->audio_buffer_capacity) {
        state->audio_buffer_frames = state->audio_buffer_capacity;
    }
}

static void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
    PlayerState *state = (PlayerState *)userdata;
    int samples_needed = len / (2 * sizeof(float));
    float *dst = (float *)stream;
    int copied = 0;
    
    while (copied < samples_needed) {
        if (state->audio_buffer_frames == 0) {
            // No audio data, fill with silence
            memset(dst + copied * 2, 0, (samples_needed - copied) * 2 * sizeof(float));
            copied = samples_needed;
            break;
        }
        
        int to_copy = state->audio_buffer_frames;
        if (to_copy > samples_needed - copied) {
            to_copy = samples_needed - copied;
        }
        
        int available = state->audio_buffer_capacity - state->audio_read_pos;
        if (to_copy > available) {
            to_copy = available;
        }
        
        memcpy(dst + copied * 2, 
               state->audio_buffer + state->audio_read_pos * 2,
               to_copy * 2 * sizeof(float));
        
        state->audio_read_pos += to_copy;
        if (state->audio_read_pos >= state->audio_buffer_capacity) {
            state->audio_read_pos = 0;
        }
        
        state->audio_buffer_frames -= to_copy;
        copied += to_copy;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <video.mpg>\n", argv[0]);
        fprintf(stderr, "Example: %s sample.mpg\n", argv[0]);
        return 1;
    }
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    PlayerState state = {0};
    state.audio_buffer_capacity = AUDIO_BUFFER_SIZE;
    state.audio_buffer = (float *)malloc(AUDIO_BUFFER_SIZE * 2 * sizeof(float));
    state.framebuffer = NULL;
    state.paused = 0;
    
    if (!state.audio_buffer) {
        fprintf(stderr, "Failed to allocate audio buffer\n");
        SDL_Quit();
        return 1;
    }
    
    // Create window
    state.window = SDL_CreateWindow("MPEG Player (16-bit Framebuffer)", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        640, 480, SDL_WINDOW_RESIZABLE);
    if (!state.window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        free(state.audio_buffer);
        SDL_Quit();
        return 1;
    }
    
    state.renderer = SDL_CreateRenderer(state.window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!state.renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(state.window);
        free(state.audio_buffer);
        SDL_Quit();
        return 1;
    }
    
    // Load MPEG
    printf("Loading %s...\n", argv[1]);
    plm_t *plm = plm_create_with_filename(argv[1]);
    if (!plm) {
        fprintf(stderr, "Failed to load: %s\n", argv[1]);
        SDL_DestroyRenderer(state.renderer);
        SDL_DestroyWindow(state.window);
        free(state.audio_buffer);
        SDL_Quit();
        return 1;
    }
    
    // Wait for headers
    printf("Waiting for headers...\n");
    while (!plm_has_headers(plm)) {
        plm_decode(plm, 0.01);
    }
    
    // Get video info
    int width = plm_get_width(plm);
    int height = plm_get_height(plm);
    double framerate = plm_get_framerate(plm);
    int samplerate = plm_get_samplerate(plm);
    
    printf("Video: %dx%d, %.2f fps\n", width, height, framerate);
    printf("Audio: %d Hz\n", samplerate);
    printf("Using 16-bit RGB565 framebuffer\n");
    
    // Setup audio
    if (samplerate > 0) {
        SDL_AudioSpec wanted = {0};
        wanted.freq = samplerate;
        wanted.format = AUDIO_F32SYS;
        wanted.channels = 2;
        wanted.samples = 1024;
        wanted.callback = sdl_audio_callback;
        wanted.userdata = &state;
        
        SDL_AudioSpec obtained;
        state.audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
        if (state.audio_device == 0) {
            fprintf(stderr, "Warning: Failed to open audio: %s\n", SDL_GetError());
        } else {
            printf("Audio opened: %d Hz, %d samples\n", obtained.freq, obtained.samples);
            SDL_PauseAudioDevice(state.audio_device, 0);
        }
    }
    
    // Set callbacks
    plm_set_video_decode_callback(plm, video_callback, &state);
    if (samplerate > 0) {
        plm_set_audio_decode_callback(plm, audio_callback, &state);
    } else {
        plm_set_audio_enabled(plm, 0);
        printf("No audio stream found\n");
    }
    
    state.last_time = SDL_GetTicks() / 1000.0;
    
    // Main loop
    SDL_Event event;
    while (!state.quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                state.quit = 1;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        state.quit = 1;
                        break;
                    case SDLK_SPACE:
                        state.paused = !state.paused;
                        if (state.audio_device) {
                            SDL_PauseAudioDevice(state.audio_device, state.paused);
                        }
                        printf(state.paused ? "Paused\n" : "Resumed\n");
                        break;
                    case SDLK_r:
                        printf("Rewinding...\n");
                        plm_rewind(plm);
                        state.audio_read_pos = 0;
                        state.audio_write_pos = 0;
                        state.audio_buffer_frames = 0;
                        state.last_time = SDL_GetTicks() / 1000.0;
                        if (state.audio_device) {
                            SDL_PauseAudioDevice(state.audio_device, 0);
                        }
                        state.paused = 0;
                        break;
                    default:
                        break;
                }
            }
        }
        
        if (!state.paused && !plm_has_ended(plm)) {
            double current_time = SDL_GetTicks() / 1000.0;
            double delta = current_time - state.last_time;
            
            if (delta > 0.1) delta = 0.1; // Cap delta time
            if (delta > 0) {
                plm_decode(plm, delta);
            }
            state.last_time = current_time;
        } else if (plm_has_ended(plm)) {
            printf("Playback ended. Press R to restart or Q to quit.\n");
            SDL_Delay(100);
        } else {
            SDL_Delay(10);
        }
    }
    
    // Cleanup
    printf("Cleaning up...\n");
    if (state.audio_device) {
        SDL_CloseAudioDevice(state.audio_device);
    }
    free(state.audio_buffer);
    if (state.framebuffer) {
        free(state.framebuffer);
    }
    if (state.texture) {
        SDL_DestroyTexture(state.texture);
    }
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    plm_destroy(plm);
    SDL_Quit();
    
    printf("Goodbye!\n");
    return 0;
}
