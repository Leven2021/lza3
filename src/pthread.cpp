#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <pthread.h>

#ifdef GUI
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "./headers/physics.h"
#include "./headers/logger.h"

int n_thd; // number of threads

int n_body;
int n_iteration;

int index_velocity;  // current index of body when updating velocity
int index_position;  // current index of body when updating postion

pthread_mutex_t lock_velocity;
pthread_mutex_t lock_position;
pthread_barrier_t barrier;

std::chrono::duration<double> time_total;


void generate_data(double *m, double *x,double *y,double *vx,double *vy, int n) {
    // TODO: Generate proper initial position and mass for better visualization
    srand((unsigned)time(NULL));
    for (int i = 0; i < n; i++) {
        m[i] = rand() % max_mass + 1.0f;
        x[i] = 2000.0f + rand() % (bound_x / 4);
        y[i] = 2000.0f + rand() % (bound_y / 4);
        vx[i] = 0.0f;
        vy[i] = 0.0f;
    }
}



void update_position(double *x, double *y, double *vx, double *vy, int i) {
    //TODO: update position
    double x_new = x[i] + vx[i] * dt;
    if (x_new >= bound_x || x_new <= 0) {
        vx[i] = - vx[i];
        x_new = x[i];
    }
    double y_new = y[i] + vy[i] * dt;
    if (y_new >= bound_y || y_new <= 0) {
        vy[i] = - vy[i];
        y_new = y[i];
    }
    x[i] = x_new;
    y[i] = y_new;
}

void update_velocity(double *m, double *x, double *y, double *vx, double *vy, int n, int i) {
    //TODO: calculate force and acceleration, update velocity
    double fx = 0;
    double fy = 0;
    double distance2;

    for (int j = 0; j < n; j++) {
        if (j == i) continue;
        distance2 = pow(x[j] - x[i], 2) + pow(y[j] - y[i], 2);
        if (distance2 < 4 * radius2) {
            vx[i] = - vx[i];
            vy[i] = - vy[i];
        }
        fx += gravity_const * m[i] * m[j] / (distance2 + err) * (x[j] - x[i]) / sqrt(distance2 + err);
        fy += gravity_const * m[i] * m[j] / (distance2 + err) * (y[j] - y[i]) / sqrt(distance2 + err);
    }
    vx[i] += fx * dt / m[i];
    vy[i] += fy * dt / m[i];
}


typedef struct {
    //TODO: specify your arguments for threads
    double* m;
    double* x;
    double* y;
    double* vx;
    double* vy;
    //TODO END
} Args;


void* worker(void* args) {
    //TODO: procedure in each threads

    Args* my_arg = (Args*) args;

    int i;  // current index of body
    // update velocity
    while (index_velocity < n_body) {
        pthread_mutex_lock(&lock_velocity);
        i = index_velocity++;
        pthread_mutex_unlock(&lock_velocity);
        if (i >= n_body) break;
        // do local task
        update_velocity(my_arg->m, my_arg->x, my_arg->y, my_arg->vx, my_arg->vy, n_body, i);
    }
    pthread_barrier_wait(&barrier);

    // update position
    while (index_position < n_body) {
        pthread_mutex_lock(&lock_position);
        i = index_position++;
        pthread_mutex_unlock(&lock_position);
        if (i >= n_body) break;
        // do local task
        update_position(my_arg->x, my_arg->y, my_arg->vx, my_arg->vy, i);
    }
    // TODO END
}


void master() {
    double* m = new double[n_body];
    double* x = new double[n_body];
    double* y = new double[n_body];
    double* vx = new double[n_body];
    double* vy = new double[n_body];

    generate_data(m, x, y, vx, vy, n_body);

    Logger l = Logger("pthread", n_body, bound_x, bound_y);

    pthread_mutex_init(&lock_velocity, NULL);
    pthread_mutex_init(&lock_position, NULL);
    pthread_barrier_init(&barrier, NULL, n_thd);

    for (int i = 0; i < n_iteration; i++) {
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        //TODO: assign jobs
        index_velocity = index_position = 0;
        
        pthread_t thds[n_thd];
        Args args[n_thd];
        for (int j = 0; j < n_thd; j++) {
            args[j].m = m;
            args[j].x = x;
            args[j].y = y;
            args[j].vx = vx;
            args[j].vy = vy;
        }
        for (int j = 0; j < n_thd; j++) pthread_create(&thds[j], NULL, worker, &args[j]);
        for (int j = 0; j < n_thd; j++) pthread_join(thds[j], NULL);
        //TODO End

        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = t2 - t1;

        printf("Iteration %d, elapsed time: %.3f\n", i, time_span);

        time_total += time_span;

        l.save_frame(x, y);

        #ifdef GUI
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1.0f, 0.0f, 0.0f);
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        double xi;
        double yi;
        for (int i = 0; i < n_body; i++){
            xi = x[i];
            yi = y[i];
            glVertex2f(xi, yi);
        }
        glEnd();
        glFlush();
        glutSwapBuffers();
        #else

        #endif
    }

    delete[] m;
    delete[] x;
    delete[] y;
    delete[] vx;
    delete[] vy;


}


int main(int argc, char *argv[]) {
    n_body = atoi(argv[1]);
    n_iteration = atoi(argv[2]);
    n_thd = atoi(argv[3]);

    #ifdef GUI
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Pthread");
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(0, bound_x, 0, bound_y);
    #endif
    master();

    printf("Student ID: 119010211\n"); // replace it with your student id
	printf("Name: Ziang Liu\n"); // replace it with your name
	printf("Assignment 3: N Body Simulation Pthread Implementation\n");
    printf("Computation Time: %.4f seconds\n", time_total);

	return 0;
}

