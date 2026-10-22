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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3POOLALLOCATOR_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3POOLALLOCATOR_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <cstddef>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <new>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
ಮಾದರಿ <typename T_Elem, ಗಾತ್ರ_t N_Chunk_ಗಾತ್ರ = 128>
class PoolAllocator final {
    static_assert(N_Chunk_ಗಾತ್ರ > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    ಒಕ್ಕೂಟ ಸ್ಥಾನ final {
        T_Elem m_elem;  // Storage for the allocated element
        ಸ್ಥಾನ* m_ಮುಂದಿನ_Freep;  // Link to the next free slot
        ಸ್ಥಾನ() {}
        ~ಸ್ಥಾನ() {}
    };

    // MEMBERS
    ಸ್ಥಾನ* m_freep = nullptr;  // Head of the free list
    std::ಸದಿಶ<std::unique_ptr<ಸ್ಥಾನ[]>> m_allocated;  // The allocated chunks

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    PoolAllocator() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    ಮಾದರಿ <typename... ವಾದಗಳು_2>
    T_Elem* alloc(ವಾದಗಳು_2&&... ವಾದಗಳು) {
        // If no free slots available, then make some
        ಆದರೆ (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_ಹಿಂಭಾಗ(new ಸ್ಥಾನ[N_Chunk_ಗಾತ್ರ]);
            // Chain the new slots into the free list
            ಸ್ಥಾನ* ಸ್ಥಿರಾಂಕ chunkp = m_allocated.ಹಿಂಭಾಗ().get();
            ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < N_Chunk_ಗಾತ್ರ; ++i) chunkp[i - 1].m_ಮುಂದಿನ_Freep = &chunkp[i];
            chunkp[N_Chunk_ಗಾತ್ರ - 1].m_ಮುಂದಿನ_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        ಸ್ಥಾನ* ಸ್ಥಿರಾಂಕ slotp = m_freep;
        m_freep = slotp->m_ಮುಂದಿನ_Freep;
        ಹಿಂತಿರುಗಿ new (&slotp->m_elem) T_Elem{std::forward<ವಾದಗಳು_2>(ವಾದಗಳು)...};
    }

    // Destroy an element, and return its slot for future allocation
    ಶೂನ್ಯ ಬಿಡುಗಡೆ(T_Elem* elemp) {
        elemp->~T_Elem();
        ಸ್ಥಾನ* ಸ್ಥಿರಾಂಕ slotp = reinterpret_cast<ಸ್ಥಾನ*>(elemp);
        slotp->m_ಮುಂದಿನ_Freep = m_freep;
        m_freep = slotp;
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
