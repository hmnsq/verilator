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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3POOLALLOCATOR_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3POOLALLOCATOR_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cstddef>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <new>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
ਸਾਂਚਾ <typename T_Elem, ਮਾਪ_t N_Chunk_ਮਾਪ = 128>
class PoolAllocator final {
    static_assert(N_Chunk_ਮਾਪ > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    ਸੰਘ ਥਾਂ final {
        T_Elem m_elem;  // Storage for the allocated element
        ਥਾਂ* m_ਅਗਲਾ_Freep;  // Link to the next free slot
        ਥਾਂ() {}
        ~ਥਾਂ() {}
    };

    // MEMBERS
    ਥਾਂ* m_freep = nullptr;  // Head of the free list
    std::ਸਦਿਸ਼<std::unique_ptr<ਥਾਂ[]>> m_allocated;  // The allocated chunks

ਜਨਤਕ:
    // CONSTRUCTORS
    PoolAllocator() = ਮੂਲ;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    ਸਾਂਚਾ <typename... ਦਲੀਲਾਂ_2>
    T_Elem* alloc(ਦਲੀਲਾਂ_2&&... ਦਲੀਲਾਂ) {
        // If no free slots available, then make some
        ਜੇ (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_ਪਿੱਠ(new ਥਾਂ[N_Chunk_ਮਾਪ]);
            // Chain the new slots into the free list
            ਥਾਂ* ਅਚਲ chunkp = m_allocated.ਪਿੱਠ().get();
            ਲਈ (ਮਾਪ_t i = 1; i < N_Chunk_ਮਾਪ; ++i) chunkp[i - 1].m_ਅਗਲਾ_Freep = &chunkp[i];
            chunkp[N_Chunk_ਮਾਪ - 1].m_ਅਗਲਾ_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        ਥਾਂ* ਅਚਲ slotp = m_freep;
        m_freep = slotp->m_ਅਗਲਾ_Freep;
        ਵਾਪਸ new (&slotp->m_elem) T_Elem{std::forward<ਦਲੀਲਾਂ_2>(ਦਲੀਲਾਂ)...};
    }

    // Destroy an element, and return its slot for future allocation
    ਖਾਲੀ ਮੁਕਤ(T_Elem* elemp) {
        elemp->~T_Elem();
        ਥਾਂ* ਅਚਲ slotp = reinterpret_cast<ਥਾਂ*>(elemp);
        slotp->m_ਅਗਲਾ_Freep = m_freep;
        m_freep = slotp;
    }
};

#ਜੇ_ਅੰਤ  // Guard
