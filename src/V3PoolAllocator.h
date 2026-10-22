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

#ifndef VERILATOR_V3POOLALLOCATOR_H_
#define VERILATOR_V3POOLALLOCATOR_H_

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব <cstddef>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <new>
#অন্তর্ভাব <utility>
#অন্তর্ভাব <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
template <typename T_Elem, আকার_t N_Chunk_আকার = 128>
class PoolAllocator final {
    static_assert(N_Chunk_আকার > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    সংঘ Slot final {
        T_Elem m_elem;  // Storage for the allocated element
        Slot* m_পরবর্তী_Freep;  // Link to the next free slot
        Slot() {}
        ~Slot() {}
    };

    // MEMBERS
    Slot* m_freep = nullptr;  // Head of the free list
    std::vector<std::unique_ptr<Slot[]>> m_allocated;  // The allocated chunks

সর্বজনীন:
    // CONSTRUCTORS
    PoolAllocator() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    template <typename... Args>
    T_Elem* alloc(Args&&... args) {
        // If no free slots available, then make some
        যদি (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_back(new Slot[N_Chunk_আকার]);
            // Chain the new slots into the free list
            Slot* ধ্রুবক chunkp = m_allocated.back().get();
            জন্য (আকার_t i = 1; i < N_Chunk_আকার; ++i) chunkp[i - 1].m_পরবর্তী_Freep = &chunkp[i];
            chunkp[N_Chunk_আকার - 1].m_পরবর্তী_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        Slot* ধ্রুবক slotp = m_freep;
        m_freep = slotp->m_পরবর্তী_Freep;
        ফেরত new (&slotp->m_elem) T_Elem{std::forward<Args>(args)...};
    }

    // Destroy an element, and return its slot for future allocation
    শূন্য মুক্ত(T_Elem* elemp) {
        elemp->~T_Elem();
        Slot* ধ্রুবক slotp = reinterpret_cast<Slot*>(elemp);
        slotp->m_পরবর্তী_Freep = m_freep;
        m_freep = slotp;
    }
};

#endif  // Guard
