# Main node Walkthrough

This document will guide you through the Main-Node codebase.

See the [README.md](./README.md) file for more information on the Main-Node.

Here is an overview of the Main-Node architecture :

<image src="../images/main-node-architecture.png" />

## `main`

The app_main function initializes each components, logging each step's success or failure. It sequentially initializes devices, BLE, GAP, GATTC, UI, cache, and I2C Master, logging errors and returning early if any initialization fails. Upon successful initialization, it logs completion and attempts to connect to all devices.

## `ble`

The init_ble function initializes the Bluetooth Low Energy (BLE) functionality. It first releases the memory allocated for the classic Bluetooth mode using `esp_bt_controller_mem_release`. Then, it configures the Bluetooth controller with default settings and initializes it. If any of these steps fail, an error message is logged, and the function returns false. After successfully initializing the controller, the function enables the BLE mode, initializes the Bluedroid stack, and enables it. If all these steps succeed, the function returns true, indicating successful initialization.

## `gap`

`gap.c` handle BLE scanning and connection events. The `handle_scan_result()` function processes scan results, checking for specific service UUIDs in the advertisement data. If a matching UUID is found, it stops scanning and initiates a GATT client connection using `open_profile()` (defined in `gattc.c`). This function is called from the `esp_gap_cb()` callback, which handles various BLE events such as scan start, scan stop, and scan results. The callback ensures that scanning is only processed when the `is_scanning` flag is set, promoting efficient resource usage.

## `gattc`

The `profiles` array stores information about connected devices, with each entry representing a device profile. The `notify_descr_uuid` is a constant UUID used for notification descriptors. The `is_profile_active` function checks if a profile at a given index is active, while `next_available_profile_idx` finds the next available profile index.

The `disconnect` function handles the disconnection process, including unregistering the GATT client and clearing the profile data. The `search_service` function initiates a service search on a device, and `subscription_success_handler` handles successful characteristic notification subscriptions.

The `connection_oppened_handler` function processes a successful GATT connection, including setting the MTU. The `get_characteristic_count` function retrieves the count of characteristics for a service, and `discover_characteristics` discovers and subscribes to characteristics of a service.

The `main_service_found_handler` and `battery_service_found_handler` functions handle the discovery of the main service and battery service, respectively. The `gattc_profile_callback` function processes GATT client events for a specific profile, while the `esp_gattc_cb` function is the main callback for the GATT client module, dispatching events to the appropriate profile callback.

The `init_gattc` function initializes the GATT client module, setting up profiles and registering the callback function. The `open_profile` function opens a new profile for a device, registering the GATT client application and allocating resources for the profile.

## `events`

`events.c` code defines several event handler functions for managing device states and interactions in a Bluetooth-enabled system. Each function responds to specific events such as screen state changes, pairing start/stop, device selection, device type changes, device state changes, data reception, and battery level updates. These handlers log events (if logging is enabled), update device states in a cache, and send corresponding messages via I2C to communicate these changes to other system components.

## `ui`

The code in `ui.c` is responsible for managing the user interface of a device, including handling button interactions and device states. It defines several functions to get and set the selected device, toggle the screen state, and manage device connections. The `init_ui` function initializes the user interface by setting up LEDs and configuring buttons for pairing, selecting the next or previous device, and toggling the screen. Each button is associated with specific callback functions to handle short and long presses, ensuring that user interactions are appropriately processed.

For example, the `next_selected_device` and `previous_selected_device` functions update the `selected_device` index and trigger the `on_device_selected` callback to reflect the change. The `start_pairing` function initiates a device scan if not already scanning and disconnects the currently selected device if it is active. The `reconnect_device` function attempts to reconnect to the selected device if it is not already connected. The `toggle_screen` function changes the screen state and calls `on_screen_state_changed` to handle the new state.

## `i2c_messages`

`i2c_messages.c` handles sending and processing I2C messages. The `i2c_send_message` function sends a message without additional data by calling `i2c_send_message_data`, which constructs the message with a header and optional data payload. It ensures the data length does not exceed the maximum allowed length and allocates memory for the message. The message is then populated with the type, device index, and data length, followed by the actual data. Any remaining space is filled with 0xFF before the message is sent via `i2c_write`. Logging is performed if enabled, and the allocated memory is freed afterward.

The `process_message` function processes incoming I2C messages by logging the received data and extracting the message type, device index, and data length. It handles specific message types, such as `msg_req_dev`, by retrieving device information from a cache and sending a response with the device's type, state, and battery level using `i2c_send_message_data`. This structured approach ensures efficient communication and handling of I2C messages within the system.

## `cache`

`cache.c` manages a cache of devices, allowing for retrieval and updating of device information such as type, state, and battery level. The `init_cache` function initializes the cache by setting all devices to an unknown type, disconnected state, and zero battery level. Functions like `get_cache_device`, `set_cache_device_type`, `set_cache_device_state`, and `set_cache_device_battery_level` provide interfaces to access and modify the cache, ensuring that device information is consistently maintained.

## `devices`

The code in `devices.c` manages a collection of devices, providing functions to add, remove, retrieve, and connect to devices. The `add_device` function saves a device's Bluetooth address, address type, and device type to persistent storage, ensuring that existing devices are either updated or overridden if necessary. The `remove_device` function deletes a device's information from storage, while `get_device` retrieves a device's details into a `saved_device_t` structure. These operations rely on generating unique keys for each device based on its index using the `generate_device_key` function.

Additionally, the code includes functions to connect to devices. The `connect_device` function retrieves a device's information and initiates a connection using the `open_profile` function. The `connect_all_devices` function iterates through all possible device indices, connecting to each existing device. The `update_device_bda` function updates a device's Bluetooth address if it has changed. These functions ensure that the system can manage and interact with a dynamic set of devices efficiently, leveraging persistent storage to maintain device information across sessions.

## `devices_config`

The `device_config.h` file defines the configuration and characteristics of various device types used in the system, including their UUIDs for services and characteristics. It categorizes devices into types such as M-node, A-node, and Sleeper, each with specific UUIDs for their services and characteristics. The file also provides structures and constants to manage these configurations, such as `device_type_config_t` for storing UUIDs and characteristic counts, and functions to retrieve device configurations and determine device types from UUIDs.

## `constants`

The `constants.h` file defines various constants used throughout the system.

## Generic files

Theses files should code that could be reused in other projects.

## `button`

`button.c` manages the initialization and state tracking of multiple buttons. The `init_button` function sets up a button by configuring its GPIO pin, setting up callbacks for press and long press events, and allocating memory for storing button states. If this is the first button being initialized, it also creates a FreeRTOS task (`button_task`) to continuously monitor the state of all buttons. The `button_task` function runs in an infinite loop, checking each button's GPIO level to determine if it is pressed or released. It debounces the button press and distinguishes between short and long presses, invoking the appropriate callback functions based on the duration of the press.

The use of FreeRTOS tasks allows for non-blocking, concurrent monitoring of multiple buttons, making the system responsive to user input. The code is designed to be scalable, allowing for the dynamic addition of new buttons and ensuring that memory is managed appropriately through the use of `malloc` and `realloc`. This approach provides a robust foundation for handling button inputs in an embedded system.

## `led`

`led.c` manages the initialization, state control, and blinking behavior of multiple LEDs. It defines arrays to keep track of each LED's state, blinking status, blink counters, and blink rates. The `init_led` function initializes each LED by setting its GPIO pin direction, creating a semaphore for synchronization, and starting a FreeRTOS task (`led_blink_task`) to handle the blinking logic. The `led_blink_task` function toggles the LED state based on the blinking parameters and uses semaphores to manage concurrent access.

Functions like `set_led`, `get_led`, `start_led_blink`, `stop_led_blink`, and `reset_led` provide interfaces to control the LEDs. `set_led` directly sets the LED state, while `start_led_blink` and `stop_led_blink` manage the blinking behavior by adjusting blink counters and rates, and signaling the semaphore to trigger the blinking task. `reset_led` stops any ongoing blinking and resets the LED to an off state. This setup ensures efficient and synchronized control of multiple LEDs, allowing for both static and dynamic (blinking) LED states.

## `preferences`

Parts of code borrowed from Arduino [Preferences](https://github.com/vshymanskyy/Preferences/blob/main/src/Preferences.cpp) library.

## `i2c_master`

`i2c_master.c` initializes and manages I2C communication for an ESP-IDF-based project. The `i2c_init` function sets up the I2C master bus by configuring the clock source, I2C port, SCL and SDA pins, glitch filter, and internal pull-up resistors. It then creates a new I2C master bus and adds a device to this bus with a specified address and clock speed. The `i2c_write` function handles data transmission to the I2C device, sending data through the I2C bus and logging any errors that occur during transmission. This setup ensures reliable I2C communication between the master and slave devices, facilitating data exchange in the system.

## `uuid128`

The code in `uuid128.c` provides utility functions for handling 128-bit UUIDs, commonly used in Bluetooth Low Energy (BLE) applications. The `get_adv_service_uuid` function extracts a 128-bit service UUID from advertisement data. It iterates through the advertisement data, checking for types 0x06 and 0x07, which indicate the presence of 128-bit UUIDs. When found, it copies the UUID into the provided `service_uuid` buffer and returns true. If no such UUID is found, it returns false.

Additionally, the `compare_uuid` function compares two 128-bit UUIDs for equality. It uses the `memcmp` function to perform a byte-by-byte comparison of the two UUIDs, returning true if they are identical and false otherwise. These functions are essential for identifying and verifying BLE services based on their UUIDs, facilitating the discovery and connection process in BLE applications.
