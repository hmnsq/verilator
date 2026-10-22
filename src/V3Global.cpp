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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Global.h"

#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3HierBlock.h"
#അന്തര്ഭാവ "V3LibMap.h"
#അന്തര്ഭാവ "V3LinkCells.h"
#അന്തര്ഭാവ "V3Parse.h"
#അന്തര്ഭാവ "V3ParseImp.h"
#അന്തര്ഭാവ "V3PreShell.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
ബാഹ്യ "C" സ്ഥിരം അക്ഷരം* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    തിരികെ ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

ശൂന്യം V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = പുതിയ AstNetlist;
    m_constPoolp = പുതിയ V3ConstPool;
    m_libMapp = പുതിയ V3LibMap;
}

ശൂന്യം V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    എങ്കിൽ (m_rootp) VL_DO_CLEAR(m_rootp->delete_വൃക്ഷം(), m_rootp = nullptr);
#endif
    File_വരി::deleteAllRemaining();
}

ശൂന്യം V3Global::vlExit(പൂർണ്ണസംഖ്യ status) {
    shutdown();
    std::exit(status);
}

ശൂന്യം V3Global::check_വൃക്ഷം() സ്ഥിരം { rootp()->check_വൃക്ഷം(); }

ശൂന്യം V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    സ്ഥിരം VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        എങ്കിൽ (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                പുതിയ File_വരി{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), അസത്യം,
                അസത്യം, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        വേണ്ടി (സ്ഥിരം VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(പുതിയ File_വരി{File_വരി::command_വരി_Filename()}, filelib.filename(),
                             അസത്യം, അസത്യം, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        എങ്കിൽ (v3Global.opt.stdPackage()) {
            parser.parseFile(
                പുതിയ File_വരി{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                അസത്യം, അസത്യം, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        വേണ്ടി (സ്ഥിരം string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(പുതിയ File_വരി{File_വരി::command_വരി_Filename()}, filename, അസത്യം, സത്യം,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        വേണ്ടി (സ്ഥിരം auto& filelib : v3Global.opt.vFiles()) {
            സ്ഥിരം string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(പുതിയ File_വരി{File_വരി::command_വരി_Filename()}, filelib.filename(),
                             അസത്യം, അസത്യം, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        വേണ്ടി (സ്ഥിരം auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(പുതിയ File_വരി{File_വരി::command_വരി_Filename()}, filelib.filename(),
                             സത്യം, അസത്യം, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        വേണ്ടി (സ്ഥിരം auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(പുതിയ File_വരി{File_വരി::command_വരി_Filename()}, filelib.filename(),
                             അസത്യം, അസത്യം, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    എങ്കിൽ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_വൃക്ഷം("cells", അസത്യം, dump_വൃക്ഷം_Either_Level() >= 9);
}

ശൂന്യം V3Global::removeStd() {
    // Delete the std package if unused
    എങ്കിൽ (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        എങ്കിൽ (Ast_കണ്ണി_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_വൃക്ഷം(), stdp);
        }
    }
}

string V3Global::debugFilename(സ്ഥിരം string& nameComment, പൂർണ്ണസംഖ്യ new_സംഖ്യ) {
    ++m_debug_File_സംഖ്യ;
    എങ്കിൽ (new_സംഖ്യ) m_debug_File_സംഖ്യ = new_സംഖ്യ;
    തിരികെ opt.hier_മുകൾ_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_സംഖ്യ)
           + "_" + nameComment;
}
string V3Global::digitsFilename(പൂർണ്ണസംഖ്യ സംഖ്യ) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << സംഖ്യ;
    തിരികെ ss.str();
}

ശൂന്യം V3Global::dump_Check_Global_വൃക്ഷം(സ്ഥിരം string& stagename, പൂർണ്ണസംഖ്യ new_സംഖ്യ, തർക്കം doDump,
                                   തർക്കം doCheck) {
    സ്ഥിരം string വൃക്ഷം_Filename = v3Global.debugFilename(stagename + ".tree", new_സംഖ്യ);
    എങ്കിൽ (dump_വൃക്ഷം_Level()) v3Global.rootp()->dump_വൃക്ഷം_File(വൃക്ഷം_Filename, doDump);
    എങ്കിൽ (dump_വൃക്ഷം_Json_Level()) {
        v3Global.rootp()->dump_വൃക്ഷം_Json_File(വൃക്ഷം_Filename + ".json", doDump);
    }
    എങ്കിൽ (v3Global.opt.dump_വൃക്ഷം_Dot()) {
        v3Global.rootp()->dump_വൃക്ഷം_Dot_File(വൃക്ഷം_Filename + ".dot", doDump);
    }
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    എങ്കിൽ (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(വൃക്ഷം_Filename + ".v");
    എങ്കിൽ (doCheck && (v3Global.opt.debugCheck() || dump_വൃക്ഷം_Either_Level())) {
        // Error check
        v3Global.rootp()->check_വൃക്ഷം();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

ശൂന്യം V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    വേണ്ടി (സ്ഥിരം auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

ശൂന്യം V3Global::saveJsonPtrFieldName(സ്ഥിരം std::string& fieldName) {
    m_jsonPtrNames.തിരുകുക(fieldName);
}

ശൂന്യം V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    വേണ്ടി (സ്ഥിരം auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

സ്ഥിരം std::string& V3Global::ptrToId(സ്ഥിരം ശൂന്യം* p) {
    സ്ഥിരം auto pair = m_ptrToId.emplace(p, "");
    എങ്കിൽ (pair.second) {
        std::ostringstream os;
        എങ്കിൽ (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            ചെയ്യുക { os << static_cast<അക്ഷരം>('A' + id % 26); } ആകുമ്പോൾ (id /= 26);
            os << ")";
        } അല്ലെങ്കിൽ {
            os << "0";
        }
        pair.first->second = os.str();
    }
    തിരികെ pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    എങ്കിൽ (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    എങ്കിൽ (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    എങ്കിൽ (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    എങ്കിൽ (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    എങ്കിൽ (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    വേണ്ടി (സ്ഥിരം string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    എങ്കിൽ (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    എങ്കിൽ (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    എങ്കിൽ (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    എങ്കിൽ (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    തിരികെ result;
}
