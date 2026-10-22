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

#ಅನ್ತರ್ಭಾವ "V3HashTable.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <utility>

ನಾಮಾಕಾಶ V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr size_t maxLoad(size_t capacity) { ಪ್ರತ್ಯಾಗಮ capacity * LOAD_FACTOR_ಅಂಕೆ / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr size_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

ಶೂನ್ಯ test_ಮೌಲ್ಯ_ಕೀಲಿಗಳು() {
    ಸಂರಚನಾ ಮೌಲ್ಯ_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ಸಂರಚನಾ Hash final {
        size_t operator()(ಧ್ರುವ ಮೌಲ್ಯ_2& ಮೌಲ್ಯ) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ operator()(ಮೌಲ್ಯ.m_hash, 0); }
        size_t operator()(size_t hash, size_t) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ hash; }
    };

    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2& a, ಧ್ರುವ ಮೌಲ್ಯ_2& b) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(a, b.m_hash, b.m_id);
        }
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2& ಮೌಲ್ಯ, size_t hash, size_t id) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ.m_hash == hash && ಮೌಲ್ಯ.m_id == id;
        }
    };

    using Set = V3HashSet<ಮೌಲ್ಯ_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ಧ್ರುವ ಮೌಲ್ಯ_2 ಮೌಲ್ಯ{1, 0};
        ಧ್ರುವ ಮೌಲ್ಯ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), ಸತ್ಯ);  // So iterates nothing
        UASSERT_SELFTEST(set.find(ಮೌಲ್ಯ) == set.end(), ಸತ್ಯ);  // And finds nothing
        ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> added = set.ಸೇರಿಸು(ಮೌಲ್ಯ);
        UASSERT_SELFTEST(added.second, ಸತ್ಯ);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, ಮೌಲ್ಯ.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &ಮೌಲ್ಯ, ಸತ್ಯ);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), ಅಸತ್ಯ);  // So the set is no longer empty
        {
            ಧ್ರುವ Set::iterator it = set.find(ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            ಧ್ರುವ Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            ಧ್ರುವ Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), ಸತ್ಯ);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), ಸತ್ಯ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> again = set.ಸೇರಿಸು(equal);
        UASSERT_SELFTEST(again.second, ಅಸತ್ಯ);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ಧ್ರುವ Set::iterator it = set.find(ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Empty again
        UASSERT_SELFTEST(set.find(ಮೌಲ್ಯ) == set.end(), ಸತ್ಯ);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(ಮೌಲ್ಯ), ಅಸತ್ಯ);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(ಮೌಲ್ಯ), ಅಸತ್ಯ);  // Nothing to erase, so says so
        set.ಸೇರಿಸು(ಮೌಲ್ಯ);
        UASSERT_SELFTEST(set.erase(equal), ಸತ್ಯ);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.ಸೇರಿಸು(ಮೌಲ್ಯ);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), ಸತ್ಯ);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
            ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> added = set.ಸೇರಿಸು(ಮೌಲ್ಯ_2{7, i});
            UASSERT_SELFTEST(added.second, ಸತ್ಯ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
            ಧ್ರುವ Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<ತಾರ್ಕಿಕ, N> seen{};
        size_t n = 0;
        ಪ್ರತಿ (ಧ್ರುವ ಮೌಲ್ಯ_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], ಅಸತ್ಯ);  // Each of them exactly once
            seen[entry.m_id] = ಸತ್ಯ;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

ಶೂನ್ಯ test_Pointer_ಕೀಲಿಗಳು() {
    ಸಂರಚನಾ ಮೌಲ್ಯ_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ಸಂರಚನಾ Hash final {
        size_t operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* valuep) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ hash; }
    };

    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* ap, ಧ್ರುವ ಮೌಲ್ಯ_2* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(ap, bp->m_hash, bp->m_id);
        }
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* valuep, size_t hash, size_t id) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<ಮೌಲ್ಯ_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ಮೌಲ್ಯ_2 ಮೌಲ್ಯ{1, 0};
        ಮೌಲ್ಯ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), ಸತ್ಯ);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&ಮೌಲ್ಯ) == set.end(), ಸತ್ಯ);  // And finds nothing
        ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> added = set.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(added.second, ಸತ್ಯ);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &ಮೌಲ್ಯ);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), ಅಸತ್ಯ);  // So the set is no longer empty
        {
            ಧ್ರುವ Set::iterator it = set.find(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Found by itself
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // At the entry held
        }
        {
            ಧ್ರುವ Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // And by an equal entry
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // At that same entry
        }
        {
            ಧ್ರುವ Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // And by its parts
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), ಸತ್ಯ);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), ಸತ್ಯ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> again = set.ಸೇರಿಸು(&equal);
        UASSERT_SELFTEST(again.second, ಅಸತ್ಯ);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &ಮೌಲ್ಯ);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ಧ್ರುವ Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, ಅಸತ್ಯ);  // Not the one asked for
        }
        {
            ಧ್ರುವ Set::iterator it = set.find(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // The entry is still there
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ಧ್ರುವ Set::iterator it = set.find(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Present before the erase
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Empty again
        UASSERT_SELFTEST(set.find(&ಮೌಲ್ಯ) == set.end(), ಸತ್ಯ);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&ಮೌಲ್ಯ), ಅಸತ್ಯ);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&ಮೌಲ್ಯ), ಅಸತ್ಯ);  // Nothing to erase, so says so
        set.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(set.erase(&equal), ಸತ್ಯ);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&ಮೌಲ್ಯ) == set.end(), ಸತ್ಯ);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), ಸತ್ಯ);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    ಪ್ರತಿ (size_t erase = 0; erase < 4; ++erase) {
        ಪ್ರತಿ (ಧ್ರುವ size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<ಮೌಲ್ಯ_2, 4> ಮೌಲ್ಯಗಳು{ಮೌಲ್ಯ_2{hash, 0}, ಮೌಲ್ಯ_2{hash, 1},  //
                                        ಮೌಲ್ಯ_2{hash, 2}, ಮೌಲ್ಯ_2{hash, 3}};
            Set set;
            ಪ್ರತಿ (ಮೌಲ್ಯ_2& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) set.ಸೇರಿಸು(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                ಧ್ರುವ Set::iterator it = set.find(&ಮೌಲ್ಯಗಳು[erase]);
                UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // The one to erase is present
                UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&ಮೌಲ್ಯಗಳು[erase]) == set.end(), ಸತ್ಯ);  // That one
            ಪ್ರತಿ (size_t i = 0; i < ಮೌಲ್ಯಗಳು.size(); ++i) {
                ಯದಿ (i == erase) ಅನುವರ್ತ;
                ಧ್ರುವ Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // The others are all there
                UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    ಪ್ರತಿ (ಧ್ರುವ size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<ಮೌಲ್ಯ_2, GROWS_TWICE> many{};
        Set set;
        ಪ್ರತಿ (size_t i = 0; i < many.size(); ++i) {
            many[i] = ಮೌಲ್ಯ_2{hash, i};
            set.ಸೇರಿಸು(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        ಪ್ರತಿ (size_t i = 0; i < many.size(); ++i) {
            ಧ್ರುವ Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        ಮೌಲ್ಯ_2 ಮೌಲ್ಯ{1, 0};
        ಮೌಲ್ಯ_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        set.ಸೇರಿಸು(&other);
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            ಧ್ರುವ Set::iterator it = set.find(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // The first of the run is there
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is that entry
        }
        {
            ಧ್ರುವ Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ಧ್ರುವ Set::iterator it = set.find(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Present before the erase
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&ಮೌಲ್ಯ) == set.end(), ಸತ್ಯ);  // Namely that one
        {
            ಧ್ರುವ Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    ಪ್ರತಿ (ಧ್ರುವ ತಾರ್ಕಿಕ doReserve : {ಅಸತ್ಯ, ಸತ್ಯ}) {
        std::array<ಮೌಲ್ಯ_2, 8> ಮೌಲ್ಯಗಳು{};
        Set set;
        ಯದಿ (doReserve) set.reserve(ಮೌಲ್ಯಗಳು.size());
        ಪ್ರತಿ (size_t i = 0; i < ಮೌಲ್ಯಗಳು.size(); ++i) {
            ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{static_cast<size_t>(i * 1234567), i};
            set.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        }
        ಪ್ರತಿ (ಮೌಲ್ಯ_2& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) {
            ಧ್ರುವ Set::iterator it = set.find(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Each entry survives
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), ಮೌಲ್ಯಗಳು.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<ಮೌಲ್ಯ_2, N> ಮೌಲ್ಯಗಳು{};
        Set set;
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
            ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{7, i};  // All of them hash the same
            ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> pair
                = set.ಸೇರಿಸು_Lazy(size_t{7}, i, [&]() -> ಮೌಲ್ಯ_2* { ಪ್ರತ್ಯಾಗಮ &ಮೌಲ್ಯಗಳು[i]; });
            UASSERT_SELFTEST(pair.second, ಸತ್ಯ);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &ಮೌಲ್ಯಗಳು[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
            ತಾರ್ಕಿಕ created = ಅಸತ್ಯ;
            ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> pair
                = set.ಸೇರಿಸು_Lazy(size_t{7}, i, [&]() -> ಮೌಲ್ಯ_2* {
                      created = ಸತ್ಯ;  // LCOV_EXCL_START
                      ಪ್ರತ್ಯಾಗಮ &ಮೌಲ್ಯಗಳು[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, ಅಸತ್ಯ);  // The factory was never called
            UASSERT_SELFTEST(pair.second, ಅಸತ್ಯ);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &ಮೌಲ್ಯಗಳು[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<ಮೌಲ್ಯ_2, 20> ಮೌಲ್ಯಗಳು{};
        std::array<ತಾರ್ಕಿಕ, 20> seen{};
        Set set;
        ಪ್ರತಿ (size_t i = 0; i < ಮೌಲ್ಯಗಳು.size(); ++i) {
            ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{i / 2, i};  // Pairs of them collide
            set.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        }
        size_t n = 0;
        ಪ್ರತಿ (ಮೌಲ್ಯ_2* ಧ್ರುವ valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), ಸತ್ಯ);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], ಅಸತ್ಯ);  // Each of them exactly once
            seen[valuep->m_id] = ಸತ್ಯ;
            ++n;
        }
        UASSERT_SELFTEST(n, ಮೌಲ್ಯಗಳು.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

ಶೂನ್ಯ testEntryLifetime() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    ವರ್ಗ NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    ಸಾರ್ವಜನಿಕ:
        NoDefault(size_t* alivep, size_t hash, size_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ಧ್ರುವ NoDefault& that)
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
        NoDefault& operator=(ಧ್ರುವ NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hash; }
        size_t id() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_id; }
        ತಾರ್ಕಿಕ operator==(ಧ್ರುವ NoDefault& that) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    ಸಂರಚನಾ Hash final {
        size_t operator()(ಧ್ರುವ NoDefault& ಮೌಲ್ಯ) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ.hash(); }
        size_t operator()(size_t hash, size_t) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ hash; }
    };

    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ NoDefault& a, ಧ್ರುವ NoDefault& b) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(a, b.hash(), b.id());
        }
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ NoDefault& a, size_t, size_t id) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
            ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> added = set.ಸೇರಿಸು(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, ಸತ್ಯ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
            ಧ್ರುವ Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> dup = set.ಸೇರಿಸು(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, ಅಸತ್ಯ);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ಧ್ರುವ Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), ಅಸತ್ಯ);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        ಪ್ರತಿ (size_t i = 1; i < N; ++i) {
            {
                ಧ್ರುವ Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            ಪ್ರತಿ (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), ಸತ್ಯ);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), ಸತ್ಯ);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) set.ಸೇರಿಸು(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), ಸತ್ಯ);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), ಅಸತ್ಯ);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) set.ಸೇರಿಸು(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // Still empty
        set.ಸೇರಿಸು(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

ಶೂನ್ಯ testMove() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    ಸಂರಚನಾ Counted final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_id;  // Entries with equal ids are equal

        Counted(size_t* alivep, size_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ಧ್ರುವ Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(ಧ್ರುವ Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    ಸಂರಚನಾ Hash final {
        size_t operator()(ಧ್ರುವ Counted& entry) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ operator()(entry.m_id); }
        size_t operator()(size_t id) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ id; }
    };

    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ Counted& a, ಧ್ರುವ Counted& b) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ operator()(a, b.m_id); }
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ Counted& a, size_t id) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) set.ಸೇರಿಸು(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), ಸತ್ಯ);  // So it iterates nothing
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), ಸತ್ಯ);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), ಅಸತ್ಯ);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.ಸೇರಿಸು(Counted{&alive, N}).second, ಸತ್ಯ);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), ಅಸತ್ಯ);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), ಅಸತ್ಯ);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), ಸತ್ಯ);  // And the source is empty again
        ಪ್ರತಿ (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), ಸತ್ಯ);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

ಶೂನ್ಯ testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    ವರ್ಗ MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    ಸಾರ್ವಜನಿಕ:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = ಸ್ವಭಾವ;
        MoveOnly(ಧ್ರುವ MoveOnly&) = delete;
        MoveOnly& operator=(ಧ್ರುವ MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = ಸ್ವಭಾವ;

        size_t id() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::ಮೌಲ್ಯ,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::ಮೌಲ್ಯ,
                  "SelfTest: 'MoveOnly' must be move constructible");

    ಸಂರಚನಾ Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(ಧ್ರುವ MoveOnly& ಮೌಲ್ಯ) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ operator()(ಮೌಲ್ಯ.id()); }
        size_t operator()(size_t) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ 7; }
    };

    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ MoveOnly& a, ಧ್ರುವ MoveOnly& b) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(a, b.id());
        }
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ MoveOnly& a, size_t id) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> added
            = set.ಸೇರಿಸು_Lazy(i, [i] { ಪ್ರತ್ಯಾಗಮ MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, ಸತ್ಯ);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    ಪ್ರತಿ (size_t i = 0; i < N; i += 2) {
        ಧ್ರುವ Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ ತಾರ್ಕಿಕ erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

ಶೂನ್ಯ testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    ವರ್ಗ Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    ಸಾರ್ವಜನಿಕ:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t ಮೌಲ್ಯ) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ & m_mask; }
    };

    ವರ್ಗ Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    ಸಾರ್ವಜನಿಕ:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        ತಾರ್ಕಿಕ operator()(size_t a, size_t b) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::ಮೌಲ್ಯ,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::ಮೌಲ್ಯ,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    ಪ್ರತಿ (size_t i = 0; i < 16; ++i) set.ಸೇರಿಸು(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    ಪ್ರತಿ (size_t i = 0; i < 16; ++i) {
        ಧ್ರುವ V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

ಶೂನ್ಯ testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    ಸಂರಚನಾ ಮೌಲ್ಯ_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Entries with equal ids are equal
    };

    ಸಂರಚನಾ Hash final {
        size_t operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* valuep) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ hash; }
    };

    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* ap, ಧ್ರುವ ಮೌಲ್ಯ_2* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(ap, bp->m_hash, bp->m_id);
        }
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* valuep, size_t hash, size_t id) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<ಮೌಲ್ಯ_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    ಪ್ರತಿ (ಧ್ರುವ size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<ಮೌಲ್ಯ_2, 4> ಮೌಲ್ಯಗಳು{ಮೌಲ್ಯ_2{h, 0}, ಮೌಲ್ಯ_2{h + 1, 1},  //
                                    ಮೌಲ್ಯ_2{h, 2}, ಮೌಲ್ಯ_2{h + 1, 3}};
        Set set;
        ಪ್ರತಿ (ಮೌಲ್ಯ_2& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) set.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ಧ್ರುವ Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), ಅಸತ್ಯ);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        ಪ್ರತಿ (size_t i = 1; i < ಮೌಲ್ಯಗಳು.size(); ++i) {
            ಧ್ರುವ Set::iterator it = set.find(ಮೌಲ್ಯಗಳು[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ಧ್ರುವ Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        ಪ್ರತಿ (size_t i = 2; i < ಮೌಲ್ಯಗಳು.size(); ++i) {
            ಧ್ರುವ Set::iterator it = set.find(ಮೌಲ್ಯಗಳು[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Both still reachable
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<ತಾರ್ಕಿಕ, 4> seen{};
        ಪ್ರತಿ (ಧ್ರುವ ಮೌಲ್ಯ_2* ಧ್ರುವ valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], ಅಸತ್ಯ);  // Each entry once
            seen[valuep->m_id] = ಸತ್ಯ;
        }
        UASSERT_SELFTEST(seen[0], ಅಸತ್ಯ);  // Erased first
        UASSERT_SELFTEST(seen[1], ಅಸತ್ಯ);  // Erased second
        UASSERT_SELFTEST(seen[2], ಸತ್ಯ);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], ಸತ್ಯ);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

ಶೂನ್ಯ testReferenceStability() {
    ಸಂರಚನಾ ಮೌಲ್ಯ_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ಸಂರಚನಾ Hash final {
        size_t operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* valuep) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ hash; }
    };

    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* ap, ಧ್ರುವ ಮೌಲ್ಯ_2* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(ap, bp->m_hash, bp->m_id);
        }
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ಮೌಲ್ಯ_2* valuep, size_t hash, size_t id) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<ಮೌಲ್ಯ_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), ಅಸತ್ಯ);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), ಸತ್ಯ);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), ಸತ್ಯ);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<ಮೌಲ್ಯ_2, N> ಮೌಲ್ಯಗಳು{};
    std::array<ಮೌಲ್ಯ_2* ಧ್ರುವ*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{7, i};
        ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> pair = set.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        UASSERT_SELFTEST(pair.second, ಸತ್ಯ);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ std::pair<Set::iterator, ತಾರ್ಕಿಕ> pair = set.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        UASSERT_SELFTEST(pair.second, ಅಸತ್ಯ);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<ಮೌಲ್ಯ_2* ಧ್ರುವ*, N> grownpps{};  // Where each entry is after the growth
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), ಸತ್ಯ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
ಸಂರಚನಾ ClusteredHash final {
    size_t operator()(size_t ಮೌಲ್ಯ) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
ಸಂರಚನಾ SpreadHash final {
    size_t operator()(size_t ಮೌಲ್ಯ) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ static_cast<size_t>(ಮೌಲ್ಯ * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
ಶೂನ್ಯ testSetAgainstModel() {
    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(size_t a, size_t b) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ಧ್ರುವ auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ಪ್ರತ್ಯಾಗಮ static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    ಪ್ರತಿ (size_t step = 0; step < STEPS; ++step) {
        ಧ್ರುವ size_t id = nextRand() % UNIVERSE;
        ವಿಕಲ್ಪ (nextRand() % 4) {
        ಪಕ್ಷ 0: {  // Insert a copy
            ಧ್ರುವ std::pair<typename Set::iterator, ತಾರ್ಕಿಕ> pair = set.ಸೇರಿಸು(id);
            UASSERT_SELFTEST(pair.second, model.ಸೇರಿಸು(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 1: {  // Insert lazily, which must create only on a miss
            ತಾರ್ಕಿಕ created = ಅಸತ್ಯ;
            ಧ್ರುವ std::pair<typename Set::iterator, ತಾರ್ಕಿಕ> pair
                = set.ಸೇರಿಸು_Lazy(id, [&]() -> size_t {
                      created = ಸತ್ಯ;
                      ಪ್ರತ್ಯಾಗಮ id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ಸೇರಿಸು(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 2: {  // Erase, if present
            ಧ್ರುವ typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.ಎಣಿಕೆ(id) != 0);  // As the model
            ಯದಿ (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            ಭಙ್ಗ;
        }
        ಸ್ವಭಾವ: {  // Look up only
            ಧ್ರುವ typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.ಎಣಿಕೆ(id) != 0);  // As the model
            ಯದಿ (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            ಭಙ್ಗ;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    ಪ್ರತಿ (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.ಎಣಿಕೆ(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<ತಾರ್ಕಿಕ, UNIVERSE> seen{};
    size_t n = 0;
    ಪ್ರತಿ (ಧ್ರುವ size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, ಸತ್ಯ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ಎಣಿಕೆ(entry) != 0, ಸತ್ಯ);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], ಅಸತ್ಯ);  // Each entry once
        seen[entry] = ಸತ್ಯ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

ಶೂನ್ಯ testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, ಪೂರ್ಣಾಙ್ಕ>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, ಪೂರ್ಣಾಙ್ಕ>>::ಮೌಲ್ಯ,
                  "SelfTest: a map entry must be a plain pair");

    constexpr size_t N = GROWS_TWICE;
    Map map;
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ std::string ಕೀಲಿ = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ಧ್ರುವ std::pair<Map::iterator, ತಾರ್ಕಿಕ> pair
            = map.ಸೇರಿಸು_Lazy(ಕೀಲಿ, [&]() -> Entry { ಪ್ರತ್ಯಾಗಮ {ಕೀಲಿ, static_cast<ಪೂರ್ಣಾಙ್ಕ>(i)}; });
        UASSERT_SELFTEST(pair.second, ಸತ್ಯ);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), ಸತ್ಯ);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<ಪೂರ್ಣಾಙ್ಕ>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ std::string ಕೀಲಿ = "key" + std::to_string(i);
        map.erase(map.find(ಕೀಲಿ));
        UASSERT_SELFTEST(map.ಸೇರಿಸು({ಕೀಲಿ, static_cast<ಪೂರ್ಣಾಙ್ಕ>(i) + 100}).second, ಸತ್ಯ);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), ಸತ್ಯ);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<ಪೂರ್ಣಾಙ್ಕ>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).ಮೌಲ್ಯ() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) {
        ಧ್ರುವ Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), ಸತ್ಯ);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<ಪೂರ್ಣಾಙ್ಕ>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    ಪ್ರತಿ (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).ಮೌಲ್ಯ() -= 100;

    // Adding a key that is present changes nothing
    {
        ಧ್ರುವ std::string ಕೀಲಿ = "key0";
        ತಾರ್ಕಿಕ created = ಅಸತ್ಯ;
        ಧ್ರುವ std::pair<Map::iterator, ತಾರ್ಕಿಕ> pair = map.ಸೇರಿಸು_Lazy(ಕೀಲಿ, [&]() -> Entry {
            created = ಸತ್ಯ;  // LCOV_EXCL_START
            ಪ್ರತ್ಯಾಗಮ {ಕೀಲಿ, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, ಅಸತ್ಯ);  // The factory was never called
        UASSERT_SELFTEST(pair.second, ಅಸತ್ಯ);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ಧ್ರುವ std::pair<Map::iterator, ತಾರ್ಕಿಕ> added = map.ಸೇರಿಸು({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, ಸತ್ಯ);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        ಧ್ರುವ std::pair<Map::iterator, ತಾರ್ಕಿಕ> again = map.ಸೇರಿಸು({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, ಅಸತ್ಯ);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        ಧ್ರುವ Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), ಸತ್ಯ);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), ಅಸತ್ಯ);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), ಸತ್ಯ);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), ಅಸತ್ಯ);  // Namely that one
    ಪ್ರತಿ (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), ಸತ್ಯ);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
ಶೂನ್ಯ testMapAgainstModel() {
    ಸಂರಚನಾ Equal final {
        ತಾರ್ಕಿಕ operator()(size_t a, size_t b) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ಧ್ರುವ auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ಪ್ರತ್ಯಾಗಮ static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    ಪ್ರತಿ (size_t step = 0; step < STEPS; ++step) {
        ಧ್ರುವ size_t ಕೀಲಿ = nextRand() % UNIVERSE;
        ಧ್ರುವ size_t val = step;  // Distinct every step, so a stale value is visible
        ವಿಕಲ್ಪ (nextRand() % 5) {
        ಪಕ್ಷ 0: {  // Insert a whole entry
            ಧ್ರುವ std::pair<typename Map::iterator, ತಾರ್ಕಿಕ> pair = map.ಸೇರಿಸು({ಕೀಲಿ, val});
            UASSERT_SELFTEST(pair.second, model.ಸೇರಿಸು({ಕೀಲಿ, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, ಕೀಲಿ);  // At the key asked for
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 1: {  // Insert lazily, which must create only on a miss
            ತಾರ್ಕಿಕ created = ಅಸತ್ಯ;
            ಧ್ರುವ std::pair<typename Map::iterator, ತಾರ್ಕಿಕ> pair
                = map.ಸೇರಿಸು_Lazy(ಕೀಲಿ, [&]() -> Entry {
                      created = ಸತ್ಯ;
                      ಪ್ರತ್ಯಾಗಮ {ಕೀಲಿ, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.ಸೇರಿಸು({ಕೀಲಿ, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, ಕೀಲಿ);  // At the key asked for
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(ಕೀಲಿ), model.erase(ಕೀಲಿ) != 0);  // As model
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 3: {  // Change a value, which is erasing the entry and inserting it again
            ಧ್ರುವ typename Map::iterator it = map.find(ಕೀಲಿ);
            ಯದಿ (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.ಸೇರಿಸು({ಕೀಲಿ, val}).second, ಸತ್ಯ);  // The key is free
                model[ಕೀಲಿ] = val;
            }
            ಭಙ್ಗ;
        }
        ಸ್ವಭಾವ: {  // Look up only
            ಧ್ರುವ typename Map::iterator it = map.find(ಕೀಲಿ);
            UASSERT_SELFTEST(it != map.end(), model.ಎಣಿಕೆ(ಕೀಲಿ) != 0);  // As the model
            ಯದಿ (it != map.end()) {
                UASSERT_SELFTEST(it->first, ಕೀಲಿ);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(ಕೀಲಿ));  // With its value
            }
            ಭಙ್ಗ;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    ಪ್ರತಿ (size_t ಕೀಲಿ = 0; ಕೀಲಿ < UNIVERSE; ++ಕೀಲಿ) {
        ಧ್ರುವ typename Map::iterator it = map.find(ಕೀಲಿ);
        UASSERT_SELFTEST(it != map.end(), model.ಎಣಿಕೆ(ಕೀಲಿ) != 0);  // Present iff modelled
        ಯದಿ (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(ಕೀಲಿ));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<ತಾರ್ಕಿಕ, UNIVERSE> seen{};
    size_t n = 0;
    ಪ್ರತಿ (ಧ್ರುವ Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, ಸತ್ಯ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ಎಣಿಕೆ(entry.first) != 0, ಸತ್ಯ);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], ಅಸತ್ಯ);  // Each entry once
        seen[entry.first] = ಸತ್ಯ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

ಶೂನ್ಯ selfTest() {
    test_ಮೌಲ್ಯ_ಕೀಲಿಗಳು();
    test_Pointer_ಕೀಲಿಗಳು();
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
