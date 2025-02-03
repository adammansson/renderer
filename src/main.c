#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>

#define WINDOW_WIDTH (800)
#define WINDOW_HEIGHT (800)

typedef struct vec2i {
	int x, y;
} vec2i_t;

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

static vec3f_t vec3f_add(vec3f_t u, vec3f_t v)
{
	vec3f_t ans;
	ans.x = u.x + v.x;
	ans.y = u.y + v.y;
	ans.z = u.z + v.z;

	return ans;
}

static vec3f_t vec3f_scale(vec3f_t u, float k)
{
	vec3f_t ans;
	ans.x = u.x * k;
	ans.y = u.y * k;
	ans.z = u.z * k;

	return ans;
}

static float vec3f_dot(vec3f_t u, vec3f_t v)
{
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

static vec3f_t vec3f_cross(vec3f_t u, vec3f_t v)
{
	vec3f_t ans;
	ans.x = u.y * v.z - u.z * v.y;
	ans.y = u.z * v.x - u.x * v.z;
	ans.z = u.x * v.y - u.y * v.x;

	return ans;
}

static vec3f_t vec3f_norm(vec3f_t u)
{
	float magnitude;
	magnitude = sqrt(u.x * u.x + u.y * u.y + u.z * u.z);

	return vec3f_scale(u, 1.0 / magnitude);
}

static vec3f_t vec3f_mult_mat3f(vec3f_t v, float *m)
{
	vec3f_t ans;
	ans.x = m[0] * v.x + m[3] * v.y + m[6] * v.z;
	ans.y = m[1] * v.x + m[4] * v.y + m[7] * v.z;
	ans.z = m[2] * v.x + m[5] * v.y + m[8] * v.z;

	return ans;
}

static vec3f_t barycentric(vec3f_t a, vec3f_t b, vec3f_t c, float px, float py)
{
	vec3f_t u; 
	vec3f_t v;
	vec3f_t cross;
	vec3f_t ans;

	u.x = c.x - a.x;
	u.y = b.x - a.x;
	u.z = a.x - px;

	v.x = c.y - a.y;
	v.y = b.y - a.y;
	v.z = a.y - py;

	cross = vec3f_cross(u, v);

	if (fabs(cross.z) < 1.0) {
		ans.x = -1;
		ans.y = 0;
		ans.z = 0;
		return ans;
	}

	ans.x = 1.0 - (cross.x + cross.y) / cross.z;
	ans.y = cross.y / cross.z;
	ans.z = cross.x / cross.z;
	return ans;
}

static float minf(float a, float b)
{
	return a < b ? a : b;
}

static float maxf(float a, float b)
{
	return a > b ? a : b;
}

static void triangle(SDL_Surface *surface, vec3f_t a, vec3f_t b, vec3f_t c, SDL_Color color, float *zbuffer)
{
	float minx;
	float maxx;
	float miny;
	float maxy;
	int px;
	int py;
	float pz;
	vec3f_t bc;
	int i;

	minx = minf(a.x, minf(b.x, c.x));
	maxx = maxf(a.x, maxf(b.x, c.x));
	miny = minf(a.y, minf(b.y, c.y));
	maxy = maxf(a.y, maxf(b.y, c.y));

	for (py = (int) floor(miny); py <= (int) ceil(maxy); ++py) {
		for (px = (int) floor(minx); px <= (int) ceil(maxx); ++px) {
			bc = barycentric(a, b, c, px + 0.5, py + 0.5);

			if (bc.x >= 0.0 && bc.y >= 0.0 && bc.z >= 0.0) {
				pz = 0.0;
				pz += a.z * bc.x;
				pz += b.z * bc.y;
				pz += c.z * bc.z;

				i = px + py * WINDOW_WIDTH;
				if (zbuffer[i] < pz) {
					zbuffer[i] = pz;
					SDL_WriteSurfacePixel(surface, px, py, color.r, color.g, color.b, color.a);
				}
			}
		}
	}
}

static void objmodel_parse(objmodel_t *objmodel, const char *filename)
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

static void objmodel_draw(objmodel_t *objmodel, SDL_Surface *surface, SDL_Color original_color)
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
	float zbuffer[WINDOW_WIDTH * WINDOW_HEIGHT];

	float flip[] = {1, 0, 0,
			0, -1, 0,
			0, 0, 1};

	vec3f_t translate = {1.0, 1.0, 1.0};

	float scale[] = {400.0, 0, 0,
			0, 400.0, 0,
			0, 0, 400.0};

	vec3f_t light = {0, 0, 1};

	color = original_color;

	for (i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; ++i)
		zbuffer[i] = -100000.0;

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

			triangle(surface, a, b, c, color, zbuffer);
		}
	}
}

int main(void)
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Surface *surface;
	SDL_Texture *texture;

	bool running;
	SDL_Event event;
	SDL_KeyboardEvent keyboard_event;
	objmodel_t objmodel;
	SDL_Color white = {255, 255, 255, SDL_ALPHA_OPAQUE};
	SDL_Color red = {255, 0, 0, SDL_ALPHA_OPAQUE};

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("renderer", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
	surface = SDL_CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_PIXELFORMAT_RGBA8888);

	objmodel_parse(&objmodel, "obj/african.obj");
	objmodel_draw(&objmodel, surface, white);

	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_SetRenderTarget(renderer, texture);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	running = true;
	while (running) {
		SDL_WaitEvent(&event);

		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			return 0;
		case SDL_EVENT_KEY_UP:
			keyboard_event = event.key;
			if (keyboard_event.key == SDLK_Q)
				return 0;
			else if (keyboard_event.key == SDLK_R) {
				printf("RENDER\n");
			}
		}
	}

	return 0;
}
