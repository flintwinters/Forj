-- https://idris2.readthedocs.io/en/latest/tutorial/interp.html
-- https://idris2.readthedocs.io/en/latest/cookbook/parsing.html

module Forj
import Data.Vect
import Data.Fin

record T a where
    constructor MkT
    name : String
    size : Integer
    t : Type
    f : t -> a

data Stack: Nat -> Type -> Type where
    Base  : Stack Z a
    (::)  : a -> Stack k a -> Stack (S k) a

Peek : Stack (S n) t -> t
Peek (s::_) = s

Pull : Stack (S n) t -> Stack n t
Pull (_::ss) = ss

Push : t -> Stack n t -> Stack (S n) t
Push s ss = (s::ss)

-- Can we type Scope so that only successors can be appended?
Tag = Nat
data Scope = Root | Branch Tag (String -> Scope {-Replace with Dict type-}) Scope

Global = Root
basic: String -> Scope
basic s = Global
Br = Branch (0) (basic) Global