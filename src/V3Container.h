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

#अगर_अघोषित VERILATOR_V3CONTAINER_H_
#घोषणा VERILATOR_V3CONTAINER_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <unordered_map>
#समावेश <unordered_set>
#समावेश <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
ढाँचा <typename T_कुंजी>
class VInsertionSet final {
    std::vector<T_कुंजी> m_कुंजियाँ;  // Elements by insertion order
    std::unordered_set<T_कुंजी> m_कुंजी_Set;  // Elements by key
खुला:
    // METHODS
    bool जोड़ो(स्थिर T_कुंजी& कुंजी) {
        // Returns if did insertion (second pair argument of traditional emplace)
        स्थिर स्वतः itFoundPair = m_कुंजी_Set.जोड़ो(कुंजी);
        अगर (itFoundPair.second) m_कुंजियाँ.धकेलो_back(कुंजी);
        वापस itFoundPair.second;
    }
    ढाँचा <typename T>
    व्योम जोड़ो(T begin, स्थिर T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::मूल्य_type, T_कुंजी>::मूल्य,
                      "T must be an iterator with value type T_Key");
        जबतक (begin != end) {
            अगर (m_कुंजी_Set.जोड़ो(*begin).second) m_कुंजियाँ.धकेलो_back(*begin);
            ++begin;
        }
    }
    व्योम clear() {
        m_कुंजियाँ.clear();
        m_कुंजी_Set.clear();
    }

    // ACCESSORS
    bool empty() स्थिर { वापस m_कुंजियाँ.empty(); }
    माप_प्रकार आकार() स्थिर { वापस m_कुंजियाँ.आकार(); }
    bool exists(स्थिर T_कुंजी& कुंजी) स्थिर { वापस m_कुंजी_Set.find(कुंजी) != m_कुंजी_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_कुंजी>::const_iterator;
    const_iterator begin() स्थिर { वापस m_कुंजियाँ.begin(); }
    const_iterator end() स्थिर { वापस m_कुंजियाँ.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#पूर्ण_अगर  // Guard
