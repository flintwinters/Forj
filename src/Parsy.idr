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

%runElab derive "Expr" [Show,Eq]

data Op = A | M

record Variable where
  constructor V
  name: String
  val : Nat

%runElab derive "Op" [Show,Eq]
%runElab derive "Variable" [Show,Eq]

data Token : Type where
  TLit : Nat -> Token
  TVar : Variable -> Token
  TSym : Char -> Token
  TOp  : Op -> Token

public export
FromChar Token where
  fromChar '+' = TOp A
  fromChar '*' = TOp M
  fromChar c   = TSym c

%runElab derive "Token" [Show,Eq]

Interpolation Token where
  interpolate (TLit n) = show n
  interpolate (TVar v) = "\{v.name}, \{show v.val}"
  interpolate (TSym s) = show s
  interpolate (TOp  s) = show s

0 Err : Type
Err = ParseError Token Void

tok4 : (cs : List Char) -> LexRes True cs e Token
tok4 ('(' :: xs) = Succ (TSym '(') xs
tok4 (')' :: xs) = Succ (TSym ')') xs
tok4 ('*' :: xs) = Succ (TOp M) xs
tok4 ('+' :: xs) = Succ (TOp A) xs
tok4 (':' :: a :: xs) = Succ (TVar $ V (cast a) 0) xs
tok4 xs          = TLit <$> dec xs

toks4 : String -> Either (Bounded Err) (List $ Bounded Token)
toks4 = (mapFst (map fromVoid)) . (singleLineDropSpaces tok4)

StrRes : String -> Either (Bounded Err) (List $ Bounded Token) -> String
StrRes _ (Right ts) = unlines $ (\(B t bs) => "\{t}: \{bs}") <$> ts
StrRes s (Left b) = uncurry (printParseError s) (virtualFromBounded b)

lex : String -> String
lex s = StrRes s (toks4 s)

opChain : SnocList (Expr,Op) -> Expr -> Expr
opChain [<]           x = x
opChain (sx :< (y,A)) x = opChain sx y + x
opChain (sx :< (y,M)) x = opChain sx (y * x)

0 Rule : Bool -> Type -> Type
Rule b t =
     (xs : List $ Bounded Token)
  -> (0 acc : SuffixAcc xs)
  -> Res b Token xs Void t

ops : Expr -> SnocList (Expr,Op) -> Rule False Expr

applied, expr : Rule True Expr

applied (B (TLit n) _ :: xs) _ = Succ0 (Lit n) xs
applied (B '(' b :: xs) (SA r) = case succT $ expr xs r of
  Succ0 v (B ')' _ :: xs) => Succ0 v xs
  res                     => failInParen b '(' res
applied xs              _ = fail xs

expr xs acc@(SA r) = case applied xs acc of
  Succ0 v ys => succT $ ops v [<] ys r
  Fail0 err  => Fail0 err

ops x sp (B (TOp o) _ :: xs) (SA r) = case succT $ applied xs r of
  Succ0 v ys => succF $ ops v (sp :< (x,o)) ys r
  Fail0 err  => Fail0 err
ops x sp xs _ = Succ0 (opChain sp x) xs

parseExpr : Origin -> String -> Either (FileContext,Err) Expr
parseExpr o str = case toks4 str of
  Left err => Left $ fromBounded o err
  Right ts => result o $ expr ts suffixAcc

parseAndPrint : String -> IO ()
parseAndPrint s =
  putStrLn $ either (uncurry $ printParseError s) show (parseExpr Virtual s)

main : IO ()
main = do
        parseAndPrint "1 + 1"
        parseAndPrint "1+1"
        parseAndPrint "(2 + 2) * 10"
        parseAndPrint "3+1"
        parseAndPrint "(3 + 2"
        parseAndPrint "1 + :b"
        parseAndPrint "1 + :a"
        parseAndPrint "1 + a bc"
