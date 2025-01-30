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

void swapf(float *a, float *b)
{
	float temp;

	temp = *a;
	*a = *b;
	*b = temp;	
}

void swapvec3f(vec3f_t *a, vec3f_t *b)
{
	vec3f_t temp;

	temp = *a;
	*a = *b;
	*b = temp;	
}

vec3f_t vec3f_add(vec3f_t u, vec3f_t v)
{
	vec3f_t ans;
	ans.x = u.x + v.x;
	ans.y = u.y + v.y;
	ans.z = u.z + v.z;

	return ans;
}

vec3f_t vec3f_scale(vec3f_t u, float k)
{
	vec3f_t ans;
	ans.x = u.x * k;
	ans.y = u.y * k;
	ans.z = u.z * k;

	return ans;
}

float vec3f_dot(vec3f_t u, vec3f_t v)
{
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

vec3f_t vec3f_cross(vec3f_t u, vec3f_t v)
{
	vec3f_t ans;
	ans.x = u.y * v.z - u.z * v.y;
	ans.y = u.z * v.x - u.x * v.z;
	ans.z = u.x * v.y - u.y * v.x;

	return ans;
}

vec3f_t vec3f_norm(vec3f_t u)
{
	float magnitude;
	magnitude = sqrt(u.x * u.x + u.y * u.y + u.z * u.z);

	return vec3f_scale(u, 1.0 / magnitude);
}

vec3f_t vec3f_mult_mat3f(vec3f_t v, float *m)
{
	vec3f_t ans;
	ans.x = m[0] * v.x + m[3] * v.y + m[6] * v.z;
	ans.y = m[1] * v.x + m[4] * v.y + m[7] * v.z;
	ans.z = m[2] * v.x + m[5] * v.y + m[8] * v.z;

	return ans;
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
		swapf(&x0, &y0);
		swapf(&x1, &y1);

		steep = true;
	}

	if (x0 > x1) {
		swapf(&x0, &x1);
		swapf(&y0, &y1);
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

void triangle(SDL_Renderer *renderer, vec3f_t a, vec3f_t b, vec3f_t c, SDL_Color color)
{
	unsigned total_height;
	unsigned segment_height;
	float y;
	float alpha;
	float beta;
	float x0;
	float x1;
	float j;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	if (a.y > b.y)
		swapvec3f(&a, &b);
	if (a.y > c.y)
		swapvec3f(&a, &c);
	if (b.y > c.y)
		swapvec3f(&b, &c);

	total_height = c.y - a.y;
	if (total_height == 0)
		return;

	segment_height = b.y - a.y + 1;
	if (segment_height != 0) {
		for (y = a.y; y <= b.y; y += 1.0) {
			alpha = (y - a.y) / total_height;
			beta = (y - a.y) / segment_height;
			x0 = a.x + alpha * (c.x - a.x);
			x1 = a.x + beta * (b.x - a.x);
			if (x0 > x1)
				swapf(&x0, &x1);

			for (j = x0; j <= x1; j += 1.0) {
				SDL_RenderPoint(renderer, j, y);
			}
		}
	}

	segment_height = c.y - b.y + 1;
	if (segment_height != 0) {
		for (y = b.y; y <= c.y; ++y) {
			alpha = (y - a.y) / total_height;
			beta = (y - b.y) / segment_height;
			x0 = a.x + (c.x - a.x) * alpha;
			x1 = b.x + (c.x - b.x) * beta;
			if (x0 > x1)
				swapf(&x0, &x1);

			for (j = x0; j <= x1; ++j) {
				SDL_RenderPoint(renderer, j, y);
			}
		}
	}
}

void objmodel_parse(objmodel_t *objmodel, const char *filename)
{
	FILE *fp;
	int c;
	vec3f_t *v;
	face_t *f;
	unsigned averts;
	unsigned afaces;

	averts = 256;
	objmodel->verts = malloc(averts * sizeof(*objmodel->verts));
	afaces = 256;
	objmodel->faces = malloc(afaces * sizeof(*objmodel->faces));
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
				if (fscanf(fp, "%f %f %f", &(v->x), &(v->y), &(v->z)) == 3) {
					objmodel->nverts += 1;
					if (averts <= objmodel->nverts) {
						averts += 256;
						objmodel->verts = realloc(objmodel->verts, averts * sizeof(*objmodel->verts));
					}
				}
			}
		}

		if (c == 'f') {
			f = objmodel->faces + objmodel->nfaces;
			if (fscanf(fp, "%u/%*u/%*u %u/%*u/%*u %u/%*u/%*u", f->v + 0, f->v + 1, f->v + 2) == 3) {
				objmodel->nfaces += 1;
				if (afaces <= objmodel->nfaces) {
					afaces += 256;
					objmodel->faces = realloc(objmodel->faces, afaces * sizeof(*objmodel->faces));
				}
			}
		}
	}
}

void objmodel_draw(objmodel_t *objmodel, SDL_Renderer *renderer, SDL_Color original_color)
{
	unsigned i;
	face_t f;
	vec3f_t a;
	vec3f_t b;
	vec3f_t c;
	vec3f_t cross;
	float brightness;
	vec3f_t ba;
	vec3f_t ca;
	SDL_Color color;

	float flip[] = {1, 0, 0,
			0, -1, 0,
			0, 0, 1};

	vec3f_t translate = {1.0, 1.0, 1.0};

	float scale[] = {WINDOW_WIDTH / 2.0, 0, 0,
			0, WINDOW_HEIGHT / 2.0, 0,
			0, 0, 1};

	vec3f_t light = {0, 0, 1};

	color = original_color;

	for (i = 0; i < objmodel->nfaces; ++i) {
		f = objmodel->faces[i];

		a = objmodel->verts[f.v[0] - 1];
		b = objmodel->verts[f.v[1] - 1];
		c = objmodel->verts[f.v[2] - 1];

		ba = vec3f_add(b, vec3f_scale(a, -1));
		ca = vec3f_add(c, vec3f_scale(a, -1));
		cross = vec3f_cross(ba, ca);
		cross = vec3f_norm(cross);
		brightness = vec3f_dot(cross, light);

		if (brightness > 0) {
			color.r = original_color.r * brightness;
			color.g = original_color.g * brightness;
			color.b = original_color.b * brightness;
			color.a = original_color.a;

			a = vec3f_mult_mat3f(a, flip);
			b = vec3f_mult_mat3f(b, flip);
			c = vec3f_mult_mat3f(c, flip);

			a = vec3f_add(a, translate);
			b = vec3f_add(b, translate);
			c = vec3f_add(c, translate);

			a = vec3f_mult_mat3f(a, scale);
			b = vec3f_mult_mat3f(b, scale);
			c = vec3f_mult_mat3f(c, scale);

			triangle(renderer, a, b, c, color);
		}
	}
}

int main(void)
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	bool running;
	SDL_Event event;
	SDL_KeyboardEvent keyboard_event;
	SDL_Color white = {255, 255, 255, SDL_ALPHA_OPAQUE};
	SDL_Color red = {255, 0, 0, SDL_ALPHA_OPAQUE};
	SDL_Color green = {0, 255, 0, SDL_ALPHA_OPAQUE};
	objmodel_t objmodel;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("renderer", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);

	objmodel_parse(&objmodel, "obj/african.obj");
	objmodel_draw(&objmodel, renderer, white);
	SDL_RenderPresent(renderer);

	running = true;
	while (running) {
		SDL_WaitEvent(&event);

		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			return 0;
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			keyboard_event = event.key;
			if (keyboard_event.key == SDLK_Q)
				return 0;
		}
	}

	return 0;
}
