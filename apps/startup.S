  .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb


/* start address for the initialization values of the .data section. 
defined in linker script */
.word	_sidata
/* start address for the .data section. defined in linker script */  
.word	_sdata
/* end address for the .data section. defined in linker script */
.word	_edata
/* start address for the .bss section. defined in linker script */
.word	_sbss
/* end address for the .bss section. defined in linker script */
.word	_ebss

    .section	.text.InitApp_Handler
	.weak	InitApp_Handler
	.type	InitApp_Handler, %function
InitApp_Handler:	

/* Copy the data segment initializers from flash to SRAM */  
  movs	r1, #0
  b	LoopCopyDataInit

CopyDataInit:
	ldr	r3, =_sappidata
	ldr	r3, [r3, r1]
	str	r3, [r0, r1]
	adds	r1, r1, #4
    
LoopCopyDataInit:
	ldr	r0, =_sappdata
	ldr	r3, =_eappdata
	adds	r2, r0, r1
	cmp	r2, r3
	bcc	CopyDataInit
	ldr	r2, =_sappbss
	b	LoopFillZerobss
/* Zero fill the bss segment. */  
FillZerobss:
	movs	r3, #0
	str	r3, [r2], #4
    
LoopFillZerobss:
	ldr	r3, = _eappbss
	cmp	r2, r3
	bcc	FillZerobss
	bx	lr    
.size	InitApp_Handler, .-InitApp_Handler

// jump table for user module
 	.section	.module_exports,"a",%progbits
	.type	g_pfnExports, %object
	.size	g_pfnExports, .-g_pfnExports
        
g_pfnExports:
	.word	InitApp_Handler
	.word	foo
        .word   0
   

