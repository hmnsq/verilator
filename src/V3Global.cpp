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

#ضمن "V3PchAstMT.h"

#ضمن "V3Global.h"

#ضمن "V3ConstPool.h"
#ضمن "V3EmitV.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3HierBlock.h"
#ضمن "V3LibMap.h"
#ضمن "V3LinkCells.h"
#ضمن "V3Parse.h"
#ضمن "V3ParseImp.h"
#ضمن "V3PreShell.h"
#ضمن "V3Stats.h"
#ضمن "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#که_تعریف_شوی VL_ASAN
خارجي "C" ثابت حرف* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    أرجع ""
#که_نه_تعریف_شوی VL_LEAK_CHECKS
           ":detect_leaks=0"
#د_که_پای
        ;
}
#د_که_پای

//######################################################################
// V3Global

فراغ V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = جديد AstNetlist;
    m_constPoolp = جديد V3ConstPool;
    m_libMapp = جديد V3LibMap;
}

فراغ V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(ړنګول m_hierGraphp, m_hierGraphp = تش_اشاره);  // delete nullptr is safe
    VL_DO_CLEAR(ړنګول m_threadPoolp, m_threadPoolp = تش_اشاره);  // delete nullptr is safe
    VL_DO_CLEAR(ړنګول m_libMapp, m_libMapp = تش_اشاره);  // delete nullptr is safe
    VL_DO_CLEAR(ړنګول m_constPoolp, m_constPoolp = تش_اشاره);  // delete nullptr is safe
#که_تعریف_شوی VL_LEAK_CHECKS
    إذا (m_rootp) VL_DO_CLEAR(m_rootp->delete_ونه(), m_rootp = تش_اشاره);
#د_که_پای
    File_کرښه::delete_All_پاتې_شوی();
}

فراغ V3Global::vlExit(صحيح حالت) {
    shutdown();
    std::وتل(حالت);
}

فراغ V3Global::check_ونه() ثابت { rootp()->check_ونه(); }

فراغ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser4_دننه_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        إذا (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                جديد File_کرښه{V3Options::get_Std_Waiver_لار()}, V3Options::get_Std_Waiver_لار(), خطأ,
                خطأ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        لكل (ثابت VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(جديد File_کرښه{File_کرښه::command_کرښه_Filename()}, filelib.filename(),
                             خطأ, خطأ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        إذا (v3Global.opt.stdPackage()) {
            parser.parseFile(
                جديد File_کرښه{V3Options::get_Std_Package_لار()}, V3Options::get_Std_Package_لار(),
                خطأ, خطأ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        لكل (ثابت string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(جديد File_کرښه{File_کرښه::command_کرښه_Filename()}, filename, خطأ, ریښتیا,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        لكل (ثابت خپلکاری& filelib : v3Global.opt.vFiles()) {
            ثابت string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->سمون_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(جديد File_کرښه{File_کرښه::command_کرښه_Filename()}, filelib.filename(),
                             خطأ, خطأ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        لكل (ثابت خپلکاری& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(جديد File_کرښه{File_کرښه::command_کرښه_Filename()}, filelib.filename(),
                             ریښتیا, خطأ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        لكل (ثابت خپلکاری& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(جديد File_کرښه{File_کرښه::command_کرښه_Filename()}, filelib.filename(),
                             خطأ, خطأ, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    إذا (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_ونه("cells", خطأ, dump_ونه_Either_Level() >= 9);
}

فراغ V3Global::removeStd() {
    // Delete the std package if unused
    إذا (!کاروي_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        إذا (Ast_غوټه_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(تش_اشاره);
            v3Global.rootp()->stdPackageProcessp(تش_اشاره);
            VL_DO_DANGLING(stdp->unlink_Fr_شا()->delete_ونه(), stdp);
        }
    }
}

string V3Global::debugFilename(ثابت string& nameComment, صحيح new_شمېره) {
    ++m_debug_File_شمېره;
    إذا (new_شمېره) m_debug_File_شمېره = new_شمېره;
    أرجع opt.hier_سر_معلومات_Dir() + "/" + opt.مختاړی() + "_" + digitsFilename(m_debug_File_شمېره)
           + "_" + nameComment;
}
string V3Global::digitsFilename(صحيح شمېره) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << شمېره;
    أرجع ss.str();
}

فراغ V3Global::dump_Check_Global_ونه(ثابت string& stagename, صحيح new_شمېره, منطقي doDump,
                                   منطقي doCheck) {
    ثابت string ونه_Filename = v3Global.debugFilename(stagename + ".tree", new_شمېره);
    إذا (dump_ونه_Level()) v3Global.rootp()->dump_ونه_File(ونه_Filename, doDump);
    إذا (dump_ونه_Json_Level()) {
        v3Global.rootp()->dump_ونه_Json_File(ونه_Filename + ".json", doDump);
    }
    إذا (v3Global.opt.dump_ونه_Dot()) {
        v3Global.rootp()->dump_ونه_Dot_File(ونه_Filename + ".dot", doDump);
    }
    إذا (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    إذا (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(ونه_Filename + ".v");
    إذا (doCheck && (v3Global.opt.debugCheck() || dump_ونه_Either_Level())) {
        // Error check
        v3Global.rootp()->check_ونه();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

فراغ V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    لكل (ثابت خپلکاری& itr : m_ptr_منزل_Id) {
        os << sep << '"' << itr.دوهم << "\": \"" << cvt_منزل_Hex(itr.لومړی) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

فراغ V3Global::save_Json_Ptr_ډګر_Name(ثابت std::string& ډګر_Name) {
    m_json_Ptr_نومونه.ورننباسه(ډګر_Name);
}

فراغ V3Global::ptr_نومونه_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    لكل (ثابت خپلکاری& itr : m_json_Ptr_نومونه) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ثابت std::string& V3Global::ptr_منزل_Id(ثابت فراغ* p) {
    ثابت خپلکاری pair = m_ptr_منزل_Id.emplace(p, "");
    إذا (pair.دوهم) {
        std::ostringstream os;
        إذا (p) {
            os << "(";
            بینښه id = m_ptr_منزل_Id.اندازه();
            افعل { os << static_cast<حرف>('A' + id % 26); } طالما (id /= 26);
            os << ")";
        } وإلا {
            os << "0";
        }
        pair.لومړی->دوهم = os.str();
    }
    أرجع pair.لومړی->دوهم;
}

std::ویکټور<std::string> V3Global::verilatedCppFiles() {
    std::ویکټور<std::string> پایله;
    پایله.emplace_شا("verilated.cpp");
    إذا (v3Global.dpi()) پایله.emplace_شا("verilated_dpi.cpp");
    إذا (v3Global.opt.vpi()) پایله.emplace_شا("verilated_vpi.cpp");
    إذا (v3Global.opt.savable()) پایله.emplace_شا("verilated_save.cpp");
    إذا (v3Global.opt.coverage()) پایله.emplace_شا("verilated_cov.cpp");
    إذا (v3Global.opt.coverage() || v3Global.useCovergroup())
        پایله.emplace_شا("verilated_covergroup.cpp");
    لكل (ثابت string& بنسټ : v3Global.opt.trace_سرچینه_Bases())
        پایله.emplace_شا(بنسټ + "_c.cpp");
    إذا (v3Global.کاروي_Prob_Dist()) پایله.emplace_شا("verilated_probdist.cpp");
    إذا (v3Global.کاروي_Timing()) پایله.emplace_شا("verilated_timing.cpp");
    إذا (v3Global.useRandomizeMethods()) پایله.emplace_شا("verilated_random.cpp");
    پایله.emplace_شا("verilated_threads.cpp");
    إذا (v3Global.opt.کاروي_Profiler()) پایله.emplace_شا("verilated_profiler.cpp");
    أرجع پایله;
}
