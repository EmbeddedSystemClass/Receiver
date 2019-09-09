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
#include "freertos/task.h"
#include "esp_task_wdt.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
const static char *TAG = "message";
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_4;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

int length, len_count =0 , pay_bit_len= 0 ,pay_bit_count= 0 ;
int state= 0;
uint8_t P_buffer = 0;
uint8_t buff[63];



void delay(uint32_t delay)
{
    uint64_t start = (uint64_t)esp_timer_get_time();
    uint64_t end = delay + start ;
    while((uint64_t)esp_timer_get_time() < end);
}
 void parse(int bit){

    int byte_position = 0;
    int bit_position = 0;

    switch(state){
        case 0 :
        P_buffer = (P_buffer << 1) | bit;
        //ESP_LOGI(TAG,"P_buffer = %u " , P_buffer);

        if(P_buffer == 0x99){
           // ESP_LOGI(TAG,"preamble  = %u " , P_buffer);

            state = 1;
            P_buffer = 0;
            len_count = 0;
            length = 0;
        }
        break; 
        case 1 :
        length |= (bit << len_count);
        len_count += 1;
        
        if(len_count >= 8){
            state = 2;
            pay_bit_len = 8 * length;
            pay_bit_count = 0;
        }
        break;
        case 2 :
        byte_position = pay_bit_count / 8;
        bit_position = pay_bit_count % 8;
        buff[byte_position] |= (bit << bit_position);
        pay_bit_count += 1;
        if(pay_bit_count >= pay_bit_len){
            state = 0;
            ESP_LOG_BUFFER_HEX(TAG, buff,length);
        }
        break;
    }

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

    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
    TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed=1;
    TIMERG0.wdt_wprotect=0;

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    uint32_t delta = 500 ;
    float alpha = 0.01;
    float average_voltage = 0;
    float next = esp_timer_get_time();
    float edge_time;
    uint32_t sample = 0;
    float sleep_duration = 0;
    int bit,last_bit = 0;
    float prev_time = 0;
    float T_min = 0.8;
    float T_max = 1.2;
    int P = 4000;
    //Continuously sample ADC1
    while (1) {

        if (unit == ADC_UNIT_1) {
            sample = adc1_get_raw((adc1_channel_t)channel); 
        }
        else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
            sample = raw;
        }
        average_voltage = alpha * sample + (1 - alpha) * average_voltage;
        bit = (sample > average_voltage) ? 1 : 0 ;
        if(bit != last_bit){
            edge_time = esp_timer_get_time();
            if(edge_time - prev_time < (T_min * P)){
                /*discarded edge */
                //ESP_LOGI(TAG, "discard shod");
            }
            else if(edge_time - prev_time > (T_max * P)){
                /* Resync */
                
                state = 0;
                length = 0;
                len_count =0;
                pay_bit_len= 0;
                pay_bit_count= 0;
                P_buffer = 0;
                parse(bit);
                prev_time = edge_time;
            }
            else{
                /*correct edge*/
                
                parse(bit);
                prev_time = edge_time;
            }
        }
        last_bit = bit;
        next += delta;
        sleep_duration = next - esp_timer_get_time();
        sleep_duration = ( sleep_duration > 0 ) ? sleep_duration : 0;
        delay(sleep_duration); 
    }      
}