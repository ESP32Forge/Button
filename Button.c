/**
 * @file      Button.c
 * @authors   Álvaro Velasco García
 * @date      March 16, 2025
 *
 * @brief     This source file declares the functions to initialize the board buttons and
 *            the functions to handle if the buttons were pressed/unpressed.
 */

/***************************************************************************************
 * Includes
 ***************************************************************************************/
#include <Button.h>
#include <freertos/FreeRTOS.h>
#include <string.h>
#include <Debug.h>

/***************************************************************************************
 * Defines
 ***************************************************************************************/

/* Assistance macro to check if the module was initialized. */
#define CHECK_IF_MODULE_WAS_INTIALIZED         \
  if(!button_module_was_initialized)           \
  {                                            \
    return BSP_BUTTON_MODULE_WAS_NOT_INIT_ERR; \
  }

#if DEBUG_MODE_ENABLE == 1
/* Tag to show traces in button BSP module. */
#define TAG "BSP_BUTTON"
#endif

/***************************************************************************************
 * Data Type Definitions
 ***************************************************************************************/

/* Structure that contains the information of a system buttons. */
typedef struct
{
  /* Identifier of the button. */
  Button_ID ID;
  /* Flag to indicate if the button was initialized. */
  bool was_initialized;
  /* Identifier of the GPIO that reads the state of the button. */
  gpio_num_t GPIO;
  /* Indicates the pull mode of the button GPIO. */
  gpio_pull_mode_t pull_mode;
  /* Which interruption type will trigger the button GPIO. */
  gpio_int_type_t interruption_type;
  /* Current button state. */
  Button_state state;
  /* Time handler that controls the button bounce. */
  TimerHandle_t debounce;
  /* Flag that indicates if the debounce timer expired. */
  bool debounce_timer_expired;
  /* Indicates the number of times the button was pressed. */
  uint64_t num_of_presses;
} system_button_info;

/***************************************************************************************
 * Global variables
 ***************************************************************************************/

/* Flag that indicates if the button module was initialized or not. */
static bool button_module_was_initialized;

/* Array that contains the configuration of the all the system buttons. */
static system_button_info system_buttons_infos[NUM_OF_BUTTONS] =
{
  #define BUTTON_CONFIG(BUTTON_ID, BUTTON_GPIO_ID, BUTTON_GPIO_PULL_MODE, INT_TYP,  \
                        BOUNCE_TIME)                                                \
    {                                                                               \
      /* Button ID */              BUTTON_ID,                                       \
      /* Was initialized */        false,                                           \
      /* Button GPIO ID */         BUTTON_GPIO_ID,                                  \
      /* Button GPIO pull mode */  BUTTON_GPIO_PULL_MODE,                           \
      /* Interruption type */      INT_TYP,                                         \
      /* Button state */           BUTTON_IS_NOT_PRESSED,                           \
      /* Debounce timer */         NULL,                                            \
      /* Debounce timer expired */ false,                                           \
      /* Number of presses. */     0u,                                              \
    },                                                                        
    BUTTONS_CONFIGURATIONS
  #undef BUTTON_CONFIG
};

/***************************************************************************************
 * Function Prototypes
 ***************************************************************************************/

/**
 * @brief Checks if the configuration of the system buttons defined in 
 *        BUTTONS_CONFIGURATIONS macro (Button_physical_connection.h) list
 *        is correct. Also, as it need to loop each button configuration,
 *        it sort them to avoid inncessary complexity cost in the rest of
 *        the module function. Also, as it needs to loop each button,
 *        it initializes the bounce timers.
 *
 * @param void
 *
 * @return True if the configuration is correct, otherwise false.
 */
static bool check_configurations_sort_and_init(void);

/**
 * @brief Checks if the given button ID is defined in the system.
 *
 * @param ID Button identifier to check.
 *
 * @return True if the button exists, otherwise false.
 */
static bool check_button_ID(const Button_ID ID);

/**
 * @brief Function that will be trigered when a button is pressed.
 *
 * @param arg [Button_id] Identifier of the button was pressed.
 *
 * @return void
 */
static void IRAM_ATTR generic_button_CB(void *args);

/**
 * @brief Function that will be triggered after the debounce time finished.
 *
 * @param timer_handler Handler of the timer to manage it.
 *
 * @return void
 */
static void timer_CB(TimerHandle_t timer_handler);

/***************************************************************************************
 * Functions 
 ***************************************************************************************/

Button_return init_BSP_button_module(void)
{

  if(!button_module_was_initialized)
  {

    if(!check_configurations_sort_and_init())
    {
      return BSP_BUTTON_INVALID_BUTTONS_CONFIG_ERR;
    }

    if(ESP_error_check(gpio_install_isr_service(0)) != ESP_OK)
    {
      return BSP_BUTTON_INIT_ERR;
    }

    button_module_was_initialized = true;
  }

  return BSP_BUTTON_OK;
}

Button_return init_button(const Button_ID ID)
{

  CHECK_IF_MODULE_WAS_INTIALIZED;

  if(!check_button_ID(ID))
  {
    return BSP_BUTTON_DOES_NOT_EXIST_ERR;
  }

  if(system_buttons_infos[ID].was_initialized)
  {
    return BSP_BUTTON_WAS_INITIALIZED_ERR;
  }

  if(ESP_error_check(gpio_reset_pin(system_buttons_infos[ID].GPIO)) != ESP_OK)
  {
    return BSP_BUTTON_INIT_ERR;
  }

  if(ESP_error_check(gpio_set_direction(system_buttons_infos[ID].GPIO, GPIO_MODE_INPUT))
      != ESP_OK)
  {
    return BSP_BUTTON_INIT_ERR;
  }
  
  if(ESP_error_check(gpio_set_pull_mode(system_buttons_infos[ID].GPIO, 
      system_buttons_infos[ID].pull_mode)) != ESP_OK)
  {
    return BSP_BUTTON_INIT_ERR;
  }

  if(ESP_error_check(gpio_set_intr_type(system_buttons_infos[ID].GPIO, 
     system_buttons_infos[ID].interruption_type)) != ESP_OK ||
     ESP_error_check(gpio_isr_handler_add(system_buttons_infos[ID].GPIO, 
      generic_button_CB, &system_buttons_infos[ID].ID)) != ESP_OK)
  {
    return BSP_BUTTON_INIT_ERR;
  }

  system_buttons_infos[ID].was_initialized = true;
  system_buttons_infos[ID].debounce_timer_expired = true;

  return BSP_BUTTON_OK;
}

Button_return de_init_button(const Button_ID ID)
{

  CHECK_IF_MODULE_WAS_INTIALIZED;

  if(!check_button_ID(ID))
  {
    return BSP_BUTTON_DOES_NOT_EXIST_ERR;
  }

  if(!system_buttons_infos[ID].was_initialized)
  {
    return BSP_BUTTON_WAS_NOT_INITIALIZED_ERR;
  }

  if(ESP_error_check(gpio_reset_pin(system_buttons_infos[ID].GPIO)) != ESP_OK)
  {
    return BSP_BUTTON_DE_INIT_ERR;
  }

  if(ESP_error_check(gpio_set_intr_type(system_buttons_infos[ID].GPIO,
     GPIO_INTR_DISABLE)) != ESP_OK)
  {
    return BSP_BUTTON_DE_INIT_ERR;
  }

  return BSP_BUTTON_OK;
}

Button_return read_button_state(const Button_ID ID, Button_state *state)
{

  CHECK_IF_MODULE_WAS_INTIALIZED;

  if(!check_button_ID(ID))
  {
    return BSP_BUTTON_DOES_NOT_EXIST_ERR;
  }

  if(!system_buttons_infos[ID].was_initialized)
  {
    return BSP_BUTTON_WAS_NOT_INITIALIZED_ERR;
  }

  *state = system_buttons_infos[ID].state;

  return BSP_BUTTON_OK;
}

Button_return get_num_of_presses(const Button_ID ID, uint64_t *num_of_presses)
{

  CHECK_IF_MODULE_WAS_INTIALIZED;

  if(!check_button_ID(ID))
  {
    return BSP_BUTTON_DOES_NOT_EXIST_ERR;
  }

  *num_of_presses = system_buttons_infos[ID].num_of_presses;

  return BSP_BUTTON_OK;
}

inline Button_return BPS_button_LOG(const Button_return ret)
{
  #if DEBUG_MODE_ENABLE == 1
    switch(ret)
    {
      #define BUTTON_RETURN(enumerate) \
        case enumerate:                \
          if(ret > 0)                  \
          {                            \
            ESP_LOGE(TAG, #enumerate); \
          }                            \
          else                         \
          {                            \
            ESP_LOGI(TAG, #enumerate); \
          }                            \
          break;       
        BUTTON_RETURNS
      #undef BUTTON_RETURN
      default:
        ESP_LOGE(TAG, "Unkown return.");
        break;
    }
  #endif
  return ret;
}

static bool check_configurations_sort_and_init(void)
{

  system_button_info buttons_info[NUM_OF_BUTTONS] = {0u};

  for(Button_ID i = 0u; i < NUM_OF_BUTTONS; i++)
  {

    if(!check_button_ID(system_buttons_infos[i].ID))
    {
      return false;
    }

    /* Check if the given GPIO is valid. */
    if(!GPIO_IS_VALID_GPIO(system_buttons_infos[i].GPIO))
    {
      return false;
    }

    /* Check if the pull mode is valid. */
    switch(system_buttons_infos[i].pull_mode)
    {
      case GPIO_PULLUP_ONLY: break;
      case GPIO_PULLDOWN_ONLY: break;
      case GPIO_PULLUP_PULLDOWN: break;
      case GPIO_FLOATING: break;
      default: 
        return false;
    }
    
    /* Ensure that is in the correct index. */
    buttons_info[system_buttons_infos[i].ID] = system_buttons_infos[i];
  }

  for(Button_ID i = 0u; i < NUM_OF_BUTTONS; i++)
  {

    /* Copy the sorted array in the global. */
    system_buttons_infos[i] = buttons_info[i];

    /* Initialize the timer that controls the bounce effect and the semaphore to acquire 
     * the state ensuring atomicity.
     */
    switch(i)
    {
      #define BUTTON_CONFIG(BUTTON_ID, BUTTON_GPIO_ID, BUTTON_GPIO_PULL_MODE, INT_TYP, \
                            BOUNCE_TIME)                                               \  
        case BUTTON_ID:                                                                \
          system_buttons_infos[i].debounce = xTimerCreate(#BUTTON_ID,                  \
            BOUNCE_TIME/portTICK_PERIOD_MS, pdFALSE, (void *) 0, timer_CB);            \
                                                                                       \
          if(system_buttons_infos[i].debounce == NULL)                                 \
          {                                                                            \
            return false;                                                              \
          }                                                                            \                                                                     
                                                                                       \                                                              
          break;                                                           
        BUTTONS_CONFIGURATIONS
      #undef BUTTON_CONFIG
      default:
        /* Imposible to reach this region code as it was cheked before, defensive code */
        return false;
    }

  }

  return true;
}

static bool check_button_ID(const Button_ID ID)
{
  switch(ID)
  {
    #define BUTTON(BUTTON_ID)      \
      case BUTTON_ID: return true;   
      BUTTONS
    #undef BUTTON
    default: 
      return false;
  }
}

static void generic_button_CB(void *args)
{

  const Button_ID ID = *((Button_ID*)args);
  BaseType_t higher_priority_task_woken = pdFALSE;

  /* Anti-bouncing logic */
  if(xTimerIsTimerActive(system_buttons_infos[ID].debounce) == pdFALSE && 
     system_buttons_infos[ID].debounce_timer_expired)
  {

    system_buttons_infos[ID].debounce_timer_expired = false;

    system_buttons_infos[ID].state = 
      (Button_state)gpio_get_level(system_buttons_infos[ID].GPIO);

    /* Call the CB provided. */
    system_buttons_infos[ID].num_of_presses++;
    button_CB(ID);

    xTimerStartFromISR(system_buttons_infos[ID].debounce, &higher_priority_task_woken);

  }

}

static void timer_CB(TimerHandle_t timer_handler)
{

  const char* timer_name = pcTimerGetName(timer_handler); 
  Button_ID ID = 0;

  /* 
   * I know it's hard to use a string comparison considering that it's computationally 
   * very expensive. My engineer soul would go to hell for this, but the other 
   * alternative I had with little effort was to use the same identifier for the button
   * and the timer, in that case the problem is that if another module uses the FreeRTOS
   * timers and the same identifier is used we would have a problem, I have sold my soul
   * for scalability. May God forgive my sins.
   */
  #define BUTTON(enumerate)                 \          
    if(strcmp(timer_name, #enumerate) == 0) \
    {                                       \
      ID = enumerate;                       \
    }
    BUTTONS
  #undef BUTTON

  BaseType_t higher_priority_task_woken = pdFALSE;
  if(gpio_get_level(system_buttons_infos[ID].GPIO) == BUTTON_IS_PRESSED)
  {
    xTimerStartFromISR(system_buttons_infos[ID].debounce, &higher_priority_task_woken);
  }
  else
  {
    system_buttons_infos[ID].debounce_timer_expired = true;
  }
}