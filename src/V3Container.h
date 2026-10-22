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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_ಕೀಲಿ>
class VInsertionSet final {
    std::vector<T_ಕೀಲಿ> m_ಕೀಲಿಗಳು;  // Elements by insertion order
    std::unordered_set<T_ಕೀಲಿ> m_ಕೀಲಿ_Set;  // Elements by key
ಸಾರ್ವಜನಿಕ:
    // METHODS
    bool ಸೇರಿಸು(ಧ್ರುವ T_ಕೀಲಿ& ಕೀಲಿ) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ಧ್ರುವ auto itFoundPair = m_ಕೀಲಿ_Set.ಸೇರಿಸು(ಕೀಲಿ);
        ಯದಿ (itFoundPair.second) m_ಕೀಲಿಗಳು.ತಳ್ಳು_back(ಕೀಲಿ);
        ಪ್ರತ್ಯಾಗಮ itFoundPair.second;
    }
    template <typename T>
    ಶೂನ್ಯ ಸೇರಿಸು(T begin, ಧ್ರುವ T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::ಮೌಲ್ಯ_type, T_ಕೀಲಿ>::ಮೌಲ್ಯ,
                      "T must be an iterator with value type T_Key");
        ಯಾವತ್ (begin != end) {
            ಯದಿ (m_ಕೀಲಿ_Set.ಸೇರಿಸು(*begin).second) m_ಕೀಲಿಗಳು.ತಳ್ಳು_back(*begin);
            ++begin;
        }
    }
    ಶೂನ್ಯ clear() {
        m_ಕೀಲಿಗಳು.clear();
        m_ಕೀಲಿ_Set.clear();
    }

    // ACCESSORS
    bool empty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿಗಳು.empty(); }
    size_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿಗಳು.size(); }
    bool exists(ಧ್ರುವ T_ಕೀಲಿ& ಕೀಲಿ) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ_Set.find(ಕೀಲಿ) != m_ಕೀಲಿ_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_ಕೀಲಿ>::const_iterator;
    const_iterator begin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿಗಳು.begin(); }
    const_iterator end() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿಗಳು.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
