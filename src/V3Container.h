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

#اگر_تعریف_نشده VERILATOR_V3CONTAINER_H_
#تعریف_کن VERILATOR_V3CONTAINER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
قالب <typename T_کلید>
class VInsertion_مجموعه final {
    std::وکتور<T_کلید> m_کلیدها;  // Elements by insertion order
    std::unordered_مجموعه<T_کلید> m_کلید_مجموعه;  // Elements by key
عام:
    // METHODS
    bool درج(ثابت T_کلید& کلید) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت خودکار it_یافت_Pair = m_کلید_مجموعه.درج(کلید);
        إذا (it_یافت_Pair.دومی) m_کلیدها.افزودن_پشت(کلید);
        أرجع it_یافت_Pair.دومی;
    }
    قالب <typename T>
    فراغ درج(T begin, ثابت T انتها) {
        static_assert(std::is_same<typename std::iterator_traits<T>::مقدار_type, T_کلید>::مقدار,
                      "T must be an iterator with value type T_Key");
        طالما (begin != انتها) {
            إذا (m_کلید_مجموعه.درج(*begin).دومی) m_کلیدها.افزودن_پشت(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_کلیدها.clear();
        m_کلید_مجموعه.clear();
    }

    // ACCESSORS
    bool خالی() ثابت { أرجع m_کلیدها.خالی(); }
    اندازه_t اندازه() ثابت { أرجع m_کلیدها.اندازه(); }
    bool exists(ثابت T_کلید& کلید) ثابت { أرجع m_کلید_مجموعه.پیدا_کن(کلید) != m_کلید_مجموعه.انتها(); }

    // ITERATORS
    using const_iterator = typename std::وکتور<T_کلید>::const_iterator;
    const_iterator begin() ثابت { أرجع m_کلیدها.begin(); }
    const_iterator انتها() ثابت { أرجع m_کلیدها.انتها(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#پایان_اگر  // Guard
