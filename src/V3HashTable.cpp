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

#અન્તર્ભાવ "V3HashTable.h"

#અન્તર્ભાવ "V3Error.h"

#અન્તર્ભાવ <array>
#અન્તર્ભાવ <functional>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <type_traits>
#અન્તર્ભાવ <utility>

નામાકાશ V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr size_t maxLoad(size_t capacity) { પાછા capacity * LOAD_FACTOR_અંક / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr size_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

શૂન્ય test_કિંમત_ચાવીઓ() {
    સંરચના કિંમત_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    સંરચના Hash final {
        size_t operator()(અચલ કિંમત_2& કિંમત) અચલ { પાછા operator()(કિંમત.m_hash, 0); }
        size_t operator()(size_t hash, size_t) અચલ { પાછા hash; }
    };

    સંરચના Equal final {
        તાર્કિક operator()(અચલ કિંમત_2& a, અચલ કિંમત_2& b) અચલ {
            પાછા operator()(a, b.m_hash, b.m_id);
        }
        તાર્કિક operator()(અચલ કિંમત_2& કિંમત, size_t hash, size_t id) અચલ {
            પાછા કિંમત.m_hash == hash && કિંમત.m_id == id;
        }
    };

    using Set = V3HashSet<કિંમત_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        અચલ કિંમત_2 કિંમત{1, 0};
        અચલ કિંમત_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), સાચું);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), સાચું);  // So iterates nothing
        UASSERT_SELFTEST(set.find(કિંમત) == set.end(), સાચું);  // And finds nothing
        અચલ std::pair<Set::iterator, તાર્કિક> added = set.ઉમેરો(કિંમત);
        UASSERT_SELFTEST(added.second, સાચું);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, કિંમત.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &કિંમત, સાચું);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), ખોટું);  // So the set is no longer empty
        {
            અચલ Set::iterator it = set.find(કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            અચલ Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            અચલ Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), સાચું);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), સાચું);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), સાચું);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        અચલ std::pair<Set::iterator, તાર્કિક> again = set.ઉમેરો(equal);
        UASSERT_SELFTEST(again.second, ખોટું);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            અચલ Set::iterator it = set.find(કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), સાચું);  // Empty again
        UASSERT_SELFTEST(set.find(કિંમત) == set.end(), સાચું);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(કિંમત), ખોટું);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(કિંમત), ખોટું);  // Nothing to erase, so says so
        set.ઉમેરો(કિંમત);
        UASSERT_SELFTEST(set.erase(equal), સાચું);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), સાચું);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.ઉમેરો(કિંમત);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), સાચું);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), સાચું);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        માટે (size_t i = 0; i < N; ++i) {
            અચલ std::pair<Set::iterator, તાર્કિક> added = set.ઉમેરો(કિંમત_2{7, i});
            UASSERT_SELFTEST(added.second, સાચું);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        માટે (size_t i = 0; i < N; ++i) {
            અચલ Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<તાર્કિક, N> seen{};
        size_t n = 0;
        માટે (અચલ કિંમત_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], ખોટું);  // Each of them exactly once
            seen[entry.m_id] = સાચું;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

શૂન્ય test_Pointer_ચાવીઓ() {
    સંરચના કિંમત_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    સંરચના Hash final {
        size_t operator()(અચલ કિંમત_2* valuep) અચલ { પાછા operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) અચલ { પાછા hash; }
    };

    સંરચના Equal final {
        તાર્કિક operator()(અચલ કિંમત_2* ap, અચલ કિંમત_2* bp) અચલ {
            પાછા operator()(ap, bp->m_hash, bp->m_id);
        }
        તાર્કિક operator()(અચલ કિંમત_2* valuep, size_t hash, size_t id) અચલ {
            પાછા valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<કિંમત_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        કિંમત_2 કિંમત{1, 0};
        કિંમત_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), સાચું);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), સાચું);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&કિંમત) == set.end(), સાચું);  // And finds nothing
        અચલ std::pair<Set::iterator, તાર્કિક> added = set.ઉમેરો(&કિંમત);
        UASSERT_SELFTEST(added.second, સાચું);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &કિંમત);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), ખોટું);  // So the set is no longer empty
        {
            અચલ Set::iterator it = set.find(&કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Found by itself
            UASSERT_SELFTEST(*it, &કિંમત);  // At the entry held
        }
        {
            અચલ Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // And by an equal entry
            UASSERT_SELFTEST(*it, &કિંમત);  // At that same entry
        }
        {
            અચલ Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), સાચું);  // And by its parts
            UASSERT_SELFTEST(*it, &કિંમત);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), સાચું);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), સાચું);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        અચલ std::pair<Set::iterator, તાર્કિક> again = set.ઉમેરો(&equal);
        UASSERT_SELFTEST(again.second, ખોટું);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &કિંમત);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            અચલ Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &કિંમત);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, ખોટું);  // Not the one asked for
        }
        {
            અચલ Set::iterator it = set.find(&કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // The entry is still there
            UASSERT_SELFTEST(*it, &કિંમત);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            અચલ Set::iterator it = set.find(&કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Present before the erase
            UASSERT_SELFTEST(*it, &કિંમત);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), સાચું);  // Empty again
        UASSERT_SELFTEST(set.find(&કિંમત) == set.end(), સાચું);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&કિંમત), ખોટું);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), સાચું);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&કિંમત), ખોટું);  // Nothing to erase, so says so
        set.ઉમેરો(&કિંમત);
        UASSERT_SELFTEST(set.erase(&equal), સાચું);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), સાચું);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&કિંમત) == set.end(), સાચું);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.ઉમેરો(&કિંમત);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), સાચું);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), સાચું);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    માટે (size_t erase = 0; erase < 4; ++erase) {
        માટે (અચલ size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<કિંમત_2, 4> કિંમતો{કિંમત_2{hash, 0}, કિંમત_2{hash, 1},  //
                                        કિંમત_2{hash, 2}, કિંમત_2{hash, 3}};
            Set set;
            માટે (કિંમત_2& કિંમત : કિંમતો) set.ઉમેરો(&કિંમત);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                અચલ Set::iterator it = set.find(&કિંમતો[erase]);
                UASSERT_SELFTEST(it != set.end(), સાચું);  // The one to erase is present
                UASSERT_SELFTEST(*it, &કિંમતો[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&કિંમતો[erase]) == set.end(), સાચું);  // That one
            માટે (size_t i = 0; i < કિંમતો.size(); ++i) {
                જો (i == erase) ચાલુ;
                અચલ Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), સાચું);  // The others are all there
                UASSERT_SELFTEST(*it, &કિંમતો[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    માટે (અચલ size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<કિંમત_2, GROWS_TWICE> many{};
        Set set;
        માટે (size_t i = 0; i < many.size(); ++i) {
            many[i] = કિંમત_2{hash, i};
            set.ઉમેરો(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        માટે (size_t i = 0; i < many.size(); ++i) {
            અચલ Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        કિંમત_2 કિંમત{1, 0};
        કિંમત_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.ઉમેરો(&કિંમત);
        set.ઉમેરો(&other);
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            અચલ Set::iterator it = set.find(&કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // The first of the run is there
            UASSERT_SELFTEST(*it, &કિંમત);  // And is that entry
        }
        {
            અચલ Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            અચલ Set::iterator it = set.find(&કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Present before the erase
            UASSERT_SELFTEST(*it, &કિંમત);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&કિંમત) == set.end(), સાચું);  // Namely that one
        {
            અચલ Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    માટે (અચલ તાર્કિક doReserve : {ખોટું, સાચું}) {
        std::array<કિંમત_2, 8> કિંમતો{};
        Set set;
        જો (doReserve) set.reserve(કિંમતો.size());
        માટે (size_t i = 0; i < કિંમતો.size(); ++i) {
            કિંમતો[i] = કિંમત_2{static_cast<size_t>(i * 1234567), i};
            set.ઉમેરો(&કિંમતો[i]);
        }
        માટે (કિંમત_2& કિંમત : કિંમતો) {
            અચલ Set::iterator it = set.find(&કિંમત);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Each entry survives
            UASSERT_SELFTEST(*it, &કિંમત);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), કિંમતો.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<કિંમત_2, N> કિંમતો{};
        Set set;
        માટે (size_t i = 0; i < N; ++i) {
            કિંમતો[i] = કિંમત_2{7, i};  // All of them hash the same
            અચલ std::pair<Set::iterator, તાર્કિક> pair
                = set.ઉમેરો_Lazy(size_t{7}, i, [&]() -> કિંમત_2* { પાછા &કિંમતો[i]; });
            UASSERT_SELFTEST(pair.second, સાચું);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &કિંમતો[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        માટે (size_t i = 0; i < N; ++i) {
            તાર્કિક created = ખોટું;
            અચલ std::pair<Set::iterator, તાર્કિક> pair
                = set.ઉમેરો_Lazy(size_t{7}, i, [&]() -> કિંમત_2* {
                      created = સાચું;  // LCOV_EXCL_START
                      પાછા &કિંમતો[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, ખોટું);  // The factory was never called
            UASSERT_SELFTEST(pair.second, ખોટું);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &કિંમતો[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<કિંમત_2, 20> કિંમતો{};
        std::array<તાર્કિક, 20> seen{};
        Set set;
        માટે (size_t i = 0; i < કિંમતો.size(); ++i) {
            કિંમતો[i] = કિંમત_2{i / 2, i};  // Pairs of them collide
            set.ઉમેરો(&કિંમતો[i]);
        }
        size_t n = 0;
        માટે (કિંમત_2* અચલ valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), સાચું);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], ખોટું);  // Each of them exactly once
            seen[valuep->m_id] = સાચું;
            ++n;
        }
        UASSERT_SELFTEST(n, કિંમતો.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

શૂન્ય testEntryLifetime() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    વર્ગ NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    જાહેર:
        NoDefault(size_t* alivep, size_t hash, size_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(અચલ NoDefault& that)
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
        NoDefault& operator=(અચલ NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() અચલ { પાછા m_hash; }
        size_t id() અચલ { પાછા m_id; }
        તાર્કિક operator==(અચલ NoDefault& that) અચલ { પાછા m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::કિંમત,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::કિંમત,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::કિંમત,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::કિંમત,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    સંરચના Hash final {
        size_t operator()(અચલ NoDefault& કિંમત) અચલ { પાછા કિંમત.hash(); }
        size_t operator()(size_t hash, size_t) અચલ { પાછા hash; }
    };

    સંરચના Equal final {
        તાર્કિક operator()(અચલ NoDefault& a, અચલ NoDefault& b) અચલ {
            પાછા operator()(a, b.hash(), b.id());
        }
        તાર્કિક operator()(અચલ NoDefault& a, size_t, size_t id) અચલ { પાછા a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        માટે (size_t i = 0; i < N; ++i) {
            અચલ std::pair<Set::iterator, તાર્કિક> added = set.ઉમેરો(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, સાચું);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        માટે (size_t i = 0; i < N; ++i) {
            અચલ Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            અચલ std::pair<Set::iterator, તાર્કિક> dup = set.ઉમેરો(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, ખોટું);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            અચલ Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), ખોટું);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        માટે (size_t i = 1; i < N; ++i) {
            {
                અચલ Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), સાચું);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            માટે (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), સાચું);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), સાચું);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        માટે (size_t i = 0; i < N; ++i) set.ઉમેરો(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), સાચું);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), સાચું);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), ખોટું);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), સાચું);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        માટે (size_t i = 0; i < N; ++i) set.ઉમેરો(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), સાચું);  // Still empty
        set.ઉમેરો(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

શૂન્ય testMove() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    સંરચના Counted final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_id;  // Entries with equal ids are equal

        Counted(size_t* alivep, size_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(અચલ Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(અચલ Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    સંરચના Hash final {
        size_t operator()(અચલ Counted& entry) અચલ { પાછા operator()(entry.m_id); }
        size_t operator()(size_t id) અચલ { પાછા id; }
    };

    સંરચના Equal final {
        તાર્કિક operator()(અચલ Counted& a, અચલ Counted& b) અચલ { પાછા operator()(a, b.m_id); }
        તાર્કિક operator()(અચલ Counted& a, size_t id) અચલ { પાછા a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        માટે (size_t i = 0; i < N; ++i) set.ઉમેરો(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), સાચું);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), સાચું);  // So it iterates nothing
        માટે (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), સાચું);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), ખોટું);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.ઉમેરો(Counted{&alive, N}).second, સાચું);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), ખોટું);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), ખોટું);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), સાચું);  // And the source is empty again
        માટે (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), સાચું);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

શૂન્ય testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    વર્ગ MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    જાહેર:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = મૂળભૂત;
        MoveOnly(અચલ MoveOnly&) = delete;
        MoveOnly& operator=(અચલ MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = મૂળભૂત;

        size_t id() અચલ { પાછા m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::કિંમત,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::કિંમત,
                  "SelfTest: 'MoveOnly' must be move constructible");

    સંરચના Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(અચલ MoveOnly& કિંમત) અચલ { પાછા operator()(કિંમત.id()); }
        size_t operator()(size_t) અચલ { પાછા 7; }
    };

    સંરચના Equal final {
        તાર્કિક operator()(અચલ MoveOnly& a, અચલ MoveOnly& b) અચલ {
            પાછા operator()(a, b.id());
        }
        તાર્કિક operator()(અચલ MoveOnly& a, size_t id) અચલ { પાછા a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    માટે (size_t i = 0; i < N; ++i) {
        અચલ std::pair<Set::iterator, તાર્કિક> added
            = set.ઉમેરો_Lazy(i, [i] { પાછા MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, સાચું);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    માટે (size_t i = 0; i < N; i += 2) {
        અચલ Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), સાચું);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    માટે (size_t i = 0; i < N; ++i) {
        અચલ તાર્કિક erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

શૂન્ય testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    વર્ગ Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    જાહેર:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t કિંમત) અચલ { પાછા કિંમત & m_mask; }
    };

    વર્ગ Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    જાહેર:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        તાર્કિક operator()(size_t a, size_t b) અચલ { પાછા (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::કિંમત,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::કિંમત,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    માટે (size_t i = 0; i < 16; ++i) set.ઉમેરો(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    માટે (size_t i = 0; i < 16; ++i) {
        અચલ V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), સાચું);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

શૂન્ય testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    સંરચના કિંમત_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Entries with equal ids are equal
    };

    સંરચના Hash final {
        size_t operator()(અચલ કિંમત_2* valuep) અચલ { પાછા operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) અચલ { પાછા hash; }
    };

    સંરચના Equal final {
        તાર્કિક operator()(અચલ કિંમત_2* ap, અચલ કિંમત_2* bp) અચલ {
            પાછા operator()(ap, bp->m_hash, bp->m_id);
        }
        તાર્કિક operator()(અચલ કિંમત_2* valuep, size_t hash, size_t id) અચલ {
            પાછા valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<કિંમત_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    માટે (અચલ size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<કિંમત_2, 4> કિંમતો{કિંમત_2{h, 0}, કિંમત_2{h + 1, 1},  //
                                    કિંમત_2{h, 2}, કિંમત_2{h + 1, 3}};
        Set set;
        માટે (કિંમત_2& કિંમત : કિંમતો) set.ઉમેરો(&કિંમત);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            અચલ Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), ખોટું);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        માટે (size_t i = 1; i < કિંમતો.size(); ++i) {
            અચલ Set::iterator it = set.find(કિંમતો[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &કિંમતો[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            અચલ Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        માટે (size_t i = 2; i < કિંમતો.size(); ++i) {
            અચલ Set::iterator it = set.find(કિંમતો[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), સાચું);  // Both still reachable
            UASSERT_SELFTEST(*it, &કિંમતો[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<તાર્કિક, 4> seen{};
        માટે (અચલ કિંમત_2* અચલ valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], ખોટું);  // Each entry once
            seen[valuep->m_id] = સાચું;
        }
        UASSERT_SELFTEST(seen[0], ખોટું);  // Erased first
        UASSERT_SELFTEST(seen[1], ખોટું);  // Erased second
        UASSERT_SELFTEST(seen[2], સાચું);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], સાચું);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

શૂન્ય testReferenceStability() {
    સંરચના કિંમત_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    સંરચના Hash final {
        size_t operator()(અચલ કિંમત_2* valuep) અચલ { પાછા operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) અચલ { પાછા hash; }
    };

    સંરચના Equal final {
        તાર્કિક operator()(અચલ કિંમત_2* ap, અચલ કિંમત_2* bp) અચલ {
            પાછા operator()(ap, bp->m_hash, bp->m_id);
        }
        તાર્કિક operator()(અચલ કિંમત_2* valuep, size_t hash, size_t id) અચલ {
            પાછા valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<કિંમત_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), ખોટું);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), સાચું);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), સાચું);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<કિંમત_2, N> કિંમતો{};
    std::array<કિંમત_2* અચલ*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    માટે (size_t i = 0; i < N; ++i) {
        કિંમતો[i] = કિંમત_2{7, i};
        અચલ std::pair<Set::iterator, તાર્કિક> pair = set.ઉમેરો(&કિંમતો[i]);
        UASSERT_SELFTEST(pair.second, સાચું);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    માટે (size_t i = 0; i < N; ++i) {
        અચલ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), સાચું);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    માટે (size_t i = 0; i < N; ++i) {
        અચલ std::pair<Set::iterator, તાર્કિક> pair = set.ઉમેરો(&કિંમતો[i]);
        UASSERT_SELFTEST(pair.second, ખોટું);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<કિંમત_2* અચલ*, N> grownpps{};  // Where each entry is after the growth
    માટે (size_t i = 0; i < N; ++i) {
        અચલ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), સાચું);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    માટે (size_t i = 0; i < N; ++i) {
        અચલ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), સાચું);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
સંરચના ClusteredHash final {
    size_t operator()(size_t કિંમત) અચલ { પાછા કિંમત & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
સંરચના SpreadHash final {
    size_t operator()(size_t કિંમત) અચલ {
        પાછા static_cast<size_t>(કિંમત * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
શૂન્ય testSetAgainstModel() {
    સંરચના Equal final {
        તાર્કિક operator()(size_t a, size_t b) અચલ { પાછા a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    અચલ auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        પાછા static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    માટે (size_t step = 0; step < STEPS; ++step) {
        અચલ size_t id = nextRand() % UNIVERSE;
        પસંદગી (nextRand() % 4) {
        સ્થિતિ 0: {  // Insert a copy
            અચલ std::pair<typename Set::iterator, તાર્કિક> pair = set.ઉમેરો(id);
            UASSERT_SELFTEST(pair.second, model.ઉમેરો(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            તોડો;
        }
        સ્થિતિ 1: {  // Insert lazily, which must create only on a miss
            તાર્કિક created = ખોટું;
            અચલ std::pair<typename Set::iterator, તાર્કિક> pair
                = set.ઉમેરો_Lazy(id, [&]() -> size_t {
                      created = સાચું;
                      પાછા id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ઉમેરો(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            તોડો;
        }
        સ્થિતિ 2: {  // Erase, if present
            અચલ typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.ગણતરી(id) != 0);  // As the model
            જો (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            તોડો;
        }
        મૂળભૂત: {  // Look up only
            અચલ typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.ગણતરી(id) != 0);  // As the model
            જો (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            તોડો;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    માટે (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.ગણતરી(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<તાર્કિક, UNIVERSE> seen{};
    size_t n = 0;
    માટે (અચલ size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, સાચું);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ગણતરી(entry) != 0, સાચું);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], ખોટું);  // Each entry once
        seen[entry] = સાચું;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

શૂન્ય testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, પૂર્ણાંક>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, પૂર્ણાંક>>::કિંમત,
                  "SelfTest: a map entry must be a plain pair");

    constexpr size_t N = GROWS_TWICE;
    Map map;
    માટે (size_t i = 0; i < N; ++i) {
        અચલ std::string ચાવી = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        અચલ std::pair<Map::iterator, તાર્કિક> pair
            = map.ઉમેરો_Lazy(ચાવી, [&]() -> Entry { પાછા {ચાવી, static_cast<પૂર્ણાંક>(i)}; });
        UASSERT_SELFTEST(pair.second, સાચું);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    માટે (size_t i = 0; i < N; ++i) {
        અચલ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), સાચું);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<પૂર્ણાંક>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    માટે (size_t i = 0; i < N; ++i) {
        અચલ std::string ચાવી = "key" + std::to_string(i);
        map.erase(map.find(ચાવી));
        UASSERT_SELFTEST(map.ઉમેરો({ચાવી, static_cast<પૂર્ણાંક>(i) + 100}).second, સાચું);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    માટે (size_t i = 0; i < N; ++i) {
        અચલ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), સાચું);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<પૂર્ણાંક>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    માટે (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).કિંમત() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    માટે (size_t i = 0; i < N; ++i) {
        અચલ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), સાચું);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<પૂર્ણાંક>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    માટે (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).કિંમત() -= 100;

    // Adding a key that is present changes nothing
    {
        અચલ std::string ચાવી = "key0";
        તાર્કિક created = ખોટું;
        અચલ std::pair<Map::iterator, તાર્કિક> pair = map.ઉમેરો_Lazy(ચાવી, [&]() -> Entry {
            created = સાચું;  // LCOV_EXCL_START
            પાછા {ચાવી, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, ખોટું);  // The factory was never called
        UASSERT_SELFTEST(pair.second, ખોટું);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        અચલ std::pair<Map::iterator, તાર્કિક> added = map.ઉમેરો({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, સાચું);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        અચલ std::pair<Map::iterator, તાર્કિક> again = map.ઉમેરો({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, ખોટું);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        અચલ Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), સાચું);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), ખોટું);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), સાચું);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), ખોટું);  // Namely that one
    માટે (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), સાચું);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
શૂન્ય testMapAgainstModel() {
    સંરચના Equal final {
        તાર્કિક operator()(size_t a, size_t b) અચલ { પાછા a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    અચલ auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        પાછા static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    માટે (size_t step = 0; step < STEPS; ++step) {
        અચલ size_t ચાવી = nextRand() % UNIVERSE;
        અચલ size_t val = step;  // Distinct every step, so a stale value is visible
        પસંદગી (nextRand() % 5) {
        સ્થિતિ 0: {  // Insert a whole entry
            અચલ std::pair<typename Map::iterator, તાર્કિક> pair = map.ઉમેરો({ચાવી, val});
            UASSERT_SELFTEST(pair.second, model.ઉમેરો({ચાવી, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, ચાવી);  // At the key asked for
            તોડો;
        }
        સ્થિતિ 1: {  // Insert lazily, which must create only on a miss
            તાર્કિક created = ખોટું;
            અચલ std::pair<typename Map::iterator, તાર્કિક> pair
                = map.ઉમેરો_Lazy(ચાવી, [&]() -> Entry {
                      created = સાચું;
                      પાછા {ચાવી, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ઉમેરો({ચાવી, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, ચાવી);  // At the key asked for
            તોડો;
        }
        સ્થિતિ 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(ચાવી), model.erase(ચાવી) != 0);  // As model
            તોડો;
        }
        સ્થિતિ 3: {  // Change a value, which is erasing the entry and inserting it again
            અચલ typename Map::iterator it = map.find(ચાવી);
            જો (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.ઉમેરો({ચાવી, val}).second, સાચું);  // The key is free
                model[ચાવી] = val;
            }
            તોડો;
        }
        મૂળભૂત: {  // Look up only
            અચલ typename Map::iterator it = map.find(ચાવી);
            UASSERT_SELFTEST(it != map.end(), model.ગણતરી(ચાવી) != 0);  // As the model
            જો (it != map.end()) {
                UASSERT_SELFTEST(it->first, ચાવી);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(ચાવી));  // With its value
            }
            તોડો;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    માટે (size_t ચાવી = 0; ચાવી < UNIVERSE; ++ચાવી) {
        અચલ typename Map::iterator it = map.find(ચાવી);
        UASSERT_SELFTEST(it != map.end(), model.ગણતરી(ચાવી) != 0);  // Present iff modelled
        જો (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(ચાવી));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<તાર્કિક, UNIVERSE> seen{};
    size_t n = 0;
    માટે (અચલ Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, સાચું);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ગણતરી(entry.first) != 0, સાચું);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], ખોટું);  // Each entry once
        seen[entry.first] = સાચું;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

શૂન્ય selfTest() {
    test_કિંમત_ચાવીઓ();
    test_Pointer_ચાવીઓ();
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
