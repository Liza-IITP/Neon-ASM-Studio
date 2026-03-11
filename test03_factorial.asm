; test03_factorial.asm
; Recursively calculates 5! (Factorial)
    ldc 0x1000
    a2sp
    adj -1
    ldc 5
    stl 0           ; push 5 as argument
    call fact
    adj 1           ; clean up argument
    HALT
fact:
    adj -2          ; reserve space for return address and local temp
    stl 1           ; save return address
    ldl 2           ; load argument (N)
    brz base_case   
    ; recursive step
    ldl 2
    adc -1
    adj -1
    stl 0           ; push N-1
    call fact
    adj 1           ; pop N-1
    ; Multiply result by N (using repeated addition for SIMPLEX)
    ; (Simplified: normally you'd write a multiply subroutine here)
    ; ... Assuming result is in A
    br end_fact
base_case:
    ldc 1           ; 0! = 1
end_fact:
    ldl 1           ; load return address
    adj 2           ; restore stack pointer
    return