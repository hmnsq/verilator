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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3DupFinder.h"

#شامل "V3File.h"

#شامل <iomanip>
#شامل <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3DupFinder class functions

V3DupFinder::سائز_type V3DupFinder::erase(Ast_گرہ* nodep) {
    ثابت خودکار& er = equal_range(m_hasher(nodep));
    برائے (iterator it = er.پہلا; it != er.دوسرا; ++it) {
        اگر (nodep == it->دوسرا) {
            erase(it);
            واپس 1;
        }
    }
    واپس 0;
}

V3DupFinder::iterator V3DupFinder::ڈھونڈو_Duplicate(Ast_گرہ* nodep, V3DupFinderUserSame* checkp) {
    ثابت خودکار& er = equal_range(m_hasher(nodep));
    برائے (iterator it = er.پہلا; it != er.دوسرا; ++it) {
        Ast_گرہ* ثابت node2p = it->دوسرا;
        اگر (nodep == node2p) جاری;  // Same node is not a duplicate
        اگر (checkp && !checkp->isSame(nodep, node2p)) جاری;  // User says it is not a duplicate
        اگر (!nodep->same_درخت(node2p)) جاری;  // Not the same trees
        // Found duplicate
        واپس it;
    }
    واپس انتہا();
}

خالی V3DupFinder::dumpFile(ثابت string& filename, منطقی درخت) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    اگر (logp->fail()) v3fatal("Can't write file: " << filename);

    std::unordered_map<صحیح, صحیح> dist;

    V3Hash lasthash;
    صحیح عدد_اندر_بالٹی = 0;
    برائے (خودکار it = cbegin(); سچ; ++it) {
        اگر (it == cend() || lasthash != it->پہلا) {
            اگر (it != cend()) lasthash = it->پہلا;
            اگر (عدد_اندر_بالٹی) ++dist[عدد_اندر_بالٹی];
            عدد_اندر_بالٹی = 0;
        }
        اگر (it == cend()) توڑو;
        عدد_اندر_بالٹی++;
    }
    *logp << "\n*** STATS:\n\n";
    *logp << "    #InBucket   Occurrences\n";
    برائے (ثابت خودکار& i : dist) {
        *logp << "    " << std::setw(9) << i.پہلا << "  " << std::setw(12) << i.دوسرا << '\n';
    }

    *logp << "\n*** Dump:\n\n";
    برائے (ثابت خودکار& it : *یہ) {
        اگر (lasthash != it.پہلا) {
            lasthash = it.پہلا;
            *logp << "    " << it.پہلا << '\n';
        }
        *logp << "\t" << it.دوسرا << '\n';
        // Dumping the entire tree may make nearly N^2 sized dumps,
        // because the nodes under this one may also be in the hash table!
        اگر (درخت) it.دوسرا->dump_درخت(*logp, "    ");
    }
}

خالی V3DupFinder::dumpFilePrefixed(ثابت string& nameComment, منطقی درخت) {
    اگر (dumpLevel()) dumpFile(v3Global.debugFilename(nameComment) + ".hash", درخت);
}
