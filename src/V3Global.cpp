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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Global.h"

#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3HierBlock.h"
#અન્તર્ભાવ "V3LibMap.h"
#અન્તર્ભાવ "V3LinkCells.h"
#અન્તર્ભાવ "V3Parse.h"
#અન્તર્ભાવ "V3ParseImp.h"
#અન્તર્ભાવ "V3PreShell.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
બાહ્ય "C" અચલ અક્ષર* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    પાછા ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

શૂન્ય V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = નવું AstNetlist;
    m_constPoolp = નવું V3ConstPool;
    m_libMapp = નવું V3LibMap;
}

શૂન્ય V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_hierGraphp, m_hierGraphp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    જો (m_rootp) VL_DO_CLEAR(m_rootp->delete_વૃક્ષ(), m_rootp = nullptr);
#endif
    File_લીટી::deleteAllRemaining();
}

શૂન્ય V3Global::vlExit(પૂર્ણાંક status) {
    shutdown();
    std::exit(status);
}

શૂન્ય V3Global::check_વૃક્ષ() અચલ { rootp()->check_વૃક્ષ(); }

શૂન્ય V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    અચલ VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        જો (v3Global.opt.stdWaiver()) {
            parser.parseFile(
                નવું File_લીટી{V3Options::getStdWaiverPath()}, V3Options::getStdWaiverPath(), ખોટું,
                ખોટું, "work",
                "Cannot find verilated_std_waiver.vlt containing built-in lint waivers: ");
        }
        // Read .vlt files
        માટે (અચલ VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(નવું File_લીટી{File_લીટી::command_લીટી_Filename()}, filelib.filename(),
                             ખોટું, ખોટું, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        જો (v3Global.opt.stdPackage()) {
            parser.parseFile(
                નવું File_લીટી{V3Options::getStdPackagePath()}, V3Options::getStdPackagePath(),
                ખોટું, ખોટું, "work",
                "Cannot find verilated_std.sv containing built-in std:: definitions: ");
        }

        // Parse libmap files
        માટે (અચલ string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(નવું File_લીટી{File_લીટી::command_લીટી_Filename()}, filename, ખોટું, સાચું,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        માટે (અચલ auto& filelib : v3Global.opt.vFiles()) {
            અચલ string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(નવું File_લીટી{File_લીટી::command_લીટી_Filename()}, filelib.filename(),
                             ખોટું, ખોટું, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        માટે (અચલ auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(નવું File_લીટી{File_લીટી::command_લીટી_Filename()}, filelib.filename(),
                             સાચું, ખોટું, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        માટે (અચલ auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(નવું File_લીટી{File_લીટી::command_લીટી_Filename()}, filelib.filename(),
                             ખોટું, ખોટું, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfErrors();

    જો (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dump_Check_Global_વૃક્ષ("cells", ખોટું, dump_વૃક્ષ_Either_Level() >= 9);
}

શૂન્ય V3Global::removeStd() {
    // Delete the std package if unused
    જો (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        જો (Ast_ગાંઠ_Module* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->delete_વૃક્ષ(), stdp);
        }
    }
}

string V3Global::debugFilename(અચલ string& nameComment, પૂર્ણાંક new_સંખ્યા) {
    ++m_debug_File_સંખ્યા;
    જો (new_સંખ્યા) m_debug_File_સંખ્યા = new_સંખ્યા;
    પાછા opt.hier_ટોચ_Data_Dir() + "/" + opt.prefix() + "_" + digitsFilename(m_debug_File_સંખ્યા)
           + "_" + nameComment;
}
string V3Global::digitsFilename(પૂર્ણાંક સંખ્યા) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << સંખ્યા;
    પાછા ss.str();
}

શૂન્ય V3Global::dump_Check_Global_વૃક્ષ(અચલ string& stagename, પૂર્ણાંક new_સંખ્યા, તાર્કિક doDump,
                                   તાર્કિક doCheck) {
    અચલ string વૃક્ષ_Filename = v3Global.debugFilename(stagename + ".tree", new_સંખ્યા);
    જો (dump_વૃક્ષ_Level()) v3Global.rootp()->dump_વૃક્ષ_File(વૃક્ષ_Filename, doDump);
    જો (dump_વૃક્ષ_Json_Level()) {
        v3Global.rootp()->dump_વૃક્ષ_Json_File(વૃક્ષ_Filename + ".json", doDump);
    }
    જો (v3Global.opt.dump_વૃક્ષ_Dot()) {
        v3Global.rootp()->dump_વૃક્ષ_Dot_File(વૃક્ષ_Filename + ".dot", doDump);
    }
    જો (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    જો (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(વૃક્ષ_Filename + ".v");
    જો (doCheck && (v3Global.opt.debugCheck() || dump_વૃક્ષ_Either_Level())) {
        // Error check
        v3Global.rootp()->check_વૃક્ષ();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

શૂન્ય V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    માટે (અચલ auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

શૂન્ય V3Global::saveJsonPtrFieldName(અચલ std::string& fieldName) {
    m_jsonPtrNames.ઉમેરો(fieldName);
}

શૂન્ય V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    માટે (અચલ auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

અચલ std::string& V3Global::ptrToId(અચલ શૂન્ય* p) {
    અચલ auto pair = m_ptrToId.emplace(p, "");
    જો (pair.second) {
        std::ostringstream os;
        જો (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            કરો { os << static_cast<અક્ષર>('A' + id % 26); } જ્યારે (id /= 26);
            os << ")";
        } નહીંતર {
            os << "0";
        }
        pair.first->second = os.str();
    }
    પાછા pair.first->second;
}

std::vector<std::string> V3Global::verilatedCppFiles() {
    std::vector<std::string> result;
    result.emplace_back("verilated.cpp");
    જો (v3Global.dpi()) result.emplace_back("verilated_dpi.cpp");
    જો (v3Global.opt.vpi()) result.emplace_back("verilated_vpi.cpp");
    જો (v3Global.opt.savable()) result.emplace_back("verilated_save.cpp");
    જો (v3Global.opt.coverage()) result.emplace_back("verilated_cov.cpp");
    જો (v3Global.opt.coverage() || v3Global.useCovergroup())
        result.emplace_back("verilated_covergroup.cpp");
    માટે (અચલ string& base : v3Global.opt.traceSourceBases())
        result.emplace_back(base + "_c.cpp");
    જો (v3Global.usesProbDist()) result.emplace_back("verilated_probdist.cpp");
    જો (v3Global.usesTiming()) result.emplace_back("verilated_timing.cpp");
    જો (v3Global.useRandomizeMethods()) result.emplace_back("verilated_random.cpp");
    result.emplace_back("verilated_threads.cpp");
    જો (v3Global.opt.usesProfiler()) result.emplace_back("verilated_profiler.cpp");
    પાછા result;
}
