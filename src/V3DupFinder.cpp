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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3DupFinder.h"

#ضمن "V3File.h"

#ضمن <iomanip>
#ضمن <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(AstNode* nodep) {
    ثابت auto& er = equal_range(m_hasher(nodep));
    لكل (iterator it = er.first; it != er.second; ++it) {
        𐤀𐤌 (nodep == it->second) {
            erase(it);
            أرجع 1;
        }
    }
    أرجع 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(AstNode* nodep, V3DupFinderUserSame* checkp) {
    ثابت auto& er = equal_range(m_hasher(nodep));
    لكل (iterator it = er.first; it != er.second; ++it) {
        AstNode* ثابت node2p = it->second;
        𐤀𐤌 (nodep == node2p) استمر;  // Same node is not a duplicate
        𐤀𐤌 (checkp && !checkp->isSame(nodep, node2p)) استمر;  // User says it is not a duplicate
        𐤀𐤌 (!nodep->sameTree(node2p)) استمر;  // Not the same trees
        // Found duplicate
        أرجع it;
    }
    أرجع end();
}

فراغ V3DupFinder::dumpFile(ثابت string& filename, منطقي tree) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    𐤀𐤌 (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<صحيح, صحيح> dist;

    V3Hash lasthash;
    صحيح num_in_bucket = 0;
    لكل (auto it = cbegin(); true; ++it) {
        𐤀𐤌 (it == cend() || lasthash != it->first) {
            𐤀𐤌 (it != cend()) lasthash = it->first;
            𐤀𐤌 (num_in_bucket) ++dist[num_in_bucket];
            num_in_bucket = 0;
        }
        𐤀𐤌 (it == cend()) اكسر;
        num_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    لكل (ثابت auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    لكل (ثابت auto& it : *هذا) {
        𐤀𐤌 (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        𐤀𐤌 (tree) it.second->dumpTree(*logp, "    ");
    }
}

فراغ V3DupFinder::dumpFilePrefixed(ثابت string& nameComment, منطقي tree) {
    𐤀𐤌 (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", tree);
}
