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

#اگر_تعریف_نشده VERILATOR_V3HASHTABLE_H_
#تعریف_کن VERILATOR_V3HASHTABLE_H_

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
constexpr اندازه_t بارگیری_ضریب_رقم = 3;  // Numerator of the maximum load factor
constexpr اندازه_t بارگیری_ضریب_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
قالب <typename T_کلید>
بنية V3_Hash_جدول_کلید_Is_مدخل final {
    using کلید_2 = T_کلید;  // What it yields, so the table need not deduce it
    ثابت T_کلید& عملگر()(ثابت T_کلید& مدخل) ثابت { أرجع مدخل; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
قالب <typename T_کلید, typename T_Val>
بنية V3_Hash_جدول_کلید_Is_اولی final {
    using کلید_2 = T_کلید;  // What it yields, so the table need not deduce it
    ثابت T_کلید& عملگر()(ثابت std::pair<T_کلید, T_Val>& مدخل) ثابت { أرجع مدخل.اولی; }
    ساكن T_Val& مقدار_از(std::pair<T_کلید, T_Val>& مدخل) { أرجع مدخل.دومی; }
};

فراغ selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
قالب <typename T_مدخل, typename T_Hash, typename T_Equal, typename T_کلید_از>
class V3_Hash_جدول VL_NOT_FINAL {
عام:
    // TYPES
    using مدخل_2 = T_مدخل;  // What is stored
    using کلید_2 = typename T_کلید_از::کلید_2;  // What entries are looked up by

خاص:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    قالب <typename... T_آرگومانها>
    using ValidHash = vlstd::is_invocable_r<اندازه_t, ثابت T_Hash&, ثابت T_آرگومانها&...>;

    // Holds if the equality accepts a key and such a lookup key
    قالب <typename... T_آرگومانها>
    using ValidEqual = vlstd::is_invocable_r<bool, ثابت T_Equal&, ثابت کلید_2&, ثابت T_آرگومانها&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<کلید_2>::مقدار, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<کلید_2>::مقدار, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    بنية جایگاه final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        اتحاد {
            مدخل_2 m_مدخل;  // Stored entry; alive only while the slot is occupied
        };
        اندازه_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        جایگاه() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~جایگاه() {
            إذا (!isFree()) destroy();
        }
        جایگاه(ثابت جایگاه&) = delete;
        جایگاه(جایگاه&&) = delete;
        ثابت جایگاه& عملگر=(ثابت جایگاه&) = delete;
        جایگاه& عملگر=(جایگاه&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_مدخل) مدخل_2{std::move(that.m_مدخل)};
            m_hash = that.m_hash;
            that.destroy();
            أرجع *this;
        }

        bool isFree() ثابت { أرجع !m_hash; }

        // Construct the entry of this free slot from the given entry
        فراغ construct(اندازه_t hash, مدخل_2&& مدخل) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_مدخل) مدخل_2{std::move(مدخل)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        فراغ destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_مدخل.~مدخل_2();
            m_hash = 0;
        }
    };

عام:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        دوست class V3_Hash_جدول;

        جایگاه* m_slotp = nullptr;  // The slot iterated, or the end of the table
        جایگاه* m_endp = nullptr;  // One past the last slot

        iterator(جایگاه* slotp, جایگاه* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    عام:
        iterator() = افتراضي;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ثابت مدخل_2& عملگر*() ثابت { أرجع m_slotp->m_مدخل; }
        ثابت مدخل_2* عملگر->() ثابت { أرجع &m_slotp->m_مدخل; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        خودکار& مقدار() ثابت { أرجع T_کلید_از::مقدار_از(m_slotp->m_مدخل); }
        // Pre-increment, skipping the free slots
        iterator& عملگر++() {
            طالما (++m_slotp != m_endp && m_slotp->isFree()) {}
            أرجع *this;
        }
        bool عملگر==(ثابت iterator& that) ثابت { أرجع m_slotp == that.m_slotp; }
        bool عملگر!=(ثابت iterator& that) ثابت { أرجع m_slotp != that.m_slotp; }
    };

خاص:
    // STATE
    std::unique_ptr<جایگاه[]> m_جدول;  // The table, null when unallocated
    اندازه_t m_ظرفیت = 0;  // Number of slots in the table, a power of two, or zero
    اندازه_t m_اندازه = 0;  // Number of occupied slots
    VL_NO_UNIQUE_آدرس_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_آدرس_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_آدرس_CXX20 T_کلید_از m_کلید_از;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    قالب <typename... T_آرگومانها>
    اندازه_t hash_از(ثابت T_آرگومانها&... آرگومانها) ثابت {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr اندازه_t استفاده_شده_بیت = اندازه_t{1} << (حجم(اندازه_t) * 8 - 1);
        أرجع static_cast<اندازه_t>(m_hash(آرگومانها...)) | استفاده_شده_بیت;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    اندازه_t free_جایگاه(اندازه_t hash) ثابت {
        ثابت اندازه_t ماسک = m_ظرفیت - 1;
        اندازه_t i = hash & ماسک;
        طالما (!m_جدول[i].isFree()) i = (i + 1) & ماسک;
        أرجع i;
    }

    // Resize to the given number of slots, which must fit all entries
    فراغ resize(اندازه_t شمار) {
        UDEBUGONLY(UASSERT(شمار && !(شمار & (شمار - 1)), "Capacity not a power of 2"););
        ثابت std::unique_ptr<جایگاه[]> old_جدول{std::move(m_جدول)};
        ثابت اندازه_t old_ظرفیت = m_ظرفیت;
        m_جدول = std::make_unique<جایگاه[]>(شمار);
        m_ظرفیت = شمار;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        لكل (اندازه_t i = 0; i < old_ظرفیت; ++i) {
            جایگاه& جایگاه_2 = old_جدول[i];
            إذا (!جایگاه_2.isFree()) m_جدول[free_جایگاه(جایگاه_2.m_hash)] = std::move(جایگاه_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    قالب <typename... T_آرگومانها>
    اندازه_t probe(اندازه_t hash, ثابت T_آرگومانها&... آرگومانها) ثابت {
        UDEBUGONLY(UASSERT(m_جدول, "Table must be allocated"););
        ثابت اندازه_t ماسک = m_ظرفیت - 1;
        اندازه_t i = hash & ماسک;
        طالما (!m_جدول[i].isFree()) {
            ثابت جایگاه& جایگاه_2 = m_جدول[i];
            إذا (جایگاه_2.m_hash == hash && m_equal(m_کلید_از(جایگاه_2.m_مدخل), آرگومانها...)) اكسر;
            i = (i + 1) & ماسک;
        }
        أرجع i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    قالب <اندازه_t... N_کلید, typename T_All>
    std::pair<iterator, bool> درج_Lazy_Impl(std::شاخص_sequence<N_کلید...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_کلید, T_All>...>::مقدار,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_کلید, T_All>...>::مقدار,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ثابت اندازه_t hash = hash_از(std::get<N_کلید>(all)...);
        // Allocate on the first insertion
        إذا (VL_UNLIKELY(!m_ظرفیت)) resize(V3_Hash_جدول_Internals::MIN_ظرفیت);
        // Find the slot for the entry
        جایگاه* slotp = m_جدول.get() + probe(hash, std::get<N_کلید>(all)...);
        // If occupied, it's the equivalent, and we are done
        إذا (!slotp->isFree()) أرجع {iterator{slotp, m_جدول.get() + m_ظرفیت}, false};
        // Table is growing
        ++m_اندازه;
        // Increase if necessary by load factor
        إذا (VL_UNLIKELY(m_اندازه * V3_Hash_جدول_Internals::بارگیری_ضریب_DEN
                        > m_ظرفیت * V3_Hash_جدول_Internals::بارگیری_ضریب_رقم)) {
            resize(m_ظرفیت * 2);
            slotp = m_جدول.get() + free_جایگاه(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<حجم...(N_کلید)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#اگر_تعریف_شده VL_DEBUG
        ثابت کلید_2& کلید = m_کلید_از(slotp->m_مدخل);
        UASSERT(hash_از(کلید) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(کلید, std::get<N_کلید>(all)...),
                "Created entry does not match the key it was looked up with");
#پایان_اگر
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
        , m_کلید_از{std::move(that.m_کلید_از)} {
        that.m_ظرفیت = 0;
        that.m_اندازه = 0;
    }
    V3_Hash_جدول& عملگر=(V3_Hash_جدول&& that) {
        m_جدول = std::move(that.m_جدول);  // Frees the table this held, if any
        m_ظرفیت = that.m_ظرفیت;
        m_اندازه = that.m_اندازه;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_کلید_از = std::move(that.m_کلید_از);
        that.m_ظرفیت = 0;
        that.m_اندازه = 0;
        أرجع *this;
    }

عام:
    // METHODS
    اندازه_t اندازه() ثابت { أرجع m_اندازه; }
    bool خالی() ثابت { أرجع !m_اندازه; }

    iterator begin() ثابت {
        جایگاه* ثابت endp = m_جدول.get() + m_ظرفیت;
        جایگاه* slotp = m_جدول.get();
        طالما (slotp != endp && slotp->isFree()) ++slotp;
        أرجع iterator{slotp, endp};
    }
    iterator انتها() ثابت {
        جایگاه* ثابت endp = m_جدول.get() + m_ظرفیت;
        أرجع iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    فراغ reserve(اندازه_t شمار) {
        اندازه_t ظرفیت = V3_Hash_جدول_Internals::MIN_ظرفیت;
        طالما (ظرفیت * V3_Hash_جدول_Internals::بارگیری_ضریب_رقم
               < شمار * V3_Hash_جدول_Internals::بارگیری_ضریب_DEN)
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
    قالب <typename... T_آرگومانها>
    iterator پیدا_کن(ثابت T_آرگومانها&... آرگومانها) ثابت {
        static_assert(ValidHash<T_آرگومانها...>::مقدار,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_آرگومانها...>::مقدار,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        إذا (!m_اندازه) أرجع انتها();  // Nothing to find, and this also covers there being no table
        جایگاه* ثابت slotp = m_جدول.get() + probe(hash_از(آرگومانها...), آرگومانها...);
        أرجع slotp->isFree() ? انتها() : iterator{slotp, m_جدول.get() + m_ظرفیت};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> درج(ثابت مدخل_2& مدخل) {
        static_assert(std::is_copy_constructible<مدخل_2>::مقدار,
                      "'Entry' must be copy constructible to use 'insert'");
        أرجع درج_Lazy(m_کلید_از(مدخل), [&مدخل]() -> مدخل_2 { أرجع مدخل; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    قالب <typename... T_آرگومانها>
    std::pair<iterator, bool> درج_Lazy(T_آرگومانها&&... آرگومانها) {
        static_assert(حجم...(T_آرگومانها) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<حجم...(T_آرگومانها) - 1, std::tuple<T_آرگومانها...>>;
        static_assert(vlstd::is_invocable_r<مدخل_2, Callable>::مقدار,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        أرجع درج_Lazy_Impl(std::make_شاخص_sequence<حجم...(T_آرگومانها) - 1>{},
                              std::forward_as_tuple(std::forward<T_آرگومانها>(آرگومانها)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    قالب <typename... T_آرگومانها>
    bool شامل_است(ثابت T_آرگومانها&... آرگومانها) ثابت {
        أرجع پیدا_کن(آرگومانها...) != انتها();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    قالب <typename... T_آرگومانها>
    bool erase(ثابت T_آرگومانها&... آرگومانها) {
        ثابت iterator it = پیدا_کن(آرگومانها...);
        إذا (it == انتها()) أرجع false;
        erase(it);
        أرجع true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    فراغ erase(iterator it) {
        UDEBUGONLY(UASSERT(it != انتها() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
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
            جایگاه& جایگاه_2 = m_جدول[j];
            إذا (جایگاه_2.isFree()) اكسر;
            // Move back if its home position does not lie in the cyclic range (i, j]
            إذا (((j - (جایگاه_2.m_hash & ماسک)) & ماسک) >= ((j - i) & ماسک)) {
                m_جدول[i] = std::move(جایگاه_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

قالب <typename T_کلید, typename T_Hash = std::hash<T_کلید>,
          typename T_Equal = std::equal_مقصد<T_کلید>>
class V3_Hash_مجموعه final : عام V3_Hash_جدول<T_کلید, T_Hash, T_Equal,
                                           V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_مدخل<T_کلید>> {
    using Super
        = V3_Hash_جدول<T_کلید, T_Hash, T_Equal, V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_مدخل<T_کلید>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_کلید>::مقدار, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلید>::مقدار, "'T_Key' must be destructible");

عام:
    // CONSTRUCTORS
    V3_Hash_مجموعه() = افتراضي;
    V3_Hash_مجموعه(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

قالب <typename T_کلید, typename T_Val, typename T_Hash = std::hash<T_کلید>,
          typename T_Equal = std::equal_مقصد<T_کلید>>
class V3HashMap final
    : عام V3_Hash_جدول<std::pair<T_کلید, T_Val>, T_Hash, T_Equal,
                         V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_اولی<T_کلید, T_Val>> {
    using Super = V3_Hash_جدول<std::pair<T_کلید, T_Val>, T_Hash, T_Equal,
                              V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_اولی<T_کلید, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_کلید>::مقدار, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلید>::مقدار, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::مقدار, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::مقدار, "'T_Val' must be destructible");

عام:
    // TYPES
    using مقدار_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = افتراضي;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#پایان_اگر  // Guard
