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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3LIST_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3LIST_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <utility>

//============================================================================
// The list links (just 2 pointers), to be instantiated as a member in the
// list element base class 'T_Base'. They are considered mutable, even if the
// list element is 'const', as they are only used for storing the elements in
// a V3List. That is, you can store const elements in a V3List.
ಮಾದರಿ <typename T_ತಳ>
class V3ListLinks final {
    // The V3List itself, but nothing else can access the link pointers
    ಮಾದರಿ <typename B, V3ListLinks<B>& (B::*)(), typename>
    ಮಿತ್ರ class V3List;

    T_ತಳ* m_nextp = nullptr;  // Next element in list
    T_ತಳ* m_prevp = nullptr;  // Previous element in list

ಸಾರ್ವಜನಿಕ:
    V3ListLinks() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3ListLinks() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        m_nextp = reinterpret_cast<T_ತಳ*>(1);
        m_prevp = reinterpret_cast<T_ತಳ*>(1);
#ಆದರೆ_ಅಂತ್ಯ
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
ಮಾದರಿ <typename T_ತಳ,  //
          V3ListLinks<T_ತಳ>& (T_ತಳ::*LinksGetter)(),  //
          typename T_Element = T_ತಳ>
class V3List final {
    static_assert(std::is_ತಳ_ದ<T_ತಳ, T_Element>::ಮೌಲ್ಯ,
                  "'T_Element' must be a subtype of 'T_Base");

    using ListType = V3List<T_ತಳ, LinksGetter, T_Element>;

    // MEMBERS
    T_ತಳ* m_headp = nullptr;
    T_ತಳ* m_lastp = nullptr;

    // Given the T_Base, return the Links. The links are always mutable, even in const elements.
    VL_ATTR_ALWINLINE
    ಸ್ಥಿರ V3ListLinks<T_ತಳ>& ಗಮ್ಯ_Links(ಸ್ಥಿರಾಂಕ T_ತಳ* elementp) {
        ಹಿಂತಿರುಗಿ (const_cast<T_ತಳ*>(elementp)->*LinksGetter)();
    }

    VL_ATTR_ALWINLINE
    ಸ್ಥಿರ ಶೂನ್ಯ prefetch(ಸ್ಥಿರಾಂಕ T_ತಳ* elementp, ಸ್ಥಿರಾಂಕ T_ತಳ* fallbackp) {
        UDEBUGONLY(UASSERT(fallbackp, "Prefetch fallback pointer must be non-nullptr"););
        // This compiles to a branchless prefetch with cmove, with the address always valid
        VL_PREFETCH_RW(elementp ? elementp : fallbackp);
    }

    // Iterator class template for V3List. This is just enough to support range based for loops
    // and basic usage. Feel free to extend as required.
    ಮಾದರಿ <typename T_IteratorElement, bool N_ಹಿಮ್ಮುಖ>
    class SimpleItertatorImpl final {
        static_assert(std::is_same<T_IteratorElement, T_Element>::ಮೌಲ್ಯ
                          || std::is_same<T_IteratorElement, ಸ್ಥಿರಾಂಕ T_Element>::ಮೌಲ್ಯ,
                      "'SimpleItertatorImpl' must be used with element type only");

        // The List itself, but nothing else can construct iterators
        ಮಾದರಿ <typename B, V3ListLinks<B>& (B::*)(), typename>
        ಮಿತ್ರ class V3List;

        using IteratorType = SimpleItertatorImpl<T_IteratorElement, N_ಹಿಮ್ಮುಖ>;

        T_ತಳ* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator

        VL_ATTR_ALWINLINE
        explicit SimpleItertatorImpl(T_ತಳ* elementp)
            : m_currp{elementp} {}

        VL_ATTR_ALWINLINE
        ಸ್ಥಿರ T_ತಳ* ಹೆಜ್ಜೆ(T_ತಳ* currp) {
            ಆದರೆ VL_CONSTEXPR_CXX17 (N_ಹಿಮ್ಮುಖ) {
                ಹಿಂತಿರುಗಿ ಗಮ್ಯ_Links(currp).m_prevp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಗಮ್ಯ_Links(currp).m_nextp;
            }
        }

    ಸಾರ್ವಜನಿಕ:
        // Dereference
        VL_ATTR_ALWINLINE
        T_IteratorElement& ಕ್ರಿಯಾಕಾರಕ*() ಸ್ಥಿರಾಂಕ {
            UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
            prefetch(ಹೆಜ್ಜೆ(m_currp), m_currp);
            ಹಿಂತಿರುಗಿ *static_cast<T_IteratorElement*>(m_currp);
        }
        // Pre increment
        VL_ATTR_ALWINLINE
        IteratorType& ಕ್ರಿಯಾಕಾರಕ++() {
            UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
            m_currp = ಹೆಜ್ಜೆ(m_currp);
            ಹಿಂತಿರುಗಿ *this;
        }
        // Post increment
        VL_ATTR_ALWINLINE
        IteratorType ಕ್ರಿಯಾಕಾರಕ++(ಪೂರ್ಣಾಂಕ) {
            UDEBUGONLY(UASSERT(m_currp, "Post-incrementing end of list iterator"););
            T_ತಳ* ಸ್ಥಿರಾಂಕ elementp = m_currp;
            m_currp = ಹೆಜ್ಜೆ(m_currp);
            ಹಿಂತಿರುಗಿ IteratorType{elementp};
        }
        VL_ATTR_ALWINLINE
        bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ IteratorType& other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_currp == other.m_currp; }
        VL_ATTR_ALWINLINE
        bool ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ IteratorType& other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_currp != other.m_currp; }
        // Convert to const iterator
        VL_ATTR_ALWINLINE
        ಕ್ರಿಯಾಕಾರಕ SimpleItertatorImpl<ಸ್ಥಿರಾಂಕ T_IteratorElement, N_ಹಿಮ್ಮುಖ>() ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ SimpleItertatorImpl<ಸ್ಥಿರಾಂಕ T_IteratorElement, N_ಹಿಮ್ಮುಖ>{m_currp};
        }
    };

    // Proxy class for creating unlinkable iterators, so we can use
    // 'for (T_Element* const ptr : list.unlinkable()) list.unlink(ptr);'
    class UnlinkableProxy final {
        // The List itself, but nothing else can construct UnlinkableProxy
        ಮಾದರಿ <typename B, V3ListLinks<B>& (B::*)(), typename>
        ಮಿತ್ರ class V3List;

        ListType& m_list;  // The proxied list

        explicit UnlinkableProxy(ListType& list)
            : m_list{list} {}

        // Unlinkable iterator class template. This only supports enough for range based for loops.
        // If you want something fancier, use and manage the direct iterator manually.
        ಮಾದರಿ <typename T_IteratorElement>
        class UnlinkableItertatorImpl final {
            static_assert(std::is_same<T_IteratorElement, T_Element>::ಮೌಲ್ಯ
                              || std::is_same<T_IteratorElement, ಸ್ಥಿರಾಂಕ T_Element>::ಮೌಲ್ಯ,
                          "'UnlinkableItertatorImpl' must be used with element type only");

            // The UnlinkableProxy, but nothing else can construct unlinkable iterators
            ಮಿತ್ರ class UnlinkableProxy;

            using IteratorType = UnlinkableItertatorImpl<T_IteratorElement>;

            T_ತಳ* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator
            T_ತಳ* m_nextp;  // Next element after current, or 'nullptr' for 'end()' iterator

            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(T_ತಳ* elementp)
                : m_currp{elementp}
                , m_nextp{ಗಮ್ಯ_Links(m_currp).m_nextp} {}
            VL_ATTR_ALWINLINE
            explicit UnlinkableItertatorImpl(std::nullptr_t)
                : m_currp{nullptr}
                , m_nextp{nullptr} {}

        ಸಾರ್ವಜನಿಕ:
            // Dereference - Note this returns a pointer.
            VL_ATTR_ALWINLINE
            T_IteratorElement* ಕ್ರಿಯಾಕಾರಕ*() ಸ್ಥಿರಾಂಕ {
                UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
                prefetch(m_nextp, m_currp);
                ಹಿಂತಿರುಗಿ static_cast<T_IteratorElement*>(m_currp);
            }
            // Pre increment - Keeps hold of current next pointer.
            VL_ATTR_ALWINLINE
            IteratorType& ಕ್ರಿಯಾಕಾರಕ++() {
                UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
                m_currp = m_nextp;
                m_nextp = m_currp ? ಗಮ್ಯ_Links(m_currp).m_nextp : nullptr;
                ಹಿಂತಿರುಗಿ *this;
            }
            VL_ATTR_ALWINLINE
            bool ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ IteratorType& other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_currp != other.m_currp; }
        };

    ಸಾರ್ವಜನಿಕ:
        using iterator = UnlinkableItertatorImpl<T_Element>;
        using const_iterator = UnlinkableItertatorImpl<ಸ್ಥಿರಾಂಕ T_Element>;
        iterator begin() {  //
            ಹಿಂತಿರುಗಿ m_list.m_headp ? iterator{m_list.m_headp} : ಕೊನೆ();
        }
        const_iterator begin() ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ m_list.m_headp ? const_iterator{m_list.m_headp} : ಕೊನೆ();
        }
        iterator ಕೊನೆ() { ಹಿಂತಿರುಗಿ iterator{nullptr}; }
        const_iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ const_iterator{nullptr}; }
    };

ಸಾರ್ವಜನಿಕ:
    using iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ false>;
    using const_iterator = SimpleItertatorImpl<ಸ್ಥಿರಾಂಕ T_Element, /* N_Reverse: */ false>;
    using ಹಿಮ್ಮುಖ_iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ true>;
    using const_ಹಿಮ್ಮುಖ_iterator = SimpleItertatorImpl<ಸ್ಥಿರಾಂಕ T_Element, /* N_Reverse: */ true>;

    // CONSTRUCTOR
    V3List() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3List() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        m_headp = reinterpret_cast<T_Element*>(1);
        m_lastp = reinterpret_cast<T_Element*>(1);
#ಆದರೆ_ಅಂತ್ಯ
    }
    VL_UNCOPYABLE(V3List);
    VL_UNMOVABLE(V3List);

    // METHDOS
    bool ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_headp; }
    bool hasSingleElement() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_headp && m_headp == m_lastp; }
    bool has_ಅಪವರ್ತ್ಯ_Elements() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_headp && m_headp != m_lastp; }
    bool hasTwoElements() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_headp && ಗಮ್ಯ_Links(m_headp).m_nextp == m_lastp; }

    // These return pointers, as we often want to unlink/delete them, and can also signal empty
    T_Element* frontp() { ಹಿಂತಿರುಗಿ static_cast<T_Element*>(m_headp); }
    ಸ್ಥಿರಾಂಕ T_Element* frontp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<T_Element*>(m_headp); }
    T_Element* backp() { ಹಿಂತಿರುಗಿ static_cast<T_Element*>(m_lastp); }
    ಸ್ಥಿರಾಂಕ T_Element* backp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<T_Element*>(m_lastp); }

    // Standard iterators. The iterator is only invalidated if the element it points to is
    // unlinked. Other list operations do not invalidate the itartor. If you want to be able to
    // unlink the currently iterated element, use 'unlinkable()' below.
    iterator begin() { ಹಿಂತಿರುಗಿ iterator{m_headp}; }
    const_iterator begin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ const_iterator{m_headp}; }
    iterator ಕೊನೆ() { ಹಿಂತಿರುಗಿ iterator{nullptr}; }
    const_iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ const_iterator{nullptr}; }
    ಹಿಮ್ಮುಖ_iterator rbegin() { ಹಿಂತಿರುಗಿ ಹಿಮ್ಮುಖ_iterator{m_lastp}; }
    const_ಹಿಮ್ಮುಖ_iterator rbegin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ const_ಹಿಮ್ಮುಖ_iterator{m_lastp}; }
    ಹಿಮ್ಮುಖ_iterator rend() { ಹಿಂತಿರುಗಿ ಹಿಮ್ಮುಖ_iterator{nullptr}; }
    const_ಹಿಮ್ಮುಖ_iterator rend() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ const_ಹಿಮ್ಮುಖ_iterator{nullptr}; }

    // Handle to create unlinkable iterators, which allows unlinking the currently iterated
    // element without invalidating the iterator. However, every other operation that mutates
    // the list invalidates the unlinkable iterator!
    UnlinkableProxy unlinkable() { ಹಿಂತಿರುಗಿ UnlinkableProxy{*this}; }

    // Link (insert) existing element at front
    ಶೂನ್ಯ linkFront(ಸ್ಥಿರಾಂಕ T_Element* elementp) {
        ಸ್ವಯಂಚಾಲಿತ& links = ಗಮ್ಯ_Links(elementp);
        links.m_nextp = m_headp;
        links.m_prevp = nullptr;
        ಆದರೆ (m_headp) ಗಮ್ಯ_Links(m_headp).m_prevp = const_cast<T_Element*>(elementp);
        m_headp = const_cast<T_Element*>(elementp);
        ಆದರೆ (!m_lastp) m_lastp = m_headp;
    }

    // Link (insert) existing element at back
    ಶೂನ್ಯ link_ಹಿಂಭಾಗ(ಸ್ಥಿರಾಂಕ T_Element* elementp) {
        ಸ್ವಯಂಚಾಲಿತ& links = ಗಮ್ಯ_Links(elementp);
        links.m_nextp = nullptr;
        links.m_prevp = m_lastp;
        ಆದರೆ (m_lastp) ಗಮ್ಯ_Links(m_lastp).m_nextp = const_cast<T_Element*>(elementp);
        m_lastp = const_cast<T_Element*>(elementp);
        ಆದರೆ (!m_headp) m_headp = m_lastp;
    }

    // Unlink (remove) and return element at the front. Returns 'nullptr' if the list is empty.
    T_Element* unlinkFront() {
        T_Element* ಸ್ಥಿರಾಂಕ headp = m_headp;
        ಆದರೆ (headp) unlink(headp);
        ಹಿಂತಿರುಗಿ headp;
    }

    // Unlink (remove) and return element at the back. Returns 'nullptr' if the list is empty.
    T_Element* unlink_ಹಿಂಭಾಗ() {
        T_Element* ಸ್ಥಿರಾಂಕ lastp = m_lastp;
        ಆದರೆ (lastp) unlink(lastp);
        ಹಿಂತಿರುಗಿ lastp;
    }

    // Unlink (remove) the given element.
    ಶೂನ್ಯ unlink(ಸ್ಥಿರಾಂಕ T_Element* elementp) {
        ಸ್ವಯಂಚಾಲಿತ& links = ಗಮ್ಯ_Links(elementp);
        ಆದರೆ (links.m_nextp) ಗಮ್ಯ_Links(links.m_nextp).m_prevp = links.m_prevp;
        ಆದರೆ (links.m_prevp) ಗಮ್ಯ_Links(links.m_prevp).m_nextp = links.m_nextp;
        ಆದರೆ (m_headp == elementp) m_headp = links.m_nextp;
        ಆದರೆ (m_lastp == elementp) m_lastp = links.m_prevp;
        links.m_prevp = nullptr;
        links.m_nextp = nullptr;
    }

    // Swap elements of 2 lists
    ಶೂನ್ಯ ಅದಲುಬದಲು(ListType& other) {
        std::ಅದಲುಬದಲು(m_headp, other.m_headp);
        std::ಅದಲುಬದಲು(m_lastp, other.m_lastp);
    }

    // Take elements from 'other' and link (insert) them all before the given position.
    ಶೂನ್ಯ splice(const_iterator pos, ListType& other) {
        ಆದರೆ (ಖಾಲಿಗಳು()) {
            ಅದಲುಬದಲು(other);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.ಖಾಲಿಗಳು()) {
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT(pos == ಕೊನೆ(), "Sorry, only splicing at the end is implemented at the moment");
            ಗಮ್ಯ_Links(m_lastp).m_nextp = other.m_headp;
            ಗಮ್ಯ_Links(other.m_headp).m_prevp = m_lastp;
            m_lastp = other.m_lastp;
            other.m_headp = nullptr;
            other.m_lastp = nullptr;
        }
    }

    // This is O(n)!
    ಗಾತ್ರ_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ {
        ಗಾತ್ರ_t ಫಲಿತಾಂಶ = 0;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = begin(); it != ಕೊನೆ(); ++it) ++ಫಲಿತಾಂಶ;
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
