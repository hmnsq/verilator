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

#ifndef VERILATOR_V3HASHTABLE_H_
#define VERILATOR_V3HASHTABLE_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3StdFuture.h"

#અન્તર્ભાવ <functional>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <new>
#અન્તર્ભાવ <tuple>
#અન્તર્ભાવ <type_traits>
#અન્તર્ભાવ <utility>

namespace V3HashTableInternals {

constexpr size_t MIN_CAPACITY = 16;  // Smallest table allocated
constexpr size_t LOAD_FACTOR_અંક = 3;  // Numerator of the maximum load factor
constexpr size_t LOAD_FACTOR_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
template <typename T_ચાવી>
સંરચના V3_Hash_Table_ચાવી_Is_Entry final {
    using ચાવી_2 = T_ચાવી;  // What it yields, so the table need not deduce it
    અચલ T_ચાવી& operator()(અચલ T_ચાવી& entry) અચલ { પાછા entry; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
template <typename T_ચાવી, typename T_Val>
સંરચના V3_Hash_Table_ચાવી_Is_First final {
    using ચાવી_2 = T_ચાવી;  // What it yields, so the table need not deduce it
    અચલ T_ચાવી& operator()(અચલ std::pair<T_ચાવી, T_Val>& entry) અચલ { પાછા entry.first; }
    સ્થિર T_Val& કિંમત_Of(std::pair<T_ચાવી, T_Val>& entry) { પાછા entry.second; }
};

શૂન્ય selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
template <typename T_Entry, typename T_Hash, typename T_Equal, typename T_ચાવી_Of>
class V3HashTable VL_NOT_FINAL {
જાહેર:
    // TYPES
    using Entry = T_Entry;  // What is stored
    using ચાવી_2 = typename T_ચાવી_Of::ચાવી_2;  // What entries are looked up by

ખાનગી:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    template <typename... T_Args>
    using ValidHash = vlstd::is_invocable_r<size_t, અચલ T_Hash&, અચલ T_Args&...>;

    // Holds if the equality accepts a key and such a lookup key
    template <typename... T_Args>
    using ValidEqual = vlstd::is_invocable_r<bool, અચલ T_Equal&, અચલ ચાવી_2&, અચલ T_Args&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<ચાવી_2>::કિંમત, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<ચાવી_2>::કિંમત, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    સંરચના Slot final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        સંઘ {
            Entry m_entry;  // Stored entry; alive only while the slot is occupied
        };
        size_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        Slot() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~Slot() {
            જો (!isFree()) destroy();
        }
        Slot(અચલ Slot&) = delete;
        Slot(Slot&&) = delete;
        અચલ Slot& operator=(અચલ Slot&) = delete;
        Slot& operator=(Slot&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_entry) Entry{std::move(that.m_entry)};
            m_hash = that.m_hash;
            that.destroy();
            પાછા *this;
        }

        bool isFree() અચલ { પાછા !m_hash; }

        // Construct the entry of this free slot from the given entry
        શૂન્ય construct(size_t hash, Entry&& entry) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_entry) Entry{std::move(entry)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        શૂન્ય destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_entry.~Entry();
            m_hash = 0;
        }
    };

જાહેર:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        friend class V3HashTable;

        Slot* m_slotp = nullptr;  // The slot iterated, or the end of the table
        Slot* m_endp = nullptr;  // One past the last slot

        iterator(Slot* slotp, Slot* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    જાહેર:
        iterator() = મૂળભૂત;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        અચલ Entry& operator*() અચલ { પાછા m_slotp->m_entry; }
        અચલ Entry* operator->() અચલ { પાછા &m_slotp->m_entry; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        auto& કિંમત() અચલ { પાછા T_ચાવી_Of::કિંમત_Of(m_slotp->m_entry); }
        // Pre-increment, skipping the free slots
        iterator& operator++() {
            જ્યારે (++m_slotp != m_endp && m_slotp->isFree()) {}
            પાછા *this;
        }
        bool operator==(અચલ iterator& that) અચલ { પાછા m_slotp == that.m_slotp; }
        bool operator!=(અચલ iterator& that) અચલ { પાછા m_slotp != that.m_slotp; }
    };

ખાનગી:
    // STATE
    std::unique_ptr<Slot[]> m_table;  // The table, null when unallocated
    size_t m_capacity = 0;  // Number of slots in the table, a power of two, or zero
    size_t m_size = 0;  // Number of occupied slots
    VL_NO_UNIQUE_ADDRESS_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_ADDRESS_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_ADDRESS_CXX20 T_ચાવી_Of m_ચાવી_Of;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    template <typename... T_Args>
    size_t hashOf(અચલ T_Args&... args) અચલ {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr size_t USED_BIT = size_t{1} << (કદ(size_t) * 8 - 1);
        પાછા static_cast<size_t>(m_hash(args...)) | USED_BIT;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    size_t freeSlot(size_t hash) અચલ {
        અચલ size_t mask = m_capacity - 1;
        size_t i = hash & mask;
        જ્યારે (!m_table[i].isFree()) i = (i + 1) & mask;
        પાછા i;
    }

    // Resize to the given number of slots, which must fit all entries
    શૂન્ય resize(size_t ગણતરી) {
        UDEBUGONLY(UASSERT(ગણતરી && !(ગણતરી & (ગણતરી - 1)), "Capacity not a power of 2"););
        અચલ std::unique_ptr<Slot[]> oldTable{std::move(m_table)};
        અચલ size_t oldCapacity = m_capacity;
        m_table = std::make_unique<Slot[]>(ગણતરી);
        m_capacity = ગણતરી;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        માટે (size_t i = 0; i < oldCapacity; ++i) {
            Slot& slot = oldTable[i];
            જો (!slot.isFree()) m_table[freeSlot(slot.m_hash)] = std::move(slot);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    template <typename... T_Args>
    size_t probe(size_t hash, અચલ T_Args&... args) અચલ {
        UDEBUGONLY(UASSERT(m_table, "Table must be allocated"););
        અચલ size_t mask = m_capacity - 1;
        size_t i = hash & mask;
        જ્યારે (!m_table[i].isFree()) {
            અચલ Slot& slot = m_table[i];
            જો (slot.m_hash == hash && m_equal(m_ચાવી_Of(slot.m_entry), args...)) તોડો;
            i = (i + 1) & mask;
        }
        પાછા i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    template <size_t... N_ચાવી, typename T_All>
    std::pair<iterator, bool> ઉમેરો_Lazy_Impl(std::index_sequence<N_ચાવી...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_ચાવી, T_All>...>::કિંમત,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_ચાવી, T_All>...>::કિંમત,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        અચલ size_t hash = hashOf(std::get<N_ચાવી>(all)...);
        // Allocate on the first insertion
        જો (VL_UNLIKELY(!m_capacity)) resize(V3HashTableInternals::MIN_CAPACITY);
        // Find the slot for the entry
        Slot* slotp = m_table.get() + probe(hash, std::get<N_ચાવી>(all)...);
        // If occupied, it's the equivalent, and we are done
        જો (!slotp->isFree()) પાછા {iterator{slotp, m_table.get() + m_capacity}, false};
        // Table is growing
        ++m_size;
        // Increase if necessary by load factor
        જો (VL_UNLIKELY(m_size * V3HashTableInternals::LOAD_FACTOR_DEN
                        > m_capacity * V3HashTableInternals::LOAD_FACTOR_અંક)) {
            resize(m_capacity * 2);
            slotp = m_table.get() + freeSlot(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<કદ...(N_ચાવી)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#ifdef VL_DEBUG
        અચલ ચાવી_2& ચાવી = m_ચાવી_Of(slotp->m_entry);
        UASSERT(hashOf(ચાવી) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(ચાવી, std::get<N_ચાવી>(all)...),
                "Created entry does not match the key it was looked up with");
#endif
        // Return newly create entry
        પાછા {iterator{slotp, m_table.get() + m_capacity}, true};
    }

સુરક્ષિત:
    // CONSTRUCTORS
    V3HashTable() = મૂળભૂત;
    V3HashTable(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3HashTable() = મૂળભૂત;
    VL_UNCOPYABLE(V3HashTable);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3HashTable(V3HashTable&& that)
        : m_table{std::move(that.m_table)}
        , m_capacity{that.m_capacity}
        , m_size{that.m_size}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_ચાવી_Of{std::move(that.m_ચાવી_Of)} {
        that.m_capacity = 0;
        that.m_size = 0;
    }
    V3HashTable& operator=(V3HashTable&& that) {
        m_table = std::move(that.m_table);  // Frees the table this held, if any
        m_capacity = that.m_capacity;
        m_size = that.m_size;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_ચાવી_Of = std::move(that.m_ચાવી_Of);
        that.m_capacity = 0;
        that.m_size = 0;
        પાછા *this;
    }

જાહેર:
    // METHODS
    size_t size() અચલ { પાછા m_size; }
    bool empty() અચલ { પાછા !m_size; }

    iterator begin() અચલ {
        Slot* અચલ endp = m_table.get() + m_capacity;
        Slot* slotp = m_table.get();
        જ્યારે (slotp != endp && slotp->isFree()) ++slotp;
        પાછા iterator{slotp, endp};
    }
    iterator end() અચલ {
        Slot* અચલ endp = m_table.get() + m_capacity;
        પાછા iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    શૂન્ય reserve(size_t ગણતરી) {
        size_t capacity = V3HashTableInternals::MIN_CAPACITY;
        જ્યારે (capacity * V3HashTableInternals::LOAD_FACTOR_અંક
               < ગણતરી * V3HashTableInternals::LOAD_FACTOR_DEN)
            capacity *= 2;
        જો (capacity > m_capacity) resize(capacity);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    શૂન્ય clear() {
        માટે (size_t i = 0; i < m_capacity; ++i) {
            જો (!m_table[i].isFree()) m_table[i].destroy();
        }
        m_size = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    template <typename... T_Args>
    iterator find(અચલ T_Args&... args) અચલ {
        static_assert(ValidHash<T_Args...>::કિંમત,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_Args...>::કિંમત,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        જો (!m_size) પાછા end();  // Nothing to find, and this also covers there being no table
        Slot* અચલ slotp = m_table.get() + probe(hashOf(args...), args...);
        પાછા slotp->isFree() ? end() : iterator{slotp, m_table.get() + m_capacity};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> ઉમેરો(અચલ Entry& entry) {
        static_assert(std::is_copy_constructible<Entry>::કિંમત,
                      "'Entry' must be copy constructible to use 'insert'");
        પાછા ઉમેરો_Lazy(m_ચાવી_Of(entry), [&entry]() -> Entry { પાછા entry; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    template <typename... T_Args>
    std::pair<iterator, bool> ઉમેરો_Lazy(T_Args&&... args) {
        static_assert(કદ...(T_Args) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<કદ...(T_Args) - 1, std::tuple<T_Args...>>;
        static_assert(vlstd::is_invocable_r<Entry, Callable>::કિંમત,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        પાછા ઉમેરો_Lazy_Impl(std::make_index_sequence<કદ...(T_Args) - 1>{},
                              std::forward_as_tuple(std::forward<T_Args>(args)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    template <typename... T_Args>
    bool contains(અચલ T_Args&... args) અચલ {
        પાછા find(args...) != end();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    template <typename... T_Args>
    bool erase(અચલ T_Args&... args) {
        અચલ iterator it = find(args...);
        જો (it == end()) પાછા false;
        erase(it);
        પાછા true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    શૂન્ય erase(iterator it) {
        UDEBUGONLY(UASSERT(it != end() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        અચલ size_t mask = m_capacity - 1;
        size_t i = static_cast<size_t>(it.m_slotp - m_table.get());
        // Destroy the entry
        m_table[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_size;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        size_t j = i;
        જ્યારે (true) {
            j = (j + 1) & mask;
            Slot& slot = m_table[j];
            જો (slot.isFree()) તોડો;
            // Move back if its home position does not lie in the cyclic range (i, j]
            જો (((j - (slot.m_hash & mask)) & mask) >= ((j - i) & mask)) {
                m_table[i] = std::move(slot);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

template <typename T_ચાવી, typename T_Hash = std::hash<T_ચાવી>,
          typename T_Equal = std::equal_to<T_ચાવી>>
class V3HashSet final : જાહેર V3HashTable<T_ચાવી, T_Hash, T_Equal,
                                           V3HashTableInternals::V3_Hash_Table_ચાવી_Is_Entry<T_ચાવી>> {
    using Super
        = V3HashTable<T_ચાવી, T_Hash, T_Equal, V3HashTableInternals::V3_Hash_Table_ચાવી_Is_Entry<T_ચાવી>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_ચાવી>::કિંમત, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ચાવી>::કિંમત, "'T_Key' must be destructible");

જાહેર:
    // CONSTRUCTORS
    V3HashSet() = મૂળભૂત;
    V3HashSet(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

template <typename T_ચાવી, typename T_Val, typename T_Hash = std::hash<T_ચાવી>,
          typename T_Equal = std::equal_to<T_ચાવી>>
class V3HashMap final
    : જાહેર V3HashTable<std::pair<T_ચાવી, T_Val>, T_Hash, T_Equal,
                         V3HashTableInternals::V3_Hash_Table_ચાવી_Is_First<T_ચાવી, T_Val>> {
    using Super = V3HashTable<std::pair<T_ચાવી, T_Val>, T_Hash, T_Equal,
                              V3HashTableInternals::V3_Hash_Table_ચાવી_Is_First<T_ચાવી, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_ચાવી>::કિંમત, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ચાવી>::કિંમત, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::કિંમત, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::કિંમત, "'T_Val' must be destructible");

જાહેર:
    // TYPES
    using કિંમત_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = મૂળભૂત;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#endif  // Guard
