// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Doubly linked endogenous (intrusive) list
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

#यदि_अपरिभाषितम् VERILATOR_V3LIST_H_
#परिभाषय VERILATOR_V3LIST_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"

#अन्तर्भाव <type_traits>
#अन्तर्भाव <utility>

//============================================================================
// The list links (just 2 pointers), to be instantiated as a member in the
// list element base class 'T_Base'. They are considered mutable, even if the
// list element is 'const', as they are only used for storing the elements in
// a V3List. That is, you can store const elements in a V3List.
प्रतिरूपम् <typename T_आधारः>
class V3ListLinks final {
    // The V3List itself, but nothing else can access the link pointers
    प्रतिरूपम् <typename B, V3ListLinks<B>& (B::*)(), typename>
    मित्रम् class V3List;

    T_आधारः* m_nextp = nullptr;  // Next element in list
    T_आधारः* m_prevp = nullptr;  // Previous element in list

सार्वजनिक:
    V3ListLinks() = स्वभाव;
    ~V3ListLinks() {
#यदि_परिभाषितम् VL_DEBUG
        m_nextp = reinterpret_cast<T_आधारः*>(1);
        m_prevp = reinterpret_cast<T_आधारः*>(1);
#यदि_अन्तः
    }
    VL_UNCOPYABLE(V3ListLinks);
    VL_UNMOVABLE(V3ListLinks);
};

//============================================================================
// Generic endogenous (or intrusive) doubly linked list, with links stored
// inside the elements. The template parameters are:
// - 'T_Base' is the base type of elements that contains the V3ListLinks
//   instance as a data member.
// - 'LinksGetter' is a member function pointer that returns a reference to
//   the links within 'T_Base'. Note that you should be able to use a data
//   member pointer, instead of a function pointer, but that is buggy in MSVC.
// - 'T_Element' is the actual type of elements, which must be the same,
//    or a subtype of 'T_Base'.
प्रतिरूपम् <typename T_आधारः,  //
          V3ListLinks<T_आधारः>& (T_आधारः::*LinksGetter)(),  //
          typename T_Element = T_आधारः>
class V3List final {
    static_assert(std::is_आधारः_सम्बन्धि<T_आधारः, T_Element>::मानम्,
                  "'T_Element' must be a subtype of 'T_Base");

    using ListType = V3List<T_आधारः, LinksGetter, T_Element>;

    // MEMBERS
    T_आधारः* m_headp = nullptr;
    T_आधारः* m_lastp = nullptr;

    // Given the T_Base, return the Links. The links are always mutable, even in const elements.
    VL_ATTR_ALWINLINE
    स्थिर V3ListLinks<T_आधारः>& गन्तव्यम्_Links(ध्रुव T_आधारः* elementp) {
        प्रत्यागम (const_cast<T_आधारः*>(elementp)->*LinksGetter)();
    }

    VL_ATTR_ALWINLINE
    स्थिर शून्य prefetch(ध्रुव T_आधारः* elementp, ध्रुव T_आधारः* fallbackp) {
        UDEBUGONLY(UASSERT(fallbackp, "Prefetch fallback pointer must be non-nullptr"););
        // This compiles to a branchless prefetch with cmove, with the address always valid
        VL_PREFETCH_RW(elementp ? elementp : fallbackp);
    }

    // Iterator class template for V3List. This is just enough to support range based for loops
    // and basic usage. Feel free to extend as required.
    प्रतिरूपम् <typename T_IteratorElement, bool N_Reverse>
    class SimpleItertatorImpl final {
        static_assert(std::is_same<T_IteratorElement, T_Element>::मानम्
                          || std::is_same<T_IteratorElement, ध्रुव T_Element>::मानम्,
                      "'SimpleItertatorImpl' must be used with element type only");

        // The List itself, but nothing else can construct iterators
        प्रतिरूपम् <typename B, V3ListLinks<B>& (B::*)(), typename>
        मित्रम् class V3List;

        using IteratorType = SimpleItertatorImpl<T_IteratorElement, N_Reverse>;

        T_आधारः* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator

        VL_ATTR_ALWINLINE
        explicit SimpleItertatorImpl(T_आधारः* elementp)
            : m_currp{elementp} {}

        VL_ATTR_ALWINLINE
        स्थिर T_आधारः* चरणः(T_आधारः* currp) {
            यदि VL_CONSTEXPR_CXX17 (N_Reverse) {
                प्रत्यागम गन्तव्यम्_Links(currp).m_prevp;
            } अन्यथा {
                प्रत्यागम गन्तव्यम्_Links(currp).m_nextp;
            }
        }

    सार्वजनिक:
        // Dereference
        VL_ATTR_ALWINLINE
        T_IteratorElement& संक्रियकः*() ध्रुव {
            UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
            prefetch(चरणः(m_currp), m_currp);
            प्रत्यागम *static_cast<T_IteratorElement*>(m_currp);
        }
        // Pre increment
        VL_ATTR_ALWINLINE
        IteratorType& संक्रियकः++() {
            UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
            m_currp = चरणः(m_currp);
            प्रत्यागम *this;
        }
        // Post increment
        VL_ATTR_ALWINLINE
        IteratorType संक्रियकः++(पूर्णाङ्क) {
            UDEBUGONLY(UASSERT(m_currp, "Post-incrementing end of list iterator"););
            T_आधारः* ध्रुव elementp = m_currp;
            m_currp = चरणः(m_currp);
            प्रत्यागम IteratorType{elementp};
        }
        VL_ATTR_ALWINLINE
        bool संक्रियकः==(ध्रुव IteratorType& other) ध्रुव { प्रत्यागम m_currp == other.m_currp; }
        VL_ATTR_ALWINLINE
        bool संक्रियकः!=(ध्रुव IteratorType& other) ध्रुव { प्रत्यागम m_currp != other.m_currp; }
        // Convert to const iterator
        VL_ATTR_ALWINLINE
        संक्रियकः SimpleItertatorImpl<ध्रुव T_IteratorElement, N_Reverse>() ध्रुव {
            प्रत्यागम SimpleItertatorImpl<ध्रुव T_IteratorElement, N_Reverse>{m_currp};
        }
    };

    // Proxy class for creating unlinkable iterators, so we can use
    // 'for (T_Element* const ptr : list.unlinkable()) list.unlink(ptr);'
    class UnlinkableProxy final {
        // The List itself, but nothing else can construct UnlinkableProxy
        प्रतिरूपम् <typename B, V3ListLinks<B>& (B::*)(), typename>
        मित्रम् class V3List;

        ListType& m_list;  // The proxied list

        explicit UnlinkableProxy(ListType& list)
            : m_list{list} {}

        // Unlinkable iterator class template. This only supports enough for range based for loops.
        // If you want something fancier, use and manage the direct iterator manually.
        प्रतिरूपम् <typename T_IteratorElement>
        class UnlinkableItertatorImpl final {
            static_assert(std::is_same<T_IteratorElement, T_Element>::मानम्
                              || std::is_same<T_IteratorElement, ध्रुव T_Element>::मानम्,
                          "'UnlinkableItertatorImpl' must be used with element type only");

            // The UnlinkableProxy, but nothing else can construct unlinkable iterators
            मित्रम् class UnlinkableProxy;

            using IteratorType = UnlinkableItertatorImpl<T_IteratorElement>;

            T_आधारः* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator
            T_आधारः* m_nextp;  // Next element after current, or 'nullptr' for 'end()' iterator

            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(T_आधारः* elementp)
                : m_currp{elementp}
                , m_nextp{गन्तव्यम्_Links(m_currp).m_nextp} {}
            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(std::nullptr_t)
                : m_currp{nullptr}
                , m_nextp{nullptr} {}

        सार्वजनिक:
            // Dereference - Note this returns a pointer.
            VL_ATTR_ALWINLINE
            T_IteratorElement* संक्रियकः*() ध्रुव {
                UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
                prefetch(m_nextp, m_currp);
                प्रत्यागम static_cast<T_IteratorElement*>(m_currp);
            }
            // Pre increment - Keeps hold of current next pointer.
            VL_ATTR_ALWINLINE
            IteratorType& संक्रियकः++() {
                UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
                m_currp = m_nextp;
                m_nextp = m_currp ? गन्तव्यम्_Links(m_currp).m_nextp : nullptr;
                प्रत्यागम *this;
            }
            VL_ATTR_ALWINLINE
            bool संक्रियकः!=(ध्रुव IteratorType& other) ध्रुव { प्रत्यागम m_currp != other.m_currp; }
        };

    सार्वजनिक:
        using iterator = UnlinkableItertatorImpl<T_Element>;
        using const_iterator = UnlinkableItertatorImpl<ध्रुव T_Element>;
        iterator begin() {  //
            प्रत्यागम m_list.m_headp ? iterator{m_list.m_headp} : अन्तः_2();
        }
        const_iterator begin() ध्रुव {
            प्रत्यागम m_list.m_headp ? const_iterator{m_list.m_headp} : अन्तः_2();
        }
        iterator अन्तः_2() { प्रत्यागम iterator{nullptr}; }
        const_iterator अन्तः_2() ध्रुव { प्रत्यागम const_iterator{nullptr}; }
    };

सार्वजनिक:
    using iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ false>;
    using const_iterator = SimpleItertatorImpl<ध्रुव T_Element, /* N_Reverse: */ false>;
    using reverse_iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ true>;
    using const_reverse_iterator = SimpleItertatorImpl<ध्रुव T_Element, /* N_Reverse: */ true>;

    // CONSTRUCTOR
    V3List() = स्वभाव;
    ~V3List() {
#यदि_परिभाषितम् VL_DEBUG
        m_headp = reinterpret_cast<T_Element*>(1);
        m_lastp = reinterpret_cast<T_Element*>(1);
#यदि_अन्तः
    }
    VL_UNCOPYABLE(V3List);
    VL_UNMOVABLE(V3List);

    // METHDOS
    bool रिक्तानि() ध्रुव { प्रत्यागम !m_headp; }
    bool hasSingleElement() ध्रुव { प्रत्यागम m_headp && m_headp == m_lastp; }
    bool has_गुणजः_Elements() ध्रुव { प्रत्यागम m_headp && m_headp != m_lastp; }
    bool hasTwoElements() ध्रुव { प्रत्यागम m_headp && गन्तव्यम्_Links(m_headp).m_nextp == m_lastp; }

    // These return pointers, as we often want to unlink/delete them, and can also signal empty
    T_Element* frontp() { प्रत्यागम static_cast<T_Element*>(m_headp); }
    ध्रुव T_Element* frontp() ध्रुव { प्रत्यागम static_cast<T_Element*>(m_headp); }
    T_Element* backp() { प्रत्यागम static_cast<T_Element*>(m_lastp); }
    ध्रुव T_Element* backp() ध्रुव { प्रत्यागम static_cast<T_Element*>(m_lastp); }

    // Standard iterators. The iterator is only invalidated if the element it points to is
    // unlinked. Other list operations do not invalidate the itartor. If you want to be able to
    // unlink the currently iterated element, use 'unlinkable()' below.
    iterator begin() { प्रत्यागम iterator{m_headp}; }
    const_iterator begin() ध्रुव { प्रत्यागम const_iterator{m_headp}; }
    iterator अन्तः_2() { प्रत्यागम iterator{nullptr}; }
    const_iterator अन्तः_2() ध्रुव { प्रत्यागम const_iterator{nullptr}; }
    reverse_iterator rbegin() { प्रत्यागम reverse_iterator{m_lastp}; }
    const_reverse_iterator rbegin() ध्रुव { प्रत्यागम const_reverse_iterator{m_lastp}; }
    reverse_iterator rend() { प्रत्यागम reverse_iterator{nullptr}; }
    const_reverse_iterator rend() ध्रुव { प्रत्यागम const_reverse_iterator{nullptr}; }

    // Handle to create unlinkable iterators, which allows unlinking the currently iterated
    // element without invalidating the iterator. However, every other operation that mutates
    // the list invalidates the unlinkable iterator!
    UnlinkableProxy unlinkable() { प्रत्यागम UnlinkableProxy{*this}; }

    // Link (insert) existing element at front
    शून्य linkFront(ध्रुव T_Element* elementp) {
        स्वचालित& links = गन्तव्यम्_Links(elementp);
        links.m_nextp = m_headp;
        links.m_prevp = nullptr;
        यदि (m_headp) गन्तव्यम्_Links(m_headp).m_prevp = const_cast<T_Element*>(elementp);
        m_headp = const_cast<T_Element*>(elementp);
        यदि (!m_lastp) m_lastp = m_headp;
    }

    // Link (insert) existing element at back
    शून्य link_पृष्ठभागः(ध्रुव T_Element* elementp) {
        स्वचालित& links = गन्तव्यम्_Links(elementp);
        links.m_nextp = nullptr;
        links.m_prevp = m_lastp;
        यदि (m_lastp) गन्तव्यम्_Links(m_lastp).m_nextp = const_cast<T_Element*>(elementp);
        m_lastp = const_cast<T_Element*>(elementp);
        यदि (!m_headp) m_headp = m_lastp;
    }

    // Unlink (remove) and return element at the front. Returns 'nullptr' if the list is empty.
    T_Element* unlinkFront() {
        T_Element* ध्रुव headp = m_headp;
        यदि (headp) unlink(headp);
        प्रत्यागम headp;
    }

    // Unlink (remove) and return element at the back. Returns 'nullptr' if the list is empty.
    T_Element* unlink_पृष्ठभागः() {
        T_Element* ध्रुव lastp = m_lastp;
        यदि (lastp) unlink(lastp);
        प्रत्यागम lastp;
    }

    // Unlink (remove) the given element.
    शून्य unlink(ध्रुव T_Element* elementp) {
        स्वचालित& links = गन्तव्यम्_Links(elementp);
        यदि (links.m_nextp) गन्तव्यम्_Links(links.m_nextp).m_prevp = links.m_prevp;
        यदि (links.m_prevp) गन्तव्यम्_Links(links.m_prevp).m_nextp = links.m_nextp;
        यदि (m_headp == elementp) m_headp = links.m_nextp;
        यदि (m_lastp == elementp) m_lastp = links.m_prevp;
        links.m_prevp = nullptr;
        links.m_nextp = nullptr;
    }

    // Swap elements of 2 lists
    शून्य swap(ListType& other) {
        std::swap(m_headp, other.m_headp);
        std::swap(m_lastp, other.m_lastp);
    }

    // Take elements from 'other' and link (insert) them all before the given position.
    शून्य splice(const_iterator pos, ListType& other) {
        यदि (रिक्तानि()) {
            swap(other);
        } अन्यथा यदि (other.रिक्तानि()) {
            प्रत्यागम;
        } अन्यथा {
            UASSERT(pos == अन्तः_2(), "Sorry, only splicing at the end is implemented at the moment");
            गन्तव्यम्_Links(m_lastp).m_nextp = other.m_headp;
            गन्तव्यम्_Links(other.m_headp).m_prevp = m_lastp;
            m_lastp = other.m_lastp;
            other.m_headp = nullptr;
            other.m_lastp = nullptr;
        }
    }

    // This is O(n)!
    आकार_t आकार() ध्रुव {
        आकार_t परिणाम = 0;
        प्रति (स्वचालित it = begin(); it != अन्तः_2(); ++it) ++परिणाम;
        प्रत्यागम परिणाम;
    }
};

#यदि_अन्तः  // Guard
