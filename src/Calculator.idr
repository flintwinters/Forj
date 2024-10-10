import Data.List1
import Derive.Prelude 
import Text.Lexer
import Text.Token
import Text.Parser
import Text.Parser.Core
import Text.Parser.Expression

%default total
%language ElabReflection

data CalculatorTokenKind
  = CTNum
  | CTStr
  | CTPlus
  | CTMinus
  | CTMultiply
  | CTDivide
  | CTIn
  | CTIgnore

%runElab derive "CalculatorTokenKind" [Show,Eq]

CalculatorToken : Type
CalculatorToken = Token CalculatorTokenKind
BoundedTok = WithBounds CalculatorToken

Show CalculatorToken where
    show (Tok kind text) = "Tok kind: " ++ show kind ++ " text: " ++ text

TokenKind CalculatorTokenKind where
  TokType CTNum = Integer
  TokType CTStr = String
  TokType _ = ()

  tokValue CTNum s = cast s
  tokValue CTStr s = cast s
  tokValue CTPlus _ = ()
  tokValue CTMinus _ = ()
  tokValue CTMultiply _ = ()
  tokValue CTDivide _ = ()
  tokValue CTIn _ = ()
  tokValue CTIgnore _ = ()

ignored : BoundedTok -> Bool
ignored (MkBounded (Tok CTIgnore _) _ _) = True
ignored _ = False

someThen : (stopAfter : Recognise c) -> (l : Lexer) -> Lexer
someThen stopAfter l = someUntil stopAfter l <+> stopAfter

bookend : (q: Lexer) -> (l : Lexer) -> Lexer
bookend q l = q <+> someUntil q l

calculatorTokenMap : TokenMap CalculatorToken
calculatorTokenMap = toTokenMap [
  (spaces, CTIgnore),
  (digits, CTNum),
  (quote (exact "\"") alpha, CTStr),
  (exact ":", CTIn),
  (exact "+", CTPlus),
  (exact "-", CTMinus),
  (exact "*", CTMultiply),
  (exact "/", CTDivide)
]

process : List BoundedTok -> Either (Bounds,String) (List BoundedTok)
process [] = Right []
process (MkBounded (Tok CTIgnore _) _ _ ::  MkBounded (Tok CTIn _) _ bnd :: xs) = Left (bnd, "space before :")
process (MkBounded (Tok CTIn _) _ bnd ::  MkBounded (Tok CTIgnore _) _ _ :: xs) = Left (bnd, "space after :")
process (MkBounded (Tok CTIgnore _) _ _ :: xs) = process xs
process (x :: xs) = (x ::) <$> process xs

lexCalculator : String -> Either (Bounds,String) (List BoundedTok)
lexCalculator = process . fst . lex calculatorTokenMap

scope: TokType CTNum -> TokType CTNum -> TokType CTNum
scope a b = b

term : Grammar state CalculatorToken True Integer
term = do
  num <- match CTNum
  pure num

expr : Grammar state CalculatorToken True Integer
expr = buildExpressionParser [
  [ Infix ((*) <$ match CTMultiply) AssocLeft
  , Infix ((div) <$ match CTDivide) AssocLeft
  ],
  [ Infix ((+) <$ match CTPlus) AssocLeft
  , Infix ((-) <$ match CTMinus) AssocLeft
  , Infix ((\a, b => b) <$ match CTIn) AssocLeft
  , Infix ((\a, b => a) <$ match CTStr) AssocLeft
  ]
] term

parseCalculator : List (BoundedTok) -> Either String Integer
parseCalculator toks =
  case parse expr $ filter (not . ignored) toks of
    Right (l, []) => Right l
    Right e => Left "contains tokens that were not consumed"
    Left e => Left (show e)

parse1 : String -> Either String Integer
parse1 x =
  case lexCalculator x of
    Right toks => parseCalculator toks
    Left (b, s) => Left ((show b) ++ " " ++ s)

main : IO ()
main = do
        printLn $ parse1 "1"
        printLn $ parse1 "4:5"
        printLn $ parse1 "4 : 5"
        printLn $ parse1 "4 + 5"
        printLn $ parse1 "1 \"okay\" 2"
        printLn $ parse1 ""
