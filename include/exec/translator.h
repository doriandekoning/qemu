/*
 * Generic intermediate code generation.
 *
 * Copyright (C) 2016-2017 Lluís Vilanova <vilanova@ac.upc.edu>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef EXEC__TRANSLATOR_H
#define EXEC__TRANSLATOR_H

/*
 * Include this header from a target-specific file, and add a
 *
 *     DisasContextBase base;
 *
 * member in your target-specific DisasContext.
 */


#include "qemu/bswap.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "exec/plugin-gen.h"
#include "tcg/tcg.h"


/**
 * DisasJumpType:
 * @DISAS_NEXT: Next instruction in program order.
 * @DISAS_TOO_MANY: Too many instructions translated.
 * @DISAS_NORETURN: Following code is dead.
 * @DISAS_TARGET_*: Start of target-specific conditions.
 *
 * What instruction to disassemble next.
 */
typedef enum DisasJumpType {
    DISAS_NEXT,
    DISAS_TOO_MANY,
    DISAS_NORETURN,
    DISAS_TARGET_0,
    DISAS_TARGET_1,
    DISAS_TARGET_2,
    DISAS_TARGET_3,
    DISAS_TARGET_4,
    DISAS_TARGET_5,
    DISAS_TARGET_6,
    DISAS_TARGET_7,
    DISAS_TARGET_8,
    DISAS_TARGET_9,
    DISAS_TARGET_10,
    DISAS_TARGET_11,
} DisasJumpType;

/**
 * DisasContextBase:
 * @tb: Translation block for this disassembly.
 * @pc_first: Address of first guest instruction in this TB.
 * @pc_next: Address of next guest instruction in this TB (current during
 *           disassembly).
 * @is_jmp: What instruction to disassemble next.
 * @num_insns: Number of translated instructions (including current).
 * @max_insns: Maximum number of instructions to be translated in this TB.
 * @singlestep_enabled: "Hardware" single stepping enabled.
 *
 * Architecture-agnostic disassembly context.
 */
typedef struct DisasContextBase {
    TranslationBlock *tb;
    target_ulong pc_first;
    target_ulong pc_next;
    DisasJumpType is_jmp;
    int num_insns;
    int max_insns;
    bool singlestep_enabled;
} DisasContextBase;

/**
 * TranslatorOps:
 * @init_disas_context:
 *      Initialize the target-specific portions of DisasContext struct.
 *      The generic DisasContextBase has already been initialized.
 *
 * @tb_start:
 *      Emit any code required before the start of the main loop,
 *      after the generic gen_tb_start().
 *
 * @insn_start:
 *      Emit the tcg_gen_insn_start opcode.
 *
 * @breakpoint_check:
 *      When called, the breakpoint has already been checked to match the PC,
 *      but the target may decide the breakpoint missed the address
 *      (e.g., due to conditions encoded in their flags).  Return true to
 *      indicate that the breakpoint did hit, in which case no more breakpoints
 *      are checked.  If the breakpoint did hit, emit any code required to
 *      signal the exception, and set db->is_jmp as necessary to terminate
 *      the main loop.
 *
 * @translate_insn:
 *      Disassemble one instruction and set db->pc_next for the start
 *      of the following instruction.  Set db->is_jmp as necessary to
 *      terminate the main loop.
 *
 * @tb_stop:
 *      Emit any opcodes required to exit the TB, based on db->is_jmp.
 *
 * @disas_log:
 *      Print instruction disassembly to log.
 */
typedef struct TranslatorOps {
    void (*init_disas_context)(DisasContextBase *db, CPUState *cpu);
    void (*tb_start)(DisasContextBase *db, CPUState *cpu);
    void (*insn_start)(DisasContextBase *db, CPUState *cpu);
    bool (*breakpoint_check)(DisasContextBase *db, CPUState *cpu,
                             const CPUBreakpoint *bp);
    void (*translate_insn)(DisasContextBase *db, CPUState *cpu);
    void (*tb_stop)(DisasContextBase *db, CPUState *cpu);
    void (*disas_log)(const DisasContextBase *db, CPUState *cpu);
} TranslatorOps;

/**
 * translator_loop:
 * @ops: Target-specific operations.
 * @db: Disassembly context.
 * @cpu: Target vCPU.
 * @tb: Translation block.
 * @max_insns: Maximum number of insns to translate.
 *
 * Generic translator loop.
 *
 * Translation will stop in the following cases (in order):
 * - When is_jmp set by #TranslatorOps::breakpoint_check.
 *   - set to DISAS_TOO_MANY exits after translating one more insn
 *   - set to any other value than DISAS_NEXT exits immediately.
 * - When is_jmp set by #TranslatorOps::translate_insn.
 *   - set to any value other than DISAS_NEXT exits immediately.
 * - When the TCG operation buffer is full.
 * - When single-stepping is enabled (system-wide or on the current vCPU).
 * - When too many instructions have been translated.
 */
void translator_loop(const TranslatorOps *ops, DisasContextBase *db,
                     CPUState *cpu, TranslationBlock *tb, int max_insns);

void translator_loop_temp_check(DisasContextBase *db);

/*
 * Translator Load Functions
 *
 * These are intended to replace the old cpu_ld*_code functions and
 * are mandatory for front-ends that have been migrated to the common
 * translator_loop. These functions are only intended to be called
 * from the translation stage and should not be called from helper
 * functions. Those functions should be converted to encode the
 * relevant information at translation time.
 */

#ifdef CONFIG_USER_ONLY

#define DO_LOAD(type, name, shift)               \
    do {                                         \
        set_helper_retaddr(1);                   \
        ret = name ## _p(g2h(pc));               \
        clear_helper_retaddr();                  \
    } while (0)

#else

#define DO_LOAD(type, name, shift)                          \
    do {                                                    \
        int mmu_idx = cpu_mmu_index(env, true);             \
        TCGMemOpIdx oi = make_memop_idx(shift, mmu_idx);    \
        ret = helper_ret_ ## name ## _cmmu(env, pc, oi, 0); \
    } while (0)

#endif

#define GEN_TRANSLATOR_LD(fullname, name, type, shift, swap_fn)         \
    static inline type                                                  \
    fullname ## _swap(CPUArchState *env, abi_ptr pc, bool do_swap)      \
    {                                                                   \
        type ret;                                                       \
        DO_LOAD(type, name, shift);                                     \
                                                                        \
        if (do_swap) {                                                  \
            ret = swap_fn(ret);                                         \
        }                                                               \
        plugin_insn_append(&ret, sizeof(ret));                          \
        return ret;                                                     \
    }                                                                   \
                                                                        \
    static inline type fullname(CPUArchState *env, abi_ptr pc)          \
    {                                                                   \
        return fullname ## _swap(env, pc, false);                       \
    }

GEN_TRANSLATOR_LD(translator_ldub, ldub, uint8_t, 0, /* no swap */ )
GEN_TRANSLATOR_LD(translator_ldsw, ldsw, int16_t, 1, bswap16)
GEN_TRANSLATOR_LD(translator_lduw, lduw, uint16_t, 1, bswap16)
GEN_TRANSLATOR_LD(translator_ldl, ldl, uint32_t, 2, bswap32)
GEN_TRANSLATOR_LD(translator_ldq, ldq, uint64_t, 3, bswap64)
#undef GEN_TRANSLATOR_LD

#endif  /* EXEC__TRANSLATOR_H */
