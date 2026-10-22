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

#اگر_غیر_تعریف_شدہ VERILATOR_V3LIST_H_
#تعریف_کرو VERILATOR_V3LIST_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"

#شامل <type_traits>
#شامل <utility>

//============================================================================
// The list links (just 2 pointers), to be instantiated as a member in the
// list element base class 'T_Base'. They are considered mutable, even if the
// list element is 'const', as they are only used for storing the elements in
// a V3List. That is, you can store const elements in a V3List.
سانچہ <typename T_بنیاد>
class V3ListLinks final {
    // The V3List itself, but nothing else can access the link pointers
    سانچہ <typename B, V3ListLinks<B>& (B::*)(), typename>
    دوست class V3List;

    T_بنیاد* m_nextp = nullptr;  // Next element in list
    T_بنیاد* m_prevp = nullptr;  // Previous element in list

عوامی:
    V3ListLinks() = طےشدہ;
    ~V3ListLinks() {
#اگر_تعریف_شدہ VL_DEBUG
        m_nextp = reinterpret_cast<T_بنیاد*>(1);
        m_prevp = reinterpret_cast<T_بنیاد*>(1);
#اگر_ختم
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
سانچہ <typename T_بنیاد,  //
          V3ListLinks<T_بنیاد>& (T_بنیاد::*LinksGetter)(),  //
          typename T_Element = T_بنیاد>
class V3List final {
    static_assert(std::is_بنیاد_کا<T_بنیاد, T_Element>::قدر,
                  "'T_Element' must be a subtype of 'T_Base");

    using ListType = V3List<T_بنیاد, LinksGetter, T_Element>;

    // MEMBERS
    T_بنیاد* m_headp = nullptr;
    T_بنیاد* m_lastp = nullptr;

    // Given the T_Base, return the Links. The links are always mutable, even in const elements.
    VL_ATTR_ALWINLINE
    ساکن V3ListLinks<T_بنیاد>& مقصد_Links(ثابت T_بنیاد* elementp) {
        واپس (const_cast<T_بنیاد*>(elementp)->*LinksGetter)();
    }

    VL_ATTR_ALWINLINE
    ساکن خالی prefetch(ثابت T_بنیاد* elementp, ثابت T_بنیاد* fallbackp) {
        UDEBUGONLY(UASSERT(fallbackp, "Prefetch fallback pointer must be non-nullptr"););
        // This compiles to a branchless prefetch with cmove, with the address always valid
        VL_PREFETCH_RW(elementp ? elementp : fallbackp);
    }

    // Iterator class template for V3List. This is just enough to support range based for loops
    // and basic usage. Feel free to extend as required.
    سانچہ <typename T_IteratorElement, bool N_الٹ>
    class SimpleItertatorImpl final {
        static_assert(std::is_same<T_IteratorElement, T_Element>::قدر
                          || std::is_same<T_IteratorElement, ثابت T_Element>::قدر,
                      "'SimpleItertatorImpl' must be used with element type only");

        // The List itself, but nothing else can construct iterators
        سانچہ <typename B, V3ListLinks<B>& (B::*)(), typename>
        دوست class V3List;

        using IteratorType = SimpleItertatorImpl<T_IteratorElement, N_الٹ>;

        T_بنیاد* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator

        VL_ATTR_ALWINLINE
        explicit SimpleItertatorImpl(T_بنیاد* elementp)
            : m_currp{elementp} {}

        VL_ATTR_ALWINLINE
        ساکن T_بنیاد* قدم(T_بنیاد* currp) {
            اگر VL_CONSTEXPR_CXX17 (N_الٹ) {
                واپس مقصد_Links(currp).m_prevp;
            } ورنہ {
                واپس مقصد_Links(currp).m_nextp;
            }
        }

    عوامی:
        // Dereference
        VL_ATTR_ALWINLINE
        T_IteratorElement& عامل*() ثابت {
            UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
            prefetch(قدم(m_currp), m_currp);
            واپس *static_cast<T_IteratorElement*>(m_currp);
        }
        // Pre increment
        VL_ATTR_ALWINLINE
        IteratorType& عامل++() {
            UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
            m_currp = قدم(m_currp);
            واپس *this;
        }
        // Post increment
        VL_ATTR_ALWINLINE
        IteratorType عامل++(صحیح) {
            UDEBUGONLY(UASSERT(m_currp, "Post-incrementing end of list iterator"););
            T_بنیاد* ثابت elementp = m_currp;
            m_currp = قدم(m_currp);
            واپس IteratorType{elementp};
        }
        VL_ATTR_ALWINLINE
        bool عامل==(ثابت IteratorType& other) ثابت { واپس m_currp == other.m_currp; }
        VL_ATTR_ALWINLINE
        bool عامل!=(ثابت IteratorType& other) ثابت { واپس m_currp != other.m_currp; }
        // Convert to const iterator
        VL_ATTR_ALWINLINE
        عامل SimpleItertatorImpl<ثابت T_IteratorElement, N_الٹ>() ثابت {
            واپس SimpleItertatorImpl<ثابت T_IteratorElement, N_الٹ>{m_currp};
        }
    };

    // Proxy class for creating unlinkable iterators, so we can use
    // 'for (T_Element* const ptr : list.unlinkable()) list.unlink(ptr);'
    class UnlinkableProxy final {
        // The List itself, but nothing else can construct UnlinkableProxy
        سانچہ <typename B, V3ListLinks<B>& (B::*)(), typename>
        دوست class V3List;

        ListType& m_list;  // The proxied list

        explicit UnlinkableProxy(ListType& list)
            : m_list{list} {}

        // Unlinkable iterator class template. This only supports enough for range based for loops.
        // If you want something fancier, use and manage the direct iterator manually.
        سانچہ <typename T_IteratorElement>
        class UnlinkableItertatorImpl final {
            static_assert(std::is_same<T_IteratorElement, T_Element>::قدر
                              || std::is_same<T_IteratorElement, ثابت T_Element>::قدر,
                          "'UnlinkableItertatorImpl' must be used with element type only");

            // The UnlinkableProxy, but nothing else can construct unlinkable iterators
            دوست class UnlinkableProxy;

            using IteratorType = UnlinkableItertatorImpl<T_IteratorElement>;

            T_بنیاد* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator
            T_بنیاد* m_nextp;  // Next element after current, or 'nullptr' for 'end()' iterator

            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(T_بنیاد* elementp)
                : m_currp{elementp}
                , m_nextp{مقصد_Links(m_currp).m_nextp} {}
            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(std::nullptr_t)
                : m_currp{nullptr}
                , m_nextp{nullptr} {}

        عوامی:
            // Dereference - Note this returns a pointer.
            VL_ATTR_ALWINLINE
            T_IteratorElement* عامل*() ثابت {
                UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
                prefetch(m_nextp, m_currp);
                واپس static_cast<T_IteratorElement*>(m_currp);
            }
            // Pre increment - Keeps hold of current next pointer.
            VL_ATTR_ALWINLINE
            IteratorType& عامل++() {
                UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
                m_currp = m_nextp;
                m_nextp = m_currp ? مقصد_Links(m_currp).m_nextp : nullptr;
                واپس *this;
            }
            VL_ATTR_ALWINLINE
            bool عامل!=(ثابت IteratorType& other) ثابت { واپس m_currp != other.m_currp; }
        };

    عوامی:
        using iterator = UnlinkableItertatorImpl<T_Element>;
        using const_iterator = UnlinkableItertatorImpl<ثابت T_Element>;
        iterator begin() {  //
            واپس m_list.m_headp ? iterator{m_list.m_headp} : انتہا();
        }
        const_iterator begin() ثابت {
            واپس m_list.m_headp ? const_iterator{m_list.m_headp} : انتہا();
        }
        iterator انتہا() { واپس iterator{nullptr}; }
        const_iterator انتہا() ثابت { واپس const_iterator{nullptr}; }
    };

عوامی:
    using iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ false>;
    using const_iterator = SimpleItertatorImpl<ثابت T_Element, /* N_Reverse: */ false>;
    using الٹ_iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ true>;
    using const_الٹ_iterator = SimpleItertatorImpl<ثابت T_Element, /* N_Reverse: */ true>;

    // CONSTRUCTOR
    V3List() = طےشدہ;
    ~V3List() {
#اگر_تعریف_شدہ VL_DEBUG
        m_headp = reinterpret_cast<T_Element*>(1);
        m_lastp = reinterpret_cast<T_Element*>(1);
#اگر_ختم
    }
    VL_UNCOPYABLE(V3List);
    VL_UNMOVABLE(V3List);

    // METHDOS
    bool خالی_خانے() ثابت { واپس !m_headp; }
    bool hasSingleElement() ثابت { واپس m_headp && m_headp == m_lastp; }
    bool has_مضرب_Elements() ثابت { واپس m_headp && m_headp != m_lastp; }
    bool hasTwoElements() ثابت { واپس m_headp && مقصد_Links(m_headp).m_nextp == m_lastp; }

    // These return pointers, as we often want to unlink/delete them, and can also signal empty
    T_Element* frontp() { واپس static_cast<T_Element*>(m_headp); }
    ثابت T_Element* frontp() ثابت { واپس static_cast<T_Element*>(m_headp); }
    T_Element* backp() { واپس static_cast<T_Element*>(m_lastp); }
    ثابت T_Element* backp() ثابت { واپس static_cast<T_Element*>(m_lastp); }

    // Standard iterators. The iterator is only invalidated if the element it points to is
    // unlinked. Other list operations do not invalidate the itartor. If you want to be able to
    // unlink the currently iterated element, use 'unlinkable()' below.
    iterator begin() { واپس iterator{m_headp}; }
    const_iterator begin() ثابت { واپس const_iterator{m_headp}; }
    iterator انتہا() { واپس iterator{nullptr}; }
    const_iterator انتہا() ثابت { واپس const_iterator{nullptr}; }
    الٹ_iterator rbegin() { واپس الٹ_iterator{m_lastp}; }
    const_الٹ_iterator rbegin() ثابت { واپس const_الٹ_iterator{m_lastp}; }
    الٹ_iterator rend() { واپس الٹ_iterator{nullptr}; }
    const_الٹ_iterator rend() ثابت { واپس const_الٹ_iterator{nullptr}; }

    // Handle to create unlinkable iterators, which allows unlinking the currently iterated
    // element without invalidating the iterator. However, every other operation that mutates
    // the list invalidates the unlinkable iterator!
    UnlinkableProxy unlinkable() { واپس UnlinkableProxy{*this}; }

    // Link (insert) existing element at front
    خالی linkFront(ثابت T_Element* elementp) {
        خودکار& links = مقصد_Links(elementp);
        links.m_nextp = m_headp;
        links.m_prevp = nullptr;
        اگر (m_headp) مقصد_Links(m_headp).m_prevp = const_cast<T_Element*>(elementp);
        m_headp = const_cast<T_Element*>(elementp);
        اگر (!m_lastp) m_lastp = m_headp;
    }

    // Link (insert) existing element at back
    خالی link_عقب(ثابت T_Element* elementp) {
        خودکار& links = مقصد_Links(elementp);
        links.m_nextp = nullptr;
        links.m_prevp = m_lastp;
        اگر (m_lastp) مقصد_Links(m_lastp).m_nextp = const_cast<T_Element*>(elementp);
        m_lastp = const_cast<T_Element*>(elementp);
        اگر (!m_headp) m_headp = m_lastp;
    }

    // Unlink (remove) and return element at the front. Returns 'nullptr' if the list is empty.
    T_Element* unlinkFront() {
        T_Element* ثابت headp = m_headp;
        اگر (headp) unlink(headp);
        واپس headp;
    }

    // Unlink (remove) and return element at the back. Returns 'nullptr' if the list is empty.
    T_Element* unlink_عقب() {
        T_Element* ثابت lastp = m_lastp;
        اگر (lastp) unlink(lastp);
        واپس lastp;
    }

    // Unlink (remove) the given element.
    خالی unlink(ثابت T_Element* elementp) {
        خودکار& links = مقصد_Links(elementp);
        اگر (links.m_nextp) مقصد_Links(links.m_nextp).m_prevp = links.m_prevp;
        اگر (links.m_prevp) مقصد_Links(links.m_prevp).m_nextp = links.m_nextp;
        اگر (m_headp == elementp) m_headp = links.m_nextp;
        اگر (m_lastp == elementp) m_lastp = links.m_prevp;
        links.m_prevp = nullptr;
        links.m_nextp = nullptr;
    }

    // Swap elements of 2 lists
    خالی تبادلہ(ListType& other) {
        std::تبادلہ(m_headp, other.m_headp);
        std::تبادلہ(m_lastp, other.m_lastp);
    }

    // Take elements from 'other' and link (insert) them all before the given position.
    خالی splice(const_iterator pos, ListType& other) {
        اگر (خالی_خانے()) {
            تبادلہ(other);
        } ورنہ اگر (other.خالی_خانے()) {
            واپس;
        } ورنہ {
            UASSERT(pos == انتہا(), "Sorry, only splicing at the end is implemented at the moment");
            مقصد_Links(m_lastp).m_nextp = other.m_headp;
            مقصد_Links(other.m_headp).m_prevp = m_lastp;
            m_lastp = other.m_lastp;
            other.m_headp = nullptr;
            other.m_lastp = nullptr;
        }
    }

    // This is O(n)!
    سائز_t سائز() ثابت {
        سائز_t نتیجہ = 0;
        برائے (خودکار it = begin(); it != انتہا(); ++it) ++نتیجہ;
        واپس نتیجہ;
    }
};

#اگر_ختم  // Guard
