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

#جيڪڏهن_وصف_ٿيل VL_ASAN
خارجي "C" ثابت حرف* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    أرجع ""
#جيڪڏهن_وصف_نه_ٿيل VL_LEAK_CHECKS
           ":detect_leaks=0"
#جيڪڏهن_پورو
        ;
}
#جيڪڏهن_پورو

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
    VL_DO_CLEAR(ميٽيو m_hierGraphp, m_hierGraphp = خالي_اشارو);  // delete nullptr is safe
    VL_DO_CLEAR(ميٽيو m_threadPoolp, m_threadPoolp = خالي_اشارو);  // delete nullptr is safe
    VL_DO_CLEAR(ميٽيو m_libMapp, m_libMapp = خالي_اشارو);  // delete nullptr is safe
    VL_DO_CLEAR(ميٽيو m_constPoolp, m_constPoolp = خالي_اشارو);  // delete nullptr is safe
#جيڪڏهن_وصف_ٿيل VL_LEAK_CHECKS
    إذا (m_rootp) VL_DO_CLEAR(m_rootp->delete_وڻ(), m_rootp = خالي_اشارو);
#جيڪڏهن_پورو
    File_سٽ::delete_All_بچيل();
}

فراغ V3Global::vlExit(صحيح حالت) {
    shutdown();
    std::ٻاهر_نڪرو(حالت);
}

فراغ V3Global::check_وڻ() ثابت { rootp()->check_وڻ(); }

فراغ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser4_اندريون_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        إذا (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                جديد File_سٽ{V3Options::get_Std_Waiver_رستو()}, V3Options::get_Std_Waiver_رستو(), خطأ,
                خطأ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        لكل (ثابت VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(جديد File_سٽ{File_سٽ::command_سٽ_Filename()}, filelib.filename(),
                             خطأ, خطأ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        إذا (v3Global.opt.stdPackage()) {
            parser.parseFile(
                جديد File_سٽ{V3Options::get_Std_Package_رستو()}, V3Options::get_Std_Package_رستو(),
                خطأ, خطأ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        لكل (ثابت string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(جديد File_سٽ{File_سٽ::command_سٽ_Filename()}, filename, خطأ, سچ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        لكل (ثابت خودڪار& filelib : v3Global.opt.vFiles()) {
            ثابت string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->ميلاپ_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(جديد File_سٽ{File_سٽ::command_سٽ_Filename()}, filelib.filename(),
                             خطأ, خطأ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        لكل (ثابت خودڪار& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(جديد File_سٽ{File_سٽ::command_سٽ_Filename()}, filelib.filename(),
                             سچ, خطأ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        لكل (ثابت خودڪار& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(جديد File_سٽ{File_سٽ::command_سٽ_Filename()}, filelib.filename(),
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

    V3Global::dump_Check_Global_وڻ("cells", خطأ, dump_وڻ_Either_Level() >= 9);
}

فراغ V3Global::removeStd() {
    // Delete the std package if unused
    إذا (!استعمال_ڪري_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        إذا (Ast_ڳنڍ_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(خالي_اشارو);
            v3Global.rootp()->stdPackageProcessp(خالي_اشارو);
            VL_DO_DANGLING(stdp->unlink_Fr_پٺ()->delete_وڻ(), stdp);
        }
    }
}

string V3Global::debugFilename(ثابت string& nameComment, صحيح new_عدد) {
    ++m_debug_File_عدد;
    إذا (new_عدد) m_debug_File_عدد = new_عدد;
    أرجع opt.hier_چوٽي_معلومات_Dir() + "/" + opt.اڳياڙي() + "_" + digitsFilename(m_debug_File_عدد)
           + "_" + nameComment;
}
string V3Global::digitsFilename(صحيح عدد) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << عدد;
    أرجع ss.str();
}

فراغ V3Global::dump_Check_Global_وڻ(ثابت string& stagename, صحيح new_عدد, منطقي doDump,
                                   منطقي doCheck) {
    ثابت string وڻ_Filename = v3Global.debugFilename(stagename + ".tree", new_عدد);
    إذا (dump_وڻ_Level()) v3Global.rootp()->dump_وڻ_File(وڻ_Filename, doDump);
    إذا (dump_وڻ_Json_Level()) {
        v3Global.rootp()->dump_وڻ_Json_File(وڻ_Filename + ".json", doDump);
    }
    إذا (v3Global.opt.dump_وڻ_Dot()) {
        v3Global.rootp()->dump_وڻ_Dot_File(وڻ_Filename + ".dot", doDump);
    }
    إذا (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    إذا (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(وڻ_Filename + ".v");
    إذا (doCheck && (v3Global.opt.debugCheck() || dump_وڻ_Either_Level())) {
        // Error check
        v3Global.rootp()->check_وڻ();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

فراغ V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    لكل (ثابت خودڪار& itr : m_ptr_منزل_Id) {
        os << sep << '"' << itr.ٻيو << "\": \"" << cvt_منزل_Hex(itr.پهريون) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

فراغ V3Global::save_Json_Ptr_ميدان_Name(ثابت std::string& ميدان_Name) {
    m_json_Ptr_نالا.وجهو(ميدان_Name);
}

فراغ V3Global::ptr_نالا_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    لكل (ثابت خودڪار& itr : m_json_Ptr_نالا) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ثابت std::string& V3Global::ptr_منزل_Id(ثابت فراغ* p) {
    ثابت خودڪار pair = m_ptr_منزل_Id.emplace(p, "");
    إذا (pair.ٻيو) {
        std::ostringstream os;
        إذا (p) {
            os << "(";
            بنانشان id = m_ptr_منزل_Id.سائيز();
            افعل { os << static_cast<حرف>('A' + id % 26); } طالما (id /= 26);
            os << ")";
        } وإلا {
            os << "0";
        }
        pair.پهريون->ٻيو = os.str();
    }
    أرجع pair.پهريون->ٻيو;
}

std::ويڪٽر<std::string> V3Global::verilatedCppFiles() {
    std::ويڪٽر<std::string> نتيجو;
    نتيجو.emplace_پٺ("verilated.cpp");
    إذا (v3Global.dpi()) نتيجو.emplace_پٺ("verilated_dpi.cpp");
    إذا (v3Global.opt.vpi()) نتيجو.emplace_پٺ("verilated_vpi.cpp");
    إذا (v3Global.opt.savable()) نتيجو.emplace_پٺ("verilated_save.cpp");
    إذا (v3Global.opt.coverage()) نتيجو.emplace_پٺ("verilated_cov.cpp");
    إذا (v3Global.opt.coverage() || v3Global.useCovergroup())
        نتيجو.emplace_پٺ("verilated_covergroup.cpp");
    لكل (ثابت string& بنياد : v3Global.opt.trace_ذريعو_Bases())
        نتيجو.emplace_پٺ(بنياد + "_c.cpp");
    إذا (v3Global.استعمال_ڪري_Prob_Dist()) نتيجو.emplace_پٺ("verilated_probdist.cpp");
    إذا (v3Global.استعمال_ڪري_Timing()) نتيجو.emplace_پٺ("verilated_timing.cpp");
    إذا (v3Global.useRandomizeMethods()) نتيجو.emplace_پٺ("verilated_random.cpp");
    نتيجو.emplace_پٺ("verilated_threads.cpp");
    إذا (v3Global.opt.استعمال_ڪري_Profiler()) نتيجو.emplace_پٺ("verilated_profiler.cpp");
    أرجع نتيجو;
}
