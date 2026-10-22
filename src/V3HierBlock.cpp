// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Hierarchical Verilation for large designs
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// Hierarchical Verilation is useful for large designs.
// It reduces
//   - time and memory for Verilation
//   - compilation time especially when a hierarchical block is used many times
//
// Hierarchical Verilation internally uses --lib-create for each
// hierarchical block.  Upper modules read the wrapper from --lib-create
// instead of the Verilog design.
//
// Hierarchical Verilation runs as the following step
// 1) Find modules marked by /*verilator hier_block*/ metacomment
// 2) Generate ${prefix}_hier.mk to create protected-lib for hierarchical blocks and
//    final Verilation to process the top module, that refers wrappers
// 3) Call child Verilator process via ${prefix}_hier.mk
//
// There are 3 kinds of Verilator run.
// a) To create ${prefix}_hier.mk (--hierarchical)
// b) To --lib-create on each hierarchical block (--hierarchical-child)
// c) To load wrappers and Verilate the top module (... what primary flags?)
//
// Then user can build Verilated module as usual.
//
// Here is more detailed internal process.
// 1) Parser adds VPragmaType::HIER_BLOCK of AstPragma to modules
//    that are marked with /*verilator hier_block*/ metacomment in Verilator run a).
// 2) If module type parameters are present, V3Control marks hier param modules
// (marked with hier_params verilator config pragma) as modp->hierParams(true).
// This is done in run b), de-parameterized modules are mapped with their params one-to-one.
// 3) AstModule with HIER_BLOCK pragma is marked modp->hierBlock(true)
//    in V3LinkResolve.cpp during run a).
// 4) In V3LinkCells.cpp, the following things are done during run b) and c).
//    4-1) Delete the upper modules of the hierarchical block because the top module in run b) is
//         hierarchical block, not the top module of run c).
//    4-2) If the top module of the run b) or c) instantiates other hierarchical blocks that is
//         parameterized,
//         module and task names are renamed to the original name to be compatible with the
//         hier module to be called.
//
//         Parameterized modules have unique name by V3Param.cpp. The unique name contains '__' and
//         Verilator encodes '__' when loading such symbols.
// 5) In V3LinkDot.cpp,
//    5-1) Dotted access across hierarchical block boundary is checked. Currently hierarchical
//    block references are not supported.
//    5-2) If present, parameters in hier params module replace parameter values of
//    de-parameterized module in run b).
// 6) In V3Dead.cpp, some parameters of parameterized modules are protected not to be deleted even
//    if the parameter is not referred. This protection is necessary to match step 6) below.
// 7) In V3Param.cpp, use --lib-create wrapper of the parameterized module made in b) and c).
//    If a hierarchical block is a parameterized module and instantiated in multiple locations,
//    all parameters must exactly match.
// 8) In V3HierBlock.cpp, relationships among hierarchical blocks are checked in run a).
//    (which block uses other blocks..)
// 9) In V3EmitMk.cpp, ${prefix}_hier.mk is created in run a).
//
// There are three hidden command options:
//   --hierarchical-child is added to Verilator run b).
//   --hierarchical-block module_name,mangled_name,name0,value0,name1,value1,...
//       module_name  :The original modulename
//       mangled_name :Mangled name of parameterized modules (named in V3Param.cpp).
//                     Same as module_name for non-parameterized hierarchical block.
//       name         :The name of the parameter
//       value        :Overridden value of the parameter
//
//       Used for b) and c).
//       These options are repeated for all instantiated hierarchical blocks.
//   --hierarchical-params-file filename
//      filename    :Name of a hierarchical parameters file
//
//      Added in a), used for b).
//      Each de-parameterized module version has exactly one hier params file specified.

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3HierBlock.h"

#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <cctype>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <sstream>
#અન્તર્ભાવ <utility>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

સ્થિર string V3HierCommandArgsFilename(અચલ string& prefix, તાર્કિક forMkJson) {
    પાછા v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

સ્થિર string V3HierParametersFileName(અચલ string& prefix) {
    પાછા v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

સ્થિર શૂન્ય V3HierWriteCommonInputs(અચલ V3HierBlock* hblockp, std::ostream* of, તાર્કિક forMkJson) {
    અચલ string ટોચ_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    જો (!forMkJson) {
        માટે (અચલ string& filename : V3HierGraph::sourceFiles(ટોચ_Module_File))
            *of << filename << "\n";
    }
    માટે (અચલ auto& i : v3Global.opt.libraryFiles()) {
        જો (V3Os::filenameRealPath(i.filename()) != ટોચ_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

તાર્કિક V3HierBlock::stringParamPassable(અચલ string& કિંમત) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    જો (કિંમત.find_first_of("\n\"") != string::npos) પાછા ખોટું;
    જો (કિંમત.find("/*") != string::npos) પાછા ખોટું;
    માટે (size_t pos = કિંમત.find("//"); pos != string::npos; pos = કિંમત.find("//", pos + 1)) {
        જો (pos > 0 && std::isspace(static_cast<unsigned અક્ષર>(કિંમત[pos - 1]))) પાછા ખોટું;
    }
    પાછા સાચું;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(અચલ std::vector<AstVar*>& gparams,
                                                     તાર્કિક forGOption) {
    StrGParams strParams;
    માટે (અચલ AstVar* અચલ gparam : gparams) {
        જો (અચલ AstConst* અચલ constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            જો (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<અક્ષર> hexFpStr(32, '\0');
                અચલ પૂર્ણાંક len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->અંક().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } નહીંતર જો (constp->isString()) {
                s = constp->અંક().toString();
                જો (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } નહીંતર {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_સંખ્યા અંક{constp->અંક()};
                અંક.isSigned(constp->isSigned());
                s = અંક.ascii(સાચું, સાચું);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    પાછા strParams;
}

VStringList V3HierBlock::commandArgs(તાર્કિક forMkJson) અચલ {
    VStringList opts;
    અચલ string prefix = hierPrefix();
    જો (!forMkJson) {
        opts.ધકેલો_back(" --prefix " + prefix);
        opts.ધકેલો_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ધકેલો_back(" --top-module-encoded " + modp()->name());
    }
    opts.ધકેલો_back(" --lib-create " + modp()->name());  // possibly mangled name
    જો (v3Global.opt.protect_ચાવી_Provided())
        opts.ધકેલો_back(" --protect-key " + v3Global.opt.protect_ચાવી_Defaulted());
    opts.ધકેલો_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    અચલ StrGParams gparamsStr = stringifyParams(m_params, સાચું);
    માટે (અચલ StrGParam& param : gparamsStr) {
        opts.ધકેલો_back("-G" + param.first + "=" + param.second + "");
    }
    જો (!m_typeParams.empty()) {
        opts.ધકેલો_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    અચલ પૂર્ણાંક blockThreads = V3Control::getHierWorkers(m_modp->origName());
    જો (blockThreads > 1) {
        જો (!inEmpty()) {
            V3Control::get_Hier_Workers_File_લીટી(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } નહીંતર {
            જો (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.ધકેલો_back(" --threads " + std::to_string(blockThreads));
        }
    }

    પાછા opts;
}

VStringList V3HierBlock::hierBlockArgs() અચલ {
    VStringList opts;
    અચલ StrGParams gparamsStr = stringifyParams(m_params, ખોટું);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    માટે (અચલ StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    પાછા opts;
}

string V3HierBlock::hierPrefix() અચલ { પાછા "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(તાર્કિક withDir, અચલ અક્ષર* prefix, અચલ અક્ષર* suffix) અચલ {
    string s;
    જો (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    પાછા s;
}

string V3HierBlock::hierWrapperFilename(તાર્કિક withDir) અચલ {
    પાછા hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(તાર્કિક withDir) અચલ {
    પાછા hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(તાર્કિક withDir) અચલ {
    પાછા hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(તાર્કિક withDir) અચલ {
    પાછા hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() અચલ {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    માટે (અચલ auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        જો (filename == V3Os::filenameRealPath(v.filename())) પાછા "";
    }
    પાછા filename;
}

શૂન્ય V3HierBlock::writeCommandArgsFile(તાર્કિક forMkJson) અચલ {
    અચલ std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    જો (!forMkJson) {
        માટે (અચલ V3GraphEdge& edge : outEdges()) {
            અચલ V3HierBlock* અચલ dependencyp = edge.ટોચ()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(સાચું) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(આ, of.get(), forMkJson);
    અચલ VStringList& commandOpts = commandArgs(ખોટું);
    માટે (અચલ string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    માટે (અચલ V3GraphEdge& edge : outEdges()) {
        અચલ V3HierBlock* અચલ dependencyp = edge.ટોચ()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(ખોટું) << "\n";
}

string V3HierBlock::commandArgsFilename(તાર્કિક forMkJson) અચલ {
    પાછા V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() અચલ {
    પાછા V3HierParametersFileName(hierPrefix());
}

શૂન્ય V3HierBlock::writeParametersFile() અચલ {
    જો (m_typeParams.empty()) પાછા;

    VHashSha512 hash{"type params"};
    અચલ string moduleName = "Vhsh" + hash.digestSymbol24();
    અચલ std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    માટે (AstParamTypeDType* અચલ gparam : m_typeParams) {
        AstTypedef* tdefp
            = નવું AstTypedef{નવું File_લીટી{File_લીટી::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_વૃક્ષ_Pure(સાચું)};
        V3EmitV::verilog_For_વૃક્ષ(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_વૃક્ષ(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
વર્ગ HierBlockUsageCollectVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    અચલ VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* અચલ m_graphp = નવું V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<અચલ AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    શૂન્ય visit(Ast_ગાંઠ_Module*) override {}  // Ignore all non-AstModule
    શૂન્ય visit(AstModule* nodep) override {
        // Visit each module once
        જો (nodep->user1SetOnce()) પાછા;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        જો (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            પાછા;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* અચલ blockp = નવું V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        માટે (V3HierBlock* અચલ childp : m_childrenp) નવું V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    શૂન્ય visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* અચલ modp = VN_CAST(nodep->modp(), Module);
        જો (!modp) પાછા;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        જો (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    શૂન્ય visit(AstVar* nodep) override {
        જો (!m_modp) પાછા;
        જો (!m_modp->hierBlock()) પાછા;
        // Can't handle interface port on hier block
        જો (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        જો (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ધકેલો_back(nodep);
        }
    }
    શૂન્ય visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        જો (!m_modp->hierBlock()) પાછા;
        // Record type parameter of this hier block
        m_typeParams.ધકેલો_back(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Stmt*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}  // Accelerate
    શૂન્ય visit(AstTypeTable*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        જો (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        જો (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

જાહેર:
    સ્થિર V3HierGraph* apply(AstNetlist* netlistp) {
        પાછા HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(અચલ string& ટોચ_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    માટે (અચલ VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    જો (!ટોચ_Module_File.empty()) sources.emplace_back(ટોચ_Module_File);
    પાછા sources;
}

શૂન્ય V3HierGraph::writeCommandArgsFiles(તાર્કિક forMkJson) અચલ {

    માટે (અચલ V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    અચલ std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(ટોચ_Command_Args_Filename(forMkJson))};
    જો (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        માટે (અચલ V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(સાચું) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    જો (!forMkJson) {
        અચલ VStringSet& cppFiles = v3Global.opt.cppFiles();
        માટે (અચલ string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->ટોચ_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    માટે (અચલ V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    જો (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    જો (v3Global.opt.protect_ચાવી_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_ચાવી_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(સાચું) << "\n";
}

string V3HierGraph::ટોચ_Command_Args_Filename(તાર્કિક forMkJson) {
    પાછા V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

શૂન્ય V3HierGraph::writeParametersFiles() અચલ {
    માટે (અચલ V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

શૂન્ય V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_ગાંઠ_Module* અચલ modp = netlistp->ટોચ_Modulep();
    જો (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(ખોટું);
    }

    V3HierGraph* અચલ graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    જો (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        પાછા;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
