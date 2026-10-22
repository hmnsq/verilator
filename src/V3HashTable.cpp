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
constexpr माप_प्रकार maxLoad(माप_प्रकार capacity) { फर्काऊ capacity * LOAD_FACTOR_अङ्क / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr माप_प्रकार GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

शून्य test_मान_साँचोहरू() {
    संरचना मान_2 final {
        माप_प्रकार m_hash = 0;  // Hash of this entry
        माप_प्रकार m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        माप_प्रकार चालक()(अचर मान_2& मान) अचर { फर्काऊ चालक()(मान.m_hash, 0); }
        माप_प्रकार चालक()(माप_प्रकार hash, माप_प्रकार) अचर { फर्काऊ hash; }
    };

    संरचना Equal final {
        तार्किक चालक()(अचर मान_2& a, अचर मान_2& b) अचर {
            फर्काऊ चालक()(a, b.m_hash, b.m_id);
        }
        तार्किक चालक()(अचर मान_2& मान, माप_प्रकार hash, माप_प्रकार id) अचर {
            फर्काऊ मान.m_hash == hash && मान.m_id == id;
        }
    };

    using Set = V3HashSet<मान_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        अचर मान_2 मान{1, 0};
        अचर मान_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), सत्य);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), सत्य);  // So iterates nothing
        UASSERT_SELFTEST(set.find(मान) == set.end(), सत्य);  // And finds nothing
        अचर std::pair<Set::iterator, तार्किक> added = set.घुसाउ(मान);
        UASSERT_SELFTEST(added.second, सत्य);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, मान.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &मान, सत्य);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), असत्य);  // So the set is no longer empty
        {
            अचर Set::iterator it = set.find(मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            अचर Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            अचर Set::iterator it = set.find(1, माप_प्रकार{0});
            UASSERT_SELFTEST(it != set.end(), सत्य);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, माप_प्रकार{1}) == set.end(), सत्य);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, माप_प्रकार{0}) == set.end(), सत्य);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        अचर std::pair<Set::iterator, तार्किक> again = set.घुसाउ(equal);
        UASSERT_SELFTEST(again.second, असत्य);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            अचर Set::iterator it = set.find(मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), सत्य);  // Empty again
        UASSERT_SELFTEST(set.find(मान) == set.end(), सत्य);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(मान), असत्य);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(मान), असत्य);  // Nothing to erase, so says so
        set.घुसाउ(मान);
        UASSERT_SELFTEST(set.erase(equal), सत्य);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), सत्य);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.घुसाउ(मान);
        UASSERT_SELFTEST(set.erase(1, माप_प्रकार{0}), सत्य);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), सत्य);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr माप_प्रकार N = GROWS_TWICE;
        Set set;
        लागि (माप_प्रकार i = 0; i < N; ++i) {
            अचर std::pair<Set::iterator, तार्किक> added = set.घुसाउ(मान_2{7, i});
            UASSERT_SELFTEST(added.second, सत्य);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        लागि (माप_प्रकार i = 0; i < N; ++i) {
            अचर Set::iterator it = set.find(माप_प्रकार{7}, i);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<तार्किक, N> seen{};
        माप_प्रकार n = 0;
        लागि (अचर मान_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, माप_प्रकार{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], असत्य);  // Each of them exactly once
            seen[entry.m_id] = सत्य;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

शून्य test_Pointer_साँचोहरू() {
    संरचना मान_2 final {
        माप_प्रकार m_hash = 0;  // Hash of this entry
        माप_प्रकार m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        माप_प्रकार चालक()(अचर मान_2* valuep) अचर { फर्काऊ चालक()(valuep->m_hash, 0); }
        माप_प्रकार चालक()(माप_प्रकार hash, माप_प्रकार) अचर { फर्काऊ hash; }
    };

    संरचना Equal final {
        तार्किक चालक()(अचर मान_2* ap, अचर मान_2* bp) अचर {
            फर्काऊ चालक()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक चालक()(अचर मान_2* valuep, माप_प्रकार hash, माप_प्रकार id) अचर {
            फर्काऊ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<मान_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        मान_2 मान{1, 0};
        मान_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), सत्य);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), सत्य);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&मान) == set.end(), सत्य);  // And finds nothing
        अचर std::pair<Set::iterator, तार्किक> added = set.घुसाउ(&मान);
        UASSERT_SELFTEST(added.second, सत्य);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &मान);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), असत्य);  // So the set is no longer empty
        {
            अचर Set::iterator it = set.find(&मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Found by itself
            UASSERT_SELFTEST(*it, &मान);  // At the entry held
        }
        {
            अचर Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // And by an equal entry
            UASSERT_SELFTEST(*it, &मान);  // At that same entry
        }
        {
            अचर Set::iterator it = set.find(1, माप_प्रकार{0});
            UASSERT_SELFTEST(it != set.end(), सत्य);  // And by its parts
            UASSERT_SELFTEST(*it, &मान);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, माप_प्रकार{1}) == set.end(), सत्य);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, माप_प्रकार{0}) == set.end(), सत्य);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        अचर std::pair<Set::iterator, तार्किक> again = set.घुसाउ(&equal);
        UASSERT_SELFTEST(again.second, असत्य);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &मान);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            अचर Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &मान);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, असत्य);  // Not the one asked for
        }
        {
            अचर Set::iterator it = set.find(&मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // The entry is still there
            UASSERT_SELFTEST(*it, &मान);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            अचर Set::iterator it = set.find(&मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Present before the erase
            UASSERT_SELFTEST(*it, &मान);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), सत्य);  // Empty again
        UASSERT_SELFTEST(set.find(&मान) == set.end(), सत्य);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&मान), असत्य);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), सत्य);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&मान), असत्य);  // Nothing to erase, so says so
        set.घुसाउ(&मान);
        UASSERT_SELFTEST(set.erase(&equal), सत्य);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), सत्य);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&मान) == set.end(), सत्य);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.घुसाउ(&मान);
        UASSERT_SELFTEST(set.erase(1, माप_प्रकार{0}), सत्य);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), सत्य);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    लागि (माप_प्रकार erase = 0; erase < 4; ++erase) {
        लागि (अचर माप_प्रकार hash : {माप_प्रकार{0}, माप_प्रकार{7}, ~माप_प्रकार{0}}) {  // Wraps too
            std::array<मान_2, 4> मानहरू{मान_2{hash, 0}, मान_2{hash, 1},  //
                                        मान_2{hash, 2}, मान_2{hash, 3}};
            Set set;
            लागि (मान_2& मान : मानहरू) set.घुसाउ(&मान);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                अचर Set::iterator it = set.find(&मानहरू[erase]);
                UASSERT_SELFTEST(it != set.end(), सत्य);  // The one to erase is present
                UASSERT_SELFTEST(*it, &मानहरू[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&मानहरू[erase]) == set.end(), सत्य);  // That one
            लागि (माप_प्रकार i = 0; i < मानहरू.size(); ++i) {
                यदि (i == erase) जारी;
                अचर Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), सत्य);  // The others are all there
                UASSERT_SELFTEST(*it, &मानहरू[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    लागि (अचर माप_प्रकार hash : {माप_प्रकार{0}, माप_प्रकार{7}, ~माप_प्रकार{0}}) {
        std::array<मान_2, GROWS_TWICE> many{};
        Set set;
        लागि (माप_प्रकार i = 0; i < many.size(); ++i) {
            many[i] = मान_2{hash, i};
            set.घुसाउ(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        लागि (माप_प्रकार i = 0; i < many.size(); ++i) {
            अचर Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        मान_2 मान{1, 0};
        मान_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.घुसाउ(&मान);
        set.घुसाउ(&other);
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            अचर Set::iterator it = set.find(&मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // The first of the run is there
            UASSERT_SELFTEST(*it, &मान);  // And is that entry
        }
        {
            अचर Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            अचर Set::iterator it = set.find(&मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Present before the erase
            UASSERT_SELFTEST(*it, &मान);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&मान) == set.end(), सत्य);  // Namely that one
        {
            अचर Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    लागि (अचर तार्किक doReserve : {असत्य, सत्य}) {
        std::array<मान_2, 8> मानहरू{};
        Set set;
        यदि (doReserve) set.reserve(मानहरू.size());
        लागि (माप_प्रकार i = 0; i < मानहरू.size(); ++i) {
            मानहरू[i] = मान_2{static_cast<माप_प्रकार>(i * 1234567), i};
            set.घुसाउ(&मानहरू[i]);
        }
        लागि (मान_2& मान : मानहरू) {
            अचर Set::iterator it = set.find(&मान);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Each entry survives
            UASSERT_SELFTEST(*it, &मान);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), मानहरू.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr माप_प्रकार N = GROWS_TWICE;
        std::array<मान_2, N> मानहरू{};
        Set set;
        लागि (माप_प्रकार i = 0; i < N; ++i) {
            मानहरू[i] = मान_2{7, i};  // All of them hash the same
            अचर std::pair<Set::iterator, तार्किक> pair
                = set.घुसाउ_Lazy(माप_प्रकार{7}, i, [&]() -> मान_2* { फर्काऊ &मानहरू[i]; });
            UASSERT_SELFTEST(pair.second, सत्य);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &मानहरू[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        लागि (माप_प्रकार i = 0; i < N; ++i) {
            तार्किक created = असत्य;
            अचर std::pair<Set::iterator, तार्किक> pair
                = set.घुसाउ_Lazy(माप_प्रकार{7}, i, [&]() -> मान_2* {
                      created = सत्य;  // LCOV_EXCL_START
                      फर्काऊ &मानहरू[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, असत्य);  // The factory was never called
            UASSERT_SELFTEST(pair.second, असत्य);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &मानहरू[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<मान_2, 20> मानहरू{};
        std::array<तार्किक, 20> seen{};
        Set set;
        लागि (माप_प्रकार i = 0; i < मानहरू.size(); ++i) {
            मानहरू[i] = मान_2{i / 2, i};  // Pairs of them collide
            set.घुसाउ(&मानहरू[i]);
        }
        माप_प्रकार n = 0;
        लागि (मान_2* अचर valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), सत्य);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], असत्य);  // Each of them exactly once
            seen[valuep->m_id] = सत्य;
            ++n;
        }
        UASSERT_SELFTEST(n, मानहरू.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

शून्य testEntryLifetime() {
    माप_प्रकार alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    वर्ग NoDefault final {
        माप_प्रकार* m_alivep;  // Where the live entries are counted
        माप_प्रकार m_hash;  // Hash of this entry
        माप_प्रकार m_id;  // Entries with equal ids are equal

    सार्वजनिक:
        NoDefault(माप_प्रकार* alivep, माप_प्रकार hash, माप_प्रकार id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(अचर NoDefault& that)
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
        NoDefault& चालक=(अचर NoDefault&) = delete;
        NoDefault& चालक=(NoDefault&&) = delete;

        माप_प्रकार hash() अचर { फर्काऊ m_hash; }
        माप_प्रकार id() अचर { फर्काऊ m_id; }
        तार्किक चालक==(अचर NoDefault& that) अचर { फर्काऊ m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::मान,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::मान,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::मान,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::मान,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    संरचना Hash final {
        माप_प्रकार चालक()(अचर NoDefault& मान) अचर { फर्काऊ मान.hash(); }
        माप_प्रकार चालक()(माप_प्रकार hash, माप_प्रकार) अचर { फर्काऊ hash; }
    };

    संरचना Equal final {
        तार्किक चालक()(अचर NoDefault& a, अचर NoDefault& b) अचर {
            फर्काऊ चालक()(a, b.hash(), b.id());
        }
        तार्किक चालक()(अचर NoDefault& a, माप_प्रकार, माप_प्रकार id) अचर { फर्काऊ a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr माप_प्रकार N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        लागि (माप_प्रकार i = 0; i < N; ++i) {
            अचर std::pair<Set::iterator, तार्किक> added = set.घुसाउ(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, सत्य);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        लागि (माप_प्रकार i = 0; i < N; ++i) {
            अचर Set::iterator it = set.find(माप_प्रकार{7}, i);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            अचर std::pair<Set::iterator, तार्किक> dup = set.घुसाउ(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, असत्य);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            अचर Set::iterator it = set.find(माप_प्रकार{7}, माप_प्रकार{0});
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Present before the erase
            UASSERT_SELFTEST(it->id(), माप_प्रकार{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(माप_प्रकार{7}, माप_प्रकार{0}), असत्य);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        लागि (माप_प्रकार i = 1; i < N; ++i) {
            {
                अचर Set::iterator it = set.find(माप_प्रकार{7}, i);
                UASSERT_SELFTEST(it != set.end(), सत्य);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            लागि (माप_प्रकार j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(माप_प्रकार{7}, j), सत्य);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), सत्य);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        लागि (माप_प्रकार i = 0; i < N; ++i) set.घुसाउ(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), सत्य);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), सत्य);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(माप_प्रकार{7}, माप_प्रकार{0}), असत्य);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), सत्य);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        लागि (माप_प्रकार i = 0; i < N; ++i) set.घुसाउ(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), सत्य);  // Still empty
        set.घुसाउ(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

शून्य testMove() {
    माप_प्रकार alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    संरचना Counted final {
        माप_प्रकार* m_alivep;  // Where the live entries are counted
        माप_प्रकार m_id;  // Entries with equal ids are equal

        Counted(माप_प्रकार* alivep, माप_प्रकार id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(अचर Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& चालक=(अचर Counted&) = delete;
        Counted& चालक=(Counted&&) = delete;
    };

    संरचना Hash final {
        माप_प्रकार चालक()(अचर Counted& entry) अचर { फर्काऊ चालक()(entry.m_id); }
        माप_प्रकार चालक()(माप_प्रकार id) अचर { फर्काऊ id; }
    };

    संरचना Equal final {
        तार्किक चालक()(अचर Counted& a, अचर Counted& b) अचर { फर्काऊ चालक()(a, b.m_id); }
        तार्किक चालक()(अचर Counted& a, माप_प्रकार id) अचर { फर्काऊ a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr माप_प्रकार N = GROWS_TWICE;
    {
        Set set;
        लागि (माप_प्रकार i = 0; i < N; ++i) set.घुसाउ(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), सत्य);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), सत्य);  // So it iterates nothing
        लागि (माप_प्रकार i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), सत्य);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), असत्य);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.घुसाउ(Counted{&alive, N}).second, सत्य);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), असत्य);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), असत्य);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), सत्य);  // And the source is empty again
        लागि (माप_प्रकार i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), सत्य);  // The rest moved
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
        माप_प्रकार m_id;  // Entries with equal ids are equal

    सार्वजनिक:
        explicit MoveOnly(माप_प्रकार id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = स्वभाव;
        MoveOnly(अचर MoveOnly&) = delete;
        MoveOnly& चालक=(अचर MoveOnly&) = delete;
        MoveOnly& चालक=(MoveOnly&&) = delete;
        ~MoveOnly() = स्वभाव;

        माप_प्रकार id() अचर { फर्काऊ m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::मान,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::मान,
                  "SelfTest: 'MoveOnly' must be move constructible");

    संरचना Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        माप_प्रकार चालक()(अचर MoveOnly& मान) अचर { फर्काऊ चालक()(मान.id()); }
        माप_प्रकार चालक()(माप_प्रकार) अचर { फर्काऊ 7; }
    };

    संरचना Equal final {
        तार्किक चालक()(अचर MoveOnly& a, अचर MoveOnly& b) अचर {
            फर्काऊ चालक()(a, b.id());
        }
        तार्किक चालक()(अचर MoveOnly& a, माप_प्रकार id) अचर { फर्काऊ a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr माप_प्रकार N = GROWS_TWICE;
    Set set;
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर std::pair<Set::iterator, तार्किक> added
            = set.घुसाउ_Lazy(i, [i] { फर्काऊ MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, सत्य);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    लागि (माप_प्रकार i = 0; i < N; i += 2) {
        अचर Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), सत्य);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर तार्किक erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

शून्य testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    वर्ग Hash final {
        माप_प्रकार m_mask;  // Only these bits of an entry matter

    सार्वजनिक:
        explicit Hash(माप_प्रकार mask)
            : m_mask{mask} {}
        माप_प्रकार चालक()(माप_प्रकार मान) अचर { फर्काऊ मान & m_mask; }
    };

    वर्ग Equal final {
        माप_प्रकार m_mask;  // Only these bits of an entry matter

    सार्वजनिक:
        explicit Equal(माप_प्रकार mask)
            : m_mask{mask} {}
        तार्किक चालक()(माप_प्रकार a, माप_प्रकार b) अचर { फर्काऊ (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::मान,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::मान,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<माप_प्रकार, Hash, Equal> set{Hash{3}, Equal{3}};
    लागि (माप_प्रकार i = 0; i < 16; ++i) set.घुसाउ(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    लागि (माप_प्रकार i = 0; i < 16; ++i) {
        अचर V3HashSet<माप_प्रकार, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), सत्य);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

शून्य testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    संरचना मान_2 final {
        माप_प्रकार m_hash = 0;  // Hash of this entry
        माप_प्रकार m_id = 0;  // Entries with equal ids are equal
    };

    संरचना Hash final {
        माप_प्रकार चालक()(अचर मान_2* valuep) अचर { फर्काऊ चालक()(valuep->m_hash, 0); }
        माप_प्रकार चालक()(माप_प्रकार hash, माप_प्रकार) अचर { फर्काऊ hash; }
    };

    संरचना Equal final {
        तार्किक चालक()(अचर मान_2* ap, अचर मान_2* bp) अचर {
            फर्काऊ चालक()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक चालक()(अचर मान_2* valuep, माप_प्रकार hash, माप_प्रकार id) अचर {
            फर्काऊ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<मान_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    लागि (अचर माप_प्रकार h : {माप_प्रकार{0}, माप_प्रकार{5}, ~माप_प्रकार{0}}) {  // Wraps too
        std::array<मान_2, 4> मानहरू{मान_2{h, 0}, मान_2{h + 1, 1},  //
                                    मान_2{h, 2}, मान_2{h + 1, 3}};
        Set set;
        लागि (मान_2& मान : मानहरू) set.घुसाउ(&मान);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            अचर Set::iterator it = set.find(h, माप_प्रकार{0});
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, माप_प्रकार{0}), असत्य);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        लागि (माप_प्रकार i = 1; i < मानहरू.size(); ++i) {
            अचर Set::iterator it = set.find(मानहरू[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &मानहरू[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            अचर Set::iterator it = set.find(h + 1, माप_प्रकार{1});
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        लागि (माप_प्रकार i = 2; i < मानहरू.size(); ++i) {
            अचर Set::iterator it = set.find(मानहरू[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), सत्य);  // Both still reachable
            UASSERT_SELFTEST(*it, &मानहरू[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<तार्किक, 4> seen{};
        लागि (अचर मान_2* अचर valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], असत्य);  // Each entry once
            seen[valuep->m_id] = सत्य;
        }
        UASSERT_SELFTEST(seen[0], असत्य);  // Erased first
        UASSERT_SELFTEST(seen[1], असत्य);  // Erased second
        UASSERT_SELFTEST(seen[2], सत्य);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], सत्य);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

शून्य testReferenceStability() {
    संरचना मान_2 final {
        माप_प्रकार m_hash = 0;  // Hash of this entry
        माप_प्रकार m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        माप_प्रकार चालक()(अचर मान_2* valuep) अचर { फर्काऊ चालक()(valuep->m_hash, 0); }
        माप_प्रकार चालक()(माप_प्रकार hash, माप_प्रकार) अचर { फर्काऊ hash; }
    };

    संरचना Equal final {
        तार्किक चालक()(अचर मान_2* ap, अचर मान_2* bp) अचर {
            फर्काऊ चालक()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक चालक()(अचर मान_2* valuep, माप_प्रकार hash, माप_प्रकार id) अचर {
            फर्काऊ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<मान_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(माप_प्रकार{7}, माप_प्रकार{0}), असत्य);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), सत्य);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), सत्य);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr माप_प्रकार N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<मान_2, N> मानहरू{};
    std::array<मान_2* अचर*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        मानहरू[i] = मान_2{7, i};
        अचर std::pair<Set::iterator, तार्किक> pair = set.घुसाउ(&मानहरू[i]);
        UASSERT_SELFTEST(pair.second, सत्य);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर Set::iterator it = set.find(माप_प्रकार{7}, i);
        UASSERT_SELFTEST(it != set.end(), सत्य);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर std::pair<Set::iterator, तार्किक> pair = set.घुसाउ(&मानहरू[i]);
        UASSERT_SELFTEST(pair.second, असत्य);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<मान_2* अचर*, N> grownpps{};  // Where each entry is after the growth
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर Set::iterator it = set.find(माप_प्रकार{7}, i);
        UASSERT_SELFTEST(it != set.end(), सत्य);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर Set::iterator it = set.find(माप_प्रकार{7}, i);
        UASSERT_SELFTEST(it != set.end(), सत्य);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
संरचना ClusteredHash final {
    माप_प्रकार चालक()(माप_प्रकार मान) अचर { फर्काऊ मान & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
संरचना SpreadHash final {
    माप_प्रकार चालक()(माप_प्रकार मान) अचर {
        फर्काऊ static_cast<माप_प्रकार>(मान * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
ढाँचा <typename T_Hash>
शून्य testSetAgainstModel() {
    संरचना Equal final {
        तार्किक चालक()(माप_प्रकार a, माप_प्रकार b) अचर { फर्काऊ a == b; }
    };

    using Set = V3HashSet<माप_प्रकार, T_Hash, Equal>;

    constexpr माप_प्रकार UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr माप_प्रकार STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    अचर स्वतः nextRand = [&state]() -> माप_प्रकार {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        फर्काऊ static_cast<माप_प्रकार>(state >> 32);
    };

    Set set;
    std::set<माप_प्रकार> model;

    लागि (माप_प्रकार step = 0; step < STEPS; ++step) {
        अचर माप_प्रकार id = nextRand() % UNIVERSE;
        विकल्प (nextRand() % 4) {
        पक्ष 0: {  // Insert a copy
            अचर std::pair<typename Set::iterator, तार्किक> pair = set.घुसाउ(id);
            UASSERT_SELFTEST(pair.second, model.घुसाउ(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            रोक;
        }
        पक्ष 1: {  // Insert lazily, which must create only on a miss
            तार्किक created = असत्य;
            अचर std::pair<typename Set::iterator, तार्किक> pair
                = set.घुसाउ_Lazy(id, [&]() -> माप_प्रकार {
                      created = सत्य;
                      फर्काऊ id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.घुसाउ(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            रोक;
        }
        पक्ष 2: {  // Erase, if present
            अचर typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.गन्ती(id) != 0);  // As the model
            यदि (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            रोक;
        }
        स्वभाव: {  // Look up only
            अचर typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.गन्ती(id) != 0);  // As the model
            यदि (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            रोक;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    लागि (माप_प्रकार id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.गन्ती(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<तार्किक, UNIVERSE> seen{};
    माप_प्रकार n = 0;
    लागि (अचर माप_प्रकार entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, सत्य);  // Nothing out of thin air
        UASSERT_SELFTEST(model.गन्ती(entry) != 0, सत्य);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], असत्य);  // Each entry once
        seen[entry] = सत्य;
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
    static_assert(std::is_same<Entry, std::pair<std::string, पूर्णाङ्क>>::मान,
                  "SelfTest: a map entry must be a plain pair");

    constexpr माप_प्रकार N = GROWS_TWICE;
    Map map;
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर std::string साँचो = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        अचर std::pair<Map::iterator, तार्किक> pair
            = map.घुसाउ_Lazy(साँचो, [&]() -> Entry { फर्काऊ {साँचो, static_cast<पूर्णाङ्क>(i)}; });
        UASSERT_SELFTEST(pair.second, सत्य);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), सत्य);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर std::string साँचो = "key" + std::to_string(i);
        map.erase(map.find(साँचो));
        UASSERT_SELFTEST(map.घुसाउ({साँचो, static_cast<पूर्णाङ्क>(i) + 100}).second, सत्य);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), सत्य);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    लागि (माप_प्रकार i = 0; i < N; ++i) map.find("key" + std::to_string(i)).मान() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    लागि (माप_प्रकार i = 0; i < N; ++i) {
        अचर Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), सत्य);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    लागि (माप_प्रकार i = 0; i < N; ++i) map.find("key" + std::to_string(i)).मान() -= 100;

    // Adding a key that is present changes nothing
    {
        अचर std::string साँचो = "key0";
        तार्किक created = असत्य;
        अचर std::pair<Map::iterator, तार्किक> pair = map.घुसाउ_Lazy(साँचो, [&]() -> Entry {
            created = सत्य;  // LCOV_EXCL_START
            फर्काऊ {साँचो, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, असत्य);  // The factory was never called
        UASSERT_SELFTEST(pair.second, असत्य);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        अचर std::pair<Map::iterator, तार्किक> added = map.घुसाउ({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, सत्य);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        अचर std::pair<Map::iterator, तार्किक> again = map.घुसाउ({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, असत्य);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        अचर Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), सत्य);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), असत्य);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), सत्य);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), असत्य);  // Namely that one
    लागि (माप_प्रकार i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), सत्य);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
ढाँचा <typename T_Hash>
शून्य testMapAgainstModel() {
    संरचना Equal final {
        तार्किक चालक()(माप_प्रकार a, माप_प्रकार b) अचर { फर्काऊ a == b; }
    };

    using Map = V3HashMap<माप_प्रकार, माप_प्रकार, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr माप_प्रकार UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr माप_प्रकार STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    अचर स्वतः nextRand = [&state]() -> माप_प्रकार {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        फर्काऊ static_cast<माप_प्रकार>(state >> 32);
    };

    Map map;
    std::map<माप_प्रकार, माप_प्रकार> model;

    लागि (माप_प्रकार step = 0; step < STEPS; ++step) {
        अचर माप_प्रकार साँचो = nextRand() % UNIVERSE;
        अचर माप_प्रकार val = step;  // Distinct every step, so a stale value is visible
        विकल्प (nextRand() % 5) {
        पक्ष 0: {  // Insert a whole entry
            अचर std::pair<typename Map::iterator, तार्किक> pair = map.घुसाउ({साँचो, val});
            UASSERT_SELFTEST(pair.second, model.घुसाउ({साँचो, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, साँचो);  // At the key asked for
            रोक;
        }
        पक्ष 1: {  // Insert lazily, which must create only on a miss
            तार्किक created = असत्य;
            अचर std::pair<typename Map::iterator, तार्किक> pair
                = map.घुसाउ_Lazy(साँचो, [&]() -> Entry {
                      created = सत्य;
                      फर्काऊ {साँचो, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.घुसाउ({साँचो, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, साँचो);  // At the key asked for
            रोक;
        }
        पक्ष 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(साँचो), model.erase(साँचो) != 0);  // As model
            रोक;
        }
        पक्ष 3: {  // Change a value, which is erasing the entry and inserting it again
            अचर typename Map::iterator it = map.find(साँचो);
            यदि (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.घुसाउ({साँचो, val}).second, सत्य);  // The key is free
                model[साँचो] = val;
            }
            रोक;
        }
        स्वभाव: {  // Look up only
            अचर typename Map::iterator it = map.find(साँचो);
            UASSERT_SELFTEST(it != map.end(), model.गन्ती(साँचो) != 0);  // As the model
            यदि (it != map.end()) {
                UASSERT_SELFTEST(it->first, साँचो);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(साँचो));  // With its value
            }
            रोक;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    लागि (माप_प्रकार साँचो = 0; साँचो < UNIVERSE; ++साँचो) {
        अचर typename Map::iterator it = map.find(साँचो);
        UASSERT_SELFTEST(it != map.end(), model.गन्ती(साँचो) != 0);  // Present iff modelled
        यदि (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(साँचो));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<तार्किक, UNIVERSE> seen{};
    माप_प्रकार n = 0;
    लागि (अचर Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, सत्य);  // Nothing out of thin air
        UASSERT_SELFTEST(model.गन्ती(entry.first) != 0, सत्य);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], असत्य);  // Each entry once
        seen[entry.first] = सत्य;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

शून्य selfTest() {
    test_मान_साँचोहरू();
    test_Pointer_साँचोहरू();
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
