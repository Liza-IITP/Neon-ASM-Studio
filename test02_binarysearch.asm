; test02_binarysearch.asm
; Binary search for value '7' in a sorted array
    ldc 0x1000
    a2sp
    ldc 0
    stl 0           ; left = 0
    ldc 5
    stl 1           ; right = 5
    ldc 7
    stl 2           ; target = 7
search_loop:
    ldl 1
    ldl 0
    sub             ; A = left - right
    brlz calculate_mid ; if left < right, continue
    brz calculate_mid  ; if left == right, continue
    br not_found
calculate_mid:
    ldl 0
    ldl 1
    add
    shr             ; mid = (left + right) / 2 (Assuming B has 1 from previous ops, or shift by 1)
    stl 3           ; store mid
    ; Load array[mid]
    ldl 3
    ldc array
    add
    stl 4           ; ptr = array + mid
    ldl 4
    ldnl 0          ; A = array[mid]
    ldl 2
    sub             ; A = target - array[mid]
    brz found       ; if target == array[mid]
    brlz go_left    ; if target < array[mid]
    ; go_right
    ldl 3
    adc 1
    stl 0           ; left = mid + 1
    br search_loop
go_left:
    ldl 3
    adc -1
    stl 1           ; right = mid - 1
    br search_loop
found:
    ldl 3           ; Return index in A
    HALT
not_found:
    ldc -1          ; Return -1 in A
    HALT

array:
    data 1
    data 3
    data 5
    data 7
    data 9
    data 11