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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3Global.h"

#অন্তর্ভাৱ "V3ConstPool.h"
#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3HierBlock.h"
#অন্তর্ভাৱ "V3LibMap.h"
#অন্তর্ভাৱ "V3LinkCells.h"
#অন্তর্ভাৱ "V3Parse.h"
#অন্তর্ভাৱ "V3ParseImp.h"
#অন্তর্ভাৱ "V3PreShell.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
বাহ্য "C" ধ্রুৱ ৱর্ণ* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    ঘূৰাই ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

শূন্য V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = নূতন AstNetlist;
    m_constPoolp = নূতন V3ConstPool;
    m_libMapp = নূতন V3LibMap;
}

শূন্য V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    যদি (m_rootp) VL_DO_CLEAR(m_rootp->delete_বৃক্ষ(), m_rootp = nullptr);
#endif
    File_সাৰি::deleteAllRemaining();
}

শূন্য V3Global::vlExit(পূর্ণাঙ্ক status) {
    shutdown();
    std::exit(status);
}

শূন্য V3Global::check_বৃক্ষ() ধ্রুৱ { rootp()->check_বৃক্ষ(); }

শূন্য V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ধ্রুৱ VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        যদি (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                নূতন File_সাৰি{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), মিছা,
                মিছা, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        বাবে (ধ্রুৱ VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()}, filelib.filename(),
                             মিছা, মিছা, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        যদি (v3Global.opt.stdPackage()) {
            parser.parseFile(
                নূতন File_সাৰি{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                মিছা, মিছা, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        বাবে (ধ্রুৱ string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()}, filename, মিছা, সঁচা,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        বাবে (ধ্রুৱ auto& filelib : v3Global.opt.vFiles()) {
            ধ্রুৱ string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()}, filelib.filename(),
                             মিছা, মিছা, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        বাবে (ধ্রুৱ auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()}, filelib.filename(),
                             সঁচা, মিছা, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        বাবে (ধ্রুৱ auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()}, filelib.filename(),
                             মিছা, মিছা, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    যদি (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_বৃক্ষ("cells", মিছা, dump_বৃক্ষ_Either_Level() >= 9);
}

শূন্য V3Global::removeStd() {
    // Delete the std package if unused
    যদি (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        যদি (Ast_গ্ৰন্থি_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_বৃক্ষ(), stdp);
        }
    }
}

string V3Global::debugFilename(ধ্রুৱ string& nameComment, পূর্ণাঙ্ক new_সংখ্যা) {
    ++m_debug_File_সংখ্যা;
    যদি (new_সংখ্যা) m_debug_File_সংখ্যা = new_সংখ্যা;
    ঘূৰাই opt.hier_শীৰ্ষ_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_সংখ্যা)
           + "_" + nameComment;
}
string V3Global::digitsFilename(পূর্ণাঙ্ক সংখ্যা) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << সংখ্যা;
    ঘূৰাই ss.str();
}

শূন্য V3Global::dump_Check_Global_বৃক্ষ(ধ্রুৱ string& stagename, পূর্ণাঙ্ক new_সংখ্যা, তার্কিক doDump,
                                   তার্কিক doCheck) {
    ধ্রুৱ string বৃক্ষ_Filename = v3Global.debugFilename(stagename + ".tree", new_সংখ্যা);
    যদি (dump_বৃক্ষ_Level()) v3Global.rootp()->dump_বৃক্ষ_File(বৃক্ষ_Filename, doDump);
    যদি (dump_বৃক্ষ_Json_Level()) {
        v3Global.rootp()->dump_বৃক্ষ_Json_File(বৃক্ষ_Filename + ".json", doDump);
    }
    যদি (v3Global.opt.dump_বৃক্ষ_Dot()) {
        v3Global.rootp()->dump_বৃক্ষ_Dot_File(বৃক্ষ_Filename + ".dot", doDump);
    }
    যদি (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    যদি (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(বৃক্ষ_Filename + ".v");
    যদি (doCheck && (v3Global.opt.debugCheck() || dump_বৃক্ষ_Either_Level())) {
        // Error check
        v3Global.rootp()->check_বৃক্ষ();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

শূন্য V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    বাবে (ধ্রুৱ auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

শূন্য V3Global::saveJsonPtrFieldName(ধ্রুৱ std::string& fieldName) {
    m_jsonPtrNames.ঢোকাও(fieldName);
}

শূন্য V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    বাবে (ধ্রুৱ auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ধ্রুৱ std::string& V3Global::ptrToId(ধ্রুৱ শূন্য* p) {
    ধ্রুৱ auto pair = m_ptrToId.emplace(p, "");
    যদি (pair.second) {
        std::ostringstream os;
        যদি (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            কৰা { os << static_cast<ৱর্ণ>('A' + id % 26); } যেতিয়ালৈ (id /= 26);
            os << ")";
        } অন্যথা {
            os << "0";
        }
        pair.first->second = os.str();
    }
    ঘূৰাই pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    যদি (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    যদি (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    যদি (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    যদি (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    যদি (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    বাবে (ধ্রুৱ string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    যদি (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    যদি (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    যদি (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    যদি (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    ঘূৰাই result;
}
