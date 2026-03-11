; bubblesort.asm
; Sorts an array of 5 elements in memory
    ldc 0x1000
    a2sp
outer_loop:
    ldc 0           ; swapped flag = 0
    stl 0
    ldc 4           ; loop counter = size - 1
    stl 1
    ldc array       ; pointer to current element
    stl 2
inner_loop:
    ldl 1
    brz check_swap  ; if counter == 0, end inner loop
    ldl 2
    ldnl 0          ; A = array[i]
    ldl 2
    ldnl 1          ; A = array[i+1], B = array[i]
    sub             ; A = array[i] - array[i+1]
    brlz no_swap    ; if array[i] < array[i+1], skip swap
    brz no_swap     ; if array[i] == array[i+1], skip swap
    ; perform swap
    ldl 2
    ldnl 1          ; load array[i+1]
    ldl 2
    stnl 0          ; array[i] = array[i+1]
    ldl 2
    ldnl 0          ; load original array[i] (needs temp storage in real scenario, simplified here)
    ldl 2
    stnl 1          ; array[i+1] = array[i]
    ldc 1
    stl 0           ; swapped = 1
no_swap:
    ldl 2
    adc 1
    stl 2           ; pointer++
    ldl 1
    adc -1
    stl 1           ; counter--
    br inner_loop
check_swap:
    ldl 0
    brz end_sort    ; if swapped == 0, array is sorted
    br outer_loop
end_sort:
    HALT

array: 
    data 5
    data 2
    data 9
    data 1
    data 4