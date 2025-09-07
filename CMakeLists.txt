cmake_minimum_required(VERSION 3.28)
project(OSDev C ASM)

# --- Compilers
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)

# --- Build as freestanding kernel (no host libc, no PIE/PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

add_compile_options(
        --target=x86_64-elf
        -ffreestanding -fno-stack-protector
        -fno-pic -fno-pie -fno-plt
        -m64 -mno-red-zone
        -O2 -Wall -Wextra
)

# Use lld and force a static, non-PIE link. Also point at our linker script.
add_link_options(
        -fuse-ld=lld
        -nostdlib -nodefaultlibs -nostartfiles
        -static -no-pie
        -Wl,-T,${CMAKE_SOURCE_DIR}/linker.ld
        -Wl,--build-id=none
        -Wl,--no-dynamic-linker
        -Wl,-z,max-page-size=0x1000
)

set(SRCS
        src/boot.S
        src/gdt.S
        src/kernel.c
        src/idt.c
        src/isr.S
        src/pic.c
        src/pit.c
        src/kbd.c
        src/irq.c
)

add_executable(kernel.elf ${SRCS})

# --- ISO packaging (unchanged)
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/myos.iso
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/iso/boot/grub
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/iso/boot/grub/grub.cfg ${CMAKE_BINARY_DIR}/iso/boot/grub/grub.cfg
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${CMAKE_BINARY_DIR}/iso/boot/kernel.elf
        COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/myos.iso ${CMAKE_BINARY_DIR}/iso
        DEPENDS kernel.elf ${CMAKE_SOURCE_DIR}/iso/boot/grub/grub.cfg
        VERBATIM
)
add_custom_target(iso ALL DEPENDS ${CMAKE_BINARY_DIR}/myos.iso)

add_custom_target(run_qemu
        COMMAND qemu-system-x86_64 -cdrom ${CMAKE_BINARY_DIR}/myos.iso -m 256M -serial stdio -no-reboot -no-shutdown
        DEPENDS iso
        USES_TERMINAL
)
