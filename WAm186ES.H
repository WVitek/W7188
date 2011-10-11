// File WAM186ES.H - Am188ES / Am186ES microcontroller definitions
#ifndef __WAM186ES_H
#define __WAM186ES_H
#pragma once

#define IntVect ((unsigned long far*)0)

#define CTL_OFF  0xFF00 // Ctl reg offset for peripherals

//*** Interrupt Controller Unit
// Registers
#define INT_I0CON   0xFF38 // INT0 control
#define INT_I1CON   0xFF3A // INT1 control
#define INT_I2CON   0xFF3C // INT2 control
#define INT_I3CON   0xFF3E // INT3 control
#define INT_I4CON   0xFF40 // INT4 control
#define INT_DMA0CON 0xFF34 // DMA0 interrupt/INT5 control
#define INT_DMA1CON 0xFF36 // DMA1 interrupt/INT6 control
#define INT_TCUCON  0xFF32 // Timer interrupt control
#define INT_SP0CON  0xFF44 // Serial port 0 interrupt control
#define INT_SP1CON  0xFF42 // Serial port 0 interrupt control
#define INT_INTSTS  0xFF30 // Interrupt status
#define INT_REQST   0xFF2E // (R/O) Interrupt request
#define INT_INSERV  0xFF2C // In-service
#define INT_PRIMSK  0xFF2A // Priority mask
#define INT_IMASK   0xFF28 // Interrupt mask
#define INT_POLLST  0xFF26 // (R/O) Poll status
#define INT_POLL    0xFF24 // (R/O) Poll
#define INT_EOI     0xFF22 // (W/O) End-of-interrupt
#define INT_VEC     0xFF20 // Interrupt vector

// Interrupt types
#define INT_T0INT   0x08   // Timer 0
#define INT_T1INT   0x12   // Timer 1
#define INT_T2INT   0x13   // Timer 2
#define INT_TMREOI  0x08   // Timer EOI

#define INT_DMA0INT 0x0A
#define INT_DMA1INT 0x0B

#define INT_I0INT   0x0C
#define INT_I1INT   0x0D
#define INT_I2INT   0x0E
#define INT_I3INT   0x0F
#define INT_I4INT   0x10
#define INT_I5INT   INT_DMA0INT
#define INT_I6INT   INT_DMA1INT

#define INT_SP0INT  0x14 // Serial Port 0 INterrupt Type
#define INT_SP1INT  0x11 // Serial Port 1 INterrupt Type
#define INT_NONSPEC 0x8000

// Masks
#define INT_CONMSK  0x0080 // Mask interrupt bit (in any int. ctrl reg.)
#define INT_MSKTMR  0x0001
#define INT_MSKRes  0x0002
#define INT_MSKD0   0x0004
#define INT_MSKD1   0x0008
#define INT_MSKI0   0x0010
#define INT_MSKI1   0x0020
#define INT_MSKI2   0x0040
#define INT_MSKI3   0x0080
#define INT_MSKI4   0x0100
#define INT_MSKSP1  0x0200
#define INT_MSKSP0  0x0400

//*** Timer/Counter Unit

#define TMR_T0CNT  0xFF50
#define TMR_T0CMPA 0xFF52
#define TMR_T0CMPB 0xFF54
#define TMR_T0CON  0xFF56

#define TMR_T1CNT  0xFF58
#define TMR_T1CMPA 0xFF5A
#define TMR_T1CMPB 0xFF5C
#define TMR_T1CON  0xFF5E

#define TMR_T2CNT  0xFF60
#define TMR_T2CMPA 0xFF62
#define TMR_T2CON  0xFF66

// Timer Control Register Bits

#define TC_EN    0x8000 // enable timer
#define TC_INH   0x4000 // inhobit (enable writes to EN bit)
#define TC_INT   0x2000 // generate interrupt
#define TC_RIU   0x1000 // compare register in use (0-A, 1-B)
#define TC_MC    0x0020 // maximum count is reached (must be cleared manually)
#define TC_RTG   0x0010 // retrigger
#define TC_P     0x0008 // prescaler (1-use Timer2 prescaler)
#define TC_EXT   0x0004 // external clock
#define TC_ALT   0x0002 // alternate compare register (dual maximum count mode)
#define TC_CONT  0x0001 // continuos mode

//***
#define PIO_MODE_0 0xFF70
#define PIO_DIR_0  0xFF72
#define PIO_DATA_0 0xFF74

#define PIO_MODE_1 0xFF76
#define PIO_DIR_1  0xFF78
#define PIO_DATA_1 0xFF7A

#endif // __AM186ES_H
