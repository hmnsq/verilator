// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Chunked pool allocator
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#اگر_غیر_تعریف_شدہ VERILATOR_V3POOLALLOCATOR_H_
#تعریف_کرو VERILATOR_V3POOLALLOCATOR_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <cstddef>
#شامل <memory>
#شامل <new>
#شامل <utility>
#شامل <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
سانچہ <typename T_Elem, سائز_t N_Chunk_سائز = 128>
class PoolAllocator final {
    static_assert(N_Chunk_سائز > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    اتحاد خانہ final {
        T_Elem m_elem;  // Storage for the allocated element
        خانہ* m_اگلا_Freep;  // Link to the next free slot
        خانہ() {}
        ~خانہ() {}
    };

    // MEMBERS
    خانہ* m_freep = nullptr;  // Head of the free list
    std::سمتیہ<std::unique_ptr<خانہ[]>> m_allocated;  // The allocated chunks

عوامی:
    // CONSTRUCTORS
    PoolAllocator() = طےشدہ;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    سانچہ <typename... دلائل_2>
    T_Elem* alloc(دلائل_2&&... دلائل) {
        // If no free slots available, then make some
        اگر (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_عقب(new خانہ[N_Chunk_سائز]);
            // Chain the new slots into the free list
            خانہ* ثابت chunkp = m_allocated.عقب().get();
            برائے (سائز_t i = 1; i < N_Chunk_سائز; ++i) chunkp[i - 1].m_اگلا_Freep = &chunkp[i];
            chunkp[N_Chunk_سائز - 1].m_اگلا_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        خانہ* ثابت slotp = m_freep;
        m_freep = slotp->m_اگلا_Freep;
        واپس new (&slotp->m_elem) T_Elem{std::forward<دلائل_2>(دلائل)...};
    }

    // Destroy an element, and return its slot for future allocation
    خالی آزاد(T_Elem* elemp) {
        elemp->~T_Elem();
        خانہ* ثابت slotp = reinterpret_cast<خانہ*>(elemp);
        slotp->m_اگلا_Freep = m_freep;
        m_freep = slotp;
    }
};

#اگر_ختم  // Guard
