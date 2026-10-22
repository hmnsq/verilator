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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3HierBlock.h"

#شامل "V3Control.h"
#شامل "V3EmitV.h"
#شامل "V3File.h"
#شامل "V3Os.h"
#شامل "V3Stats.h"
#شامل "V3String.h"

#شامل <cctype>
#شامل <memory>
#شامل <sstream>
#شامل <utility>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ایستا string V3HierCommandArgsFilename(ثابت string& prefix, منطقی forMkJson) {
    بازگشت v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ایستا string V3HierParametersFileName(ثابت string& prefix) {
    بازگشت v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

ایستا تهی V3HierWriteCommonInputs(ثابت V3HierBlock* hblockp, std::ostream* of, منطقی forMkJson) {
    ثابت string بالاترین_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    اگر (!forMkJson) {
        برای (ثابت string& filename : V3HierGraph::sourceFiles(بالاترین_Module_File))
            *of << filename << "\n";
    }
    برای (ثابت auto& i : v3Global.opt.libraryFiles()) {
        اگر (V3Os::filenameRealPath(i.filename()) != بالاترین_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

منطقی V3HierBlock::stringParamPassable(ثابت string& مقدار) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    اگر (مقدار.find_first_of("\n\"") != string::npos) بازگشت نادرست;
    اگر (مقدار.find("/*") != string::npos) بازگشت نادرست;
    برای (size_t pos = مقدار.find("//"); pos != string::npos; pos = مقدار.find("//", pos + 1)) {
        اگر (pos > 0 && std::isspace(static_cast<unsigned نویسه>(مقدار[pos - 1]))) بازگشت نادرست;
    }
    بازگشت درست;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(ثابت std::vector<AstVar*>& gparams,
                                                     منطقی forGOption) {
    StrGParams strParams;
    برای (ثابت AstVar* ثابت gparam : gparams) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            اگر (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<نویسه> hexFpStr(32, '\0');
                ثابت صحیح len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->رقم().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } وگرنه اگر (constp->isString()) {
                s = constp->رقم().toString();
                اگر (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } وگرنه {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_عدد رقم{constp->رقم()};
                رقم.isSigned(constp->isSigned());
                s = رقم.ascii(درست, درست);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    بازگشت strParams;
}

VStringList V3HierBlock::commandArgs(منطقی forMkJson) ثابت {
    VStringList opts;
    ثابت string prefix = hierPrefix();
    اگر (!forMkJson) {
        opts.افزودن_back(" --prefix " + prefix);
        opts.افزودن_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.افزودن_back(" --top-module-encoded " + modp()->name());
    }
    opts.افزودن_back(" --lib-create " + modp()->name());  // possibly mangled name
    اگر (v3Global.opt.protect_کلید_Provided())
        opts.افزودن_back(" --protect-key " + v3Global.opt.protect_کلید_Defaulted());
    opts.افزودن_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    ثابت StrGParams gparamsStr = stringifyParams(m_params, درست);
    برای (ثابت StrGParam& param : gparamsStr) {
        opts.افزودن_back("-G" + param.first + "=" + param.second + "");
    }
    اگر (!m_typeParams.empty()) {
        opts.افزودن_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    ثابت صحیح blockThreads = V3Control::getHierWorkers(m_modp->origName());
    اگر (blockThreads > 1) {
        اگر (!inEmpty()) {
            V3Control::get_Hier_Workers_File_سطر(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } وگرنه {
            اگر (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.افزودن_back(" --threads " + std::to_string(blockThreads));
        }
    }

    بازگشت opts;
}

VStringList V3HierBlock::hierBlockArgs() ثابت {
    VStringList opts;
    ثابت StrGParams gparamsStr = stringifyParams(m_params, نادرست);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    برای (ثابت StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    بازگشت opts;
}

string V3HierBlock::hierPrefix() ثابت { بازگشت "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(منطقی withDir, ثابت نویسه* prefix, ثابت نویسه* suffix) ثابت {
    string s;
    اگر (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    بازگشت s;
}

string V3HierBlock::hierWrapperFilename(منطقی withDir) ثابت {
    بازگشت hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(منطقی withDir) ثابت {
    بازگشت hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(منطقی withDir) ثابت {
    بازگشت hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(منطقی withDir) ثابت {
    بازگشت hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() ثابت {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    برای (ثابت auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        اگر (filename == V3Os::filenameRealPath(v.filename())) بازگشت "";
    }
    بازگشت filename;
}

تهی V3HierBlock::writeCommandArgsFile(منطقی forMkJson) ثابت {
    ثابت std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    اگر (!forMkJson) {
        برای (ثابت V3GraphEdge& edge : outEdges()) {
            ثابت V3HierBlock* ثابت dependencyp = edge.بالاترین()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(درست) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(این, of.get(), forMkJson);
    ثابت VStringList& commandOpts = commandArgs(نادرست);
    برای (ثابت string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    برای (ثابت V3GraphEdge& edge : outEdges()) {
        ثابت V3HierBlock* ثابت dependencyp = edge.بالاترین()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(نادرست) << "\n";
}

string V3HierBlock::commandArgsFilename(منطقی forMkJson) ثابت {
    بازگشت V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() ثابت {
    بازگشت V3HierParametersFileName(hierPrefix());
}

تهی V3HierBlock::writeParametersFile() ثابت {
    اگر (m_typeParams.empty()) بازگشت;

    VHashSha512 hash{"type params"};
    ثابت string moduleName = "Vhsh" + hash.digestSymbol24();
    ثابت std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    برای (AstParamTypeDType* ثابت gparam : m_typeParams) {
        AstTypedef* tdefp
            = جدید AstTypedef{جدید File_سطر{File_سطر::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_درخت_Pure(درست)};
        V3EmitV::verilog_For_درخت(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_درخت(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
رده HierBlockUsageCollectVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ثابت VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* ثابت m_graphp = جدید V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ثابت AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    تهی visit(Ast_گره_Module*) override {}  // Ignore all non-AstModule
    تهی visit(AstModule* nodep) override {
        // Visit each module once
        اگر (nodep->user1SetOnce()) بازگشت;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        اگر (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            بازگشت;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* ثابت blockp = جدید V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        برای (V3HierBlock* ثابت childp : m_childrenp) جدید V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    تهی visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ثابت modp = VN_CAST(nodep->modp(), Module);
        اگر (!modp) بازگشت;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        اگر (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    تهی visit(AstVar* nodep) override {
        اگر (!m_modp) بازگشت;
        اگر (!m_modp->hierBlock()) بازگشت;
        // Can't handle interface port on hier block
        اگر (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        اگر (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.افزودن_back(nodep);
        }
    }
    تهی visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        اگر (!m_modp->hierBlock()) بازگشت;
        // Record type parameter of this hier block
        m_typeParams.افزودن_back(nodep);
    }

    تهی visit(Ast_گره_Stmt*) override {}  // Accelerate
    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    تهی visit(AstTypeTable*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        اگر (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        اگر (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

عمومی:
    ایستا V3HierGraph* apply(AstNetlist* netlistp) {
        بازگشت HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(ثابت string& بالاترین_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    برای (ثابت VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    اگر (!بالاترین_Module_File.empty()) sources.emplace_back(بالاترین_Module_File);
    بازگشت sources;
}

تهی V3HierGraph::writeCommandArgsFiles(منطقی forMkJson) ثابت {

    برای (ثابت V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    ثابت std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(بالاترین_Command_Args_Filename(forMkJson))};
    اگر (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        برای (ثابت V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(درست) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    اگر (!forMkJson) {
        ثابت VStringSet& cppFiles = v3Global.opt.cppFiles();
        برای (ثابت string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->بالاترین_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    برای (ثابت V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    اگر (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    اگر (v3Global.opt.protect_کلید_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_کلید_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(درست) << "\n";
}

string V3HierGraph::بالاترین_Command_Args_Filename(منطقی forMkJson) {
    بازگشت V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

تهی V3HierGraph::writeParametersFiles() ثابت {
    برای (ثابت V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

تهی V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_گره_Module* ثابت modp = netlistp->بالاترین_Modulep();
    اگر (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(نادرست);
    }

    V3HierGraph* ثابت graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    اگر (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        بازگشت;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
