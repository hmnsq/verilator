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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3StdFuture.h"

#ضمن <functional>
#ضمن <memory>
#ضمن <new>
#ضمن <tuple>
#ضمن <type_traits>
#ضمن <utility>

namespace V3HashTableInternals {

constexpr size_t MIN_CAPACITY = 16;  // Smallest table allocated
constexpr size_t LOAD_FACTOR_لمب = 3;  // Numerator of the maximum load factor
constexpr size_t LOAD_FACTOR_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
template <typename T_مبودي>
بنية V3_Hash_Table_مبودي_Is_Entry final {
    using مبودي_2 = T_مبودي;  // What it yields, so the table need not deduce it
    ثابت T_مبودي& operator()(ثابت T_مبودي& entry) ثابت { أرجع entry; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
template <typename T_مبودي, typename T_Val>
بنية V3_Hash_Table_مبودي_Is_First final {
    using مبودي_2 = T_مبودي;  // What it yields, so the table need not deduce it
    ثابت T_مبودي& operator()(ثابت std::pair<T_مبودي, T_Val>& entry) ثابت { أرجع entry.first; }
    ساكن T_Val& درج_Of(std::pair<T_مبودي, T_Val>& entry) { أرجع entry.second; }
};

فراغ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
template <typename T_Entry, typename T_Hash, typename T_Equal, typename T_مبودي_Of>
class V3HashTable VL_NOT_FINAL {
عام:
    // TYPES
    using Entry = T_Entry;  // What is stored
    using مبودي_2 = typename T_مبودي_Of::مبودي_2;  // What entries are looked up by

خاص:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    template <typename... T_حجوجي>
    using ValidHash = vlstd::is_invocable_r<size_t, ثابت T_Hash&, ثابت T_حجوجي&...>;

    // Holds if the equality accepts a key and such a lookup key
    template <typename... T_حجوجي>
    using ValidEqual = vlstd::is_invocable_r<bool, ثابت T_Equal&, ثابت مبودي_2&, ثابت T_حجوجي&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<مبودي_2>::درج, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<مبودي_2>::درج, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    بنية Slot final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        اتحاد {
            Entry m_entry;  // Stored entry; alive only while the slot is occupied
        };
        size_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        Slot() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~Slot() {
            إذا (!isFree()) destroy();
        }
        Slot(ثابت Slot&) = delete;
        Slot(Slot&&) = delete;
        ثابت Slot& operator=(ثابت Slot&) = delete;
        Slot& operator=(Slot&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_entry) Entry{std::move(that.m_entry)};
            m_hash = that.m_hash;
            that.destroy();
            أرجع *this;
        }

        bool isFree() ثابت { أرجع !m_hash; }

        // Construct the entry of this free slot from the given entry
        فراغ construct(size_t hash, Entry&& entry) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_entry) Entry{std::move(entry)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        فراغ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_entry.~Entry();
            m_hash = 0;
        }
    };

عام:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        friend class V3HashTable;

        Slot* m_slotp = nullptr;  // The slot iterated, or the end of the table
        Slot* m_endp = nullptr;  // One past the last slot

        iterator(Slot* slotp, Slot* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    عام:
        iterator() = افتراضي;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ثابت Entry& operator*() ثابت { أرجع m_slotp->m_entry; }
        ثابت Entry* operator->() ثابت { أرجع &m_slotp->m_entry; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        auto& درج() ثابت { أرجع T_مبودي_Of::درج_Of(m_slotp->m_entry); }
        // Pre-increment, skipping the free slots
        iterator& operator++() {
            طالما (++m_slotp != m_endp && m_slotp->isFree()) {}
            أرجع *this;
        }
        bool operator==(ثابت iterator& that) ثابت { أرجع m_slotp == that.m_slotp; }
        bool operator!=(ثابت iterator& that) ثابت { أرجع m_slotp != that.m_slotp; }
    };

خاص:
    // STATE
    std::unique_ptr<Slot[]> m_table;  // The table, null when unallocated
    size_t m_capacity = 0;  // Number of slots in the table, a power of two, or zero
    size_t m_size = 0;  // Number of occupied slots
    VL_NO_UNIQUE_ADDRESS_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_ADDRESS_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_ADDRESS_CXX20 T_مبودي_Of m_مبودي_Of;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    template <typename... T_حجوجي>
    size_t hashOf(ثابت T_حجوجي&... حجوجي) ثابت {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr size_t USED_BIT = size_t{1} << (حجم(size_t) * 8 - 1);
        أرجع static_cast<size_t>(m_hash(حجوجي...)) | USED_BIT;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    size_t freeSlot(size_t hash) ثابت {
        ثابت size_t mask = m_capacity - 1;
        size_t i = hash & mask;
        طالما (!m_table[i].isFree()) i = (i + 1) & mask;
        أرجع i;
    }

    // Resize to the given number of slots, which must fit all entries
    فراغ resize(size_t كدي) {
        UDEBUGONLY(UASSERT(كدي && !(كدي & (كدي - 1)), "Capacity not a power of 2"););
        ثابت std::unique_ptr<Slot[]> oldTable{std::move(m_table)};
        ثابت size_t oldCapacity = m_capacity;
        m_table = std::make_unique<Slot[]>(كدي);
        m_capacity = كدي;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        لكل (size_t i = 0; i < oldCapacity; ++i) {
            Slot& slot = oldTable[i];
            إذا (!slot.isFree()) m_table[freeSlot(slot.m_hash)] = std::move(slot);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    template <typename... T_حجوجي>
    size_t probe(size_t hash, ثابت T_حجوجي&... حجوجي) ثابت {
        UDEBUGONLY(UASSERT(m_table, "Table must be allocated"););
        ثابت size_t mask = m_capacity - 1;
        size_t i = hash & mask;
        طالما (!m_table[i].isFree()) {
            ثابت Slot& slot = m_table[i];
            إذا (slot.m_hash == hash && m_equal(m_مبودي_Of(slot.m_entry), حجوجي...)) اكسر;
            i = (i + 1) & mask;
        }
        أرجع i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    template <size_t... N_مبودي, typename T_All>
    std::pair<iterator, bool> سك_Lazy_Impl(std::index_sequence<N_مبودي...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_مبودي, T_All>...>::درج,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_مبودي, T_All>...>::درج,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ثابت size_t hash = hashOf(std::get<N_مبودي>(all)...);
        // Allocate on the first insertion
        إذا (VL_UNLIKELY(!m_capacity)) resize(V3HashTableInternals::MIN_CAPACITY);
        // Find the slot for the entry
        Slot* slotp = m_table.get() + probe(hash, std::get<N_مبودي>(all)...);
        // If occupied, it's the equivalent, and we are done
        إذا (!slotp->isFree()) أرجع {iterator{slotp, m_table.get() + m_capacity}, false};
        // Table is growing
        ++m_size;
        // Increase if necessary by load factor
        إذا (VL_UNLIKELY(m_size * V3HashTableInternals::LOAD_FACTOR_DEN
                        > m_capacity * V3HashTableInternals::LOAD_FACTOR_لمب)) {
            resize(m_capacity * 2);
            slotp = m_table.get() + freeSlot(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<حجم...(N_مبودي)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#ifdef VL_DEBUG
        ثابت مبودي_2& مبودي = m_مبودي_Of(slotp->m_entry);
        UASSERT(hashOf(مبودي) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(مبودي, std::get<N_مبودي>(all)...),
                "Created entry does not match the key it was looked up with");
#endif
        // Return newly create entry
        أرجع {iterator{slotp, m_table.get() + m_capacity}, true};
    }

محمي:
    // CONSTRUCTORS
    V3HashTable() = افتراضي;
    V3HashTable(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3HashTable() = افتراضي;
    VL_UNCOPYABLE(V3HashTable);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3HashTable(V3HashTable&& that)
        : m_table{std::move(that.m_table)}
        , m_capacity{that.m_capacity}
        , m_size{that.m_size}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_مبودي_Of{std::move(that.m_مبودي_Of)} {
        that.m_capacity = 0;
        that.m_size = 0;
    }
    V3HashTable& operator=(V3HashTable&& that) {
        m_table = std::move(that.m_table);  // Frees the table this held, if any
        m_capacity = that.m_capacity;
        m_size = that.m_size;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_مبودي_Of = std::move(that.m_مبودي_Of);
        that.m_capacity = 0;
        that.m_size = 0;
        أرجع *this;
    }

عام:
    // METHODS
    size_t size() ثابت { أرجع m_size; }
    bool empty() ثابت { أرجع !m_size; }

    iterator begin() ثابت {
        Slot* ثابت endp = m_table.get() + m_capacity;
        Slot* slotp = m_table.get();
        طالما (slotp != endp && slotp->isFree()) ++slotp;
        أرجع iterator{slotp, endp};
    }
    iterator end() ثابت {
        Slot* ثابت endp = m_table.get() + m_capacity;
        أرجع iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    فراغ reserve(size_t كدي) {
        size_t capacity = V3HashTableInternals::MIN_CAPACITY;
        طالما (capacity * V3HashTableInternals::LOAD_FACTOR_لمب
               < كدي * V3HashTableInternals::LOAD_FACTOR_DEN)
            capacity *= 2;
        إذا (capacity > m_capacity) resize(capacity);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    فراغ clear() {
        لكل (size_t i = 0; i < m_capacity; ++i) {
            إذا (!m_table[i].isFree()) m_table[i].destroy();
        }
        m_size = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    template <typename... T_حجوجي>
    iterator find(ثابت T_حجوجي&... حجوجي) ثابت {
        static_assert(ValidHash<T_حجوجي...>::درج,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_حجوجي...>::درج,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        إذا (!m_size) أرجع end();  // Nothing to find, and this also covers there being no table
        Slot* ثابت slotp = m_table.get() + probe(hashOf(حجوجي...), حجوجي...);
        أرجع slotp->isFree() ? end() : iterator{slotp, m_table.get() + m_capacity};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> سك(ثابت Entry& entry) {
        static_assert(std::is_copy_constructible<Entry>::درج,
                      "'Entry' must be copy constructible to use 'insert'");
        أرجع سك_Lazy(m_مبودي_Of(entry), [&entry]() -> Entry { أرجع entry; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    template <typename... T_حجوجي>
    std::pair<iterator, bool> سك_Lazy(T_حجوجي&&... حجوجي) {
        static_assert(حجم...(T_حجوجي) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<حجم...(T_حجوجي) - 1, std::tuple<T_حجوجي...>>;
        static_assert(vlstd::is_invocable_r<Entry, Callable>::درج,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        أرجع سك_Lazy_Impl(std::make_index_sequence<حجم...(T_حجوجي) - 1>{},
                              std::forward_as_tuple(std::forward<T_حجوجي>(حجوجي)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    template <typename... T_حجوجي>
    bool contains(ثابت T_حجوجي&... حجوجي) ثابت {
        أرجع find(حجوجي...) != end();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    template <typename... T_حجوجي>
    bool erase(ثابت T_حجوجي&... حجوجي) {
        ثابت iterator it = find(حجوجي...);
        إذا (it == end()) أرجع false;
        erase(it);
        أرجع true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    فراغ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != end() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ثابت size_t mask = m_capacity - 1;
        size_t i = static_cast<size_t>(it.m_slotp - m_table.get());
        // Destroy the entry
        m_table[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_size;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        size_t j = i;
        طالما (true) {
            j = (j + 1) & mask;
            Slot& slot = m_table[j];
            إذا (slot.isFree()) اكسر;
            // Move back if its home position does not lie in the cyclic range (i, j]
            إذا (((j - (slot.m_hash & mask)) & mask) >= ((j - i) & mask)) {
                m_table[i] = std::move(slot);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

template <typename T_مبودي, typename T_Hash = std::hash<T_مبودي>,
          typename T_Equal = std::equal_to<T_مبودي>>
class V3HashSet final : عام V3HashTable<T_مبودي, T_Hash, T_Equal,
                                           V3HashTableInternals::V3_Hash_Table_مبودي_Is_Entry<T_مبودي>> {
    using Super
        = V3HashTable<T_مبودي, T_Hash, T_Equal, V3HashTableInternals::V3_Hash_Table_مبودي_Is_Entry<T_مبودي>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_مبودي>::درج, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_مبودي>::درج, "'T_Key' must be destructible");

عام:
    // CONSTRUCTORS
    V3HashSet() = افتراضي;
    V3HashSet(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

template <typename T_مبودي, typename T_Val, typename T_Hash = std::hash<T_مبودي>,
          typename T_Equal = std::equal_to<T_مبودي>>
class V3HashMap final
    : عام V3HashTable<std::pair<T_مبودي, T_Val>, T_Hash, T_Equal,
                         V3HashTableInternals::V3_Hash_Table_مبودي_Is_First<T_مبودي, T_Val>> {
    using Super = V3HashTable<std::pair<T_مبودي, T_Val>, T_Hash, T_Equal,
                              V3HashTableInternals::V3_Hash_Table_مبودي_Is_First<T_مبودي, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_مبودي>::درج, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_مبودي>::درج, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::درج, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::درج, "'T_Val' must be destructible");

عام:
    // TYPES
    using درج_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = افتراضي;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#endif  // Guard
