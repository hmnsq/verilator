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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_کونچي>
class VInsertionSet final {
    std::vector<T_کونچي> m_کونچي_سموا;  // Elements by insertion order
    std::unordered_set<T_کونچي> m_کونچي_Set;  // Elements by key
عام:
    // METHODS
    bool سيسيڤ(ثابت T_کونچي& کونچي) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت auto itFoundPair = m_کونچي_Set.سيسيڤ(کونچي);
        إذا (itFoundPair.second) m_کونچي_سموا.تولق_back(کونچي);
        أرجع itFoundPair.second;
    }
    template <typename T>
    فراغ سيسيڤ(T begin, ثابت T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::نيلاي_type, T_کونچي>::نيلاي,
                      "T must be an iterator with value type T_Key");
        طالما (begin != end) {
            إذا (m_کونچي_Set.سيسيڤ(*begin).second) m_کونچي_سموا.تولق_back(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_کونچي_سموا.clear();
        m_کونچي_Set.clear();
    }

    // ACCESSORS
    bool empty() ثابت { أرجع m_کونچي_سموا.empty(); }
    size_t size() ثابت { أرجع m_کونچي_سموا.size(); }
    bool exists(ثابت T_کونچي& کونچي) ثابت { أرجع m_کونچي_Set.find(کونچي) != m_کونچي_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_کونچي>::const_iterator;
    const_iterator begin() ثابت { أرجع m_کونچي_سموا.begin(); }
    const_iterator end() ثابت { أرجع m_کونچي_سموا.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
