# Forj

## Overview

Forj is a programming language designed for expressive metaprogramming and low-level control.

Some sort of graphics like Raylib or SDL will be exposed from C to Forj eventually, for making games and GUI.

I would like to collaborate on this project with others.  Join the discord here [https://discord.gg/9dVuQs6wYg](https://discord.gg/9dVuQs6wYg).

It is most similar to [Joy](https://github.com/Wodan58/Joy) and Lisp.

Since static types are a compile-time construct, why not let programmers define custom types by asserting properties of their program at compile-time with an ordinary function?

Forj seeks to do just that.

## Reversibility

The stack based structure will be leveraged to achieve efficient reversible debugging in the near future.

## AI code generation

Forj's metaprogrammatic typechecking will facilitate adversarial training of LLMs for Forj, by pitting two competing models against each other (One which writes code, and another which writes typechecking code to assert that the first model is correct).

## Code examples

### Assert int list
```scala
:allints @. [. int map.. and reduce.. .. ].
@. [. 1 2 3 :str ]. allints.
@. [. 1 2 3 1 ]. allints.
```
Checks whether an object is entirely of type int.
```scala
1
@ 1 2 3 1
0
@ 1 2 3 str
```

### TODO

- put together a simple cli repl ide system. <- can vibe code this skeleton easily
  - write it in C then expose the ncurses behavior so it can be ported to forj simply.
- move the testing utility over to forj itself.
- some decisions about syntax still need to be made (all sort of inter-related)
  - need some more control over variable scan behavior
  - `:varname :.` behavior is a bit odd but will likely stay.
  - find some room in the symbolspace for a different comment style other than ().  I would like to be able to use () in other places.
  - maybe splitting `:varname` and `"varname"` into two different types of strings
