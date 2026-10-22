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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3DupFinder.h"

#अन्तर्भाव "V3File.h"

#अन्तर्भाव <iomanip>
#अन्तर्भाव <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(Ast_गाठ* nodep) {
    स्थिरांक स्वतः& er = equal_range(m_hasher(nodep));
    साठी (iterator it = er.first; it != er.second; ++it) {
        जर (nodep == it->second) {
            erase(it);
            परत 1;
        }
    }
    परत 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_गाठ* nodep, V3DupFinderUserSame* checkp) {
    स्थिरांक स्वतः& er = equal_range(m_hasher(nodep));
    साठी (iterator it = er.first; it != er.second; ++it) {
        Ast_गाठ* स्थिरांक node2p = it->second;
        जर (nodep == node2p) सुरूठेवा;  // Same node is not a duplicate
        जर (checkp && !checkp->isSame(nodep, node2p)) सुरूठेवा;  // User says it is not a duplicate
        जर (!nodep->same_वृक्ष(node2p)) सुरूठेवा;  // Not the same trees
        // Found duplicate
        परत it;
    }
    परत end();
}

शून्य V3DupFinder::dumpFile(स्थिरांक string& filename, तार्किक वृक्ष) {
    UINFO(2, "Dumping " << filename);
    स्थिरांक std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    जर (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<पूर्णांक, पूर्णांक> dist;

    V3Hash lasthash;
    पूर्णांक अंक_in_bucket = 0;
    साठी (स्वतः it = cbegin(); सत्य; ++it) {
        जर (it == cend() || lasthash != it->first) {
            जर (it != cend()) lasthash = it->first;
            जर (अंक_in_bucket) ++dist[अंक_in_bucket];
            अंक_in_bucket = 0;
        }
        जर (it == cend()) थांबा;
        अंक_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    साठी (स्थिरांक स्वतः& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    साठी (स्थिरांक स्वतः& it : *हे) {
        जर (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        जर (वृक्ष) it.second->dump_वृक्ष(*logp, "    ");
    }
}

शून्य V3DupFinder::dumpFilePrefixed(स्थिरांक string& nameComment, तार्किक वृक्ष) {
    जर (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", वृक्ष);
}
