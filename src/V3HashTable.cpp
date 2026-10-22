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
constexpr ناپ_t max_لدو(ناپ_t سمرتھا) { أرجع سمرتھا * لدو_ضارب_عدد / لدو_ضارب_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr ناپ_t GROWS_TWICE = max_لدو(4 * MIN_سمرتھا);
static_assert(GROWS_TWICE > max_لدو(2 * MIN_سمرتھا),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

فراغ test_قدر_کلیدیں() {
    بنية قدر_2 final {
        ناپ_t m_hash = 0;  // Hash of this entry
        ناپ_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        ناپ_t عامل()(ثابت قدر_2& قدر) ثابت { أرجع عامل()(قدر.m_hash, 0); }
        ناپ_t عامل()(ناپ_t hash, ناپ_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2& a, ثابت قدر_2& b) ثابت {
            أرجع عامل()(a, b.m_hash, b.m_id);
        }
        منطقي عامل()(ثابت قدر_2& قدر, ناپ_t hash, ناپ_t id) ثابت {
            أرجع قدر.m_hash == hash && قدر.m_id == id;
        }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<قدر_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ثابت قدر_2 قدر{1, 0};
        ثابت قدر_2 equal{1, 0};  // Equal to 'value', and hashes the same
        سیٹ_2 سیٹ;
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Starts empty
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.سرا(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سیٹ.لبھو(قدر) == سیٹ.سرا(), سچ);  // And finds nothing
        ثابت std::pair<سیٹ_2::iterator, منطقي> added = سیٹ.داخل_کرو(قدر);
        UASSERT_SELFTEST(added.دوجا, سچ);  // Added, as it was absent
        UASSERT_SELFTEST(added.پہلا->m_id, قدر.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.پہلا != &قدر, سچ);  // Which is a copy of the argument
        UASSERT_SELFTEST(سیٹ.ناپ(), 1);  // And is the only one
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), خطأ);  // So the set is no longer empty
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.پہلا);  // At the entry held
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(equal);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.پہلا);  // At that same entry
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(1, ناپ_t{0});
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.پہلا);  // At that same entry again
        }
        UASSERT_SELFTEST(سیٹ.لبھو(1, ناپ_t{1}) == سیٹ.سرا(), سچ);  // Hash same, id not
        UASSERT_SELFTEST(سیٹ.لبھو(2, ناپ_t{0}) == سیٹ.سرا(), سچ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<سیٹ_2::iterator, منطقي> again = سیٹ.داخل_کرو(equal);
        UASSERT_SELFTEST(again.دوجا, خطأ);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.پہلا, &*added.پہلا);  // At the entry already held
        UASSERT_SELFTEST(سیٹ.ناپ(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Present before the erase
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Empty again
        UASSERT_SELFTEST(سیٹ.لبھو(قدر) == سیٹ.سرا(), سچ);  // So the entry is gone
        UASSERT_SELFTEST(سیٹ.شامل_اے(قدر), خطأ);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(سیٹ.erase(قدر), خطأ);  // Nothing to erase, so says so
        سیٹ.داخل_کرو(قدر);
        UASSERT_SELFTEST(سیٹ.erase(equal), سچ);  // An equal key erases the entry held
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        سیٹ.داخل_کرو(قدر);
        UASSERT_SELFTEST(سیٹ.erase(1, ناپ_t{0}), سچ);  // Those parts find and erase it
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr ناپ_t N = GROWS_TWICE;
        سیٹ_2 سیٹ;
        لكل (ناپ_t i = 0; i < N; ++i) {
            ثابت std::pair<سیٹ_2::iterator, منطقي> added = سیٹ.داخل_کرو(قدر_2{7, i});
            UASSERT_SELFTEST(added.دوجا, سچ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), N);  // All of them, in the one probe run
        لكل (ناپ_t i = 0; i < N; ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(ناپ_t{7}, i);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<منطقي, N> تکیا{};
        ناپ_t n = 0;
        لكل (ثابت قدر_2& اندراج : سیٹ) {
            UASSERT_SELFTEST(اندراج.m_hash, ناپ_t{7});  // Only entries added
            UASSERT_SELFTEST(تکیا[اندراج.m_id], خطأ);  // Each of them exactly once
            تکیا[اندراج.m_id] = سچ;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

فراغ test_اشارہ_گر_کلیدیں() {
    بنية قدر_2 final {
        ناپ_t m_hash = 0;  // Hash of this entry
        ناپ_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        ناپ_t عامل()(ثابت قدر_2* valuep) ثابت { أرجع عامل()(valuep->m_hash, 0); }
        ناپ_t عامل()(ناپ_t hash, ناپ_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            أرجع عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عامل()(ثابت قدر_2* valuep, ناپ_t hash, ناپ_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<قدر_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        قدر_2 قدر{1, 0};
        قدر_2 equal{1, 0};  // Equal to 'value', and hashes the same
        سیٹ_2 سیٹ;
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Starts empty
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.سرا(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سیٹ.لبھو(&قدر) == سیٹ.سرا(), سچ);  // And finds nothing
        ثابت std::pair<سیٹ_2::iterator, منطقي> added = سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(added.دوجا, سچ);  // Added, as it was absent
        UASSERT_SELFTEST(*added.پہلا, &قدر);  // The iterator is at the new entry
        UASSERT_SELFTEST(سیٹ.ناپ(), 1);  // Which is the only one
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), خطأ);  // So the set is no longer empty
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Found by itself
            UASSERT_SELFTEST(*it, &قدر);  // At the entry held
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&equal);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // And by an equal entry
            UASSERT_SELFTEST(*it, &قدر);  // At that same entry
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(1, ناپ_t{0});
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // And by its parts
            UASSERT_SELFTEST(*it, &قدر);  // At that same entry again
        }
        UASSERT_SELFTEST(سیٹ.لبھو(1, ناپ_t{1}) == سیٹ.سرا(), سچ);  // Hash same, id not
        UASSERT_SELFTEST(سیٹ.لبھو(2, ناپ_t{0}) == سیٹ.سرا(), سچ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<سیٹ_2::iterator, منطقي> again = سیٹ.داخل_کرو(&equal);
        UASSERT_SELFTEST(again.دوجا, خطأ);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.پہلا, &قدر);  // The iterator is at the stored one
        UASSERT_SELFTEST(سیٹ.ناپ(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&equal);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &قدر);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, خطأ);  // Not the one asked for
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // The entry is still there
            UASSERT_SELFTEST(*it, &قدر);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Present before the erase
            UASSERT_SELFTEST(*it, &قدر);  // And is the object asked for
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Empty again
        UASSERT_SELFTEST(سیٹ.لبھو(&قدر) == سیٹ.سرا(), سچ);  // So the entry is gone
        UASSERT_SELFTEST(سیٹ.شامل_اے(&قدر), خطأ);  // As 'contains' agrees
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(سیٹ.erase(&قدر), خطأ);  // Nothing to erase, so says so
        سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(سیٹ.erase(&equal), سچ);  // The equal key erases the stored one
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Which leaves the set empty
        UASSERT_SELFTEST(سیٹ.لبھو(&قدر) == سیٹ.سرا(), سچ);  // And unreachable
        // And by a key spelled as the parts of an entry
        سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(سیٹ.erase(1, ناپ_t{0}), سچ);  // Those parts find and erase it
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    لكل (ناپ_t erase = 0; erase < 4; ++erase) {
        لكل (ثابت ناپ_t hash : {ناپ_t{0}, ناپ_t{7}, ~ناپ_t{0}}) {  // Wraps too
            std::array<قدر_2, 4> اقدار{قدر_2{hash, 0}, قدر_2{hash, 1},  //
                                        قدر_2{hash, 2}, قدر_2{hash, 3}};
            سیٹ_2 سیٹ;
            لكل (قدر_2& قدر : اقدار) سیٹ.داخل_کرو(&قدر);
            UASSERT_SELFTEST(سیٹ.ناپ(), 4);  // All distinct, so all added
            {
                ثابت سیٹ_2::iterator it = سیٹ.لبھو(&اقدار[erase]);
                UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // The one to erase is present
                UASSERT_SELFTEST(*it, &اقدار[erase]);  // And is the object held
                سیٹ.erase(it);
            }
            UASSERT_SELFTEST(سیٹ.ناپ(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(سیٹ.لبھو(&اقدار[erase]) == سیٹ.سرا(), سچ);  // That one
            لكل (ناپ_t i = 0; i < اقدار.ناپ(); ++i) {
                إذا (i == erase) استمر;
                ثابت سیٹ_2::iterator it = سیٹ.لبھو(hash, i);
                UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // The others are all there
                UASSERT_SELFTEST(*it, &اقدار[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    لكل (ثابت ناپ_t hash : {ناپ_t{0}, ناپ_t{7}, ~ناپ_t{0}}) {
        std::array<قدر_2, GROWS_TWICE> many{};
        سیٹ_2 سیٹ;
        لكل (ناپ_t i = 0; i < many.ناپ(); ++i) {
            many[i] = قدر_2{hash, i};
            سیٹ.داخل_کرو(&many[i]);
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), many.ناپ());  // All of them were added
        لكل (ناپ_t i = 0; i < many.ناپ(); ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(hash, i);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        قدر_2 قدر{1, 0};
        قدر_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        سیٹ_2 سیٹ;
        سیٹ.reserve(2);
        سیٹ.داخل_کرو(&قدر);
        سیٹ.داخل_کرو(&other);
        UASSERT_SELFTEST(سیٹ.ناپ(), 2);  // Both kept, despite the collision
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // The first of the run is there
            UASSERT_SELFTEST(*it, &قدر);  // And is that entry
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&other);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Present before the erase
            UASSERT_SELFTEST(*it, &قدر);  // And is the object held
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), 1);  // Only one was erased
        UASSERT_SELFTEST(سیٹ.لبھو(&قدر) == سیٹ.سرا(), سچ);  // Namely that one
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&other);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    لكل (ثابت منطقي doReserve : {خطأ, سچ}) {
        std::array<قدر_2, 8> اقدار{};
        سیٹ_2 سیٹ;
        إذا (doReserve) سیٹ.reserve(اقدار.ناپ());
        لكل (ناپ_t i = 0; i < اقدار.ناپ(); ++i) {
            اقدار[i] = قدر_2{static_cast<ناپ_t>(i * 1234567), i};
            سیٹ.داخل_کرو(&اقدار[i]);
        }
        لكل (قدر_2& قدر : اقدار) {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Each entry survives
            UASSERT_SELFTEST(*it, &قدر);  // And is the one inserted
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), اقدار.ناپ());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr ناپ_t N = GROWS_TWICE;
        std::array<قدر_2, N> اقدار{};
        سیٹ_2 سیٹ;
        لكل (ناپ_t i = 0; i < N; ++i) {
            اقدار[i] = قدر_2{7, i};  // All of them hash the same
            ثابت std::pair<سیٹ_2::iterator, منطقي> pair
                = سیٹ.داخل_کرو_Lazy(ناپ_t{7}, i, [&]() -> قدر_2* { أرجع &اقدار[i]; });
            UASSERT_SELFTEST(pair.دوجا, سچ);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.پہلا, &اقدار[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), N);  // All of them were added
        // They are all present now, so nothing is created
        لكل (ناپ_t i = 0; i < N; ++i) {
            منطقي created = خطأ;
            ثابت std::pair<سیٹ_2::iterator, منطقي> pair
                = سیٹ.داخل_کرو_Lazy(ناپ_t{7}, i, [&]() -> قدر_2* {
                      created = سچ;  // LCOV_EXCL_START
                      أرجع &اقدار[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, خطأ);  // The factory was never called
            UASSERT_SELFTEST(pair.دوجا, خطأ);  // As the lookup hit
            UASSERT_SELFTEST(*pair.پہلا, &اقدار[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<قدر_2, 20> اقدار{};
        std::array<منطقي, 20> تکیا{};
        سیٹ_2 سیٹ;
        لكل (ناپ_t i = 0; i < اقدار.ناپ(); ++i) {
            اقدار[i] = قدر_2{i / 2, i};  // Pairs of them collide
            سیٹ.داخل_کرو(&اقدار[i]);
        }
        ناپ_t n = 0;
        لكل (قدر_2* ثابت valuep : سیٹ) {
            UASSERT_SELFTEST(valuep->m_id < تکیا.ناپ(), سچ);  // Only entries added
            UASSERT_SELFTEST(تکیا[valuep->m_id], خطأ);  // Each of them exactly once
            تکیا[valuep->m_id] = سچ;
            ++n;
        }
        UASSERT_SELFTEST(n, اقدار.ناپ());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

فراغ test_اندراج_Lifetime() {
    ناپ_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    صنف NoDefault final {
        ناپ_t* m_alivep;  // Where the live entries are counted
        ناپ_t m_hash;  // Hash of this entry
        ناپ_t m_id;  // Entries with equal ids are equal

    عام:
        NoDefault(ناپ_t* alivep, ناپ_t hash, ناپ_t id)
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
        NoDefault& عامل=(ثابت NoDefault&) = مٹاؤ;
        NoDefault& عامل=(NoDefault&&) = مٹاؤ;

        ناپ_t hash() ثابت { أرجع m_hash; }
        ناپ_t id() ثابت { أرجع m_id; }
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
        ناپ_t عامل()(ثابت NoDefault& قدر) ثابت { أرجع قدر.hash(); }
        ناپ_t عامل()(ناپ_t hash, ناپ_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت NoDefault& a, ثابت NoDefault& b) ثابت {
            أرجع عامل()(a, b.hash(), b.id());
        }
        منطقي عامل()(ثابت NoDefault& a, ناپ_t, ناپ_t id) ثابت { أرجع a.id() == id; }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<NoDefault, Hash, Equal>;

    constexpr ناپ_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        سیٹ_2 سیٹ;
        لكل (ناپ_t i = 0; i < N; ++i) {
            ثابت std::pair<سیٹ_2::iterator, منطقي> added = سیٹ.داخل_کرو(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.دوجا, سچ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        لكل (ناپ_t i = 0; i < N; ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(ناپ_t{7}, i);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ثابت std::pair<سیٹ_2::iterator, منطقي> dup = سیٹ.داخل_کرو(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.دوجا, خطأ);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.پہلا->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(ناپ_t{7}, ناپ_t{0});
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Present before the erase
            UASSERT_SELFTEST(it->id(), ناپ_t{0});  // And is the entry asked for
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(سیٹ.شامل_اے(ناپ_t{7}, ناپ_t{0}), خطأ);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        لكل (ناپ_t i = 1; i < N; ++i) {
            {
                ثابت سیٹ_2::iterator it = سیٹ.لبھو(ناپ_t{7}, i);
                UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                سیٹ.erase(it);
            }
            UASSERT_SELFTEST(سیٹ.ناپ(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            لكل (ناپ_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(سیٹ.شامل_اے(ناپ_t{7}, j), سچ);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.سرا(), سچ);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        لكل (ناپ_t i = 0; i < N; ++i) سیٹ.داخل_کرو(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        سیٹ.clear();
        UASSERT_SELFTEST(سیٹ.ناپ(), 0);  // Nothing left
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Says so
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.سرا(), سچ);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(سیٹ.شامل_اے(ناپ_t{7}, ناپ_t{0}), خطأ);  // Nor finds any
        سیٹ.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        لكل (ناپ_t i = 0; i < N; ++i) سیٹ.داخل_کرو(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        سیٹ_2 سیٹ;
        سیٹ.clear();
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // Still empty
        سیٹ.داخل_کرو(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(سیٹ.ناپ(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

فراغ testMove() {
    ناپ_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    بنية Counted final {
        ناپ_t* m_alivep;  // Where the live entries are counted
        ناپ_t m_id;  // Entries with equal ids are equal

        Counted(ناپ_t* alivep, ناپ_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ثابت Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& عامل=(ثابت Counted&) = مٹاؤ;
        Counted& عامل=(Counted&&) = مٹاؤ;
    };

    بنية Hash final {
        ناپ_t عامل()(ثابت Counted& اندراج) ثابت { أرجع عامل()(اندراج.m_id); }
        ناپ_t عامل()(ناپ_t id) ثابت { أرجع id; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت Counted& a, ثابت Counted& b) ثابت { أرجع عامل()(a, b.m_id); }
        منطقي عامل()(ثابت Counted& a, ناپ_t id) ثابت { أرجع a.m_id == id; }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<Counted, Hash, Equal>;

    constexpr ناپ_t N = GROWS_TWICE;
    {
        سیٹ_2 سیٹ;
        لكل (ناپ_t i = 0; i < N; ++i) سیٹ.داخل_کرو(Counted{&alive, i});
        UASSERT_SELFTEST(سیٹ.ناپ(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        سیٹ_2 moved{std::move(سیٹ)};
        UASSERT_SELFTEST(moved.ناپ(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // And the source no longer holds them
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.سرا(), سچ);  // So it iterates nothing
        لكل (ناپ_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.شامل_اے(i), سچ);  // Every entry came across
            UASSERT_SELFTEST(سیٹ.شامل_اے(i), خطأ);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(سیٹ.داخل_کرو(Counted{&alive, N}).دوجا, سچ);  // It took an entry
        UASSERT_SELFTEST(سیٹ.ناپ(), 1);  // Which is all it holds
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), خطأ);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        سیٹ = std::move(moved);
        UASSERT_SELFTEST(سیٹ.ناپ(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(سیٹ.شامل_اے(N), خطأ);  // Namely that one
        UASSERT_SELFTEST(moved.خالی_تھاواں(), سچ);  // And the source is empty again
        لكل (ناپ_t i = 0; i < N; ++i) UASSERT_SELFTEST(سیٹ.شامل_اے(i), سچ);  // The rest moved
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
        ناپ_t m_id;  // Entries with equal ids are equal

    عام:
        واضح MoveOnly(ناپ_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = افتراضي;
        MoveOnly(ثابت MoveOnly&) = مٹاؤ;
        MoveOnly& عامل=(ثابت MoveOnly&) = مٹاؤ;
        MoveOnly& عامل=(MoveOnly&&) = مٹاؤ;
        ~MoveOnly() = افتراضي;

        ناپ_t id() ثابت { أرجع m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::قدر,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::قدر,
                  "SelfTest: 'MoveOnly' must be move constructible");

    بنية Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        ناپ_t عامل()(ثابت MoveOnly& قدر) ثابت { أرجع عامل()(قدر.id()); }
        ناپ_t عامل()(ناپ_t) ثابت { أرجع 7; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت MoveOnly& a, ثابت MoveOnly& b) ثابت {
            أرجع عامل()(a, b.id());
        }
        منطقي عامل()(ثابت MoveOnly& a, ناپ_t id) ثابت { أرجع a.id() == id; }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<MoveOnly, Hash, Equal>;

    constexpr ناپ_t N = GROWS_TWICE;
    سیٹ_2 سیٹ;
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت std::pair<سیٹ_2::iterator, منطقي> added
            = سیٹ.داخل_کرو_Lazy(i, [i] { أرجع MoveOnly{i}; });
        UASSERT_SELFTEST(added.دوجا, سچ);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.پہلا->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(سیٹ.ناپ(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    لكل (ناپ_t i = 0; i < N; i += 2) {
        ثابت سیٹ_2::iterator it = سیٹ.لبھو(i);
        UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Present before the erase
        سیٹ.erase(it);
    }
    UASSERT_SELFTEST(سیٹ.ناپ(), N / 2);  // Half of them are gone
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت منطقي erased = (i % 2) == 0;
        UASSERT_SELFTEST(سیٹ.شامل_اے(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

فراغ testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    صنف Hash final {
        ناپ_t m_نقاب;  // Only these bits of an entry matter

    عام:
        واضح Hash(ناپ_t نقاب)
            : m_نقاب{نقاب} {}
        ناپ_t عامل()(ناپ_t قدر) ثابت { أرجع قدر & m_نقاب; }
    };

    صنف Equal final {
        ناپ_t m_نقاب;  // Only these bits of an entry matter

    عام:
        واضح Equal(ناپ_t نقاب)
            : m_نقاب{نقاب} {}
        منطقي عامل()(ناپ_t a, ناپ_t b) ثابت { أرجع (a & m_نقاب) == (b & m_نقاب); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::قدر,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::قدر,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_سیٹ<ناپ_t, Hash, Equal> سیٹ{Hash{3}, Equal{3}};
    لكل (ناپ_t i = 0; i < 16; ++i) سیٹ.داخل_کرو(i);
    UASSERT_SELFTEST(سیٹ.ناپ(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    لكل (ناپ_t i = 0; i < 16; ++i) {
        ثابت V3_Hash_سیٹ<ناپ_t, Hash, Equal>::iterator it = سیٹ.لبھو(i);
        UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

فراغ test_Backward_کھسکاؤ_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    بنية قدر_2 final {
        ناپ_t m_hash = 0;  // Hash of this entry
        ناپ_t m_id = 0;  // Entries with equal ids are equal
    };

    بنية Hash final {
        ناپ_t عامل()(ثابت قدر_2* valuep) ثابت { أرجع عامل()(valuep->m_hash, 0); }
        ناپ_t عامل()(ناپ_t hash, ناپ_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            أرجع عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عامل()(ثابت قدر_2* valuep, ناپ_t hash, ناپ_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<قدر_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    لكل (ثابت ناپ_t h : {ناپ_t{0}, ناپ_t{5}, ~ناپ_t{0}}) {  // Wraps too
        std::array<قدر_2, 4> اقدار{قدر_2{h, 0}, قدر_2{h + 1, 1},  //
                                    قدر_2{h, 2}, قدر_2{h + 1, 3}};
        سیٹ_2 سیٹ;
        لكل (قدر_2& قدر : اقدار) سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(سیٹ.ناپ(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(h, ناپ_t{0});
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Present before the erase
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), 3);  // One fewer entry
        UASSERT_SELFTEST(سیٹ.شامل_اے(h, ناپ_t{0}), خطأ);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        لكل (ناپ_t i = 1; i < اقدار.ناپ(); ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(اقدار[i].m_hash, i);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &اقدار[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(h + 1, ناپ_t{1});
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Left where the first erase found it
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), 2);  // Two are left
        لكل (ناپ_t i = 2; i < اقدار.ناپ(); ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.لبھو(اقدار[i].m_hash, i);
            UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Both still reachable
            UASSERT_SELFTEST(*it, &اقدار[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<منطقي, 4> تکیا{};
        لكل (ثابت قدر_2* ثابت valuep : سیٹ) {
            UASSERT_SELFTEST(تکیا[valuep->m_id], خطأ);  // Each entry once
            تکیا[valuep->m_id] = سچ;
        }
        UASSERT_SELFTEST(تکیا[0], خطأ);  // Erased first
        UASSERT_SELFTEST(تکیا[1], خطأ);  // Erased second
        UASSERT_SELFTEST(تکیا[2], سچ);  // Moved back over the first hole
        UASSERT_SELFTEST(تکیا[3], سچ);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

فراغ testReferenceStability() {
    بنية قدر_2 final {
        ناپ_t m_hash = 0;  // Hash of this entry
        ناپ_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        ناپ_t عامل()(ثابت قدر_2* valuep) ثابت { أرجع عامل()(valuep->m_hash, 0); }
        ناپ_t عامل()(ناپ_t hash, ناپ_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            أرجع عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عامل()(ثابت قدر_2* valuep, ناپ_t hash, ناپ_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<قدر_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        سیٹ_2 سیٹ;
        سیٹ.reserve(8);
        UASSERT_SELFTEST(سیٹ.شامل_اے(ناپ_t{7}, ناپ_t{0}), خطأ);  // Room, but no entry
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.سرا(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سیٹ.خالی_تھاواں(), سچ);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr ناپ_t N = max_لدو(2 * MIN_سمرتھا);
    static_assert(N > max_لدو(MIN_سمرتھا), "SelfTest: 'N' must need more than a new table");
    std::array<قدر_2, N> اقدار{};
    std::array<قدر_2* ثابت*, N> entrypps{};  // Where each entry is stored, as inserted
    سیٹ_2 سیٹ;
    سیٹ.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    لكل (ناپ_t i = 0; i < N; ++i) {
        اقدار[i] = قدر_2{7, i};
        ثابت std::pair<سیٹ_2::iterator, منطقي> pair = سیٹ.داخل_کرو(&اقدار[i]);
        UASSERT_SELFTEST(pair.دوجا, سچ);  // Ids differ, so each is added
        entrypps[i] = &*pair.پہلا;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت سیٹ_2::iterator it = سیٹ.لبھو(ناپ_t{7}, i);
        UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت std::pair<سیٹ_2::iterator, منطقي> pair = سیٹ.داخل_کرو(&اقدار[i]);
        UASSERT_SELFTEST(pair.دوجا, خطأ);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.پہلا, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(سیٹ.ناپ(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    سیٹ.reserve(8 * N);
    UASSERT_SELFTEST(سیٹ.ناپ(), N);  // Rehashing dropped nothing
    std::array<قدر_2* ثابت*, N> grownpps{};  // Where each entry is after the growth
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت سیٹ_2::iterator it = سیٹ.لبھو(ناپ_t{7}, i);
        UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    سیٹ.reserve(N);
    UASSERT_SELFTEST(سیٹ.ناپ(), N);  // The smaller request changed nothing
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت سیٹ_2::iterator it = سیٹ.لبھو(ناپ_t{7}, i);
        UASSERT_SELFTEST(it != سیٹ.سرا(), سچ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
بنية ClusteredHash final {
    ناپ_t عامل()(ناپ_t قدر) ثابت { أرجع قدر & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
بنية پھیلاؤ_Hash final {
    ناپ_t عامل()(ناپ_t قدر) ثابت {
        أرجع static_cast<ناپ_t>(قدر * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
سانچا <قسم_ناں T_Hash>
فراغ test_سیٹ_Against_Model() {
    بنية Equal final {
        منطقي عامل()(ناپ_t a, ناپ_t b) ثابت { أرجع a == b; }
    };

    ورتوں سیٹ_2 = V3_Hash_سیٹ<ناپ_t, T_Hash, Equal>;

    constexpr ناپ_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr ناپ_t قدماں = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ثابت خودکار اگلا_Rand = [&state]() -> ناپ_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        أرجع static_cast<ناپ_t>(state >> 32);
    };

    سیٹ_2 سیٹ;
    std::سیٹ<ناپ_t> model;

    لكل (ناپ_t قدم = 0; قدم < قدماں; ++قدم) {
        ثابت ناپ_t id = اگلا_Rand() % UNIVERSE;
        بدل (اگلا_Rand() % 4) {
        حالة 0: {  // Insert a copy
            ثابت std::pair<قسم_ناں سیٹ_2::iterator, منطقي> pair = سیٹ.داخل_کرو(id);
            UASSERT_SELFTEST(pair.دوجا, model.داخل_کرو(id).دوجا);  // As the model
            UASSERT_SELFTEST(*pair.پہلا, id);  // At the entry asked for
            اكسر;
        }
        حالة 1: {  // Insert lazily, which must create only on a miss
            منطقي created = خطأ;
            ثابت std::pair<قسم_ناں سیٹ_2::iterator, منطقي> pair
                = سیٹ.داخل_کرو_Lazy(id, [&]() -> ناپ_t {
                      created = سچ;
                      أرجع id;
                  });
            UASSERT_SELFTEST(pair.دوجا, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.دوجا, model.داخل_کرو(id).دوجا);  // As the model
            UASSERT_SELFTEST(*pair.پہلا, id);  // At the entry asked for
            اكسر;
        }
        حالة 2: {  // Erase, if present
            ثابت قسم_ناں سیٹ_2::iterator it = سیٹ.لبھو(id);
            UASSERT_SELFTEST(it != سیٹ.سرا(), model.گنتی(id) != 0);  // As the model
            إذا (it != سیٹ.سرا()) {
                سیٹ.erase(it);
                model.erase(id);
            }
            اكسر;
        }
        افتراضي: {  // Look up only
            ثابت قسم_ناں سیٹ_2::iterator it = سیٹ.لبھو(id);
            UASSERT_SELFTEST(it != سیٹ.سرا(), model.گنتی(id) != 0);  // As the model
            إذا (it != سیٹ.سرا()) UASSERT_SELFTEST(*it, id);  // And is the entry
            اكسر;
        }
        }
        UASSERT_SELFTEST(سیٹ.ناپ(), model.ناپ());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    لكل (ناپ_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(سیٹ.شامل_اے(id), model.گنتی(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقي, UNIVERSE> تکیا{};
    ناپ_t n = 0;
    لكل (ثابت ناپ_t اندراج : سیٹ) {
        UASSERT_SELFTEST(اندراج < UNIVERSE, سچ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.گنتی(اندراج) != 0, سچ);  // Only what was inserted
        UASSERT_SELFTEST(تکیا[اندراج], خطأ);  // Each entry once
        تکیا[اندراج] = سچ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.ناپ());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

فراغ testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    ورتوں Map = V3HashMap<std::string, صحيح>;
    ورتوں اندراج_2 = Map::اندراج_2;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<اندراج_2, std::pair<std::string, صحيح>>::قدر,
                  "SelfTest: a map entry must be a plain pair");

    constexpr ناپ_t N = GROWS_TWICE;
    Map map;
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::منزل_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ثابت std::pair<Map::iterator, منطقي> pair
            = map.داخل_کرو_Lazy(کلید, [&]() -> اندراج_2 { أرجع {کلید, static_cast<صحيح>(i)}; });
        UASSERT_SELFTEST(pair.دوجا, سچ);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.ناپ(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.لبھو("key" + std::منزل_string(i));
        UASSERT_SELFTEST(it != map.سرا(), سچ);  // The key alone finds it
        UASSERT_SELFTEST(it->دوجا, static_cast<صحيح>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::منزل_string(i);
        map.erase(map.لبھو(کلید));
        UASSERT_SELFTEST(map.داخل_کرو({کلید, static_cast<صحيح>(i) + 100}).دوجا, سچ);  // Anew
    }
    UASSERT_SELFTEST(map.ناپ(), N);  // The same keys, so the map has not grown
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.لبھو("key" + std::منزل_string(i));
        UASSERT_SELFTEST(it != map.سرا(), سچ);  // Each key is still there
        UASSERT_SELFTEST(it->دوجا, static_cast<صحيح>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    لكل (ناپ_t i = 0; i < N; ++i) map.لبھو("key" + std::منزل_string(i)).قدر() += 100;
    UASSERT_SELFTEST(map.ناپ(), N);  // The map is unchanged otherwise
    لكل (ناپ_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.لبھو("key" + std::منزل_string(i));
        UASSERT_SELFTEST(it != map.سرا(), سچ);  // Each key still finds its entry
        UASSERT_SELFTEST(it->دوجا, static_cast<صحيح>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    لكل (ناپ_t i = 0; i < N; ++i) map.لبھو("key" + std::منزل_string(i)).قدر() -= 100;

    // Adding a key that is present changes nothing
    {
        ثابت std::string کلید = "key0";
        منطقي created = خطأ;
        ثابت std::pair<Map::iterator, منطقي> pair = map.داخل_کرو_Lazy(کلید, [&]() -> اندراج_2 {
            created = سچ;  // LCOV_EXCL_START
            أرجع {کلید, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, خطأ);  // The factory was never called
        UASSERT_SELFTEST(pair.دوجا, خطأ);  // As the key is present
        UASSERT_SELFTEST(pair.پہلا->دوجا, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.ناپ(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ثابت std::pair<Map::iterator, منطقي> added = map.داخل_کرو({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.دوجا, سچ);  // Added, as the key is new
        UASSERT_SELFTEST(added.پہلا->دوجا, 5);  // With the value given
        ثابت std::pair<Map::iterator, منطقي> again = map.داخل_کرو({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.دوجا, خطأ);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.پہلا->دوجا, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.ناپ(), N + 1);  // Just the one entry was added
    {
        ثابت Map::iterator it = map.لبھو(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.سرا(), سچ);  // Present before the erase
        UASSERT_SELFTEST(it->دوجا, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.ناپ(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), خطأ);  // No such key
    UASSERT_SELFTEST(map.ناپ(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), سچ);  // That key is present
    UASSERT_SELFTEST(map.ناپ(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.شامل_اے(std::string{"key0"}), خطأ);  // Namely that one
    لكل (ناپ_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.شامل_اے("key" + std::منزل_string(i)), سچ);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
سانچا <قسم_ناں T_Hash>
فراغ testMapAgainstModel() {
    بنية Equal final {
        منطقي عامل()(ناپ_t a, ناپ_t b) ثابت { أرجع a == b; }
    };

    ورتوں Map = V3HashMap<ناپ_t, ناپ_t, T_Hash, Equal>;
    ورتوں اندراج_2 = قسم_ناں Map::اندراج_2;

    constexpr ناپ_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr ناپ_t قدماں = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ثابت خودکار اگلا_Rand = [&state]() -> ناپ_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        أرجع static_cast<ناپ_t>(state >> 32);
    };

    Map map;
    std::map<ناپ_t, ناپ_t> model;

    لكل (ناپ_t قدم = 0; قدم < قدماں; ++قدم) {
        ثابت ناپ_t کلید = اگلا_Rand() % UNIVERSE;
        ثابت ناپ_t val = قدم;  // Distinct every step, so a stale value is visible
        بدل (اگلا_Rand() % 5) {
        حالة 0: {  // Insert a whole entry
            ثابت std::pair<قسم_ناں Map::iterator, منطقي> pair = map.داخل_کرو({کلید, val});
            UASSERT_SELFTEST(pair.دوجا, model.داخل_کرو({کلید, val}).دوجا);  // Ditto
            UASSERT_SELFTEST(pair.پہلا->پہلا, کلید);  // At the key asked for
            اكسر;
        }
        حالة 1: {  // Insert lazily, which must create only on a miss
            منطقي created = خطأ;
            ثابت std::pair<قسم_ناں Map::iterator, منطقي> pair
                = map.داخل_کرو_Lazy(کلید, [&]() -> اندراج_2 {
                      created = سچ;
                      أرجع {کلید, val};
                  });
            UASSERT_SELFTEST(pair.دوجا, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.دوجا, model.داخل_کرو({کلید, val}).دوجا);  // Ditto
            UASSERT_SELFTEST(pair.پہلا->پہلا, کلید);  // At the key asked for
            اكسر;
        }
        حالة 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(کلید), model.erase(کلید) != 0);  // As model
            اكسر;
        }
        حالة 3: {  // Change a value, which is erasing the entry and inserting it again
            ثابت قسم_ناں Map::iterator it = map.لبھو(کلید);
            إذا (it != map.سرا()) {
                map.erase(it);
                UASSERT_SELFTEST(map.داخل_کرو({کلید, val}).دوجا, سچ);  // The key is free
                model[کلید] = val;
            }
            اكسر;
        }
        افتراضي: {  // Look up only
            ثابت قسم_ناں Map::iterator it = map.لبھو(کلید);
            UASSERT_SELFTEST(it != map.سرا(), model.گنتی(کلید) != 0);  // As the model
            إذا (it != map.سرا()) {
                UASSERT_SELFTEST(it->پہلا, کلید);  // At the key asked for
                UASSERT_SELFTEST(it->دوجا, model.at(کلید));  // With its value
            }
            اكسر;
        }
        }
        UASSERT_SELFTEST(map.ناپ(), model.ناپ());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    لكل (ناپ_t کلید = 0; کلید < UNIVERSE; ++کلید) {
        ثابت قسم_ناں Map::iterator it = map.لبھو(کلید);
        UASSERT_SELFTEST(it != map.سرا(), model.گنتی(کلید) != 0);  // Present iff modelled
        إذا (it != map.سرا()) {
            UASSERT_SELFTEST(it->دوجا, model.at(کلید));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقي, UNIVERSE> تکیا{};
    ناپ_t n = 0;
    لكل (ثابت اندراج_2& اندراج : map) {
        UASSERT_SELFTEST(اندراج.پہلا < UNIVERSE, سچ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.گنتی(اندراج.پہلا) != 0, سچ);  // Only what was inserted
        UASSERT_SELFTEST(اندراج.دوجا, model.at(اندراج.پہلا));  // With the right value
        UASSERT_SELFTEST(تکیا[اندراج.پہلا], خطأ);  // Each entry once
        تکیا[اندراج.پہلا] = سچ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.ناپ());  // And all of them
}

فراغ selfTest() {
    test_قدر_کلیدیں();
    test_اشارہ_گر_کلیدیں();
    test_اندراج_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_کھسکاؤ_Deletion();
    testReferenceStability();
    test_سیٹ_Against_Model<ClusteredHash>();
    test_سیٹ_Against_Model<پھیلاؤ_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<پھیلاؤ_Hash>();
}

}  // namespace V3HashTableInternals
