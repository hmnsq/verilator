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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3HierBlock.h"
#ଅନ୍ତର୍ଭାଵ "V3LibMap.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkCells.h"
#ଅନ୍ତର୍ଭାଵ "V3Parse.h"
#ଅନ୍ତର୍ଭାଵ "V3ParseImp.h"
#ଅନ୍ତର୍ଭାଵ "V3PreShell.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
ବାହ୍ୟ "C" ଅଚଳ ଅକ୍ଷର* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    ଫେରନ୍ତୁ ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

ଶୂନ୍ୟ V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = ନୂତନ AstNetlist;
    m_constPoolp = ନୂତନ V3ConstPool;
    m_libMapp = ନୂତନ V3LibMap;
}

ଶୂନ୍ୟ V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    ଯଦି (m_rootp) VL_DO_CLEAR(m_rootp->delete_ବୃକ୍ଷ(), m_rootp = nullptr);
#endif
    File_ଧାଡ଼ି::deleteAllRemaining();
}

ଶୂନ୍ୟ V3Global::vlExit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା status) {
    shutdown();
    std::exit(status);
}

ଶୂନ୍ୟ V3Global::check_ବୃକ୍ଷ() ଅଚଳ { rootp()->check_ବୃକ୍ଷ(); }

ଶୂନ୍ୟ V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ଅଚଳ VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        ଯଦି (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                ନୂତନ File_ଧାଡ଼ି{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), ମିଥ୍ୟା,
                ମିଥ୍ୟା, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        ପାଇଁ (ଅଚଳ VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()}, filelib.filename(),
                             ମିଥ୍ୟା, ମିଥ୍ୟା, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        ଯଦି (v3Global.opt.stdPackage()) {
            parser.parseFile(
                ନୂତନ File_ଧାଡ଼ି{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                ମିଥ୍ୟା, ମିଥ୍ୟା, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        ପାଇଁ (ଅଚଳ string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()}, filename, ମିଥ୍ୟା, ସତ୍ୟ,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        ପାଇଁ (ଅଚଳ auto& filelib : v3Global.opt.vFiles()) {
            ଅଚଳ string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()}, filelib.filename(),
                             ମିଥ୍ୟା, ମିଥ୍ୟା, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        ପାଇଁ (ଅଚଳ auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()}, filelib.filename(),
                             ସତ୍ୟ, ମିଥ୍ୟା, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        ପାଇଁ (ଅଚଳ auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()}, filelib.filename(),
                             ମିଥ୍ୟା, ମିଥ୍ୟା, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    ଯଦି (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_ବୃକ୍ଷ("cells", ମିଥ୍ୟା, dump_ବୃକ୍ଷ_Either_Level() >= 9);
}

ଶୂନ୍ୟ V3Global::removeStd() {
    // Delete the std package if unused
    ଯଦି (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        ଯଦି (Ast_ଗଣ୍ଠି_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_ବୃକ୍ଷ(), stdp);
        }
    }
}

string V3Global::debugFilename(ଅଚଳ string& nameComment, ପୂର୍ଣ୍ଣସଂଖ୍ୟା new_ସଂଖ୍ୟା) {
    ++m_debug_File_ସଂଖ୍ୟା;
    ଯଦି (new_ସଂଖ୍ୟା) m_debug_File_ସଂଖ୍ୟା = new_ସଂଖ୍ୟା;
    ଫେରନ୍ତୁ opt.hier_ଶୀର୍ଷ_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_ସଂଖ୍ୟା)
           + "_" + nameComment;
}
string V3Global::digitsFilename(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ସଂଖ୍ୟା) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << ସଂଖ୍ୟା;
    ଫେରନ୍ତୁ ss.str();
}

ଶୂନ୍ୟ V3Global::dump_Check_Global_ବୃକ୍ଷ(ଅଚଳ string& stagename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା new_ସଂଖ୍ୟା, ତାର୍କିକ doDump,
                                   ତାର୍କିକ doCheck) {
    ଅଚଳ string ବୃକ୍ଷ_Filename = v3Global.debugFilename(stagename + ".tree", new_ସଂଖ୍ୟା);
    ଯଦି (dump_ବୃକ୍ଷ_Level()) v3Global.rootp()->dump_ବୃକ୍ଷ_File(ବୃକ୍ଷ_Filename, doDump);
    ଯଦି (dump_ବୃକ୍ଷ_Json_Level()) {
        v3Global.rootp()->dump_ବୃକ୍ଷ_Json_File(ବୃକ୍ଷ_Filename + ".json", doDump);
    }
    ଯଦି (v3Global.opt.dump_ବୃକ୍ଷ_Dot()) {
        v3Global.rootp()->dump_ବୃକ୍ଷ_Dot_File(ବୃକ୍ଷ_Filename + ".dot", doDump);
    }
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    ଯଦି (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(ବୃକ୍ଷ_Filename + ".v");
    ଯଦି (doCheck && (v3Global.opt.debugCheck() || dump_ବୃକ୍ଷ_Either_Level())) {
        // Error check
        v3Global.rootp()->check_ବୃକ୍ଷ();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

ଶୂନ୍ୟ V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    ପାଇଁ (ଅଚଳ auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

ଶୂନ୍ୟ V3Global::saveJsonPtrFieldName(ଅଚଳ std::string& fieldName) {
    m_jsonPtrNames.ଭର୍ତ୍ତି(fieldName);
}

ଶୂନ୍ୟ V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    ପାଇଁ (ଅଚଳ auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

ଅଚଳ std::string& V3Global::ptrToId(ଅଚଳ ଶୂନ୍ୟ* p) {
    ଅଚଳ auto pair = m_ptrToId.emplace(p, "");
    ଯଦି (pair.second) {
        std::ostringstream os;
        ଯଦି (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            କର { os << static_cast<ଅକ୍ଷର>('A' + id % 26); } ଯେତେବେଳେ (id /= 26);
            os << ")";
        } ଅନ୍ୟଥା {
            os << "0";
        }
        pair.first->second = os.str();
    }
    ଫେରନ୍ତୁ pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    ଯଦି (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    ଯଦି (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    ଯଦି (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    ଯଦି (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    ଯଦି (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    ପାଇଁ (ଅଚଳ string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    ଯଦି (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    ଯଦି (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    ଯଦି (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    ଯଦି (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    ଫେରନ୍ତୁ result;
}
