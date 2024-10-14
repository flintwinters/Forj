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
  | CTOParen
  | CTCParen
  | CTIn
  | CTIgnore

Eq CalculatorTokenKind where
  (==) CTNum CTNum = True
  (==) CTStr CTStr = True
  (==) CTPlus CTPlus = True
  (==) CTMinus CTMinus = True
  (==) CTMultiply CTMultiply = True
  (==) CTDivide CTDivide = True
  (==) CTOParen CTOParen = True
  (==) CTCParen CTCParen = True
  (==) CTIn CTIn = True
  (==) _ _ = False

Show CalculatorTokenKind where
  show CTNum = "C#"
  show CTStr = "C\""
  show CTPlus = "C+"
  show CTMinus = "C-"
  show CTMultiply = "C*"
  show CTDivide = "C/"
  show CTOParen = "C("
  show CTCParen = "C)"
  show CTIn = "C:"
  show CTIgnore = "C\\s"

CalculatorToken : Type
CalculatorToken = Token CalculatorTokenKind
BoundedTok = WithBounds CalculatorToken

Show CalculatorToken where
    show (Tok kind text) = "Tok kind: " ++ show kind ++ " text: " ++ text

TokenKind CalculatorTokenKind where
<<<<<<< HEAD
  TokType CTNum = Integer
  TokType CTStr = String
=======
  TokType CTNum = Double
  TokType CTStr = Double
>>>>>>> main
  TokType _ = ()

  tokValue CTNum s = cast s
  tokValue CTStr s = cast s
  tokValue CTPlus _ = ()
  tokValue CTMinus _ = ()
  tokValue CTMultiply _ = ()
  tokValue CTDivide _ = ()
<<<<<<< HEAD
=======
  tokValue CTOParen _ = ()
  tokValue CTCParen _ = ()
>>>>>>> main
  tokValue CTIn _ = ()
  tokValue CTIgnore _ = ()

ignored : BoundedTok -> Bool
ignored (MkBounded (Tok CTIgnore _) _ _) = True
ignored _ = False

<<<<<<< HEAD
=======
-- someUntil : (stopBefore : Recognise c) -> (l : Lexer) -> Lexer
-- someUntil stopBefore l = some (reject stopBefore <+> l)

>>>>>>> main
someThen : (stopAfter : Recognise c) -> (l : Lexer) -> Lexer
someThen stopAfter l = someUntil stopAfter l <+> stopAfter

bookend : (q: Lexer) -> (l : Lexer) -> Lexer
bookend q l = q <+> someUntil q l

calculatorTokenMap : TokenMap CalculatorToken
calculatorTokenMap = toTokenMap [
  (spaces, CTIgnore),
<<<<<<< HEAD
  (digits, CTNum),
  (quote (exact "\"") alpha, CTStr),
  (exact ":", CTIn),
=======
  (alphas, CTNum),
  (digits, CTStr),
  (exact ":::", CTIn),
  -- (bookend (non spaces) (exact ":"), CTIn),
>>>>>>> main
  (exact "+", CTPlus),
  (exact "-", CTMinus),
  (exact "*", CTMultiply),
  (exact "/", CTDivide)
]

<<<<<<< HEAD
process : List BoundedTok -> Either (Bounds,String) (List BoundedTok)
process [] = Right []
process (MkBounded (Tok CTIgnore _) _ _ ::  MkBounded (Tok CTIn _) _ bnd :: xs) = Left (bnd, "space before :")
process (MkBounded (Tok CTIn _) _ bnd ::  MkBounded (Tok CTIgnore _) _ _ :: xs) = Left (bnd, "space after :")
process (MkBounded (Tok CTIgnore _) _ _ :: xs) = process xs
process (x :: xs) = (x ::) <$> process xs
=======
(:::): TokType CTNum -> TokType CTNum -> TokType CTNum
(:::) a b = b

lexCalculator : String -> Maybe (List (WithBounds CalculatorToken))
lexCalculator str = 
  case lex calculatorTokenMap str of
    (tokens, _, _, "") => Just tokens
    _ => Nothing
>>>>>>> main

lexCalculator : String -> Either (Bounds,String) (List BoundedTok)
lexCalculator = process . fst . lex calculatorTokenMap

<<<<<<< HEAD
scope: TokType CTNum -> TokType CTNum -> TokType CTNum
scope a b = b
=======
  expr : Grammar state CalculatorToken True Double
  expr = buildExpressionParser [
    [ Infix ((*) <$ match CTMultiply) AssocLeft
    , Infix ((/) <$ match CTDivide) AssocLeft
    ],
    [ Infix ((+) <$ match CTPlus) AssocLeft
    , Infix ((-) <$ match CTMinus) AssocLeft
    , Infix ((:::) <$ match CTIn) AssocLeft
    ]
  ] term
>>>>>>> main

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
<<<<<<< HEAD
        printLn $ parse1 "4:5"
        printLn $ parse1 "4 : 5"
        printLn $ parse1 "4 + 5"
        printLn $ parse1 "1 \"okay\" 2"
        printLn $ parse1 ""
=======
        printLn $ parse1 "4:::5"
        printLn $ parse1 "ok"
        printLn $ parse1 ""
>>>>>>> main
