#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#ifdef GUI
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "./headers/physics.h"

int size;
int iteration;

int n_omp_threads;

void initialize(float *data) {
    // intialize the temperature distribution
    int len = size * size;
    for (int i = 0; i < len; i++) {
        data[i] = wall_temp;
    }
}

void generate_fire_area(bool *fire_area){
    // generate the fire area
    int len = size * size;
    for (int i = 0; i < len; i++) {
        fire_area[i] = 0;
    }

    float fire1_r2 = fire_size * fire_size;
    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            int a = i - size / 2;
            int b = j - size / 2;
            int r2 = 0.5 * a * a + 0.8 * b * b - 0.5 * a * b;
            if (r2 < fire1_r2) fire_area[i * size + j] = 1;
        }
    }

    float fire2_r2 = (fire_size / 2) * (fire_size / 2);
    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            int a = i - 1 * size / 3;
            int b = j - 1 * size / 3;
            int r2 = a * a + b * b;
            if (r2 < fire2_r2) fire_area[i * size + j] = 1;
        }
    }
}

void update(float *data, float *new_data, int idx) {
    // TODO: update temperature for each point  (in parallelized way)
    if (idx < size * size) {
        int i = idx / size;
        if (i == 0 || i == size - 1) return;
        int j = idx % size;
        if (j == 0 || j == size - 1) return;

        float up = data[idx - size];
        float down = data[idx + size];
        float left = data[idx - 1];
        float right = data[idx + 1];
        float new_val = (up + down + left + right) / 4;
        new_data[idx] = new_val;
    }
}

void maintain_fire(float *data, bool *fire_area, int idx) {
    // TODO: maintain the temperature of the fire (in parallelized way)
    if (idx < size * size) {
        if (fire_area[idx]) data[idx] = fire_temp;
    }
}

void maintain_wall(float *data) {
    // TODO: maintain the temperature of the wall
    for (int i = 0; i < size; i++) {
        data[i] = wall_temp;
    }
    for (int i = size * (size - 1); i < size * size; i++) {
        data[i] = wall_temp;
    }
    for (int i = 0; i < size; i++) {
        data[size * i] = wall_temp;
        data[size * i + size - 1] = wall_temp;
    }
}

#ifdef GUI
void data2pixels(float *data, GLubyte* pixels, int idx){
    // TODO: convert rawdata (large, size^2) to pixels (small, resolution^2) for faster rendering speed (in parallelized way)
    if (idx < resolution * resolution) {
        float factor_data_pixel = (float) size / resolution;
        float factor_temp_color = (float) 255 / fire_temp;

        int x = idx / resolution;
        int y = idx % resolution;

        int idx_pixel = idx * 3;
        int x_raw = x * factor_data_pixel;
        int y_raw = y * factor_data_pixel;
        int idx_raw = x_raw * size + y_raw;
        float temp = data[idx_raw];
        int color =  ((int) temp / 5 * 5) * factor_temp_color;
        pixels[idx_pixel] = color;
        pixels[idx_pixel + 1] = 255 - color;
        pixels[idx_pixel + 2] = 255 - color;
    }
}

void plot(GLubyte* pixels){
    // visualize temprature distribution
    #ifdef GUI
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(resolution, resolution, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glutSwapBuffers();
    #endif
}
#endif

void master(){
    float *data_odd;
    float *data_even;
    bool *fire_area;

    #ifdef GUI
    GLubyte* pixels = new GLubyte[resolution * resolution * 3];
    #endif
    
    data_odd = new float[size * size];
    data_even = new float[size * size];
    fire_area = new bool[size * size];

    generate_fire_area(fire_area);
    initialize(data_odd);

    int count = 1;
    double total_time = 0;

    while (count <= iteration) {
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        float* new_data = (count % 2 == 1) ? data_even : data_odd;
        float* old_data = (count % 2 == 1) ? data_odd : data_even;

        int len = size * size;

        omp_set_num_threads(n_omp_threads);
        #pragma omp parallel for
        for (int i = 0; i < len; i++) {
            update(old_data, new_data, i);
        }

        omp_set_num_threads(n_omp_threads);
        #pragma omp parallel for
        for (int i = 0; i < len; i++) {
            maintain_fire(new_data, fire_area, i);
        }

        maintain_wall(new_data);

        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        double this_time = std::chrono::duration<double>(t2 - t1).count();
        total_time += this_time;
        printf("Iteration %d, elapsed time: %.6f\n", count, this_time);
        count++;

        #ifdef GUI
        for (int i = 0; i < len; i++) {
            data2pixels(new_data, pixels, i);
        }
        plot(pixels);
        #endif
    }

    printf("Stop after %d iterations, elapsed time: %.6f, average computation time: %.6f\n", count-1, total_time, (double) total_time / (count-1));

    delete[] data_odd;
    delete[] data_even;
    delete[] fire_area;

    #ifdef GUI
    delete[] pixels;
    #endif
}

int main(int argc, char *argv[]) {
    size = atoi(argv[1]);
    iteration = atoi(argv[2]);
    n_omp_threads = atoi(argv[3]);
	
    #ifdef GUI
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(resolution, resolution);
    glutCreateWindow("Heat Distribution Simulation OpenMP Implementation");
    gluOrtho2D(0, resolution, 0, resolution);
    #endif

    master();

    printf("Student ID: 119010211\n"); // replace it with your student id
    printf("Name: Ziang Liu\n"); // replace it with your name
    printf("Assignment 4: Heat Distribution OpenMP Implementation\n");

    return 0;
}
