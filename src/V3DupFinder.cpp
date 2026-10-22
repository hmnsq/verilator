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

V3DupFinder::आकार_type V3DupFinder::erase(Ast_ग्रन्थिः* nodep) {
    ध्रुव स्वचालित& er = equal_range(m_hasher(nodep));
    प्रति (iterator it = er.first; it != er.second; ++it) {
        यदि (nodep == it->second) {
            erase(it);
            प्रत्यागम 1;
        }
    }
    प्रत्यागम 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_ग्रन्थिः* nodep, V3DupFinderUserSame* checkp) {
    ध्रुव स्वचालित& er = equal_range(m_hasher(nodep));
    प्रति (iterator it = er.first; it != er.second; ++it) {
        Ast_ग्रन्थिः* ध्रुव node2p = it->second;
        यदि (nodep == node2p) अनुवर्त;  // Same node is not a duplicate
        यदि (checkp && !checkp->isSame(nodep, node2p)) अनुवर्त;  // User says it is not a duplicate
        यदि (!nodep->same_वृक्षः(node2p)) अनुवर्त;  // Not the same trees
        // Found duplicate
        प्रत्यागम it;
    }
    प्रत्यागम अन्तः_2();
}

शून्य V3DupFinder::dumpFile(ध्रुव string& filename, तार्किक वृक्षः) {
    UINFO(2, "Dumping " << filename);
    ध्रुव std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    यदि (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<पूर्णाङ्क, पूर्णाङ्क> dist;

    V3Hash lasthash;
    पूर्णाङ्क अङ्कः_अन्तर्_bucket = 0;
    प्रति (स्वचालित it = cbegin(); सत्य; ++it) {
        यदि (it == cend() || lasthash != it->first) {
            यदि (it != cend()) lasthash = it->first;
            यदि (अङ्कः_अन्तर्_bucket) ++dist[अङ्कः_अन्तर्_bucket];
            अङ्कः_अन्तर्_bucket = 0;
        }
        यदि (it == cend()) भङ्ग;
        अङ्कः_अन्तर्_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    प्रति (ध्रुव स्वचालित& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    प्रति (ध्रुव स्वचालित& it : *एतत्) {
        यदि (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        यदि (वृक्षः) it.second->dump_वृक्षः(*logp, "    ");
    }
}

शून्य V3DupFinder::dumpFilePrefixed(ध्रुव string& nameComment, तार्किक वृक्षः) {
    यदि (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", वृक्षः);
}
