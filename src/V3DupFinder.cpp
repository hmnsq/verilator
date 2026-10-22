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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3DupFinder.h"

#समावेश "V3File.h"

#समावेश <iomanip>
#समावेश <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::आकार_type V3DupFinder::erase(Ast_गाँठ* nodep) {
    स्थिर स्वतः& er = equal_range(m_hasher(nodep));
    क्रम (iterator it = er.first; it != er.second; ++it) {
        अगर (nodep == it->second) {
            erase(it);
            वापस 1;
        }
    }
    वापस 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_गाँठ* nodep, V3DupFinderUserSame* checkp) {
    स्थिर स्वतः& er = equal_range(m_hasher(nodep));
    क्रम (iterator it = er.first; it != er.second; ++it) {
        Ast_गाँठ* स्थिर node2p = it->second;
        अगर (nodep == node2p) जारी;  // Same node is not a duplicate
        अगर (checkp && !checkp->isSame(nodep, node2p)) जारी;  // User says it is not a duplicate
        अगर (!nodep->same_वृक्ष(node2p)) जारी;  // Not the same trees
        // Found duplicate
        वापस it;
    }
    वापस end();
}

व्योम V3DupFinder::dumpFile(स्थिर string& filename, bool वृक्ष) {
    UINFO(2, "Dumping " << filename);
    स्थिर std::unique_ptr<std::बख_प्रवाह> logp{V3File::new_ofstream(filename)};
    अगर (logp->हार()) v3fatal("Can't write file: " << filename);

    std::unordered_map<पूर्णांक, पूर्णांक> dist;

    V3Hash lasthash;
    पूर्णांक अंक_in_bucket = 0;
    क्रम (स्वतः it = cbegin(); true; ++it) {
        अगर (it == cend() || lasthash != it->first) {
            अगर (it != cend()) lasthash = it->first;
            अगर (अंक_in_bucket) ++dist[अंक_in_bucket];
            अंक_in_bucket = 0;
        }
        अगर (it == cend()) अवरोध;
        अंक_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    क्रम (स्थिर स्वतः& i : dist) {
        *logp << "    " << std::रखो_चौड़ाई(9) << i.first << "  " << std::रखो_चौड़ाई(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    क्रम (स्थिर स्वतः& it : *यह) {
        अगर (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        अगर (वृक्ष) it.second->dump_वृक्ष(*logp, "    ");
    }
}

व्योम V3DupFinder::dumpFilePrefixed(स्थिर string& nameComment, bool वृक्ष) {
    अगर (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", वृक्ष);
}
