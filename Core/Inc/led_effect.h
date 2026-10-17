/**
  ******************************************************************************
  * @file    led_effect.h
  * @author  Lukasz Garlicki
  * @brief   This file contains function prototypes, variables and structs
  *          for handling LED effects
  ******************************************************************************
  */

/* Includes  -----------------------------------------------------------------*/
#include <stdint.h>
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef INC_LED_EFFECT_H_
#define INC_LED_EFFECT_H_

/* Defines -------------------------------------------------------------------*/
/* Amount of LED Heart lighting states */
#define STATE_AMOUNT 7
/* Max brightness value of LEDs */
#define MAX_BRIGHTNESS 100
/* Higher delay, used for Clock effect (making the effect not rushed) */
#define HIGH_DELAY 100
/* Lower delay, used for e.g. Heart and Breathing effect (giving smoothness) */
#define LOW_DELAY 30

/* Typedefs ------------------------------------------------------------------*/

/**
 * @brief  Enumeration describing various lighting states of LED Heart
 */
typedef enum
{
    STATE_OFF = 1,          /*!< All diodes are OFF */
    STATE_LOW_BRIGHTNESS,   /*!< All diodes are lighted up, emanating low brightness light */
    STATE_HIGH_BRIGHTNESS,  /*!< All diodes are lighted up, emanating hight brightness light */
    STATE_CLOCK_WITH_TAIL,  /*!< Single diode is light up following clockwise orientation with
                                 'tail' of dimmed LEDs following the leading diode */
    STATE_HEART,            /*!< Diodes light up in a manner resembling beating of human heart */
    STATE_BREATHING,        /*!< Diode slowly light up to max brightness and slowly dim down */
    STATE_INIT              /*!< Short initialization sequence for extra effect, can be skipped
                                 with button */
} led_state_t;


/**
 * @brief  Struct holding information about GPIO port and pin responsible for controlling a
 *         certain LED, alongside information about it's brightness
 */
typedef struct
{
    GPIO_TypeDef * port;    /*!< Pointer to the GPIO port */
    uint16_t pin;           /*!< Certain pin number */
    uint16_t brightness;    /*!< Level of brightness, from 0 to 100 */
} led_t;

/* Variables -----------------------------------------------------------------*/



/**
  * @brief Processing of each LED Heart state and its corresponding mode of lighting
  */
void LED_Process_State(void);
/**
 * @brief Handler of simple software PWM LED brightness control
 */
void LED_Handler(void);
/**
  * @brief Sets the PWM tick flag active (writes as 1)
  */
void PWM_Flag_SetActive(void);
/**
 * @brief Sets the PWM tick flag ainctive (writes as 0)
 */
void PWM_Flag_SetInactive(void);
/**
 * @brief Gets the state of PWM tick flag
 * @retval Returns the value of PWM flag (0 or 1)
 */
uint8_t PWM_Flag_Get(void);
/**
 * @brief Sets next state of LED Heart
 */
void LED_Next_State(void);

#endif /* INC_LED_EFFECT_H_ */
