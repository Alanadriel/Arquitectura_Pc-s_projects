#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "tm1637.h"
#include "led_strip.h"
//Define del pin que tiene el led RGB
#define PIN_RGB 8

//Tag para debugger serie
static const char *TAG = "Reloj";
//Definimos una variable para manejar el led RGB
static led_strip_handle_t ledRGB;

// defino pin boton
#define PIN_BOTON 0 
//Variables globales
uint8_t segundos=0;
uint8_t minutos=0;


#define ROJO 0
#define VERDE 1
#define AZUL 2
uint8_t color=ROJO;
uint8_t brillo=0;
uint8_t sentido=1;

//IRQ RELOJ
static void IRAM_ATTR rutinaRelojISR_IRQ(void* args){
		segundos = 0;
		minutos =0;
}
static void rutinaLED_ISR(void* args){
	color = (color +1) % 3 ;
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

void app_main(void)
{
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
	//interrupcion
	gpio_set_direction(PIN_BOTON,GPIO_MODE_INPUT);
	gpio_pulldown_dis(PIN_BOTON);
	gpio_pullup_dis(PIN_BOTON);
	gpio_set_intr_type(PIN_BOTON,GPIO_INTR_POSEDGE);
	gpio_install_isr_service(0);
	
	//defino rutina
	gpio_isr_handler_add(PIN_BOTON,rutinaRelojISR_IRQ,NULL);
	ESP_LOGI(TAG,"Comienza...");
	// timer---------
	const esp_timer_create_args_t variableTimerLed ={
		.callback = &rutinaLED_ISR,
		.name = "Rutina del Led"
		};
		esp_timer_handle_t timer_handler_led;
		esp_timer_create(&variableTimerLed, &timer_handler_led);
		esp_timer_start_periodic(timer_handler_led,1000);
	//Definimos el TM1637
	const gpio_num_t pin_clk = 1;
	const gpio_num_t pin_dta = 10;
	tm1637_led_t * lcd = tm1637_init(pin_clk, pin_dta);
	tm1637_set_segment_number(lcd,0,0x0,1);
	tm1637_set_segment_number(lcd,1,0x0,1);
	tm1637_set_segment_number(lcd,2,0x0,1);
	tm1637_set_segment_number(lcd,3,0x0,1);
	
	while(1){
		vTaskDelay( 1000 / portTICK_PERIOD_MS);
		segundos++;
		if (segundos==60){
			segundos=0;
			minutos++;
			if (minutos==60){
				minutos=0;
			}
		}
		uint8_t low = segundos%10;
		uint8_t high = segundos/10;
		tm1637_set_segment_number(lcd,3,low,1);
		tm1637_set_segment_number(lcd,2,high,1);
		low = minutos%10;
		high = minutos/10;
		tm1637_set_segment_number(lcd,1,low,1);
		tm1637_set_segment_number(lcd,0,high,1);
	}

}