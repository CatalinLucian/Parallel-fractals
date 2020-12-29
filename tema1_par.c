/*
 * APD - Tema 1
 * Octombrie 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>


char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;
int P; // no of threads
int N1; // no of iterations for Julia along OX
int M1; // height for Julia along OY
int N2; // no of iterations for Mandelbrot along OX
int M2; // height for Mandelbrot along OY
int** result_julia;
int** result_mandelbrot;

pthread_barrier_t barrier;

// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

params par_julia;
params par_mandelbrot;

int min(int a, int b) {
  if (a < b)
    return a;
  return b;
}

// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 5) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
  P = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}

void *thread_function(void *arg){

  int thread_id = *(int *)arg;

	// starting point for thread ID when calculating the result fractal Julia
	int start1 = thread_id * (double)M1 / P;
	// end point for thread ID when calculating the result fractal Julia
	int end1 = min((thread_id + 1) * (double)M1 / P, M1);
	// starting point for thread ID when writing the result fractal Julia to PGM
	int start_write_julia = thread_id * (double)(M1 / 2) / P;
	// end point for thread ID when writing the result fractal Julia to PGM
	int end_write_julia = min((thread_id + 1) * (double)(M1 / 2) / P, M1 / 2);

  int start2 = thread_id * (double)M2 / P;
  int end2 = min((thread_id + 1) * (double)M2 / P, M2);
	int start_write_mandelbrot = thread_id * (double)(M2 / 2) / P;
	int end_write_mandelbrot = min((thread_id + 1) * (double)(M2 / 2) / P, M2 / 2);

  int w, h;

  for (w = 0; w < N1; w++) {
    for (h = start1; h < end1; h++) {
      int step = 0;
      complex z = { .a = w * par_julia.resolution + par_julia.x_min,
              .b = h * par_julia.resolution + par_julia.y_min };

      while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par_julia.iterations) {
        complex z_aux = { .a = z.a, .b = z.b };

        z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par_julia.c_julia.a;
        z.b = 2 * z_aux.a * z_aux.b + par_julia.c_julia.b;

        step++;
      }

      result_julia[h][w] = step % 256;
    }
  }

	pthread_barrier_wait(&barrier);

	for (int i = start_write_julia; i < end_write_julia; i++) {
		int *aux = result_julia[i];
		result_julia[i] = result_julia[M1 - i - 1];
		result_julia[M1 - i - 1] = aux;
	}

	for (w = 0; w < N2; w++) {
		for (h = start2; h < end2; h++) {
			complex c = { .a = w * par_mandelbrot.resolution + par_mandelbrot.x_min,
							.b = h * par_mandelbrot.resolution + par_mandelbrot.y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par_mandelbrot.iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result_mandelbrot[h][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier);

	for (int i = start_write_mandelbrot; i < end_write_mandelbrot; i++) {
		int *aux = result_mandelbrot[i];
		result_mandelbrot[i] = result_mandelbrot[M2 - i - 1];
		result_mandelbrot[M2 - i - 1] = aux;
	}


  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int width_julia, height_julia, width_mandelbrot, height_mandelbrot;
	// se citesc argumentele programului
	get_args(argc, argv);

  int i;
  pthread_t tid[P];
  int thread_id[P];
  pthread_barrier_init(&barrier, NULL, P);

	read_input_file(in_filename_julia, &par_julia);

	width_julia = (par_julia.x_max - par_julia.x_min) / par_julia.resolution;
	height_julia = (par_julia.y_max - par_julia.y_min) / par_julia.resolution;
  N1 = width_julia;
  M1 = height_julia;

	result_julia = allocate_memory(width_julia, height_julia);
	read_input_file(in_filename_mandelbrot, &par_mandelbrot);

	width_mandelbrot = (par_mandelbrot.x_max - par_mandelbrot.x_min) / par_mandelbrot.resolution;
	height_mandelbrot = (par_mandelbrot.y_max - par_mandelbrot.y_min) / par_mandelbrot.resolution;
  N2 = width_mandelbrot;
  M2 = height_mandelbrot;

	result_mandelbrot = allocate_memory(width_mandelbrot, height_mandelbrot);

	// creation of threads and running of them
  for (i = 0; i < P; i++) {
    thread_id[i] = i;
    pthread_create(&tid[i], NULL, thread_function, &thread_id[i]);
  }

  for (i = 0; i < P; i++) {
    pthread_join(tid[i], NULL);
  }

  write_output_file(out_filename_julia, result_julia, width_julia, height_julia);
  free_memory(result_julia, height_julia);

  write_output_file(out_filename_mandelbrot, result_mandelbrot, width_mandelbrot, height_mandelbrot);
  free_memory(result_mandelbrot, height_mandelbrot);

  pthread_barrier_destroy(&barrier);
	return 0;

}
