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

ನಾಮಾಕಾಶ V3_Hash_ಕೋಷ್ಟಕ_Internals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr ಗಾತ್ರ_t max_ತುಂಬು(ಗಾತ್ರ_t ಸಾಮರ್ಥ್ಯ) { ಹಿಂತಿರುಗಿ ಸಾಮರ್ಥ್ಯ * ತುಂಬು_ಗುಣಕ_ಅಂಕೆ / ತುಂಬು_ಗುಣಕ_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr ಗಾತ್ರ_t GROWS_TWICE = max_ತುಂಬು(4 * MIN_ಸಾಮರ್ಥ್ಯ);
static_assert(GROWS_TWICE > max_ತುಂಬು(2 * MIN_ಸಾಮರ್ಥ್ಯ),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

ಶೂನ್ಯ test_ಮೌಲ್ಯ_ಕೀಲಿಗಳು() {
    ರಚನೆ ಮೌಲ್ಯ_2 final {
        ಗಾತ್ರ_t m_hash = 0;  // Hash of this entry
        ಗಾತ್ರ_t m_id = 0;  // Id of this entry
    };

    ರಚನೆ Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2& ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ಮೌಲ್ಯ.m_hash, 0); }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t hash, ಗಾತ್ರ_t) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ hash; }
    };

    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2& a, ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2& b) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(a, b.m_hash, b.m_id);
        }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2& ಮೌಲ್ಯ, ಗಾತ್ರ_t hash, ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ.m_hash == hash && ಮೌಲ್ಯ.m_id == id;
        }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<ಮೌಲ್ಯ_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2 ಮೌಲ್ಯ{1, 0};
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        ಗಣ_2 ಗಣ;
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Starts empty
        UASSERT_SELFTEST(ಗಣ.begin() == ಗಣ.ಕೊನೆ(), ನಿಜ);  // So iterates nothing
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯ) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // And finds nothing
        ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> added = ಗಣ.ಸೇರಿಸು(ಮೌಲ್ಯ);
        UASSERT_SELFTEST(added.ಎರಡನೇ, ನಿಜ);  // Added, as it was absent
        UASSERT_SELFTEST(added.ಮೊದಲ->m_id, ಮೌಲ್ಯ.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.ಮೊದಲ != &ಮೌಲ್ಯ, ನಿಜ);  // Which is a copy of the argument
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 1);  // And is the only one
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ಸುಳ್ಳು);  // So the set is no longer empty
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.ಮೊದಲ);  // At the entry held
        }
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(equal);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.ಮೊದಲ);  // At that same entry
        }
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(1, ಗಾತ್ರ_t{0});
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.ಮೊದಲ);  // At that same entry again
        }
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(1, ಗಾತ್ರ_t{1}) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // Hash same, id not
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(2, ಗಾತ್ರ_t{0}) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> again = ಗಣ.ಸೇರಿಸು(equal);
        UASSERT_SELFTEST(again.ಎರಡನೇ, ಸುಳ್ಳು);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.ಮೊದಲ, &*added.ಮೊದಲ);  // At the entry already held
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Present before the erase
            ಗಣ.erase(it);
        }
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Empty again
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯ) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // So the entry is gone
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(ಮೌಲ್ಯ), ಸುಳ್ಳು);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(ಗಣ.erase(ಮೌಲ್ಯ), ಸುಳ್ಳು);  // Nothing to erase, so says so
        ಗಣ.ಸೇರಿಸು(ಮೌಲ್ಯ);
        UASSERT_SELFTEST(ಗಣ.erase(equal), ನಿಜ);  // An equal key erases the entry held
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        ಗಣ.ಸೇರಿಸು(ಮೌಲ್ಯ);
        UASSERT_SELFTEST(ಗಣ.erase(1, ಗಾತ್ರ_t{0}), ನಿಜ);  // Those parts find and erase it
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr ಗಾತ್ರ_t N = GROWS_TWICE;
        ಗಣ_2 ಗಣ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
            ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> added = ಗಣ.ಸೇರಿಸು(ಮೌಲ್ಯ_2{7, i});
            UASSERT_SELFTEST(added.ಎರಡನೇ, ನಿಜ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // All of them, in the one probe run
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಗಾತ್ರ_t{7}, i);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<ತಾರ್ಕಿಕ, N> ನೋಡಿದ{};
        ಗಾತ್ರ_t n = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2& ನಮೂದು : ಗಣ) {
            UASSERT_SELFTEST(ನಮೂದು.m_hash, ಗಾತ್ರ_t{7});  // Only entries added
            UASSERT_SELFTEST(ನೋಡಿದ[ನಮೂದು.m_id], ಸುಳ್ಳು);  // Each of them exactly once
            ನೋಡಿದ[ನಮೂದು.m_id] = ನಿಜ;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

ಶೂನ್ಯ test_ಸೂಚಕ_ಕೀಲಿಗಳು() {
    ರಚನೆ ಮೌಲ್ಯ_2 final {
        ಗಾತ್ರ_t m_hash = 0;  // Hash of this entry
        ಗಾತ್ರ_t m_id = 0;  // Id of this entry
    };

    ರಚನೆ Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* valuep) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(valuep->m_hash, 0); }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t hash, ಗಾತ್ರ_t) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ hash; }
    };

    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* ap, ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ap, bp->m_hash, bp->m_id);
        }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* valuep, ಗಾತ್ರ_t hash, ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<ಮೌಲ್ಯ_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ಮೌಲ್ಯ_2 ಮೌಲ್ಯ{1, 0};
        ಮೌಲ್ಯ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        ಗಣ_2 ಗಣ;
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Starts empty
        UASSERT_SELFTEST(ಗಣ.begin() == ಗಣ.ಕೊನೆ(), ನಿಜ);  // So iterates nothing
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // And finds nothing
        ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> added = ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(added.ಎರಡನೇ, ನಿಜ);  // Added, as it was absent
        UASSERT_SELFTEST(*added.ಮೊದಲ, &ಮೌಲ್ಯ);  // The iterator is at the new entry
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 1);  // Which is the only one
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ಸುಳ್ಳು);  // So the set is no longer empty
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Found by itself
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // At the entry held
        }
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&equal);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // And by an equal entry
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // At that same entry
        }
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(1, ಗಾತ್ರ_t{0});
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // And by its parts
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // At that same entry again
        }
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(1, ಗಾತ್ರ_t{1}) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // Hash same, id not
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(2, ಗಾತ್ರ_t{0}) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> again = ಗಣ.ಸೇರಿಸು(&equal);
        UASSERT_SELFTEST(again.ಎರಡನೇ, ಸುಳ್ಳು);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.ಮೊದಲ, &ಮೌಲ್ಯ);  // The iterator is at the stored one
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&equal);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, ಸುಳ್ಳು);  // Not the one asked for
        }
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // The entry is still there
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Present before the erase
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the object asked for
            ಗಣ.erase(it);
        }
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Empty again
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // So the entry is gone
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(&ಮೌಲ್ಯ), ಸುಳ್ಳು);  // As 'contains' agrees
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(ಗಣ.erase(&ಮೌಲ್ಯ), ಸುಳ್ಳು);  // Nothing to erase, so says so
        ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(ಗಣ.erase(&equal), ನಿಜ);  // The equal key erases the stored one
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Which leaves the set empty
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // And unreachable
        // And by a key spelled as the parts of an entry
        ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(ಗಣ.erase(1, ಗಾತ್ರ_t{0}), ನಿಜ);  // Those parts find and erase it
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    ಗಾಗಿ (ಗಾತ್ರ_t erase = 0; erase < 4; ++erase) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t hash : {ಗಾತ್ರ_t{0}, ಗಾತ್ರ_t{7}, ~ಗಾತ್ರ_t{0}}) {  // Wraps too
            std::array<ಮೌಲ್ಯ_2, 4> ಮೌಲ್ಯಗಳು{ಮೌಲ್ಯ_2{hash, 0}, ಮೌಲ್ಯ_2{hash, 1},  //
                                        ಮೌಲ್ಯ_2{hash, 2}, ಮೌಲ್ಯ_2{hash, 3}};
            ಗಣ_2 ಗಣ;
            ಗಾಗಿ (ಮೌಲ್ಯ_2& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 4);  // All distinct, so all added
            {
                ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯಗಳು[erase]);
                UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // The one to erase is present
                UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[erase]);  // And is the object held
                ಗಣ.erase(it);
            }
            UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯಗಳು[erase]) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // That one
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2(); ++i) {
                ಆದರೆ (i == erase) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(hash, i);
                UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // The others are all there
                UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t hash : {ಗಾತ್ರ_t{0}, ಗಾತ್ರ_t{7}, ~ಗಾತ್ರ_t{0}}) {
        std::array<ಮೌಲ್ಯ_2, GROWS_TWICE> many{};
        ಗಣ_2 ಗಣ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < many.ಗಾತ್ರ_2(); ++i) {
            many[i] = ಮೌಲ್ಯ_2{hash, i};
            ಗಣ.ಸೇರಿಸು(&many[i]);
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), many.ಗಾತ್ರ_2());  // All of them were added
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < many.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(hash, i);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        ಮೌಲ್ಯ_2 ಮೌಲ್ಯ{1, 0};
        ಮೌಲ್ಯ_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        ಗಣ_2 ಗಣ;
        ಗಣ.reserve(2);
        ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        ಗಣ.ಸೇರಿಸು(&other);
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 2);  // Both kept, despite the collision
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // The first of the run is there
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is that entry
        }
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&other);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Present before the erase
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the object held
            ಗಣ.erase(it);
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 1);  // Only one was erased
        UASSERT_SELFTEST(ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ) == ಗಣ.ಕೊನೆ(), ನಿಜ);  // Namely that one
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&other);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ doReserve : {ಸುಳ್ಳು, ನಿಜ}) {
        std::array<ಮೌಲ್ಯ_2, 8> ಮೌಲ್ಯಗಳು{};
        ಗಣ_2 ಗಣ;
        ಆದರೆ (doReserve) ಗಣ.reserve(ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2(); ++i) {
            ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{static_cast<ಗಾತ್ರ_t>(i * 1234567), i};
            ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        }
        ಗಾಗಿ (ಮೌಲ್ಯ_2& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(&ಮೌಲ್ಯ);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Each entry survives
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯ);  // And is the one inserted
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr ಗಾತ್ರ_t N = GROWS_TWICE;
        std::array<ಮೌಲ್ಯ_2, N> ಮೌಲ್ಯಗಳು{};
        ಗಣ_2 ಗಣ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
            ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{7, i};  // All of them hash the same
            ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> pair
                = ಗಣ.ಸೇರಿಸು_Lazy(ಗಾತ್ರ_t{7}, i, [&]() -> ಮೌಲ್ಯ_2* { ಹಿಂತಿರುಗಿ &ಮೌಲ್ಯಗಳು[i]; });
            UASSERT_SELFTEST(pair.ಎರಡನೇ, ನಿಜ);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.ಮೊದಲ, &ಮೌಲ್ಯಗಳು[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // All of them were added
        // They are all present now, so nothing is created
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
            ತಾರ್ಕಿಕ created = ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> pair
                = ಗಣ.ಸೇರಿಸು_Lazy(ಗಾತ್ರ_t{7}, i, [&]() -> ಮೌಲ್ಯ_2* {
                      created = ನಿಜ;  // LCOV_EXCL_START
                      ಹಿಂತಿರುಗಿ &ಮೌಲ್ಯಗಳು[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, ಸುಳ್ಳು);  // The factory was never called
            UASSERT_SELFTEST(pair.ಎರಡನೇ, ಸುಳ್ಳು);  // As the lookup hit
            UASSERT_SELFTEST(*pair.ಮೊದಲ, &ಮೌಲ್ಯಗಳು[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<ಮೌಲ್ಯ_2, 20> ಮೌಲ್ಯಗಳು{};
        std::array<ತಾರ್ಕಿಕ, 20> ನೋಡಿದ{};
        ಗಣ_2 ಗಣ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2(); ++i) {
            ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{i / 2, i};  // Pairs of them collide
            ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        }
        ಗಾತ್ರ_t n = 0;
        ಗಾಗಿ (ಮೌಲ್ಯ_2* ಸ್ಥಿರಾಂಕ valuep : ಗಣ) {
            UASSERT_SELFTEST(valuep->m_id < ನೋಡಿದ.ಗಾತ್ರ_2(), ನಿಜ);  // Only entries added
            UASSERT_SELFTEST(ನೋಡಿದ[valuep->m_id], ಸುಳ್ಳು);  // Each of them exactly once
            ನೋಡಿದ[valuep->m_id] = ನಿಜ;
            ++n;
        }
        UASSERT_SELFTEST(n, ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

ಶೂನ್ಯ test_ನಮೂದು_Lifetime() {
    ಗಾತ್ರ_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    ವರ್ಗ NoDefault final {
        ಗಾತ್ರ_t* m_alivep;  // Where the live entries are counted
        ಗಾತ್ರ_t m_hash;  // Hash of this entry
        ಗಾತ್ರ_t m_id;  // Entries with equal ids are equal

    ಸಾರ್ವಜನಿಕ:
        NoDefault(ಗಾತ್ರ_t* alivep, ಗಾತ್ರ_t hash, ಗಾತ್ರ_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ಸ್ಥಿರಾಂಕ NoDefault& that)
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
        NoDefault& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ NoDefault&) = ಅಳಿಸು;
        NoDefault& ಕ್ರಿಯಾಕಾರಕ=(NoDefault&&) = ಅಳಿಸು;

        ಗಾತ್ರ_t hash() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hash; }
        ಗಾತ್ರ_t id() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_id; }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ NoDefault& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::ಮೌಲ್ಯ,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    ರಚನೆ Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ NoDefault& ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ.hash(); }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t hash, ಗಾತ್ರ_t) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ hash; }
    };

    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ NoDefault& a, ಸ್ಥಿರಾಂಕ NoDefault& b) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(a, b.hash(), b.id());
        }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ NoDefault& a, ಗಾತ್ರ_t, ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ a.id() == id; }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<NoDefault, Hash, Equal>;

    constexpr ಗಾತ್ರ_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        ಗಣ_2 ಗಣ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
            ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> added = ಗಣ.ಸೇರಿಸು(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.ಎರಡನೇ, ನಿಜ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಗಾತ್ರ_t{7}, i);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> dup = ಗಣ.ಸೇರಿಸು(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.ಎರಡನೇ, ಸುಳ್ಳು);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.ಮೊದಲ->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಗಾತ್ರ_t{7}, ಗಾತ್ರ_t{0});
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Present before the erase
            UASSERT_SELFTEST(it->id(), ಗಾತ್ರ_t{0});  // And is the entry asked for
            ಗಣ.erase(it);
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(ಗಾತ್ರ_t{7}, ಗಾತ್ರ_t{0}), ಸುಳ್ಳು);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < N; ++i) {
            {
                ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಗಾತ್ರ_t{7}, i);
                UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                ಗಣ.erase(it);
            }
            UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            ಗಾಗಿ (ಗಾತ್ರ_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(ಗಾತ್ರ_t{7}, j), ನಿಜ);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(ಗಣ.begin() == ಗಣ.ಕೊನೆ(), ನಿಜ);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) ಗಣ.ಸೇರಿಸು(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        ಗಣ.clear();
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 0);  // Nothing left
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Says so
        UASSERT_SELFTEST(ಗಣ.begin() == ಗಣ.ಕೊನೆ(), ನಿಜ);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(ಗಾತ್ರ_t{7}, ಗಾತ್ರ_t{0}), ಸುಳ್ಳು);  // Nor finds any
        ಗಣ.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) ಗಣ.ಸೇರಿಸು(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        ಗಣ_2 ಗಣ;
        ಗಣ.clear();
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // Still empty
        ಗಣ.ಸೇರಿಸು(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

ಶೂನ್ಯ testMove() {
    ಗಾತ್ರ_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    ರಚನೆ Counted final {
        ಗಾತ್ರ_t* m_alivep;  // Where the live entries are counted
        ಗಾತ್ರ_t m_id;  // Entries with equal ids are equal

        Counted(ಗಾತ್ರ_t* alivep, ಗಾತ್ರ_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ಸ್ಥಿರಾಂಕ Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ Counted&) = ಅಳಿಸು;
        Counted& ಕ್ರಿಯಾಕಾರಕ=(Counted&&) = ಅಳಿಸು;
    };

    ರಚನೆ Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Counted& ನಮೂದು) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ನಮೂದು.m_id); }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ id; }
    };

    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Counted& a, ಸ್ಥಿರಾಂಕ Counted& b) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(a, b.m_id); }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Counted& a, ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ a.m_id == id; }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<Counted, Hash, Equal>;

    constexpr ಗಾತ್ರ_t N = GROWS_TWICE;
    {
        ಗಣ_2 ಗಣ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) ಗಣ.ಸೇರಿಸು(Counted{&alive, i});
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        ಗಣ_2 moved{std::move(ಗಣ)};
        UASSERT_SELFTEST(moved.ಗಾತ್ರ_2(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // And the source no longer holds them
        UASSERT_SELFTEST(ಗಣ.begin() == ಗಣ.ಕೊನೆ(), ನಿಜ);  // So it iterates nothing
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.ಒಳಗೊಂಡಿದೆ(i), ನಿಜ);  // Every entry came across
            UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(i), ಸುಳ್ಳು);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(ಗಣ.ಸೇರಿಸು(Counted{&alive, N}).ಎರಡನೇ, ನಿಜ);  // It took an entry
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 1);  // Which is all it holds
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ಸುಳ್ಳು);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        ಗಣ = std::move(moved);
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(N), ಸುಳ್ಳು);  // Namely that one
        UASSERT_SELFTEST(moved.ಖಾಲಿಗಳು(), ನಿಜ);  // And the source is empty again
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(i), ನಿಜ);  // The rest moved
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
        ಗಾತ್ರ_t m_id;  // Entries with equal ids are equal

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ MoveOnly(ಗಾತ್ರ_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = ಪೂರ್ವನಿಯೋಜಿತ;
        MoveOnly(ಸ್ಥಿರಾಂಕ MoveOnly&) = ಅಳಿಸು;
        MoveOnly& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ MoveOnly&) = ಅಳಿಸು;
        MoveOnly& ಕ್ರಿಯಾಕಾರಕ=(MoveOnly&&) = ಅಳಿಸು;
        ~MoveOnly() = ಪೂರ್ವನಿಯೋಜಿತ;

        ಗಾತ್ರ_t id() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::ಮೌಲ್ಯ,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::ಮೌಲ್ಯ,
                  "SelfTest: 'MoveOnly' must be move constructible");

    ರಚನೆ Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ MoveOnly& ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ಮೌಲ್ಯ.id()); }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ 7; }
    };

    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ MoveOnly& a, ಸ್ಥಿರಾಂಕ MoveOnly& b) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(a, b.id());
        }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ MoveOnly& a, ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ a.id() == id; }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<MoveOnly, Hash, Equal>;

    constexpr ಗಾತ್ರ_t N = GROWS_TWICE;
    ಗಣ_2 ಗಣ;
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> added
            = ಗಣ.ಸೇರಿಸು_Lazy(i, [i] { ಹಿಂತಿರುಗಿ MoveOnly{i}; });
        UASSERT_SELFTEST(added.ಎರಡನೇ, ನಿಜ);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.ಮೊದಲ->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; i += 2) {
        ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(i);
        UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Present before the erase
        ಗಣ.erase(it);
    }
    UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N / 2);  // Half of them are gone
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ erased = (i % 2) == 0;
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

ಶೂನ್ಯ testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    ವರ್ಗ Hash final {
        ಗಾತ್ರ_t m_ಮುಖವಾಡ;  // Only these bits of an entry matter

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ Hash(ಗಾತ್ರ_t ಮುಖವಾಡ)
            : m_ಮುಖವಾಡ{ಮುಖವಾಡ} {}
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ & m_ಮುಖವಾಡ; }
    };

    ವರ್ಗ Equal final {
        ಗಾತ್ರ_t m_ಮುಖವಾಡ;  // Only these bits of an entry matter

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ Equal(ಗಾತ್ರ_t ಮುಖವಾಡ)
            : m_ಮುಖವಾಡ{ಮುಖವಾಡ} {}
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t a, ಗಾತ್ರ_t b) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (a & m_ಮುಖವಾಡ) == (b & m_ಮುಖವಾಡ); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::ಮೌಲ್ಯ,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::ಮೌಲ್ಯ,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_ಗಣ<ಗಾತ್ರ_t, Hash, Equal> ಗಣ{Hash{3}, Equal{3}};
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < 16; ++i) ಗಣ.ಸೇರಿಸು(i);
    UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < 16; ++i) {
        ಸ್ಥಿರಾಂಕ V3_Hash_ಗಣ<ಗಾತ್ರ_t, Hash, Equal>::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(i);
        UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

ಶೂನ್ಯ test_Backward_ಸರಿಸು_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    ರಚನೆ ಮೌಲ್ಯ_2 final {
        ಗಾತ್ರ_t m_hash = 0;  // Hash of this entry
        ಗಾತ್ರ_t m_id = 0;  // Entries with equal ids are equal
    };

    ರಚನೆ Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* valuep) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(valuep->m_hash, 0); }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t hash, ಗಾತ್ರ_t) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ hash; }
    };

    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* ap, ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ap, bp->m_hash, bp->m_id);
        }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* valuep, ಗಾತ್ರ_t hash, ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<ಮೌಲ್ಯ_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t h : {ಗಾತ್ರ_t{0}, ಗಾತ್ರ_t{5}, ~ಗಾತ್ರ_t{0}}) {  // Wraps too
        std::array<ಮೌಲ್ಯ_2, 4> ಮೌಲ್ಯಗಳು{ಮೌಲ್ಯ_2{h, 0}, ಮೌಲ್ಯ_2{h + 1, 1},  //
                                    ಮೌಲ್ಯ_2{h, 2}, ಮೌಲ್ಯ_2{h + 1, 3}};
        ಗಣ_2 ಗಣ;
        ಗಾಗಿ (ಮೌಲ್ಯ_2& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯ);
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(h, ಗಾತ್ರ_t{0});
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Present before the erase
            ಗಣ.erase(it);
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 3);  // One fewer entry
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(h, ಗಾತ್ರ_t{0}), ಸುಳ್ಳು);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯಗಳು[i].m_hash, i);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(h + 1, ಗಾತ್ರ_t{1});
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Left where the first erase found it
            ಗಣ.erase(it);
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), 2);  // Two are left
        ಗಾಗಿ (ಗಾತ್ರ_t i = 2; i < ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯಗಳು[i].m_hash, i);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Both still reachable
            UASSERT_SELFTEST(*it, &ಮೌಲ್ಯಗಳು[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<ತಾರ್ಕಿಕ, 4> ನೋಡಿದ{};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* ಸ್ಥಿರಾಂಕ valuep : ಗಣ) {
            UASSERT_SELFTEST(ನೋಡಿದ[valuep->m_id], ಸುಳ್ಳು);  // Each entry once
            ನೋಡಿದ[valuep->m_id] = ನಿಜ;
        }
        UASSERT_SELFTEST(ನೋಡಿದ[0], ಸುಳ್ಳು);  // Erased first
        UASSERT_SELFTEST(ನೋಡಿದ[1], ಸುಳ್ಳು);  // Erased second
        UASSERT_SELFTEST(ನೋಡಿದ[2], ನಿಜ);  // Moved back over the first hole
        UASSERT_SELFTEST(ನೋಡಿದ[3], ನಿಜ);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

ಶೂನ್ಯ testReferenceStability() {
    ರಚನೆ ಮೌಲ್ಯ_2 final {
        ಗಾತ್ರ_t m_hash = 0;  // Hash of this entry
        ಗಾತ್ರ_t m_id = 0;  // Id of this entry
    };

    ರಚನೆ Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* valuep) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(valuep->m_hash, 0); }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t hash, ಗಾತ್ರ_t) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ hash; }
    };

    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* ap, ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ap, bp->m_hash, bp->m_id);
        }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_2* valuep, ಗಾತ್ರ_t hash, ಗಾತ್ರ_t id) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<ಮೌಲ್ಯ_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        ಗಣ_2 ಗಣ;
        ಗಣ.reserve(8);
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(ಗಾತ್ರ_t{7}, ಗಾತ್ರ_t{0}), ಸುಳ್ಳು);  // Room, but no entry
        UASSERT_SELFTEST(ಗಣ.begin() == ಗಣ.ಕೊನೆ(), ನಿಜ);  // So iterates nothing
        UASSERT_SELFTEST(ಗಣ.ಖಾಲಿಗಳು(), ನಿಜ);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr ಗಾತ್ರ_t N = max_ತುಂಬು(2 * MIN_ಸಾಮರ್ಥ್ಯ);
    static_assert(N > max_ತುಂಬು(MIN_ಸಾಮರ್ಥ್ಯ), "SelfTest: 'N' must need more than a new table");
    std::array<ಮೌಲ್ಯ_2, N> ಮೌಲ್ಯಗಳು{};
    std::array<ಮೌಲ್ಯ_2* ಸ್ಥಿರಾಂಕ*, N> entrypps{};  // Where each entry is stored, as inserted
    ಗಣ_2 ಗಣ;
    ಗಣ.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಮೌಲ್ಯಗಳು[i] = ಮೌಲ್ಯ_2{7, i};
        ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> pair = ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        UASSERT_SELFTEST(pair.ಎರಡನೇ, ನಿಜ);  // Ids differ, so each is added
        entrypps[i] = &*pair.ಮೊದಲ;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಗಾತ್ರ_t{7}, i);
        UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ std::pair<ಗಣ_2::iterator, ತಾರ್ಕಿಕ> pair = ಗಣ.ಸೇರಿಸು(&ಮೌಲ್ಯಗಳು[i]);
        UASSERT_SELFTEST(pair.ಎರಡನೇ, ಸುಳ್ಳು);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.ಮೊದಲ, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    ಗಣ.reserve(8 * N);
    UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // Rehashing dropped nothing
    std::array<ಮೌಲ್ಯ_2* ಸ್ಥಿರಾಂಕ*, N> grownpps{};  // Where each entry is after the growth
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಗಾತ್ರ_t{7}, i);
        UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    ಗಣ.reserve(N);
    UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), N);  // The smaller request changed nothing
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(ಗಾತ್ರ_t{7}, i);
        UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), ನಿಜ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
ರಚನೆ ClusteredHash final {
    ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
ರಚನೆ ಹರಡುವಿಕೆ_Hash final {
    ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ static_cast<ಗಾತ್ರ_t>(ಮೌಲ್ಯ * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Hash>
ಶೂನ್ಯ test_ಗಣ_Against_Model() {
    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t a, ಗಾತ್ರ_t b) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ a == b; }
    };

    ಬಳಸಿ ಗಣ_2 = V3_Hash_ಗಣ<ಗಾತ್ರ_t, T_Hash, Equal>;

    constexpr ಗಾತ್ರ_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr ಗಾತ್ರ_t ಹೆಜ್ಜೆಗಳು = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಮುಂದಿನ_Rand = [&state]() -> ಗಾತ್ರ_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ಹಿಂತಿರುಗಿ static_cast<ಗಾತ್ರ_t>(state >> 32);
    };

    ಗಣ_2 ಗಣ;
    std::ಗಣ<ಗಾತ್ರ_t> model;

    ಗಾಗಿ (ಗಾತ್ರ_t ಹೆಜ್ಜೆ = 0; ಹೆಜ್ಜೆ < ಹೆಜ್ಜೆಗಳು; ++ಹೆಜ್ಜೆ) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t id = ಮುಂದಿನ_Rand() % UNIVERSE;
        ಆಯ್ಕೆ (ಮುಂದಿನ_Rand() % 4) {
        ಸ್ಥಿತಿ 0: {  // Insert a copy
            ಸ್ಥಿರಾಂಕ std::pair<ಬಗೆ_ಹೆಸರು ಗಣ_2::iterator, ತಾರ್ಕಿಕ> pair = ಗಣ.ಸೇರಿಸು(id);
            UASSERT_SELFTEST(pair.ಎರಡನೇ, model.ಸೇರಿಸು(id).ಎರಡನೇ);  // As the model
            UASSERT_SELFTEST(*pair.ಮೊದಲ, id);  // At the entry asked for
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 1: {  // Insert lazily, which must create only on a miss
            ತಾರ್ಕಿಕ created = ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ std::pair<ಬಗೆ_ಹೆಸರು ಗಣ_2::iterator, ತಾರ್ಕಿಕ> pair
                = ಗಣ.ಸೇರಿಸು_Lazy(id, [&]() -> ಗಾತ್ರ_t {
                      created = ನಿಜ;
                      ಹಿಂತಿರುಗಿ id;
                  });
            UASSERT_SELFTEST(pair.ಎರಡನೇ, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.ಎರಡನೇ, model.ಸೇರಿಸು(id).ಎರಡನೇ);  // As the model
            UASSERT_SELFTEST(*pair.ಮೊದಲ, id);  // At the entry asked for
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 2: {  // Erase, if present
            ಸ್ಥಿರಾಂಕ ಬಗೆ_ಹೆಸರು ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(id);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), model.ಎಣಿಕೆ_2(id) != 0);  // As the model
            ಆದರೆ (it != ಗಣ.ಕೊನೆ()) {
                ಗಣ.erase(it);
                model.erase(id);
            }
            ನಿಲ್ಲಿಸು;
        }
        ಪೂರ್ವನಿಯೋಜಿತ: {  // Look up only
            ಸ್ಥಿರಾಂಕ ಬಗೆ_ಹೆಸರು ಗಣ_2::iterator it = ಗಣ.ಕಂಡುಹಿಡಿ(id);
            UASSERT_SELFTEST(it != ಗಣ.ಕೊನೆ(), model.ಎಣಿಕೆ_2(id) != 0);  // As the model
            ಆದರೆ (it != ಗಣ.ಕೊನೆ()) UASSERT_SELFTEST(*it, id);  // And is the entry
            ನಿಲ್ಲಿಸು;
        }
        }
        UASSERT_SELFTEST(ಗಣ.ಗಾತ್ರ_2(), model.ಗಾತ್ರ_2());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    ಗಾಗಿ (ಗಾತ್ರ_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(ಗಣ.ಒಳಗೊಂಡಿದೆ(id), model.ಎಣಿಕೆ_2(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<ತಾರ್ಕಿಕ, UNIVERSE> ನೋಡಿದ{};
    ಗಾತ್ರ_t n = 0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ನಮೂದು : ಗಣ) {
        UASSERT_SELFTEST(ನಮೂದು < UNIVERSE, ನಿಜ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ಎಣಿಕೆ_2(ನಮೂದು) != 0, ನಿಜ);  // Only what was inserted
        UASSERT_SELFTEST(ನೋಡಿದ[ನಮೂದು], ಸುಳ್ಳು);  // Each entry once
        ನೋಡಿದ[ನಮೂದು] = ನಿಜ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.ಗಾತ್ರ_2());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

ಶೂನ್ಯ testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    ಬಳಸಿ Map = V3HashMap<std::string, ಪೂರ್ಣಾಂಕ>;
    ಬಳಸಿ ನಮೂದು_2 = Map::ನಮೂದು_2;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<ನಮೂದು_2, std::pair<std::string, ಪೂರ್ಣಾಂಕ>>::ಮೌಲ್ಯ,
                  "SelfTest: a map entry must be a plain pair");

    constexpr ಗಾತ್ರ_t N = GROWS_TWICE;
    Map map;
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ std::string ಕೀಲಿ = "key" + std::ಗಮ್ಯ_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ಸ್ಥಿರಾಂಕ std::pair<Map::iterator, ತಾರ್ಕಿಕ> pair
            = map.ಸೇರಿಸು_Lazy(ಕೀಲಿ, [&]() -> ನಮೂದು_2 { ಹಿಂತಿರುಗಿ {ಕೀಲಿ, static_cast<ಪೂರ್ಣಾಂಕ>(i)}; });
        UASSERT_SELFTEST(pair.ಎರಡನೇ, ನಿಜ);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ Map::iterator it = map.ಕಂಡುಹಿಡಿ("key" + std::ಗಮ್ಯ_string(i));
        UASSERT_SELFTEST(it != map.ಕೊನೆ(), ನಿಜ);  // The key alone finds it
        UASSERT_SELFTEST(it->ಎರಡನೇ, static_cast<ಪೂರ್ಣಾಂಕ>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ std::string ಕೀಲಿ = "key" + std::ಗಮ್ಯ_string(i);
        map.erase(map.ಕಂಡುಹಿಡಿ(ಕೀಲಿ));
        UASSERT_SELFTEST(map.ಸೇರಿಸು({ಕೀಲಿ, static_cast<ಪೂರ್ಣಾಂಕ>(i) + 100}).ಎರಡನೇ, ನಿಜ);  // Anew
    }
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N);  // The same keys, so the map has not grown
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ Map::iterator it = map.ಕಂಡುಹಿಡಿ("key" + std::ಗಮ್ಯ_string(i));
        UASSERT_SELFTEST(it != map.ಕೊನೆ(), ನಿಜ);  // Each key is still there
        UASSERT_SELFTEST(it->ಎರಡನೇ, static_cast<ಪೂರ್ಣಾಂಕ>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) map.ಕಂಡುಹಿಡಿ("key" + std::ಗಮ್ಯ_string(i)).ಮೌಲ್ಯ() += 100;
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N);  // The map is unchanged otherwise
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) {
        ಸ್ಥಿರಾಂಕ Map::iterator it = map.ಕಂಡುಹಿಡಿ("key" + std::ಗಮ್ಯ_string(i));
        UASSERT_SELFTEST(it != map.ಕೊನೆ(), ನಿಜ);  // Each key still finds its entry
        UASSERT_SELFTEST(it->ಎರಡನೇ, static_cast<ಪೂರ್ಣಾಂಕ>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < N; ++i) map.ಕಂಡುಹಿಡಿ("key" + std::ಗಮ್ಯ_string(i)).ಮೌಲ್ಯ() -= 100;

    // Adding a key that is present changes nothing
    {
        ಸ್ಥಿರಾಂಕ std::string ಕೀಲಿ = "key0";
        ತಾರ್ಕಿಕ created = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ std::pair<Map::iterator, ತಾರ್ಕಿಕ> pair = map.ಸೇರಿಸು_Lazy(ಕೀಲಿ, [&]() -> ನಮೂದು_2 {
            created = ನಿಜ;  // LCOV_EXCL_START
            ಹಿಂತಿರುಗಿ {ಕೀಲಿ, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, ಸುಳ್ಳು);  // The factory was never called
        UASSERT_SELFTEST(pair.ಎರಡನೇ, ಸುಳ್ಳು);  // As the key is present
        UASSERT_SELFTEST(pair.ಮೊದಲ->ಎರಡನೇ, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ಸ್ಥಿರಾಂಕ std::pair<Map::iterator, ತಾರ್ಕಿಕ> added = map.ಸೇರಿಸು({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.ಎರಡನೇ, ನಿಜ);  // Added, as the key is new
        UASSERT_SELFTEST(added.ಮೊದಲ->ಎರಡನೇ, 5);  // With the value given
        ಸ್ಥಿರಾಂಕ std::pair<Map::iterator, ತಾರ್ಕಿಕ> again = map.ಸೇರಿಸು({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.ಎರಡನೇ, ಸುಳ್ಳು);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.ಮೊದಲ->ಎರಡನೇ, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N + 1);  // Just the one entry was added
    {
        ಸ್ಥಿರಾಂಕ Map::iterator it = map.ಕಂಡುಹಿಡಿ(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.ಕೊನೆ(), ನಿಜ);  // Present before the erase
        UASSERT_SELFTEST(it->ಎರಡನೇ, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), ಸುಳ್ಳು);  // No such key
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), ನಿಜ);  // That key is present
    UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.ಒಳಗೊಂಡಿದೆ(std::string{"key0"}), ಸುಳ್ಳು);  // Namely that one
    ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.ಒಳಗೊಂಡಿದೆ("key" + std::ಗಮ್ಯ_string(i)), ನಿಜ);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Hash>
ಶೂನ್ಯ testMapAgainstModel() {
    ರಚನೆ Equal final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಗಾತ್ರ_t a, ಗಾತ್ರ_t b) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ a == b; }
    };

    ಬಳಸಿ Map = V3HashMap<ಗಾತ್ರ_t, ಗಾತ್ರ_t, T_Hash, Equal>;
    ಬಳಸಿ ನಮೂದು_2 = ಬಗೆ_ಹೆಸರು Map::ನಮೂದು_2;

    constexpr ಗಾತ್ರ_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr ಗಾತ್ರ_t ಹೆಜ್ಜೆಗಳು = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಮುಂದಿನ_Rand = [&state]() -> ಗಾತ್ರ_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ಹಿಂತಿರುಗಿ static_cast<ಗಾತ್ರ_t>(state >> 32);
    };

    Map map;
    std::map<ಗಾತ್ರ_t, ಗಾತ್ರ_t> model;

    ಗಾಗಿ (ಗಾತ್ರ_t ಹೆಜ್ಜೆ = 0; ಹೆಜ್ಜೆ < ಹೆಜ್ಜೆಗಳು; ++ಹೆಜ್ಜೆ) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೀಲಿ = ಮುಂದಿನ_Rand() % UNIVERSE;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t val = ಹೆಜ್ಜೆ;  // Distinct every step, so a stale value is visible
        ಆಯ್ಕೆ (ಮುಂದಿನ_Rand() % 5) {
        ಸ್ಥಿತಿ 0: {  // Insert a whole entry
            ಸ್ಥಿರಾಂಕ std::pair<ಬಗೆ_ಹೆಸರು Map::iterator, ತಾರ್ಕಿಕ> pair = map.ಸೇರಿಸು({ಕೀಲಿ, val});
            UASSERT_SELFTEST(pair.ಎರಡನೇ, model.ಸೇರಿಸು({ಕೀಲಿ, val}).ಎರಡನೇ);  // Ditto
            UASSERT_SELFTEST(pair.ಮೊದಲ->ಮೊದಲ, ಕೀಲಿ);  // At the key asked for
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 1: {  // Insert lazily, which must create only on a miss
            ತಾರ್ಕಿಕ created = ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ std::pair<ಬಗೆ_ಹೆಸರು Map::iterator, ತಾರ್ಕಿಕ> pair
                = map.ಸೇರಿಸು_Lazy(ಕೀಲಿ, [&]() -> ನಮೂದು_2 {
                      created = ನಿಜ;
                      ಹಿಂತಿರುಗಿ {ಕೀಲಿ, val};
                  });
            UASSERT_SELFTEST(pair.ಎರಡನೇ, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.ಎರಡನೇ, model.ಸೇರಿಸು({ಕೀಲಿ, val}).ಎರಡನೇ);  // Ditto
            UASSERT_SELFTEST(pair.ಮೊದಲ->ಮೊದಲ, ಕೀಲಿ);  // At the key asked for
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(ಕೀಲಿ), model.erase(ಕೀಲಿ) != 0);  // As model
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 3: {  // Change a value, which is erasing the entry and inserting it again
            ಸ್ಥಿರಾಂಕ ಬಗೆ_ಹೆಸರು Map::iterator it = map.ಕಂಡುಹಿಡಿ(ಕೀಲಿ);
            ಆದರೆ (it != map.ಕೊನೆ()) {
                map.erase(it);
                UASSERT_SELFTEST(map.ಸೇರಿಸು({ಕೀಲಿ, val}).ಎರಡನೇ, ನಿಜ);  // The key is free
                model[ಕೀಲಿ] = val;
            }
            ನಿಲ್ಲಿಸು;
        }
        ಪೂರ್ವನಿಯೋಜಿತ: {  // Look up only
            ಸ್ಥಿರಾಂಕ ಬಗೆ_ಹೆಸರು Map::iterator it = map.ಕಂಡುಹಿಡಿ(ಕೀಲಿ);
            UASSERT_SELFTEST(it != map.ಕೊನೆ(), model.ಎಣಿಕೆ_2(ಕೀಲಿ) != 0);  // As the model
            ಆದರೆ (it != map.ಕೊನೆ()) {
                UASSERT_SELFTEST(it->ಮೊದಲ, ಕೀಲಿ);  // At the key asked for
                UASSERT_SELFTEST(it->ಎರಡನೇ, model.at(ಕೀಲಿ));  // With its value
            }
            ನಿಲ್ಲಿಸು;
        }
        }
        UASSERT_SELFTEST(map.ಗಾತ್ರ_2(), model.ಗಾತ್ರ_2());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    ಗಾಗಿ (ಗಾತ್ರ_t ಕೀಲಿ = 0; ಕೀಲಿ < UNIVERSE; ++ಕೀಲಿ) {
        ಸ್ಥಿರಾಂಕ ಬಗೆ_ಹೆಸರು Map::iterator it = map.ಕಂಡುಹಿಡಿ(ಕೀಲಿ);
        UASSERT_SELFTEST(it != map.ಕೊನೆ(), model.ಎಣಿಕೆ_2(ಕೀಲಿ) != 0);  // Present iff modelled
        ಆದರೆ (it != map.ಕೊನೆ()) {
            UASSERT_SELFTEST(it->ಎರಡನೇ, model.at(ಕೀಲಿ));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<ತಾರ್ಕಿಕ, UNIVERSE> ನೋಡಿದ{};
    ಗಾತ್ರ_t n = 0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ನಮೂದು_2& ನಮೂದು : map) {
        UASSERT_SELFTEST(ನಮೂದು.ಮೊದಲ < UNIVERSE, ನಿಜ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ಎಣಿಕೆ_2(ನಮೂದು.ಮೊದಲ) != 0, ನಿಜ);  // Only what was inserted
        UASSERT_SELFTEST(ನಮೂದು.ಎರಡನೇ, model.at(ನಮೂದು.ಮೊದಲ));  // With the right value
        UASSERT_SELFTEST(ನೋಡಿದ[ನಮೂದು.ಮೊದಲ], ಸುಳ್ಳು);  // Each entry once
        ನೋಡಿದ[ನಮೂದು.ಮೊದಲ] = ನಿಜ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.ಗಾತ್ರ_2());  // And all of them
}

ಶೂನ್ಯ selfTest() {
    test_ಮೌಲ್ಯ_ಕೀಲಿಗಳು();
    test_ಸೂಚಕ_ಕೀಲಿಗಳು();
    test_ನಮೂದು_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_ಸರಿಸು_Deletion();
    testReferenceStability();
    test_ಗಣ_Against_Model<ClusteredHash>();
    test_ಗಣ_Against_Model<ಹರಡುವಿಕೆ_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<ಹರಡುವಿಕೆ_Hash>();
}

}  // namespace V3HashTableInternals
