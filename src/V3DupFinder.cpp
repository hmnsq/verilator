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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3DupFinder.h"

#અન્તર્ભાવ "V3File.h"

#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(Ast_ગાંઠ* nodep) {
    અચલ auto& er = equal_range(m_hasher(nodep));
    માટે (iterator it = er.first; it != er.second; ++it) {
        જો (nodep == it->second) {
            erase(it);
            પાછા 1;
        }
    }
    પાછા 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_ગાંઠ* nodep, V3DupFinderUserSame* checkp) {
    અચલ auto& er = equal_range(m_hasher(nodep));
    માટે (iterator it = er.first; it != er.second; ++it) {
        Ast_ગાંઠ* અચલ node2p = it->second;
        જો (nodep == node2p) ચાલુ;  // Same node is not a duplicate
        જો (checkp && !checkp->isSame(nodep, node2p)) ચાલુ;  // User says it is not a duplicate
        જો (!nodep->same_વૃક્ષ(node2p)) ચાલુ;  // Not the same trees
        // Found duplicate
        પાછા it;
    }
    પાછા end();
}

શૂન્ય V3DupFinder::dumpFile(અચલ string& filename, તાર્કિક વૃક્ષ) {
    UINFO(2, "Dumping " << filename);
    અચલ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    જો (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<પૂર્ણાંક, પૂર્ણાંક> dist;

    V3Hash lasthash;
    પૂર્ણાંક અંક_in_bucket = 0;
    માટે (auto it = cbegin(); સાચું; ++it) {
        જો (it == cend() || lasthash != it->first) {
            જો (it != cend()) lasthash = it->first;
            જો (અંક_in_bucket) ++dist[અંક_in_bucket];
            અંક_in_bucket = 0;
        }
        જો (it == cend()) તોડો;
        અંક_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    માટે (અચલ auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    માટે (અચલ auto& it : *આ) {
        જો (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        જો (વૃક્ષ) it.second->dump_વૃક્ષ(*logp, "    ");
    }
}

શૂન્ય V3DupFinder::dumpFilePrefixed(અચલ string& nameComment, તાર્કિક વૃક્ષ) {
    જો (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", વૃક્ષ);
}
