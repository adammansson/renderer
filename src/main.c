#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL3/SDL.h>

#define WINDOW_WIDTH (800)
#define WINDOW_HEIGHT (800)

typedef struct vec3f {
	float x, y, z;
} vec3f_t;

typedef struct face {
	unsigned v[3];
} face_t;

typedef struct objmodel {
	vec3f_t *verts; 
	face_t *faces;
	unsigned nverts;
	unsigned nfaces;
} objmodel_t;

void swap(float *a, float *b)
{
	float temp;

	temp = *a;
	*a = *b;
	*b = temp;	
}

void line(SDL_Renderer *renderer, float x0, float y0, float x1, float y1, SDL_Color color)
{
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

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
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

void objmodel_parse(objmodel_t *objmodel, const char *filename)
{
	FILE *fp;
	vec3f_t *v;
	face_t *f;
	int c;

	objmodel->verts = malloc(1258 * sizeof(vec3f_t));
	objmodel->faces = malloc(2492 * sizeof(face_t));
	objmodel->nverts = 0;
	objmodel->nfaces = 0;
	fp = fopen(filename, "r");

	while (1) {
		c = getc(fp);
		if (c == EOF)
			return;

		if (c == 'v') {
			c = getc(fp);
			if (c == ' ') {
				v = objmodel->verts + objmodel->nverts;
				if (fscanf(fp, "%f %f %f", &(v->x), &(v->y), &(v->z)) == 3)
					objmodel->nverts += 1;
			}
		}

		if (c == 'f') {
			f = objmodel->faces + objmodel->nfaces;
			if (fscanf(fp, "%u/%*u/%*u %u/%*u/%*u %u/%*u/%*u", f->v + 0, f->v + 1, f->v + 2) == 3)
				objmodel->nfaces += 1;
		}
	}
}

void objmodel_draw(objmodel_t *objmodel, SDL_Renderer *renderer, SDL_Color color)
{
	unsigned i;
	unsigned j;
	float x0;
	float y0;
	float x1;
	float y1;

	for (i = 0; i < objmodel->nfaces; ++i) {
		face_t f = objmodel->faces[i];
		for (j = 0; j < 3; ++j) {
			vec3f_t v0 = objmodel->verts[f.v[j] - 1];
			vec3f_t v1 = objmodel->verts[f.v[(j + 1) % 3] - 1];

			x0 = (v0.x + 1.0) * WINDOW_WIDTH / 2.0;
			y0 = (v0.y + 1.0) * WINDOW_HEIGHT / 2.0;
			y0 = WINDOW_HEIGHT - y0;

			x1 = (v1.x + 1.0) * WINDOW_WIDTH / 2.0;
			y1 = (v1.y + 1.0) * WINDOW_HEIGHT / 2.0;
			y1 = WINDOW_HEIGHT - y1;

			line(renderer, x0, y0, x1, y1, color);
		}
	}
}

int main(void)
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	bool running;
	SDL_Event event;
	SDL_Color white;
	SDL_Color red;
	objmodel_t objmodel;

	white.r = 255;
	white.g = 255;
	white.b = 255;
	white.a = SDL_ALPHA_OPAQUE;
	red.r = 255;
	red.g = 0;
	red.b = 0;
	red.a = SDL_ALPHA_OPAQUE;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("renderer", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);

	objmodel_parse(&objmodel, "obj/african.obj");
	printf("nverts = %u\n", objmodel.nverts);
	printf("nfaces = %u\n", objmodel.nfaces);

	objmodel_draw(&objmodel, renderer, white);
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
