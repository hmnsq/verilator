// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Open addressing hash set and hash map
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
//
// An open addressing, linear probing hash table, with backward shift deletion.
// Usable as V3HashSet or V3HashMap. The benefit of these over
// std::unordered_set and std::unordered_map is far better memory locality
// during lookup, and fewer dynamic memory allocations (which also means less
// heap fragmentation). Consider using these if profiling shows that the
// unordered STL collections contribute a significant cost to an algorithm.
//
// Four types tell the table what it holds: the entry, a hash, an equality, and
// a key extractor yielding the lookup key of an entry. The extractor is what
// lets one table serve both roles: a set's entry is its own key, a map's is a
// pair keyed by its first. The hash and equality hence only ever see keys,
// never entries. V3HashSet and V3HashMap at the bottom of this file derive
// from the table, pairing it with the extractor that suits each.
//
// Those two work on keys via Hash and Equal functors as in std::unordered_set
// or std::unordered_map, but lookup is always heterogeneous, with no
// is_transparent to opt in like in the STL, and a lookup key can be spelled as
// several arguments, being the parts a key is made of. An entry can hence be
// looked up without one at hand, as when it is only created on a miss. The
// functors must provide call operators as const members, for the key of an
// entry and for every lookup key spelling used:
//
//   size_t Hash::operator()(const T_Key&) const
//   size_t Hash::operator()(<lookup keys>...) const
//   bool Equal::operator()(const T_Key&, const T_Key&) const
//   bool Equal::operator()(const T_Key&, <lookup keys>...) const
//
// with equal keys hashing equal, as usual, and consistently across the
// spellings.
//
// As only entries are stored, a slot is just a hash and an entry, so probing
// touches few cache lines. The table is doubled when an insertion would take
// it over the maximum load factor, or sized up front with 'reserve', to keep
// the probe runs short.
//
// Entries are referred to by iterators, as in the STL containers, but unlike
// STL containers, mapped values in a V3HashMap are only mutable through
// 'iterator::value()'. Iterators and entry addresses stay valid until the table grows,
// an entry is erased, or the table is cleared; any of these invalidates all
// of them.
//
// Erasure uses backward shift deletion: entries following the hole are moved
// back over it where their probe run ran through it (no tombstones).
//
//*************************************************************************

#यदि_अपरिभाषितम् VERILATOR_V3HASHTABLE_H_
#परिभाषय VERILATOR_V3HASHTABLE_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3StdFuture.h"

#अन्तर्भाव <functional>
#अन्तर्भाव <memory>
#अन्तर्भाव <new>
#अन्तर्भाव <tuple>
#अन्तर्भाव <type_traits>
#अन्तर्भाव <utility>

namespace V3_Hash_सारणी_Internals {

constexpr आकार_t MIN_क्षमता = 16;  // Smallest table allocated
constexpr आकार_t आरोपय_गुणकः_अङ्कः = 3;  // Numerator of the maximum load factor
constexpr आकार_t आरोपय_गुणकः_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
प्रतिरूपम् <typename T_कुञ्चिका>
संरचना V3_Hash_सारणी_कुञ्चिका_Is_प्रविष्टिः final {
    using कुञ्चिका_2 = T_कुञ्चिका;  // What it yields, so the table need not deduce it
    ध्रुव T_कुञ्चिका& संक्रियकः()(ध्रुव T_कुञ्चिका& प्रविष्टिः) ध्रुव { प्रत्यागम प्रविष्टिः; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
प्रतिरूपम् <typename T_कुञ्चिका, typename T_Val>
संरचना V3_Hash_सारणी_कुञ्चिका_Is_First final {
    using कुञ्चिका_2 = T_कुञ्चिका;  // What it yields, so the table need not deduce it
    ध्रुव T_कुञ्चिका& संक्रियकः()(ध्रुव std::pair<T_कुञ्चिका, T_Val>& प्रविष्टिः) ध्रुव { प्रत्यागम प्रविष्टिः.first; }
    स्थिर T_Val& मानम्_सम्बन्धि(std::pair<T_कुञ्चिका, T_Val>& प्रविष्टिः) { प्रत्यागम प्रविष्टिः.second; }
};

शून्य selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
प्रतिरूपम् <typename T_प्रविष्टिः, typename T_Hash, typename T_Equal, typename T_कुञ्चिका_सम्बन्धि>
class V3_Hash_सारणी VL_NOT_FINAL {
सार्वजनिक:
    // TYPES
    using प्रविष्टिः_2 = T_प्रविष्टिः;  // What is stored
    using कुञ्चिका_2 = typename T_कुञ्चिका_सम्बन्धि::कुञ्चिका_2;  // What entries are looked up by

गुप्त:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    प्रतिरूपम् <typename... T_तर्काः>
    using ValidHash = vlstd::is_invocable_r<आकार_t, ध्रुव T_Hash&, ध्रुव T_तर्काः&...>;

    // Holds if the equality accepts a key and such a lookup key
    प्रतिरूपम् <typename... T_तर्काः>
    using ValidEqual = vlstd::is_invocable_r<bool, ध्रुव T_Equal&, ध्रुव कुञ्चिका_2&, ध्रुव T_तर्काः&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<कुञ्चिका_2>::मानम्, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<कुञ्चिका_2>::मानम्, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    संरचना Slot final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        सङ्घ {
            प्रविष्टिः_2 m_प्रविष्टिः;  // Stored entry; alive only while the slot is occupied
        };
        आकार_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        Slot() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~Slot() {
            यदि (!isFree()) destroy();
        }
        Slot(ध्रुव Slot&) = delete;
        Slot(Slot&&) = delete;
        ध्रुव Slot& संक्रियकः=(ध्रुव Slot&) = delete;
        Slot& संक्रियकः=(Slot&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_प्रविष्टिः) प्रविष्टिः_2{std::move(that.m_प्रविष्टिः)};
            m_hash = that.m_hash;
            that.destroy();
            प्रत्यागम *this;
        }

        bool isFree() ध्रुव { प्रत्यागम !m_hash; }

        // Construct the entry of this free slot from the given entry
        शून्य construct(आकार_t hash, प्रविष्टिः_2&& प्रविष्टिः) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_प्रविष्टिः) प्रविष्टिः_2{std::move(प्रविष्टिः)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        शून्य destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_प्रविष्टिः.~प्रविष्टिः_2();
            m_hash = 0;
        }
    };

सार्वजनिक:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        मित्रम् class V3_Hash_सारणी;

        Slot* m_slotp = nullptr;  // The slot iterated, or the end of the table
        Slot* m_endp = nullptr;  // One past the last slot

        iterator(Slot* slotp, Slot* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    सार्वजनिक:
        iterator() = स्वभाव;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ध्रुव प्रविष्टिः_2& संक्रियकः*() ध्रुव { प्रत्यागम m_slotp->m_प्रविष्टिः; }
        ध्रुव प्रविष्टिः_2* संक्रियकः->() ध्रुव { प्रत्यागम &m_slotp->m_प्रविष्टिः; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        स्वचालित& मानम्() ध्रुव { प्रत्यागम T_कुञ्चिका_सम्बन्धि::मानम्_सम्बन्धि(m_slotp->m_प्रविष्टिः); }
        // Pre-increment, skipping the free slots
        iterator& संक्रियकः++() {
            यावत् (++m_slotp != m_endp && m_slotp->isFree()) {}
            प्रत्यागम *this;
        }
        bool संक्रियकः==(ध्रुव iterator& that) ध्रुव { प्रत्यागम m_slotp == that.m_slotp; }
        bool संक्रियकः!=(ध्रुव iterator& that) ध्रुव { प्रत्यागम m_slotp != that.m_slotp; }
    };

गुप्त:
    // STATE
    std::unique_ptr<Slot[]> m_सारणी;  // The table, null when unallocated
    आकार_t m_क्षमता = 0;  // Number of slots in the table, a power of two, or zero
    आकार_t m_आकार = 0;  // Number of occupied slots
    VL_NO_UNIQUE_स्थानाङ्कः_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_स्थानाङ्कः_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_स्थानाङ्कः_CXX20 T_कुञ्चिका_सम्बन्धि m_कुञ्चिका_सम्बन्धि;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    प्रतिरूपम् <typename... T_तर्काः>
    आकार_t hash_सम्बन्धि(ध्रुव T_तर्काः&... तर्काः) ध्रुव {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr आकार_t प्रयुक्तम्_BIT = आकार_t{1} << (परिमाण(आकार_t) * 8 - 1);
        प्रत्यागम static_cast<आकार_t>(m_hash(तर्काः...)) | प्रयुक्तम्_BIT;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    आकार_t freeSlot(आकार_t hash) ध्रुव {
        ध्रुव आकार_t आवरणम् = m_क्षमता - 1;
        आकार_t i = hash & आवरणम्;
        यावत् (!m_सारणी[i].isFree()) i = (i + 1) & आवरणम्;
        प्रत्यागम i;
    }

    // Resize to the given number of slots, which must fit all entries
    शून्य resize(आकार_t गणनम्) {
        UDEBUGONLY(UASSERT(गणनम् && !(गणनम् & (गणनम् - 1)), "Capacity not a power of 2"););
        ध्रुव std::unique_ptr<Slot[]> old_सारणी{std::move(m_सारणी)};
        ध्रुव आकार_t old_क्षमता = m_क्षमता;
        m_सारणी = std::make_unique<Slot[]>(गणनम्);
        m_क्षमता = गणनम्;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        प्रति (आकार_t i = 0; i < old_क्षमता; ++i) {
            Slot& slot = old_सारणी[i];
            यदि (!slot.isFree()) m_सारणी[freeSlot(slot.m_hash)] = std::move(slot);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    प्रतिरूपम् <typename... T_तर्काः>
    आकार_t probe(आकार_t hash, ध्रुव T_तर्काः&... तर्काः) ध्रुव {
        UDEBUGONLY(UASSERT(m_सारणी, "Table must be allocated"););
        ध्रुव आकार_t आवरणम् = m_क्षमता - 1;
        आकार_t i = hash & आवरणम्;
        यावत् (!m_सारणी[i].isFree()) {
            ध्रुव Slot& slot = m_सारणी[i];
            यदि (slot.m_hash == hash && m_equal(m_कुञ्चिका_सम्बन्धि(slot.m_प्रविष्टिः), तर्काः...)) भङ्ग;
            i = (i + 1) & आवरणम्;
        }
        प्रत्यागम i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    प्रतिरूपम् <आकार_t... N_कुञ्चिका, typename T_All>
    std::pair<iterator, bool> निवेशय_Lazy_Impl(std::सूचकांक_sequence<N_कुञ्चिका...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_कुञ्चिका, T_All>...>::मानम्,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_कुञ्चिका, T_All>...>::मानम्,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ध्रुव आकार_t hash = hash_सम्बन्धि(std::get<N_कुञ्चिका>(all)...);
        // Allocate on the first insertion
        यदि (VL_UNLIKELY(!m_क्षमता)) resize(V3_Hash_सारणी_Internals::MIN_क्षमता);
        // Find the slot for the entry
        Slot* slotp = m_सारणी.get() + probe(hash, std::get<N_कुञ्चिका>(all)...);
        // If occupied, it's the equivalent, and we are done
        यदि (!slotp->isFree()) प्रत्यागम {iterator{slotp, m_सारणी.get() + m_क्षमता}, false};
        // Table is growing
        ++m_आकार;
        // Increase if necessary by load factor
        यदि (VL_UNLIKELY(m_आकार * V3_Hash_सारणी_Internals::आरोपय_गुणकः_DEN
                        > m_क्षमता * V3_Hash_सारणी_Internals::आरोपय_गुणकः_अङ्कः)) {
            resize(m_क्षमता * 2);
            slotp = m_सारणी.get() + freeSlot(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<परिमाण...(N_कुञ्चिका)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#यदि_परिभाषितम् VL_DEBUG
        ध्रुव कुञ्चिका_2& कुञ्चिका = m_कुञ्चिका_सम्बन्धि(slotp->m_प्रविष्टिः);
        UASSERT(hash_सम्बन्धि(कुञ्चिका) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(कुञ्चिका, std::get<N_कुञ्चिका>(all)...),
                "Created entry does not match the key it was looked up with");
#यदि_अन्तः
        // Return newly create entry
        प्रत्यागम {iterator{slotp, m_सारणी.get() + m_क्षमता}, true};
    }

रक्षित:
    // CONSTRUCTORS
    V3_Hash_सारणी() = स्वभाव;
    V3_Hash_सारणी(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3_Hash_सारणी() = स्वभाव;
    VL_UNCOPYABLE(V3_Hash_सारणी);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3_Hash_सारणी(V3_Hash_सारणी&& that)
        : m_सारणी{std::move(that.m_सारणी)}
        , m_क्षमता{that.m_क्षमता}
        , m_आकार{that.m_आकार}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_कुञ्चिका_सम्बन्धि{std::move(that.m_कुञ्चिका_सम्बन्धि)} {
        that.m_क्षमता = 0;
        that.m_आकार = 0;
    }
    V3_Hash_सारणी& संक्रियकः=(V3_Hash_सारणी&& that) {
        m_सारणी = std::move(that.m_सारणी);  // Frees the table this held, if any
        m_क्षमता = that.m_क्षमता;
        m_आकार = that.m_आकार;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_कुञ्चिका_सम्बन्धि = std::move(that.m_कुञ्चिका_सम्बन्धि);
        that.m_क्षमता = 0;
        that.m_आकार = 0;
        प्रत्यागम *this;
    }

सार्वजनिक:
    // METHODS
    आकार_t आकार() ध्रुव { प्रत्यागम m_आकार; }
    bool रिक्तानि() ध्रुव { प्रत्यागम !m_आकार; }

    iterator begin() ध्रुव {
        Slot* ध्रुव endp = m_सारणी.get() + m_क्षमता;
        Slot* slotp = m_सारणी.get();
        यावत् (slotp != endp && slotp->isFree()) ++slotp;
        प्रत्यागम iterator{slotp, endp};
    }
    iterator अन्तः_2() ध्रुव {
        Slot* ध्रुव endp = m_सारणी.get() + m_क्षमता;
        प्रत्यागम iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    शून्य reserve(आकार_t गणनम्) {
        आकार_t क्षमता = V3_Hash_सारणी_Internals::MIN_क्षमता;
        यावत् (क्षमता * V3_Hash_सारणी_Internals::आरोपय_गुणकः_अङ्कः
               < गणनम् * V3_Hash_सारणी_Internals::आरोपय_गुणकः_DEN)
            क्षमता *= 2;
        यदि (क्षमता > m_क्षमता) resize(क्षमता);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    शून्य clear() {
        प्रति (आकार_t i = 0; i < m_क्षमता; ++i) {
            यदि (!m_सारणी[i].isFree()) m_सारणी[i].destroy();
        }
        m_आकार = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    प्रतिरूपम् <typename... T_तर्काः>
    iterator find(ध्रुव T_तर्काः&... तर्काः) ध्रुव {
        static_assert(ValidHash<T_तर्काः...>::मानम्,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_तर्काः...>::मानम्,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        यदि (!m_आकार) प्रत्यागम अन्तः_2();  // Nothing to find, and this also covers there being no table
        Slot* ध्रुव slotp = m_सारणी.get() + probe(hash_सम्बन्धि(तर्काः...), तर्काः...);
        प्रत्यागम slotp->isFree() ? अन्तः_2() : iterator{slotp, m_सारणी.get() + m_क्षमता};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> निवेशय(ध्रुव प्रविष्टिः_2& प्रविष्टिः) {
        static_assert(std::is_copy_constructible<प्रविष्टिः_2>::मानम्,
                      "'Entry' must be copy constructible to use 'insert'");
        प्रत्यागम निवेशय_Lazy(m_कुञ्चिका_सम्बन्धि(प्रविष्टिः), [&प्रविष्टिः]() -> प्रविष्टिः_2 { प्रत्यागम प्रविष्टिः; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    प्रतिरूपम् <typename... T_तर्काः>
    std::pair<iterator, bool> निवेशय_Lazy(T_तर्काः&&... तर्काः) {
        static_assert(परिमाण...(T_तर्काः) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<परिमाण...(T_तर्काः) - 1, std::tuple<T_तर्काः...>>;
        static_assert(vlstd::is_invocable_r<प्रविष्टिः_2, Callable>::मानम्,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        प्रत्यागम निवेशय_Lazy_Impl(std::make_सूचकांक_sequence<परिमाण...(T_तर्काः) - 1>{},
                              std::forward_as_tuple(std::forward<T_तर्काः>(तर्काः)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    प्रतिरूपम् <typename... T_तर्काः>
    bool अन्तर्भवति(ध्रुव T_तर्काः&... तर्काः) ध्रुव {
        प्रत्यागम find(तर्काः...) != अन्तः_2();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    प्रतिरूपम् <typename... T_तर्काः>
    bool erase(ध्रुव T_तर्काः&... तर्काः) {
        ध्रुव iterator it = find(तर्काः...);
        यदि (it == अन्तः_2()) प्रत्यागम false;
        erase(it);
        प्रत्यागम true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    शून्य erase(iterator it) {
        UDEBUGONLY(UASSERT(it != अन्तः_2() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ध्रुव आकार_t आवरणम् = m_क्षमता - 1;
        आकार_t i = static_cast<आकार_t>(it.m_slotp - m_सारणी.get());
        // Destroy the entry
        m_सारणी[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_आकार;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        आकार_t j = i;
        यावत् (true) {
            j = (j + 1) & आवरणम्;
            Slot& slot = m_सारणी[j];
            यदि (slot.isFree()) भङ्ग;
            // Move back if its home position does not lie in the cyclic range (i, j]
            यदि (((j - (slot.m_hash & आवरणम्)) & आवरणम्) >= ((j - i) & आवरणम्)) {
                m_सारणी[i] = std::move(slot);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

प्रतिरूपम् <typename T_कुञ्चिका, typename T_Hash = std::hash<T_कुञ्चिका>,
          typename T_Equal = std::equal_गन्तव्यम्<T_कुञ्चिका>>
class V3_Hash_समुच्चयः final : सार्वजनिक V3_Hash_सारणी<T_कुञ्चिका, T_Hash, T_Equal,
                                           V3_Hash_सारणी_Internals::V3_Hash_सारणी_कुञ्चिका_Is_प्रविष्टिः<T_कुञ्चिका>> {
    using Super
        = V3_Hash_सारणी<T_कुञ्चिका, T_Hash, T_Equal, V3_Hash_सारणी_Internals::V3_Hash_सारणी_कुञ्चिका_Is_प्रविष्टिः<T_कुञ्चिका>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_कुञ्चिका>::मानम्, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_कुञ्चिका>::मानम्, "'T_Key' must be destructible");

सार्वजनिक:
    // CONSTRUCTORS
    V3_Hash_समुच्चयः() = स्वभाव;
    V3_Hash_समुच्चयः(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

प्रतिरूपम् <typename T_कुञ्चिका, typename T_Val, typename T_Hash = std::hash<T_कुञ्चिका>,
          typename T_Equal = std::equal_गन्तव्यम्<T_कुञ्चिका>>
class V3HashMap final
    : सार्वजनिक V3_Hash_सारणी<std::pair<T_कुञ्चिका, T_Val>, T_Hash, T_Equal,
                         V3_Hash_सारणी_Internals::V3_Hash_सारणी_कुञ्चिका_Is_First<T_कुञ्चिका, T_Val>> {
    using Super = V3_Hash_सारणी<std::pair<T_कुञ्चिका, T_Val>, T_Hash, T_Equal,
                              V3_Hash_सारणी_Internals::V3_Hash_सारणी_कुञ्चिका_Is_First<T_कुञ्चिका, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_कुञ्चिका>::मानम्, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_कुञ्चिका>::मानम्, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::मानम्, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::मानम्, "'T_Val' must be destructible");

सार्वजनिक:
    // TYPES
    using मानम्_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = स्वभाव;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#यदि_अन्तः  // Guard
