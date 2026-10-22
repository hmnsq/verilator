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

#ifdef VL_ASAN
خارجي "C" ثابت حرف* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    ܦܢܐ ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

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
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    ܐܢ (m_rootp) VL_DO_CLEAR(m_rootp->deleteTree(), m_rootp = nullptr);
#endif
    FileLine::deleteAllRemaining();
}

فراغ V3Global::vlExit(صحيح status) {
    shutdown();
    std::exit(status);
}

فراغ V3Global::checkTree() ثابت { rootp()->checkTree(); }

فراغ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        ܐܢ (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                جديد FileLine{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), ܕܓܠܐ,
                ܕܓܠܐ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        لكل (ثابت VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(جديد FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             ܕܓܠܐ, ܕܓܠܐ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        ܐܢ (v3Global.opt.stdPackage()) {
            parser.parseFile(
                جديد FileLine{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                ܕܓܠܐ, ܕܓܠܐ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        لكل (ثابت string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(جديد FileLine{FileLine::commandLineFilename()}, filename, ܕܓܠܐ, ܫܪܪܐ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        لكل (ثابت auto& filelib : v3Global.opt.vFiles()) {
            ثابت string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(جديد FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             ܕܓܠܐ, ܕܓܠܐ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        لكل (ثابت auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(جديد FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             ܫܪܪܐ, ܕܓܠܐ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        لكل (ثابت auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(جديد FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             ܕܓܠܐ, ܕܓܠܐ, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    ܐܢ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dumpCheckGlobalTree("cells", ܕܓܠܐ, dumpTreeEitherLevel() >= 9);
}

فراغ V3Global::removeStd() {
    // Delete the std package if unused
    ܐܢ (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        ܐܢ (AstNodeModule* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->deleteTree(), stdp);
        }
    }
}

string V3Global::debugFilename(ثابت string& nameComment, صحيح newNumber) {
    ++m_debugFileNumber;
    ܐܢ (newNumber) m_debugFileNumber = newNumber;
    ܦܢܐ opt.hierTopDataDir() + "/" + opt.prefix() + "_" + digitsFilename(m_debugFileNumber)
           + "_" + nameComment;
}
string V3Global::digitsFilename(صحيح number) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << number;
    ܦܢܐ ss.str();
}

فراغ V3Global::dumpCheckGlobalTree(ثابت string& stagename, صحيح newNumber, منطقي doDump,
                                   منطقي doCheck) {
    ثابت string treeFilename = v3Global.debugFilename(stagename + ".tree", newNumber);
    ܐܢ (dumpTreeLevel()) v3Global.rootp()->dumpTreeFile(treeFilename, doDump);
    ܐܢ (dumpTreeJsonLevel()) {
        v3Global.rootp()->dumpTreeJsonFile(treeFilename + ".json", doDump);
    }
    ܐܢ (v3Global.opt.dumpTreeDot()) {
        v3Global.rootp()->dumpTreeDotFile(treeFilename + ".dot", doDump);
    }
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    ܐܢ (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(treeFilename + ".v");
    ܐܢ (doCheck && (v3Global.opt.debugCheck() || dumpTreeEitherLevel())) {
        // Error check
        v3Global.rootp()->checkTree();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

فراغ V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    لكل (ثابت auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

فراغ V3Global::saveJsonPtrFieldName(ثابت std::string& fieldName) {
    m_jsonPtrNames.insert(fieldName);
}

فراغ V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    لكل (ثابت auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ثابت std::string& V3Global::ptrToId(ثابت فراغ* p) {
    ثابت auto pair = m_ptrToId.emplace(p, "");
    ܐܢ (pair.second) {
        std::ostringstream os;
        ܐܢ (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            ܥܒܕ { os << static_cast<حرف>('A' + id % 26); } ܟܡܐ (id /= 26);
            os << ")";
        } ܐܚܪܢܐ {
            os << "0";
        }
        pair.first->second = os.str();
    }
    ܦܢܐ pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    ܐܢ (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    ܐܢ (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    ܐܢ (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    ܐܢ (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    ܐܢ (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    لكل (ثابت string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    ܐܢ (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    ܐܢ (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    ܐܢ (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    ܐܢ (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    ܦܢܐ result;
}
