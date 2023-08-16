/**
 * @file hid_app.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

/// @brief Task of HID App. Must be called frequently
void hid_app_task();

/// @brief Must be called once before calling \ref hid_app_task
void hid_app_init();
