#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <unistd.h>
#include "esp_log.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"


#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
//#define NO_OF_SAMPLES   64          //Multisampling
const static char *TAG = "err";
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_4;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;


void delay(uint32_t delay)
{

    uint64_t start = (uint64_t)esp_timer_get_time();
    uint64_t end = delay + start ;
    while((uint64_t)esp_timer_get_time() < end);
}

void app_main(void)
{
    //Configure ADC
    if (unit == ADC_UNIT_1) {
        ESP_LOGI(TAG, "configuration ADC1");
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    } else {
         ESP_LOGI(TAG, "configuration ADC2");
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed=1;
    TIMERG0.wdt_wprotect=0;
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    uint32_t delta = 250 ;
   // int NumOfSamples =  100000;
   // int counter = NumOfSamples ;
    float start = esp_timer_get_time();
    float next = esp_timer_get_time();
    uint32_t sample = 0;
    float sleep_duration = 0;
    //Continuously sample ADC1
    while (1) {
            if (unit == ADC_UNIT_1) {
                
                sample = adc1_get_raw((adc1_channel_t)channel);
            } else {
             
                int raw;
                adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
                sample = raw;
            }
            next += delta;
            counter = counter - 1;
        
     /*    if(counter == 0){
           
            float duration = esp_timer_get_time() - start ;
            duration = duration / NumOfSamples;
            ESP_LOGI(TAG, "duration = %f" , duration);
            counter = NumOfSamples ;
            start = esp_timer_get_time();
        }*/
        sleep_duration = next - esp_timer_get_time() ;
        sleep_duration = ( sleep_duration > 0 ) ? sleep_duration : 0;
       // ESP_LOGI(TAG, "sleep_duration = %f" , sleep_duration);
       //delay(sleep_duration);
       //delay(100);
    }
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(sample, adc_chars);
        printf("Raw: %d\tVoltage: %dmV\n", sample, voltage);
        //vTaskDelay(pdMS_TO_TICKS(1000));
}