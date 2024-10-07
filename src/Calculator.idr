import Data.List1
import Text.Lexer
import Text.Parser
import Text.Parser.Expression

%default total

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

Show CalculatorToken where
    show (Tok kind text) = "Tok kind: " ++ show kind ++ " text: " ++ text

TokenKind CalculatorTokenKind where
  TokType CTNum = Double
  TokType CTStr = Double
  TokType _ = ()

  tokValue CTNum s = cast s
  tokValue CTStr s = cast s
  tokValue CTPlus _ = ()
  tokValue CTMinus _ = ()
  tokValue CTMultiply _ = ()
  tokValue CTDivide _ = ()
  tokValue CTOParen _ = ()
  tokValue CTCParen _ = ()
  tokValue CTIn _ = ()
  tokValue CTIgnore _ = ()

ignored : WithBounds CalculatorToken -> Bool
ignored (MkBounded (Tok CTIgnore _) _ _) = True
ignored _ = False

-- someUntil : (stopBefore : Recognise c) -> (l : Lexer) -> Lexer
-- someUntil stopBefore l = some (reject stopBefore <+> l)

someThen : (stopAfter : Recognise c) -> (l : Lexer) -> Lexer
someThen stopAfter l = someUntil stopAfter l <+> stopAfter

bookend : (q: Lexer) -> (l : Lexer) -> Lexer
bookend q l = q <+> someUntil q l

calculatorTokenMap : TokenMap CalculatorToken
calculatorTokenMap = toTokenMap [
  (spaces, CTIgnore),
  (alphas, CTNum),
  (digits, CTStr),
  (exact ":::", CTIn),
  -- (bookend (non spaces) (exact ":"), CTIn),
  (exact "+", CTPlus),
  (exact "-", CTMinus),
  (exact "*", CTMultiply),
  (exact "/", CTDivide)
]

(:::): TokType CTNum -> TokType CTNum -> TokType CTNum
(:::) a b = b

lexCalculator : String -> Maybe (List (WithBounds CalculatorToken))
lexCalculator str = 
  case lex calculatorTokenMap str of
    (tokens, _, _, "") => Just tokens
    _ => Nothing

mutual
  term : Grammar state CalculatorToken True Double
  term = do
    num <- match CTNum
    pure num

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

parseCalculator : List (WithBounds CalculatorToken) -> Either String Double
parseCalculator toks =
  case parse expr $ filter (not . ignored) toks of
    Right (l, []) => Right l
    Right e => Left "contains tokens that were not consumed"
    Left e => Left (show e)

parse1 : String -> Either String Double
parse1 x =
  case lexCalculator x of
    Just toks => parseCalculator toks
    Nothing => Left "Failed to lex."

main : IO ()
main = do
        printLn $ parse1 "1"
        printLn $ parse1 "4:::5"
        printLn $ parse1 "ok"
        printLn $ parse1 ""