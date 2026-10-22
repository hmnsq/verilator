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

#ifndef VERILATOR_V3CONTAINER_H_
#define VERILATOR_V3CONTAINER_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_ચાવી>
class VInsertionSet final {
    std::vector<T_ચાવી> m_ચાવીઓ;  // Elements by insertion order
    std::unordered_set<T_ચાવી> m_ચાવી_Set;  // Elements by key
જાહેર:
    // METHODS
    bool ઉમેરો(અચલ T_ચાવી& ચાવી) {
        // Returns if did insertion (second pair argument of traditional emplace)
        અચલ auto itFoundPair = m_ચાવી_Set.ઉમેરો(ચાવી);
        જો (itFoundPair.second) m_ચાવીઓ.ધકેલો_back(ચાવી);
        પાછા itFoundPair.second;
    }
    template <typename T>
    શૂન્ય ઉમેરો(T begin, અચલ T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::કિંમત_type, T_ચાવી>::કિંમત,
                      "T must be an iterator with value type T_Key");
        જ્યારે (begin != end) {
            જો (m_ચાવી_Set.ઉમેરો(*begin).second) m_ચાવીઓ.ધકેલો_back(*begin);
            ++begin;
        }
    }
    શૂન્ય clear() {
        m_ચાવીઓ.clear();
        m_ચાવી_Set.clear();
    }

    // ACCESSORS
    bool empty() અચલ { પાછા m_ચાવીઓ.empty(); }
    size_t size() અચલ { પાછા m_ચાવીઓ.size(); }
    bool exists(અચલ T_ચાવી& ચાવી) અચલ { પાછા m_ચાવી_Set.find(ચાવી) != m_ચાવી_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_ચાવી>::const_iterator;
    const_iterator begin() અચલ { પાછા m_ચાવીઓ.begin(); }
    const_iterator end() અચલ { પાછા m_ચાવીઓ.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
