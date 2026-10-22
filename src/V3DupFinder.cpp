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

V3DupFinder::ಗಾತ್ರ_type V3DupFinder::erase(Ast_ಗಂಟು* nodep) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& er = equal_range(m_hasher(nodep));
    ಗಾಗಿ (iterator it = er.ಮೊದಲ; it != er.ಎರಡನೇ; ++it) {
        ಆದರೆ (nodep == it->ಎರಡನೇ) {
            erase(it);
            ಹಿಂತಿರುಗಿ 1;
        }
    }
    ಹಿಂತಿರುಗಿ 0;
}

V3DupFinder::iterator V3DupFinder::ಕಂಡುಹಿಡಿ_Duplicate(Ast_ಗಂಟು* nodep, V3DupFinderUserSame* checkp) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& er = equal_range(m_hasher(nodep));
    ಗಾಗಿ (iterator it = er.ಮೊದಲ; it != er.ಎರಡನೇ; ++it) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ node2p = it->ಎರಡನೇ;
        ಆದರೆ (nodep == node2p) ಮುಂದುವರಿ;  // Same node is not a duplicate
        ಆದರೆ (checkp && !checkp->isSame(nodep, node2p)) ಮುಂದುವರಿ;  // User says it is not a duplicate
        ಆದರೆ (!nodep->same_ವೃಕ್ಷ(node2p)) ಮುಂದುವರಿ;  // Not the same trees
        // Found duplicate
        ಹಿಂತಿರುಗಿ it;
    }
    ಹಿಂತಿರುಗಿ ಕೊನೆ();
}

ಶೂನ್ಯ V3DupFinder::dumpFile(ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ ವೃಕ್ಷ) {
    UINFO(2, "Dumping " << filename);
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ಆದರೆ (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ> dist;

    V3Hash lasthash;
    ಪೂರ್ಣಾಂಕ ಅಂಕೆ_ಒಳ_ಪಾತ್ರೆ = 0;
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = cbegin(); ನಿಜ; ++it) {
        ಆದರೆ (it == cend() || lasthash != it->ಮೊದಲ) {
            ಆದರೆ (it != cend()) lasthash = it->ಮೊದಲ;
            ಆದರೆ (ಅಂಕೆ_ಒಳ_ಪಾತ್ರೆ) ++dist[ಅಂಕೆ_ಒಳ_ಪಾತ್ರೆ];
            ಅಂಕೆ_ಒಳ_ಪಾತ್ರೆ = 0;
        }
        ಆದರೆ (it == cend()) ನಿಲ್ಲಿಸು;
        ಅಂಕೆ_ಒಳ_ಪಾತ್ರೆ++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : dist) {
        *logp << "    " << std::setw(9) << i.ಮೊದಲ << "  " << std::setw(12) << i.ಎರಡನೇ << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : *ಇದು) {
        ಆದರೆ (lasthash != it.ಮೊದಲ) {
            lasthash = it.ಮೊದಲ;
            *logp << "    " << it.ಮೊದಲ << '\n';
        }
        *logp << "\t" << it.ಎರಡನೇ << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        ಆದರೆ (ವೃಕ್ಷ) it.ಎರಡನೇ->dump_ವೃಕ್ಷ(*logp, "    ");
    }
}

ಶೂನ್ಯ V3DupFinder::dumpFilePrefixed(ಸ್ಥಿರಾಂಕ string& nameComment, ತಾರ್ಕಿಕ ವೃಕ್ಷ) {
    ಆದರೆ (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", ವೃಕ್ಷ);
}
