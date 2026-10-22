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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3HierBlock.h"

#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3String.h"

#അന്തര്ഭാവ <cctype>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <sstream>
#അന്തര്ഭാവ <utility>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

സ്ഥിതം string V3HierCommandArgsFilename(സ്ഥിരം string& prefix, തർക്കം forMkJson) {
    തിരികെ v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

സ്ഥിതം string V3HierParametersFileName(സ്ഥിരം string& prefix) {
    തിരികെ v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

സ്ഥിതം ശൂന്യം V3HierWriteCommonInputs(സ്ഥിരം V3HierBlock* hblockp, std::ostream* of, തർക്കം forMkJson) {
    സ്ഥിരം string മുകൾ_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    എങ്കിൽ (!forMkJson) {
        വേണ്ടി (സ്ഥിരം string& filename : V3HierGraph::sourceFiles(മുകൾ_Module_File))
            *of << filename << "\n";
    }
    വേണ്ടി (സ്ഥിരം auto& i : v3Global.opt.libraryFiles()) {
        എങ്കിൽ (V3Os::filenameRealPath(i.filename()) != മുകൾ_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

തർക്കം V3HierBlock::stringParamPassable(സ്ഥിരം string& മൂല്യം) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    എങ്കിൽ (മൂല്യം.find_first_of("\n\"") != string::npos) തിരികെ അസത്യം;
    എങ്കിൽ (മൂല്യം.find("/*") != string::npos) തിരികെ അസത്യം;
    വേണ്ടി (size_t pos = മൂല്യം.find("//"); pos != string::npos; pos = മൂല്യം.find("//", pos + 1)) {
        എങ്കിൽ (pos > 0 && std::isspace(static_cast<unsigned അക്ഷരം>(മൂല്യം[pos - 1]))) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(സ്ഥിരം std::vector<AstVar*>& gparams,
                                                     തർക്കം forGOption) {
    StrGParams strParams;
    വേണ്ടി (സ്ഥിരം AstVar* സ്ഥിരം gparam : gparams) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            എങ്കിൽ (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<അക്ഷരം> hexFpStr(32, '\0');
                സ്ഥിരം പൂർണ്ണസംഖ്യ len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->അക്കം().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } അല്ലെങ്കിൽ എങ്കിൽ (constp->isString()) {
                s = constp->അക്കം().toString();
                എങ്കിൽ (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } അല്ലെങ്കിൽ {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_സംഖ്യ അക്കം{constp->അക്കം()};
                അക്കം.isSigned(constp->isSigned());
                s = അക്കം.ascii(സത്യം, സത്യം);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    തിരികെ strParams;
}

VStringList V3HierBlock::commandArgs(തർക്കം forMkJson) സ്ഥിരം {
    VStringList opts;
    സ്ഥിരം string prefix = hierPrefix();
    എങ്കിൽ (!forMkJson) {
        opts.തള്ളുക_back(" --prefix " + prefix);
        opts.തള്ളുക_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.തള്ളുക_back(" --top-module-encoded " + modp()->name());
    }
    opts.തള്ളുക_back(" --lib-create " + modp()->name());  // possibly mangled name
    എങ്കിൽ (v3Global.opt.protect_താക്കോൽ_Provided())
        opts.തള്ളുക_back(" --protect-key " + v3Global.opt.protect_താക്കോൽ_Defaulted());
    opts.തള്ളുക_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    സ്ഥിരം StrGParams gparamsStr = stringifyParams(m_params, സത്യം);
    വേണ്ടി (സ്ഥിരം StrGParam& param : gparamsStr) {
        opts.തള്ളുക_back("-G" + param.first + "=" + param.second + "");
    }
    എങ്കിൽ (!m_typeParams.empty()) {
        opts.തള്ളുക_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    സ്ഥിരം പൂർണ്ണസംഖ്യ blockThreads = V3Control::getHierWorkers(m_modp->origName());
    എങ്കിൽ (blockThreads > 1) {
        എങ്കിൽ (!inEmpty()) {
            V3Control::get_Hier_Workers_File_വരി(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.തള്ളുക_back(" --threads " + std::to_string(blockThreads));
        }
    }

    തിരികെ opts;
}

VStringList V3HierBlock::hierBlockArgs() സ്ഥിരം {
    VStringList opts;
    സ്ഥിരം StrGParams gparamsStr = stringifyParams(m_params, അസത്യം);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    വേണ്ടി (സ്ഥിരം StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    തിരികെ opts;
}

string V3HierBlock::hierPrefix() സ്ഥിരം { തിരികെ "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(തർക്കം withDir, സ്ഥിരം അക്ഷരം* prefix, സ്ഥിരം അക്ഷരം* suffix) സ്ഥിരം {
    string s;
    എങ്കിൽ (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    തിരികെ s;
}

string V3HierBlock::hierWrapperFilename(തർക്കം withDir) സ്ഥിരം {
    തിരികെ hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(തർക്കം withDir) സ്ഥിരം {
    തിരികെ hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(തർക്കം withDir) സ്ഥിരം {
    തിരികെ hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(തർക്കം withDir) സ്ഥിരം {
    തിരികെ hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() സ്ഥിരം {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    വേണ്ടി (സ്ഥിരം auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        എങ്കിൽ (filename == V3Os::filenameRealPath(v.filename())) തിരികെ "";
    }
    തിരികെ filename;
}

ശൂന്യം V3HierBlock::writeCommandArgsFile(തർക്കം forMkJson) സ്ഥിരം {
    സ്ഥിരം std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    എങ്കിൽ (!forMkJson) {
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : outEdges()) {
            സ്ഥിരം V3HierBlock* സ്ഥിരം dependencyp = edge.മുകൾ()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(സത്യം) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(ഇത്, of.get(), forMkJson);
    സ്ഥിരം VStringList& commandOpts = commandArgs(അസത്യം);
    വേണ്ടി (സ്ഥിരം string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : outEdges()) {
        സ്ഥിരം V3HierBlock* സ്ഥിരം dependencyp = edge.മുകൾ()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(അസത്യം) << "\n";
}

string V3HierBlock::commandArgsFilename(തർക്കം forMkJson) സ്ഥിരം {
    തിരികെ V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() സ്ഥിരം {
    തിരികെ V3HierParametersFileName(hierPrefix());
}

ശൂന്യം V3HierBlock::writeParametersFile() സ്ഥിരം {
    എങ്കിൽ (m_typeParams.empty()) തിരികെ;

    VHashSha512 hash{"type params"};
    സ്ഥിരം string moduleName = "Vhsh" + hash.digestSymbol24();
    സ്ഥിരം std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    വേണ്ടി (AstParamTypeDType* സ്ഥിരം gparam : m_typeParams) {
        AstTypedef* tdefp
            = പുതിയ AstTypedef{പുതിയ File_വരി{File_വരി::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_വൃക്ഷം_Pure(സത്യം)};
        V3EmitV::verilog_For_വൃക്ഷം(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_വൃക്ഷം(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
വർഗ്ഗം HierBlockUsageCollectVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* സ്ഥിരം m_graphp = പുതിയ V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<സ്ഥിരം AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    ശൂന്യം visit(Ast_കണ്ണി_Module*) override {}  // Ignore all non-AstModule
    ശൂന്യം visit(AstModule* nodep) override {
        // Visit each module once
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        എങ്കിൽ (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            തിരികെ;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* സ്ഥിരം blockp = പുതിയ V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        വേണ്ടി (V3HierBlock* സ്ഥിരം childp : m_childrenp) പുതിയ V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    ശൂന്യം visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* സ്ഥിരം modp = VN_CAST(nodep->modp(), Module);
        എങ്കിൽ (!modp) തിരികെ;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        എങ്കിൽ (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (!m_modp) തിരികെ;
        എങ്കിൽ (!m_modp->hierBlock()) തിരികെ;
        // Can't handle interface port on hier block
        എങ്കിൽ (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        എങ്കിൽ (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.തള്ളുക_back(nodep);
        }
    }
    ശൂന്യം visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        എങ്കിൽ (!m_modp->hierBlock()) തിരികെ;
        // Record type parameter of this hier block
        m_typeParams.തള്ളുക_back(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Stmt*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}  // Accelerate
    ശൂന്യം visit(AstTypeTable*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        എങ്കിൽ (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        എങ്കിൽ (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

പൊതു:
    സ്ഥിതം V3HierGraph* apply(AstNetlist* netlistp) {
        തിരികെ HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(സ്ഥിരം string& മുകൾ_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    വേണ്ടി (സ്ഥിരം VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    എങ്കിൽ (!മുകൾ_Module_File.empty()) sources.emplace_back(മുകൾ_Module_File);
    തിരികെ sources;
}

ശൂന്യം V3HierGraph::writeCommandArgsFiles(തർക്കം forMkJson) സ്ഥിരം {

    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    സ്ഥിരം std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(മുകൾ_Command_Args_Filename(forMkJson))};
    എങ്കിൽ (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(സത്യം) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    എങ്കിൽ (!forMkJson) {
        സ്ഥിരം VStringSet& cppFiles = v3Global.opt.cppFiles();
        വേണ്ടി (സ്ഥിരം string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->മുകൾ_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    എങ്കിൽ (v3Global.opt.protect_താക്കോൽ_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_താക്കോൽ_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(സത്യം) << "\n";
}

string V3HierGraph::മുകൾ_Command_Args_Filename(തർക്കം forMkJson) {
    തിരികെ V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

ശൂന്യം V3HierGraph::writeParametersFiles() സ്ഥിരം {
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

ശൂന്യം V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_കണ്ണി_Module* സ്ഥിരം modp = netlistp->മുകൾ_Modulep();
    എങ്കിൽ (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(അസത്യം);
    }

    V3HierGraph* സ്ഥിരം graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    എങ്കിൽ (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        തിരികെ;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
