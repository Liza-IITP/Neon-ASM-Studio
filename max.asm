ldc var1
ldnl 0
ldc var2
ldnl 0
sub                 ; A = var1 - var2
brlz var1small      ; If A < 0, var2 is larger. Jump to var1small.

; -- Path 1: var1 is the maximum --
ldc var1
ldnl 0
ldc result
stnl 0
HALT                ; <-- THE FIX: Stop execution here! Don't fall through.

; -- Path 2: var2 is the maximum --
var1small:
ldc var2
ldnl 0
ldc result
stnl 0
HALT                ; Stop execution here too.

; -- Data definitions safely at the bottom --
var1: data 50
var2: data 25
result: data 0