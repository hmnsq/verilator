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

#ضمن "V3HashTable.h"

#ضمن "V3Error.h"

#ضمن <array>
#ضمن <functional>
#ضمن <map>
#ضمن <set>
#ضمن <string>
#ضمن <type_traits>
#ضمن <utility>

نطاق V3_Hash_جدول_Internals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr سائيز_t max_ڀريو(سائيز_t صلاحيت) { أرجع صلاحيت * ڀريو_ضرب_ڪندڙ_انگ / ڀريو_ضرب_ڪندڙ_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr سائيز_t GROWS_TWICE = max_ڀريو(4 * MIN_صلاحيت);
static_assert(GROWS_TWICE > max_ڀريو(2 * MIN_صلاحيت),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

فراغ test_قدر_ڪنجيون() {
    بنية قدر_2 final {
        سائيز_t m_hash = 0;  // Hash of this entry
        سائيز_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        سائيز_t عامل()(ثابت قدر_2& قدر) ثابت { أرجع عامل()(قدر.m_hash, 0); }
        سائيز_t عامل()(سائيز_t hash, سائيز_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2& a, ثابت قدر_2& b) ثابت {
            أرجع عامل()(a, b.m_hash, b.m_id);
        }
        منطقي عامل()(ثابت قدر_2& قدر, سائيز_t hash, سائيز_t id) ثابت {
            أرجع قدر.m_hash == hash && قدر.m_id == id;
        }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<قدر_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ثابت قدر_2 قدر{1, 0};
        ثابت قدر_2 equal{1, 0};  // Equal to 'value', and hashes the same
        سيٽ_2 سيٽ;
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Starts empty
        UASSERT_SELFTEST(سيٽ.begin() == سيٽ.پڇاڙي(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سيٽ.ڳوليو(قدر) == سيٽ.پڇاڙي(), سچ);  // And finds nothing
        ثابت std::pair<سيٽ_2::iterator, منطقي> added = سيٽ.وجهو(قدر);
        UASSERT_SELFTEST(added.ٻيو, سچ);  // Added, as it was absent
        UASSERT_SELFTEST(added.پهريون->m_id, قدر.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.پهريون != &قدر, سچ);  // Which is a copy of the argument
        UASSERT_SELFTEST(سيٽ.سائيز(), 1);  // And is the only one
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), خطأ);  // So the set is no longer empty
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.پهريون);  // At the entry held
        }
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(equal);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.پهريون);  // At that same entry
        }
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(1, سائيز_t{0});
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.پهريون);  // At that same entry again
        }
        UASSERT_SELFTEST(سيٽ.ڳوليو(1, سائيز_t{1}) == سيٽ.پڇاڙي(), سچ);  // Hash same, id not
        UASSERT_SELFTEST(سيٽ.ڳوليو(2, سائيز_t{0}) == سيٽ.پڇاڙي(), سچ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<سيٽ_2::iterator, منطقي> again = سيٽ.وجهو(equal);
        UASSERT_SELFTEST(again.ٻيو, خطأ);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.پهريون, &*added.پهريون);  // At the entry already held
        UASSERT_SELFTEST(سيٽ.سائيز(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Present before the erase
            سيٽ.erase(it);
        }
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Empty again
        UASSERT_SELFTEST(سيٽ.ڳوليو(قدر) == سيٽ.پڇاڙي(), سچ);  // So the entry is gone
        UASSERT_SELFTEST(سيٽ.شامل_آهي(قدر), خطأ);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(سيٽ.erase(قدر), خطأ);  // Nothing to erase, so says so
        سيٽ.وجهو(قدر);
        UASSERT_SELFTEST(سيٽ.erase(equal), سچ);  // An equal key erases the entry held
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        سيٽ.وجهو(قدر);
        UASSERT_SELFTEST(سيٽ.erase(1, سائيز_t{0}), سچ);  // Those parts find and erase it
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr سائيز_t N = GROWS_TWICE;
        سيٽ_2 سيٽ;
        لكل (سائيز_t i = 0; i < N; ++i) {
            ثابت std::pair<سيٽ_2::iterator, منطقي> added = سيٽ.وجهو(قدر_2{7, i});
            UASSERT_SELFTEST(added.ٻيو, سچ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), N);  // All of them, in the one probe run
        لكل (سائيز_t i = 0; i < N; ++i) {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(سائيز_t{7}, i);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<منطقي, N> ڏٺل{};
        سائيز_t n = 0;
        لكل (ثابت قدر_2& داخلا_2 : سيٽ) {
            UASSERT_SELFTEST(داخلا_2.m_hash, سائيز_t{7});  // Only entries added
            UASSERT_SELFTEST(ڏٺل[داخلا_2.m_id], خطأ);  // Each of them exactly once
            ڏٺل[داخلا_2.m_id] = سچ;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

فراغ test_اشارو_ڪنجيون() {
    بنية قدر_2 final {
        سائيز_t m_hash = 0;  // Hash of this entry
        سائيز_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        سائيز_t عامل()(ثابت قدر_2* valuep) ثابت { أرجع عامل()(valuep->m_hash, 0); }
        سائيز_t عامل()(سائيز_t hash, سائيز_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            أرجع عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عامل()(ثابت قدر_2* valuep, سائيز_t hash, سائيز_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<قدر_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        قدر_2 قدر{1, 0};
        قدر_2 equal{1, 0};  // Equal to 'value', and hashes the same
        سيٽ_2 سيٽ;
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Starts empty
        UASSERT_SELFTEST(سيٽ.begin() == سيٽ.پڇاڙي(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سيٽ.ڳوليو(&قدر) == سيٽ.پڇاڙي(), سچ);  // And finds nothing
        ثابت std::pair<سيٽ_2::iterator, منطقي> added = سيٽ.وجهو(&قدر);
        UASSERT_SELFTEST(added.ٻيو, سچ);  // Added, as it was absent
        UASSERT_SELFTEST(*added.پهريون, &قدر);  // The iterator is at the new entry
        UASSERT_SELFTEST(سيٽ.سائيز(), 1);  // Which is the only one
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), خطأ);  // So the set is no longer empty
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Found by itself
            UASSERT_SELFTEST(*it, &قدر);  // At the entry held
        }
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&equal);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // And by an equal entry
            UASSERT_SELFTEST(*it, &قدر);  // At that same entry
        }
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(1, سائيز_t{0});
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // And by its parts
            UASSERT_SELFTEST(*it, &قدر);  // At that same entry again
        }
        UASSERT_SELFTEST(سيٽ.ڳوليو(1, سائيز_t{1}) == سيٽ.پڇاڙي(), سچ);  // Hash same, id not
        UASSERT_SELFTEST(سيٽ.ڳوليو(2, سائيز_t{0}) == سيٽ.پڇاڙي(), سچ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<سيٽ_2::iterator, منطقي> again = سيٽ.وجهو(&equal);
        UASSERT_SELFTEST(again.ٻيو, خطأ);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.پهريون, &قدر);  // The iterator is at the stored one
        UASSERT_SELFTEST(سيٽ.سائيز(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&equal);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &قدر);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, خطأ);  // Not the one asked for
        }
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // The entry is still there
            UASSERT_SELFTEST(*it, &قدر);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Present before the erase
            UASSERT_SELFTEST(*it, &قدر);  // And is the object asked for
            سيٽ.erase(it);
        }
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Empty again
        UASSERT_SELFTEST(سيٽ.ڳوليو(&قدر) == سيٽ.پڇاڙي(), سچ);  // So the entry is gone
        UASSERT_SELFTEST(سيٽ.شامل_آهي(&قدر), خطأ);  // As 'contains' agrees
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(سيٽ.erase(&قدر), خطأ);  // Nothing to erase, so says so
        سيٽ.وجهو(&قدر);
        UASSERT_SELFTEST(سيٽ.erase(&equal), سچ);  // The equal key erases the stored one
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Which leaves the set empty
        UASSERT_SELFTEST(سيٽ.ڳوليو(&قدر) == سيٽ.پڇاڙي(), سچ);  // And unreachable
        // And by a key spelled as the parts of an entry
        سيٽ.وجهو(&قدر);
        UASSERT_SELFTEST(سيٽ.erase(1, سائيز_t{0}), سچ);  // Those parts find and erase it
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    لكل (سائيز_t erase = 0; erase < 4; ++erase) {
        لكل (ثابت سائيز_t hash : {سائيز_t{0}, سائيز_t{7}, ~سائيز_t{0}}) {  // Wraps too
            std::array<قدر_2, 4> قدرون{قدر_2{hash, 0}, قدر_2{hash, 1},  //
                                        قدر_2{hash, 2}, قدر_2{hash, 3}};
            سيٽ_2 سيٽ;
            لكل (قدر_2& قدر : قدرون) سيٽ.وجهو(&قدر);
            UASSERT_SELFTEST(سيٽ.سائيز(), 4);  // All distinct, so all added
            {
                ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&قدرون[erase]);
                UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // The one to erase is present
                UASSERT_SELFTEST(*it, &قدرون[erase]);  // And is the object held
                سيٽ.erase(it);
            }
            UASSERT_SELFTEST(سيٽ.سائيز(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(سيٽ.ڳوليو(&قدرون[erase]) == سيٽ.پڇاڙي(), سچ);  // That one
            لكل (سائيز_t i = 0; i < قدرون.سائيز(); ++i) {
                إذا (i == erase) استمر;
                ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(hash, i);
                UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // The others are all there
                UASSERT_SELFTEST(*it, &قدرون[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    لكل (ثابت سائيز_t hash : {سائيز_t{0}, سائيز_t{7}, ~سائيز_t{0}}) {
        std::array<قدر_2, GROWS_TWICE> many{};
        سيٽ_2 سيٽ;
        لكل (سائيز_t i = 0; i < many.سائيز(); ++i) {
            many[i] = قدر_2{hash, i};
            سيٽ.وجهو(&many[i]);
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), many.سائيز());  // All of them were added
        لكل (سائيز_t i = 0; i < many.سائيز(); ++i) {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(hash, i);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        قدر_2 قدر{1, 0};
        قدر_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        سيٽ_2 سيٽ;
        سيٽ.reserve(2);
        سيٽ.وجهو(&قدر);
        سيٽ.وجهو(&other);
        UASSERT_SELFTEST(سيٽ.سائيز(), 2);  // Both kept, despite the collision
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // The first of the run is there
            UASSERT_SELFTEST(*it, &قدر);  // And is that entry
        }
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&other);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Present before the erase
            UASSERT_SELFTEST(*it, &قدر);  // And is the object held
            سيٽ.erase(it);
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), 1);  // Only one was erased
        UASSERT_SELFTEST(سيٽ.ڳوليو(&قدر) == سيٽ.پڇاڙي(), سچ);  // Namely that one
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&other);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    لكل (ثابت منطقي doReserve : {خطأ, سچ}) {
        std::array<قدر_2, 8> قدرون{};
        سيٽ_2 سيٽ;
        إذا (doReserve) سيٽ.reserve(قدرون.سائيز());
        لكل (سائيز_t i = 0; i < قدرون.سائيز(); ++i) {
            قدرون[i] = قدر_2{static_cast<سائيز_t>(i * 1234567), i};
            سيٽ.وجهو(&قدرون[i]);
        }
        لكل (قدر_2& قدر : قدرون) {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(&قدر);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Each entry survives
            UASSERT_SELFTEST(*it, &قدر);  // And is the one inserted
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), قدرون.سائيز());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr سائيز_t N = GROWS_TWICE;
        std::array<قدر_2, N> قدرون{};
        سيٽ_2 سيٽ;
        لكل (سائيز_t i = 0; i < N; ++i) {
            قدرون[i] = قدر_2{7, i};  // All of them hash the same
            ثابت std::pair<سيٽ_2::iterator, منطقي> pair
                = سيٽ.وجهو_Lazy(سائيز_t{7}, i, [&]() -> قدر_2* { أرجع &قدرون[i]; });
            UASSERT_SELFTEST(pair.ٻيو, سچ);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.پهريون, &قدرون[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), N);  // All of them were added
        // They are all present now, so nothing is created
        لكل (سائيز_t i = 0; i < N; ++i) {
            منطقي created = خطأ;
            ثابت std::pair<سيٽ_2::iterator, منطقي> pair
                = سيٽ.وجهو_Lazy(سائيز_t{7}, i, [&]() -> قدر_2* {
                      created = سچ;  // LCOV_EXCL_START
                      أرجع &قدرون[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, خطأ);  // The factory was never called
            UASSERT_SELFTEST(pair.ٻيو, خطأ);  // As the lookup hit
            UASSERT_SELFTEST(*pair.پهريون, &قدرون[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<قدر_2, 20> قدرون{};
        std::array<منطقي, 20> ڏٺل{};
        سيٽ_2 سيٽ;
        لكل (سائيز_t i = 0; i < قدرون.سائيز(); ++i) {
            قدرون[i] = قدر_2{i / 2, i};  // Pairs of them collide
            سيٽ.وجهو(&قدرون[i]);
        }
        سائيز_t n = 0;
        لكل (قدر_2* ثابت valuep : سيٽ) {
            UASSERT_SELFTEST(valuep->m_id < ڏٺل.سائيز(), سچ);  // Only entries added
            UASSERT_SELFTEST(ڏٺل[valuep->m_id], خطأ);  // Each of them exactly once
            ڏٺل[valuep->m_id] = سچ;
            ++n;
        }
        UASSERT_SELFTEST(n, قدرون.سائيز());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

فراغ test_داخلا_Lifetime() {
    سائيز_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    صنف NoDefault final {
        سائيز_t* m_alivep;  // Where the live entries are counted
        سائيز_t m_hash;  // Hash of this entry
        سائيز_t m_id;  // Entries with equal ids are equal

    عام:
        NoDefault(سائيز_t* alivep, سائيز_t hash, سائيز_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ثابت NoDefault& that)
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
        NoDefault& عامل=(ثابت NoDefault&) = ميٽيو;
        NoDefault& عامل=(NoDefault&&) = ميٽيو;

        سائيز_t hash() ثابت { أرجع m_hash; }
        سائيز_t id() ثابت { أرجع m_id; }
        منطقي عامل==(ثابت NoDefault& that) ثابت { أرجع m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    بنية Hash final {
        سائيز_t عامل()(ثابت NoDefault& قدر) ثابت { أرجع قدر.hash(); }
        سائيز_t عامل()(سائيز_t hash, سائيز_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت NoDefault& a, ثابت NoDefault& b) ثابت {
            أرجع عامل()(a, b.hash(), b.id());
        }
        منطقي عامل()(ثابت NoDefault& a, سائيز_t, سائيز_t id) ثابت { أرجع a.id() == id; }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<NoDefault, Hash, Equal>;

    constexpr سائيز_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        سيٽ_2 سيٽ;
        لكل (سائيز_t i = 0; i < N; ++i) {
            ثابت std::pair<سيٽ_2::iterator, منطقي> added = سيٽ.وجهو(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.ٻيو, سچ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        لكل (سائيز_t i = 0; i < N; ++i) {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(سائيز_t{7}, i);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ثابت std::pair<سيٽ_2::iterator, منطقي> dup = سيٽ.وجهو(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.ٻيو, خطأ);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.پهريون->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(سائيز_t{7}, سائيز_t{0});
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Present before the erase
            UASSERT_SELFTEST(it->id(), سائيز_t{0});  // And is the entry asked for
            سيٽ.erase(it);
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(سيٽ.شامل_آهي(سائيز_t{7}, سائيز_t{0}), خطأ);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        لكل (سائيز_t i = 1; i < N; ++i) {
            {
                ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(سائيز_t{7}, i);
                UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                سيٽ.erase(it);
            }
            UASSERT_SELFTEST(سيٽ.سائيز(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            لكل (سائيز_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(سيٽ.شامل_آهي(سائيز_t{7}, j), سچ);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(سيٽ.begin() == سيٽ.پڇاڙي(), سچ);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        لكل (سائيز_t i = 0; i < N; ++i) سيٽ.وجهو(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        سيٽ.clear();
        UASSERT_SELFTEST(سيٽ.سائيز(), 0);  // Nothing left
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Says so
        UASSERT_SELFTEST(سيٽ.begin() == سيٽ.پڇاڙي(), سچ);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(سيٽ.شامل_آهي(سائيز_t{7}, سائيز_t{0}), خطأ);  // Nor finds any
        سيٽ.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        لكل (سائيز_t i = 0; i < N; ++i) سيٽ.وجهو(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        سيٽ_2 سيٽ;
        سيٽ.clear();
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // Still empty
        سيٽ.وجهو(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(سيٽ.سائيز(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

فراغ testMove() {
    سائيز_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    بنية Counted final {
        سائيز_t* m_alivep;  // Where the live entries are counted
        سائيز_t m_id;  // Entries with equal ids are equal

        Counted(سائيز_t* alivep, سائيز_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ثابت Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& عامل=(ثابت Counted&) = ميٽيو;
        Counted& عامل=(Counted&&) = ميٽيو;
    };

    بنية Hash final {
        سائيز_t عامل()(ثابت Counted& داخلا_2) ثابت { أرجع عامل()(داخلا_2.m_id); }
        سائيز_t عامل()(سائيز_t id) ثابت { أرجع id; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت Counted& a, ثابت Counted& b) ثابت { أرجع عامل()(a, b.m_id); }
        منطقي عامل()(ثابت Counted& a, سائيز_t id) ثابت { أرجع a.m_id == id; }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<Counted, Hash, Equal>;

    constexpr سائيز_t N = GROWS_TWICE;
    {
        سيٽ_2 سيٽ;
        لكل (سائيز_t i = 0; i < N; ++i) سيٽ.وجهو(Counted{&alive, i});
        UASSERT_SELFTEST(سيٽ.سائيز(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        سيٽ_2 moved{std::move(سيٽ)};
        UASSERT_SELFTEST(moved.سائيز(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // And the source no longer holds them
        UASSERT_SELFTEST(سيٽ.begin() == سيٽ.پڇاڙي(), سچ);  // So it iterates nothing
        لكل (سائيز_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.شامل_آهي(i), سچ);  // Every entry came across
            UASSERT_SELFTEST(سيٽ.شامل_آهي(i), خطأ);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(سيٽ.وجهو(Counted{&alive, N}).ٻيو, سچ);  // It took an entry
        UASSERT_SELFTEST(سيٽ.سائيز(), 1);  // Which is all it holds
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), خطأ);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        سيٽ = std::move(moved);
        UASSERT_SELFTEST(سيٽ.سائيز(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(سيٽ.شامل_آهي(N), خطأ);  // Namely that one
        UASSERT_SELFTEST(moved.خالي_جايون(), سچ);  // And the source is empty again
        لكل (سائيز_t i = 0; i < N; ++i) UASSERT_SELFTEST(سيٽ.شامل_آهي(i), سچ);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

فراغ testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    صنف MoveOnly final {
        سائيز_t m_id;  // Entries with equal ids are equal

    عام:
        واضح MoveOnly(سائيز_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = افتراضي;
        MoveOnly(ثابت MoveOnly&) = ميٽيو;
        MoveOnly& عامل=(ثابت MoveOnly&) = ميٽيو;
        MoveOnly& عامل=(MoveOnly&&) = ميٽيو;
        ~MoveOnly() = افتراضي;

        سائيز_t id() ثابت { أرجع m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::قدر,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::قدر,
                  "SelfTest: 'MoveOnly' must be move constructible");

    بنية Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        سائيز_t عامل()(ثابت MoveOnly& قدر) ثابت { أرجع عامل()(قدر.id()); }
        سائيز_t عامل()(سائيز_t) ثابت { أرجع 7; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت MoveOnly& a, ثابت MoveOnly& b) ثابت {
            أرجع عامل()(a, b.id());
        }
        منطقي عامل()(ثابت MoveOnly& a, سائيز_t id) ثابت { أرجع a.id() == id; }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<MoveOnly, Hash, Equal>;

    constexpr سائيز_t N = GROWS_TWICE;
    سيٽ_2 سيٽ;
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت std::pair<سيٽ_2::iterator, منطقي> added
            = سيٽ.وجهو_Lazy(i, [i] { أرجع MoveOnly{i}; });
        UASSERT_SELFTEST(added.ٻيو, سچ);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.پهريون->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(سيٽ.سائيز(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    لكل (سائيز_t i = 0; i < N; i += 2) {
        ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(i);
        UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Present before the erase
        سيٽ.erase(it);
    }
    UASSERT_SELFTEST(سيٽ.سائيز(), N / 2);  // Half of them are gone
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت منطقي erased = (i % 2) == 0;
        UASSERT_SELFTEST(سيٽ.شامل_آهي(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

فراغ testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    صنف Hash final {
        سائيز_t m_نقاب;  // Only these bits of an entry matter

    عام:
        واضح Hash(سائيز_t نقاب)
            : m_نقاب{نقاب} {}
        سائيز_t عامل()(سائيز_t قدر) ثابت { أرجع قدر & m_نقاب; }
    };

    صنف Equal final {
        سائيز_t m_نقاب;  // Only these bits of an entry matter

    عام:
        واضح Equal(سائيز_t نقاب)
            : m_نقاب{نقاب} {}
        منطقي عامل()(سائيز_t a, سائيز_t b) ثابت { أرجع (a & m_نقاب) == (b & m_نقاب); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::قدر,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::قدر,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_سيٽ<سائيز_t, Hash, Equal> سيٽ{Hash{3}, Equal{3}};
    لكل (سائيز_t i = 0; i < 16; ++i) سيٽ.وجهو(i);
    UASSERT_SELFTEST(سيٽ.سائيز(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    لكل (سائيز_t i = 0; i < 16; ++i) {
        ثابت V3_Hash_سيٽ<سائيز_t, Hash, Equal>::iterator it = سيٽ.ڳوليو(i);
        UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

فراغ test_Backward_سرڪايو_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    بنية قدر_2 final {
        سائيز_t m_hash = 0;  // Hash of this entry
        سائيز_t m_id = 0;  // Entries with equal ids are equal
    };

    بنية Hash final {
        سائيز_t عامل()(ثابت قدر_2* valuep) ثابت { أرجع عامل()(valuep->m_hash, 0); }
        سائيز_t عامل()(سائيز_t hash, سائيز_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            أرجع عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عامل()(ثابت قدر_2* valuep, سائيز_t hash, سائيز_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<قدر_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    لكل (ثابت سائيز_t h : {سائيز_t{0}, سائيز_t{5}, ~سائيز_t{0}}) {  // Wraps too
        std::array<قدر_2, 4> قدرون{قدر_2{h, 0}, قدر_2{h + 1, 1},  //
                                    قدر_2{h, 2}, قدر_2{h + 1, 3}};
        سيٽ_2 سيٽ;
        لكل (قدر_2& قدر : قدرون) سيٽ.وجهو(&قدر);
        UASSERT_SELFTEST(سيٽ.سائيز(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(h, سائيز_t{0});
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Present before the erase
            سيٽ.erase(it);
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), 3);  // One fewer entry
        UASSERT_SELFTEST(سيٽ.شامل_آهي(h, سائيز_t{0}), خطأ);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        لكل (سائيز_t i = 1; i < قدرون.سائيز(); ++i) {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(قدرون[i].m_hash, i);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &قدرون[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(h + 1, سائيز_t{1});
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Left where the first erase found it
            سيٽ.erase(it);
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), 2);  // Two are left
        لكل (سائيز_t i = 2; i < قدرون.سائيز(); ++i) {
            ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(قدرون[i].m_hash, i);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Both still reachable
            UASSERT_SELFTEST(*it, &قدرون[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<منطقي, 4> ڏٺل{};
        لكل (ثابت قدر_2* ثابت valuep : سيٽ) {
            UASSERT_SELFTEST(ڏٺل[valuep->m_id], خطأ);  // Each entry once
            ڏٺل[valuep->m_id] = سچ;
        }
        UASSERT_SELFTEST(ڏٺل[0], خطأ);  // Erased first
        UASSERT_SELFTEST(ڏٺل[1], خطأ);  // Erased second
        UASSERT_SELFTEST(ڏٺل[2], سچ);  // Moved back over the first hole
        UASSERT_SELFTEST(ڏٺل[3], سچ);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

فراغ testReferenceStability() {
    بنية قدر_2 final {
        سائيز_t m_hash = 0;  // Hash of this entry
        سائيز_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        سائيز_t عامل()(ثابت قدر_2* valuep) ثابت { أرجع عامل()(valuep->m_hash, 0); }
        سائيز_t عامل()(سائيز_t hash, سائيز_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            أرجع عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عامل()(ثابت قدر_2* valuep, سائيز_t hash, سائيز_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<قدر_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        سيٽ_2 سيٽ;
        سيٽ.reserve(8);
        UASSERT_SELFTEST(سيٽ.شامل_آهي(سائيز_t{7}, سائيز_t{0}), خطأ);  // Room, but no entry
        UASSERT_SELFTEST(سيٽ.begin() == سيٽ.پڇاڙي(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سيٽ.خالي_جايون(), سچ);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr سائيز_t N = max_ڀريو(2 * MIN_صلاحيت);
    static_assert(N > max_ڀريو(MIN_صلاحيت), "SelfTest: 'N' must need more than a new table");
    std::array<قدر_2, N> قدرون{};
    std::array<قدر_2* ثابت*, N> entrypps{};  // Where each entry is stored, as inserted
    سيٽ_2 سيٽ;
    سيٽ.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    لكل (سائيز_t i = 0; i < N; ++i) {
        قدرون[i] = قدر_2{7, i};
        ثابت std::pair<سيٽ_2::iterator, منطقي> pair = سيٽ.وجهو(&قدرون[i]);
        UASSERT_SELFTEST(pair.ٻيو, سچ);  // Ids differ, so each is added
        entrypps[i] = &*pair.پهريون;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(سائيز_t{7}, i);
        UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت std::pair<سيٽ_2::iterator, منطقي> pair = سيٽ.وجهو(&قدرون[i]);
        UASSERT_SELFTEST(pair.ٻيو, خطأ);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.پهريون, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(سيٽ.سائيز(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    سيٽ.reserve(8 * N);
    UASSERT_SELFTEST(سيٽ.سائيز(), N);  // Rehashing dropped nothing
    std::array<قدر_2* ثابت*, N> grownpps{};  // Where each entry is after the growth
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(سائيز_t{7}, i);
        UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    سيٽ.reserve(N);
    UASSERT_SELFTEST(سيٽ.سائيز(), N);  // The smaller request changed nothing
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت سيٽ_2::iterator it = سيٽ.ڳوليو(سائيز_t{7}, i);
        UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), سچ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
بنية ClusteredHash final {
    سائيز_t عامل()(سائيز_t قدر) ثابت { أرجع قدر & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
بنية پکيڙ_Hash final {
    سائيز_t عامل()(سائيز_t قدر) ثابت {
        أرجع static_cast<سائيز_t>(قدر * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
سانچو <قسم_نالو T_Hash>
فراغ test_سيٽ_Against_Model() {
    بنية Equal final {
        منطقي عامل()(سائيز_t a, سائيز_t b) ثابت { أرجع a == b; }
    };

    استعمال سيٽ_2 = V3_Hash_سيٽ<سائيز_t, T_Hash, Equal>;

    constexpr سائيز_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr سائيز_t قدمن = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ثابت خودڪار ايندڙ_Rand = [&state]() -> سائيز_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        أرجع static_cast<سائيز_t>(state >> 32);
    };

    سيٽ_2 سيٽ;
    std::سيٽ<سائيز_t> model;

    لكل (سائيز_t قدم = 0; قدم < قدمن; ++قدم) {
        ثابت سائيز_t id = ايندڙ_Rand() % UNIVERSE;
        بدل (ايندڙ_Rand() % 4) {
        حالة 0: {  // Insert a copy
            ثابت std::pair<قسم_نالو سيٽ_2::iterator, منطقي> pair = سيٽ.وجهو(id);
            UASSERT_SELFTEST(pair.ٻيو, model.وجهو(id).ٻيو);  // As the model
            UASSERT_SELFTEST(*pair.پهريون, id);  // At the entry asked for
            اكسر;
        }
        حالة 1: {  // Insert lazily, which must create only on a miss
            منطقي created = خطأ;
            ثابت std::pair<قسم_نالو سيٽ_2::iterator, منطقي> pair
                = سيٽ.وجهو_Lazy(id, [&]() -> سائيز_t {
                      created = سچ;
                      أرجع id;
                  });
            UASSERT_SELFTEST(pair.ٻيو, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.ٻيو, model.وجهو(id).ٻيو);  // As the model
            UASSERT_SELFTEST(*pair.پهريون, id);  // At the entry asked for
            اكسر;
        }
        حالة 2: {  // Erase, if present
            ثابت قسم_نالو سيٽ_2::iterator it = سيٽ.ڳوليو(id);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), model.ڳڻپ(id) != 0);  // As the model
            إذا (it != سيٽ.پڇاڙي()) {
                سيٽ.erase(it);
                model.erase(id);
            }
            اكسر;
        }
        افتراضي: {  // Look up only
            ثابت قسم_نالو سيٽ_2::iterator it = سيٽ.ڳوليو(id);
            UASSERT_SELFTEST(it != سيٽ.پڇاڙي(), model.ڳڻپ(id) != 0);  // As the model
            إذا (it != سيٽ.پڇاڙي()) UASSERT_SELFTEST(*it, id);  // And is the entry
            اكسر;
        }
        }
        UASSERT_SELFTEST(سيٽ.سائيز(), model.سائيز());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    لكل (سائيز_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(سيٽ.شامل_آهي(id), model.ڳڻپ(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقي, UNIVERSE> ڏٺل{};
    سائيز_t n = 0;
    لكل (ثابت سائيز_t داخلا_2 : سيٽ) {
        UASSERT_SELFTEST(داخلا_2 < UNIVERSE, سچ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ڳڻپ(داخلا_2) != 0, سچ);  // Only what was inserted
        UASSERT_SELFTEST(ڏٺل[داخلا_2], خطأ);  // Each entry once
        ڏٺل[داخلا_2] = سچ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.سائيز());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

فراغ testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    استعمال Map = V3HashMap<std::string, صحيح>;
    استعمال داخلا_3 = Map::داخلا_3;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<داخلا_3, std::pair<std::string, صحيح>>::قدر,
                  "SelfTest: a map entry must be a plain pair");

    constexpr سائيز_t N = GROWS_TWICE;
    Map map;
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت std::string ڪنجي = "key" + std::منزل_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ثابت std::pair<Map::iterator, منطقي> pair
            = map.وجهو_Lazy(ڪنجي, [&]() -> داخلا_3 { أرجع {ڪنجي, static_cast<صحيح>(i)}; });
        UASSERT_SELFTEST(pair.ٻيو, سچ);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.سائيز(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.ڳوليو("key" + std::منزل_string(i));
        UASSERT_SELFTEST(it != map.پڇاڙي(), سچ);  // The key alone finds it
        UASSERT_SELFTEST(it->ٻيو, static_cast<صحيح>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت std::string ڪنجي = "key" + std::منزل_string(i);
        map.erase(map.ڳوليو(ڪنجي));
        UASSERT_SELFTEST(map.وجهو({ڪنجي, static_cast<صحيح>(i) + 100}).ٻيو, سچ);  // Anew
    }
    UASSERT_SELFTEST(map.سائيز(), N);  // The same keys, so the map has not grown
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.ڳوليو("key" + std::منزل_string(i));
        UASSERT_SELFTEST(it != map.پڇاڙي(), سچ);  // Each key is still there
        UASSERT_SELFTEST(it->ٻيو, static_cast<صحيح>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    لكل (سائيز_t i = 0; i < N; ++i) map.ڳوليو("key" + std::منزل_string(i)).قدر() += 100;
    UASSERT_SELFTEST(map.سائيز(), N);  // The map is unchanged otherwise
    لكل (سائيز_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.ڳوليو("key" + std::منزل_string(i));
        UASSERT_SELFTEST(it != map.پڇاڙي(), سچ);  // Each key still finds its entry
        UASSERT_SELFTEST(it->ٻيو, static_cast<صحيح>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    لكل (سائيز_t i = 0; i < N; ++i) map.ڳوليو("key" + std::منزل_string(i)).قدر() -= 100;

    // Adding a key that is present changes nothing
    {
        ثابت std::string ڪنجي = "key0";
        منطقي created = خطأ;
        ثابت std::pair<Map::iterator, منطقي> pair = map.وجهو_Lazy(ڪنجي, [&]() -> داخلا_3 {
            created = سچ;  // LCOV_EXCL_START
            أرجع {ڪنجي, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, خطأ);  // The factory was never called
        UASSERT_SELFTEST(pair.ٻيو, خطأ);  // As the key is present
        UASSERT_SELFTEST(pair.پهريون->ٻيو, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.سائيز(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ثابت std::pair<Map::iterator, منطقي> added = map.وجهو({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.ٻيو, سچ);  // Added, as the key is new
        UASSERT_SELFTEST(added.پهريون->ٻيو, 5);  // With the value given
        ثابت std::pair<Map::iterator, منطقي> again = map.وجهو({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.ٻيو, خطأ);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.پهريون->ٻيو, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.سائيز(), N + 1);  // Just the one entry was added
    {
        ثابت Map::iterator it = map.ڳوليو(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.پڇاڙي(), سچ);  // Present before the erase
        UASSERT_SELFTEST(it->ٻيو, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.سائيز(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), خطأ);  // No such key
    UASSERT_SELFTEST(map.سائيز(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), سچ);  // That key is present
    UASSERT_SELFTEST(map.سائيز(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.شامل_آهي(std::string{"key0"}), خطأ);  // Namely that one
    لكل (سائيز_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.شامل_آهي("key" + std::منزل_string(i)), سچ);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
سانچو <قسم_نالو T_Hash>
فراغ testMapAgainstModel() {
    بنية Equal final {
        منطقي عامل()(سائيز_t a, سائيز_t b) ثابت { أرجع a == b; }
    };

    استعمال Map = V3HashMap<سائيز_t, سائيز_t, T_Hash, Equal>;
    استعمال داخلا_3 = قسم_نالو Map::داخلا_3;

    constexpr سائيز_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr سائيز_t قدمن = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ثابت خودڪار ايندڙ_Rand = [&state]() -> سائيز_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        أرجع static_cast<سائيز_t>(state >> 32);
    };

    Map map;
    std::map<سائيز_t, سائيز_t> model;

    لكل (سائيز_t قدم = 0; قدم < قدمن; ++قدم) {
        ثابت سائيز_t ڪنجي = ايندڙ_Rand() % UNIVERSE;
        ثابت سائيز_t val = قدم;  // Distinct every step, so a stale value is visible
        بدل (ايندڙ_Rand() % 5) {
        حالة 0: {  // Insert a whole entry
            ثابت std::pair<قسم_نالو Map::iterator, منطقي> pair = map.وجهو({ڪنجي, val});
            UASSERT_SELFTEST(pair.ٻيو, model.وجهو({ڪنجي, val}).ٻيو);  // Ditto
            UASSERT_SELFTEST(pair.پهريون->پهريون, ڪنجي);  // At the key asked for
            اكسر;
        }
        حالة 1: {  // Insert lazily, which must create only on a miss
            منطقي created = خطأ;
            ثابت std::pair<قسم_نالو Map::iterator, منطقي> pair
                = map.وجهو_Lazy(ڪنجي, [&]() -> داخلا_3 {
                      created = سچ;
                      أرجع {ڪنجي, val};
                  });
            UASSERT_SELFTEST(pair.ٻيو, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.ٻيو, model.وجهو({ڪنجي, val}).ٻيو);  // Ditto
            UASSERT_SELFTEST(pair.پهريون->پهريون, ڪنجي);  // At the key asked for
            اكسر;
        }
        حالة 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(ڪنجي), model.erase(ڪنجي) != 0);  // As model
            اكسر;
        }
        حالة 3: {  // Change a value, which is erasing the entry and inserting it again
            ثابت قسم_نالو Map::iterator it = map.ڳوليو(ڪنجي);
            إذا (it != map.پڇاڙي()) {
                map.erase(it);
                UASSERT_SELFTEST(map.وجهو({ڪنجي, val}).ٻيو, سچ);  // The key is free
                model[ڪنجي] = val;
            }
            اكسر;
        }
        افتراضي: {  // Look up only
            ثابت قسم_نالو Map::iterator it = map.ڳوليو(ڪنجي);
            UASSERT_SELFTEST(it != map.پڇاڙي(), model.ڳڻپ(ڪنجي) != 0);  // As the model
            إذا (it != map.پڇاڙي()) {
                UASSERT_SELFTEST(it->پهريون, ڪنجي);  // At the key asked for
                UASSERT_SELFTEST(it->ٻيو, model.at(ڪنجي));  // With its value
            }
            اكسر;
        }
        }
        UASSERT_SELFTEST(map.سائيز(), model.سائيز());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    لكل (سائيز_t ڪنجي = 0; ڪنجي < UNIVERSE; ++ڪنجي) {
        ثابت قسم_نالو Map::iterator it = map.ڳوليو(ڪنجي);
        UASSERT_SELFTEST(it != map.پڇاڙي(), model.ڳڻپ(ڪنجي) != 0);  // Present iff modelled
        إذا (it != map.پڇاڙي()) {
            UASSERT_SELFTEST(it->ٻيو, model.at(ڪنجي));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقي, UNIVERSE> ڏٺل{};
    سائيز_t n = 0;
    لكل (ثابت داخلا_3& داخلا_2 : map) {
        UASSERT_SELFTEST(داخلا_2.پهريون < UNIVERSE, سچ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ڳڻپ(داخلا_2.پهريون) != 0, سچ);  // Only what was inserted
        UASSERT_SELFTEST(داخلا_2.ٻيو, model.at(داخلا_2.پهريون));  // With the right value
        UASSERT_SELFTEST(ڏٺل[داخلا_2.پهريون], خطأ);  // Each entry once
        ڏٺل[داخلا_2.پهريون] = سچ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.سائيز());  // And all of them
}

فراغ selfTest() {
    test_قدر_ڪنجيون();
    test_اشارو_ڪنجيون();
    test_داخلا_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_سرڪايو_Deletion();
    testReferenceStability();
    test_سيٽ_Against_Model<ClusteredHash>();
    test_سيٽ_Against_Model<پکيڙ_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<پکيڙ_Hash>();
}

}  // namespace V3HashTableInternals
