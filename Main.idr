-- https://idris2.readthedocs.io/en/latest/cookbook/parsing.html

module Main
import Data.Vect
import Data.Fin

%default total

-- https://idris2.readthedocs.io/en/latest/tutorial/interp.html
data Ty = TyInt | TyBool | TyFun Ty Ty | TyFun2 Ty Ty Ty | TyFunn (Vect n Ty) Ty
InterpTy : Ty -> Type
InterpTy TyInt       = Integer
InterpTy TyBool      = Bool
InterpTy (TyFun a t) = InterpTy a -> InterpTy t
InterpTy (TyFun2 a b t) = InterpTy a -> InterpTy b -> InterpTy t
InterpTy (TyFunn Nil t) = InterpTy t
InterpTy (TyFunn (a::as) t) = InterpTy a -> InterpTy (TyFunn as t) -> InterpTy t 

data HasType : (i : Fin n) -> Vect n Ty -> Ty -> Type where
    Stop : HasType FZ (t :: context) t
    Pop  : HasType k context t -> HasType (FS k) (u :: context) t

data Expr : Vect n Ty -> Ty -> Type where
    Var : HasType i context t -> Expr context t
    Val : (x : Integer) -> Expr context TyInt
    Lam : Expr (a :: context) t -> Expr context (TyFun a t)
    Lam2: Expr (a :: b :: context) t -> Expr context (TyFun2 a b t)
    Lamn: {a : _} -> Expr (a ++ context) t -> Expr context (TyFunn a t)
    App : Expr context (TyFun a t) -> Expr context a -> Expr context t
    Op  : ((a: Ty) -> (b: Ty) -> (c: Ty)) ->
          Expr context a -> Expr context b -> Expr context c
    If  : Expr context TyBool ->
        Lazy (Expr context a) ->
        Lazy (Expr context a) -> Expr context a

data Stk : Vect n Ty -> Type where
    Nil   : Stk Nil
    Push  : (a: Ty) -> Stk context -> Stk (a :: context)
    Pushn :   Stk a -> Stk context -> Stk (a ++ context)

-- lookup : HasType i context t -> Stk context -> InterpTy t
-- lookup Stop    (Push x xs)            = x
-- lookup (Pop k) (Push x xs)            = lookup k xs
-- lookup k       (Pushn Nil xs)         = lookup k xs
-- lookup Stop    (Pushn (Push a as) xs) = a
-- lookup (Pop k) (Pushn (Push a as) xs) = lookup k (Pushn as xs)
-- lookup _       _ = ?hol

lookup : HasType i context t -> (context: Vect n Ty) -> (t: Ty)
lookup Stop    (x::xs)       = x
lookup (Pop k) (x::xs)       = lookup k xs
lookup Stop    ((a::as)++xs) = a
lookup (Pop k) ((a::as)++xs) = lookup k (as++xs)
lookup _       _ = ?hol

interp : (context: Vect n Ty) -> Expr context t -> (t: Ty)
interp stk (Var i)     = lookup i stk
interp stk (Val x)     = x
interp stk (Lam sc)    = \x    => interp (x::stk) sc
interp stk (Lam2 sc)   = \x, y => interp (x::y::stk) sc
-- interp stk (Lamn sc)   = \xs   => interp (Pushn xs stk) sc
interp stk (Lamn sc)   = \x, xs => interp (x::xs++stk) sc -- xs => interp (Pushn xs stk) sc
-- interp stk (Lamn {a = b::c} sc) = \x, xs => interp ((x::xs)++stk) sc -- xs => interp (Pushn xs stk) sc
interp stk (App f s)   = interp stk f (interp stk s)
interp stk (Op op x y) = op (interp stk x) (interp stk y)
interp stk (If x t e)  = if interp stk x then interp stk t
                                        else interp stk e

add : Expr context (TyFun2 TyInt TyInt TyInt)
add = Lam2 (Op (+) (Var Stop) (Var (Pop Stop)))

sub : Expr context (TyFun TyInt (TyFun TyInt TyInt))
sub = Lam (Lam (Op (-) (Var Stop) (Var (Pop Stop))))

partial --<<< remove this
fact : Expr context (TyFun TyInt TyInt)
fact = Lam (If (Op (==) (Var Stop) (Val 0))
            (Val 1)
            (Op (*) (App fact (Op (-) (Var Stop) (Val 1))) (Var Stop)))
-- fact = Lam (If (Op (==) (Var Stop) (Val 0))
--             (Val 1)
--             (Op (*) (App fact (App sub (Var Stop) (Val 1))) (Var Stop)))

-- t: Tree

main : IO ()
main = do putStr "Enter a number: "
          x <- getLine
          printLn (interp [] add (cast x) (cast x + 1))