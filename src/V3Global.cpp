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

#جے_تعریف_شدہ VL_ASAN
خارجي "C" ثابت حرف* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    أرجع ""
#جے_بنا_تعریف VL_LEAK_CHECKS
           ":detect_leaks=0"
#جے_انت
        ;
}
#جے_انت

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
    VL_DO_CLEAR(مٹاؤ m_hierGraphp, m_hierGraphp = خالی_سوچک);  // delete nullptr is safe
    VL_DO_CLEAR(مٹاؤ m_threadPoolp, m_threadPoolp = خالی_سوچک);  // delete nullptr is safe
    VL_DO_CLEAR(مٹاؤ m_libMapp, m_libMapp = خالی_سوچک);  // delete nullptr is safe
    VL_DO_CLEAR(مٹاؤ m_constPoolp, m_constPoolp = خالی_سوچک);  // delete nullptr is safe
#جے_تعریف_شدہ VL_LEAK_CHECKS
    إذا (m_rootp) VL_DO_CLEAR(m_rootp->delete_درخت(), m_rootp = خالی_سوچک);
#جے_انت
    File_سطر::delete_All_بچیا();
}

فراغ V3Global::vlExit(صحيح حالت) {
    shutdown();
    std::باہر_نکلو(حالت);
}

فراغ V3Global::check_درخت() ثابت { rootp()->check_درخت(); }

فراغ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser4_وچ_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        إذا (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                جديد File_سطر{V3Options::get_Std_Waiver_رستہ()}, V3Options::get_Std_Waiver_رستہ(), خطأ,
                خطأ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        لكل (ثابت VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(جديد File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             خطأ, خطأ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        إذا (v3Global.opt.stdPackage()) {
            parser.parseFile(
                جديد File_سطر{V3Options::get_Std_Package_رستہ()}, V3Options::get_Std_Package_رستہ(),
                خطأ, خطأ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        لكل (ثابت string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(جديد File_سطر{File_سطر::command_سطر_Filename()}, filename, خطأ, سچ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        لكل (ثابت خودکار& filelib : v3Global.opt.vFiles()) {
            ثابت string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->میل_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(جديد File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             خطأ, خطأ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        لكل (ثابت خودکار& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(جديد File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             سچ, خطأ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        لكل (ثابت خودکار& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(جديد File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
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

    V3Global::dump_Check_Global_درخت("cells", خطأ, dump_درخت_Either_Level() >= 9);
}

فراغ V3Global::removeStd() {
    // Delete the std package if unused
    إذا (!ورتدا_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        إذا (Ast_گرہ_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(خالی_سوچک);
            v3Global.rootp()->stdPackageProcessp(خالی_سوچک);
            VL_DO_DANGLING(stdp->unlink_Fr_پٹھ()->delete_درخت(), stdp);
        }
    }
}

string V3Global::debugFilename(ثابت string& nameComment, صحيح new_نمبر) {
    ++m_debug_File_نمبر;
    إذا (new_نمبر) m_debug_File_نمبر = new_نمبر;
    أرجع opt.hier_چوٹی_انکڑے_Dir() + "/" + opt.اگیتر() + "_" + digitsFilename(m_debug_File_نمبر)
           + "_" + nameComment;
}
string V3Global::digitsFilename(صحيح نمبر) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << نمبر;
    أرجع ss.str();
}

فراغ V3Global::dump_Check_Global_درخت(ثابت string& stagename, صحيح new_نمبر, منطقي doDump,
                                   منطقي doCheck) {
    ثابت string درخت_Filename = v3Global.debugFilename(stagename + ".tree", new_نمبر);
    إذا (dump_درخت_Level()) v3Global.rootp()->dump_درخت_File(درخت_Filename, doDump);
    إذا (dump_درخت_Json_Level()) {
        v3Global.rootp()->dump_درخت_Json_File(درخت_Filename + ".json", doDump);
    }
    إذا (v3Global.opt.dump_درخت_Dot()) {
        v3Global.rootp()->dump_درخت_Dot_File(درخت_Filename + ".dot", doDump);
    }
    إذا (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    إذا (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(درخت_Filename + ".v");
    إذا (doCheck && (v3Global.opt.debugCheck() || dump_درخت_Either_Level())) {
        // Error check
        v3Global.rootp()->check_درخت();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

فراغ V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    لكل (ثابت خودکار& itr : m_ptr_منزل_Id) {
        os << sep << '"' << itr.دوجا << "\": \"" << cvt_منزل_Hex(itr.پہلا) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

فراغ V3Global::save_Json_Ptr_میدان_Name(ثابت std::string& میدان_Name) {
    m_json_Ptr_ناں.داخل_کرو(میدان_Name);
}

فراغ V3Global::ptr_ناں_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    لكل (ثابت خودکار& itr : m_json_Ptr_ناں) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ثابت std::string& V3Global::ptr_منزل_Id(ثابت فراغ* p) {
    ثابت خودکار pair = m_ptr_منزل_Id.emplace(p, "");
    إذا (pair.دوجا) {
        std::ostringstream os;
        إذا (p) {
            os << "(";
            بنانشانی id = m_ptr_منزل_Id.ناپ();
            افعل { os << static_cast<حرف>('A' + id % 26); } طالما (id /= 26);
            os << ")";
        } وإلا {
            os << "0";
        }
        pair.پہلا->دوجا = os.str();
    }
    أرجع pair.پہلا->دوجا;
}

std::سمتیہ<std::string> V3Global::verilatedCppFiles() {
    std::سمتیہ<std::string> نتیجہ;
    نتیجہ.emplace_پٹھ("verilated.cpp");
    إذا (v3Global.dpi()) نتیجہ.emplace_پٹھ("verilated_dpi.cpp");
    إذا (v3Global.opt.vpi()) نتیجہ.emplace_پٹھ("verilated_vpi.cpp");
    إذا (v3Global.opt.savable()) نتیجہ.emplace_پٹھ("verilated_save.cpp");
    إذا (v3Global.opt.coverage()) نتیجہ.emplace_پٹھ("verilated_cov.cpp");
    إذا (v3Global.opt.coverage() || v3Global.useCovergroup())
        نتیجہ.emplace_پٹھ("verilated_covergroup.cpp");
    لكل (ثابت string& ادھار : v3Global.opt.trace_سروت_Bases())
        نتیجہ.emplace_پٹھ(ادھار + "_c.cpp");
    إذا (v3Global.ورتدا_Prob_Dist()) نتیجہ.emplace_پٹھ("verilated_probdist.cpp");
    إذا (v3Global.ورتدا_Timing()) نتیجہ.emplace_پٹھ("verilated_timing.cpp");
    إذا (v3Global.useRandomizeMethods()) نتیجہ.emplace_پٹھ("verilated_random.cpp");
    نتیجہ.emplace_پٹھ("verilated_threads.cpp");
    إذا (v3Global.opt.ورتدا_Profiler()) نتیجہ.emplace_پٹھ("verilated_profiler.cpp");
    أرجع نتیجہ;
}
