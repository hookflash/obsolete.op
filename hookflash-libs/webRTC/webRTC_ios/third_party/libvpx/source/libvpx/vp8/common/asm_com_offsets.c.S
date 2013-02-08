	.section	__TEXT,__text,regular,pure_instructions
	.section	__TEXT,__textcoal_nt,coalesced,pure_instructions
	.section	__TEXT,__const_coal,coalesced
	.section	__TEXT,__picsymbolstub4,symbol_stubs,none,16
	.section	__TEXT,__StaticInit,regular,pure_instructions
	.syntax unified
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.align	2
	.code	16
	.thumb_func	_main
_main:
	movs	r0, #0
	@ InlineAsm Start
	
yv12_buffer_config_y_width EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_y_height EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_y_stride EQU #8
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_uv_width EQU #12
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_uv_height EQU #16
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_uv_stride EQU #20
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_y_buffer EQU #24
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_u_buffer EQU #28
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_v_buffer EQU #32
	@ InlineAsm End
	@ InlineAsm Start
	
yv12_buffer_config_border EQU #40
	@ InlineAsm End
	@ InlineAsm Start
	
VP8BORDERINPIXELS_VAL EQU #32
	@ InlineAsm End
	bx	lr


.subsections_via_symbols
