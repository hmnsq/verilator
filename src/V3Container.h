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
ढाँचा <typename T_साँचो>
class VInsertionSet final {
    std::vector<T_साँचो> m_साँचोहरू;  // Elements by insertion order
    std::unordered_set<T_साँचो> m_साँचो_Set;  // Elements by key
सार्वजनिक:
    // METHODS
    bool घुसाउ(अचर T_साँचो& साँचो) {
        // Returns if did insertion (second pair argument of traditional emplace)
        अचर स्वतः itFoundPair = m_साँचो_Set.घुसाउ(साँचो);
        यदि (itFoundPair.second) m_साँचोहरू.थप_back(साँचो);
        फर्काऊ itFoundPair.second;
    }
    ढाँचा <typename T>
    शून्य घुसाउ(T begin, अचर T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::मान_type, T_साँचो>::मान,
                      "T must be an iterator with value type T_Key");
        जबसम्म (begin != end) {
            यदि (m_साँचो_Set.घुसाउ(*begin).second) m_साँचोहरू.थप_back(*begin);
            ++begin;
        }
    }
    शून्य clear() {
        m_साँचोहरू.clear();
        m_साँचो_Set.clear();
    }

    // ACCESSORS
    bool empty() अचर { फर्काऊ m_साँचोहरू.empty(); }
    माप_प्रकार size() अचर { फर्काऊ m_साँचोहरू.size(); }
    bool exists(अचर T_साँचो& साँचो) अचर { फर्काऊ m_साँचो_Set.find(साँचो) != m_साँचो_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_साँचो>::const_iterator;
    const_iterator begin() अचर { फर्काऊ m_साँचोहरू.begin(); }
    const_iterator end() अचर { फर्काऊ m_साँचोहरू.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
