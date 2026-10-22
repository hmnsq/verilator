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

#اگر_تعریف_نشده VERILATOR_V3POOLALLOCATOR_H_
#تعریف_کن VERILATOR_V3POOLALLOCATOR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <cstddef>
#ضمن <memory>
#ضمن <new>
#ضمن <utility>
#ضمن <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
قالب <typename T_Elem, اندازه_t N_Chunk_اندازه = 128>
class PoolAllocator final {
    static_assert(N_Chunk_اندازه > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    اتحاد جایگاه final {
        T_Elem m_elem;  // Storage for the allocated element
        جایگاه* m_بعدی_Freep;  // Link to the next free slot
        جایگاه() {}
        ~جایگاه() {}
    };

    // MEMBERS
    جایگاه* m_freep = nullptr;  // Head of the free list
    std::وکتور<std::unique_ptr<جایگاه[]>> m_allocated;  // The allocated chunks

عام:
    // CONSTRUCTORS
    PoolAllocator() = افتراضي;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    قالب <typename... آرگومانها_2>
    T_Elem* alloc(آرگومانها_2&&... آرگومانها) {
        // If no free slots available, then make some
        إذا (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_پشت(new جایگاه[N_Chunk_اندازه]);
            // Chain the new slots into the free list
            جایگاه* ثابت chunkp = m_allocated.پشت().get();
            لكل (اندازه_t i = 1; i < N_Chunk_اندازه; ++i) chunkp[i - 1].m_بعدی_Freep = &chunkp[i];
            chunkp[N_Chunk_اندازه - 1].m_بعدی_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        جایگاه* ثابت slotp = m_freep;
        m_freep = slotp->m_بعدی_Freep;
        أرجع new (&slotp->m_elem) T_Elem{std::forward<آرگومانها_2>(آرگومانها)...};
    }

    // Destroy an element, and return its slot for future allocation
    فراغ حرر(T_Elem* elemp) {
        elemp->~T_Elem();
        جایگاه* ثابت slotp = reinterpret_cast<جایگاه*>(elemp);
        slotp->m_بعدی_Freep = m_freep;
        m_freep = slotp;
    }
};

#پایان_اگر  // Guard
