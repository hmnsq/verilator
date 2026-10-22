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
template <typename T_مفتاح>
class VInsertionSet final {
    std::vector<T_مفتاح> m_مفاتيح;  // Elements by insertion order
    std::unordered_set<T_مفتاح> m_مفتاح_Set;  // Elements by key
عام:
    // METHODS
    bool أدرج(ثابت T_مفتاح& مفتاح) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت auto itFoundPair = m_مفتاح_Set.أدرج(مفتاح);
        إذا (itFoundPair.second) m_مفاتيح.ادفع_back(مفتاح);
        أرجع itFoundPair.second;
    }
    template <typename T>
    فراغ أدرج(T begin, ثابت T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::قيمة_type, T_مفتاح>::قيمة,
                      "T must be an iterator with value type T_Key");
        طالما (begin != end) {
            إذا (m_مفتاح_Set.أدرج(*begin).second) m_مفاتيح.ادفع_back(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_مفاتيح.clear();
        m_مفتاح_Set.clear();
    }

    // ACCESSORS
    bool empty() ثابت { أرجع m_مفاتيح.empty(); }
    size_t size() ثابت { أرجع m_مفاتيح.size(); }
    bool exists(ثابت T_مفتاح& مفتاح) ثابت { أرجع m_مفتاح_Set.find(مفتاح) != m_مفتاح_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_مفتاح>::const_iterator;
    const_iterator begin() ثابت { أرجع m_مفاتيح.begin(); }
    const_iterator end() ثابت { أرجع m_مفاتيح.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
