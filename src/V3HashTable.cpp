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

#अन्तर्भाव "V3HashTable.h"

#अन्तर्भाव "V3Error.h"

#अन्तर्भाव <array>
#अन्तर्भाव <functional>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <string>
#अन्तर्भाव <type_traits>
#अन्तर्भाव <utility>

नामाकाश V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr size_t maxLoad(size_t capacity) { प्रत्यागम capacity * LOAD_FACTOR_अङ्कः / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr size_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

शून्य test_मानम्_कुञ्चिकाः() {
    संरचना मानम्_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        size_t operator()(ध्रुव मानम्_2& मानम्) ध्रुव { प्रत्यागम operator()(मानम्.m_hash, 0); }
        size_t operator()(size_t hash, size_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक operator()(ध्रुव मानम्_2& a, ध्रुव मानम्_2& b) ध्रुव {
            प्रत्यागम operator()(a, b.m_hash, b.m_id);
        }
        तार्किक operator()(ध्रुव मानम्_2& मानम्, size_t hash, size_t id) ध्रुव {
            प्रत्यागम मानम्.m_hash == hash && मानम्.m_id == id;
        }
    };

    using Set = V3HashSet<मानम्_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ध्रुव मानम्_2 मानम्{1, 0};
        ध्रुव मानम्_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), सच्च);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), सच्च);  // So iterates nothing
        UASSERT_SELFTEST(set.find(मानम्) == set.end(), सच्च);  // And finds nothing
        ध्रुव std::pair<Set::iterator, तार्किक> added = set.निवेशय(मानम्);
        UASSERT_SELFTEST(added.second, सच्च);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, मानम्.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &मानम्, सच्च);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), मिच्छ);  // So the set is no longer empty
        {
            ध्रुव Set::iterator it = set.find(मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            ध्रुव Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            ध्रुव Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), सच्च);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), सच्च);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), सच्च);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ध्रुव std::pair<Set::iterator, तार्किक> again = set.निवेशय(equal);
        UASSERT_SELFTEST(again.second, मिच्छ);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ध्रुव Set::iterator it = set.find(मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), सच्च);  // Empty again
        UASSERT_SELFTEST(set.find(मानम्) == set.end(), सच्च);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(मानम्), मिच्छ);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(मानम्), मिच्छ);  // Nothing to erase, so says so
        set.निवेशय(मानम्);
        UASSERT_SELFTEST(set.erase(equal), सच्च);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), सच्च);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.निवेशय(मानम्);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), सच्च);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), सच्च);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        प्रति (size_t i = 0; i < N; ++i) {
            ध्रुव std::pair<Set::iterator, तार्किक> added = set.निवेशय(मानम्_2{7, i});
            UASSERT_SELFTEST(added.second, सच्च);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        प्रति (size_t i = 0; i < N; ++i) {
            ध्रुव Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<तार्किक, N> seen{};
        size_t n = 0;
        प्रति (ध्रुव मानम्_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], मिच्छ);  // Each of them exactly once
            seen[entry.m_id] = सच्च;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

शून्य test_Pointer_कुञ्चिकाः() {
    संरचना मानम्_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        size_t operator()(ध्रुव मानम्_2* valuep) ध्रुव { प्रत्यागम operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक operator()(ध्रुव मानम्_2* ap, ध्रुव मानम्_2* bp) ध्रुव {
            प्रत्यागम operator()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक operator()(ध्रुव मानम्_2* valuep, size_t hash, size_t id) ध्रुव {
            प्रत्यागम valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<मानम्_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        मानम्_2 मानम्{1, 0};
        मानम्_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), सच्च);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), सच्च);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&मानम्) == set.end(), सच्च);  // And finds nothing
        ध्रुव std::pair<Set::iterator, तार्किक> added = set.निवेशय(&मानम्);
        UASSERT_SELFTEST(added.second, सच्च);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &मानम्);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), मिच्छ);  // So the set is no longer empty
        {
            ध्रुव Set::iterator it = set.find(&मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Found by itself
            UASSERT_SELFTEST(*it, &मानम्);  // At the entry held
        }
        {
            ध्रुव Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // And by an equal entry
            UASSERT_SELFTEST(*it, &मानम्);  // At that same entry
        }
        {
            ध्रुव Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), सच्च);  // And by its parts
            UASSERT_SELFTEST(*it, &मानम्);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), सच्च);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), सच्च);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ध्रुव std::pair<Set::iterator, तार्किक> again = set.निवेशय(&equal);
        UASSERT_SELFTEST(again.second, मिच्छ);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &मानम्);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ध्रुव Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &मानम्);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, मिच्छ);  // Not the one asked for
        }
        {
            ध्रुव Set::iterator it = set.find(&मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // The entry is still there
            UASSERT_SELFTEST(*it, &मानम्);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ध्रुव Set::iterator it = set.find(&मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Present before the erase
            UASSERT_SELFTEST(*it, &मानम्);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), सच्च);  // Empty again
        UASSERT_SELFTEST(set.find(&मानम्) == set.end(), सच्च);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&मानम्), मिच्छ);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), सच्च);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&मानम्), मिच्छ);  // Nothing to erase, so says so
        set.निवेशय(&मानम्);
        UASSERT_SELFTEST(set.erase(&equal), सच्च);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), सच्च);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&मानम्) == set.end(), सच्च);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.निवेशय(&मानम्);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), सच्च);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), सच्च);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    प्रति (size_t erase = 0; erase < 4; ++erase) {
        प्रति (ध्रुव size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<मानम्_2, 4> मानानि{मानम्_2{hash, 0}, मानम्_2{hash, 1},  //
                                        मानम्_2{hash, 2}, मानम्_2{hash, 3}};
            Set set;
            प्रति (मानम्_2& मानम् : मानानि) set.निवेशय(&मानम्);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                ध्रुव Set::iterator it = set.find(&मानानि[erase]);
                UASSERT_SELFTEST(it != set.end(), सच्च);  // The one to erase is present
                UASSERT_SELFTEST(*it, &मानानि[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&मानानि[erase]) == set.end(), सच्च);  // That one
            प्रति (size_t i = 0; i < मानानि.size(); ++i) {
                जइ (i == erase) अनुवर्त;
                ध्रुव Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), सच्च);  // The others are all there
                UASSERT_SELFTEST(*it, &मानानि[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    प्रति (ध्रुव size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<मानम्_2, GROWS_TWICE> many{};
        Set set;
        प्रति (size_t i = 0; i < many.size(); ++i) {
            many[i] = मानम्_2{hash, i};
            set.निवेशय(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        प्रति (size_t i = 0; i < many.size(); ++i) {
            ध्रुव Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        मानम्_2 मानम्{1, 0};
        मानम्_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.निवेशय(&मानम्);
        set.निवेशय(&other);
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            ध्रुव Set::iterator it = set.find(&मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // The first of the run is there
            UASSERT_SELFTEST(*it, &मानम्);  // And is that entry
        }
        {
            ध्रुव Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ध्रुव Set::iterator it = set.find(&मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Present before the erase
            UASSERT_SELFTEST(*it, &मानम्);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&मानम्) == set.end(), सच्च);  // Namely that one
        {
            ध्रुव Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    प्रति (ध्रुव तार्किक doReserve : {मिच्छ, सच्च}) {
        std::array<मानम्_2, 8> मानानि{};
        Set set;
        जइ (doReserve) set.reserve(मानानि.size());
        प्रति (size_t i = 0; i < मानानि.size(); ++i) {
            मानानि[i] = मानम्_2{static_cast<size_t>(i * 1234567), i};
            set.निवेशय(&मानानि[i]);
        }
        प्रति (मानम्_2& मानम् : मानानि) {
            ध्रुव Set::iterator it = set.find(&मानम्);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Each entry survives
            UASSERT_SELFTEST(*it, &मानम्);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), मानानि.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<मानम्_2, N> मानानि{};
        Set set;
        प्रति (size_t i = 0; i < N; ++i) {
            मानानि[i] = मानम्_2{7, i};  // All of them hash the same
            ध्रुव std::pair<Set::iterator, तार्किक> pair
                = set.निवेशय_Lazy(size_t{7}, i, [&]() -> मानम्_2* { प्रत्यागम &मानानि[i]; });
            UASSERT_SELFTEST(pair.second, सच्च);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &मानानि[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        प्रति (size_t i = 0; i < N; ++i) {
            तार्किक created = मिच्छ;
            ध्रुव std::pair<Set::iterator, तार्किक> pair
                = set.निवेशय_Lazy(size_t{7}, i, [&]() -> मानम्_2* {
                      created = सच्च;  // LCOV_EXCL_START
                      प्रत्यागम &मानानि[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, मिच्छ);  // The factory was never called
            UASSERT_SELFTEST(pair.second, मिच्छ);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &मानानि[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<मानम्_2, 20> मानानि{};
        std::array<तार्किक, 20> seen{};
        Set set;
        प्रति (size_t i = 0; i < मानानि.size(); ++i) {
            मानानि[i] = मानम्_2{i / 2, i};  // Pairs of them collide
            set.निवेशय(&मानानि[i]);
        }
        size_t n = 0;
        प्रति (मानम्_2* ध्रुव valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), सच्च);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], मिच्छ);  // Each of them exactly once
            seen[valuep->m_id] = सच्च;
            ++n;
        }
        UASSERT_SELFTEST(n, मानानि.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

शून्य testEntryLifetime() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    वर्ग NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    सार्वजनिक:
        NoDefault(size_t* alivep, size_t hash, size_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ध्रुव NoDefault& that)
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
        NoDefault& operator=(ध्रुव NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() ध्रुव { प्रत्यागम m_hash; }
        size_t id() ध्रुव { प्रत्यागम m_id; }
        तार्किक operator==(ध्रुव NoDefault& that) ध्रुव { प्रत्यागम m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    संरचना Hash final {
        size_t operator()(ध्रुव NoDefault& मानम्) ध्रुव { प्रत्यागम मानम्.hash(); }
        size_t operator()(size_t hash, size_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक operator()(ध्रुव NoDefault& a, ध्रुव NoDefault& b) ध्रुव {
            प्रत्यागम operator()(a, b.hash(), b.id());
        }
        तार्किक operator()(ध्रुव NoDefault& a, size_t, size_t id) ध्रुव { प्रत्यागम a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        प्रति (size_t i = 0; i < N; ++i) {
            ध्रुव std::pair<Set::iterator, तार्किक> added = set.निवेशय(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, सच्च);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        प्रति (size_t i = 0; i < N; ++i) {
            ध्रुव Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ध्रुव std::pair<Set::iterator, तार्किक> dup = set.निवेशय(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, मिच्छ);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ध्रुव Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), मिच्छ);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        प्रति (size_t i = 1; i < N; ++i) {
            {
                ध्रुव Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), सच्च);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            प्रति (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), सच्च);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), सच्च);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        प्रति (size_t i = 0; i < N; ++i) set.निवेशय(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), सच्च);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), सच्च);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), मिच्छ);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), सच्च);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        प्रति (size_t i = 0; i < N; ++i) set.निवेशय(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), सच्च);  // Still empty
        set.निवेशय(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

शून्य testMove() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    संरचना Counted final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_id;  // Entries with equal ids are equal

        Counted(size_t* alivep, size_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ध्रुव Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(ध्रुव Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    संरचना Hash final {
        size_t operator()(ध्रुव Counted& entry) ध्रुव { प्रत्यागम operator()(entry.m_id); }
        size_t operator()(size_t id) ध्रुव { प्रत्यागम id; }
    };

    संरचना Equal final {
        तार्किक operator()(ध्रुव Counted& a, ध्रुव Counted& b) ध्रुव { प्रत्यागम operator()(a, b.m_id); }
        तार्किक operator()(ध्रुव Counted& a, size_t id) ध्रुव { प्रत्यागम a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        प्रति (size_t i = 0; i < N; ++i) set.निवेशय(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), सच्च);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), सच्च);  // So it iterates nothing
        प्रति (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), सच्च);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), मिच्छ);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.निवेशय(Counted{&alive, N}).second, सच्च);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), मिच्छ);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), मिच्छ);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), सच्च);  // And the source is empty again
        प्रति (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), सच्च);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

शून्य testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    वर्ग MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    सार्वजनिक:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = स्वभाव;
        MoveOnly(ध्रुव MoveOnly&) = delete;
        MoveOnly& operator=(ध्रुव MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = स्वभाव;

        size_t id() ध्रुव { प्रत्यागम m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::मानम्,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::मानम्,
                  "SelfTest: 'MoveOnly' must be move constructible");

    संरचना Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(ध्रुव MoveOnly& मानम्) ध्रुव { प्रत्यागम operator()(मानम्.id()); }
        size_t operator()(size_t) ध्रुव { प्रत्यागम 7; }
    };

    संरचना Equal final {
        तार्किक operator()(ध्रुव MoveOnly& a, ध्रुव MoveOnly& b) ध्रुव {
            प्रत्यागम operator()(a, b.id());
        }
        तार्किक operator()(ध्रुव MoveOnly& a, size_t id) ध्रुव { प्रत्यागम a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव std::pair<Set::iterator, तार्किक> added
            = set.निवेशय_Lazy(i, [i] { प्रत्यागम MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, सच्च);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    प्रति (size_t i = 0; i < N; i += 2) {
        ध्रुव Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), सच्च);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव तार्किक erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

शून्य testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    वर्ग Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    सार्वजनिक:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t मानम्) ध्रुव { प्रत्यागम मानम् & m_mask; }
    };

    वर्ग Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    सार्वजनिक:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        तार्किक operator()(size_t a, size_t b) ध्रुव { प्रत्यागम (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::मानम्,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::मानम्,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    प्रति (size_t i = 0; i < 16; ++i) set.निवेशय(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    प्रति (size_t i = 0; i < 16; ++i) {
        ध्रुव V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), सच्च);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

शून्य testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    संरचना मानम्_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Entries with equal ids are equal
    };

    संरचना Hash final {
        size_t operator()(ध्रुव मानम्_2* valuep) ध्रुव { प्रत्यागम operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक operator()(ध्रुव मानम्_2* ap, ध्रुव मानम्_2* bp) ध्रुव {
            प्रत्यागम operator()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक operator()(ध्रुव मानम्_2* valuep, size_t hash, size_t id) ध्रुव {
            प्रत्यागम valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<मानम्_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    प्रति (ध्रुव size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<मानम्_2, 4> मानानि{मानम्_2{h, 0}, मानम्_2{h + 1, 1},  //
                                    मानम्_2{h, 2}, मानम्_2{h + 1, 3}};
        Set set;
        प्रति (मानम्_2& मानम् : मानानि) set.निवेशय(&मानम्);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ध्रुव Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), मिच्छ);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        प्रति (size_t i = 1; i < मानानि.size(); ++i) {
            ध्रुव Set::iterator it = set.find(मानानि[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &मानानि[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ध्रुव Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        प्रति (size_t i = 2; i < मानानि.size(); ++i) {
            ध्रुव Set::iterator it = set.find(मानानि[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), सच्च);  // Both still reachable
            UASSERT_SELFTEST(*it, &मानानि[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<तार्किक, 4> seen{};
        प्रति (ध्रुव मानम्_2* ध्रुव valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], मिच्छ);  // Each entry once
            seen[valuep->m_id] = सच्च;
        }
        UASSERT_SELFTEST(seen[0], मिच्छ);  // Erased first
        UASSERT_SELFTEST(seen[1], मिच्छ);  // Erased second
        UASSERT_SELFTEST(seen[2], सच्च);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], सच्च);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

शून्य testReferenceStability() {
    संरचना मानम्_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        size_t operator()(ध्रुव मानम्_2* valuep) ध्रुव { प्रत्यागम operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक operator()(ध्रुव मानम्_2* ap, ध्रुव मानम्_2* bp) ध्रुव {
            प्रत्यागम operator()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक operator()(ध्रुव मानम्_2* valuep, size_t hash, size_t id) ध्रुव {
            प्रत्यागम valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<मानम्_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), मिच्छ);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), सच्च);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), सच्च);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<मानम्_2, N> मानानि{};
    std::array<मानम्_2* ध्रुव*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    प्रति (size_t i = 0; i < N; ++i) {
        मानानि[i] = मानम्_2{7, i};
        ध्रुव std::pair<Set::iterator, तार्किक> pair = set.निवेशय(&मानानि[i]);
        UASSERT_SELFTEST(pair.second, सच्च);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), सच्च);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव std::pair<Set::iterator, तार्किक> pair = set.निवेशय(&मानानि[i]);
        UASSERT_SELFTEST(pair.second, मिच्छ);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<मानम्_2* ध्रुव*, N> grownpps{};  // Where each entry is after the growth
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), सच्च);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), सच्च);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
संरचना ClusteredHash final {
    size_t operator()(size_t मानम्) ध्रुव { प्रत्यागम मानम् & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
संरचना SpreadHash final {
    size_t operator()(size_t मानम्) ध्रुव {
        प्रत्यागम static_cast<size_t>(मानम् * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
शून्य testSetAgainstModel() {
    संरचना Equal final {
        तार्किक operator()(size_t a, size_t b) ध्रुव { प्रत्यागम a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ध्रुव auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        प्रत्यागम static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    प्रति (size_t step = 0; step < STEPS; ++step) {
        ध्रुव size_t id = nextRand() % UNIVERSE;
        विकल्प (nextRand() % 4) {
        पक्ष 0: {  // Insert a copy
            ध्रुव std::pair<typename Set::iterator, तार्किक> pair = set.निवेशय(id);
            UASSERT_SELFTEST(pair.second, model.निवेशय(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            भङ्ग;
        }
        पक्ष 1: {  // Insert lazily, which must create only on a miss
            तार्किक created = मिच्छ;
            ध्रुव std::pair<typename Set::iterator, तार्किक> pair
                = set.निवेशय_Lazy(id, [&]() -> size_t {
                      created = सच्च;
                      प्रत्यागम id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.निवेशय(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            भङ्ग;
        }
        पक्ष 2: {  // Erase, if present
            ध्रुव typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.गणनम्(id) != 0);  // As the model
            जइ (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            भङ्ग;
        }
        स्वभाव: {  // Look up only
            ध्रुव typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.गणनम्(id) != 0);  // As the model
            जइ (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            भङ्ग;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    प्रति (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.गणनम्(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<तार्किक, UNIVERSE> seen{};
    size_t n = 0;
    प्रति (ध्रुव size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, सच्च);  // Nothing out of thin air
        UASSERT_SELFTEST(model.गणनम्(entry) != 0, सच्च);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], मिच्छ);  // Each entry once
        seen[entry] = सच्च;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

शून्य testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, पूर्णाङ्क>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, पूर्णाङ्क>>::मानम्,
                  "SelfTest: a map entry must be a plain pair");

    constexpr size_t N = GROWS_TWICE;
    Map map;
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव std::string कुञ्चिका = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ध्रुव std::pair<Map::iterator, तार्किक> pair
            = map.निवेशय_Lazy(कुञ्चिका, [&]() -> Entry { प्रत्यागम {कुञ्चिका, static_cast<पूर्णाङ्क>(i)}; });
        UASSERT_SELFTEST(pair.second, सच्च);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), सच्च);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव std::string कुञ्चिका = "key" + std::to_string(i);
        map.erase(map.find(कुञ्चिका));
        UASSERT_SELFTEST(map.निवेशय({कुञ्चिका, static_cast<पूर्णाङ्क>(i) + 100}).second, सच्च);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), सच्च);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    प्रति (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).मानम्() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    प्रति (size_t i = 0; i < N; ++i) {
        ध्रुव Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), सच्च);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    प्रति (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).मानम्() -= 100;

    // Adding a key that is present changes nothing
    {
        ध्रुव std::string कुञ्चिका = "key0";
        तार्किक created = मिच्छ;
        ध्रुव std::pair<Map::iterator, तार्किक> pair = map.निवेशय_Lazy(कुञ्चिका, [&]() -> Entry {
            created = सच्च;  // LCOV_EXCL_START
            प्रत्यागम {कुञ्चिका, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, मिच्छ);  // The factory was never called
        UASSERT_SELFTEST(pair.second, मिच्छ);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ध्रुव std::pair<Map::iterator, तार्किक> added = map.निवेशय({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, सच्च);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        ध्रुव std::pair<Map::iterator, तार्किक> again = map.निवेशय({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, मिच्छ);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        ध्रुव Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), सच्च);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), मिच्छ);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), सच्च);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), मिच्छ);  // Namely that one
    प्रति (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), सच्च);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
शून्य testMapAgainstModel() {
    संरचना Equal final {
        तार्किक operator()(size_t a, size_t b) ध्रुव { प्रत्यागम a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ध्रुव auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        प्रत्यागम static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    प्रति (size_t step = 0; step < STEPS; ++step) {
        ध्रुव size_t कुञ्चिका = nextRand() % UNIVERSE;
        ध्रुव size_t val = step;  // Distinct every step, so a stale value is visible
        विकल्प (nextRand() % 5) {
        पक्ष 0: {  // Insert a whole entry
            ध्रुव std::pair<typename Map::iterator, तार्किक> pair = map.निवेशय({कुञ्चिका, val});
            UASSERT_SELFTEST(pair.second, model.निवेशय({कुञ्चिका, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, कुञ्चिका);  // At the key asked for
            भङ्ग;
        }
        पक्ष 1: {  // Insert lazily, which must create only on a miss
            तार्किक created = मिच्छ;
            ध्रुव std::pair<typename Map::iterator, तार्किक> pair
                = map.निवेशय_Lazy(कुञ्चिका, [&]() -> Entry {
                      created = सच्च;
                      प्रत्यागम {कुञ्चिका, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.निवेशय({कुञ्चिका, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, कुञ्चिका);  // At the key asked for
            भङ्ग;
        }
        पक्ष 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(कुञ्चिका), model.erase(कुञ्चिका) != 0);  // As model
            भङ्ग;
        }
        पक्ष 3: {  // Change a value, which is erasing the entry and inserting it again
            ध्रुव typename Map::iterator it = map.find(कुञ्चिका);
            जइ (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.निवेशय({कुञ्चिका, val}).second, सच्च);  // The key is free
                model[कुञ्चिका] = val;
            }
            भङ्ग;
        }
        स्वभाव: {  // Look up only
            ध्रुव typename Map::iterator it = map.find(कुञ्चिका);
            UASSERT_SELFTEST(it != map.end(), model.गणनम्(कुञ्चिका) != 0);  // As the model
            जइ (it != map.end()) {
                UASSERT_SELFTEST(it->first, कुञ्चिका);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(कुञ्चिका));  // With its value
            }
            भङ्ग;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    प्रति (size_t कुञ्चिका = 0; कुञ्चिका < UNIVERSE; ++कुञ्चिका) {
        ध्रुव typename Map::iterator it = map.find(कुञ्चिका);
        UASSERT_SELFTEST(it != map.end(), model.गणनम्(कुञ्चिका) != 0);  // Present iff modelled
        जइ (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(कुञ्चिका));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<तार्किक, UNIVERSE> seen{};
    size_t n = 0;
    प्रति (ध्रुव Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, सच्च);  // Nothing out of thin air
        UASSERT_SELFTEST(model.गणनम्(entry.first) != 0, सच्च);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], मिच्छ);  // Each entry once
        seen[entry.first] = सच्च;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

शून्य selfTest() {
    test_मानम्_कुञ्चिकाः();
    test_Pointer_कुञ्चिकाः();
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
