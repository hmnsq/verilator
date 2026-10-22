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
template <typename T_کلیل>
class VInsertionSet final {
    std::vector<T_کلیل> m_کلیلەکان;  // Elements by insertion order
    std::unordered_set<T_کلیل> m_کلیل_Set;  // Elements by key
عام:
    // METHODS
    bool زیادکردن(ثابت T_کلیل& کلیل) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت auto itFoundPair = m_کلیل_Set.زیادکردن(کلیل);
        إذا (itFoundPair.second) m_کلیلەکان.پاڵنان_back(کلیل);
        أرجع itFoundPair.second;
    }
    template <typename T>
    فراغ زیادکردن(T begin, ثابت T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::بەها_type, T_کلیل>::بەها,
                      "T must be an iterator with value type T_Key");
        طالما (begin != end) {
            إذا (m_کلیل_Set.زیادکردن(*begin).second) m_کلیلەکان.پاڵنان_back(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_کلیلەکان.clear();
        m_کلیل_Set.clear();
    }

    // ACCESSORS
    bool empty() ثابت { أرجع m_کلیلەکان.empty(); }
    size_t size() ثابت { أرجع m_کلیلەکان.size(); }
    bool exists(ثابت T_کلیل& کلیل) ثابت { أرجع m_کلیل_Set.find(کلیل) != m_کلیل_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_کلیل>::const_iterator;
    const_iterator begin() ثابت { أرجع m_کلیلەکان.begin(); }
    const_iterator end() ثابت { أرجع m_کلیلەکان.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
