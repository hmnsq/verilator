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

#که_نه_تعریف_شوی VERILATOR_V3CONTAINER_H_
#تعریف_کړه VERILATOR_V3CONTAINER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
کينډۍ <typename T_کلی>
class VInsertion_سېټ final {
    std::ویکټور<T_کلی> m_کلیګانې;  // Elements by insertion order
    std::unordered_سېټ<T_کلی> m_کلی_سېټ;  // Elements by key
عام:
    // METHODS
    bool ورننباسه(ثابت T_کلی& کلی) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت خپلکاری it_وموندل_Pair = m_کلی_سېټ.ورننباسه(کلی);
        إذا (it_وموندل_Pair.دوهم) m_کلیګانې.ټېل_شا(کلی);
        أرجع it_وموندل_Pair.دوهم;
    }
    کينډۍ <typename T>
    فراغ ورننباسه(T begin, ثابت T پای_ټکی) {
        static_assert(std::is_same<typename std::iterator_traits<T>::ارزښت_type, T_کلی>::ارزښت,
                      "T must be an iterator with value type T_Key");
        طالما (begin != پای_ټکی) {
            إذا (m_کلی_سېټ.ورننباسه(*begin).دوهم) m_کلیګانې.ټېل_شا(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_کلیګانې.clear();
        m_کلی_سېټ.clear();
    }

    // ACCESSORS
    bool تشه() ثابت { أرجع m_کلیګانې.تشه(); }
    اندازه_t اندازه() ثابت { أرجع m_کلیګانې.اندازه(); }
    bool exists(ثابت T_کلی& کلی) ثابت { أرجع m_کلی_سېټ.پیدا_کړه(کلی) != m_کلی_سېټ.پای_ټکی(); }

    // ITERATORS
    using const_iterator = typename std::ویکټور<T_کلی>::const_iterator;
    const_iterator begin() ثابت { أرجع m_کلیګانې.begin(); }
    const_iterator پای_ټکی() ثابت { أرجع m_کلیګانې.پای_ټکی(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#د_که_پای  // Guard
