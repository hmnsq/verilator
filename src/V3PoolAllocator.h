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

#నిర్వచితం_కాకపోతే VERILATOR_V3POOLALLOCATOR_H_
#నిర్వచించు VERILATOR_V3POOLALLOCATOR_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <cstddef>
#అన్తర్భావ <memory>
#అన్తర్భావ <new>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

// Hands out elements of a single type, allocated 'N_ChunkSize' at a time for
// efficiency. Released elements are recycled via a free list stored in the
// same storage. All memory is released when the pool is destroyed, so the
// elements it handed out must not be used beyond its lifetime, and must all
// have been released by then.
మూస <typename T_Elem, కొలత_t N_Chunk_కొలత = 128>
class PoolAllocator final {
    static_assert(N_Chunk_కొలత > 0, "Chunk size must be non-zero");

    // A slot of storage, holding either a live element, or a link in the free list.
    సంఘం స్థానం final {
        T_Elem m_elem;  // Storage for the allocated element
        స్థానం* m_తదుపరి_Freep;  // Link to the next free slot
        స్థానం() {}
        ~స్థానం() {}
    };

    // MEMBERS
    స్థానం* m_freep = nullptr;  // Head of the free list
    std::సదిశ<std::unique_ptr<స్థానం[]>> m_allocated;  // The allocated chunks

బహిరంగ:
    // CONSTRUCTORS
    PoolAllocator() = అప్రమేయ;
    VL_UNCOPYABLE(PoolAllocator);
    VL_UNMOVABLE(PoolAllocator);

    // METHODS
    // Allocate an element, constructed with the given arguments
    మూస <typename... వాదనలు_2>
    T_Elem* alloc(వాదనలు_2&&... వాదనలు) {
        // If no free slots available, then make some
        ఒకవేళ (!m_freep) {
            // Allocate in chunks for efficiency
            m_allocated.emplace_వెనుక(new స్థానం[N_Chunk_కొలత]);
            // Chain the new slots into the free list
            స్థానం* స్థిరరాశి chunkp = m_allocated.వెనుక().get();
            కొరకు (కొలత_t i = 1; i < N_Chunk_కొలత; ++i) chunkp[i - 1].m_తదుపరి_Freep = &chunkp[i];
            chunkp[N_Chunk_కొలత - 1].m_తదుపరి_Freep = nullptr;
            m_freep = chunkp;
        }
        // Free slots are available, pick up the first one
        స్థానం* స్థిరరాశి slotp = m_freep;
        m_freep = slotp->m_తదుపరి_Freep;
        తిరిగి new (&slotp->m_elem) T_Elem{std::forward<వాదనలు_2>(వాదనలు)...};
    }

    // Destroy an element, and return its slot for future allocation
    శూన్య విడుదల(T_Elem* elemp) {
        elemp->~T_Elem();
        స్థానం* స్థిరరాశి slotp = reinterpret_cast<స్థానం*>(elemp);
        slotp->m_తదుపరి_Freep = m_freep;
        m_freep = slotp;
    }
};

#అయితే_ముగింపు  // Guard
