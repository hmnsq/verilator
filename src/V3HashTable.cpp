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
constexpr اندازه_t max_بارگیری(اندازه_t ظرفیت) { أرجع ظرفیت * بارگیری_ضریب_رقم / بارگیری_ضریب_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr اندازه_t GROWS_TWICE = max_بارگیری(4 * MIN_ظرفیت);
static_assert(GROWS_TWICE > max_بارگیری(2 * MIN_ظرفیت),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

فراغ test_مقدار_کلیدها() {
    بنية مقدار_2 final {
        اندازه_t m_hash = 0;  // Hash of this entry
        اندازه_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        اندازه_t عملگر()(ثابت مقدار_2& مقدار) ثابت { أرجع عملگر()(مقدار.m_hash, 0); }
        اندازه_t عملگر()(اندازه_t hash, اندازه_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عملگر()(ثابت مقدار_2& a, ثابت مقدار_2& b) ثابت {
            أرجع عملگر()(a, b.m_hash, b.m_id);
        }
        منطقي عملگر()(ثابت مقدار_2& مقدار, اندازه_t hash, اندازه_t id) ثابت {
            أرجع مقدار.m_hash == hash && مقدار.m_id == id;
        }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<مقدار_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ثابت مقدار_2 مقدار{1, 0};
        ثابت مقدار_2 equal{1, 0};  // Equal to 'value', and hashes the same
        مجموعه_2 مجموعه;
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Starts empty
        UASSERT_SELFTEST(مجموعه.begin() == مجموعه.انتها(), درست);  // So iterates nothing
        UASSERT_SELFTEST(مجموعه.پیدا_کن(مقدار) == مجموعه.انتها(), درست);  // And finds nothing
        ثابت std::pair<مجموعه_2::iterator, منطقي> added = مجموعه.درج(مقدار);
        UASSERT_SELFTEST(added.دومی, درست);  // Added, as it was absent
        UASSERT_SELFTEST(added.اولی->m_id, مقدار.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.اولی != &مقدار, درست);  // Which is a copy of the argument
        UASSERT_SELFTEST(مجموعه.اندازه(), 1);  // And is the only one
        UASSERT_SELFTEST(مجموعه.خالی(), خطأ);  // So the set is no longer empty
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.اولی);  // At the entry held
        }
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(equal);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.اولی);  // At that same entry
        }
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(1, اندازه_t{0});
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.اولی);  // At that same entry again
        }
        UASSERT_SELFTEST(مجموعه.پیدا_کن(1, اندازه_t{1}) == مجموعه.انتها(), درست);  // Hash same, id not
        UASSERT_SELFTEST(مجموعه.پیدا_کن(2, اندازه_t{0}) == مجموعه.انتها(), درست);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<مجموعه_2::iterator, منطقي> again = مجموعه.درج(equal);
        UASSERT_SELFTEST(again.دومی, خطأ);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.اولی, &*added.اولی);  // At the entry already held
        UASSERT_SELFTEST(مجموعه.اندازه(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Present before the erase
            مجموعه.erase(it);
        }
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Empty again
        UASSERT_SELFTEST(مجموعه.پیدا_کن(مقدار) == مجموعه.انتها(), درست);  // So the entry is gone
        UASSERT_SELFTEST(مجموعه.شامل_است(مقدار), خطأ);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(مجموعه.erase(مقدار), خطأ);  // Nothing to erase, so says so
        مجموعه.درج(مقدار);
        UASSERT_SELFTEST(مجموعه.erase(equal), درست);  // An equal key erases the entry held
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        مجموعه.درج(مقدار);
        UASSERT_SELFTEST(مجموعه.erase(1, اندازه_t{0}), درست);  // Those parts find and erase it
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr اندازه_t N = GROWS_TWICE;
        مجموعه_2 مجموعه;
        لكل (اندازه_t i = 0; i < N; ++i) {
            ثابت std::pair<مجموعه_2::iterator, منطقي> added = مجموعه.درج(مقدار_2{7, i});
            UASSERT_SELFTEST(added.دومی, درست);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), N);  // All of them, in the one probe run
        لكل (اندازه_t i = 0; i < N; ++i) {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(اندازه_t{7}, i);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<منطقي, N> دیده{};
        اندازه_t n = 0;
        لكل (ثابت مقدار_2& مدخل : مجموعه) {
            UASSERT_SELFTEST(مدخل.m_hash, اندازه_t{7});  // Only entries added
            UASSERT_SELFTEST(دیده[مدخل.m_id], خطأ);  // Each of them exactly once
            دیده[مدخل.m_id] = درست;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

فراغ test_اشاره_گر_کلیدها() {
    بنية مقدار_2 final {
        اندازه_t m_hash = 0;  // Hash of this entry
        اندازه_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        اندازه_t عملگر()(ثابت مقدار_2* valuep) ثابت { أرجع عملگر()(valuep->m_hash, 0); }
        اندازه_t عملگر()(اندازه_t hash, اندازه_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عملگر()(ثابت مقدار_2* ap, ثابت مقدار_2* bp) ثابت {
            أرجع عملگر()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عملگر()(ثابت مقدار_2* valuep, اندازه_t hash, اندازه_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<مقدار_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        مقدار_2 مقدار{1, 0};
        مقدار_2 equal{1, 0};  // Equal to 'value', and hashes the same
        مجموعه_2 مجموعه;
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Starts empty
        UASSERT_SELFTEST(مجموعه.begin() == مجموعه.انتها(), درست);  // So iterates nothing
        UASSERT_SELFTEST(مجموعه.پیدا_کن(&مقدار) == مجموعه.انتها(), درست);  // And finds nothing
        ثابت std::pair<مجموعه_2::iterator, منطقي> added = مجموعه.درج(&مقدار);
        UASSERT_SELFTEST(added.دومی, درست);  // Added, as it was absent
        UASSERT_SELFTEST(*added.اولی, &مقدار);  // The iterator is at the new entry
        UASSERT_SELFTEST(مجموعه.اندازه(), 1);  // Which is the only one
        UASSERT_SELFTEST(مجموعه.خالی(), خطأ);  // So the set is no longer empty
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Found by itself
            UASSERT_SELFTEST(*it, &مقدار);  // At the entry held
        }
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&equal);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // And by an equal entry
            UASSERT_SELFTEST(*it, &مقدار);  // At that same entry
        }
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(1, اندازه_t{0});
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // And by its parts
            UASSERT_SELFTEST(*it, &مقدار);  // At that same entry again
        }
        UASSERT_SELFTEST(مجموعه.پیدا_کن(1, اندازه_t{1}) == مجموعه.انتها(), درست);  // Hash same, id not
        UASSERT_SELFTEST(مجموعه.پیدا_کن(2, اندازه_t{0}) == مجموعه.انتها(), درست);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<مجموعه_2::iterator, منطقي> again = مجموعه.درج(&equal);
        UASSERT_SELFTEST(again.دومی, خطأ);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.اولی, &مقدار);  // The iterator is at the stored one
        UASSERT_SELFTEST(مجموعه.اندازه(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&equal);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &مقدار);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, خطأ);  // Not the one asked for
        }
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // The entry is still there
            UASSERT_SELFTEST(*it, &مقدار);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Present before the erase
            UASSERT_SELFTEST(*it, &مقدار);  // And is the object asked for
            مجموعه.erase(it);
        }
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Empty again
        UASSERT_SELFTEST(مجموعه.پیدا_کن(&مقدار) == مجموعه.انتها(), درست);  // So the entry is gone
        UASSERT_SELFTEST(مجموعه.شامل_است(&مقدار), خطأ);  // As 'contains' agrees
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(مجموعه.erase(&مقدار), خطأ);  // Nothing to erase, so says so
        مجموعه.درج(&مقدار);
        UASSERT_SELFTEST(مجموعه.erase(&equal), درست);  // The equal key erases the stored one
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Which leaves the set empty
        UASSERT_SELFTEST(مجموعه.پیدا_کن(&مقدار) == مجموعه.انتها(), درست);  // And unreachable
        // And by a key spelled as the parts of an entry
        مجموعه.درج(&مقدار);
        UASSERT_SELFTEST(مجموعه.erase(1, اندازه_t{0}), درست);  // Those parts find and erase it
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    لكل (اندازه_t erase = 0; erase < 4; ++erase) {
        لكل (ثابت اندازه_t hash : {اندازه_t{0}, اندازه_t{7}, ~اندازه_t{0}}) {  // Wraps too
            std::array<مقدار_2, 4> مقادیر{مقدار_2{hash, 0}, مقدار_2{hash, 1},  //
                                        مقدار_2{hash, 2}, مقدار_2{hash, 3}};
            مجموعه_2 مجموعه;
            لكل (مقدار_2& مقدار : مقادیر) مجموعه.درج(&مقدار);
            UASSERT_SELFTEST(مجموعه.اندازه(), 4);  // All distinct, so all added
            {
                ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&مقادیر[erase]);
                UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // The one to erase is present
                UASSERT_SELFTEST(*it, &مقادیر[erase]);  // And is the object held
                مجموعه.erase(it);
            }
            UASSERT_SELFTEST(مجموعه.اندازه(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(مجموعه.پیدا_کن(&مقادیر[erase]) == مجموعه.انتها(), درست);  // That one
            لكل (اندازه_t i = 0; i < مقادیر.اندازه(); ++i) {
                إذا (i == erase) استمر;
                ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(hash, i);
                UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // The others are all there
                UASSERT_SELFTEST(*it, &مقادیر[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    لكل (ثابت اندازه_t hash : {اندازه_t{0}, اندازه_t{7}, ~اندازه_t{0}}) {
        std::array<مقدار_2, GROWS_TWICE> many{};
        مجموعه_2 مجموعه;
        لكل (اندازه_t i = 0; i < many.اندازه(); ++i) {
            many[i] = مقدار_2{hash, i};
            مجموعه.درج(&many[i]);
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), many.اندازه());  // All of them were added
        لكل (اندازه_t i = 0; i < many.اندازه(); ++i) {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(hash, i);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        مقدار_2 مقدار{1, 0};
        مقدار_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        مجموعه_2 مجموعه;
        مجموعه.reserve(2);
        مجموعه.درج(&مقدار);
        مجموعه.درج(&other);
        UASSERT_SELFTEST(مجموعه.اندازه(), 2);  // Both kept, despite the collision
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // The first of the run is there
            UASSERT_SELFTEST(*it, &مقدار);  // And is that entry
        }
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&other);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Present before the erase
            UASSERT_SELFTEST(*it, &مقدار);  // And is the object held
            مجموعه.erase(it);
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), 1);  // Only one was erased
        UASSERT_SELFTEST(مجموعه.پیدا_کن(&مقدار) == مجموعه.انتها(), درست);  // Namely that one
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&other);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    لكل (ثابت منطقي doReserve : {خطأ, درست}) {
        std::array<مقدار_2, 8> مقادیر{};
        مجموعه_2 مجموعه;
        إذا (doReserve) مجموعه.reserve(مقادیر.اندازه());
        لكل (اندازه_t i = 0; i < مقادیر.اندازه(); ++i) {
            مقادیر[i] = مقدار_2{static_cast<اندازه_t>(i * 1234567), i};
            مجموعه.درج(&مقادیر[i]);
        }
        لكل (مقدار_2& مقدار : مقادیر) {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(&مقدار);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Each entry survives
            UASSERT_SELFTEST(*it, &مقدار);  // And is the one inserted
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), مقادیر.اندازه());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr اندازه_t N = GROWS_TWICE;
        std::array<مقدار_2, N> مقادیر{};
        مجموعه_2 مجموعه;
        لكل (اندازه_t i = 0; i < N; ++i) {
            مقادیر[i] = مقدار_2{7, i};  // All of them hash the same
            ثابت std::pair<مجموعه_2::iterator, منطقي> pair
                = مجموعه.درج_Lazy(اندازه_t{7}, i, [&]() -> مقدار_2* { أرجع &مقادیر[i]; });
            UASSERT_SELFTEST(pair.دومی, درست);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.اولی, &مقادیر[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), N);  // All of them were added
        // They are all present now, so nothing is created
        لكل (اندازه_t i = 0; i < N; ++i) {
            منطقي created = خطأ;
            ثابت std::pair<مجموعه_2::iterator, منطقي> pair
                = مجموعه.درج_Lazy(اندازه_t{7}, i, [&]() -> مقدار_2* {
                      created = درست;  // LCOV_EXCL_START
                      أرجع &مقادیر[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, خطأ);  // The factory was never called
            UASSERT_SELFTEST(pair.دومی, خطأ);  // As the lookup hit
            UASSERT_SELFTEST(*pair.اولی, &مقادیر[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<مقدار_2, 20> مقادیر{};
        std::array<منطقي, 20> دیده{};
        مجموعه_2 مجموعه;
        لكل (اندازه_t i = 0; i < مقادیر.اندازه(); ++i) {
            مقادیر[i] = مقدار_2{i / 2, i};  // Pairs of them collide
            مجموعه.درج(&مقادیر[i]);
        }
        اندازه_t n = 0;
        لكل (مقدار_2* ثابت valuep : مجموعه) {
            UASSERT_SELFTEST(valuep->m_id < دیده.اندازه(), درست);  // Only entries added
            UASSERT_SELFTEST(دیده[valuep->m_id], خطأ);  // Each of them exactly once
            دیده[valuep->m_id] = درست;
            ++n;
        }
        UASSERT_SELFTEST(n, مقادیر.اندازه());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

فراغ test_مدخل_Lifetime() {
    اندازه_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    صنف NoDefault final {
        اندازه_t* m_alivep;  // Where the live entries are counted
        اندازه_t m_hash;  // Hash of this entry
        اندازه_t m_id;  // Entries with equal ids are equal

    عام:
        NoDefault(اندازه_t* alivep, اندازه_t hash, اندازه_t id)
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
        NoDefault& عملگر=(ثابت NoDefault&) = حذف;
        NoDefault& عملگر=(NoDefault&&) = حذف;

        اندازه_t hash() ثابت { أرجع m_hash; }
        اندازه_t id() ثابت { أرجع m_id; }
        منطقي عملگر==(ثابت NoDefault& that) ثابت { أرجع m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    بنية Hash final {
        اندازه_t عملگر()(ثابت NoDefault& مقدار) ثابت { أرجع مقدار.hash(); }
        اندازه_t عملگر()(اندازه_t hash, اندازه_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عملگر()(ثابت NoDefault& a, ثابت NoDefault& b) ثابت {
            أرجع عملگر()(a, b.hash(), b.id());
        }
        منطقي عملگر()(ثابت NoDefault& a, اندازه_t, اندازه_t id) ثابت { أرجع a.id() == id; }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<NoDefault, Hash, Equal>;

    constexpr اندازه_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        مجموعه_2 مجموعه;
        لكل (اندازه_t i = 0; i < N; ++i) {
            ثابت std::pair<مجموعه_2::iterator, منطقي> added = مجموعه.درج(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.دومی, درست);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        لكل (اندازه_t i = 0; i < N; ++i) {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(اندازه_t{7}, i);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ثابت std::pair<مجموعه_2::iterator, منطقي> dup = مجموعه.درج(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.دومی, خطأ);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.اولی->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(اندازه_t{7}, اندازه_t{0});
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Present before the erase
            UASSERT_SELFTEST(it->id(), اندازه_t{0});  // And is the entry asked for
            مجموعه.erase(it);
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(مجموعه.شامل_است(اندازه_t{7}, اندازه_t{0}), خطأ);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        لكل (اندازه_t i = 1; i < N; ++i) {
            {
                ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(اندازه_t{7}, i);
                UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                مجموعه.erase(it);
            }
            UASSERT_SELFTEST(مجموعه.اندازه(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            لكل (اندازه_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(مجموعه.شامل_است(اندازه_t{7}, j), درست);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(مجموعه.begin() == مجموعه.انتها(), درست);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        لكل (اندازه_t i = 0; i < N; ++i) مجموعه.درج(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        مجموعه.clear();
        UASSERT_SELFTEST(مجموعه.اندازه(), 0);  // Nothing left
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Says so
        UASSERT_SELFTEST(مجموعه.begin() == مجموعه.انتها(), درست);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(مجموعه.شامل_است(اندازه_t{7}, اندازه_t{0}), خطأ);  // Nor finds any
        مجموعه.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        لكل (اندازه_t i = 0; i < N; ++i) مجموعه.درج(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        مجموعه_2 مجموعه;
        مجموعه.clear();
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // Still empty
        مجموعه.درج(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(مجموعه.اندازه(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

فراغ testMove() {
    اندازه_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    بنية Counted final {
        اندازه_t* m_alivep;  // Where the live entries are counted
        اندازه_t m_id;  // Entries with equal ids are equal

        Counted(اندازه_t* alivep, اندازه_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ثابت Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& عملگر=(ثابت Counted&) = حذف;
        Counted& عملگر=(Counted&&) = حذف;
    };

    بنية Hash final {
        اندازه_t عملگر()(ثابت Counted& مدخل) ثابت { أرجع عملگر()(مدخل.m_id); }
        اندازه_t عملگر()(اندازه_t id) ثابت { أرجع id; }
    };

    بنية Equal final {
        منطقي عملگر()(ثابت Counted& a, ثابت Counted& b) ثابت { أرجع عملگر()(a, b.m_id); }
        منطقي عملگر()(ثابت Counted& a, اندازه_t id) ثابت { أرجع a.m_id == id; }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<Counted, Hash, Equal>;

    constexpr اندازه_t N = GROWS_TWICE;
    {
        مجموعه_2 مجموعه;
        لكل (اندازه_t i = 0; i < N; ++i) مجموعه.درج(Counted{&alive, i});
        UASSERT_SELFTEST(مجموعه.اندازه(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        مجموعه_2 moved{std::move(مجموعه)};
        UASSERT_SELFTEST(moved.اندازه(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // And the source no longer holds them
        UASSERT_SELFTEST(مجموعه.begin() == مجموعه.انتها(), درست);  // So it iterates nothing
        لكل (اندازه_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.شامل_است(i), درست);  // Every entry came across
            UASSERT_SELFTEST(مجموعه.شامل_است(i), خطأ);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(مجموعه.درج(Counted{&alive, N}).دومی, درست);  // It took an entry
        UASSERT_SELFTEST(مجموعه.اندازه(), 1);  // Which is all it holds
        UASSERT_SELFTEST(مجموعه.خالی(), خطأ);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        مجموعه = std::move(moved);
        UASSERT_SELFTEST(مجموعه.اندازه(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(مجموعه.شامل_است(N), خطأ);  // Namely that one
        UASSERT_SELFTEST(moved.خالی(), درست);  // And the source is empty again
        لكل (اندازه_t i = 0; i < N; ++i) UASSERT_SELFTEST(مجموعه.شامل_است(i), درست);  // The rest moved
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
        اندازه_t m_id;  // Entries with equal ids are equal

    عام:
        صریح MoveOnly(اندازه_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = افتراضي;
        MoveOnly(ثابت MoveOnly&) = حذف;
        MoveOnly& عملگر=(ثابت MoveOnly&) = حذف;
        MoveOnly& عملگر=(MoveOnly&&) = حذف;
        ~MoveOnly() = افتراضي;

        اندازه_t id() ثابت { أرجع m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::مقدار,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::مقدار,
                  "SelfTest: 'MoveOnly' must be move constructible");

    بنية Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        اندازه_t عملگر()(ثابت MoveOnly& مقدار) ثابت { أرجع عملگر()(مقدار.id()); }
        اندازه_t عملگر()(اندازه_t) ثابت { أرجع 7; }
    };

    بنية Equal final {
        منطقي عملگر()(ثابت MoveOnly& a, ثابت MoveOnly& b) ثابت {
            أرجع عملگر()(a, b.id());
        }
        منطقي عملگر()(ثابت MoveOnly& a, اندازه_t id) ثابت { أرجع a.id() == id; }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<MoveOnly, Hash, Equal>;

    constexpr اندازه_t N = GROWS_TWICE;
    مجموعه_2 مجموعه;
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت std::pair<مجموعه_2::iterator, منطقي> added
            = مجموعه.درج_Lazy(i, [i] { أرجع MoveOnly{i}; });
        UASSERT_SELFTEST(added.دومی, درست);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.اولی->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(مجموعه.اندازه(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    لكل (اندازه_t i = 0; i < N; i += 2) {
        ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(i);
        UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Present before the erase
        مجموعه.erase(it);
    }
    UASSERT_SELFTEST(مجموعه.اندازه(), N / 2);  // Half of them are gone
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت منطقي erased = (i % 2) == 0;
        UASSERT_SELFTEST(مجموعه.شامل_است(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

فراغ testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    صنف Hash final {
        اندازه_t m_ماسک;  // Only these bits of an entry matter

    عام:
        صریح Hash(اندازه_t ماسک)
            : m_ماسک{ماسک} {}
        اندازه_t عملگر()(اندازه_t مقدار) ثابت { أرجع مقدار & m_ماسک; }
    };

    صنف Equal final {
        اندازه_t m_ماسک;  // Only these bits of an entry matter

    عام:
        صریح Equal(اندازه_t ماسک)
            : m_ماسک{ماسک} {}
        منطقي عملگر()(اندازه_t a, اندازه_t b) ثابت { أرجع (a & m_ماسک) == (b & m_ماسک); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::مقدار,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::مقدار,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_مجموعه<اندازه_t, Hash, Equal> مجموعه{Hash{3}, Equal{3}};
    لكل (اندازه_t i = 0; i < 16; ++i) مجموعه.درج(i);
    UASSERT_SELFTEST(مجموعه.اندازه(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    لكل (اندازه_t i = 0; i < 16; ++i) {
        ثابت V3_Hash_مجموعه<اندازه_t, Hash, Equal>::iterator it = مجموعه.پیدا_کن(i);
        UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

فراغ test_Backward_لغزش_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    بنية مقدار_2 final {
        اندازه_t m_hash = 0;  // Hash of this entry
        اندازه_t m_id = 0;  // Entries with equal ids are equal
    };

    بنية Hash final {
        اندازه_t عملگر()(ثابت مقدار_2* valuep) ثابت { أرجع عملگر()(valuep->m_hash, 0); }
        اندازه_t عملگر()(اندازه_t hash, اندازه_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عملگر()(ثابت مقدار_2* ap, ثابت مقدار_2* bp) ثابت {
            أرجع عملگر()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عملگر()(ثابت مقدار_2* valuep, اندازه_t hash, اندازه_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<مقدار_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    لكل (ثابت اندازه_t h : {اندازه_t{0}, اندازه_t{5}, ~اندازه_t{0}}) {  // Wraps too
        std::array<مقدار_2, 4> مقادیر{مقدار_2{h, 0}, مقدار_2{h + 1, 1},  //
                                    مقدار_2{h, 2}, مقدار_2{h + 1, 3}};
        مجموعه_2 مجموعه;
        لكل (مقدار_2& مقدار : مقادیر) مجموعه.درج(&مقدار);
        UASSERT_SELFTEST(مجموعه.اندازه(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(h, اندازه_t{0});
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Present before the erase
            مجموعه.erase(it);
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), 3);  // One fewer entry
        UASSERT_SELFTEST(مجموعه.شامل_است(h, اندازه_t{0}), خطأ);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        لكل (اندازه_t i = 1; i < مقادیر.اندازه(); ++i) {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(مقادیر[i].m_hash, i);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &مقادیر[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(h + 1, اندازه_t{1});
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Left where the first erase found it
            مجموعه.erase(it);
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), 2);  // Two are left
        لكل (اندازه_t i = 2; i < مقادیر.اندازه(); ++i) {
            ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(مقادیر[i].m_hash, i);
            UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Both still reachable
            UASSERT_SELFTEST(*it, &مقادیر[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<منطقي, 4> دیده{};
        لكل (ثابت مقدار_2* ثابت valuep : مجموعه) {
            UASSERT_SELFTEST(دیده[valuep->m_id], خطأ);  // Each entry once
            دیده[valuep->m_id] = درست;
        }
        UASSERT_SELFTEST(دیده[0], خطأ);  // Erased first
        UASSERT_SELFTEST(دیده[1], خطأ);  // Erased second
        UASSERT_SELFTEST(دیده[2], درست);  // Moved back over the first hole
        UASSERT_SELFTEST(دیده[3], درست);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

فراغ testReferenceStability() {
    بنية مقدار_2 final {
        اندازه_t m_hash = 0;  // Hash of this entry
        اندازه_t m_id = 0;  // Id of this entry
    };

    بنية Hash final {
        اندازه_t عملگر()(ثابت مقدار_2* valuep) ثابت { أرجع عملگر()(valuep->m_hash, 0); }
        اندازه_t عملگر()(اندازه_t hash, اندازه_t) ثابت { أرجع hash; }
    };

    بنية Equal final {
        منطقي عملگر()(ثابت مقدار_2* ap, ثابت مقدار_2* bp) ثابت {
            أرجع عملگر()(ap, bp->m_hash, bp->m_id);
        }
        منطقي عملگر()(ثابت مقدار_2* valuep, اندازه_t hash, اندازه_t id) ثابت {
            أرجع valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<مقدار_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        مجموعه_2 مجموعه;
        مجموعه.reserve(8);
        UASSERT_SELFTEST(مجموعه.شامل_است(اندازه_t{7}, اندازه_t{0}), خطأ);  // Room, but no entry
        UASSERT_SELFTEST(مجموعه.begin() == مجموعه.انتها(), درست);  // So iterates nothing
        UASSERT_SELFTEST(مجموعه.خالی(), درست);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr اندازه_t N = max_بارگیری(2 * MIN_ظرفیت);
    static_assert(N > max_بارگیری(MIN_ظرفیت), "SelfTest: 'N' must need more than a new table");
    std::array<مقدار_2, N> مقادیر{};
    std::array<مقدار_2* ثابت*, N> entrypps{};  // Where each entry is stored, as inserted
    مجموعه_2 مجموعه;
    مجموعه.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    لكل (اندازه_t i = 0; i < N; ++i) {
        مقادیر[i] = مقدار_2{7, i};
        ثابت std::pair<مجموعه_2::iterator, منطقي> pair = مجموعه.درج(&مقادیر[i]);
        UASSERT_SELFTEST(pair.دومی, درست);  // Ids differ, so each is added
        entrypps[i] = &*pair.اولی;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(اندازه_t{7}, i);
        UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت std::pair<مجموعه_2::iterator, منطقي> pair = مجموعه.درج(&مقادیر[i]);
        UASSERT_SELFTEST(pair.دومی, خطأ);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.اولی, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(مجموعه.اندازه(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    مجموعه.reserve(8 * N);
    UASSERT_SELFTEST(مجموعه.اندازه(), N);  // Rehashing dropped nothing
    std::array<مقدار_2* ثابت*, N> grownpps{};  // Where each entry is after the growth
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(اندازه_t{7}, i);
        UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    مجموعه.reserve(N);
    UASSERT_SELFTEST(مجموعه.اندازه(), N);  // The smaller request changed nothing
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت مجموعه_2::iterator it = مجموعه.پیدا_کن(اندازه_t{7}, i);
        UASSERT_SELFTEST(it != مجموعه.انتها(), درست);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
بنية ClusteredHash final {
    اندازه_t عملگر()(اندازه_t مقدار) ثابت { أرجع مقدار & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
بنية پراکندگی_Hash final {
    اندازه_t عملگر()(اندازه_t مقدار) ثابت {
        أرجع static_cast<اندازه_t>(مقدار * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
قالب <نام_نوع T_Hash>
فراغ test_مجموعه_Against_Model() {
    بنية Equal final {
        منطقي عملگر()(اندازه_t a, اندازه_t b) ثابت { أرجع a == b; }
    };

    استفاده مجموعه_2 = V3_Hash_مجموعه<اندازه_t, T_Hash, Equal>;

    constexpr اندازه_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr اندازه_t گامها = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ثابت خودکار بعدی_Rand = [&state]() -> اندازه_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        أرجع static_cast<اندازه_t>(state >> 32);
    };

    مجموعه_2 مجموعه;
    std::مجموعه<اندازه_t> model;

    لكل (اندازه_t گام = 0; گام < گامها; ++گام) {
        ثابت اندازه_t id = بعدی_Rand() % UNIVERSE;
        بدل (بعدی_Rand() % 4) {
        حالة 0: {  // Insert a copy
            ثابت std::pair<نام_نوع مجموعه_2::iterator, منطقي> pair = مجموعه.درج(id);
            UASSERT_SELFTEST(pair.دومی, model.درج(id).دومی);  // As the model
            UASSERT_SELFTEST(*pair.اولی, id);  // At the entry asked for
            اكسر;
        }
        حالة 1: {  // Insert lazily, which must create only on a miss
            منطقي created = خطأ;
            ثابت std::pair<نام_نوع مجموعه_2::iterator, منطقي> pair
                = مجموعه.درج_Lazy(id, [&]() -> اندازه_t {
                      created = درست;
                      أرجع id;
                  });
            UASSERT_SELFTEST(pair.دومی, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.دومی, model.درج(id).دومی);  // As the model
            UASSERT_SELFTEST(*pair.اولی, id);  // At the entry asked for
            اكسر;
        }
        حالة 2: {  // Erase, if present
            ثابت نام_نوع مجموعه_2::iterator it = مجموعه.پیدا_کن(id);
            UASSERT_SELFTEST(it != مجموعه.انتها(), model.شمار(id) != 0);  // As the model
            إذا (it != مجموعه.انتها()) {
                مجموعه.erase(it);
                model.erase(id);
            }
            اكسر;
        }
        افتراضي: {  // Look up only
            ثابت نام_نوع مجموعه_2::iterator it = مجموعه.پیدا_کن(id);
            UASSERT_SELFTEST(it != مجموعه.انتها(), model.شمار(id) != 0);  // As the model
            إذا (it != مجموعه.انتها()) UASSERT_SELFTEST(*it, id);  // And is the entry
            اكسر;
        }
        }
        UASSERT_SELFTEST(مجموعه.اندازه(), model.اندازه());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    لكل (اندازه_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(مجموعه.شامل_است(id), model.شمار(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقي, UNIVERSE> دیده{};
    اندازه_t n = 0;
    لكل (ثابت اندازه_t مدخل : مجموعه) {
        UASSERT_SELFTEST(مدخل < UNIVERSE, درست);  // Nothing out of thin air
        UASSERT_SELFTEST(model.شمار(مدخل) != 0, درست);  // Only what was inserted
        UASSERT_SELFTEST(دیده[مدخل], خطأ);  // Each entry once
        دیده[مدخل] = درست;
        ++n;
    }
    UASSERT_SELFTEST(n, model.اندازه());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

فراغ testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    استفاده Map = V3HashMap<std::string, صحيح>;
    استفاده مدخل_2 = Map::مدخل_2;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<مدخل_2, std::pair<std::string, صحيح>>::مقدار,
                  "SelfTest: a map entry must be a plain pair");

    constexpr اندازه_t N = GROWS_TWICE;
    Map map;
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::مقصد_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ثابت std::pair<Map::iterator, منطقي> pair
            = map.درج_Lazy(کلید, [&]() -> مدخل_2 { أرجع {کلید, static_cast<صحيح>(i)}; });
        UASSERT_SELFTEST(pair.دومی, درست);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.اندازه(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.پیدا_کن("key" + std::مقصد_string(i));
        UASSERT_SELFTEST(it != map.انتها(), درست);  // The key alone finds it
        UASSERT_SELFTEST(it->دومی, static_cast<صحيح>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::مقصد_string(i);
        map.erase(map.پیدا_کن(کلید));
        UASSERT_SELFTEST(map.درج({کلید, static_cast<صحيح>(i) + 100}).دومی, درست);  // Anew
    }
    UASSERT_SELFTEST(map.اندازه(), N);  // The same keys, so the map has not grown
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.پیدا_کن("key" + std::مقصد_string(i));
        UASSERT_SELFTEST(it != map.انتها(), درست);  // Each key is still there
        UASSERT_SELFTEST(it->دومی, static_cast<صحيح>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    لكل (اندازه_t i = 0; i < N; ++i) map.پیدا_کن("key" + std::مقصد_string(i)).مقدار() += 100;
    UASSERT_SELFTEST(map.اندازه(), N);  // The map is unchanged otherwise
    لكل (اندازه_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.پیدا_کن("key" + std::مقصد_string(i));
        UASSERT_SELFTEST(it != map.انتها(), درست);  // Each key still finds its entry
        UASSERT_SELFTEST(it->دومی, static_cast<صحيح>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    لكل (اندازه_t i = 0; i < N; ++i) map.پیدا_کن("key" + std::مقصد_string(i)).مقدار() -= 100;

    // Adding a key that is present changes nothing
    {
        ثابت std::string کلید = "key0";
        منطقي created = خطأ;
        ثابت std::pair<Map::iterator, منطقي> pair = map.درج_Lazy(کلید, [&]() -> مدخل_2 {
            created = درست;  // LCOV_EXCL_START
            أرجع {کلید, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, خطأ);  // The factory was never called
        UASSERT_SELFTEST(pair.دومی, خطأ);  // As the key is present
        UASSERT_SELFTEST(pair.اولی->دومی, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.اندازه(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ثابت std::pair<Map::iterator, منطقي> added = map.درج({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.دومی, درست);  // Added, as the key is new
        UASSERT_SELFTEST(added.اولی->دومی, 5);  // With the value given
        ثابت std::pair<Map::iterator, منطقي> again = map.درج({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.دومی, خطأ);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.اولی->دومی, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.اندازه(), N + 1);  // Just the one entry was added
    {
        ثابت Map::iterator it = map.پیدا_کن(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.انتها(), درست);  // Present before the erase
        UASSERT_SELFTEST(it->دومی, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.اندازه(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), خطأ);  // No such key
    UASSERT_SELFTEST(map.اندازه(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), درست);  // That key is present
    UASSERT_SELFTEST(map.اندازه(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.شامل_است(std::string{"key0"}), خطأ);  // Namely that one
    لكل (اندازه_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.شامل_است("key" + std::مقصد_string(i)), درست);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
قالب <نام_نوع T_Hash>
فراغ testMapAgainstModel() {
    بنية Equal final {
        منطقي عملگر()(اندازه_t a, اندازه_t b) ثابت { أرجع a == b; }
    };

    استفاده Map = V3HashMap<اندازه_t, اندازه_t, T_Hash, Equal>;
    استفاده مدخل_2 = نام_نوع Map::مدخل_2;

    constexpr اندازه_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr اندازه_t گامها = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ثابت خودکار بعدی_Rand = [&state]() -> اندازه_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        أرجع static_cast<اندازه_t>(state >> 32);
    };

    Map map;
    std::map<اندازه_t, اندازه_t> model;

    لكل (اندازه_t گام = 0; گام < گامها; ++گام) {
        ثابت اندازه_t کلید = بعدی_Rand() % UNIVERSE;
        ثابت اندازه_t val = گام;  // Distinct every step, so a stale value is visible
        بدل (بعدی_Rand() % 5) {
        حالة 0: {  // Insert a whole entry
            ثابت std::pair<نام_نوع Map::iterator, منطقي> pair = map.درج({کلید, val});
            UASSERT_SELFTEST(pair.دومی, model.درج({کلید, val}).دومی);  // Ditto
            UASSERT_SELFTEST(pair.اولی->اولی, کلید);  // At the key asked for
            اكسر;
        }
        حالة 1: {  // Insert lazily, which must create only on a miss
            منطقي created = خطأ;
            ثابت std::pair<نام_نوع Map::iterator, منطقي> pair
                = map.درج_Lazy(کلید, [&]() -> مدخل_2 {
                      created = درست;
                      أرجع {کلید, val};
                  });
            UASSERT_SELFTEST(pair.دومی, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.دومی, model.درج({کلید, val}).دومی);  // Ditto
            UASSERT_SELFTEST(pair.اولی->اولی, کلید);  // At the key asked for
            اكسر;
        }
        حالة 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(کلید), model.erase(کلید) != 0);  // As model
            اكسر;
        }
        حالة 3: {  // Change a value, which is erasing the entry and inserting it again
            ثابت نام_نوع Map::iterator it = map.پیدا_کن(کلید);
            إذا (it != map.انتها()) {
                map.erase(it);
                UASSERT_SELFTEST(map.درج({کلید, val}).دومی, درست);  // The key is free
                model[کلید] = val;
            }
            اكسر;
        }
        افتراضي: {  // Look up only
            ثابت نام_نوع Map::iterator it = map.پیدا_کن(کلید);
            UASSERT_SELFTEST(it != map.انتها(), model.شمار(کلید) != 0);  // As the model
            إذا (it != map.انتها()) {
                UASSERT_SELFTEST(it->اولی, کلید);  // At the key asked for
                UASSERT_SELFTEST(it->دومی, model.at(کلید));  // With its value
            }
            اكسر;
        }
        }
        UASSERT_SELFTEST(map.اندازه(), model.اندازه());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    لكل (اندازه_t کلید = 0; کلید < UNIVERSE; ++کلید) {
        ثابت نام_نوع Map::iterator it = map.پیدا_کن(کلید);
        UASSERT_SELFTEST(it != map.انتها(), model.شمار(کلید) != 0);  // Present iff modelled
        إذا (it != map.انتها()) {
            UASSERT_SELFTEST(it->دومی, model.at(کلید));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقي, UNIVERSE> دیده{};
    اندازه_t n = 0;
    لكل (ثابت مدخل_2& مدخل : map) {
        UASSERT_SELFTEST(مدخل.اولی < UNIVERSE, درست);  // Nothing out of thin air
        UASSERT_SELFTEST(model.شمار(مدخل.اولی) != 0, درست);  // Only what was inserted
        UASSERT_SELFTEST(مدخل.دومی, model.at(مدخل.اولی));  // With the right value
        UASSERT_SELFTEST(دیده[مدخل.اولی], خطأ);  // Each entry once
        دیده[مدخل.اولی] = درست;
        ++n;
    }
    UASSERT_SELFTEST(n, model.اندازه());  // And all of them
}

فراغ selfTest() {
    test_مقدار_کلیدها();
    test_اشاره_گر_کلیدها();
    test_مدخل_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_لغزش_Deletion();
    testReferenceStability();
    test_مجموعه_Against_Model<ClusteredHash>();
    test_مجموعه_Against_Model<پراکندگی_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<پراکندگی_Hash>();
}

}  // namespace V3HashTableInternals
