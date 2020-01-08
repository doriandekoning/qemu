/*
 * Common Atomic Helper Functions
 *
 * This file should be included before the various instantiations of
 * the atomic_template.h helpers.
 *
 * Copyright (c) 2019 Linaro
 * Written by Alex Bennée <alex.bennee@linaro.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>

static inline
void atomic_trace_rmw_pre(CPUArchState *env, target_ulong addr, uint16_t info)
{
    CPUState *cpu = env_cpu(env);
    trace_guest_mem_before_exec(cpu, addr, info);
    trace_guest_mem_before_exec(cpu, addr, info | TRACE_MEM_ST);
}

static inline void
atomic_trace_rmw_post(CPUArchState *env, target_ulong addr, uint16_t info, uint64_t val)
{
    uint64_t physaddr_ld = lookup_tlb_informational(env, info >> 8, 0, addr);
    trace_guest_mem_load_before_exec(env_cpu(env), addr, physaddr_ld, info, 9);
    uint64_t physaddr_st = lookup_tlb_informational(env, info >> 8, 0, addr);
    trace_guest_mem_store_before_exec(env_cpu(env), addr, physaddr_st, info | TRACE_MEM_ST, 7, val, env->cr[3]);
    qemu_plugin_vcpu_mem_cb(env_cpu(env), addr, info);
    qemu_plugin_vcpu_mem_cb(env_cpu(env), addr, info | TRACE_MEM_ST);
}

static inline
void atomic_trace_ld_pre(CPUArchState *env, target_ulong addr, uint16_t info)
{
    trace_guest_mem_before_exec(env_cpu(env), addr, info);
    // trace_guest_mem_load_before_exec(env_cpu(env), addr, info);//fixed in post
}

static inline
void atomic_trace_ld_post(CPUArchState *env, target_ulong addr, uint16_t info)
{
    uint64_t physaddr = lookup_tlb_informational(env, info >> 8, 0, addr);
    trace_guest_mem_load_before_exec(env_cpu(env), addr, physaddr, info, 10);
    qemu_plugin_vcpu_mem_cb(env_cpu(env), addr, info);
}

static inline
void atomic_trace_st_pre(CPUArchState *env, target_ulong addr, uint16_t info)
{
    trace_guest_mem_before_exec(env_cpu(env), addr, info);
}

static inline
void atomic_trace_st_post(CPUArchState *env, target_ulong addr, uint16_t info, uint64_t val)
{
    uint64_t physaddr = lookup_tlb_informational(env, info >> 8, 1, addr);
    trace_guest_mem_store_before_exec(env_cpu(env), addr, physaddr, info, 8, val, env->cr[3]);
    qemu_plugin_vcpu_mem_cb(env_cpu(env), addr, info);
}
