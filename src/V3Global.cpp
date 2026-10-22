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

#شامل "V3PchAstMT.h"

#شامل "V3Global.h"

#شامل "V3ConstPool.h"
#شامل "V3EmitV.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3HierBlock.h"
#شامل "V3LibMap.h"
#شامل "V3LinkCells.h"
#شامل "V3Parse.h"
#شامل "V3ParseImp.h"
#شامل "V3PreShell.h"
#شامل "V3Stats.h"
#شامل "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#اگر_تعریف_شدہ VL_ASAN
خارجی "C" ثابت حرف* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    واپس ""
#اگر_غیر_تعریف_شدہ VL_LEAK_CHECKS
           ":detect_leaks=0"
#اگر_ختم
        ;
}
#اگر_ختم

//######################################################################
// V3Global

خالی V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = نیا AstNetlist;
    m_constPoolp = نیا V3ConstPool;
    m_libMapp = نیا V3LibMap;
}

خالی V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(حذف m_hierGraphp, m_hierGraphp = خالی_اشاریہ);  // delete nullptr is safe
    VL_DO_CLEAR(حذف m_threadPoolp, m_threadPoolp = خالی_اشاریہ);  // delete nullptr is safe
    VL_DO_CLEAR(حذف m_libMapp, m_libMapp = خالی_اشاریہ);  // delete nullptr is safe
    VL_DO_CLEAR(حذف m_constPoolp, m_constPoolp = خالی_اشاریہ);  // delete nullptr is safe
#اگر_تعریف_شدہ VL_LEAK_CHECKS
    اگر (m_rootp) VL_DO_CLEAR(m_rootp->delete_درخت(), m_rootp = خالی_اشاریہ);
#اگر_ختم
    File_سطر::delete_All_بقایا();
}

خالی V3Global::vlExit(صحیح حالت_2) {
    shutdown();
    std::باہر_نکلو(حالت_2);
}

خالی V3Global::check_درخت() ثابت { rootp()->check_درخت(); }

خالی V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser4_اندر_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        اگر (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                نیا File_سطر{V3Options::get_Std_Waiver_راستہ()}, V3Options::get_Std_Waiver_راستہ(), جھوٹ,
                جھوٹ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        برائے (ثابت VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(نیا File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             جھوٹ, جھوٹ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        اگر (v3Global.opt.stdPackage()) {
            parser.parseFile(
                نیا File_سطر{V3Options::get_Std_Package_راستہ()}, V3Options::get_Std_Package_راستہ(),
                جھوٹ, جھوٹ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        برائے (ثابت string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(نیا File_سطر{File_سطر::command_سطر_Filename()}, filename, جھوٹ, سچ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        برائے (ثابت خودکار& filelib : v3Global.opt.vFiles()) {
            ثابت string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->مطابقت_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(نیا File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             جھوٹ, جھوٹ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        برائے (ثابت خودکار& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(نیا File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             سچ, جھوٹ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        برائے (ثابت خودکار& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(نیا File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             جھوٹ, جھوٹ, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    اگر (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_درخت("cells", جھوٹ, dump_درخت_Either_Level() >= 9);
}

خالی V3Global::removeStd() {
    // Delete the std package if unused
    اگر (!استعمال_کرتا_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        اگر (Ast_گرہ_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(خالی_اشاریہ);
            v3Global.rootp()->stdPackageProcessp(خالی_اشاریہ);
            VL_DO_DANGLING(stdp->unlink_Fr_عقب()->delete_درخت(), stdp);
        }
    }
}

string V3Global::debugFilename(ثابت string& nameComment, صحیح new_نمبر) {
    ++m_debug_File_نمبر;
    اگر (new_نمبر) m_debug_File_نمبر = new_نمبر;
    واپس opt.hier_چوٹی_ڈیٹا_Dir() + "/" + opt.سابقہ() + "_" + digitsFilename(m_debug_File_نمبر)
           + "_" + nameComment;
}
string V3Global::digitsFilename(صحیح نمبر) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << نمبر;
    واپس ss.str();
}

خالی V3Global::dump_Check_Global_درخت(ثابت string& stagename, صحیح new_نمبر, منطقی doDump,
                                   منطقی doCheck) {
    ثابت string درخت_Filename = v3Global.debugFilename(stagename + ".tree", new_نمبر);
    اگر (dump_درخت_Level()) v3Global.rootp()->dump_درخت_File(درخت_Filename, doDump);
    اگر (dump_درخت_Json_Level()) {
        v3Global.rootp()->dump_درخت_Json_File(درخت_Filename + ".json", doDump);
    }
    اگر (v3Global.opt.dump_درخت_Dot()) {
        v3Global.rootp()->dump_درخت_Dot_File(درخت_Filename + ".dot", doDump);
    }
    اگر (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    اگر (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(درخت_Filename + ".v");
    اگر (doCheck && (v3Global.opt.debugCheck() || dump_درخت_Either_Level())) {
        // Error check
        v3Global.rootp()->check_درخت();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

خالی V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    برائے (ثابت خودکار& itr : m_ptr_مقصد_Id) {
        os << sep << '"' << itr.دوسرا << "\": \"" << cvt_مقصد_Hex(itr.پہلا) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

خالی V3Global::save_Json_Ptr_میدان_Name(ثابت std::string& میدان_Name) {
    m_json_Ptr_نام.داخل_کرو(میدان_Name);
}

خالی V3Global::ptr_نام_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    برائے (ثابت خودکار& itr : m_json_Ptr_نام) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ثابت std::string& V3Global::ptr_مقصد_Id(ثابت خالی* p) {
    ثابت خودکار pair = m_ptr_مقصد_Id.emplace(p, "");
    اگر (pair.دوسرا) {
        std::ostringstream os;
        اگر (p) {
            os << "(";
            غیرعلامتی id = m_ptr_مقصد_Id.سائز();
            کرو { os << static_cast<حرف>('A' + id % 26); } جب (id /= 26);
            os << ")";
        } ورنہ {
            os << "0";
        }
        pair.پہلا->دوسرا = os.str();
    }
    واپس pair.پہلا->دوسرا;
}

std::سمتیہ<std::string> V3Global::verilatedCppFiles() {
    std::سمتیہ<std::string> نتیجہ;
    نتیجہ.emplace_عقب("verilated.cpp");
    اگر (v3Global.dpi()) نتیجہ.emplace_عقب("verilated_dpi.cpp");
    اگر (v3Global.opt.vpi()) نتیجہ.emplace_عقب("verilated_vpi.cpp");
    اگر (v3Global.opt.savable()) نتیجہ.emplace_عقب("verilated_save.cpp");
    اگر (v3Global.opt.coverage()) نتیجہ.emplace_عقب("verilated_cov.cpp");
    اگر (v3Global.opt.coverage() || v3Global.useCovergroup())
        نتیجہ.emplace_عقب("verilated_covergroup.cpp");
    برائے (ثابت string& بنیاد : v3Global.opt.trace_ماخذ_Bases())
        نتیجہ.emplace_عقب(بنیاد + "_c.cpp");
    اگر (v3Global.استعمال_کرتا_Prob_Dist()) نتیجہ.emplace_عقب("verilated_probdist.cpp");
    اگر (v3Global.استعمال_کرتا_Timing()) نتیجہ.emplace_عقب("verilated_timing.cpp");
    اگر (v3Global.useRandomizeMethods()) نتیجہ.emplace_عقب("verilated_random.cpp");
    نتیجہ.emplace_عقب("verilated_threads.cpp");
    اگر (v3Global.opt.استعمال_کرتا_Profiler()) نتیجہ.emplace_عقب("verilated_profiler.cpp");
    واپس نتیجہ;
}
