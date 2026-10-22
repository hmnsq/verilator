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

#که_نه_تعریف_شوی VERILATOR_V3HASHTABLE_H_
#تعریف_کړه VERILATOR_V3HASHTABLE_H_

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

constexpr اندازه_t MIN_ظرفیت = 16;  // Smallest table allocated
constexpr اندازه_t بارول_ضریب_عدد = 3;  // Numerator of the maximum load factor
constexpr اندازه_t بارول_ضریب_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
کينډۍ <typename T_کلی>
بنية V3_Hash_جدول_کلی_Is_ثبت final {
    using کلی_2 = T_کلی;  // What it yields, so the table need not deduce it
    ثابت T_کلی& چلوونکی()(ثابت T_کلی& ثبت) ثابت { أرجع ثبت; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
کينډۍ <typename T_کلی, typename T_Val>
بنية V3_Hash_جدول_کلی_Is_لومړی final {
    using کلی_2 = T_کلی;  // What it yields, so the table need not deduce it
    ثابت T_کلی& چلوونکی()(ثابت std::pair<T_کلی, T_Val>& ثبت) ثابت { أرجع ثبت.لومړی; }
    ساكن T_Val& ارزښت_د(std::pair<T_کلی, T_Val>& ثبت) { أرجع ثبت.دوهم; }
};

فراغ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
کينډۍ <typename T_ثبت, typename T_Hash, typename T_Equal, typename T_کلی_د>
class V3_Hash_جدول VL_NOT_FINAL {
عام:
    // TYPES
    using ثبت_2 = T_ثبت;  // What is stored
    using کلی_2 = typename T_کلی_د::کلی_2;  // What entries are looked up by

خاص:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    کينډۍ <typename... T_دلیلونه>
    using ValidHash = vlstd::is_invocable_r<اندازه_t, ثابت T_Hash&, ثابت T_دلیلونه&...>;

    // Holds if the equality accepts a key and such a lookup key
    کينډۍ <typename... T_دلیلونه>
    using ValidEqual = vlstd::is_invocable_r<bool, ثابت T_Equal&, ثابت کلی_2&, ثابت T_دلیلونه&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<کلی_2>::ارزښت, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<کلی_2>::ارزښت, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    بنية خونه final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        اتحاد {
            ثبت_2 m_ثبت;  // Stored entry; alive only while the slot is occupied
        };
        اندازه_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        خونه() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~خونه() {
            إذا (!isFree()) destroy();
        }
        خونه(ثابت خونه&) = delete;
        خونه(خونه&&) = delete;
        ثابت خونه& چلوونکی=(ثابت خونه&) = delete;
        خونه& چلوونکی=(خونه&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_ثبت) ثبت_2{std::move(that.m_ثبت)};
            m_hash = that.m_hash;
            that.destroy();
            أرجع *this;
        }

        bool isFree() ثابت { أرجع !m_hash; }

        // Construct the entry of this free slot from the given entry
        فراغ construct(اندازه_t hash, ثبت_2&& ثبت) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_ثبت) ثبت_2{std::move(ثبت)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        فراغ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_ثبت.~ثبت_2();
            m_hash = 0;
        }
    };

عام:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        ملګری class V3_Hash_جدول;

        خونه* m_slotp = nullptr;  // The slot iterated, or the end of the table
        خونه* m_endp = nullptr;  // One past the last slot

        iterator(خونه* slotp, خونه* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    عام:
        iterator() = افتراضي;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ثابت ثبت_2& چلوونکی*() ثابت { أرجع m_slotp->m_ثبت; }
        ثابت ثبت_2* چلوونکی->() ثابت { أرجع &m_slotp->m_ثبت; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        خپلکاری& ارزښت() ثابت { أرجع T_کلی_د::ارزښت_د(m_slotp->m_ثبت); }
        // Pre-increment, skipping the free slots
        iterator& چلوونکی++() {
            طالما (++m_slotp != m_endp && m_slotp->isFree()) {}
            أرجع *this;
        }
        bool چلوونکی==(ثابت iterator& that) ثابت { أرجع m_slotp == that.m_slotp; }
        bool چلوونکی!=(ثابت iterator& that) ثابت { أرجع m_slotp != that.m_slotp; }
    };

خاص:
    // STATE
    std::unique_ptr<خونه[]> m_جدول;  // The table, null when unallocated
    اندازه_t m_ظرفیت = 0;  // Number of slots in the table, a power of two, or zero
    اندازه_t m_اندازه = 0;  // Number of occupied slots
    VL_NO_UNIQUE_پته_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_پته_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_پته_CXX20 T_کلی_د m_کلی_د;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    کينډۍ <typename... T_دلیلونه>
    اندازه_t hash_د(ثابت T_دلیلونه&... دلیلونه) ثابت {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr اندازه_t کارول_شوی_بټ = اندازه_t{1} << (حجم(اندازه_t) * 8 - 1);
        أرجع static_cast<اندازه_t>(m_hash(دلیلونه...)) | کارول_شوی_بټ;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    اندازه_t free_خونه(اندازه_t hash) ثابت {
        ثابت اندازه_t ماسک = m_ظرفیت - 1;
        اندازه_t i = hash & ماسک;
        طالما (!m_جدول[i].isFree()) i = (i + 1) & ماسک;
        أرجع i;
    }

    // Resize to the given number of slots, which must fit all entries
    فراغ resize(اندازه_t شمېر) {
        UDEBUGONLY(UASSERT(شمېر && !(شمېر & (شمېر - 1)), "Capacity not a power of 2"););
        ثابت std::unique_ptr<خونه[]> old_جدول{std::move(m_جدول)};
        ثابت اندازه_t old_ظرفیت = m_ظرفیت;
        m_جدول = std::make_unique<خونه[]>(شمېر);
        m_ظرفیت = شمېر;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        لكل (اندازه_t i = 0; i < old_ظرفیت; ++i) {
            خونه& خونه_2 = old_جدول[i];
            إذا (!خونه_2.isFree()) m_جدول[free_خونه(خونه_2.m_hash)] = std::move(خونه_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    کينډۍ <typename... T_دلیلونه>
    اندازه_t probe(اندازه_t hash, ثابت T_دلیلونه&... دلیلونه) ثابت {
        UDEBUGONLY(UASSERT(m_جدول, "Table must be allocated"););
        ثابت اندازه_t ماسک = m_ظرفیت - 1;
        اندازه_t i = hash & ماسک;
        طالما (!m_جدول[i].isFree()) {
            ثابت خونه& خونه_2 = m_جدول[i];
            إذا (خونه_2.m_hash == hash && m_equal(m_کلی_د(خونه_2.m_ثبت), دلیلونه...)) اكسر;
            i = (i + 1) & ماسک;
        }
        أرجع i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    کينډۍ <اندازه_t... N_کلی, typename T_All>
    std::pair<iterator, bool> ورننباسه_Lazy_Impl(std::شاخص_sequence<N_کلی...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_کلی, T_All>...>::ارزښت,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_کلی, T_All>...>::ارزښت,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ثابت اندازه_t hash = hash_د(std::get<N_کلی>(all)...);
        // Allocate on the first insertion
        إذا (VL_UNLIKELY(!m_ظرفیت)) resize(V3_Hash_جدول_Internals::MIN_ظرفیت);
        // Find the slot for the entry
        خونه* slotp = m_جدول.get() + probe(hash, std::get<N_کلی>(all)...);
        // If occupied, it's the equivalent, and we are done
        إذا (!slotp->isFree()) أرجع {iterator{slotp, m_جدول.get() + m_ظرفیت}, false};
        // Table is growing
        ++m_اندازه;
        // Increase if necessary by load factor
        إذا (VL_UNLIKELY(m_اندازه * V3_Hash_جدول_Internals::بارول_ضریب_DEN
                        > m_ظرفیت * V3_Hash_جدول_Internals::بارول_ضریب_عدد)) {
            resize(m_ظرفیت * 2);
            slotp = m_جدول.get() + free_خونه(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<حجم...(N_کلی)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#که_تعریف_شوی VL_DEBUG
        ثابت کلی_2& کلی = m_کلی_د(slotp->m_ثبت);
        UASSERT(hash_د(کلی) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(کلی, std::get<N_کلی>(all)...),
                "Created entry does not match the key it was looked up with");
#د_که_پای
        // Return newly create entry
        أرجع {iterator{slotp, m_جدول.get() + m_ظرفیت}, true};
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
        , m_ظرفیت{that.m_ظرفیت}
        , m_اندازه{that.m_اندازه}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_کلی_د{std::move(that.m_کلی_د)} {
        that.m_ظرفیت = 0;
        that.m_اندازه = 0;
    }
    V3_Hash_جدول& چلوونکی=(V3_Hash_جدول&& that) {
        m_جدول = std::move(that.m_جدول);  // Frees the table this held, if any
        m_ظرفیت = that.m_ظرفیت;
        m_اندازه = that.m_اندازه;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_کلی_د = std::move(that.m_کلی_د);
        that.m_ظرفیت = 0;
        that.m_اندازه = 0;
        أرجع *this;
    }

عام:
    // METHODS
    اندازه_t اندازه() ثابت { أرجع m_اندازه; }
    bool تشه() ثابت { أرجع !m_اندازه; }

    iterator begin() ثابت {
        خونه* ثابت endp = m_جدول.get() + m_ظرفیت;
        خونه* slotp = m_جدول.get();
        طالما (slotp != endp && slotp->isFree()) ++slotp;
        أرجع iterator{slotp, endp};
    }
    iterator پای_ټکی() ثابت {
        خونه* ثابت endp = m_جدول.get() + m_ظرفیت;
        أرجع iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    فراغ reserve(اندازه_t شمېر) {
        اندازه_t ظرفیت = V3_Hash_جدول_Internals::MIN_ظرفیت;
        طالما (ظرفیت * V3_Hash_جدول_Internals::بارول_ضریب_عدد
               < شمېر * V3_Hash_جدول_Internals::بارول_ضریب_DEN)
            ظرفیت *= 2;
        إذا (ظرفیت > m_ظرفیت) resize(ظرفیت);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    فراغ clear() {
        لكل (اندازه_t i = 0; i < m_ظرفیت; ++i) {
            إذا (!m_جدول[i].isFree()) m_جدول[i].destroy();
        }
        m_اندازه = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    کينډۍ <typename... T_دلیلونه>
    iterator پیدا_کړه(ثابت T_دلیلونه&... دلیلونه) ثابت {
        static_assert(ValidHash<T_دلیلونه...>::ارزښت,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_دلیلونه...>::ارزښت,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        إذا (!m_اندازه) أرجع پای_ټکی();  // Nothing to find, and this also covers there being no table
        خونه* ثابت slotp = m_جدول.get() + probe(hash_د(دلیلونه...), دلیلونه...);
        أرجع slotp->isFree() ? پای_ټکی() : iterator{slotp, m_جدول.get() + m_ظرفیت};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> ورننباسه(ثابت ثبت_2& ثبت) {
        static_assert(std::is_copy_constructible<ثبت_2>::ارزښت,
                      "'Entry' must be copy constructible to use 'insert'");
        أرجع ورننباسه_Lazy(m_کلی_د(ثبت), [&ثبت]() -> ثبت_2 { أرجع ثبت; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    کينډۍ <typename... T_دلیلونه>
    std::pair<iterator, bool> ورننباسه_Lazy(T_دلیلونه&&... دلیلونه) {
        static_assert(حجم...(T_دلیلونه) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<حجم...(T_دلیلونه) - 1, std::tuple<T_دلیلونه...>>;
        static_assert(vlstd::is_invocable_r<ثبت_2, Callable>::ارزښت,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        أرجع ورننباسه_Lazy_Impl(std::make_شاخص_sequence<حجم...(T_دلیلونه) - 1>{},
                              std::forward_as_tuple(std::forward<T_دلیلونه>(دلیلونه)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    کينډۍ <typename... T_دلیلونه>
    bool لري(ثابت T_دلیلونه&... دلیلونه) ثابت {
        أرجع پیدا_کړه(دلیلونه...) != پای_ټکی();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    کينډۍ <typename... T_دلیلونه>
    bool erase(ثابت T_دلیلونه&... دلیلونه) {
        ثابت iterator it = پیدا_کړه(دلیلونه...);
        إذا (it == پای_ټکی()) أرجع false;
        erase(it);
        أرجع true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    فراغ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != پای_ټکی() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ثابت اندازه_t ماسک = m_ظرفیت - 1;
        اندازه_t i = static_cast<اندازه_t>(it.m_slotp - m_جدول.get());
        // Destroy the entry
        m_جدول[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_اندازه;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        اندازه_t j = i;
        طالما (true) {
            j = (j + 1) & ماسک;
            خونه& خونه_2 = m_جدول[j];
            إذا (خونه_2.isFree()) اكسر;
            // Move back if its home position does not lie in the cyclic range (i, j]
            إذا (((j - (خونه_2.m_hash & ماسک)) & ماسک) >= ((j - i) & ماسک)) {
                m_جدول[i] = std::move(خونه_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

کينډۍ <typename T_کلی, typename T_Hash = std::hash<T_کلی>,
          typename T_Equal = std::equal_منزل<T_کلی>>
class V3_Hash_سېټ final : عام V3_Hash_جدول<T_کلی, T_Hash, T_Equal,
                                           V3_Hash_جدول_Internals::V3_Hash_جدول_کلی_Is_ثبت<T_کلی>> {
    using Super
        = V3_Hash_جدول<T_کلی, T_Hash, T_Equal, V3_Hash_جدول_Internals::V3_Hash_جدول_کلی_Is_ثبت<T_کلی>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_کلی>::ارزښت, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلی>::ارزښت, "'T_Key' must be destructible");

عام:
    // CONSTRUCTORS
    V3_Hash_سېټ() = افتراضي;
    V3_Hash_سېټ(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

کينډۍ <typename T_کلی, typename T_Val, typename T_Hash = std::hash<T_کلی>,
          typename T_Equal = std::equal_منزل<T_کلی>>
class V3HashMap final
    : عام V3_Hash_جدول<std::pair<T_کلی, T_Val>, T_Hash, T_Equal,
                         V3_Hash_جدول_Internals::V3_Hash_جدول_کلی_Is_لومړی<T_کلی, T_Val>> {
    using Super = V3_Hash_جدول<std::pair<T_کلی, T_Val>, T_Hash, T_Equal,
                              V3_Hash_جدول_Internals::V3_Hash_جدول_کلی_Is_لومړی<T_کلی, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_کلی>::ارزښت, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلی>::ارزښت, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::ارزښت, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::ارزښت, "'T_Val' must be destructible");

عام:
    // TYPES
    using ارزښت_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = افتراضي;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#د_که_پای  // Guard
