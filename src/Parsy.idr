-- https://github.com/stefan-hoeck/idris2-parser/blob/main/docs/src/Intro.md
-- rlwrap idris2 Parsy.idr -q -p parser -p contrib -p elab-util
module Parsy

import Data.List
import Derive.Prelude
import Text.Parse.Manual

%default total
%language ElabReflection

public export
data Expr : Type where
  Lit  : Nat -> Expr
  Add  : Expr -> Expr -> Expr
  Mult : Expr -> Expr -> Expr

%runElab derive "Expr" [Show,Eq]

public export %inline
Num Expr where
  fromInteger = Lit . fromInteger
  (+) = Add
  (*) = Mult

public export
eval : Expr -> Nat
eval (Lit k)    = k
eval (Add x y)  = eval x + eval y
eval (Mult x y) = eval x * eval y

test1 : eval (7 + 3 * 12) === 43
test1 = Refl


||| `A`ddition and `M`ultiplication
public export
data Op = A | M

%runElab derive "Op" [Show,Eq]

public export
data Token : Type where
  TLit : Nat -> Token
  TSym : Char -> Token
  TOp  : Op -> Token

%runElab derive "Token" [Show,Eq]

public export
FromChar Token where
  fromChar '+' = TOp A
  fromChar '*' = TOp M
  fromChar c   = TSym c

public export
0 Err : Type
Err = ParseError Token Void

lit :
     SnocList (Bounded Token)
  -> (start,cur : Position)
  -> Nat
  -> List Char
  -> Either (Bounded Err) (List $ Bounded Token)

tok :
     SnocList (Bounded Token)
  -> (cur : Position)
  -> List Char
  -> Either (Bounded Err) (List $ Bounded Token)

lit st s c n []        = Right $ st <>> [bounded (TLit n) s c]
lit st s c n (x :: xs) =
  if isDigit x
     then lit st s (incCol c) (n*10 + digit x) xs
     else tok (st :< bounded (TLit n) s c) c (x::xs)

isSymbol : Char -> Bool
isSymbol '(' = True
isSymbol ')' = True
isSymbol '*' = True
isSymbol '+' = True
isSymbol _   = False

tok st c (x :: xs) =
  if      isSymbol x then tok (st :< oneChar (fromChar x) c) (incCol c) xs
  else if isSpace x then tok st (next x c) xs
  else if isDigit x then lit st c (incCol c) (digit x) xs
  else Left (oneChar (Unknown . Left $ show x) c)
tok st c []        = Right $ st <>> []