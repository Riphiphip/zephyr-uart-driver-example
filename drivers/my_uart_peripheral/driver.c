
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <string.h>

#include <errno.h>

#include "api.h"

// Compatible with "riphiphip,my_uart_peripheral"
#define DT_DRV_COMPAT riphiphip_my_uart_peripheral
#define MY_UART_PERIPHERAL_INIT_PRIORITY 41

#include <logging/log.h>
LOG_MODULE_REGISTER(my_uart_peripheral, CONFIG_MY_UART_PERIPHERAL_LOG_LEVEL);

/**
 * @brief Contains runtime mutable data for the UART peripheral.
 */
struct my_uart_data
{
    char str[CONFIG_MY_UART_PERIPHERAL_MAX_STR_LEN + 1];   // String to be transmitted. +1 for null terminator.
    uint8_t rx_buf[CONFIG_MY_UART_PERIPHERAL_RX_BUF_SIZE]; // Buffer to hold received data.
    size_t rx_data_len;                                    // Length of currently recieved data.
    my_uart_peripheral_callback_t string_callback;         // Callback function to be called when a string is received.
    void *user_data;                                       // User data to be passed to the callback function.
};

/**
 * @brief Build time configurations for the UART peripheral.
 */
struct my_uart_conf
{
    struct my_uart_data *data;           // Pointer to runtime data.
    const struct device *uart_dev;       // UART device.
    const struct gpio_dt_spec gpio_spec; // GPIO spec for pin used to start transmitting.
    struct gpio_callback gpio_cb;        // GPIO callback
};

/**
 * @brief Set the string to be transmitted by the UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @param string New string.
 * @param length Length of string excluding NULL terminator.
 */
static void user_set_string(const struct device *dev, const char *string, size_t length)
{
    struct my_uart_data *data = (struct my_uart_data *)dev->data;
    __ASSERT(length <= CONFIG_MY_UART_PERIPHERAL_MAX_STR_LEN, "String length too long.");
    memcpy(data->str, string, length);
    data->str[length] = '\0';
}

/**
 * @brief Set callback function to be called when a string is received.
 * 
 * @param dev UART peripheral device.
 * @param callback New callback function.
 * @param user_data Data to be passed to the callback function.
 */
static void user_set_string_callback(const struct device *dev, my_uart_peripheral_callback_t callback, void *user_data)
{
    struct my_uart_data *data = (struct my_uart_data *)dev->data;
    data->string_callback = callback;
    data->user_data = user_data;
}

const static struct my_uart_peripheral_api api = {
    .set_string = user_set_string,
    .set_callback = user_set_string_callback,
};

/**
 * @brief Gpio callback that starts transmitting the string specified in data->str.
 * 
 * @param port GPIO port that triggered the callback.
 * @param cb Gpio callback struct.
 * @param pins Pins that triggered the callback.
 */
static void transmit_string(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    LOG_DBG("Transmitting string");
    struct my_uart_conf *conf = CONTAINER_OF(cb, struct my_uart_conf, gpio_cb);
    struct my_uart_data *data = conf->data;
    size_t len = strlen(data->str);
    for (size_t i = 0; i < len; i++)
    {
        uart_poll_out(conf->uart_dev, data->str[i]);
    }
    uart_poll_out(conf->uart_dev, '\0');
}

/**
 * @brief Handles UART interrupts.
 * 
 * @param uart_dev UART bus device.
 * @param user_data Custom data. Should be a pointer to a my_uart_peripheral device.
 */
static void uart_int_handler(const struct device *uart_dev, void *user_data)
{
    uart_irq_update(uart_dev);
    const struct device *dev = (const struct device *)user_data; // Uart peripheral device.
    struct my_uart_data *data = dev->data;
    if (uart_irq_rx_ready(uart_dev))
    {
        my_uart_peripheral_callback_t callback = data->string_callback;
        char c;
        while (!uart_poll_in(uart_dev, &c))
        {
            data->rx_buf[data->rx_data_len] = c;
            data->rx_data_len++;
            size_t rx_buf_capacity = CONFIG_MY_UART_PERIPHERAL_RX_BUF_SIZE - data->rx_data_len;
            if (c == 0) // String found.
            {
                if (callback != NULL)
                {
                    callback(dev, data->rx_buf, data->rx_data_len, true, data->user_data);
                }
                data->rx_data_len = 0;
                memset(data->rx_buf, 0, sizeof(data->rx_buf));
            }
            else if (rx_buf_capacity == 0) // Buffer full. No string found.
            {
                if (callback != NULL)
                {
                    callback(dev, data->rx_buf, data->rx_data_len, false, data->user_data);
                }
                data->rx_data_len = 0;
                memset(data->rx_buf, 0, sizeof(data->rx_buf));
            }
        }
    }
}

/**
 * @brief Initializes the uart bus for the UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @return 0 if successful, negative errno value otherwise.
 */
static int init_uart(const struct device *dev)
{
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    uart_irq_callback_user_data_set(conf->uart_dev, uart_int_handler, (void *)dev);
    uart_irq_rx_enable(conf->uart_dev);
    return 0;
}

/**
 * @brief Initializes GPIO for the UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @return 0 on success, negative errno value on failure.
 */
static int init_gpio(const struct device *dev)
{
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    int ret;
    ret = gpio_pin_configure_dt(&conf->gpio_spec, GPIO_INPUT | GPIO_INT_DEBOUNCE);
    if (ret)
        return ret;
    ret = gpio_pin_interrupt_configure_dt(&conf->gpio_spec, GPIO_INT_EDGE_TO_ACTIVE | GPIO_INT_DEBOUNCE);
    if (ret)
        return ret;
    gpio_init_callback(&conf->gpio_cb, transmit_string, BIT(conf->gpio_spec.pin));
    ret = gpio_add_callback(conf->gpio_spec.port, &conf->gpio_cb);
    return ret;
}

/**
 * @brief Initializes UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @return 0 on success, negative error code otherwise.
 */
static int init_my_uart_peripheral(const struct device *dev)
{
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    if (!device_is_ready(conf->uart_dev))
    {
        __ASSERT(false, "UART device not ready");
        return -ENODEV;
    }
    if (init_uart(dev))
    {
        __ASSERT(false, "Failed to initialize UART device");
        return -ENODEV;
    }
    if (!device_is_ready(conf->gpio_spec.port))
    {
        __ASSERT(false, "GPIO device not ready");
        return -ENODEV;
    }
    if (init_gpio(dev))
    {
        __ASSERT(false, "Failed to initialize GPIO device.")
        return -ENODEV;
    }
    LOG_DBG("My UART peripheral initialized");
    return 0;
}

#define INIT_MY_UART_PERIPHERAL(inst)                             \
    static struct my_uart_data my_uart_peripheral_data_##inst = { \
        .str = DT_INST_PROP_OR(inst, initial_string, ""),         \
        .string_callback = NULL,                                  \
        .rx_data_len = 0,                                         \
        .rx_buf = {0},                                            \
    };                                                            \
    static struct my_uart_conf my_uart_peripheral_conf_##inst = { \
        .data = &my_uart_peripheral_data_##inst,                  \
        .uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),             \
        .gpio_spec = GPIO_DT_SPEC_INST_GET(inst, button_gpios),   \
        .gpio_cb = {0},                                           \
    };                                                            \
    DEVICE_DT_INST_DEFINE(inst,                                   \
                          init_my_uart_peripheral,                \
                          NULL,                                   \
                          &my_uart_peripheral_data_##inst,        \
                          &my_uart_peripheral_conf_##inst,        \
                          POST_KERNEL,                            \
                          MY_UART_PERIPHERAL_INIT_PRIORITY,       \
                          &api);

DT_INST_FOREACH_STATUS_OKAY(INIT_MY_UART_PERIPHERAL);