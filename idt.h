#ifndef IDT_H
#define IDT_H

void init_idt();

//    0 - Division by zero exception
//    1 - Debug exception
//    2 - Non maskable interrupt
//    3 - Breakpoint exception
//    4 - 'Into detected overflow'
//    5 - Out of bounds exception
//    6 - Invalid opcode exception
//    7 - No coprocessor exception
//    8 - Double fault (pushes an error code)
//    9 - Coprocessor segment overrun
//    10 - Bad TSS (pushes an error code)
//    11 - Segment not present (pushes an error code)
//    12 - Stack fault (pushes an error code)
//    13 - General protection fault (pushes an error code)
//    14 - Page fault (pushes an error code)
//    15 - Unknown interrupt exception
//    16 - Coprocessor fault
//    17 - Alignment check exception
//    18 - Machine check exception
//    19-31 - Reserved

#define DIVISION_BY_ZERO            0
#define DEBUG_EXCEPTION             1
#define NON_MASKABLE_INTERRUPT      2
#define BREAKPOINT_EXCEPTION        3
#define INTO_DETECTED_OVERFLOW      4
#define OUT_OF_BOUNDS_EXCEPTION     5
#define INVALID_OPCODE_EXCEPTION    6
#define NO_COPROCESSOR_EXCEPTION    7
#define DOUBLE_FAULT                8
#define COPROCESSOR_SEGMENT_OVERRUN 9
#define BAD_TSS                     10
#define SEGMENT_NOT_PRESENT         11
#define STACK_FAULT                 12
#define GENERAL_PROTECTION_FAULT    13
#define PAGE_FAULT                  14
#define UNKNOWN_INTERRUPT_EXCEPTION 15
#define COPROCESSOR_FAULT           16
#define ALIGNMENT_CHECK_EXCEPTION   17
#define MACHINE_CHECK_EXCEPTION     18

#endif
