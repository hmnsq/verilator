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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3HierBlock.h"

#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3String.h"

#অন্তর্ভাব <cctype>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <sstream>
#অন্তর্ভাব <utility>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

স্থির string V3HierCommandArgsFilename(ধ্রুব string& prefix, তার্কিক forMkJson) {
    প্রত্যাগম v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

স্থির string V3HierParametersFileName(ধ্রুব string& prefix) {
    প্রত্যাগম v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

স্থির শূন্য V3HierWriteCommonInputs(ধ্রুব V3HierBlock* hblockp, std::ostream* of, তার্কিক forMkJson) {
    ধ্রুব string শীর্ষ_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    যদি (!forMkJson) {
        প্রতি (ধ্রুব string& filename : V3HierGraph::sourceFiles(শীর্ষ_Module_File))
            *of << filename << "\n";
    }
    প্রতি (ধ্রুব auto& i : v3Global.opt.libraryFiles()) {
        যদি (V3Os::filenameRealPath(i.filename()) != শীর্ষ_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

তার্কিক V3HierBlock::stringParamPassable(ধ্রুব string& মান) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    যদি (মান.find_first_of("\n\"") != string::npos) প্রত্যাগম অসত্য;
    যদি (মান.find("/*") != string::npos) প্রত্যাগম অসত্য;
    প্রতি (size_t pos = মান.find("//"); pos != string::npos; pos = মান.find("//", pos + 1)) {
        যদি (pos > 0 && std::isspace(static_cast<unsigned বর্ণ>(মান[pos - 1]))) প্রত্যাগম অসত্য;
    }
    প্রত্যাগম সত্য;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(ধ্রুব std::vector<AstVar*>& gparams,
                                                     তার্কিক forGOption) {
    StrGParams strParams;
    প্রতি (ধ্রুব AstVar* ধ্রুব gparam : gparams) {
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            যদি (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<বর্ণ> hexFpStr(32, '\0');
                ধ্রুব পূর্ণাঙ্ক len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->অঙ্ক().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } অন্যথা যদি (constp->isString()) {
                s = constp->অঙ্ক().toString();
                যদি (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } অন্যথা {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_সংখ্যা অঙ্ক{constp->অঙ্ক()};
                অঙ্ক.isSigned(constp->isSigned());
                s = অঙ্ক.ascii(সত্য, সত্য);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    প্রত্যাগম strParams;
}

VStringList V3HierBlock::commandArgs(তার্কিক forMkJson) ধ্রুব {
    VStringList opts;
    ধ্রুব string prefix = hierPrefix();
    যদি (!forMkJson) {
        opts.ঠেলো_back(" --prefix " + prefix);
        opts.ঠেলো_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ঠেলো_back(" --top-module-encoded " + modp()->name());
    }
    opts.ঠেলো_back(" --lib-create " + modp()->name());  // possibly mangled name
    যদি (v3Global.opt.protect_চাবি_Provided())
        opts.ঠেলো_back(" --protect-key " + v3Global.opt.protect_চাবি_Defaulted());
    opts.ঠেলো_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    ধ্রুব StrGParams gparamsStr = stringifyParams(m_params, সত্য);
    প্রতি (ধ্রুব StrGParam& param : gparamsStr) {
        opts.ঠেলো_back("-G" + param.first + "=" + param.second + "");
    }
    যদি (!m_typeParams.empty()) {
        opts.ঠেলো_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    ধ্রুব পূর্ণাঙ্ক blockThreads = V3Control::getHierWorkers(m_modp->origName());
    যদি (blockThreads > 1) {
        যদি (!inEmpty()) {
            V3Control::get_Hier_Workers_File_সারি(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } অন্যথা {
            যদি (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.ঠেলো_back(" --threads " + std::to_string(blockThreads));
        }
    }

    প্রত্যাগম opts;
}

VStringList V3HierBlock::hierBlockArgs() ধ্রুব {
    VStringList opts;
    ধ্রুব StrGParams gparamsStr = stringifyParams(m_params, অসত্য);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    প্রতি (ধ্রুব StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    প্রত্যাগম opts;
}

string V3HierBlock::hierPrefix() ধ্রুব { প্রত্যাগম "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(তার্কিক withDir, ধ্রুব বর্ণ* prefix, ধ্রুব বর্ণ* suffix) ধ্রুব {
    string s;
    যদি (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    প্রত্যাগম s;
}

string V3HierBlock::hierWrapperFilename(তার্কিক withDir) ধ্রুব {
    প্রত্যাগম hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(তার্কিক withDir) ধ্রুব {
    প্রত্যাগম hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(তার্কিক withDir) ধ্রুব {
    প্রত্যাগম hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(তার্কিক withDir) ধ্রুব {
    প্রত্যাগম hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() ধ্রুব {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    প্রতি (ধ্রুব auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        যদি (filename == V3Os::filenameRealPath(v.filename())) প্রত্যাগম "";
    }
    প্রত্যাগম filename;
}

শূন্য V3HierBlock::writeCommandArgsFile(তার্কিক forMkJson) ধ্রুব {
    ধ্রুব std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    যদি (!forMkJson) {
        প্রতি (ধ্রুব V3GraphEdge& edge : outEdges()) {
            ধ্রুব V3HierBlock* ধ্রুব dependencyp = edge.শীর্ষ()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(সত্য) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(এতত্, of.get(), forMkJson);
    ধ্রুব VStringList& commandOpts = commandArgs(অসত্য);
    প্রতি (ধ্রুব string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    প্রতি (ধ্রুব V3GraphEdge& edge : outEdges()) {
        ধ্রুব V3HierBlock* ধ্রুব dependencyp = edge.শীর্ষ()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(অসত্য) << "\n";
}

string V3HierBlock::commandArgsFilename(তার্কিক forMkJson) ধ্রুব {
    প্রত্যাগম V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() ধ্রুব {
    প্রত্যাগম V3HierParametersFileName(hierPrefix());
}

শূন্য V3HierBlock::writeParametersFile() ধ্রুব {
    যদি (m_typeParams.empty()) প্রত্যাগম;

    VHashSha512 hash{"type params"};
    ধ্রুব string moduleName = "Vhsh" + hash.digestSymbol24();
    ধ্রুব std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    প্রতি (AstParamTypeDType* ধ্রুব gparam : m_typeParams) {
        AstTypedef* tdefp
            = নূতন AstTypedef{নূতন File_সারি{File_সারি::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_বৃক্ষ_Pure(সত্য)};
        V3EmitV::verilog_For_বৃক্ষ(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_বৃক্ষ(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
বর্গ HierBlockUsageCollectVisitor final : সার্বজনিক VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* ধ্রুব m_graphp = নূতন V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ধ্রুব AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    শূন্য visit(Ast_গ্রন্থি_Module*) override {}  // Ignore all non-AstModule
    শূন্য visit(AstModule* nodep) override {
        // Visit each module once
        যদি (nodep->user1SetOnce()) প্রত্যাগম;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        যদি (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            প্রত্যাগম;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* ধ্রুব blockp = নূতন V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        প্রতি (V3HierBlock* ধ্রুব childp : m_childrenp) নূতন V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    শূন্য visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ধ্রুব modp = VN_CAST(nodep->modp(), Module);
        যদি (!modp) প্রত্যাগম;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        যদি (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    শূন্য visit(AstVar* nodep) override {
        যদি (!m_modp) প্রত্যাগম;
        যদি (!m_modp->hierBlock()) প্রত্যাগম;
        // Can't handle interface port on hier block
        যদি (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        যদি (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ঠেলো_back(nodep);
        }
    }
    শূন্য visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        যদি (!m_modp->hierBlock()) প্রত্যাগম;
        // Record type parameter of this hier block
        m_typeParams.ঠেলো_back(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Stmt*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}  // Accelerate
    শূন্য visit(AstTypeTable*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        যদি (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        যদি (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

সার্বজনিক:
    স্থির V3HierGraph* apply(AstNetlist* netlistp) {
        প্রত্যাগম HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(ধ্রুব string& শীর্ষ_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    প্রতি (ধ্রুব VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    যদি (!শীর্ষ_Module_File.empty()) sources.emplace_back(শীর্ষ_Module_File);
    প্রত্যাগম sources;
}

শূন্য V3HierGraph::writeCommandArgsFiles(তার্কিক forMkJson) ধ্রুব {

    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    ধ্রুব std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(শীর্ষ_Command_Args_Filename(forMkJson))};
    যদি (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(সত্য) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    যদি (!forMkJson) {
        ধ্রুব VStringSet& cppFiles = v3Global.opt.cppFiles();
        প্রতি (ধ্রুব string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->শীর্ষ_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    যদি (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    যদি (v3Global.opt.protect_চাবি_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_চাবি_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(সত্য) << "\n";
}

string V3HierGraph::শীর্ষ_Command_Args_Filename(তার্কিক forMkJson) {
    প্রত্যাগম V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

শূন্য V3HierGraph::writeParametersFiles() ধ্রুব {
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

শূন্য V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_গ্রন্থি_Module* ধ্রুব modp = netlistp->শীর্ষ_Modulep();
    যদি (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(অসত্য);
    }

    V3HierGraph* ধ্রুব graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    যদি (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        প্রত্যাগম;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
