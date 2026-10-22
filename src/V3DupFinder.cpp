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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3DupFinder.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"

#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(Ast_ಗಂಟು* nodep) {
    ಧ್ರುವ auto& er = equal_range(m_hasher(nodep));
    ಪ್ರತಿ (iterator it = er.first; it != er.second; ++it) {
        ಯದಿ (nodep == it->second) {
            erase(it);
            ಪ್ರತ್ಯಾಗಮ 1;
        }
    }
    ಪ್ರತ್ಯಾಗಮ 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_ಗಂಟು* nodep, V3DupFinderUserSame* checkp) {
    ಧ್ರುವ auto& er = equal_range(m_hasher(nodep));
    ಪ್ರತಿ (iterator it = er.first; it != er.second; ++it) {
        Ast_ಗಂಟು* ಧ್ರುವ node2p = it->second;
        ಯದಿ (nodep == node2p) ಅನುವರ್ತ;  // Same node is not a duplicate
        ಯದಿ (checkp && !checkp->isSame(nodep, node2p)) ಅನುವರ್ತ;  // User says it is not a duplicate
        ಯದಿ (!nodep->same_ವೃಕ್ಷ(node2p)) ಅನುವರ್ತ;  // Not the same trees
        // Found duplicate
        ಪ್ರತ್ಯಾಗಮ it;
    }
    ಪ್ರತ್ಯಾಗಮ end();
}

ಶೂನ್ಯ V3DupFinder::dumpFile(ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ ವೃಕ್ಷ) {
    UINFO(2, "Dumping " << filename);
    ಧ್ರುವ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ಯದಿ (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ> dist;

    V3Hash lasthash;
    ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_in_bucket = 0;
    ಪ್ರತಿ (auto it = cbegin(); ಸತ್ಯ; ++it) {
        ಯದಿ (it == cend() || lasthash != it->first) {
            ಯದಿ (it != cend()) lasthash = it->first;
            ಯದಿ (ಅಂಕೆ_in_bucket) ++dist[ಅಂಕೆ_in_bucket];
            ಅಂಕೆ_in_bucket = 0;
        }
        ಯದಿ (it == cend()) ಭಙ್ಗ;
        ಅಂಕೆ_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    ಪ್ರತಿ (ಧ್ರುವ auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    ಪ್ರತಿ (ಧ್ರುವ auto& it : *ಏತತ್) {
        ಯದಿ (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        ಯದಿ (ವೃಕ್ಷ) it.second->dump_ವೃಕ್ಷ(*logp, "    ");
    }
}

ಶೂನ್ಯ V3DupFinder::dumpFilePrefixed(ಧ್ರುವ string& nameComment, ತಾರ್ಕಿಕ ವೃಕ್ಷ) {
    ಯದಿ (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", ವೃಕ್ಷ);
}
