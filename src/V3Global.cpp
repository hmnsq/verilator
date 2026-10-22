// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Common implementations
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3HierBlock.h"
#ಅನ್ತರ್ಭಾವ "V3LibMap.h"
#ಅನ್ತರ್ಭಾವ "V3LinkCells.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"
#ಅನ್ತರ್ಭಾವ "V3ParseImp.h"
#ಅನ್ತರ್ಭಾವ "V3PreShell.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
ಬಾಹ್ಯ "C" ಧ್ರುವ ವರ್ಣ* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    ಪ್ರತ್ಯಾಗಮ ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

ಶೂನ್ಯ V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = ನೂತನ AstNetlist;
    m_constPoolp = ನೂತನ V3ConstPool;
    m_libMapp = ನೂತನ V3LibMap;
}

ಶೂನ್ಯ V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    ಯದಿ (m_rootp) VL_DO_CLEAR(m_rootp->delete_ವೃಕ್ಷ(), m_rootp = nullptr);
#endif
    File_ಸಾಲು::deleteAllRemaining();
}

ಶೂನ್ಯ V3Global::vlExit(ಪೂರ್ಣಾಙ್ಕ status) {
    shutdown();
    std::exit(status);
}

ಶೂನ್ಯ V3Global::check_ವೃಕ್ಷ() ಧ್ರುವ { rootp()->check_ವೃಕ್ಷ(); }

ಶೂನ್ಯ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ಧ್ರುವ VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        ಯದಿ (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                ನೂತನ File_ಸಾಲು{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), ಅಸತ್ಯ,
                ಅಸತ್ಯ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        ಪ್ರತಿ (ಧ್ರುವ VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ಅಸತ್ಯ, ಅಸತ್ಯ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        ಯದಿ (v3Global.opt.stdPackage()) {
            parser.parseFile(
                ನೂತನ File_ಸಾಲು{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                ಅಸತ್ಯ, ಅಸತ್ಯ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        ಪ್ರತಿ (ಧ್ರುವ string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filename, ಅಸತ್ಯ, ಸತ್ಯ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        ಪ್ರತಿ (ಧ್ರುವ auto& filelib : v3Global.opt.vFiles()) {
            ಧ್ರುವ string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ಅಸತ್ಯ, ಅಸತ್ಯ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        ಪ್ರತಿ (ಧ್ರುವ auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ಸತ್ಯ, ಅಸತ್ಯ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        ಪ್ರತಿ (ಧ್ರುವ auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ಅಸತ್ಯ, ಅಸತ್ಯ, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    ಯದಿ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("cells", ಅಸತ್ಯ, dump_ವೃಕ್ಷ_Either_Level() >= 9);
}

ಶೂನ್ಯ V3Global::removeStd() {
    // Delete the std package if unused
    ಯದಿ (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        ಯದಿ (Ast_ಗಂಟು_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_ವೃಕ್ಷ(), stdp);
        }
    }
}

string V3Global::debugFilename(ಧ್ರುವ string& nameComment, ಪೂರ್ಣಾಙ್ಕ new_ಸಂಖ್ಯೆ) {
    ++m_debug_File_ಸಂಖ್ಯೆ;
    ಯದಿ (new_ಸಂಖ್ಯೆ) m_debug_File_ಸಂಖ್ಯೆ = new_ಸಂಖ್ಯೆ;
    ಪ್ರತ್ಯಾಗಮ opt.hier_ತುದಿ_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_ಸಂಖ್ಯೆ)
           + "_" + nameComment;
}
string V3Global::digitsFilename(ಪೂರ್ಣಾಙ್ಕ ಸಂಖ್ಯೆ) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << ಸಂಖ್ಯೆ;
    ಪ್ರತ್ಯಾಗಮ ss.str();
}

ಶೂನ್ಯ V3Global::dump_Check_Global_ವೃಕ್ಷ(ಧ್ರುವ string& stagename, ಪೂರ್ಣಾಙ್ಕ new_ಸಂಖ್ಯೆ, ತಾರ್ಕಿಕ doDump,
                                   ತಾರ್ಕಿಕ doCheck) {
    ಧ್ರುವ string ವೃಕ್ಷ_Filename = v3Global.debugFilename(stagename + ".tree", new_ಸಂಖ್ಯೆ);
    ಯದಿ (dump_ವೃಕ್ಷ_Level()) v3Global.rootp()->dump_ವೃಕ್ಷ_File(ವೃಕ್ಷ_Filename, doDump);
    ಯದಿ (dump_ವೃಕ್ಷ_Json_Level()) {
        v3Global.rootp()->dump_ವೃಕ್ಷ_Json_File(ವೃಕ್ಷ_Filename + ".json", doDump);
    }
    ಯದಿ (v3Global.opt.dump_ವೃಕ್ಷ_Dot()) {
        v3Global.rootp()->dump_ವೃಕ್ಷ_Dot_File(ವೃಕ್ಷ_Filename + ".dot", doDump);
    }
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    ಯದಿ (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(ವೃಕ್ಷ_Filename + ".v");
    ಯದಿ (doCheck && (v3Global.opt.debugCheck() || dump_ವೃಕ್ಷ_Either_Level())) {
        // Error check
        v3Global.rootp()->check_ವೃಕ್ಷ();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

ಶೂನ್ಯ V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

ಶೂನ್ಯ V3Global::saveJsonPtrFieldName(ಧ್ರುವ std::string& fieldName) {
    m_jsonPtrNames.ಸೇರಿಸು(fieldName);
}

ಶೂನ್ಯ V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ಧ್ರುವ std::string& V3Global::ptrToId(ಧ್ರುವ ಶೂನ್ಯ* p) {
    ಧ್ರುವ auto pair = m_ptrToId.emplace(p, "");
    ಯದಿ (pair.second) {
        std::ostringstream os;
        ಯದಿ (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            ಕುರು { os << static_cast<ವರ್ಣ>('A' + id % 26); } ಯಾವತ್ (id /= 26);
            os << ")";
        } ಅನ್ಯಥಾ {
            os << "0";
        }
        pair.first->second = os.str();
    }
    ಪ್ರತ್ಯಾಗಮ pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    ಯದಿ (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    ಯದಿ (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    ಯದಿ (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    ಯದಿ (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    ಯದಿ (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    ಪ್ರತಿ (ಧ್ರುವ string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    ಯದಿ (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    ಯದಿ (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    ಯದಿ (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    ಯದಿ (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    ಪ್ರತ್ಯಾಗಮ result;
}
