# uCnix

![Kernel Build](https://github.com/imbluedabr/uCnix/actions/workflows/c-cpp.yml/badge.svg)

uCnix is a *non* posix compliant toy oprating system for extremely resource constrained microcontrollers.
Currently i am targeting arm/riscv microcontrollers with around 64KiB sram but i also want to suport more powerfull mcu's like the rp2040/rp2350.

## Suported devices
- MCXA153VFM
- LPC55S69 (barely suported)

## Dependencies

- GNU Make
- cross compiler toolchain (for ARM this is arm-none-eabi)

## Building

1. clone and cd to the repo root.
2. build the tool(s): `make tools`
3. build the kernel image: `make image`
4. flash the image onto your microcontroller(see Flashing).
5. profit.

> [!NOTE]
> the build system is still in its early stages so this is subject to change.

## Flashing

To actually install the operating system on a microcontroller you have to flash `image.bin` or `kernel.elf`. Below are flashing instructions per suported microcontroller.

### FRDM-MCXA153

There are 2 ways of flashing the frdm-mcxa153. You can either use `pyocd` or you can use Jlink. `pyocd` is recommended for people new to the board.

#### pyocd
1. install the `pyocd` debugger/programmer.
2. install the mcxa153 pack: `pyocd pack install MCXA153VFM`
3. connect the microcontroller.
4. check the connection, the mcxa153 should show up: `pyocd list`
5. flash the image: `./scripts/pyocd_flash.sh`
6. connect a serial terminal to the usb vcom port.
7. done.

#### Jlink

You need to have already flashed and updated the jlink debugger firmware onto the nxp board, then all you have to do is run `./scripts/jlink_flash.sh`.

