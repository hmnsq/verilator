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

#اگر_غیر_تعریف_شدہ VERILATOR_V3HASHTABLE_H_
#تعریف_کرو VERILATOR_V3HASHTABLE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3StdFuture.h"

#شامل <functional>
#شامل <memory>
#شامل <new>
#شامل <tuple>
#شامل <type_traits>
#شامل <utility>

namespace V3_Hash_جدول_Internals {

constexpr سائز_t MIN_استعداد = 16;  // Smallest table allocated
constexpr سائز_t بھرو_ضربی_عدد_عدد = 3;  // Numerator of the maximum load factor
constexpr سائز_t بھرو_ضربی_عدد_DEN = 4;  // Denominator of the maximum load factor

// Key extractor for a table whose entries are their own keys, that is, a set
سانچہ <typename T_کلید>
ساخت V3_Hash_جدول_کلید_Is_اندراج final {
    using کلید_2 = T_کلید;  // What it yields, so the table need not deduce it
    ثابت T_کلید& عامل()(ثابت T_کلید& اندراج_2) ثابت { واپس اندراج_2; }
};

// Key extractor for a table whose entries are pairs keyed by the first, that is, a map
سانچہ <typename T_کلید, typename T_Val>
ساخت V3_Hash_جدول_کلید_Is_پہلا final {
    using کلید_2 = T_کلید;  // What it yields, so the table need not deduce it
    ثابت T_کلید& عامل()(ثابت std::pair<T_کلید, T_Val>& اندراج_2) ثابت { واپس اندراج_2.پہلا; }
    ساکن T_Val& قدر_کا(std::pair<T_کلید, T_Val>& اندراج_2) { واپس اندراج_2.دوسرا; }
};

خالی selfTest();

}  // namespace V3HashTableInternals

// V3HashTable, see the file header
//  T_Entry  The entries (STL calls this value_type)
//  T_Hash   Hashes a lookup key
//  T_Equal  Compares a key to a lookup key
//  T_KeyOf  Yields the key of an entry
سانچہ <typename T_اندراج, typename T_Hash, typename T_Equal, typename T_کلید_کا>
class V3_Hash_جدول VL_NOT_FINAL {
عوامی:
    // TYPES
    using اندراج_3 = T_اندراج;  // What is stored
    using کلید_2 = typename T_کلید_کا::کلید_2;  // What entries are looked up by

نجی:
    // TYPES
    // Holds if the hash accepts a lookup key spelled as the given arguments
    سانچہ <typename... T_دلائل>
    using ValidHash = vlstd::is_invocable_r<سائز_t, ثابت T_Hash&, ثابت T_دلائل&...>;

    // Holds if the equality accepts a key and such a lookup key
    سانچہ <typename... T_دلائل>
    using ValidEqual = vlstd::is_invocable_r<bool, ثابت T_Equal&, ثابت کلید_2&, ثابت T_دلائل&...>;

    // The Key must itself be a valid lookup key, as every lookup ends in comparing one
    // against a stored entry. Asserted separately, so the failure names the functor.
    static_assert(ValidHash<کلید_2>::قدر, "The 'Hash' functor must accept the 'Key'");
    static_assert(ValidEqual<کلید_2>::قدر, "The 'Equal' functor must accept two 'Key's");

    // A table slot
    ساخت خانہ final {
        // The entry comes first, so it starts the slot whatever its alignment.
        // It is a union so it is alive only while the slot is occupied.
        اتحاد {
            اندراج_3 m_اندراج;  // Stored entry; alive only while the slot is occupied
        };
        سائز_t m_hash = 0;  // Hash of the entry, or zero when the slot is free

        خانہ() {}  // Leaves 'm_entry' uninitialized, as the slot is free
        ~خانہ() {
            اگر (!isFree()) destroy();
        }
        خانہ(ثابت خانہ&) = delete;
        خانہ(خانہ&&) = delete;
        ثابت خانہ& عامل=(ثابت خانہ&) = delete;
        خانہ& عامل=(خانہ&& that) {
            UDEBUGONLY(UASSERT(this != &that, "Moving a slot onto itself"););
            UDEBUGONLY(UASSERT(!that.isFree(), "Moving from a free slot"););
            UDEBUGONLY(UASSERT(isFree(), "Moving into an occupied slot"););
            new (&m_اندراج) اندراج_3{std::move(that.m_اندراج)};
            m_hash = that.m_hash;
            that.destroy();
            واپس *this;
        }

        bool isFree() ثابت { واپس !m_hash; }

        // Construct the entry of this free slot from the given entry
        خالی construct(سائز_t hash, اندراج_3&& اندراج_2) {
            UDEBUGONLY(UASSERT(isFree(), "Constructing the entry of an occupied slot"););
            new (&m_اندراج) اندراج_3{std::move(اندراج_2)};
            m_hash = hash;
        }
        // Destroy the entry of this occupied slot, leaving it free
        خالی destroy() {
            UDEBUGONLY(UASSERT(!isFree(), "Destroying the entry of a free slot"););
            m_اندراج.~اندراج_3();
            m_hash = 0;
        }
    };

عوامی:
    // Iterator over the entries, see the file header on invalidation
    class iterator final {
        دوست class V3_Hash_جدول;

        خانہ* m_slotp = nullptr;  // The slot iterated, or the end of the table
        خانہ* m_endp = nullptr;  // One past the last slot

        iterator(خانہ* slotp, خانہ* endp)
            : m_slotp{slotp}
            , m_endp{endp} {}

    عوامی:
        iterator() = طےشدہ;
        // As opposed to the STL, this always returns a const reference so an
        // entry is not mutable through an iterator alone. This is required
        // because entries must be movable, hence can't be const, but the key
        // of a map must not be modified. See 'value()' for the mapped value.
        ثابت اندراج_3& عامل*() ثابت { واپس m_slotp->m_اندراج; }
        ثابت اندراج_3* عامل->() ثابت { واپس &m_slotp->m_اندراج; }
        // Mutable reference to the mapped value, for a V3HashMap only, as in a set
        // the entry is the key. Changing it is safe, as it takes no part in the lookup.
        خودکار& قدر() ثابت { واپس T_کلید_کا::قدر_کا(m_slotp->m_اندراج); }
        // Pre-increment, skipping the free slots
        iterator& عامل++() {
            جب (++m_slotp != m_endp && m_slotp->isFree()) {}
            واپس *this;
        }
        bool عامل==(ثابت iterator& that) ثابت { واپس m_slotp == that.m_slotp; }
        bool عامل!=(ثابت iterator& that) ثابت { واپس m_slotp != that.m_slotp; }
    };

نجی:
    // STATE
    std::unique_ptr<خانہ[]> m_جدول;  // The table, null when unallocated
    سائز_t m_استعداد = 0;  // Number of slots in the table, a power of two, or zero
    سائز_t m_سائز = 0;  // Number of occupied slots
    VL_NO_UNIQUE_پتہ_CXX20 T_Hash m_hash;  // Hashes a lookup key
    VL_NO_UNIQUE_پتہ_CXX20 T_Equal m_equal;  // Compares a key to a lookup key
    VL_NO_UNIQUE_پتہ_CXX20 T_کلید_کا m_کلید_کا;  // Yields the lookup key of an entry

    // METHODS

    // The hash of the given entry or lookup key, as stored in a slot
    سانچہ <typename... T_دلائل>
    سائز_t hash_کا(ثابت T_دلائل&... دلائل) ثابت {
        // A free slot is one with a zero hash, so force the high bit into every hash.
        constexpr سائز_t مستعمل_ثنائی_ہندسہ = سائز_t{1} << (حجم(سائز_t) * 8 - 1);
        واپس static_cast<سائز_t>(m_hash(دلائل...)) | مستعمل_ثنائی_ہندسہ;
    }

    // Index of the free slot the given hash probes to. There must always be one.
    سائز_t free_خانہ(سائز_t hash) ثابت {
        ثابت سائز_t نقاب = m_استعداد - 1;
        سائز_t i = hash & نقاب;
        جب (!m_جدول[i].isFree()) i = (i + 1) & نقاب;
        واپس i;
    }

    // Resize to the given number of slots, which must fit all entries
    خالی resize(سائز_t گنتی) {
        UDEBUGONLY(UASSERT(گنتی && !(گنتی & (گنتی - 1)), "Capacity not a power of 2"););
        ثابت std::unique_ptr<خانہ[]> old_جدول{std::move(m_جدول)};
        ثابت سائز_t old_استعداد = m_استعداد;
        m_جدول = std::make_unique<خانہ[]>(گنتی);
        m_استعداد = گنتی;
        // Reinsert the entries. 'freeSlot' appends to the probe run of each, so the runs
        // come out contiguous whatever order this visits the old slots in.
        برائے (سائز_t i = 0; i < old_استعداد; ++i) {
            خانہ& خانہ_2 = old_جدول[i];
            اگر (!خانہ_2.isFree()) m_جدول[free_خانہ(خانہ_2.m_hash)] = std::move(خانہ_2);
        }
    }

    // Index of the slot holding the entry equal to the given key, or of the free slot its
    // probe sequence ends at. The table must not be empty.
    سانچہ <typename... T_دلائل>
    سائز_t probe(سائز_t hash, ثابت T_دلائل&... دلائل) ثابت {
        UDEBUGONLY(UASSERT(m_جدول, "Table must be allocated"););
        ثابت سائز_t نقاب = m_استعداد - 1;
        سائز_t i = hash & نقاب;
        جب (!m_جدول[i].isFree()) {
            ثابت خانہ& خانہ_2 = m_جدول[i];
            اگر (خانہ_2.m_hash == hash && m_equal(m_کلید_کا(خانہ_2.m_اندراج), دلائل...)) توڑو;
            i = (i + 1) & نقاب;
        }
        واپس i;
    }

    // Implementation of 'insertLazy' below. 'all' holds the key arguments, followed by
    // the callable that creates the entry, so 'N_Key' indexes the key.
    سانچہ <سائز_t... N_کلید, typename T_All>
    std::pair<iterator, bool> داخل_کرو_Lazy_Impl(std::اشاریہ_sequence<N_کلید...>, T_All&& all) {
        static_assert(ValidHash<std::tuple_element_t<N_کلید, T_All>...>::قدر,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<std::tuple_element_t<N_کلید, T_All>...>::قدر,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        ثابت سائز_t hash = hash_کا(std::get<N_کلید>(all)...);
        // Allocate on the first insertion
        اگر (VL_UNLIKELY(!m_استعداد)) resize(V3_Hash_جدول_Internals::MIN_استعداد);
        // Find the slot for the entry
        خانہ* slotp = m_جدول.get() + probe(hash, std::get<N_کلید>(all)...);
        // If occupied, it's the equivalent, and we are done
        اگر (!slotp->isFree()) واپس {iterator{slotp, m_جدول.get() + m_استعداد}, false};
        // Table is growing
        ++m_سائز;
        // Increase if necessary by load factor
        اگر (VL_UNLIKELY(m_سائز * V3_Hash_جدول_Internals::بھرو_ضربی_عدد_DEN
                        > m_استعداد * V3_Hash_جدول_Internals::بھرو_ضربی_عدد_عدد)) {
            resize(m_استعداد * 2);
            slotp = m_جدول.get() + free_خانہ(hash);
        }
        // Construct the entry via the user provided callable (last item in 'all')
        slotp->construct(hash, std::get<حجم...(N_کلید)>(all)());
        // The key of the created entry must both hash and compare as the key looked up
#اگر_تعریف_شدہ VL_DEBUG
        ثابت کلید_2& کلید = m_کلید_کا(slotp->m_اندراج);
        UASSERT(hash_کا(کلید) == hash,
                "Created entry does not hash as the key it was looked up with");
        UASSERT(m_equal(کلید, std::get<N_کلید>(all)...),
                "Created entry does not match the key it was looked up with");
#اگر_ختم
        // Return newly create entry
        واپس {iterator{slotp, m_جدول.get() + m_استعداد}, true};
    }

محفوظ:
    // CONSTRUCTORS
    V3_Hash_جدول() = طےشدہ;
    V3_Hash_جدول(T_Hash hash, T_Equal equal)
        : m_hash{std::move(hash)}
        , m_equal{std::move(equal)} {}
    ~V3_Hash_جدول() = طےشدہ;
    VL_UNCOPYABLE(V3_Hash_جدول);
    // Movable, as the table is just a pointer. The source is left empty rather than
    // merely unspecified, so it remains a usable, empty table.
    V3_Hash_جدول(V3_Hash_جدول&& that)
        : m_جدول{std::move(that.m_جدول)}
        , m_استعداد{that.m_استعداد}
        , m_سائز{that.m_سائز}
        , m_hash{std::move(that.m_hash)}
        , m_equal{std::move(that.m_equal)}
        , m_کلید_کا{std::move(that.m_کلید_کا)} {
        that.m_استعداد = 0;
        that.m_سائز = 0;
    }
    V3_Hash_جدول& عامل=(V3_Hash_جدول&& that) {
        m_جدول = std::move(that.m_جدول);  // Frees the table this held, if any
        m_استعداد = that.m_استعداد;
        m_سائز = that.m_سائز;
        m_hash = std::move(that.m_hash);
        m_equal = std::move(that.m_equal);
        m_کلید_کا = std::move(that.m_کلید_کا);
        that.m_استعداد = 0;
        that.m_سائز = 0;
        واپس *this;
    }

عوامی:
    // METHODS
    سائز_t سائز() ثابت { واپس m_سائز; }
    bool خالی_خانے() ثابت { واپس !m_سائز; }

    iterator begin() ثابت {
        خانہ* ثابت endp = m_جدول.get() + m_استعداد;
        خانہ* slotp = m_جدول.get();
        جب (slotp != endp && slotp->isFree()) ++slotp;
        واپس iterator{slotp, endp};
    }
    iterator انتہا() ثابت {
        خانہ* ثابت endp = m_جدول.get() + m_استعداد;
        واپس iterator{endp, endp};
    }

    // Make room for the given number of entries, so inserting that many will not resize
    خالی reserve(سائز_t گنتی) {
        سائز_t استعداد = V3_Hash_جدول_Internals::MIN_استعداد;
        جب (استعداد * V3_Hash_جدول_Internals::بھرو_ضربی_عدد_عدد
               < گنتی * V3_Hash_جدول_Internals::بھرو_ضربی_عدد_DEN)
            استعداد *= 2;
        اگر (استعداد > m_استعداد) resize(استعداد);
    }

    // Remove all entries. The table stays allocated. Invalidates every iterator.
    خالی clear() {
        برائے (سائز_t i = 0; i < m_استعداد; ++i) {
            اگر (!m_جدول[i].isFree()) m_جدول[i].destroy();
        }
        m_سائز = 0;
    }

    // Return iterator to the entry equal to the given key, or 'end()' if there
    // is none. The key is whatever T_Hash and T_Equal accept, spelled as any
    // number of arguments. Same as STL containers.
    سانچہ <typename... T_دلائل>
    iterator ڈھونڈو(ثابت T_دلائل&... دلائل) ثابت {
        static_assert(ValidHash<T_دلائل...>::قدر,
                      "The 'Hash' functor does not accept a lookup key spelled like this");
        static_assert(ValidEqual<T_دلائل...>::قدر,
                      "The 'Equal' functor does not accept a lookup key spelled like this");
        اگر (!m_سائز) واپس انتہا();  // Nothing to find, and this also covers there being no table
        خانہ* ثابت slotp = m_جدول.get() + probe(hash_کا(دلائل...), دلائل...);
        واپس slotp->isFree() ? انتہا() : iterator{slotp, m_جدول.get() + m_استعداد};
    }

    // Add the given entry, unless an equal one is in the table already. Return
    // iterator to the entry and true if insertion happened. Same as STL containers.
    std::pair<iterator, bool> داخل_کرو(ثابت اندراج_3& اندراج_2) {
        static_assert(std::is_copy_constructible<اندراج_3>::قدر,
                      "'Entry' must be copy constructible to use 'insert'");
        واپس داخل_کرو_Lazy(m_کلید_کا(اندراج_2), [&اندراج_2]() -> اندراج_3 { واپس اندراج_2; });
    }

    // As 'insert', but the entry is only made when needed: all but the last argument spell
    // the key, and the last is a callable to create the entry on a miss. Note the created entry
    // must hash and compare equal to the key, and the call must not touch the container, as this
    // holds the slot the entry will go in.
    سانچہ <typename... T_دلائل>
    std::pair<iterator, bool> داخل_کرو_Lazy(T_دلائل&&... دلائل) {
        static_assert(حجم...(T_دلائل) >= 2,
                      "'insertLazy' needs a lookup key, then a callable to create the entry");
        using Callable = std::tuple_element_t<حجم...(T_دلائل) - 1, std::tuple<T_دلائل...>>;
        static_assert(vlstd::is_invocable_r<اندراج_3, Callable>::قدر,
                      "The last argument of 'insertLazy' must be a callable that takes no "
                      "arguments and returns an 'Entry'");
        واپس داخل_کرو_Lazy_Impl(std::make_اشاریہ_sequence<حجم...(T_دلائل) - 1>{},
                              std::forward_as_tuple(std::forward<T_دلائل>(دلائل)...));
    }

    // Whether an entry equal to the given key is in the table. The key is spelled as for 'find'.
    سانچہ <typename... T_دلائل>
    bool شامل_ہے(ثابت T_دلائل&... دلائل) ثابت {
        واپس ڈھونڈو(دلائل...) != انتہا();
    }

    // Remove the entry equal to the given key, and return whether there was one.
    سانچہ <typename... T_دلائل>
    bool erase(ثابت T_دلائل&... دلائل) {
        ثابت iterator it = ڈھونڈو(دلائل...);
        اگر (it == انتہا()) واپس false;
        erase(it);
        واپس true;
    }

    // Remove the entry the given iterator refers to, which must not be 'end()'. Note that
    // unlike STL erase this returns nothing, as every iterator is invalidated on deletion.
    خالی erase(iterator it) {
        UDEBUGONLY(UASSERT(it != انتہا() && !it.m_slotp->isFree(), "Erasing a bad iterator"););
        ثابت سائز_t نقاب = m_استعداد - 1;
        سائز_t i = static_cast<سائز_t>(it.m_slotp - m_جدول.get());
        // Destroy the entry
        m_جدول[i].destroy();
        // The entry is gone, so slot 'i' is now a hole
        --m_سائز;
        // Backward shift deletion: move back the entries whose probing the hole breaks
        سائز_t j = i;
        جب (true) {
            j = (j + 1) & نقاب;
            خانہ& خانہ_2 = m_جدول[j];
            اگر (خانہ_2.isFree()) توڑو;
            // Move back if its home position does not lie in the cyclic range (i, j]
            اگر (((j - (خانہ_2.m_hash & نقاب)) & نقاب) >= ((j - i) & نقاب)) {
                m_جدول[i] = std::move(خانہ_2);  // Frees 'slot', which is then the hole
                i = j;
            }
        }
    }
};

سانچہ <typename T_کلید, typename T_Hash = std::hash<T_کلید>,
          typename T_Equal = std::equal_مقصد<T_کلید>>
class V3_Hash_سیٹ final : عوامی V3_Hash_جدول<T_کلید, T_Hash, T_Equal,
                                           V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_اندراج<T_کلید>> {
    using Super
        = V3_Hash_جدول<T_کلید, T_Hash, T_Equal, V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_اندراج<T_کلید>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    static_assert(std::is_move_constructible<T_کلید>::قدر, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلید>::قدر, "'T_Key' must be destructible");

عوامی:
    // CONSTRUCTORS
    V3_Hash_سیٹ() = طےشدہ;
    V3_Hash_سیٹ(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

سانچہ <typename T_کلید, typename T_Val, typename T_Hash = std::hash<T_کلید>,
          typename T_Equal = std::equal_مقصد<T_کلید>>
class V3HashMap final
    : عوامی V3_Hash_جدول<std::pair<T_کلید, T_Val>, T_Hash, T_Equal,
                         V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_پہلا<T_کلید, T_Val>> {
    using Super = V3_Hash_جدول<std::pair<T_کلید, T_Val>, T_Hash, T_Equal,
                              V3_Hash_جدول_Internals::V3_Hash_جدول_کلید_Is_پہلا<T_کلید, T_Val>>;

    // Entries are only ever moved. Note 'insert' additionally needs copy construction.
    // Asserted separately, so the failure names the one at fault.
    static_assert(std::is_move_constructible<T_کلید>::قدر, "'T_Key' must be move constructible");
    static_assert(std::is_destructible<T_کلید>::قدر, "'T_Key' must be destructible");
    static_assert(std::is_move_constructible<T_Val>::قدر, "'T_Val' must be move constructible");
    static_assert(std::is_destructible<T_Val>::قدر, "'T_Val' must be destructible");

عوامی:
    // TYPES
    using قدر_2 = T_Val;  // What a key maps to

    // CONSTRUCTORS
    V3HashMap() = طےشدہ;
    V3HashMap(T_Hash hash, T_Equal equal)
        : Super{std::move(hash), std::move(equal)} {}
};

#اگر_ختم  // Guard
