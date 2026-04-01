ldc var1
ldnl 0
ldc var2
ldnl 0
add         ; A is now var1 + var2
ldc var3
ldnl 0
add         ; A is now (var1 + var2) + var3
ldc result
stnl 0      ; memory[result] := B (which holds the final sum)

; Data definitions safely out of reach of the PC!
var1: data 10
var2: data 15
var3: data 20
result: data 0