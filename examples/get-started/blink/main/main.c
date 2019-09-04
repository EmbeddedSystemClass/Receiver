#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/wdev_reg.h"
#include "xtensa/core-macros.h"
#include <stdint.h>
#include <reent.h>
#include <sys/times.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

void delay(uint32_t delay)
{

    uint64_t start = (uint64_t)esp_timer_get_time();
    uint64_t end = delay + start ;
    while((uint64_t)esp_timer_get_time() < end);
}
uint32_t getRandomDelay(){
    uint32_t random = esp_random();
    return 50 + random / 9544371 ;
}
uint8_t *buffer = 0;
uint8_t length = 0;
 void blink_task(void *pvParameter){
     gpio_pad_select_gpio(CONFIG_BLINK_GPIO);
     gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);
     buffer[4] = {1 , 1 , 0 , 0};
     length = sizeof(buffer);
    for(int i = 0 ; i < length ; i++){
        for(int b = 0 ; b < 8 ; b++ ){
           int state = (buffer[i] >> b ) & 1 ;
           if(state == 0){
               gpio_set_level(CONFIG_BLINK_GPIO , 0);
                delay(1000000);
           }
           else{
               gpio_set_level(CONFIG_BLINK_GPIO , 1);
                delay(1000000);
           }

        }

    }
 }
void app_main(){
  
    xTaskCreate(&blink_task , "blink_task" , 512 , NULL , 5 , NULL);

}