# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Application/Startup/startup_stm32l475vgtx.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32l4xx_it.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Application/User/syscalls.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Application/User/sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/Src/es_wifi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/Src/es_wifi_io.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/Src/wifi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/readme.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/system_stm32l4xx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_cortex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_spi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_spi_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart_ex.c"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VGTX_FLASH.ld" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/STM32L475VGTX_FLASH.ld")
