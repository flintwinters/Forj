-- https://idris2.readthedocs.io/en/latest/tutorial/interp.html
-- https://idris2.readthedocs.io/en/latest/cookbook/parsing.html

module Forj
import Data.Vect
import Data.Fin
import Data.List.Quantifiers

export 
data Stack: Nat -> Type -> Type where
    Base  : Stack Z a
    (::)  : a -> Stack k a -> Stack (S k) a

export infixr 10 <?
export
data Tree : Type where
  Node : List Tree -> Tree

data (<?) : Tree -> Tree -> Type where
  Here  : t <? t
  There : Any (<? t) ts -> Node ts <? t

Subtree: Tree -> Tree -> Type
Subtree a b = (b : Tree ** (a <? b))
root = Node []
f : (child: Tree) -> Subtree root child -> Nat
-- t = Node []
-- root = Node [t]
-- s: a -> Subtree a b ->
-- s = (t ** (?hol))
-- s = (t ** (There ?hol1))

Peek : Stack (S n) t -> t
Peek (s::_) = s

Pull : Stack (S n) t -> Stack n t
Pull (_::ss) = ss

Push : t -> Stack n t -> Stack (S n) t
Push s ss = (s::ss)
