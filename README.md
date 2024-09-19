# Forj

The Forj systems programming language is a simple compiled stack language.

## Planned language Features

#### Nested scopes.
`Country:City:Street:House:Room`

##### Partially ordered scopes
`Country :< Room`

#### Explicit function application operator "bang"

`func !`

Everything is a value until `!` is used on it.

The words, `!!`, `!!!`, etc, represent functions that return their successor.

`3 fact !! !` (6)

`!` is distributive over arrays:

`[sayhi sayhello] !` ("hi" "hello")

But not recursively:

`[sayapple [saybeetroot] saycarambola] !` ("apple" \[saybeetroot\] "carambola")

We need to use `!!`

`[sayapple [saybeetroot] !! saycarambola] !` ("apple" "beetroot" "carambola")

## Project Goals

- Create a self hosting stack based language.
- Write a kernel for RISC-V in the language.
- Build Forj as a syntax wrapper for functions on a stack machine in `Idris`, allowing Forj code to be translated back to Idris for easier verification.

### Stretch Goals

- Prove some stuff about the system
- Use Forj as the kernel's shell
- Timer and paging interrupts
- Self-host Forj