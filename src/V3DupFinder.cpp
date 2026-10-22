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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3DupFinder.h"

#അന്തര്ഭാവ "V3File.h"

#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::size_type V3DupFinder::erase(Ast_കണ്ണി* nodep) {
    സ്ഥിരം auto& er = equal_range(m_hasher(nodep));
    വേണ്ടി (iterator it = er.first; it != er.second; ++it) {
        എങ്കിൽ (nodep == it->second) {
            erase(it);
            തിരികെ 1;
        }
    }
    തിരികെ 0;
}

V3DupFinder::iterator V3DupFinder::findDuplicate(Ast_കണ്ണി* nodep, V3DupFinderUserSame* checkp) {
    സ്ഥിരം auto& er = equal_range(m_hasher(nodep));
    വേണ്ടി (iterator it = er.first; it != er.second; ++it) {
        Ast_കണ്ണി* സ്ഥിരം node2p = it->second;
        എങ്കിൽ (nodep == node2p) തുടരുക;  // Same node is not a duplicate
        എങ്കിൽ (checkp && !checkp->isSame(nodep, node2p)) തുടരുക;  // User says it is not a duplicate
        എങ്കിൽ (!nodep->same_വൃക്ഷം(node2p)) തുടരുക;  // Not the same trees
        // Found duplicate
        തിരികെ it;
    }
    തിരികെ end();
}

ശൂന്യം V3DupFinder::dumpFile(സ്ഥിരം string& filename, തർക്കം വൃക്ഷം) {
    UINFO(2, "Dumping " << filename);
    സ്ഥിരം std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    എങ്കിൽ (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ> dist;

    V3Hash lasthash;
    പൂർണ്ണസംഖ്യ അക്കം_in_bucket = 0;
    വേണ്ടി (auto it = cbegin(); സത്യം; ++it) {
        എങ്കിൽ (it == cend() || lasthash != it->first) {
            എങ്കിൽ (it != cend()) lasthash = it->first;
            എങ്കിൽ (അക്കം_in_bucket) ++dist[അക്കം_in_bucket];
            അക്കം_in_bucket = 0;
        }
        എങ്കിൽ (it == cend()) നിർത്തുക;
        അക്കം_in_bucket++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    വേണ്ടി (സ്ഥിരം auto& i : dist) {
        *logp << "    " << std::setw(9) << i.first << "  " << std::setw(12) << i.second << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    വേണ്ടി (സ്ഥിരം auto& it : *ഇത്) {
        എങ്കിൽ (lasthash != it.first) {
            lasthash = it.first;
            *logp << "    " << it.first << '\n';
        }
        *logp << "\t" << it.second << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        എങ്കിൽ (വൃക്ഷം) it.second->dump_വൃക്ഷം(*logp, "    ");
    }
}

ശൂന്യം V3DupFinder::dumpFilePrefixed(സ്ഥിരം string& nameComment, തർക്കം വൃക്ഷം) {
    എങ്കിൽ (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", വൃക്ഷം);
}
