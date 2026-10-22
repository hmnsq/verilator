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

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_ASAN
ಬಾಹ್ಯ "C" ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    ಹಿಂತಿರುಗಿ ""
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VL_LEAK_CHECKS
           ":detect_leaks=0"
#ಆದರೆ_ಅಂತ್ಯ
        ;
}
#ಆದರೆ_ಅಂತ್ಯ

//######################################################################
// V3Global

ಶೂನ್ಯ V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = ಹೊಸ AstNetlist;
    m_constPoolp = ಹೊಸ V3ConstPool;
    m_libMapp = ಹೊಸ V3LibMap;
}

ಶೂನ್ಯ V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(ಅಳಿಸು m_hierGraphp, m_hierGraphp = ಶೂನ್ಯಸೂಚಕ);  // delete nullptr is safe
    VL_DO_CLEAR(ಅಳಿಸು m_threadPoolp, m_threadPoolp = ಶೂನ್ಯಸೂಚಕ);  // delete nullptr is safe
    VL_DO_CLEAR(ಅಳಿಸು m_libMapp, m_libMapp = ಶೂನ್ಯಸೂಚಕ);  // delete nullptr is safe
    VL_DO_CLEAR(ಅಳಿಸು m_constPoolp, m_constPoolp = ಶೂನ್ಯಸೂಚಕ);  // delete nullptr is safe
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_LEAK_CHECKS
    ಆದರೆ (m_rootp) VL_DO_CLEAR(m_rootp->delete_ವೃಕ್ಷ(), m_rootp = ಶೂನ್ಯಸೂಚಕ);
#ಆದರೆ_ಅಂತ್ಯ
    File_ಸಾಲು::delete_All_ಉಳಿದ();
}

ಶೂನ್ಯ V3Global::vlExit(ಪೂರ್ಣಾಂಕ ಸ್ಥಿತಿ_2) {
    shutdown();
    std::ನಿರ್ಗಮಿಸು(ಸ್ಥಿತಿ_2);
}

ಶೂನ್ಯ V3Global::check_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ { rootp()->check_ವೃಕ್ಷ(); }

ಶೂನ್ಯ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        ಆದರೆ (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                ಹೊಸ File_ಸಾಲು{V3Options::get_Std_Waiver_ಮಾರ್ಗ()}, V3Options::get_Std_Waiver_ಮಾರ್ಗ(), ಸುಳ್ಳು,
                ಸುಳ್ಳು, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ಸುಳ್ಳು, ಸುಳ್ಳು, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        ಆದರೆ (v3Global.opt.stdPackage()) {
            parser.parseFile(
                ಹೊಸ File_ಸಾಲು{V3Options::get_Std_Package_ಮಾರ್ಗ()}, V3Options::get_Std_Package_ಮಾರ್ಗ(),
                ಸುಳ್ಳು, ಸುಳ್ಳು, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filename, ಸುಳ್ಳು, ನಿಜ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& filelib : v3Global.opt.vFiles()) {
            ಸ್ಥಿರಾಂಕ string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->ಹೊಂದಿಕೆ_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ಸುಳ್ಳು, ಸುಳ್ಳು, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ನಿಜ, ಸುಳ್ಳು, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()}, filelib.filename(),
                             ಸುಳ್ಳು, ಸುಳ್ಳು, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    ಆದರೆ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("cells", ಸುಳ್ಳು, dump_ವೃಕ್ಷ_Either_Level() >= 9);
}

ಶೂನ್ಯ V3Global::removeStd() {
    // Delete the std package if unused
    ಆದರೆ (!ಬಳಸುತ್ತದೆ_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        ಆದರೆ (Ast_ಗಂಟು_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(ಶೂನ್ಯಸೂಚಕ);
            v3Global.rootp()->stdPackageProcessp(ಶೂನ್ಯಸೂಚಕ);
            VL_DO_DANGLING(stdp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), stdp);
        }
    }
}

string V3Global::debugFilename(ಸ್ಥಿರಾಂಕ string& nameComment, ಪೂರ್ಣಾಂಕ new_ಸಂಖ್ಯೆ) {
    ++m_debug_File_ಸಂಖ್ಯೆ;
    ಆದರೆ (new_ಸಂಖ್ಯೆ) m_debug_File_ಸಂಖ್ಯೆ = new_ಸಂಖ್ಯೆ;
    ಹಿಂತಿರುಗಿ opt.hier_ತುದಿ_ದತ್ತಾಂಶ_Dir() + "/" + opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "_" + digitsFilename(m_debug_File_ಸಂಖ್ಯೆ)
           + "_" + nameComment;
}
string V3Global::digitsFilename(ಪೂರ್ಣಾಂಕ ಸಂಖ್ಯೆ) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << ಸಂಖ್ಯೆ;
    ಹಿಂತಿರುಗಿ ss.str();
}

ಶೂನ್ಯ V3Global::dump_Check_Global_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ string& stagename, ಪೂರ್ಣಾಂಕ new_ಸಂಖ್ಯೆ, ತಾರ್ಕಿಕ doDump,
                                   ತಾರ್ಕಿಕ doCheck) {
    ಸ್ಥಿರಾಂಕ string ವೃಕ್ಷ_Filename = v3Global.debugFilename(stagename + ".tree", new_ಸಂಖ್ಯೆ);
    ಆದರೆ (dump_ವೃಕ್ಷ_Level()) v3Global.rootp()->dump_ವೃಕ್ಷ_File(ವೃಕ್ಷ_Filename, doDump);
    ಆದರೆ (dump_ವೃಕ್ಷ_Json_Level()) {
        v3Global.rootp()->dump_ವೃಕ್ಷ_Json_File(ವೃಕ್ಷ_Filename + ".json", doDump);
    }
    ಆದರೆ (v3Global.opt.dump_ವೃಕ್ಷ_Dot()) {
        v3Global.rootp()->dump_ವೃಕ್ಷ_Dot_File(ವೃಕ್ಷ_Filename + ".dot", doDump);
    }
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    ಆದರೆ (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(ವೃಕ್ಷ_Filename + ".v");
    ಆದರೆ (doCheck && (v3Global.opt.debugCheck() || dump_ವೃಕ್ಷ_Either_Level())) {
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
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_ptr_ಗಮ್ಯ_Id) {
        os << sep << '"' << itr.ಎರಡನೇ << "\": \"" << cvt_ಗಮ್ಯ_Hex(itr.ಮೊದಲ) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

ಶೂನ್ಯ V3Global::save_Json_Ptr_ಕ್ಷೇತ್ರ_Name(ಸ್ಥಿರಾಂಕ std::string& ಕ್ಷೇತ್ರ_Name) {
    m_json_Ptr_ಹೆಸರುಗಳು.ಸೇರಿಸು(ಕ್ಷೇತ್ರ_Name);
}

ಶೂನ್ಯ V3Global::ptr_ಹೆಸರುಗಳು_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_json_Ptr_ಹೆಸರುಗಳು) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ಸ್ಥಿರಾಂಕ std::string& V3Global::ptr_ಗಮ್ಯ_Id(ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* p) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_ptr_ಗಮ್ಯ_Id.emplace(p, "");
    ಆದರೆ (pair.ಎರಡನೇ) {
        std::ostringstream os;
        ಆದರೆ (p) {
            os << "(";
            ಚಿಹ್ನೆರಹಿತ id = m_ptr_ಗಮ್ಯ_Id.ಗಾತ್ರ_2();
            ಮಾಡು { os << static_cast<ಅಕ್ಷರ>('A' + id % 26); } ಆಗ (id /= 26);
            os << ")";
        } ಇಲ್ಲದಿದ್ದರೆ {
            os << "0";
        }
        pair.ಮೊದಲ->ಎರಡನೇ = os.str();
    }
    ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
}

std::ಸದಿಶ<std::string> V3Global::verilatedCppFiles() {
    std::ಸದಿಶ<std::string> ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated.cpp");
    ಆದರೆ (v3Global.dpi()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_dpi.cpp");
    ಆದರೆ (v3Global.opt.vpi()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_vpi.cpp");
    ಆದರೆ (v3Global.opt.savable()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_save.cpp");
    ಆದರೆ (v3Global.opt.coverage()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_cov.cpp");
    ಆದರೆ (v3Global.opt.coverage() || v3Global.useCovergroup())
        ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_covergroup.cpp");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ತಳ : v3Global.opt.trace_ಮೂಲ_Bases())
        ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ(ತಳ + "_c.cpp");
    ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Prob_Dist()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_probdist.cpp");
    ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Timing()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_timing.cpp");
    ಆದರೆ (v3Global.useRandomizeMethods()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_random.cpp");
    ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_threads.cpp");
    ಆದರೆ (v3Global.opt.ಬಳಸುತ್ತದೆ_Profiler()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_profiler.cpp");
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
