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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_சாவி>
class VInsertionSet final {
    std::vector<T_சாவி> m_சாவிகள்;  // Elements by insertion order
    std::unordered_set<T_சாவி> m_சாவி_Set;  // Elements by key
பொது:
    // METHODS
    bool செருகு(மாறிலி T_சாவி& சாவி) {
        // Returns if did insertion (second pair argument of traditional emplace)
        மாறிலி auto itFoundPair = m_சாவி_Set.செருகு(சாவி);
        எனில் (itFoundPair.second) m_சாவிகள்.தள்ளு_back(சாவி);
        திரும்பு itFoundPair.second;
    }
    template <typename T>
    வெறுமை செருகு(T begin, மாறிலி T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::மதிப்பு_type, T_சாவி>::மதிப்பு,
                      "T must be an iterator with value type T_Key");
        போது (begin != end) {
            எனில் (m_சாவி_Set.செருகு(*begin).second) m_சாவிகள்.தள்ளு_back(*begin);
            ++begin;
        }
    }
    வெறுமை clear() {
        m_சாவிகள்.clear();
        m_சாவி_Set.clear();
    }

    // ACCESSORS
    bool empty() மாறிலி { திரும்பு m_சாவிகள்.empty(); }
    size_t size() மாறிலி { திரும்பு m_சாவிகள்.size(); }
    bool exists(மாறிலி T_சாவி& சாவி) மாறிலி { திரும்பு m_சாவி_Set.find(சாவி) != m_சாவி_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_சாவி>::const_iterator;
    const_iterator begin() மாறிலி { திரும்பு m_சாவிகள்.begin(); }
    const_iterator end() மாறிலி { திரும்பு m_சாவிகள்.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
