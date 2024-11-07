# Forj

As computing becomes necessary for daily life, its providers attain greater influence over us, making democratization, and commoditization of computing an ethical imperative.

Forj is a language and an OS.
The language is stack based, functional, and has first-class types.
The OS is written in Forj, has a 39 bit virtual address space, and round robin scheduler.

The project mission statement is to "create the world's best documentation system."
Language and operating system integration are essential to achieve that.

## Planned language Features

#### Nested scopes.
`Country:City:Street:House:Room`

##### Partially ordered scopes
`Country :< Room`

#### Explicit function application operator "bang"

`func !`

Everything is a value until `!` is used on it.

The words, `!!`, `!!!`, etc, represent functions that return their predecessor.

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

### Stretch Goals

- Prove some stuff about the system
- Use Forj as the kernel's shell
- Timer and paging interrupts
- Self-host Forj
