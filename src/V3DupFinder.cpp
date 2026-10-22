// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Hashed common code into functions
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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3DupFinder.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::ਮਾਪ_type V3DupFinder::erase(Ast_ਗੰਢ* nodep) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ& er = equal_range(m_hasher(nodep));
    ਲਈ (iterator it = er.ਪਹਿਲਾ; it != er.ਦੂਜਾ; ++it) {
        ਜੇ (nodep == it->ਦੂਜਾ) {
            erase(it);
            ਵਾਪਸ 1;
        }
    }
    ਵਾਪਸ 0;
}

V3DupFinder::iterator V3DupFinder::ਲੱਭੋ_Duplicate(Ast_ਗੰਢ* nodep, V3DupFinderUserSame* checkp) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ& er = equal_range(m_hasher(nodep));
    ਲਈ (iterator it = er.ਪਹਿਲਾ; it != er.ਦੂਜਾ; ++it) {
        Ast_ਗੰਢ* ਅਚਲ node2p = it->ਦੂਜਾ;
        ਜੇ (nodep == node2p) ਜਾਰੀ;  // Same node is not a duplicate
        ਜੇ (checkp && !checkp->isSame(nodep, node2p)) ਜਾਰੀ;  // User says it is not a duplicate
        ਜੇ (!nodep->same_ਰੁੱਖ(node2p)) ਜਾਰੀ;  // Not the same trees
        // Found duplicate
        ਵਾਪਸ it;
    }
    ਵਾਪਸ ਸਿਰਾ();
}

ਖਾਲੀ V3DupFinder::dumpFile(ਅਚਲ string& filename, ਤਾਰਕਿਕ ਰੁੱਖ) {
    UINFO(2, "Dumping " << filename);
    ਅਚਲ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ਜੇ (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<ਪੂਰਨ, ਪੂਰਨ> dist;

    V3Hash lasthash;
    ਪੂਰਨ ਅੰਕ_ਵਿੱਚ_ਬਾਲਟੀ = 0;
    ਲਈ (ਸਵੈਚਾਲਿਤ it = cbegin(); ਸੱਚ; ++it) {
        ਜੇ (it == cend() || lasthash != it->ਪਹਿਲਾ) {
            ਜੇ (it != cend()) lasthash = it->ਪਹਿਲਾ;
            ਜੇ (ਅੰਕ_ਵਿੱਚ_ਬਾਲਟੀ) ++dist[ਅੰਕ_ਵਿੱਚ_ਬਾਲਟੀ];
            ਅੰਕ_ਵਿੱਚ_ਬਾਲਟੀ = 0;
        }
        ਜੇ (it == cend()) ਰੋਕੋ;
        ਅੰਕ_ਵਿੱਚ_ਬਾਲਟੀ++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : dist) {
        *logp << "    " << std::setw(9) << i.ਪਹਿਲਾ << "  " << std::setw(12) << i.ਦੂਜਾ << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : *ਇਹ) {
        ਜੇ (lasthash != it.ਪਹਿਲਾ) {
            lasthash = it.ਪਹਿਲਾ;
            *logp << "    " << it.ਪਹਿਲਾ << '\n';
        }
        *logp << "\t" << it.ਦੂਜਾ << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        ਜੇ (ਰੁੱਖ) it.ਦੂਜਾ->dump_ਰੁੱਖ(*logp, "    ");
    }
}

ਖਾਲੀ V3DupFinder::dumpFilePrefixed(ਅਚਲ string& nameComment, ਤਾਰਕਿਕ ਰੁੱਖ) {
    ਜੇ (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", ਰੁੱਖ);
}
