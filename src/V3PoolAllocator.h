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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3POOLALLOCATOR_H_
#وصف_ڪريو VERILATOR_V3POOLALLOCATOR_H_

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
سانچو <typename T_Elem, سائيز_t N_Chunk_سائيز = 128>
class PoolAllocator final {
    static_assert(N_Chunk_سائيز > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    اتحاد جڳهه final {
        T_Elem m_elem;  // Storage for the allocated element
        جڳهه* m_ايندڙ_Freep;  // Link to the next free slot
        جڳهه() {}
        ~جڳهه() {}
    };

    // MEMBERS
    جڳهه* m_freep = nullptr;  // Head of the free list
    std::ويڪٽر<std::unique_ptr<جڳهه[]>> m_allocated;  // The allocated chunks

عام:
    // CONSTRUCTORS
    PoolAllocator() = افتراضي;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    سانچو <typename... دليلون_2>
    T_Elem* alloc(دليلون_2&&... دليلون) {
        // If no free slots available, then make some
        إذا (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_پٺ(new جڳهه[N_Chunk_سائيز]);
            // Chain the new slots into the free list
            جڳهه* ثابت chunkp = m_allocated.پٺ().get();
            لكل (سائيز_t i = 1; i < N_Chunk_سائيز; ++i) chunkp[i - 1].m_ايندڙ_Freep = &chunkp[i];
            chunkp[N_Chunk_سائيز - 1].m_ايندڙ_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        جڳهه* ثابت slotp = m_freep;
        m_freep = slotp->m_ايندڙ_Freep;
        أرجع new (&slotp->m_elem) T_Elem{std::forward<دليلون_2>(دليلون)...};
    }

    // Destroy an element, and return its slot for future allocation
    فراغ حرر(T_Elem* elemp) {
        elemp->~T_Elem();
        جڳهه* ثابت slotp = reinterpret_cast<جڳهه*>(elemp);
        slotp->m_ايندڙ_Freep = m_freep;
        m_freep = slotp;
    }
};

#جيڪڏهن_پورو  // Guard
