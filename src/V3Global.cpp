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

#समावेश "V3PchAstMT.h"

#समावेश "V3Global.h"

#समावेश "V3ConstPool.h"
#समावेश "V3EmitV.h"
#समावेश "V3Error.h"
#समावेश "V3File.h"
#समावेश "V3HierBlock.h"
#समावेश "V3LibMap.h"
#समावेश "V3LinkCells.h"
#समावेश "V3Parse.h"
#समावेश "V3ParseImp.h"
#समावेश "V3PreShell.h"
#समावेश "V3Stats.h"
#समावेश "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#अगर_घोषित VL_ASAN
बाह्य "C" स्थिर अक्षर* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    वापस ""
#अगर_अघोषित VL_LEAK_CHECKS
           ":detect_leaks=0"
#पूर्ण_अगर
        ;
}
#पूर्ण_अगर

//######################################################################
// V3Global

व्योम V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = नया AstNetlist;
    m_constPoolp = नया V3ConstPool;
    m_libMapp = नया V3LibMap;
}

व्योम V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(मिटाओ m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(मिटाओ m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(मिटाओ m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(मिटाओ m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#अगर_घोषित VL_LEAK_CHECKS
    अगर (m_rootp) VL_DO_CLEAR(m_rootp->delete_वृक्ष(), m_rootp = nullptr);
#पूर्ण_अगर
    File_पंक्ति::deleteAllRemaining();
}

व्योम V3Global::vlExit(पूर्णांक status) {
    shutdown();
    std::निकास(status);
}

व्योम V3Global::check_वृक्ष() स्थिर { rootp()->check_वृक्ष(); }

व्योम V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    स्थिर VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        अगर (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                नया File_पंक्ति{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), false,
                false, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        क्रम (स्थिर VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()}, filelib.filename(),
                             false, false, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        अगर (v3Global.opt.stdPackage()) {
            parser.parseFile(
                नया File_पंक्ति{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                false, false, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        क्रम (स्थिर string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()}, filename, false, true,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        क्रम (स्थिर स्वतः& filelib : v3Global.opt.vFiles()) {
            स्थिर string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()}, filelib.filename(),
                             false, false, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        क्रम (स्थिर स्वतः& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()}, filelib.filename(),
                             true, false, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        क्रम (स्थिर स्वतः& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()}, filelib.filename(),
                             false, false, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    अगर (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_वृक्ष("cells", false, dump_वृक्ष_Either_Level() >= 9);
}

व्योम V3Global::removeStd() {
    // Delete the std package if unused
    अगर (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        अगर (Ast_गाँठ_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_वृक्ष(), stdp);
        }
    }
}

string V3Global::debugFilename(स्थिर string& nameComment, पूर्णांक new_संख्या) {
    ++m_debug_File_संख्या;
    अगर (new_संख्या) m_debug_File_संख्या = new_संख्या;
    वापस opt.hier_शीर्ष_तथ्य_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_संख्या)
           + "_" + nameComment;
}
string V3Global::digitsFilename(पूर्णांक संख्या) {
    std::stringstream ss;
    ss << std::रखो_भरो('0') << std::रखो_चौड़ाई(3) << संख्या;
    वापस ss.str();
}

व्योम V3Global::dump_Check_Global_वृक्ष(स्थिर string& stagename, पूर्णांक new_संख्या, bool doDump,
                                   bool doCheck) {
    स्थिर string वृक्ष_Filename = v3Global.debugFilename(stagename + ".tree", new_संख्या);
    अगर (dump_वृक्ष_Level()) v3Global.rootp()->dump_वृक्ष_File(वृक्ष_Filename, doDump);
    अगर (dump_वृक्ष_Json_Level()) {
        v3Global.rootp()->dump_वृक्ष_Json_File(वृक्ष_Filename + ".json", doDump);
    }
    अगर (v3Global.opt.dump_वृक्ष_Dot()) {
        v3Global.rootp()->dump_वृक्ष_Dot_File(वृक्ष_Filename + ".dot", doDump);
    }
    अगर (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    अगर (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(वृक्ष_Filename + ".v");
    अगर (doCheck && (v3Global.opt.debugCheck() || dump_वृक्ष_Either_Level())) {
        // Error check
        v3Global.rootp()->check_वृक्ष();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

व्योम V3Global::idPtrMapDumpJson(std::ब_प्रवाह& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    क्रम (स्थिर स्वतः& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

व्योम V3Global::saveJsonPtrFieldName(स्थिर std::string& fieldName) {
    m_jsonPtrNames.जोड़ो(fieldName);
}

व्योम V3Global::ptrNamesDumpJson(std::ब_प्रवाह& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    क्रम (स्थिर स्वतः& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

स्थिर std::string& V3Global::ptrToId(स्थिर व्योम* p) {
    स्थिर स्वतः pair = m_ptrToId.emplace(p, "");
    अगर (pair.second) {
        std::ostringstream os;
        अगर (p) {
            os << "(";
            अचिन्हित id = m_ptrToId.आकार();
            करो { os << static_cast<अक्षर>('A' + id % 26); } जबतक (id /= 26);
            os << ")";
        } अन्यथा {
            os << "0";
        }
        pair.first->second = os.str();
    }
    वापस pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> परिणाम;
    परिणाम.emplace_back("verilated.cpp");
    अगर (v3Global.dpi()) परिणाम.emplace_back("verilated_dpi.cpp");
    अगर (v3Global.opt.vpi()) परिणाम.emplace_back("verilated_vpi.cpp");
    अगर (v3Global.opt.savable()) परिणाम.emplace_back("verilated_save.cpp");
    अगर (v3Global.opt.coverage()) परिणाम.emplace_back("verilated_cov.cpp");
    अगर (v3Global.opt.coverage() || v3Global.useCovergroup())
        परिणाम.emplace_back("verilated_covergroup.cpp");
    क्रम (स्थिर string& base : v3Global.opt.traceSourceBases())
        परिणाम.emplace_back(base + "_c.cpp");
    अगर (v3Global.usesProbDist()) परिणाम.emplace_back("verilated_probdist.cpp");
    अगर (v3Global.usesTiming()) परिणाम.emplace_back("verilated_timing.cpp");
    अगर (v3Global.useRandomizeMethods()) परिणाम.emplace_back("verilated_random.cpp");
    परिणाम.emplace_back("verilated_threads.cpp");
    अगर (v3Global.opt.usesProfiler()) परिणाम.emplace_back("verilated_profiler.cpp");
    वापस परिणाम;
}
