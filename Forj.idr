-- https://idris2.readthedocs.io/en/latest/tutorial/interp.html
-- https://idris2.readthedocs.io/en/latest/cookbook/parsing.html

module Forj
import Data.Vect
import Data.Fin

public export 
data Stack: Nat -> Type -> Type where
    Base  : Stack Z a
    (::)  : a -> Stack k a -> Stack (S k) a

Path = List Nat
(<?): Path -> Path -> Bool
infixr 10 <?
Nil <? Nil = True
_   <? Nil = True
Nil <? _   = False
(c::cs) <? (p::ps) = (c == p) && (cs <? ps)

public export
data Node : (P: Path) -> Type where
    Root  : String -> Node Nil
    -- needs to be changed to List (Node ps), along with an assertion that
    -- P <? ps will be true
    Branch: String -> Node ps -> List (Node ps) -> Node (p::ps)

public export
Show (Node p) where
    show (Root s) = s
    show (Branch s a b) = s ++ " - " ++ show b

Peek : Stack (S n) t -> t
Peek (s::_) = s

Pull : Stack (S n) t -> Stack n t
Pull (_::ss) = ss

Push : t -> Stack n t -> Stack (S n) t
Push s ss = (s::ss)
