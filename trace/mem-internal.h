/*
 * Helper functions for guest memory tracing
 *
 * Copyright (C) 2016 Lluís Vilanova <vilanova@ac.upc.edu>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef TRACE__MEM_INTERNAL_H
#define TRACE__MEM_INTERNAL_H

#include "exec/cpu_ldst.h"


#define TRACE_MEM_SZ_SHIFT_MASK 0xf /* size shift mask */
#define TRACE_MEM_SE (1ULL << 4)    /* sign extended (y/n) */
#define TRACE_MEM_BE (1ULL << 5)    /* big endian (y/n) */
#define TRACE_MEM_ST (1ULL << 6)    /* store (y/n) */
#define TRACE_MEM_MMU_SHIFT 8       /* mmu idx */

static inline uint16_t trace_mem_build_info(
    int size_shift, bool sign_extend, MemOp endianness,
    bool store, unsigned int mmu_idx)
{
    uint16_t res;

    res = size_shift & TRACE_MEM_SZ_SHIFT_MASK;
    if (sign_extend) {
        res |= TRACE_MEM_SE;
    }
    if (endianness == MO_BE) {
        res |= TRACE_MEM_BE;
    }
    if (store) {
        res |= TRACE_MEM_ST;
    }
#ifdef CONFIG_SOFTMMU
    res |= mmu_idx << TRACE_MEM_MMU_SHIFT;
#endif
    return res;
}

static inline uint16_t trace_mem_get_info(MemOp op,
                                          unsigned int mmu_idx,
                                          bool store)
{
    return trace_mem_build_info(op & MO_SIZE, !!(op & MO_SIGN),
                                op & MO_BSWAP, store,
                                mmu_idx);
}

/* Used by the atomic helpers */
static inline
uint16_t trace_mem_build_info_no_se_be(int size_shift, bool store,
                                      TCGMemOpIdx oi)
{
    return trace_mem_build_info(size_shift, false, MO_BE, store,
                                get_mmuidx(oi));
}

static inline
uint16_t trace_mem_build_info_no_se_le(int size_shift, bool store,
                                       TCGMemOpIdx oi)
{
    return trace_mem_build_info(size_shift, false, MO_LE, store,
                                get_mmuidx(oi));
}

#ifdef CONFIG_SOFTMMU
static inline
uint64_t lookup_tlb_informational(CPUX86State *env, uint32_t mmuidx, uint32_t store, uint64_t vaddr) {
    CPUTLBEntry *tlbe = tlb_entry(env, mmuidx, vaddr);
    // uintptr_t index = tlb_index(env, mmuidx, addr);
    uint64_t tlbe_addr = store ? tlb_addr_write(tlbe) : tlbe->addr_read;
    if (likely(tlb_hit(tlbe_addr, vaddr))) {
        //TODO check if IO
        // printf("%016lx,\t%016lx+%016lx\n", physaddr, vaddr, tlbe->addend);
        return (tlbe_addr & ~(0xfff)) | (vaddr & 0xfff);
        // return vaddr + tlbe->addend;

    }
    return 1ULL << 52;
}
#endif /* CONFIG_SOFTMMU */
#endif /* TRACE__MEM_INTERNAL_H */
