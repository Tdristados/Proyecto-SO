/******************************************************************
 * Autor: Andrés Manrique
 * Fecha: 20/05/2024
 * Asignatura: Sistemas operativos
 * Objetivo: Entrega final del proyecto. 
 * Objetivos especificos:
 *     - Enviar, mediante la tuberia, los mensajes correspondientes 
 *       a las medidas del respectivo sensor.
 *     - Se capturará los argumentos de la siguiente forma 
 *       ./sensor -s #sensor -t tiempo -f archivo.txt -p pipeNominal
 * Rol del programa: Sensor
 *
 * ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s -s tipo_sensor -t tiempo -f archivo -p pipe_nominal\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int tipo_sensor = 0, tiempo = 0;
    char *archivo = NULL, *pipe_nominal = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "s:t:f:p:")) != -1) {
        switch (opt) {
            case 's': tipo_sensor = atoi(optarg); break;
            case 't': tiempo = atoi(optarg); break;
            case 'f': archivo = optarg; break;
            case 'p': pipe_nominal = optarg; break;
            default: usage(argv[0]);
        }
    }

    if (tipo_sensor == 0 || tiempo == 0 || archivo == NULL || pipe_nominal == NULL) {
        usage(argv[0]);
    }

    FILE *file = fopen(archivo, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int fd = open(pipe_nominal, O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        int value = atoi(line);
        write(fd, &tipo_sensor, sizeof(tipo_sensor));
        write(fd, &value, sizeof(value));
        sleep(tiempo);
    }

    close(fd);
    fclose(file);
    return 0;
}
