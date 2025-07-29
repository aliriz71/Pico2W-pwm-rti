#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

#define TRIG_PIN 2  // Trigger Pin for Ultrasonic Sensor
#define ECHO_PIN 3  // Echo Pin for Ultrasonic Sensor
#define LED_PIN 9   // PWM Output (For LED or Servo)
#define PWM_WRAP 65535
#define DISTANCE_THRESHOLD 15  // 15cm threshold

// Function to measure distance
float get_distance() {
    uint64_t start_time, end_time;
    
    // Send a 10micro second pulse to trigger the sensor
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);
    
    // Wait for ECHO to go HIGH
    while (gpio_get(ECHO_PIN) == 0);
    start_time = time_us_64();
    
    // Wait for ECHO to go LOW
    while (gpio_get(ECHO_PIN) == 1);
    end_time = time_us_64();
    
    // Calculate pulse duration and convert to distance (cm)
    float distance = (end_time - start_time) * 0.0343 / 2;
    return distance;
}

volatile int duty_cycle = PWM_WRAP * 0.1;  // Start at 10% duty cycle
volatile bool increasing = false;

// RTI Interrupt Handler for PWM Soft Start/Stop
int64_t pwm_rti_handler(alarm_id_t id, void *user_data) {
    float distance = get_distance();
    
    if (distance <= DISTANCE_THRESHOLD) {
        increasing = true;  // Object in range --> Soft Start
    } else {
        increasing = false;  // Object out of range --> Soft Stop
    }

    if (increasing) {
        duty_cycle += (PWM_WRAP * 0.9) / 9;
        if (duty_cycle >= PWM_WRAP) duty_cycle = PWM_WRAP;
    } else {
        duty_cycle -= PWM_WRAP / 10;
        if (duty_cycle <= 0) duty_cycle = 0;
    }

    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_PIN), pwm_gpio_to_channel(LED_PIN), duty_cycle);
    return 100000;  // Re-trigger RTI every 100ms
}

// Function to Initialize RTI
void setup_rti() {
    add_alarm_in_us(100000, pwm_rti_handler, NULL, true);
}

int main() {
    stdio_init_all();

    // Configure Ultrasonic Sensor Pins
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    // Configure LED/Servo for PWM
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
    uint channel = pwm_gpio_to_channel(LED_PIN);

    //configure pwm 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0);
    pwm_config_set_wrap(&config, PWM_WRAP);
    pwm_init(slice_num, &config, true);

    // Start RTI for soft start/stop
    setup_rti();

    while (1) {
        sleep_ms(1000);  // Keep the program running
    }
}





