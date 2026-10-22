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

V3DupFinder::سائيز_type V3DupFinder::erase(Ast_ڳنڍ* nodep) {
    ثابت خودڪار& er = equal_range(m_hasher(nodep));
    لكل (iterator it = er.پهريون; it != er.ٻيو; ++it) {
        إذا (nodep == it->ٻيو) {
            erase(it);
            أرجع 1;
        }
    }
    أرجع 0;
}

V3DupFinder::iterator V3DupFinder::ڳوليو_Duplicate(Ast_ڳنڍ* nodep, V3DupFinderUserSame* checkp) {
    ثابت خودڪار& er = equal_range(m_hasher(nodep));
    لكل (iterator it = er.پهريون; it != er.ٻيو; ++it) {
        Ast_ڳنڍ* ثابت node2p = it->ٻيو;
        إذا (nodep == node2p) استمر;  // Same node is not a duplicate
        إذا (checkp && !checkp->isSame(nodep, node2p)) استمر;  // User says it is not a duplicate
        إذا (!nodep->same_وڻ(node2p)) استمر;  // Not the same trees
        // Found duplicate
        أرجع it;
    }
    أرجع پڇاڙي();
}

فراغ V3DupFinder::dumpFile(ثابت string& filename, منطقي وڻ) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    إذا (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<صحيح, صحيح> dist;

    V3Hash lasthash;
    صحيح انگ_اندريون_ٻالٽي = 0;
    لكل (خودڪار it = cbegin(); سچ; ++it) {
        إذا (it == cend() || lasthash != it->پهريون) {
            إذا (it != cend()) lasthash = it->پهريون;
            إذا (انگ_اندريون_ٻالٽي) ++dist[انگ_اندريون_ٻالٽي];
            انگ_اندريون_ٻالٽي = 0;
        }
        إذا (it == cend()) اكسر;
        انگ_اندريون_ٻالٽي++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    لكل (ثابت خودڪار& i : dist) {
        *logp << "    " << std::setw(9) << i.پهريون << "  " << std::setw(12) << i.ٻيو << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    لكل (ثابت خودڪار& it : *هذا) {
        إذا (lasthash != it.پهريون) {
            lasthash = it.پهريون;
            *logp << "    " << it.پهريون << '\n';
        }
        *logp << "\t" << it.ٻيو << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        إذا (وڻ) it.ٻيو->dump_وڻ(*logp, "    ");
    }
}

فراغ V3DupFinder::dumpFilePrefixed(ثابت string& nameComment, منطقي وڻ) {
    إذا (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", وڻ);
}
