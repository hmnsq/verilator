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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <unordered_map>
#شامل <unordered_set>
#شامل <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_کلید>
class VInsertionSet final {
    std::vector<T_کلید> m_کلیدها;  // Elements by insertion order
    std::unordered_set<T_کلید> m_کلید_Set;  // Elements by key
عمومی:
    // METHODS
    bool درج(ثابت T_کلید& کلید) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت auto itFoundPair = m_کلید_Set.درج(کلید);
        اگر (itFoundPair.second) m_کلیدها.افزودن_back(کلید);
        بازگشت itFoundPair.second;
    }
    template <typename T>
    تهی درج(T begin, ثابت T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::مقدار_type, T_کلید>::مقدار,
                      "T must be an iterator with value type T_Key");
        وقتی (begin != end) {
            اگر (m_کلید_Set.درج(*begin).second) m_کلیدها.افزودن_back(*begin);
            ++begin;
        }
    }
    تهی clear() {
        m_کلیدها.clear();
        m_کلید_Set.clear();
    }

    // ACCESSORS
    bool empty() ثابت { بازگشت m_کلیدها.empty(); }
    size_t size() ثابت { بازگشت m_کلیدها.size(); }
    bool exists(ثابت T_کلید& کلید) ثابت { بازگشت m_کلید_Set.find(کلید) != m_کلید_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_کلید>::const_iterator;
    const_iterator begin() ثابت { بازگشت m_کلیدها.begin(); }
    const_iterator end() ثابت { بازگشت m_کلیدها.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
