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

#جے_بنا_تعریف VERILATOR_V3LIST_H_
#تعریف_کرو VERILATOR_V3LIST_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"

#ضمن <type_traits>
#ضمن <utility>

//============================================================================
// The list links (just 2 pointers), to be instantiated as a member in the
// list element base class 'T_Base'. They are considered mutable, even if the
// list element is 'const', as they are only used for storing the elements in
// a V3List. That is, you can store const elements in a V3List.
سانچا <typename T_ادھار>
class V3ListLinks final {
    // The V3List itself, but nothing else can access the link pointers
    سانچا <typename B, V3ListLinks<B>& (B::*)(), typename>
    یار class V3List;

    T_ادھار* m_nextp = nullptr;  // Next element in list
    T_ادھار* m_prevp = nullptr;  // Previous element in list

عام:
    V3ListLinks() = افتراضي;
    ~V3ListLinks() {
#جے_تعریف_شدہ VL_DEBUG
        m_nextp = reinterpret_cast<T_ادھار*>(1);
        m_prevp = reinterpret_cast<T_ادھار*>(1);
#جے_انت
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
سانچا <typename T_ادھار,  //
          V3ListLinks<T_ادھار>& (T_ادھار::*LinksGetter)(),  //
          typename T_Element = T_ادھار>
class V3List final {
    static_assert(std::is_ادھار_دا<T_ادھار, T_Element>::قدر,
                  "'T_Element' must be a subtype of 'T_Base");

    using ListType = V3List<T_ادھار, LinksGetter, T_Element>;

    // MEMBERS
    T_ادھار* m_headp = nullptr;
    T_ادھار* m_lastp = nullptr;

    // Given the T_Base, return the Links. The links are always mutable, even in const elements.
    VL_ATTR_ALWINLINE
    ساكن V3ListLinks<T_ادھار>& منزل_Links(ثابت T_ادھار* elementp) {
        أرجع (const_cast<T_ادھار*>(elementp)->*LinksGetter)();
    }

    VL_ATTR_ALWINLINE
    ساكن فراغ prefetch(ثابت T_ادھار* elementp, ثابت T_ادھار* fallbackp) {
        UDEBUGONLY(UASSERT(fallbackp, "Prefetch fallback pointer must be non-nullptr"););
        // This compiles to a branchless prefetch with cmove, with the address always valid
        VL_PREFETCH_RW(elementp ? elementp : fallbackp);
    }

    // Iterator class template for V3List. This is just enough to support range based for loops
    // and basic usage. Feel free to extend as required.
    سانچا <typename T_IteratorElement, bool N_الٹ>
    class SimpleItertatorImpl final {
        static_assert(std::is_same<T_IteratorElement, T_Element>::قدر
                          || std::is_same<T_IteratorElement, ثابت T_Element>::قدر,
                      "'SimpleItertatorImpl' must be used with element type only");

        // The List itself, but nothing else can construct iterators
        سانچا <typename B, V3ListLinks<B>& (B::*)(), typename>
        یار class V3List;

        using IteratorType = SimpleItertatorImpl<T_IteratorElement, N_الٹ>;

        T_ادھار* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator

        VL_ATTR_ALWINLINE
        explicit SimpleItertatorImpl(T_ادھار* elementp)
            : m_currp{elementp} {}

        VL_ATTR_ALWINLINE
        ساكن T_ادھار* قدم(T_ادھار* currp) {
            إذا VL_CONSTEXPR_CXX17 (N_الٹ) {
                أرجع منزل_Links(currp).m_prevp;
            } وإلا {
                أرجع منزل_Links(currp).m_nextp;
            }
        }

    عام:
        // Dereference
        VL_ATTR_ALWINLINE
        T_IteratorElement& عامل*() ثابت {
            UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
            prefetch(قدم(m_currp), m_currp);
            أرجع *static_cast<T_IteratorElement*>(m_currp);
        }
        // Pre increment
        VL_ATTR_ALWINLINE
        IteratorType& عامل++() {
            UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
            m_currp = قدم(m_currp);
            أرجع *this;
        }
        // Post increment
        VL_ATTR_ALWINLINE
        IteratorType عامل++(صحيح) {
            UDEBUGONLY(UASSERT(m_currp, "Post-incrementing end of list iterator"););
            T_ادھار* ثابت elementp = m_currp;
            m_currp = قدم(m_currp);
            أرجع IteratorType{elementp};
        }
        VL_ATTR_ALWINLINE
        bool عامل==(ثابت IteratorType& other) ثابت { أرجع m_currp == other.m_currp; }
        VL_ATTR_ALWINLINE
        bool عامل!=(ثابت IteratorType& other) ثابت { أرجع m_currp != other.m_currp; }
        // Convert to const iterator
        VL_ATTR_ALWINLINE
        عامل SimpleItertatorImpl<ثابت T_IteratorElement, N_الٹ>() ثابت {
            أرجع SimpleItertatorImpl<ثابت T_IteratorElement, N_الٹ>{m_currp};
        }
    };

    // Proxy class for creating unlinkable iterators, so we can use
    // 'for (T_Element* const ptr : list.unlinkable()) list.unlink(ptr);'
    class UnlinkableProxy final {
        // The List itself, but nothing else can construct UnlinkableProxy
        سانچا <typename B, V3ListLinks<B>& (B::*)(), typename>
        یار class V3List;

        ListType& m_list;  // The proxied list

        explicit UnlinkableProxy(ListType& list)
            : m_list{list} {}

        // Unlinkable iterator class template. This only supports enough for range based for loops.
        // If you want something fancier, use and manage the direct iterator manually.
        سانچا <typename T_IteratorElement>
        class UnlinkableItertatorImpl final {
            static_assert(std::is_same<T_IteratorElement, T_Element>::قدر
                              || std::is_same<T_IteratorElement, ثابت T_Element>::قدر,
                          "'UnlinkableItertatorImpl' must be used with element type only");

            // The UnlinkableProxy, but nothing else can construct unlinkable iterators
            یار class UnlinkableProxy;

            using IteratorType = UnlinkableItertatorImpl<T_IteratorElement>;

            T_ادھار* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator
            T_ادھار* m_nextp;  // Next element after current, or 'nullptr' for 'end()' iterator

            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(T_ادھار* elementp)
                : m_currp{elementp}
                , m_nextp{منزل_Links(m_currp).m_nextp} {}
            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(std::nullptr_t)
                : m_currp{nullptr}
                , m_nextp{nullptr} {}

        عام:
            // Dereference - Note this returns a pointer.
            VL_ATTR_ALWINLINE
            T_IteratorElement* عامل*() ثابت {
                UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
                prefetch(m_nextp, m_currp);
                أرجع static_cast<T_IteratorElement*>(m_currp);
            }
            // Pre increment - Keeps hold of current next pointer.
            VL_ATTR_ALWINLINE
            IteratorType& عامل++() {
                UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
                m_currp = m_nextp;
                m_nextp = m_currp ? منزل_Links(m_currp).m_nextp : nullptr;
                أرجع *this;
            }
            VL_ATTR_ALWINLINE
            bool عامل!=(ثابت IteratorType& other) ثابت { أرجع m_currp != other.m_currp; }
        };

    عام:
        using iterator = UnlinkableItertatorImpl<T_Element>;
        using const_iterator = UnlinkableItertatorImpl<ثابت T_Element>;
        iterator begin() {  //
            أرجع m_list.m_headp ? iterator{m_list.m_headp} : سرا();
        }
        const_iterator begin() ثابت {
            أرجع m_list.m_headp ? const_iterator{m_list.m_headp} : سرا();
        }
        iterator سرا() { أرجع iterator{nullptr}; }
        const_iterator سرا() ثابت { أرجع const_iterator{nullptr}; }
    };

عام:
    using iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ false>;
    using const_iterator = SimpleItertatorImpl<ثابت T_Element, /* N_Reverse: */ false>;
    using الٹ_iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ true>;
    using const_الٹ_iterator = SimpleItertatorImpl<ثابت T_Element, /* N_Reverse: */ true>;

    // CONSTRUCTOR
    V3List() = افتراضي;
    ~V3List() {
#جے_تعریف_شدہ VL_DEBUG
        m_headp = reinterpret_cast<T_Element*>(1);
        m_lastp = reinterpret_cast<T_Element*>(1);
#جے_انت
    }
    VL_UNCOPYABLE(V3List);
    VL_UNMOVABLE(V3List);

    // METHDOS
    bool خالی_تھاواں() ثابت { أرجع !m_headp; }
    bool hasSingleElement() ثابت { أرجع m_headp && m_headp == m_lastp; }
    bool has_مضرب_Elements() ثابت { أرجع m_headp && m_headp != m_lastp; }
    bool hasTwoElements() ثابت { أرجع m_headp && منزل_Links(m_headp).m_nextp == m_lastp; }

    // These return pointers, as we often want to unlink/delete them, and can also signal empty
    T_Element* frontp() { أرجع static_cast<T_Element*>(m_headp); }
    ثابت T_Element* frontp() ثابت { أرجع static_cast<T_Element*>(m_headp); }
    T_Element* backp() { أرجع static_cast<T_Element*>(m_lastp); }
    ثابت T_Element* backp() ثابت { أرجع static_cast<T_Element*>(m_lastp); }

    // Standard iterators. The iterator is only invalidated if the element it points to is
    // unlinked. Other list operations do not invalidate the itartor. If you want to be able to
    // unlink the currently iterated element, use 'unlinkable()' below.
    iterator begin() { أرجع iterator{m_headp}; }
    const_iterator begin() ثابت { أرجع const_iterator{m_headp}; }
    iterator سرا() { أرجع iterator{nullptr}; }
    const_iterator سرا() ثابت { أرجع const_iterator{nullptr}; }
    الٹ_iterator rbegin() { أرجع الٹ_iterator{m_lastp}; }
    const_الٹ_iterator rbegin() ثابت { أرجع const_الٹ_iterator{m_lastp}; }
    الٹ_iterator rend() { أرجع الٹ_iterator{nullptr}; }
    const_الٹ_iterator rend() ثابت { أرجع const_الٹ_iterator{nullptr}; }

    // Handle to create unlinkable iterators, which allows unlinking the currently iterated
    // element without invalidating the iterator. However, every other operation that mutates
    // the list invalidates the unlinkable iterator!
    UnlinkableProxy unlinkable() { أرجع UnlinkableProxy{*this}; }

    // Link (insert) existing element at front
    فراغ linkFront(ثابت T_Element* elementp) {
        خودکار& links = منزل_Links(elementp);
        links.m_nextp = m_headp;
        links.m_prevp = nullptr;
        إذا (m_headp) منزل_Links(m_headp).m_prevp = const_cast<T_Element*>(elementp);
        m_headp = const_cast<T_Element*>(elementp);
        إذا (!m_lastp) m_lastp = m_headp;
    }

    // Link (insert) existing element at back
    فراغ link_پٹھ(ثابت T_Element* elementp) {
        خودکار& links = منزل_Links(elementp);
        links.m_nextp = nullptr;
        links.m_prevp = m_lastp;
        إذا (m_lastp) منزل_Links(m_lastp).m_nextp = const_cast<T_Element*>(elementp);
        m_lastp = const_cast<T_Element*>(elementp);
        إذا (!m_headp) m_headp = m_lastp;
    }

    // Unlink (remove) and return element at the front. Returns 'nullptr' if the list is empty.
    T_Element* unlinkFront() {
        T_Element* ثابت headp = m_headp;
        إذا (headp) unlink(headp);
        أرجع headp;
    }

    // Unlink (remove) and return element at the back. Returns 'nullptr' if the list is empty.
    T_Element* unlink_پٹھ() {
        T_Element* ثابت lastp = m_lastp;
        إذا (lastp) unlink(lastp);
        أرجع lastp;
    }

    // Unlink (remove) the given element.
    فراغ unlink(ثابت T_Element* elementp) {
        خودکار& links = منزل_Links(elementp);
        إذا (links.m_nextp) منزل_Links(links.m_nextp).m_prevp = links.m_prevp;
        إذا (links.m_prevp) منزل_Links(links.m_prevp).m_nextp = links.m_nextp;
        إذا (m_headp == elementp) m_headp = links.m_nextp;
        إذا (m_lastp == elementp) m_lastp = links.m_prevp;
        links.m_prevp = nullptr;
        links.m_nextp = nullptr;
    }

    // Swap elements of 2 lists
    فراغ وٹاندرا(ListType& other) {
        std::وٹاندرا(m_headp, other.m_headp);
        std::وٹاندرا(m_lastp, other.m_lastp);
    }

    // Take elements from 'other' and link (insert) them all before the given position.
    فراغ splice(const_iterator pos, ListType& other) {
        إذا (خالی_تھاواں()) {
            وٹاندرا(other);
        } وإلا إذا (other.خالی_تھاواں()) {
            أرجع;
        } وإلا {
            UASSERT(pos == سرا(), "Sorry, only splicing at the end is implemented at the moment");
            منزل_Links(m_lastp).m_nextp = other.m_headp;
            منزل_Links(other.m_headp).m_prevp = m_lastp;
            m_lastp = other.m_lastp;
            other.m_headp = nullptr;
            other.m_lastp = nullptr;
        }
    }

    // This is O(n)!
    ناپ_t ناپ() ثابت {
        ناپ_t نتیجہ = 0;
        لكل (خودکار it = begin(); it != سرا(); ++it) ++نتیجہ;
        أرجع نتیجہ;
    }
};

#جے_انت  // Guard
