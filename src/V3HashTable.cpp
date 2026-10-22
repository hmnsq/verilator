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

فضاینام V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr size_t maxLoad(size_t capacity) { بازگشت capacity * LOAD_FACTOR_رقم / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr size_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

تهی test_مقدار_کلیدها() {
    ساختار مقدار_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ساختار Hash final {
        size_t operator()(ثابت مقدار_2& مقدار) ثابت { بازگشت operator()(مقدار.m_hash, 0); }
        size_t operator()(size_t hash, size_t) ثابت { بازگشت hash; }
    };

    ساختار Equal final {
        منطقی operator()(ثابت مقدار_2& a, ثابت مقدار_2& b) ثابت {
            بازگشت operator()(a, b.m_hash, b.m_id);
        }
        منطقی operator()(ثابت مقدار_2& مقدار, size_t hash, size_t id) ثابت {
            بازگشت مقدار.m_hash == hash && مقدار.m_id == id;
        }
    };

    using Set = V3HashSet<مقدار_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ثابت مقدار_2 مقدار{1, 0};
        ثابت مقدار_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), درست);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), درست);  // So iterates nothing
        UASSERT_SELFTEST(set.find(مقدار) == set.end(), درست);  // And finds nothing
        ثابت std::pair<Set::iterator, منطقی> added = set.درج(مقدار);
        UASSERT_SELFTEST(added.second, درست);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, مقدار.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &مقدار, درست);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), نادرست);  // So the set is no longer empty
        {
            ثابت Set::iterator it = set.find(مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            ثابت Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), درست);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            ثابت Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), درست);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), درست);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), درست);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<Set::iterator, منطقی> again = set.درج(equal);
        UASSERT_SELFTEST(again.second, نادرست);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ثابت Set::iterator it = set.find(مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), درست);  // Empty again
        UASSERT_SELFTEST(set.find(مقدار) == set.end(), درست);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(مقدار), نادرست);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(مقدار), نادرست);  // Nothing to erase, so says so
        set.درج(مقدار);
        UASSERT_SELFTEST(set.erase(equal), درست);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), درست);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.درج(مقدار);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), درست);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), درست);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        برای (size_t i = 0; i < N; ++i) {
            ثابت std::pair<Set::iterator, منطقی> added = set.درج(مقدار_2{7, i});
            UASSERT_SELFTEST(added.second, درست);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        برای (size_t i = 0; i < N; ++i) {
            ثابت Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), درست);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<منطقی, N> seen{};
        size_t n = 0;
        برای (ثابت مقدار_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], نادرست);  // Each of them exactly once
            seen[entry.m_id] = درست;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

تهی test_Pointer_کلیدها() {
    ساختار مقدار_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ساختار Hash final {
        size_t operator()(ثابت مقدار_2* valuep) ثابت { بازگشت operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ثابت { بازگشت hash; }
    };

    ساختار Equal final {
        منطقی operator()(ثابت مقدار_2* ap, ثابت مقدار_2* bp) ثابت {
            بازگشت operator()(ap, bp->m_hash, bp->m_id);
        }
        منطقی operator()(ثابت مقدار_2* valuep, size_t hash, size_t id) ثابت {
            بازگشت valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<مقدار_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        مقدار_2 مقدار{1, 0};
        مقدار_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), درست);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), درست);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&مقدار) == set.end(), درست);  // And finds nothing
        ثابت std::pair<Set::iterator, منطقی> added = set.درج(&مقدار);
        UASSERT_SELFTEST(added.second, درست);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &مقدار);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), نادرست);  // So the set is no longer empty
        {
            ثابت Set::iterator it = set.find(&مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // Found by itself
            UASSERT_SELFTEST(*it, &مقدار);  // At the entry held
        }
        {
            ثابت Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), درست);  // And by an equal entry
            UASSERT_SELFTEST(*it, &مقدار);  // At that same entry
        }
        {
            ثابت Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), درست);  // And by its parts
            UASSERT_SELFTEST(*it, &مقدار);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), درست);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), درست);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ثابت std::pair<Set::iterator, منطقی> again = set.درج(&equal);
        UASSERT_SELFTEST(again.second, نادرست);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &مقدار);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ثابت Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), درست);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &مقدار);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, نادرست);  // Not the one asked for
        }
        {
            ثابت Set::iterator it = set.find(&مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // The entry is still there
            UASSERT_SELFTEST(*it, &مقدار);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ثابت Set::iterator it = set.find(&مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // Present before the erase
            UASSERT_SELFTEST(*it, &مقدار);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), درست);  // Empty again
        UASSERT_SELFTEST(set.find(&مقدار) == set.end(), درست);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&مقدار), نادرست);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), درست);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&مقدار), نادرست);  // Nothing to erase, so says so
        set.درج(&مقدار);
        UASSERT_SELFTEST(set.erase(&equal), درست);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), درست);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&مقدار) == set.end(), درست);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.درج(&مقدار);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), درست);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), درست);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    برای (size_t erase = 0; erase < 4; ++erase) {
        برای (ثابت size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<مقدار_2, 4> مقادیر{مقدار_2{hash, 0}, مقدار_2{hash, 1},  //
                                        مقدار_2{hash, 2}, مقدار_2{hash, 3}};
            Set set;
            برای (مقدار_2& مقدار : مقادیر) set.درج(&مقدار);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                ثابت Set::iterator it = set.find(&مقادیر[erase]);
                UASSERT_SELFTEST(it != set.end(), درست);  // The one to erase is present
                UASSERT_SELFTEST(*it, &مقادیر[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&مقادیر[erase]) == set.end(), درست);  // That one
            برای (size_t i = 0; i < مقادیر.size(); ++i) {
                اگر (i == erase) ادامه;
                ثابت Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), درست);  // The others are all there
                UASSERT_SELFTEST(*it, &مقادیر[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    برای (ثابت size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<مقدار_2, GROWS_TWICE> many{};
        Set set;
        برای (size_t i = 0; i < many.size(); ++i) {
            many[i] = مقدار_2{hash, i};
            set.درج(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        برای (size_t i = 0; i < many.size(); ++i) {
            ثابت Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), درست);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        مقدار_2 مقدار{1, 0};
        مقدار_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.درج(&مقدار);
        set.درج(&other);
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            ثابت Set::iterator it = set.find(&مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // The first of the run is there
            UASSERT_SELFTEST(*it, &مقدار);  // And is that entry
        }
        {
            ثابت Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), درست);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ثابت Set::iterator it = set.find(&مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // Present before the erase
            UASSERT_SELFTEST(*it, &مقدار);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&مقدار) == set.end(), درست);  // Namely that one
        {
            ثابت Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), درست);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    برای (ثابت منطقی doReserve : {نادرست, درست}) {
        std::array<مقدار_2, 8> مقادیر{};
        Set set;
        اگر (doReserve) set.reserve(مقادیر.size());
        برای (size_t i = 0; i < مقادیر.size(); ++i) {
            مقادیر[i] = مقدار_2{static_cast<size_t>(i * 1234567), i};
            set.درج(&مقادیر[i]);
        }
        برای (مقدار_2& مقدار : مقادیر) {
            ثابت Set::iterator it = set.find(&مقدار);
            UASSERT_SELFTEST(it != set.end(), درست);  // Each entry survives
            UASSERT_SELFTEST(*it, &مقدار);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), مقادیر.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<مقدار_2, N> مقادیر{};
        Set set;
        برای (size_t i = 0; i < N; ++i) {
            مقادیر[i] = مقدار_2{7, i};  // All of them hash the same
            ثابت std::pair<Set::iterator, منطقی> pair
                = set.درج_Lazy(size_t{7}, i, [&]() -> مقدار_2* { بازگشت &مقادیر[i]; });
            UASSERT_SELFTEST(pair.second, درست);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &مقادیر[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        برای (size_t i = 0; i < N; ++i) {
            منطقی created = نادرست;
            ثابت std::pair<Set::iterator, منطقی> pair
                = set.درج_Lazy(size_t{7}, i, [&]() -> مقدار_2* {
                      created = درست;  // LCOV_EXCL_START
                      بازگشت &مقادیر[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, نادرست);  // The factory was never called
            UASSERT_SELFTEST(pair.second, نادرست);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &مقادیر[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<مقدار_2, 20> مقادیر{};
        std::array<منطقی, 20> seen{};
        Set set;
        برای (size_t i = 0; i < مقادیر.size(); ++i) {
            مقادیر[i] = مقدار_2{i / 2, i};  // Pairs of them collide
            set.درج(&مقادیر[i]);
        }
        size_t n = 0;
        برای (مقدار_2* ثابت valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), درست);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], نادرست);  // Each of them exactly once
            seen[valuep->m_id] = درست;
            ++n;
        }
        UASSERT_SELFTEST(n, مقادیر.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

تهی testEntryLifetime() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    رده NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    عمومی:
        NoDefault(size_t* alivep, size_t hash, size_t id)
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
        NoDefault& operator=(ثابت NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() ثابت { بازگشت m_hash; }
        size_t id() ثابت { بازگشت m_id; }
        منطقی operator==(ثابت NoDefault& that) ثابت { بازگشت m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::مقدار,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    ساختار Hash final {
        size_t operator()(ثابت NoDefault& مقدار) ثابت { بازگشت مقدار.hash(); }
        size_t operator()(size_t hash, size_t) ثابت { بازگشت hash; }
    };

    ساختار Equal final {
        منطقی operator()(ثابت NoDefault& a, ثابت NoDefault& b) ثابت {
            بازگشت operator()(a, b.hash(), b.id());
        }
        منطقی operator()(ثابت NoDefault& a, size_t, size_t id) ثابت { بازگشت a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        برای (size_t i = 0; i < N; ++i) {
            ثابت std::pair<Set::iterator, منطقی> added = set.درج(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, درست);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        برای (size_t i = 0; i < N; ++i) {
            ثابت Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), درست);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ثابت std::pair<Set::iterator, منطقی> dup = set.درج(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, نادرست);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ثابت Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), درست);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), نادرست);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        برای (size_t i = 1; i < N; ++i) {
            {
                ثابت Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), درست);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            برای (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), درست);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), درست);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        برای (size_t i = 0; i < N; ++i) set.درج(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), درست);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), درست);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), نادرست);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), درست);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        برای (size_t i = 0; i < N; ++i) set.درج(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), درست);  // Still empty
        set.درج(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

تهی testMove() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    ساختار Counted final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_id;  // Entries with equal ids are equal

        Counted(size_t* alivep, size_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ثابت Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(ثابت Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    ساختار Hash final {
        size_t operator()(ثابت Counted& entry) ثابت { بازگشت operator()(entry.m_id); }
        size_t operator()(size_t id) ثابت { بازگشت id; }
    };

    ساختار Equal final {
        منطقی operator()(ثابت Counted& a, ثابت Counted& b) ثابت { بازگشت operator()(a, b.m_id); }
        منطقی operator()(ثابت Counted& a, size_t id) ثابت { بازگشت a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        برای (size_t i = 0; i < N; ++i) set.درج(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), درست);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), درست);  // So it iterates nothing
        برای (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), درست);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), نادرست);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.درج(Counted{&alive, N}).second, درست);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), نادرست);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), نادرست);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), درست);  // And the source is empty again
        برای (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), درست);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

تهی testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    رده MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    عمومی:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = پیشفرض;
        MoveOnly(ثابت MoveOnly&) = delete;
        MoveOnly& operator=(ثابت MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = پیشفرض;

        size_t id() ثابت { بازگشت m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::مقدار,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::مقدار,
                  "SelfTest: 'MoveOnly' must be move constructible");

    ساختار Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(ثابت MoveOnly& مقدار) ثابت { بازگشت operator()(مقدار.id()); }
        size_t operator()(size_t) ثابت { بازگشت 7; }
    };

    ساختار Equal final {
        منطقی operator()(ثابت MoveOnly& a, ثابت MoveOnly& b) ثابت {
            بازگشت operator()(a, b.id());
        }
        منطقی operator()(ثابت MoveOnly& a, size_t id) ثابت { بازگشت a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    برای (size_t i = 0; i < N; ++i) {
        ثابت std::pair<Set::iterator, منطقی> added
            = set.درج_Lazy(i, [i] { بازگشت MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, درست);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    برای (size_t i = 0; i < N; i += 2) {
        ثابت Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), درست);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    برای (size_t i = 0; i < N; ++i) {
        ثابت منطقی erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

تهی testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    رده Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    عمومی:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t مقدار) ثابت { بازگشت مقدار & m_mask; }
    };

    رده Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    عمومی:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        منطقی operator()(size_t a, size_t b) ثابت { بازگشت (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::مقدار,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::مقدار,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    برای (size_t i = 0; i < 16; ++i) set.درج(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    برای (size_t i = 0; i < 16; ++i) {
        ثابت V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), درست);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

تهی testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    ساختار مقدار_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Entries with equal ids are equal
    };

    ساختار Hash final {
        size_t operator()(ثابت مقدار_2* valuep) ثابت { بازگشت operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ثابت { بازگشت hash; }
    };

    ساختار Equal final {
        منطقی operator()(ثابت مقدار_2* ap, ثابت مقدار_2* bp) ثابت {
            بازگشت operator()(ap, bp->m_hash, bp->m_id);
        }
        منطقی operator()(ثابت مقدار_2* valuep, size_t hash, size_t id) ثابت {
            بازگشت valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<مقدار_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    برای (ثابت size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<مقدار_2, 4> مقادیر{مقدار_2{h, 0}, مقدار_2{h + 1, 1},  //
                                    مقدار_2{h, 2}, مقدار_2{h + 1, 3}};
        Set set;
        برای (مقدار_2& مقدار : مقادیر) set.درج(&مقدار);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ثابت Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), درست);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), نادرست);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        برای (size_t i = 1; i < مقادیر.size(); ++i) {
            ثابت Set::iterator it = set.find(مقادیر[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), درست);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &مقادیر[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ثابت Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), درست);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        برای (size_t i = 2; i < مقادیر.size(); ++i) {
            ثابت Set::iterator it = set.find(مقادیر[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), درست);  // Both still reachable
            UASSERT_SELFTEST(*it, &مقادیر[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<منطقی, 4> seen{};
        برای (ثابت مقدار_2* ثابت valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], نادرست);  // Each entry once
            seen[valuep->m_id] = درست;
        }
        UASSERT_SELFTEST(seen[0], نادرست);  // Erased first
        UASSERT_SELFTEST(seen[1], نادرست);  // Erased second
        UASSERT_SELFTEST(seen[2], درست);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], درست);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

تهی testReferenceStability() {
    ساختار مقدار_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ساختار Hash final {
        size_t operator()(ثابت مقدار_2* valuep) ثابت { بازگشت operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) ثابت { بازگشت hash; }
    };

    ساختار Equal final {
        منطقی operator()(ثابت مقدار_2* ap, ثابت مقدار_2* bp) ثابت {
            بازگشت operator()(ap, bp->m_hash, bp->m_id);
        }
        منطقی operator()(ثابت مقدار_2* valuep, size_t hash, size_t id) ثابت {
            بازگشت valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<مقدار_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), نادرست);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), درست);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), درست);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<مقدار_2, N> مقادیر{};
    std::array<مقدار_2* ثابت*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    برای (size_t i = 0; i < N; ++i) {
        مقادیر[i] = مقدار_2{7, i};
        ثابت std::pair<Set::iterator, منطقی> pair = set.درج(&مقادیر[i]);
        UASSERT_SELFTEST(pair.second, درست);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    برای (size_t i = 0; i < N; ++i) {
        ثابت Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), درست);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    برای (size_t i = 0; i < N; ++i) {
        ثابت std::pair<Set::iterator, منطقی> pair = set.درج(&مقادیر[i]);
        UASSERT_SELFTEST(pair.second, نادرست);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<مقدار_2* ثابت*, N> grownpps{};  // Where each entry is after the growth
    برای (size_t i = 0; i < N; ++i) {
        ثابت Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), درست);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    برای (size_t i = 0; i < N; ++i) {
        ثابت Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), درست);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
ساختار ClusteredHash final {
    size_t operator()(size_t مقدار) ثابت { بازگشت مقدار & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
ساختار SpreadHash final {
    size_t operator()(size_t مقدار) ثابت {
        بازگشت static_cast<size_t>(مقدار * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
تهی testSetAgainstModel() {
    ساختار Equal final {
        منطقی operator()(size_t a, size_t b) ثابت { بازگشت a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ثابت auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        بازگشت static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    برای (size_t step = 0; step < STEPS; ++step) {
        ثابت size_t id = nextRand() % UNIVERSE;
        انتخاب (nextRand() % 4) {
        حالت 0: {  // Insert a copy
            ثابت std::pair<typename Set::iterator, منطقی> pair = set.درج(id);
            UASSERT_SELFTEST(pair.second, model.درج(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            بشکن;
        }
        حالت 1: {  // Insert lazily, which must create only on a miss
            منطقی created = نادرست;
            ثابت std::pair<typename Set::iterator, منطقی> pair
                = set.درج_Lazy(id, [&]() -> size_t {
                      created = درست;
                      بازگشت id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.درج(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            بشکن;
        }
        حالت 2: {  // Erase, if present
            ثابت typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.شمار(id) != 0);  // As the model
            اگر (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            بشکن;
        }
        پیشفرض: {  // Look up only
            ثابت typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.شمار(id) != 0);  // As the model
            اگر (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            بشکن;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    برای (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.شمار(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقی, UNIVERSE> seen{};
    size_t n = 0;
    برای (ثابت size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, درست);  // Nothing out of thin air
        UASSERT_SELFTEST(model.شمار(entry) != 0, درست);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], نادرست);  // Each entry once
        seen[entry] = درست;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

تهی testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, صحیح>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, صحیح>>::مقدار,
                  "SelfTest: a map entry must be a plain pair");

    constexpr size_t N = GROWS_TWICE;
    Map map;
    برای (size_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ثابت std::pair<Map::iterator, منطقی> pair
            = map.درج_Lazy(کلید, [&]() -> Entry { بازگشت {کلید, static_cast<صحیح>(i)}; });
        UASSERT_SELFTEST(pair.second, درست);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    برای (size_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), درست);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<صحیح>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    برای (size_t i = 0; i < N; ++i) {
        ثابت std::string کلید = "key" + std::to_string(i);
        map.erase(map.find(کلید));
        UASSERT_SELFTEST(map.درج({کلید, static_cast<صحیح>(i) + 100}).second, درست);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    برای (size_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), درست);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<صحیح>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    برای (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).مقدار() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    برای (size_t i = 0; i < N; ++i) {
        ثابت Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), درست);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<صحیح>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    برای (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).مقدار() -= 100;

    // Adding a key that is present changes nothing
    {
        ثابت std::string کلید = "key0";
        منطقی created = نادرست;
        ثابت std::pair<Map::iterator, منطقی> pair = map.درج_Lazy(کلید, [&]() -> Entry {
            created = درست;  // LCOV_EXCL_START
            بازگشت {کلید, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, نادرست);  // The factory was never called
        UASSERT_SELFTEST(pair.second, نادرست);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ثابت std::pair<Map::iterator, منطقی> added = map.درج({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, درست);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        ثابت std::pair<Map::iterator, منطقی> again = map.درج({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, نادرست);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        ثابت Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), درست);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), نادرست);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), درست);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), نادرست);  // Namely that one
    برای (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), درست);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
تهی testMapAgainstModel() {
    ساختار Equal final {
        منطقی operator()(size_t a, size_t b) ثابت { بازگشت a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ثابت auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        بازگشت static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    برای (size_t step = 0; step < STEPS; ++step) {
        ثابت size_t کلید = nextRand() % UNIVERSE;
        ثابت size_t val = step;  // Distinct every step, so a stale value is visible
        انتخاب (nextRand() % 5) {
        حالت 0: {  // Insert a whole entry
            ثابت std::pair<typename Map::iterator, منطقی> pair = map.درج({کلید, val});
            UASSERT_SELFTEST(pair.second, model.درج({کلید, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, کلید);  // At the key asked for
            بشکن;
        }
        حالت 1: {  // Insert lazily, which must create only on a miss
            منطقی created = نادرست;
            ثابت std::pair<typename Map::iterator, منطقی> pair
                = map.درج_Lazy(کلید, [&]() -> Entry {
                      created = درست;
                      بازگشت {کلید, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.درج({کلید, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, کلید);  // At the key asked for
            بشکن;
        }
        حالت 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(کلید), model.erase(کلید) != 0);  // As model
            بشکن;
        }
        حالت 3: {  // Change a value, which is erasing the entry and inserting it again
            ثابت typename Map::iterator it = map.find(کلید);
            اگر (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.درج({کلید, val}).second, درست);  // The key is free
                model[کلید] = val;
            }
            بشکن;
        }
        پیشفرض: {  // Look up only
            ثابت typename Map::iterator it = map.find(کلید);
            UASSERT_SELFTEST(it != map.end(), model.شمار(کلید) != 0);  // As the model
            اگر (it != map.end()) {
                UASSERT_SELFTEST(it->first, کلید);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(کلید));  // With its value
            }
            بشکن;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    برای (size_t کلید = 0; کلید < UNIVERSE; ++کلید) {
        ثابت typename Map::iterator it = map.find(کلید);
        UASSERT_SELFTEST(it != map.end(), model.شمار(کلید) != 0);  // Present iff modelled
        اگر (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(کلید));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<منطقی, UNIVERSE> seen{};
    size_t n = 0;
    برای (ثابت Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, درست);  // Nothing out of thin air
        UASSERT_SELFTEST(model.شمار(entry.first) != 0, درست);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], نادرست);  // Each entry once
        seen[entry.first] = درست;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

تهی selfTest() {
    test_مقدار_کلیدها();
    test_Pointer_کلیدها();
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
