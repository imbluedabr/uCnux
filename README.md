
# uCnux

uCnux is a toy oprating system for extremely resource constrained microcontrollers.
Currently i am targeting arm/riscv microcontrollers with around 64KiB sram but i also want to suport more powerfull mcu's like the rp2040/rp2350.

## Dependencies

- GNU Make
- cross compilation toolchain (for ARM this is arm-none-eabi)

## Building

1. clone and cd to the repo root.
2. run `make kernel.elf`
3. profit

> [!NOTE]
> the build system is still in its early stages so this is subject to change.


