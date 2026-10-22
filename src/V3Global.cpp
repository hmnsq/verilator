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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Global.h"

#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3HierBlock.h"
#అన్తర్భావ "V3LibMap.h"
#అన్తర్భావ "V3LinkCells.h"
#అన్తర్భావ "V3Parse.h"
#అన్తర్భావ "V3ParseImp.h"
#అన్తర్భావ "V3PreShell.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#నిర్వచితమైతే VL_ASAN
బాహ్య "C" స్థిరరాశి అక్షరం* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    తిరిగి ""
#నిర్వచితం_కాకపోతే VL_LEAK_CHECKS
           ":detect_leaks=0"
#అయితే_ముగింపు
        ;
}
#అయితే_ముగింపు

//######################################################################
// V3Global

శూన్య V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = కొత్త AstNetlist;
    m_constPoolp = కొత్త V3ConstPool;
    m_libMapp = కొత్త V3LibMap;
}

శూన్య V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(తొలగించు m_hierGraphp, m_hierGraphp = శూన్యసూచిక);  // delete nullptr is safe
    VL_DO_CLEAR(తొలగించు m_threadPoolp, m_threadPoolp = శూన్యసూచిక);  // delete nullptr is safe
    VL_DO_CLEAR(తొలగించు m_libMapp, m_libMapp = శూన్యసూచిక);  // delete nullptr is safe
    VL_DO_CLEAR(తొలగించు m_constPoolp, m_constPoolp = శూన్యసూచిక);  // delete nullptr is safe
#నిర్వచితమైతే VL_LEAK_CHECKS
    ఒకవేళ (m_rootp) VL_DO_CLEAR(m_rootp->delete_వృక్షం(), m_rootp = శూన్యసూచిక);
#అయితే_ముగింపు
    File_పంక్తి::delete_All_మిగిలిన();
}

శూన్య V3Global::vlExit(పూర్ణాంకం స్థితి_2) {
    shutdown();
    std::నిష్క్రమించు(స్థితి_2);
}

శూన్య V3Global::check_వృక్షం() స్థిరరాశి { rootp()->check_వృక్షం(); }

శూన్య V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    స్థిరరాశి VNUser4_లోపల_Use inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        ఒకవేళ (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                కొత్త File_పంక్తి{V3Options::get_Std_Waiver_మార్గం()}, V3Options::get_Std_Waiver_మార్గం(), అబద్ధం,
                అబద్ధం, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        కొరకు (స్థిరరాశి VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()}, filelib.filename(),
                             అబద్ధం, అబద్ధం, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        ఒకవేళ (v3Global.opt.stdPackage()) {
            parser.parseFile(
                కొత్త File_పంక్తి{V3Options::get_Std_Package_మార్గం()}, V3Options::get_Std_Package_మార్గం(),
                అబద్ధం, అబద్ధం, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        కొరకు (స్థిరరాశి string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()}, filename, అబద్ధం, నిజం,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        కొరకు (స్థిరరాశి స్వయంచాలక& filelib : v3Global.opt.vFiles()) {
            స్థిరరాశి string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->సరిపోలిక_Mapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()}, filelib.filename(),
                             అబద్ధం, అబద్ధం, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        కొరకు (స్థిరరాశి స్వయంచాలక& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()}, filelib.filename(),
                             నిజం, అబద్ధం, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        కొరకు (స్థిరరాశి స్వయంచాలక& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()}, filelib.filename(),
                             అబద్ధం, అబద్ధం, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    ఒకవేళ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_వృక్షం("cells", అబద్ధం, dump_వృక్షం_Either_Level() >= 9);
}

శూన్య V3Global::removeStd() {
    // Delete the std package if unused
    ఒకవేళ (!ఉపయోగిస్తుంది_Std_Package()) {
        UINFO(3, "Removing unused std:: package");
        ఒకవేళ (Ast_కణుపు_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(శూన్యసూచిక);
            v3Global.rootp()->stdPackageProcessp(శూన్యసూచిక);
            VL_DO_DANGLING(stdp->unlink_Fr_వెనుక()->delete_వృక్షం(), stdp);
        }
    }
}

string V3Global::debugFilename(స్థిరరాశి string& nameComment, పూర్ణాంకం new_సంఖ్య) {
    ++m_debug_File_సంఖ్య;
    ఒకవేళ (new_సంఖ్య) m_debug_File_సంఖ్య = new_సంఖ్య;
    తిరిగి opt.hier_శిఖరం_దత్తాంశం_Dir() + "/" + opt.ఉపసర్గ() + "_" + digitsFilename(m_debug_File_సంఖ్య)
           + "_" + nameComment;
}
string V3Global::digitsFilename(పూర్ణాంకం సంఖ్య) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << సంఖ్య;
    తిరిగి ss.str();
}

శూన్య V3Global::dump_Check_Global_వృక్షం(స్థిరరాశి string& stagename, పూర్ణాంకం new_సంఖ్య, తార్కిక doDump,
                                   తార్కిక doCheck) {
    స్థిరరాశి string వృక్షం_Filename = v3Global.debugFilename(stagename + ".tree", new_సంఖ్య);
    ఒకవేళ (dump_వృక్షం_Level()) v3Global.rootp()->dump_వృక్షం_File(వృక్షం_Filename, doDump);
    ఒకవేళ (dump_వృక్షం_Json_Level()) {
        v3Global.rootp()->dump_వృక్షం_Json_File(వృక్షం_Filename + ".json", doDump);
    }
    ఒకవేళ (v3Global.opt.dump_వృక్షం_Dot()) {
        v3Global.rootp()->dump_వృక్షం_Dot_File(వృక్షం_Filename + ".dot", doDump);
    }
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    ఒకవేళ (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(వృక్షం_Filename + ".v");
    ఒకవేళ (doCheck && (v3Global.opt.debugCheck() || dump_వృక్షం_Either_Level())) {
        // Error check
        v3Global.rootp()->check_వృక్షం();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

శూన్య V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_ptr_గమ్యం_Id) {
        os << sep << '"' << itr.రెండవ << "\": \"" << cvt_గమ్యం_Hex(itr.మొదటి) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

శూన్య V3Global::save_Json_Ptr_క్షేత్రం_Name(స్థిరరాశి std::string& క్షేత్రం_Name) {
    m_json_Ptr_పేర్లు.చొప్పించు(క్షేత్రం_Name);
}

శూన్య V3Global::ptr_పేర్లు_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_json_Ptr_పేర్లు) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

స్థిరరాశి std::string& V3Global::ptr_గమ్యం_Id(స్థిరరాశి శూన్య* p) {
    స్థిరరాశి స్వయంచాలక pair = m_ptr_గమ్యం_Id.emplace(p, "");
    ఒకవేళ (pair.రెండవ) {
        std::ostringstream os;
        ఒకవేళ (p) {
            os << "(";
            గుర్తులేని id = m_ptr_గమ్యం_Id.కొలత();
            చేయి { os << static_cast<అక్షరం>('A' + id % 26); } అయితే (id /= 26);
            os << ")";
        } లేకపోతే {
            os << "0";
        }
        pair.మొదటి->రెండవ = os.str();
    }
    తిరిగి pair.మొదటి->రెండవ;
}

std::సదిశ<std::string> V3Global::verilatedCppFiles() {
    std::సదిశ<std::string> ఫలితం;
    ఫలితం.emplace_వెనుక("verilated.cpp");
    ఒకవేళ (v3Global.dpi()) ఫలితం.emplace_వెనుక("verilated_dpi.cpp");
    ఒకవేళ (v3Global.opt.vpi()) ఫలితం.emplace_వెనుక("verilated_vpi.cpp");
    ఒకవేళ (v3Global.opt.savable()) ఫలితం.emplace_వెనుక("verilated_save.cpp");
    ఒకవేళ (v3Global.opt.coverage()) ఫలితం.emplace_వెనుక("verilated_cov.cpp");
    ఒకవేళ (v3Global.opt.coverage() || v3Global.useCovergroup())
        ఫలితం.emplace_వెనుక("verilated_covergroup.cpp");
    కొరకు (స్థిరరాశి string& పునాది : v3Global.opt.trace_ఆధారం_Bases())
        ఫలితం.emplace_వెనుక(పునాది + "_c.cpp");
    ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Prob_Dist()) ఫలితం.emplace_వెనుక("verilated_probdist.cpp");
    ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Timing()) ఫలితం.emplace_వెనుక("verilated_timing.cpp");
    ఒకవేళ (v3Global.useRandomizeMethods()) ఫలితం.emplace_వెనుక("verilated_random.cpp");
    ఫలితం.emplace_వెనుక("verilated_threads.cpp");
    ఒకవేళ (v3Global.opt.ఉపయోగిస్తుంది_Profiler()) ఫలితం.emplace_వెనుక("verilated_profiler.cpp");
    తిరిగి ఫలితం;
}
