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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3StdFuture.h"

#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <new>
#ଅନ୍ତର୍ଭାଵ <tuple>
#ଅନ୍ତର୍ଭାଵ <type_traits>
#ଅନ୍ତର୍ଭାଵ <utility>

namespace V3HashTableInternals {

constexpr size_t MIN_CAPACITY = 16;  // Smallest table allocated
constexpr size_t LOAD_FACTOR_ଅଙ୍କ = 3;  // Numerator of the maximum load factor
constexpr size_t LOAD_FACTOR_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
template <typename T_ଚାବି>
ସଂରଚନା V3_Hash_Table_ଚାବି_Is_Entry final {
    using ଚାବି_2 = T_ଚାବି;  // What it yields, so the table need not deduce it
    ଅଚଳ T_ଚାବି& operator()(ଅଚଳ T_ଚାବି& entry) ଅଚଳ { ଫେରନ୍ତୁ entry; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
template <typename T_ଚାବି, typename T_Val>
ସଂରଚନା V3_Hash_Table_ଚାବି_Is_First final {
    using ଚାବି_2 = T_ଚାବି;  // What it yields, so the table need not deduce it
    ଅଚଳ T_ଚାବି& operator()(ଅଚଳ std::pair<T_ଚାବି, T_Val>& entry) ଅଚଳ { ଫେରନ୍ତୁ entry.first; }
    ସ୍ଥିର T_Val& ମୂଲ୍ୟ_Of(std::pair<T_ଚାବି, T_Val>& entry) { ଫେରନ୍ତୁ entry.second; }
};

ଶୂନ୍ୟ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
template <typename T_Entry, typename T_Hash, typename T_Equal, typename T_ଚାବି_Of>
class V3HashTable VL_NOT_FINAL {
ସର୍ବସାଧାରଣ:
    // TYPES
    using Entry = T_Entry;  // What is stored
    using ଚାବି_2 = typename T_ଚାବି_Of::ଚାବି_2;  // What entries are looked up by

ବ୍ୟକ୍ତିଗତ:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    template <typename... T_Args>
    using ValidHash = vlstd::is_invocable_r<size_t, ଅଚଳ T_Hash&, ଅଚଳ T_Args&...>;

    // Holds if the equality accepts a key and such a lookup key
    template <typename... T_Args>
    using ValidEqual = vlstd::is_invocable_r<bool, ଅଚଳ T_Equal&, ଅଚଳ ଚାବି_2&, ଅଚଳ T_Args&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<ଚାବି_2>::ମୂଲ୍ୟ, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<ଚାବି_2>::ମୂଲ୍ୟ, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    ସଂରଚନା Slot final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        ସଂଘ {
            Entry m_entry;  // Stored entry; alive only while the slot is occupied
        };
        size_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        Slot() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~Slot() {
            ଯଦି (!isFree()) destroy();
        }
        Slot(ଅଚଳ Slot&) = delete;
        Slot(Slot&&) = delete;
        ଅଚଳ Slot& operator=(ଅଚଳ Slot&) = delete;
        Slot& operator=(Slot&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_entry) Entry{std::move(that.m_entry)};
            m_hash = that.m_hash;
            that.destroy();
            ଫେରନ୍ତୁ *this;
        }

        bool isFree() ଅଚଳ { ଫେରନ୍ତୁ !m_hash; }

        // Construct the entry of this free slot from the given entry
        ଶୂନ୍ୟ construct(size_t hash, Entry&& entry) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_entry) Entry{std::move(entry)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        ଶୂନ୍ୟ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_entry.~Entry();
            m_hash = 0;
        }
    };

ସର୍ବସାଧାରଣ:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        friend class V3HashTable;

        Slot* m_slotp = nullptr;  // The slot iterated, or the end of the table
        Slot* m_endp = nullptr;  // One past the last slot

        iterator(Slot* slotp, Slot* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    ସର୍ବସାଧାରଣ:
        iterator() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ଅଚଳ Entry& operator*() ଅଚଳ { ଫେରନ୍ତୁ m_slotp->m_entry; }
        ଅଚଳ Entry* operator->() ଅଚଳ { ଫେରନ୍ତୁ &m_slotp->m_entry; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        auto& ମୂଲ୍ୟ() ଅଚଳ { ଫେରନ୍ତୁ T_ଚାବି_Of::ମୂଲ୍ୟ_Of(m_slotp->m_entry); }
        // Pre-increment, skipping the free slots
        iterator& operator++() {
            ଯେତେବେଳେ (++m_slotp != m_endp && m_slotp->isFree()) {}
            ଫେରନ୍ତୁ *this;
        }
        bool operator==(ଅଚଳ iterator& that) ଅଚଳ { ଫେରନ୍ତୁ m_slotp == that.m_slotp; }
        bool operator!=(ଅଚଳ iterator& that) ଅଚଳ { ଫେରନ୍ତୁ m_slotp != that.m_slotp; }
    };

ବ୍ୟକ୍ତିଗତ:
    // STATE
    std::unique_ptr<Slot[]> m_table;  // The table, null when unallocated
    size_t m_capacity = 0;  // Number of slots in the table, a power of two, or zero
    size_t m_size = 0;  // Number of occupied slots
    VL_NO_UNIQUE_ADDRESS_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_ADDRESS_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_ADDRESS_CXX20 T_ଚାବି_Of m_ଚାବି_Of;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    template <typename... T_Args>
    size_t hashOf(ଅଚଳ T_Args&... args) ଅଚଳ {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr size_t USED_BIT = size_t{1} << (ଆକାର(size_t) * 8 - 1);
        ଫେରନ୍ତୁ static_cast<size_t>(m_hash(args...)) | USED_BIT;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    size_t freeSlot(size_t hash) ଅଚଳ {
        ଅଚଳ size_t mask = m_capacity - 1;
        size_t i = hash & mask;
        ଯେତେବେଳେ (!m_table[i].isFree()) i = (i + 1) & mask;
        ଫେରନ୍ତୁ i;
    }

    // Resize to the given number of slots, which must fit all entries
    ଶୂନ୍ୟ resize(size_t ଗଣନା_2) {
        UDEBUGONLY(UASSERT(ଗଣନା_2 && !(ଗଣନା_2 & (ଗଣନା_2 - 1)), "Capacity not a power of 2"););
        ଅଚଳ std::unique_ptr<Slot[]> oldTable{std::move(m_table)};
        ଅଚଳ size_t oldCapacity = m_capacity;
        m_table = std::make_unique<Slot[]>(ଗଣନା_2);
        m_capacity = ଗଣନା_2;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        ପାଇଁ (size_t i = 0; i < oldCapacity; ++i) {
            Slot& slot = oldTable[i];
            ଯଦି (!slot.isFree()) m_table[freeSlot(slot.m_hash)] = std::move(slot);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    template <typename... T_Args>
    size_t probe(size_t hash, ଅଚଳ T_Args&... args) ଅଚଳ {
        UDEBUGONLY(UASSERT(m_table, "Table must be allocated"););
        ଅଚଳ size_t mask = m_capacity - 1;
        size_t i = hash & mask;
        ଯେତେବେଳେ (!m_table[i].isFree()) {
            ଅଚଳ Slot& slot = m_table[i];
            ଯଦି (slot.m_hash == hash && m_equal(m_ଚାବି_Of(slot.m_entry), args...)) ଭାଙ୍ଗ;
            i = (i + 1) & mask;
        }
        ଫେରନ୍ତୁ i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    template <size_t... N_ଚାବି, typename T_All>
    std::pair<iterator, bool> ଭର୍ତ୍ତି_Lazy_Impl(std::index_sequence<N_ଚାବି...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_ଚାବି, T_All>...>::ମୂଲ୍ୟ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_ଚାବି, T_All>...>::ମୂଲ୍ୟ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ଅଚଳ size_t hash = hashOf(std::get<N_ଚାବି>(all)...);
        // Allocate on the first insertion
        ଯଦି (VL_UNLIKELY(!m_capacity)) resize(V3HashTableInternals::MIN_CAPACITY);
        // Find the slot for the entry
        Slot* slotp = m_table.get() + probe(hash, std::get<N_ଚାବି>(all)...);
        // If occupied, it's the equivalent, and we are done
        ଯଦି (!slotp->isFree()) ଫେରନ୍ତୁ {iterator{slotp, m_table.get() + m_capacity}, false};
        // Table is growing
        ++m_size;
        // Increase if necessary by load factor
        ଯଦି (VL_UNLIKELY(m_size * V3HashTableInternals::LOAD_FACTOR_DEN
                        > m_capacity * V3HashTableInternals::LOAD_FACTOR_ଅଙ୍କ)) {
            resize(m_capacity * 2);
            slotp = m_table.get() + freeSlot(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<ଆକାର...(N_ଚାବି)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#ifdef VL_DEBUG
        ଅଚଳ ଚାବି_2& ଚାବି = m_ଚାବି_Of(slotp->m_entry);
        UASSERT(hashOf(ଚାବି) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(ଚାବି, std::get<N_ଚାବି>(all)...),
                "Created entry does not match the key it was looked up with");
#endif
        // Return newly create entry
        ଫେରନ୍ତୁ {iterator{slotp, m_table.get() + m_capacity}, true};
    }

ସୁରକ୍ଷିତ:
    // CONSTRUCTORS
    V3HashTable() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    V3HashTable(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3HashTable() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(V3HashTable);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3HashTable(V3HashTable&& that)
        : m_table{std::move(that.m_table)}
        , m_capacity{that.m_capacity}
        , m_size{that.m_size}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_ଚାବି_Of{std::move(that.m_ଚାବି_Of)} {
        that.m_capacity = 0;
        that.m_size = 0;
    }
    V3HashTable& operator=(V3HashTable&& that) {
        m_table = std::move(that.m_table);  // Frees the table this held, if any
        m_capacity = that.m_capacity;
        m_size = that.m_size;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_ଚାବି_Of = std::move(that.m_ଚାବି_Of);
        that.m_capacity = 0;
        that.m_size = 0;
        ଫେରନ୍ତୁ *this;
    }

ସର୍ବସାଧାରଣ:
    // METHODS
    size_t size() ଅଚଳ { ଫେରନ୍ତୁ m_size; }
    bool empty() ଅଚଳ { ଫେରନ୍ତୁ !m_size; }

    iterator begin() ଅଚଳ {
        Slot* ଅଚଳ endp = m_table.get() + m_capacity;
        Slot* slotp = m_table.get();
        ଯେତେବେଳେ (slotp != endp && slotp->isFree()) ++slotp;
        ଫେରନ୍ତୁ iterator{slotp, endp};
    }
    iterator end() ଅଚଳ {
        Slot* ଅଚଳ endp = m_table.get() + m_capacity;
        ଫେରନ୍ତୁ iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    ଶୂନ୍ୟ reserve(size_t ଗଣନା_2) {
        size_t capacity = V3HashTableInternals::MIN_CAPACITY;
        ଯେତେବେଳେ (capacity * V3HashTableInternals::LOAD_FACTOR_ଅଙ୍କ
               < ଗଣନା_2 * V3HashTableInternals::LOAD_FACTOR_DEN)
            capacity *= 2;
        ଯଦି (capacity > m_capacity) resize(capacity);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    ଶୂନ୍ୟ clear() {
        ପାଇଁ (size_t i = 0; i < m_capacity; ++i) {
            ଯଦି (!m_table[i].isFree()) m_table[i].destroy();
        }
        m_size = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    template <typename... T_Args>
    iterator find(ଅଚଳ T_Args&... args) ଅଚଳ {
        static_assert(ValidHash<T_Args...>::ମୂଲ୍ୟ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_Args...>::ମୂଲ୍ୟ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ଯଦି (!m_size) ଫେରନ୍ତୁ end();  // Nothing to find, and this also covers there being no table
        Slot* ଅଚଳ slotp = m_table.get() + probe(hashOf(args...), args...);
        ଫେରନ୍ତୁ slotp->isFree() ? end() : iterator{slotp, m_table.get() + m_capacity};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> ଭର୍ତ୍ତି(ଅଚଳ Entry& entry) {
        static_assert(std::is_copy_constructible<Entry>::ମୂଲ୍ୟ,
                      "'Entry' must be copy constructible to use 'insert'");
        ଫେରନ୍ତୁ ଭର୍ତ୍ତି_Lazy(m_ଚାବି_Of(entry), [&entry]() -> Entry { ଫେରନ୍ତୁ entry; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    template <typename... T_Args>
    std::pair<iterator, bool> ଭର୍ତ୍ତି_Lazy(T_Args&&... args) {
        static_assert(ଆକାର...(T_Args) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<ଆକାର...(T_Args) - 1, std::tuple<T_Args...>>;
        static_assert(vlstd::is_invocable_r<Entry, Callable>::ମୂଲ୍ୟ,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        ଫେରନ୍ତୁ ଭର୍ତ୍ତି_Lazy_Impl(std::make_index_sequence<ଆକାର...(T_Args) - 1>{},
                              std::forward_as_tuple(std::forward<T_Args>(args)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    template <typename... T_Args>
    bool contains(ଅଚଳ T_Args&... args) ଅଚଳ {
        ଫେରନ୍ତୁ find(args...) != end();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    template <typename... T_Args>
    bool erase(ଅଚଳ T_Args&... args) {
        ଅଚଳ iterator it = find(args...);
        ଯଦି (it == end()) ଫେରନ୍ତୁ false;
        erase(it);
        ଫେରନ୍ତୁ true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    ଶୂନ୍ୟ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != end() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ଅଚଳ size_t mask = m_capacity - 1;
        size_t i = static_cast<size_t>(it.m_slotp - m_table.get());
        // Destroy the entry
        m_table[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_size;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        size_t j = i;
        ଯେତେବେଳେ (true) {
            j = (j + 1) & mask;
            Slot& slot = m_table[j];
            ଯଦି (slot.isFree()) ଭାଙ୍ଗ;
            // Move back if its home position does not lie in the cyclic range (i, j]
            ଯଦି (((j - (slot.m_hash & mask)) & mask) >= ((j - i) & mask)) {
                m_table[i] = std::move(slot);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

template <typename T_ଚାବି, typename T_Hash = std::hash<T_ଚାବି>,
          typename T_Equal = std::equal_to<T_ଚାବି>>
class V3HashSet final : ସର୍ବସାଧାରଣ V3HashTable<T_ଚାବି, T_Hash, T_Equal,
                                           V3HashTableInternals::V3_Hash_Table_ଚାବି_Is_Entry<T_ଚାବି>> {
    using Super
        = V3HashTable<T_ଚାବି, T_Hash, T_Equal, V3HashTableInternals::V3_Hash_Table_ଚାବି_Is_Entry<T_ଚାବି>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_ଚାବି>::ମୂଲ୍ୟ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ଚାବି>::ମୂଲ୍ୟ, "'T_Key' must be destructible");

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    V3HashSet() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    V3HashSet(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

template <typename T_ଚାବି, typename T_Val, typename T_Hash = std::hash<T_ଚାବି>,
          typename T_Equal = std::equal_to<T_ଚାବି>>
class V3HashMap final
    : ସର୍ବସାଧାରଣ V3HashTable<std::pair<T_ଚାବି, T_Val>, T_Hash, T_Equal,
                         V3HashTableInternals::V3_Hash_Table_ଚାବି_Is_First<T_ଚାବି, T_Val>> {
    using Super = V3HashTable<std::pair<T_ଚାବି, T_Val>, T_Hash, T_Equal,
                              V3HashTableInternals::V3_Hash_Table_ଚାବି_Is_First<T_ଚାବି, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_ଚାବି>::ମୂଲ୍ୟ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ଚାବି>::ମୂଲ୍ୟ, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::ମୂଲ୍ୟ, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::ମୂଲ୍ୟ, "'T_Val' must be destructible");

ସର୍ବସାଧାରଣ:
    // TYPES
    using ମୂଲ୍ୟ_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#endif  // Guard
