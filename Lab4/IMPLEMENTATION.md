# Lab 4: BLE accelerometer and significant motion

## What it does

The B-L475E-IOT01A1 STM32 IoT board is a BLE peripheral (GATT server). It reads real X, Y, and Z acceleration from its LSM6DSL sensor. A Raspberry Pi is the BLE central (GATT client): it sets the reporting frequency and receives acceleration notifications. The optional significant-motion feature also sends a notification when the sensor detects movement.

## BLE characteristics

All three characteristics belong to service `a4c00000-7a67-4b5a-9f5b-c3c893bdae40`.

| Characteristic | UUID | Properties | Value |
| --- | --- | --- | --- |
| Acceleration | `a4c00001-7a67-4b5a-9f5b-c3c893bdae40` | Read, Notify | X, Y, Z as three signed 16-bit integers in mg, little-endian (6 bytes) |
| Reporting rate | `a4c00002-7a67-4b5a-9f5b-c3c893bdae40` | Read, Write | Unsigned 16-bit Hz, little-endian (2 bytes); accepts 1–20 Hz |
| Significant motion | `a4c00003-7a67-4b5a-9f5b-c3c893bdae40` | Read, Notify | One-byte event counter; increases for each detected event |

The accelerometer itself runs at 52 Hz and ±2 g. The Pi's rate setting controls how often the STM32 reads and reports acceleration; it does not change the sensor's 52 Hz setting.

## STM32 implementation

The firmware uses FreeRTOS with two tasks:

- `TASK_ACC` reads acceleration at the requested rate. It sends samples to `TASK_BLE` through `accel_queue`.
- `TASK_BLE` handles BLE events and updates the GATT characteristics. It is the only task that sends BLE updates.

For optional significant motion, the LSM6DSL uses its built-in step-based detector with a configured threshold of 3 recognized steps. Its INT1 output is connected to STM32 pin PD11. A GPIO interrupt sets a thread flag for `TASK_ACC`. That task reads the sensor's event register, then places a `motion = 1` message in `accel_queue`. `TASK_BLE` receives the message, increments the motion counter, and updates the motion characteristic. The Pi enables notifications through its CCCD and prints each event. The sensor's internal step debounce can make the effective trigger require more than 3 steps.

Main source files:

- `lab4_ble_acc/Core/Src/lab4_accel.c`: LSM6DSL setup, acceleration reads, and motion detection setup.
- `lab4_ble_acc/Core/Src/main.c`: RTOS tasks, queue, and GPIO interrupt callback.
- `lab4_ble_acc/Core/Src/lab4_ble.c`: GATT service, rate write handling, and notifications.
- `lab4_rpi_client.py`: Pi scan, connection, rate write, and notification display.

## Run and result

Build and flash `lab4_ble_acc` with the STM32 VS Code extension and its CMake toolchain. The serial log should report that the BLE service and significant-motion detection are ready. On the Pi, run:

```bash
sudo /home/eric/Lab3/.venv/bin/python /home/eric/Lab4/lab4_rpi_client.py --rate 5 --duration 90
```

Walk several steps while carrying the board. A successful run shows rate and CCCD readback, changing X/Y/Z values, and `SIGNIFICANT MOTION` messages. In the demonstrated run, the Pi received 151 acceleration notifications and 12 motion notifications.
