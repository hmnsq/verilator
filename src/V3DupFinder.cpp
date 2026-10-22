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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3DupFinder.h"

#অন্তর্ভাব "V3File.h"

#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(Ast_গ্রন্থি* nodep) {
    ধ্রুব auto& er = equal_range(m_hasher(nodep));
    প্রতি (iterator it = er.first; it != er.second; ++it) {
        যদি (nodep == it->second) {
            erase(it);
            প্রত্যাগম 1;
        }
    }
    প্রত্যাগম 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_গ্রন্থি* nodep, V3DupFinderUserSame* checkp) {
    ধ্রুব auto& er = equal_range(m_hasher(nodep));
    প্রতি (iterator it = er.first; it != er.second; ++it) {
        Ast_গ্রন্থি* ধ্রুব node2p = it->second;
        যদি (nodep == node2p) অনুবর্ত;  // Same node is not a duplicate
        যদি (checkp && !checkp->isSame(nodep, node2p)) অনুবর্ত;  // User says it is not a duplicate
        যদি (!nodep->same_বৃক্ষ(node2p)) অনুবর্ত;  // Not the same trees
        // Found duplicate
        প্রত্যাগম it;
    }
    প্রত্যাগম end();
}

শূন্য V3DupFinder::dumpFile(ধ্রুব string& filename, তার্কিক বৃক্ষ) {
    UINFO(2, "Dumping " << filename);
    ধ্রুব std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    যদি (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<পূর্ণাঙ্ক, পূর্ণাঙ্ক> dist;

    V3Hash lasthash;
    পূর্ণাঙ্ক অঙ্ক_in_bucket = 0;
    প্রতি (auto it = cbegin(); সত্য; ++it) {
        যদি (it == cend() || lasthash != it->first) {
            যদি (it != cend()) lasthash = it->first;
            যদি (অঙ্ক_in_bucket) ++dist[অঙ্ক_in_bucket];
            অঙ্ক_in_bucket = 0;
        }
        যদি (it == cend()) ভঙ্গ;
        অঙ্ক_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    প্রতি (ধ্রুব auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    প্রতি (ধ্রুব auto& it : *এতত্) {
        যদি (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        যদি (বৃক্ষ) it.second->dump_বৃক্ষ(*logp, "    ");
    }
}

শূন্য V3DupFinder::dumpFilePrefixed(ধ্রুব string& nameComment, তার্কিক বৃক্ষ) {
    যদি (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", বৃক্ষ);
}
