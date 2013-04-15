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
	
vp8_block_coeff EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_zbin EQU #20
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_round EQU #28
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_quant EQU #8
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_quant_fast EQU #12
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_zbin_extra EQU #32
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_zrun_zbin_boost EQU #24
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_quant_shift EQU #16
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_blockd_qcoeff EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_blockd_dequant EQU #12
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_blockd_dqcoeff EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_blockd_eob EQU #20
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_base_src EQU #36
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_src EQU #40
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_src_diff EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_block_src_stride EQU #44
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_blockd_predictor EQU #8
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_lowvalue EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_range EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_count EQU #8
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_pos EQU #12
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_buffer EQU #16
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_buffer_end EQU #20
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_error EQU #24
	@ InlineAsm End
	@ InlineAsm Start
	
tokenextra_token EQU #6
	@ InlineAsm End
	@ InlineAsm Start
	
tokenextra_extra EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
tokenextra_context_tree EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
tokenextra_skip_eob_node EQU #7
	@ InlineAsm End
	@ InlineAsm Start
	
TOKENEXTRA_SZ EQU #8
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_extra_bit_struct_sz EQU #16
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_token_value EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_token_len EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_extra_bit_struct_tree EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_extra_bit_struct_prob EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_extra_bit_struct_len EQU #8
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_extra_bit_struct_base_val EQU #12
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_comp_tplist EQU #163652
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_comp_common EQU #103296
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_comp_bc EQU #109232
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_writer_sz EQU #36
	@ InlineAsm End
	@ InlineAsm Start
	
tokenlist_start EQU #0
	@ InlineAsm End
	@ InlineAsm Start
	
tokenlist_stop EQU #4
	@ InlineAsm End
	@ InlineAsm Start
	
TOKENLIST_SZ EQU #8
	@ InlineAsm End
	@ InlineAsm Start
	
vp8_common_mb_rows EQU #2124
	@ InlineAsm End
	bx	lr


.subsections_via_symbols
