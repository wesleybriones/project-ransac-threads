#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define FILE_NAME "boston.csv"
#define EPSILON 0.1

typedef struct {
    double rooms;
    double price;
} Point;

typedef struct {
    double slope;
    double intercept;
    int inliers;
} Model;

typedef struct {
    const Point *points;
    int numPoints;
    double epsilon;
    int iterations;
    Model *models;
} ThreadData;

Point *readPointsFromFile(const char *filename, int *numPoints);
Model generateModel(const Point *points, int numPoints, double epsilon);
int countInliers(const Point *points, int numPoints, const Model *model, double epsilon);
Model *ransac(const Point *points, int numPoints, int maxIterations, double epsilon);
Model selectBestModel(const Model *models, int numModels);
void printBestModel(const Model *model, int numInliers, int numPoints);
void *generateModelThread(void *arg);

int main(int argc, char *argv[]) {
    printf("#######INICIO#######\n\n");
    srand(time(NULL));

    int numberOfIterations = 0;
    int option;

    // Parsear los argumentos de línea de comandos
    while ((option = getopt(argc, argv, "N:")) != -1) {
        switch (option) {
        case 'N':
            numberOfIterations = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Uso: %s -N numberOfIterations\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Verificar si se proporcionó el argumento -N
    if (numberOfIterations == 0) {
        fprintf(stderr, "Se requiere especificar el número de iteraciones con el flag -N\n");
        exit(EXIT_FAILURE);
    }

    // Medir el tiempo de ejecución
    clock_t startTime = clock();

    // Variables para almacenar los puntos
    Point *points;
    int numPoints;

    // Leer los puntos desde el archivo
    points = readPointsFromFile(FILE_NAME, &numPoints);
    if (points == NULL) {
        printf("Error al leer los puntos desde el archivo \"%s\".\n", FILE_NAME);
        return 1;
    }

    // Imprimir la cantidad de puntos leídos
    printf("Cantidad de puntos leídos desde el archivo \"%s\": %d\n\n", FILE_NAME, numPoints);

    // Ejecutar RANSAC
    Model *bestModel = ransac(points, numPoints, numberOfIterations, EPSILON);
    if (bestModel == NULL) {
        printf("Error al ejecutar RANSAC.\n");
        free(points);
        return 1;
    }

    // Calcular el tiempo transcurrido
    clock_t endTime = clock();
    double executionTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;


    printBestModel(bestModel, bestModel->inliers, numPoints);
    printf("\n");

    // Imprimir el tiempo de ejecución
    printf("Tiempo de ejecución: %f segundos\n", executionTime);

    // Liberar la memoria
    free(points);
    free(bestModel);

    printf(" #######FIN######\n\n");

    return 0;
}

Point *readPointsFromFile(const char *filename, int *numPoints) {
    // Abrir el archivo
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
        return NULL;
    }

    // Omitir la línea de cabecera
    char buffer[256];
    fgets(buffer, sizeof(buffer), file);

    // Contar el número de líneas en el archivo
    int linesCount = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        linesCount++;
    }

    // Regresar al inicio del archivo
    rewind(file);

    // Omitir la línea de cabecera de nuevo
    fgets(buffer, sizeof(buffer), file);

    // Leer los puntos desde el archivo
    Point *points = (Point *)malloc(linesCount * sizeof(Point));
    if (points == NULL) {
        printf("Error al asignar memoria.\n");
        fclose(file);
        return NULL;
    }

    int numRead = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        sscanf(buffer, "%lf,%lf", &(points[numRead].rooms), &(points[numRead].price));
        numRead++;
    }

    // Cerrar el archivo
    fclose(file);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    *numPoints = numRead;
    return points;
}

Model generateModel(const Point *points, int numPoints, double epsilon) {
    int index1 = rand() % numPoints;
    int index2 = rand() % numPoints;
    Point p1 = points[index1];
    Point p2 = points[index2];

    Model model;
    model.slope = (p2.price - p1.price) / (p2.rooms - p1.rooms);
    model.intercept = p1.price - model.slope * p1.rooms;
    model.inliers = countInliers(points, numPoints, &model, epsilon);

    return model;
}

int countInliers(const Point *points, int numPoints, const Model *model, double epsilon) {
    int inliers = 0;
    for (int i = 0; i < numPoints; i++) {
        double distance = fabs(model->slope * points[i].rooms - points[i].price + model->intercept) / sqrt(1 + pow(model->slope, 2));
        if (distance < epsilon) {
            inliers++;
        }
    }
    return inliers;
}

Model *ransac(const Point *points, int numPoints, int maxIterations, double epsilon) {
    Model *models = (Model *)malloc(maxIterations * sizeof(Model));
    
    if (models == NULL) {
        printf("Error al asignar memoria.\n");
        return NULL;
    }

    //Calcular los nucleos disponibles del computador
    long numCores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Número de núcleos: %ld\n", numCores);

    pthread_t threads[numCores];
    ThreadData threadData[numCores];
    int iterationsPerThread = maxIterations / numCores;
    int extraIterations = maxIterations % numCores;
    int currentIndex = 0;

    // Crear hilos y asignar trabajo
    for (int i = 0; i < numCores; i++) {
        threadData[i].points = points;
        threadData[i].numPoints = numPoints;
        threadData[i].epsilon = epsilon;
        threadData[i].models = &models[currentIndex];

        int iterations = iterationsPerThread;
        if (extraIterations > 0) {
            iterations++;
            extraIterations--;
        }

        threadData[i].iterations = iterations;

        currentIndex += iterations;

        pthread_create(&threads[i], NULL, generateModelThread, &threadData[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numCores; i++) {
        pthread_join(threads[i], NULL);
    }

    // Seleccionar el mejor modelo
    Model *bestModel = (Model *)malloc(sizeof(Model));

    *bestModel = selectBestModel(models, maxIterations);

    free(models);

    return bestModel;
}

void *generateModelThread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    Model *models = data->models;
    int iterations = data->iterations;

    for (int i = 0; i < iterations; i++) {
        models[i] = generateModel(data->points, data->numPoints, data->epsilon);
    }

    pthread_exit(NULL);
}

Model selectBestModel(const Model *models, int numModels) {
    Model bestModel = models[0];

    for (int i = 0; i < numModels; i++) {
        if (models[i].inliers > bestModel.inliers) {
            bestModel = models[i];
        }
    }

    return bestModel;
}

void printBestModel(const Model *model, int numInliers, int numPoints) {
    printf("Mejor modelo: y = %.2lfx + %.2lf\n", model->slope, model->intercept);
    printf("Número de inliers: %d\n", numInliers);
    printf("Número de outliers: %d\n", numPoints - numInliers);
}
