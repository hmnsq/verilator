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

#ifndef VERILATOR_V3LIST_H_
#define VERILATOR_V3LIST_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"

#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <utility>

//============================================================================
// The list links (just 2 pointers), to be instantiated as a member in the
// list element base class 'T_Base'. They are considered mutable, even if the
// list element is 'const', as they are only used for storing the elements in
// a V3List. That is, you can store const elements in a V3List.
template <typename T_Base>
class V3ListLinks final {
    // The V3List itself, but nothing else can access the link pointers
    template <typename B, V3ListLinks<B>& (B::*)(), typename>
    friend class V3List;

    T_Base* m_nextp = nullptr;  // Next element in list
    T_Base* m_prevp = nullptr;  // Previous element in list

பொது:
    V3ListLinks() = இயல்பு;
    ~V3ListLinks() {
#ifdef VL_DEBUG
        m_nextp = reinterpret_cast<T_Base*>(1);
        m_prevp = reinterpret_cast<T_Base*>(1);
#endif
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
template <typename T_Base,  //
          V3ListLinks<T_Base>& (T_Base::*LinksGetter)(),  //
          typename T_Element = T_Base>
class V3List final {
    static_assert(std::is_base_of<T_Base, T_Element>::மதிப்பு,
                  "'T_Element' must be a subtype of 'T_Base");

    using ListType = V3List<T_Base, LinksGetter, T_Element>;

    // MEMBERS
    T_Base* m_headp = nullptr;
    T_Base* m_lastp = nullptr;

    // Given the T_Base, return the Links. The links are always mutable, even in const elements.
    VL_ATTR_ALWINLINE
    நிலையான V3ListLinks<T_Base>& toLinks(மாறிலி T_Base* elementp) {
        திரும்பு (const_cast<T_Base*>(elementp)->*LinksGetter)();
    }

    VL_ATTR_ALWINLINE
    நிலையான வெறுமை prefetch(மாறிலி T_Base* elementp, மாறிலி T_Base* fallbackp) {
        UDEBUGONLY(UASSERT(fallbackp, "Prefetch fallback pointer must be non-nullptr"););
        // This compiles to a branchless prefetch with cmove, with the address always valid
        VL_PREFETCH_RW(elementp ? elementp : fallbackp);
    }

    // Iterator class template for V3List. This is just enough to support range based for loops
    // and basic usage. Feel free to extend as required.
    template <typename T_IteratorElement, bool N_Reverse>
    class SimpleItertatorImpl final {
        static_assert(std::is_same<T_IteratorElement, T_Element>::மதிப்பு
                          || std::is_same<T_IteratorElement, மாறிலி T_Element>::மதிப்பு,
                      "'SimpleItertatorImpl' must be used with element type only");

        // The List itself, but nothing else can construct iterators
        template <typename B, V3ListLinks<B>& (B::*)(), typename>
        friend class V3List;

        using IteratorType = SimpleItertatorImpl<T_IteratorElement, N_Reverse>;

        T_Base* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator

        VL_ATTR_ALWINLINE
        explicit SimpleItertatorImpl(T_Base* elementp)
            : m_currp{elementp} {}

        VL_ATTR_ALWINLINE
        நிலையான T_Base* step(T_Base* currp) {
            எனில் VL_CONSTEXPR_CXX17 (N_Reverse) {
                திரும்பு toLinks(currp).m_prevp;
            } இல்லையெனில் {
                திரும்பு toLinks(currp).m_nextp;
            }
        }

    பொது:
        // Dereference
        VL_ATTR_ALWINLINE
        T_IteratorElement& operator*() மாறிலி {
            UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
            prefetch(step(m_currp), m_currp);
            திரும்பு *static_cast<T_IteratorElement*>(m_currp);
        }
        // Pre increment
        VL_ATTR_ALWINLINE
        IteratorType& operator++() {
            UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
            m_currp = step(m_currp);
            திரும்பு *this;
        }
        // Post increment
        VL_ATTR_ALWINLINE
        IteratorType operator++(முழு) {
            UDEBUGONLY(UASSERT(m_currp, "Post-incrementing end of list iterator"););
            T_Base* மாறிலி elementp = m_currp;
            m_currp = step(m_currp);
            திரும்பு IteratorType{elementp};
        }
        VL_ATTR_ALWINLINE
        bool operator==(மாறிலி IteratorType& other) மாறிலி { திரும்பு m_currp == other.m_currp; }
        VL_ATTR_ALWINLINE
        bool operator!=(மாறிலி IteratorType& other) மாறிலி { திரும்பு m_currp != other.m_currp; }
        // Convert to const iterator
        VL_ATTR_ALWINLINE
        operator SimpleItertatorImpl<மாறிலி T_IteratorElement, N_Reverse>() மாறிலி {
            திரும்பு SimpleItertatorImpl<மாறிலி T_IteratorElement, N_Reverse>{m_currp};
        }
    };

    // Proxy class for creating unlinkable iterators, so we can use
    // 'for (T_Element* const ptr : list.unlinkable()) list.unlink(ptr);'
    class UnlinkableProxy final {
        // The List itself, but nothing else can construct UnlinkableProxy
        template <typename B, V3ListLinks<B>& (B::*)(), typename>
        friend class V3List;

        ListType& m_list;  // The proxied list

        explicit UnlinkableProxy(ListType& list)
            : m_list{list} {}

        // Unlinkable iterator class template. This only supports enough for range based for loops.
        // If you want something fancier, use and manage the direct iterator manually.
        template <typename T_IteratorElement>
        class UnlinkableItertatorImpl final {
            static_assert(std::is_same<T_IteratorElement, T_Element>::மதிப்பு
                              || std::is_same<T_IteratorElement, மாறிலி T_Element>::மதிப்பு,
                          "'UnlinkableItertatorImpl' must be used with element type only");

            // The UnlinkableProxy, but nothing else can construct unlinkable iterators
            friend class UnlinkableProxy;

            using IteratorType = UnlinkableItertatorImpl<T_IteratorElement>;

            T_Base* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator
            T_Base* m_nextp;  // Next element after current, or 'nullptr' for 'end()' iterator

            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(T_Base* elementp)
                : m_currp{elementp}
                , m_nextp{toLinks(m_currp).m_nextp} {}
            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(std::nullptr_t)
                : m_currp{nullptr}
                , m_nextp{nullptr} {}

        பொது:
            // Dereference - Note this returns a pointer.
            VL_ATTR_ALWINLINE
            T_IteratorElement* operator*() மாறிலி {
                UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
                prefetch(m_nextp, m_currp);
                திரும்பு static_cast<T_IteratorElement*>(m_currp);
            }
            // Pre increment - Keeps hold of current next pointer.
            VL_ATTR_ALWINLINE
            IteratorType& operator++() {
                UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
                m_currp = m_nextp;
                m_nextp = m_currp ? toLinks(m_currp).m_nextp : nullptr;
                திரும்பு *this;
            }
            VL_ATTR_ALWINLINE
            bool operator!=(மாறிலி IteratorType& other) மாறிலி { திரும்பு m_currp != other.m_currp; }
        };

    பொது:
        using iterator = UnlinkableItertatorImpl<T_Element>;
        using const_iterator = UnlinkableItertatorImpl<மாறிலி T_Element>;
        iterator begin() {  //
            திரும்பு m_list.m_headp ? iterator{m_list.m_headp} : end();
        }
        const_iterator begin() மாறிலி {
            திரும்பு m_list.m_headp ? const_iterator{m_list.m_headp} : end();
        }
        iterator end() { திரும்பு iterator{nullptr}; }
        const_iterator end() மாறிலி { திரும்பு const_iterator{nullptr}; }
    };

பொது:
    using iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ false>;
    using const_iterator = SimpleItertatorImpl<மாறிலி T_Element, /* N_Reverse: */ false>;
    using reverse_iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ true>;
    using const_reverse_iterator = SimpleItertatorImpl<மாறிலி T_Element, /* N_Reverse: */ true>;

    // CONSTRUCTOR
    V3List() = இயல்பு;
    ~V3List() {
#ifdef VL_DEBUG
        m_headp = reinterpret_cast<T_Element*>(1);
        m_lastp = reinterpret_cast<T_Element*>(1);
#endif
    }
    VL_UNCOPYABLE(V3List);
    VL_UNMOVABLE(V3List);

    // METHDOS
    bool empty() மாறிலி { திரும்பு !m_headp; }
    bool hasSingleElement() மாறிலி { திரும்பு m_headp && m_headp == m_lastp; }
    bool hasMultipleElements() மாறிலி { திரும்பு m_headp && m_headp != m_lastp; }
    bool hasTwoElements() மாறிலி { திரும்பு m_headp && toLinks(m_headp).m_nextp == m_lastp; }

    // These return pointers, as we often want to unlink/delete them, and can also signal empty
    T_Element* frontp() { திரும்பு static_cast<T_Element*>(m_headp); }
    மாறிலி T_Element* frontp() மாறிலி { திரும்பு static_cast<T_Element*>(m_headp); }
    T_Element* backp() { திரும்பு static_cast<T_Element*>(m_lastp); }
    மாறிலி T_Element* backp() மாறிலி { திரும்பு static_cast<T_Element*>(m_lastp); }

    // Standard iterators. The iterator is only invalidated if the element it points to is
    // unlinked. Other list operations do not invalidate the itartor. If you want to be able to
    // unlink the currently iterated element, use 'unlinkable()' below.
    iterator begin() { திரும்பு iterator{m_headp}; }
    const_iterator begin() மாறிலி { திரும்பு const_iterator{m_headp}; }
    iterator end() { திரும்பு iterator{nullptr}; }
    const_iterator end() மாறிலி { திரும்பு const_iterator{nullptr}; }
    reverse_iterator rbegin() { திரும்பு reverse_iterator{m_lastp}; }
    const_reverse_iterator rbegin() மாறிலி { திரும்பு const_reverse_iterator{m_lastp}; }
    reverse_iterator rend() { திரும்பு reverse_iterator{nullptr}; }
    const_reverse_iterator rend() மாறிலி { திரும்பு const_reverse_iterator{nullptr}; }

    // Handle to create unlinkable iterators, which allows unlinking the currently iterated
    // element without invalidating the iterator. However, every other operation that mutates
    // the list invalidates the unlinkable iterator!
    UnlinkableProxy unlinkable() { திரும்பு UnlinkableProxy{*this}; }

    // Link (insert) existing element at front
    வெறுமை linkFront(மாறிலி T_Element* elementp) {
        auto& links = toLinks(elementp);
        links.m_nextp = m_headp;
        links.m_prevp = nullptr;
        எனில் (m_headp) toLinks(m_headp).m_prevp = const_cast<T_Element*>(elementp);
        m_headp = const_cast<T_Element*>(elementp);
        எனில் (!m_lastp) m_lastp = m_headp;
    }

    // Link (insert) existing element at back
    வெறுமை linkBack(மாறிலி T_Element* elementp) {
        auto& links = toLinks(elementp);
        links.m_nextp = nullptr;
        links.m_prevp = m_lastp;
        எனில் (m_lastp) toLinks(m_lastp).m_nextp = const_cast<T_Element*>(elementp);
        m_lastp = const_cast<T_Element*>(elementp);
        எனில் (!m_headp) m_headp = m_lastp;
    }

    // Unlink (remove) and return element at the front. Returns 'nullptr' if the list is empty.
    T_Element* unlinkFront() {
        T_Element* மாறிலி headp = m_headp;
        எனில் (headp) unlink(headp);
        திரும்பு headp;
    }

    // Unlink (remove) and return element at the back. Returns 'nullptr' if the list is empty.
    T_Element* unlinkBack() {
        T_Element* மாறிலி lastp = m_lastp;
        எனில் (lastp) unlink(lastp);
        திரும்பு lastp;
    }

    // Unlink (remove) the given element.
    வெறுமை unlink(மாறிலி T_Element* elementp) {
        auto& links = toLinks(elementp);
        எனில் (links.m_nextp) toLinks(links.m_nextp).m_prevp = links.m_prevp;
        எனில் (links.m_prevp) toLinks(links.m_prevp).m_nextp = links.m_nextp;
        எனில் (m_headp == elementp) m_headp = links.m_nextp;
        எனில் (m_lastp == elementp) m_lastp = links.m_prevp;
        links.m_prevp = nullptr;
        links.m_nextp = nullptr;
    }

    // Swap elements of 2 lists
    வெறுமை swap(ListType& other) {
        std::swap(m_headp, other.m_headp);
        std::swap(m_lastp, other.m_lastp);
    }

    // Take elements from 'other' and link (insert) them all before the given position.
    வெறுமை splice(const_iterator pos, ListType& other) {
        எனில் (empty()) {
            swap(other);
        } இல்லையெனில் எனில் (other.empty()) {
            திரும்பு;
        } இல்லையெனில் {
            UASSERT(pos == end(), "Sorry, only splicing at the end is implemented at the moment");
            toLinks(m_lastp).m_nextp = other.m_headp;
            toLinks(other.m_headp).m_prevp = m_lastp;
            m_lastp = other.m_lastp;
            other.m_headp = nullptr;
            other.m_lastp = nullptr;
        }
    }

    // This is O(n)!
    size_t size() மாறிலி {
        size_t result = 0;
        க்கு (auto it = begin(); it != end(); ++it) ++result;
        திரும்பு result;
    }
};

#endif  // Guard
