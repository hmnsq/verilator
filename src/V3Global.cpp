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

#यदि_परिभाषितम् VL_ASAN
बाह्य "C" ध्रुव वर्ण* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    प्रत्यागम ""
#यदि_अपरिभाषितम् VL_LEAK_CHECKS
           ":detect_leaks=0"
#यदि_अन्तः
        ;
}
#यदि_अन्तः

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
    VL_DO_CLEAR(विलोपय m_hierGraphp, m_hierGraphp = शून्यसूचकः);  // delete nullptr is safe
    VL_DO_CLEAR(विलोपय m_threadPoolp, m_threadPoolp = शून्यसूचकः);  // delete nullptr is safe
    VL_DO_CLEAR(विलोपय m_libMapp, m_libMapp = शून्यसूचकः);  // delete nullptr is safe
    VL_DO_CLEAR(विलोपय m_constPoolp, m_constPoolp = शून्यसूचकः);  // delete nullptr is safe
#यदि_परिभाषितम् VL_LEAK_CHECKS
    यदि (m_rootp) VL_DO_CLEAR(m_rootp->delete_वृक्षः(), m_rootp = शून्यसूचकः);
#यदि_अन्तः
    File_पङ्क्तिः::deleteAllRemaining();
}

शून्य V3Global::vlExit(पूर्णाङ्क स्थितिः) {
    shutdown();
    std::निर्गच्छ(स्थितिः);
}

शून्य V3Global::check_वृक्षः() ध्रुव { rootp()->check_वृक्षः(); }

शून्य V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ध्रुव VNUser4_अन्तर्_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        यदि (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                नूतन File_पङ्क्तिः{V3Options::get_Std_Waiver_मार्गः()}, V3Options::get_Std_Waiver_मार्गः(), असत्य,
                असत्य, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        प्रति (ध्रुव VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             असत्य, असत्य, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        यदि (v3Global.opt.stdPackage()) {
            parser.parseFile(
                नूतन File_पङ्क्तिः{V3Options::get_Std_Package_मार्गः()}, V3Options::get_Std_Package_मार्गः(),
                असत्य, असत्य, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        प्रति (ध्रुव string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filename, असत्य, सत्य,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        प्रति (ध्रुव स्वचालित& filelib : v3Global.opt.vFiles()) {
            ध्रुव string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->मेलनम्_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             असत्य, असत्य, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        प्रति (ध्रुव स्वचालित& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             सत्य, असत्य, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        प्रति (ध्रुव स्वचालित& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()}, filelib.filename(),
                             असत्य, असत्य, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    यदि (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_वृक्षः("cells", असत्य, dump_वृक्षः_Either_Level() >= 9);
}

शून्य V3Global::removeStd() {
    // Delete the std package if unused
    यदि (!उपयुङ्क्ते_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        यदि (Ast_ग्रन्थिः_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(शून्यसूचकः);
            v3Global.rootp()->stdPackageProcessp(शून्यसूचकः);
            VL_DO_DANGLING(stdp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), stdp);
        }
    }
}

string V3Global::debugFilename(ध्रुव string& nameComment, पूर्णाङ्क new_सङ्ख्या) {
    ++m_debug_File_सङ्ख्या;
    यदि (new_सङ्ख्या) m_debug_File_सङ्ख्या = new_सङ्ख्या;
    प्रत्यागम opt.hier_शिखरम्_दत्त_Dir() + "/" + opt.उपसर्गः() + "_" + digitsFilename(m_debug_File_सङ्ख्या)
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
    यदि (dump_वृक्षः_Level()) v3Global.rootp()->dump_वृक्षः_File(वृक्षः_Filename, doDump);
    यदि (dump_वृक्षः_Json_Level()) {
        v3Global.rootp()->dump_वृक्षः_Json_File(वृक्षः_Filename + ".json", doDump);
    }
    यदि (v3Global.opt.dump_वृक्षः_Dot()) {
        v3Global.rootp()->dump_वृक्षः_Dot_File(वृक्षः_Filename + ".dot", doDump);
    }
    यदि (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    यदि (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(वृक्षः_Filename + ".v");
    यदि (doCheck && (v3Global.opt.debugCheck() || dump_वृक्षः_Either_Level())) {
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
    प्रति (ध्रुव स्वचालित& itr : m_ptr_गन्तव्यम्_Id) {
        os << sep << '"' << itr.second << "\": \"" << cvt_गन्तव्यम्_Hex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

शून्य V3Global::save_Json_Ptr_क्षेत्रम्_Name(ध्रुव std::string& क्षेत्रम्_Name) {
    m_json_Ptr_नामानि.निवेशय(क्षेत्रम्_Name);
}

शून्य V3Global::ptr_नामानि_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    प्रति (ध्रुव स्वचालित& itr : m_json_Ptr_नामानि) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ध्रुव std::string& V3Global::ptr_गन्तव्यम्_Id(ध्रुव शून्य* p) {
    ध्रुव स्वचालित pair = m_ptr_गन्तव्यम्_Id.emplace(p, "");
    यदि (pair.second) {
        std::ostringstream os;
        यदि (p) {
            os << "(";
            अचिह्नित id = m_ptr_गन्तव्यम्_Id.आकार();
            कुरु { os << static_cast<वर्ण>('A' + id % 26); } यावत् (id /= 26);
            os << ")";
        } अन्यथा {
            os << "0";
        }
        pair.first->second = os.str();
    }
    प्रत्यागम pair.first->second;
}

std::सदिशम्<std::string> V3Global::verilatedCppFiles() {
    std::सदिशम्<std::string> परिणाम;
    परिणाम.emplace_पृष्ठभागः("verilated.cpp");
    यदि (v3Global.dpi()) परिणाम.emplace_पृष्ठभागः("verilated_dpi.cpp");
    यदि (v3Global.opt.vpi()) परिणाम.emplace_पृष्ठभागः("verilated_vpi.cpp");
    यदि (v3Global.opt.savable()) परिणाम.emplace_पृष्ठभागः("verilated_save.cpp");
    यदि (v3Global.opt.coverage()) परिणाम.emplace_पृष्ठभागः("verilated_cov.cpp");
    यदि (v3Global.opt.coverage() || v3Global.useCovergroup())
        परिणाम.emplace_पृष्ठभागः("verilated_covergroup.cpp");
    प्रति (ध्रुव string& आधारः : v3Global.opt.traceSourceBases())
        परिणाम.emplace_पृष्ठभागः(आधारः + "_c.cpp");
    यदि (v3Global.उपयुङ्क्ते_Prob_Dist()) परिणाम.emplace_पृष्ठभागः("verilated_probdist.cpp");
    यदि (v3Global.उपयुङ्क्ते_Timing()) परिणाम.emplace_पृष्ठभागः("verilated_timing.cpp");
    यदि (v3Global.useRandomizeMethods()) परिणाम.emplace_पृष्ठभागः("verilated_random.cpp");
    परिणाम.emplace_पृष्ठभागः("verilated_threads.cpp");
    यदि (v3Global.opt.उपयुङ्क्ते_Profiler()) परिणाम.emplace_पृष्ठभागः("verilated_profiler.cpp");
    प्रत्यागम परिणाम;
}
