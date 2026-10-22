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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HierBlock.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LibMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkCells.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Parse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ParseImp.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreShell.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_ASAN
ਬਾਹਰੀ "C" ਅਚਲ ਅੱਖਰ* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    ਵਾਪਸ ""
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
           ":detect_leaks=0"
#ਜੇ_ਅੰਤ
        ;
}
#ਜੇ_ਅੰਤ

//######################################################################
// V3Global

ਖਾਲੀ V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = ਨਵਾਂ AstNetlist;
    m_constPoolp = ਨਵਾਂ V3ConstPool;
    m_libMapp = ਨਵਾਂ V3LibMap;
}

ਖਾਲੀ V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(ਮਿਟਾਓ m_hierGraphp, m_hierGraphp = ਖਾਲੀ_ਸੂਚਕ);  // delete nullptr is safe
    VL_DO_CLEAR(ਮਿਟਾਓ m_threadPoolp, m_threadPoolp = ਖਾਲੀ_ਸੂਚਕ);  // delete nullptr is safe
    VL_DO_CLEAR(ਮਿਟਾਓ m_libMapp, m_libMapp = ਖਾਲੀ_ਸੂਚਕ);  // delete nullptr is safe
    VL_DO_CLEAR(ਮਿਟਾਓ m_constPoolp, m_constPoolp = ਖਾਲੀ_ਸੂਚਕ);  // delete nullptr is safe
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
    ਜੇ (m_rootp) VL_DO_CLEAR(m_rootp->delete_ਰੁੱਖ(), m_rootp = ਖਾਲੀ_ਸੂਚਕ);
#ਜੇ_ਅੰਤ
    File_ਸਤਰ::delete_All_ਬਚਿਆ();
}

ਖਾਲੀ V3Global::vlExit(ਪੂਰਨ ਸਥਿਤੀ) {
    shutdown();
    std::ਬਾਹਰ_ਨਿਕਲੋ(ਸਥਿਤੀ);
}

ਖਾਲੀ V3Global::check_ਰੁੱਖ() ਅਚਲ { rootp()->check_ਰੁੱਖ(); }

ਖਾਲੀ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ਅਚਲ VNUser4_ਵਿੱਚ_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        ਜੇ (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                ਨਵਾਂ File_ਸਤਰ{V3Options::get_Std_Waiver_ਰਸਤਾ()}, V3Options::get_Std_Waiver_ਰਸਤਾ(), ਝੂਠ,
                ਝੂਠ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        ਲਈ (ਅਚਲ VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()}, filelib.filename(),
                             ਝੂਠ, ਝੂਠ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        ਜੇ (v3Global.opt.stdPackage()) {
            parser.parseFile(
                ਨਵਾਂ File_ਸਤਰ{V3Options::get_Std_Package_ਰਸਤਾ()}, V3Options::get_Std_Package_ਰਸਤਾ(),
                ਝੂਠ, ਝੂਠ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        ਲਈ (ਅਚਲ string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()}, filename, ਝੂਠ, ਸੱਚ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& filelib : v3Global.opt.vFiles()) {
            ਅਚਲ string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->ਮੇਲ_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()}, filelib.filename(),
                             ਝੂਠ, ਝੂਠ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()}, filelib.filename(),
                             ਸੱਚ, ਝੂਠ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()}, filelib.filename(),
                             ਝੂਠ, ਝੂਠ, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    ਜੇ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_ਰੁੱਖ("cells", ਝੂਠ, dump_ਰੁੱਖ_Either_Level() >= 9);
}

ਖਾਲੀ V3Global::removeStd() {
    // Delete the std package if unused
    ਜੇ (!ਵਰਤਦਾ_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        ਜੇ (Ast_ਗੰਢ_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(ਖਾਲੀ_ਸੂਚਕ);
            v3Global.rootp()->stdPackageProcessp(ਖਾਲੀ_ਸੂਚਕ);
            VL_DO_DANGLING(stdp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), stdp);
        }
    }
}

string V3Global::debugFilename(ਅਚਲ string& nameComment, ਪੂਰਨ new_ਸੰਖਿਆ) {
    ++m_debug_File_ਸੰਖਿਆ;
    ਜੇ (new_ਸੰਖਿਆ) m_debug_File_ਸੰਖਿਆ = new_ਸੰਖਿਆ;
    ਵਾਪਸ opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() + "/" + opt.ਅਗੇਤਰ() + "_" + digitsFilename(m_debug_File_ਸੰਖਿਆ)
           + "_" + nameComment;
}
string V3Global::digitsFilename(ਪੂਰਨ ਸੰਖਿਆ) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << ਸੰਖਿਆ;
    ਵਾਪਸ ss.str();
}

ਖਾਲੀ V3Global::dump_Check_Global_ਰੁੱਖ(ਅਚਲ string& stagename, ਪੂਰਨ new_ਸੰਖਿਆ, ਤਾਰਕਿਕ doDump,
                                   ਤਾਰਕਿਕ doCheck) {
    ਅਚਲ string ਰੁੱਖ_Filename = v3Global.debugFilename(stagename + ".tree", new_ਸੰਖਿਆ);
    ਜੇ (dump_ਰੁੱਖ_Level()) v3Global.rootp()->dump_ਰੁੱਖ_File(ਰੁੱਖ_Filename, doDump);
    ਜੇ (dump_ਰੁੱਖ_Json_Level()) {
        v3Global.rootp()->dump_ਰੁੱਖ_Json_File(ਰੁੱਖ_Filename + ".json", doDump);
    }
    ਜੇ (v3Global.opt.dump_ਰੁੱਖ_Dot()) {
        v3Global.rootp()->dump_ਰੁੱਖ_Dot_File(ਰੁੱਖ_Filename + ".dot", doDump);
    }
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    ਜੇ (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(ਰੁੱਖ_Filename + ".v");
    ਜੇ (doCheck && (v3Global.opt.debugCheck() || dump_ਰੁੱਖ_Either_Level())) {
        // Error check
        v3Global.rootp()->check_ਰੁੱਖ();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

ਖਾਲੀ V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_ptr_ਮੰਜ਼ਿਲ_Id) {
        os << sep << '"' << itr.ਦੂਜਾ << "\": \"" << cvt_ਮੰਜ਼ਿਲ_Hex(itr.ਪਹਿਲਾ) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

ਖਾਲੀ V3Global::save_Json_Ptr_ਖੇਤਰ_Name(ਅਚਲ std::string& ਖੇਤਰ_Name) {
    m_json_Ptr_ਨਾਂ.ਪਾਓ(ਖੇਤਰ_Name);
}

ਖਾਲੀ V3Global::ptr_ਨਾਂ_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_json_Ptr_ਨਾਂ) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ਅਚਲ std::string& V3Global::ptr_ਮੰਜ਼ਿਲ_Id(ਅਚਲ ਖਾਲੀ* p) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_ptr_ਮੰਜ਼ਿਲ_Id.emplace(p, "");
    ਜੇ (pair.ਦੂਜਾ) {
        std::ostringstream os;
        ਜੇ (p) {
            os << "(";
            ਬਿਨਾਨਿਸ਼ਾਨੀ id = m_ptr_ਮੰਜ਼ਿਲ_Id.ਮਾਪ();
            ਕਰੋ { os << static_cast<ਅੱਖਰ>('A' + id % 26); } ਜਦੋਂ (id /= 26);
            os << ")";
        } ਨਹੀਂਤਾਂ {
            os << "0";
        }
        pair.ਪਹਿਲਾ->ਦੂਜਾ = os.str();
    }
    ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
}

std::ਸਦਿਸ਼<std::string> V3Global::verilatedCppFiles() {
    std::ਸਦਿਸ਼<std::string> ਨਤੀਜਾ;
    ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated.cpp");
    ਜੇ (v3Global.dpi()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_dpi.cpp");
    ਜੇ (v3Global.opt.vpi()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_vpi.cpp");
    ਜੇ (v3Global.opt.savable()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_save.cpp");
    ਜੇ (v3Global.opt.coverage()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_cov.cpp");
    ਜੇ (v3Global.opt.coverage() || v3Global.useCovergroup())
        ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_covergroup.cpp");
    ਲਈ (ਅਚਲ string& ਅਧਾਰ : v3Global.opt.trace_ਸਰੋਤ_Bases())
        ਨਤੀਜਾ.emplace_ਪਿੱਠ(ਅਧਾਰ + "_c.cpp");
    ਜੇ (v3Global.ਵਰਤਦਾ_Prob_Dist()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_probdist.cpp");
    ਜੇ (v3Global.ਵਰਤਦਾ_Timing()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_timing.cpp");
    ਜੇ (v3Global.useRandomizeMethods()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_random.cpp");
    ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_threads.cpp");
    ਜੇ (v3Global.opt.ਵਰਤਦਾ_Profiler()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_profiler.cpp");
    ਵਾਪਸ ਨਤੀਜਾ;
}
