/* 
 * Created by Shui Jiang.
 * Date of Creation: July 13rd, 2021
 * Affiliation: USSLab, Zhejiang University
 */

#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "dutyArray.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_11_BIT // Set duty resolution to 11 bits
#define LEDC_FREQUENCY          (32000) // Frequency in Hertz. Set frequency at 25 kHz
#define LEDC_INT_ENA_REG        (0x3FF59188) // enable interrupt bit 
#define LEDC_INT_CLR_REG        (0x3FF5918C) // clear interrupt bit 

static intr_handle_t handle_console;

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_FADE_END,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set initial duty to 0%
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static int num = 0;
static int calibFactor = 2;
static int fiveSecCnt = 0; 
static void IRAM_ATTR ledc_isr_handler(void* arg)
{
    WRITE_PERI_REG(LEDC_INT_ENA_REG, 0x1);
    WRITE_PERI_REG(LEDC_INT_CLR_REG, 0x1);
    if (fiveSecCnt <= 2*LEDC_FREQUENCY)
    {
        num++;
        if (num == (sizeof(dutyArr)/sizeof(dutyArr[0]))*calibFactor+1) {num = 0; return; }
        if (num % calibFactor == 0){
            // assign the new duty cycle according to dutyArr
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, dutyArr[num/calibFactor]));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        }

    }
    // redundant last 3 secs, fill in zeros
    if (fiveSecCnt >= 5*LEDC_FREQUENCY)
    {
        fiveSecCnt = 0;
    }
    fiveSecCnt++;
} 

void app_main(void)
{
    // Set the LEDC peripheral configuration
    example_ledc_init();
    WRITE_PERI_REG(LEDC_INT_ENA_REG, 0x1);
    ESP_ERROR_CHECK(ledc_isr_register(ledc_isr_handler, NULL, ESP_INTR_FLAG_IRAM, &handle_console));
    
}
