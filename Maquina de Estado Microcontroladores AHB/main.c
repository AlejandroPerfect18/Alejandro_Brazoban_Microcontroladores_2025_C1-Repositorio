#include <stdio.h>
#include <string.h>

// Definici�n de estados
typedef enum {
    CERRADA,
    ABIERTA,
    BLOQUEADA
} EstadoPuerta;

// Funci�n para obtener el nombre del estado
const char* getEstadoNombre(EstadoPuerta estado) {
    switch (estado) {
        case CERRADA: return "Cerrada";
        case ABIERTA: return "Abierta";
        case BLOQUEADA: return "Bloqueada";
        default: return "Desconocido";
    }
}

 in main() {
    // Variables de estado
    EstadoPuerta estadoAnterior = CERRADA; // Estado inicial anterior
    EstadoPuerta estadoActual = CERRADA;  // Estado inicial actual
    EstadoPuerta estadoSiguiente = CERRADA; // Estado siguiente

    int opcion;

    while (1) {
        // Mostrar estado actual
        printf("\nEstado actual: %s\n", getEstadoNombre(estadoActual));
        printf("Acciones disponibles:\n");

        // Mostrar opciones seg�n el estado actual
        if (estadoActual == CERRADA) {
            printf("1. Abrir\n");
            printf("2. Bloquear\n");
        } else if (estadoActual == ABIERTA) {
            printf("1. Cerrar\n");
        } else if (estadoActual == BLOQUEADA) {
            printf("1. Desbloquear\n");
        }

        // Leer acci�n del usuario
        printf("Seleccione una acci�n (1/2): ");
        scanf("%d", &opcion);

        // Guardar estado anterior
        estadoAnterior = estadoActual;

        // Actualizar estado siguiente seg�n la acci�n
        if (estadoActual == CERRADA) {
            if (opcion == 1) {
                estadoSiguiente = ABIERTA;
            } else if (opcion == 2) {
                estadoSiguiente = BLOQUEADA;
            }
        } else if (estadoActual == ABIERTA) {
            if (opcion == 1) {
                estadoSiguiente = CERRADA;
            }
        } else if (estadoActual == BLOQUEADA) {
            if (opcion == 1) {
                estadoSiguiente = CERRADA;
            }
        }

        // Actualizar estado actual
        estadoActual = estadoSiguiente;

        // Mostrar transici�n de estados
        printf("Transici�n: %s --> %s\n", getEstadoNombre(estadoAnterior), getEstadoNombre(estadoActual));
    }

    return 0;
}
