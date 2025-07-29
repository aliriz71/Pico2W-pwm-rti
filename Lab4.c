#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

#define LED_PIN 9         // GPIO9 for PWM output
#define PWM_WRAP 65535    // 16-bit resolution
#define RTI_INTERVAL_US 100000  // RTI interval in microseconds (100ms)

// Global Variables
volatile int duty_cycle = PWM_WRAP * 0.1;  // Start at 10% duty cycle as said in manual
volatile bool increasing = true;  // while true increase dutycycle(brightness)

// RTI Interrupt Handler
int64_t pwm_rti_handler(alarm_id_t id, void *user_data) {
    if (increasing) {
        duty_cycle += (PWM_WRAP * 0.9) / 9;  // Increment duty cycle
        if (duty_cycle >= PWM_WRAP) {
            duty_cycle = PWM_WRAP;
            increasing = false;  // Switch to soft stop mode
        }
    } else {
        duty_cycle -= PWM_WRAP / 10;  // Decrement duty cycle
        if (duty_cycle <= 0) {
            duty_cycle = 0;
            increasing = true;  // Switch back to soft start mode
        }
    }

    // Update PWM level
    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_PIN), pwm_gpio_to_channel(LED_PIN), duty_cycle);

    // Schedule next interrupt
    return RTI_INTERVAL_US;  // fixing the RTI for the next cycle
}

//timer used in Pico, becuze Pico does not have an RTICTL
void setup_rti() {
    add_alarm_in_us(RTI_INTERVAL_US, pwm_rti_handler, NULL, true);  // Schedule the first RTI
}

int main() {
    stdio_init_all();

    // Configure LED pin for PWM
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
    uint channel = pwm_gpio_to_channel(LED_PIN);

    // Configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0);  // Adjust clock divider
    pwm_config_set_wrap(&config, PWM_WRAP);
    pwm_init(slice_num, &config, true);

    // Set up the Real-Time Interrupt (RTI)
    setup_rti();

    while (1) {
        sleep_ms(1000);  
    }
}



