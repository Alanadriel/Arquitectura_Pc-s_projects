#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "led_strip.h"


//Tag para debugger serie
static const char *TAG = "Led";

//Define del pin que tiene el led RGB
#define PIN_RGB 8
//Define del pin del boton
#define PIN_BOTON 0

//Definimos los colores
#define ROJO 0
#define VERDE 1
#define AZUL 2
uint8_t color=ROJO;
uint8_t brillo=0;
uint8_t sentido=1;

//Definimos una variable para manejar el led RGB
static led_strip_handle_t ledRGB;

//IRQ
static void IRAM_ATTR rutinaISR_IRQ(void* args){
	color = (color +1) % 3 ;
}


void app_main(void)
{
	//interrupcion
	gpio_set_direction(PIN_BOTON,GPIO_MODE_INPUT);
	gpio_pulldown_dis(PIN_BOTON);
	gpio_pullup_dis(PIN_BOTON);
	gpio_set_intr_type(PIN_BOTON,GPIO_INTR_POSEDGE);
	gpio_install_isr_service(0);
	//defino rutina
	gpio_isr_handler_add(PIN_BOTON,rutinaISR_IRQ,NULL);
	//Inicializamos la variable global con los datos
	//del led RGB
	led_strip_config_t strip_config = {
        .strip_gpio_num = PIN_RGB,
        .max_leds = 1, //Solo tenemos un led RGB
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    //Vamos a llamar a una funcion de ESP-IDF que puede
    //fallar, asi que usamos una macro que verifica que
    //todo salga bien.. caso contrario imprime el error
    //por el debugger serie y llama a abort()
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &ledRGB));
    //Si llegamos hasta aca, el led  RGB esta configurado correctamente
    //Lo apagamos
    led_strip_clear(ledRGB);
    
    led_strip_set_pixel(ledRGB, 0, 0, 0, 0);
    led_strip_refresh(ledRGB);
    ESP_LOGI(TAG,"Comienza...");
    while(1){
		vTaskDelay( 10 / portTICK_PERIOD_MS);
		if (sentido){
			brillo++;
			if (brillo==255) sentido=0;
			} else {
			brillo--;
			if (brillo==0){
				 sentido=1;
			}
		}
		switch(color){
			case ROJO:
				led_strip_set_pixel(ledRGB, 0, brillo, 0, 0);
				break;
			case VERDE:
				led_strip_set_pixel(ledRGB, 0, 0, brillo, 0);
				break;
			default:
				led_strip_set_pixel(ledRGB, 0, 0, 0, brillo);
		}
		led_strip_refresh(ledRGB);
		
	}


}