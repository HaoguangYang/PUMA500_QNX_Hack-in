/*
 * $QNXLicenseC: 
 * Copyright 2013, QNX Software Systems.
 * Copyright 2013, Adeneo Embedded.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $
 */

#include "dma.h"

/****************************************************************************/
/**
 * Construction function for DMAEND instruction. This function fills the program
 * buffer with the constructed instruction.
 *
 * @param	dma_prog the DMA program buffer, it's the starting address for
 *		the instruction being constructed
 *
 * @return 	The number of bytes for this instruction which is 1.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMAEND(char *dma_prog)
{
	/*
	 * DMAEND encoding:
	 * 7 6 5 4 3 2 1 0
	 * 0 0 0 0 0 0 0 0
	 */
	*dma_prog = 0x0;

	return 1;
}

/****************************************************************************/
/**
 *
 * Construction function for DMAGO instruction. This function fills the program
 * buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 * @param	cn is the channel number, 0 - 7
 * @param	imm is 32-bit immediate number written to the channel Program
 *		Counter.
 * @param	ns is Non-secure flag. If ns is 1, the DMA channel operates in
 *		the Non-secure state. If ns is 0, the execution depends on the
 *		security state of the DMA manager:
 *		DMA manager is in the Secure state, DMA channel operates in the
 *		Secure state.
 *		DMA manager is in the Non-secure state, DMAC aborts.
 *
 * @return	The number of bytes for this instruction which is 6.
 *
 * @note		None
 *
 *****************************************************************************/
inline int xzynq_instr_DMAGO(char *dma_prog, unsigned int cn, uint32_t imm,
		unsigned int ns)
{
	/*
	 * DMAGO encoding:
	 * 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
	 *  0  0  0  0  0 |cn[2:0]| 1  0  1  0  0  0 ns  0
	 *
	 * 47 ... 16
	 *  imm[32:0]
	 */
	*dma_prog = 0xA0 | ((ns << 1) & 0x02);
	*(dma_prog + 1) = (uint8_t) (cn & 0x07);
	memcpy(dma_prog + 2, (char *) &imm, 4);

	/* success */
	return 6;
}

/****************************************************************************/
/**
 *
 * Construction function for DMALD instruction. This function fills the program
 * buffer with the constructed instruction.
 *
 * @param	dma_prog the DMA program buffer, it's the starting address for the
 *		instruction being constructed
 *
 * @return 	The number of bytes for this instruction which is 1.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMALD(char *dma_prog)
{
	/*
	 * DMALD encoding
	 * 7 6 5 4 3 2 1  0
	 * 0 0 0 0 0 1 bs x
	 *
	 * Note: this driver doesn't support conditional load or store,
	 * so the bs bit is 0 and x bit is 0.
	 */
	*dma_prog = 0x04;
	return 1;
}

/****************************************************************************/
/**
 *
 * Construction function for DMALP instruction. This function fills the program
 * buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 * @param	Lc is the Loop counter register, can either be 0 or 1.
 * @param	loop_iterations: the number of interations, LoopInterations - 1
 *		will be encoded in the DMALP instruction.
 *
 * @return 	The number of bytes for this instruction which is 2.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMALP(char *dma_prog, unsigned Lc,
		unsigned loop_iterations)
{
	/*
	 * DMALP encoding
	 * 15   ...   8 7 6 5 4 3 2 1  0
	 * | iter[7:0] |0 0 1 0 0 0 lc 0
	 */
	*dma_prog = (uint8_t) (0x20 | ((Lc & 1) << 1));
	*(dma_prog + 1) = (uint8_t) (loop_iterations - 1);
	return 2;
}

/****************************************************************************/
/**
 *
 * Construction function for DMALPEND instruction. This function fills the
 * program buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 * @param	body_start is the starting address of the loop body. It is used
 * 		to calculate the bytes of backward jump.
 * @param	Lc is the Loop counter register, can either be 0 or 1.
 *
 * @return 	The number of bytes for this instruction which is 2.
 *
 * @note	None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMALPEND(char *dma_prog, char *body_start, unsigned Lc)
{
	/*
	 * DMALPEND encoding
	 * 15       ...        8 7 6 5 4  3 2  1  0
	 * | backward_jump[7:0] |0 0 1 nf 1 lc bs x
	 *
	 * lc: loop counter
	 * nf is for loop forever. The driver does not support loop forever,
	 * so nf is 1.
	 * The driver does not support conditional LPEND, so bs is 0, x is 0.
	 */
	*dma_prog = 0x38 | ((Lc & 1) << 2);
	*(dma_prog + 1) = (uint8_t) (dma_prog - body_start);

	return 2;
}

/*
 * Register number for the DMAMOV instruction
 */
#define XZYNQ_DMA_MOV_SAR 0x0
#define XZYNQ_DMA_MOV_CCR 0x1
#define XZYNQ_DMA_MOV_DAR 0x2

/****************************************************************************/
/**
 *
 * Construction function for DMAMOV instruction. This function fills the
 * program buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 * @param	Rd is the register id, 0 for SAR, 1 for CCR, and 2 for DAR
 * @param	imm is the 32-bit immediate number
 *
 * @return 	The number of bytes for this instruction which is 6.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMAMOV(char *dma_prog, unsigned Rd, uint32_t imm)
{
	/*
	 * DMAMOV encoding
	 * 15 4 3 2 1 10 ... 8 7 6 5 4 3 2 1 0
	 *  0 0 0 0 0 |rd[2:0]|1 0 1 1 1 1 0 0
	 *
	 * 47 ... 16
	 *  imm[32:0]
	 *
	 * rd: b000 for SAR, b001 CCR, b010 DAR
	 */
	*dma_prog = 0xBC;
	*(dma_prog + 1) = Rd & 0x7;
	memcpy(dma_prog + 2, (char *) &imm, 4);

	return 6;
}

/****************************************************************************/
/**
 *
 * Construction function for DMANOP instruction. This function fills the
 * program buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 * @return 	The number of bytes for this instruction which is 1.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMANOP(char *dma_prog)
{
	/*
	 * DMANOP encoding
	 * 7 6 5 4 3 2 1 0
	 * 0 0 0 1 1 0 0 0
	 */
	*dma_prog = 0x18;
	return 1;
}

/****************************************************************************/
/**
 *
 * Construction function for DMARMB instruction. This function fills the
 * program buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 *
 * @return 	The number of bytes for this instruction which is 1.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMARMB(char *dma_prog)
{
	/*
	 * DMARMB encoding
	 * 7 6 5 4 3 2 1 0
	 * 0 0 0 1 0 0 1 0
	 */
	*dma_prog = 0x12;
	return 1;
}

/****************************************************************************/
/**
 *
 * Construction function for DMASEV instruction. This function fills the
 * program buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 * @param	event_num is the Event number to signal.
 *
 * @return 	The number of bytes for this instruction which is 2.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMASEV(char *dma_prog, unsigned int event_num)
{
	/*
	 * DMASEV encoding
	 * 15 4 3 2 1  10 9 8 7 6 5 4 3 2 1 0
	 * |event[4:0]| 0 0 0 0 0 1 1 0 1 0 0
	 */
	*dma_prog = 0x34;
	*(dma_prog + 1) = (uint8_t) (event_num << 3);

	return 2;
}

/****************************************************************************/
/**
 *
 * Construction function for DMAST instruction. This function fills the
 * program buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 *
 * @return 	The number of bytes for this instruction which is 1.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMAST(char *dma_prog)
{
	/*
	 * DMAST encoding
	 * 7 6 5 4 3 2 1  0
	 * 0 0 0 0 1 0 bs x
	 *
	 * Note: this driver doesn't support conditional load or store,
	 * so the bs bit is 0 and x bit is 0.
	 */
	*dma_prog = 0x08;
	return 1;
}

/****************************************************************************/
/**
 *
 * Construction function for DMAWMB instruction. This function fills the
 * program buffer with the constructed instruction.
 *
 * @param	dma_prog is the DMA program buffer, it's the starting address
 *		for the instruction being constructed
 *
 * @return 	The number of bytes for this instruction which is 1.
 *
 * @note		None.
 *
 *****************************************************************************/
inline int xzynq_instr_DMAWMB(char *dma_prog)
{
	/*
	 * DMAWMB encoding
	 * 7 6 5 4 3 2 1 0
	 * 0 0 0 1 0 0 1 0
	 */
	*dma_prog = 0x13;
	return 1;
}
/****************************************************************************/
/**
 *
 * Conversion function from the endian swap size to the bit encoding of the CCR
 *
 * @param	endian_swap_size is the endian swap size, in terms of bits, it
 *		could be 8, 16, 32, 64, or 128(We are using DMA assembly syntax)
 *
 * @return	The endian swap size bit encoding for the CCR.
 *
 * @note	None.
 *
 *****************************************************************************/
inline unsigned xzynq_to_endian_swap_bits(unsigned int endian_swap_size)
{
	switch (endian_swap_size) {
	case 0:
	case 8:
		return 0;
	case 16:
		return 1;
	case 32:
		return 2;
	case 64:
		return 3;
	case 128:
		return 4;
	default:
		return 0;
	}

}

/****************************************************************************/
/**
 *
 * Conversion function from the burst size to the bit encoding of the CCR
 *
 * @param	BurstSize is the burst size. It's the data width.
 *		In terms of bytes, it could be 1, 2, 4, 8, 16, 32, 64, or 128.
 *		It must be no larger than the bus width.
 *		(We are using DMA assembly syntax.)
 *
 * @note		None.
 *
 *****************************************************************************/
inline unsigned xzynq_to_burst_size_bits(unsigned BurstSize)
{
	switch (BurstSize) {
	case 1:
		return 0;
	case 2:
		return 1;
	case 4:
		return 2;
	case 8:
		return 3;
	case 16:
		return 4;
	case 32:
		return 5;
	case 64:
		return 6;
	case 128:
		return 7;
	default:
		return 0;
	}
}

/****************************************************************************/
/**
 *
 * Conversion function from PL330 bus transfer descriptors to CCR value. All the
 * values passed to the functions are in terms of assembly languages, not in
 * terms of the register bit encoding.
 *
 * @param	chan_ctrl is the Instance of dma_chan_ctrl_t.
 *
 * @return	The 32-bit CCR value.
 *
 * @note		None.
 *
 *****************************************************************************/
uint32_t xzynq_to_ccr_value(dma_chan_ctrl_t *chan_ctrl)
{
	/*
	 * channel Control Register encoding
	 * [31:28] - endian_swap_size
	 * [27:25] - dst_cache_ctrl
	 * [24:22] - dst_prot_ctrl
	 * [21:18] - dst_burst_len
	 * [17:15] - dst_burst_size
	 * [14]    - dst_inc
	 * [13:11] - src_cache_ctrl
	 * [10:8] - src_prot_ctrl
	 * [7:4]  - src_burst_len
	 * [3:1]  - src_burst_size
	 * [0]     - src_inc
	 */

	unsigned es = xzynq_to_endian_swap_bits(chan_ctrl->endian_swap_size);

	unsigned dst_burst_size = xzynq_to_burst_size_bits(chan_ctrl->dst_burst_size);
	unsigned dst_burst_len = (chan_ctrl->dst_burst_len - 1) & 0x0F;
	unsigned dst_cache_ctrl = (chan_ctrl->dst_cache_ctrl & 0x03)
			| ((chan_ctrl->dst_cache_ctrl & 0x08) >> 1);
	unsigned dst_prot_ctrl = chan_ctrl->dst_prot_ctrl & 0x07;
	unsigned dst_inc_bit = chan_ctrl->dst_inc & 1;

	unsigned src_burst_size = xzynq_to_burst_size_bits(chan_ctrl->src_burst_size);
	unsigned src_burst_len = (chan_ctrl->src_burst_len - 1) & 0x0F;
	unsigned src_cache_ctrl = (chan_ctrl->src_cache_ctrl & 0x03)
			| ((chan_ctrl->src_cache_ctrl & 0x08) >> 1);
	unsigned src_prot_ctrl = chan_ctrl->src_prot_ctrl & 0x07;
	unsigned src_inc_bit = chan_ctrl->src_inc & 1;

	uint32_t ccr_value = (es << 28) | (dst_cache_ctrl << 25) | (dst_prot_ctrl
			<< 22) | (dst_burst_len << 18) | (dst_burst_size << 15)
			| (dst_inc_bit << 14) | (src_cache_ctrl << 11) | (src_prot_ctrl
			<< 8) | (src_burst_len << 4) | (src_burst_size << 1)
			| (src_inc_bit);

	LOG("CCR: es %x\r\n", es);
	LOG("CCR: dca %x, dpr %x, dbl %x, dbs %x, di %x\r\n",
			dst_cache_ctrl, dst_prot_ctrl, dst_burst_len, dst_burst_size,
			dst_inc_bit);
	LOG("CCR: sca %x, spr %x, sbl %x, sbs %x, si %x\r\n",
			src_cache_ctrl, src_prot_ctrl, src_burst_len, src_burst_size,
			src_inc_bit);
	LOG("CCR: %#8.8x\n", ccr_value);

	return ccr_value;
}

/****************************************************************************/
/**
 * Construct a loop with only DMALD and DMAST as the body using loop counter 0.
 * The function also makes sure the loop body and the lpend is in the same
 * cache line.
 *
 * @param	dma_prog_start is the very start address of the DMA program.
 *		This is used to calculate whether the loop is in a cache line.
 * @param	cache_length is the icache line length, in terms of bytes.
 *		If it's zero, the performance enhancement feature will be
 *		turned off.
 * @param	dma_progLoopStart The starting address of the loop (DMALP).
 * @param	loop_count The inner loop count. Loop count - 1 will be used to
 * 		initialize the loop counter.
 *
 * @return	The number of bytes the loop has.
 *
 * @note		None.
 *
 *****************************************************************************/
int xzynq_construct_single_loop(char *dma_prog_start, int cache_length,
		char *dma_progLoopStart, int loop_count)
{
	int cache_start_offset;
	int cache_end_offset;
	int num_nops;
	char *dma_progbuf = dma_progLoopStart;

	LOG("Contructing single loop: loop count %d\r\n", loop_count);

	dma_progbuf += xzynq_instr_DMALP(dma_progbuf, 0, loop_count);

	if (cache_length > 0) {
		/*
		 * the cache_length > 0 switch is ued to turn on/off nop
		 * insertion
		 */
		cache_start_offset = dma_progbuf - dma_prog_start;
		cache_end_offset = cache_start_offset + 3;

		/*
		 * check whether the body and lpend fit in one cache line
		 */
		if (cache_start_offset / cache_length != cache_end_offset / cache_length) {
			/* insert the nops */
			num_nops = cache_length - cache_start_offset % cache_length;
			while (num_nops--) {
				dma_progbuf += xzynq_instr_DMANOP(dma_progbuf);
			}
		}
	}

	dma_progbuf += xzynq_instr_DMALD(dma_progbuf);
	dma_progbuf += xzynq_instr_DMAST(dma_progbuf);
	dma_progbuf += xzynq_instr_DMALPEND(dma_progbuf, dma_progbuf - 2, 0);

	return dma_progbuf - dma_progLoopStart;
}

/**
 * Construct a nested loop with only DMALD and DMAST in the inner loop body.
 * It uses loop counter 1 for the outer loop and loop counter 0 for the
 * inner loop.

 * @param dma_prog_start The very start address of the DMA program. This is used
 * 	to caculate whether the loop is in a cache line.
 * @param cache_length The icache line length, in terms of bytes. If it's zero,
 * 	the performance enhancement feture will be turned off.
 * @param dma_progLoopStart The starting address of the loop (DMALP).
 * @param loop_countOuter The outer loop count. Loop count - 1 will be used to
 * 	initialize the loop counter.
 * @param loop_countInner The inner loop count. Loop count - 1 will be used to
 * 	initialize the loop counter.
 * @return the number byes the nested loop program has.
 */
/****************************************************************************/
/**
 * Construct a nested loop with only DMALD and DMAST in the inner loop body.
 * It uses loop counter 1 for the outer loop and loop counter 0 for the
 * inner loop.
 *
 * @param	dma_prog_start is the very start address of the DMA program.
 *		This is used to calculate whether the loop is in a cache line.
 * @param	cache_length is the icache line length, in terms of bytes.
 *		If it's zero, the performance enhancement feature will be
 *		turned off.
 * @param	dma_progLoopStart The starting address of the loop (DMALP).
 * @param	loop_countOuter The outer loop count. Loop count - 1 will be
 *		used to initialize the loop counter.
 * @param	loop_countInner The inner loop count. Loop count - 1 will be
 *		used to initialize the loop counter.
 *
 * @return	The number byes the nested loop program has.
 *
 * @note		None.
 *
 *****************************************************************************/
int xzynq_construct_nested_loop(char *dma_prog_start, int cache_length,
		char *dma_progLoopStart, unsigned int loop_countOuter,
		unsigned int loop_countInner)
{
	int cache_start_offset;
	int cache_end_offset;
	int num_nops;
	char *inner_loop_start;
	char *dma_progbuf = dma_progLoopStart;

	LOG("Contructing nested loop outer %d, inner %d\r\n",
			loop_countOuter, loop_countInner);

	dma_progbuf += xzynq_instr_DMALP(dma_progbuf, 1, loop_countOuter);
	inner_loop_start = dma_progbuf;

	if (cache_length > 0) {
		/*
		 * the cache_length > 0 switch is ued to turn on/off nop
		 * insertion
		 */
		if (cache_length < 8) {
			/*
			 * if the cache line is too small to fit both loops
			 * just align the inner loop
			 */
			dma_progbuf += xzynq_construct_single_loop(dma_prog_start,
					cache_length, dma_progbuf, loop_countInner);
			/* outer loop end */
			dma_progbuf
					+= xzynq_instr_DMALPEND(dma_progbuf, inner_loop_start, 1);

			/*
			 * the nested loop is constructed for
			 * smaller cache line
			 */
			return dma_progbuf - dma_progLoopStart;
		}

		/*
		 * Now let's handle the case where a cache line can
		 * fit the nested loops.
		 */
		cache_start_offset = dma_progbuf - dma_prog_start;
		cache_end_offset = cache_start_offset + 7;

		/*
		 * check whether the body and lpend fit in one cache line
		 */
		if (cache_start_offset / cache_length != cache_end_offset / cache_length) {
			/* insert the nops */
			num_nops = cache_length - cache_start_offset % cache_length;
			while (num_nops--) {
				dma_progbuf += xzynq_instr_DMANOP(dma_progbuf);
			}
		}
	}

	/* insert the inner DMALP */
	dma_progbuf += xzynq_instr_DMALP(dma_progbuf, 0, loop_countInner);

	/* DMALD and DMAST instructions */
	dma_progbuf += xzynq_instr_DMALD(dma_progbuf);
	dma_progbuf += xzynq_instr_DMAST(dma_progbuf);

	/* inner DMALPEND */
	dma_progbuf += xzynq_instr_DMALPEND(dma_progbuf, dma_progbuf - 2, 0);
	/* outer DMALPEND */
	dma_progbuf += xzynq_instr_DMALPEND(dma_progbuf, inner_loop_start, 1);

	/* return the number of bytes */
	return dma_progbuf - dma_progLoopStart;
}

/*
 * [31:28] endian_swap_size	b0000
 * [27:25] dst_cache_ctrl	b000
 * [24:22] dst_prot_ctrl	b000
 * [21:18] dst_burst_len	b0000
 * [17:15] dst_burst_size	b000
 * [14]    dst_inc		b0
 * [27:25] src_cache_ctrl	b000
 * [24:22] src_prot_ctrl	b000
 * [21:18] src_burst_len	b0000
 * [17:15] src_burst_size	b000
 * [14]    src_inc		b0
 */
#define XZYNQ_DMA_CCR_SINGLE_BYTE	(0x0)
#define XZYNQ_DMA_CCR_M2M_SINGLE_BYTE	((0x1 << 14) | 0x1)

/****************************************************************************/
/**
 *
 * Construct the DMA program based on the descriptions of the DMA transfer.
 * The function handles memory to device and device to memory DMA transfers.
 * It also handles unalgined head and small amount of residue tail.
 *
 * @param	channel DMA channel number
 * @param	cmd is the DMA command.
 * @param	cache_length is the icache line length, in terms of bytes.
 *		If it's zero, the performance enhancement feature will be
 *		turned off.
 *
 * @returns	The number of bytes for the program.
 *
 * @note		None.
 *
 *****************************************************************************/
static int xzynq_build_dma_prog(unsigned channel, dma_cmd_t *cmd,
		unsigned cache_length)
{
	/*
	 * unpack arguments
	 */
	unsigned dev_chan = channel;
	unsigned long dma_length = cmd->bd.len;
	uint32_t src_addr = (uint32_t)cmd->bd.src_addr.paddr;

	unsigned src_inc = cmd->chan_ctrl.src_inc;
	uint32_t dst_addr = (uint32_t)cmd->bd.dst_addr.paddr;
	unsigned dst_inc = cmd->chan_ctrl.dst_inc;

	unsigned int burst_bytes;
	unsigned int loop_count;
	unsigned int loop_count1 = 0;
	unsigned int loop_residue = 0;
	unsigned int tail_bytes;
	unsigned int tail_words;
	int dma_prog_bytes;
	uint32_t ccr_value;
	unsigned int unaligned;
	unsigned int unaligned_count;
	unsigned int mem_burst_size = 1;
	unsigned int mem_burst_len = 1;
	uint32_t mem_addr = 0;
	unsigned int index;
	unsigned int src_unaligned = 0;
	unsigned int dst_unaligned = 0;
	int use_m2m_byte = 0;

	dma_chan_ctrl_t *chan_ctrl;
	dma_chan_ctrl_t wordchan_ctrl;
	static dma_chan_ctrl_t m2m_bytecc;

	char *dma_progbuf;
	char *dma_prog_start;

	dma_progbuf = cmd->gen_dma_prog;
	dma_prog_start = dma_progbuf;

	m2m_bytecc.endian_swap_size = 0;
	m2m_bytecc.dst_cache_ctrl = 0;
	m2m_bytecc.dst_prot_ctrl = 0;
	m2m_bytecc.dst_burst_len = 1;
	m2m_bytecc.dst_burst_size = 1;
	m2m_bytecc.dst_inc = 1;
	m2m_bytecc.src_cache_ctrl = 0;
	m2m_bytecc.src_prot_ctrl = 0;
	m2m_bytecc.src_burst_len = 1;
	m2m_bytecc.src_burst_size = 1;
	m2m_bytecc.src_inc = 1;

	chan_ctrl = &cmd->chan_ctrl;

	/* insert DMAMOV for SAR and DAR */
	dma_progbuf += xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_SAR, 0);
	dma_progbuf += xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_DAR, 0);

	/* insert DMAMOV for SAR and DAR */
	dma_progbuf += xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_SAR, src_addr);
	dma_progbuf += xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_DAR, dst_addr);

	if (chan_ctrl->src_inc)
		src_unaligned = src_addr % chan_ctrl->src_burst_size;

	if (chan_ctrl->dst_inc)
		dst_unaligned = dst_addr % chan_ctrl->dst_burst_size;

	if ((src_unaligned && dst_inc) || (dst_unaligned && src_inc)) {
		chan_ctrl = &m2m_bytecc;
		use_m2m_byte = 1;
	}

	if (chan_ctrl->src_inc) {
		mem_burst_size = chan_ctrl->src_burst_size;
		mem_burst_len = chan_ctrl->src_burst_len;
		mem_addr = src_addr;

	} else if (chan_ctrl->dst_inc) {
		mem_burst_size = chan_ctrl->dst_burst_size;
		mem_burst_len = chan_ctrl->dst_burst_len;
		mem_addr = dst_addr;
	}

	/* check whether the head is aligned or not */
	unaligned = mem_addr % mem_burst_size;

	if (unaligned) {
		/* if head is unaligned, transfer head in bytes */
		unaligned_count = mem_burst_size - unaligned;
		ccr_value = XZYNQ_DMA_CCR_SINGLE_BYTE | (src_inc & 1) | ((dst_inc & 1)
				<< 14);

		dma_progbuf += xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_CCR,
				ccr_value);

		LOG("unaligned head count %d\r\n", unaligned_count);
		for (index = 0; index < unaligned_count; index++) {
			dma_progbuf += xzynq_instr_DMALD(dma_progbuf);
			dma_progbuf += xzynq_instr_DMAST(dma_progbuf);
		}

		dma_length -= unaligned_count;
	}

	/* now the burst transfer part */
	ccr_value = xzynq_to_ccr_value(chan_ctrl);
	dma_progbuf
			+= xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_CCR, ccr_value);

	burst_bytes = chan_ctrl->src_burst_size * chan_ctrl->src_burst_len;

	loop_count = dma_length / burst_bytes;
	tail_bytes = dma_length % burst_bytes;

	/*
	 * the loop count register is 8-bit wide, so if we need
	 * a larger loop, we need to have nested loops
	 */
	if (loop_count > 256) {
		loop_count1 = loop_count / 256;
		if (loop_count1 > 256) {
			LOG("DMA operation cannot fit in a 2-level "
				"loop for channel %d, please reduce the "
				"DMA length or increase the burst size or "
				"length", channel);
			return 0;
		}
		loop_residue = loop_count % 256;

		LOG("loop count %d is greater than 256\r\n", loop_count);
		if (loop_count1 > 1)
			dma_progbuf += xzynq_construct_nested_loop(dma_prog_start,
					cache_length, dma_progbuf, loop_count1, 256);
		else
			dma_progbuf += xzynq_construct_single_loop(dma_prog_start,
					cache_length, dma_progbuf, 256);

		/* there will be some that cannot be covered by
		 * nested loops
		 */
		loop_count = loop_residue;
	}

	if (loop_count > 0) {
		LOG("now loop count is %d \r\n", loop_count);
		dma_progbuf += xzynq_construct_single_loop(dma_prog_start, cache_length,
				dma_progbuf, loop_count);
	}

	if (tail_bytes) {
		/* handle the tail */
		tail_words = tail_bytes / mem_burst_size;
		tail_bytes = tail_bytes % mem_burst_size;

		if (tail_words) {
			LOG("tail words is %d \r\n", tail_words);
			wordchan_ctrl = *chan_ctrl;
			/*
			 * if we can transfer the tail in words, we will
			 * transfer words as much as possible
			 */
			wordchan_ctrl.src_burst_size = mem_burst_size;
			wordchan_ctrl.src_burst_len = 1;
			wordchan_ctrl.dst_burst_size = mem_burst_size;
			wordchan_ctrl.dst_burst_len = 1;

			/*
			 * the burst length is 1
			 */
			ccr_value = xzynq_to_ccr_value(&wordchan_ctrl);

			dma_progbuf += xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_CCR,
					ccr_value);
			dma_progbuf += xzynq_construct_single_loop(dma_prog_start,
					cache_length, dma_progbuf, tail_words);

		}

		if (tail_bytes) {
			/*
			 * for the rest, we'll tranfer in bytes
			 */
			/*
			 * So far just to be safe, the tail bytes
			 * are transfered in a loop. We can optimize a little
			 * to perform a burst.
			 */
			ccr_value = XZYNQ_DMA_CCR_SINGLE_BYTE | (src_inc & 1) | ((dst_inc & 1)
					<< 14);

			dma_progbuf += xzynq_instr_DMAMOV(dma_progbuf, XZYNQ_DMA_MOV_CCR,
					ccr_value);

			LOG("tail bytes is %d \r\n", tail_bytes);
			dma_progbuf += xzynq_construct_single_loop(dma_prog_start,
					cache_length, dma_progbuf, tail_bytes);

		}
	}

	dma_progbuf += xzynq_instr_DMASEV(dma_progbuf, dev_chan);
	dma_progbuf += xzynq_instr_DMAEND(dma_progbuf);

	dma_prog_bytes = dma_progbuf - dma_prog_start;

	return dma_prog_bytes;

}

/****************************************************************************/
/**
 *
 * Generate a DMA program based for the DMA command, the buffer will be pointed
 * by the gen_dma_prog field of the command.
 *
 * @param	p is then DMA instance.
 * @param	channel is the DMA channel number.
 * @param	cmd is the DMA command.
 *
 * @return	- EOK on success.
 * 		- EFAIL if it fails
 *
 * @note		None.
 *
 *****************************************************************************/
int xzynq_gen_dma_prog(dma_ctx_t *p, unsigned int channel, dma_cmd_t *cmd)
{
	void *buf;
	int prog_len;
	dma_channel_t *chan_ptr;
	dma_chan_ctrl_t *chan_ctrl;

	LOG("inside %s\n", __func__);

	if (channel > XZYNQ_DMA_CHANNELS_PER_DEV)
		return EFAIL;

	chan_ptr = p->chans + channel;
	chan_ctrl = &cmd->chan_ctrl;

	if (chan_ctrl->src_burst_size * chan_ctrl->src_burst_len
			!= chan_ctrl->dst_burst_size * chan_ctrl->dst_burst_len) {
		fprintf(stderr, "source burst_size * burst_len does not match "
			"that of destination\r\n");
		return EFAIL;
	}

	/*
	 * unaligned fixed address is not supported
	 */
	if (!chan_ctrl->src_inc && ((unsigned int)cmd->bd.src_addr.paddr % chan_ctrl->src_burst_size)) {
		fprintf(stderr, "source address is fixed but is unaligned\r\n");
		return EFAIL;
	}

	if (!chan_ctrl->dst_inc && ((unsigned int)cmd->bd.dst_addr.paddr % chan_ctrl->dst_burst_size)) {
		fprintf(stderr, "destination address is fixed but is "
			"unaligned\r\n");
		return EFAIL;
	}

	buf = xzynq_buf_pool_alloc(chan_ptr->prog_buf_pool);
	if (buf == NULL) {
		fprintf(stderr, "failed to allocate program buffer\r\n");
		return EFAIL;
	}
	LOG("buf allocated %x\r\n", (uint32_t) buf);

	cmd->gen_dma_prog = buf;
	prog_len = xzynq_build_dma_prog(channel, cmd, p->cache_length);
	cmd->gen_dma_prog_len = prog_len;

	if (prog_len <= 0) {
		/* something wrong, release the buffer */
		xzynq_buf_pool_free(chan_ptr->prog_buf_pool, buf);
		cmd->gen_dma_prog_len = 0;
		cmd->gen_dma_prog = NULL;
		return EFAIL;
	}

	LOG("Generated DMA Prog length is %d\r\n", prog_len);

	return EOK;
}

/****************************************************************************/
/**
 * Free the DMA program buffer that is pointed by the gen_dma_prog field
 * of the command.
 *
 * @param	p is then DMA instance.
 * @param	channel is the DMA channel number.
 * @param	cmd is the DMA command.
 *
 * @return	EOK on success.
 * 		EFAIL if there is any error.
 *
 * @note	None.
 *
 ****************************************************************************/
int xzynq_free_dma_prog(dma_ctx_t *p, unsigned int channel,
		dma_cmd_t *cmd)
{
	void *buf;
	dma_channel_t *chan_ptr;

	if (channel > XZYNQ_DMA_CHANNELS_PER_DEV)
		return EFAIL;

	buf = (void *) cmd->gen_dma_prog;
	chan_ptr = p->chans + channel;

	if (buf) {
		xzynq_buf_pool_free(chan_ptr->prog_buf_pool, buf);
		cmd->gen_dma_prog = 0;
		cmd->gen_dma_prog_len = 0;
	}

	return EOK;
}

/****************************************************************************/
/**
 * Use the debug registers to kill the DMA thread.
 *
 * @param	base_addr is DMA device base address.
 * @param	channel is the DMA channel number.
 * @param	thread is Debug thread encoding.
 * 		0: DMA manager thread, 1: DMA channel.
 *
 * @return	0 on success, -1 on time out
 *
 * @note		None.
 *
 *****************************************************************************/
int xzynq_exec_DMAKILL(uint32_t base_addr, unsigned int channel,
		unsigned int thread)
{
	uint32_t dbg_inst_0;
	int wait_count;

	dbg_inst_0 = XZYNQ_DMA_DBGINST0(0, 0x01, channel, thread);

	/* wait while debug status is busy */
	wait_count = 0;
	while ((in32(base_addr + XZYNQ_DMA_DBGSTATUS_OFFSET)
			& XZYNQ_DMA_DBGSTATUS_BUSY) && (wait_count < XZYNQ_DMA_MAX_WAIT))
		wait_count++;

	if (wait_count >= XZYNQ_DMA_MAX_WAIT) {
		/* wait time out */
		fprintf(stderr, "PL330 device at %x debug status busy time out\n",
				base_addr);
		return -1;
	}

	/* write debug instructions */
	out32(base_addr + XZYNQ_DMA_DBGINST0_OFFSET, dbg_inst_0);
	out32(base_addr + XZYNQ_DMA_DBGINST1_OFFSET, 0);

	/* run the command in dbg_inst_0 and dbg_inst1 */
	out32(base_addr + XZYNQ_DMA_DBGCMD_OFFSET, 0);

	return 0;
}

/****************************************************************************/
/**
 *
 * Allocate a buffer of the DMA program buffer from the pool.
 *
 * @param	Pool the DMA program pool.
 *
 * @return	The allocated buffer, NULL if there is any error.
 *
 * @note		None.
 *
 *****************************************************************************/
void *xzynq_buf_pool_alloc(dma_prog_buf_t *pool)
{
	int index;

	for (index = 0; index < XZYNQ_DMA_MAX_CHAN_BUFS; index++) {
		if (!pool[index].allocated) {
			LOG("Allocate buf %d\r\n", index);
			pool[index].allocated = 1;
			return pool[index].buf;
		}
	}

	return NULL;
}

/****************************************************************************/
/**
 *
 *
 * Free a buffer of the DMA program buffer.
 * @param	pool the DMA program pool.
 * @param	buf the DMA program buffer to be release.
 *
 * @return	None
 *
 * @note		None.
 *
 *****************************************************************************/
void xzynq_buf_pool_free(dma_prog_buf_t *pool, void *buf)
{
	int index;
	int found = 0;

	for (index = 0; index < XZYNQ_DMA_MAX_CHAN_BUFS; index++) {
		if (pool[index].buf == buf) {
			if (pool[index].allocated) {
				LOG("Freed buf %d\r\n", index);
				pool[index].allocated = 0;
			} else {
				LOG("Trying to free a free buf %d\r\n", index);
			}
			found = 1;
		}
	}

	if (!found)
		LOG("Trying to free a buf that is not in the pool\r\n");

}

/*****************************************************************************/
/**
 * xzynq_exec_DMAGO - Execute the DMAGO to start a channel.
 *
 * @param	base_addr PL330 device base address
 * @param	channel channel number for the device
 * @param	dma_prog DMA program starting address, this should be DMA address
 *
 * @return	0 on success, -1 on time out
 *
 * @note		None.
 *
 ****************************************************************************/
int xzynq_exec_DMAGO(uint32_t base_addr, unsigned int channel,
		uint32_t dma_prog)
{
	char dma_go_prog[8];
	uint32_t dbg_inst0;
	uint32_t dbg_inst1;
	int wait_count;

	xzynq_instr_DMAGO(dma_go_prog, channel, dma_prog, 0);
	LOG("dma_go 0: %#2.2x 1: %#2.2x\n", *dma_go_prog, *(dma_go_prog + 1));
	dbg_inst0 = XZYNQ_DMA_DBGINST0(*(dma_go_prog + 1), *dma_go_prog, 0, 0);
	dbg_inst1 = (uint32_t) dma_prog;

	LOG("inside xzynq_exec_DMAGO: base %x, channel %d, dma_prog %x\r\n",
			base_addr, channel, dma_prog);
	LOG("inside xzynq_exec_DMAGO: dbg_inst0 %x, dbg_inst1 %x\r\n",
			dbg_inst0, dbg_inst1);

	/* wait while debug status is busy */
	wait_count = 0;
	LOG("Checking DBGSTATUS\r\n");
	while ((in32(base_addr + XZYNQ_DMA_DBGSTATUS_OFFSET)
			& XZYNQ_DMA_DBGSTATUS_BUSY) && (wait_count < XZYNQ_DMA_MAX_WAIT)) {
		LOG("dbgstatus %x\r\n", in32(base_addr
				+ XZYNQ_DMA_DBGSTATUS_OFFSET));

		wait_count++;
	}

	if (wait_count >= XZYNQ_DMA_MAX_WAIT) {
		LOG("PL330 device at %x debug status busy time out\r\n",
				base_addr);
		return -1;
	}

	LOG("dbgstatus idle\r\n");

	/* write debug instruction 0 */
	out32(base_addr + XZYNQ_DMA_DBGINST0_OFFSET, dbg_inst0);
	/* write debug instruction 1 */
	out32(base_addr + XZYNQ_DMA_DBGINST1_OFFSET, dbg_inst1);

	/* wait while the DMA Manager is busy */
	wait_count = 0;
	while ((in32(base_addr + XZYNQ_DMA_DS_OFFSET) & XZYNQ_DMA_DS_DMA_STATUS)
			!= XZYNQ_DMA_DS_DMA_STATUS_STOPPED && wait_count
			<= XZYNQ_DMA_MAX_WAIT) {
		LOG("ds %x\r\n", in32(base_addr + XZYNQ_DMA_DS_OFFSET));
		wait_count++;
	}

	if (wait_count >= XZYNQ_DMA_MAX_WAIT) {
		LOG("PL330 device at %x debug status busy time out\r\n",
				base_addr);
		return -1;
	}

	/* run the command in dbg_inst0 and dbg_inst1 */
	out32(base_addr + XZYNQ_DMA_DBGCMD_OFFSET, 0);
	LOG("xzynq_exec_DMAGO done\r\n");

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
