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

#جے_بنا_تعریف VERILATOR_V3HASHTABLE_H_
#تعریف_کرو VERILATOR_V3HASHTABLE_H_

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

constexpr ناپ_t MIN_سمرتھا = 16;  // Smallest table allocated
constexpr ناپ_t لدو_ضارب_عدد = 3;  // Numerator of the maximum load factor
constexpr ناپ_t لدو_ضارب_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
سانچا <typename T_کلید>
بنية V3_Hash_جدول_کلید_Is_اندراج final {
    using کلید_2 = T_کلید;  // What it yields, so the table need not deduce it
    ثابت T_کلید& عامل()(ثابت T_کلید& اندراج) ثابت { أرجع اندراج; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
سانچا <typename T_کلید, typename T_Val>
بنية V3_Hash_جدول_کلید_Is_پہلا final {
    using کلید_2 = T_کلید;  // What it yields, so the table need not deduce it
    ثابت T_کلید& عامل()(ثابت std::pair<T_کلید, T_Val>& اندراج) ثابت { أرجع اندراج.پہلا; }
    ساكن T_Val& قدر_دا(std::pair<T_کلید, T_Val>& اندراج) { أرجع اندراج.دوجا; }
};

فراغ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
سانچا <typename T_اندراج, typename T_Hash, typename T_Equal, typename T_کلید_دا>
class V3_Hash_جدول VL_NOT_FINAL {
عام:
    // TYPES
    using اندراج_2 = T_اندراج;  // What is stored
    using کلید_2 = typename T_کلید_دا::کلید_2;  // What entries are looked up by

خاص:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    سانچا <typename... T_دلیلاں>
    using ValidHash = vlstd::is_invocable_r<ناپ_t, ثابت T_Hash&, ثابت T_دلیلاں&...>;

    // Holds if the equality accepts a key and such a lookup key
    سانچا <typename... T_دلیلاں>
    using ValidEqual = vlstd::is_invocable_r<bool, ثابت T_Equal&, ثابت کلید_2&, ثابت T_دلیلاں&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<کلید_2>::قدر, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<کلید_2>::قدر, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    بنية تھاں final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        اتحاد {
            اندراج_2 m_اندراج;  // Stored entry; alive only while the slot is occupied
        };
        ناپ_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        تھاں() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~تھاں() {
            إذا (!isFree()) destroy();
        }
        تھاں(ثابت تھاں&) = delete;
        تھاں(تھاں&&) = delete;
        ثابت تھاں& عامل=(ثابت تھاں&) = delete;
        تھاں& عامل=(تھاں&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_اندراج) اندراج_2{std::move(that.m_اندراج)};
            m_hash = that.m_hash;
            that.destroy();
            أرجع *this;
        }

        bool isFree() ثابت { أرجع !m_hash; }

        // Construct the entry of this free slot from the given entry
        فراغ construct(ناپ_t hash, اندراج_2&& اندراج) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_اندراج) اندراج_2{std::move(اندراج)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        فراغ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_اندراج.~اندراج_2();
            m_hash = 0;
        }
    };

عام:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        یار class V3_Hash_جدول;

        تھاں* m_slotp = nullptr;  // The slot iterated, or the end of the table
        تھاں* m_endp = nullptr;  // One past the last slot

        iterator(تھاں* slotp, تھاں* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    عام:
        iterator() = افتراضي;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ثابت اندراج_2& عامل*() ثابت { أرجع m_slotp->m_اندراج; }
        ثابت اندراج_2* عامل->() ثابت { أرجع &m_slotp->m_اندراج; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        خودکار& قدر() ثابت { أرجع T_کلید_دا::قدر_دا(m_slotp->m_اندراج); }
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
    std::unique_ptr<تھاں[]> m_جدول;  // The table, null when unallocated
    ناپ_t m_سمرتھا = 0;  // Number of slots in the table, a power of two, or zero
    ناپ_t m_ناپ = 0;  // Number of occupied slots
    VL_NO_UNIQUE_پتہ_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_پتہ_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_پتہ_CXX20 T_کلید_دا m_کلید_دا;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    سانچا <typename... T_دلیلاں>
    ناپ_t hash_دا(ثابت T_دلیلاں&... دلیلاں) ثابت {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr ناپ_t ورتیا_ثنائی_ہندسہ = ناپ_t{1} << (حجم(ناپ_t) * 8 - 1);
        أرجع static_cast<ناپ_t>(m_hash(دلیلاں...)) | ورتیا_ثنائی_ہندسہ;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    ناپ_t free_تھاں(ناپ_t hash) ثابت {
        ثابت ناپ_t نقاب = m_سمرتھا - 1;
        ناپ_t i = hash & نقاب;
        طالما (!m_جدول[i].isFree()) i = (i + 1) & نقاب;
        أرجع i;
    }

    // Resize to the given number of slots, which must fit all entries
    فراغ resize(ناپ_t گنتی) {
        UDEBUGONLY(UASSERT(گنتی && !(گنتی & (گنتی - 1)), "Capacity not a power of 2"););
        ثابت std::unique_ptr<تھاں[]> old_جدول{std::move(m_جدول)};
        ثابت ناپ_t old_سمرتھا = m_سمرتھا;
        m_جدول = std::make_unique<تھاں[]>(گنتی);
        m_سمرتھا = گنتی;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        لكل (ناپ_t i = 0; i < old_سمرتھا; ++i) {
            تھاں& تھاں_2 = old_جدول[i];
            إذا (!تھاں_2.isFree()) m_جدول[free_تھاں(تھاں_2.m_hash)] = std::move(تھاں_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    سانچا <typename... T_دلیلاں>
    ناپ_t probe(ناپ_t hash, ثابت T_دلیلاں&... دلیلاں) ثابت {
        UDEBUGONLY(UASSERT(m_جدول, "Table must be allocated"););
        ثابت ناپ_t نقاب = m_سمرتھا - 1;
        ناپ_t i = hash & نقاب;
        طالما (!m_جدول[i].isFree()) {
            ثابت تھاں& تھاں_2 = m_جدول[i];
            إذا (تھاں_2.m_hash == hash && m_equal(m_کلید_دا(تھاں_2.m_اندراج), دلیلاں...)) اكسر;
            i = (i + 1) & نقاب;
        }
        أرجع i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    سانچا <ناپ_t... N_کلید, typename T_All>
    std::pair<iterator, bool> داخل_کرو_Lazy_Impl(std::اشاریہ_sequence<N_کلید...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_کلید, T_All>...>::قدر,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_کلید, T_All>...>::قدر,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ثابت ناپ_t hash = hash_دا(std::get<N_کلید>(all)...);
        // Allocate on the first insertion
        إذا (VL_UNLIKELY(!m_سمرتھا)) resize(V3_Hash_جدول_Internals::MIN_سمرتھا);
        // Find the slot for the entry
        تھاں* slotp = m_جدول.get() + probe(hash, std::get<N_کلید>(all)...);
        // If occupied, it's the equivalent, and we are done
        إذا (!slotp->isFree()) أرجع {iterator{slotp, m_جدول.get() + m_سمرتھا}, false};
        // Table is growing
        ++m_ناپ;
        // Increase if necessary by load factor
        إذا (VL_UNLIKELY(m_ناپ * V3_Hash_جدول_Internals::لدو_ضارب_DEN
                        > m_سمرتھا * V3_Hash_جدول_Internals::لدو_ضارب_عدد)) {
            resize(m_سمرتھا * 2);
            slotp = m_جدول.get() + free_تھاں(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<حجم...(N_کلید)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#جے_تعریف_شدہ VL_DEBUG
        ثابت کلید_2& کلید = m_کلید_دا(slotp->m_اندراج);
        UASSERT(hash_دا(کلید) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(کلید, std::get<N_کلید>(all)...),
                "Created entry does not match the key it was looked up with");
#جے_انت
        // Return newly create entry
        أرجع {iterator{slotp, m_جدول.get() + m_سمرتھا}, true};
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
        , m_سمرتھا{that.m_سمرتھا}
        , m_ناپ{that.m_ناپ}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_کلید_دا{std::move(that.m_کلید_دا)} {
        that.m_سمرتھا = 0;
        that.m_ناپ = 0;
    }
    V3_Hash_جدول& عامل=(V3_Hash_جدول&& that) {
        m_جدول = std::move(that.m_جدول);  // Frees the table this held, if any
        m_سمرتھا = that.m_سمرتھا;
        m_ناپ = that.m_ناپ;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_کلید_دا = std::move(that.m_کلید_دا);
        that.m_سمرتھا = 0;
        that.m_ناپ = 0;
        أرجع *this;
    }

عام:
    // METHODS
    ناپ_t ناپ() ثابت { أرجع m_ناپ; }
    bool خالی_تھاواں() ثابت { أرجع !m_ناپ; }

    iterator begin() ثابت {
        تھاں* ثابت endp = m_جدول.get() + m_سمرتھا;
        تھاں* slotp = m_جدول.get();
        طالما (slotp != endp && slotp->isFree()) ++slotp;
        أرجع iterator{slotp, endp};
    }
    iterator سرا() ثابت {
        تھاں* ثابت endp = m_جدول.get() + m_سمرتھا;
        أرجع iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    فراغ reserve(ناپ_t گنتی) {
        ناپ_t سمرتھا = V3_Hash_جدول_Internals::MIN_سمرتھا;
        طالما (سمرتھا * V3_Hash_جدول_Internals::لدو_ضارب_عدد
               < گنتی * V3_Hash_جدول_Internals::لدو_ضارب_DEN)
            سمرتھا *= 2;
        إذا (سمرتھا > m_سمرتھا) resize(سمرتھا);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    فراغ clear() {
        لكل (ناپ_t i = 0; i < m_سمرتھا; ++i) {
            إذا (!m_جدول[i].isFree()) m_جدول[i].destroy();
        }
        m_ناپ = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    سانچا <typename... T_دلیلاں>
    iterator لبھو(ثابت T_دلیلاں&... دلیلاں) ثابت {
        static_assert(ValidHash<T_دلیلاں...>::قدر,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_دلیلاں...>::قدر,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        إذا (!m_ناپ) أرجع سرا();  // Nothing to find, and this also covers there being no table
        تھاں* ثابت slotp = m_جدول.get() + probe(hash_دا(دلیلاں...), دلیلاں...);
        أرجع slotp->isFree() ? سرا() : iterator{slotp, m_جدول.get() + m_سمرتھا};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> داخل_کرو(ثابت اندراج_2& اندراج) {
        static_assert(std::is_copy_constructible<اندراج_2>::قدر,
                      "'Entry' must be copy constructible to use 'insert'");
        أرجع داخل_کرو_Lazy(m_کلید_دا(اندراج), [&اندراج]() -> اندراج_2 { أرجع اندراج; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    سانچا <typename... T_دلیلاں>
    std::pair<iterator, bool> داخل_کرو_Lazy(T_دلیلاں&&... دلیلاں) {
        static_assert(حجم...(T_دلیلاں) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<حجم...(T_دلیلاں) - 1, std::tuple<T_دلیلاں...>>;
        static_assert(vlstd::is_invocable_r<اندراج_2, Callable>::قدر,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        أرجع داخل_کرو_Lazy_Impl(std::make_اشاریہ_sequence<حجم...(T_دلیلاں) - 1>{},
                              std::forward_as_tuple(std::forward<T_دلیلاں>(دلیلاں)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    سانچا <typename... T_دلیلاں>
    bool شامل_اے(ثابت T_دلیلاں&... دلیلاں) ثابت {
        أرجع لبھو(دلیلاں...) != سرا();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    سانچا <typename... T_دلیلاں>
    bool erase(ثابت T_دلیلاں&... دلیلاں) {
        ثابت iterator it = لبھو(دلیلاں...);
        إذا (it == سرا()) أرجع false;
        erase(it);
        أرجع true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    فراغ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != سرا() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ثابت ناپ_t نقاب = m_سمرتھا - 1;
        ناپ_t i = static_cast<ناپ_t>(it.m_slotp - m_جدول.get());
        // Destroy the entry
        m_جدول[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_ناپ;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        ناپ_t j = i;
        طالما (true) {
            j = (j + 1) & نقاب;
            تھاں& تھاں_2 = m_جدول[j];
            إذا (تھاں_2.isFree()) اكسر;
            // Move back if its home position does not lie in the cyclic range (i, j]
            إذا (((j - (تھاں_2.m_hash & نقاب)) & نقاب) >= ((j - i) & نقاب)) {
                m_جدول[i] = std::move(تھاں_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

سانچا <typename T_کلید, typename T_Hash = std::hash<T_کلید>,
          typename T_Equal = std::equal_منزل<T_کلید>>
class V3_Hash_سیٹ final : عام V3_Hash_جدول<T_کلید, T_Hash, T_Equal,
                                           V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_اندراج<T_کلید>> {
    using Super
        = V3_Hash_جدول<T_کلید, T_Hash, T_Equal, V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_اندراج<T_کلید>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_کلید>::قدر, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلید>::قدر, "'T_Key' must be destructible");

عام:
    // CONSTRUCTORS
    V3_Hash_سیٹ() = افتراضي;
    V3_Hash_سیٹ(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

سانچا <typename T_کلید, typename T_Val, typename T_Hash = std::hash<T_کلید>,
          typename T_Equal = std::equal_منزل<T_کلید>>
class V3HashMap final
    : عام V3_Hash_جدول<std::pair<T_کلید, T_Val>, T_Hash, T_Equal,
                         V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_پہلا<T_کلید, T_Val>> {
    using Super = V3_Hash_جدول<std::pair<T_کلید, T_Val>, T_Hash, T_Equal,
                              V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_پہلا<T_کلید, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_کلید>::قدر, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلید>::قدر, "'T_Key' must be destructible");
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

#جے_انت  // Guard
