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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3DupFinder.h"

#include "V3File.h"

#include <iomanip>
#include <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(AstNode* nodep) {
    const auto& er = equal_range(m_hasher(nodep));
    for (iterator it = er.first; it != er.second; ++it) {
        ᮚᮦᮔ᮪ (nodep == it->second) {
            erase(it);
            ᮝᮜᮤ 1;
        }
    }
    ᮝᮜᮤ 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(AstNode* nodep, V3DupFinderUserSame* checkp) {
    const auto& er = equal_range(m_hasher(nodep));
    for (iterator it = er.first; it != er.second; ++it) {
        AstNode* const node2p = it->second;
        ᮚᮦᮔ᮪ (nodep == node2p) continue;  // Same node is not a duplicate
        ᮚᮦᮔ᮪ (checkp && !checkp->isSame(nodep, node2p)) continue;  // User says it is not a duplicate
        ᮚᮦᮔ᮪ (!nodep->sameTree(node2p)) continue;  // Not the same trees
        // Found duplicate
        ᮝᮜᮤ it;
    }
    ᮝᮜᮤ end();
}

void V3DupFinder::dumpFile(const string& filename, bool tree) {
    UINFO(2, "Dumping " << filename);
    const std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ᮚᮦᮔ᮪ (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<puur_naa_nka, puur_naa_nka> dist;

    V3Hash lasthash;
    puur_naa_nka num_in_bucket = 0;
    for (auto it = cbegin(); true; ++it) {
        ᮚᮦᮔ᮪ (it == cend() || lasthash != it->first) {
            ᮚᮦᮔ᮪ (it != cend()) lasthash = it->first;
            ᮚᮦᮔ᮪ (num_in_bucket) ++dist[num_in_bucket];
            num_in_bucket = 0;
        }
        ᮚᮦᮔ᮪ (it == cend()) bha_nga;
        num_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    for (const auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    for (const auto& it : *this) {
        ᮚᮦᮔ᮪ (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        ᮚᮦᮔ᮪ (tree) it.second->dumpTree(*logp, "    ");
    }
}

void V3DupFinder::dumpFilePrefixed(const string& nameComment, bool tree) {
    ᮚᮦᮔ᮪ (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", tree);
}
