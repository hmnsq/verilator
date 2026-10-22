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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3DupFinder.h"

#ଅନ୍ତର୍ଭାଵ "V3File.h"

#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(Ast_ଗଣ୍ଠି* nodep) {
    ଅଚଳ auto& er = equal_range(m_hasher(nodep));
    ପାଇଁ (iterator it = er.first; it != er.second; ++it) {
        ଯଦି (nodep == it->second) {
            erase(it);
            ଫେରନ୍ତୁ 1;
        }
    }
    ଫେରନ୍ତୁ 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_ଗଣ୍ଠି* nodep, V3DupFinderUserSame* checkp) {
    ଅଚଳ auto& er = equal_range(m_hasher(nodep));
    ପାଇଁ (iterator it = er.first; it != er.second; ++it) {
        Ast_ଗଣ୍ଠି* ଅଚଳ node2p = it->second;
        ଯଦି (nodep == node2p) ଜାରି;  // Same node is not a duplicate
        ଯଦି (checkp && !checkp->isSame(nodep, node2p)) ଜାରି;  // User says it is not a duplicate
        ଯଦି (!nodep->same_ବୃକ୍ଷ(node2p)) ଜାରି;  // Not the same trees
        // Found duplicate
        ଫେରନ୍ତୁ it;
    }
    ଫେରନ୍ତୁ end();
}

ଶୂନ୍ୟ V3DupFinder::dumpFile(ଅଚଳ string& filename, ତାର୍କିକ ବୃକ୍ଷ) {
    UINFO(2, "Dumping " << filename);
    ଅଚଳ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ଯଦି (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> dist;

    V3Hash lasthash;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_in_bucket = 0;
    ପାଇଁ (auto it = cbegin(); ସତ୍ୟ; ++it) {
        ଯଦି (it == cend() || lasthash != it->first) {
            ଯଦି (it != cend()) lasthash = it->first;
            ଯଦି (ଅଙ୍କ_in_bucket) ++dist[ଅଙ୍କ_in_bucket];
            ଅଙ୍କ_in_bucket = 0;
        }
        ଯଦି (it == cend()) ଭାଙ୍ଗ;
        ଅଙ୍କ_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    ପାଇଁ (ଅଚଳ auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    ପାଇଁ (ଅଚଳ auto& it : *ଏହି) {
        ଯଦି (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        ଯଦି (ବୃକ୍ଷ) it.second->dump_ବୃକ୍ଷ(*logp, "    ");
    }
}

ଶୂନ୍ୟ V3DupFinder::dumpFilePrefixed(ଅଚଳ string& nameComment, ତାର୍କିକ ବୃକ୍ଷ) {
    ଯଦି (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", ବୃକ୍ଷ);
}
