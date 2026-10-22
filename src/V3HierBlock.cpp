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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3HierBlock.h"

#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3String.h"

#அந்தர்பாவ <cctype>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <sstream>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

நிலையான string V3HierCommandArgsFilename(மாறிலி string& prefix, தருக்க forMkJson) {
    திரும்பு v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

நிலையான string V3HierParametersFileName(மாறிலி string& prefix) {
    திரும்பு v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

நிலையான வெறுமை V3HierWriteCommonInputs(மாறிலி V3HierBlock* hblockp, std::ostream* of, தருக்க forMkJson) {
    மாறிலி string உச்சி_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    எனில் (!forMkJson) {
        க்கு (மாறிலி string& filename : V3HierGraph::sourceFiles(உச்சி_Module_File))
            *of << filename << "\n";
    }
    க்கு (மாறிலி auto& i : v3Global.opt.libraryFiles()) {
        எனில் (V3Os::filenameRealPath(i.filename()) != உச்சி_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

தருக்க V3HierBlock::stringParamPassable(மாறிலி string& மதிப்பு) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    எனில் (மதிப்பு.find_first_of("\n\"") != string::npos) திரும்பு பொய்;
    எனில் (மதிப்பு.find("/*") != string::npos) திரும்பு பொய்;
    க்கு (size_t pos = மதிப்பு.find("//"); pos != string::npos; pos = மதிப்பு.find("//", pos + 1)) {
        எனில் (pos > 0 && std::isspace(static_cast<unsigned எழுத்து>(மதிப்பு[pos - 1]))) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(மாறிலி std::vector<AstVar*>& gparams,
                                                     தருக்க forGOption) {
    StrGParams strParams;
    க்கு (மாறிலி AstVar* மாறிலி gparam : gparams) {
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            எனில் (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<எழுத்து> hexFpStr(32, '\0');
                மாறிலி முழு len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->இலக்கம்().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } இல்லையெனில் எனில் (constp->isString()) {
                s = constp->இலக்கம்().toString();
                எனில் (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } இல்லையெனில் {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_எண் இலக்கம்{constp->இலக்கம்()};
                இலக்கம்.isSigned(constp->isSigned());
                s = இலக்கம்.ascii(உண்மை, உண்மை);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    திரும்பு strParams;
}

VStringList V3HierBlock::commandArgs(தருக்க forMkJson) மாறிலி {
    VStringList opts;
    மாறிலி string prefix = hierPrefix();
    எனில் (!forMkJson) {
        opts.தள்ளு_back(" --prefix " + prefix);
        opts.தள்ளு_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.தள்ளு_back(" --top-module-encoded " + modp()->name());
    }
    opts.தள்ளு_back(" --lib-create " + modp()->name());  // possibly mangled name
    எனில் (v3Global.opt.protect_சாவி_Provided())
        opts.தள்ளு_back(" --protect-key " + v3Global.opt.protect_சாவி_Defaulted());
    opts.தள்ளு_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    மாறிலி StrGParams gparamsStr = stringifyParams(m_params, உண்மை);
    க்கு (மாறிலி StrGParam& param : gparamsStr) {
        opts.தள்ளு_back("-G" + param.first + "=" + param.second + "");
    }
    எனில் (!m_typeParams.empty()) {
        opts.தள்ளு_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    மாறிலி முழு blockThreads = V3Control::getHierWorkers(m_modp->origName());
    எனில் (blockThreads > 1) {
        எனில் (!inEmpty()) {
            V3Control::get_Hier_Workers_File_வரி(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } இல்லையெனில் {
            எனில் (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.தள்ளு_back(" --threads " + std::to_string(blockThreads));
        }
    }

    திரும்பு opts;
}

VStringList V3HierBlock::hierBlockArgs() மாறிலி {
    VStringList opts;
    மாறிலி StrGParams gparamsStr = stringifyParams(m_params, பொய்);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    க்கு (மாறிலி StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    திரும்பு opts;
}

string V3HierBlock::hierPrefix() மாறிலி { திரும்பு "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(தருக்க withDir, மாறிலி எழுத்து* prefix, மாறிலி எழுத்து* suffix) மாறிலி {
    string s;
    எனில் (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    திரும்பு s;
}

string V3HierBlock::hierWrapperFilename(தருக்க withDir) மாறிலி {
    திரும்பு hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(தருக்க withDir) மாறிலி {
    திரும்பு hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(தருக்க withDir) மாறிலி {
    திரும்பு hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(தருக்க withDir) மாறிலி {
    திரும்பு hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() மாறிலி {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    க்கு (மாறிலி auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        எனில் (filename == V3Os::filenameRealPath(v.filename())) திரும்பு "";
    }
    திரும்பு filename;
}

வெறுமை V3HierBlock::writeCommandArgsFile(தருக்க forMkJson) மாறிலி {
    மாறிலி std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    எனில் (!forMkJson) {
        க்கு (மாறிலி V3GraphEdge& edge : outEdges()) {
            மாறிலி V3HierBlock* மாறிலி dependencyp = edge.உச்சி()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(உண்மை) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(இது, of.get(), forMkJson);
    மாறிலி VStringList& commandOpts = commandArgs(பொய்);
    க்கு (மாறிலி string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    க்கு (மாறிலி V3GraphEdge& edge : outEdges()) {
        மாறிலி V3HierBlock* மாறிலி dependencyp = edge.உச்சி()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(பொய்) << "\n";
}

string V3HierBlock::commandArgsFilename(தருக்க forMkJson) மாறிலி {
    திரும்பு V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() மாறிலி {
    திரும்பு V3HierParametersFileName(hierPrefix());
}

வெறுமை V3HierBlock::writeParametersFile() மாறிலி {
    எனில் (m_typeParams.empty()) திரும்பு;

    VHashSha512 hash{"type params"};
    மாறிலி string moduleName = "Vhsh" + hash.digestSymbol24();
    மாறிலி std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    க்கு (AstParamTypeDType* மாறிலி gparam : m_typeParams) {
        AstTypedef* tdefp
            = புதிய AstTypedef{புதிய File_வரி{File_வரி::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_மரம்_Pure(உண்மை)};
        V3EmitV::verilog_For_மரம்(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_மரம்(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
வகுப்பு HierBlockUsageCollectVisitor final : பொது VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* மாறிலி m_graphp = புதிய V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<மாறிலி AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    வெறுமை visit(Ast_முனை_Module*) override {}  // Ignore all non-AstModule
    வெறுமை visit(AstModule* nodep) override {
        // Visit each module once
        எனில் (nodep->user1SetOnce()) திரும்பு;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        எனில் (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            திரும்பு;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* மாறிலி blockp = புதிய V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        க்கு (V3HierBlock* மாறிலி childp : m_childrenp) புதிய V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    வெறுமை visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* மாறிலி modp = VN_CAST(nodep->modp(), Module);
        எனில் (!modp) திரும்பு;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        எனில் (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    வெறுமை visit(AstVar* nodep) override {
        எனில் (!m_modp) திரும்பு;
        எனில் (!m_modp->hierBlock()) திரும்பு;
        // Can't handle interface port on hier block
        எனில் (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        எனில் (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.தள்ளு_back(nodep);
        }
    }
    வெறுமை visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        எனில் (!m_modp->hierBlock()) திரும்பு;
        // Record type parameter of this hier block
        m_typeParams.தள்ளு_back(nodep);
    }

    வெறுமை visit(Ast_முனை_Stmt*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை_கோவை*) override {}  // Accelerate
    வெறுமை visit(AstTypeTable*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        எனில் (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        எனில் (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

பொது:
    நிலையான V3HierGraph* apply(AstNetlist* netlistp) {
        திரும்பு HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(மாறிலி string& உச்சி_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    க்கு (மாறிலி VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    எனில் (!உச்சி_Module_File.empty()) sources.emplace_back(உச்சி_Module_File);
    திரும்பு sources;
}

வெறுமை V3HierGraph::writeCommandArgsFiles(தருக்க forMkJson) மாறிலி {

    க்கு (மாறிலி V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    மாறிலி std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(உச்சி_Command_Args_Filename(forMkJson))};
    எனில் (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        க்கு (மாறிலி V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(உண்மை) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    எனில் (!forMkJson) {
        மாறிலி VStringSet& cppFiles = v3Global.opt.cppFiles();
        க்கு (மாறிலி string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->உச்சி_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    க்கு (மாறிலி V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    எனில் (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    எனில் (v3Global.opt.protect_சாவி_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_சாவி_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(உண்மை) << "\n";
}

string V3HierGraph::உச்சி_Command_Args_Filename(தருக்க forMkJson) {
    திரும்பு V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

வெறுமை V3HierGraph::writeParametersFiles() மாறிலி {
    க்கு (மாறிலி V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

வெறுமை V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_முனை_Module* மாறிலி modp = netlistp->உச்சி_Modulep();
    எனில் (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(பொய்);
    }

    V3HierGraph* மாறிலி graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    எனில் (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        திரும்பு;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
