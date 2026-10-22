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

#अगर_अघोषित VERILATOR_V3LIST_H_
#घोषणा VERILATOR_V3LIST_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"

#समावेश <type_traits>
#समावेश <utility>

//============================================================================
// The list links (just 2 pointers), to be instantiated as a member in the
// list element base class 'T_Base'. They are considered mutable, even if the
// list element is 'const', as they are only used for storing the elements in
// a V3List. That is, you can store const elements in a V3List.
ढाँचा <typename T_Base>
class V3ListLinks final {
    // The V3List itself, but nothing else can access the link pointers
    ढाँचा <typename B, V3ListLinks<B>& (B::*)(), typename>
    मित्र class V3List;

    T_Base* m_nextp = nullptr;  // Next element in list
    T_Base* m_prevp = nullptr;  // Previous element in list

खुला:
    V3ListLinks() = शेष;
    ~V3ListLinks() {
#अगर_घोषित VL_DEBUG
        m_nextp = reinterpret_cast<T_Base*>(1);
        m_prevp = reinterpret_cast<T_Base*>(1);
#पूर्ण_अगर
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
ढाँचा <typename T_Base,  //
          V3ListLinks<T_Base>& (T_Base::*LinksGetter)(),  //
          typename T_Element = T_Base>
class V3List final {
    static_assert(std::is_base_of<T_Base, T_Element>::मूल्य,
                  "'T_Element' must be a subtype of 'T_Base");

    using ListType = V3List<T_Base, LinksGetter, T_Element>;

    // MEMBERS
    T_Base* m_headp = nullptr;
    T_Base* m_lastp = nullptr;

    // Given the T_Base, return the Links. The links are always mutable, even in const elements.
    VL_ATTR_ALWINLINE
    अटल V3ListLinks<T_Base>& toLinks(स्थिर T_Base* elementp) {
        वापस (const_cast<T_Base*>(elementp)->*LinksGetter)();
    }

    VL_ATTR_ALWINLINE
    अटल व्योम prefetch(स्थिर T_Base* elementp, स्थिर T_Base* fallbackp) {
        UDEBUGONLY(UASSERT(fallbackp, "Prefetch fallback pointer must be non-nullptr"););
        // This compiles to a branchless prefetch with cmove, with the address always valid
        VL_PREFETCH_RW(elementp ? elementp : fallbackp);
    }

    // Iterator class template for V3List. This is just enough to support range based for loops
    // and basic usage. Feel free to extend as required.
    ढाँचा <typename T_IteratorElement, bool N_Reverse>
    class SimpleItertatorImpl final {
        static_assert(std::is_same<T_IteratorElement, T_Element>::मूल्य
                          || std::is_same<T_IteratorElement, स्थिर T_Element>::मूल्य,
                      "'SimpleItertatorImpl' must be used with element type only");

        // The List itself, but nothing else can construct iterators
        ढाँचा <typename B, V3ListLinks<B>& (B::*)(), typename>
        मित्र class V3List;

        using IteratorType = SimpleItertatorImpl<T_IteratorElement, N_Reverse>;

        T_Base* m_currp;  // Currently iterated element, or 'nullptr' for 'end()' iterator

        VL_ATTR_ALWINLINE
        explicit SimpleItertatorImpl(T_Base* elementp)
            : m_currp{elementp} {}

        VL_ATTR_ALWINLINE
        अटल T_Base* step(T_Base* currp) {
            अगर VL_CONSTEXPR_CXX17 (N_Reverse) {
                वापस toLinks(currp).m_prevp;
            } अन्यथा {
                वापस toLinks(currp).m_nextp;
            }
        }

    खुला:
        // Dereference
        VL_ATTR_ALWINLINE
        T_IteratorElement& चालक*() स्थिर {
            UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
            prefetch(step(m_currp), m_currp);
            वापस *static_cast<T_IteratorElement*>(m_currp);
        }
        // Pre increment
        VL_ATTR_ALWINLINE
        IteratorType& चालक++() {
            UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
            m_currp = step(m_currp);
            वापस *this;
        }
        // Post increment
        VL_ATTR_ALWINLINE
        IteratorType चालक++(पूर्णांक) {
            UDEBUGONLY(UASSERT(m_currp, "Post-incrementing end of list iterator"););
            T_Base* स्थिर elementp = m_currp;
            m_currp = step(m_currp);
            वापस IteratorType{elementp};
        }
        VL_ATTR_ALWINLINE
        bool चालक==(स्थिर IteratorType& other) स्थिर { वापस m_currp == other.m_currp; }
        VL_ATTR_ALWINLINE
        bool चालक!=(स्थिर IteratorType& other) स्थिर { वापस m_currp != other.m_currp; }
        // Convert to const iterator
        VL_ATTR_ALWINLINE
        चालक SimpleItertatorImpl<स्थिर T_IteratorElement, N_Reverse>() स्थिर {
            वापस SimpleItertatorImpl<स्थिर T_IteratorElement, N_Reverse>{m_currp};
        }
    };

    // Proxy class for creating unlinkable iterators, so we can use
    // 'for (T_Element* const ptr : list.unlinkable()) list.unlink(ptr);'
    class UnlinkableProxy final {
        // The List itself, but nothing else can construct UnlinkableProxy
        ढाँचा <typename B, V3ListLinks<B>& (B::*)(), typename>
        मित्र class V3List;

        ListType& m_list;  // The proxied list

        explicit UnlinkableProxy(ListType& list)
            : m_list{list} {}

        // Unlinkable iterator class template. This only supports enough for range based for loops.
        // If you want something fancier, use and manage the direct iterator manually.
        ढाँचा <typename T_IteratorElement>
        class UnlinkableItertatorImpl final {
            static_assert(std::is_same<T_IteratorElement, T_Element>::मूल्य
                              || std::is_same<T_IteratorElement, स्थिर T_Element>::मूल्य,
                          "'UnlinkableItertatorImpl' must be used with element type only");

            // The UnlinkableProxy, but nothing else can construct unlinkable iterators
            मित्र class UnlinkableProxy;

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

        खुला:
            // Dereference - Note this returns a pointer.
            VL_ATTR_ALWINLINE
            T_IteratorElement* चालक*() स्थिर {
                UDEBUGONLY(UASSERT(m_currp, "Dereferencing end of list iterator"););
                prefetch(m_nextp, m_currp);
                वापस static_cast<T_IteratorElement*>(m_currp);
            }
            // Pre increment - Keeps hold of current next pointer.
            VL_ATTR_ALWINLINE
            IteratorType& चालक++() {
                UDEBUGONLY(UASSERT(m_currp, "Pre-incrementing end of list iterator"););
                m_currp = m_nextp;
                m_nextp = m_currp ? toLinks(m_currp).m_nextp : nullptr;
                वापस *this;
            }
            VL_ATTR_ALWINLINE
            bool चालक!=(स्थिर IteratorType& other) स्थिर { वापस m_currp != other.m_currp; }
        };

    खुला:
        using iterator = UnlinkableItertatorImpl<T_Element>;
        using const_iterator = UnlinkableItertatorImpl<स्थिर T_Element>;
        iterator begin() {  //
            वापस m_list.m_headp ? iterator{m_list.m_headp} : end();
        }
        const_iterator begin() स्थिर {
            वापस m_list.m_headp ? const_iterator{m_list.m_headp} : end();
        }
        iterator end() { वापस iterator{nullptr}; }
        const_iterator end() स्थिर { वापस const_iterator{nullptr}; }
    };

खुला:
    using iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ false>;
    using const_iterator = SimpleItertatorImpl<स्थिर T_Element, /* N_Reverse: */ false>;
    using reverse_iterator = SimpleItertatorImpl<T_Element, /* N_Reverse: */ true>;
    using const_reverse_iterator = SimpleItertatorImpl<स्थिर T_Element, /* N_Reverse: */ true>;

    // CONSTRUCTOR
    V3List() = शेष;
    ~V3List() {
#अगर_घोषित VL_DEBUG
        m_headp = reinterpret_cast<T_Element*>(1);
        m_lastp = reinterpret_cast<T_Element*>(1);
#पूर्ण_अगर
    }
    VL_UNCOPYABLE(V3List);
    VL_UNMOVABLE(V3List);

    // METHDOS
    bool empty() स्थिर { वापस !m_headp; }
    bool hasSingleElement() स्थिर { वापस m_headp && m_headp == m_lastp; }
    bool hasMultipleElements() स्थिर { वापस m_headp && m_headp != m_lastp; }
    bool hasTwoElements() स्थिर { वापस m_headp && toLinks(m_headp).m_nextp == m_lastp; }

    // These return pointers, as we often want to unlink/delete them, and can also signal empty
    T_Element* frontp() { वापस static_cast<T_Element*>(m_headp); }
    स्थिर T_Element* frontp() स्थिर { वापस static_cast<T_Element*>(m_headp); }
    T_Element* backp() { वापस static_cast<T_Element*>(m_lastp); }
    स्थिर T_Element* backp() स्थिर { वापस static_cast<T_Element*>(m_lastp); }

    // Standard iterators. The iterator is only invalidated if the element it points to is
    // unlinked. Other list operations do not invalidate the itartor. If you want to be able to
    // unlink the currently iterated element, use 'unlinkable()' below.
    iterator begin() { वापस iterator{m_headp}; }
    const_iterator begin() स्थिर { वापस const_iterator{m_headp}; }
    iterator end() { वापस iterator{nullptr}; }
    const_iterator end() स्थिर { वापस const_iterator{nullptr}; }
    reverse_iterator rbegin() { वापस reverse_iterator{m_lastp}; }
    const_reverse_iterator rbegin() स्थिर { वापस const_reverse_iterator{m_lastp}; }
    reverse_iterator rend() { वापस reverse_iterator{nullptr}; }
    const_reverse_iterator rend() स्थिर { वापस const_reverse_iterator{nullptr}; }

    // Handle to create unlinkable iterators, which allows unlinking the currently iterated
    // element without invalidating the iterator. However, every other operation that mutates
    // the list invalidates the unlinkable iterator!
    UnlinkableProxy unlinkable() { वापस UnlinkableProxy{*this}; }

    // Link (insert) existing element at front
    व्योम linkFront(स्थिर T_Element* elementp) {
        स्वतः& links = toLinks(elementp);
        links.m_nextp = m_headp;
        links.m_prevp = nullptr;
        अगर (m_headp) toLinks(m_headp).m_prevp = const_cast<T_Element*>(elementp);
        m_headp = const_cast<T_Element*>(elementp);
        अगर (!m_lastp) m_lastp = m_headp;
    }

    // Link (insert) existing element at back
    व्योम linkBack(स्थिर T_Element* elementp) {
        स्वतः& links = toLinks(elementp);
        links.m_nextp = nullptr;
        links.m_prevp = m_lastp;
        अगर (m_lastp) toLinks(m_lastp).m_nextp = const_cast<T_Element*>(elementp);
        m_lastp = const_cast<T_Element*>(elementp);
        अगर (!m_headp) m_headp = m_lastp;
    }

    // Unlink (remove) and return element at the front. Returns 'nullptr' if the list is empty.
    T_Element* unlinkFront() {
        T_Element* स्थिर headp = m_headp;
        अगर (headp) unlink(headp);
        वापस headp;
    }

    // Unlink (remove) and return element at the back. Returns 'nullptr' if the list is empty.
    T_Element* unlinkBack() {
        T_Element* स्थिर lastp = m_lastp;
        अगर (lastp) unlink(lastp);
        वापस lastp;
    }

    // Unlink (remove) the given element.
    व्योम unlink(स्थिर T_Element* elementp) {
        स्वतः& links = toLinks(elementp);
        अगर (links.m_nextp) toLinks(links.m_nextp).m_prevp = links.m_prevp;
        अगर (links.m_prevp) toLinks(links.m_prevp).m_nextp = links.m_nextp;
        अगर (m_headp == elementp) m_headp = links.m_nextp;
        अगर (m_lastp == elementp) m_lastp = links.m_prevp;
        links.m_prevp = nullptr;
        links.m_nextp = nullptr;
    }

    // Swap elements of 2 lists
    व्योम swap(ListType& other) {
        std::swap(m_headp, other.m_headp);
        std::swap(m_lastp, other.m_lastp);
    }

    // Take elements from 'other' and link (insert) them all before the given position.
    व्योम splice(const_iterator pos, ListType& other) {
        अगर (empty()) {
            swap(other);
        } अन्यथा अगर (other.empty()) {
            वापस;
        } अन्यथा {
            UASSERT(pos == end(), "Sorry, only splicing at the end is implemented at the moment");
            toLinks(m_lastp).m_nextp = other.m_headp;
            toLinks(other.m_headp).m_prevp = m_lastp;
            m_lastp = other.m_lastp;
            other.m_headp = nullptr;
            other.m_lastp = nullptr;
        }
    }

    // This is O(n)!
    माप_प्रकार आकार() स्थिर {
        माप_प्रकार परिणाम = 0;
        क्रम (स्वतः it = begin(); it != end(); ++it) ++परिणाम;
        वापस परिणाम;
    }
};

#पूर्ण_अगर  // Guard
