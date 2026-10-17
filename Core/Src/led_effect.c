/**
  ******************************************************************************
  * @file    led_effect.c
  * @author  Lukasz Garlicki
  * @brief   This file contains the implementation of LED effects
  *
  *
  * In this file, all of the LED Heart lighting states are implemented,
  * alongside the handling of interrupt from the button. It is fairly possible
  * for the state to be expanded in the future
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "led_effect.h"

/* Variables -----------------------------------------------------------------*/
/* Array of parameters for each of the LED */
led_t heart_leds[16] = {
    {GPIOA, GPIO_PIN_5, MAX_BRIGHTNESS}, {GPIOA, GPIO_PIN_6, MAX_BRIGHTNESS},
    {GPIOA, GPIO_PIN_4, MAX_BRIGHTNESS}, {GPIOA, GPIO_PIN_7, MAX_BRIGHTNESS},
    {GPIOA, GPIO_PIN_3, MAX_BRIGHTNESS}, {GPIOB, GPIO_PIN_0, MAX_BRIGHTNESS},
    {GPIOA, GPIO_PIN_2, MAX_BRIGHTNESS}, {GPIOB, GPIO_PIN_2, MAX_BRIGHTNESS},
    {GPIOA, GPIO_PIN_15, MAX_BRIGHTNESS}, {GPIOA, GPIO_PIN_1, MAX_BRIGHTNESS},
    {GPIOA, GPIO_PIN_8, MAX_BRIGHTNESS}, {GPIOA, GPIO_PIN_11, MAX_BRIGHTNESS},
    {GPIOA, GPIO_PIN_0, MAX_BRIGHTNESS}, {GPIOC, GPIO_PIN_15, MAX_BRIGHTNESS},
    {GPIOC, GPIO_PIN_6, MAX_BRIGHTNESS}, {GPIOC, GPIO_PIN_14, MAX_BRIGHTNESS}
};
/* Holds the current state of LED heart */
volatile static uint8_t current_state = STATE_INIT;
/* Flag for state change, set by interrupt from a button */
volatile static uint8_t state_change = 0;
/* Flag being set by timer, used for simple software PWM module */
volatile static uint8_t pwm_tick_flag;

/* Sequence of LEDs to be light up to achieve clockwise effect. Irregular numbers
 * are the effect of how the PCB was designed */
static const uint8_t clock_sequence[16] = {0, 1, 3, 5, 7, 10, 14, 11, 8, 12, 15, 13, 9, 6, 4, 2};
/* Values of brightnesses for leading diode and it's tail in Clock state */
static const uint8_t clock_tail_values[4] = {100, 20, 8, 3};

/* Values of brightnesses for diodes in Heart state */
static const uint8_t heart_sequence[64] = {
            /* First hearbeat */
            0, 10, 20, 60, 90, 60, 20, 10, 5,
            /* Break */
            0, 0,
            /* Second heartbeat */
            10, 40, 80, 100, 100, 80, 40, 10,
            /* Pause */
            5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Values of brightnesses for diodes in Breathing state */
static const uint8_t breathing_sequence[64] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28,
                                            31, 34, 37, 40, 43, 46, 49, 52, 55, 58,
                                            61, 64, 67, 70, 73, 76, 79, 82, 85, 88,
                                            91, 94, 100, 94, 91, 88, 85, 82,
                                            79, 76, 73, 70, 67, 64, 61, 58, 55, 52,
                                            49, 46, 43, 40, 37, 34, 31, 28, 25, 22,
                                            19, 16, 13, 10, 7, 4};


/* Information about which diodes to light up to display initialization sequence.
 * Initialization is performed in steps.
 * Each row of the matrix tells which LEDs should be light up in current step.
 * -1 are placeholders due to the heart having 4 LEDs in the widest place.
 */
static const int8_t initialization_sequence[7][4] = {{0, -1, -1, -1},
                                                        {1, 2, -1 ,-1},
                                                        {3, 4, -1, -1},
                                                        {5, 6, -1, -1},
                                                        {7, 8, 9, -1},
                                                        {10, 11, 12, 13},
                                                        {14, 15, -1, -1}};

/**
  * @brief Set brightness parameter of single LED
  * @param target_led LED number to be set (from 0 to 15) - note that diodes on PCB are
  * numbered from 1 to 16, thus requiring to substract one from the desire diode number
  * @param desired_brightness Desired brigthness level to set chosen diode to (value between 0 and 100)
  */
static void LED_Set_Brightness_Single(uint8_t target_led, uint8_t desired_brightness)
{
    heart_leds[target_led].brightness = desired_brightness;
}
/**
  * @brief Set brightness parameter of every single LED
  * @param desired_brightness Desired brigthness level to set every diode (value between 0 and 100)
  */
static void LED_Set_Brightness_All(uint8_t desired_brightness)
{
    for (uint8_t led = 0; led < 16; led++)
    {
        heart_leds[led].brightness = desired_brightness;
    }
}

void PWM_Flag_SetActive(void)
{
    pwm_tick_flag = 1;
}
void PWM_Flag_SetInactive(void)
{
    pwm_tick_flag = 0;
}
uint8_t PWM_Flag_Get(void)
{
    return pwm_tick_flag;
}
void LED_Next_State(void)
{
    state_change = 1;

    current_state++;

    if (current_state == STATE_AMOUNT)
    {
        current_state = STATE_OFF;
    }
}


/**
  * @brief LED Heart initialization sequence (added for extra effect; can be skipped)
  *
  * Initialization consists of two parts - first is gradually lighting up rows of LED with short delay,
  * and the second makes the entire LED blink twice with shorter that first part delay in-between.
  */
static void LED_Initialization(void) {
    static uint32_t last_time = 0;
    static uint8_t row = 0;
    static uint8_t blink = 0;
    static uint16_t delay = 300;
    static uint8_t leds_on = 1;
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_time > delay)
    {
        last_time = current_time;
        LED_Set_Brightness_All(0);
        /* Gradual lighting up of diodes is finished, so the delay is reduced */
        if (row == 7)
        {
            delay = 200;
        }
        /* Handles the first part (gradual lighting up) */
        if (row < 7) {
            for (uint8_t col = 0; col < 4; col++)
            {
                /* Due to 'rows' having inconstant number of diodes, initialization_sequence matrix
                 * is normalized to have 4 elements (equivalent of longest row) in each row, thus
                 * -1 serve as a filler meant to be ignored
                 */
                if (initialization_sequence[row][col] != -1)
                {
                    LED_Set_Brightness_Single(initialization_sequence[row][col], 100);
                }
            }

            row++;
        }

        /* Handling the second part of initialization - the blinking state. */
        else if (blink != 4)
        {
            if (leds_on)
            {
                LED_Set_Brightness_All(100);
            }

            leds_on = !leds_on;
            blink++;
        }

        /* Initialization has finished */
        else
        {
            row = 0;
            blink = 0;
            current_state = STATE_LOW_BRIGHTNESS;
            state_change = 1;
            leds_on = 0;
        }

    }
}


/**
  * @brief Processing of each LED Heart state and its corresponding mode of lighting
  */

void LED_Process_State(void){
    static uint32_t last_step_time = 0;
    static uint8_t step_index = 0;
    /* Diode index array for tail effect in Clock state */
    static int8_t tail[4] = {-1, -1, -1, -1};
    uint32_t current_time = HAL_GetTick();

    if (state_change)
    {
        LED_Set_Brightness_All(0);
        step_index = 0;
        last_step_time = HAL_GetTick();
        state_change = 0;

        /* STATE_HEART follows after STATE_CLOCK_WITH_TAIL - we need to
         * reset the tail array so the next time we trigger Clock state,
         * it starts constructing the tail properly
         */
        if (current_state == STATE_HEART)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                tail[i] = -1;
            }
        }
    }

    switch(current_state)
    {
      case STATE_OFF:
      break;

      case STATE_LOW_BRIGHTNESS:
          LED_Set_Brightness_All(20);
      break;

      case STATE_HIGH_BRIGHTNESS:
          LED_Set_Brightness_All(100);
      break;

      case STATE_CLOCK_WITH_TAIL:
        if (current_time - last_step_time >= HIGH_DELAY)
        {
            last_step_time = current_time;
            LED_Set_Brightness_All(0);
            /* Construction of tail effect - slightly diminishing LED tail following the leading
             * diode */
            tail[3] = tail[2];
            tail[2] = tail[1];
            tail[1] = tail[0];
            tail[0] = clock_sequence[step_index];

            for (uint8_t i = 0; i < 4; i++)
            {
                if (tail[i] != -1)
                {
                    LED_Set_Brightness_Single(tail[i], clock_tail_values[i]);
                }
            }

            step_index++;

            if (step_index >= 16)
            {
                step_index = 0;
            }

          }
      break;

      case STATE_HEART:
          if (current_time - last_step_time >= LOW_DELAY)
          {
            last_step_time = current_time;
            LED_Set_Brightness_All(heart_sequence[step_index]);
            step_index++;
            if (step_index >= 64)
            {
                step_index = 0;
            }

          }

      break;

      case STATE_BREATHING:
          if(current_time - last_step_time >= LOW_DELAY)
          {
            last_step_time = current_time;
            LED_Set_Brightness_All(breathing_sequence[step_index]);
            step_index++;
            if (step_index >= 64)
            {
                step_index = 0;
            }

          }
      break;

      case STATE_INIT:
          LED_Initialization();
      }
}
/**
 * @brief Handler of simple software PWM LED brightness control
 */

void LED_Handler(void){

    static uint8_t pwm_counter = 0;
    if (!pwm_tick_flag)
    {
        return;
    }

    pwm_tick_flag = 0;

    /* Using atomic register operations to ensure fastest operation */
    for (uint8_t led = 0; led < 16; led++)
    {
        if (heart_leds[led].brightness > pwm_counter)
        {
            heart_leds[led].port->BSRR = heart_leds[led].pin;
        }

        else
        {
            heart_leds[led].port->BSRR = (uint32_t)heart_leds[led].pin << 16U;
        }

    }

      pwm_counter++;
      if (pwm_counter >= 100)
      {
          pwm_counter = 0;
      }
}

