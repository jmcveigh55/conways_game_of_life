#include <stdio.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "cell.h"
#include "utilities.h"

static cell_t *cell_init(size_t width, size_t height)
{
	cell_t *cell_new = malloc(sizeof(*cell_new));
	if (!cell_new) {
		perror("cell_init: Failed to malloc cell_new");
		exit(EXIT_FAILURE);
	}

	cell_new->rect.w = width;
	cell_new->rect.h = height;
	cell_new->alive = 0;

	return cell_new;
}

body_t *body_init(size_t rows, size_t cols)
{
	size_t x, y;

	body_t *body_new = malloc(sizeof(*body_new));
	if (!body_new) {
		perror("body_init: Failed to malloc body_new");
		exit(EXIT_FAILURE);
	}

	body_new->rows = rows;
	body_new->cols = cols;

	body_new->cells = malloc(body_new->rows * body_new->cols * sizeof(*body_new->cells));
	if (!body_new->cells) {
		perror("body_init: Failed to malloc body_new->cells");
		exit(EXIT_FAILURE);
	}

	for (x = 0; x < body_new->rows; x++)
		for (y = 0; y < body_new->cols; y++)
			body_new->cells[x * body_new->cols + y] = cell_init(cell_meta.width, cell_meta.height);

	return body_new;
}

static void cell_destroy(cell_t *cell)
{
	free(cell);
}

void body_destory(body_t *body)
{
	size_t x, y;

	for (x = 0; x < body->rows; x++)
		for (y = 0; y < body->cols; y++)
			cell_destroy(body->cells[x * body->cols + y]);
	free(body->cells);
	free(body);
}

static int draw_cell(SDL_Renderer *renderer, cell_t *cell, int x, int y)
{
	cell->rect.x = cell_meta.width * x;
	cell->rect.y = cell_meta.height * y;
	cell->rect.w = cell_meta.width;
	cell->rect.h = cell_meta.height;

	if (cell->alive)
		SDL_SetRenderDrawColor(renderer, cell_meta.color_r, cell_meta.color_g, cell_meta.color_b, SDL_ALPHA_OPAQUE);
	else
		SDL_SetRenderDrawColor(renderer, bg_meta.color_r, bg_meta.color_g, bg_meta.color_b, SDL_ALPHA_OPAQUE);

	return SDL_RenderFillRect(renderer, &cell->rect);
}

void draw_generation(SDL_Renderer *renderer, body_t *body)
{
	size_t x, y;

	for (x=0; x < body->rows; x++)
		for (y=0; y < body->cols; y++)
			draw_cell(renderer, body->cells[x * body->cols + y], x, y);
}

static void random_mode(body_t *body_new, body_t *body_old, int *pop)
{
	size_t x, y;

	for (x=(size_t)(body_new->rows * 0.25); x < (size_t)(body_new->rows * 0.75); x++) {
		for (y=(size_t)(body_new->cols * 0.25); y < (size_t)(body_new->cols * 0.75); y++) {
			body_new->cells[x * body_new->cols + y]->alive = ((rand() % 100 + 1) <= cell_meta.alive_prob);
			*pop += body_new->cells[x * body_new->cols + y]->alive;
		}
	}
}

static void pattern_mode(body_t *body_new, body_t *body_old, int *pop)
{
	FILE *pattern_fd;
	size_t len, x, y;
	char *pattern, *point;

	pattern = parse_pattern_choice();

	pattern_fd = fopen(pattern, "r");
	if (!pattern_fd) {
		perror("pattern_mode: Error opening pattern file");
		exit(EXIT_FAILURE);
	}
	free(pattern);

	point = NULL;
	getline(&point, &len, pattern_fd); /* Skip header */
	while (getline(&point, &len, pattern_fd) != -1) {
		point[strlen(point) - 1] = '\0';
		x = (size_t)atoi(strtok(point, ",")) + (body_new->rows * 0.5);
		y = (size_t)atoi(strtok(NULL, ",")) + (body_new->cols * 0.5);
		body_new->cells[x * body_new->cols + y]->alive = 1;
		*pop += 1;
	}

	fclose(pattern_fd);
	free(point);
}

static void drawing_mode(body_t *body_new, body_t *body_old, int *pop)
{
	return;
}

void inital_generation(body_t *body_new, body_t *body_old, int *pop)
{
	*pop = 0;

	switch (mode) {
		case 'r':
			random_mode(body_new, body_old, pop);
			break;
		case 'p':
			pattern_mode(body_new, body_old, pop);
			break;
		case 'd':
			fprintf(stderr, "initial_generation: mode %c is not implemented yet.\n", mode);
	}

}

void compute_generation(body_t *body_new, body_t *body_old, int *pop)
{
	size_t x, y, a, b, i;
	int neighbors;
	*pop = 0;

	for (x=0; x < body_new->rows; x++) {
		for (y=0; y < body_new->cols; y++) {
			neighbors = 0;

			for (a=0; a < 3; a++) {
				for (b=0; b < 3; b++) {
					if (x - 1 + a < 0 || x - 1 + a > body_new->rows - 1 ||
						y - 1 + b < 0 || y - 1 + b > body_new->cols - 1)
						continue;

					neighbors += body_new->cells[(x - 1 + a) * body_new->cols + (y - 1 + b)]->alive;
				}
			}

			i = x * body_new->cols + y;

			body_old->cells[i]->alive = 0;
			if ((!body_new->cells[i]->alive && neighbors == 3) ||
				(neighbors == 3 || neighbors == 4))
				body_old->cells[i]->alive = 1;

			*pop += body_old->cells[i]->alive;
		}
	}

}
