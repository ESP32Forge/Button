/**
 * @file      Button.h
 * @authors   Álvaro Velasco García
 * @date      March 16, 2025
 *
 * @brief     This header file defines the functions to initialize the board buttons and
 *            the functions to handle if the buttons were pressed/unpressed.
 */

#ifndef BSP_BUTTON_H_
#define BSP_BUTTON_H_

/***************************************************************************************
 * Includes
 ***************************************************************************************/
#include <Button_physical_connection.h>

/***************************************************************************************
 * Defines
 ***************************************************************************************/

/* List of the possible return codes that module button can return. */
#define BUTTON_RETURNS                                 \
  /* Info codes */                                     \
  BUTTON_RETURN(BSP_BUTTON_OK)                         \
  /* Error codes */                                    \
  BUTTON_RETURN(BSP_BUTTON_INIT_ERR)                   \
  BUTTON_RETURN(BSP_BUTTON_DE_INIT_ERR)                \
  BUTTON_RETURN(BSP_BUTTON_INVALID_BUTTONS_CONFIG_ERR) \
  BUTTON_RETURN(BSP_BUTTON_MODULE_WAS_NOT_INIT_ERR)    \
  BUTTON_RETURN(BSP_BUTTON_DOES_NOT_EXIST_ERR)         \
  BUTTON_RETURN(BSP_BUTTON_WAS_INITIALIZED_ERR)        \
  BUTTON_RETURN(BSP_BUTTON_WAS_NOT_INITIALIZED_ERR)              
       
/***************************************************************************************
 * Data Type Definitions
 ***************************************************************************************/

/* Enumerate that lists the posible return codes that the module can return. */
typedef enum
{
  #define BUTTON_RETURN(enumerate) enumerate,
    BUTTON_RETURNS
  #undef BUTTON_RETURN
  /* Last enumerate always, indicates the number of elements. Do not delete */
  NUM_OF_BUTTON_RETURNS,
} Button_return;

/***************************************************************************************
 * Functions Prototypes
 ***************************************************************************************/

/**
 * @brief Initializes the BSP module structures to operate the buttons. It is mandatory
 *        to call first this function before any other function of this module.
 *
 * @param void
 *
 * @return BSP_BUTTON_OK If the initialization went well, 
 *         otherise:
 * 
 *           - BSP_BUTTON_INVALID_BUTTONS_CONFIG_ERR:
 *               The provided configuration in Button_physical_connection.h is not valid.
 * 
 *           - BSP_BUTTON_INIT_ERR:
 *               An error ocurred calling intermediate functions.
 */
Button_return init_BSP_button_module(void);

/**
 * @brief Initializes a board button.
 *
 * @param ID Identifier of the button to initialize.
 *
 * @return BSP_BUTTON_OK If the initialization went well, 
 *         otherwise:
 * 
 *           - BSP_BUTTON_MODULE_WAS_NOT_INIT_ERR: 
 *               init_BSP_button_module function was not called before.
 * 
 *           - BSP_BUTTON_DOES_NOT_EXIST_ERR: 
 *               The given button does not exist in the 
 *               Button_physical_connection.h file.
 * 
 *           - BSP_BUTTON_WAS_INITIALIZED_ERR:
 *               Button was previously initialized.
 * 
 *           - BSP_BUTTON_INIT_ERR:
 *               Error trying to execute an intermediate function.
 * 
 */
Button_return init_button(const Button_ID ID);

/**
 * @brief De-initializes a board button.
 *
 * @param ID Identifier of the button to de-initialize.
 *
 * @return BSP_BUTTON_OK If the initialization went well, 
 *         otherwise:
 * 
 *           - BSP_BUTTON_MODULE_WAS_NOT_INIT_ERR: 
 *               init_BSP_button_module function was not called before.
 * 
 *           - BSP_BUTTON_DOES_NOT_EXIST_ERR: 
 *               The given button does not exist in the 
 *               Button_physical_connection.h file.
 * 
 *           - BSP_BUTTON_WAS_NOT_INITIALIZED_ERR:
 *               Button was not initialized before.
 * 
 *           - BSP_BUTTON_DE_INIT_ERR:
 *               Error trying to execute an intermediate function.
 */
Button_return de_init_button(const Button_ID ID);

/**
 * @brief Returns the current state of a button.
 *
 * @param ID Identifier of the button from which it will return the state.
 * 
 * @param state Indicates if the button was pressed or not.
 *
 * @return BSP_BUTTON_OK If the operation went well,
 *         otherwise:
 * 
 *           - BSP_BUTTON_MODULE_WAS_NOT_INIT_ERR: 
 *               init_BSP_button_module function was not called.
 * 
 *           - BSP_BUTTON_DOES_NOT_EXIST_ERR: 
 *               The given button does not exist in the 
 *               Button_physical_connection.h file.
 * 
 *           - BSP_BUTTON_WAS_NOT_INITIALIZED_ERR:
 *               Button was not initialized before.
 * 
 */
Button_return read_button_state(const Button_ID ID, Button_state *state);

/**
 * @brief Returns the number of times a button was pressed.
 *
 * @param ID Identifier of the button from which it will obtain the
 *           number of times was pressed.
 * 
 * @param num_of_presses Variable where it will store the number of times
 *                       the button was pressed.
 *
 * @return BSP_BUTTON_OK If the operation went well,
 *         otherwise:
 * 
 *           - BSP_BUTTON_MODULE_WAS_NOT_INIT_ERR: 
 *               init_BSP_button_module function was not called.
 * 
 *           - BSP_BUTTON_DOES_NOT_EXIST_ERR: 
 *               The given button does not exist in the 
 *               Button_physical_connection.h file.
 * 
 */
Button_return get_num_of_presses(const Button_ID ID, uint64_t *num_of_presses);

/**
 * @brief Prints the return of a button module function if the system was configured 
 *        in debug mode.
 *
 * @param ret Received return from a button module function.
 *
 * @return The given return.
 */
Button_return BPS_button_LOG(const Button_return ret);

/**
 * @brief Function that will be triggered when a button is pressed.
 *
 * @param ID Identifier of the button that was pressed.
 *
 * @return void
 */
void __attribute__((weak)) button_CB(const Button_ID ID); 

#endif /* BSP_BUTTON_H_ */