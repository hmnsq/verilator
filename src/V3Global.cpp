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
बाह्य "C" स्थिरांक अक्षर* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    परत ""
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
    m_rootp = नवीन AstNetlist;
    m_constPoolp = नवीन V3ConstPool;
    m_libMapp = नवीन V3LibMap;
}

शून्य V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    जर (m_rootp) VL_DO_CLEAR(m_rootp->delete_वृक्ष(), m_rootp = nullptr);
#endif
    File_ओळ::deleteAllRemaining();
}

शून्य V3Global::vlExit(पूर्णांक status) {
    shutdown();
    std::निकास(status);
}

शून्य V3Global::check_वृक्ष() स्थिरांक { rootp()->check_वृक्ष(); }

शून्य V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    स्थिरांक VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        जर (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                नवीन File_ओळ{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), असत्य,
                असत्य, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        साठी (स्थिरांक VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(नवीन File_ओळ{File_ओळ::command_ओळ_Filename()}, filelib.filename(),
                             असत्य, असत्य, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        जर (v3Global.opt.stdPackage()) {
            parser.parseFile(
                नवीन File_ओळ{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                असत्य, असत्य, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        साठी (स्थिरांक string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(नवीन File_ओळ{File_ओळ::command_ओळ_Filename()}, filename, असत्य, सत्य,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        साठी (स्थिरांक स्वतः& filelib : v3Global.opt.vFiles()) {
            स्थिरांक string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(नवीन File_ओळ{File_ओळ::command_ओळ_Filename()}, filelib.filename(),
                             असत्य, असत्य, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        साठी (स्थिरांक स्वतः& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(नवीन File_ओळ{File_ओळ::command_ओळ_Filename()}, filelib.filename(),
                             सत्य, असत्य, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        साठी (स्थिरांक स्वतः& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(नवीन File_ओळ{File_ओळ::command_ओळ_Filename()}, filelib.filename(),
                             असत्य, असत्य, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    जर (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_वृक्ष("cells", असत्य, dump_वृक्ष_Either_Level() >= 9);
}

शून्य V3Global::removeStd() {
    // Delete the std package if unused
    जर (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        जर (Ast_गाठ_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_वृक्ष(), stdp);
        }
    }
}

string V3Global::debugFilename(स्थिरांक string& nameComment, पूर्णांक new_संख्या) {
    ++m_debug_File_संख्या;
    जर (new_संख्या) m_debug_File_संख्या = new_संख्या;
    परत opt.hier_शीर्ष_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_संख्या)
           + "_" + nameComment;
}
string V3Global::digitsFilename(पूर्णांक संख्या) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << संख्या;
    परत ss.str();
}

शून्य V3Global::dump_Check_Global_वृक्ष(स्थिरांक string& stagename, पूर्णांक new_संख्या, तार्किक doDump,
                                   तार्किक doCheck) {
    स्थिरांक string वृक्ष_Filename = v3Global.debugFilename(stagename + ".tree", new_संख्या);
    जर (dump_वृक्ष_Level()) v3Global.rootp()->dump_वृक्ष_File(वृक्ष_Filename, doDump);
    जर (dump_वृक्ष_Json_Level()) {
        v3Global.rootp()->dump_वृक्ष_Json_File(वृक्ष_Filename + ".json", doDump);
    }
    जर (v3Global.opt.dump_वृक्ष_Dot()) {
        v3Global.rootp()->dump_वृक्ष_Dot_File(वृक्ष_Filename + ".dot", doDump);
    }
    जर (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    जर (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(वृक्ष_Filename + ".v");
    जर (doCheck && (v3Global.opt.debugCheck() || dump_वृक्ष_Either_Level())) {
        // Error check
        v3Global.rootp()->check_वृक्ष();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

शून्य V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    साठी (स्थिरांक स्वतः& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

शून्य V3Global::saveJsonPtrFieldName(स्थिरांक std::string& fieldName) {
    m_jsonPtrNames.घाला(fieldName);
}

शून्य V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    साठी (स्थिरांक स्वतः& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

स्थिरांक std::string& V3Global::ptrToId(स्थिरांक शून्य* p) {
    स्थिरांक स्वतः pair = m_ptrToId.emplace(p, "");
    जर (pair.second) {
        std::ostringstream os;
        जर (p) {
            os << "(";
            अचिन्हित id = m_ptrToId.size();
            करा { os << static_cast<अक्षर>('A' + id % 26); } पर्यंत (id /= 26);
            os << ")";
        } अन्यथा {
            os << "0";
        }
        pair.first->second = os.str();
    }
    परत pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    जर (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    जर (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    जर (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    जर (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    जर (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    साठी (स्थिरांक string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    जर (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    जर (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    जर (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    जर (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    परत result;
}
