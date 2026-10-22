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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_താക്കോൽ>
class VInsertionSet final {
    std::vector<T_താക്കോൽ> m_താക്കോലുകൾ;  // Elements by insertion order
    std::unordered_set<T_താക്കോൽ> m_താക്കോൽ_Set;  // Elements by key
പൊതു:
    // METHODS
    bool തിരുകുക(സ്ഥിരം T_താക്കോൽ& താക്കോൽ) {
        // Returns if did insertion (second pair argument of traditional emplace)
        സ്ഥിരം auto itFoundPair = m_താക്കോൽ_Set.തിരുകുക(താക്കോൽ);
        എങ്കിൽ (itFoundPair.second) m_താക്കോലുകൾ.തള്ളുക_back(താക്കോൽ);
        തിരികെ itFoundPair.second;
    }
    template <typename T>
    ശൂന്യം തിരുകുക(T begin, സ്ഥിരം T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::മൂല്യം_type, T_താക്കോൽ>::മൂല്യം,
                      "T must be an iterator with value type T_Key");
        ആകുമ്പോൾ (begin != end) {
            എങ്കിൽ (m_താക്കോൽ_Set.തിരുകുക(*begin).second) m_താക്കോലുകൾ.തള്ളുക_back(*begin);
            ++begin;
        }
    }
    ശൂന്യം clear() {
        m_താക്കോലുകൾ.clear();
        m_താക്കോൽ_Set.clear();
    }

    // ACCESSORS
    bool empty() സ്ഥിരം { തിരികെ m_താക്കോലുകൾ.empty(); }
    size_t size() സ്ഥിരം { തിരികെ m_താക്കോലുകൾ.size(); }
    bool exists(സ്ഥിരം T_താക്കോൽ& താക്കോൽ) സ്ഥിരം { തിരികെ m_താക്കോൽ_Set.find(താക്കോൽ) != m_താക്കോൽ_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_താക്കോൽ>::const_iterator;
    const_iterator begin() സ്ഥിരം { തിരികെ m_താക്കോലുകൾ.begin(); }
    const_iterator end() സ്ഥിരം { തിരികെ m_താക്കോലുകൾ.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
