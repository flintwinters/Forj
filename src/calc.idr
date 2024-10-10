record Position where
  constructor P
  i: Nat

record Error where
  constructor E
  p: Position
  e: String

Tracker = Either (Error) (List Nat)
interp: List Char -> Tracker
interp []         = Right []
interp (' ' :: s) = Left $ E (P 0) " "
interp (_   :: s) = interp s