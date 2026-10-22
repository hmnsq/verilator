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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3CONTAINER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3CONTAINER_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
ਸਾਂਚਾ <typename T_ਕੁੰਜੀ>
class VInsertion_ਸੈੱਟ final {
    std::ਸਦਿਸ਼<T_ਕੁੰਜੀ> m_ਕੁੰਜੀਆਂ;  // Elements by insertion order
    std::unordered_ਸੈੱਟ<T_ਕੁੰਜੀ> m_ਕੁੰਜੀ_ਸੈੱਟ;  // Elements by key
ਜਨਤਕ:
    // METHODS
    bool ਪਾਓ(ਅਚਲ T_ਕੁੰਜੀ& ਕੁੰਜੀ) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ਅਚਲ ਸਵੈਚਾਲਿਤ it_ਮਿਲਿਆ_Pair = m_ਕੁੰਜੀ_ਸੈੱਟ.ਪਾਓ(ਕੁੰਜੀ);
        ਜੇ (it_ਮਿਲਿਆ_Pair.ਦੂਜਾ) m_ਕੁੰਜੀਆਂ.ਧੱਕੋ_ਪਿੱਠ(ਕੁੰਜੀ);
        ਵਾਪਸ it_ਮਿਲਿਆ_Pair.ਦੂਜਾ;
    }
    ਸਾਂਚਾ <typename T>
    ਖਾਲੀ ਪਾਓ(T begin, ਅਚਲ T ਸਿਰਾ) {
        static_assert(std::is_same<typename std::iterator_traits<T>::ਮੁੱਲ_type, T_ਕੁੰਜੀ>::ਮੁੱਲ,
                      "T must be an iterator with value type T_Key");
        ਜਦੋਂ (begin != ਸਿਰਾ) {
            ਜੇ (m_ਕੁੰਜੀ_ਸੈੱਟ.ਪਾਓ(*begin).ਦੂਜਾ) m_ਕੁੰਜੀਆਂ.ਧੱਕੋ_ਪਿੱਠ(*begin);
            ++begin;
        }
    }
    ਖਾਲੀ clear() {
        m_ਕੁੰਜੀਆਂ.clear();
        m_ਕੁੰਜੀ_ਸੈੱਟ.clear();
    }

    // ACCESSORS
    bool ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ(); }
    ਮਾਪ_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀਆਂ.ਮਾਪ(); }
    bool exists(ਅਚਲ T_ਕੁੰਜੀ& ਕੁੰਜੀ) ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀ_ਸੈੱਟ.ਲੱਭੋ(ਕੁੰਜੀ) != m_ਕੁੰਜੀ_ਸੈੱਟ.ਸਿਰਾ(); }

    // ITERATORS
    using const_iterator = typename std::ਸਦਿਸ਼<T_ਕੁੰਜੀ>::const_iterator;
    const_iterator begin() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀਆਂ.begin(); }
    const_iterator ਸਿਰਾ() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀਆਂ.ਸਿਰਾ(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#ਜੇ_ਅੰਤ  // Guard
