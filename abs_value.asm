ldc var
ldnl 0
brlz is_neg
br end

is_neg:
ldc 1
shl         ; A := B << A (A becomes -30)
sub         ; A := B - A  (A becomes 15)


ldc var     ; B = 15 (our positive result), A = address of var
stnl 0      ; memory[A+0] := B [cite: 55]

end:
HALT

var: data -15