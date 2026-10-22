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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3HASHTABLE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3HASHTABLE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3StdFuture.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <new>
#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>

namespace V3_Hash_ਸਾਰਣੀ_Internals {

constexpr ਮਾਪ_t MIN_ਸਮਰੱਥਾ = 16;  // Smallest table allocated
constexpr ਮਾਪ_t ਲੱਦੋ_ਗੁਣਕ_ਅੰਕ = 3;  // Numerator of the maximum load factor
constexpr ਮਾਪ_t ਲੱਦੋ_ਗੁਣਕ_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
ਸਾਂਚਾ <typename T_ਕੁੰਜੀ>
ਬਣਤਰ V3_Hash_ਸਾਰਣੀ_ਕੁੰਜੀ_Is_ਇੰਦਰਾਜ final {
    using ਕੁੰਜੀ_2 = T_ਕੁੰਜੀ;  // What it yields, so the table need not deduce it
    ਅਚਲ T_ਕੁੰਜੀ& ਸੰਚਾਲਕ()(ਅਚਲ T_ਕੁੰਜੀ& ਇੰਦਰਾਜ) ਅਚਲ { ਵਾਪਸ ਇੰਦਰਾਜ; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_Val>
ਬਣਤਰ V3_Hash_ਸਾਰਣੀ_ਕੁੰਜੀ_Is_ਪਹਿਲਾ final {
    using ਕੁੰਜੀ_2 = T_ਕੁੰਜੀ;  // What it yields, so the table need not deduce it
    ਅਚਲ T_ਕੁੰਜੀ& ਸੰਚਾਲਕ()(ਅਚਲ std::pair<T_ਕੁੰਜੀ, T_Val>& ਇੰਦਰਾਜ) ਅਚਲ { ਵਾਪਸ ਇੰਦਰਾਜ.ਪਹਿਲਾ; }
    ਸਥਿਰ T_Val& ਮੁੱਲ_ਦਾ(std::pair<T_ਕੁੰਜੀ, T_Val>& ਇੰਦਰਾਜ) { ਵਾਪਸ ਇੰਦਰਾਜ.ਦੂਜਾ; }
};

ਖਾਲੀ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
ਸਾਂਚਾ <typename T_ਇੰਦਰਾਜ, typename T_Hash, typename T_Equal, typename T_ਕੁੰਜੀ_ਦਾ>
class V3_Hash_ਸਾਰਣੀ VL_NOT_FINAL {
ਜਨਤਕ:
    // TYPES
    using ਇੰਦਰਾਜ_2 = T_ਇੰਦਰਾਜ;  // What is stored
    using ਕੁੰਜੀ_2 = typename T_ਕੁੰਜੀ_ਦਾ::ਕੁੰਜੀ_2;  // What entries are looked up by

ਨਿਜੀ:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    using ValidHash = vlstd::is_invocable_r<ਮਾਪ_t, ਅਚਲ T_Hash&, ਅਚਲ T_ਦਲੀਲਾਂ&...>;

    // Holds if the equality accepts a key and such a lookup key
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    using ValidEqual = vlstd::is_invocable_r<bool, ਅਚਲ T_Equal&, ਅਚਲ ਕੁੰਜੀ_2&, ਅਚਲ T_ਦਲੀਲਾਂ&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<ਕੁੰਜੀ_2>::ਮੁੱਲ, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<ਕੁੰਜੀ_2>::ਮੁੱਲ, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    ਬਣਤਰ ਥਾਂ final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        ਸੰਘ {
            ਇੰਦਰਾਜ_2 m_ਇੰਦਰਾਜ;  // Stored entry; alive only while the slot is occupied
        };
        ਮਾਪ_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        ਥਾਂ() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~ਥਾਂ() {
            ਜੇ (!isFree()) destroy();
        }
        ਥਾਂ(ਅਚਲ ਥਾਂ&) = delete;
        ਥਾਂ(ਥਾਂ&&) = delete;
        ਅਚਲ ਥਾਂ& ਸੰਚਾਲਕ=(ਅਚਲ ਥਾਂ&) = delete;
        ਥਾਂ& ਸੰਚਾਲਕ=(ਥਾਂ&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_ਇੰਦਰਾਜ) ਇੰਦਰਾਜ_2{std::move(that.m_ਇੰਦਰਾਜ)};
            m_hash = that.m_hash;
            that.destroy();
            ਵਾਪਸ *this;
        }

        bool isFree() ਅਚਲ { ਵਾਪਸ !m_hash; }

        // Construct the entry of this free slot from the given entry
        ਖਾਲੀ construct(ਮਾਪ_t hash, ਇੰਦਰਾਜ_2&& ਇੰਦਰਾਜ) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_ਇੰਦਰਾਜ) ਇੰਦਰਾਜ_2{std::move(ਇੰਦਰਾਜ)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        ਖਾਲੀ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_ਇੰਦਰਾਜ.~ਇੰਦਰਾਜ_2();
            m_hash = 0;
        }
    };

ਜਨਤਕ:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        ਮਿੱਤਰ class V3_Hash_ਸਾਰਣੀ;

        ਥਾਂ* m_slotp = nullptr;  // The slot iterated, or the end of the table
        ਥਾਂ* m_endp = nullptr;  // One past the last slot

        iterator(ਥਾਂ* slotp, ਥਾਂ* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    ਜਨਤਕ:
        iterator() = ਮੂਲ;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ਅਚਲ ਇੰਦਰਾਜ_2& ਸੰਚਾਲਕ*() ਅਚਲ { ਵਾਪਸ m_slotp->m_ਇੰਦਰਾਜ; }
        ਅਚਲ ਇੰਦਰਾਜ_2* ਸੰਚਾਲਕ->() ਅਚਲ { ਵਾਪਸ &m_slotp->m_ਇੰਦਰਾਜ; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        ਸਵੈਚਾਲਿਤ& ਮੁੱਲ() ਅਚਲ { ਵਾਪਸ T_ਕੁੰਜੀ_ਦਾ::ਮੁੱਲ_ਦਾ(m_slotp->m_ਇੰਦਰਾਜ); }
        // Pre-increment, skipping the free slots
        iterator& ਸੰਚਾਲਕ++() {
            ਜਦੋਂ (++m_slotp != m_endp && m_slotp->isFree()) {}
            ਵਾਪਸ *this;
        }
        bool ਸੰਚਾਲਕ==(ਅਚਲ iterator& that) ਅਚਲ { ਵਾਪਸ m_slotp == that.m_slotp; }
        bool ਸੰਚਾਲਕ!=(ਅਚਲ iterator& that) ਅਚਲ { ਵਾਪਸ m_slotp != that.m_slotp; }
    };

ਨਿਜੀ:
    // STATE
    std::unique_ptr<ਥਾਂ[]> m_ਸਾਰਣੀ;  // The table, null when unallocated
    ਮਾਪ_t m_ਸਮਰੱਥਾ = 0;  // Number of slots in the table, a power of two, or zero
    ਮਾਪ_t m_ਮਾਪ = 0;  // Number of occupied slots
    VL_NO_UNIQUE_ਪਤਾ_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_ਪਤਾ_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_ਪਤਾ_CXX20 T_ਕੁੰਜੀ_ਦਾ m_ਕੁੰਜੀ_ਦਾ;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    ਮਾਪ_t hash_ਦਾ(ਅਚਲ T_ਦਲੀਲਾਂ&... ਦਲੀਲਾਂ) ਅਚਲ {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr ਮਾਪ_t ਵਰਤਿਆ_ਦੋਆਧਾਰੀ_ਅੰਕ = ਮਾਪ_t{1} << (ਆਕਾਰ(ਮਾਪ_t) * 8 - 1);
        ਵਾਪਸ static_cast<ਮਾਪ_t>(m_hash(ਦਲੀਲਾਂ...)) | ਵਰਤਿਆ_ਦੋਆਧਾਰੀ_ਅੰਕ;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    ਮਾਪ_t free_ਥਾਂ(ਮਾਪ_t hash) ਅਚਲ {
        ਅਚਲ ਮਾਪ_t ਨਕਾਬ = m_ਸਮਰੱਥਾ - 1;
        ਮਾਪ_t i = hash & ਨਕਾਬ;
        ਜਦੋਂ (!m_ਸਾਰਣੀ[i].isFree()) i = (i + 1) & ਨਕਾਬ;
        ਵਾਪਸ i;
    }

    // Resize to the given number of slots, which must fit all entries
    ਖਾਲੀ resize(ਮਾਪ_t ਗਿਣਤੀ_2) {
        UDEBUGONLY(UASSERT(ਗਿਣਤੀ_2 && !(ਗਿਣਤੀ_2 & (ਗਿਣਤੀ_2 - 1)), "Capacity not a power of 2"););
        ਅਚਲ std::unique_ptr<ਥਾਂ[]> old_ਸਾਰਣੀ{std::move(m_ਸਾਰਣੀ)};
        ਅਚਲ ਮਾਪ_t old_ਸਮਰੱਥਾ = m_ਸਮਰੱਥਾ;
        m_ਸਾਰਣੀ = std::make_unique<ਥਾਂ[]>(ਗਿਣਤੀ_2);
        m_ਸਮਰੱਥਾ = ਗਿਣਤੀ_2;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        ਲਈ (ਮਾਪ_t i = 0; i < old_ਸਮਰੱਥਾ; ++i) {
            ਥਾਂ& ਥਾਂ_2 = old_ਸਾਰਣੀ[i];
            ਜੇ (!ਥਾਂ_2.isFree()) m_ਸਾਰਣੀ[free_ਥਾਂ(ਥਾਂ_2.m_hash)] = std::move(ਥਾਂ_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    ਮਾਪ_t probe(ਮਾਪ_t hash, ਅਚਲ T_ਦਲੀਲਾਂ&... ਦਲੀਲਾਂ) ਅਚਲ {
        UDEBUGONLY(UASSERT(m_ਸਾਰਣੀ, "Table must be allocated"););
        ਅਚਲ ਮਾਪ_t ਨਕਾਬ = m_ਸਮਰੱਥਾ - 1;
        ਮਾਪ_t i = hash & ਨਕਾਬ;
        ਜਦੋਂ (!m_ਸਾਰਣੀ[i].isFree()) {
            ਅਚਲ ਥਾਂ& ਥਾਂ_2 = m_ਸਾਰਣੀ[i];
            ਜੇ (ਥਾਂ_2.m_hash == hash && m_equal(m_ਕੁੰਜੀ_ਦਾ(ਥਾਂ_2.m_ਇੰਦਰਾਜ), ਦਲੀਲਾਂ...)) ਰੋਕੋ;
            i = (i + 1) & ਨਕਾਬ;
        }
        ਵਾਪਸ i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    ਸਾਂਚਾ <ਮਾਪ_t... N_ਕੁੰਜੀ, typename T_All>
    std::pair<iterator, bool> ਪਾਓ_Lazy_Impl(std::ਸੂਚਕਾਂਕ_sequence<N_ਕੁੰਜੀ...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_ਕੁੰਜੀ, T_All>...>::ਮੁੱਲ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_ਕੁੰਜੀ, T_All>...>::ਮੁੱਲ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ਅਚਲ ਮਾਪ_t hash = hash_ਦਾ(std::get<N_ਕੁੰਜੀ>(all)...);
        // Allocate on the first insertion
        ਜੇ (VL_UNLIKELY(!m_ਸਮਰੱਥਾ)) resize(V3_Hash_ਸਾਰਣੀ_Internals::MIN_ਸਮਰੱਥਾ);
        // Find the slot for the entry
        ਥਾਂ* slotp = m_ਸਾਰਣੀ.get() + probe(hash, std::get<N_ਕੁੰਜੀ>(all)...);
        // If occupied, it's the equivalent, and we are done
        ਜੇ (!slotp->isFree()) ਵਾਪਸ {iterator{slotp, m_ਸਾਰਣੀ.get() + m_ਸਮਰੱਥਾ}, false};
        // Table is growing
        ++m_ਮਾਪ;
        // Increase if necessary by load factor
        ਜੇ (VL_UNLIKELY(m_ਮਾਪ * V3_Hash_ਸਾਰਣੀ_Internals::ਲੱਦੋ_ਗੁਣਕ_DEN
                        > m_ਸਮਰੱਥਾ * V3_Hash_ਸਾਰਣੀ_Internals::ਲੱਦੋ_ਗੁਣਕ_ਅੰਕ)) {
            resize(m_ਸਮਰੱਥਾ * 2);
            slotp = m_ਸਾਰਣੀ.get() + free_ਥਾਂ(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<ਆਕਾਰ...(N_ਕੁੰਜੀ)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        ਅਚਲ ਕੁੰਜੀ_2& ਕੁੰਜੀ = m_ਕੁੰਜੀ_ਦਾ(slotp->m_ਇੰਦਰਾਜ);
        UASSERT(hash_ਦਾ(ਕੁੰਜੀ) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(ਕੁੰਜੀ, std::get<N_ਕੁੰਜੀ>(all)...),
                "Created entry does not match the key it was looked up with");
#ਜੇ_ਅੰਤ
        // Return newly create entry
        ਵਾਪਸ {iterator{slotp, m_ਸਾਰਣੀ.get() + m_ਸਮਰੱਥਾ}, true};
    }

ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTORS
    V3_Hash_ਸਾਰਣੀ() = ਮੂਲ;
    V3_Hash_ਸਾਰਣੀ(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3_Hash_ਸਾਰਣੀ() = ਮੂਲ;
    VL_UNCOPYABLE(V3_Hash_ਸਾਰਣੀ);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3_Hash_ਸਾਰਣੀ(V3_Hash_ਸਾਰਣੀ&& that)
        : m_ਸਾਰਣੀ{std::move(that.m_ਸਾਰਣੀ)}
        , m_ਸਮਰੱਥਾ{that.m_ਸਮਰੱਥਾ}
        , m_ਮਾਪ{that.m_ਮਾਪ}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_ਕੁੰਜੀ_ਦਾ{std::move(that.m_ਕੁੰਜੀ_ਦਾ)} {
        that.m_ਸਮਰੱਥਾ = 0;
        that.m_ਮਾਪ = 0;
    }
    V3_Hash_ਸਾਰਣੀ& ਸੰਚਾਲਕ=(V3_Hash_ਸਾਰਣੀ&& that) {
        m_ਸਾਰਣੀ = std::move(that.m_ਸਾਰਣੀ);  // Frees the table this held, if any
        m_ਸਮਰੱਥਾ = that.m_ਸਮਰੱਥਾ;
        m_ਮਾਪ = that.m_ਮਾਪ;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_ਕੁੰਜੀ_ਦਾ = std::move(that.m_ਕੁੰਜੀ_ਦਾ);
        that.m_ਸਮਰੱਥਾ = 0;
        that.m_ਮਾਪ = 0;
        ਵਾਪਸ *this;
    }

ਜਨਤਕ:
    // METHODS
    ਮਾਪ_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਮਾਪ; }
    bool ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ !m_ਮਾਪ; }

    iterator begin() ਅਚਲ {
        ਥਾਂ* ਅਚਲ endp = m_ਸਾਰਣੀ.get() + m_ਸਮਰੱਥਾ;
        ਥਾਂ* slotp = m_ਸਾਰਣੀ.get();
        ਜਦੋਂ (slotp != endp && slotp->isFree()) ++slotp;
        ਵਾਪਸ iterator{slotp, endp};
    }
    iterator ਸਿਰਾ() ਅਚਲ {
        ਥਾਂ* ਅਚਲ endp = m_ਸਾਰਣੀ.get() + m_ਸਮਰੱਥਾ;
        ਵਾਪਸ iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    ਖਾਲੀ reserve(ਮਾਪ_t ਗਿਣਤੀ_2) {
        ਮਾਪ_t ਸਮਰੱਥਾ = V3_Hash_ਸਾਰਣੀ_Internals::MIN_ਸਮਰੱਥਾ;
        ਜਦੋਂ (ਸਮਰੱਥਾ * V3_Hash_ਸਾਰਣੀ_Internals::ਲੱਦੋ_ਗੁਣਕ_ਅੰਕ
               < ਗਿਣਤੀ_2 * V3_Hash_ਸਾਰਣੀ_Internals::ਲੱਦੋ_ਗੁਣਕ_DEN)
            ਸਮਰੱਥਾ *= 2;
        ਜੇ (ਸਮਰੱਥਾ > m_ਸਮਰੱਥਾ) resize(ਸਮਰੱਥਾ);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    ਖਾਲੀ clear() {
        ਲਈ (ਮਾਪ_t i = 0; i < m_ਸਮਰੱਥਾ; ++i) {
            ਜੇ (!m_ਸਾਰਣੀ[i].isFree()) m_ਸਾਰਣੀ[i].destroy();
        }
        m_ਮਾਪ = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    iterator ਲੱਭੋ(ਅਚਲ T_ਦਲੀਲਾਂ&... ਦਲੀਲਾਂ) ਅਚਲ {
        static_assert(ValidHash<T_ਦਲੀਲਾਂ...>::ਮੁੱਲ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_ਦਲੀਲਾਂ...>::ਮੁੱਲ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ਜੇ (!m_ਮਾਪ) ਵਾਪਸ ਸਿਰਾ();  // Nothing to find, and this also covers there being no table
        ਥਾਂ* ਅਚਲ slotp = m_ਸਾਰਣੀ.get() + probe(hash_ਦਾ(ਦਲੀਲਾਂ...), ਦਲੀਲਾਂ...);
        ਵਾਪਸ slotp->isFree() ? ਸਿਰਾ() : iterator{slotp, m_ਸਾਰਣੀ.get() + m_ਸਮਰੱਥਾ};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> ਪਾਓ(ਅਚਲ ਇੰਦਰਾਜ_2& ਇੰਦਰਾਜ) {
        static_assert(std::is_copy_constructible<ਇੰਦਰਾਜ_2>::ਮੁੱਲ,
                      "'Entry' must be copy constructible to use 'insert'");
        ਵਾਪਸ ਪਾਓ_Lazy(m_ਕੁੰਜੀ_ਦਾ(ਇੰਦਰਾਜ), [&ਇੰਦਰਾਜ]() -> ਇੰਦਰਾਜ_2 { ਵਾਪਸ ਇੰਦਰਾਜ; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    std::pair<iterator, bool> ਪਾਓ_Lazy(T_ਦਲੀਲਾਂ&&... ਦਲੀਲਾਂ) {
        static_assert(ਆਕਾਰ...(T_ਦਲੀਲਾਂ) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<ਆਕਾਰ...(T_ਦਲੀਲਾਂ) - 1, std::tuple<T_ਦਲੀਲਾਂ...>>;
        static_assert(vlstd::is_invocable_r<ਇੰਦਰਾਜ_2, Callable>::ਮੁੱਲ,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        ਵਾਪਸ ਪਾਓ_Lazy_Impl(std::make_ਸੂਚਕਾਂਕ_sequence<ਆਕਾਰ...(T_ਦਲੀਲਾਂ) - 1>{},
                              std::forward_as_tuple(std::forward<T_ਦਲੀਲਾਂ>(ਦਲੀਲਾਂ)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    bool ਸ਼ਾਮਲ_ਹੈ(ਅਚਲ T_ਦਲੀਲਾਂ&... ਦਲੀਲਾਂ) ਅਚਲ {
        ਵਾਪਸ ਲੱਭੋ(ਦਲੀਲਾਂ...) != ਸਿਰਾ();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    ਸਾਂਚਾ <typename... T_ਦਲੀਲਾਂ>
    bool erase(ਅਚਲ T_ਦਲੀਲਾਂ&... ਦਲੀਲਾਂ) {
        ਅਚਲ iterator it = ਲੱਭੋ(ਦਲੀਲਾਂ...);
        ਜੇ (it == ਸਿਰਾ()) ਵਾਪਸ false;
        erase(it);
        ਵਾਪਸ true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    ਖਾਲੀ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != ਸਿਰਾ() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ਅਚਲ ਮਾਪ_t ਨਕਾਬ = m_ਸਮਰੱਥਾ - 1;
        ਮਾਪ_t i = static_cast<ਮਾਪ_t>(it.m_slotp - m_ਸਾਰਣੀ.get());
        // Destroy the entry
        m_ਸਾਰਣੀ[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_ਮਾਪ;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        ਮਾਪ_t j = i;
        ਜਦੋਂ (true) {
            j = (j + 1) & ਨਕਾਬ;
            ਥਾਂ& ਥਾਂ_2 = m_ਸਾਰਣੀ[j];
            ਜੇ (ਥਾਂ_2.isFree()) ਰੋਕੋ;
            // Move back if its home position does not lie in the cyclic range (i, j]
            ਜੇ (((j - (ਥਾਂ_2.m_hash & ਨਕਾਬ)) & ਨਕਾਬ) >= ((j - i) & ਨਕਾਬ)) {
                m_ਸਾਰਣੀ[i] = std::move(ਥਾਂ_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_Hash = std::hash<T_ਕੁੰਜੀ>,
          typename T_Equal = std::equal_ਮੰਜ਼ਿਲ<T_ਕੁੰਜੀ>>
class V3_Hash_ਸੈੱਟ final : ਜਨਤਕ V3_Hash_ਸਾਰਣੀ<T_ਕੁੰਜੀ, T_Hash, T_Equal,
                                           V3_Hash_ਸਾਰਣੀ_Internals::V3_Hash_ਸਾਰਣੀ_ਕੁੰਜੀ_Is_ਇੰਦਰਾਜ<T_ਕੁੰਜੀ>> {
    using Super
        = V3_Hash_ਸਾਰਣੀ<T_ਕੁੰਜੀ, T_Hash, T_Equal, V3_Hash_ਸਾਰਣੀ_Internals::V3_Hash_ਸਾਰਣੀ_ਕੁੰਜੀ_Is_ਇੰਦਰਾਜ<T_ਕੁੰਜੀ>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_ਕੁੰਜੀ>::ਮੁੱਲ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ਕੁੰਜੀ>::ਮੁੱਲ, "'T_Key' must be destructible");

ਜਨਤਕ:
    // CONSTRUCTORS
    V3_Hash_ਸੈੱਟ() = ਮੂਲ;
    V3_Hash_ਸੈੱਟ(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_Val, typename T_Hash = std::hash<T_ਕੁੰਜੀ>,
          typename T_Equal = std::equal_ਮੰਜ਼ਿਲ<T_ਕੁੰਜੀ>>
class V3HashMap final
    : ਜਨਤਕ V3_Hash_ਸਾਰਣੀ<std::pair<T_ਕੁੰਜੀ, T_Val>, T_Hash, T_Equal,
                         V3_Hash_ਸਾਰਣੀ_Internals::V3_Hash_ਸਾਰਣੀ_ਕੁੰਜੀ_Is_ਪਹਿਲਾ<T_ਕੁੰਜੀ, T_Val>> {
    using Super = V3_Hash_ਸਾਰਣੀ<std::pair<T_ਕੁੰਜੀ, T_Val>, T_Hash, T_Equal,
                              V3_Hash_ਸਾਰਣੀ_Internals::V3_Hash_ਸਾਰਣੀ_ਕੁੰਜੀ_Is_ਪਹਿਲਾ<T_ਕੁੰਜੀ, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_ਕੁੰਜੀ>::ਮੁੱਲ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ਕੁੰਜੀ>::ਮੁੱਲ, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::ਮੁੱਲ, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::ਮੁੱਲ, "'T_Val' must be destructible");

ਜਨਤਕ:
    // TYPES
    using ਮੁੱਲ_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = ਮੂਲ;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#ਜੇ_ਅੰਤ  // Guard
