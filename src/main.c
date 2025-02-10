#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

typedef struct vec4f {
	float x, y, z, w;
} vec4f_t;

typedef struct objmodel {
	vec3f_t *verts; 
	vec3f_t *norms; 
	face_t *faces;
	unsigned nverts;
	unsigned nnorms;
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
	ans.x = m[0] * v.x + m[1] * v.y + m[2] * v.z;
	ans.y = m[3] * v.x + m[4] * v.y + m[5] * v.z;
	ans.z = m[6] * v.x + m[7] * v.y + m[8] * v.z;

	return ans;
}

static vec4f_t vec3f_to_vec4f(vec3f_t v)
{
	vec4f_t ans;
	ans.x = v.x;
	ans.y = v.y;
	ans.z = v.z;
	ans.w = 1.0;

	return ans;
}

static vec4f_t vec4f_mult_mat4f(vec4f_t v, float *m)
{
	vec4f_t ans;
	ans.x = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
	ans.y = m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7] * v.w;
	ans.z = m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11] * v.w;
	ans.w = m[12] * v.x + m[13] * v.y + m[14] * v.z + m[15] * v.w;

	return ans;
}

static vec3f_t vec4f_to_vec3f(vec4f_t v)
{
	vec3f_t ans;
	ans.x = v.x / v.w;
	ans.y = v.y / v.w;
	ans.z = v.z / v.w;

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

	minx = maxf(0.0, minf(a.x, minf(b.x, c.x)));
	miny = maxf(0.0, minf(a.y, minf(b.y, c.y)));
	maxx = minf((float) WINDOW_WIDTH, maxf(a.x, maxf(b.x, c.x)));
	maxy = minf((float) WINDOW_HEIGHT, maxf(a.y, maxf(b.y, c.y)));

	for (py = (int) floor(miny); py <= (int) ceil(maxy); ++py) {
		for (px = (int) floor(minx); px <= (int) ceil(maxx); ++px) {
			bc = barycentric(a, b, c, px + 0.5, py + 0.5);

			if (bc.x < 0.0 || bc.y < 0.0 || bc.z < 0.0)
				continue;
			
			pz = a.z * bc.x;
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

static void objmodel_parse(objmodel_t *objmodel, const char *filename)
{
	FILE *fp;
	int c;
	vec3f_t *v;
	vec3f_t *vn;
	face_t *f;
	unsigned averts;
	unsigned anorms;
	unsigned afaces;

	averts = 256;
	objmodel->verts = malloc(averts * sizeof(*objmodel->verts));
	anorms = 256;
	objmodel->norms = malloc(anorms * sizeof(*objmodel->norms));
	afaces = 256;
	objmodel->faces = malloc(afaces * sizeof(*objmodel->faces));
	objmodel->nverts = 0;
	objmodel->nnorms = 0;
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
			if (c == 'n') {
				vn = objmodel->norms + objmodel->nnorms;
				if (fscanf(fp, "%f %f %f", &(vn->x), &(vn->y), &(vn->z)) == 3) {
					objmodel->nnorms += 1;
					if (anorms <= objmodel->nnorms) {
						anorms += 256;
						objmodel->norms = realloc(objmodel->norms, anorms * sizeof(*objmodel->norms));
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

static void objmodel_draw(objmodel_t *objmodel, SDL_Surface *surface, float *viewport, float *projection, float *view, vec3f_t light)
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
	vec4f_t ae;
	vec4f_t be;
	vec4f_t ce;

	for (i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; ++i)
		zbuffer[i] = -1e-10;

	for (i = 0; i < objmodel->nfaces; ++i) {
		f = objmodel->faces[i];

		a = objmodel->verts[f.v[0] - 1];
		b = objmodel->verts[f.v[1] - 1];
		c = objmodel->verts[f.v[2] - 1];

		ba = vec3f_add(b, vec3f_scale(a, -1));
		ca = vec3f_add(c, vec3f_scale(a, -1));
		cross = vec3f_norm(vec3f_cross(ba, ca));
		brightness = vec3f_dot(cross, light);

		color.r = 255 * maxf(0.1, brightness);
		color.g = 255 * maxf(0.1, brightness);
		color.b = 255 * maxf(0.1, brightness);
		color.a = SDL_ALPHA_OPAQUE;

		ae = vec3f_to_vec4f(a);
		be = vec3f_to_vec4f(b);
		ce = vec3f_to_vec4f(c);

		ae = vec4f_mult_mat4f(ae, view);
		be = vec4f_mult_mat4f(be, view);
		ce = vec4f_mult_mat4f(ce, view);

		ae = vec4f_mult_mat4f(ae, projection);
		be = vec4f_mult_mat4f(be, projection);
		ce = vec4f_mult_mat4f(ce, projection);

		ae = vec4f_mult_mat4f(ae, viewport);
		be = vec4f_mult_mat4f(be, viewport);
		ce = vec4f_mult_mat4f(ce, viewport);

		a = vec4f_to_vec3f(ae);
		b = vec4f_to_vec3f(be);
		c = vec4f_to_vec3f(ce);

		triangle(surface, a, b, c, color, zbuffer);
	}
}

static void init_viewport(float *m, float x, float y, float w, float h)
{
	float depth;
	
	depth = 255.0;
	memset(m, 0.0, 16 * sizeof(*m));

	m[0 * 4 + 3] = x + w / 2.0;
	m[1 * 4 + 3] = y + h / 2.0;
	m[2 * 4 + 3] = depth / 2.0;

	m[0 * 4 + 0] = w / 2.0;
	m[1 * 4 + 1] = -h / 2.0;
	m[2 * 4 + 2] = depth / 2.0;
	m[3 * 4 + 3] = 1.0;
}

static void init_lookat(float *m, vec3f_t eye, vec3f_t center, vec3f_t up)
{
	vec3f_t a;
	vec3f_t b;
	vec3f_t c;

	memset(m, 0.0, 16 * sizeof(*m));
	m[0 * 4 + 0] = 1.0;
	m[1 * 4 + 1] = 1.0;
	m[2 * 4 + 2] = 1.0;
	m[3 * 4 + 3] = 1.0;

	a = vec3f_norm(vec3f_add(eye, vec3f_scale(center, -1)));
	b = vec3f_norm(vec3f_cross(up, a));
	c = vec3f_norm(vec3f_cross(a, b));

	m[0 * 4 + 0] = b.x;
	m[1 * 4 + 0] = c.x;
	m[2 * 4 + 0] = a.x;
	m[0 * 4 + 3] = -center.x;

	m[0 * 4 + 1] = b.y;
	m[1 * 4 + 1] = c.y;
	m[2 * 4 + 1] = a.y;
	m[1 * 4 + 3] = -center.y;

	m[0 * 4 + 2] = b.z;
	m[1 * 4 + 2] = c.z;
	m[2 * 4 + 2] = a.z;
	m[2 * 4 + 3] = -center.z;
}

static void render(SDL_Renderer *renderer, objmodel_t *objmodel, SDL_Surface *surface, float *viewport, float *projection, float *view, vec3f_t light)
{
	SDL_Texture *texture;

	SDL_ClearSurface(surface, 0.0, 0.0, 0.0, 255.0);
	SDL_RenderClear(renderer);

	objmodel_draw(objmodel, surface, viewport, projection, view, light);
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_SetRenderTarget(renderer, texture);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

int main(void)
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Surface *surface;

	bool running;
	bool should_rerender;
	SDL_Event event;
	SDL_KeyboardEvent keyboard_event;
	objmodel_t objmodel;

	float viewport[16];

	float projection[16] =
		{1.0,	0,	0,	0,
		0,	1.0,	0,	0,
		0,	0,	1.0,	0,
		0,	0,	0,	1.0};

	float view[16];

	vec3f_t light =
		{0.0,	0.0,	1.0};

	vec3f_t eye =
		{0.0,	0.0,	3.0};

	vec3f_t center =
		{0.0,	0.0,	0.0};

	vec3f_t up =
		{0.0,	1.0,	0.0};

	init_viewport(viewport, 100.0, 100.0, 600.0, 600.0);
	init_lookat(view, eye, center, up);
	projection[3 * 4 + 2] = -1.0 / eye.z;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("renderer", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
	surface = SDL_CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_PIXELFORMAT_RGBA8888);

	objmodel_parse(&objmodel, "obj/african.obj");

	running = true;
	should_rerender = true;
	while (running) {
		if (should_rerender) {
			render(renderer, &objmodel, surface, viewport, projection, view, light);
			should_rerender = false;
		}

		SDL_WaitEvent(&event);

		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			return 0;

		case SDL_EVENT_KEY_UP:
			keyboard_event = event.key;
			switch (keyboard_event.key)
			{
			case SDLK_Q:
				return 0;

			case SDLK_W:
				eye.y += 1;
				init_lookat(view, eye, center, up);
				should_rerender = true;
				break;

			case SDLK_A:
				eye.x -= 1;
				init_lookat(view, eye, center, up);
				should_rerender = true;
				break;

			case SDLK_S:
				eye.y -= 1;
				init_lookat(view, eye, center, up);
				should_rerender = true;
				break;

			case SDLK_D:
				eye.x += 1;
				init_lookat(view, eye, center, up);
				should_rerender = true;
				break;
			}
		}
	}

	return 0;
}
