// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Tests for V3HashTable.h
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

#অন্তর্ভাব "V3HashTable.h"

#অন্তর্ভাব "V3Error.h"

#অন্তর্ভাব <array>
#অন্তর্ভাব <functional>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <string>
#অন্তর্ভাব <type_traits>
#অন্তর্ভাব <utility>

নামাকাশ V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr আকার_t maxLoad(আকার_t capacity) { ফেরত capacity * LOAD_FACTOR_অঙ্ক / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr আকার_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

শূন্য test_মান_চাবিগুলি() {
    কাঠামো মান_2 final {
        আকার_t m_hash = 0;  // Hash of this entry
        আকার_t m_id = 0;  // Id of this entry
    };

    কাঠামো Hash final {
        আকার_t operator()(ধ্রুবক মান_2& মান) ধ্রুবক { ফেরত operator()(মান.m_hash, 0); }
        আকার_t operator()(আকার_t hash, আকার_t) ধ্রুবক { ফেরত hash; }
    };

    কাঠামো Equal final {
        যৌক্তিক operator()(ধ্রুবক মান_2& a, ধ্রুবক মান_2& b) ধ্রুবক {
            ফেরত operator()(a, b.m_hash, b.m_id);
        }
        যৌক্তিক operator()(ধ্রুবক মান_2& মান, আকার_t hash, আকার_t id) ধ্রুবক {
            ফেরত মান.m_hash == hash && মান.m_id == id;
        }
    };

    using Set = V3HashSet<মান_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ধ্রুবক মান_2 মান{1, 0};
        ধ্রুবক মান_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), সত্য);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // So iterates nothing
        UASSERT_SELFTEST(set.find(মান) == set.end(), সত্য);  // And finds nothing
        ধ্রুবক std::pair<Set::iterator, যৌক্তিক> added = set.ঢোকাও(মান);
        UASSERT_SELFTEST(added.second, সত্য);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, মান.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &মান, সত্য);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.আকার_2(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), মিথ্যা);  // So the set is no longer empty
        {
            ধ্রুবক Set::iterator it = set.find(মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            ধ্রুবক Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            ধ্রুবক Set::iterator it = set.find(1, আকার_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, আকার_t{1}) == set.end(), সত্য);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, আকার_t{0}) == set.end(), সত্য);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ধ্রুবক std::pair<Set::iterator, যৌক্তিক> again = set.ঢোকাও(equal);
        UASSERT_SELFTEST(again.second, মিথ্যা);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.আকার_2(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ধ্রুবক Set::iterator it = set.find(মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty again
        UASSERT_SELFTEST(set.find(মান) == set.end(), সত্য);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(মান), মিথ্যা);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(মান), মিথ্যা);  // Nothing to erase, so says so
        set.ঢোকাও(মান);
        UASSERT_SELFTEST(set.erase(equal), সত্য);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), সত্য);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.ঢোকাও(মান);
        UASSERT_SELFTEST(set.erase(1, আকার_t{0}), সত্য);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr আকার_t N = GROWS_TWICE;
        Set set;
        জন্য (আকার_t i = 0; i < N; ++i) {
            ধ্রুবক std::pair<Set::iterator, যৌক্তিক> added = set.ঢোকাও(মান_2{7, i});
            UASSERT_SELFTEST(added.second, সত্য);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.আকার_2(), N);  // All of them, in the one probe run
        জন্য (আকার_t i = 0; i < N; ++i) {
            ধ্রুবক Set::iterator it = set.find(আকার_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<যৌক্তিক, N> seen{};
        আকার_t n = 0;
        জন্য (ধ্রুবক মান_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, আকার_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], মিথ্যা);  // Each of them exactly once
            seen[entry.m_id] = সত্য;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

শূন্য test_Pointer_চাবিগুলি() {
    কাঠামো মান_2 final {
        আকার_t m_hash = 0;  // Hash of this entry
        আকার_t m_id = 0;  // Id of this entry
    };

    কাঠামো Hash final {
        আকার_t operator()(ধ্রুবক মান_2* valuep) ধ্রুবক { ফেরত operator()(valuep->m_hash, 0); }
        আকার_t operator()(আকার_t hash, আকার_t) ধ্রুবক { ফেরত hash; }
    };

    কাঠামো Equal final {
        যৌক্তিক operator()(ধ্রুবক মান_2* ap, ধ্রুবক মান_2* bp) ধ্রুবক {
            ফেরত operator()(ap, bp->m_hash, bp->m_id);
        }
        যৌক্তিক operator()(ধ্রুবক মান_2* valuep, আকার_t hash, আকার_t id) ধ্রুবক {
            ফেরত valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        মান_2 মান{1, 0};
        মান_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), সত্য);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সত্য);  // And finds nothing
        ধ্রুবক std::pair<Set::iterator, যৌক্তিক> added = set.ঢোকাও(&মান);
        UASSERT_SELFTEST(added.second, সত্য);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &মান);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.আকার_2(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), মিথ্যা);  // So the set is no longer empty
        {
            ধ্রুবক Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Found by itself
            UASSERT_SELFTEST(*it, &মান);  // At the entry held
        }
        {
            ধ্রুবক Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by an equal entry
            UASSERT_SELFTEST(*it, &মান);  // At that same entry
        }
        {
            ধ্রুবক Set::iterator it = set.find(1, আকার_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by its parts
            UASSERT_SELFTEST(*it, &মান);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, আকার_t{1}) == set.end(), সত্য);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, আকার_t{0}) == set.end(), সত্য);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ধ্রুবক std::pair<Set::iterator, যৌক্তিক> again = set.ঢোকাও(&equal);
        UASSERT_SELFTEST(again.second, মিথ্যা);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &মান);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.আকার_2(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ধ্রুবক Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &মান);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, মিথ্যা);  // Not the one asked for
        }
        {
            ধ্রুবক Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The entry is still there
            UASSERT_SELFTEST(*it, &মান);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ধ্রুবক Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            UASSERT_SELFTEST(*it, &মান);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty again
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সত্য);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&মান), মিথ্যা);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), সত্য);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&মান), মিথ্যা);  // Nothing to erase, so says so
        set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.erase(&equal), সত্য);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), সত্য);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সত্য);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.erase(1, আকার_t{0}), সত্য);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    জন্য (আকার_t erase = 0; erase < 4; ++erase) {
        জন্য (ধ্রুবক আকার_t hash : {আকার_t{0}, আকার_t{7}, ~আকার_t{0}}) {  // Wraps too
            std::array<মান_2, 4> মানসমূহ{মান_2{hash, 0}, মান_2{hash, 1},  //
                                        মান_2{hash, 2}, মান_2{hash, 3}};
            Set set;
            জন্য (মান_2& মান : মানসমূহ) set.ঢোকাও(&মান);
            UASSERT_SELFTEST(set.আকার_2(), 4);  // All distinct, so all added
            {
                ধ্রুবক Set::iterator it = set.find(&মানসমূহ[erase]);
                UASSERT_SELFTEST(it != set.end(), সত্য);  // The one to erase is present
                UASSERT_SELFTEST(*it, &মানসমূহ[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.আকার_2(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&মানসমূহ[erase]) == set.end(), সত্য);  // That one
            জন্য (আকার_t i = 0; i < মানসমূহ.আকার_2(); ++i) {
                যদি (i == erase) চালু;
                ধ্রুবক Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), সত্য);  // The others are all there
                UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    জন্য (ধ্রুবক আকার_t hash : {আকার_t{0}, আকার_t{7}, ~আকার_t{0}}) {
        std::array<মান_2, GROWS_TWICE> many{};
        Set set;
        জন্য (আকার_t i = 0; i < many.আকার_2(); ++i) {
            many[i] = মান_2{hash, i};
            set.ঢোকাও(&many[i]);
        }
        UASSERT_SELFTEST(set.আকার_2(), many.আকার_2());  // All of them were added
        জন্য (আকার_t i = 0; i < many.আকার_2(); ++i) {
            ধ্রুবক Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        মান_2 মান{1, 0};
        মান_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.ঢোকাও(&মান);
        set.ঢোকাও(&other);
        UASSERT_SELFTEST(set.আকার_2(), 2);  // Both kept, despite the collision
        {
            ধ্রুবক Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The first of the run is there
            UASSERT_SELFTEST(*it, &মান);  // And is that entry
        }
        {
            ধ্রুবক Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ধ্রুবক Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            UASSERT_SELFTEST(*it, &মান);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.আকার_2(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সত্য);  // Namely that one
        {
            ধ্রুবক Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    জন্য (ধ্রুবক যৌক্তিক doReserve : {মিথ্যা, সত্য}) {
        std::array<মান_2, 8> মানসমূহ{};
        Set set;
        যদি (doReserve) set.reserve(মানসমূহ.আকার_2());
        জন্য (আকার_t i = 0; i < মানসমূহ.আকার_2(); ++i) {
            মানসমূহ[i] = মান_2{static_cast<আকার_t>(i * 1234567), i};
            set.ঢোকাও(&মানসমূহ[i]);
        }
        জন্য (মান_2& মান : মানসমূহ) {
            ধ্রুবক Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Each entry survives
            UASSERT_SELFTEST(*it, &মান);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.আকার_2(), মানসমূহ.আকার_2());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr আকার_t N = GROWS_TWICE;
        std::array<মান_2, N> মানসমূহ{};
        Set set;
        জন্য (আকার_t i = 0; i < N; ++i) {
            মানসমূহ[i] = মান_2{7, i};  // All of them hash the same
            ধ্রুবক std::pair<Set::iterator, যৌক্তিক> pair
                = set.ঢোকাও_Lazy(আকার_t{7}, i, [&]() -> মান_2* { ফেরত &মানসমূহ[i]; });
            UASSERT_SELFTEST(pair.second, সত্য);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &মানসমূহ[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.আকার_2(), N);  // All of them were added
        // They are all present now, so nothing is created
        জন্য (আকার_t i = 0; i < N; ++i) {
            যৌক্তিক created = মিথ্যা;
            ধ্রুবক std::pair<Set::iterator, যৌক্তিক> pair
                = set.ঢোকাও_Lazy(আকার_t{7}, i, [&]() -> মান_2* {
                      created = সত্য;  // LCOV_EXCL_START
                      ফেরত &মানসমূহ[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, মিথ্যা);  // The factory was never called
            UASSERT_SELFTEST(pair.second, মিথ্যা);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &মানসমূহ[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.আকার_2(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<মান_2, 20> মানসমূহ{};
        std::array<যৌক্তিক, 20> seen{};
        Set set;
        জন্য (আকার_t i = 0; i < মানসমূহ.আকার_2(); ++i) {
            মানসমূহ[i] = মান_2{i / 2, i};  // Pairs of them collide
            set.ঢোকাও(&মানসমূহ[i]);
        }
        আকার_t n = 0;
        জন্য (মান_2* ধ্রুবক valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.আকার_2(), সত্য);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], মিথ্যা);  // Each of them exactly once
            seen[valuep->m_id] = সত্য;
            ++n;
        }
        UASSERT_SELFTEST(n, মানসমূহ.আকার_2());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

শূন্য testEntryLifetime() {
    আকার_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    শ্রেণী NoDefault final {
        আকার_t* m_alivep;  // Where the live entries are counted
        আকার_t m_hash;  // Hash of this entry
        আকার_t m_id;  // Entries with equal ids are equal

    সর্বজনীন:
        NoDefault(আকার_t* alivep, আকার_t hash, আকার_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ধ্রুবক NoDefault& that)
            : m_alivep{that.m_alivep}
            , m_hash{that.m_hash}
            , m_id{that.m_id} {
            ++*m_alivep;
        }
        NoDefault(NoDefault&& that)
            : m_alivep{that.m_alivep}
            , m_hash{that.m_hash}
            , m_id{that.m_id} {
            ++*m_alivep;
        }
        ~NoDefault() { --*m_alivep; }
        NoDefault& operator=(ধ্রুবক NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        আকার_t hash() ধ্রুবক { ফেরত m_hash; }
        আকার_t id() ধ্রুবক { ফেরত m_id; }
        যৌক্তিক operator==(ধ্রুবক NoDefault& that) ধ্রুবক { ফেরত m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    কাঠামো Hash final {
        আকার_t operator()(ধ্রুবক NoDefault& মান) ধ্রুবক { ফেরত মান.hash(); }
        আকার_t operator()(আকার_t hash, আকার_t) ধ্রুবক { ফেরত hash; }
    };

    কাঠামো Equal final {
        যৌক্তিক operator()(ধ্রুবক NoDefault& a, ধ্রুবক NoDefault& b) ধ্রুবক {
            ফেরত operator()(a, b.hash(), b.id());
        }
        যৌক্তিক operator()(ধ্রুবক NoDefault& a, আকার_t, আকার_t id) ধ্রুবক { ফেরত a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr আকার_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        জন্য (আকার_t i = 0; i < N; ++i) {
            ধ্রুবক std::pair<Set::iterator, যৌক্তিক> added = set.ঢোকাও(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, সত্য);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.আকার_2(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        জন্য (আকার_t i = 0; i < N; ++i) {
            ধ্রুবক Set::iterator it = set.find(আকার_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ধ্রুবক std::pair<Set::iterator, যৌক্তিক> dup = set.ঢোকাও(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, মিথ্যা);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.আকার_2(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ধ্রুবক Set::iterator it = set.find(আকার_t{7}, আকার_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            UASSERT_SELFTEST(it->id(), আকার_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.আকার_2(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(আকার_t{7}, আকার_t{0}), মিথ্যা);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        জন্য (আকার_t i = 1; i < N; ++i) {
            {
                ধ্রুবক Set::iterator it = set.find(আকার_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), সত্য);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.আকার_2(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            জন্য (আকার_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(আকার_t{7}, j), সত্য);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        জন্য (আকার_t i = 0; i < N; ++i) set.ঢোকাও(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.আকার_2(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), সত্য);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(আকার_t{7}, আকার_t{0}), মিথ্যা);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), সত্য);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        জন্য (আকার_t i = 0; i < N; ++i) set.ঢোকাও(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), সত্য);  // Still empty
        set.ঢোকাও(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.আকার_2(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

শূন্য testMove() {
    আকার_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    কাঠামো Counted final {
        আকার_t* m_alivep;  // Where the live entries are counted
        আকার_t m_id;  // Entries with equal ids are equal

        Counted(আকার_t* alivep, আকার_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ধ্রুবক Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(ধ্রুবক Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    কাঠামো Hash final {
        আকার_t operator()(ধ্রুবক Counted& entry) ধ্রুবক { ফেরত operator()(entry.m_id); }
        আকার_t operator()(আকার_t id) ধ্রুবক { ফেরত id; }
    };

    কাঠামো Equal final {
        যৌক্তিক operator()(ধ্রুবক Counted& a, ধ্রুবক Counted& b) ধ্রুবক { ফেরত operator()(a, b.m_id); }
        যৌক্তিক operator()(ধ্রুবক Counted& a, আকার_t id) ধ্রুবক { ফেরত a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr আকার_t N = GROWS_TWICE;
    {
        Set set;
        জন্য (আকার_t i = 0; i < N; ++i) set.ঢোকাও(Counted{&alive, i});
        UASSERT_SELFTEST(set.আকার_2(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.আকার_2(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), সত্য);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // So it iterates nothing
        জন্য (আকার_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), সত্য);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), মিথ্যা);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.ঢোকাও(Counted{&alive, N}).second, সত্য);  // It took an entry
        UASSERT_SELFTEST(set.আকার_2(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), মিথ্যা);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.আকার_2(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), মিথ্যা);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), সত্য);  // And the source is empty again
        জন্য (আকার_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), সত্য);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

শূন্য testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    শ্রেণী MoveOnly final {
        আকার_t m_id;  // Entries with equal ids are equal

    সর্বজনীন:
        explicit MoveOnly(আকার_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = পূর্বনির্ধারিত;
        MoveOnly(ধ্রুবক MoveOnly&) = delete;
        MoveOnly& operator=(ধ্রুবক MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = পূর্বনির্ধারিত;

        আকার_t id() ধ্রুবক { ফেরত m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::মান,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::মান,
                  "SelfTest: 'MoveOnly' must be move constructible");

    কাঠামো Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        আকার_t operator()(ধ্রুবক MoveOnly& মান) ধ্রুবক { ফেরত operator()(মান.id()); }
        আকার_t operator()(আকার_t) ধ্রুবক { ফেরত 7; }
    };

    কাঠামো Equal final {
        যৌক্তিক operator()(ধ্রুবক MoveOnly& a, ধ্রুবক MoveOnly& b) ধ্রুবক {
            ফেরত operator()(a, b.id());
        }
        যৌক্তিক operator()(ধ্রুবক MoveOnly& a, আকার_t id) ধ্রুবক { ফেরত a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr আকার_t N = GROWS_TWICE;
    Set set;
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক std::pair<Set::iterator, যৌক্তিক> added
            = set.ঢোকাও_Lazy(i, [i] { ফেরত MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, সত্য);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.আকার_2(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    জন্য (আকার_t i = 0; i < N; i += 2) {
        ধ্রুবক Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.আকার_2(), N / 2);  // Half of them are gone
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক যৌক্তিক erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

শূন্য testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    শ্রেণী Hash final {
        আকার_t m_mask;  // Only these bits of an entry matter

    সর্বজনীন:
        explicit Hash(আকার_t mask)
            : m_mask{mask} {}
        আকার_t operator()(আকার_t মান) ধ্রুবক { ফেরত মান & m_mask; }
    };

    শ্রেণী Equal final {
        আকার_t m_mask;  // Only these bits of an entry matter

    সর্বজনীন:
        explicit Equal(আকার_t mask)
            : m_mask{mask} {}
        যৌক্তিক operator()(আকার_t a, আকার_t b) ধ্রুবক { ফেরত (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::মান,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::মান,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<আকার_t, Hash, Equal> set{Hash{3}, Equal{3}};
    জন্য (আকার_t i = 0; i < 16; ++i) set.ঢোকাও(i);
    UASSERT_SELFTEST(set.আকার_2(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    জন্য (আকার_t i = 0; i < 16; ++i) {
        ধ্রুবক V3HashSet<আকার_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

শূন্য testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    কাঠামো মান_2 final {
        আকার_t m_hash = 0;  // Hash of this entry
        আকার_t m_id = 0;  // Entries with equal ids are equal
    };

    কাঠামো Hash final {
        আকার_t operator()(ধ্রুবক মান_2* valuep) ধ্রুবক { ফেরত operator()(valuep->m_hash, 0); }
        আকার_t operator()(আকার_t hash, আকার_t) ধ্রুবক { ফেরত hash; }
    };

    কাঠামো Equal final {
        যৌক্তিক operator()(ধ্রুবক মান_2* ap, ধ্রুবক মান_2* bp) ধ্রুবক {
            ফেরত operator()(ap, bp->m_hash, bp->m_id);
        }
        যৌক্তিক operator()(ধ্রুবক মান_2* valuep, আকার_t hash, আকার_t id) ধ্রুবক {
            ফেরত valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    জন্য (ধ্রুবক আকার_t h : {আকার_t{0}, আকার_t{5}, ~আকার_t{0}}) {  // Wraps too
        std::array<মান_2, 4> মানসমূহ{মান_2{h, 0}, মান_2{h + 1, 1},  //
                                    মান_2{h, 2}, মান_2{h + 1, 3}};
        Set set;
        জন্য (মান_2& মান : মানসমূহ) set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.আকার_2(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ধ্রুবক Set::iterator it = set.find(h, আকার_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.আকার_2(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, আকার_t{0}), মিথ্যা);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        জন্য (আকার_t i = 1; i < মানসমূহ.আকার_2(); ++i) {
            ধ্রুবক Set::iterator it = set.find(মানসমূহ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ধ্রুবক Set::iterator it = set.find(h + 1, আকার_t{1});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.আকার_2(), 2);  // Two are left
        জন্য (আকার_t i = 2; i < মানসমূহ.আকার_2(); ++i) {
            ধ্রুবক Set::iterator it = set.find(মানসমূহ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Both still reachable
            UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<যৌক্তিক, 4> seen{};
        জন্য (ধ্রুবক মান_2* ধ্রুবক valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], মিথ্যা);  // Each entry once
            seen[valuep->m_id] = সত্য;
        }
        UASSERT_SELFTEST(seen[0], মিথ্যা);  // Erased first
        UASSERT_SELFTEST(seen[1], মিথ্যা);  // Erased second
        UASSERT_SELFTEST(seen[2], সত্য);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], সত্য);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

শূন্য testReferenceStability() {
    কাঠামো মান_2 final {
        আকার_t m_hash = 0;  // Hash of this entry
        আকার_t m_id = 0;  // Id of this entry
    };

    কাঠামো Hash final {
        আকার_t operator()(ধ্রুবক মান_2* valuep) ধ্রুবক { ফেরত operator()(valuep->m_hash, 0); }
        আকার_t operator()(আকার_t hash, আকার_t) ধ্রুবক { ফেরত hash; }
    };

    কাঠামো Equal final {
        যৌক্তিক operator()(ধ্রুবক মান_2* ap, ধ্রুবক মান_2* bp) ধ্রুবক {
            ফেরত operator()(ap, bp->m_hash, bp->m_id);
        }
        যৌক্তিক operator()(ধ্রুবক মান_2* valuep, আকার_t hash, আকার_t id) ধ্রুবক {
            ফেরত valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(আকার_t{7}, আকার_t{0}), মিথ্যা);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), সত্য);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr আকার_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<মান_2, N> মানসমূহ{};
    std::array<মান_2* ধ্রুবক*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    জন্য (আকার_t i = 0; i < N; ++i) {
        মানসমূহ[i] = মান_2{7, i};
        ধ্রুবক std::pair<Set::iterator, যৌক্তিক> pair = set.ঢোকাও(&মানসমূহ[i]);
        UASSERT_SELFTEST(pair.second, সত্য);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক Set::iterator it = set.find(আকার_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক std::pair<Set::iterator, যৌক্তিক> pair = set.ঢোকাও(&মানসমূহ[i]);
        UASSERT_SELFTEST(pair.second, মিথ্যা);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.আকার_2(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.আকার_2(), N);  // Rehashing dropped nothing
    std::array<মান_2* ধ্রুবক*, N> grownpps{};  // Where each entry is after the growth
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক Set::iterator it = set.find(আকার_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.আকার_2(), N);  // The smaller request changed nothing
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক Set::iterator it = set.find(আকার_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
কাঠামো ClusteredHash final {
    আকার_t operator()(আকার_t মান) ধ্রুবক { ফেরত মান & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
কাঠামো SpreadHash final {
    আকার_t operator()(আকার_t মান) ধ্রুবক {
        ফেরত static_cast<আকার_t>(মান * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
শূন্য testSetAgainstModel() {
    কাঠামো Equal final {
        যৌক্তিক operator()(আকার_t a, আকার_t b) ধ্রুবক { ফেরত a == b; }
    };

    using Set = V3HashSet<আকার_t, T_Hash, Equal>;

    constexpr আকার_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr আকার_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ধ্রুবক auto পরবর্তী_Rand = [&state]() -> আকার_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ফেরত static_cast<আকার_t>(state >> 32);
    };

    Set set;
    std::set<আকার_t> model;

    জন্য (আকার_t step = 0; step < STEPS; ++step) {
        ধ্রুবক আকার_t id = পরবর্তী_Rand() % UNIVERSE;
        নির্বাচন (পরবর্তী_Rand() % 4) {
        ক্ষেত্র 0: {  // Insert a copy
            ধ্রুবক std::pair<typename Set::iterator, যৌক্তিক> pair = set.ঢোকাও(id);
            UASSERT_SELFTEST(pair.second, model.ঢোকাও(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            বিরতি;
        }
        ক্ষেত্র 1: {  // Insert lazily, which must create only on a miss
            যৌক্তিক created = মিথ্যা;
            ধ্রুবক std::pair<typename Set::iterator, যৌক্তিক> pair
                = set.ঢোকাও_Lazy(id, [&]() -> আকার_t {
                      created = সত্য;
                      ফেরত id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ঢোকাও(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            বিরতি;
        }
        ক্ষেত্র 2: {  // Erase, if present
            ধ্রুবক typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.গুনতি(id) != 0);  // As the model
            যদি (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            বিরতি;
        }
        পূর্বনির্ধারিত: {  // Look up only
            ধ্রুবক typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.গুনতি(id) != 0);  // As the model
            যদি (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            বিরতি;
        }
        }
        UASSERT_SELFTEST(set.আকার_2(), model.আকার_2());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    জন্য (আকার_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.গুনতি(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<যৌক্তিক, UNIVERSE> seen{};
    আকার_t n = 0;
    জন্য (ধ্রুবক আকার_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, সত্য);  // Nothing out of thin air
        UASSERT_SELFTEST(model.গুনতি(entry) != 0, সত্য);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], মিথ্যা);  // Each entry once
        seen[entry] = সত্য;
        ++n;
    }
    UASSERT_SELFTEST(n, model.আকার_2());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

শূন্য testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, পূর্ণসংখ্যা>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, পূর্ণসংখ্যা>>::মান,
                  "SelfTest: a map entry must be a plain pair");

    constexpr আকার_t N = GROWS_TWICE;
    Map map;
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক std::string চাবি = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ধ্রুবক std::pair<Map::iterator, যৌক্তিক> pair
            = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry { ফেরত {চাবি, static_cast<পূর্ণসংখ্যা>(i)}; });
        UASSERT_SELFTEST(pair.second, সত্য);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.আকার_2(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সত্য);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণসংখ্যা>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক std::string চাবি = "key" + std::to_string(i);
        map.erase(map.find(চাবি));
        UASSERT_SELFTEST(map.ঢোকাও({চাবি, static_cast<পূর্ণসংখ্যা>(i) + 100}).second, সত্য);  // Anew
    }
    UASSERT_SELFTEST(map.আকার_2(), N);  // The same keys, so the map has not grown
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সত্য);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণসংখ্যা>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    জন্য (আকার_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).মান() += 100;
    UASSERT_SELFTEST(map.আকার_2(), N);  // The map is unchanged otherwise
    জন্য (আকার_t i = 0; i < N; ++i) {
        ধ্রুবক Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সত্য);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণসংখ্যা>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    জন্য (আকার_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).মান() -= 100;

    // Adding a key that is present changes nothing
    {
        ধ্রুবক std::string চাবি = "key0";
        যৌক্তিক created = মিথ্যা;
        ধ্রুবক std::pair<Map::iterator, যৌক্তিক> pair = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry {
            created = সত্য;  // LCOV_EXCL_START
            ফেরত {চাবি, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, মিথ্যা);  // The factory was never called
        UASSERT_SELFTEST(pair.second, মিথ্যা);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.আকার_2(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ধ্রুবক std::pair<Map::iterator, যৌক্তিক> added = map.ঢোকাও({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, সত্য);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        ধ্রুবক std::pair<Map::iterator, যৌক্তিক> again = map.ঢোকাও({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, মিথ্যা);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.আকার_2(), N + 1);  // Just the one entry was added
    {
        ধ্রুবক Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), সত্য);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.আকার_2(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), মিথ্যা);  // No such key
    UASSERT_SELFTEST(map.আকার_2(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), সত্য);  // That key is present
    UASSERT_SELFTEST(map.আকার_2(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), মিথ্যা);  // Namely that one
    জন্য (আকার_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), সত্য);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
শূন্য testMapAgainstModel() {
    কাঠামো Equal final {
        যৌক্তিক operator()(আকার_t a, আকার_t b) ধ্রুবক { ফেরত a == b; }
    };

    using Map = V3HashMap<আকার_t, আকার_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr আকার_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr আকার_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ধ্রুবক auto পরবর্তী_Rand = [&state]() -> আকার_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ফেরত static_cast<আকার_t>(state >> 32);
    };

    Map map;
    std::map<আকার_t, আকার_t> model;

    জন্য (আকার_t step = 0; step < STEPS; ++step) {
        ধ্রুবক আকার_t চাবি = পরবর্তী_Rand() % UNIVERSE;
        ধ্রুবক আকার_t val = step;  // Distinct every step, so a stale value is visible
        নির্বাচন (পরবর্তী_Rand() % 5) {
        ক্ষেত্র 0: {  // Insert a whole entry
            ধ্রুবক std::pair<typename Map::iterator, যৌক্তিক> pair = map.ঢোকাও({চাবি, val});
            UASSERT_SELFTEST(pair.second, model.ঢোকাও({চাবি, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, চাবি);  // At the key asked for
            বিরতি;
        }
        ক্ষেত্র 1: {  // Insert lazily, which must create only on a miss
            যৌক্তিক created = মিথ্যা;
            ধ্রুবক std::pair<typename Map::iterator, যৌক্তিক> pair
                = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry {
                      created = সত্য;
                      ফেরত {চাবি, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ঢোকাও({চাবি, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, চাবি);  // At the key asked for
            বিরতি;
        }
        ক্ষেত্র 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(চাবি), model.erase(চাবি) != 0);  // As model
            বিরতি;
        }
        ক্ষেত্র 3: {  // Change a value, which is erasing the entry and inserting it again
            ধ্রুবক typename Map::iterator it = map.find(চাবি);
            যদি (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.ঢোকাও({চাবি, val}).second, সত্য);  // The key is free
                model[চাবি] = val;
            }
            বিরতি;
        }
        পূর্বনির্ধারিত: {  // Look up only
            ধ্রুবক typename Map::iterator it = map.find(চাবি);
            UASSERT_SELFTEST(it != map.end(), model.গুনতি(চাবি) != 0);  // As the model
            যদি (it != map.end()) {
                UASSERT_SELFTEST(it->first, চাবি);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(চাবি));  // With its value
            }
            বিরতি;
        }
        }
        UASSERT_SELFTEST(map.আকার_2(), model.আকার_2());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    জন্য (আকার_t চাবি = 0; চাবি < UNIVERSE; ++চাবি) {
        ধ্রুবক typename Map::iterator it = map.find(চাবি);
        UASSERT_SELFTEST(it != map.end(), model.গুনতি(চাবি) != 0);  // Present iff modelled
        যদি (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(চাবি));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<যৌক্তিক, UNIVERSE> seen{};
    আকার_t n = 0;
    জন্য (ধ্রুবক Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, সত্য);  // Nothing out of thin air
        UASSERT_SELFTEST(model.গুনতি(entry.first) != 0, সত্য);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], মিথ্যা);  // Each entry once
        seen[entry.first] = সত্য;
        ++n;
    }
    UASSERT_SELFTEST(n, model.আকার_2());  // And all of them
}

শূন্য selfTest() {
    test_মান_চাবিগুলি();
    test_Pointer_চাবিগুলি();
    testEntryLifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    testBackwardShiftDeletion();
    testReferenceStability();
    testSetAgainstModel<ClusteredHash>();
    testSetAgainstModel<SpreadHash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<SpreadHash>();
}

}  // namespace V3HashTableInternals
