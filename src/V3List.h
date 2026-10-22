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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3LIST_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3LIST_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>

//============================================================================
// The list links (just 2 pointers), to be instantiated as a member in the
// list element base class 'T_Base'. They are considered mutable, even if the
// list element is 'const', as they are only used for storing the elements in
// a V3List. That is, you can store const elements in a V3List.
ਸਾਂਚਾ <typename T_ਅਧਾਰ>
class V3ListLinks final {
    // The V3List itself, but nothing else can access the link pointers
    ਸਾਂਚਾ <typename B, V3ListLinks<B>& (B::*)(), typename>
    ਮਿੱਤਰ class V3List;

    T_ਅਧਾਰ* m_nextp = nullptr;  // Next element in list
    T_ਅਧਾਰ* m_prevp = nullptr;  // Previous element in list

ਜਨਤਕ:
    V3ListLinks() = ਮੂਲ;
    ~V3ListLinks() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        m_nextp = reinterpret_cast<T_ਅਧਾਰ*>(1);
        m_prevp = reinterpret_cast<T_ਅਧਾਰ*>(1);
#ਜੇ_ਅੰਤ
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
ਸਾਂਚਾ <typename T_ਅਧਾਰ,  //
          V3ListLinks<T_ਅਧਾਰ>& (T_ਅਧਾਰ::*LinksGetter)(),  //
          typename T_Element = T_ਅਧਾਰ>
class V3List final {
    static_assert(std::is_ਅਧਾਰ_ਦਾ<T_ਅਧਾਰ, T_Element>::ਮੁੱਲ,
                  "'T_Element' must be a subtype of 'T_Base");

    using ListType = V3List<T_ਅਧਾਰ, LinksGetter, T_Element>;

    // MEMBERS
    T_ਅਧਾਰ* m_headp = nullptr;
    T_ਅਧਾਰ* m_lastp = nullptr;

    // Given the T_Base, return the Links. The links are always mutable, even in const elements.
    VL_ATTR_ALWINLINE
    ਸਥਿਰ V3ListLinks<T_ਅਧਾਰ>& ਮੰਜ਼ਿਲ_Links(ਅਚਲ T_ਅਧਾਰ* elementp) {
        ਵਾਪਸ (const_cast<T_ਅਧਾਰ*>(elementp)->*LinksGetter)();
    }

    VL_ATTR_ALWINLINE
    ਸਥਿਰ ਖਾਲੀ prefetch(ਅਚਲ T_ਅਧਾਰ* elementp, ਅਚਲ T_ਅਧਾਰ* fallbackp) {
        UDEBUGONLY(UASSERT(fallbackp, "Prefetch fallback pointer must be non-nullptr"););
        // This compiles to a branchless prefetch with cmove, with the address always valid
        VL_PREFETCH_RW(elementp ? elementp : fallbackp);
    }

    // Iterator class template for V3List. This is just enough to support range based for loops
    // and basic usage. Feel free to extend as required.
    ਸਾਂਚਾ <typename T_IteratorElement, bool N_ਉਲਟ>
    class SimpleItertatorImpl final {
        static_assert(std::is_same<T_IteratorElement, T_Element>::ਮੁੱਲ
                          || std::is_same<T_IteratorElement, ਅਚਲ T_Element>::ਮੁੱਲ,
                      "'SimpleItertatorImpl' must be used with element type only");

        // The List itself, but nothing else can construct iterators
        ਸਾਂਚਾ <typename B, V3ListLinks<B>& (B::*)(), typename>
        ਮਿੱਤਰ class V3List;

        using IteratorType = SimpleItertatorImpl<T_IteratorElement, N_ਉਲਟ>;

        T_ਅਧਾਰ* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator

        VL_ATTR_ALWINLINE
        explicit SimpleItertatorImpl(T_ਅਧਾਰ* elementp)
            : m_currp{elementp} {}

        VL_ATTR_ALWINLINE
        ਸਥਿਰ T_ਅਧਾਰ* ਕਦਮ(T_ਅਧਾਰ* currp) {
            ਜੇ VL_CONSTEXPR_CXX17 (N_ਉਲਟ) {
                ਵਾਪਸ ਮੰਜ਼ਿਲ_Links(currp).m_prevp;
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਮੰਜ਼ਿਲ_Links(currp).m_nextp;
            }
        }

    ਜਨਤਕ:
        // Dereference
        VL_ATTR_ALWINLINE
        T_IteratorElement& ਸੰਚਾਲਕ*() ਅਚਲ {
            UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
            prefetch(ਕਦਮ(m_currp), m_currp);
            ਵਾਪਸ *static_cast<T_IteratorElement*>(m_currp);
        }
        // Pre increment
        VL_ATTR_ALWINLINE
        IteratorType& ਸੰਚਾਲਕ++() {
            UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
            m_currp = ਕਦਮ(m_currp);
            ਵਾਪਸ *this;
        }
        // Post increment
        VL_ATTR_ALWINLINE
        IteratorType ਸੰਚਾਲਕ++(ਪੂਰਨ) {
            UDEBUGONLY(UASSERT(m_currp, "Post-incrementing end of list iterator"););
            T_ਅਧਾਰ* ਅਚਲ elementp = m_currp;
            m_currp = ਕਦਮ(m_currp);
            ਵਾਪਸ IteratorType{elementp};
        }
        VL_ATTR_ALWINLINE
        bool ਸੰਚਾਲਕ==(ਅਚਲ IteratorType& other) ਅਚਲ { ਵਾਪਸ m_currp == other.m_currp; }
        VL_ATTR_ALWINLINE
        bool ਸੰਚਾਲਕ!=(ਅਚਲ IteratorType& other) ਅਚਲ { ਵਾਪਸ m_currp != other.m_currp; }
        // Convert to const iterator
        VL_ATTR_ALWINLINE
        ਸੰਚਾਲਕ SimpleItertatorImpl<ਅਚਲ T_IteratorElement, N_ਉਲਟ>() ਅਚਲ {
            ਵਾਪਸ SimpleItertatorImpl<ਅਚਲ T_IteratorElement, N_ਉਲਟ>{m_currp};
        }
    };

    // Proxy class for creating unlinkable iterators, so we can use
    // 'for (T_Element* const ptr : list.unlinkable()) list.unlink(ptr);'
    class UnlinkableProxy final {
        // The List itself, but nothing else can construct UnlinkableProxy
        ਸਾਂਚਾ <typename B, V3ListLinks<B>& (B::*)(), typename>
        ਮਿੱਤਰ class V3List;

        ListType& m_list;  // The proxied list

        explicit UnlinkableProxy(ListType& list)
            : m_list{list} {}

        // Unlinkable iterator class template. This only supports enough for range based for loops.
        // If you want something fancier, use and manage the direct iterator manually.
        ਸਾਂਚਾ <typename T_IteratorElement>
        class UnlinkableItertatorImpl final {
            static_assert(std::is_same<T_IteratorElement, T_Element>::ਮੁੱਲ
                              || std::is_same<T_IteratorElement, ਅਚਲ T_Element>::ਮੁੱਲ,
                          "'UnlinkableItertatorImpl' must be used with element type only");

            // The UnlinkableProxy, but nothing else can construct unlinkable iterators
            ਮਿੱਤਰ class UnlinkableProxy;

            using IteratorType = UnlinkableItertatorImpl<T_IteratorElement>;

            T_ਅਧਾਰ* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator
            T_ਅਧਾਰ* m_nextp;  // Next element after current, or 'nullptr' for 'end()' iterator

            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(T_ਅਧਾਰ* elementp)
                : m_currp{elementp}
                , m_nextp{ਮੰਜ਼ਿਲ_Links(m_currp).m_nextp} {}
            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(std::nullptr_t)
                : m_currp{nullptr}
                , m_nextp{nullptr} {}

        ਜਨਤਕ:
            // Dereference - Note this returns a pointer.
            VL_ATTR_ALWINLINE
            T_IteratorElement* ਸੰਚਾਲਕ*() ਅਚਲ {
                UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
                prefetch(m_nextp, m_currp);
                ਵਾਪਸ static_cast<T_IteratorElement*>(m_currp);
            }
            // Pre increment - Keeps hold of current next pointer.
            VL_ATTR_ALWINLINE
            IteratorType& ਸੰਚਾਲਕ++() {
                UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
                m_currp = m_nextp;
                m_nextp = m_currp ? ਮੰਜ਼ਿਲ_Links(m_currp).m_nextp : nullptr;
                ਵਾਪਸ *this;
            }
            VL_ATTR_ALWINLINE
            bool ਸੰਚਾਲਕ!=(ਅਚਲ IteratorType& other) ਅਚਲ { ਵਾਪਸ m_currp != other.m_currp; }
        };

    ਜਨਤਕ:
        using iterator = UnlinkableItertatorImpl<T_Element>;
        using const_iterator = UnlinkableItertatorImpl<ਅਚਲ T_Element>;
        iterator begin() {  //
            ਵਾਪਸ m_list.m_headp ? iterator{m_list.m_headp} : ਸਿਰਾ();
        }
        const_iterator begin() ਅਚਲ {
            ਵਾਪਸ m_list.m_headp ? const_iterator{m_list.m_headp} : ਸਿਰਾ();
        }
        iterator ਸਿਰਾ() { ਵਾਪਸ iterator{nullptr}; }
        const_iterator ਸਿਰਾ() ਅਚਲ { ਵਾਪਸ const_iterator{nullptr}; }
    };

ਜਨਤਕ:
    using iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ false>;
    using const_iterator = SimpleItertatorImpl<ਅਚਲ T_Element, /* N_Reverse: */ false>;
    using ਉਲਟ_iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ true>;
    using const_ਉਲਟ_iterator = SimpleItertatorImpl<ਅਚਲ T_Element, /* N_Reverse: */ true>;

    // CONSTRUCTOR
    V3List() = ਮੂਲ;
    ~V3List() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        m_headp = reinterpret_cast<T_Element*>(1);
        m_lastp = reinterpret_cast<T_Element*>(1);
#ਜੇ_ਅੰਤ
    }
    VL_UNCOPYABLE(V3List);
    VL_UNMOVABLE(V3List);

    // METHDOS
    bool ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ !m_headp; }
    bool hasSingleElement() ਅਚਲ { ਵਾਪਸ m_headp && m_headp == m_lastp; }
    bool has_ਗੁਣਜ_Elements() ਅਚਲ { ਵਾਪਸ m_headp && m_headp != m_lastp; }
    bool hasTwoElements() ਅਚਲ { ਵਾਪਸ m_headp && ਮੰਜ਼ਿਲ_Links(m_headp).m_nextp == m_lastp; }

    // These return pointers, as we often want to unlink/delete them, and can also signal empty
    T_Element* frontp() { ਵਾਪਸ static_cast<T_Element*>(m_headp); }
    ਅਚਲ T_Element* frontp() ਅਚਲ { ਵਾਪਸ static_cast<T_Element*>(m_headp); }
    T_Element* backp() { ਵਾਪਸ static_cast<T_Element*>(m_lastp); }
    ਅਚਲ T_Element* backp() ਅਚਲ { ਵਾਪਸ static_cast<T_Element*>(m_lastp); }

    // Standard iterators. The iterator is only invalidated if the element it points to is
    // unlinked. Other list operations do not invalidate the itartor. If you want to be able to
    // unlink the currently iterated element, use 'unlinkable()' below.
    iterator begin() { ਵਾਪਸ iterator{m_headp}; }
    const_iterator begin() ਅਚਲ { ਵਾਪਸ const_iterator{m_headp}; }
    iterator ਸਿਰਾ() { ਵਾਪਸ iterator{nullptr}; }
    const_iterator ਸਿਰਾ() ਅਚਲ { ਵਾਪਸ const_iterator{nullptr}; }
    ਉਲਟ_iterator rbegin() { ਵਾਪਸ ਉਲਟ_iterator{m_lastp}; }
    const_ਉਲਟ_iterator rbegin() ਅਚਲ { ਵਾਪਸ const_ਉਲਟ_iterator{m_lastp}; }
    ਉਲਟ_iterator rend() { ਵਾਪਸ ਉਲਟ_iterator{nullptr}; }
    const_ਉਲਟ_iterator rend() ਅਚਲ { ਵਾਪਸ const_ਉਲਟ_iterator{nullptr}; }

    // Handle to create unlinkable iterators, which allows unlinking the currently iterated
    // element without invalidating the iterator. However, every other operation that mutates
    // the list invalidates the unlinkable iterator!
    UnlinkableProxy unlinkable() { ਵਾਪਸ UnlinkableProxy{*this}; }

    // Link (insert) existing element at front
    ਖਾਲੀ linkFront(ਅਚਲ T_Element* elementp) {
        ਸਵੈਚਾਲਿਤ& links = ਮੰਜ਼ਿਲ_Links(elementp);
        links.m_nextp = m_headp;
        links.m_prevp = nullptr;
        ਜੇ (m_headp) ਮੰਜ਼ਿਲ_Links(m_headp).m_prevp = const_cast<T_Element*>(elementp);
        m_headp = const_cast<T_Element*>(elementp);
        ਜੇ (!m_lastp) m_lastp = m_headp;
    }

    // Link (insert) existing element at back
    ਖਾਲੀ link_ਪਿੱਠ(ਅਚਲ T_Element* elementp) {
        ਸਵੈਚਾਲਿਤ& links = ਮੰਜ਼ਿਲ_Links(elementp);
        links.m_nextp = nullptr;
        links.m_prevp = m_lastp;
        ਜੇ (m_lastp) ਮੰਜ਼ਿਲ_Links(m_lastp).m_nextp = const_cast<T_Element*>(elementp);
        m_lastp = const_cast<T_Element*>(elementp);
        ਜੇ (!m_headp) m_headp = m_lastp;
    }

    // Unlink (remove) and return element at the front. Returns 'nullptr' if the list is empty.
    T_Element* unlinkFront() {
        T_Element* ਅਚਲ headp = m_headp;
        ਜੇ (headp) unlink(headp);
        ਵਾਪਸ headp;
    }

    // Unlink (remove) and return element at the back. Returns 'nullptr' if the list is empty.
    T_Element* unlink_ਪਿੱਠ() {
        T_Element* ਅਚਲ lastp = m_lastp;
        ਜੇ (lastp) unlink(lastp);
        ਵਾਪਸ lastp;
    }

    // Unlink (remove) the given element.
    ਖਾਲੀ unlink(ਅਚਲ T_Element* elementp) {
        ਸਵੈਚਾਲਿਤ& links = ਮੰਜ਼ਿਲ_Links(elementp);
        ਜੇ (links.m_nextp) ਮੰਜ਼ਿਲ_Links(links.m_nextp).m_prevp = links.m_prevp;
        ਜੇ (links.m_prevp) ਮੰਜ਼ਿਲ_Links(links.m_prevp).m_nextp = links.m_nextp;
        ਜੇ (m_headp == elementp) m_headp = links.m_nextp;
        ਜੇ (m_lastp == elementp) m_lastp = links.m_prevp;
        links.m_prevp = nullptr;
        links.m_nextp = nullptr;
    }

    // Swap elements of 2 lists
    ਖਾਲੀ ਅਦਲਾ_ਬਦਲੀ(ListType& other) {
        std::ਅਦਲਾ_ਬਦਲੀ(m_headp, other.m_headp);
        std::ਅਦਲਾ_ਬਦਲੀ(m_lastp, other.m_lastp);
    }

    // Take elements from 'other' and link (insert) them all before the given position.
    ਖਾਲੀ splice(const_iterator pos, ListType& other) {
        ਜੇ (ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਦਲਾ_ਬਦਲੀ(other);
        } ਨਹੀਂਤਾਂ ਜੇ (other.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            UASSERT(pos == ਸਿਰਾ(), "Sorry, only splicing at the end is implemented at the moment");
            ਮੰਜ਼ਿਲ_Links(m_lastp).m_nextp = other.m_headp;
            ਮੰਜ਼ਿਲ_Links(other.m_headp).m_prevp = m_lastp;
            m_lastp = other.m_lastp;
            other.m_headp = nullptr;
            other.m_lastp = nullptr;
        }
    }

    // This is O(n)!
    ਮਾਪ_t ਮਾਪ() ਅਚਲ {
        ਮਾਪ_t ਨਤੀਜਾ = 0;
        ਲਈ (ਸਵੈਚਾਲਿਤ it = begin(); it != ਸਿਰਾ(); ++it) ++ਨਤੀਜਾ;
        ਵਾਪਸ ਨਤੀਜਾ;
    }
};

#ਜੇ_ਅੰਤ  // Guard
