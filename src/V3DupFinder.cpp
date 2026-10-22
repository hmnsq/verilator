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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3DupFinder.h"

#அந்தர்பாவ "V3File.h"

#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(Ast_முனை* nodep) {
    மாறிலி auto& er = equal_range(m_hasher(nodep));
    க்கு (iterator it = er.first; it != er.second; ++it) {
        எனில் (nodep == it->second) {
            erase(it);
            திரும்பு 1;
        }
    }
    திரும்பு 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_முனை* nodep, V3DupFinderUserSame* checkp) {
    மாறிலி auto& er = equal_range(m_hasher(nodep));
    க்கு (iterator it = er.first; it != er.second; ++it) {
        Ast_முனை* மாறிலி node2p = it->second;
        எனில் (nodep == node2p) தொடர்;  // Same node is not a duplicate
        எனில் (checkp && !checkp->isSame(nodep, node2p)) தொடர்;  // User says it is not a duplicate
        எனில் (!nodep->same_மரம்(node2p)) தொடர்;  // Not the same trees
        // Found duplicate
        திரும்பு it;
    }
    திரும்பு end();
}

வெறுமை V3DupFinder::dumpFile(மாறிலி string& filename, தருக்க மரம்) {
    UINFO(2, "Dumping " << filename);
    மாறிலி std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    எனில் (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<முழு, முழு> dist;

    V3Hash lasthash;
    முழு இலக்கம்_in_bucket = 0;
    க்கு (auto it = cbegin(); உண்மை; ++it) {
        எனில் (it == cend() || lasthash != it->first) {
            எனில் (it != cend()) lasthash = it->first;
            எனில் (இலக்கம்_in_bucket) ++dist[இலக்கம்_in_bucket];
            இலக்கம்_in_bucket = 0;
        }
        எனில் (it == cend()) நிறுத்து;
        இலக்கம்_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    க்கு (மாறிலி auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    க்கு (மாறிலி auto& it : *இது) {
        எனில் (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        எனில் (மரம்) it.second->dump_மரம்(*logp, "    ");
    }
}

வெறுமை V3DupFinder::dumpFilePrefixed(மாறிலி string& nameComment, தருக்க மரம்) {
    எனில் (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", மரம்);
}
