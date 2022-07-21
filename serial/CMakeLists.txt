if (TARGET tinyusb_device)
    add_executable(serial
            serial.c
            )

    # pull in common dependencies
    target_link_libraries(serial pico_stdlib pico_multicore)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(serial 1)
    pico_enable_stdio_uart(serial 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(serial)

elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
