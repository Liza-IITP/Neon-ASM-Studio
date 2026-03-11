; test04_errors.asm
; This file intentionally contains syntax and logical errors
; to test the assembler's error detection capabilities.

valid_label: 
    ldc 10

valid_label:        ; ERROR: Duplicate label definition
    add

1bad_label:         ; ERROR: Invalid label name (starts with a number)
    ldc 5

    fakeinst 5      ; ERROR: Unknown instruction

    ldc             ; ERROR: Missing operand for an instruction that needs one

    add 5           ; ERROR: Unexpected operand for a zero-operand instruction

    ldc 5 6         ; ERROR: Extra operand on the end of the line

    brz nobody_here ; ERROR: No such label defined anywhere in the file

    HALT