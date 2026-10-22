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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3HashTable.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>

ਨਾਮਾਕਾਸ਼ V3_Hash_ਸਾਰਣੀ_Internals {

// Entries that fill a table of the given capacity to its maximum load, so one more grows it
constexpr ਮਾਪ_t max_ਲੱਦੋ(ਮਾਪ_t ਸਮਰੱਥਾ) { ਵਾਪਸ ਸਮਰੱਥਾ * ਲੱਦੋ_ਗੁਣਕ_ਅੰਕ / ਲੱਦੋ_ਗੁਣਕ_DEN; }

// Enough entries to grow the smallest table twice, that is, to fill four times the minimum
constexpr ਮਾਪ_t GROWS_TWICE = max_ਲੱਦੋ(4 * MIN_ਸਮਰੱਥਾ);
static_assert(GROWS_TWICE > max_ਲੱਦੋ(2 * MIN_ਸਮਰੱਥਾ),
              "SelfTest: 'GROWS_TWICE' must overflow a table twice the minimum");

//######################################################################
// Set key by value

ਖਾਲੀ test_ਮੁੱਲ_ਕੁੰਜੀਆਂ() {
    ਬਣਤਰ ਮੁੱਲ_2 final {
        ਮਾਪ_t m_hash = 0;  // Hash of this entry
        ਮਾਪ_t m_id = 0;  // Id of this entry
    };

    ਬਣਤਰ Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2& ਮੁੱਲ) ਅਚਲ { ਵਾਪਸ ਸੰਚਾਲਕ()(ਮੁੱਲ.m_hash, 0); }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t hash, ਮਾਪ_t) ਅਚਲ { ਵਾਪਸ hash; }
    };

    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2& a, ਅਚਲ ਮੁੱਲ_2& b) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(a, b.m_hash, b.m_id);
        }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2& ਮੁੱਲ, ਮਾਪ_t hash, ਮਾਪ_t id) ਅਚਲ {
            ਵਾਪਸ ਮੁੱਲ.m_hash == hash && ਮੁੱਲ.m_id == id;
        }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<ਮੁੱਲ_2, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ਅਚਲ ਮੁੱਲ_2 ਮੁੱਲ{1, 0};
        ਅਚਲ ਮੁੱਲ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        ਸੈੱਟ_2 ਸੈੱਟ;
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Starts empty
        UASSERT_SELFTEST(ਸੈੱਟ.begin() == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // So iterates nothing
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(ਮੁੱਲ) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And finds nothing
        ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> added = ਸੈੱਟ.ਪਾਓ(ਮੁੱਲ);
        UASSERT_SELFTEST(added.ਦੂਜਾ, ਸੱਚ);  // Added, as it was absent
        UASSERT_SELFTEST(added.ਪਹਿਲਾ->m_id, ਮੁੱਲ.m_id);  // The iterator is at the entry
        UASSERT_SELFTEST(&*added.ਪਹਿਲਾ != &ਮੁੱਲ, ਸੱਚ);  // Which is a copy of the argument
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 1);  // And is the only one
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਝੂਠ);  // So the set is no longer empty
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Found by itself
            UASSERT_SELFTEST(&*it, &*added.ਪਹਿਲਾ);  // At the entry held
        }
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(equal);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And by an equal entry
            UASSERT_SELFTEST(&*it, &*added.ਪਹਿਲਾ);  // At that same entry
        }
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(1, ਮਾਪ_t{0});
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And by its parts
            UASSERT_SELFTEST(&*it, &*added.ਪਹਿਲਾ);  // At that same entry again
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(1, ਮਾਪ_t{1}) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Hash same, id not
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(2, ਮਾਪ_t{0}) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> again = ਸੈੱਟ.ਪਾਓ(equal);
        UASSERT_SELFTEST(again.ਦੂਜਾ, ਝੂਠ);  // Not added, as an equal is present
        UASSERT_SELFTEST(&*again.ਪਹਿਲਾ, &*added.ਪਹਿਲਾ);  // At the entry already held
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 1);  // Still just the one entry
        // Erasing through an iterator removes the entry it is at
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Present before the erase
            ਸੈੱਟ.erase(it);
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Empty again
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(ਮੁੱਲ) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // So the entry is gone
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(ਮੁੱਲ), ਝੂਠ);  // As 'contains' agrees
        // Erase by key removes whatever that key finds
        UASSERT_SELFTEST(ਸੈੱਟ.erase(ਮੁੱਲ), ਝੂਠ);  // Nothing to erase, so says so
        ਸੈੱਟ.ਪਾਓ(ਮੁੱਲ);
        UASSERT_SELFTEST(ਸੈੱਟ.erase(equal), ਸੱਚ);  // An equal key erases the entry held
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Which leaves the set empty
        // And by a key spelled as the parts of an entry
        ਸੈੱਟ.ਪਾਓ(ਮੁੱਲ);
        UASSERT_SELFTEST(ਸੈੱਟ.erase(1, ਮਾਪ_t{0}), ਸੱਚ);  // Those parts find and erase it
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Empty once more
    }

    // Colliding entries are all kept and stay reachable, including after the table grows
    {
        constexpr ਮਾਪ_t N = GROWS_TWICE;
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
            ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> added = ਸੈੱਟ.ਪਾਓ(ਮੁੱਲ_2{7, i});
            UASSERT_SELFTEST(added.ਦੂਜਾ, ਸੱਚ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // All of them, in the one probe run
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮਾਪ_t{7}, i);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // The growth lost nothing
            UASSERT_SELFTEST(it->m_id, i);  // And is the entry asked for
        }
        // Iterating visits every entry exactly once
        std::array<ਤਾਰਕਿਕ, N> ਵੇਖਿਆ{};
        ਮਾਪ_t n = 0;
        ਲਈ (ਅਚਲ ਮੁੱਲ_2& ਇੰਦਰਾਜ : ਸੈੱਟ) {
            UASSERT_SELFTEST(ਇੰਦਰਾਜ.m_hash, ਮਾਪ_t{7});  // Only entries added
            UASSERT_SELFTEST(ਵੇਖਿਆ[ਇੰਦਰਾਜ.m_id], ਝੂਠ);  // Each of them exactly once
            ਵੇਖਿਆ[ਇੰਦਰਾਜ.m_id] = ਸੱਚ;
            ++n;
        }
        UASSERT_SELFTEST(n, N);  // And every one of them
    }
}

//######################################################################
// Set key by pointer

ਖਾਲੀ test_ਸੂਚਕ_ਕੁੰਜੀਆਂ() {
    ਬਣਤਰ ਮੁੱਲ_2 final {
        ਮਾਪ_t m_hash = 0;  // Hash of this entry
        ਮਾਪ_t m_id = 0;  // Id of this entry
    };

    ਬਣਤਰ Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* valuep) ਅਚਲ { ਵਾਪਸ ਸੰਚਾਲਕ()(valuep->m_hash, 0); }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t hash, ਮਾਪ_t) ਅਚਲ { ਵਾਪਸ hash; }
    };

    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* ap, ਅਚਲ ਮੁੱਲ_2* bp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(ap, bp->m_hash, bp->m_id);
        }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* valuep, ਮਾਪ_t hash, ਮਾਪ_t id) ਅਚਲ {
            ਵਾਪਸ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<ਮੁੱਲ_2*, Hash, Equal>;

    // Entries that are added are found, entries that are not are not
    {
        ਮੁੱਲ_2 ਮੁੱਲ{1, 0};
        ਮੁੱਲ_2 equal{1, 0};  // Equal to 'value', and hashes the same
        ਸੈੱਟ_2 ਸੈੱਟ;
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Starts empty
        UASSERT_SELFTEST(ਸੈੱਟ.begin() == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // So iterates nothing
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And finds nothing
        ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> added = ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲ);
        UASSERT_SELFTEST(added.ਦੂਜਾ, ਸੱਚ);  // Added, as it was absent
        UASSERT_SELFTEST(*added.ਪਹਿਲਾ, &ਮੁੱਲ);  // The iterator is at the new entry
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 1);  // Which is the only one
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਝੂਠ);  // So the set is no longer empty
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Found by itself
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // At the entry held
        }
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&equal);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And by an equal entry
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // At that same entry
        }
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(1, ਮਾਪ_t{0});
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And by its parts
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // At that same entry again
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(1, ਮਾਪ_t{1}) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Hash same, id not
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(2, ਮਾਪ_t{0}) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Id same, hash not
        // Adding an equal entry returns the one in the set, and does not add
        ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> again = ਸੈੱਟ.ਪਾਓ(&equal);
        UASSERT_SELFTEST(again.ਦੂਜਾ, ਝੂਠ);  // Not added, as an equal is present
        UASSERT_SELFTEST(*again.ਪਹਿਲਾ, &ਮੁੱਲ);  // The iterator is at the stored one
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 1);  // Still just the one entry
        // Given the distinct but equal '&equal', 'find' still yields the stored '&value'
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&equal);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Found, as they compare equal
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // But it is the stored one
            UASSERT_SELFTEST(*it == &equal, ਝੂਠ);  // Not the one asked for
        }
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // The entry is still there
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // And is the one stored
        }
        // Erasing the very entry held does remove it
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Present before the erase
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // And is the object asked for
            ਸੈੱਟ.erase(it);
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Empty again
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // So the entry is gone
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(&ਮੁੱਲ), ਝੂਠ);  // As 'contains' agrees
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // And no lookup added anything
        // Erase by key makes no such check, so it removes whatever the key finds, here
        // the stored '&value' when given the equal '&equal'
        UASSERT_SELFTEST(ਸੈੱਟ.erase(&ਮੁੱਲ), ਝੂਠ);  // Nothing to erase, so says so
        ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲ);
        UASSERT_SELFTEST(ਸੈੱਟ.erase(&equal), ਸੱਚ);  // The equal key erases the stored one
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Which leaves the set empty
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And unreachable
        // And by a key spelled as the parts of an entry
        ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲ);
        UASSERT_SELFTEST(ਸੈੱਟ.erase(1, ਮਾਪ_t{0}), ਸੱਚ);  // Those parts find and erase it
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Empty once more
    }

    // Erasing leaves the other entries reachable, whichever is erased, including when
    // entries with equal hashes are all in the one probe run
    ਲਈ (ਮਾਪ_t erase = 0; erase < 4; ++erase) {
        ਲਈ (ਅਚਲ ਮਾਪ_t hash : {ਮਾਪ_t{0}, ਮਾਪ_t{7}, ~ਮਾਪ_t{0}}) {  // Wraps too
            std::array<ਮੁੱਲ_2, 4> ਮੁੱਲਾਂ{ਮੁੱਲ_2{hash, 0}, ਮੁੱਲ_2{hash, 1},  //
                                        ਮੁੱਲ_2{hash, 2}, ਮੁੱਲ_2{hash, 3}};
            ਸੈੱਟ_2 ਸੈੱਟ;
            ਲਈ (ਮੁੱਲ_2& ਮੁੱਲ : ਮੁੱਲਾਂ) ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲ);
            UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 4);  // All distinct, so all added
            {
                ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲਾਂ[erase]);
                UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // The one to erase is present
                UASSERT_SELFTEST(*it, &ਮੁੱਲਾਂ[erase]);  // And is the object held
                ਸੈੱਟ.erase(it);
            }
            UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 3);  // Exactly one was erased
            UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲਾਂ[erase]) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // That one
            ਲਈ (ਮਾਪ_t i = 0; i < ਮੁੱਲਾਂ.ਮਾਪ(); ++i) {
                ਜੇ (i == erase) ਜਾਰੀ;
                ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(hash, i);
                UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // The others are all there
                UASSERT_SELFTEST(*it, &ਮੁੱਲਾਂ[i]);  // Each at the entry inserted
            }
        }
    }

    // Growing leaves all entries reachable, including when their run wraps around the end
    // of the table. Note this needs enough entries to actually grow, so do not reserve here.
    ਲਈ (ਅਚਲ ਮਾਪ_t hash : {ਮਾਪ_t{0}, ਮਾਪ_t{7}, ~ਮਾਪ_t{0}}) {
        std::array<ਮੁੱਲ_2, GROWS_TWICE> many{};
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਲਈ (ਮਾਪ_t i = 0; i < many.ਮਾਪ(); ++i) {
            many[i] = ਮੁੱਲ_2{hash, i};
            ਸੈੱਟ.ਪਾਓ(&many[i]);
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), many.ਮਾਪ());  // All of them were added
        ਲਈ (ਮਾਪ_t i = 0; i < many.ਮਾਪ(); ++i) {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(hash, i);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And all survived the growth
            UASSERT_SELFTEST(*it, &many[i]);  // Each at the entry inserted
        }
    }

    // Entries that collide but are not equal are both kept, and erase removes only the one
    {
        ਮੁੱਲ_2 ਮੁੱਲ{1, 0};
        ਮੁੱਲ_2 other{1, 1};  // Not equal to 'value', but lands on the same slot
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਸੈੱਟ.reserve(2);
        ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲ);
        ਸੈੱਟ.ਪਾਓ(&other);
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 2);  // Both kept, despite the collision
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // The first of the run is there
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // And is that entry
        }
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&other);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // As is the one behind it
            UASSERT_SELFTEST(*it, &other);  // Which probing reached past the first
        }
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Present before the erase
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // And is the object held
            ਸੈੱਟ.erase(it);
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 1);  // Only one was erased
        UASSERT_SELFTEST(ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ) == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Namely that one
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&other);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // The collider stays
            UASSERT_SELFTEST(*it, &other);  // Shifted back over the hole left
        }
    }

    // Reserving avoids growing, and all entries survive either way
    ਲਈ (ਅਚਲ ਤਾਰਕਿਕ doReserve : {ਝੂਠ, ਸੱਚ}) {
        std::array<ਮੁੱਲ_2, 8> ਮੁੱਲਾਂ{};
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਜੇ (doReserve) ਸੈੱਟ.reserve(ਮੁੱਲਾਂ.ਮਾਪ());
        ਲਈ (ਮਾਪ_t i = 0; i < ਮੁੱਲਾਂ.ਮਾਪ(); ++i) {
            ਮੁੱਲਾਂ[i] = ਮੁੱਲ_2{static_cast<ਮਾਪ_t>(i * 1234567), i};
            ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲਾਂ[i]);
        }
        ਲਈ (ਮੁੱਲ_2& ਮੁੱਲ : ਮੁੱਲਾਂ) {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(&ਮੁੱਲ);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Each entry survives
            UASSERT_SELFTEST(*it, &ਮੁੱਲ);  // And is the one inserted
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), ਮੁੱਲਾਂ.ਮਾਪ());  // And none was added twice
    }

    // 'insertLazy' creates only on a miss, including when that grows the table
    {
        constexpr ਮਾਪ_t N = GROWS_TWICE;
        std::array<ਮੁੱਲ_2, N> ਮੁੱਲਾਂ{};
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
            ਮੁੱਲਾਂ[i] = ਮੁੱਲ_2{7, i};  // All of them hash the same
            ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> pair
                = ਸੈੱਟ.ਪਾਓ_Lazy(ਮਾਪ_t{7}, i, [&]() -> ਮੁੱਲ_2* { ਵਾਪਸ &ਮੁੱਲਾਂ[i]; });
            UASSERT_SELFTEST(pair.ਦੂਜਾ, ਸੱਚ);  // Created, as it was absent
            UASSERT_SELFTEST(*pair.ਪਹਿਲਾ, &ਮੁੱਲਾਂ[i]);  // And is what the factory made
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // All of them were added
        // They are all present now, so nothing is created
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
            ਤਾਰਕਿਕ created = ਝੂਠ;
            ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> pair
                = ਸੈੱਟ.ਪਾਓ_Lazy(ਮਾਪ_t{7}, i, [&]() -> ਮੁੱਲ_2* {
                      created = ਸੱਚ;  // LCOV_EXCL_START
                      ਵਾਪਸ &ਮੁੱਲਾਂ[i];  // LCOV_EXCL_STOP
                  });
            UASSERT_SELFTEST(created, ਝੂਠ);  // The factory was never called
            UASSERT_SELFTEST(pair.ਦੂਜਾ, ਝੂਠ);  // As the lookup hit
            UASSERT_SELFTEST(*pair.ਪਹਿਲਾ, &ਮੁੱਲਾਂ[i]);  // On the stored entry
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // And the set is unchanged
    }

    // Iterating visits every entry exactly once
    {
        std::array<ਮੁੱਲ_2, 20> ਮੁੱਲਾਂ{};
        std::array<ਤਾਰਕਿਕ, 20> ਵੇਖਿਆ{};
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਲਈ (ਮਾਪ_t i = 0; i < ਮੁੱਲਾਂ.ਮਾਪ(); ++i) {
            ਮੁੱਲਾਂ[i] = ਮੁੱਲ_2{i / 2, i};  // Pairs of them collide
            ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲਾਂ[i]);
        }
        ਮਾਪ_t n = 0;
        ਲਈ (ਮੁੱਲ_2* ਅਚਲ valuep : ਸੈੱਟ) {
            UASSERT_SELFTEST(valuep->m_id < ਵੇਖਿਆ.ਮਾਪ(), ਸੱਚ);  // Only entries added
            UASSERT_SELFTEST(ਵੇਖਿਆ[valuep->m_id], ਝੂਠ);  // Each of them exactly once
            ਵੇਖਿਆ[valuep->m_id] = ਸੱਚ;
            ++n;
        }
        UASSERT_SELFTEST(n, ਮੁੱਲਾਂ.ਮਾਪ());  // And every one of them
    }
}

//######################################################################
// Entries held by value, which are constructed and destroyed in step with the slots

ਖਾਲੀ test_ਇੰਦਰਾਜ_Lifetime() {
    ਮਾਪ_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that is not default constructible, nor assignable, and that counts how
    // many are alive. A move makes another live entry, so the count tracks the occupied
    // slots however the container shuffles them about.
    ਵਰਗ NoDefault final {
        ਮਾਪ_t* m_alivep;  // Where the live entries are counted
        ਮਾਪ_t m_hash;  // Hash of this entry
        ਮਾਪ_t m_id;  // Entries with equal ids are equal

    ਜਨਤਕ:
        NoDefault(ਮਾਪ_t* alivep, ਮਾਪ_t hash, ਮਾਪ_t id)
            : m_alivep{alivep}
            , m_hash{hash}
            , m_id{id} {
            ++*m_alivep;
        }
        NoDefault(ਅਚਲ NoDefault& that)
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
        NoDefault& ਸੰਚਾਲਕ=(ਅਚਲ NoDefault&) = ਮਿਟਾਓ;
        NoDefault& ਸੰਚਾਲਕ=(NoDefault&&) = ਮਿਟਾਓ;

        ਮਾਪ_t hash() ਅਚਲ { ਵਾਪਸ m_hash; }
        ਮਾਪ_t id() ਅਚਲ { ਵਾਪਸ m_id; }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ==(ਅਚਲ NoDefault& that) ਅਚਲ { ਵਾਪਸ m_id == that.m_id; }
    };

    static_assert(!std::is_default_constructible<NoDefault>::ਮੁੱਲ,
                  "SelfTest: 'NoDefault' must not be default constructible");
    static_assert(!std::is_copy_assignable<NoDefault>::ਮੁੱਲ,
                  "SelfTest: 'NoDefault' must not be copy assignable");
    static_assert(!std::is_move_assignable<NoDefault>::ਮੁੱਲ,
                  "SelfTest: 'NoDefault' must not be move assignable");
    static_assert(std::is_move_constructible<NoDefault>::ਮੁੱਲ,
                  "SelfTest: 'NoDefault' must be move constructible, to exercise moving");

    ਬਣਤਰ Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ NoDefault& ਮੁੱਲ) ਅਚਲ { ਵਾਪਸ ਮੁੱਲ.hash(); }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t hash, ਮਾਪ_t) ਅਚਲ { ਵਾਪਸ hash; }
    };

    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ NoDefault& a, ਅਚਲ NoDefault& b) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(a, b.hash(), b.id());
        }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ NoDefault& a, ਮਾਪ_t, ਮਾਪ_t id) ਅਚਲ { ਵਾਪਸ a.id() == id; }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<NoDefault, Hash, Equal>;

    constexpr ਮਾਪ_t N = GROWS_TWICE;
    UASSERT_SELFTEST(alive, 0);  // Nothing built yet
    {
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
            ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> added = ਸੈੱਟ.ਪਾਓ(NoDefault{&alive, 7, i});
            UASSERT_SELFTEST(added.ਦੂਜਾ, ਸੱਚ);  // Ids differ, so each is added
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots
        // All of them hash the same, so they are all in the one probe run
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮਾਪ_t{7}, i);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Reachable through the run
            UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
        }
        // A rejected insert constructs no entry: the argument is only copied on a miss
        {
            ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> dup = ਸੈੱਟ.ਪਾਓ(NoDefault{&alive, 7, 0});
            UASSERT_SELFTEST(dup.ਦੂਜਾ, ਝੂਠ);  // Not added, as an equal is present
            UASSERT_SELFTEST(dup.ਪਹਿਲਾ->id(), 0);  // The iterator is at the stored one
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // Nothing was added
        UASSERT_SELFTEST(alive, N);  // And no copy of the argument was kept
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮਾਪ_t{7}, ਮਾਪ_t{0});
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Present before the erase
            UASSERT_SELFTEST(it->id(), ਮਾਪ_t{0});  // And is the entry asked for
            ਸੈੱਟ.erase(it);
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N - 1);  // One fewer entry
        UASSERT_SELFTEST(alive, N - 1);  // And one fewer live object
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(ਮਾਪ_t{7}, ਮਾਪ_t{0}), ਝੂਠ);  // Namely that one
        // Erasing the rest keeps the live entries in step with the slots, all the way down.
        // They all collide, so every erase shifts entries back over the hole.
        ਲਈ (ਮਾਪ_t i = 1; i < N; ++i) {
            {
                ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮਾਪ_t{7}, i);
                UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Still reachable
                UASSERT_SELFTEST(it->id(), i);  // And is the entry asked for
                ਸੈੱਟ.erase(it);
            }
            UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N - 1 - i);  // The count follows the erases
            UASSERT_SELFTEST(alive, N - 1 - i);  // As do the live entries
            ਲਈ (ਮਾਪ_t j = i + 1; j < N; ++j) {
                UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(ਮਾਪ_t{7}, j), ਸੱਚ);  // Shifted, not lost
            }
        }
        UASSERT_SELFTEST(ਸੈੱਟ.begin() == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Erased down to empty
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        // Clearing destroys every entry at once, and leaves a usable, empty set
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) ਸੈੱਟ.ਪਾਓ(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Filled up again
        ਸੈੱਟ.clear();
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 0);  // Nothing left
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Says so
        UASSERT_SELFTEST(ਸੈੱਟ.begin() == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Iterates nothing
        UASSERT_SELFTEST(alive, 0);  // With every entry destroyed
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(ਮਾਪ_t{7}, ਮਾਪ_t{0}), ਝੂਠ);  // Nor finds any
        ਸੈੱਟ.clear();  // Clearing an empty set does nothing
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Still empty
        // Leave entries in the set, so its destructor has some to destroy
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) ਸੈੱਟ.ਪਾਓ(NoDefault{&alive, 7, i});
        UASSERT_SELFTEST(alive, N);  // Live as the set goes out of scope
    }
    // The set is gone, so every entry it still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
    // Clearing a set that never allocated a table does nothing
    {
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਸੈੱਟ.clear();
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // Still empty
        ਸੈੱਟ.ਪਾਓ(NoDefault{&alive, 7, 0});
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 1);  // And still usable
    }
    UASSERT_SELFTEST(alive, 0);  // Leaking nothing
}

//######################################################################
// A table can be moved, handing over the entries and leaving an empty table behind

ਖਾਲੀ testMove() {
    ਮਾਪ_t alive = 0;  // Number of live entries, which the entries themselves count

    // A test entry that counts the live ones, so a move that copied an entry, dropped
    // one, or destroyed one twice, shows up in the count
    ਬਣਤਰ Counted final {
        ਮਾਪ_t* m_alivep;  // Where the live entries are counted
        ਮਾਪ_t m_id;  // Entries with equal ids are equal

        Counted(ਮਾਪ_t* alivep, ਮਾਪ_t id)
            : m_alivep{alivep}
            , m_id{id} {
            ++*m_alivep;
        }
        Counted(ਅਚਲ Counted& that)
            : Counted{that.m_alivep, that.m_id} {}
        Counted(Counted&& that)
            : Counted{that.m_alivep, that.m_id} {}
        ~Counted() { --*m_alivep; }
        Counted& ਸੰਚਾਲਕ=(ਅਚਲ Counted&) = ਮਿਟਾਓ;
        Counted& ਸੰਚਾਲਕ=(Counted&&) = ਮਿਟਾਓ;
    };

    ਬਣਤਰ Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Counted& ਇੰਦਰਾਜ) ਅਚਲ { ਵਾਪਸ ਸੰਚਾਲਕ()(ਇੰਦਰਾਜ.m_id); }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t id) ਅਚਲ { ਵਾਪਸ id; }
    };

    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ Counted& a, ਅਚਲ Counted& b) ਅਚਲ { ਵਾਪਸ ਸੰਚਾਲਕ()(a, b.m_id); }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ Counted& a, ਮਾਪ_t id) ਅਚਲ { ਵਾਪਸ a.m_id == id; }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<Counted, Hash, Equal>;

    constexpr ਮਾਪ_t N = GROWS_TWICE;
    {
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) ਸੈੱਟ.ਪਾਓ(Counted{&alive, i});
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // All of them are in
        UASSERT_SELFTEST(alive, N);  // Held by exactly that many slots

        // Move construction takes the entries, making and destroying none
        ਸੈੱਟ_2 moved{std::move(ਸੈੱਟ)};
        UASSERT_SELFTEST(moved.ਮਾਪ(), N);  // Which the target now holds
        UASSERT_SELFTEST(alive, N);  // With no entry made or destroyed
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // And the source no longer holds them
        UASSERT_SELFTEST(ਸੈੱਟ.begin() == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // So it iterates nothing
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
            UASSERT_SELFTEST(moved.ਸ਼ਾਮਲ_ਹੈ(i), ਸੱਚ);  // Every entry came across
            UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(i), ਝੂਠ);  // And none stayed behind
        }

        // The moved from table is empty rather than broken, so it can be filled again
        UASSERT_SELFTEST(ਸੈੱਟ.ਪਾਓ(Counted{&alive, N}).ਦੂਜਾ, ਸੱਚ);  // It took an entry
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 1);  // Which is all it holds
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਝੂਠ);  // So it is no longer empty
        UASSERT_SELFTEST(alive, N + 1);  // And is one more live entry

        // Move assignment destroys what the target held, then takes the source's
        ਸੈੱਟ = std::move(moved);
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // The target holds the moved entries
        UASSERT_SELFTEST(alive, N);  // The entry it held itself was destroyed
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(N), ਝੂਠ);  // Namely that one
        UASSERT_SELFTEST(moved.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // And the source is empty again
        ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(i), ਸੱਚ);  // The rest moved
    }
    // Both tables are gone, so every entry either still held has been destroyed
    UASSERT_SELFTEST(alive, 0);  // Leaking none of them
}

//######################################################################
// Entries that cannot be copied, only moved

ਖਾਲੀ testMoveOnlyEntries() {
    // Only 'insertLazy' can add one of these, as 'insert' would copy it, and any copy the
    // container made of an entry would stop this compiling.
    ਵਰਗ MoveOnly final {
        ਮਾਪ_t m_id;  // Entries with equal ids are equal

    ਜਨਤਕ:
        ਸਪਸ਼ਟ MoveOnly(ਮਾਪ_t id)
            : m_id{id} {}
        MoveOnly(MoveOnly&&) = ਮੂਲ;
        MoveOnly(ਅਚਲ MoveOnly&) = ਮਿਟਾਓ;
        MoveOnly& ਸੰਚਾਲਕ=(ਅਚਲ MoveOnly&) = ਮਿਟਾਓ;
        MoveOnly& ਸੰਚਾਲਕ=(MoveOnly&&) = ਮਿਟਾਓ;
        ~MoveOnly() = ਮੂਲ;

        ਮਾਪ_t id() ਅਚਲ { ਵਾਪਸ m_id; }
    };

    static_assert(!std::is_copy_constructible<MoveOnly>::ਮੁੱਲ,
                  "SelfTest: 'MoveOnly' must not be copy constructible");
    static_assert(std::is_move_constructible<MoveOnly>::ਮੁੱਲ,
                  "SelfTest: 'MoveOnly' must be move constructible");

    ਬਣਤਰ Hash final {
        // Every entry hashes the same, so they all end up in the one probe run
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ MoveOnly& ਮੁੱਲ) ਅਚਲ { ਵਾਪਸ ਸੰਚਾਲਕ()(ਮੁੱਲ.id()); }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t) ਅਚਲ { ਵਾਪਸ 7; }
    };

    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ MoveOnly& a, ਅਚਲ MoveOnly& b) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(a, b.id());
        }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ MoveOnly& a, ਮਾਪ_t id) ਅਚਲ { ਵਾਪਸ a.id() == id; }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<MoveOnly, Hash, Equal>;

    constexpr ਮਾਪ_t N = GROWS_TWICE;
    ਸੈੱਟ_2 ਸੈੱਟ;
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> added
            = ਸੈੱਟ.ਪਾਓ_Lazy(i, [i] { ਵਾਪਸ MoveOnly{i}; });
        UASSERT_SELFTEST(added.ਦੂਜਾ, ਸੱਚ);  // Ids differ, so each is added
        UASSERT_SELFTEST(added.ਪਹਿਲਾ->id(), i);  // Moved into the slot, never copied
    }
    UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // All of them are in
    // They all collide, so erasing every other one shifts the rest back over the holes
    ਲਈ (ਮਾਪ_t i = 0; i < N; i += 2) {
        ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(i);
        UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Present before the erase
        ਸੈੱਟ.erase(it);
    }
    UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N / 2);  // Half of them are gone
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ ਤਾਰਕਿਕ erased = (i % 2) == 0;
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(i), !erased);  // And it is the right half
    }
}

//######################################################################
// Stateful functors, which the two argument constructor moves in

ਖਾਲੀ testStatefulFunctors() {
    // Hashing and comparison that both depend on a mask the functor holds, so the set only
    // works if it keeps the instances it was handed
    ਵਰਗ Hash final {
        ਮਾਪ_t m_ਨਕਾਬ;  // Only these bits of an entry matter

    ਜਨਤਕ:
        ਸਪਸ਼ਟ Hash(ਮਾਪ_t ਨਕਾਬ)
            : m_ਨਕਾਬ{ਨਕਾਬ} {}
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t ਮੁੱਲ) ਅਚਲ { ਵਾਪਸ ਮੁੱਲ & m_ਨਕਾਬ; }
    };

    ਵਰਗ Equal final {
        ਮਾਪ_t m_ਨਕਾਬ;  // Only these bits of an entry matter

    ਜਨਤਕ:
        ਸਪਸ਼ਟ Equal(ਮਾਪ_t ਨਕਾਬ)
            : m_ਨਕਾਬ{ਨਕਾਬ} {}
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਮਾਪ_t a, ਮਾਪ_t b) ਅਚਲ { ਵਾਪਸ (a & m_ਨਕਾਬ) == (b & m_ਨਕਾਬ); }
    };

    // Neither is default constructible, so the set cannot make its own
    static_assert(!std::is_default_constructible<Hash>::ਮੁੱਲ,
                  "SelfTest: 'Hash' must not be default constructible");
    static_assert(!std::is_default_constructible<Equal>::ਮੁੱਲ,
                  "SelfTest: 'Equal' must not be default constructible");

    // Only the low two bits matter, so the 16 entries fall into 4 classes
    V3_Hash_ਸੈੱਟ<ਮਾਪ_t, Hash, Equal> ਸੈੱਟ{Hash{3}, Equal{3}};
    ਲਈ (ਮਾਪ_t i = 0; i < 16; ++i) ਸੈੱਟ.ਪਾਓ(i);
    UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 4);  // So the set kept the functors it was given
    // Every entry finds the first one added of its class, which is the class itself
    ਲਈ (ਮਾਪ_t i = 0; i < 16; ++i) {
        ਅਚਲ V3_Hash_ਸੈੱਟ<ਮਾਪ_t, Hash, Equal>::iterator it = ਸੈੱਟ.ਲੱਭੋ(i);
        UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Its class is present
        UASSERT_SELFTEST(*it, (i & 3));  // Represented by the first one added
    }
}

//######################################################################
// Backward shift deletion moves back exactly the entries whose probe run crosses the
// hole: an entry standing at its home position past the hole must stay put

ਖਾਲੀ test_Backward_ਖਿਸਕਾਓ_Deletion() {
    // A test entry with an explicit hash, so probe runs can be laid out at will
    ਬਣਤਰ ਮੁੱਲ_2 final {
        ਮਾਪ_t m_hash = 0;  // Hash of this entry
        ਮਾਪ_t m_id = 0;  // Entries with equal ids are equal
    };

    ਬਣਤਰ Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* valuep) ਅਚਲ { ਵਾਪਸ ਸੰਚਾਲਕ()(valuep->m_hash, 0); }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t hash, ਮਾਪ_t) ਅਚਲ { ਵਾਪਸ hash; }
    };

    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* ap, ਅਚਲ ਮੁੱਲ_2* bp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(ap, bp->m_hash, bp->m_id);
        }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* valuep, ਮਾਪ_t hash, ਮਾਪ_t id) ਅਚਲ {
            ਵਾਪਸ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<ਮੁੱਲ_2*, Hash, Equal>;

    // One probe run of four entries with alternating home positions 'h' and 'h + 1',
    // occupying four adjacent slots. Erasing the first leaves a hole: the second sits
    // at its own home and must not be moved into it, while the third and fourth have
    // their runs broken by the hole and must be moved back.
    ਲਈ (ਅਚਲ ਮਾਪ_t h : {ਮਾਪ_t{0}, ਮਾਪ_t{5}, ~ਮਾਪ_t{0}}) {  // Wraps too
        std::array<ਮੁੱਲ_2, 4> ਮੁੱਲਾਂ{ਮੁੱਲ_2{h, 0}, ਮੁੱਲ_2{h + 1, 1},  //
                                    ਮੁੱਲ_2{h, 2}, ਮੁੱਲ_2{h + 1, 3}};
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਲਈ (ਮੁੱਲ_2& ਮੁੱਲ : ਮੁੱਲਾਂ) ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲ);
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 4);  // The run holds all four

        // Erase the entry at the head of the run
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(h, ਮਾਪ_t{0});
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Present before the erase
            ਸੈੱਟ.erase(it);
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 3);  // One fewer entry
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(h, ਮਾਪ_t{0}), ਝੂਠ);  // Namely that one
        // Whether moved back or left in place, every entry must remain reachable
        ਲਈ (ਮਾਪ_t i = 1; i < ਮੁੱਲਾਂ.ਮਾਪ(); ++i) {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮੁੱਲਾਂ[i].m_hash, i);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // The shift lost nothing
            UASSERT_SELFTEST(*it, &ਮੁੱਲਾਂ[i]);  // And moved back the right entries
        }

        // Erase the entry that stayed at its home position, shifting the last one again
        {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(h + 1, ਮਾਪ_t{1});
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Left where the first erase found it
            ਸੈੱਟ.erase(it);
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), 2);  // Two are left
        ਲਈ (ਮਾਪ_t i = 2; i < ਮੁੱਲਾਂ.ਮਾਪ(); ++i) {
            ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮੁੱਲਾਂ[i].m_hash, i);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Both still reachable
            UASSERT_SELFTEST(*it, &ਮੁੱਲਾਂ[i]);  // And are the entries they were
        }
        // Iterating visits exactly the remaining entries
        std::array<ਤਾਰਕਿਕ, 4> ਵੇਖਿਆ{};
        ਲਈ (ਅਚਲ ਮੁੱਲ_2* ਅਚਲ valuep : ਸੈੱਟ) {
            UASSERT_SELFTEST(ਵੇਖਿਆ[valuep->m_id], ਝੂਠ);  // Each entry once
            ਵੇਖਿਆ[valuep->m_id] = ਸੱਚ;
        }
        UASSERT_SELFTEST(ਵੇਖਿਆ[0], ਝੂਠ);  // Erased first
        UASSERT_SELFTEST(ਵੇਖਿਆ[1], ਝੂਠ);  // Erased second
        UASSERT_SELFTEST(ਵੇਖਿਆ[2], ਸੱਚ);  // Moved back over the first hole
        UASSERT_SELFTEST(ਵੇਖਿਆ[3], ਸੱਚ);  // And back again over the second
    }
}

//######################################################################
// Entries stay in place unless the table grows or an entry is erased, as only those
// two invalidate iterators

ਖਾਲੀ testReferenceStability() {
    ਬਣਤਰ ਮੁੱਲ_2 final {
        ਮਾਪ_t m_hash = 0;  // Hash of this entry
        ਮਾਪ_t m_id = 0;  // Id of this entry
    };

    ਬਣਤਰ Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* valuep) ਅਚਲ { ਵਾਪਸ ਸੰਚਾਲਕ()(valuep->m_hash, 0); }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t hash, ਮਾਪ_t) ਅਚਲ { ਵਾਪਸ hash; }
    };

    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* ap, ਅਚਲ ਮੁੱਲ_2* bp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(ap, bp->m_hash, bp->m_id);
        }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ਮੁੱਲ_2* valuep, ਮਾਪ_t hash, ਮਾਪ_t id) ਅਚਲ {
            ਵਾਪਸ valuep->m_hash == hash && valuep->m_id == id;
        }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<ਮੁੱਲ_2*, Hash, Equal>;

    // A reserved but still empty set finds nothing and iterates nothing
    {
        ਸੈੱਟ_2 ਸੈੱਟ;
        ਸੈੱਟ.reserve(8);
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(ਮਾਪ_t{7}, ਮਾਪ_t{0}), ਝੂਠ);  // Room, but no entry
        UASSERT_SELFTEST(ਸੈੱਟ.begin() == ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // So iterates nothing
        UASSERT_SELFTEST(ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ(), ਸੱਚ);  // And holds nothing
    }

    // 'N' is exactly what the reservation must hold, so this also checks the boundary
    // arithmetic of 'reserve' against that of the growth check. It is more than an
    // unreserved table holds without growing, so the reservation is doing the work.
    constexpr ਮਾਪ_t N = max_ਲੱਦੋ(2 * MIN_ਸਮਰੱਥਾ);
    static_assert(N > max_ਲੱਦੋ(MIN_ਸਮਰੱਥਾ), "SelfTest: 'N' must need more than a new table");
    std::array<ਮੁੱਲ_2, N> ਮੁੱਲਾਂ{};
    std::array<ਮੁੱਲ_2* ਅਚਲ*, N> entrypps{};  // Where each entry is stored, as inserted
    ਸੈੱਟ_2 ਸੈੱਟ;
    ਸੈੱਟ.reserve(N);
    // All entries collide, so every insertion probes through the whole existing run
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਮੁੱਲਾਂ[i] = ਮੁੱਲ_2{7, i};
        ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> pair = ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲਾਂ[i]);
        UASSERT_SELFTEST(pair.ਦੂਜਾ, ਸੱਚ);  // Ids differ, so each is added
        entrypps[i] = &*pair.ਪਹਿਲਾ;
    }
    // The set was reserved, so no insertion grew the table, and no entry has moved
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮਾਪ_t{7}, i);
        UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, entrypps[i]);  // In the slot it was put in
    }
    // Inserting entries that are present adds nothing and moves nothing
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ std::pair<ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> pair = ਸੈੱਟ.ਪਾਓ(&ਮੁੱਲਾਂ[i]);
        UASSERT_SELFTEST(pair.ਦੂਜਾ, ਝੂਠ);  // Rejected, as it is present
        UASSERT_SELFTEST(&*pair.ਪਹਿਲਾ, entrypps[i]);  // And nothing moved
    }
    UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // No duplicate was added
    // Growing an occupied set through 'reserve' keeps every entry
    ਸੈੱਟ.reserve(8 * N);
    UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // Rehashing dropped nothing
    std::array<ਮੁੱਲ_2* ਅਚਲ*, N> grownpps{};  // Where each entry is after the growth
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮਾਪ_t{7}, i);
        UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // And left every entry reachable
        grownpps[i] = &*it;
    }
    // Reserving room that is there already leaves the table alone, so nothing moves
    ਸੈੱਟ.reserve(N);
    UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), N);  // The smaller request changed nothing
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(ਮਾਪ_t{7}, i);
        UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), ਸੱਚ);  // Every entry is still there
        UASSERT_SELFTEST(&*it, grownpps[i]);  // In the slot the growth left it in
    }
}

//######################################################################
// A pseudo random workload checked against std::set or std::map as the reference model

// Only a few distinct hashes, so probe runs are long and every erase shifts entries
ਬਣਤਰ ClusteredHash final {
    ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t ਮੁੱਲ) ਅਚਲ { ਵਾਪਸ ਮੁੱਲ & 0x7; }
};

// Hashes spread by a large odd multiplier, so most probe runs are short
ਬਣਤਰ ਫੈਲਾਅ_Hash final {
    ਮਾਪ_t ਸੰਚਾਲਕ()(ਮਾਪ_t ਮੁੱਲ) ਅਚਲ {
        ਵਾਪਸ static_cast<ਮਾਪ_t>(ਮੁੱਲ * 0x9e3779b97f4a7c15ULL);
    }
};

// Drive a set through a deterministic pseudo random workload of insertions, erasures,
// and lookups, checking every step against a std::set holding the same entries
ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Hash>
ਖਾਲੀ test_ਸੈੱਟ_Against_Model() {
    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਮਾਪ_t a, ਮਾਪ_t b) ਅਚਲ { ਵਾਪਸ a == b; }
    };

    ਵਰਤੋਂ ਸੈੱਟ_2 = V3_Hash_ਸੈੱਟ<ਮਾਪ_t, T_Hash, Equal>;

    constexpr ਮਾਪ_t UNIVERSE = 64;  // Entries drawn from a small range, so lookups hit
    constexpr ਮਾਪ_t ਕਦਮਾਂ = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x123456789abcdef0ULL;
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਅਗਲਾ_Rand = [&state]() -> ਮਾਪ_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ਵਾਪਸ static_cast<ਮਾਪ_t>(state >> 32);
    };

    ਸੈੱਟ_2 ਸੈੱਟ;
    std::ਸੈੱਟ<ਮਾਪ_t> model;

    ਲਈ (ਮਾਪ_t ਕਦਮ = 0; ਕਦਮ < ਕਦਮਾਂ; ++ਕਦਮ) {
        ਅਚਲ ਮਾਪ_t id = ਅਗਲਾ_Rand() % UNIVERSE;
        ਚੋਣ (ਅਗਲਾ_Rand() % 4) {
        ਹਾਲਤ 0: {  // Insert a copy
            ਅਚਲ std::pair<ਕਿਸਮ_ਨਾਂ ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> pair = ਸੈੱਟ.ਪਾਓ(id);
            UASSERT_SELFTEST(pair.ਦੂਜਾ, model.ਪਾਓ(id).ਦੂਜਾ);  // As the model
            UASSERT_SELFTEST(*pair.ਪਹਿਲਾ, id);  // At the entry asked for
            ਰੋਕੋ;
        }
        ਹਾਲਤ 1: {  // Insert lazily, which must create only on a miss
            ਤਾਰਕਿਕ created = ਝੂਠ;
            ਅਚਲ std::pair<ਕਿਸਮ_ਨਾਂ ਸੈੱਟ_2::iterator, ਤਾਰਕਿਕ> pair
                = ਸੈੱਟ.ਪਾਓ_Lazy(id, [&]() -> ਮਾਪ_t {
                      created = ਸੱਚ;
                      ਵਾਪਸ id;
                  });
            UASSERT_SELFTEST(pair.ਦੂਜਾ, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.ਦੂਜਾ, model.ਪਾਓ(id).ਦੂਜਾ);  // As the model
            UASSERT_SELFTEST(*pair.ਪਹਿਲਾ, id);  // At the entry asked for
            ਰੋਕੋ;
        }
        ਹਾਲਤ 2: {  // Erase, if present
            ਅਚਲ ਕਿਸਮ_ਨਾਂ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(id);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), model.ਗਿਣਤੀ_2(id) != 0);  // As the model
            ਜੇ (it != ਸੈੱਟ.ਸਿਰਾ()) {
                ਸੈੱਟ.erase(it);
                model.erase(id);
            }
            ਰੋਕੋ;
        }
        ਮੂਲ: {  // Look up only
            ਅਚਲ ਕਿਸਮ_ਨਾਂ ਸੈੱਟ_2::iterator it = ਸੈੱਟ.ਲੱਭੋ(id);
            UASSERT_SELFTEST(it != ਸੈੱਟ.ਸਿਰਾ(), model.ਗਿਣਤੀ_2(id) != 0);  // As the model
            ਜੇ (it != ਸੈੱਟ.ਸਿਰਾ()) UASSERT_SELFTEST(*it, id);  // And is the entry
            ਰੋਕੋ;
        }
        }
        UASSERT_SELFTEST(ਸੈੱਟ.ਮਾਪ(), model.ਮਾਪ());  // Every step, not just at the end
    }

    // Check the final content exhaustively
    ਲਈ (ਮਾਪ_t id = 0; id < UNIVERSE; ++id) {
        UASSERT_SELFTEST(ਸੈੱਟ.ਸ਼ਾਮਲ_ਹੈ(id), model.ਗਿਣਤੀ_2(id) != 0);  // Present iff modelled
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<ਤਾਰਕਿਕ, UNIVERSE> ਵੇਖਿਆ{};
    ਮਾਪ_t n = 0;
    ਲਈ (ਅਚਲ ਮਾਪ_t ਇੰਦਰਾਜ : ਸੈੱਟ) {
        UASSERT_SELFTEST(ਇੰਦਰਾਜ < UNIVERSE, ਸੱਚ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ਗਿਣਤੀ_2(ਇੰਦਰਾਜ) != 0, ਸੱਚ);  // Only what was inserted
        UASSERT_SELFTEST(ਵੇਖਿਆ[ਇੰਦਰਾਜ], ਝੂਠ);  // Each entry once
        ਵੇਖਿਆ[ਇੰਦਰਾਜ] = ਸੱਚ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.ਮਾਪ());  // And all of them
}

//######################################################################
// A hash table used as a map, whose entries are plain key and value pairs

ਖਾਲੀ testMap() {
    // The key extractor means the hash and equality see only keys, never entries, so
    // the standard functors do, and they are what V3HashMap defaults to
    ਵਰਤੋਂ Map = V3HashMap<std::string, ਪੂਰਨ>;
    ਵਰਤੋਂ ਇੰਦਰਾਜ_2 = Map::ਇੰਦਰਾਜ_2;

    // A map entry is a plain pair, so a caller never meets an internal type
    static_assert(std::is_same<ਇੰਦਰਾਜ_2, std::pair<std::string, ਪੂਰਨ>>::ਮੁੱਲ,
                  "SelfTest: a map entry must be a plain pair");

    constexpr ਮਾਪ_t N = GROWS_TWICE;
    Map map;
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ std::string ਕੁੰਜੀ = "key" + std::ਮੰਜ਼ਿਲ_string(i);
        // 'insertLazy' calls the factory only on a miss, so a hit builds no entry
        ਅਚਲ std::pair<Map::iterator, ਤਾਰਕਿਕ> pair
            = map.ਪਾਓ_Lazy(ਕੁੰਜੀ, [&]() -> ਇੰਦਰਾਜ_2 { ਵਾਪਸ {ਕੁੰਜੀ, static_cast<ਪੂਰਨ>(i)}; });
        UASSERT_SELFTEST(pair.ਦੂਜਾ, ਸੱਚ);  // Keys differ, so each is added
    }
    UASSERT_SELFTEST(map.ਮਾਪ(), N);  // All of them are in

    // A bare key finds the entry, with no Entry built to look it up
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ Map::iterator it = map.ਲੱਭੋ("key" + std::ਮੰਜ਼ਿਲ_string(i));
        UASSERT_SELFTEST(it != map.ਸਿਰਾ(), ਸੱਚ);  // The key alone finds it
        UASSERT_SELFTEST(it->ਦੂਜਾ, static_cast<ਪੂਰਨ>(i));  // With the value given
    }

    // A value can be changed by erasing the entry and inserting it again
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ std::string ਕੁੰਜੀ = "key" + std::ਮੰਜ਼ਿਲ_string(i);
        map.erase(map.ਲੱਭੋ(ਕੁੰਜੀ));
        UASSERT_SELFTEST(map.ਪਾਓ({ਕੁੰਜੀ, static_cast<ਪੂਰਨ>(i) + 100}).ਦੂਜਾ, ਸੱਚ);  // Anew
    }
    UASSERT_SELFTEST(map.ਮਾਪ(), N);  // The same keys, so the map has not grown
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ Map::iterator it = map.ਲੱਭੋ("key" + std::ਮੰਜ਼ਿਲ_string(i));
        UASSERT_SELFTEST(it != map.ਸਿਰਾ(), ਸੱਚ);  // Each key is still there
        UASSERT_SELFTEST(it->ਦੂਜਾ, static_cast<ਪੂਰਨ>(i) + 100);  // With its new value
    }

    // Or a value is changed in place, via 'value()', which leaves the key alone
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) map.ਲੱਭੋ("key" + std::ਮੰਜ਼ਿਲ_string(i)).ਮੁੱਲ() += 100;
    UASSERT_SELFTEST(map.ਮਾਪ(), N);  // The map is unchanged otherwise
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) {
        ਅਚਲ Map::iterator it = map.ਲੱਭੋ("key" + std::ਮੰਜ਼ਿਲ_string(i));
        UASSERT_SELFTEST(it != map.ਸਿਰਾ(), ਸੱਚ);  // Each key still finds its entry
        UASSERT_SELFTEST(it->ਦੂਜਾ, static_cast<ਪੂਰਨ>(i) + 200);  // With its new value
    }
    // And back, as the checks below expect the earlier values
    ਲਈ (ਮਾਪ_t i = 0; i < N; ++i) map.ਲੱਭੋ("key" + std::ਮੰਜ਼ਿਲ_string(i)).ਮੁੱਲ() -= 100;

    // Adding a key that is present changes nothing
    {
        ਅਚਲ std::string ਕੁੰਜੀ = "key0";
        ਤਾਰਕਿਕ created = ਝੂਠ;
        ਅਚਲ std::pair<Map::iterator, ਤਾਰਕਿਕ> pair = map.ਪਾਓ_Lazy(ਕੁੰਜੀ, [&]() -> ਇੰਦਰਾਜ_2 {
            created = ਸੱਚ;  // LCOV_EXCL_START
            ਵਾਪਸ {ਕੁੰਜੀ, 0};  // LCOV_EXCL_STOP
        });
        UASSERT_SELFTEST(created, ਝੂਠ);  // The factory was never called
        UASSERT_SELFTEST(pair.ਦੂਜਾ, ਝੂਠ);  // As the key is present
        UASSERT_SELFTEST(pair.ਪਹਿਲਾ->ਦੂਜਾ, 100);  // Holding its old value
    }
    UASSERT_SELFTEST(map.ਮਾਪ(), N);  // And nothing was added

    // 'insert' adds a key and a value as a plain pair, as std::unordered_map::insert does
    {
        ਅਚਲ std::pair<Map::iterator, ਤਾਰਕਿਕ> added = map.ਪਾਓ({std::string{"fresh"}, 5});
        UASSERT_SELFTEST(added.ਦੂਜਾ, ਸੱਚ);  // Added, as the key is new
        UASSERT_SELFTEST(added.ਪਹਿਲਾ->ਦੂਜਾ, 5);  // With the value given
        ਅਚਲ std::pair<Map::iterator, ਤਾਰਕਿਕ> again = map.ਪਾਓ({std::string{"fresh"}, 6});
        UASSERT_SELFTEST(again.ਦੂਜਾ, ਝੂਠ);  // Rejected, as the key is present
        UASSERT_SELFTEST(again.ਪਹਿਲਾ->ਦੂਜਾ, 5);  // And the value is untouched
    }
    UASSERT_SELFTEST(map.ਮਾਪ(), N + 1);  // Just the one entry was added
    {
        ਅਚਲ Map::iterator it = map.ਲੱਭੋ(std::string{"fresh"});
        UASSERT_SELFTEST(it != map.ਸਿਰਾ(), ਸੱਚ);  // Present before the erase
        UASSERT_SELFTEST(it->ਦੂਜਾ, 5);  // Still holding the first value
        map.erase(it);
    }
    UASSERT_SELFTEST(map.ਮਾਪ(), N);  // Back to the earlier content

    // Erasing by key removes just that entry, and says whether it did
    UASSERT_SELFTEST(map.erase(std::string{"absent"}), ਝੂਠ);  // No such key
    UASSERT_SELFTEST(map.ਮਾਪ(), N);  // So nothing was erased
    UASSERT_SELFTEST(map.erase(std::string{"key0"}), ਸੱਚ);  // That key is present
    UASSERT_SELFTEST(map.ਮਾਪ(), N - 1);  // And just the one entry went
    UASSERT_SELFTEST(map.ਸ਼ਾਮਲ_ਹੈ(std::string{"key0"}), ਝੂਠ);  // Namely that one
    ਲਈ (ਮਾਪ_t i = 1; i < N; ++i) {
        UASSERT_SELFTEST(map.ਸ਼ਾਮਲ_ਹੈ("key" + std::ਮੰਜ਼ਿਲ_string(i)), ਸੱਚ);  // Others intact
    }
}

//######################################################################
// The pseudo random workload again, run against a map and checked against std::map

// Drive a map through a deterministic pseudo random workload of insertions, erasures,
// value changes and lookups, checking every step against a std::map holding the same
ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Hash>
ਖਾਲੀ testMapAgainstModel() {
    ਬਣਤਰ Equal final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਮਾਪ_t a, ਮਾਪ_t b) ਅਚਲ { ਵਾਪਸ a == b; }
    };

    ਵਰਤੋਂ Map = V3HashMap<ਮਾਪ_t, ਮਾਪ_t, T_Hash, Equal>;
    ਵਰਤੋਂ ਇੰਦਰਾਜ_2 = ਕਿਸਮ_ਨਾਂ Map::ਇੰਦਰਾਜ_2;

    constexpr ਮਾਪ_t UNIVERSE = 64;  // Keys drawn from a small range, so lookups hit
    constexpr ਮਾਪ_t ਕਦਮਾਂ = 10000;  // Number of operations applied

    // A simple linear congruential generator, with a fixed seed so failures reproduce
    uint64_t state = 0x0fedcba987654321ULL;
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਅਗਲਾ_Rand = [&state]() -> ਮਾਪ_t {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        ਵਾਪਸ static_cast<ਮਾਪ_t>(state >> 32);
    };

    Map map;
    std::map<ਮਾਪ_t, ਮਾਪ_t> model;

    ਲਈ (ਮਾਪ_t ਕਦਮ = 0; ਕਦਮ < ਕਦਮਾਂ; ++ਕਦਮ) {
        ਅਚਲ ਮਾਪ_t ਕੁੰਜੀ = ਅਗਲਾ_Rand() % UNIVERSE;
        ਅਚਲ ਮਾਪ_t val = ਕਦਮ;  // Distinct every step, so a stale value is visible
        ਚੋਣ (ਅਗਲਾ_Rand() % 5) {
        ਹਾਲਤ 0: {  // Insert a whole entry
            ਅਚਲ std::pair<ਕਿਸਮ_ਨਾਂ Map::iterator, ਤਾਰਕਿਕ> pair = map.ਪਾਓ({ਕੁੰਜੀ, val});
            UASSERT_SELFTEST(pair.ਦੂਜਾ, model.ਪਾਓ({ਕੁੰਜੀ, val}).ਦੂਜਾ);  // Ditto
            UASSERT_SELFTEST(pair.ਪਹਿਲਾ->ਪਹਿਲਾ, ਕੁੰਜੀ);  // At the key asked for
            ਰੋਕੋ;
        }
        ਹਾਲਤ 1: {  // Insert lazily, which must create only on a miss
            ਤਾਰਕਿਕ created = ਝੂਠ;
            ਅਚਲ std::pair<ਕਿਸਮ_ਨਾਂ Map::iterator, ਤਾਰਕਿਕ> pair
                = map.ਪਾਓ_Lazy(ਕੁੰਜੀ, [&]() -> ਇੰਦਰਾਜ_2 {
                      created = ਸੱਚ;
                      ਵਾਪਸ {ਕੁੰਜੀ, val};
                  });
            UASSERT_SELFTEST(pair.ਦੂਜਾ, created);  // Created only on a miss
            UASSERT_SELFTEST(pair.ਦੂਜਾ, model.ਪਾਓ({ਕੁੰਜੀ, val}).ਦੂਜਾ);  // Ditto
            UASSERT_SELFTEST(pair.ਪਹਿਲਾ->ਪਹਿਲਾ, ਕੁੰਜੀ);  // At the key asked for
            ਰੋਕੋ;
        }
        ਹਾਲਤ 2: {  // Erase by key, if present
            UASSERT_SELFTEST(map.erase(ਕੁੰਜੀ), model.erase(ਕੁੰਜੀ) != 0);  // As model
            ਰੋਕੋ;
        }
        ਹਾਲਤ 3: {  // Change a value, which is erasing the entry and inserting it again
            ਅਚਲ ਕਿਸਮ_ਨਾਂ Map::iterator it = map.ਲੱਭੋ(ਕੁੰਜੀ);
            ਜੇ (it != map.ਸਿਰਾ()) {
                map.erase(it);
                UASSERT_SELFTEST(map.ਪਾਓ({ਕੁੰਜੀ, val}).ਦੂਜਾ, ਸੱਚ);  // The key is free
                model[ਕੁੰਜੀ] = val;
            }
            ਰੋਕੋ;
        }
        ਮੂਲ: {  // Look up only
            ਅਚਲ ਕਿਸਮ_ਨਾਂ Map::iterator it = map.ਲੱਭੋ(ਕੁੰਜੀ);
            UASSERT_SELFTEST(it != map.ਸਿਰਾ(), model.ਗਿਣਤੀ_2(ਕੁੰਜੀ) != 0);  // As the model
            ਜੇ (it != map.ਸਿਰਾ()) {
                UASSERT_SELFTEST(it->ਪਹਿਲਾ, ਕੁੰਜੀ);  // At the key asked for
                UASSERT_SELFTEST(it->ਦੂਜਾ, model.at(ਕੁੰਜੀ));  // With its value
            }
            ਰੋਕੋ;
        }
        }
        UASSERT_SELFTEST(map.ਮਾਪ(), model.ਮਾਪ());  // Every step, not just at the end
    }

    // Check the final content exhaustively, values included
    ਲਈ (ਮਾਪ_t ਕੁੰਜੀ = 0; ਕੁੰਜੀ < UNIVERSE; ++ਕੁੰਜੀ) {
        ਅਚਲ ਕਿਸਮ_ਨਾਂ Map::iterator it = map.ਲੱਭੋ(ਕੁੰਜੀ);
        UASSERT_SELFTEST(it != map.ਸਿਰਾ(), model.ਗਿਣਤੀ_2(ਕੁੰਜੀ) != 0);  // Present iff modelled
        ਜੇ (it != map.ਸਿਰਾ()) {
            UASSERT_SELFTEST(it->ਦੂਜਾ, model.at(ਕੁੰਜੀ));  // With the right value
        }
    }
    // And that iterating visits exactly the model content, each entry once
    std::array<ਤਾਰਕਿਕ, UNIVERSE> ਵੇਖਿਆ{};
    ਮਾਪ_t n = 0;
    ਲਈ (ਅਚਲ ਇੰਦਰਾਜ_2& ਇੰਦਰਾਜ : map) {
        UASSERT_SELFTEST(ਇੰਦਰਾਜ.ਪਹਿਲਾ < UNIVERSE, ਸੱਚ);  // Nothing out of thin air
        UASSERT_SELFTEST(model.ਗਿਣਤੀ_2(ਇੰਦਰਾਜ.ਪਹਿਲਾ) != 0, ਸੱਚ);  // Only what was inserted
        UASSERT_SELFTEST(ਇੰਦਰਾਜ.ਦੂਜਾ, model.at(ਇੰਦਰਾਜ.ਪਹਿਲਾ));  // With the right value
        UASSERT_SELFTEST(ਵੇਖਿਆ[ਇੰਦਰਾਜ.ਪਹਿਲਾ], ਝੂਠ);  // Each entry once
        ਵੇਖਿਆ[ਇੰਦਰਾਜ.ਪਹਿਲਾ] = ਸੱਚ;
        ++n;
    }
    UASSERT_SELFTEST(n, model.ਮਾਪ());  // And all of them
}

ਖਾਲੀ selfTest() {
    test_ਮੁੱਲ_ਕੁੰਜੀਆਂ();
    test_ਸੂਚਕ_ਕੁੰਜੀਆਂ();
    test_ਇੰਦਰਾਜ_Lifetime();
    testMove();
    testMoveOnlyEntries();
    testStatefulFunctors();
    test_Backward_ਖਿਸਕਾਓ_Deletion();
    testReferenceStability();
    test_ਸੈੱਟ_Against_Model<ClusteredHash>();
    test_ਸੈੱਟ_Against_Model<ਫੈਲਾਅ_Hash>();
    testMap();
    testMapAgainstModel<ClusteredHash>();
    testMapAgainstModel<ਫੈਲਾਅ_Hash>();
}

}  // namespace V3HashTableInternals
