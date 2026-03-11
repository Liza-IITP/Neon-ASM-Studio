; test01_fibonacci.asm
; Computes the 8th Fibonacci number iteratively
    ldc 0x1000
    a2sp
    ldc 8
    stl 0      ; N = 8
    ldc 0
    stl 1      ; a = 0
    ldc 1
    stl 2      ; b = 1
loop:
    ldl 0
    brz done   ; If N == 0, finish
    ldl 1
    ldl 2
    add
    stl 3      ; temp = a + b
    ldl 2
    stl 1      ; a = b
    ldl 3
    stl 2      ; b = temp
    ldl 0
    adc -1
    stl 0      ; N = N - 1
    br loop
done:
    ldl 1      ; Final result in Accumulator
    HALT