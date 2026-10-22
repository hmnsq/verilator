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

#यदि_अपरिभाषितम् VERILATOR_V3POOLALLOCATOR_H_
#परिभाषय VERILATOR_V3POOLALLOCATOR_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <cstddef>
#अन्तर्भाव <memory>
#अन्तर्भाव <new>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
प्रतिरूपम् <typename T_Elem, आकार_t N_Chunk_आकार = 128>
class PoolAllocator final {
    static_assert(N_Chunk_आकार > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    सङ्घ Slot final {
        T_Elem m_elem;  // Storage for the allocated element
        Slot* m_अग्र_Freep;  // Link to the next free slot
        Slot() {}
        ~Slot() {}
    };

    // MEMBERS
    Slot* m_freep = nullptr;  // Head of the free list
    std::सदिशम्<std::unique_ptr<Slot[]>> m_allocated;  // The allocated chunks

सार्वजनिक:
    // CONSTRUCTORS
    PoolAllocator() = स्वभाव;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    प्रतिरूपम् <typename... तर्काः_2>
    T_Elem* alloc(तर्काः_2&&... तर्काः) {
        // If no free slots available, then make some
        यदि (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_पृष्ठभागः(new Slot[N_Chunk_आकार]);
            // Chain the new slots into the free list
            Slot* ध्रुव chunkp = m_allocated.पृष्ठभागः().get();
            प्रति (आकार_t i = 1; i < N_Chunk_आकार; ++i) chunkp[i - 1].m_अग्र_Freep = &chunkp[i];
            chunkp[N_Chunk_आकार - 1].m_अग्र_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        Slot* ध्रुव slotp = m_freep;
        m_freep = slotp->m_अग्र_Freep;
        प्रत्यागम new (&slotp->m_elem) T_Elem{std::forward<तर्काः_2>(तर्काः)...};
    }

    // Destroy an element, and return its slot for future allocation
    शून्य मुक्त(T_Elem* elemp) {
        elemp->~T_Elem();
        Slot* ध्रुव slotp = reinterpret_cast<Slot*>(elemp);
        slotp->m_अग्र_Freep = m_freep;
        m_freep = slotp;
    }
};

#यदि_अन्तः  // Guard
