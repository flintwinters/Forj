module Asm

import Derive.Prelude

Machine = List (Nat, Int64)

Addr : Nat -> Machine -> Maybe Int64
Addr k Nil = Nothing
Addr k ((a, b)::xs) = if k == a then Just b else Addr k xs

data Regs = Zz | Ra | Sp | Gp |
            Tp | T0 | T1 | T2 |
            S0 | S1 | A0 | A1 |
            A2 | A3 | A4 | A5 |
            A6 | A7 | S2 | S3 |
            S4 | S5 | S6 | S7 |
            S8 | S9 | Sa | Sb |
            T3 | T4 | T5 | T6

record Instruction where
    constructor MkI
    name: String
    argc: Nat
    func: Vect argc Regs -> Machine -> Machine

Mfunc: Vect argc Regs -> Machine -> Machine
Mfunc _ m = m

addi: Vect 3 Regs -> Machine -> Machine
addi [a, b, c] m = m

Addi: Instruction
Addi = MkI "li" 3 addi
