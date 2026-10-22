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

#অন্তর্ভাৱ "V3HashTable.h"

#অন্তর্ভাৱ "V3Error.h"

#অন্তর্ভাৱ <array>
#অন্তর্ভাৱ <functional>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <string>
#অন্তর্ভাৱ <type_traits>
#অন্তর্ভাৱ <utility>

নামাকাশ V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr size_t maxLoad(size_t capacity) { ঘূৰাই capacity * LOAD_FACTOR_অঙ্ক / LOAD_FACTOR_DEN; }

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
        size_t operator()(ধ্রুৱ মান_2& মান) ধ্রুৱ { ঘূৰাই operator()(মান.m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুৱ { ঘূৰাই hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুৱ মান_2& a, ধ্রুৱ মান_2& b) ধ্রুৱ {
            ঘূৰাই operator()(a, b.m_hash, b.m_id);
        }
        তার্কিক operator()(ধ্রুৱ মান_2& মান, size_t hash, size_t id) ধ্রুৱ {
            ঘূৰাই মান.m_hash == hash && মান.m_id == id;
        }
    };

    using Set = V3HashSet<মান_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ধ্রুৱ মান_2 মান{1, 0};
        ধ্রুৱ মান_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), সঁচা);  // So iterates nothing
        UASSERT_SELFTEST(set.find(মান) == set.end(), সঁচা);  // And finds nothing
        ধ্রুৱ std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(মান);
        UASSERT_SELFTEST(added.second, সঁচা);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, মান.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &মান, সঁচা);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), মিছা);  // So the set is no longer empty
        {
            ধ্রুৱ Set::iterator it = set.find(মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            ধ্রুৱ Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            ধ্রুৱ Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), সঁচা);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), সঁচা);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ধ্রুৱ std::pair<Set::iterator, তার্কিক> again = set.ঢোকাও(equal);
        UASSERT_SELFTEST(again.second, মিছা);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ধ্রুৱ Set::iterator it = set.find(মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Empty again
        UASSERT_SELFTEST(set.find(মান) == set.end(), সঁচা);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(মান), মিছা);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(মান), মিছা);  // Nothing to erase, so says so
        set.ঢোকাও(মান);
        UASSERT_SELFTEST(set.erase(equal), সঁচা);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.ঢোকাও(মান);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), সঁচা);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        বাবে (size_t i = 0; i < N; ++i) {
            ধ্রুৱ std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(মান_2{7, i});
            UASSERT_SELFTEST(added.second, সঁচা);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        বাবে (size_t i = 0; i < N; ++i) {
            ধ্রুৱ Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<তার্কিক, N> seen{};
        size_t n = 0;
        বাবে (ধ্রুৱ মান_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], মিছা);  // Each of them exactly once
            seen[entry.m_id] = সঁচা;
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
        size_t operator()(ধ্রুৱ মান_2* valuep) ধ্রুৱ { ঘূৰাই operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুৱ { ঘূৰাই hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুৱ মান_2* ap, ধ্রুৱ মান_2* bp) ধ্রুৱ {
            ঘূৰাই operator()(ap, bp->m_hash, bp->m_id);
        }
        তার্কিক operator()(ধ্রুৱ মান_2* valuep, size_t hash, size_t id) ধ্রুৱ {
            ঘূৰাই valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        মান_2 মান{1, 0};
        মান_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), সঁচা);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সঁচা);  // And finds nothing
        ধ্রুৱ std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(&মান);
        UASSERT_SELFTEST(added.second, সঁচা);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &মান);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), মিছা);  // So the set is no longer empty
        {
            ধ্রুৱ Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Found by itself
            UASSERT_SELFTEST(*it, &মান);  // At the entry held
        }
        {
            ধ্রুৱ Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // And by an equal entry
            UASSERT_SELFTEST(*it, &মান);  // At that same entry
        }
        {
            ধ্রুৱ Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // And by its parts
            UASSERT_SELFTEST(*it, &মান);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), সঁচা);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), সঁচা);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ধ্রুৱ std::pair<Set::iterator, তার্কিক> again = set.ঢোকাও(&equal);
        UASSERT_SELFTEST(again.second, মিছা);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &মান);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ধ্রুৱ Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &মান);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, মিছা);  // Not the one asked for
        }
        {
            ধ্রুৱ Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // The entry is still there
            UASSERT_SELFTEST(*it, &মান);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ধ্রুৱ Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Present before the erase
            UASSERT_SELFTEST(*it, &মান);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Empty again
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সঁচা);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&মান), মিছা);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), সঁচা);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&মান), মিছা);  // Nothing to erase, so says so
        set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.erase(&equal), সঁচা);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সঁচা);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), সঁচা);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    বাবে (size_t erase = 0; erase < 4; ++erase) {
        বাবে (ধ্রুৱ size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<মান_2, 4> মানসমূহ{মান_2{hash, 0}, মান_2{hash, 1},  //
                                        মান_2{hash, 2}, মান_2{hash, 3}};
            Set set;
            বাবে (মান_2& মান : মানসমূহ) set.ঢোকাও(&মান);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                ধ্রুৱ Set::iterator it = set.find(&মানসমূহ[erase]);
                UASSERT_SELFTEST(it != set.end(), সঁচা);  // The one to erase is present
                UASSERT_SELFTEST(*it, &মানসমূহ[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&মানসমূহ[erase]) == set.end(), সঁচা);  // That one
            বাবে (size_t i = 0; i < মানসমূহ.size(); ++i) {
                যদি (i == erase) অনুৱর্ত;
                ধ্রুৱ Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), সঁচা);  // The others are all there
                UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    বাবে (ধ্রুৱ size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<মান_2, GROWS_TWICE> many{};
        Set set;
        বাবে (size_t i = 0; i < many.size(); ++i) {
            many[i] = মান_2{hash, i};
            set.ঢোকাও(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        বাবে (size_t i = 0; i < many.size(); ++i) {
            ধ্রুৱ Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // And all survived the growth
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
            ধ্রুৱ Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // The first of the run is there
            UASSERT_SELFTEST(*it, &মান);  // And is that entry
        }
        {
            ধ্রুৱ Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ধ্রুৱ Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Present before the erase
            UASSERT_SELFTEST(*it, &মান);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&মান) == set.end(), সঁচা);  // Namely that one
        {
            ধ্রুৱ Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    বাবে (ধ্রুৱ তার্কিক doReserve : {মিছা, সঁচা}) {
        std::array<মান_2, 8> মানসমূহ{};
        Set set;
        যদি (doReserve) set.reserve(মানসমূহ.size());
        বাবে (size_t i = 0; i < মানসমূহ.size(); ++i) {
            মানসমূহ[i] = মান_2{static_cast<size_t>(i * 1234567), i};
            set.ঢোকাও(&মানসমূহ[i]);
        }
        বাবে (মান_2& মান : মানসমূহ) {
            ধ্রুৱ Set::iterator it = set.find(&মান);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Each entry survives
            UASSERT_SELFTEST(*it, &মান);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), মানসমূহ.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<মান_2, N> মানসমূহ{};
        Set set;
        বাবে (size_t i = 0; i < N; ++i) {
            মানসমূহ[i] = মান_2{7, i};  // All of them hash the same
            ধ্রুৱ std::pair<Set::iterator, তার্কিক> pair
                = set.ঢোকাও_Lazy(size_t{7}, i, [&]() -> মান_2* { ঘূৰাই &মানসমূহ[i]; });
            UASSERT_SELFTEST(pair.second, সঁচা);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &মানসমূহ[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        বাবে (size_t i = 0; i < N; ++i) {
            তার্কিক created = মিছা;
            ধ্রুৱ std::pair<Set::iterator, তার্কিক> pair
                = set.ঢোকাও_Lazy(size_t{7}, i, [&]() -> মান_2* {
                      created = সঁচা;  // LCOV_EXCL_START
                      ঘূৰাই &মানসমূহ[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, মিছা);  // The factory was never called
            UASSERT_SELFTEST(pair.second, মিছা);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &মানসমূহ[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<মান_2, 20> মানসমূহ{};
        std::array<তার্কিক, 20> seen{};
        Set set;
        বাবে (size_t i = 0; i < মানসমূহ.size(); ++i) {
            মানসমূহ[i] = মান_2{i / 2, i};  // Pairs of them collide
            set.ঢোকাও(&মানসমূহ[i]);
        }
        size_t n = 0;
        বাবে (মান_2* ধ্রুৱ valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), সঁচা);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], মিছা);  // Each of them exactly once
            seen[valuep->m_id] = সঁচা;
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
    শ্ৰেণী NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    সার্ৱজনিক:
        NoDefault(size_t* alivep, size_t hash, size_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ধ্রুৱ NoDefault& that)
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
        NoDefault& operator=(ধ্রুৱ NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() ধ্রুৱ { ঘূৰাই m_hash; }
        size_t id() ধ্রুৱ { ঘূৰাই m_id; }
        তার্কিক operator==(ধ্রুৱ NoDefault& that) ধ্রুৱ { ঘূৰাই m_id == that.m_id; }
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
        size_t operator()(ধ্রুৱ NoDefault& মান) ধ্রুৱ { ঘূৰাই মান.hash(); }
        size_t operator()(size_t hash, size_t) ধ্রুৱ { ঘূৰাই hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুৱ NoDefault& a, ধ্রুৱ NoDefault& b) ধ্রুৱ {
            ঘূৰাই operator()(a, b.hash(), b.id());
        }
        তার্কিক operator()(ধ্রুৱ NoDefault& a, size_t, size_t id) ধ্রুৱ { ঘূৰাই a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        বাবে (size_t i = 0; i < N; ++i) {
            ধ্রুৱ std::pair<Set::iterator, তার্কিক> added = set.ঢোকাও(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, সঁচা);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        বাবে (size_t i = 0; i < N; ++i) {
            ধ্রুৱ Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ধ্রুৱ std::pair<Set::iterator, তার্কিক> dup = set.ঢোকাও(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, মিছা);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ধ্রুৱ Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), মিছা);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        বাবে (size_t i = 1; i < N; ++i) {
            {
                ধ্রুৱ Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), সঁচা);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            বাবে (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), সঁচা);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), সঁচা);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        বাবে (size_t i = 0; i < N; ++i) set.ঢোকাও(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), সঁচা);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), মিছা);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        বাবে (size_t i = 0; i < N; ++i) set.ঢোকাও(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), সঁচা);  // Still empty
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
        Counted(ধ্রুৱ Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(ধ্রুৱ Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    সংরচনা Hash final {
        size_t operator()(ধ্রুৱ Counted& entry) ধ্রুৱ { ঘূৰাই operator()(entry.m_id); }
        size_t operator()(size_t id) ধ্রুৱ { ঘূৰাই id; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুৱ Counted& a, ধ্রুৱ Counted& b) ধ্রুৱ { ঘূৰাই operator()(a, b.m_id); }
        তার্কিক operator()(ধ্রুৱ Counted& a, size_t id) ধ্রুৱ { ঘূৰাই a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        বাবে (size_t i = 0; i < N; ++i) set.ঢোকাও(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), সঁচা);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), সঁচা);  // So it iterates nothing
        বাবে (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), সঁচা);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), মিছা);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.ঢোকাও(Counted{&alive, N}).second, সঁচা);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), মিছা);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), মিছা);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), সঁচা);  // And the source is empty again
        বাবে (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), সঁচা);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

শূন্য testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    শ্ৰেণী MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    সার্ৱজনিক:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = স্ৱভাৱ;
        MoveOnly(ধ্রুৱ MoveOnly&) = delete;
        MoveOnly& operator=(ধ্রুৱ MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = স্ৱভাৱ;

        size_t id() ধ্রুৱ { ঘূৰাই m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::মান,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::মান,
                  "SelfTest: 'MoveOnly' must be move constructible");

    সংরচনা Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(ধ্রুৱ MoveOnly& মান) ধ্রুৱ { ঘূৰাই operator()(মান.id()); }
        size_t operator()(size_t) ধ্রুৱ { ঘূৰাই 7; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুৱ MoveOnly& a, ধ্রুৱ MoveOnly& b) ধ্রুৱ {
            ঘূৰাই operator()(a, b.id());
        }
        তার্কিক operator()(ধ্রুৱ MoveOnly& a, size_t id) ধ্রুৱ { ঘূৰাই a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ std::pair<Set::iterator, তার্কিক> added
            = set.ঢোকাও_Lazy(i, [i] { ঘূৰাই MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, সঁচা);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    বাবে (size_t i = 0; i < N; i += 2) {
        ধ্রুৱ Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), সঁচা);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ তার্কিক erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

শূন্য testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    শ্ৰেণী Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    সার্ৱজনিক:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t মান) ধ্রুৱ { ঘূৰাই মান & m_mask; }
    };

    শ্ৰেণী Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    সার্ৱজনিক:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        তার্কিক operator()(size_t a, size_t b) ধ্রুৱ { ঘূৰাই (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::মান,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::মান,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    বাবে (size_t i = 0; i < 16; ++i) set.ঢোকাও(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    বাবে (size_t i = 0; i < 16; ++i) {
        ধ্রুৱ V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), সঁচা);  // Its class is present
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
        size_t operator()(ধ্রুৱ মান_2* valuep) ধ্রুৱ { ঘূৰাই operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুৱ { ঘূৰাই hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুৱ মান_2* ap, ধ্রুৱ মান_2* bp) ধ্রুৱ {
            ঘূৰাই operator()(ap, bp->m_hash, bp->m_id);
        }
        তার্কিক operator()(ধ্রুৱ মান_2* valuep, size_t hash, size_t id) ধ্রুৱ {
            ঘূৰাই valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    বাবে (ধ্রুৱ size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<মান_2, 4> মানসমূহ{মান_2{h, 0}, মান_2{h + 1, 1},  //
                                    মান_2{h, 2}, মান_2{h + 1, 3}};
        Set set;
        বাবে (মান_2& মান : মানসমূহ) set.ঢোকাও(&মান);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ধ্রুৱ Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), মিছা);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        বাবে (size_t i = 1; i < মানসমূহ.size(); ++i) {
            ধ্রুৱ Set::iterator it = set.find(মানসমূহ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ধ্রুৱ Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        বাবে (size_t i = 2; i < মানসমূহ.size(); ++i) {
            ধ্রুৱ Set::iterator it = set.find(মানসমূহ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), সঁচা);  // Both still reachable
            UASSERT_SELFTEST(*it, &মানসমূহ[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<তার্কিক, 4> seen{};
        বাবে (ধ্রুৱ মান_2* ধ্রুৱ valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], মিছা);  // Each entry once
            seen[valuep->m_id] = সঁচা;
        }
        UASSERT_SELFTEST(seen[0], মিছা);  // Erased first
        UASSERT_SELFTEST(seen[1], মিছা);  // Erased second
        UASSERT_SELFTEST(seen[2], সঁচা);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], সঁচা);  // And back again over the second
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
        size_t operator()(ধ্রুৱ মান_2* valuep) ধ্রুৱ { ঘূৰাই operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ধ্রুৱ { ঘূৰাই hash; }
    };

    সংরচনা Equal final {
        তার্কিক operator()(ধ্রুৱ মান_2* ap, ধ্রুৱ মান_2* bp) ধ্রুৱ {
            ঘূৰাই operator()(ap, bp->m_hash, bp->m_id);
        }
        তার্কিক operator()(ধ্রুৱ মান_2* valuep, size_t hash, size_t id) ধ্রুৱ {
            ঘূৰাই valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<মান_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), মিছা);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), সঁচা);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), সঁচা);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<মান_2, N> মানসমূহ{};
    std::array<মান_2* ধ্রুৱ*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    বাবে (size_t i = 0; i < N; ++i) {
        মানসমূহ[i] = মান_2{7, i};
        ধ্রুৱ std::pair<Set::iterator, তার্কিক> pair = set.ঢোকাও(&মানসমূহ[i]);
        UASSERT_SELFTEST(pair.second, সঁচা);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সঁচা);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ std::pair<Set::iterator, তার্কিক> pair = set.ঢোকাও(&মানসমূহ[i]);
        UASSERT_SELFTEST(pair.second, মিছা);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<মান_2* ধ্রুৱ*, N> grownpps{};  // Where each entry is after the growth
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সঁচা);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), সঁচা);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
সংরচনা ClusteredHash final {
    size_t operator()(size_t মান) ধ্রুৱ { ঘূৰাই মান & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
সংরচনা SpreadHash final {
    size_t operator()(size_t মান) ধ্রুৱ {
        ঘূৰাই static_cast<size_t>(মান * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
শূন্য testSetAgainstModel() {
    সংরচনা Equal final {
        তার্কিক operator()(size_t a, size_t b) ধ্রুৱ { ঘূৰাই a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ধ্রুৱ auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ঘূৰাই static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    বাবে (size_t step = 0; step < STEPS; ++step) {
        ধ্রুৱ size_t id = nextRand() % UNIVERSE;
        ৱিকল্প (nextRand() % 4) {
        পক্ষ 0: {  // Insert a copy
            ধ্রুৱ std::pair<typename Set::iterator, তার্কিক> pair = set.ঢোকাও(id);
            UASSERT_SELFTEST(pair.second, model.ঢোকাও(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            ভঙ্গ;
        }
        পক্ষ 1: {  // Insert lazily, which must create only on a miss
            তার্কিক created = মিছা;
            ধ্রুৱ std::pair<typename Set::iterator, তার্কিক> pair
                = set.ঢোকাও_Lazy(id, [&]() -> size_t {
                      created = সঁচা;
                      ঘূৰাই id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ঢোকাও(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            ভঙ্গ;
        }
        পক্ষ 2: {  // Erase, if present
            ধ্রুৱ typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.গুনতি(id) != 0);  // As the model
            যদি (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            ভঙ্গ;
        }
        স্ৱভাৱ: {  // Look up only
            ধ্রুৱ typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.গুনতি(id) != 0);  // As the model
            যদি (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            ভঙ্গ;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    বাবে (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.গুনতি(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<তার্কিক, UNIVERSE> seen{};
    size_t n = 0;
    বাবে (ধ্রুৱ size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, সঁচা);  // Nothing out of thin air
        UASSERT_SELFTEST(model.গুনতি(entry) != 0, সঁচা);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], মিছা);  // Each entry once
        seen[entry] = সঁচা;
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
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ std::string চাবি = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ধ্রুৱ std::pair<Map::iterator, তার্কিক> pair
            = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry { ঘূৰাই {চাবি, static_cast<পূর্ণাঙ্ক>(i)}; });
        UASSERT_SELFTEST(pair.second, সঁচা);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সঁচা);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণাঙ্ক>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ std::string চাবি = "key" + std::to_string(i);
        map.erase(map.find(চাবি));
        UASSERT_SELFTEST(map.ঢোকাও({চাবি, static_cast<পূর্ণাঙ্ক>(i) + 100}).second, সঁচা);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সঁচা);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণাঙ্ক>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    বাবে (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).মান() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    বাবে (size_t i = 0; i < N; ++i) {
        ধ্রুৱ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), সঁচা);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<পূর্ণাঙ্ক>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    বাবে (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).মান() -= 100;

    // Adding a key that is present changes nothing
    {
        ধ্রুৱ std::string চাবি = "key0";
        তার্কিক created = মিছা;
        ধ্রুৱ std::pair<Map::iterator, তার্কিক> pair = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry {
            created = সঁচা;  // LCOV_EXCL_START
            ঘূৰাই {চাবি, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, মিছা);  // The factory was never called
        UASSERT_SELFTEST(pair.second, মিছা);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ধ্রুৱ std::pair<Map::iterator, তার্কিক> added = map.ঢোকাও({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, সঁচা);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        ধ্রুৱ std::pair<Map::iterator, তার্কিক> again = map.ঢোকাও({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, মিছা);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        ধ্রুৱ Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), সঁচা);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), মিছা);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), সঁচা);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), মিছা);  // Namely that one
    বাবে (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), সঁচা);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
শূন্য testMapAgainstModel() {
    সংরচনা Equal final {
        তার্কিক operator()(size_t a, size_t b) ধ্রুৱ { ঘূৰাই a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ধ্রুৱ auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ঘূৰাই static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    বাবে (size_t step = 0; step < STEPS; ++step) {
        ধ্রুৱ size_t চাবি = nextRand() % UNIVERSE;
        ধ্রুৱ size_t val = step;  // Distinct every step, so a stale value is visible
        ৱিকল্প (nextRand() % 5) {
        পক্ষ 0: {  // Insert a whole entry
            ধ্রুৱ std::pair<typename Map::iterator, তার্কিক> pair = map.ঢোকাও({চাবি, val});
            UASSERT_SELFTEST(pair.second, model.ঢোকাও({চাবি, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, চাবি);  // At the key asked for
            ভঙ্গ;
        }
        পক্ষ 1: {  // Insert lazily, which must create only on a miss
            তার্কিক created = মিছা;
            ধ্রুৱ std::pair<typename Map::iterator, তার্কিক> pair
                = map.ঢোকাও_Lazy(চাবি, [&]() -> Entry {
                      created = সঁচা;
                      ঘূৰাই {চাবি, val};
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
            ধ্রুৱ typename Map::iterator it = map.find(চাবি);
            যদি (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.ঢোকাও({চাবি, val}).second, সঁচা);  // The key is free
                model[চাবি] = val;
            }
            ভঙ্গ;
        }
        স্ৱভাৱ: {  // Look up only
            ধ্রুৱ typename Map::iterator it = map.find(চাবি);
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
    বাবে (size_t চাবি = 0; চাবি < UNIVERSE; ++চাবি) {
        ধ্রুৱ typename Map::iterator it = map.find(চাবি);
        UASSERT_SELFTEST(it != map.end(), model.গুনতি(চাবি) != 0);  // Present iff modelled
        যদি (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(চাবি));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<তার্কিক, UNIVERSE> seen{};
    size_t n = 0;
    বাবে (ধ্রুৱ Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, সঁচা);  // Nothing out of thin air
        UASSERT_SELFTEST(model.গুনতি(entry.first) != 0, সঁচা);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], মিছা);  // Each entry once
        seen[entry.first] = সঁচা;
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
