#include <stdio.h>
#include <math.h>
#include <SDL3/SDL.h>

typedef struct color {
	Uint8 r, g, b;
} color_t;

#define WINDOW_WIDTH (800)
#define WINDOW_HEIGHT (800)

void swap(float *a, float *b) {
	float temp;

	temp = *a;
	*a = *b;
	*b = temp;	
}

void draw_line(SDL_Renderer *renderer, float x0, float y0, float x1, float y1, color_t color) {
	bool steep;
	float x;
	float y;
	float dx;
	float dy;
	float derror;
	float error;

	steep = false;

	if (fabs(x0 - x1) < fabs(y0 - y1)) {
		swap(&x0, &y0);
		swap(&x1, &y1);

		steep = true;
	}

	if (x0 > x1) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}

	dx = x1 - x0;
	dy = y1 - y0;
	derror = fabs(dy) * 2;
	error = 0;
	y = y0;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
	for (x = x0; x <= x1; ++x) {
		if (steep)
			SDL_RenderPoint(renderer, y, x);
		else
			SDL_RenderPoint(renderer, x, y);
	
		error += derror;
		if (error > dx) {
			y += (y1 > y0 ? 1 : -1);
			error -= dx * 2;
		}
	}
}

int main(void)
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	bool running;
	SDL_Event event;
	color_t white;
	color_t red;

	white.r = 255;
	white.g = 255;
	white.b = 255;
	red.r = 255;
	red.g = 0;
	red.b = 0;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("renderer", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);

	draw_line(renderer, 13, 20, 80, 40, white);
	draw_line(renderer, 20, 13, 40, 80, red);
	draw_line(renderer, 80, 40, 13, 20, red);

	SDL_RenderPresent(renderer);

	running = true;
	while (running) {
		if (SDL_PollEvent(&event)) {
			switch (event.type)
			{
			case SDL_EVENT_QUIT:
				return 0;
			}
		}
	}

	return 0;
}
