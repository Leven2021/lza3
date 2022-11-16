#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <mpi.h>
#include <omp.h>

#ifdef GUI
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "./headers/physics.h"
#include "./headers/logger.h"


int n_body;
int n_iteration;

int my_rank;
int world_size;

MPI_Group whole_world;
MPI_Group slave_world;
MPI_Comm slaves_world;

int n_omp_threads;

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


void update_position(double *x, double *y, double *vx, double *vy, int i, int start_body, double* my_x, double* my_y, double* my_vx, double* my_vy) {
    //TODO: update position 
    double x_new = x[i] + vx[i] * dt;
    if (x_new >= bound_x || x_new <= 0) {
        vx[i] = - vx[i];
        my_vx[i - start_body] = vx[i];
        x_new = x[i];
    }
    double y_new = y[i] + vy[i] * dt;
    if (y_new >= bound_y || y_new <= 0) {
        vy[i] = - vy[i];
        my_vy[i - start_body] = vy[i];
        y_new = y[i];
    }
    x[i] = x_new;
    y[i] = y_new;
    my_x[i - start_body] = x[i];
    my_y[i - start_body] = y[i];
}


void update_velocity(double *m, double *x, double *y, double *vx, double *vy, int n, int i, int start_body, double* my_vx, double* my_vy) {
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
    my_vx[i - start_body] = vx[i];
    my_vy[i - start_body] = vy[i];
}


void slave() {
    // TODO: MPI routine
    double* local_m = new double[n_body];
    double* local_x = new double[n_body];
    double* local_y = new double[n_body];
    double* local_vx = new double[n_body];
    double* local_vy = new double[n_body];

    int slave_rank;
    int slave_size;
    MPI_Comm_rank(slaves_world, &slave_rank);
    MPI_Comm_size(slaves_world, &slave_size);

    int my_body = (n_body % slave_size == 0) ? n_body / slave_size : n_body / slave_size + 1;
    int start_body = slave_rank * my_body;
    int end_body = (slave_rank + 1) * my_body;
    if (slave_rank == slave_size - 1) end_body -= slave_size * my_body - n_body;

    double* my_x = new double[my_body];
    double* my_y = new double[my_body];
    double* my_vx = new double[my_body];
    double* my_vy = new double[my_body];

    MPI_Recv(local_m, n_body, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int terminator = 0;
    while (terminator == 0) {
        MPI_Barrier(slaves_world);
        MPI_Recv(&terminator, 1, MPI_INT, 0, 77, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_x, n_body, MPI_DOUBLE, 0, 777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_y, n_body, MPI_DOUBLE, 0, 777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_vx, n_body, MPI_DOUBLE, 0, 777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_vy, n_body, MPI_DOUBLE, 0, 777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Barrier(slaves_world);

        omp_set_num_threads(n_omp_threads);
        for (int i = start_body; i < end_body; i++) {
            update_velocity(local_m, local_x, local_y, local_vx, local_vy, n_body, i, start_body, my_vx, my_vy);
        }
        MPI_Barrier(slaves_world);

        omp_set_num_threads(n_omp_threads);
        for (int i = start_body; i < end_body; i++) {
            update_position(local_x, local_y, local_vx, local_vy, i, start_body, my_x, my_y, my_vx, my_vy);
        }
        MPI_Barrier(slaves_world);

        MPI_Gather(my_x, my_body, MPI_DOUBLE, local_x, my_body, MPI_DOUBLE, 0, slaves_world);
        MPI_Gather(my_y, my_body, MPI_DOUBLE, local_y, my_body, MPI_DOUBLE, 0, slaves_world);
        MPI_Gather(my_vx, my_body, MPI_DOUBLE, local_vx, my_body, MPI_DOUBLE, 0, slaves_world);
        MPI_Gather(my_vy, my_body, MPI_DOUBLE, local_vy, my_body, MPI_DOUBLE, 0, slaves_world);

        if (slave_rank == 0) {
            MPI_Send(local_x, n_body, MPI_DOUBLE, 0, 7777, MPI_COMM_WORLD);
            MPI_Send(local_y, n_body, MPI_DOUBLE, 0, 7777, MPI_COMM_WORLD);
            MPI_Send(local_vx, n_body, MPI_DOUBLE, 0, 7777, MPI_COMM_WORLD);
            MPI_Send(local_vy, n_body, MPI_DOUBLE, 0, 7777, MPI_COMM_WORLD);
        }
    }
    // TODO End
}


void master() {
    double* total_m = new double[n_body];
    double* total_x = new double[n_body];
    double* total_y = new double[n_body];
    double* total_vx = new double[n_body];
    double* total_vy = new double[n_body];

    generate_data(total_m, total_x, total_y, total_vx, total_vy, n_body);

    Logger l = Logger("MPI_openmp", n_body, bound_x, bound_y);

    for (int i = 1; i < world_size; i++) {
        MPI_Send(total_m, n_body, MPI_DOUBLE, i, 7, MPI_COMM_WORLD);
    }
    int terminator = 0;
    for (int i = 0; i < n_iteration; i++) {
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

        // TODO: MPI routine
        if (i == n_iteration - 1) terminator = 1;
        for (int i = 1; i < world_size; i++) {
            MPI_Send(&terminator, 1, MPI_INT, i, 77, MPI_COMM_WORLD);
            MPI_Send(total_x, n_body, MPI_DOUBLE, i, 777, MPI_COMM_WORLD);
            MPI_Send(total_y, n_body, MPI_DOUBLE, i, 777, MPI_COMM_WORLD);
            MPI_Send(total_vx, n_body, MPI_DOUBLE, i, 777, MPI_COMM_WORLD);
            MPI_Send(total_vy, n_body, MPI_DOUBLE, i, 777, MPI_COMM_WORLD);
        }

        MPI_Recv(total_x, n_body, MPI_DOUBLE, MPI_ANY_SOURCE, 7777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(total_y, n_body, MPI_DOUBLE, MPI_ANY_SOURCE, 7777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(total_vx, n_body, MPI_DOUBLE, MPI_ANY_SOURCE, 7777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(total_vy, n_body, MPI_DOUBLE, MPI_ANY_SOURCE, 7777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // TODO End

        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = t2 - t1;

        printf("Iteration %d, elapsed time: %.3f\n", i, time_span);

        time_total += time_span;

        l.save_frame(total_x, total_y);

        #ifdef GUI
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1.0f, 0.0f, 0.0f);
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        double xi;
        double yi;
        for (int i = 0; i < n_body; i++){
            xi = total_x[i];
            yi = total_y[i];
            glVertex2f(xi, yi);
        }
        glEnd();
        glFlush();
        glutSwapBuffers();
        #else

        #endif
    }

    delete[] total_m;
    delete[] total_x;
    delete[] total_y;
    delete[] total_vx;
    delete[] total_vy;

}


int main(int argc, char *argv[]) {
    n_body = atoi(argv[1]);
    n_iteration = atoi(argv[2]);
    n_omp_threads = atoi(argv[3]);

	MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int* slaves = new int[world_size - 1];
    for (int i = 0; i < world_size - 1; i++) {
        slaves[i] = i + 1;
    }
    MPI_Comm_group(MPI_COMM_WORLD, &whole_world);
    MPI_Group_incl(whole_world, world_size - 1, slaves, &slave_world);
    MPI_Comm_create(MPI_COMM_WORLD, slave_world, &slaves_world);

	if (my_rank == 0) {
		#ifdef GUI
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
		glutInitWindowSize(500, 500); 
		glutInitWindowPosition(0, 0);
		glutCreateWindow("N Body Simulation MPI Implementation");
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glMatrixMode(GL_PROJECTION);
		gluOrtho2D(0, bound_x, 0, bound_y);
		#endif
        master();
	} else {
        slave();
    }

	if (my_rank == 0) {
		printf("Student ID: 119010211\n"); // replace it with your student id
		printf("Name: Ziang Liu\n"); // replace it with your name
		printf("Assignment 3: N Body Simulation MPI + OpenMP Implementation\n");
        printf("Computation Time: %.4f seconds\n", time_total);
	}

	MPI_Finalize();

	return 0;
}

