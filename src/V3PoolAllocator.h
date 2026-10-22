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

#अगर_अघोषित VERILATOR_V3POOLALLOCATOR_H_
#घोषणा VERILATOR_V3POOLALLOCATOR_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <cstddef>
#समावेश <memory>
#समावेश <new>
#समावेश <utility>
#समावेश <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
ढाँचा <typename T_Elem, माप_प्रकार N_Chunk_आकार = 128>
class PoolAllocator final {
    static_assert(N_Chunk_आकार > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    जोड़ Slot final {
        T_Elem m_elem;  // Storage for the allocated element
        Slot* m_अग्र_Freep;  // Link to the next free slot
        Slot() {}
        ~Slot() {}
    };

    // MEMBERS
    Slot* m_freep = nullptr;  // Head of the free list
    std::vector<std::unique_ptr<Slot[]>> m_allocated;  // The allocated chunks

खुला:
    // CONSTRUCTORS
    PoolAllocator() = शेष;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    ढाँचा <typename... Args>
    T_Elem* alloc(Args&&... args) {
        // If no free slots available, then make some
        अगर (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_back(new Slot[N_Chunk_आकार]);
            // Chain the new slots into the free list
            Slot* स्थिर chunkp = m_allocated.back().get();
            क्रम (माप_प्रकार i = 1; i < N_Chunk_आकार; ++i) chunkp[i - 1].m_अग्र_Freep = &chunkp[i];
            chunkp[N_Chunk_आकार - 1].m_अग्र_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        Slot* स्थिर slotp = m_freep;
        m_freep = slotp->m_अग्र_Freep;
        वापस new (&slotp->m_elem) T_Elem{std::forward<Args>(args)...};
    }

    // Destroy an element, and return its slot for future allocation
    व्योम मुक्त(T_Elem* elemp) {
        elemp->~T_Elem();
        Slot* स्थिर slotp = reinterpret_cast<Slot*>(elemp);
        slotp->m_अग्र_Freep = m_freep;
        m_freep = slotp;
    }
};

#पूर्ण_अगर  // Guard
