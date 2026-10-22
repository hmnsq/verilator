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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_कुंजी>
class VInsertionSet final {
    std::vector<T_कुंजी> m_कुंजियाँ;  // Elements by insertion order
    std::unordered_set<T_कुंजी> m_कुंजी_Set;  // Elements by key
सार्वजनिक:
    // METHODS
    bool जोड़ो(ध्रुव T_कुंजी& कुंजी) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ध्रुव auto itFoundPair = m_कुंजी_Set.जोड़ो(कुंजी);
        यदि (itFoundPair.second) m_कुंजियाँ.धकेलो_back(कुंजी);
        प्रत्यागम itFoundPair.second;
    }
    template <typename T>
    शून्य जोड़ो(T begin, ध्रुव T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::मूल्य_type, T_कुंजी>::मूल्य,
                      "T must be an iterator with value type T_Key");
        यावत् (begin != end) {
            यदि (m_कुंजी_Set.जोड़ो(*begin).second) m_कुंजियाँ.धकेलो_back(*begin);
            ++begin;
        }
    }
    शून्य clear() {
        m_कुंजियाँ.clear();
        m_कुंजी_Set.clear();
    }

    // ACCESSORS
    bool empty() ध्रुव { प्रत्यागम m_कुंजियाँ.empty(); }
    size_t size() ध्रुव { प्रत्यागम m_कुंजियाँ.size(); }
    bool exists(ध्रुव T_कुंजी& कुंजी) ध्रुव { प्रत्यागम m_कुंजी_Set.find(कुंजी) != m_कुंजी_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_कुंजी>::const_iterator;
    const_iterator begin() ध्रुव { प्रत्यागम m_कुंजियाँ.begin(); }
    const_iterator end() ध्रुव { प्रत्यागम m_कुंजियाँ.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
