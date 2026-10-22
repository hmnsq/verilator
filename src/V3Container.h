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

#جے_بنا_تعریف VERILATOR_V3CONTAINER_H_
#تعریف_کرو VERILATOR_V3CONTAINER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
سانچا <typename T_کلید>
class VInsertion_سیٹ final {
    std::سمتیہ<T_کلید> m_کلیدیں;  // Elements by insertion order
    std::unordered_سیٹ<T_کلید> m_کلید_سیٹ;  // Elements by key
عام:
    // METHODS
    bool داخل_کرو(ثابت T_کلید& کلید) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت خودکار it_لبھیا_Pair = m_کلید_سیٹ.داخل_کرو(کلید);
        إذا (it_لبھیا_Pair.دوجا) m_کلیدیں.ڈالو_پٹھ(کلید);
        أرجع it_لبھیا_Pair.دوجا;
    }
    سانچا <typename T>
    فراغ داخل_کرو(T begin, ثابت T سرا) {
        static_assert(std::is_same<typename std::iterator_traits<T>::قدر_type, T_کلید>::قدر,
                      "T must be an iterator with value type T_Key");
        طالما (begin != سرا) {
            إذا (m_کلید_سیٹ.داخل_کرو(*begin).دوجا) m_کلیدیں.ڈالو_پٹھ(*begin);
            ++begin;
        }
    }
    فراغ clear() {
        m_کلیدیں.clear();
        m_کلید_سیٹ.clear();
    }

    // ACCESSORS
    bool خالی_تھاواں() ثابت { أرجع m_کلیدیں.خالی_تھاواں(); }
    ناپ_t ناپ() ثابت { أرجع m_کلیدیں.ناپ(); }
    bool exists(ثابت T_کلید& کلید) ثابت { أرجع m_کلید_سیٹ.لبھو(کلید) != m_کلید_سیٹ.سرا(); }

    // ITERATORS
    using const_iterator = typename std::سمتیہ<T_کلید>::const_iterator;
    const_iterator begin() ثابت { أرجع m_کلیدیں.begin(); }
    const_iterator سرا() ثابت { أرجع m_کلیدیں.سرا(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#جے_انت  // Guard
