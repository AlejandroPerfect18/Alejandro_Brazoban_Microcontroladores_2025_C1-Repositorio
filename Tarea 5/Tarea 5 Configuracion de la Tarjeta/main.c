#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include <stdint.h>
#include <stdbool.h>

#define BUTTON_PIN  0    // Pin donde está conectado el botón
#define LED_PIN     1    // Pin donde está conectado el LED
#define LED_ON()    GPIO_Set(LED_PIN)   // Macro para encender el LED
#define LED_OFF()   GPIO_Clear(LED_PIN) // Macro para apagar el LED

// Variables globales
TimerHandle_t timerCaptura;  // Timer para medir el tiempo de pulsación del botón
TimerHandle_t timerParpadeo; // Timer para controlar el parpadeo del LED
volatile uint32_t tiempoPresionado = 0; // Tiempo de pulsación del botón
SemaphoreHandle_t semaforoLed;  // Semáforo para la notificación del parpadeo

// ISR para manejar la interrupción del botón (simulada)
void ISR_Boton(void) {
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    if (GPIO_Read(BUTTON_PIN) == 1) {  // Si el botón es presionado
        LED_ON();  // Enciende el LED
        tiempoPresionado = 0;  // Reinicia el contador de tiempo
        xTimerStartFromISR(timerCaptura, &higherPriorityTaskWoken);  // Inicia el timer de captura
    } else {  // Si el botón es liberado
        LED_OFF();  // Apaga el LED
        xTimerStopFromISR(timerCaptura, &higherPriorityTaskWoken);  // Detiene el timer de captura
        xTimerChangePeriodFromISR(timerParpadeo, tiempoPresionado / portTICK_PERIOD_MS, &higherPriorityTaskWoken);
        xSemaphoreGiveFromISR(semaforoLed, &higherPriorityTaskWoken);  // Notifica que debe parpadear
    }
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

// Callback del timer de captura
void timerCapturaCallback(TimerHandle_t xTimer) {
    tiempoPresionado += 10;  // Incrementa el tiempo en 10 ms (ajustado al período del timer)
}

// Callback del timer de parpadeo del LED
void timerParpadeoCallback(TimerHandle_t xTimer) {
    static bool ledOn = false;
    ledOn = !ledOn;  // Cambia el estado del LED
    if (ledOn) {
        LED_ON();
    } else {
        LED_OFF();
    }
}

// Tarea para manejar el parpadeo del LED
void tareaParpadeoLed(void *pvParameters) {
    for (;;) {
        xSemaphoreTake(semaforoLed, portMAX_DELAY);  // Espera a que se libere el semáforo
        xTimerStart(timerParpadeo, 0);  // Inicia el timer de parpadeo
        vTaskDelay(tiempoPresionado / portTICK_PERIOD_MS);  // Espera el tiempo que duró el botón presionado
        xTimerStop(timerParpadeo, 0);  // Detiene el timer
        LED_OFF();  // Asegura que el LED termine apagado
    }
}

int main(void) {
    // Configuración de pines y periféricos
    GPIO_Setup(BUTTON_PIN, INPUT);
    GPIO_Setup(LED_PIN, OUTPUT);

    // Creación de timers
    timerCaptura = xTimerCreate("TimerCaptura", pdMS_TO_TICKS(10), pdTRUE, NULL, timerCapturaCallback);
    timerParpadeo = xTimerCreate("TimerParpadeo", pdMS_TO_TICKS(100), pdTRUE, NULL, timerParpadeoCallback);

    // Creación del semáforo binario
    semaforoLed = xSemaphoreCreateBinary();

    // Creación de la tarea de parpadeo del LED
    xTaskCreate(tareaParpadeoLed, "TareaParpadeoLed", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // Simulación de interrupción de botón conectada a la ISR
    GPIO_SetInterrupt(BUTTON_PIN, ISR_Boton);

    // Arranque del sistema operativo
    vTaskStartScheduler();

    // El programa no debería llegar aquí
    while (1) {}
    return 0;
}
