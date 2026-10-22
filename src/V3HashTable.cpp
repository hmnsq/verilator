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

#شامل "V3HashTable.h"

#شامل "V3Error.h"

#شامل <array>
#شامل <functional>
#شامل <map>
#شامل <set>
#شامل <string>
#شامل <type_traits>
#شامل <utility>

نامخلا V3_Hash_جدول_Internals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr سائز_t max_بھرو(سائز_t استعداد) { واپس استعداد * بھرو_ضربی_عدد_عدد / بھرو_ضربی_عدد_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr سائز_t GROWS_TWICE = max_بھرو(4 * MIN_استعداد);
static_assert(GROWS_TWICE > max_بھرو(2 * MIN_استعداد),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

خالی test_قدر_کلیدیں() {
    ساخت قدر_2 final {
        سائز_t m_hash = 0;  // Hash of this entry
        سائز_t m_id = 0;  // Id of this entry
    };

    ساخت Hash final {
        سائز_t عامل()(ثابت قدر_2& قدر) ثابت { واپس عامل()(قدر.m_hash, 0); }
        سائز_t عامل()(سائز_t hash, سائز_t) ثابت { واپس hash; }
    };

    ساخت Equal final {
        منطقی عامل()(ثابت قدر_2& a, ثابت قدر_2& b) ثابت {
            واپس عامل()(a, b.m_hash, b.m_id);
        }
        منطقی عامل()(ثابت قدر_2& قدر, سائز_t hash, سائز_t id) ثابت {
            واپس قدر.m_hash == hash && قدر.m_id == id;
        }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<قدر_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ثابت قدر_2 قدر{1, 0};
        ثابت قدر_2 equal{1, 0};  // Equal to 'value', and hashes the same
        سیٹ_2 سیٹ;
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Starts empty
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.انتہا(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(قدر) == سیٹ.انتہا(), سچ);  // And finds nothing
        ثابت std::pair<سیٹ_2::iterator, منطقی> added = سیٹ.داخل_کرو(قدر);
        UASSERT_SELFTEST(added.دوسرا, سچ);  // Added, as it was absent
        UASSERT_SELFTEST(added.پہلا->m_id, قدر.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.پہلا != &قدر, سچ);  // Which is a copy of the argument
        UASSERT_SELFTEST(سیٹ.سائز(), 1);  // And is the only one
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), جھوٹ);  // So the set is no longer empty
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.پہلا);  // At the entry held
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(equal);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.پہلا);  // At that same entry
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(1, سائز_t{0});
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.پہلا);  // At that same entry again
        }
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(1, سائز_t{1}) == سیٹ.انتہا(), سچ);  // Hash same, id not
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(2, سائز_t{0}) == سیٹ.انتہا(), سچ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<سیٹ_2::iterator, منطقی> again = سیٹ.داخل_کرو(equal);
        UASSERT_SELFTEST(again.دوسرا, جھوٹ);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.پہلا, &*added.پہلا);  // At the entry already held
        UASSERT_SELFTEST(سیٹ.سائز(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Present before the erase
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Empty again
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(قدر) == سیٹ.انتہا(), سچ);  // So the entry is gone
        UASSERT_SELFTEST(سیٹ.شامل_ہے(قدر), جھوٹ);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(سیٹ.erase(قدر), جھوٹ);  // Nothing to erase, so says so
        سیٹ.داخل_کرو(قدر);
        UASSERT_SELFTEST(سیٹ.erase(equal), سچ);  // An equal key erases the entry held
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        سیٹ.داخل_کرو(قدر);
        UASSERT_SELFTEST(سیٹ.erase(1, سائز_t{0}), سچ);  // Those parts find and erase it
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr سائز_t N = GROWS_TWICE;
        سیٹ_2 سیٹ;
        برائے (سائز_t i = 0; i < N; ++i) {
            ثابت std::pair<سیٹ_2::iterator, منطقی> added = سیٹ.داخل_کرو(قدر_2{7, i});
            UASSERT_SELFTEST(added.دوسرا, سچ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(سیٹ.سائز(), N);  // All of them, in the one probe run
        برائے (سائز_t i = 0; i < N; ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(سائز_t{7}, i);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<منطقی, N> نظر_آیا{};
        سائز_t n = 0;
        برائے (ثابت قدر_2& اندراج_2 : سیٹ) {
            UASSERT_SELFTEST(اندراج_2.m_hash, سائز_t{7});  // Only entries added
            UASSERT_SELFTEST(نظر_آیا[اندراج_2.m_id], جھوٹ);  // Each of them exactly once
            نظر_آیا[اندراج_2.m_id] = سچ;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

خالی test_اشارہ_گر_کلیدیں() {
    ساخت قدر_2 final {
        سائز_t m_hash = 0;  // Hash of this entry
        سائز_t m_id = 0;  // Id of this entry
    };

    ساخت Hash final {
        سائز_t عامل()(ثابت قدر_2* valuep) ثابت { واپس عامل()(valuep->m_hash, 0); }
        سائز_t عامل()(سائز_t hash, سائز_t) ثابت { واپس hash; }
    };

    ساخت Equal final {
        منطقی عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            واپس عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقی عامل()(ثابت قدر_2* valuep, سائز_t hash, سائز_t id) ثابت {
            واپس valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<قدر_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        قدر_2 قدر{1, 0};
        قدر_2 equal{1, 0};  // Equal to 'value', and hashes the same
        سیٹ_2 سیٹ;
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Starts empty
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.انتہا(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(&قدر) == سیٹ.انتہا(), سچ);  // And finds nothing
        ثابت std::pair<سیٹ_2::iterator, منطقی> added = سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(added.دوسرا, سچ);  // Added, as it was absent
        UASSERT_SELFTEST(*added.پہلا, &قدر);  // The iterator is at the new entry
        UASSERT_SELFTEST(سیٹ.سائز(), 1);  // Which is the only one
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), جھوٹ);  // So the set is no longer empty
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Found by itself
            UASSERT_SELFTEST(*it, &قدر);  // At the entry held
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&equal);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // And by an equal entry
            UASSERT_SELFTEST(*it, &قدر);  // At that same entry
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(1, سائز_t{0});
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // And by its parts
            UASSERT_SELFTEST(*it, &قدر);  // At that same entry again
        }
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(1, سائز_t{1}) == سیٹ.انتہا(), سچ);  // Hash same, id not
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(2, سائز_t{0}) == سیٹ.انتہا(), سچ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<سیٹ_2::iterator, منطقی> again = سیٹ.داخل_کرو(&equal);
        UASSERT_SELFTEST(again.دوسرا, جھوٹ);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.پہلا, &قدر);  // The iterator is at the stored one
        UASSERT_SELFTEST(سیٹ.سائز(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&equal);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &قدر);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, جھوٹ);  // Not the one asked for
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // The entry is still there
            UASSERT_SELFTEST(*it, &قدر);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Present before the erase
            UASSERT_SELFTEST(*it, &قدر);  // And is the object asked for
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Empty again
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(&قدر) == سیٹ.انتہا(), سچ);  // So the entry is gone
        UASSERT_SELFTEST(سیٹ.شامل_ہے(&قدر), جھوٹ);  // As 'contains' agrees
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(سیٹ.erase(&قدر), جھوٹ);  // Nothing to erase, so says so
        سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(سیٹ.erase(&equal), سچ);  // The equal key erases the stored one
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Which leaves the set empty
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(&قدر) == سیٹ.انتہا(), سچ);  // And unreachable
        // And by a key spelled as the parts of an entry
        سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(سیٹ.erase(1, سائز_t{0}), سچ);  // Those parts find and erase it
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    برائے (سائز_t erase = 0; erase < 4; ++erase) {
        برائے (ثابت سائز_t hash : {سائز_t{0}, سائز_t{7}, ~سائز_t{0}}) {  // Wraps too
            std::array<قدر_2, 4> اقدار{قدر_2{hash, 0}, قدر_2{hash, 1},  //
                                        قدر_2{hash, 2}, قدر_2{hash, 3}};
            سیٹ_2 سیٹ;
            برائے (قدر_2& قدر : اقدار) سیٹ.داخل_کرو(&قدر);
            UASSERT_SELFTEST(سیٹ.سائز(), 4);  // All distinct, so all added
            {
                ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&اقدار[erase]);
                UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // The one to erase is present
                UASSERT_SELFTEST(*it, &اقدار[erase]);  // And is the object held
                سیٹ.erase(it);
            }
            UASSERT_SELFTEST(سیٹ.سائز(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(سیٹ.ڈھونڈو(&اقدار[erase]) == سیٹ.انتہا(), سچ);  // That one
            برائے (سائز_t i = 0; i < اقدار.سائز(); ++i) {
                اگر (i == erase) جاری;
                ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(hash, i);
                UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // The others are all there
                UASSERT_SELFTEST(*it, &اقدار[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    برائے (ثابت سائز_t hash : {سائز_t{0}, سائز_t{7}, ~سائز_t{0}}) {
        std::array<قدر_2, GROWS_TWICE> many{};
        سیٹ_2 سیٹ;
        برائے (سائز_t i = 0; i < many.سائز(); ++i) {
            many[i] = قدر_2{hash, i};
            سیٹ.داخل_کرو(&many[i]);
        }
        UASSERT_SELFTEST(سیٹ.سائز(), many.سائز());  // All of them were added
        برائے (سائز_t i = 0; i < many.سائز(); ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(hash, i);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // And all survived the growth
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
        UASSERT_SELFTEST(سیٹ.سائز(), 2);  // Both kept, despite the collision
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // The first of the run is there
            UASSERT_SELFTEST(*it, &قدر);  // And is that entry
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&other);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Present before the erase
            UASSERT_SELFTEST(*it, &قدر);  // And is the object held
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.سائز(), 1);  // Only one was erased
        UASSERT_SELFTEST(سیٹ.ڈھونڈو(&قدر) == سیٹ.انتہا(), سچ);  // Namely that one
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&other);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    برائے (ثابت منطقی doReserve : {جھوٹ, سچ}) {
        std::array<قدر_2, 8> اقدار{};
        سیٹ_2 سیٹ;
        اگر (doReserve) سیٹ.reserve(اقدار.سائز());
        برائے (سائز_t i = 0; i < اقدار.سائز(); ++i) {
            اقدار[i] = قدر_2{static_cast<سائز_t>(i * 1234567), i};
            سیٹ.داخل_کرو(&اقدار[i]);
        }
        برائے (قدر_2& قدر : اقدار) {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(&قدر);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Each entry survives
            UASSERT_SELFTEST(*it, &قدر);  // And is the one inserted
        }
        UASSERT_SELFTEST(سیٹ.سائز(), اقدار.سائز());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr سائز_t N = GROWS_TWICE;
        std::array<قدر_2, N> اقدار{};
        سیٹ_2 سیٹ;
        برائے (سائز_t i = 0; i < N; ++i) {
            اقدار[i] = قدر_2{7, i};  // All of them hash the same
            ثابت std::pair<سیٹ_2::iterator, منطقی> pair
                = سیٹ.داخل_کرو_Lazy(سائز_t{7}, i, [&]() -> قدر_2* { واپس &اقدار[i]; });
            UASSERT_SELFTEST(pair.دوسرا, سچ);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.پہلا, &اقدار[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(سیٹ.سائز(), N);  // All of them were added
        // They are all present now, so nothing is created
        برائے (سائز_t i = 0; i < N; ++i) {
            منطقی created = جھوٹ;
            ثابت std::pair<سیٹ_2::iterator, منطقی> pair
                = سیٹ.داخل_کرو_Lazy(سائز_t{7}, i, [&]() -> قدر_2* {
                      created = سچ;  // LCOV_EXCL_START
                      واپس &اقدار[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, جھوٹ);  // The factory was never called
            UASSERT_SELFTEST(pair.دوسرا, جھوٹ);  // As the lookup hit
            UASSERT_SELFTEST(*pair.پہلا, &اقدار[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(سیٹ.سائز(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<قدر_2, 20> اقدار{};
        std::array<منطقی, 20> نظر_آیا{};
        سیٹ_2 سیٹ;
        برائے (سائز_t i = 0; i < اقدار.سائز(); ++i) {
            اقدار[i] = قدر_2{i / 2, i};  // Pairs of them collide
            سیٹ.داخل_کرو(&اقدار[i]);
        }
        سائز_t n = 0;
        برائے (قدر_2* ثابت valuep : سیٹ) {
            UASSERT_SELFTEST(valuep->m_id < نظر_آیا.سائز(), سچ);  // Only entries added
            UASSERT_SELFTEST(نظر_آیا[valuep->m_id], جھوٹ);  // Each of them exactly once
            نظر_آیا[valuep->m_id] = سچ;
            ++n;
        }
        UASSERT_SELFTEST(n, اقدار.سائز());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

خالی test_اندراج_Lifetime() {
    سائز_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    جماعت NoDefault final {
        سائز_t* m_alivep;  // Where the live entries are counted
        سائز_t m_hash;  // Hash of this entry
        سائز_t m_id;  // Entries with equal ids are equal

    عوامی:
        NoDefault(سائز_t* alivep, سائز_t hash, سائز_t id)
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
        NoDefault& عامل=(ثابت NoDefault&) = حذف;
        NoDefault& عامل=(NoDefault&&) = حذف;

        سائز_t hash() ثابت { واپس m_hash; }
        سائز_t id() ثابت { واپس m_id; }
        منطقی عامل==(ثابت NoDefault& that) ثابت { واپس m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::قدر,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    ساخت Hash final {
        سائز_t عامل()(ثابت NoDefault& قدر) ثابت { واپس قدر.hash(); }
        سائز_t عامل()(سائز_t hash, سائز_t) ثابت { واپس hash; }
    };

    ساخت Equal final {
        منطقی عامل()(ثابت NoDefault& a, ثابت NoDefault& b) ثابت {
            واپس عامل()(a, b.hash(), b.id());
        }
        منطقی عامل()(ثابت NoDefault& a, سائز_t, سائز_t id) ثابت { واپس a.id() == id; }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<NoDefault, Hash, Equal>;

    constexpr سائز_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        سیٹ_2 سیٹ;
        برائے (سائز_t i = 0; i < N; ++i) {
            ثابت std::pair<سیٹ_2::iterator, منطقی> added = سیٹ.داخل_کرو(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.دوسرا, سچ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(سیٹ.سائز(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        برائے (سائز_t i = 0; i < N; ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(سائز_t{7}, i);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ثابت std::pair<سیٹ_2::iterator, منطقی> dup = سیٹ.داخل_کرو(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.دوسرا, جھوٹ);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.پہلا->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(سیٹ.سائز(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(سائز_t{7}, سائز_t{0});
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Present before the erase
            UASSERT_SELFTEST(it->id(), سائز_t{0});  // And is the entry asked for
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.سائز(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(سیٹ.شامل_ہے(سائز_t{7}, سائز_t{0}), جھوٹ);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        برائے (سائز_t i = 1; i < N; ++i) {
            {
                ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(سائز_t{7}, i);
                UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                سیٹ.erase(it);
            }
            UASSERT_SELFTEST(سیٹ.سائز(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            برائے (سائز_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(سیٹ.شامل_ہے(سائز_t{7}, j), سچ);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.انتہا(), سچ);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        برائے (سائز_t i = 0; i < N; ++i) سیٹ.داخل_کرو(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        سیٹ.clear();
        UASSERT_SELFTEST(سیٹ.سائز(), 0);  // Nothing left
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Says so
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.انتہا(), سچ);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(سیٹ.شامل_ہے(سائز_t{7}, سائز_t{0}), جھوٹ);  // Nor finds any
        سیٹ.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        برائے (سائز_t i = 0; i < N; ++i) سیٹ.داخل_کرو(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        سیٹ_2 سیٹ;
        سیٹ.clear();
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // Still empty
        سیٹ.داخل_کرو(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(سیٹ.سائز(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

خالی testMove() {
    سائز_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    ساخت Counted final {
        سائز_t* m_alivep;  // Where the live entries are counted
        سائز_t m_id;  // Entries with equal ids are equal

        Counted(سائز_t* alivep, سائز_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ثابت Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& عامل=(ثابت Counted&) = حذف;
        Counted& عامل=(Counted&&) = حذف;
    };

    ساخت Hash final {
        سائز_t عامل()(ثابت Counted& اندراج_2) ثابت { واپس عامل()(اندراج_2.m_id); }
        سائز_t عامل()(سائز_t id) ثابت { واپس id; }
    };

    ساخت Equal final {
        منطقی عامل()(ثابت Counted& a, ثابت Counted& b) ثابت { واپس عامل()(a, b.m_id); }
        منطقی عامل()(ثابت Counted& a, سائز_t id) ثابت { واپس a.m_id == id; }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<Counted, Hash, Equal>;

    constexpr سائز_t N = GROWS_TWICE;
    {
        سیٹ_2 سیٹ;
        برائے (سائز_t i = 0; i < N; ++i) سیٹ.داخل_کرو(Counted{&alive, i});
        UASSERT_SELFTEST(سیٹ.سائز(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        سیٹ_2 moved{std::move(سیٹ)};
        UASSERT_SELFTEST(moved.سائز(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // And the source no longer holds them
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.انتہا(), سچ);  // So it iterates nothing
        برائے (سائز_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.شامل_ہے(i), سچ);  // Every entry came across
            UASSERT_SELFTEST(سیٹ.شامل_ہے(i), جھوٹ);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(سیٹ.داخل_کرو(Counted{&alive, N}).دوسرا, سچ);  // It took an entry
        UASSERT_SELFTEST(سیٹ.سائز(), 1);  // Which is all it holds
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), جھوٹ);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        سیٹ = std::move(moved);
        UASSERT_SELFTEST(سیٹ.سائز(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(سیٹ.شامل_ہے(N), جھوٹ);  // Namely that one
        UASSERT_SELFTEST(moved.خالی_خانے(), سچ);  // And the source is empty again
        برائے (سائز_t i = 0; i < N; ++i) UASSERT_SELFTEST(سیٹ.شامل_ہے(i), سچ);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

خالی testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    جماعت MoveOnly final {
        سائز_t m_id;  // Entries with equal ids are equal

    عوامی:
        صریح MoveOnly(سائز_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = طےشدہ;
        MoveOnly(ثابت MoveOnly&) = حذف;
        MoveOnly& عامل=(ثابت MoveOnly&) = حذف;
        MoveOnly& عامل=(MoveOnly&&) = حذف;
        ~MoveOnly() = طےشدہ;

        سائز_t id() ثابت { واپس m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::قدر,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::قدر,
                  "SelfTest: 'MoveOnly' must be move constructible");

    ساخت Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        سائز_t عامل()(ثابت MoveOnly& قدر) ثابت { واپس عامل()(قدر.id()); }
        سائز_t عامل()(سائز_t) ثابت { واپس 7; }
    };

    ساخت Equal final {
        منطقی عامل()(ثابت MoveOnly& a, ثابت MoveOnly& b) ثابت {
            واپس عامل()(a, b.id());
        }
        منطقی عامل()(ثابت MoveOnly& a, سائز_t id) ثابت { واپس a.id() == id; }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<MoveOnly, Hash, Equal>;

    constexpr سائز_t N = GROWS_TWICE;
    سیٹ_2 سیٹ;
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت std::pair<سیٹ_2::iterator, منطقی> added
            = سیٹ.داخل_کرو_Lazy(i, [i] { واپس MoveOnly{i}; });
        UASSERT_SELFTEST(added.دوسرا, سچ);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.پہلا->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(سیٹ.سائز(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    برائے (سائز_t i = 0; i < N; i += 2) {
        ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(i);
        UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Present before the erase
        سیٹ.erase(it);
    }
    UASSERT_SELFTEST(سیٹ.سائز(), N / 2);  // Half of them are gone
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت منطقی erased = (i % 2) == 0;
        UASSERT_SELFTEST(سیٹ.شامل_ہے(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

خالی testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    جماعت Hash final {
        سائز_t m_نقاب;  // Only these bits of an entry matter

    عوامی:
        صریح Hash(سائز_t نقاب)
            : m_نقاب{نقاب} {}
        سائز_t عامل()(سائز_t قدر) ثابت { واپس قدر & m_نقاب; }
    };

    جماعت Equal final {
        سائز_t m_نقاب;  // Only these bits of an entry matter

    عوامی:
        صریح Equal(سائز_t نقاب)
            : m_نقاب{نقاب} {}
        منطقی عامل()(سائز_t a, سائز_t b) ثابت { واپس (a & m_نقاب) == (b & m_نقاب); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::قدر,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::قدر,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_سیٹ<سائز_t, Hash, Equal> سیٹ{Hash{3}, Equal{3}};
    برائے (سائز_t i = 0; i < 16; ++i) سیٹ.داخل_کرو(i);
    UASSERT_SELFTEST(سیٹ.سائز(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    برائے (سائز_t i = 0; i < 16; ++i) {
        ثابت V3_Hash_سیٹ<سائز_t, Hash, Equal>::iterator it = سیٹ.ڈھونڈو(i);
        UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

خالی test_Backward_سرکاؤ_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    ساخت قدر_2 final {
        سائز_t m_hash = 0;  // Hash of this entry
        سائز_t m_id = 0;  // Entries with equal ids are equal
    };

    ساخت Hash final {
        سائز_t عامل()(ثابت قدر_2* valuep) ثابت { واپس عامل()(valuep->m_hash, 0); }
        سائز_t عامل()(سائز_t hash, سائز_t) ثابت { واپس hash; }
    };

    ساخت Equal final {
        منطقی عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            واپس عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقی عامل()(ثابت قدر_2* valuep, سائز_t hash, سائز_t id) ثابت {
            واپس valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<قدر_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    برائے (ثابت سائز_t h : {سائز_t{0}, سائز_t{5}, ~سائز_t{0}}) {  // Wraps too
        std::array<قدر_2, 4> اقدار{قدر_2{h, 0}, قدر_2{h + 1, 1},  //
                                    قدر_2{h, 2}, قدر_2{h + 1, 3}};
        سیٹ_2 سیٹ;
        برائے (قدر_2& قدر : اقدار) سیٹ.داخل_کرو(&قدر);
        UASSERT_SELFTEST(سیٹ.سائز(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(h, سائز_t{0});
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Present before the erase
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.سائز(), 3);  // One fewer entry
        UASSERT_SELFTEST(سیٹ.شامل_ہے(h, سائز_t{0}), جھوٹ);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        برائے (سائز_t i = 1; i < اقدار.سائز(); ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(اقدار[i].m_hash, i);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &اقدار[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(h + 1, سائز_t{1});
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Left where the first erase found it
            سیٹ.erase(it);
        }
        UASSERT_SELFTEST(سیٹ.سائز(), 2);  // Two are left
        برائے (سائز_t i = 2; i < اقدار.سائز(); ++i) {
            ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(اقدار[i].m_hash, i);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Both still reachable
            UASSERT_SELFTEST(*it, &اقدار[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<منطقی, 4> نظر_آیا{};
        برائے (ثابت قدر_2* ثابت valuep : سیٹ) {
            UASSERT_SELFTEST(نظر_آیا[valuep->m_id], جھوٹ);  // Each entry once
            نظر_آیا[valuep->m_id] = سچ;
        }
        UASSERT_SELFTEST(نظر_آیا[0], جھوٹ);  // Erased first
        UASSERT_SELFTEST(نظر_آیا[1], جھوٹ);  // Erased second
        UASSERT_SELFTEST(نظر_آیا[2], سچ);  // Moved back over the first hole
        UASSERT_SELFTEST(نظر_آیا[3], سچ);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

خالی testReferenceStability() {
    ساخت قدر_2 final {
        سائز_t m_hash = 0;  // Hash of this entry
        سائز_t m_id = 0;  // Id of this entry
    };

    ساخت Hash final {
        سائز_t عامل()(ثابت قدر_2* valuep) ثابت { واپس عامل()(valuep->m_hash, 0); }
        سائز_t عامل()(سائز_t hash, سائز_t) ثابت { واپس hash; }
    };

    ساخت Equal final {
        منطقی عامل()(ثابت قدر_2* ap, ثابت قدر_2* bp) ثابت {
            واپس عامل()(ap, bp->m_hash, bp->m_id);
        }
        منطقی عامل()(ثابت قدر_2* valuep, سائز_t hash, سائز_t id) ثابت {
            واپس valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<قدر_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        سیٹ_2 سیٹ;
        سیٹ.reserve(8);
        UASSERT_SELFTEST(سیٹ.شامل_ہے(سائز_t{7}, سائز_t{0}), جھوٹ);  // Room, but no entry
        UASSERT_SELFTEST(سیٹ.begin() == سیٹ.انتہا(), سچ);  // So iterates nothing
        UASSERT_SELFTEST(سیٹ.خالی_خانے(), سچ);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr سائز_t N = max_بھرو(2 * MIN_استعداد);
    static_assert(N > max_بھرو(MIN_استعداد), "SelfTest: 'N' must need more than a new table");
    std::array<قدر_2, N> اقدار{};
    std::array<قدر_2* ثابت*, N> entrypps{};  // Where each entry is stored, as inserted
    سیٹ_2 سیٹ;
    سیٹ.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    برائے (سائز_t i = 0; i < N; ++i) {
        اقدار[i] = قدر_2{7, i};
        ثابت std::pair<سیٹ_2::iterator, منطقی> pair = سیٹ.داخل_کرو(&اقدار[i]);
        UASSERT_SELFTEST(pair.دوسرا, سچ);  // Ids differ, so each is added
        entrypps[i] = &*pair.پہلا;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(سائز_t{7}, i);
        UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت std::pair<سیٹ_2::iterator, منطقی> pair = سیٹ.داخل_کرو(&اقدار[i]);
        UASSERT_SELFTEST(pair.دوسرا, جھوٹ);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.پہلا, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(سیٹ.سائز(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    سیٹ.reserve(8 * N);
    UASSERT_SELFTEST(سیٹ.سائز(), N);  // Rehashing dropped nothing
    std::array<قدر_2* ثابت*, N> grownpps{};  // Where each entry is after the growth
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(سائز_t{7}, i);
        UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    سیٹ.reserve(N);
    UASSERT_SELFTEST(سیٹ.سائز(), N);  // The smaller request changed nothing
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت سیٹ_2::iterator it = سیٹ.ڈھونڈو(سائز_t{7}, i);
        UASSERT_SELFTEST(it != سیٹ.انتہا(), سچ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
ساخت ClusteredHash final {
    سائز_t عامل()(سائز_t قدر) ثابت { واپس قدر & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
ساخت پھیلاؤ_Hash final {
    سائز_t عامل()(سائز_t قدر) ثابت {
        واپس static_cast<سائز_t>(قدر * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
سانچہ <قسم_نام T_Hash>
خالی test_سیٹ_Against_Model() {
    ساخت Equal final {
        منطقی عامل()(سائز_t a, سائز_t b) ثابت { واپس a == b; }
    };

    استعمال سیٹ_2 = V3_Hash_سیٹ<سائز_t, T_Hash, Equal>;

    constexpr سائز_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr سائز_t اقدام = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ثابت خودکار اگلا_Rand = [&state]() -> سائز_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        واپس static_cast<سائز_t>(state >> 32);
    };

    سیٹ_2 سیٹ;
    std::سیٹ<سائز_t> model;

    برائے (سائز_t قدم = 0; قدم < اقدام; ++قدم) {
        ثابت سائز_t id = اگلا_Rand() % UNIVERSE;
        انتخاب (اگلا_Rand() % 4) {
        حالت 0: {  // Insert a copy
            ثابت std::pair<قسم_نام سیٹ_2::iterator, منطقی> pair = سیٹ.داخل_کرو(id);
            UASSERT_SELFTEST(pair.دوسرا, model.داخل_کرو(id).دوسرا);  // As the model
            UASSERT_SELFTEST(*pair.پہلا, id);  // At the entry asked for
            توڑو;
        }
        حالت 1: {  // Insert lazily, which must create only on a miss
            منطقی created = جھوٹ;
            ثابت std::pair<قسم_نام سیٹ_2::iterator, منطقی> pair
                = سیٹ.داخل_کرو_Lazy(id, [&]() -> سائز_t {
                      created = سچ;
                      واپس id;
                  });
            UASSERT_SELFTEST(pair.دوسرا, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.دوسرا, model.داخل_کرو(id).دوسرا);  // As the model
            UASSERT_SELFTEST(*pair.پہلا, id);  // At the entry asked for
            توڑو;
        }
        حالت 2: {  // Erase, if present
            ثابت قسم_نام سیٹ_2::iterator it = سیٹ.ڈھونڈو(id);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), model.گنتی(id) != 0);  // As the model
            اگر (it != سیٹ.انتہا()) {
                سیٹ.erase(it);
                model.erase(id);
            }
            توڑو;
        }
        طےشدہ: {  // Look up only
            ثابت قسم_نام سیٹ_2::iterator it = سیٹ.ڈھونڈو(id);
            UASSERT_SELFTEST(it != سیٹ.انتہا(), model.گنتی(id) != 0);  // As the model
            اگر (it != سیٹ.انتہا()) UASSERT_SELFTEST(*it, id);  // And is the entry
            توڑو;
        }
        }
        UASSERT_SELFTEST(سیٹ.سائز(), model.سائز());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    برائے (سائز_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(سیٹ.شامل_ہے(id), model.گنتی(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقی, UNIVERSE> نظر_آیا{};
    سائز_t n = 0;
    برائے (ثابت سائز_t اندراج_2 : سیٹ) {
        UASSERT_SELFTEST(اندراج_2 < UNIVERSE, سچ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.گنتی(اندراج_2) != 0, سچ);  // Only what was inserted
        UASSERT_SELFTEST(نظر_آیا[اندراج_2], جھوٹ);  // Each entry once
        نظر_آیا[اندراج_2] = سچ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.سائز());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

خالی testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    استعمال Map = V3HashMap<std::string, صحیح>;
    استعمال اندراج_3 = Map::اندراج_3;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<اندراج_3, std::pair<std::string, صحیح>>::قدر,
                  "SelfTest: a map entry must be a plain pair");

    constexpr سائز_t N = GROWS_TWICE;
    Map map;
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::مقصد_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ثابت std::pair<Map::iterator, منطقی> pair
            = map.داخل_کرو_Lazy(کلید, [&]() -> اندراج_3 { واپس {کلید, static_cast<صحیح>(i)}; });
        UASSERT_SELFTEST(pair.دوسرا, سچ);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.سائز(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.ڈھونڈو("key" + std::مقصد_string(i));
        UASSERT_SELFTEST(it != map.انتہا(), سچ);  // The key alone finds it
        UASSERT_SELFTEST(it->دوسرا, static_cast<صحیح>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::مقصد_string(i);
        map.erase(map.ڈھونڈو(کلید));
        UASSERT_SELFTEST(map.داخل_کرو({کلید, static_cast<صحیح>(i) + 100}).دوسرا, سچ);  // Anew
    }
    UASSERT_SELFTEST(map.سائز(), N);  // The same keys, so the map has not grown
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.ڈھونڈو("key" + std::مقصد_string(i));
        UASSERT_SELFTEST(it != map.انتہا(), سچ);  // Each key is still there
        UASSERT_SELFTEST(it->دوسرا, static_cast<صحیح>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    برائے (سائز_t i = 0; i < N; ++i) map.ڈھونڈو("key" + std::مقصد_string(i)).قدر() += 100;
    UASSERT_SELFTEST(map.سائز(), N);  // The map is unchanged otherwise
    برائے (سائز_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.ڈھونڈو("key" + std::مقصد_string(i));
        UASSERT_SELFTEST(it != map.انتہا(), سچ);  // Each key still finds its entry
        UASSERT_SELFTEST(it->دوسرا, static_cast<صحیح>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    برائے (سائز_t i = 0; i < N; ++i) map.ڈھونڈو("key" + std::مقصد_string(i)).قدر() -= 100;

    // Adding a key that is present changes nothing
    {
        ثابت std::string کلید = "key0";
        منطقی created = جھوٹ;
        ثابت std::pair<Map::iterator, منطقی> pair = map.داخل_کرو_Lazy(کلید, [&]() -> اندراج_3 {
            created = سچ;  // LCOV_EXCL_START
            واپس {کلید, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, جھوٹ);  // The factory was never called
        UASSERT_SELFTEST(pair.دوسرا, جھوٹ);  // As the key is present
        UASSERT_SELFTEST(pair.پہلا->دوسرا, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.سائز(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ثابت std::pair<Map::iterator, منطقی> added = map.داخل_کرو({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.دوسرا, سچ);  // Added, as the key is new
        UASSERT_SELFTEST(added.پہلا->دوسرا, 5);  // With the value given
        ثابت std::pair<Map::iterator, منطقی> again = map.داخل_کرو({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.دوسرا, جھوٹ);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.پہلا->دوسرا, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.سائز(), N + 1);  // Just the one entry was added
    {
        ثابت Map::iterator it = map.ڈھونڈو(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.انتہا(), سچ);  // Present before the erase
        UASSERT_SELFTEST(it->دوسرا, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.سائز(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), جھوٹ);  // No such key
    UASSERT_SELFTEST(map.سائز(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), سچ);  // That key is present
    UASSERT_SELFTEST(map.سائز(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.شامل_ہے(std::string{"key0"}), جھوٹ);  // Namely that one
    برائے (سائز_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.شامل_ہے("key" + std::مقصد_string(i)), سچ);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
سانچہ <قسم_نام T_Hash>
خالی testMapAgainstModel() {
    ساخت Equal final {
        منطقی عامل()(سائز_t a, سائز_t b) ثابت { واپس a == b; }
    };

    استعمال Map = V3HashMap<سائز_t, سائز_t, T_Hash, Equal>;
    استعمال اندراج_3 = قسم_نام Map::اندراج_3;

    constexpr سائز_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr سائز_t اقدام = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ثابت خودکار اگلا_Rand = [&state]() -> سائز_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        واپس static_cast<سائز_t>(state >> 32);
    };

    Map map;
    std::map<سائز_t, سائز_t> model;

    برائے (سائز_t قدم = 0; قدم < اقدام; ++قدم) {
        ثابت سائز_t کلید = اگلا_Rand() % UNIVERSE;
        ثابت سائز_t val = قدم;  // Distinct every step, so a stale value is visible
        انتخاب (اگلا_Rand() % 5) {
        حالت 0: {  // Insert a whole entry
            ثابت std::pair<قسم_نام Map::iterator, منطقی> pair = map.داخل_کرو({کلید, val});
            UASSERT_SELFTEST(pair.دوسرا, model.داخل_کرو({کلید, val}).دوسرا);  // Ditto
            UASSERT_SELFTEST(pair.پہلا->پہلا, کلید);  // At the key asked for
            توڑو;
        }
        حالت 1: {  // Insert lazily, which must create only on a miss
            منطقی created = جھوٹ;
            ثابت std::pair<قسم_نام Map::iterator, منطقی> pair
                = map.داخل_کرو_Lazy(کلید, [&]() -> اندراج_3 {
                      created = سچ;
                      واپس {کلید, val};
                  });
            UASSERT_SELFTEST(pair.دوسرا, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.دوسرا, model.داخل_کرو({کلید, val}).دوسرا);  // Ditto
            UASSERT_SELFTEST(pair.پہلا->پہلا, کلید);  // At the key asked for
            توڑو;
        }
        حالت 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(کلید), model.erase(کلید) != 0);  // As model
            توڑو;
        }
        حالت 3: {  // Change a value, which is erasing the entry and inserting it again
            ثابت قسم_نام Map::iterator it = map.ڈھونڈو(کلید);
            اگر (it != map.انتہا()) {
                map.erase(it);
                UASSERT_SELFTEST(map.داخل_کرو({کلید, val}).دوسرا, سچ);  // The key is free
                model[کلید] = val;
            }
            توڑو;
        }
        طےشدہ: {  // Look up only
            ثابت قسم_نام Map::iterator it = map.ڈھونڈو(کلید);
            UASSERT_SELFTEST(it != map.انتہا(), model.گنتی(کلید) != 0);  // As the model
            اگر (it != map.انتہا()) {
                UASSERT_SELFTEST(it->پہلا, کلید);  // At the key asked for
                UASSERT_SELFTEST(it->دوسرا, model.at(کلید));  // With its value
            }
            توڑو;
        }
        }
        UASSERT_SELFTEST(map.سائز(), model.سائز());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    برائے (سائز_t کلید = 0; کلید < UNIVERSE; ++کلید) {
        ثابت قسم_نام Map::iterator it = map.ڈھونڈو(کلید);
        UASSERT_SELFTEST(it != map.انتہا(), model.گنتی(کلید) != 0);  // Present iff modelled
        اگر (it != map.انتہا()) {
            UASSERT_SELFTEST(it->دوسرا, model.at(کلید));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقی, UNIVERSE> نظر_آیا{};
    سائز_t n = 0;
    برائے (ثابت اندراج_3& اندراج_2 : map) {
        UASSERT_SELFTEST(اندراج_2.پہلا < UNIVERSE, سچ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.گنتی(اندراج_2.پہلا) != 0, سچ);  // Only what was inserted
        UASSERT_SELFTEST(اندراج_2.دوسرا, model.at(اندراج_2.پہلا));  // With the right value
        UASSERT_SELFTEST(نظر_آیا[اندراج_2.پہلا], جھوٹ);  // Each entry once
        نظر_آیا[اندراج_2.پہلا] = سچ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.سائز());  // And all of them
}

خالی selfTest() {
    test_قدر_کلیدیں();
    test_اشارہ_گر_کلیدیں();
    test_اندراج_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_سرکاؤ_Deletion();
    testReferenceStability();
    test_سیٹ_Against_Model<ClusteredHash>();
    test_سیٹ_Against_Model<پھیلاؤ_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<پھیلاؤ_Hash>();
}

}  // namespace V3HashTableInternals
