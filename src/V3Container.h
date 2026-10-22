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
template <typename T_کلید>
class VInsertionSet final {
    std::vector<T_کلید> m_کلیدیں;  // Elements by insertion order
    std::unordered_set<T_کلید> m_کلید_Set;  // Elements by key
عام:
    // METHODS
    bool داخل_کرو(ثابت T_کلید& کلید) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت auto itFoundPair = m_کلید_Set.داخل_کرو(کلید);
        إذا (itFoundPair.second) m_کلیدیں.ڈالو_back(کلید);
        أرجع itFoundPair.second;
    }
    template <typename T>
    فراغ داخل_کرو(T begin, ثابت T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::قدر_type, T_کلید>::قدر,
                      "T must be an iterator with value type T_Key");
        طالما (begin != end) {
            إذا (m_کلید_Set.داخل_کرو(*begin).second) m_کلیدیں.ڈالو_back(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_کلیدیں.clear();
        m_کلید_Set.clear();
    }

    // ACCESSORS
    bool empty() ثابت { أرجع m_کلیدیں.empty(); }
    size_t size() ثابت { أرجع m_کلیدیں.size(); }
    bool exists(ثابت T_کلید& کلید) ثابت { أرجع m_کلید_Set.find(کلید) != m_کلید_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_کلید>::const_iterator;
    const_iterator begin() ثابت { أرجع m_کلیدیں.begin(); }
    const_iterator end() ثابت { أرجع m_کلیدیں.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
