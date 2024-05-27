# * Autor: Andrés Manrique
# * Fecha: 30/05/2024
# * Asignatura: Sistemas operativos
# * Objetivo: Entrega final del proyecto
# * Rol del programa: Makefile
# * Se ejecuta de la siguiente forma:
# 		make all:  para compilar todo
# 		make clean: para borrar los archivos que habían sido creados

# Nombre del ejecutable para el monitor
MONITOR_EXEC = monitor
# Nombre del ejecutable para el sensor
SENSOR_EXEC = sensor

# Compilador a utilizar
CC = gcc
# Opciones de compilación
CFLAGS = -Wall -Wextra -pthread

# Compilamos tanto monitor como sensor
all: $(MONITOR_EXEC) $(SENSOR_EXEC)

# Compilación del monitor
$(MONITOR_EXEC): mainMonitor.c
	$(CC) $(CFLAGS) $^ -o $@

# Compilación del sensor
$(SENSOR_EXEC): mainSensor.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(MONITOR_EXEC) $(SENSOR_EXEC)
