#include <zephyr.h>
#include <device.h>

#ifndef MY_UART_PERIPHERAL_H
#define MY_UART_PERIPHERAL_H

// Set string to be transmitted
typedef void (*my_uart_peripheral_set_string_t)(const struct device *dev, const char *string, size_t length);

// Callback
typedef void (*my_uart_peripheral_callback_t)(const struct device *dev, char *data, size_t length, bool is_string, void *user_data);

// Set the data callback function for the device
typedef void (*my_uart_peripheral_set_callback_t)(const struct device *dev, my_uart_peripheral_callback_t callback, void *user_data);

struct my_uart_peripheral_api
{
    my_uart_peripheral_set_string_t set_string;
    my_uart_peripheral_set_callback_t set_callback;
};

/**
 * @brief Set string to be transmitted.
 * 
 * @param dev Pointer to the device structure.
 * @param string String to be transmitted.
 * @param length Length of the string (excluding null byte).
 */
static inline void my_uart_set_string(const struct device *dev, const char *string, size_t length)
{
    struct my_uart_peripheral_api *api = (struct my_uart_peripheral_api *)dev->api;
    return api->set_string(dev, string, length);
}

/**
 * @brief Set the data callback function for the device
 * 
 * @param dev Pointer to the device structure.
 * @param callback Callback function pointer.
 * @param user_data Pointer to data accesible from the callback function.
 */
static inline void my_uart_set_callback(const struct device *dev, my_uart_peripheral_callback_t callback, void *user_data)
{
    struct my_uart_peripheral_api *api = (struct my_uart_peripheral_api *)dev->api;
    return api->set_callback(dev, callback, user_data);
}

#endif //MY_UART_PERIPHERAL_H