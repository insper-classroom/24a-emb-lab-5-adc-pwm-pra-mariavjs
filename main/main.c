/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

int dead_zone(int input) {
    // Rescale from 0-4095 to -2047-2047
    int rescaled = (input - 2047);

    // Rescale from -2047-2047 to -255-255
    int rescaled2 = (rescaled * 255) / 2047;

    // Apply dead zone
    if (rescaled2 > -30 && rescaled2 < 30) {
        return 0;
    } else {
        return rescaled2;
    }
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}



void x_task(void *p) {

    adc_init();

    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(28);
    // Select ADC input 1 (GPIO27)
    adc_select_input(2);
    
    while (1) {
        
        int x_read = adc_read();
        struct adc x = {0,x_read};
        
        xQueueSend(xQueueAdc, &x, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_init();

    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(27);
    // Select ADC input 1 (GPIO27)
    adc_select_input(1);
    
    
    while (1) {
        int y_read = adc_read();
        struct adc y = {1,y_read};

        xQueueSend(xQueueAdc, &y, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        if(xQueueReceive(xQueueAdc, &data, portMAX_DELAY)){
            data.val = dead_zone(data.val);
            write_package(data);
        }
    }
}


int main() {
    stdio_init_all();  
    // adc_init(); 
    // adc_gpio_init(28); // X
    // adc_gpio_init(27); // y
    // precisa fazer gpio_set_dir ??

    


    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(x_task, "x_task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 256, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
    
}
