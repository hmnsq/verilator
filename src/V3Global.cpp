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

#ifdef VL_ASAN
بیرونی "C" ثابت نویسه* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    بازگشت ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

تهی V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = جدید AstNetlist;
    m_constPoolp = جدید V3ConstPool;
    m_libMapp = جدید V3LibMap;
}

تهی V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    اگر (m_rootp) VL_DO_CLEAR(m_rootp->delete_درخت(), m_rootp = nullptr);
#endif
    File_سطر::deleteAllRemaining();
}

تهی V3Global::vlExit(صحیح status) {
    shutdown();
    std::exit(status);
}

تهی V3Global::check_درخت() ثابت { rootp()->check_درخت(); }

تهی V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        اگر (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                جدید File_سطر{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), نادرست,
                نادرست, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        برای (ثابت VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(جدید File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             نادرست, نادرست, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        اگر (v3Global.opt.stdPackage()) {
            parser.parseFile(
                جدید File_سطر{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                نادرست, نادرست, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        برای (ثابت string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(جدید File_سطر{File_سطر::command_سطر_Filename()}, filename, نادرست, درست,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        برای (ثابت auto& filelib : v3Global.opt.vFiles()) {
            ثابت string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(جدید File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             نادرست, نادرست, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        برای (ثابت auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(جدید File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             درست, نادرست, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        برای (ثابت auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(جدید File_سطر{File_سطر::command_سطر_Filename()}, filelib.filename(),
                             نادرست, نادرست, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    اگر (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_درخت("cells", نادرست, dump_درخت_Either_Level() >= 9);
}

تهی V3Global::removeStd() {
    // Delete the std package if unused
    اگر (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        اگر (Ast_گره_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_درخت(), stdp);
        }
    }
}

string V3Global::debugFilename(ثابت string& nameComment, صحیح new_عدد) {
    ++m_debug_File_عدد;
    اگر (new_عدد) m_debug_File_عدد = new_عدد;
    بازگشت opt.hier_بالاترین_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_عدد)
           + "_" + nameComment;
}
string V3Global::digitsFilename(صحیح عدد) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << عدد;
    بازگشت ss.str();
}

تهی V3Global::dump_Check_Global_درخت(ثابت string& stagename, صحیح new_عدد, منطقی doDump,
                                   منطقی doCheck) {
    ثابت string درخت_Filename = v3Global.debugFilename(stagename + ".tree", new_عدد);
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

تهی V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    برای (ثابت auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

تهی V3Global::saveJsonPtrFieldName(ثابت std::string& fieldName) {
    m_jsonPtrNames.درج(fieldName);
}

تهی V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    برای (ثابت auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ثابت std::string& V3Global::ptrToId(ثابت تهی* p) {
    ثابت auto pair = m_ptrToId.emplace(p, "");
    اگر (pair.second) {
        std::ostringstream os;
        اگر (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            انجامبده { os << static_cast<نویسه>('A' + id % 26); } وقتی (id /= 26);
            os << ")";
        } وگرنه {
            os << "0";
        }
        pair.first->second = os.str();
    }
    بازگشت pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    اگر (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    اگر (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    اگر (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    اگر (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    اگر (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    برای (ثابت string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    اگر (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    اگر (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    اگر (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    اگر (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    بازگشت result;
}
