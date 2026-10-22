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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3HASHTABLE_H_
#وصف_ڪريو VERILATOR_V3HASHTABLE_H_

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

namespace V3_Hash_جدول_Internals {

constexpr سائيز_t MIN_صلاحيت = 16;  // Smallest table allocated
constexpr سائيز_t ڀريو_ضرب_ڪندڙ_انگ = 3;  // Numerator of the maximum load factor
constexpr سائيز_t ڀريو_ضرب_ڪندڙ_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
سانچو <typename T_ڪنجي>
بنية V3_Hash_جدول_ڪنجي_Is_داخلا final {
    using ڪنجي_2 = T_ڪنجي;  // What it yields, so the table need not deduce it
    ثابت T_ڪنجي& عامل()(ثابت T_ڪنجي& داخلا_2) ثابت { أرجع داخلا_2; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
سانچو <typename T_ڪنجي, typename T_Val>
بنية V3_Hash_جدول_ڪنجي_Is_پهريون final {
    using ڪنجي_2 = T_ڪنجي;  // What it yields, so the table need not deduce it
    ثابت T_ڪنجي& عامل()(ثابت std::pair<T_ڪنجي, T_Val>& داخلا_2) ثابت { أرجع داخلا_2.پهريون; }
    ساكن T_Val& قدر_جو(std::pair<T_ڪنجي, T_Val>& داخلا_2) { أرجع داخلا_2.ٻيو; }
};

فراغ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
سانچو <typename T_داخلا, typename T_Hash, typename T_Equal, typename T_ڪنجي_جو>
class V3_Hash_جدول VL_NOT_FINAL {
عام:
    // TYPES
    using داخلا_3 = T_داخلا;  // What is stored
    using ڪنجي_2 = typename T_ڪنجي_جو::ڪنجي_2;  // What entries are looked up by

خاص:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    سانچو <typename... T_دليلون>
    using ValidHash = vlstd::is_invocable_r<سائيز_t, ثابت T_Hash&, ثابت T_دليلون&...>;

    // Holds if the equality accepts a key and such a lookup key
    سانچو <typename... T_دليلون>
    using ValidEqual = vlstd::is_invocable_r<bool, ثابت T_Equal&, ثابت ڪنجي_2&, ثابت T_دليلون&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<ڪنجي_2>::قدر, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<ڪنجي_2>::قدر, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    بنية جڳهه final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        اتحاد {
            داخلا_3 m_داخلا;  // Stored entry; alive only while the slot is occupied
        };
        سائيز_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        جڳهه() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~جڳهه() {
            إذا (!isFree()) destroy();
        }
        جڳهه(ثابت جڳهه&) = delete;
        جڳهه(جڳهه&&) = delete;
        ثابت جڳهه& عامل=(ثابت جڳهه&) = delete;
        جڳهه& عامل=(جڳهه&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_داخلا) داخلا_3{std::move(that.m_داخلا)};
            m_hash = that.m_hash;
            that.destroy();
            أرجع *this;
        }

        bool isFree() ثابت { أرجع !m_hash; }

        // Construct the entry of this free slot from the given entry
        فراغ construct(سائيز_t hash, داخلا_3&& داخلا_2) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_داخلا) داخلا_3{std::move(داخلا_2)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        فراغ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_داخلا.~داخلا_3();
            m_hash = 0;
        }
    };

عام:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        دوست class V3_Hash_جدول;

        جڳهه* m_slotp = nullptr;  // The slot iterated, or the end of the table
        جڳهه* m_endp = nullptr;  // One past the last slot

        iterator(جڳهه* slotp, جڳهه* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    عام:
        iterator() = افتراضي;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ثابت داخلا_3& عامل*() ثابت { أرجع m_slotp->m_داخلا; }
        ثابت داخلا_3* عامل->() ثابت { أرجع &m_slotp->m_داخلا; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        خودڪار& قدر() ثابت { أرجع T_ڪنجي_جو::قدر_جو(m_slotp->m_داخلا); }
        // Pre-increment, skipping the free slots
        iterator& عامل++() {
            طالما (++m_slotp != m_endp && m_slotp->isFree()) {}
            أرجع *this;
        }
        bool عامل==(ثابت iterator& that) ثابت { أرجع m_slotp == that.m_slotp; }
        bool عامل!=(ثابت iterator& that) ثابت { أرجع m_slotp != that.m_slotp; }
    };

خاص:
    // STATE
    std::unique_ptr<جڳهه[]> m_جدول;  // The table, null when unallocated
    سائيز_t m_صلاحيت = 0;  // Number of slots in the table, a power of two, or zero
    سائيز_t m_سائيز = 0;  // Number of occupied slots
    VL_NO_UNIQUE_پتو_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_پتو_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_پتو_CXX20 T_ڪنجي_جو m_ڪنجي_جو;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    سانچو <typename... T_دليلون>
    سائيز_t hash_جو(ثابت T_دليلون&... دليلون) ثابت {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr سائيز_t استعمال_ٿيل_ٻٽو_انگ = سائيز_t{1} << (حجم(سائيز_t) * 8 - 1);
        أرجع static_cast<سائيز_t>(m_hash(دليلون...)) | استعمال_ٿيل_ٻٽو_انگ;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    سائيز_t free_جڳهه(سائيز_t hash) ثابت {
        ثابت سائيز_t نقاب = m_صلاحيت - 1;
        سائيز_t i = hash & نقاب;
        طالما (!m_جدول[i].isFree()) i = (i + 1) & نقاب;
        أرجع i;
    }

    // Resize to the given number of slots, which must fit all entries
    فراغ resize(سائيز_t ڳڻپ) {
        UDEBUGONLY(UASSERT(ڳڻپ && !(ڳڻپ & (ڳڻپ - 1)), "Capacity not a power of 2"););
        ثابت std::unique_ptr<جڳهه[]> old_جدول{std::move(m_جدول)};
        ثابت سائيز_t old_صلاحيت = m_صلاحيت;
        m_جدول = std::make_unique<جڳهه[]>(ڳڻپ);
        m_صلاحيت = ڳڻپ;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        لكل (سائيز_t i = 0; i < old_صلاحيت; ++i) {
            جڳهه& جڳهه_2 = old_جدول[i];
            إذا (!جڳهه_2.isFree()) m_جدول[free_جڳهه(جڳهه_2.m_hash)] = std::move(جڳهه_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    سانچو <typename... T_دليلون>
    سائيز_t probe(سائيز_t hash, ثابت T_دليلون&... دليلون) ثابت {
        UDEBUGONLY(UASSERT(m_جدول, "Table must be allocated"););
        ثابت سائيز_t نقاب = m_صلاحيت - 1;
        سائيز_t i = hash & نقاب;
        طالما (!m_جدول[i].isFree()) {
            ثابت جڳهه& جڳهه_2 = m_جدول[i];
            إذا (جڳهه_2.m_hash == hash && m_equal(m_ڪنجي_جو(جڳهه_2.m_داخلا), دليلون...)) اكسر;
            i = (i + 1) & نقاب;
        }
        أرجع i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    سانچو <سائيز_t... N_ڪنجي, typename T_All>
    std::pair<iterator, bool> وجهو_Lazy_Impl(std::اشاريو_sequence<N_ڪنجي...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_ڪنجي, T_All>...>::قدر,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_ڪنجي, T_All>...>::قدر,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ثابت سائيز_t hash = hash_جو(std::get<N_ڪنجي>(all)...);
        // Allocate on the first insertion
        إذا (VL_UNLIKELY(!m_صلاحيت)) resize(V3_Hash_جدول_Internals::MIN_صلاحيت);
        // Find the slot for the entry
        جڳهه* slotp = m_جدول.get() + probe(hash, std::get<N_ڪنجي>(all)...);
        // If occupied, it's the equivalent, and we are done
        إذا (!slotp->isFree()) أرجع {iterator{slotp, m_جدول.get() + m_صلاحيت}, false};
        // Table is growing
        ++m_سائيز;
        // Increase if necessary by load factor
        إذا (VL_UNLIKELY(m_سائيز * V3_Hash_جدول_Internals::ڀريو_ضرب_ڪندڙ_DEN
                        > m_صلاحيت * V3_Hash_جدول_Internals::ڀريو_ضرب_ڪندڙ_انگ)) {
            resize(m_صلاحيت * 2);
            slotp = m_جدول.get() + free_جڳهه(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<حجم...(N_ڪنجي)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#جيڪڏهن_وصف_ٿيل VL_DEBUG
        ثابت ڪنجي_2& ڪنجي = m_ڪنجي_جو(slotp->m_داخلا);
        UASSERT(hash_جو(ڪنجي) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(ڪنجي, std::get<N_ڪنجي>(all)...),
                "Created entry does not match the key it was looked up with");
#جيڪڏهن_پورو
        // Return newly create entry
        أرجع {iterator{slotp, m_جدول.get() + m_صلاحيت}, true};
    }

محمي:
    // CONSTRUCTORS
    V3_Hash_جدول() = افتراضي;
    V3_Hash_جدول(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3_Hash_جدول() = افتراضي;
    VL_UNCOPYABLE(V3_Hash_جدول);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3_Hash_جدول(V3_Hash_جدول&& that)
        : m_جدول{std::move(that.m_جدول)}
        , m_صلاحيت{that.m_صلاحيت}
        , m_سائيز{that.m_سائيز}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_ڪنجي_جو{std::move(that.m_ڪنجي_جو)} {
        that.m_صلاحيت = 0;
        that.m_سائيز = 0;
    }
    V3_Hash_جدول& عامل=(V3_Hash_جدول&& that) {
        m_جدول = std::move(that.m_جدول);  // Frees the table this held, if any
        m_صلاحيت = that.m_صلاحيت;
        m_سائيز = that.m_سائيز;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_ڪنجي_جو = std::move(that.m_ڪنجي_جو);
        that.m_صلاحيت = 0;
        that.m_سائيز = 0;
        أرجع *this;
    }

عام:
    // METHODS
    سائيز_t سائيز() ثابت { أرجع m_سائيز; }
    bool خالي_جايون() ثابت { أرجع !m_سائيز; }

    iterator begin() ثابت {
        جڳهه* ثابت endp = m_جدول.get() + m_صلاحيت;
        جڳهه* slotp = m_جدول.get();
        طالما (slotp != endp && slotp->isFree()) ++slotp;
        أرجع iterator{slotp, endp};
    }
    iterator پڇاڙي() ثابت {
        جڳهه* ثابت endp = m_جدول.get() + m_صلاحيت;
        أرجع iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    فراغ reserve(سائيز_t ڳڻپ) {
        سائيز_t صلاحيت = V3_Hash_جدول_Internals::MIN_صلاحيت;
        طالما (صلاحيت * V3_Hash_جدول_Internals::ڀريو_ضرب_ڪندڙ_انگ
               < ڳڻپ * V3_Hash_جدول_Internals::ڀريو_ضرب_ڪندڙ_DEN)
            صلاحيت *= 2;
        إذا (صلاحيت > m_صلاحيت) resize(صلاحيت);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    فراغ clear() {
        لكل (سائيز_t i = 0; i < m_صلاحيت; ++i) {
            إذا (!m_جدول[i].isFree()) m_جدول[i].destroy();
        }
        m_سائيز = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    سانچو <typename... T_دليلون>
    iterator ڳوليو(ثابت T_دليلون&... دليلون) ثابت {
        static_assert(ValidHash<T_دليلون...>::قدر,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_دليلون...>::قدر,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        إذا (!m_سائيز) أرجع پڇاڙي();  // Nothing to find, and this also covers there being no table
        جڳهه* ثابت slotp = m_جدول.get() + probe(hash_جو(دليلون...), دليلون...);
        أرجع slotp->isFree() ? پڇاڙي() : iterator{slotp, m_جدول.get() + m_صلاحيت};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> وجهو(ثابت داخلا_3& داخلا_2) {
        static_assert(std::is_copy_constructible<داخلا_3>::قدر,
                      "'Entry' must be copy constructible to use 'insert'");
        أرجع وجهو_Lazy(m_ڪنجي_جو(داخلا_2), [&داخلا_2]() -> داخلا_3 { أرجع داخلا_2; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    سانچو <typename... T_دليلون>
    std::pair<iterator, bool> وجهو_Lazy(T_دليلون&&... دليلون) {
        static_assert(حجم...(T_دليلون) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<حجم...(T_دليلون) - 1, std::tuple<T_دليلون...>>;
        static_assert(vlstd::is_invocable_r<داخلا_3, Callable>::قدر,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        أرجع وجهو_Lazy_Impl(std::make_اشاريو_sequence<حجم...(T_دليلون) - 1>{},
                              std::forward_as_tuple(std::forward<T_دليلون>(دليلون)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    سانچو <typename... T_دليلون>
    bool شامل_آهي(ثابت T_دليلون&... دليلون) ثابت {
        أرجع ڳوليو(دليلون...) != پڇاڙي();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    سانچو <typename... T_دليلون>
    bool erase(ثابت T_دليلون&... دليلون) {
        ثابت iterator it = ڳوليو(دليلون...);
        إذا (it == پڇاڙي()) أرجع false;
        erase(it);
        أرجع true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    فراغ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != پڇاڙي() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ثابت سائيز_t نقاب = m_صلاحيت - 1;
        سائيز_t i = static_cast<سائيز_t>(it.m_slotp - m_جدول.get());
        // Destroy the entry
        m_جدول[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_سائيز;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        سائيز_t j = i;
        طالما (true) {
            j = (j + 1) & نقاب;
            جڳهه& جڳهه_2 = m_جدول[j];
            إذا (جڳهه_2.isFree()) اكسر;
            // Move back if its home position does not lie in the cyclic range (i, j]
            إذا (((j - (جڳهه_2.m_hash & نقاب)) & نقاب) >= ((j - i) & نقاب)) {
                m_جدول[i] = std::move(جڳهه_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

سانچو <typename T_ڪنجي, typename T_Hash = std::hash<T_ڪنجي>,
          typename T_Equal = std::equal_منزل<T_ڪنجي>>
class V3_Hash_سيٽ final : عام V3_Hash_جدول<T_ڪنجي, T_Hash, T_Equal,
                                           V3_Hash_جدول_Internals::V3_Hash_جدول_ڪنجي_Is_داخلا<T_ڪنجي>> {
    using Super
        = V3_Hash_جدول<T_ڪنجي, T_Hash, T_Equal, V3_Hash_جدول_Internals::V3_Hash_جدول_ڪنجي_Is_داخلا<T_ڪنجي>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_ڪنجي>::قدر, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ڪنجي>::قدر, "'T_Key' must be destructible");

عام:
    // CONSTRUCTORS
    V3_Hash_سيٽ() = افتراضي;
    V3_Hash_سيٽ(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

سانچو <typename T_ڪنجي, typename T_Val, typename T_Hash = std::hash<T_ڪنجي>,
          typename T_Equal = std::equal_منزل<T_ڪنجي>>
class V3HashMap final
    : عام V3_Hash_جدول<std::pair<T_ڪنجي, T_Val>, T_Hash, T_Equal,
                         V3_Hash_جدول_Internals::V3_Hash_جدول_ڪنجي_Is_پهريون<T_ڪنجي, T_Val>> {
    using Super = V3_Hash_جدول<std::pair<T_ڪنجي, T_Val>, T_Hash, T_Equal,
                              V3_Hash_جدول_Internals::V3_Hash_جدول_ڪنجي_Is_پهريون<T_ڪنجي, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_ڪنجي>::قدر, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_ڪنجي>::قدر, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::قدر, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::قدر, "'T_Val' must be destructible");

عام:
    // TYPES
    using قدر_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = افتراضي;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#جيڪڏهن_پورو  // Guard
