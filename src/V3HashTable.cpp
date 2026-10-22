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
constexpr size_t maxLoad(size_t capacity) { প্রত্যাগম capacity * LOAD_FACTOR_অঙ্ক / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr size_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

শূন্য test_মান_চাবিগুলি() {
    সংরচনা মান_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    সংরচনা Hash final {
        size_t operator()(ধ্রুব মান_2& মান) ধ্রুব { প্রত্যাগম operator()(মান.m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুব { প্রত্যাগম hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুব মান_2& a, ধ্রুব মান_2& b) ধ্রুব {
            প্রত্যাগম operator()(a, b.m_hash, b.m_id);
        }
        তার্কিক operator()(ধ্রুব মান_2& মান, size_t hash, size_t id) ধ্রুব {
            প্রত্যাগম মান.m_hash == hash && মান.m_id == id;
        }
    };

    using Set = V3HashSet<মান_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ধ্রুব মান_2 মান{1, 0};
        ধ্রুব মান_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), সত্য);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // So iterates nothing
        UASSERT_SELFTEST(set.find(মান) == set.end(), সত্য);  // And finds nothing
        ধ্রুব std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(মান);
        UASSERT_SELFTEST(added.second, সত্য);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, মান.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &মান, সত্য);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), অসত্য);  // So the set is no longer empty
        {
            ধ্রুব Set::iterator it = set.find(মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            ধ্রুব Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            ধ্রুব Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), সত্য);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), সত্য);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ধ্রুব std::pair<Set::iterator, তার্কিক> again = set.ঢোকাও(equal);
        UASSERT_SELFTEST(again.second, অসত্য);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ধ্রুব Set::iterator it = set.find(মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty again
        UASSERT_SELFTEST(set.find(মান) == set.end(), সত্য);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(মান), অসত্য);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(মান), অসত্য);  // Nothing to erase, so says so
        set.ঢোকাও(মান);
        UASSERT_SELFTEST(set.erase(equal), সত্য);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), সত্য);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.ঢোকাও(মান);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), সত্য);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        প্রতি (size_t i = 0; i < N; ++i) {
            ধ্রুব std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(মান_2{7, i});
            UASSERT_SELFTEST(added.second, সত্য);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        প্রতি (size_t i = 0; i < N; ++i) {
            ধ্রুব Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<তার্কিক, N> seen{};
        size_t n = 0;
        প্রতি (ধ্রুব মান_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], অসত্য);  // Each of them exactly once
            seen[entry.m_id] = সত্য;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

শূন্য test_Pointer_চাবিগুলি() {
    সংরচনা মান_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    সংরচনা Hash final {
        size_t operator()(ধ্রুব মান_2* valuep) ধ্রুব { প্রত্যাগম operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুব { প্রত্যাগম hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুব মান_2* ap, ধ্রুব মান_2* bp) ধ্রুব {
            প্রত্যাগম operator()(ap, bp->m_hash, bp->m_id);
        }
        তার্কিক operator()(ধ্রুব মান_2* valuep, size_t hash, size_t id) ধ্রুব {
            প্রত্যাগম valuep->m_hash == hash && valuep->m_id == id;
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
        ধ্রুব std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(&মান);
        UASSERT_SELFTEST(added.second, সত্য);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &মান);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), অসত্য);  // So the set is no longer empty
        {
            ধ্রুব Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Found by itself
            UASSERT_SELFTEST(*it, &মান);  // At the entry held
        }
        {
            ধ্রুব Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by an equal entry
            UASSERT_SELFTEST(*it, &মান);  // At that same entry
        }
        {
            ধ্রুব Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // And by its parts
            UASSERT_SELFTEST(*it, &মান);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), সত্য);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), সত্য);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ধ্রুব std::pair<Set::iterator, তার্কিক> again = set.ঢোকাও(&equal);
        UASSERT_SELFTEST(again.second, অসত্য);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &মান);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ধ্রুব Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &মান);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, অসত্য);  // Not the one asked for
        }
        {
            ধ্রুব Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The entry is still there
            UASSERT_SELFTEST(*it, &মান);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ধ্রুব Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            UASSERT_SELFTEST(*it, &মান);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty again
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সত্য);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&মান), অসত্য);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), সত্য);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&মান), অসত্য);  // Nothing to erase, so says so
        set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.erase(&equal), সত্য);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), সত্য);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সত্য);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), সত্য);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), সত্য);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    প্রতি (size_t erase = 0; erase < 4; ++erase) {
        প্রতি (ধ্রুব size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<মান_2, 4> মানসমূহ{মান_2{hash, 0}, মান_2{hash, 1},  //
                                        মান_2{hash, 2}, মান_2{hash, 3}};
            Set set;
            প্রতি (মান_2& মান : মানসমূহ) set.ঢোকাও(&মান);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                ধ্রুব Set::iterator it = set.find(&মানসমূহ[erase]);
                UASSERT_SELFTEST(it != set.end(), সত্য);  // The one to erase is present
                UASSERT_SELFTEST(*it, &মানসমূহ[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&মানসমূহ[erase]) == set.end(), সত্য);  // That one
            প্রতি (size_t i = 0; i < মানসমূহ.size(); ++i) {
                যদি (i == erase) অনুবর্ত;
                ধ্রুব Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), সত্য);  // The others are all there
                UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    প্রতি (ধ্রুব size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<মান_2, GROWS_TWICE> many{};
        Set set;
        প্রতি (size_t i = 0; i < many.size(); ++i) {
            many[i] = মান_2{hash, i};
            set.ঢোকাও(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        প্রতি (size_t i = 0; i < many.size(); ++i) {
            ধ্রুব Set::iterator it = set.find(hash, i);
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
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            ধ্রুব Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The first of the run is there
            UASSERT_SELFTEST(*it, &মান);  // And is that entry
        }
        {
            ধ্রুব Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ধ্রুব Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            UASSERT_SELFTEST(*it, &মান);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সত্য);  // Namely that one
        {
            ধ্রুব Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    প্রতি (ধ্রুব তার্কিক doReserve : {অসত্য, সত্য}) {
        std::array<মান_2, 8> মানসমূহ{};
        Set set;
        যদি (doReserve) set.reserve(মানসমূহ.size());
        প্রতি (size_t i = 0; i < মানসমূহ.size(); ++i) {
            মানসমূহ[i] = মান_2{static_cast<size_t>(i * 1234567), i};
            set.ঢোকাও(&মানসমূহ[i]);
        }
        প্রতি (মান_2& মান : মানসমূহ) {
            ধ্রুব Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Each entry survives
            UASSERT_SELFTEST(*it, &মান);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), মানসমূহ.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<মান_2, N> মানসমূহ{};
        Set set;
        প্রতি (size_t i = 0; i < N; ++i) {
            মানসমূহ[i] = মান_2{7, i};  // All of them hash the same
            ধ্রুব std::pair<Set::iterator, তার্কিক> pair
                = set.ঢোকাও_Lazy(size_t{7}, i, [&]() -> মান_2* { প্রত্যাগম &মানসমূহ[i]; });
            UASSERT_SELFTEST(pair.second, সত্য);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &মানসমূহ[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        প্রতি (size_t i = 0; i < N; ++i) {
            তার্কিক created = অসত্য;
            ধ্রুব std::pair<Set::iterator, তার্কিক> pair
                = set.ঢোকাও_Lazy(size_t{7}, i, [&]() -> মান_2* {
                      created = সত্য;  // LCOV_EXCL_START
                      প্রত্যাগম &মানসমূহ[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, অসত্য);  // The factory was never called
            UASSERT_SELFTEST(pair.second, অসত্য);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &মানসমূহ[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<মান_2, 20> মানসমূহ{};
        std::array<তার্কিক, 20> seen{};
        Set set;
        প্রতি (size_t i = 0; i < মানসমূহ.size(); ++i) {
            মানসমূহ[i] = মান_2{i / 2, i};  // Pairs of them collide
            set.ঢোকাও(&মানসমূহ[i]);
        }
        size_t n = 0;
        প্রতি (মান_2* ধ্রুব valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), সত্য);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], অসত্য);  // Each of them exactly once
            seen[valuep->m_id] = সত্য;
            ++n;
        }
        UASSERT_SELFTEST(n, মানসমূহ.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

শূন্য testEntryLifetime() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    বর্গ NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    সার্বজনিক:
        NoDefault(size_t* alivep, size_t hash, size_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ধ্রুব NoDefault& that)
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
        NoDefault& operator=(ধ্রুব NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() ধ্রুব { প্রত্যাগম m_hash; }
        size_t id() ধ্রুব { প্রত্যাগম m_id; }
        তার্কিক operator==(ধ্রুব NoDefault& that) ধ্রুব { প্রত্যাগম m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::মান,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    সংরচনা Hash final {
        size_t operator()(ধ্রুব NoDefault& মান) ধ্রুব { প্রত্যাগম মান.hash(); }
        size_t operator()(size_t hash, size_t) ধ্রুব { প্রত্যাগম hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুব NoDefault& a, ধ্রুব NoDefault& b) ধ্রুব {
            প্রত্যাগম operator()(a, b.hash(), b.id());
        }
        তার্কিক operator()(ধ্রুব NoDefault& a, size_t, size_t id) ধ্রুব { প্রত্যাগম a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        প্রতি (size_t i = 0; i < N; ++i) {
            ধ্রুব std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, সত্য);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        প্রতি (size_t i = 0; i < N; ++i) {
            ধ্রুব Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ধ্রুব std::pair<Set::iterator, তার্কিক> dup = set.ঢোকাও(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, অসত্য);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ধ্রুব Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), অসত্য);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        প্রতি (size_t i = 1; i < N; ++i) {
            {
                ধ্রুব Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), সত্য);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            প্রতি (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), সত্য);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        প্রতি (size_t i = 0; i < N; ++i) set.ঢোকাও(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), সত্য);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), অসত্য);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), সত্য);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        প্রতি (size_t i = 0; i < N; ++i) set.ঢোকাও(NoDefault{&alive, 7, i});
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
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

শূন্য testMove() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    সংরচনা Counted final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_id;  // Entries with equal ids are equal

        Counted(size_t* alivep, size_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ধ্রুব Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(ধ্রুব Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    সংরচনা Hash final {
        size_t operator()(ধ্রুব Counted& entry) ধ্রুব { প্রত্যাগম operator()(entry.m_id); }
        size_t operator()(size_t id) ধ্রুব { প্রত্যাগম id; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুব Counted& a, ধ্রুব Counted& b) ধ্রুব { প্রত্যাগম operator()(a, b.m_id); }
        তার্কিক operator()(ধ্রুব Counted& a, size_t id) ধ্রুব { প্রত্যাগম a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        প্রতি (size_t i = 0; i < N; ++i) set.ঢোকাও(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), সত্য);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // So it iterates nothing
        প্রতি (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), সত্য);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), অসত্য);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.ঢোকাও(Counted{&alive, N}).second, সত্য);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), অসত্য);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), অসত্য);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), সত্য);  // And the source is empty again
        প্রতি (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), সত্য);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

শূন্য testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    বর্গ MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    সার্বজনিক:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = স্বভাব;
        MoveOnly(ধ্রুব MoveOnly&) = delete;
        MoveOnly& operator=(ধ্রুব MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = স্বভাব;

        size_t id() ধ্রুব { প্রত্যাগম m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::মান,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::মান,
                  "SelfTest: 'MoveOnly' must be move constructible");

    সংরচনা Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(ধ্রুব MoveOnly& মান) ধ্রুব { প্রত্যাগম operator()(মান.id()); }
        size_t operator()(size_t) ধ্রুব { প্রত্যাগম 7; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুব MoveOnly& a, ধ্রুব MoveOnly& b) ধ্রুব {
            প্রত্যাগম operator()(a, b.id());
        }
        তার্কিক operator()(ধ্রুব MoveOnly& a, size_t id) ধ্রুব { প্রত্যাগম a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব std::pair<Set::iterator, তার্কিক> added
            = set.ঢোকাও_Lazy(i, [i] { প্রত্যাগম MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, সত্য);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    প্রতি (size_t i = 0; i < N; i += 2) {
        ধ্রুব Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব তার্কিক erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

শূন্য testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    বর্গ Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    সার্বজনিক:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t মান) ধ্রুব { প্রত্যাগম মান & m_mask; }
    };

    বর্গ Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    সার্বজনিক:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        তার্কিক operator()(size_t a, size_t b) ধ্রুব { প্রত্যাগম (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::মান,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::মান,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    প্রতি (size_t i = 0; i < 16; ++i) set.ঢোকাও(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    প্রতি (size_t i = 0; i < 16; ++i) {
        ধ্রুব V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

শূন্য testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    সংরচনা মান_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Entries with equal ids are equal
    };

    সংরচনা Hash final {
        size_t operator()(ধ্রুব মান_2* valuep) ধ্রুব { প্রত্যাগম operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুব { প্রত্যাগম hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুব মান_2* ap, ধ্রুব মান_2* bp) ধ্রুব {
            প্রত্যাগম operator()(ap, bp->m_hash, bp->m_id);
        }
        তার্কিক operator()(ধ্রুব মান_2* valuep, size_t hash, size_t id) ধ্রুব {
            প্রত্যাগম valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    প্রতি (ধ্রুব size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<মান_2, 4> মানসমূহ{মান_2{h, 0}, মান_2{h + 1, 1},  //
                                    মান_2{h, 2}, মান_2{h + 1, 3}};
        Set set;
        প্রতি (মান_2& মান : মানসমূহ) set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ধ্রুব Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), অসত্য);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        প্রতি (size_t i = 1; i < মানসমূহ.size(); ++i) {
            ধ্রুব Set::iterator it = set.find(মানসমূহ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ধ্রুব Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        প্রতি (size_t i = 2; i < মানসমূহ.size(); ++i) {
            ধ্রুব Set::iterator it = set.find(মানসমূহ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), সত্য);  // Both still reachable
            UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<তার্কিক, 4> seen{};
        প্রতি (ধ্রুব মান_2* ধ্রুব valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], অসত্য);  // Each entry once
            seen[valuep->m_id] = সত্য;
        }
        UASSERT_SELFTEST(seen[0], অসত্য);  // Erased first
        UASSERT_SELFTEST(seen[1], অসত্য);  // Erased second
        UASSERT_SELFTEST(seen[2], সত্য);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], সত্য);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

শূন্য testReferenceStability() {
    সংরচনা মান_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    সংরচনা Hash final {
        size_t operator()(ধ্রুব মান_2* valuep) ধ্রুব { প্রত্যাগম operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুব { প্রত্যাগম hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুব মান_2* ap, ধ্রুব মান_2* bp) ধ্রুব {
            প্রত্যাগম operator()(ap, bp->m_hash, bp->m_id);
        }
        তার্কিক operator()(ধ্রুব মান_2* valuep, size_t hash, size_t id) ধ্রুব {
            প্রত্যাগম valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), অসত্য);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), সত্য);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), সত্য);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<মান_2, N> মানসমূহ{};
    std::array<মান_2* ধ্রুব*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    প্রতি (size_t i = 0; i < N; ++i) {
        মানসমূহ[i] = মান_2{7, i};
        ধ্রুব std::pair<Set::iterator, তার্কিক> pair = set.ঢোকাও(&মানসমূহ[i]);
        UASSERT_SELFTEST(pair.second, সত্য);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব std::pair<Set::iterator, তার্কিক> pair = set.ঢোকাও(&মানসমূহ[i]);
        UASSERT_SELFTEST(pair.second, অসত্য);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<মান_2* ধ্রুব*, N> grownpps{};  // Where each entry is after the growth
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সত্য);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
সংরচনা ClusteredHash final {
    size_t operator()(size_t মান) ধ্রুব { প্রত্যাগম মান & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
সংরচনা SpreadHash final {
    size_t operator()(size_t মান) ধ্রুব {
        প্রত্যাগম static_cast<size_t>(মান * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
শূন্য testSetAgainstModel() {
    সংরচনা Equal final {
        তার্কিক operator()(size_t a, size_t b) ধ্রুব { প্রত্যাগম a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ধ্রুব auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        প্রত্যাগম static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    প্রতি (size_t step = 0; step < STEPS; ++step) {
        ধ্রুব size_t id = nextRand() % UNIVERSE;
        বিকল্প (nextRand() % 4) {
        পক্ষ 0: {  // Insert a copy
            ধ্রুব std::pair<typename Set::iterator, তার্কিক> pair = set.ঢোকাও(id);
            UASSERT_SELFTEST(pair.second, model.ঢোকাও(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            ভঙ্গ;
        }
        পক্ষ 1: {  // Insert lazily, which must create only on a miss
            তার্কিক created = অসত্য;
            ধ্রুব std::pair<typename Set::iterator, তার্কিক> pair
                = set.ঢোকাও_Lazy(id, [&]() -> size_t {
                      created = সত্য;
                      প্রত্যাগম id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ঢোকাও(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            ভঙ্গ;
        }
        পক্ষ 2: {  // Erase, if present
            ধ্রুব typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.গুনতি(id) != 0);  // As the model
            যদি (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            ভঙ্গ;
        }
        স্বভাব: {  // Look up only
            ধ্রুব typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.গুনতি(id) != 0);  // As the model
            যদি (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            ভঙ্গ;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    প্রতি (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.গুনতি(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<তার্কিক, UNIVERSE> seen{};
    size_t n = 0;
    প্রতি (ধ্রুব size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, সত্য);  // Nothing out of thin air
        UASSERT_SELFTEST(model.গুনতি(entry) != 0, সত্য);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], অসত্য);  // Each entry once
        seen[entry] = সত্য;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

শূন্য testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, পূর্ণাঙ্ক>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, পূর্ণাঙ্ক>>::মান,
                  "SelfTest: a map entry must be a plain pair");

    constexpr size_t N = GROWS_TWICE;
    Map map;
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব std::string চাবি = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ধ্রুব std::pair<Map::iterator, তার্কিক> pair
            = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry { প্রত্যাগম {চাবি, static_cast<পূর্ণাঙ্ক>(i)}; });
        UASSERT_SELFTEST(pair.second, সত্য);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সত্য);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণাঙ্ক>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব std::string চাবি = "key" + std::to_string(i);
        map.erase(map.find(চাবি));
        UASSERT_SELFTEST(map.ঢোকাও({চাবি, static_cast<পূর্ণাঙ্ক>(i) + 100}).second, সত্য);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সত্য);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণাঙ্ক>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    প্রতি (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).মান() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    প্রতি (size_t i = 0; i < N; ++i) {
        ধ্রুব Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সত্য);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণাঙ্ক>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    প্রতি (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).মান() -= 100;

    // Adding a key that is present changes nothing
    {
        ধ্রুব std::string চাবি = "key0";
        তার্কিক created = অসত্য;
        ধ্রুব std::pair<Map::iterator, তার্কিক> pair = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry {
            created = সত্য;  // LCOV_EXCL_START
            প্রত্যাগম {চাবি, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, অসত্য);  // The factory was never called
        UASSERT_SELFTEST(pair.second, অসত্য);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ধ্রুব std::pair<Map::iterator, তার্কিক> added = map.ঢোকাও({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, সত্য);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        ধ্রুব std::pair<Map::iterator, তার্কিক> again = map.ঢোকাও({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, অসত্য);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        ধ্রুব Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), সত্য);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), অসত্য);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), সত্য);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), অসত্য);  // Namely that one
    প্রতি (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), সত্য);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
শূন্য testMapAgainstModel() {
    সংরচনা Equal final {
        তার্কিক operator()(size_t a, size_t b) ধ্রুব { প্রত্যাগম a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ধ্রুব auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        প্রত্যাগম static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    প্রতি (size_t step = 0; step < STEPS; ++step) {
        ধ্রুব size_t চাবি = nextRand() % UNIVERSE;
        ধ্রুব size_t val = step;  // Distinct every step, so a stale value is visible
        বিকল্প (nextRand() % 5) {
        পক্ষ 0: {  // Insert a whole entry
            ধ্রুব std::pair<typename Map::iterator, তার্কিক> pair = map.ঢোকাও({চাবি, val});
            UASSERT_SELFTEST(pair.second, model.ঢোকাও({চাবি, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, চাবি);  // At the key asked for
            ভঙ্গ;
        }
        পক্ষ 1: {  // Insert lazily, which must create only on a miss
            তার্কিক created = অসত্য;
            ধ্রুব std::pair<typename Map::iterator, তার্কিক> pair
                = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry {
                      created = সত্য;
                      প্রত্যাগম {চাবি, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ঢোকাও({চাবি, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, চাবি);  // At the key asked for
            ভঙ্গ;
        }
        পক্ষ 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(চাবি), model.erase(চাবি) != 0);  // As model
            ভঙ্গ;
        }
        পক্ষ 3: {  // Change a value, which is erasing the entry and inserting it again
            ধ্রুব typename Map::iterator it = map.find(চাবি);
            যদি (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.ঢোকাও({চাবি, val}).second, সত্য);  // The key is free
                model[চাবি] = val;
            }
            ভঙ্গ;
        }
        স্বভাব: {  // Look up only
            ধ্রুব typename Map::iterator it = map.find(চাবি);
            UASSERT_SELFTEST(it != map.end(), model.গুনতি(চাবি) != 0);  // As the model
            যদি (it != map.end()) {
                UASSERT_SELFTEST(it->first, চাবি);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(চাবি));  // With its value
            }
            ভঙ্গ;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    প্রতি (size_t চাবি = 0; চাবি < UNIVERSE; ++চাবি) {
        ধ্রুব typename Map::iterator it = map.find(চাবি);
        UASSERT_SELFTEST(it != map.end(), model.গুনতি(চাবি) != 0);  // Present iff modelled
        যদি (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(চাবি));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<তার্কিক, UNIVERSE> seen{};
    size_t n = 0;
    প্রতি (ধ্রুব Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, সত্য);  // Nothing out of thin air
        UASSERT_SELFTEST(model.গুনতি(entry.first) != 0, সত্য);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], অসত্য);  // Each entry once
        seen[entry.first] = সত্য;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
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
