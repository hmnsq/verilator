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

नामाकाश V3_Hash_सारणी_Internals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr आकार_t max_आरोपय(आकार_t क्षमता) { प्रत्यागम क्षमता * आरोपय_गुणकः_अङ्कः / आरोपय_गुणकः_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr आकार_t GROWS_TWICE = max_आरोपय(4 * MIN_क्षमता);
static_assert(GROWS_TWICE > max_आरोपय(2 * MIN_क्षमता),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

शून्य test_मानम्_कुञ्चिकाः() {
    संरचना मानम्_2 final {
        आकार_t m_hash = 0;  // Hash of this entry
        आकार_t m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        आकार_t संक्रियकः()(ध्रुव मानम्_2& मानम्) ध्रुव { प्रत्यागम संक्रियकः()(मानम्.m_hash, 0); }
        आकार_t संक्रियकः()(आकार_t hash, आकार_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक संक्रियकः()(ध्रुव मानम्_2& a, ध्रुव मानम्_2& b) ध्रुव {
            प्रत्यागम संक्रियकः()(a, b.m_hash, b.m_id);
        }
        तार्किक संक्रियकः()(ध्रुव मानम्_2& मानम्, आकार_t hash, आकार_t id) ध्रुव {
            प्रत्यागम मानम्.m_hash == hash && मानम्.m_id == id;
        }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<मानम्_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ध्रुव मानम्_2 मानम्{1, 0};
        ध्रुव मानम्_2 equal{1, 0};  // Equal to 'value', and hashes the same
        समुच्चयः_2 समुच्चयः;
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Starts empty
        UASSERT_SELFTEST(समुच्चयः.begin() == समुच्चयः.अन्तः_2(), सत्य);  // So iterates nothing
        UASSERT_SELFTEST(समुच्चयः.find(मानम्) == समुच्चयः.अन्तः_2(), सत्य);  // And finds nothing
        ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> added = समुच्चयः.निवेशय(मानम्);
        UASSERT_SELFTEST(added.second, सत्य);  // Added, as it was absent
        UASSERT_SELFTEST(added.first->m_id, मानम्.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.first != &मानम्, सत्य);  // Which is a copy of the argument
        UASSERT_SELFTEST(समुच्चयः.आकार(), 1);  // And is the only one
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), असत्य);  // So the set is no longer empty
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.first);  // At the entry held
        }
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(equal);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry
        }
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(1, आकार_t{0});
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.first);  // At that same entry again
        }
        UASSERT_SELFTEST(समुच्चयः.find(1, आकार_t{1}) == समुच्चयः.अन्तः_2(), सत्य);  // Hash same, id not
        UASSERT_SELFTEST(समुच्चयः.find(2, आकार_t{0}) == समुच्चयः.अन्तः_2(), सत्य);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> again = समुच्चयः.निवेशय(equal);
        UASSERT_SELFTEST(again.second, असत्य);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.first, &*added.first);  // At the entry already held
        UASSERT_SELFTEST(समुच्चयः.आकार(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Present before the erase
            समुच्चयः.erase(it);
        }
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Empty again
        UASSERT_SELFTEST(समुच्चयः.find(मानम्) == समुच्चयः.अन्तः_2(), सत्य);  // So the entry is gone
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(मानम्), असत्य);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(समुच्चयः.erase(मानम्), असत्य);  // Nothing to erase, so says so
        समुच्चयः.निवेशय(मानम्);
        UASSERT_SELFTEST(समुच्चयः.erase(equal), सत्य);  // An equal key erases the entry held
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        समुच्चयः.निवेशय(मानम्);
        UASSERT_SELFTEST(समुच्चयः.erase(1, आकार_t{0}), सत्य);  // Those parts find and erase it
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr आकार_t N = GROWS_TWICE;
        समुच्चयः_2 समुच्चयः;
        प्रति (आकार_t i = 0; i < N; ++i) {
            ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> added = समुच्चयः.निवेशय(मानम्_2{7, i});
            UASSERT_SELFTEST(added.second, सत्य);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // All of them, in the one probe run
        प्रति (आकार_t i = 0; i < N; ++i) {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(आकार_t{7}, i);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<तार्किक, N> आलोकितम्{};
        आकार_t n = 0;
        प्रति (ध्रुव मानम्_2& प्रविष्टिः : समुच्चयः) {
            UASSERT_SELFTEST(प्रविष्टिः.m_hash, आकार_t{7});  // Only entries added
            UASSERT_SELFTEST(आलोकितम्[प्रविष्टिः.m_id], असत्य);  // Each of them exactly once
            आलोकितम्[प्रविष्टिः.m_id] = सत्य;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

शून्य test_Pointer_कुञ्चिकाः() {
    संरचना मानम्_2 final {
        आकार_t m_hash = 0;  // Hash of this entry
        आकार_t m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        आकार_t संक्रियकः()(ध्रुव मानम्_2* valuep) ध्रुव { प्रत्यागम संक्रियकः()(valuep->m_hash, 0); }
        आकार_t संक्रियकः()(आकार_t hash, आकार_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक संक्रियकः()(ध्रुव मानम्_2* ap, ध्रुव मानम्_2* bp) ध्रुव {
            प्रत्यागम संक्रियकः()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक संक्रियकः()(ध्रुव मानम्_2* valuep, आकार_t hash, आकार_t id) ध्रुव {
            प्रत्यागम valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<मानम्_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        मानम्_2 मानम्{1, 0};
        मानम्_2 equal{1, 0};  // Equal to 'value', and hashes the same
        समुच्चयः_2 समुच्चयः;
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Starts empty
        UASSERT_SELFTEST(समुच्चयः.begin() == समुच्चयः.अन्तः_2(), सत्य);  // So iterates nothing
        UASSERT_SELFTEST(समुच्चयः.find(&मानम्) == समुच्चयः.अन्तः_2(), सत्य);  // And finds nothing
        ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> added = समुच्चयः.निवेशय(&मानम्);
        UASSERT_SELFTEST(added.second, सत्य);  // Added, as it was absent
        UASSERT_SELFTEST(*added.first, &मानम्);  // The iterator is at the new entry
        UASSERT_SELFTEST(समुच्चयः.आकार(), 1);  // Which is the only one
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), असत्य);  // So the set is no longer empty
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Found by itself
            UASSERT_SELFTEST(*it, &मानम्);  // At the entry held
        }
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&equal);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // And by an equal entry
            UASSERT_SELFTEST(*it, &मानम्);  // At that same entry
        }
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(1, आकार_t{0});
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // And by its parts
            UASSERT_SELFTEST(*it, &मानम्);  // At that same entry again
        }
        UASSERT_SELFTEST(समुच्चयः.find(1, आकार_t{1}) == समुच्चयः.अन्तः_2(), सत्य);  // Hash same, id not
        UASSERT_SELFTEST(समुच्चयः.find(2, आकार_t{0}) == समुच्चयः.अन्तः_2(), सत्य);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> again = समुच्चयः.निवेशय(&equal);
        UASSERT_SELFTEST(again.second, असत्य);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.first, &मानम्);  // The iterator is at the stored one
        UASSERT_SELFTEST(समुच्चयः.आकार(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&equal);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &मानम्);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, असत्य);  // Not the one asked for
        }
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // The entry is still there
            UASSERT_SELFTEST(*it, &मानम्);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Present before the erase
            UASSERT_SELFTEST(*it, &मानम्);  // And is the object asked for
            समुच्चयः.erase(it);
        }
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Empty again
        UASSERT_SELFTEST(समुच्चयः.find(&मानम्) == समुच्चयः.अन्तः_2(), सत्य);  // So the entry is gone
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(&मानम्), असत्य);  // As 'contains' agrees
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(समुच्चयः.erase(&मानम्), असत्य);  // Nothing to erase, so says so
        समुच्चयः.निवेशय(&मानम्);
        UASSERT_SELFTEST(समुच्चयः.erase(&equal), सत्य);  // The equal key erases the stored one
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Which leaves the set empty
        UASSERT_SELFTEST(समुच्चयः.find(&मानम्) == समुच्चयः.अन्तः_2(), सत्य);  // And unreachable
        // And by a key spelled as the parts of an entry
        समुच्चयः.निवेशय(&मानम्);
        UASSERT_SELFTEST(समुच्चयः.erase(1, आकार_t{0}), सत्य);  // Those parts find and erase it
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    प्रति (आकार_t erase = 0; erase < 4; ++erase) {
        प्रति (ध्रुव आकार_t hash : {आकार_t{0}, आकार_t{7}, ~आकार_t{0}}) {  // Wraps too
            std::array<मानम्_2, 4> मानानि{मानम्_2{hash, 0}, मानम्_2{hash, 1},  //
                                        मानम्_2{hash, 2}, मानम्_2{hash, 3}};
            समुच्चयः_2 समुच्चयः;
            प्रति (मानम्_2& मानम् : मानानि) समुच्चयः.निवेशय(&मानम्);
            UASSERT_SELFTEST(समुच्चयः.आकार(), 4);  // All distinct, so all added
            {
                ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&मानानि[erase]);
                UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // The one to erase is present
                UASSERT_SELFTEST(*it, &मानानि[erase]);  // And is the object held
                समुच्चयः.erase(it);
            }
            UASSERT_SELFTEST(समुच्चयः.आकार(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(समुच्चयः.find(&मानानि[erase]) == समुच्चयः.अन्तः_2(), सत्य);  // That one
            प्रति (आकार_t i = 0; i < मानानि.आकार(); ++i) {
                यदि (i == erase) अनुवर्त;
                ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(hash, i);
                UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // The others are all there
                UASSERT_SELFTEST(*it, &मानानि[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    प्रति (ध्रुव आकार_t hash : {आकार_t{0}, आकार_t{7}, ~आकार_t{0}}) {
        std::array<मानम्_2, GROWS_TWICE> many{};
        समुच्चयः_2 समुच्चयः;
        प्रति (आकार_t i = 0; i < many.आकार(); ++i) {
            many[i] = मानम्_2{hash, i};
            समुच्चयः.निवेशय(&many[i]);
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), many.आकार());  // All of them were added
        प्रति (आकार_t i = 0; i < many.आकार(); ++i) {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(hash, i);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        मानम्_2 मानम्{1, 0};
        मानम्_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        समुच्चयः_2 समुच्चयः;
        समुच्चयः.reserve(2);
        समुच्चयः.निवेशय(&मानम्);
        समुच्चयः.निवेशय(&other);
        UASSERT_SELFTEST(समुच्चयः.आकार(), 2);  // Both kept, despite the collision
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // The first of the run is there
            UASSERT_SELFTEST(*it, &मानम्);  // And is that entry
        }
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&other);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Present before the erase
            UASSERT_SELFTEST(*it, &मानम्);  // And is the object held
            समुच्चयः.erase(it);
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), 1);  // Only one was erased
        UASSERT_SELFTEST(समुच्चयः.find(&मानम्) == समुच्चयः.अन्तः_2(), सत्य);  // Namely that one
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&other);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    प्रति (ध्रुव तार्किक doReserve : {असत्य, सत्य}) {
        std::array<मानम्_2, 8> मानानि{};
        समुच्चयः_2 समुच्चयः;
        यदि (doReserve) समुच्चयः.reserve(मानानि.आकार());
        प्रति (आकार_t i = 0; i < मानानि.आकार(); ++i) {
            मानानि[i] = मानम्_2{static_cast<आकार_t>(i * 1234567), i};
            समुच्चयः.निवेशय(&मानानि[i]);
        }
        प्रति (मानम्_2& मानम् : मानानि) {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(&मानम्);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Each entry survives
            UASSERT_SELFTEST(*it, &मानम्);  // And is the one inserted
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), मानानि.आकार());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr आकार_t N = GROWS_TWICE;
        std::array<मानम्_2, N> मानानि{};
        समुच्चयः_2 समुच्चयः;
        प्रति (आकार_t i = 0; i < N; ++i) {
            मानानि[i] = मानम्_2{7, i};  // All of them hash the same
            ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> pair
                = समुच्चयः.निवेशय_Lazy(आकार_t{7}, i, [&]() -> मानम्_2* { प्रत्यागम &मानानि[i]; });
            UASSERT_SELFTEST(pair.second, सत्य);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.first, &मानानि[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // All of them were added
        // They are all present now, so nothing is created
        प्रति (आकार_t i = 0; i < N; ++i) {
            तार्किक created = असत्य;
            ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> pair
                = समुच्चयः.निवेशय_Lazy(आकार_t{7}, i, [&]() -> मानम्_2* {
                      created = सत्य;  // LCOV_EXCL_START
                      प्रत्यागम &मानानि[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, असत्य);  // The factory was never called
            UASSERT_SELFTEST(pair.second, असत्य);  // As the lookup hit
            UASSERT_SELFTEST(*pair.first, &मानानि[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<मानम्_2, 20> मानानि{};
        std::array<तार्किक, 20> आलोकितम्{};
        समुच्चयः_2 समुच्चयः;
        प्रति (आकार_t i = 0; i < मानानि.आकार(); ++i) {
            मानानि[i] = मानम्_2{i / 2, i};  // Pairs of them collide
            समुच्चयः.निवेशय(&मानानि[i]);
        }
        आकार_t n = 0;
        प्रति (मानम्_2* ध्रुव valuep : समुच्चयः) {
            UASSERT_SELFTEST(valuep->m_id < आलोकितम्.आकार(), सत्य);  // Only entries added
            UASSERT_SELFTEST(आलोकितम्[valuep->m_id], असत्य);  // Each of them exactly once
            आलोकितम्[valuep->m_id] = सत्य;
            ++n;
        }
        UASSERT_SELFTEST(n, मानानि.आकार());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

शून्य test_प्रविष्टिः_Lifetime() {
    आकार_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    वर्ग NoDefault final {
        आकार_t* m_alivep;  // Where the live entries are counted
        आकार_t m_hash;  // Hash of this entry
        आकार_t m_id;  // Entries with equal ids are equal

    सार्वजनिक:
        NoDefault(आकार_t* alivep, आकार_t hash, आकार_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ध्रुव NoDefault& that)
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
        NoDefault& संक्रियकः=(ध्रुव NoDefault&) = विलोपय;
        NoDefault& संक्रियकः=(NoDefault&&) = विलोपय;

        आकार_t hash() ध्रुव { प्रत्यागम m_hash; }
        आकार_t id() ध्रुव { प्रत्यागम m_id; }
        तार्किक संक्रियकः==(ध्रुव NoDefault& that) ध्रुव { प्रत्यागम m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::मानम्,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    संरचना Hash final {
        आकार_t संक्रियकः()(ध्रुव NoDefault& मानम्) ध्रुव { प्रत्यागम मानम्.hash(); }
        आकार_t संक्रियकः()(आकार_t hash, आकार_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक संक्रियकः()(ध्रुव NoDefault& a, ध्रुव NoDefault& b) ध्रुव {
            प्रत्यागम संक्रियकः()(a, b.hash(), b.id());
        }
        तार्किक संक्रियकः()(ध्रुव NoDefault& a, आकार_t, आकार_t id) ध्रुव { प्रत्यागम a.id() == id; }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<NoDefault, Hash, Equal>;

    constexpr आकार_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        समुच्चयः_2 समुच्चयः;
        प्रति (आकार_t i = 0; i < N; ++i) {
            ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> added = समुच्चयः.निवेशय(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.second, सत्य);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        प्रति (आकार_t i = 0; i < N; ++i) {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(आकार_t{7}, i);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> dup = समुच्चयः.निवेशय(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.second, असत्य);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.first->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(आकार_t{7}, आकार_t{0});
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Present before the erase
            UASSERT_SELFTEST(it->id(), आकार_t{0});  // And is the entry asked for
            समुच्चयः.erase(it);
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(आकार_t{7}, आकार_t{0}), असत्य);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        प्रति (आकार_t i = 1; i < N; ++i) {
            {
                ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(आकार_t{7}, i);
                UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                समुच्चयः.erase(it);
            }
            UASSERT_SELFTEST(समुच्चयः.आकार(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            प्रति (आकार_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(आकार_t{7}, j), सत्य);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(समुच्चयः.begin() == समुच्चयः.अन्तः_2(), सत्य);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        प्रति (आकार_t i = 0; i < N; ++i) समुच्चयः.निवेशय(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        समुच्चयः.clear();
        UASSERT_SELFTEST(समुच्चयः.आकार(), 0);  // Nothing left
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Says so
        UASSERT_SELFTEST(समुच्चयः.begin() == समुच्चयः.अन्तः_2(), सत्य);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(आकार_t{7}, आकार_t{0}), असत्य);  // Nor finds any
        समुच्चयः.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        प्रति (आकार_t i = 0; i < N; ++i) समुच्चयः.निवेशय(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        समुच्चयः_2 समुच्चयः;
        समुच्चयः.clear();
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // Still empty
        समुच्चयः.निवेशय(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(समुच्चयः.आकार(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

शून्य testMove() {
    आकार_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    संरचना Counted final {
        आकार_t* m_alivep;  // Where the live entries are counted
        आकार_t m_id;  // Entries with equal ids are equal

        Counted(आकार_t* alivep, आकार_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ध्रुव Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& संक्रियकः=(ध्रुव Counted&) = विलोपय;
        Counted& संक्रियकः=(Counted&&) = विलोपय;
    };

    संरचना Hash final {
        आकार_t संक्रियकः()(ध्रुव Counted& प्रविष्टिः) ध्रुव { प्रत्यागम संक्रियकः()(प्रविष्टिः.m_id); }
        आकार_t संक्रियकः()(आकार_t id) ध्रुव { प्रत्यागम id; }
    };

    संरचना Equal final {
        तार्किक संक्रियकः()(ध्रुव Counted& a, ध्रुव Counted& b) ध्रुव { प्रत्यागम संक्रियकः()(a, b.m_id); }
        तार्किक संक्रियकः()(ध्रुव Counted& a, आकार_t id) ध्रुव { प्रत्यागम a.m_id == id; }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<Counted, Hash, Equal>;

    constexpr आकार_t N = GROWS_TWICE;
    {
        समुच्चयः_2 समुच्चयः;
        प्रति (आकार_t i = 0; i < N; ++i) समुच्चयः.निवेशय(Counted{&alive, i});
        UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        समुच्चयः_2 moved{std::move(समुच्चयः)};
        UASSERT_SELFTEST(moved.आकार(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // And the source no longer holds them
        UASSERT_SELFTEST(समुच्चयः.begin() == समुच्चयः.अन्तः_2(), सत्य);  // So it iterates nothing
        प्रति (आकार_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.अन्तर्भवति(i), सत्य);  // Every entry came across
            UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(i), असत्य);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(समुच्चयः.निवेशय(Counted{&alive, N}).second, सत्य);  // It took an entry
        UASSERT_SELFTEST(समुच्चयः.आकार(), 1);  // Which is all it holds
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), असत्य);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        समुच्चयः = std::move(moved);
        UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(N), असत्य);  // Namely that one
        UASSERT_SELFTEST(moved.रिक्तानि(), सत्य);  // And the source is empty again
        प्रति (आकार_t i = 0; i < N; ++i) UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(i), सत्य);  // The rest moved
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
        आकार_t m_id;  // Entries with equal ids are equal

    सार्वजनिक:
        स्पष्ट MoveOnly(आकार_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = स्वभाव;
        MoveOnly(ध्रुव MoveOnly&) = विलोपय;
        MoveOnly& संक्रियकः=(ध्रुव MoveOnly&) = विलोपय;
        MoveOnly& संक्रियकः=(MoveOnly&&) = विलोपय;
        ~MoveOnly() = स्वभाव;

        आकार_t id() ध्रुव { प्रत्यागम m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::मानम्,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::मानम्,
                  "SelfTest: 'MoveOnly' must be move constructible");

    संरचना Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        आकार_t संक्रियकः()(ध्रुव MoveOnly& मानम्) ध्रुव { प्रत्यागम संक्रियकः()(मानम्.id()); }
        आकार_t संक्रियकः()(आकार_t) ध्रुव { प्रत्यागम 7; }
    };

    संरचना Equal final {
        तार्किक संक्रियकः()(ध्रुव MoveOnly& a, ध्रुव MoveOnly& b) ध्रुव {
            प्रत्यागम संक्रियकः()(a, b.id());
        }
        तार्किक संक्रियकः()(ध्रुव MoveOnly& a, आकार_t id) ध्रुव { प्रत्यागम a.id() == id; }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<MoveOnly, Hash, Equal>;

    constexpr आकार_t N = GROWS_TWICE;
    समुच्चयः_2 समुच्चयः;
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> added
            = समुच्चयः.निवेशय_Lazy(i, [i] { प्रत्यागम MoveOnly{i}; });
        UASSERT_SELFTEST(added.second, सत्य);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.first->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    प्रति (आकार_t i = 0; i < N; i += 2) {
        ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(i);
        UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Present before the erase
        समुच्चयः.erase(it);
    }
    UASSERT_SELFTEST(समुच्चयः.आकार(), N / 2);  // Half of them are gone
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव तार्किक erased = (i % 2) == 0;
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

शून्य testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    वर्ग Hash final {
        आकार_t m_आवरणम्;  // Only these bits of an entry matter

    सार्वजनिक:
        स्पष्ट Hash(आकार_t आवरणम्)
            : m_आवरणम्{आवरणम्} {}
        आकार_t संक्रियकः()(आकार_t मानम्) ध्रुव { प्रत्यागम मानम् & m_आवरणम्; }
    };

    वर्ग Equal final {
        आकार_t m_आवरणम्;  // Only these bits of an entry matter

    सार्वजनिक:
        स्पष्ट Equal(आकार_t आवरणम्)
            : m_आवरणम्{आवरणम्} {}
        तार्किक संक्रियकः()(आकार_t a, आकार_t b) ध्रुव { प्रत्यागम (a & m_आवरणम्) == (b & m_आवरणम्); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::मानम्,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::मानम्,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_समुच्चयः<आकार_t, Hash, Equal> समुच्चयः{Hash{3}, Equal{3}};
    प्रति (आकार_t i = 0; i < 16; ++i) समुच्चयः.निवेशय(i);
    UASSERT_SELFTEST(समुच्चयः.आकार(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    प्रति (आकार_t i = 0; i < 16; ++i) {
        ध्रुव V3_Hash_समुच्चयः<आकार_t, Hash, Equal>::iterator it = समुच्चयः.find(i);
        UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

शून्य test_Backward_सरणम्_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    संरचना मानम्_2 final {
        आकार_t m_hash = 0;  // Hash of this entry
        आकार_t m_id = 0;  // Entries with equal ids are equal
    };

    संरचना Hash final {
        आकार_t संक्रियकः()(ध्रुव मानम्_2* valuep) ध्रुव { प्रत्यागम संक्रियकः()(valuep->m_hash, 0); }
        आकार_t संक्रियकः()(आकार_t hash, आकार_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक संक्रियकः()(ध्रुव मानम्_2* ap, ध्रुव मानम्_2* bp) ध्रुव {
            प्रत्यागम संक्रियकः()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक संक्रियकः()(ध्रुव मानम्_2* valuep, आकार_t hash, आकार_t id) ध्रुव {
            प्रत्यागम valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<मानम्_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    प्रति (ध्रुव आकार_t h : {आकार_t{0}, आकार_t{5}, ~आकार_t{0}}) {  // Wraps too
        std::array<मानम्_2, 4> मानानि{मानम्_2{h, 0}, मानम्_2{h + 1, 1},  //
                                    मानम्_2{h, 2}, मानम्_2{h + 1, 3}};
        समुच्चयः_2 समुच्चयः;
        प्रति (मानम्_2& मानम् : मानानि) समुच्चयः.निवेशय(&मानम्);
        UASSERT_SELFTEST(समुच्चयः.आकार(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(h, आकार_t{0});
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Present before the erase
            समुच्चयः.erase(it);
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), 3);  // One fewer entry
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(h, आकार_t{0}), असत्य);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        प्रति (आकार_t i = 1; i < मानानि.आकार(); ++i) {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(मानानि[i].m_hash, i);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &मानानि[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(h + 1, आकार_t{1});
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Left where the first erase found it
            समुच्चयः.erase(it);
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), 2);  // Two are left
        प्रति (आकार_t i = 2; i < मानानि.आकार(); ++i) {
            ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(मानानि[i].m_hash, i);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Both still reachable
            UASSERT_SELFTEST(*it, &मानानि[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<तार्किक, 4> आलोकितम्{};
        प्रति (ध्रुव मानम्_2* ध्रुव valuep : समुच्चयः) {
            UASSERT_SELFTEST(आलोकितम्[valuep->m_id], असत्य);  // Each entry once
            आलोकितम्[valuep->m_id] = सत्य;
        }
        UASSERT_SELFTEST(आलोकितम्[0], असत्य);  // Erased first
        UASSERT_SELFTEST(आलोकितम्[1], असत्य);  // Erased second
        UASSERT_SELFTEST(आलोकितम्[2], सत्य);  // Moved back over the first hole
        UASSERT_SELFTEST(आलोकितम्[3], सत्य);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

शून्य testReferenceStability() {
    संरचना मानम्_2 final {
        आकार_t m_hash = 0;  // Hash of this entry
        आकार_t m_id = 0;  // Id of this entry
    };

    संरचना Hash final {
        आकार_t संक्रियकः()(ध्रुव मानम्_2* valuep) ध्रुव { प्रत्यागम संक्रियकः()(valuep->m_hash, 0); }
        आकार_t संक्रियकः()(आकार_t hash, आकार_t) ध्रुव { प्रत्यागम hash; }
    };

    संरचना Equal final {
        तार्किक संक्रियकः()(ध्रुव मानम्_2* ap, ध्रुव मानम्_2* bp) ध्रुव {
            प्रत्यागम संक्रियकः()(ap, bp->m_hash, bp->m_id);
        }
        तार्किक संक्रियकः()(ध्रुव मानम्_2* valuep, आकार_t hash, आकार_t id) ध्रुव {
            प्रत्यागम valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<मानम्_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        समुच्चयः_2 समुच्चयः;
        समुच्चयः.reserve(8);
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(आकार_t{7}, आकार_t{0}), असत्य);  // Room, but no entry
        UASSERT_SELFTEST(समुच्चयः.begin() == समुच्चयः.अन्तः_2(), सत्य);  // So iterates nothing
        UASSERT_SELFTEST(समुच्चयः.रिक्तानि(), सत्य);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr आकार_t N = max_आरोपय(2 * MIN_क्षमता);
    static_assert(N > max_आरोपय(MIN_क्षमता), "SelfTest: 'N' must need more than a new table");
    std::array<मानम्_2, N> मानानि{};
    std::array<मानम्_2* ध्रुव*, N> entrypps{};  // Where each entry is stored, as inserted
    समुच्चयः_2 समुच्चयः;
    समुच्चयः.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    प्रति (आकार_t i = 0; i < N; ++i) {
        मानानि[i] = मानम्_2{7, i};
        ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> pair = समुच्चयः.निवेशय(&मानानि[i]);
        UASSERT_SELFTEST(pair.second, सत्य);  // Ids differ, so each is added
        entrypps[i] = &*pair.first;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(आकार_t{7}, i);
        UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव std::pair<समुच्चयः_2::iterator, तार्किक> pair = समुच्चयः.निवेशय(&मानानि[i]);
        UASSERT_SELFTEST(pair.second, असत्य);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.first, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    समुच्चयः.reserve(8 * N);
    UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // Rehashing dropped nothing
    std::array<मानम्_2* ध्रुव*, N> grownpps{};  // Where each entry is after the growth
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(आकार_t{7}, i);
        UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    समुच्चयः.reserve(N);
    UASSERT_SELFTEST(समुच्चयः.आकार(), N);  // The smaller request changed nothing
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव समुच्चयः_2::iterator it = समुच्चयः.find(आकार_t{7}, i);
        UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), सत्य);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
संरचना ClusteredHash final {
    आकार_t संक्रियकः()(आकार_t मानम्) ध्रुव { प्रत्यागम मानम् & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
संरचना प्रसारः_Hash final {
    आकार_t संक्रियकः()(आकार_t मानम्) ध्रुव {
        प्रत्यागम static_cast<आकार_t>(मानम् * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
प्रतिरूपम् <प्रकारनाम T_Hash>
शून्य test_समुच्चयः_Against_Model() {
    संरचना Equal final {
        तार्किक संक्रियकः()(आकार_t a, आकार_t b) ध्रुव { प्रत्यागम a == b; }
    };

    उपयुज्य समुच्चयः_2 = V3_Hash_समुच्चयः<आकार_t, T_Hash, Equal>;

    constexpr आकार_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr आकार_t चरणाः = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ध्रुव स्वचालित अग्र_Rand = [&state]() -> आकार_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        प्रत्यागम static_cast<आकार_t>(state >> 32);
    };

    समुच्चयः_2 समुच्चयः;
    std::समुच्चयः<आकार_t> model;

    प्रति (आकार_t चरणः = 0; चरणः < चरणाः; ++चरणः) {
        ध्रुव आकार_t id = अग्र_Rand() % UNIVERSE;
        विकल्प (अग्र_Rand() % 4) {
        पक्ष 0: {  // Insert a copy
            ध्रुव std::pair<प्रकारनाम समुच्चयः_2::iterator, तार्किक> pair = समुच्चयः.निवेशय(id);
            UASSERT_SELFTEST(pair.second, model.निवेशय(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            भङ्ग;
        }
        पक्ष 1: {  // Insert lazily, which must create only on a miss
            तार्किक created = असत्य;
            ध्रुव std::pair<प्रकारनाम समुच्चयः_2::iterator, तार्किक> pair
                = समुच्चयः.निवेशय_Lazy(id, [&]() -> आकार_t {
                      created = सत्य;
                      प्रत्यागम id;
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.निवेशय(id).second);  // As the model
            UASSERT_SELFTEST(*pair.first, id);  // At the entry asked for
            भङ्ग;
        }
        पक्ष 2: {  // Erase, if present
            ध्रुव प्रकारनाम समुच्चयः_2::iterator it = समुच्चयः.find(id);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), model.गणनम्(id) != 0);  // As the model
            यदि (it != समुच्चयः.अन्तः_2()) {
                समुच्चयः.erase(it);
                model.erase(id);
            }
            भङ्ग;
        }
        स्वभाव: {  // Look up only
            ध्रुव प्रकारनाम समुच्चयः_2::iterator it = समुच्चयः.find(id);
            UASSERT_SELFTEST(it != समुच्चयः.अन्तः_2(), model.गणनम्(id) != 0);  // As the model
            यदि (it != समुच्चयः.अन्तः_2()) UASSERT_SELFTEST(*it, id);  // And is the entry
            भङ्ग;
        }
        }
        UASSERT_SELFTEST(समुच्चयः.आकार(), model.आकार());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    प्रति (आकार_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(समुच्चयः.अन्तर्भवति(id), model.गणनम्(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<तार्किक, UNIVERSE> आलोकितम्{};
    आकार_t n = 0;
    प्रति (ध्रुव आकार_t प्रविष्टिः : समुच्चयः) {
        UASSERT_SELFTEST(प्रविष्टिः < UNIVERSE, सत्य);  // Nothing out of thin air
        UASSERT_SELFTEST(model.गणनम्(प्रविष्टिः) != 0, सत्य);  // Only what was inserted
        UASSERT_SELFTEST(आलोकितम्[प्रविष्टिः], असत्य);  // Each entry once
        आलोकितम्[प्रविष्टिः] = सत्य;
        ++n;
    }
    UASSERT_SELFTEST(n, model.आकार());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

शून्य testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    उपयुज्य Map = V3HashMap<std::string, पूर्णाङ्क>;
    उपयुज्य प्रविष्टिः_2 = Map::प्रविष्टिः_2;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<प्रविष्टिः_2, std::pair<std::string, पूर्णाङ्क>>::मानम्,
                  "SelfTest: a map entry must be a plain pair");

    constexpr आकार_t N = GROWS_TWICE;
    Map map;
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव std::string कुञ्चिका = "key" + std::गन्तव्यम्_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ध्रुव std::pair<Map::iterator, तार्किक> pair
            = map.निवेशय_Lazy(कुञ्चिका, [&]() -> प्रविष्टिः_2 { प्रत्यागम {कुञ्चिका, static_cast<पूर्णाङ्क>(i)}; });
        UASSERT_SELFTEST(pair.second, सत्य);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.आकार(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव Map::iterator it = map.find("key" + std::गन्तव्यम्_string(i));
        UASSERT_SELFTEST(it != map.अन्तः_2(), सत्य);  // The key alone finds it
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव std::string कुञ्चिका = "key" + std::गन्तव्यम्_string(i);
        map.erase(map.find(कुञ्चिका));
        UASSERT_SELFTEST(map.निवेशय({कुञ्चिका, static_cast<पूर्णाङ्क>(i) + 100}).second, सत्य);  // Anew
    }
    UASSERT_SELFTEST(map.आकार(), N);  // The same keys, so the map has not grown
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव Map::iterator it = map.find("key" + std::गन्तव्यम्_string(i));
        UASSERT_SELFTEST(it != map.अन्तः_2(), सत्य);  // Each key is still there
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    प्रति (आकार_t i = 0; i < N; ++i) map.find("key" + std::गन्तव्यम्_string(i)).मानम्() += 100;
    UASSERT_SELFTEST(map.आकार(), N);  // The map is unchanged otherwise
    प्रति (आकार_t i = 0; i < N; ++i) {
        ध्रुव Map::iterator it = map.find("key" + std::गन्तव्यम्_string(i));
        UASSERT_SELFTEST(it != map.अन्तः_2(), सत्य);  // Each key still finds its entry
        UASSERT_SELFTEST(it->second, static_cast<पूर्णाङ्क>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    प्रति (आकार_t i = 0; i < N; ++i) map.find("key" + std::गन्तव्यम्_string(i)).मानम्() -= 100;

    // Adding a key that is present changes nothing
    {
        ध्रुव std::string कुञ्चिका = "key0";
        तार्किक created = असत्य;
        ध्रुव std::pair<Map::iterator, तार्किक> pair = map.निवेशय_Lazy(कुञ्चिका, [&]() -> प्रविष्टिः_2 {
            created = सत्य;  // LCOV_EXCL_START
            प्रत्यागम {कुञ्चिका, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, असत्य);  // The factory was never called
        UASSERT_SELFTEST(pair.second, असत्य);  // As the key is present
        UASSERT_SELFTEST(pair.first->second, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.आकार(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ध्रुव std::pair<Map::iterator, तार्किक> added = map.निवेशय({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.second, सत्य);  // Added, as the key is new
        UASSERT_SELFTEST(added.first->second, 5);  // With the value given
        ध्रुव std::pair<Map::iterator, तार्किक> again = map.निवेशय({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.second, असत्य);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.first->second, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.आकार(), N + 1);  // Just the one entry was added
    {
        ध्रुव Map::iterator it = map.find(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.अन्तः_2(), सत्य);  // Present before the erase
        UASSERT_SELFTEST(it->second, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.आकार(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), असत्य);  // No such key
    UASSERT_SELFTEST(map.आकार(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), सत्य);  // That key is present
    UASSERT_SELFTEST(map.आकार(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.अन्तर्भवति(std::string{"key0"}), असत्य);  // Namely that one
    प्रति (आकार_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.अन्तर्भवति("key" + std::गन्तव्यम्_string(i)), सत्य);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
प्रतिरूपम् <प्रकारनाम T_Hash>
शून्य testMapAgainstModel() {
    संरचना Equal final {
        तार्किक संक्रियकः()(आकार_t a, आकार_t b) ध्रुव { प्रत्यागम a == b; }
    };

    उपयुज्य Map = V3HashMap<आकार_t, आकार_t, T_Hash, Equal>;
    उपयुज्य प्रविष्टिः_2 = प्रकारनाम Map::प्रविष्टिः_2;

    constexpr आकार_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr आकार_t चरणाः = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ध्रुव स्वचालित अग्र_Rand = [&state]() -> आकार_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        प्रत्यागम static_cast<आकार_t>(state >> 32);
    };

    Map map;
    std::map<आकार_t, आकार_t> model;

    प्रति (आकार_t चरणः = 0; चरणः < चरणाः; ++चरणः) {
        ध्रुव आकार_t कुञ्चिका = अग्र_Rand() % UNIVERSE;
        ध्रुव आकार_t val = चरणः;  // Distinct every step, so a stale value is visible
        विकल्प (अग्र_Rand() % 5) {
        पक्ष 0: {  // Insert a whole entry
            ध्रुव std::pair<प्रकारनाम Map::iterator, तार्किक> pair = map.निवेशय({कुञ्चिका, val});
            UASSERT_SELFTEST(pair.second, model.निवेशय({कुञ्चिका, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, कुञ्चिका);  // At the key asked for
            भङ्ग;
        }
        पक्ष 1: {  // Insert lazily, which must create only on a miss
            तार्किक created = असत्य;
            ध्रुव std::pair<प्रकारनाम Map::iterator, तार्किक> pair
                = map.निवेशय_Lazy(कुञ्चिका, [&]() -> प्रविष्टिः_2 {
                      created = सत्य;
                      प्रत्यागम {कुञ्चिका, val};
                  });
            UASSERT_SELFTEST(pair.second, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.second, model.निवेशय({कुञ्चिका, val}).second);  // Ditto
            UASSERT_SELFTEST(pair.first->first, कुञ्चिका);  // At the key asked for
            भङ्ग;
        }
        पक्ष 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(कुञ्चिका), model.erase(कुञ्चिका) != 0);  // As model
            भङ्ग;
        }
        पक्ष 3: {  // Change a value, which is erasing the entry and inserting it again
            ध्रुव प्रकारनाम Map::iterator it = map.find(कुञ्चिका);
            यदि (it != map.अन्तः_2()) {
                map.erase(it);
                UASSERT_SELFTEST(map.निवेशय({कुञ्चिका, val}).second, सत्य);  // The key is free
                model[कुञ्चिका] = val;
            }
            भङ्ग;
        }
        स्वभाव: {  // Look up only
            ध्रुव प्रकारनाम Map::iterator it = map.find(कुञ्चिका);
            UASSERT_SELFTEST(it != map.अन्तः_2(), model.गणनम्(कुञ्चिका) != 0);  // As the model
            यदि (it != map.अन्तः_2()) {
                UASSERT_SELFTEST(it->first, कुञ्चिका);  // At the key asked for
                UASSERT_SELFTEST(it->second, model.at(कुञ्चिका));  // With its value
            }
            भङ्ग;
        }
        }
        UASSERT_SELFTEST(map.आकार(), model.आकार());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    प्रति (आकार_t कुञ्चिका = 0; कुञ्चिका < UNIVERSE; ++कुञ्चिका) {
        ध्रुव प्रकारनाम Map::iterator it = map.find(कुञ्चिका);
        UASSERT_SELFTEST(it != map.अन्तः_2(), model.गणनम्(कुञ्चिका) != 0);  // Present iff modelled
        यदि (it != map.अन्तः_2()) {
            UASSERT_SELFTEST(it->second, model.at(कुञ्चिका));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<तार्किक, UNIVERSE> आलोकितम्{};
    आकार_t n = 0;
    प्रति (ध्रुव प्रविष्टिः_2& प्रविष्टिः : map) {
        UASSERT_SELFTEST(प्रविष्टिः.first < UNIVERSE, सत्य);  // Nothing out of thin air
        UASSERT_SELFTEST(model.गणनम्(प्रविष्टिः.first) != 0, सत्य);  // Only what was inserted
        UASSERT_SELFTEST(प्रविष्टिः.second, model.at(प्रविष्टिः.first));  // With the right value
        UASSERT_SELFTEST(आलोकितम्[प्रविष्टिः.first], असत्य);  // Each entry once
        आलोकितम्[प्रविष्टिः.first] = सत्य;
        ++n;
    }
    UASSERT_SELFTEST(n, model.आकार());  // And all of them
}

शून्य selfTest() {
    test_मानम्_कुञ्चिकाः();
    test_Pointer_कुञ्चिकाः();
    test_प्रविष्टिः_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_सरणम्_Deletion();
    testReferenceStability();
    test_समुच्चयः_Against_Model<ClusteredHash>();
    test_समुच्चयः_Against_Model<प्रसारः_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<प्रसारः_Hash>();
}

}  // namespace V3HashTableInternals
