# Zephyr uart peripheral driver 

(Near) minimal example for a UART peripheral device driver for Zephyr v2.6.0. Implements a driver for a "soft" device that sends and recieves null-terminated strings over UART. 

This project is intended as a companion for [this article on DevZone](https://devzone.nordicsemi.com/nordic/nrf-connect-sdk-guides/b/peripherals/posts/writing-device-drivers-for-uart-peripherals).

## Required hardware
To run this project the following hardware is required:
 - One of the [supported boards](#supported-boards) or any other zephyr supported board if you're comfortable writing your own devicetree overlays.   
 - Jumper wire or some other way to connect the TX and RX pin of your microcontroller.
 - Some way to set a GPIO input pin. The supported boards all do this through inbuilt buttons.

## Supported boards
This project currently supports the following boards out of the box:

- [nRF9160 Development Kit](https://www.nordicsemi.com/Products/Development-hardware/nrf9160-dk)
- [nRF52840 Development Kit](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk)

## Setup
1. Install zephyr dependencies as described by the [Zephyr docs](https://docs.zephyrproject.org/2.6.0/getting_started/index.html#install-dependencies). For boards from Nordic Semiconductor you will also require [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools) for flashing the project.

2. Install a toolchain as described by the [Zephyr docs](https://docs.zephyrproject.org/2.6.0/getting_started/index.html#install-a-toolchain).

3. Install project by running the following commands from your project directory. This will download the project and any dependencies.
```sh
west init -m https://github.com/Riphiphipzephyr-uart-driver-example
west update
```

4. Install Zephyr Python dependencies as described by the [Zephyr docs](https://docs.zephyrproject.org/2.6.0/getting_started/index.html#install-dependencies).

5. Build the project by entering the directory `./uart-driver-example` and running
```sh
west build --board {BOARD}
```