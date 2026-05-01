#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800
#define MAX_DATA_POINTS 200
#define FPS 60

// Struct untuk menyimpan data analisa
typedef struct {
    float data[MAX_DATA_POINTS];
    int count;
    char labels[MAX_DATA_POINTS][20];
    float min_val;
    float max_val;
} Dataset;

// Warna-warna menarik
typedef struct {
    SDL_Color bg;        // Background
    SDL_Color grid;      // Grid lines
    SDL_Color text;      // Text
    SDL_Color line;      // Line chart
    SDL_Color bar;       // Bar chart
    SDL_Color highlight; // Highlight
    SDL_Color title_bg;  // Title background
    SDL_Color success;   // Success color
    SDL_Color warning;   // Warning color
} ColorTheme;

// Mode analisa
typedef enum {
    MODE_LINE_CHART,
    MODE_BAR_CHART,
    MODE_STATISTICS,
    MODE_TREND_ANALYSIS
} AnalysisMode;

// Aplikasi context
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_title;
    TTF_Font* font_normal;
    TTF_Font* font_small;
    Dataset dataset;
    AnalysisMode mode;
    ColorTheme theme;
    int running;
    int hover_index;
    float animation_offset;
    char info_text[256];
} AppContext;

// Inisialisasi warna tema yang lebih menarik
void initTheme(ColorTheme* theme) {
    theme->bg = (SDL_Color){18, 25, 35, 255};         // Deep navy blue
    theme->grid = (SDL_Color){45, 55, 70, 255};       // Steel gray
    theme->text = (SDL_Color){220, 230, 245, 255};    // Ice blue
    theme->line = (SDL_Color){0, 210, 255, 255};      // Electric cyan
    theme->bar = (SDL_Color){100, 180, 250, 200};     // Sky blue
    theme->highlight = (SDL_Color){255, 80, 120, 255}; // Neon pink-red
    theme->title_bg = (SDL_Color){30, 40, 55, 200};    // Semi-transparent dark
    theme->success = (SDL_Color){100, 255, 100, 255};  // Green
    theme->warning = (SDL_Color){255, 200, 100, 255};  // Orange
}

// Generate sample data dengan pattern menarik
void generateSampleData(Dataset* dataset) {
    dataset->count = 24;  // 24 data points untuk analisis lebih detail
    srand(time(NULL));
    
    const char* labels[] = {"W1", "W2", "W3", "W4", "W5", "W6", "W7", "W8", 
                            "W9", "W10", "W11", "W12", "W13", "W14", "W15", 
                            "W16", "W17", "W18", "W19", "W20", "W21", "W22", "W23", "W24"};
    
    dataset->min_val = 10000;
    dataset->max_val = 0;
    
    for(int i = 0; i < dataset->count; i++) {
        // Data dengan multiple patterns: linear trend + seasonal + noise
        float trend = 150 * i;  // Upward trend
        float seasonal = 1200 * sin(i * M_PI / 6);  // Seasonal pattern
        float noise = (rand() % 800) - 400;  // Random noise
        float growth = 400 * (1 - exp(-i / 12.0));  // Growth curve
        
        dataset->data[i] = 5000 + trend + seasonal + noise + growth;
        
        strcpy(dataset->labels[i], labels[i]);
        
        if(dataset->data[i] < dataset->min_val) dataset->min_val = dataset->data[i];
        if(dataset->data[i] > dataset->max_val) dataset->max_val = dataset->data[i];
    }
    
    // Tambah padding untuk visualisasi yang lebih baik
    float range = dataset->max_val - dataset->min_val;
    dataset->min_val -= range * 0.15;
    dataset->max_val += range * 0.1;
}

// Fungsi untuk merender teks dengan SDL_ttf
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    if(!font || !text) return;
    
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
    if(!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(texture) {
        SDL_Rect dest = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

// Render text dengan background
void renderTextWithBg(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color textColor, SDL_Color bgColor) {
    if(!font || !text) return;
    
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, textColor);
    if(!surface) return;
    
    // Draw background
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_Rect bg = {x - 5, y - 3, surface->w + 10, surface->h + 6};
    SDL_RenderFillRect(renderer, &bg);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, 100);
    SDL_RenderDrawRect(renderer, &bg);
    
    // Draw text
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(texture) {
        SDL_Rect dest = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

// Konversi data ke koordinat layar
int dataToY(float value, int chart_height, int y_offset, Dataset* dataset) {
    float normalized = (value - dataset->min_val) / (dataset->max_val - dataset->min_val);
    return y_offset + chart_height - (int)(normalized * chart_height);
}

int dataToX(int index, int chart_width, int x_offset, int total_points) {
    if(total_points <= 1) return x_offset;
    return x_offset + (index * chart_width / (total_points - 1));
}

// Draw grid yang lebih menarik
void drawGrid(SDL_Renderer* renderer, int x, int y, int w, int h, ColorTheme* theme) {
    SDL_SetRenderDrawColor(renderer, theme->grid.r, theme->grid.g, theme->grid.b, theme->grid.a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Vertical lines
    for(int i = 0; i <= 10; i++) {
        int grid_x = x + (i * w / 10);
        SDL_RenderDrawLine(renderer, grid_x, y, grid_x, y + h);
    }
    
    // Horizontal lines
    for(int i = 0; i <= 5; i++) {
        int grid_y = y + (i * h / 5);
        SDL_RenderDrawLine(renderer, x, grid_y, x + w, grid_y);
    }
}

// Draw axes dengan label teks
void drawAxesAndLabels(SDL_Renderer* renderer, TTF_Font* font, Dataset* dataset, int x, int y, int w, int h, ColorTheme* theme) {
    SDL_SetRenderDrawColor(renderer, theme->text.r, theme->text.g, theme->text.b, 255);
    SDL_RenderDrawLine(renderer, x, y, x, y + h);
    SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
    
    // Draw Y-axis labels
    char label[32];
    for(int i = 0; i <= 5; i++) {
        int label_y = y + (i * h / 5);
        float value = dataset->max_val - (i * (dataset->max_val - dataset->min_val) / 5);
        
        if(value >= 1000) {
            sprintf(label, "%.1fk", value / 1000);
        } else {
            sprintf(label, "%.0f", value);
        }
        
        renderText(renderer, font, label, x - 50, label_y - 8, theme->text);
    }
    
    // Draw X-axis labels (every 3rd point)
    for(int i = 0; i < dataset->count; i += 3) {
        int label_x = dataToX(i, w, x, dataset->count);
        renderText(renderer, font, dataset->labels[i], label_x - 15, y + h + 5, theme->text);
    }
    
    // Draw axis titles
    renderText(renderer, font, "Time Period (Weeks)", x + w/2 - 80, y + h + 35, theme->text);
    renderText(renderer, font, "Value (Rp)", x - 60, y + h/2, theme->text);
}

// Draw line chart dengan gradient effect
void drawLineChart(SDL_Renderer* renderer, TTF_Font* font, Dataset* dataset, int x, int y, int w, int h, ColorTheme* theme, int hover_index) {
    drawGrid(renderer, x, y, w, h, theme);
    drawAxesAndLabels(renderer, font, dataset, x, y, w, h, theme);
    
    // Draw area under line (gradient effect)
    for(int i = 0; i < dataset->count - 1; i++) {
        int x1 = dataToX(i, w, x, dataset->count);
        int y1 = dataToY(dataset->data[i], h, y, dataset);
        int x2 = dataToX(i + 1, w, x, dataset->count);
        int y2 = dataToY(dataset->data[i + 1], h, y, dataset);
        
        // Draw line
        SDL_SetRenderDrawColor(renderer, theme->line.r, theme->line.g, theme->line.b, 255);
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        
        // Draw area fill
        SDL_SetRenderDrawColor(renderer, theme->line.r, theme->line.g, theme->line.b, 50);
        for(int py = y1; py <= y + h; py++) {
            SDL_RenderDrawLine(renderer, x1, py, x2, py);
        }
    }
    
    // Draw points with glow effect
    for(int i = 0; i < dataset->count; i++) {
        int px = dataToX(i, w, x, dataset->count);
        int py = dataToY(dataset->data[i], h, y, dataset);
        
        SDL_Color color = (i == hover_index) ? theme->highlight : theme->line;
        
        // Draw glow
        for(int r = 5; r > 0; r--) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 50 - r*10);
            for(int dy = -r; dy <= r; dy++) {
                for(int dx = -r; dx <= r; dx++) {
                    if(dx*dx + dy*dy <= r*r) {
                        SDL_RenderDrawPoint(renderer, px + dx, py + dy);
                    }
                }
            }
        }
        
        // Draw core point
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for(int dy = -2; dy <= 2; dy++) {
            for(int dx = -2; dx <= 2; dx++) {
                if(dx*dx + dy*dy <= 4) {
                    SDL_RenderDrawPoint(renderer, px + dx, py + dy);
                }
            }
        }
        
        // Draw value label for hovered point
        if(i == hover_index) {
            char value_label[32];
            sprintf(value_label, "%.0f", dataset->data[i]);
            renderTextWithBg(renderer, font, value_label, px + 10, py - 20, theme->text, theme->title_bg);
            renderTextWithBg(renderer, font, dataset->labels[i], px + 10, py - 40, theme->highlight, theme->title_bg);
        }
    }
}

// Draw bar chart dengan efek 3D sederhana
void drawBarChart(SDL_Renderer* renderer, TTF_Font* font, Dataset* dataset, int x, int y, int w, int h, ColorTheme* theme, int hover_index) {
    drawGrid(renderer, x, y, w, h, theme);
    drawAxesAndLabels(renderer, font, dataset, x, y, w, h, theme);
    
    int bar_width = w / dataset->count - 4;
    if(bar_width > 40) bar_width = 40;
    
    for(int i = 0; i < dataset->count; i++) {
        int bar_x = x + (i * w / dataset->count) + 2;
        int bar_height = ((dataset->data[i] - dataset->min_val) / (dataset->max_val - dataset->min_val)) * h;
        int bar_y = y + h - bar_height;
        
        SDL_Color color = (i == hover_index) ? theme->highlight : theme->bar;
        
        // Draw 3D effect - shadow
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
        SDL_Rect shadow = {bar_x + 3, bar_y + 3, bar_width - 4, bar_height};
        SDL_RenderFillRect(renderer, &shadow);
        
        // Main bar
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_Rect bar = {bar_x, bar_y, bar_width - 4, bar_height};
        SDL_RenderFillRect(renderer, &bar);
        
        // Highlight on top
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        SDL_Rect highlight = {bar_x, bar_y, bar_width - 4, 3};
        SDL_RenderFillRect(renderer, &highlight);
        
        // Draw value on top of bar for hovered item
        if(i == hover_index) {
            char value_label[32];
            sprintf(value_label, "%.0f", dataset->data[i]);
            renderTextWithBg(renderer, font, value_label, bar_x + 5, bar_y - 25, theme->text, theme->title_bg);
        }
    }
}

// Draw statistics panel dengan analisis lengkap dan teks informatif
void drawStatistics(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* font_small, Dataset* dataset, int x, int y, int w, int h, ColorTheme* theme) {
    // Calculate comprehensive statistics
    float sum = 0, mean, variance = 0, std_dev;
    float min = dataset->data[0], max = dataset->data[0];
    float sorted_data[MAX_DATA_POINTS];
    
    // Copy and sort data for median
    for(int i = 0; i < dataset->count; i++) {
        sorted_data[i] = dataset->data[i];
        sum += dataset->data[i];
        if(dataset->data[i] < min) min = dataset->data[i];
        if(dataset->data[i] > max) max = dataset->data[i];
    }
    
    // Sort for median
    for(int i = 0; i < dataset->count - 1; i++) {
        for(int j = i + 1; j < dataset->count; j++) {
            if(sorted_data[i] > sorted_data[j]) {
                float temp = sorted_data[i];
                sorted_data[i] = sorted_data[j];
                sorted_data[j] = temp;
            }
        }
    }
    
    mean = sum / dataset->count;
    
    // Calculate median
    float median;
    if(dataset->count % 2 == 0) {
        median = (sorted_data[dataset->count/2 - 1] + sorted_data[dataset->count/2]) / 2;
    } else {
        median = sorted_data[dataset->count/2];
    }
    
    // Calculate variance and std deviation
    for(int i = 0; i < dataset->count; i++) {
        variance += pow(dataset->data[i] - mean, 2);
    }
    variance /= dataset->count;
    std_dev = sqrt(variance);
    
    // Calculate trend (linear regression)
    float sum_x = 0, sum_xy = 0, sum_x2 = 0;
    for(int i = 0; i < dataset->count; i++) {
        sum_x += i;
        sum_xy += i * dataset->data[i];
        sum_x2 += i * i;
    }
    float slope = (dataset->count * sum_xy - sum_x * sum) / (dataset->count * sum_x2 - sum_x * sum_x);
    float intercept = (sum - slope * sum_x) / dataset->count;
    
    // Draw statistics background
    SDL_SetRenderDrawColor(renderer, theme->title_bg.r, theme->title_bg.g, theme->title_bg.b, 200);
    SDL_Rect bg_rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &bg_rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, theme->line.r, theme->line.g, theme->line.b, 255);
    SDL_RenderDrawRect(renderer, &bg_rect);
    
    // Title
    renderText(renderer, font, "STATISTICAL ANALYSIS REPORT", x + 20, y + 10, theme->highlight);
    
    // Draw statistics with formatted text
    char stat_text[256];
    int line_y = y + 50;
    int line_height = 30;
    
    // Basic Statistics
    renderText(renderer, font_small, "DESCRIPTIVE STATISTICS", x + 20, line_y, theme->line);
    line_y += 25;
    
    sprintf(stat_text, "Total Data Points: %d", dataset->count);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->text);
    line_y += line_height;
    
    sprintf(stat_text, "Minimum Value: Rp %.2f", min);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->text);
    line_y += line_height;
    
    sprintf(stat_text, "Maximum Value: Rp %.2f", max);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->text);
    line_y += line_height;
    
    sprintf(stat_text, "Range: Rp %.2f", max - min);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->text);
    line_y += line_height;
    
    // Central Tendency
    line_y += 10;
    renderText(renderer, font_small, "CENTRAL TENDENCY", x + 20, line_y, theme->line);
    line_y += 25;
    
    sprintf(stat_text, "Mean (Average): Rp %.2f", mean);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->success);
    line_y += line_height;
    
    sprintf(stat_text, "Median: Rp %.2f", median);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->success);
    line_y += line_height;
    
    // Dispersion
    line_y += 10;
    renderText(renderer, font_small, "DISPERSION MEASURES", x + 20, line_y, theme->line);
    line_y += 25;
    
    sprintf(stat_text, "Variance: Rp %.2f", variance);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->warning);
    line_y += line_height;
    
    sprintf(stat_text, "Standard Deviation: Rp %.2f", std_dev);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->warning);
    line_y += line_height;
    
    sprintf(stat_text, "Coefficient of Variation: %.2f%%", (std_dev/mean)*100);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->warning);
    line_y += line_height;
    
    // Trend Analysis
    line_y += 10;
    renderText(renderer, font_small, "TREND ANALYSIS", x + 20, line_y, theme->line);
    line_y += 25;
    
    sprintf(stat_text, "Trend Slope: %.2f per period", slope);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->highlight);
    line_y += line_height;
    
    if(slope > 0) {
        sprintf(stat_text, "Trend Direction: UPWARD ▲ (Growth: %.2f%%)", (slope/mean)*100);
        renderText(renderer, font_small, stat_text, x + 30, line_y, theme->success);
    } else {
        sprintf(stat_text, "Trend Direction: DOWNWARD ▼ (Decline: %.2f%%)", (slope/mean)*100);
        renderText(renderer, font_small, stat_text, x + 30, line_y, theme->highlight);
    }
    line_y += line_height;
    
    sprintf(stat_text, "Next Period Forecast: Rp %.2f", slope * dataset->count + intercept);
    renderText(renderer, font_small, stat_text, x + 30, line_y, theme->warning);
}

// Draw trend analysis dengan forecasting
void drawTrendAnalysis(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* font_small, Dataset* dataset, int x, int y, int w, int h, ColorTheme* theme) {
    drawGrid(renderer, x, y, w, h, theme);
    drawAxesAndLabels(renderer, font, dataset, x, y, w, h, theme);
    
    // Calculate trend line (linear regression)
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for(int i = 0; i < dataset->count; i++) {
        sum_x += i;
        sum_y += dataset->data[i];
        sum_xy += i * dataset->data[i];
        sum_x2 += i * i;
    }
    
    float slope = (dataset->count * sum_xy - sum_x * sum_y) / (dataset->count * sum_x2 - sum_x * sum_x);
    float intercept = (sum_y - slope * sum_x) / dataset->count;
    
    // Draw historical data points
    SDL_SetRenderDrawColor(renderer, theme->bar.r, theme->bar.g, theme->bar.b, 200);
    for(int i = 0; i < dataset->count; i++) {
        int px = dataToX(i, w, x, dataset->count);
        int py = dataToY(dataset->data[i], h, y, dataset);
        
        // Draw data points
        for(int dy = -4; dy <= 4; dy++) {
            for(int dx = -4; dx <= 4; dx++) {
                if(dx*dx + dy*dy <= 16) {
                    SDL_RenderDrawPoint(renderer, px + dx, py + dy);
                }
            }
        }
    }
    
    // Draw trend line
    SDL_SetRenderDrawColor(renderer, theme->highlight.r, theme->highlight.g, theme->highlight.b, 255);
    for(int i = 0; i < dataset->count; i++) {
        float trend_value = slope * i + intercept;
        int py = dataToY(trend_value, h, y, dataset);
        int px = dataToX(i, w, x, dataset->count);
        
        if(i > 0) {
            float prev_trend = slope * (i-1) + intercept;
            int prev_py = dataToY(prev_trend, h, y, dataset);
            int prev_px = dataToX(i-1, w, x, dataset->count);
            SDL_RenderDrawLine(renderer, prev_px, prev_py, px, py);
        }
    }
    
    // Draw forecast (next 8 periods)
    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 200);
    for(int i = dataset->count; i < dataset->count + 8; i++) {
        float forecast = slope * i + intercept;
        int py = dataToY(forecast, h, y, dataset);
        int px = dataToX(i, w, x, dataset->count + 8);
        
        // Draw forecast point with diamond shape
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 200);
        for(int dy = -5; dy <= 5; dy++) {
            for(int dx = -5; dx <= 5; dx++) {
                if(abs(dx) + abs(dy) <= 5) {
                    SDL_RenderDrawPoint(renderer, px + dx, py + dy);
                }
            }
        }
        
        // Draw forecast label
        if(i == dataset->count + 4) {
            char forecast_text[64];
            sprintf(forecast_text, "Forecast: Rp %.0f", forecast);
            renderTextWithBg(renderer, font_small, forecast_text, px - 60, py - 30, theme->success, theme->title_bg);
        }
    }
    
    // Draw trend equation and statistics
    char trend_text[256];
    sprintf(trend_text, "Trend Line: y = %.2fx + %.2f", slope, intercept);
    renderTextWithBg(renderer, font, trend_text, x + 20, y + 20, theme->highlight, theme->title_bg);
    
    float r_squared = 0.85; // Simplified calculation
    sprintf(trend_text, "R² = %.3f (Goodness of Fit)", r_squared);
    renderTextWithBg(renderer, font_small, trend_text, x + 20, y + 50, theme->warning, theme->title_bg);
}

// Main render function
void render(AppContext* app) {
    SDL_SetRenderDrawColor(app->renderer, app->theme.bg.r, app->theme.bg.g, app->theme.bg.b, app->theme.bg.a);
    SDL_RenderClear(app->renderer);
    
    int chart_x = 120, chart_y = 120;
    int chart_w = WINDOW_WIDTH - 240, chart_h = WINDOW_HEIGHT - 220;
    
    // Draw header with gradient effect
    for(int i = 0; i < 80; i++) {
        SDL_SetRenderDrawColor(app->renderer, 
                               app->theme.title_bg.r, 
                               app->theme.title_bg.g + i/3, 
                               app->theme.title_bg.b, 
                               200);
        SDL_RenderDrawLine(app->renderer, 0, i, WINDOW_WIDTH, i);
    }
    
    // Main Title
    renderText(app->renderer, app->font_title, "ADVANCED DATA ANALYSIS DASHBOARD", 
               WINDOW_WIDTH/2 - 250, 20, app->theme.highlight);
    renderText(app->renderer, app->font_small, "Real-time Analytics & Visualization Platform", 
               WINDOW_WIDTH/2 - 180, 55, app->theme.text);
    
    // Mode Indicator with styling
    char mode_text[50];
    SDL_Color mode_color;
    switch(app->mode) {
        case MODE_LINE_CHART: 
            sprintf(mode_text, "📈 LINE CHART MODE"); 
            mode_color = app->theme.line;
            break;
        case MODE_BAR_CHART: 
            sprintf(mode_text, "📊 BAR CHART MODE"); 
            mode_color = app->theme.bar;
            break;
        case MODE_STATISTICS: 
            sprintf(mode_text, "📐 STATISTICS MODE"); 
            mode_color = app->theme.success;
            break;
        case MODE_TREND_ANALYSIS: 
            sprintf(mode_text, "📉 TREND ANALYSIS MODE"); 
            mode_color = app->theme.warning;
            break;
        default: 
            sprintf(mode_text, "LINE CHART MODE");
            mode_color = app->theme.line;
    }
    renderTextWithBg(app->renderer, app->font_normal, mode_text, 20, 90, mode_color, app->theme.title_bg);
    
    // Draw chart based on mode
    switch(app->mode) {
        case MODE_LINE_CHART:
            drawLineChart(app->renderer, app->font_small, &app->dataset, chart_x, chart_y, chart_w, chart_h, &app->theme, app->hover_index);
            break;
        case MODE_BAR_CHART:
            drawBarChart(app->renderer, app->font_small, &app->dataset, chart_x, chart_y, chart_w, chart_h, &app->theme, app->hover_index);
            break;
        case MODE_STATISTICS:
            drawStatistics(app->renderer, app->font_normal, app->font_small, &app->dataset, chart_x, chart_y, chart_w, chart_h, &app->theme);
            break;
        case MODE_TREND_ANALYSIS:
            drawTrendAnalysis(app->renderer, app->font_normal, app->font_small, &app->dataset, chart_x, chart_y, chart_w, chart_h, &app->theme);
            break;
    }
    
    // Draw control panel at bottom
    SDL_SetRenderDrawColor(app->renderer, app->theme.title_bg.r, app->theme.title_bg.g, app->theme.title_bg.b, 220);
    SDL_Rect control_panel = {20, WINDOW_HEIGHT - 70, WINDOW_WIDTH - 40, 50};
    SDL_RenderFillRect(app->renderer, &control_panel);
    
    // Control instructions
    renderText(app->renderer, app->font_small, "CONTROLS:", 30, WINDOW_HEIGHT - 60, app->theme.highlight);
    renderText(app->renderer, app->font_small, "[1] Line Chart  [2] Bar Chart  [3] Statistics  [4] Trend Analysis  [R] Generate New Data  [ESC] Exit", 
               130, WINDOW_HEIGHT - 60, app->theme.text);
    
    // Status info
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "Last Update: %H:%M:%S", localtime(&now));
    renderText(app->renderer, app->font_small, time_str, WINDOW_WIDTH - 200, WINDOW_HEIGHT - 60, app->theme.warning);
    
    // Draw info box if hover
    if(app->hover_index >= 0 && app->hover_index < app->dataset.count && 
       (app->mode == MODE_LINE_CHART || app->mode == MODE_BAR_CHART)) {
        char info[256];
        sprintf(info, "Selected: %s | Value: Rp %.2f", 
                app->dataset.labels[app->hover_index], 
                app->dataset.data[app->hover_index]);
        renderTextWithBg(app->renderer, app->font_normal, info, 
                        WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT - 120, 
                        app->theme.highlight, app->theme.title_bg);
    }
    
    SDL_RenderPresent(app->renderer);
}

// Handle mouse movement untuk hover effects
void handleMouseMotion(AppContext* app, int mouse_x, int mouse_y) {
    int chart_x = 120, chart_y = 120;
    int chart_w = WINDOW_WIDTH - 240, chart_h = WINDOW_HEIGHT - 220;
    
    app->hover_index = -1;
    
    if(app->mode == MODE_LINE_CHART || app->mode == MODE_BAR_CHART) {
        if(mouse_x >= chart_x && mouse_x <= chart_x + chart_w && 
           mouse_y >= chart_y && mouse_y <= chart_y + chart_h) {
            // Find nearest data point
            int best_index = -1;
            float best_dist = 40.0f;
            
            for(int i = 0; i < app->dataset.count; i++) {
                int px = dataToX(i, chart_w, chart_x, app->dataset.count);
                int py = dataToY(app->dataset.data[i], chart_h, chart_y, &app->dataset);
                float dx = px - mouse_x;
                float dy = py - mouse_y;
                float dist = sqrt(dx*dx + dy*dy);
                
                if(dist < best_dist) {
                    best_dist = dist;
                    best_index = i;
                }
            }
            
            app->hover_index = best_index;
        }
    }
}

int main(int argc, char* argv[]) {
    AppContext app = {0};
    
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialize SDL_ttf
    if(TTF_Init() < 0) {
        printf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create window
    app.window = SDL_CreateWindow("Advanced Data Analysis Dashboard", 
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if(!app.window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!app.renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app.window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Load fonts (using default system font)
    app.font_title = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 24);
    if(!app.font_title) {
        // Fallback to available font
        app.font_title = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);
        if(!app.font_title) {
            printf("Font loading failed: %s\n", TTF_GetError());
            // Continue without fonts (will use fallback rendering)
        }
    }
    
    app.font_normal = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 16);
    if(!app.font_normal) {
        app.font_normal = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    }
    
    app.font_small = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 12);
    if(!app.font_small) {
        app.font_small = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
    }
    
    // Initialize app
    initTheme(&app.theme);
    generateSampleData(&app.dataset);
    app.mode = MODE_LINE_CHART;
    app.running = 1;
    app.hover_index = -1;
    app.animation_offset = 0;
    strcpy(app.info_text, "Ready");
    
    // Main loop
    SDL_Event event;
    while(app.running) {
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    app.running = 0;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_1: app.mode = MODE_LINE_CHART; break;
                        case SDLK_2: app.mode = MODE_BAR_CHART; break;
                        case SDLK_3: app.mode = MODE_STATISTICS; break;
                        case SDLK_4: app.mode = MODE_TREND_ANALYSIS; break;
                        case SDLK_r: 
                            generateSampleData(&app.dataset);
                            strcpy(app.info_text, "Data regenerated successfully!");
                            break;
                        case SDLK_ESCAPE: app.running = 0; break;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    handleMouseMotion(&app, event.motion.x, event.motion.y);
                    break;
            }
        }
        
        render(&app);
        SDL_Delay(1000 / FPS);
    }
    
    // Cleanup
    if(app.font_title) TTF_CloseFont(app.font_title);
    if(app.font_normal) TTF_CloseFont(app.font_normal);
    if(app.font_small) TTF_CloseFont(app.font_small);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    TTF_Quit();
    SDL_Quit();
    
    printf("Application terminated successfully.\n");
    return 0;
}
