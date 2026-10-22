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

#അന്തര്ഭാവ "V3HashTable.h"

#അന്തര്ഭാവ "V3Error.h"

#അന്തര്ഭാവ <array>
#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <type_traits>
#അന്തര്ഭാവ <utility>

നാമാകാശ V3HashTableInternals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr size_t maxLoad(size_t capacity) { തിരികെ capacity * LOAD_FACTOR_അക്കം / LOAD_FACTOR_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr size_t GROWS_TWICE = maxLoad(4 * MIN_CAPACITY);
static_assert(GROWS_TWICE > maxLoad(2 * MIN_CAPACITY),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

ശൂന്യം test_മൂല്യം_താക്കോലുകൾ() {
    ഘടന മൂല്യം_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ഘടന Hash final {
        size_t operator()(സ്ഥിരം മൂല്യം_2& മൂല്യം) സ്ഥിരം { തിരികെ operator()(മൂല്യം.m_hash, 0); }
        size_t operator()(size_t hash, size_t) സ്ഥിരം { തിരികെ hash; }
    };

    ഘടന Equal final {
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2& a, സ്ഥിരം മൂല്യം_2& b) സ്ഥിരം {
            തിരികെ operator()(a, b.m_hash, b.m_id);
        }
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2& മൂല്യം, size_t hash, size_t id) സ്ഥിരം {
            തിരികെ മൂല്യം.m_hash == hash && മൂല്യം.m_id == id;
        }
    };

    using Set = V3HashSet<മൂല്യം_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        സ്ഥിരം മൂല്യം_2 മൂല്യം{1, 0};
        സ്ഥിരം മൂല്യം_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), സത്യം);  // So iterates nothing
        UASSERT_SELFTEST(set.find(മൂല്യം) == set.end(), സത്യം);  // And finds nothing
        സ്ഥിരം std::pair<Set::iterator, തർക്കം> added = set.തിരുകുക(മൂല്യം);
        UASSERT_SELFTEST(added.second, സത്യം);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, മൂല്യം.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &മൂല്യം, സത്യം);  // Which is a copy of the argument
        UASSERT_SELFTEST(set.size(), 1);  // And is the only one
        UASSERT_SELFTEST(set.empty(), അസത്യം);  // So the set is no longer empty
        {
            സ്ഥിരം Set::iterator it = set.find(മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            സ്ഥിരം Set::iterator it = set.find(equal);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            സ്ഥിരം Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), സത്യം);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), സത്യം);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        സ്ഥിരം std::pair<Set::iterator, തർക്കം> again = set.തിരുകുക(equal);
        UASSERT_SELFTEST(again.second, അസത്യം);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            സ്ഥിരം Set::iterator it = set.find(മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Empty again
        UASSERT_SELFTEST(set.find(മൂല്യം) == set.end(), സത്യം);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(മൂല്യം), അസത്യം);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(set.erase(മൂല്യം), അസത്യം);  // Nothing to erase, so says so
        set.തിരുകുക(മൂല്യം);
        UASSERT_SELFTEST(set.erase(equal), സത്യം);  // An equal key erases the entry held
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        set.തിരുകുക(മൂല്യം);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), സത്യം);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr size_t N = GROWS_TWICE;
        Set set;
        വേണ്ടി (size_t i = 0; i < N; ++i) {
            സ്ഥിരം std::pair<Set::iterator, തർക്കം> added = set.തിരുകുക(മൂല്യം_2{7, i});
            UASSERT_SELFTEST(added.second, സത്യം);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them, in the one probe run
        വേണ്ടി (size_t i = 0; i < N; ++i) {
            സ്ഥിരം Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<തർക്കം, N> seen{};
        size_t n = 0;
        വേണ്ടി (സ്ഥിരം മൂല്യം_2& entry : set) {
            UASSERT_SELFTEST(entry.m_hash, size_t{7});  // Only entries added
            UASSERT_SELFTEST(seen[entry.m_id], അസത്യം);  // Each of them exactly once
            seen[entry.m_id] = സത്യം;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

ശൂന്യം test_Pointer_താക്കോലുകൾ() {
    ഘടന മൂല്യം_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ഘടന Hash final {
        size_t operator()(സ്ഥിരം മൂല്യം_2* valuep) സ്ഥിരം { തിരികെ operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) സ്ഥിരം { തിരികെ hash; }
    };

    ഘടന Equal final {
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2* ap, സ്ഥിരം മൂല്യം_2* bp) സ്ഥിരം {
            തിരികെ operator()(ap, bp->m_hash, bp->m_id);
        }
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2* valuep, size_t hash, size_t id) സ്ഥിരം {
            തിരികെ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<മൂല്യം_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        മൂല്യം_2 മൂല്യം{1, 0};
        മൂല്യം_2 equal{1, 0};  // Equal to 'value', and hashes the same
        Set set;
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Starts empty
        UASSERT_SELFTEST(set.begin() == set.end(), സത്യം);  // So iterates nothing
        UASSERT_SELFTEST(set.find(&മൂല്യം) == set.end(), സത്യം);  // And finds nothing
        സ്ഥിരം std::pair<Set::iterator, തർക്കം> added = set.തിരുകുക(&മൂല്യം);
        UASSERT_SELFTEST(added.second, സത്യം);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &മൂല്യം);  // The iterator is at the new entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is the only one
        UASSERT_SELFTEST(set.empty(), അസത്യം);  // So the set is no longer empty
        {
            സ്ഥിരം Set::iterator it = set.find(&മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Found by itself
            UASSERT_SELFTEST(*it, &മൂല്യം);  // At the entry held
        }
        {
            സ്ഥിരം Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // And by an equal entry
            UASSERT_SELFTEST(*it, &മൂല്യം);  // At that same entry
        }
        {
            സ്ഥിരം Set::iterator it = set.find(1, size_t{0});
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // And by its parts
            UASSERT_SELFTEST(*it, &മൂല്യം);  // At that same entry again
        }
        UASSERT_SELFTEST(set.find(1, size_t{1}) == set.end(), സത്യം);  // Hash same, id not
        UASSERT_SELFTEST(set.find(2, size_t{0}) == set.end(), സത്യം);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        സ്ഥിരം std::pair<Set::iterator, തർക്കം> again = set.തിരുകുക(&equal);
        UASSERT_SELFTEST(again.second, അസത്യം);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &മൂല്യം);  // The iterator is at the stored one
        UASSERT_SELFTEST(set.size(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            സ്ഥിരം Set::iterator it = set.find(&equal);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &മൂല്യം);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, അസത്യം);  // Not the one asked for
        }
        {
            സ്ഥിരം Set::iterator it = set.find(&മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // The entry is still there
            UASSERT_SELFTEST(*it, &മൂല്യം);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            സ്ഥിരം Set::iterator it = set.find(&മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Present before the erase
            UASSERT_SELFTEST(*it, &മൂല്യം);  // And is the object asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Empty again
        UASSERT_SELFTEST(set.find(&മൂല്യം) == set.end(), സത്യം);  // So the entry is gone
        UASSERT_SELFTEST(set.contains(&മൂല്യം), അസത്യം);  // As 'contains' agrees
        UASSERT_SELFTEST(set.empty(), സത്യം);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(set.erase(&മൂല്യം), അസത്യം);  // Nothing to erase, so says so
        set.തിരുകുക(&മൂല്യം);
        UASSERT_SELFTEST(set.erase(&equal), സത്യം);  // The equal key erases the stored one
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Which leaves the set empty
        UASSERT_SELFTEST(set.find(&മൂല്യം) == set.end(), സത്യം);  // And unreachable
        // And by a key spelled as the parts of an entry
        set.തിരുകുക(&മൂല്യം);
        UASSERT_SELFTEST(set.erase(1, size_t{0}), സത്യം);  // Those parts find and erase it
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    വേണ്ടി (size_t erase = 0; erase < 4; ++erase) {
        വേണ്ടി (സ്ഥിരം size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {  // Wraps too
            std::array<മൂല്യം_2, 4> മൂല്യങ്ങൾ{മൂല്യം_2{hash, 0}, മൂല്യം_2{hash, 1},  //
                                        മൂല്യം_2{hash, 2}, മൂല്യം_2{hash, 3}};
            Set set;
            വേണ്ടി (മൂല്യം_2& മൂല്യം : മൂല്യങ്ങൾ) set.തിരുകുക(&മൂല്യം);
            UASSERT_SELFTEST(set.size(), 4);  // All distinct, so all added
            {
                സ്ഥിരം Set::iterator it = set.find(&മൂല്യങ്ങൾ[erase]);
                UASSERT_SELFTEST(it != set.end(), സത്യം);  // The one to erase is present
                UASSERT_SELFTEST(*it, &മൂല്യങ്ങൾ[erase]);  // And is the object held
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(set.find(&മൂല്യങ്ങൾ[erase]) == set.end(), സത്യം);  // That one
            വേണ്ടി (size_t i = 0; i < മൂല്യങ്ങൾ.size(); ++i) {
                എങ്കിൽ (i == erase) തുടരുക;
                സ്ഥിരം Set::iterator it = set.find(hash, i);
                UASSERT_SELFTEST(it != set.end(), സത്യം);  // The others are all there
                UASSERT_SELFTEST(*it, &മൂല്യങ്ങൾ[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    വേണ്ടി (സ്ഥിരം size_t hash : {size_t{0}, size_t{7}, ~size_t{0}}) {
        std::array<മൂല്യം_2, GROWS_TWICE> many{};
        Set set;
        വേണ്ടി (size_t i = 0; i < many.size(); ++i) {
            many[i] = മൂല്യം_2{hash, i};
            set.തിരുകുക(&many[i]);
        }
        UASSERT_SELFTEST(set.size(), many.size());  // All of them were added
        വേണ്ടി (size_t i = 0; i < many.size(); ++i) {
            സ്ഥിരം Set::iterator it = set.find(hash, i);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        മൂല്യം_2 മൂല്യം{1, 0};
        മൂല്യം_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        Set set;
        set.reserve(2);
        set.തിരുകുക(&മൂല്യം);
        set.തിരുകുക(&other);
        UASSERT_SELFTEST(set.size(), 2);  // Both kept, despite the collision
        {
            സ്ഥിരം Set::iterator it = set.find(&മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // The first of the run is there
            UASSERT_SELFTEST(*it, &മൂല്യം);  // And is that entry
        }
        {
            സ്ഥിരം Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            സ്ഥിരം Set::iterator it = set.find(&മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Present before the erase
            UASSERT_SELFTEST(*it, &മൂല്യം);  // And is the object held
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 1);  // Only one was erased
        UASSERT_SELFTEST(set.find(&മൂല്യം) == set.end(), സത്യം);  // Namely that one
        {
            സ്ഥിരം Set::iterator it = set.find(&other);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    വേണ്ടി (സ്ഥിരം തർക്കം doReserve : {അസത്യം, സത്യം}) {
        std::array<മൂല്യം_2, 8> മൂല്യങ്ങൾ{};
        Set set;
        എങ്കിൽ (doReserve) set.reserve(മൂല്യങ്ങൾ.size());
        വേണ്ടി (size_t i = 0; i < മൂല്യങ്ങൾ.size(); ++i) {
            മൂല്യങ്ങൾ[i] = മൂല്യം_2{static_cast<size_t>(i * 1234567), i};
            set.തിരുകുക(&മൂല്യങ്ങൾ[i]);
        }
        വേണ്ടി (മൂല്യം_2& മൂല്യം : മൂല്യങ്ങൾ) {
            സ്ഥിരം Set::iterator it = set.find(&മൂല്യം);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Each entry survives
            UASSERT_SELFTEST(*it, &മൂല്യം);  // And is the one inserted
        }
        UASSERT_SELFTEST(set.size(), മൂല്യങ്ങൾ.size());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr size_t N = GROWS_TWICE;
        std::array<മൂല്യം_2, N> മൂല്യങ്ങൾ{};
        Set set;
        വേണ്ടി (size_t i = 0; i < N; ++i) {
            മൂല്യങ്ങൾ[i] = മൂല്യം_2{7, i};  // All of them hash the same
            സ്ഥിരം std::pair<Set::iterator, തർക്കം> pair
                = set.തിരുകുക_Lazy(size_t{7}, i, [&]() -> മൂല്യം_2* { തിരികെ &മൂല്യങ്ങൾ[i]; });
            UASSERT_SELFTEST(pair.second, സത്യം);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &മൂല്യങ്ങൾ[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them were added
        // They are all present now, so nothing is created
        വേണ്ടി (size_t i = 0; i < N; ++i) {
            തർക്കം created = അസത്യം;
            സ്ഥിരം std::pair<Set::iterator, തർക്കം> pair
                = set.തിരുകുക_Lazy(size_t{7}, i, [&]() -> മൂല്യം_2* {
                      created = സത്യം;  // LCOV_EXCL_START
                      തിരികെ &മൂല്യങ്ങൾ[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, അസത്യം);  // The factory was never called
            UASSERT_SELFTEST(pair.second, അസത്യം);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &മൂല്യങ്ങൾ[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(set.size(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<മൂല്യം_2, 20> മൂല്യങ്ങൾ{};
        std::array<തർക്കം, 20> seen{};
        Set set;
        വേണ്ടി (size_t i = 0; i < മൂല്യങ്ങൾ.size(); ++i) {
            മൂല്യങ്ങൾ[i] = മൂല്യം_2{i / 2, i};  // Pairs of them collide
            set.തിരുകുക(&മൂല്യങ്ങൾ[i]);
        }
        size_t n = 0;
        വേണ്ടി (മൂല്യം_2* സ്ഥിരം valuep : set) {
            UASSERT_SELFTEST(valuep->m_id < seen.size(), സത്യം);  // Only entries added
            UASSERT_SELFTEST(seen[valuep->m_id], അസത്യം);  // Each of them exactly once
            seen[valuep->m_id] = സത്യം;
            ++n;
        }
        UASSERT_SELFTEST(n, മൂല്യങ്ങൾ.size());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

ശൂന്യം testEntryLifetime() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    വർഗ്ഗം NoDefault final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_hash;  // Hash of this entry
        size_t m_id;  // Entries with equal ids are equal

    പൊതു:
        NoDefault(size_t* alivep, size_t hash, size_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(സ്ഥിരം NoDefault& that)
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
        NoDefault& operator=(സ്ഥിരം NoDefault&) = delete;
        NoDefault& operator=(NoDefault&&) = delete;

        size_t hash() സ്ഥിരം { തിരികെ m_hash; }
        size_t id() സ്ഥിരം { തിരികെ m_id; }
        തർക്കം operator==(സ്ഥിരം NoDefault& that) സ്ഥിരം { തിരികെ m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::മൂല്യം,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::മൂല്യം,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::മൂല്യം,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::മൂല്യം,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    ഘടന Hash final {
        size_t operator()(സ്ഥിരം NoDefault& മൂല്യം) സ്ഥിരം { തിരികെ മൂല്യം.hash(); }
        size_t operator()(size_t hash, size_t) സ്ഥിരം { തിരികെ hash; }
    };

    ഘടന Equal final {
        തർക്കം operator()(സ്ഥിരം NoDefault& a, സ്ഥിരം NoDefault& b) സ്ഥിരം {
            തിരികെ operator()(a, b.hash(), b.id());
        }
        തർക്കം operator()(സ്ഥിരം NoDefault& a, size_t, size_t id) സ്ഥിരം { തിരികെ a.id() == id; }
    };

    using Set = V3HashSet<NoDefault, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        Set set;
        വേണ്ടി (size_t i = 0; i < N; ++i) {
            സ്ഥിരം std::pair<Set::iterator, തർക്കം> added = set.തിരുകുക(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, സത്യം);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        വേണ്ടി (size_t i = 0; i < N; ++i) {
            സ്ഥിരം Set::iterator it = set.find(size_t{7}, i);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            സ്ഥിരം std::pair<Set::iterator, തർക്കം> dup = set.തിരുകുക(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, അസത്യം);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(set.size(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            സ്ഥിരം Set::iterator it = set.find(size_t{7}, size_t{0});
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Present before the erase
            UASSERT_SELFTEST(it->id(), size_t{0});  // And is the entry asked for
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), അസത്യം);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        വേണ്ടി (size_t i = 1; i < N; ++i) {
            {
                സ്ഥിരം Set::iterator it = set.find(size_t{7}, i);
                UASSERT_SELFTEST(it != set.end(), സത്യം);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                set.erase(it);
            }
            UASSERT_SELFTEST(set.size(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            വേണ്ടി (size_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(set.contains(size_t{7}, j), സത്യം);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(set.begin() == set.end(), സത്യം);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        വേണ്ടി (size_t i = 0; i < N; ++i) set.തിരുകുക(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        set.clear();
        UASSERT_SELFTEST(set.size(), 0);  // Nothing left
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Says so
        UASSERT_SELFTEST(set.begin() == set.end(), സത്യം);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), അസത്യം);  // Nor finds any
        set.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        വേണ്ടി (size_t i = 0; i < N; ++i) set.തിരുകുക(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        Set set;
        set.clear();
        UASSERT_SELFTEST(set.empty(), സത്യം);  // Still empty
        set.തിരുകുക(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(set.size(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

ശൂന്യം testMove() {
    size_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    ഘടന Counted final {
        size_t* m_alivep;  // Where the live entries are counted
        size_t m_id;  // Entries with equal ids are equal

        Counted(size_t* alivep, size_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(സ്ഥിരം Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& operator=(സ്ഥിരം Counted&) = delete;
        Counted& operator=(Counted&&) = delete;
    };

    ഘടന Hash final {
        size_t operator()(സ്ഥിരം Counted& entry) സ്ഥിരം { തിരികെ operator()(entry.m_id); }
        size_t operator()(size_t id) സ്ഥിരം { തിരികെ id; }
    };

    ഘടന Equal final {
        തർക്കം operator()(സ്ഥിരം Counted& a, സ്ഥിരം Counted& b) സ്ഥിരം { തിരികെ operator()(a, b.m_id); }
        തർക്കം operator()(സ്ഥിരം Counted& a, size_t id) സ്ഥിരം { തിരികെ a.m_id == id; }
    };

    using Set = V3HashSet<Counted, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    {
        Set set;
        വേണ്ടി (size_t i = 0; i < N; ++i) set.തിരുകുക(Counted{&alive, i});
        UASSERT_SELFTEST(set.size(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        Set moved{std::move(set)};
        UASSERT_SELFTEST(moved.size(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(set.empty(), സത്യം);  // And the source no longer holds them
        UASSERT_SELFTEST(set.begin() == set.end(), സത്യം);  // So it iterates nothing
        വേണ്ടി (size_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.contains(i), സത്യം);  // Every entry came across
            UASSERT_SELFTEST(set.contains(i), അസത്യം);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(set.തിരുകുക(Counted{&alive, N}).second, സത്യം);  // It took an entry
        UASSERT_SELFTEST(set.size(), 1);  // Which is all it holds
        UASSERT_SELFTEST(set.empty(), അസത്യം);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        set = std::move(moved);
        UASSERT_SELFTEST(set.size(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(set.contains(N), അസത്യം);  // Namely that one
        UASSERT_SELFTEST(moved.empty(), സത്യം);  // And the source is empty again
        വേണ്ടി (size_t i = 0; i < N; ++i) UASSERT_SELFTEST(set.contains(i), സത്യം);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

ശൂന്യം testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    വർഗ്ഗം MoveOnly final {
        size_t m_id;  // Entries with equal ids are equal

    പൊതു:
        explicit MoveOnly(size_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = സ്ഥിരസ്ഥിതി;
        MoveOnly(സ്ഥിരം MoveOnly&) = delete;
        MoveOnly& operator=(സ്ഥിരം MoveOnly&) = delete;
        MoveOnly& operator=(MoveOnly&&) = delete;
        ~MoveOnly() = സ്ഥിരസ്ഥിതി;

        size_t id() സ്ഥിരം { തിരികെ m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::മൂല്യം,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::മൂല്യം,
                  "SelfTest: 'MoveOnly' must be move constructible");

    ഘടന Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        size_t operator()(സ്ഥിരം MoveOnly& മൂല്യം) സ്ഥിരം { തിരികെ operator()(മൂല്യം.id()); }
        size_t operator()(size_t) സ്ഥിരം { തിരികെ 7; }
    };

    ഘടന Equal final {
        തർക്കം operator()(സ്ഥിരം MoveOnly& a, സ്ഥിരം MoveOnly& b) സ്ഥിരം {
            തിരികെ operator()(a, b.id());
        }
        തർക്കം operator()(സ്ഥിരം MoveOnly& a, size_t id) സ്ഥിരം { തിരികെ a.id() == id; }
    };

    using Set = V3HashSet<MoveOnly, Hash, Equal>;

    constexpr size_t N = GROWS_TWICE;
    Set set;
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം std::pair<Set::iterator, തർക്കം> added
            = set.തിരുകുക_Lazy(i, [i] { തിരികെ MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, സത്യം);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(set.size(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    വേണ്ടി (size_t i = 0; i < N; i += 2) {
        സ്ഥിരം Set::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), സത്യം);  // Present before the erase
        set.erase(it);
    }
    UASSERT_SELFTEST(set.size(), N / 2);  // Half of them are gone
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം തർക്കം erased = (i % 2) == 0;
        UASSERT_SELFTEST(set.contains(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

ശൂന്യം testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    വർഗ്ഗം Hash final {
        size_t m_mask;  // Only these bits of an entry matter

    പൊതു:
        explicit Hash(size_t mask)
            : m_mask{mask} {}
        size_t operator()(size_t മൂല്യം) സ്ഥിരം { തിരികെ മൂല്യം & m_mask; }
    };

    വർഗ്ഗം Equal final {
        size_t m_mask;  // Only these bits of an entry matter

    പൊതു:
        explicit Equal(size_t mask)
            : m_mask{mask} {}
        തർക്കം operator()(size_t a, size_t b) സ്ഥിരം { തിരികെ (a & m_mask) == (b & m_mask); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::മൂല്യം,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::മൂല്യം,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3HashSet<size_t, Hash, Equal> set{Hash{3}, Equal{3}};
    വേണ്ടി (size_t i = 0; i < 16; ++i) set.തിരുകുക(i);
    UASSERT_SELFTEST(set.size(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    വേണ്ടി (size_t i = 0; i < 16; ++i) {
        സ്ഥിരം V3HashSet<size_t, Hash, Equal>::iterator it = set.find(i);
        UASSERT_SELFTEST(it != set.end(), സത്യം);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

ശൂന്യം testBackwardShiftDeletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    ഘടന മൂല്യം_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Entries with equal ids are equal
    };

    ഘടന Hash final {
        size_t operator()(സ്ഥിരം മൂല്യം_2* valuep) സ്ഥിരം { തിരികെ operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) സ്ഥിരം { തിരികെ hash; }
    };

    ഘടന Equal final {
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2* ap, സ്ഥിരം മൂല്യം_2* bp) സ്ഥിരം {
            തിരികെ operator()(ap, bp->m_hash, bp->m_id);
        }
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2* valuep, size_t hash, size_t id) സ്ഥിരം {
            തിരികെ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<മൂല്യം_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    വേണ്ടി (സ്ഥിരം size_t h : {size_t{0}, size_t{5}, ~size_t{0}}) {  // Wraps too
        std::array<മൂല്യം_2, 4> മൂല്യങ്ങൾ{മൂല്യം_2{h, 0}, മൂല്യം_2{h + 1, 1},  //
                                    മൂല്യം_2{h, 2}, മൂല്യം_2{h + 1, 3}};
        Set set;
        വേണ്ടി (മൂല്യം_2& മൂല്യം : മൂല്യങ്ങൾ) set.തിരുകുക(&മൂല്യം);
        UASSERT_SELFTEST(set.size(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            സ്ഥിരം Set::iterator it = set.find(h, size_t{0});
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Present before the erase
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 3);  // One fewer entry
        UASSERT_SELFTEST(set.contains(h, size_t{0}), അസത്യം);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        വേണ്ടി (size_t i = 1; i < മൂല്യങ്ങൾ.size(); ++i) {
            സ്ഥിരം Set::iterator it = set.find(മൂല്യങ്ങൾ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &മൂല്യങ്ങൾ[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            സ്ഥിരം Set::iterator it = set.find(h + 1, size_t{1});
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Left where the first erase found it
            set.erase(it);
        }
        UASSERT_SELFTEST(set.size(), 2);  // Two are left
        വേണ്ടി (size_t i = 2; i < മൂല്യങ്ങൾ.size(); ++i) {
            സ്ഥിരം Set::iterator it = set.find(മൂല്യങ്ങൾ[i].m_hash, i);
            UASSERT_SELFTEST(it != set.end(), സത്യം);  // Both still reachable
            UASSERT_SELFTEST(*it, &മൂല്യങ്ങൾ[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<തർക്കം, 4> seen{};
        വേണ്ടി (സ്ഥിരം മൂല്യം_2* സ്ഥിരം valuep : set) {
            UASSERT_SELFTEST(seen[valuep->m_id], അസത്യം);  // Each entry once
            seen[valuep->m_id] = സത്യം;
        }
        UASSERT_SELFTEST(seen[0], അസത്യം);  // Erased first
        UASSERT_SELFTEST(seen[1], അസത്യം);  // Erased second
        UASSERT_SELFTEST(seen[2], സത്യം);  // Moved back over the first hole
        UASSERT_SELFTEST(seen[3], സത്യം);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

ശൂന്യം testReferenceStability() {
    ഘടന മൂല്യം_2 final {
        size_t m_hash = 0;  // Hash of this entry
        size_t m_id = 0;  // Id of this entry
    };

    ഘടന Hash final {
        size_t operator()(സ്ഥിരം മൂല്യം_2* valuep) സ്ഥിരം { തിരികെ operator()(valuep->m_hash, 0); }
        size_t operator()(size_t hash, size_t) സ്ഥിരം { തിരികെ hash; }
    };

    ഘടന Equal final {
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2* ap, സ്ഥിരം മൂല്യം_2* bp) സ്ഥിരം {
            തിരികെ operator()(ap, bp->m_hash, bp->m_id);
        }
        തർക്കം operator()(സ്ഥിരം മൂല്യം_2* valuep, size_t hash, size_t id) സ്ഥിരം {
            തിരികെ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    using Set = V3HashSet<മൂല്യം_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        Set set;
        set.reserve(8);
        UASSERT_SELFTEST(set.contains(size_t{7}, size_t{0}), അസത്യം);  // Room, but no entry
        UASSERT_SELFTEST(set.begin() == set.end(), സത്യം);  // So iterates nothing
        UASSERT_SELFTEST(set.empty(), സത്യം);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr size_t N = maxLoad(2 * MIN_CAPACITY);
    static_assert(N > maxLoad(MIN_CAPACITY), "SelfTest: 'N' must need more than a new table");
    std::array<മൂല്യം_2, N> മൂല്യങ്ങൾ{};
    std::array<മൂല്യം_2* സ്ഥിരം*, N> entrypps{};  // Where each entry is stored, as inserted
    Set set;
    set.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        മൂല്യങ്ങൾ[i] = മൂല്യം_2{7, i};
        സ്ഥിരം std::pair<Set::iterator, തർക്കം> pair = set.തിരുകുക(&മൂല്യങ്ങൾ[i]);
        UASSERT_SELFTEST(pair.second, സത്യം);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), സത്യം);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം std::pair<Set::iterator, തർക്കം> pair = set.തിരുകുക(&മൂല്യങ്ങൾ[i]);
        UASSERT_SELFTEST(pair.second, അസത്യം);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(set.size(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    set.reserve(8 * N);
    UASSERT_SELFTEST(set.size(), N);  // Rehashing dropped nothing
    std::array<മൂല്യം_2* സ്ഥിരം*, N> grownpps{};  // Where each entry is after the growth
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), സത്യം);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    set.reserve(N);
    UASSERT_SELFTEST(set.size(), N);  // The smaller request changed nothing
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം Set::iterator it = set.find(size_t{7}, i);
        UASSERT_SELFTEST(it != set.end(), സത്യം);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
ഘടന ClusteredHash final {
    size_t operator()(size_t മൂല്യം) സ്ഥിരം { തിരികെ മൂല്യം & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
ഘടന SpreadHash final {
    size_t operator()(size_t മൂല്യം) സ്ഥിരം {
        തിരികെ static_cast<size_t>(മൂല്യം * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
template <typename T_Hash>
ശൂന്യം testSetAgainstModel() {
    ഘടന Equal final {
        തർക്കം operator()(size_t a, size_t b) സ്ഥിരം { തിരികെ a == b; }
    };

    using Set = V3HashSet<size_t, T_Hash, Equal>;

    constexpr size_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    സ്ഥിരം auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        തിരികെ static_cast<size_t>(state >> 32);
    };

    Set set;
    std::set<size_t> model;

    വേണ്ടി (size_t step = 0; step < STEPS; ++step) {
        സ്ഥിരം size_t id = nextRand() % UNIVERSE;
        തിരഞ്ഞെടുപ്പ് (nextRand() % 4) {
        അവസ്ഥ 0: {  // Insert a copy
            സ്ഥിരം std::pair<typename Set::iterator, തർക്കം> pair = set.തിരുകുക(id);
            UASSERT_SELFTEST(pair.second, model.തിരുകുക(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            നിർത്തുക;
        }
        അവസ്ഥ 1: {  // Insert lazily, which must create only on a miss
            തർക്കം created = അസത്യം;
            സ്ഥിരം std::pair<typename Set::iterator, തർക്കം> pair
                = set.തിരുകുക_Lazy(id, [&]() -> size_t {
                      created = സത്യം;
                      തിരികെ id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.തിരുകുക(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            നിർത്തുക;
        }
        അവസ്ഥ 2: {  // Erase, if present
            സ്ഥിരം typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.എണ്ണം(id) != 0);  // As the model
            എങ്കിൽ (it != set.end()) {
                set.erase(it);
                model.erase(id);
            }
            നിർത്തുക;
        }
        സ്ഥിരസ്ഥിതി: {  // Look up only
            സ്ഥിരം typename Set::iterator it = set.find(id);
            UASSERT_SELFTEST(it != set.end(), model.എണ്ണം(id) != 0);  // As the model
            എങ്കിൽ (it != set.end()) UASSERT_SELFTEST(*it, id);  // And is the entry
            നിർത്തുക;
        }
        }
        UASSERT_SELFTEST(set.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    വേണ്ടി (size_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(set.contains(id), model.എണ്ണം(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<തർക്കം, UNIVERSE> seen{};
    size_t n = 0;
    വേണ്ടി (സ്ഥിരം size_t entry : set) {
        UASSERT_SELFTEST(entry < UNIVERSE, സത്യം);  // Nothing out of thin air
        UASSERT_SELFTEST(model.എണ്ണം(entry) != 0, സത്യം);  // Only what was inserted
        UASSERT_SELFTEST(seen[entry], അസത്യം);  // Each entry once
        seen[entry] = സത്യം;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

ശൂന്യം testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    using Map = V3HashMap<std::string, പൂർണ്ണസംഖ്യ>;
    using Entry = Map::Entry;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<Entry, std::pair<std::string, പൂർണ്ണസംഖ്യ>>::മൂല്യം,
                  "SelfTest: a map entry must be a plain pair");

    constexpr size_t N = GROWS_TWICE;
    Map map;
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം std::string താക്കോൽ = "key" + std::to_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        സ്ഥിരം std::pair<Map::iterator, തർക്കം> pair
            = map.തിരുകുക_Lazy(താക്കോൽ, [&]() -> Entry { തിരികെ {താക്കോൽ, static_cast<പൂർണ്ണസംഖ്യ>(i)}; });
        UASSERT_SELFTEST(pair.second, സത്യം);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.size(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), സത്യം);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<പൂർണ്ണസംഖ്യ>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം std::string താക്കോൽ = "key" + std::to_string(i);
        map.erase(map.find(താക്കോൽ));
        UASSERT_SELFTEST(map.തിരുകുക({താക്കോൽ, static_cast<പൂർണ്ണസംഖ്യ>(i) + 100}).second, സത്യം);  // Anew
    }
    UASSERT_SELFTEST(map.size(), N);  // The same keys, so the map has not grown
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), സത്യം);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<പൂർണ്ണസംഖ്യ>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    വേണ്ടി (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).മൂല്യം() += 100;
    UASSERT_SELFTEST(map.size(), N);  // The map is unchanged otherwise
    വേണ്ടി (size_t i = 0; i < N; ++i) {
        സ്ഥിരം Map::iterator it = map.find("key" + std::to_string(i));
        UASSERT_SELFTEST(it != map.end(), സത്യം);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<പൂർണ്ണസംഖ്യ>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    വേണ്ടി (size_t i = 0; i < N; ++i) map.find("key" + std::to_string(i)).മൂല്യം() -= 100;

    // Adding a key that is present changes nothing
    {
        സ്ഥിരം std::string താക്കോൽ = "key0";
        തർക്കം created = അസത്യം;
        സ്ഥിരം std::pair<Map::iterator, തർക്കം> pair = map.തിരുകുക_Lazy(താക്കോൽ, [&]() -> Entry {
            created = സത്യം;  // LCOV_EXCL_START
            തിരികെ {താക്കോൽ, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, അസത്യം);  // The factory was never called
        UASSERT_SELFTEST(pair.second, അസത്യം);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.size(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        സ്ഥിരം std::pair<Map::iterator, തർക്കം> added = map.തിരുകുക({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, സത്യം);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        സ്ഥിരം std::pair<Map::iterator, തർക്കം> again = map.തിരുകുക({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, അസത്യം);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.size(), N + 1);  // Just the one entry was added
    {
        സ്ഥിരം Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.end(), സത്യം);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.size(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), അസത്യം);  // No such key
    UASSERT_SELFTEST(map.size(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), സത്യം);  // That key is present
    UASSERT_SELFTEST(map.size(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.contains(std::string{"key0"}), അസത്യം);  // Namely that one
    വേണ്ടി (size_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.contains("key" + std::to_string(i)), സത്യം);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
template <typename T_Hash>
ശൂന്യം testMapAgainstModel() {
    ഘടന Equal final {
        തർക്കം operator()(size_t a, size_t b) സ്ഥിരം { തിരികെ a == b; }
    };

    using Map = V3HashMap<size_t, size_t, T_Hash, Equal>;
    using Entry = typename Map::Entry;

    constexpr size_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr size_t STEPS = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    സ്ഥിരം auto nextRand = [&state]() -> size_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        തിരികെ static_cast<size_t>(state >> 32);
    };

    Map map;
    std::map<size_t, size_t> model;

    വേണ്ടി (size_t step = 0; step < STEPS; ++step) {
        സ്ഥിരം size_t താക്കോൽ = nextRand() % UNIVERSE;
        സ്ഥിരം size_t val = step;  // Distinct every step, so a stale value is visible
        തിരഞ്ഞെടുപ്പ് (nextRand() % 5) {
        അവസ്ഥ 0: {  // Insert a whole entry
            സ്ഥിരം std::pair<typename Map::iterator, തർക്കം> pair = map.തിരുകുക({താക്കോൽ, val});
            UASSERT_SELFTEST(pair.second, model.തിരുകുക({താക്കോൽ, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, താക്കോൽ);  // At the key asked for
            നിർത്തുക;
        }
        അവസ്ഥ 1: {  // Insert lazily, which must create only on a miss
            തർക്കം created = അസത്യം;
            സ്ഥിരം std::pair<typename Map::iterator, തർക്കം> pair
                = map.തിരുകുക_Lazy(താക്കോൽ, [&]() -> Entry {
                      created = സത്യം;
                      തിരികെ {താക്കോൽ, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.തിരുകുക({താക്കോൽ, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, താക്കോൽ);  // At the key asked for
            നിർത്തുക;
        }
        അവസ്ഥ 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(താക്കോൽ), model.erase(താക്കോൽ) != 0);  // As model
            നിർത്തുക;
        }
        അവസ്ഥ 3: {  // Change a value, which is erasing the entry and inserting it again
            സ്ഥിരം typename Map::iterator it = map.find(താക്കോൽ);
            എങ്കിൽ (it != map.end()) {
                map.erase(it);
                UASSERT_SELFTEST(map.തിരുകുക({താക്കോൽ, val}).second, സത്യം);  // The key is free
                model[താക്കോൽ] = val;
            }
            നിർത്തുക;
        }
        സ്ഥിരസ്ഥിതി: {  // Look up only
            സ്ഥിരം typename Map::iterator it = map.find(താക്കോൽ);
            UASSERT_SELFTEST(it != map.end(), model.എണ്ണം(താക്കോൽ) != 0);  // As the model
            എങ്കിൽ (it != map.end()) {
                UASSERT_SELFTEST(it->first, താക്കോൽ);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(താക്കോൽ));  // With its value
            }
            നിർത്തുക;
        }
        }
        UASSERT_SELFTEST(map.size(), model.size());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    വേണ്ടി (size_t താക്കോൽ = 0; താക്കോൽ < UNIVERSE; ++താക്കോൽ) {
        സ്ഥിരം typename Map::iterator it = map.find(താക്കോൽ);
        UASSERT_SELFTEST(it != map.end(), model.എണ്ണം(താക്കോൽ) != 0);  // Present iff modelled
        എങ്കിൽ (it != map.end()) {
            UASSERT_SELFTEST(it->second, model.at(താക്കോൽ));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<തർക്കം, UNIVERSE> seen{};
    size_t n = 0;
    വേണ്ടി (സ്ഥിരം Entry& entry : map) {
        UASSERT_SELFTEST(entry.first < UNIVERSE, സത്യം);  // Nothing out of thin air
        UASSERT_SELFTEST(model.എണ്ണം(entry.first) != 0, സത്യം);  // Only what was inserted
        UASSERT_SELFTEST(entry.second, model.at(entry.first));  // With the right value
        UASSERT_SELFTEST(seen[entry.first], അസത്യം);  // Each entry once
        seen[entry.first] = സത്യം;
        ++n;
    }
    UASSERT_SELFTEST(n, model.size());  // And all of them
}

ശൂന്യം selfTest() {
    test_മൂല്യം_താക്കോലുകൾ();
    test_Pointer_താക്കോലുകൾ();
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
