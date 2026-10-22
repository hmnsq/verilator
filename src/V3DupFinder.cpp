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

V3DupFinder::size_type V3DupFinder::erase(Ast_गाँठो* nodep) {
    अचर स्वतः& er = equal_range(m_hasher(nodep));
    लागि (iterator it = er.first; it != er.second; ++it) {
        यदि (nodep == it->second) {
            erase(it);
            फर्काऊ 1;
        }
    }
    फर्काऊ 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_गाँठो* nodep, V3DupFinderUserSame* checkp) {
    अचर स्वतः& er = equal_range(m_hasher(nodep));
    लागि (iterator it = er.first; it != er.second; ++it) {
        Ast_गाँठो* अचर node2p = it->second;
        यदि (nodep == node2p) जारी;  // Same node is not a duplicate
        यदि (checkp && !checkp->isSame(nodep, node2p)) जारी;  // User says it is not a duplicate
        यदि (!nodep->same_रूख(node2p)) जारी;  // Not the same trees
        // Found duplicate
        फर्काऊ it;
    }
    फर्काऊ end();
}

शून्य V3DupFinder::dumpFile(अचर string& filename, तार्किक रूख) {
    UINFO(2, "Dumping " << filename);
    अचर std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    यदि (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<पूर्णाङ्क, पूर्णाङ्क> dist;

    V3Hash lasthash;
    पूर्णाङ्क अङ्क_in_bucket = 0;
    लागि (स्वतः it = cbegin(); सत्य; ++it) {
        यदि (it == cend() || lasthash != it->first) {
            यदि (it != cend()) lasthash = it->first;
            यदि (अङ्क_in_bucket) ++dist[अङ्क_in_bucket];
            अङ्क_in_bucket = 0;
        }
        यदि (it == cend()) रोक;
        अङ्क_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    लागि (अचर स्वतः& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    लागि (अचर स्वतः& it : *एतत्) {
        यदि (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        यदि (रूख) it.second->dump_रूख(*logp, "    ");
    }
}

शून्य V3DupFinder::dumpFilePrefixed(अचर string& nameComment, तार्किक रूख) {
    यदि (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", रूख);
}
