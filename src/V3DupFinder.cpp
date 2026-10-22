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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3DupFinder.h"

#అన్తర్భావ "V3File.h"

#అన్తర్భావ <iomanip>
#అన్తర్భావ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::కొలత_type V3DupFinder::erase(Ast_కణుపు* nodep) {
    స్థిరరాశి స్వయంచాలక& er = equal_range(m_hasher(nodep));
    కొరకు (iterator it = er.మొదటి; it != er.రెండవ; ++it) {
        ఒకవేళ (nodep == it->రెండవ) {
            erase(it);
            తిరిగి 1;
        }
    }
    తిరిగి 0;
}

V3DupFinder::iterator V3DupFinder::కనుగొను_Duplicate(Ast_కణుపు* nodep, V3DupFinderUserSame* checkp) {
    స్థిరరాశి స్వయంచాలక& er = equal_range(m_hasher(nodep));
    కొరకు (iterator it = er.మొదటి; it != er.రెండవ; ++it) {
        Ast_కణుపు* స్థిరరాశి node2p = it->రెండవ;
        ఒకవేళ (nodep == node2p) కొనసాగు;  // Same node is not a duplicate
        ఒకవేళ (checkp && !checkp->isSame(nodep, node2p)) కొనసాగు;  // User says it is not a duplicate
        ఒకవేళ (!nodep->same_వృక్షం(node2p)) కొనసాగు;  // Not the same trees
        // Found duplicate
        తిరిగి it;
    }
    తిరిగి చివర();
}

శూన్య V3DupFinder::dumpFile(స్థిరరాశి string& filename, తార్కిక వృక్షం) {
    UINFO(2, "Dumping " << filename);
    స్థిరరాశి std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ఒకవేళ (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<పూర్ణాంకం, పూర్ణాంకం> dist;

    V3Hash lasthash;
    పూర్ణాంకం అంకె_లోపల_పాత్ర = 0;
    కొరకు (స్వయంచాలక it = cbegin(); నిజం; ++it) {
        ఒకవేళ (it == cend() || lasthash != it->మొదటి) {
            ఒకవేళ (it != cend()) lasthash = it->మొదటి;
            ఒకవేళ (అంకె_లోపల_పాత్ర) ++dist[అంకె_లోపల_పాత్ర];
            అంకె_లోపల_పాత్ర = 0;
        }
        ఒకవేళ (it == cend()) ఆపు;
        అంకె_లోపల_పాత్ర++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    కొరకు (స్థిరరాశి స్వయంచాలక& i : dist) {
        *logp << "    " << std::setw(9) << i.మొదటి << "  " << std::setw(12) << i.రెండవ << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    కొరకు (స్థిరరాశి స్వయంచాలక& it : *ఇది) {
        ఒకవేళ (lasthash != it.మొదటి) {
            lasthash = it.మొదటి;
            *logp << "    " << it.మొదటి << '\n';
        }
        *logp << "\t" << it.రెండవ << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        ఒకవేళ (వృక్షం) it.రెండవ->dump_వృక్షం(*logp, "    ");
    }
}

శూన్య V3DupFinder::dumpFilePrefixed(స్థిరరాశి string& nameComment, తార్కిక వృక్షం) {
    ఒకవేళ (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", వృక్షం);
}
