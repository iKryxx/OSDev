# OSDev Project

# OSDev Project

A hobby operating system written in **C** and **x86_64 assembly**, built from scratch for educational purposes.  
Currently bootstraps with a Multiboot2-compliant loader, sets up the GDT and IDT, and handles interrupts through the PIC.

---

## Features so far
- Multiboot2 header and bootloader entry
- Basic C runtime environment (freestanding, no libc)
- GDT setup for protected mode
- IDT initialization with ISR stubs
- PIC initialization and IRQ remapping
- Basic interrupt handlers for exceptions and hardware IRQs

---

## Building
This project uses **CMake** with a cross-compiler toolchain targeting `x86_64-elf`.

### Prerequisites
- `x86_64-elf-gcc` cross-compiler
- `nasm` or `clang` (for assembly)
- `CMake`
- `qemu-system-x86_64` (for testing)

### Build steps
```bash
mkdir build
cd build
cmake ..
make
```

---

## Running
use **QEMU** to run the kernel:
```bash
qemu-system-x86_64 -cdrom os.iso
```

---

## Roadmap
- [x] Bootloader and Multiboot2 header
- [x] GDT and IDT setup
- [x] PIC initialization and IRQ remapping
- [ ] Keyboard input driver
- [ ] Basic memory management (paging)
- [ ] System calls
- [ ] User mode