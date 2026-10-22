// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Generic container types
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

#నిర్వచితం_కాకపోతే VERILATOR_V3CONTAINER_H_
#నిర్వచించు VERILATOR_V3CONTAINER_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
మూస <typename T_కీలకం>
class VInsertion_సమితి final {
    std::సదిశ<T_కీలకం> m_కీలకాలు;  // Elements by insertion order
    std::unordered_సమితి<T_కీలకం> m_కీలకం_సమితి;  // Elements by key
బహిరంగ:
    // METHODS
    bool చొప్పించు(స్థిరరాశి T_కీలకం& కీలకం) {
        // Returns if did insertion (second pair argument of traditional emplace)
        స్థిరరాశి స్వయంచాలక it_దొరికింది_Pair = m_కీలకం_సమితి.చొప్పించు(కీలకం);
        ఒకవేళ (it_దొరికింది_Pair.రెండవ) m_కీలకాలు.నెట్టు_వెనుక(కీలకం);
        తిరిగి it_దొరికింది_Pair.రెండవ;
    }
    మూస <typename T>
    శూన్య చొప్పించు(T begin, స్థిరరాశి T చివర) {
        static_assert(std::is_same<typename std::iterator_traits<T>::విలువ_type, T_కీలకం>::విలువ,
                      "T must be an iterator with value type T_Key");
        అయితే (begin != చివర) {
            ఒకవేళ (m_కీలకం_సమితి.చొప్పించు(*begin).రెండవ) m_కీలకాలు.నెట్టు_వెనుక(*begin);
            ++begin;
        }
    }
    శూన్య clear() {
        m_కీలకాలు.clear();
        m_కీలకం_సమితి.clear();
    }

    // ACCESSORS
    bool ఖాళీలు() స్థిరరాశి { తిరిగి m_కీలకాలు.ఖాళీలు(); }
    కొలత_t కొలత() స్థిరరాశి { తిరిగి m_కీలకాలు.కొలత(); }
    bool exists(స్థిరరాశి T_కీలకం& కీలకం) స్థిరరాశి { తిరిగి m_కీలకం_సమితి.కనుగొను(కీలకం) != m_కీలకం_సమితి.చివర(); }

    // ITERATORS
    using const_iterator = typename std::సదిశ<T_కీలకం>::const_iterator;
    const_iterator begin() స్థిరరాశి { తిరిగి m_కీలకాలు.begin(); }
    const_iterator చివర() స్థిరరాశి { తిరిగి m_కీలకాలు.చివర(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#అయితే_ముగింపు  // Guard
