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

#यदि_अपरिभाषितम् VERILATOR_V3CONTAINER_H_
#परिभाषय VERILATOR_V3CONTAINER_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

//============================================================================

// Similar to std::set, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Keys may not be modified.  (If needed in future, m_set could contain vector positions.)
प्रतिरूपम् <typename T_कुञ्चिका>
class VInsertion_समुच्चयः final {
    std::सदिशम्<T_कुञ्चिका> m_कुञ्चिकाः;  // Elements by insertion order
    std::unordered_समुच्चयः<T_कुञ्चिका> m_कुञ्चिका_समुच्चयः;  // Elements by key
सार्वजनिक:
    // METHODS
    bool निवेशय(ध्रुव T_कुञ्चिका& कुञ्चिका) {
        // Returns if did insertion (second pair argument of traditional emplace)
        ध्रुव स्वचालित it_प्राप्तम्_Pair = m_कुञ्चिका_समुच्चयः.निवेशय(कुञ्चिका);
        यदि (it_प्राप्तम्_Pair.second) m_कुञ्चिकाः.प्रक्षिप_पृष्ठभागः(कुञ्चिका);
        प्रत्यागम it_प्राप्तम्_Pair.second;
    }
    प्रतिरूपम् <typename T>
    शून्य निवेशय(T begin, ध्रुव T अन्तः_2) {
        static_assert(std::is_same<typename std::iterator_traits<T>::मानम्_type, T_कुञ्चिका>::मानम्,
                      "T must be an iterator with value type T_Key");
        यावत् (begin != अन्तः_2) {
            यदि (m_कुञ्चिका_समुच्चयः.निवेशय(*begin).second) m_कुञ्चिकाः.प्रक्षिप_पृष्ठभागः(*begin);
            ++begin;
        }
    }
    शून्य clear() {
        m_कुञ्चिकाः.clear();
        m_कुञ्चिका_समुच्चयः.clear();
    }

    // ACCESSORS
    bool रिक्तानि() ध्रुव { प्रत्यागम m_कुञ्चिकाः.रिक्तानि(); }
    आकार_t आकार() ध्रुव { प्रत्यागम m_कुञ्चिकाः.आकार(); }
    bool exists(ध्रुव T_कुञ्चिका& कुञ्चिका) ध्रुव { प्रत्यागम m_कुञ्चिका_समुच्चयः.find(कुञ्चिका) != m_कुञ्चिका_समुच्चयः.अन्तः_2(); }

    // ITERATORS
    using const_iterator = typename std::सदिशम्<T_कुञ्चिका>::const_iterator;
    const_iterator begin() ध्रुव { प्रत्यागम m_कुञ्चिकाः.begin(); }
    const_iterator अन्तः_2() ध्रुव { प्रत्यागम m_कुञ्चिकाः.अन्तः_2(); }
};

//============================================================================
// VInsertionMap: Not currently needed; prototype code exists, just ask.
// Similar to std::map, but ordered based on call order to emplace.  Used
// when insertion order is desired (e.g. std::vector), but duplicates need removal.
// Values may be modified.

#यदि_अन्तः  // Guard
