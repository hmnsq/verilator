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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3CONTAINER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3CONTAINER_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
ಮಾದರಿ <typename T_ಕೀಲಿ>
class VInsertion_ಗಣ final {
    std::ಸದಿಶ<T_ಕೀಲಿ> m_ಕೀಲಿಗಳು;  // Elements by insertion order
    std::unordered_ಗಣ<T_ಕೀಲಿ> m_ಕೀಲಿ_ಗಣ;  // Elements by key
ಸಾರ್ವಜನಿಕ:
    // METHODS
    bool ಸೇರಿಸು(ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ಕೀಲಿ) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it_ಸಿಕ್ಕಿತು_Pair = m_ಕೀಲಿ_ಗಣ.ಸೇರಿಸು(ಕೀಲಿ);
        ಆದರೆ (it_ಸಿಕ್ಕಿತು_Pair.ಎರಡನೇ) m_ಕೀಲಿಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(ಕೀಲಿ);
        ಹಿಂತಿರುಗಿ it_ಸಿಕ್ಕಿತು_Pair.ಎರಡನೇ;
    }
    ಮಾದರಿ <typename T>
    ಶೂನ್ಯ ಸೇರಿಸು(T begin, ಸ್ಥಿರಾಂಕ T ಕೊನೆ) {
        static_assert(std::is_same<typename std::iterator_traits<T>::ಮೌಲ್ಯ_type, T_ಕೀಲಿ>::ಮೌಲ್ಯ,
                      "T must be an iterator with value type T_Key");
        ಆಗ (begin != ಕೊನೆ) {
            ಆದರೆ (m_ಕೀಲಿ_ಗಣ.ಸೇರಿಸು(*begin).ಎರಡನೇ) m_ಕೀಲಿಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(*begin);
            ++begin;
        }
    }
    ಶೂನ್ಯ clear() {
        m_ಕೀಲಿಗಳು.clear();
        m_ಕೀಲಿ_ಗಣ.clear();
    }

    // ACCESSORS
    bool ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿಗಳು.ಖಾಲಿಗಳು(); }
    ಗಾತ್ರ_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿಗಳು.ಗಾತ್ರ_2(); }
    bool exists(ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ಕೀಲಿ) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ_ಗಣ.ಕಂಡುಹಿಡಿ(ಕೀಲಿ) != m_ಕೀಲಿ_ಗಣ.ಕೊನೆ(); }

    // ITERATORS
    using const_iterator = typename std::ಸದಿಶ<T_ಕೀಲಿ>::const_iterator;
    const_iterator begin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿಗಳು.begin(); }
    const_iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿಗಳು.ಕೊನೆ(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#ಆದರೆ_ಅಂತ್ಯ  // Guard
