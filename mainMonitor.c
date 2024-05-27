/******************************************************************
 * Autor: Andrés Manrique
 * Fecha: 30/05/2024
 * Asignatura: Sistemas operativos
 * Objetivo: Entrega final del proyecto. 
 * Objetivos específicos:
 *     - Captura de argumentos y reconocimiento de los mismos
 *     - Se capturará los argumentos de la siguiente forma 
 *       $ ./monitor –b tam_buffer –t file-temp –h file-ph -p pipe-nominal
 * Rol del programa: Monitor
 *
 * *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define BUFFER_SIZE 10 // Tamaño máximo de los buffers

// Creamos esta estructura para manejar los buffers y los semáforos
typedef struct {       
    int buffer[BUFFER_SIZE];
    int count;
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
} Buffer;     // La llamaremos Buffer


Buffer buffer_ph, buffer_temp; // Buffers para PH y temperatura
int ejecutar = 1;
// Función para recolectar datos desde el pipe
void *recolector(void *arg) {
    char *pipe_nominal = (char *)arg;  // Nombre del pipe nominal
    int fd = open(pipe_nominal, O_RDONLY); // Lo abrimos con el permiso de solo leer
    if (fd == -1) {
        perror("Error: Pipe no abierto");
        exit(EXIT_FAILURE);
    }

    while (ejecutar) {
        int tipo_sensor, value;
        int n = read(fd, &tipo_sensor, sizeof(tipo_sensor)); // Leemos cada letra
        if (n == 0) { // Hasta un salto de línea
            break;
        }
        read(fd, &value, sizeof(value));
        
        if (value < 0) {
            printf("Error: PH de valor negativo recibido: %d\n", value);
            continue;
        }

        Buffer *buffer = (tipo_sensor == 1) ? &buffer_temp : &buffer_ph; // Operador ternario para asignar el tipo de buffer 
                                                                         //(Es 1? entonces será el de temperatura, sino, el de ph)

        sem_wait(&buffer->empty); // Esperamos a empty
        pthread_mutex_lock(&buffer->mutex); // Protegemos que sólo ingrese uno a la vez
        buffer->buffer[buffer->count++] = value; // Metemos valor en el buffer
        pthread_mutex_unlock(&buffer->mutex); // Desbloqueamos el mutex
        sem_post(&buffer->full); // Se libera el semaforo full que se abrirá en la siguiente función
    }

    close(fd); // Cerramos el pipe nominal
}

// Función para procesar las medidas desde los buffers y escribir en los archivos correspondientes
void *procesar_medidas(void *arg) {
    Buffer *buffer = (Buffer *)arg;
    char *file_name = buffer == &buffer_temp ? "file-temp.txt" : "file-ph.txt"; // Ternario para saber sobre qué archivo se trabajará 
    FILE *file = fopen(file_name, "a"); // Creamos el archivo con "a" y el respectivo nombre

    while (ejecutar) {
        sem_wait(&buffer->full); // Esperamos a full
        pthread_mutex_lock(&buffer->mutex); // Aseguramos 
        if (buffer->count == 0) {
            pthread_mutex_unlock(&buffer->mutex); // Desbloqueamos
            sem_post(&buffer->full);
            continue;
        }
        int value = buffer->buffer[--buffer->count]; // Asignamos el valor el valor penultimo del buffer (--buffer->count)
        pthread_mutex_unlock(&buffer->mutex); 
        sem_post(&buffer->empty);

        time_t now = time(NULL); // Ahora al valor le asignaremos las horas de cuándo se obtuvo
        struct tm *tm_info = localtime(&now); // Creamos la variable tm_info con el tiempo del computador en el momento
        char time_str[26]; // guadaremos el tiempo aquí
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info); // Asignamos el tiempo 

        fprintf(file, "%s %d\n", time_str, value); // Agregamos al archivo la fecha y la hora exacta seguido del valor
        fflush(file);

        if ((buffer == &buffer_temp && (value < 20 || value > 31.6))){ //Imcumplomiento de los parametros
            printf("¡Alerta!: Temperatura fuera de rango: %d\n", value);
        }
        else if ((buffer == &buffer_ph && (value < 6.0 || value > 8.0))){
            printf("¡Alerta!: Ph fuera de rango: %d\n", value);
        }
    }

    fclose(file); // Cerramos el archivo y lod dejamos creado en el compu
}


// Main principal
int main(int argc, char *argv[]) {
    int buffer_size = BUFFER_SIZE;
    char *file_temp = NULL, *file_ph = NULL, *pipe_nominal = NULL;
    int opt; 

    // Captura de argumentos de la línea de comandos
    while ((opt = getopt(argc, argv, "b:t:h:p:")) != -1) { // Comprobamos sin usar el orden, para eso usamos switch
        switch (opt) {
            case 'b': buffer_size = atoi(optarg); break; // Tamaño del buffer
            case 't': file_temp = optarg; break; // Nombre del archivo de temperatura
            case 'h': file_ph = optarg; break; // Nombre del archivo de PH
            case 'p': pipe_nominal = optarg; break; // Nombre del pipe
            default: exit(EXIT_FAILURE);
        }
    }

    if (file_temp == NULL || file_ph == NULL || pipe_nominal == NULL) { // Si están nulos, es que no se pasaron los argumentos
        fprintf(stderr, "Usage: %s -b buffer_size -t file_temp -h file_ph -p pipe_nominal\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Crear el pipe
    if (mkfifo(pipe_nominal, 0666) == -1) { // Creamos en esta linea el pipe, para comprimir código
        perror("Error creating named pipe");
        exit(EXIT_FAILURE);
    }
  
    // Inicialización de semáforos y mutex
    buffer_ph.count = buffer_temp.count = 0;
    sem_init(&buffer_ph.empty, 0, buffer_size);
    sem_init(&buffer_ph.full, 0, 0);
    pthread_mutex_init(&buffer_ph.mutex, NULL);
    sem_init(&buffer_temp.empty, 0, buffer_size);
    sem_init(&buffer_temp.full, 0, 0);
    pthread_mutex_init(&buffer_temp.mutex, NULL);

    pthread_t thread_recolector, thread_ph, thread_temp;
    pthread_create(&thread_recolector, NULL, recolector, pipe_nominal);
    pthread_create(&thread_ph, NULL, procesar_medidas, &buffer_ph);
    pthread_create(&thread_temp, NULL, procesar_medidas, &buffer_temp);

    pthread_join(thread_recolector, NULL);
    ejecutar = 0;
    sem_post(&buffer_ph.full);
    sem_post(&buffer_temp.full);
    pthread_join(thread_ph, NULL);
    pthread_join(thread_temp, NULL);

    // Destrucción de semáforos y mutex
    sem_destroy(&buffer_ph.empty);
    sem_destroy(&buffer_ph.full);
    pthread_mutex_destroy(&buffer_ph.mutex);
    sem_destroy(&buffer_temp.empty);
    sem_destroy(&buffer_temp.full);
    pthread_mutex_destroy(&buffer_temp.mutex);

    // Borramos el pipe nominal, para que no quede creado
    unlink(pipe_nominal);

    return 0;
}
