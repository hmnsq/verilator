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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Global.h"

#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3HierBlock.h"
#அந்தர்பாவ "V3LibMap.h"
#அந்தர்பாவ "V3LinkCells.h"
#அந்தர்பாவ "V3Parse.h"
#அந்தர்பாவ "V3ParseImp.h"
#அந்தர்பாவ "V3PreShell.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
வெளி "C" மாறிலி எழுத்து* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    திரும்பு ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

வெறுமை V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = புதிய AstNetlist;
    m_constPoolp = புதிய V3ConstPool;
    m_libMapp = புதிய V3LibMap;
}

வெறுமை V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    எனில் (m_rootp) VL_DO_CLEAR(m_rootp->delete_மரம்(), m_rootp = nullptr);
#endif
    File_வரி::deleteAllRemaining();
}

வெறுமை V3Global::vlExit(முழு status) {
    shutdown();
    std::exit(status);
}

வெறுமை V3Global::check_மரம்() மாறிலி { rootp()->check_மரம்(); }

வெறுமை V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    மாறிலி VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        எனில் (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                புதிய File_வரி{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), பொய்,
                பொய், "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        க்கு (மாறிலி VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(புதிய File_வரி{File_வரி::command_வரி_Filename()}, filelib.filename(),
                             பொய், பொய், filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        எனில் (v3Global.opt.stdPackage()) {
            parser.parseFile(
                புதிய File_வரி{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                பொய், பொய், "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        க்கு (மாறிலி string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(புதிய File_வரி{File_வரி::command_வரி_Filename()}, filename, பொய், உண்மை,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        க்கு (மாறிலி auto& filelib : v3Global.opt.vFiles()) {
            மாறிலி string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(புதிய File_வரி{File_வரி::command_வரி_Filename()}, filelib.filename(),
                             பொய், பொய், libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        க்கு (மாறிலி auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(புதிய File_வரி{File_வரி::command_வரி_Filename()}, filelib.filename(),
                             உண்மை, பொய், filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        க்கு (மாறிலி auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(புதிய File_வரி{File_வரி::command_வரி_Filename()}, filelib.filename(),
                             பொய், பொய், filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    எனில் (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_மரம்("cells", பொய், dump_மரம்_Either_Level() >= 9);
}

வெறுமை V3Global::removeStd() {
    // Delete the std package if unused
    எனில் (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        எனில் (Ast_முனை_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_மரம்(), stdp);
        }
    }
}

string V3Global::debugFilename(மாறிலி string& nameComment, முழு new_எண்) {
    ++m_debug_File_எண்;
    எனில் (new_எண்) m_debug_File_எண் = new_எண்;
    திரும்பு opt.hier_உச்சி_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_எண்)
           + "_" + nameComment;
}
string V3Global::digitsFilename(முழு எண்) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << எண்;
    திரும்பு ss.str();
}

வெறுமை V3Global::dump_Check_Global_மரம்(மாறிலி string& stagename, முழு new_எண், தருக்க doDump,
                                   தருக்க doCheck) {
    மாறிலி string மரம்_Filename = v3Global.debugFilename(stagename + ".tree", new_எண்);
    எனில் (dump_மரம்_Level()) v3Global.rootp()->dump_மரம்_File(மரம்_Filename, doDump);
    எனில் (dump_மரம்_Json_Level()) {
        v3Global.rootp()->dump_மரம்_Json_File(மரம்_Filename + ".json", doDump);
    }
    எனில் (v3Global.opt.dump_மரம்_Dot()) {
        v3Global.rootp()->dump_மரம்_Dot_File(மரம்_Filename + ".dot", doDump);
    }
    எனில் (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    எனில் (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(மரம்_Filename + ".v");
    எனில் (doCheck && (v3Global.opt.debugCheck() || dump_மரம்_Either_Level())) {
        // Error check
        v3Global.rootp()->check_மரம்();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

வெறுமை V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    க்கு (மாறிலி auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

வெறுமை V3Global::saveJsonPtrFieldName(மாறிலி std::string& fieldName) {
    m_jsonPtrNames.செருகு(fieldName);
}

வெறுமை V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    க்கு (மாறிலி auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

மாறிலி std::string& V3Global::ptrToId(மாறிலி வெறுமை* p) {
    மாறிலி auto pair = m_ptrToId.emplace(p, "");
    எனில் (pair.second) {
        std::ostringstream os;
        எனில் (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            செய் { os << static_cast<எழுத்து>('A' + id % 26); } போது (id /= 26);
            os << ")";
        } இல்லையெனில் {
            os << "0";
        }
        pair.first->second = os.str();
    }
    திரும்பு pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    எனில் (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    எனில் (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    எனில் (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    எனில் (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    எனில் (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    க்கு (மாறிலி string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    எனில் (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    எனில் (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    எனில் (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    எனில் (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    திரும்பு result;
}
