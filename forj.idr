-- https://idris2.readthedocs.io/en/latest/tutorial/interp.html#testing
-- https://idris2.readthedocs.io/en/latest/cookbook/parsing.html

import Data.Vect
import Data.Fin

namespace Forj
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

    -- https://idris2.readthedocs.io/en/latest/tutorial/interfaces.html#extending-interfaces
    -- Can we type Scope so that only successors can be appended?
    Tag = String -- Placeholder
    mutual
        data ScMap = Vect n (List (String, Scope))
        data Scope = Root | Branch Tag ScMap

    -- https://idris2.readthedocs.io/en/latest/tutorial/typesfuns.html#maybe
    SearchLst : String -> List (String, Scope) -> Maybe Scope
    SearchLst _ Nil       = Nothing
    SearchLst str1 ((str2, s) :: xs) =
        if (str1 == str2) then Just s else SearchLst str1 xs
    Hash : String -> (1 n: Nat) -> Fin n
    GetMap : String -> ScMap -> Maybe Scope
    -- GetMap s m = SearchLst s (index (Hash s ?hol) m)
    Fetch : Scope -> Tag -> Maybe Scope
    -- Fetch Root _ = Nothing
    -- Fetch Branch a b s = if ()
    (:<) : Scope -> Scope -> Maybe Bool
    infixl 10 :<
    Root :< Root = Just False
    Root :< _    = Just True
    _    :< Root = Just False
    (Branch a b) :< (Branch c d) =
        if (a == c) then Just False else
        case (Fetch (Branch a b) c) of
            Nothing => case (Fetch (Branch c d) a) of
                Nothing => Nothing
                _ => Just False
            _  => Just True

    

data Ty = TyInt | TyBool | TyFun Ty Ty
interpTy : Ty -> Type
interpTy TyInt       = Integer
interpTy TyBool      = Bool
interpTy (TyFun a t) = interpTy a -> interpTy t

data HasType : (i : Fin n) -> Vect n Ty -> Ty -> Type where
    Stop : HasType FZ (t :: context) t
    Pop  : HasType k context t -> HasType (FS k) (u :: context) t

data Expr : Vect n Ty -> Ty -> Type where
    Var : HasType i context t -> Expr context t
    Val : (x : Integer) -> Expr context TyInt
    Lam : Expr (a :: context) t -> Expr context (TyFun a t)
    App : Expr context (TyFun a t) -> Expr context a -> Expr context t
    Op  : (interpTy a -> interpTy b -> interpTy c) ->
        Expr context a -> Expr context b -> Expr context c
    If  : Expr context TyBool ->
        Lazy (Expr context a) ->
        Lazy (Expr context a) -> Expr context a

data Env : Vect n Ty -> Type where
    Nil  : Env Nil
    (::) : interpTy a -> Env context -> Env (a :: context)

lookup : HasType i context t -> Env context -> interpTy t
lookup Stop    (x :: xs) = x
lookup (Pop k) (x :: xs) = lookup k xs

interp : Env context -> Expr context t -> interpTy t
interp env (Var i)     = lookup i env
interp env (Val x)     = x
interp env (Lam sc)    = \x => interp (x :: env) sc
interp env (App f s)   = interp env f (interp env s)
interp env (Op op x y) = op (interp env x) (interp env y)
interp env (If x t e)  = if interp env x then interp env t
                                        else interp env e
add : Expr context (TyFun TyInt (TyFun TyInt TyInt))
add = Lam (Lam (Op (+) (Var Stop) (Var (Pop Stop))))
sub : Expr context (TyFun TyInt (TyFun TyInt TyInt))
sub = Lam (Lam (Op (-) (Var Stop) (Var (Pop Stop))))

fact : Expr context (TyFun TyInt TyInt)
fact = Lam (If (Op (==) (Var Stop) (Val 0))
            (Val 1)
            (Op (*) (App fact (Op (-) (Var Stop) (Val 1)))
                    (Var Stop)))
main : IO ()
main = do 
        putStr "Enter a number: "
        x <- getLine
        printLn (interp [] fact (cast x))

