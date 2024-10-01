# RV64 platform

A simple OS written in Forj will run here

## Planned OS Features

FUI - Forj user interface.

Timer interrupts and threads.

Paging interrupts and virtual memory.

## Project Goals

- Create a self hosting stack based language.
- Write a kernel for RISC-V in the language.
- Build Forj as a syntax wrapper for functions on a stack machine in `Idris`, allowing Forj code to be translated back to Idris for easier verification.

### Stretch Goals

- Prove some stuff about the system
- Use Forj as the kernel's shell
- Timer and paging interrupts
- Self-host Forj
