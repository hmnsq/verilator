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

#నిర్వచితం_కాకపోతే VERILATOR_V3HASHTABLE_H_
#నిర్వచించు VERILATOR_V3HASHTABLE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3StdFuture.h"

#అన్తర్భావ <functional>
#అన్తర్భావ <memory>
#అన్తర్భావ <new>
#అన్తర్భావ <tuple>
#అన్తర్భావ <type_traits>
#అన్తర్భావ <utility>

namespace V3_Hash_పట్టిక_Internals {

constexpr కొలత_t MIN_సామర్థ్యం = 16;  // Smallest table allocated
constexpr కొలత_t ఎక్కించు_గుణకం_అంకె = 3;  // Numerator of the maximum load factor
constexpr కొలత_t ఎక్కించు_గుణకం_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
మూస <typename T_కీలకం>
నిర్మాణం V3_Hash_పట్టిక_కీలకం_Is_నమోదు final {
    using కీలకం_2 = T_కీలకం;  // What it yields, so the table need not deduce it
    స్థిరరాశి T_కీలకం& క్రియాకారకం()(స్థిరరాశి T_కీలకం& నమోదు_2) స్థిరరాశి { తిరిగి నమోదు_2; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
మూస <typename T_కీలకం, typename T_Val>
నిర్మాణం V3_Hash_పట్టిక_కీలకం_Is_మొదటి final {
    using కీలకం_2 = T_కీలకం;  // What it yields, so the table need not deduce it
    స్థిరరాశి T_కీలకం& క్రియాకారకం()(స్థిరరాశి std::pair<T_కీలకం, T_Val>& నమోదు_2) స్థిరరాశి { తిరిగి నమోదు_2.మొదటి; }
    స్థిర T_Val& విలువ_యొక్క(std::pair<T_కీలకం, T_Val>& నమోదు_2) { తిరిగి నమోదు_2.రెండవ; }
};

శూన్య selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
మూస <typename T_నమోదు, typename T_Hash, typename T_Equal, typename T_కీలకం_యొక్క>
class V3_Hash_పట్టిక VL_NOT_FINAL {
బహిరంగ:
    // TYPES
    using నమోదు_3 = T_నమోదు;  // What is stored
    using కీలకం_2 = typename T_కీలకం_యొక్క::కీలకం_2;  // What entries are looked up by

అంతరంగ:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    మూస <typename... T_వాదనలు>
    using ValidHash = vlstd::is_invocable_r<కొలత_t, స్థిరరాశి T_Hash&, స్థిరరాశి T_వాదనలు&...>;

    // Holds if the equality accepts a key and such a lookup key
    మూస <typename... T_వాదనలు>
    using ValidEqual = vlstd::is_invocable_r<bool, స్థిరరాశి T_Equal&, స్థిరరాశి కీలకం_2&, స్థిరరాశి T_వాదనలు&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<కీలకం_2>::విలువ, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<కీలకం_2>::విలువ, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    నిర్మాణం స్థానం final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        సంఘం {
            నమోదు_3 m_నమోదు;  // Stored entry; alive only while the slot is occupied
        };
        కొలత_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        స్థానం() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~స్థానం() {
            ఒకవేళ (!isFree()) destroy();
        }
        స్థానం(స్థిరరాశి స్థానం&) = delete;
        స్థానం(స్థానం&&) = delete;
        స్థిరరాశి స్థానం& క్రియాకారకం=(స్థిరరాశి స్థానం&) = delete;
        స్థానం& క్రియాకారకం=(స్థానం&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_నమోదు) నమోదు_3{std::move(that.m_నమోదు)};
            m_hash = that.m_hash;
            that.destroy();
            తిరిగి *this;
        }

        bool isFree() స్థిరరాశి { తిరిగి !m_hash; }

        // Construct the entry of this free slot from the given entry
        శూన్య construct(కొలత_t hash, నమోదు_3&& నమోదు_2) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_నమోదు) నమోదు_3{std::move(నమోదు_2)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        శూన్య destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_నమోదు.~నమోదు_3();
            m_hash = 0;
        }
    };

బహిరంగ:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        మిత్ర class V3_Hash_పట్టిక;

        స్థానం* m_slotp = nullptr;  // The slot iterated, or the end of the table
        స్థానం* m_endp = nullptr;  // One past the last slot

        iterator(స్థానం* slotp, స్థానం* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    బహిరంగ:
        iterator() = అప్రమేయ;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        స్థిరరాశి నమోదు_3& క్రియాకారకం*() స్థిరరాశి { తిరిగి m_slotp->m_నమోదు; }
        స్థిరరాశి నమోదు_3* క్రియాకారకం->() స్థిరరాశి { తిరిగి &m_slotp->m_నమోదు; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        స్వయంచాలక& విలువ() స్థిరరాశి { తిరిగి T_కీలకం_యొక్క::విలువ_యొక్క(m_slotp->m_నమోదు); }
        // Pre-increment, skipping the free slots
        iterator& క్రియాకారకం++() {
            అయితే (++m_slotp != m_endp && m_slotp->isFree()) {}
            తిరిగి *this;
        }
        bool క్రియాకారకం==(స్థిరరాశి iterator& that) స్థిరరాశి { తిరిగి m_slotp == that.m_slotp; }
        bool క్రియాకారకం!=(స్థిరరాశి iterator& that) స్థిరరాశి { తిరిగి m_slotp != that.m_slotp; }
    };

అంతరంగ:
    // STATE
    std::unique_ptr<స్థానం[]> m_పట్టిక;  // The table, null when unallocated
    కొలత_t m_సామర్థ్యం = 0;  // Number of slots in the table, a power of two, or zero
    కొలత_t m_కొలత = 0;  // Number of occupied slots
    VL_NO_UNIQUE_చిరునామా_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_చిరునామా_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_చిరునామా_CXX20 T_కీలకం_యొక్క m_కీలకం_యొక్క;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    మూస <typename... T_వాదనలు>
    కొలత_t hash_యొక్క(స్థిరరాశి T_వాదనలు&... వాదనలు) స్థిరరాశి {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr కొలత_t వాడిన_బిట్టు = కొలత_t{1} << (పరిమాణం(కొలత_t) * 8 - 1);
        తిరిగి static_cast<కొలత_t>(m_hash(వాదనలు...)) | వాడిన_బిట్టు;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    కొలత_t free_స్థానం(కొలత_t hash) స్థిరరాశి {
        స్థిరరాశి కొలత_t ముసుగు = m_సామర్థ్యం - 1;
        కొలత_t i = hash & ముసుగు;
        అయితే (!m_పట్టిక[i].isFree()) i = (i + 1) & ముసుగు;
        తిరిగి i;
    }

    // Resize to the given number of slots, which must fit all entries
    శూన్య resize(కొలత_t లెక్క) {
        UDEBUGONLY(UASSERT(లెక్క && !(లెక్క & (లెక్క - 1)), "Capacity not a power of 2"););
        స్థిరరాశి std::unique_ptr<స్థానం[]> old_పట్టిక{std::move(m_పట్టిక)};
        స్థిరరాశి కొలత_t old_సామర్థ్యం = m_సామర్థ్యం;
        m_పట్టిక = std::make_unique<స్థానం[]>(లెక్క);
        m_సామర్థ్యం = లెక్క;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        కొరకు (కొలత_t i = 0; i < old_సామర్థ్యం; ++i) {
            స్థానం& స్థానం_2 = old_పట్టిక[i];
            ఒకవేళ (!స్థానం_2.isFree()) m_పట్టిక[free_స్థానం(స్థానం_2.m_hash)] = std::move(స్థానం_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    మూస <typename... T_వాదనలు>
    కొలత_t probe(కొలత_t hash, స్థిరరాశి T_వాదనలు&... వాదనలు) స్థిరరాశి {
        UDEBUGONLY(UASSERT(m_పట్టిక, "Table must be allocated"););
        స్థిరరాశి కొలత_t ముసుగు = m_సామర్థ్యం - 1;
        కొలత_t i = hash & ముసుగు;
        అయితే (!m_పట్టిక[i].isFree()) {
            స్థిరరాశి స్థానం& స్థానం_2 = m_పట్టిక[i];
            ఒకవేళ (స్థానం_2.m_hash == hash && m_equal(m_కీలకం_యొక్క(స్థానం_2.m_నమోదు), వాదనలు...)) ఆపు;
            i = (i + 1) & ముసుగు;
        }
        తిరిగి i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    మూస <కొలత_t... N_కీలకం, typename T_All>
    std::pair<iterator, bool> చొప్పించు_Lazy_Impl(std::సూచీ_sequence<N_కీలకం...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_కీలకం, T_All>...>::విలువ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_కీలకం, T_All>...>::విలువ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        స్థిరరాశి కొలత_t hash = hash_యొక్క(std::get<N_కీలకం>(all)...);
        // Allocate on the first insertion
        ఒకవేళ (VL_UNLIKELY(!m_సామర్థ్యం)) resize(V3_Hash_పట్టిక_Internals::MIN_సామర్థ్యం);
        // Find the slot for the entry
        స్థానం* slotp = m_పట్టిక.get() + probe(hash, std::get<N_కీలకం>(all)...);
        // If occupied, it's the equivalent, and we are done
        ఒకవేళ (!slotp->isFree()) తిరిగి {iterator{slotp, m_పట్టిక.get() + m_సామర్థ్యం}, false};
        // Table is growing
        ++m_కొలత;
        // Increase if necessary by load factor
        ఒకవేళ (VL_UNLIKELY(m_కొలత * V3_Hash_పట్టిక_Internals::ఎక్కించు_గుణకం_DEN
                        > m_సామర్థ్యం * V3_Hash_పట్టిక_Internals::ఎక్కించు_గుణకం_అంకె)) {
            resize(m_సామర్థ్యం * 2);
            slotp = m_పట్టిక.get() + free_స్థానం(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<పరిమాణం...(N_కీలకం)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#నిర్వచితమైతే VL_DEBUG
        స్థిరరాశి కీలకం_2& కీలకం = m_కీలకం_యొక్క(slotp->m_నమోదు);
        UASSERT(hash_యొక్క(కీలకం) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(కీలకం, std::get<N_కీలకం>(all)...),
                "Created entry does not match the key it was looked up with");
#అయితే_ముగింపు
        // Return newly create entry
        తిరిగి {iterator{slotp, m_పట్టిక.get() + m_సామర్థ్యం}, true};
    }

రక్షిత:
    // CONSTRUCTORS
    V3_Hash_పట్టిక() = అప్రమేయ;
    V3_Hash_పట్టిక(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3_Hash_పట్టిక() = అప్రమేయ;
    VL_UNCOPYABLE(V3_Hash_పట్టిక);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3_Hash_పట్టిక(V3_Hash_పట్టిక&& that)
        : m_పట్టిక{std::move(that.m_పట్టిక)}
        , m_సామర్థ్యం{that.m_సామర్థ్యం}
        , m_కొలత{that.m_కొలత}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_కీలకం_యొక్క{std::move(that.m_కీలకం_యొక్క)} {
        that.m_సామర్థ్యం = 0;
        that.m_కొలత = 0;
    }
    V3_Hash_పట్టిక& క్రియాకారకం=(V3_Hash_పట్టిక&& that) {
        m_పట్టిక = std::move(that.m_పట్టిక);  // Frees the table this held, if any
        m_సామర్థ్యం = that.m_సామర్థ్యం;
        m_కొలత = that.m_కొలత;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_కీలకం_యొక్క = std::move(that.m_కీలకం_యొక్క);
        that.m_సామర్థ్యం = 0;
        that.m_కొలత = 0;
        తిరిగి *this;
    }

బహిరంగ:
    // METHODS
    కొలత_t కొలత() స్థిరరాశి { తిరిగి m_కొలత; }
    bool ఖాళీలు() స్థిరరాశి { తిరిగి !m_కొలత; }

    iterator begin() స్థిరరాశి {
        స్థానం* స్థిరరాశి endp = m_పట్టిక.get() + m_సామర్థ్యం;
        స్థానం* slotp = m_పట్టిక.get();
        అయితే (slotp != endp && slotp->isFree()) ++slotp;
        తిరిగి iterator{slotp, endp};
    }
    iterator చివర() స్థిరరాశి {
        స్థానం* స్థిరరాశి endp = m_పట్టిక.get() + m_సామర్థ్యం;
        తిరిగి iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    శూన్య reserve(కొలత_t లెక్క) {
        కొలత_t సామర్థ్యం = V3_Hash_పట్టిక_Internals::MIN_సామర్థ్యం;
        అయితే (సామర్థ్యం * V3_Hash_పట్టిక_Internals::ఎక్కించు_గుణకం_అంకె
               < లెక్క * V3_Hash_పట్టిక_Internals::ఎక్కించు_గుణకం_DEN)
            సామర్థ్యం *= 2;
        ఒకవేళ (సామర్థ్యం > m_సామర్థ్యం) resize(సామర్థ్యం);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    శూన్య clear() {
        కొరకు (కొలత_t i = 0; i < m_సామర్థ్యం; ++i) {
            ఒకవేళ (!m_పట్టిక[i].isFree()) m_పట్టిక[i].destroy();
        }
        m_కొలత = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    మూస <typename... T_వాదనలు>
    iterator కనుగొను(స్థిరరాశి T_వాదనలు&... వాదనలు) స్థిరరాశి {
        static_assert(ValidHash<T_వాదనలు...>::విలువ,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_వాదనలు...>::విలువ,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ఒకవేళ (!m_కొలత) తిరిగి చివర();  // Nothing to find, and this also covers there being no table
        స్థానం* స్థిరరాశి slotp = m_పట్టిక.get() + probe(hash_యొక్క(వాదనలు...), వాదనలు...);
        తిరిగి slotp->isFree() ? చివర() : iterator{slotp, m_పట్టిక.get() + m_సామర్థ్యం};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> చొప్పించు(స్థిరరాశి నమోదు_3& నమోదు_2) {
        static_assert(std::is_copy_constructible<నమోదు_3>::విలువ,
                      "'Entry' must be copy constructible to use 'insert'");
        తిరిగి చొప్పించు_Lazy(m_కీలకం_యొక్క(నమోదు_2), [&నమోదు_2]() -> నమోదు_3 { తిరిగి నమోదు_2; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    మూస <typename... T_వాదనలు>
    std::pair<iterator, bool> చొప్పించు_Lazy(T_వాదనలు&&... వాదనలు) {
        static_assert(పరిమాణం...(T_వాదనలు) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<పరిమాణం...(T_వాదనలు) - 1, std::tuple<T_వాదనలు...>>;
        static_assert(vlstd::is_invocable_r<నమోదు_3, Callable>::విలువ,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        తిరిగి చొప్పించు_Lazy_Impl(std::make_సూచీ_sequence<పరిమాణం...(T_వాదనలు) - 1>{},
                              std::forward_as_tuple(std::forward<T_వాదనలు>(వాదనలు)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    మూస <typename... T_వాదనలు>
    bool కలిగియుంది(స్థిరరాశి T_వాదనలు&... వాదనలు) స్థిరరాశి {
        తిరిగి కనుగొను(వాదనలు...) != చివర();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    మూస <typename... T_వాదనలు>
    bool erase(స్థిరరాశి T_వాదనలు&... వాదనలు) {
        స్థిరరాశి iterator it = కనుగొను(వాదనలు...);
        ఒకవేళ (it == చివర()) తిరిగి false;
        erase(it);
        తిరిగి true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    శూన్య erase(iterator it) {
        UDEBUGONLY(UASSERT(it != చివర() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        స్థిరరాశి కొలత_t ముసుగు = m_సామర్థ్యం - 1;
        కొలత_t i = static_cast<కొలత_t>(it.m_slotp - m_పట్టిక.get());
        // Destroy the entry
        m_పట్టిక[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_కొలత;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        కొలత_t j = i;
        అయితే (true) {
            j = (j + 1) & ముసుగు;
            స్థానం& స్థానం_2 = m_పట్టిక[j];
            ఒకవేళ (స్థానం_2.isFree()) ఆపు;
            // Move back if its home position does not lie in the cyclic range (i, j]
            ఒకవేళ (((j - (స్థానం_2.m_hash & ముసుగు)) & ముసుగు) >= ((j - i) & ముసుగు)) {
                m_పట్టిక[i] = std::move(స్థానం_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

మూస <typename T_కీలకం, typename T_Hash = std::hash<T_కీలకం>,
          typename T_Equal = std::equal_గమ్యం<T_కీలకం>>
class V3_Hash_సమితి final : బహిరంగ V3_Hash_పట్టిక<T_కీలకం, T_Hash, T_Equal,
                                           V3_Hash_పట్టిక_Internals::V3_Hash_పట్టిక_కీలకం_Is_నమోదు<T_కీలకం>> {
    using Super
        = V3_Hash_పట్టిక<T_కీలకం, T_Hash, T_Equal, V3_Hash_పట్టిక_Internals::V3_Hash_పట్టిక_కీలకం_Is_నమోదు<T_కీలకం>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_కీలకం>::విలువ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_కీలకం>::విలువ, "'T_Key' must be destructible");

బహిరంగ:
    // CONSTRUCTORS
    V3_Hash_సమితి() = అప్రమేయ;
    V3_Hash_సమితి(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

మూస <typename T_కీలకం, typename T_Val, typename T_Hash = std::hash<T_కీలకం>,
          typename T_Equal = std::equal_గమ్యం<T_కీలకం>>
class V3HashMap final
    : బహిరంగ V3_Hash_పట్టిక<std::pair<T_కీలకం, T_Val>, T_Hash, T_Equal,
                         V3_Hash_పట్టిక_Internals::V3_Hash_పట్టిక_కీలకం_Is_మొదటి<T_కీలకం, T_Val>> {
    using Super = V3_Hash_పట్టిక<std::pair<T_కీలకం, T_Val>, T_Hash, T_Equal,
                              V3_Hash_పట్టిక_Internals::V3_Hash_పట్టిక_కీలకం_Is_మొదటి<T_కీలకం, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_కీలకం>::విలువ, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_కీలకం>::విలువ, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::విలువ, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::విలువ, "'T_Val' must be destructible");

బహిరంగ:
    // TYPES
    using విలువ_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = అప్రమేయ;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#అయితే_ముగింపు  // Guard
