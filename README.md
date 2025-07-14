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
