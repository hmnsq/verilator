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

#اگر_غیر_تعریف_شدہ VERILATOR_V3CONTAINER_H_
#تعریف_کرو VERILATOR_V3CONTAINER_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <unordered_map>
#شامل <unordered_set>
#شامل <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
سانچہ <typename T_کلید>
class VInsertion_سیٹ final {
    std::سمتیہ<T_کلید> m_کلیدیں;  // Elements by insertion order
    std::unordered_سیٹ<T_کلید> m_کلید_سیٹ;  // Elements by key
عوامی:
    // METHODS
    bool داخل_کرو(ثابت T_کلید& کلید) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ثابت خودکار it_ملا_Pair = m_کلید_سیٹ.داخل_کرو(کلید);
        اگر (it_ملا_Pair.دوسرا) m_کلیدیں.ڈالو_عقب(کلید);
        واپس it_ملا_Pair.دوسرا;
    }
    سانچہ <typename T>
    خالی داخل_کرو(T begin, ثابت T انتہا) {
        static_assert(std::is_same<typename std::iterator_traits<T>::قدر_type, T_کلید>::قدر,
                      "T must be an iterator with value type T_Key");
        جب (begin != انتہا) {
            اگر (m_کلید_سیٹ.داخل_کرو(*begin).دوسرا) m_کلیدیں.ڈالو_عقب(*begin);
            ++begin;
        }
    }
    خالی clear() {
        m_کلیدیں.clear();
        m_کلید_سیٹ.clear();
    }

    // ACCESSORS
    bool خالی_خانے() ثابت { واپس m_کلیدیں.خالی_خانے(); }
    سائز_t سائز() ثابت { واپس m_کلیدیں.سائز(); }
    bool exists(ثابت T_کلید& کلید) ثابت { واپس m_کلید_سیٹ.ڈھونڈو(کلید) != m_کلید_سیٹ.انتہا(); }

    // ITERATORS
    using const_iterator = typename std::سمتیہ<T_کلید>::const_iterator;
    const_iterator begin() ثابت { واپس m_کلیدیں.begin(); }
    const_iterator انتہا() ثابت { واپس m_کلیدیں.انتہا(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#اگر_ختم  // Guard
