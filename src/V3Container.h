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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
template <typename T_ଚାବି>
class VInsertionSet final {
    std::vector<T_ଚାବି> m_ଚାବିଗୁଡ଼ିକ;  // Elements by insertion order
    std::unordered_set<T_ଚାବି> m_ଚାବି_Set;  // Elements by key
ସର୍ବସାଧାରଣ:
    // METHODS
    bool ଭର୍ତ୍ତି(ଅଚଳ T_ଚାବି& ଚାବି) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ଅଚଳ auto itFoundPair = m_ଚାବି_Set.ଭର୍ତ୍ତି(ଚାବି);
        ଯଦି (itFoundPair.second) m_ଚାବିଗୁଡ଼ିକ.ଠେଲ_back(ଚାବି);
        ଫେରନ୍ତୁ itFoundPair.second;
    }
    template <typename T>
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(T begin, ଅଚଳ T end) {
        static_assert(std::is_same<typename std::iterator_traits<T>::ମୂଲ୍ୟ_type, T_ଚାବି>::ମୂଲ୍ୟ,
                      "T must be an iterator with value type T_Key");
        ଯେତେବେଳେ (begin != end) {
            ଯଦି (m_ଚାବି_Set.ଭର୍ତ୍ତି(*begin).second) m_ଚାବିଗୁଡ଼ିକ.ଠେଲ_back(*begin);
            ++begin;
        }
    }
    ଶୂନ୍ୟ clear() {
        m_ଚାବିଗୁଡ଼ିକ.clear();
        m_ଚାବି_Set.clear();
    }

    // ACCESSORS
    bool empty() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବିଗୁଡ଼ିକ.empty(); }
    size_t size() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବିଗୁଡ଼ିକ.size(); }
    bool exists(ଅଚଳ T_ଚାବି& ଚାବି) ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବି_Set.find(ଚାବି) != m_ଚାବି_Set.end(); }

    // ITERATORS
    using const_iterator = typename std::vector<T_ଚାବି>::const_iterator;
    const_iterator begin() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବିଗୁଡ଼ିକ.begin(); }
    const_iterator end() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବିଗୁଡ଼ିକ.end(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#endif  // Guard
