if (TARGET tinyusb_device)
    add_executable(shift
            shift.c
            )

    # pull in common dependencies
    target_link_libraries(shift pico_stdlib pico_multicore)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(shift 1)
    pico_enable_stdio_uart(shift 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(shift)

elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
