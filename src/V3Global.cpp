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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Global.h"

#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3HierBlock.h"
#अन्तर्भाव "V3LibMap.h"
#अन्तर्भाव "V3LinkCells.h"
#अन्तर्भाव "V3Parse.h"
#अन्तर्भाव "V3ParseImp.h"
#अन्तर्भाव "V3PreShell.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
बाह्य "C" ध्रुव वर्ण* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    प्रत्यागम ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

शून्य V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = नूतन AstNetlist;
    m_constPoolp = नूतन V3ConstPool;
    m_libMapp = नूतन V3LibMap;
}

शून्य V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    जइ (m_rootp) VL_DO_CLEAR(m_rootp->delete_वृक्षः(), m_rootp = nullptr);
#endif
    File_पङ्क्तिः::deleteAllRemaining();
}

शून्य V3Global::vlExit(पूर्णाङ्क status) {
    shutdown();
    std::exit(status);
}

शून्य V3Global::check_वृक्षः() ध्रुव { rootp()->check_वृक्षः(); }

शून्य V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ध्रुव VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        जइ (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                नूतन File_पङ्क्तिः{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), मिच्छ,
                मिच्छ, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        प्रति (ध्रुव VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             मिच्छ, मिच्छ, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        जइ (v3Global.opt.stdPackage()) {
            parser.parseFile(
                नूतन File_पङ्क्तिः{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                मिच्छ, मिच्छ, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        प्रति (ध्रुव string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filename, मिच्छ, सच्च,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        प्रति (ध्रुव auto& filelib : v3Global.opt.vFiles()) {
            ध्रुव string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             मिच्छ, मिच्छ, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        प्रति (ध्रुव auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             सच्च, मिच्छ, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        प्रति (ध्रुव auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             मिच्छ, मिच्छ, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    जइ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_वृक्षः("cells", मिच्छ, dump_वृक्षः_Either_Level() >= 9);
}

शून्य V3Global::removeStd() {
    // Delete the std package if unused
    जइ (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        जइ (Ast_ग्रन्थिः_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_वृक्षः(), stdp);
        }
    }
}

string V3Global::debugFilename(ध्रुव string& nameComment, पूर्णाङ्क new_सङ्ख्या) {
    ++m_debug_File_सङ्ख्या;
    जइ (new_सङ्ख्या) m_debug_File_सङ्ख्या = new_सङ्ख्या;
    प्रत्यागम opt.hier_शिखरम्_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_सङ्ख्या)
           + "_" + nameComment;
}
string V3Global::digitsFilename(पूर्णाङ्क सङ्ख्या) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << सङ्ख्या;
    प्रत्यागम ss.str();
}

शून्य V3Global::dump_Check_Global_वृक्षः(ध्रुव string& stagename, पूर्णाङ्क new_सङ्ख्या, तार्किक doDump,
                                   तार्किक doCheck) {
    ध्रुव string वृक्षः_Filename = v3Global.debugFilename(stagename + ".tree", new_सङ्ख्या);
    जइ (dump_वृक्षः_Level()) v3Global.rootp()->dump_वृक्षः_File(वृक्षः_Filename, doDump);
    जइ (dump_वृक्षः_Json_Level()) {
        v3Global.rootp()->dump_वृक्षः_Json_File(वृक्षः_Filename + ".json", doDump);
    }
    जइ (v3Global.opt.dump_वृक्षः_Dot()) {
        v3Global.rootp()->dump_वृक्षः_Dot_File(वृक्षः_Filename + ".dot", doDump);
    }
    जइ (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    जइ (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(वृक्षः_Filename + ".v");
    जइ (doCheck && (v3Global.opt.debugCheck() || dump_वृक्षः_Either_Level())) {
        // Error check
        v3Global.rootp()->check_वृक्षः();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

शून्य V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    प्रति (ध्रुव auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

शून्य V3Global::saveJsonPtrFieldName(ध्रुव std::string& fieldName) {
    m_jsonPtrNames.निवेशय(fieldName);
}

शून्य V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    प्रति (ध्रुव auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ध्रुव std::string& V3Global::ptrToId(ध्रुव शून्य* p) {
    ध्रुव auto pair = m_ptrToId.emplace(p, "");
    जइ (pair.second) {
        std::ostringstream os;
        जइ (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            करेहि { os << static_cast<वर्ण>('A' + id % 26); } जाव (id /= 26);
            os << ")";
        } अण्णहा {
            os << "0";
        }
        pair.first->second = os.str();
    }
    प्रत्यागम pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    जइ (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    जइ (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    जइ (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    जइ (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    जइ (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    प्रति (ध्रुव string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    जइ (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    जइ (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    जइ (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    जइ (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    प्रत्यागम result;
}
