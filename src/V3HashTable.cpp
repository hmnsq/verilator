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

#அந்தர்பாவ "V3HashTable.h"

#அந்தர்பாவ "V3Error.h"

#அந்தர்பாவ <array>
#அந்தர்பாவ <functional>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <string>
#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <utility>

நாமாகாஶ V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr size_t maxLoad(size_t capacity) { திரும்பு capacity * LOAD_FACTOR_இலக்கம் / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr size_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

வெறுமை test_மதிப்பு_சாவிகள்() {
    கட்டமைப்பு மதிப்பு_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    கட்டமைப்பு Hash final {
        size_t operator()(மாறிலி மதிப்பு_2& மதிப்பு) மாறிலி { திரும்பு operator()(மதிப்பு.m_hash, 0); }
        size_t operator()(size_t hash, size_t) மாறிலி { திரும்பு hash; }
    };

    கட்டமைப்பு Equal final {
        தருக்க operator()(மாறிலி மதிப்பு_2& a, மாறிலி மதிப்பு_2& b) மாறிலி {
            திரும்பு operator()(a, b.m_hash, b.m_id);
        }
        தருக்க operator()(மாறிலி மதிப்பு_2& மதிப்பு, size_t hash, size_t id) மாறிலி {
            திரும்பு மதிப்பு.m_hash == hash && மதிப்பு.m_id == id;
        }
    };

    using Set = V3HashSet<மதிப்பு_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        மாறிலி மதிப்பு_2 மதிப்பு{1, 0};
        மாறிலி மதிப்பு_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), உண்மை);  // So iterates nothing
        UASSERT_SELFTEST(set.find(மதிப்பு) == set.end(), உண்மை);  // And finds nothing
        மாறிலி std::pair<Set::iterator, தருக்க> added = set.செருகு(மதிப்பு);
        UASSERT_SELFTEST(added.second, உண்மை);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, மதிப்பு.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &மதிப்பு, உண்மை);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), பொய்);  // So the set is no longer empty
        {
            மாறிலி Set::iterator it = set.find(மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            மாறிலி Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            மாறிலி Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), உண்மை);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), உண்மை);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        மாறிலி std::pair<Set::iterator, தருக்க> again = set.செருகு(equal);
        UASSERT_SELFTEST(again.second, பொய்);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            மாறிலி Set::iterator it = set.find(மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Empty again
        UASSERT_SELFTEST(set.find(மதிப்பு) == set.end(), உண்மை);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(மதிப்பு), பொய்);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(மதிப்பு), பொய்);  // Nothing to erase, so says so
        set.செருகு(மதிப்பு);
        UASSERT_SELFTEST(set.erase(equal), உண்மை);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.செருகு(மதிப்பு);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), உண்மை);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        க்கு (size_t i = 0; i < N; ++i) {
            மாறிலி std::pair<Set::iterator, தருக்க> added = set.செருகு(மதிப்பு_2{7, i});
            UASSERT_SELFTEST(added.second, உண்மை);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        க்கு (size_t i = 0; i < N; ++i) {
            மாறிலி Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<தருக்க, N> seen{};
        size_t n = 0;
        க்கு (மாறிலி மதிப்பு_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], பொய்);  // Each of them exactly once
            seen[entry.m_id] = உண்மை;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

வெறுமை test_Pointer_சாவிகள்() {
    கட்டமைப்பு மதிப்பு_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    கட்டமைப்பு Hash final {
        size_t operator()(மாறிலி மதிப்பு_2* valuep) மாறிலி { திரும்பு operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) மாறிலி { திரும்பு hash; }
    };

    கட்டமைப்பு Equal final {
        தருக்க operator()(மாறிலி மதிப்பு_2* ap, மாறிலி மதிப்பு_2* bp) மாறிலி {
            திரும்பு operator()(ap, bp->m_hash, bp->m_id);
        }
        தருக்க operator()(மாறிலி மதிப்பு_2* valuep, size_t hash, size_t id) மாறிலி {
            திரும்பு valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<மதிப்பு_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        மதிப்பு_2 மதிப்பு{1, 0};
        மதிப்பு_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), உண்மை);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&மதிப்பு) == set.end(), உண்மை);  // And finds nothing
        மாறிலி std::pair<Set::iterator, தருக்க> added = set.செருகு(&மதிப்பு);
        UASSERT_SELFTEST(added.second, உண்மை);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &மதிப்பு);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), பொய்);  // So the set is no longer empty
        {
            மாறிலி Set::iterator it = set.find(&மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Found by itself
            UASSERT_SELFTEST(*it, &மதிப்பு);  // At the entry held
        }
        {
            மாறிலி Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // And by an equal entry
            UASSERT_SELFTEST(*it, &மதிப்பு);  // At that same entry
        }
        {
            மாறிலி Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // And by its parts
            UASSERT_SELFTEST(*it, &மதிப்பு);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), உண்மை);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), உண்மை);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        மாறிலி std::pair<Set::iterator, தருக்க> again = set.செருகு(&equal);
        UASSERT_SELFTEST(again.second, பொய்);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &மதிப்பு);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            மாறிலி Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &மதிப்பு);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, பொய்);  // Not the one asked for
        }
        {
            மாறிலி Set::iterator it = set.find(&மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // The entry is still there
            UASSERT_SELFTEST(*it, &மதிப்பு);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            மாறிலி Set::iterator it = set.find(&மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Present before the erase
            UASSERT_SELFTEST(*it, &மதிப்பு);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Empty again
        UASSERT_SELFTEST(set.find(&மதிப்பு) == set.end(), உண்மை);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&மதிப்பு), பொய்);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), உண்மை);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&மதிப்பு), பொய்);  // Nothing to erase, so says so
        set.செருகு(&மதிப்பு);
        UASSERT_SELFTEST(set.erase(&equal), உண்மை);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&மதிப்பு) == set.end(), உண்மை);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.செருகு(&மதிப்பு);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), உண்மை);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    க்கு (size_t erase = 0; erase < 4; ++erase) {
        க்கு (மாறிலி size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<மதிப்பு_2, 4> மதிப்புகள்{மதிப்பு_2{hash, 0}, மதிப்பு_2{hash, 1},  //
                                        மதிப்பு_2{hash, 2}, மதிப்பு_2{hash, 3}};
            Set set;
            க்கு (மதிப்பு_2& மதிப்பு : மதிப்புகள்) set.செருகு(&மதிப்பு);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                மாறிலி Set::iterator it = set.find(&மதிப்புகள்[erase]);
                UASSERT_SELFTEST(it != set.end(), உண்மை);  // The one to erase is present
                UASSERT_SELFTEST(*it, &மதிப்புகள்[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&மதிப்புகள்[erase]) == set.end(), உண்மை);  // That one
            க்கு (size_t i = 0; i < மதிப்புகள்.size(); ++i) {
                எனில் (i == erase) தொடர்;
                மாறிலி Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), உண்மை);  // The others are all there
                UASSERT_SELFTEST(*it, &மதிப்புகள்[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    க்கு (மாறிலி size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<மதிப்பு_2, GROWS_TWICE> many{};
        Set set;
        க்கு (size_t i = 0; i < many.size(); ++i) {
            many[i] = மதிப்பு_2{hash, i};
            set.செருகு(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        க்கு (size_t i = 0; i < many.size(); ++i) {
            மாறிலி Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        மதிப்பு_2 மதிப்பு{1, 0};
        மதிப்பு_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.செருகு(&மதிப்பு);
        set.செருகு(&other);
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            மாறிலி Set::iterator it = set.find(&மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // The first of the run is there
            UASSERT_SELFTEST(*it, &மதிப்பு);  // And is that entry
        }
        {
            மாறிலி Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            மாறிலி Set::iterator it = set.find(&மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Present before the erase
            UASSERT_SELFTEST(*it, &மதிப்பு);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&மதிப்பு) == set.end(), உண்மை);  // Namely that one
        {
            மாறிலி Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    க்கு (மாறிலி தருக்க doReserve : {பொய், உண்மை}) {
        std::array<மதிப்பு_2, 8> மதிப்புகள்{};
        Set set;
        எனில் (doReserve) set.reserve(மதிப்புகள்.size());
        க்கு (size_t i = 0; i < மதிப்புகள்.size(); ++i) {
            மதிப்புகள்[i] = மதிப்பு_2{static_cast<size_t>(i * 1234567), i};
            set.செருகு(&மதிப்புகள்[i]);
        }
        க்கு (மதிப்பு_2& மதிப்பு : மதிப்புகள்) {
            மாறிலி Set::iterator it = set.find(&மதிப்பு);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Each entry survives
            UASSERT_SELFTEST(*it, &மதிப்பு);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), மதிப்புகள்.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<மதிப்பு_2, N> மதிப்புகள்{};
        Set set;
        க்கு (size_t i = 0; i < N; ++i) {
            மதிப்புகள்[i] = மதிப்பு_2{7, i};  // All of them hash the same
            மாறிலி std::pair<Set::iterator, தருக்க> pair
                = set.செருகு_Lazy(size_t{7}, i, [&]() -> மதிப்பு_2* { திரும்பு &மதிப்புகள்[i]; });
            UASSERT_SELFTEST(pair.second, உண்மை);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &மதிப்புகள்[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        க்கு (size_t i = 0; i < N; ++i) {
            தருக்க created = பொய்;
            மாறிலி std::pair<Set::iterator, தருக்க> pair
                = set.செருகு_Lazy(size_t{7}, i, [&]() -> மதிப்பு_2* {
                      created = உண்மை;  // LCOV_EXCL_START
                      திரும்பு &மதிப்புகள்[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, பொய்);  // The factory was never called
            UASSERT_SELFTEST(pair.second, பொய்);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &மதிப்புகள்[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<மதிப்பு_2, 20> மதிப்புகள்{};
        std::array<தருக்க, 20> seen{};
        Set set;
        க்கு (size_t i = 0; i < மதிப்புகள்.size(); ++i) {
            மதிப்புகள்[i] = மதிப்பு_2{i / 2, i};  // Pairs of them collide
            set.செருகு(&மதிப்புகள்[i]);
        }
        size_t n = 0;
        க்கு (மதிப்பு_2* மாறிலி valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), உண்மை);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], பொய்);  // Each of them exactly once
            seen[valuep->m_id] = உண்மை;
            ++n;
        }
        UASSERT_SELFTEST(n, மதிப்புகள்.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

வெறுமை testEntryLifetime() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    வகுப்பு NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    பொது:
        NoDefault(size_t* alivep, size_t hash, size_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(மாறிலி NoDefault& that)
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
        NoDefault& operator=(மாறிலி NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() மாறிலி { திரும்பு m_hash; }
        size_t id() மாறிலி { திரும்பு m_id; }
        தருக்க operator==(மாறிலி NoDefault& that) மாறிலி { திரும்பு m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::மதிப்பு,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::மதிப்பு,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::மதிப்பு,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::மதிப்பு,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    கட்டமைப்பு Hash final {
        size_t operator()(மாறிலி NoDefault& மதிப்பு) மாறிலி { திரும்பு மதிப்பு.hash(); }
        size_t operator()(size_t hash, size_t) மாறிலி { திரும்பு hash; }
    };

    கட்டமைப்பு Equal final {
        தருக்க operator()(மாறிலி NoDefault& a, மாறிலி NoDefault& b) மாறிலி {
            திரும்பு operator()(a, b.hash(), b.id());
        }
        தருக்க operator()(மாறிலி NoDefault& a, size_t, size_t id) மாறிலி { திரும்பு a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        க்கு (size_t i = 0; i < N; ++i) {
            மாறிலி std::pair<Set::iterator, தருக்க> added = set.செருகு(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, உண்மை);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        க்கு (size_t i = 0; i < N; ++i) {
            மாறிலி Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            மாறிலி std::pair<Set::iterator, தருக்க> dup = set.செருகு(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, பொய்);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            மாறிலி Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), பொய்);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        க்கு (size_t i = 1; i < N; ++i) {
            {
                மாறிலி Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), உண்மை);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            க்கு (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), உண்மை);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), உண்மை);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        க்கு (size_t i = 0; i < N; ++i) set.செருகு(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), உண்மை);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), பொய்);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        க்கு (size_t i = 0; i < N; ++i) set.செருகு(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), உண்மை);  // Still empty
        set.செருகு(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

வெறுமை testMove() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    கட்டமைப்பு Counted final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_id;  // Entries with equal ids are equal

        Counted(size_t* alivep, size_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(மாறிலி Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(மாறிலி Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    கட்டமைப்பு Hash final {
        size_t operator()(மாறிலி Counted& entry) மாறிலி { திரும்பு operator()(entry.m_id); }
        size_t operator()(size_t id) மாறிலி { திரும்பு id; }
    };

    கட்டமைப்பு Equal final {
        தருக்க operator()(மாறிலி Counted& a, மாறிலி Counted& b) மாறிலி { திரும்பு operator()(a, b.m_id); }
        தருக்க operator()(மாறிலி Counted& a, size_t id) மாறிலி { திரும்பு a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        க்கு (size_t i = 0; i < N; ++i) set.செருகு(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), உண்மை);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), உண்மை);  // So it iterates nothing
        க்கு (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), உண்மை);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), பொய்);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.செருகு(Counted{&alive, N}).second, உண்மை);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), பொய்);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), பொய்);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), உண்மை);  // And the source is empty again
        க்கு (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), உண்மை);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

வெறுமை testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    வகுப்பு MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    பொது:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = இயல்பு;
        MoveOnly(மாறிலி MoveOnly&) = delete;
        MoveOnly& operator=(மாறிலி MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = இயல்பு;

        size_t id() மாறிலி { திரும்பு m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::மதிப்பு,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::மதிப்பு,
                  "SelfTest: 'MoveOnly' must be move constructible");

    கட்டமைப்பு Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(மாறிலி MoveOnly& மதிப்பு) மாறிலி { திரும்பு operator()(மதிப்பு.id()); }
        size_t operator()(size_t) மாறிலி { திரும்பு 7; }
    };

    கட்டமைப்பு Equal final {
        தருக்க operator()(மாறிலி MoveOnly& a, மாறிலி MoveOnly& b) மாறிலி {
            திரும்பு operator()(a, b.id());
        }
        தருக்க operator()(மாறிலி MoveOnly& a, size_t id) மாறிலி { திரும்பு a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி std::pair<Set::iterator, தருக்க> added
            = set.செருகு_Lazy(i, [i] { திரும்பு MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, உண்மை);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    க்கு (size_t i = 0; i < N; i += 2) {
        மாறிலி Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), உண்மை);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி தருக்க erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

வெறுமை testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    வகுப்பு Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    பொது:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t மதிப்பு) மாறிலி { திரும்பு மதிப்பு & m_mask; }
    };

    வகுப்பு Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    பொது:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        தருக்க operator()(size_t a, size_t b) மாறிலி { திரும்பு (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::மதிப்பு,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::மதிப்பு,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    க்கு (size_t i = 0; i < 16; ++i) set.செருகு(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    க்கு (size_t i = 0; i < 16; ++i) {
        மாறிலி V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), உண்மை);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

வெறுமை testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    கட்டமைப்பு மதிப்பு_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Entries with equal ids are equal
    };

    கட்டமைப்பு Hash final {
        size_t operator()(மாறிலி மதிப்பு_2* valuep) மாறிலி { திரும்பு operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) மாறிலி { திரும்பு hash; }
    };

    கட்டமைப்பு Equal final {
        தருக்க operator()(மாறிலி மதிப்பு_2* ap, மாறிலி மதிப்பு_2* bp) மாறிலி {
            திரும்பு operator()(ap, bp->m_hash, bp->m_id);
        }
        தருக்க operator()(மாறிலி மதிப்பு_2* valuep, size_t hash, size_t id) மாறிலி {
            திரும்பு valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<மதிப்பு_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    க்கு (மாறிலி size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<மதிப்பு_2, 4> மதிப்புகள்{மதிப்பு_2{h, 0}, மதிப்பு_2{h + 1, 1},  //
                                    மதிப்பு_2{h, 2}, மதிப்பு_2{h + 1, 3}};
        Set set;
        க்கு (மதிப்பு_2& மதிப்பு : மதிப்புகள்) set.செருகு(&மதிப்பு);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            மாறிலி Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), பொய்);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        க்கு (size_t i = 1; i < மதிப்புகள்.size(); ++i) {
            மாறிலி Set::iterator it = set.find(மதிப்புகள்[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &மதிப்புகள்[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            மாறிலி Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        க்கு (size_t i = 2; i < மதிப்புகள்.size(); ++i) {
            மாறிலி Set::iterator it = set.find(மதிப்புகள்[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), உண்மை);  // Both still reachable
            UASSERT_SELFTEST(*it, &மதிப்புகள்[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<தருக்க, 4> seen{};
        க்கு (மாறிலி மதிப்பு_2* மாறிலி valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], பொய்);  // Each entry once
            seen[valuep->m_id] = உண்மை;
        }
        UASSERT_SELFTEST(seen[0], பொய்);  // Erased first
        UASSERT_SELFTEST(seen[1], பொய்);  // Erased second
        UASSERT_SELFTEST(seen[2], உண்மை);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], உண்மை);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

வெறுமை testReferenceStability() {
    கட்டமைப்பு மதிப்பு_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    கட்டமைப்பு Hash final {
        size_t operator()(மாறிலி மதிப்பு_2* valuep) மாறிலி { திரும்பு operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) மாறிலி { திரும்பு hash; }
    };

    கட்டமைப்பு Equal final {
        தருக்க operator()(மாறிலி மதிப்பு_2* ap, மாறிலி மதிப்பு_2* bp) மாறிலி {
            திரும்பு operator()(ap, bp->m_hash, bp->m_id);
        }
        தருக்க operator()(மாறிலி மதிப்பு_2* valuep, size_t hash, size_t id) மாறிலி {
            திரும்பு valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<மதிப்பு_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), பொய்);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), உண்மை);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), உண்மை);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<மதிப்பு_2, N> மதிப்புகள்{};
    std::array<மதிப்பு_2* மாறிலி*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    க்கு (size_t i = 0; i < N; ++i) {
        மதிப்புகள்[i] = மதிப்பு_2{7, i};
        மாறிலி std::pair<Set::iterator, தருக்க> pair = set.செருகு(&மதிப்புகள்[i]);
        UASSERT_SELFTEST(pair.second, உண்மை);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), உண்மை);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி std::pair<Set::iterator, தருக்க> pair = set.செருகு(&மதிப்புகள்[i]);
        UASSERT_SELFTEST(pair.second, பொய்);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<மதிப்பு_2* மாறிலி*, N> grownpps{};  // Where each entry is after the growth
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), உண்மை);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), உண்மை);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
கட்டமைப்பு ClusteredHash final {
    size_t operator()(size_t மதிப்பு) மாறிலி { திரும்பு மதிப்பு & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
கட்டமைப்பு SpreadHash final {
    size_t operator()(size_t மதிப்பு) மாறிலி {
        திரும்பு static_cast<size_t>(மதிப்பு * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
வெறுமை testSetAgainstModel() {
    கட்டமைப்பு Equal final {
        தருக்க operator()(size_t a, size_t b) மாறிலி { திரும்பு a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    மாறிலி auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        திரும்பு static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    க்கு (size_t step = 0; step < STEPS; ++step) {
        மாறிலி size_t id = nextRand() % UNIVERSE;
        தேர்வு (nextRand() % 4) {
        நிலை 0: {  // Insert a copy
            மாறிலி std::pair<typename Set::iterator, தருக்க> pair = set.செருகு(id);
            UASSERT_SELFTEST(pair.second, model.செருகு(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            நிறுத்து;
        }
        நிலை 1: {  // Insert lazily, which must create only on a miss
            தருக்க created = பொய்;
            மாறிலி std::pair<typename Set::iterator, தருக்க> pair
                = set.செருகு_Lazy(id, [&]() -> size_t {
                      created = உண்மை;
                      திரும்பு id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.செருகு(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            நிறுத்து;
        }
        நிலை 2: {  // Erase, if present
            மாறிலி typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.மொத்தம்(id) != 0);  // As the model
            எனில் (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            நிறுத்து;
        }
        இயல்பு: {  // Look up only
            மாறிலி typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.மொத்தம்(id) != 0);  // As the model
            எனில் (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            நிறுத்து;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    க்கு (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.மொத்தம்(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<தருக்க, UNIVERSE> seen{};
    size_t n = 0;
    க்கு (மாறிலி size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, உண்மை);  // Nothing out of thin air
        UASSERT_SELFTEST(model.மொத்தம்(entry) != 0, உண்மை);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], பொய்);  // Each entry once
        seen[entry] = உண்மை;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

வெறுமை testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, முழு>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, முழு>>::மதிப்பு,
                  "SelfTest: a map entry must be a plain pair");

    constexpr size_t N = GROWS_TWICE;
    Map map;
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி std::string சாவி = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        மாறிலி std::pair<Map::iterator, தருக்க> pair
            = map.செருகு_Lazy(சாவி, [&]() -> Entry { திரும்பு {சாவி, static_cast<முழு>(i)}; });
        UASSERT_SELFTEST(pair.second, உண்மை);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), உண்மை);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<முழு>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி std::string சாவி = "key" + std::to_string(i);
        map.erase(map.find(சாவி));
        UASSERT_SELFTEST(map.செருகு({சாவி, static_cast<முழு>(i) + 100}).second, உண்மை);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), உண்மை);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<முழு>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    க்கு (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).மதிப்பு() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    க்கு (size_t i = 0; i < N; ++i) {
        மாறிலி Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), உண்மை);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<முழு>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    க்கு (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).மதிப்பு() -= 100;

    // Adding a key that is present changes nothing
    {
        மாறிலி std::string சாவி = "key0";
        தருக்க created = பொய்;
        மாறிலி std::pair<Map::iterator, தருக்க> pair = map.செருகு_Lazy(சாவி, [&]() -> Entry {
            created = உண்மை;  // LCOV_EXCL_START
            திரும்பு {சாவி, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, பொய்);  // The factory was never called
        UASSERT_SELFTEST(pair.second, பொய்);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        மாறிலி std::pair<Map::iterator, தருக்க> added = map.செருகு({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, உண்மை);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        மாறிலி std::pair<Map::iterator, தருக்க> again = map.செருகு({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, பொய்);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        மாறிலி Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), உண்மை);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), பொய்);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), உண்மை);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), பொய்);  // Namely that one
    க்கு (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), உண்மை);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
வெறுமை testMapAgainstModel() {
    கட்டமைப்பு Equal final {
        தருக்க operator()(size_t a, size_t b) மாறிலி { திரும்பு a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    மாறிலி auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        திரும்பு static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    க்கு (size_t step = 0; step < STEPS; ++step) {
        மாறிலி size_t சாவி = nextRand() % UNIVERSE;
        மாறிலி size_t val = step;  // Distinct every step, so a stale value is visible
        தேர்வு (nextRand() % 5) {
        நிலை 0: {  // Insert a whole entry
            மாறிலி std::pair<typename Map::iterator, தருக்க> pair = map.செருகு({சாவி, val});
            UASSERT_SELFTEST(pair.second, model.செருகு({சாவி, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, சாவி);  // At the key asked for
            நிறுத்து;
        }
        நிலை 1: {  // Insert lazily, which must create only on a miss
            தருக்க created = பொய்;
            மாறிலி std::pair<typename Map::iterator, தருக்க> pair
                = map.செருகு_Lazy(சாவி, [&]() -> Entry {
                      created = உண்மை;
                      திரும்பு {சாவி, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.செருகு({சாவி, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, சாவி);  // At the key asked for
            நிறுத்து;
        }
        நிலை 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(சாவி), model.erase(சாவி) != 0);  // As model
            நிறுத்து;
        }
        நிலை 3: {  // Change a value, which is erasing the entry and inserting it again
            மாறிலி typename Map::iterator it = map.find(சாவி);
            எனில் (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.செருகு({சாவி, val}).second, உண்மை);  // The key is free
                model[சாவி] = val;
            }
            நிறுத்து;
        }
        இயல்பு: {  // Look up only
            மாறிலி typename Map::iterator it = map.find(சாவி);
            UASSERT_SELFTEST(it != map.end(), model.மொத்தம்(சாவி) != 0);  // As the model
            எனில் (it != map.end()) {
                UASSERT_SELFTEST(it->first, சாவி);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(சாவி));  // With its value
            }
            நிறுத்து;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    க்கு (size_t சாவி = 0; சாவி < UNIVERSE; ++சாவி) {
        மாறிலி typename Map::iterator it = map.find(சாவி);
        UASSERT_SELFTEST(it != map.end(), model.மொத்தம்(சாவி) != 0);  // Present iff modelled
        எனில் (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(சாவி));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<தருக்க, UNIVERSE> seen{};
    size_t n = 0;
    க்கு (மாறிலி Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, உண்மை);  // Nothing out of thin air
        UASSERT_SELFTEST(model.மொத்தம்(entry.first) != 0, உண்மை);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], பொய்);  // Each entry once
        seen[entry.first] = உண்மை;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

வெறுமை selfTest() {
    test_மதிப்பு_சாவிகள்();
    test_Pointer_சாவிகள்();
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
