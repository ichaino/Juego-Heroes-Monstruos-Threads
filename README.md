# Tarea 2 - Sistemas Operativos

## Juego de Héroes y Monstruos con Threads

Proyecto académico que implementa una simulación de un héroe y múltiples monstruos en un tablero, utilizando **hilos (threads)** y mecanismos de **sincronización con mutex** para gestionar la concurrencia entre los participantes.

## Integrantes

* **José Avello**
* **Álvaro Valdebenito**

**Asignatura:** Sistemas Operativos
**Sección:** 01
**Profesor:** Yerko Ortiz

## Contenido

* `Juego_SO.c`: código fuente de la simulación.
* `makefile`: instrucciones para compilar el programa.
* `ejemplo1.txt`: archivo de configuración de ejemplo 1.
* `ejemplo2.txt`: archivo de configuración de ejemplo 2.
* `ejemplo3.txt`: archivo de configuración de ejemplo 3.

## Herramientas utilizadas

* Lenguaje C
* GCC
* POSIX Threads (`pthreads`)
* Mutex para sincronización
* Linux/UNIX
* Make

## Funcionamiento general

1. Se lee el archivo de configuración, que define el tablero, la ruta del héroe y las características de los monstruos.
2. Se crean hilos independientes para el héroe y cada monstruo.
3. El héroe recorre la ruta definida y ataca a los monstruos que se encuentran dentro de su rango de ataque.
4. Los monstruos detectan al héroe según su rango de visión, se desplazan hacia él y pueden alertar a otros monstruos cercanos.
5. Los ataques se realizan cuando se cumplen las condiciones de distancia correspondientes.
6. La simulación finaliza cuando el héroe completa su ruta o muere.

Se utilizan **mutex** para sincronizar el acceso a variables compartidas y reducir los conflictos asociados a la ejecución concurrente de los hilos.

## Compilación

En una terminal Linux/UNIX, ejecutar:

```bash
make
```

Esto generará el ejecutable `Juego_SO`.

## Ejecución

Para iniciar la simulación, se debe indicar un archivo de configuración:

```bash
./Juego_SO ejemplo1.txt
```

También se pueden utilizar los otros archivos disponibles:

```bash
./Juego_SO ejemplo2.txt
./Juego_SO ejemplo3.txt
```

## Requisitos

* Sistema operativo Linux/UNIX.
* Compilador GCC.
* Soporte para POSIX Threads (`pthread`).
* Herramienta `make`.
