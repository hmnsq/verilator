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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3CONTAINER_H_
#وصف_ڪريو VERILATOR_V3CONTAINER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
سانچو <typename T_ڪنجي>
class VInsertion_سيٽ final {
    std::ويڪٽر<T_ڪنجي> m_ڪنجيون;  // Elements by insertion order
    std::unordered_سيٽ<T_ڪنجي> m_ڪنجي_سيٽ;  // Elements by key
عام:
    // METHODS
    bool وجهو(ثابت T_ڪنجي& ڪنجي) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت خودڪار it_مليو_Pair = m_ڪنجي_سيٽ.وجهو(ڪنجي);
        إذا (it_مليو_Pair.ٻيو) m_ڪنجيون.ڌڪو_پٺ(ڪنجي);
        أرجع it_مليو_Pair.ٻيو;
    }
    سانچو <typename T>
    فراغ وجهو(T begin, ثابت T پڇاڙي) {
        static_assert(std::is_same<typename std::iterator_traits<T>::قدر_type, T_ڪنجي>::قدر,
                      "T must be an iterator with value type T_Key");
        طالما (begin != پڇاڙي) {
            إذا (m_ڪنجي_سيٽ.وجهو(*begin).ٻيو) m_ڪنجيون.ڌڪو_پٺ(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_ڪنجيون.clear();
        m_ڪنجي_سيٽ.clear();
    }

    // ACCESSORS
    bool خالي_جايون() ثابت { أرجع m_ڪنجيون.خالي_جايون(); }
    سائيز_t سائيز() ثابت { أرجع m_ڪنجيون.سائيز(); }
    bool exists(ثابت T_ڪنجي& ڪنجي) ثابت { أرجع m_ڪنجي_سيٽ.ڳوليو(ڪنجي) != m_ڪنجي_سيٽ.پڇاڙي(); }

    // ITERATORS
    using const_iterator = typename std::ويڪٽر<T_ڪنجي>::const_iterator;
    const_iterator begin() ثابت { أرجع m_ڪنجيون.begin(); }
    const_iterator پڇاڙي() ثابت { أرجع m_ڪنجيون.پڇاڙي(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#جيڪڏهن_پورو  // Guard
