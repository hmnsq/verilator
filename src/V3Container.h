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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_চাবি>
class VInsertionSet final {
    std::vector<T_চাবি> m_চাবিগুলি;  // Elements by insertion order
    std::unordered_set<T_চাবি> m_চাবি_Set;  // Elements by key
সর্বজনীন:
    // METHODS
    bool ঢোকাও(ধ্রুবক T_চাবি& চাবি) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ধ্রুবক auto itFoundPair = m_চাবি_Set.ঢোকাও(চাবি);
        যদি (itFoundPair.second) m_চাবিগুলি.ঠেলো_back(চাবি);
        ফেরত itFoundPair.second;
    }
    template <typename T>
    শূন্য ঢোকাও(T begin, ধ্রুবক T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::মান_type, T_চাবি>::মান,
                      "T must be an iterator with value type T_Key");
        যখন (begin != end) {
            যদি (m_চাবি_Set.ঢোকাও(*begin).second) m_চাবিগুলি.ঠেলো_back(*begin);
            ++begin;
        }
    }
    শূন্য clear() {
        m_চাবিগুলি.clear();
        m_চাবি_Set.clear();
    }

    // ACCESSORS
    bool empty() ধ্রুবক { ফেরত m_চাবিগুলি.empty(); }
    আকার_t আকার_2() ধ্রুবক { ফেরত m_চাবিগুলি.আকার_2(); }
    bool exists(ধ্রুবক T_চাবি& চাবি) ধ্রুবক { ফেরত m_চাবি_Set.find(চাবি) != m_চাবি_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_চাবি>::const_iterator;
    const_iterator begin() ধ্রুবক { ফেরত m_চাবিগুলি.begin(); }
    const_iterator end() ধ্রুবক { ফেরত m_চাবিগুলি.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
