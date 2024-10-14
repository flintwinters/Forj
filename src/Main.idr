-- https://idris2.readthedocs.io/en/latest/cookbook/parsing.html

module Main
import Data.Vect
import Derive.Prelude
import Text.Parse.Manual
import Data.Fin
import Forj

%default total
%language ElabReflection

-- https://idris2.readthedocs.io/en/latest/tutorial/interp.html

data Ty = TyInt | TyBool | TyString | TyFun Ty Ty
InterpTy : Ty -> Type
InterpTy TyInt       = Int64
InterpTy TyBool      = Bool
InterpTy TyString    = String
InterpTy (TyFun a t) = InterpTy a -> InterpTy t

%runElab derive "Ty" [Show,Eq]

Context: (n: Nat) -> Type
Context n = Vect n Ty

Scopes = List (String, (t: Ty ** InterpTy t))

showITy: {t: Ty} -> InterpTy t -> String
showITy {t = TyInt} x = show x
showITy {t = TyBool} x = show x
showITy {t = TyString} x = show x
showITy {t = TyFun a b} x = "TyFun"


data HasType : (i : Fin n)
                -> Scopes
                -> Context n
                -> Ty
                -> Type
                where
    Stop : HasType FZ s (t :: context) t
    Pop  : HasType k s context t -> HasType (FS k) s (u :: context) t

data Expr : Scopes -> Context n -> Ty -> Type where
    Var : HasType i s context t -> Expr s context t
    Val : (x : Int64) -> Expr s context TyInt
    Str : (str : String) -> Expr s context TyString
    Lam : Expr s (a :: context) t -> Expr s context (TyFun a t)
    App : Expr s context (TyFun a t) -> Expr s context a -> Expr s context t
    Op  : (InterpTy a -> InterpTy b -> InterpTy c) ->
        Expr s context a -> Expr s context b -> Expr s context c
    If  : Expr s context TyBool ->
        Lazy (Expr s context a) ->
        Lazy (Expr s context a) -> Expr s context a

data Env : Context n -> Type where
    Nil  : Env Nil
    (::) : InterpTy a -> Env context -> Env (a :: context)

lookup : HasType i s context t -> Env context -> InterpTy t
lookup Stop    (x :: xs) = x
lookup (Pop k) (x :: xs) = lookup k xs

namelookup : String -> Scopes -> Either String Ty
namelookup str [] = Left "Error not found"
namelookup str ((a, (b ** bb)) :: xs) = 
    if a == str
    then Left (show b ++ showITy bb)
    else namelookup str xs

interp : Env context -> Expr s context t -> InterpTy t
interp env (Var i)     = lookup i env
interp env (Val x)     = x
interp env (Str x)     = x
interp env (Lam sc)    = \x => interp (x :: env) sc
interp env (App f s)   = interp env f (interp env s)
interp env (Op op x y) = op (interp env x) (interp env y)
interp env (If x t e)  = if interp env x then interp env t
                                        else interp env e
add : Expr s context (TyFun TyInt (TyFun TyInt TyInt))
add = Lam (Lam (Op (+) (Var Stop) (Var (Pop Stop))))
sub : Expr s context (TyFun TyInt (TyFun TyInt TyInt))
sub = Lam (Lam (Op (-) (Var Stop) (Var (Pop Stop))))

partial
fact : Expr s context (TyFun TyInt TyInt)
fact = Lam (If (Op (<=) (Var Stop) (Val 2))
            (Val 2)
            (Op (*) (App fact (
                        App (App sub (Val 1)) (Var Stop)
                    ))
                    (Var Stop)))

A = namelookup "global" [("global", (TyInt ** 1)), ("local", (TyString ** "ok"))]

sayhi : Expr s context (TyString)
sayhi = Str "hello!"

partial
main : IO ()
main = do 
          printLn (interp [] fact {s = [("global", (TyInt ** 1))]} 4)
          printLn $ namelookup "global" [("global", (TyInt ** 1)), ("local", (TyString ** "ok"))]
          printLn $ namelookup "local" [("global", (TyInt ** 1)), ("local", (TyString ** "ok"))]
