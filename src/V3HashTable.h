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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3HASHTABLE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3HASHTABLE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3StdFuture.h"

#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <new>
#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <utility>

namespace V3_Hash_ಕೋಷ್ಟಕ_Internals {

constexpr ಗಾತ್ರ_t MIN_ಸಾಮರ್ಥ್ಯ = 16;  // Smallest table allocated
constexpr ಗಾತ್ರ_t ತುಂಬು_ಗುಣಕ_ಅಂಕೆ = 3;  // Numerator of the maximum load factor
constexpr ಗಾತ್ರ_t ತುಂಬು_ಗುಣಕ_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
ಮಾದರಿ <typename T_ಕೀಲಿ>
ರಚನೆ V3_Hash_ಕೋಷ್ಟಕ_ಕೀಲಿ_Is_ನಮೂದು final {
    using ಕೀಲಿ_2 = T_ಕೀಲಿ;  // What it yields, so the table need not deduce it
    ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ನಮೂದು) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ನಮೂದು; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_Val>
ರಚನೆ V3_Hash_ಕೋಷ್ಟಕ_ಕೀಲಿ_Is_ಮೊದಲ final {
    using ಕೀಲಿ_2 = T_ಕೀಲಿ;  // What it yields, so the table need not deduce it
    ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ std::pair<T_ಕೀಲಿ, T_Val>& ನಮೂದು) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ನಮೂದು.ಮೊದಲ; }
    ಸ್ಥಿರ T_Val& ಮೌಲ್ಯ_ದ(std::pair<T_ಕೀಲಿ, T_Val>& ನಮೂದು) { ಹಿಂತಿರುಗಿ ನಮೂದು.ಎರಡನೇ; }
};

ಶೂನ್ಯ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
ಮಾದರಿ <typename T_ನಮೂದು, typename T_Hash, typename T_Equal, typename T_ಕೀಲಿ_ದ>
class V3_Hash_ಕೋಷ್ಟಕ VL_NOT_FINAL {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using ನಮೂದು_2 = T_ನಮೂದು;  // What is stored
    using ಕೀಲಿ_2 = typename T_ಕೀಲಿ_ದ::ಕೀಲಿ_2;  // What entries are looked up by

ಖಾಸಗಿ:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    using ValidHash = vlstd::is_invocable_r<ಗಾತ್ರ_t, ಸ್ಥಿರಾಂಕ T_Hash&, ಸ್ಥಿರಾಂಕ T_ವಾದಗಳು&...>;

    // Holds if the equality accepts a key and such a lookup key
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    using ValidEqual = vlstd::is_invocable_r<bool, ಸ್ಥಿರಾಂಕ T_Equal&, ಸ್ಥಿರಾಂಕ ಕೀಲಿ_2&, ಸ್ಥಿರಾಂಕ T_ವಾದಗಳು&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<ಕೀಲಿ_2>::ಮೌಲ್ಯ, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<ಕೀಲಿ_2>::ಮೌಲ್ಯ, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    ರಚನೆ ಸ್ಥಾನ final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        ಒಕ್ಕೂಟ {
            ನಮೂದು_2 m_ನಮೂದು;  // Stored entry; alive only while the slot is occupied
        };
        ಗಾತ್ರ_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        ಸ್ಥಾನ() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~ಸ್ಥಾನ() {
            ಆದರೆ (!isFree()) destroy();
        }
        ಸ್ಥಾನ(ಸ್ಥಿರಾಂಕ ಸ್ಥಾನ&) = delete;
        ಸ್ಥಾನ(ಸ್ಥಾನ&&) = delete;
        ಸ್ಥಿರಾಂಕ ಸ್ಥಾನ& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ ಸ್ಥಾನ&) = delete;
        ಸ್ಥಾನ& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಾನ&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_ನಮೂದು) ನಮೂದು_2{std::move(that.m_ನಮೂದು)};
            m_hash = that.m_hash;
            that.destroy();
            ಹಿಂತಿರುಗಿ *this;
        }

        bool isFree() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_hash; }

        // Construct the entry of this free slot from the given entry
        ಶೂನ್ಯ construct(ಗಾತ್ರ_t hash, ನಮೂದು_2&& ನಮೂದು) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_ನಮೂದು) ನಮೂದು_2{std::move(ನಮೂದು)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        ಶೂನ್ಯ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_ನಮೂದು.~ನಮೂದು_2();
            m_hash = 0;
        }
    };

ಸಾರ್ವಜನಿಕ:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        ಮಿತ್ರ class V3_Hash_ಕೋಷ್ಟಕ;

        ಸ್ಥಾನ* m_slotp = nullptr;  // The slot iterated, or the end of the table
        ಸ್ಥಾನ* m_endp = nullptr;  // One past the last slot

        iterator(ಸ್ಥಾನ* slotp, ಸ್ಥಾನ* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    ಸಾರ್ವಜನಿಕ:
        iterator() = ಪೂರ್ವನಿಯೋಜಿತ;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ಸ್ಥಿರಾಂಕ ನಮೂದು_2& ಕ್ರಿಯಾಕಾರಕ*() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_slotp->m_ನಮೂದು; }
        ಸ್ಥಿರಾಂಕ ನಮೂದು_2* ಕ್ರಿಯಾಕಾರಕ->() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ &m_slotp->m_ನಮೂದು; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        ಸ್ವಯಂಚಾಲಿತ& ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ T_ಕೀಲಿ_ದ::ಮೌಲ್ಯ_ದ(m_slotp->m_ನಮೂದು); }
        // Pre-increment, skipping the free slots
        iterator& ಕ್ರಿಯಾಕಾರಕ++() {
            ಆಗ (++m_slotp != m_endp && m_slotp->isFree()) {}
            ಹಿಂತಿರುಗಿ *this;
        }
        bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ iterator& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_slotp == that.m_slotp; }
        bool ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ iterator& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_slotp != that.m_slotp; }
    };

ಖಾಸಗಿ:
    // STATE
    std::unique_ptr<ಸ್ಥಾನ[]> m_ಕೋಷ್ಟಕ;  // The table, null when unallocated
    ಗಾತ್ರ_t m_ಸಾಮರ್ಥ್ಯ = 0;  // Number of slots in the table, a power of two, or zero
    ಗಾತ್ರ_t m_ಗಾತ್ರ = 0;  // Number of occupied slots
    VL_NO_UNIQUE_ವಿಳಾಸ_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_ವಿಳಾಸ_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_ವಿಳಾಸ_CXX20 T_ಕೀಲಿ_ದ m_ಕೀಲಿ_ದ;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    ಗಾತ್ರ_t hash_ದ(ಸ್ಥಿರಾಂಕ T_ವಾದಗಳು&... ವಾದಗಳು) ಸ್ಥಿರಾಂಕ {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr ಗಾತ್ರ_t ಬಳಸಿದ_ಬಿಟ್ = ಗಾತ್ರ_t{1} << (ಗಾತ್ರ(ಗಾತ್ರ_t) * 8 - 1);
        ಹಿಂತಿರುಗಿ static_cast<ಗಾತ್ರ_t>(m_hash(ವಾದಗಳು...)) | ಬಳಸಿದ_ಬಿಟ್;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    ಗಾತ್ರ_t free_ಸ್ಥಾನ(ಗಾತ್ರ_t hash) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮುಖವಾಡ = m_ಸಾಮರ್ಥ್ಯ - 1;
        ಗಾತ್ರ_t i = hash & ಮುಖವಾಡ;
        ಆಗ (!m_ಕೋಷ್ಟಕ[i].isFree()) i = (i + 1) & ಮುಖವಾಡ;
        ಹಿಂತಿರುಗಿ i;
    }

    // Resize to the given number of slots, which must fit all entries
    ಶೂನ್ಯ resize(ಗಾತ್ರ_t ಎಣಿಕೆ_2) {
        UDEBUGONLY(UASSERT(ಎಣಿಕೆ_2 && !(ಎಣಿಕೆ_2 & (ಎಣಿಕೆ_2 - 1)), "Capacity not a power of 2"););
        ಸ್ಥಿರಾಂಕ std::unique_ptr<ಸ್ಥಾನ[]> old_ಕೋಷ್ಟಕ{std::move(m_ಕೋಷ್ಟಕ)};
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t old_ಸಾಮರ್ಥ್ಯ = m_ಸಾಮರ್ಥ್ಯ;
        m_ಕೋಷ್ಟಕ = std::make_unique<ಸ್ಥಾನ[]>(ಎಣಿಕೆ_2);
        m_ಸಾಮರ್ಥ್ಯ = ಎಣಿಕೆ_2;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < old_ಸಾಮರ್ಥ್ಯ; ++i) {
            ಸ್ಥಾನ& ಸ್ಥಾನ_2 = old_ಕೋಷ್ಟಕ[i];
            ಆದರೆ (!ಸ್ಥಾನ_2.isFree()) m_ಕೋಷ್ಟಕ[free_ಸ್ಥಾನ(ಸ್ಥಾನ_2.m_hash)] = std::move(ಸ್ಥಾನ_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    ಗಾತ್ರ_t probe(ಗಾತ್ರ_t hash, ಸ್ಥಿರಾಂಕ T_ವಾದಗಳು&... ವಾದಗಳು) ಸ್ಥಿರಾಂಕ {
        UDEBUGONLY(UASSERT(m_ಕೋಷ್ಟಕ, "Table must be allocated"););
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮುಖವಾಡ = m_ಸಾಮರ್ಥ್ಯ - 1;
        ಗಾತ್ರ_t i = hash & ಮುಖವಾಡ;
        ಆಗ (!m_ಕೋಷ್ಟಕ[i].isFree()) {
            ಸ್ಥಿರಾಂಕ ಸ್ಥಾನ& ಸ್ಥಾನ_2 = m_ಕೋಷ್ಟಕ[i];
            ಆದರೆ (ಸ್ಥಾನ_2.m_hash == hash && m_equal(m_ಕೀಲಿ_ದ(ಸ್ಥಾನ_2.m_ನಮೂದು), ವಾದಗಳು...)) ನಿಲ್ಲಿಸು;
            i = (i + 1) & ಮುಖವಾಡ;
        }
        ಹಿಂತಿರುಗಿ i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    ಮಾದರಿ <ಗಾತ್ರ_t... N_ಕೀಲಿ, typename T_All>
    std::pair<iterator, bool> ಸೇರಿಸು_Lazy_Impl(std::ಸೂಚ್ಯಂಕ_sequence<N_ಕೀಲಿ...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_ಕೀಲಿ, T_All>...>::ಮೌಲ್ಯ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_ಕೀಲಿ, T_All>...>::ಮೌಲ್ಯ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t hash = hash_ದ(std::get<N_ಕೀಲಿ>(all)...);
        // Allocate on the first insertion
        ಆದರೆ (VL_UNLIKELY(!m_ಸಾಮರ್ಥ್ಯ)) resize(V3_Hash_ಕೋಷ್ಟಕ_Internals::MIN_ಸಾಮರ್ಥ್ಯ);
        // Find the slot for the entry
        ಸ್ಥಾನ* slotp = m_ಕೋಷ್ಟಕ.get() + probe(hash, std::get<N_ಕೀಲಿ>(all)...);
        // If occupied, it's the equivalent, and we are done
        ಆದರೆ (!slotp->isFree()) ಹಿಂತಿರುಗಿ {iterator{slotp, m_ಕೋಷ್ಟಕ.get() + m_ಸಾಮರ್ಥ್ಯ}, false};
        // Table is growing
        ++m_ಗಾತ್ರ;
        // Increase if necessary by load factor
        ಆದರೆ (VL_UNLIKELY(m_ಗಾತ್ರ * V3_Hash_ಕೋಷ್ಟಕ_Internals::ತುಂಬು_ಗುಣಕ_DEN
                        > m_ಸಾಮರ್ಥ್ಯ * V3_Hash_ಕೋಷ್ಟಕ_Internals::ತುಂಬು_ಗುಣಕ_ಅಂಕೆ)) {
            resize(m_ಸಾಮರ್ಥ್ಯ * 2);
            slotp = m_ಕೋಷ್ಟಕ.get() + free_ಸ್ಥಾನ(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<ಗಾತ್ರ...(N_ಕೀಲಿ)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        ಸ್ಥಿರಾಂಕ ಕೀಲಿ_2& ಕೀಲಿ = m_ಕೀಲಿ_ದ(slotp->m_ನಮೂದು);
        UASSERT(hash_ದ(ಕೀಲಿ) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(ಕೀಲಿ, std::get<N_ಕೀಲಿ>(all)...),
                "Created entry does not match the key it was looked up with");
#ಆದರೆ_ಅಂತ್ಯ
        // Return newly create entry
        ಹಿಂತಿರುಗಿ {iterator{slotp, m_ಕೋಷ್ಟಕ.get() + m_ಸಾಮರ್ಥ್ಯ}, true};
    }

ಸಂರಕ್ಷಿತ:
    // CONSTRUCTORS
    V3_Hash_ಕೋಷ್ಟಕ() = ಪೂರ್ವನಿಯೋಜಿತ;
    V3_Hash_ಕೋಷ್ಟಕ(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3_Hash_ಕೋಷ್ಟಕ() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(V3_Hash_ಕೋಷ್ಟಕ);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3_Hash_ಕೋಷ್ಟಕ(V3_Hash_ಕೋಷ್ಟಕ&& that)
        : m_ಕೋಷ್ಟಕ{std::move(that.m_ಕೋಷ್ಟಕ)}
        , m_ಸಾಮರ್ಥ್ಯ{that.m_ಸಾಮರ್ಥ್ಯ}
        , m_ಗಾತ್ರ{that.m_ಗಾತ್ರ}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_ಕೀಲಿ_ದ{std::move(that.m_ಕೀಲಿ_ದ)} {
        that.m_ಸಾಮರ್ಥ್ಯ = 0;
        that.m_ಗಾತ್ರ = 0;
    }
    V3_Hash_ಕೋಷ್ಟಕ& ಕ್ರಿಯಾಕಾರಕ=(V3_Hash_ಕೋಷ್ಟಕ&& that) {
        m_ಕೋಷ್ಟಕ = std::move(that.m_ಕೋಷ್ಟಕ);  // Frees the table this held, if any
        m_ಸಾಮರ್ಥ್ಯ = that.m_ಸಾಮರ್ಥ್ಯ;
        m_ಗಾತ್ರ = that.m_ಗಾತ್ರ;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_ಕೀಲಿ_ದ = std::move(that.m_ಕೀಲಿ_ದ);
        that.m_ಸಾಮರ್ಥ್ಯ = 0;
        that.m_ಗಾತ್ರ = 0;
        ಹಿಂತಿರುಗಿ *this;
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಗಾತ್ರ_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗಾತ್ರ; }
    bool ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_ಗಾತ್ರ; }

    iterator begin() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಾನ* ಸ್ಥಿರಾಂಕ endp = m_ಕೋಷ್ಟಕ.get() + m_ಸಾಮರ್ಥ್ಯ;
        ಸ್ಥಾನ* slotp = m_ಕೋಷ್ಟಕ.get();
        ಆಗ (slotp != endp && slotp->isFree()) ++slotp;
        ಹಿಂತಿರುಗಿ iterator{slotp, endp};
    }
    iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಾನ* ಸ್ಥಿರಾಂಕ endp = m_ಕೋಷ್ಟಕ.get() + m_ಸಾಮರ್ಥ್ಯ;
        ಹಿಂತಿರುಗಿ iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    ಶೂನ್ಯ reserve(ಗಾತ್ರ_t ಎಣಿಕೆ_2) {
        ಗಾತ್ರ_t ಸಾಮರ್ಥ್ಯ = V3_Hash_ಕೋಷ್ಟಕ_Internals::MIN_ಸಾಮರ್ಥ್ಯ;
        ಆಗ (ಸಾಮರ್ಥ್ಯ * V3_Hash_ಕೋಷ್ಟಕ_Internals::ತುಂಬು_ಗುಣಕ_ಅಂಕೆ
               < ಎಣಿಕೆ_2 * V3_Hash_ಕೋಷ್ಟಕ_Internals::ತುಂಬು_ಗುಣಕ_DEN)
            ಸಾಮರ್ಥ್ಯ *= 2;
        ಆದರೆ (ಸಾಮರ್ಥ್ಯ > m_ಸಾಮರ್ಥ್ಯ) resize(ಸಾಮರ್ಥ್ಯ);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    ಶೂನ್ಯ clear() {
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < m_ಸಾಮರ್ಥ್ಯ; ++i) {
            ಆದರೆ (!m_ಕೋಷ್ಟಕ[i].isFree()) m_ಕೋಷ್ಟಕ[i].destroy();
        }
        m_ಗಾತ್ರ = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    iterator ಕಂಡುಹಿಡಿ(ಸ್ಥಿರಾಂಕ T_ವಾದಗಳು&... ವಾದಗಳು) ಸ್ಥಿರಾಂಕ {
        static_assert(ValidHash<T_ವಾದಗಳು...>::ಮೌಲ್ಯ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_ವಾದಗಳು...>::ಮೌಲ್ಯ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ಆದರೆ (!m_ಗಾತ್ರ) ಹಿಂತಿರುಗಿ ಕೊನೆ();  // Nothing to find, and this also covers there being no table
        ಸ್ಥಾನ* ಸ್ಥಿರಾಂಕ slotp = m_ಕೋಷ್ಟಕ.get() + probe(hash_ದ(ವಾದಗಳು...), ವಾದಗಳು...);
        ಹಿಂತಿರುಗಿ slotp->isFree() ? ಕೊನೆ() : iterator{slotp, m_ಕೋಷ್ಟಕ.get() + m_ಸಾಮರ್ಥ್ಯ};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> ಸೇರಿಸು(ಸ್ಥಿರಾಂಕ ನಮೂದು_2& ನಮೂದು) {
        static_assert(std::is_copy_constructible<ನಮೂದು_2>::ಮೌಲ್ಯ,
                      "'Entry' must be copy constructible to use 'insert'");
        ಹಿಂತಿರುಗಿ ಸೇರಿಸು_Lazy(m_ಕೀಲಿ_ದ(ನಮೂದು), [&ನಮೂದು]() -> ನಮೂದು_2 { ಹಿಂತಿರುಗಿ ನಮೂದು; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    std::pair<iterator, bool> ಸೇರಿಸು_Lazy(T_ವಾದಗಳು&&... ವಾದಗಳು) {
        static_assert(ಗಾತ್ರ...(T_ವಾದಗಳು) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<ಗಾತ್ರ...(T_ವಾದಗಳು) - 1, std::tuple<T_ವಾದಗಳು...>>;
        static_assert(vlstd::is_invocable_r<ನಮೂದು_2, Callable>::ಮೌಲ್ಯ,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        ಹಿಂತಿರುಗಿ ಸೇರಿಸು_Lazy_Impl(std::make_ಸೂಚ್ಯಂಕ_sequence<ಗಾತ್ರ...(T_ವಾದಗಳು) - 1>{},
                              std::forward_as_tuple(std::forward<T_ವಾದಗಳು>(ವಾದಗಳು)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    bool ಒಳಗೊಂಡಿದೆ(ಸ್ಥಿರಾಂಕ T_ವಾದಗಳು&... ವಾದಗಳು) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ(ವಾದಗಳು...) != ಕೊನೆ();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    ಮಾದರಿ <typename... T_ವಾದಗಳು>
    bool erase(ಸ್ಥಿರಾಂಕ T_ವಾದಗಳು&... ವಾದಗಳು) {
        ಸ್ಥಿರಾಂಕ iterator it = ಕಂಡುಹಿಡಿ(ವಾದಗಳು...);
        ಆದರೆ (it == ಕೊನೆ()) ಹಿಂತಿರುಗಿ false;
        erase(it);
        ಹಿಂತಿರುಗಿ true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    ಶೂನ್ಯ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != ಕೊನೆ() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮುಖವಾಡ = m_ಸಾಮರ್ಥ್ಯ - 1;
        ಗಾತ್ರ_t i = static_cast<ಗಾತ್ರ_t>(it.m_slotp - m_ಕೋಷ್ಟಕ.get());
        // Destroy the entry
        m_ಕೋಷ್ಟಕ[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_ಗಾತ್ರ;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        ಗಾತ್ರ_t j = i;
        ಆಗ (true) {
            j = (j + 1) & ಮುಖವಾಡ;
            ಸ್ಥಾನ& ಸ್ಥಾನ_2 = m_ಕೋಷ್ಟಕ[j];
            ಆದರೆ (ಸ್ಥಾನ_2.isFree()) ನಿಲ್ಲಿಸು;
            // Move back if its home position does not lie in the cyclic range (i, j]
            ಆದರೆ (((j - (ಸ್ಥಾನ_2.m_hash & ಮುಖವಾಡ)) & ಮುಖವಾಡ) >= ((j - i) & ಮುಖವಾಡ)) {
                m_ಕೋಷ್ಟಕ[i] = std::move(ಸ್ಥಾನ_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_Hash = std::hash<T_ಕೀಲಿ>,
          typename T_Equal = std::equal_ಗಮ್ಯ<T_ಕೀಲಿ>>
class V3_Hash_ಗಣ final : ಸಾರ್ವಜನಿಕ V3_Hash_ಕೋಷ್ಟಕ<T_ಕೀಲಿ, T_Hash, T_Equal,
                                           V3_Hash_ಕೋಷ್ಟಕ_Internals::V3_Hash_ಕೋಷ್ಟಕ_ಕೀಲಿ_Is_ನಮೂದು<T_ಕೀಲಿ>> {
    using Super
        = V3_Hash_ಕೋಷ್ಟಕ<T_ಕೀಲಿ, T_Hash, T_Equal, V3_Hash_ಕೋಷ್ಟಕ_Internals::V3_Hash_ಕೋಷ್ಟಕ_ಕೀಲಿ_Is_ನಮೂದು<T_ಕೀಲಿ>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_ಕೀಲಿ>::ಮೌಲ್ಯ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ಕೀಲಿ>::ಮೌಲ್ಯ, "'T_Key' must be destructible");

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3_Hash_ಗಣ() = ಪೂರ್ವನಿಯೋಜಿತ;
    V3_Hash_ಗಣ(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_Val, typename T_Hash = std::hash<T_ಕೀಲಿ>,
          typename T_Equal = std::equal_ಗಮ್ಯ<T_ಕೀಲಿ>>
class V3HashMap final
    : ಸಾರ್ವಜನಿಕ V3_Hash_ಕೋಷ್ಟಕ<std::pair<T_ಕೀಲಿ, T_Val>, T_Hash, T_Equal,
                         V3_Hash_ಕೋಷ್ಟಕ_Internals::V3_Hash_ಕೋಷ್ಟಕ_ಕೀಲಿ_Is_ಮೊದಲ<T_ಕೀಲಿ, T_Val>> {
    using Super = V3_Hash_ಕೋಷ್ಟಕ<std::pair<T_ಕೀಲಿ, T_Val>, T_Hash, T_Equal,
                              V3_Hash_ಕೋಷ್ಟಕ_Internals::V3_Hash_ಕೋಷ್ಟಕ_ಕೀಲಿ_Is_ಮೊದಲ<T_ಕೀಲಿ, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_ಕೀಲಿ>::ಮೌಲ್ಯ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ಕೀಲಿ>::ಮೌಲ್ಯ, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::ಮೌಲ್ಯ, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::ಮೌಲ್ಯ, "'T_Val' must be destructible");

ಸಾರ್ವಜನಿಕ:
    // TYPES
    using ಮೌಲ್ಯ_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = ಪೂರ್ವನಿಯೋಜಿತ;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
