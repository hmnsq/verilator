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
ढाँचा <typename T_किल्ली>
class VInsertionSet final {
    std::vector<T_किल्ली> m_किल्ल्या;  // Elements by insertion order
    std::unordered_set<T_किल्ली> m_किल्ली_Set;  // Elements by key
सार्वजनिक:
    // METHODS
    bool घाला(स्थिरांक T_किल्ली& किल्ली) {
        // Returns if did insertion (second pair argument of traditional emplace)
        स्थिरांक स्वतः itFoundPair = m_किल्ली_Set.घाला(किल्ली);
        जर (itFoundPair.second) m_किल्ल्या.ढकला_back(किल्ली);
        परत itFoundPair.second;
    }
    ढाँचा <typename T>
    शून्य घाला(T begin, स्थिरांक T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::मूल्य_type, T_किल्ली>::मूल्य,
                      "T must be an iterator with value type T_Key");
        पर्यंत (begin != end) {
            जर (m_किल्ली_Set.घाला(*begin).second) m_किल्ल्या.ढकला_back(*begin);
            ++begin;
        }
    }
    शून्य clear() {
        m_किल्ल्या.clear();
        m_किल्ली_Set.clear();
    }

    // ACCESSORS
    bool empty() स्थिरांक { परत m_किल्ल्या.empty(); }
    माप_प्रकार size() स्थिरांक { परत m_किल्ल्या.size(); }
    bool exists(स्थिरांक T_किल्ली& किल्ली) स्थिरांक { परत m_किल्ली_Set.find(किल्ली) != m_किल्ली_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_किल्ली>::const_iterator;
    const_iterator begin() स्थिरांक { परत m_किल्ल्या.begin(); }
    const_iterator end() स्थिरांक { परत m_किल्ल्या.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
