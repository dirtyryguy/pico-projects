add_executable(stepper
        stepper.c
        )

# pull in common dependencies
target_link_libraries(stepper pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(stepper)

# enable usb output, disable uart output
pico_enable_stdio_usb(stepper 1)
pico_enable_stdio_uart(stepper 0)
