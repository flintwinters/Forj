-- https://github.com/stefan-hoeck/idris2-parser/blob/main/docs/src/Intro.md
-- rlwrap idris2 Parsy.idr -q -p parser -p contrib -p elab-util
module Parsy

import Data.List
import Derive.Prelude
import Text.Parse.Manual

%default total
%language ElabReflection

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
tok4 ('b' :: xs) = Succ (TVar $ V "okay" 0) xs
tok4 xs          = TLit <$> dec xs

toks4 : String -> Either (Bounded Err) (List $ Bounded Token)
toks4 = (mapFst (map fromVoid)) . (singleLineDropSpaces tok4)

StrRes : String -> Either (Bounded Err) (List $ Bounded Token) -> String
StrRes _ (Right ts) = unlines $ (\(B t bs) => "\{t}: \{bs}") <$> ts
StrRes s (Left b) = uncurry (printParseError s) (virtualFromBounded b)

lex : String -> String
lex s = StrRes s (toks4 $ s)

main : IO ()
main = do
        putStrLn $ lex "1 + 1"
        putStrLn $ lex "1+1"
        putStrLn $ lex "(2 + 2) * 10"
        putStrLn $ lex "3+1"
        putStrLn $ lex "(3 + a"
        putStrLn $ lex "1 + b"
        putStrLn $ lex "1 + a bc"
