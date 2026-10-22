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

#అన్తర్భావ "V3HashTable.h"

#అన్తర్భావ "V3Error.h"

#అన్తర్భావ <array>
#అన్తర్భావ <functional>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <string>
#అన్తర్భావ <type_traits>
#అన్తర్భావ <utility>

నామాకాశ V3_Hash_పట్టిక_Internals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr కొలత_t max_ఎక్కించు(కొలత_t సామర్థ్యం) { తిరిగి సామర్థ్యం * ఎక్కించు_గుణకం_అంకె / ఎక్కించు_గుణకం_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr కొలత_t GROWS_TWICE = max_ఎక్కించు(4 * MIN_సామర్థ్యం);
static_assert(GROWS_TWICE > max_ఎక్కించు(2 * MIN_సామర్థ్యం),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

శూన్య test_విలువ_కీలకాలు() {
    నిర్మాణం విలువ_2 final {
        కొలత_t m_hash = 0;  // Hash of this entry
        కొలత_t m_id = 0;  // Id of this entry
    };

    నిర్మాణం Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి విలువ_2& విలువ) స్థిరరాశి { తిరిగి క్రియాకారకం()(విలువ.m_hash, 0); }
        కొలత_t క్రియాకారకం()(కొలత_t hash, కొలత_t) స్థిరరాశి { తిరిగి hash; }
    };

    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2& a, స్థిరరాశి విలువ_2& b) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(a, b.m_hash, b.m_id);
        }
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2& విలువ, కొలత_t hash, కొలత_t id) స్థిరరాశి {
            తిరిగి విలువ.m_hash == hash && విలువ.m_id == id;
        }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<విలువ_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        స్థిరరాశి విలువ_2 విలువ{1, 0};
        స్థిరరాశి విలువ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        సమితి_2 సమితి;
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Starts empty
        UASSERT_SELFTEST(సమితి.begin() == సమితి.చివర(), నిజం);  // So iterates nothing
        UASSERT_SELFTEST(సమితి.కనుగొను(విలువ) == సమితి.చివర(), నిజం);  // And finds nothing
        స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> added = సమితి.చొప్పించు(విలువ);
        UASSERT_SELFTEST(added.రెండవ, నిజం);  // Added, as it was absent
        UASSERT_SELFTEST(added.మొదటి->m_id, విలువ.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.మొదటి != &విలువ, నిజం);  // Which is a copy of the argument
        UASSERT_SELFTEST(సమితి.కొలత(), 1);  // And is the only one
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), అబద్ధం);  // So the set is no longer empty
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.మొదటి);  // At the entry held
        }
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(equal);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.మొదటి);  // At that same entry
        }
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(1, కొలత_t{0});
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.మొదటి);  // At that same entry again
        }
        UASSERT_SELFTEST(సమితి.కనుగొను(1, కొలత_t{1}) == సమితి.చివర(), నిజం);  // Hash same, id not
        UASSERT_SELFTEST(సమితి.కనుగొను(2, కొలత_t{0}) == సమితి.చివర(), నిజం);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> again = సమితి.చొప్పించు(equal);
        UASSERT_SELFTEST(again.రెండవ, అబద్ధం);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.మొదటి, &*added.మొదటి);  // At the entry already held
        UASSERT_SELFTEST(సమితి.కొలత(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Present before the erase
            సమితి.erase(it);
        }
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Empty again
        UASSERT_SELFTEST(సమితి.కనుగొను(విలువ) == సమితి.చివర(), నిజం);  // So the entry is gone
        UASSERT_SELFTEST(సమితి.కలిగియుంది(విలువ), అబద్ధం);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(సమితి.erase(విలువ), అబద్ధం);  // Nothing to erase, so says so
        సమితి.చొప్పించు(విలువ);
        UASSERT_SELFTEST(సమితి.erase(equal), నిజం);  // An equal key erases the entry held
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        సమితి.చొప్పించు(విలువ);
        UASSERT_SELFTEST(సమితి.erase(1, కొలత_t{0}), నిజం);  // Those parts find and erase it
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr కొలత_t N = GROWS_TWICE;
        సమితి_2 సమితి;
        కొరకు (కొలత_t i = 0; i < N; ++i) {
            స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> added = సమితి.చొప్పించు(విలువ_2{7, i});
            UASSERT_SELFTEST(added.రెండవ, నిజం);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(సమితి.కొలత(), N);  // All of them, in the one probe run
        కొరకు (కొలత_t i = 0; i < N; ++i) {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(కొలత_t{7}, i);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<తార్కిక, N> చూసిన{};
        కొలత_t n = 0;
        కొరకు (స్థిరరాశి విలువ_2& నమోదు_2 : సమితి) {
            UASSERT_SELFTEST(నమోదు_2.m_hash, కొలత_t{7});  // Only entries added
            UASSERT_SELFTEST(చూసిన[నమోదు_2.m_id], అబద్ధం);  // Each of them exactly once
            చూసిన[నమోదు_2.m_id] = నిజం;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

శూన్య test_సూచిక_కీలకాలు() {
    నిర్మాణం విలువ_2 final {
        కొలత_t m_hash = 0;  // Hash of this entry
        కొలత_t m_id = 0;  // Id of this entry
    };

    నిర్మాణం Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి విలువ_2* valuep) స్థిరరాశి { తిరిగి క్రియాకారకం()(valuep->m_hash, 0); }
        కొలత_t క్రియాకారకం()(కొలత_t hash, కొలత_t) స్థిరరాశి { తిరిగి hash; }
    };

    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2* ap, స్థిరరాశి విలువ_2* bp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(ap, bp->m_hash, bp->m_id);
        }
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2* valuep, కొలత_t hash, కొలత_t id) స్థిరరాశి {
            తిరిగి valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<విలువ_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        విలువ_2 విలువ{1, 0};
        విలువ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        సమితి_2 సమితి;
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Starts empty
        UASSERT_SELFTEST(సమితి.begin() == సమితి.చివర(), నిజం);  // So iterates nothing
        UASSERT_SELFTEST(సమితి.కనుగొను(&విలువ) == సమితి.చివర(), నిజం);  // And finds nothing
        స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> added = సమితి.చొప్పించు(&విలువ);
        UASSERT_SELFTEST(added.రెండవ, నిజం);  // Added, as it was absent
        UASSERT_SELFTEST(*added.మొదటి, &విలువ);  // The iterator is at the new entry
        UASSERT_SELFTEST(సమితి.కొలత(), 1);  // Which is the only one
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), అబద్ధం);  // So the set is no longer empty
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Found by itself
            UASSERT_SELFTEST(*it, &విలువ);  // At the entry held
        }
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&equal);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // And by an equal entry
            UASSERT_SELFTEST(*it, &విలువ);  // At that same entry
        }
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(1, కొలత_t{0});
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // And by its parts
            UASSERT_SELFTEST(*it, &విలువ);  // At that same entry again
        }
        UASSERT_SELFTEST(సమితి.కనుగొను(1, కొలత_t{1}) == సమితి.చివర(), నిజం);  // Hash same, id not
        UASSERT_SELFTEST(సమితి.కనుగొను(2, కొలత_t{0}) == సమితి.చివర(), నిజం);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> again = సమితి.చొప్పించు(&equal);
        UASSERT_SELFTEST(again.రెండవ, అబద్ధం);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.మొదటి, &విలువ);  // The iterator is at the stored one
        UASSERT_SELFTEST(సమితి.కొలత(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&equal);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &విలువ);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, అబద్ధం);  // Not the one asked for
        }
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // The entry is still there
            UASSERT_SELFTEST(*it, &విలువ);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Present before the erase
            UASSERT_SELFTEST(*it, &విలువ);  // And is the object asked for
            సమితి.erase(it);
        }
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Empty again
        UASSERT_SELFTEST(సమితి.కనుగొను(&విలువ) == సమితి.చివర(), నిజం);  // So the entry is gone
        UASSERT_SELFTEST(సమితి.కలిగియుంది(&విలువ), అబద్ధం);  // As 'contains' agrees
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(సమితి.erase(&విలువ), అబద్ధం);  // Nothing to erase, so says so
        సమితి.చొప్పించు(&విలువ);
        UASSERT_SELFTEST(సమితి.erase(&equal), నిజం);  // The equal key erases the stored one
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Which leaves the set empty
        UASSERT_SELFTEST(సమితి.కనుగొను(&విలువ) == సమితి.చివర(), నిజం);  // And unreachable
        // And by a key spelled as the parts of an entry
        సమితి.చొప్పించు(&విలువ);
        UASSERT_SELFTEST(సమితి.erase(1, కొలత_t{0}), నిజం);  // Those parts find and erase it
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    కొరకు (కొలత_t erase = 0; erase < 4; ++erase) {
        కొరకు (స్థిరరాశి కొలత_t hash : {కొలత_t{0}, కొలత_t{7}, ~కొలత_t{0}}) {  // Wraps too
            std::array<విలువ_2, 4> విలువలు{విలువ_2{hash, 0}, విలువ_2{hash, 1},  //
                                        విలువ_2{hash, 2}, విలువ_2{hash, 3}};
            సమితి_2 సమితి;
            కొరకు (విలువ_2& విలువ : విలువలు) సమితి.చొప్పించు(&విలువ);
            UASSERT_SELFTEST(సమితి.కొలత(), 4);  // All distinct, so all added
            {
                స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&విలువలు[erase]);
                UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // The one to erase is present
                UASSERT_SELFTEST(*it, &విలువలు[erase]);  // And is the object held
                సమితి.erase(it);
            }
            UASSERT_SELFTEST(సమితి.కొలత(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(సమితి.కనుగొను(&విలువలు[erase]) == సమితి.చివర(), నిజం);  // That one
            కొరకు (కొలత_t i = 0; i < విలువలు.కొలత(); ++i) {
                ఒకవేళ (i == erase) కొనసాగు;
                స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(hash, i);
                UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // The others are all there
                UASSERT_SELFTEST(*it, &విలువలు[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    కొరకు (స్థిరరాశి కొలత_t hash : {కొలత_t{0}, కొలత_t{7}, ~కొలత_t{0}}) {
        std::array<విలువ_2, GROWS_TWICE> many{};
        సమితి_2 సమితి;
        కొరకు (కొలత_t i = 0; i < many.కొలత(); ++i) {
            many[i] = విలువ_2{hash, i};
            సమితి.చొప్పించు(&many[i]);
        }
        UASSERT_SELFTEST(సమితి.కొలత(), many.కొలత());  // All of them were added
        కొరకు (కొలత_t i = 0; i < many.కొలత(); ++i) {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(hash, i);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        విలువ_2 విలువ{1, 0};
        విలువ_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        సమితి_2 సమితి;
        సమితి.reserve(2);
        సమితి.చొప్పించు(&విలువ);
        సమితి.చొప్పించు(&other);
        UASSERT_SELFTEST(సమితి.కొలత(), 2);  // Both kept, despite the collision
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // The first of the run is there
            UASSERT_SELFTEST(*it, &విలువ);  // And is that entry
        }
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&other);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Present before the erase
            UASSERT_SELFTEST(*it, &విలువ);  // And is the object held
            సమితి.erase(it);
        }
        UASSERT_SELFTEST(సమితి.కొలత(), 1);  // Only one was erased
        UASSERT_SELFTEST(సమితి.కనుగొను(&విలువ) == సమితి.చివర(), నిజం);  // Namely that one
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&other);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    కొరకు (స్థిరరాశి తార్కిక doReserve : {అబద్ధం, నిజం}) {
        std::array<విలువ_2, 8> విలువలు{};
        సమితి_2 సమితి;
        ఒకవేళ (doReserve) సమితి.reserve(విలువలు.కొలత());
        కొరకు (కొలత_t i = 0; i < విలువలు.కొలత(); ++i) {
            విలువలు[i] = విలువ_2{static_cast<కొలత_t>(i * 1234567), i};
            సమితి.చొప్పించు(&విలువలు[i]);
        }
        కొరకు (విలువ_2& విలువ : విలువలు) {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(&విలువ);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Each entry survives
            UASSERT_SELFTEST(*it, &విలువ);  // And is the one inserted
        }
        UASSERT_SELFTEST(సమితి.కొలత(), విలువలు.కొలత());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr కొలత_t N = GROWS_TWICE;
        std::array<విలువ_2, N> విలువలు{};
        సమితి_2 సమితి;
        కొరకు (కొలత_t i = 0; i < N; ++i) {
            విలువలు[i] = విలువ_2{7, i};  // All of them hash the same
            స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> pair
                = సమితి.చొప్పించు_Lazy(కొలత_t{7}, i, [&]() -> విలువ_2* { తిరిగి &విలువలు[i]; });
            UASSERT_SELFTEST(pair.రెండవ, నిజం);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.మొదటి, &విలువలు[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(సమితి.కొలత(), N);  // All of them were added
        // They are all present now, so nothing is created
        కొరకు (కొలత_t i = 0; i < N; ++i) {
            తార్కిక created = అబద్ధం;
            స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> pair
                = సమితి.చొప్పించు_Lazy(కొలత_t{7}, i, [&]() -> విలువ_2* {
                      created = నిజం;  // LCOV_EXCL_START
                      తిరిగి &విలువలు[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, అబద్ధం);  // The factory was never called
            UASSERT_SELFTEST(pair.రెండవ, అబద్ధం);  // As the lookup hit
            UASSERT_SELFTEST(*pair.మొదటి, &విలువలు[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(సమితి.కొలత(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<విలువ_2, 20> విలువలు{};
        std::array<తార్కిక, 20> చూసిన{};
        సమితి_2 సమితి;
        కొరకు (కొలత_t i = 0; i < విలువలు.కొలత(); ++i) {
            విలువలు[i] = విలువ_2{i / 2, i};  // Pairs of them collide
            సమితి.చొప్పించు(&విలువలు[i]);
        }
        కొలత_t n = 0;
        కొరకు (విలువ_2* స్థిరరాశి valuep : సమితి) {
            UASSERT_SELFTEST(valuep->m_id < చూసిన.కొలత(), నిజం);  // Only entries added
            UASSERT_SELFTEST(చూసిన[valuep->m_id], అబద్ధం);  // Each of them exactly once
            చూసిన[valuep->m_id] = నిజం;
            ++n;
        }
        UASSERT_SELFTEST(n, విలువలు.కొలత());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

శూన్య test_నమోదు_Lifetime() {
    కొలత_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    తరగతి NoDefault final {
        కొలత_t* m_alivep;  // Where the live entries are counted
        కొలత_t m_hash;  // Hash of this entry
        కొలత_t m_id;  // Entries with equal ids are equal

    బహిరంగ:
        NoDefault(కొలత_t* alivep, కొలత_t hash, కొలత_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(స్థిరరాశి NoDefault& that)
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
        NoDefault& క్రియాకారకం=(స్థిరరాశి NoDefault&) = తొలగించు;
        NoDefault& క్రియాకారకం=(NoDefault&&) = తొలగించు;

        కొలత_t hash() స్థిరరాశి { తిరిగి m_hash; }
        కొలత_t id() స్థిరరాశి { తిరిగి m_id; }
        తార్కిక క్రియాకారకం==(స్థిరరాశి NoDefault& that) స్థిరరాశి { తిరిగి m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::విలువ,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::విలువ,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::విలువ,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::విలువ,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    నిర్మాణం Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి NoDefault& విలువ) స్థిరరాశి { తిరిగి విలువ.hash(); }
        కొలత_t క్రియాకారకం()(కొలత_t hash, కొలత_t) స్థిరరాశి { తిరిగి hash; }
    };

    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి NoDefault& a, స్థిరరాశి NoDefault& b) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(a, b.hash(), b.id());
        }
        తార్కిక క్రియాకారకం()(స్థిరరాశి NoDefault& a, కొలత_t, కొలత_t id) స్థిరరాశి { తిరిగి a.id() == id; }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<NoDefault, Hash, Equal>;

    constexpr కొలత_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        సమితి_2 సమితి;
        కొరకు (కొలత_t i = 0; i < N; ++i) {
            స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> added = సమితి.చొప్పించు(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.రెండవ, నిజం);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(సమితి.కొలత(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        కొరకు (కొలత_t i = 0; i < N; ++i) {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(కొలత_t{7}, i);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> dup = సమితి.చొప్పించు(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.రెండవ, అబద్ధం);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.మొదటి->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(సమితి.కొలత(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(కొలత_t{7}, కొలత_t{0});
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Present before the erase
            UASSERT_SELFTEST(it->id(), కొలత_t{0});  // And is the entry asked for
            సమితి.erase(it);
        }
        UASSERT_SELFTEST(సమితి.కొలత(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(సమితి.కలిగియుంది(కొలత_t{7}, కొలత_t{0}), అబద్ధం);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        కొరకు (కొలత_t i = 1; i < N; ++i) {
            {
                స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(కొలత_t{7}, i);
                UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                సమితి.erase(it);
            }
            UASSERT_SELFTEST(సమితి.కొలత(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            కొరకు (కొలత_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(సమితి.కలిగియుంది(కొలత_t{7}, j), నిజం);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(సమితి.begin() == సమితి.చివర(), నిజం);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        కొరకు (కొలత_t i = 0; i < N; ++i) సమితి.చొప్పించు(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        సమితి.clear();
        UASSERT_SELFTEST(సమితి.కొలత(), 0);  // Nothing left
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Says so
        UASSERT_SELFTEST(సమితి.begin() == సమితి.చివర(), నిజం);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(సమితి.కలిగియుంది(కొలత_t{7}, కొలత_t{0}), అబద్ధం);  // Nor finds any
        సమితి.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        కొరకు (కొలత_t i = 0; i < N; ++i) సమితి.చొప్పించు(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        సమితి_2 సమితి;
        సమితి.clear();
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // Still empty
        సమితి.చొప్పించు(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(సమితి.కొలత(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

శూన్య testMove() {
    కొలత_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    నిర్మాణం Counted final {
        కొలత_t* m_alivep;  // Where the live entries are counted
        కొలత_t m_id;  // Entries with equal ids are equal

        Counted(కొలత_t* alivep, కొలత_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(స్థిరరాశి Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& క్రియాకారకం=(స్థిరరాశి Counted&) = తొలగించు;
        Counted& క్రియాకారకం=(Counted&&) = తొలగించు;
    };

    నిర్మాణం Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి Counted& నమోదు_2) స్థిరరాశి { తిరిగి క్రియాకారకం()(నమోదు_2.m_id); }
        కొలత_t క్రియాకారకం()(కొలత_t id) స్థిరరాశి { తిరిగి id; }
    };

    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి Counted& a, స్థిరరాశి Counted& b) స్థిరరాశి { తిరిగి క్రియాకారకం()(a, b.m_id); }
        తార్కిక క్రియాకారకం()(స్థిరరాశి Counted& a, కొలత_t id) స్థిరరాశి { తిరిగి a.m_id == id; }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<Counted, Hash, Equal>;

    constexpr కొలత_t N = GROWS_TWICE;
    {
        సమితి_2 సమితి;
        కొరకు (కొలత_t i = 0; i < N; ++i) సమితి.చొప్పించు(Counted{&alive, i});
        UASSERT_SELFTEST(సమితి.కొలత(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        సమితి_2 moved{std::move(సమితి)};
        UASSERT_SELFTEST(moved.కొలత(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // And the source no longer holds them
        UASSERT_SELFTEST(సమితి.begin() == సమితి.చివర(), నిజం);  // So it iterates nothing
        కొరకు (కొలత_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.కలిగియుంది(i), నిజం);  // Every entry came across
            UASSERT_SELFTEST(సమితి.కలిగియుంది(i), అబద్ధం);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(సమితి.చొప్పించు(Counted{&alive, N}).రెండవ, నిజం);  // It took an entry
        UASSERT_SELFTEST(సమితి.కొలత(), 1);  // Which is all it holds
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), అబద్ధం);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        సమితి = std::move(moved);
        UASSERT_SELFTEST(సమితి.కొలత(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(సమితి.కలిగియుంది(N), అబద్ధం);  // Namely that one
        UASSERT_SELFTEST(moved.ఖాళీలు(), నిజం);  // And the source is empty again
        కొరకు (కొలత_t i = 0; i < N; ++i) UASSERT_SELFTEST(సమితి.కలిగియుంది(i), నిజం);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

శూన్య testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    తరగతి MoveOnly final {
        కొలత_t m_id;  // Entries with equal ids are equal

    బహిరంగ:
        స్పష్టమైన MoveOnly(కొలత_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = అప్రమేయ;
        MoveOnly(స్థిరరాశి MoveOnly&) = తొలగించు;
        MoveOnly& క్రియాకారకం=(స్థిరరాశి MoveOnly&) = తొలగించు;
        MoveOnly& క్రియాకారకం=(MoveOnly&&) = తొలగించు;
        ~MoveOnly() = అప్రమేయ;

        కొలత_t id() స్థిరరాశి { తిరిగి m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::విలువ,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::విలువ,
                  "SelfTest: 'MoveOnly' must be move constructible");

    నిర్మాణం Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        కొలత_t క్రియాకారకం()(స్థిరరాశి MoveOnly& విలువ) స్థిరరాశి { తిరిగి క్రియాకారకం()(విలువ.id()); }
        కొలత_t క్రియాకారకం()(కొలత_t) స్థిరరాశి { తిరిగి 7; }
    };

    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి MoveOnly& a, స్థిరరాశి MoveOnly& b) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(a, b.id());
        }
        తార్కిక క్రియాకారకం()(స్థిరరాశి MoveOnly& a, కొలత_t id) స్థిరరాశి { తిరిగి a.id() == id; }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<MoveOnly, Hash, Equal>;

    constexpr కొలత_t N = GROWS_TWICE;
    సమితి_2 సమితి;
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> added
            = సమితి.చొప్పించు_Lazy(i, [i] { తిరిగి MoveOnly{i}; });
        UASSERT_SELFTEST(added.రెండవ, నిజం);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.మొదటి->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(సమితి.కొలత(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    కొరకు (కొలత_t i = 0; i < N; i += 2) {
        స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(i);
        UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Present before the erase
        సమితి.erase(it);
    }
    UASSERT_SELFTEST(సమితి.కొలత(), N / 2);  // Half of them are gone
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి తార్కిక erased = (i % 2) == 0;
        UASSERT_SELFTEST(సమితి.కలిగియుంది(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

శూన్య testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    తరగతి Hash final {
        కొలత_t m_ముసుగు;  // Only these bits of an entry matter

    బహిరంగ:
        స్పష్టమైన Hash(కొలత_t ముసుగు)
            : m_ముసుగు{ముసుగు} {}
        కొలత_t క్రియాకారకం()(కొలత_t విలువ) స్థిరరాశి { తిరిగి విలువ & m_ముసుగు; }
    };

    తరగతి Equal final {
        కొలత_t m_ముసుగు;  // Only these bits of an entry matter

    బహిరంగ:
        స్పష్టమైన Equal(కొలత_t ముసుగు)
            : m_ముసుగు{ముసుగు} {}
        తార్కిక క్రియాకారకం()(కొలత_t a, కొలత_t b) స్థిరరాశి { తిరిగి (a & m_ముసుగు) == (b & m_ముసుగు); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::విలువ,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::విలువ,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_సమితి<కొలత_t, Hash, Equal> సమితి{Hash{3}, Equal{3}};
    కొరకు (కొలత_t i = 0; i < 16; ++i) సమితి.చొప్పించు(i);
    UASSERT_SELFTEST(సమితి.కొలత(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    కొరకు (కొలత_t i = 0; i < 16; ++i) {
        స్థిరరాశి V3_Hash_సమితి<కొలత_t, Hash, Equal>::iterator it = సమితి.కనుగొను(i);
        UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

శూన్య test_Backward_జరుపు_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    నిర్మాణం విలువ_2 final {
        కొలత_t m_hash = 0;  // Hash of this entry
        కొలత_t m_id = 0;  // Entries with equal ids are equal
    };

    నిర్మాణం Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి విలువ_2* valuep) స్థిరరాశి { తిరిగి క్రియాకారకం()(valuep->m_hash, 0); }
        కొలత_t క్రియాకారకం()(కొలత_t hash, కొలత_t) స్థిరరాశి { తిరిగి hash; }
    };

    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2* ap, స్థిరరాశి విలువ_2* bp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(ap, bp->m_hash, bp->m_id);
        }
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2* valuep, కొలత_t hash, కొలత_t id) స్థిరరాశి {
            తిరిగి valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<విలువ_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    కొరకు (స్థిరరాశి కొలత_t h : {కొలత_t{0}, కొలత_t{5}, ~కొలత_t{0}}) {  // Wraps too
        std::array<విలువ_2, 4> విలువలు{విలువ_2{h, 0}, విలువ_2{h + 1, 1},  //
                                    విలువ_2{h, 2}, విలువ_2{h + 1, 3}};
        సమితి_2 సమితి;
        కొరకు (విలువ_2& విలువ : విలువలు) సమితి.చొప్పించు(&విలువ);
        UASSERT_SELFTEST(సమితి.కొలత(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(h, కొలత_t{0});
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Present before the erase
            సమితి.erase(it);
        }
        UASSERT_SELFTEST(సమితి.కొలత(), 3);  // One fewer entry
        UASSERT_SELFTEST(సమితి.కలిగియుంది(h, కొలత_t{0}), అబద్ధం);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        కొరకు (కొలత_t i = 1; i < విలువలు.కొలత(); ++i) {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(విలువలు[i].m_hash, i);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &విలువలు[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(h + 1, కొలత_t{1});
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Left where the first erase found it
            సమితి.erase(it);
        }
        UASSERT_SELFTEST(సమితి.కొలత(), 2);  // Two are left
        కొరకు (కొలత_t i = 2; i < విలువలు.కొలత(); ++i) {
            స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(విలువలు[i].m_hash, i);
            UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Both still reachable
            UASSERT_SELFTEST(*it, &విలువలు[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<తార్కిక, 4> చూసిన{};
        కొరకు (స్థిరరాశి విలువ_2* స్థిరరాశి valuep : సమితి) {
            UASSERT_SELFTEST(చూసిన[valuep->m_id], అబద్ధం);  // Each entry once
            చూసిన[valuep->m_id] = నిజం;
        }
        UASSERT_SELFTEST(చూసిన[0], అబద్ధం);  // Erased first
        UASSERT_SELFTEST(చూసిన[1], అబద్ధం);  // Erased second
        UASSERT_SELFTEST(చూసిన[2], నిజం);  // Moved back over the first hole
        UASSERT_SELFTEST(చూసిన[3], నిజం);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

శూన్య testReferenceStability() {
    నిర్మాణం విలువ_2 final {
        కొలత_t m_hash = 0;  // Hash of this entry
        కొలత_t m_id = 0;  // Id of this entry
    };

    నిర్మాణం Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి విలువ_2* valuep) స్థిరరాశి { తిరిగి క్రియాకారకం()(valuep->m_hash, 0); }
        కొలత_t క్రియాకారకం()(కొలత_t hash, కొలత_t) స్థిరరాశి { తిరిగి hash; }
    };

    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2* ap, స్థిరరాశి విలువ_2* bp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(ap, bp->m_hash, bp->m_id);
        }
        తార్కిక క్రియాకారకం()(స్థిరరాశి విలువ_2* valuep, కొలత_t hash, కొలత_t id) స్థిరరాశి {
            తిరిగి valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<విలువ_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        సమితి_2 సమితి;
        సమితి.reserve(8);
        UASSERT_SELFTEST(సమితి.కలిగియుంది(కొలత_t{7}, కొలత_t{0}), అబద్ధం);  // Room, but no entry
        UASSERT_SELFTEST(సమితి.begin() == సమితి.చివర(), నిజం);  // So iterates nothing
        UASSERT_SELFTEST(సమితి.ఖాళీలు(), నిజం);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr కొలత_t N = max_ఎక్కించు(2 * MIN_సామర్థ్యం);
    static_assert(N > max_ఎక్కించు(MIN_సామర్థ్యం), "SelfTest: 'N' must need more than a new table");
    std::array<విలువ_2, N> విలువలు{};
    std::array<విలువ_2* స్థిరరాశి*, N> entrypps{};  // Where each entry is stored, as inserted
    సమితి_2 సమితి;
    సమితి.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        విలువలు[i] = విలువ_2{7, i};
        స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> pair = సమితి.చొప్పించు(&విలువలు[i]);
        UASSERT_SELFTEST(pair.రెండవ, నిజం);  // Ids differ, so each is added
        entrypps[i] = &*pair.మొదటి;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(కొలత_t{7}, i);
        UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి std::pair<సమితి_2::iterator, తార్కిక> pair = సమితి.చొప్పించు(&విలువలు[i]);
        UASSERT_SELFTEST(pair.రెండవ, అబద్ధం);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.మొదటి, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(సమితి.కొలత(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    సమితి.reserve(8 * N);
    UASSERT_SELFTEST(సమితి.కొలత(), N);  // Rehashing dropped nothing
    std::array<విలువ_2* స్థిరరాశి*, N> grownpps{};  // Where each entry is after the growth
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(కొలత_t{7}, i);
        UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    సమితి.reserve(N);
    UASSERT_SELFTEST(సమితి.కొలత(), N);  // The smaller request changed nothing
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి సమితి_2::iterator it = సమితి.కనుగొను(కొలత_t{7}, i);
        UASSERT_SELFTEST(it != సమితి.చివర(), నిజం);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
నిర్మాణం ClusteredHash final {
    కొలత_t క్రియాకారకం()(కొలత_t విలువ) స్థిరరాశి { తిరిగి విలువ & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
నిర్మాణం విస్తరణ_Hash final {
    కొలత_t క్రియాకారకం()(కొలత_t విలువ) స్థిరరాశి {
        తిరిగి static_cast<కొలత_t>(విలువ * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
మూస <రకం_పేరు T_Hash>
శూన్య test_సమితి_Against_Model() {
    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(కొలత_t a, కొలత_t b) స్థిరరాశి { తిరిగి a == b; }
    };

    ఉపయోగించి సమితి_2 = V3_Hash_సమితి<కొలత_t, T_Hash, Equal>;

    constexpr కొలత_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr కొలత_t అడుగులు = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    స్థిరరాశి స్వయంచాలక తదుపరి_Rand = [&state]() -> కొలత_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        తిరిగి static_cast<కొలత_t>(state >> 32);
    };

    సమితి_2 సమితి;
    std::సమితి<కొలత_t> model;

    కొరకు (కొలత_t అడుగు = 0; అడుగు < అడుగులు; ++అడుగు) {
        స్థిరరాశి కొలత_t id = తదుపరి_Rand() % UNIVERSE;
        ఎంపిక (తదుపరి_Rand() % 4) {
        స్థితి 0: {  // Insert a copy
            స్థిరరాశి std::pair<రకం_పేరు సమితి_2::iterator, తార్కిక> pair = సమితి.చొప్పించు(id);
            UASSERT_SELFTEST(pair.రెండవ, model.చొప్పించు(id).రెండవ);  // As the model
            UASSERT_SELFTEST(*pair.మొదటి, id);  // At the entry asked for
            ఆపు;
        }
        స్థితి 1: {  // Insert lazily, which must create only on a miss
            తార్కిక created = అబద్ధం;
            స్థిరరాశి std::pair<రకం_పేరు సమితి_2::iterator, తార్కిక> pair
                = సమితి.చొప్పించు_Lazy(id, [&]() -> కొలత_t {
                      created = నిజం;
                      తిరిగి id;
                  });
            UASSERT_SELFTEST(pair.రెండవ, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.రెండవ, model.చొప్పించు(id).రెండవ);  // As the model
            UASSERT_SELFTEST(*pair.మొదటి, id);  // At the entry asked for
            ఆపు;
        }
        స్థితి 2: {  // Erase, if present
            స్థిరరాశి రకం_పేరు సమితి_2::iterator it = సమితి.కనుగొను(id);
            UASSERT_SELFTEST(it != సమితి.చివర(), model.లెక్క(id) != 0);  // As the model
            ఒకవేళ (it != సమితి.చివర()) {
                సమితి.erase(it);
                model.erase(id);
            }
            ఆపు;
        }
        అప్రమేయ: {  // Look up only
            స్థిరరాశి రకం_పేరు సమితి_2::iterator it = సమితి.కనుగొను(id);
            UASSERT_SELFTEST(it != సమితి.చివర(), model.లెక్క(id) != 0);  // As the model
            ఒకవేళ (it != సమితి.చివర()) UASSERT_SELFTEST(*it, id);  // And is the entry
            ఆపు;
        }
        }
        UASSERT_SELFTEST(సమితి.కొలత(), model.కొలత());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    కొరకు (కొలత_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(సమితి.కలిగియుంది(id), model.లెక్క(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<తార్కిక, UNIVERSE> చూసిన{};
    కొలత_t n = 0;
    కొరకు (స్థిరరాశి కొలత_t నమోదు_2 : సమితి) {
        UASSERT_SELFTEST(నమోదు_2 < UNIVERSE, నిజం);  // Nothing out of thin air
        UASSERT_SELFTEST(model.లెక్క(నమోదు_2) != 0, నిజం);  // Only what was inserted
        UASSERT_SELFTEST(చూసిన[నమోదు_2], అబద్ధం);  // Each entry once
        చూసిన[నమోదు_2] = నిజం;
        ++n;
    }
    UASSERT_SELFTEST(n, model.కొలత());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

శూన్య testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    ఉపయోగించి Map = V3HashMap<std::string, పూర్ణాంకం>;
    ఉపయోగించి నమోదు_3 = Map::నమోదు_3;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<నమోదు_3, std::pair<std::string, పూర్ణాంకం>>::విలువ,
                  "SelfTest: a map entry must be a plain pair");

    constexpr కొలత_t N = GROWS_TWICE;
    Map map;
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి std::string కీలకం = "key" + std::గమ్యం_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        స్థిరరాశి std::pair<Map::iterator, తార్కిక> pair
            = map.చొప్పించు_Lazy(కీలకం, [&]() -> నమోదు_3 { తిరిగి {కీలకం, static_cast<పూర్ణాంకం>(i)}; });
        UASSERT_SELFTEST(pair.రెండవ, నిజం);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.కొలత(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి Map::iterator it = map.కనుగొను("key" + std::గమ్యం_string(i));
        UASSERT_SELFTEST(it != map.చివర(), నిజం);  // The key alone finds it
        UASSERT_SELFTEST(it->రెండవ, static_cast<పూర్ణాంకం>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి std::string కీలకం = "key" + std::గమ్యం_string(i);
        map.erase(map.కనుగొను(కీలకం));
        UASSERT_SELFTEST(map.చొప్పించు({కీలకం, static_cast<పూర్ణాంకం>(i) + 100}).రెండవ, నిజం);  // Anew
    }
    UASSERT_SELFTEST(map.కొలత(), N);  // The same keys, so the map has not grown
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి Map::iterator it = map.కనుగొను("key" + std::గమ్యం_string(i));
        UASSERT_SELFTEST(it != map.చివర(), నిజం);  // Each key is still there
        UASSERT_SELFTEST(it->రెండవ, static_cast<పూర్ణాంకం>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    కొరకు (కొలత_t i = 0; i < N; ++i) map.కనుగొను("key" + std::గమ్యం_string(i)).విలువ() += 100;
    UASSERT_SELFTEST(map.కొలత(), N);  // The map is unchanged otherwise
    కొరకు (కొలత_t i = 0; i < N; ++i) {
        స్థిరరాశి Map::iterator it = map.కనుగొను("key" + std::గమ్యం_string(i));
        UASSERT_SELFTEST(it != map.చివర(), నిజం);  // Each key still finds its entry
        UASSERT_SELFTEST(it->రెండవ, static_cast<పూర్ణాంకం>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    కొరకు (కొలత_t i = 0; i < N; ++i) map.కనుగొను("key" + std::గమ్యం_string(i)).విలువ() -= 100;

    // Adding a key that is present changes nothing
    {
        స్థిరరాశి std::string కీలకం = "key0";
        తార్కిక created = అబద్ధం;
        స్థిరరాశి std::pair<Map::iterator, తార్కిక> pair = map.చొప్పించు_Lazy(కీలకం, [&]() -> నమోదు_3 {
            created = నిజం;  // LCOV_EXCL_START
            తిరిగి {కీలకం, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, అబద్ధం);  // The factory was never called
        UASSERT_SELFTEST(pair.రెండవ, అబద్ధం);  // As the key is present
        UASSERT_SELFTEST(pair.మొదటి->రెండవ, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.కొలత(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        స్థిరరాశి std::pair<Map::iterator, తార్కిక> added = map.చొప్పించు({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.రెండవ, నిజం);  // Added, as the key is new
        UASSERT_SELFTEST(added.మొదటి->రెండవ, 5);  // With the value given
        స్థిరరాశి std::pair<Map::iterator, తార్కిక> again = map.చొప్పించు({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.రెండవ, అబద్ధం);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.మొదటి->రెండవ, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.కొలత(), N + 1);  // Just the one entry was added
    {
        స్థిరరాశి Map::iterator it = map.కనుగొను(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.చివర(), నిజం);  // Present before the erase
        UASSERT_SELFTEST(it->రెండవ, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.కొలత(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), అబద్ధం);  // No such key
    UASSERT_SELFTEST(map.కొలత(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), నిజం);  // That key is present
    UASSERT_SELFTEST(map.కొలత(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.కలిగియుంది(std::string{"key0"}), అబద్ధం);  // Namely that one
    కొరకు (కొలత_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.కలిగియుంది("key" + std::గమ్యం_string(i)), నిజం);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
మూస <రకం_పేరు T_Hash>
శూన్య testMapAgainstModel() {
    నిర్మాణం Equal final {
        తార్కిక క్రియాకారకం()(కొలత_t a, కొలత_t b) స్థిరరాశి { తిరిగి a == b; }
    };

    ఉపయోగించి Map = V3HashMap<కొలత_t, కొలత_t, T_Hash, Equal>;
    ఉపయోగించి నమోదు_3 = రకం_పేరు Map::నమోదు_3;

    constexpr కొలత_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr కొలత_t అడుగులు = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    స్థిరరాశి స్వయంచాలక తదుపరి_Rand = [&state]() -> కొలత_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        తిరిగి static_cast<కొలత_t>(state >> 32);
    };

    Map map;
    std::map<కొలత_t, కొలత_t> model;

    కొరకు (కొలత_t అడుగు = 0; అడుగు < అడుగులు; ++అడుగు) {
        స్థిరరాశి కొలత_t కీలకం = తదుపరి_Rand() % UNIVERSE;
        స్థిరరాశి కొలత_t val = అడుగు;  // Distinct every step, so a stale value is visible
        ఎంపిక (తదుపరి_Rand() % 5) {
        స్థితి 0: {  // Insert a whole entry
            స్థిరరాశి std::pair<రకం_పేరు Map::iterator, తార్కిక> pair = map.చొప్పించు({కీలకం, val});
            UASSERT_SELFTEST(pair.రెండవ, model.చొప్పించు({కీలకం, val}).రెండవ);  // Ditto
            UASSERT_SELFTEST(pair.మొదటి->మొదటి, కీలకం);  // At the key asked for
            ఆపు;
        }
        స్థితి 1: {  // Insert lazily, which must create only on a miss
            తార్కిక created = అబద్ధం;
            స్థిరరాశి std::pair<రకం_పేరు Map::iterator, తార్కిక> pair
                = map.చొప్పించు_Lazy(కీలకం, [&]() -> నమోదు_3 {
                      created = నిజం;
                      తిరిగి {కీలకం, val};
                  });
            UASSERT_SELFTEST(pair.రెండవ, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.రెండవ, model.చొప్పించు({కీలకం, val}).రెండవ);  // Ditto
            UASSERT_SELFTEST(pair.మొదటి->మొదటి, కీలకం);  // At the key asked for
            ఆపు;
        }
        స్థితి 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(కీలకం), model.erase(కీలకం) != 0);  // As model
            ఆపు;
        }
        స్థితి 3: {  // Change a value, which is erasing the entry and inserting it again
            స్థిరరాశి రకం_పేరు Map::iterator it = map.కనుగొను(కీలకం);
            ఒకవేళ (it != map.చివర()) {
                map.erase(it);
                UASSERT_SELFTEST(map.చొప్పించు({కీలకం, val}).రెండవ, నిజం);  // The key is free
                model[కీలకం] = val;
            }
            ఆపు;
        }
        అప్రమేయ: {  // Look up only
            స్థిరరాశి రకం_పేరు Map::iterator it = map.కనుగొను(కీలకం);
            UASSERT_SELFTEST(it != map.చివర(), model.లెక్క(కీలకం) != 0);  // As the model
            ఒకవేళ (it != map.చివర()) {
                UASSERT_SELFTEST(it->మొదటి, కీలకం);  // At the key asked for
                UASSERT_SELFTEST(it->రెండవ, model.at(కీలకం));  // With its value
            }
            ఆపు;
        }
        }
        UASSERT_SELFTEST(map.కొలత(), model.కొలత());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    కొరకు (కొలత_t కీలకం = 0; కీలకం < UNIVERSE; ++కీలకం) {
        స్థిరరాశి రకం_పేరు Map::iterator it = map.కనుగొను(కీలకం);
        UASSERT_SELFTEST(it != map.చివర(), model.లెక్క(కీలకం) != 0);  // Present iff modelled
        ఒకవేళ (it != map.చివర()) {
            UASSERT_SELFTEST(it->రెండవ, model.at(కీలకం));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<తార్కిక, UNIVERSE> చూసిన{};
    కొలత_t n = 0;
    కొరకు (స్థిరరాశి నమోదు_3& నమోదు_2 : map) {
        UASSERT_SELFTEST(నమోదు_2.మొదటి < UNIVERSE, నిజం);  // Nothing out of thin air
        UASSERT_SELFTEST(model.లెక్క(నమోదు_2.మొదటి) != 0, నిజం);  // Only what was inserted
        UASSERT_SELFTEST(నమోదు_2.రెండవ, model.at(నమోదు_2.మొదటి));  // With the right value
        UASSERT_SELFTEST(చూసిన[నమోదు_2.మొదటి], అబద్ధం);  // Each entry once
        చూసిన[నమోదు_2.మొదటి] = నిజం;
        ++n;
    }
    UASSERT_SELFTEST(n, model.కొలత());  // And all of them
}

శూన్య selfTest() {
    test_విలువ_కీలకాలు();
    test_సూచిక_కీలకాలు();
    test_నమోదు_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_జరుపు_Deletion();
    testReferenceStability();
    test_సమితి_Against_Model<ClusteredHash>();
    test_సమితి_Against_Model<విస్తరణ_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<విస్తరణ_Hash>();
}

}  // namespace V3HashTableInternals
