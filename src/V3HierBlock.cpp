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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3HierBlock.h"

#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ଅନ୍ତର୍ଭାଵ <cctype>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <sstream>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ସ୍ଥିର string V3HierCommandArgsFilename(ଅଚଳ string& prefix, ତାର୍କିକ forMkJson) {
    ଫେରନ୍ତୁ v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ସ୍ଥିର string V3HierParametersFileName(ଅଚଳ string& prefix) {
    ଫେରନ୍ତୁ v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

ସ୍ଥିର ଶୂନ୍ୟ V3HierWriteCommonInputs(ଅଚଳ V3HierBlock* hblockp, std::ostream* of, ତାର୍କିକ forMkJson) {
    ଅଚଳ string ଶୀର୍ଷ_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    ଯଦି (!forMkJson) {
        ପାଇଁ (ଅଚଳ string& filename : V3HierGraph::sourceFiles(ଶୀର୍ଷ_Module_File))
            *of << filename << "\n";
    }
    ପାଇଁ (ଅଚଳ auto& i : v3Global.opt.libraryFiles()) {
        ଯଦି (V3Os::filenameRealPath(i.filename()) != ଶୀର୍ଷ_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

ତାର୍କିକ V3HierBlock::stringParamPassable(ଅଚଳ string& ମୂଲ୍ୟ) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    ଯଦି (ମୂଲ୍ୟ.find_first_of("\n\"") != string::npos) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (ମୂଲ୍ୟ.find("/*") != string::npos) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (size_t pos = ମୂଲ୍ୟ.find("//"); pos != string::npos; pos = ମୂଲ୍ୟ.find("//", pos + 1)) {
        ଯଦି (pos > 0 && std::isspace(static_cast<unsigned ଅକ୍ଷର>(ମୂଲ୍ୟ[pos - 1]))) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(ଅଚଳ std::vector<AstVar*>& gparams,
                                                     ତାର୍କିକ forGOption) {
    StrGParams strParams;
    ପାଇଁ (ଅଚଳ AstVar* ଅଚଳ gparam : gparams) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            ଯଦି (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<ଅକ୍ଷର> hexFpStr(32, '\0');
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->ଅଙ୍କ().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } ଅନ୍ୟଥା ଯଦି (constp->isString()) {
                s = constp->ଅଙ୍କ().toString();
                ଯଦି (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } ଅନ୍ୟଥା {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_ସଂଖ୍ୟା ଅଙ୍କ{constp->ଅଙ୍କ()};
                ଅଙ୍କ.isSigned(constp->isSigned());
                s = ଅଙ୍କ.ascii(ସତ୍ୟ, ସତ୍ୟ);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    ଫେରନ୍ତୁ strParams;
}

VStringList V3HierBlock::commandArgs(ତାର୍କିକ forMkJson) ଅଚଳ {
    VStringList opts;
    ଅଚଳ string prefix = hierPrefix();
    ଯଦି (!forMkJson) {
        opts.ଠେଲ_back(" --prefix " + prefix);
        opts.ଠେଲ_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ଠେଲ_back(" --top-module-encoded " + modp()->name());
    }
    opts.ଠେଲ_back(" --lib-create " + modp()->name());  // possibly mangled name
    ଯଦି (v3Global.opt.protect_ଚାବି_Provided())
        opts.ଠେଲ_back(" --protect-key " + v3Global.opt.protect_ଚାବି_Defaulted());
    opts.ଠେଲ_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    ଅଚଳ StrGParams gparamsStr = stringifyParams(m_params, ସତ୍ୟ);
    ପାଇଁ (ଅଚଳ StrGParam& param : gparamsStr) {
        opts.ଠେଲ_back("-G" + param.first + "=" + param.second + "");
    }
    ଯଦି (!m_typeParams.empty()) {
        opts.ଠେଲ_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା blockThreads = V3Control::getHierWorkers(m_modp->origName());
    ଯଦି (blockThreads > 1) {
        ଯଦି (!inEmpty()) {
            V3Control::get_Hier_Workers_File_ଧାଡ଼ି(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } ଅନ୍ୟଥା {
            ଯଦି (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.ଠେଲ_back(" --threads " + std::to_string(blockThreads));
        }
    }

    ଫେରନ୍ତୁ opts;
}

VStringList V3HierBlock::hierBlockArgs() ଅଚଳ {
    VStringList opts;
    ଅଚଳ StrGParams gparamsStr = stringifyParams(m_params, ମିଥ୍ୟା);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    ପାଇଁ (ଅଚଳ StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    ଫେରନ୍ତୁ opts;
}

string V3HierBlock::hierPrefix() ଅଚଳ { ଫେରନ୍ତୁ "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(ତାର୍କିକ withDir, ଅଚଳ ଅକ୍ଷର* prefix, ଅଚଳ ଅକ୍ଷର* suffix) ଅଚଳ {
    string s;
    ଯଦି (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    ଫେରନ୍ତୁ s;
}

string V3HierBlock::hierWrapperFilename(ତାର୍କିକ withDir) ଅଚଳ {
    ଫେରନ୍ତୁ hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(ତାର୍କିକ withDir) ଅଚଳ {
    ଫେରନ୍ତୁ hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(ତାର୍କିକ withDir) ଅଚଳ {
    ଫେରନ୍ତୁ hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(ତାର୍କିକ withDir) ଅଚଳ {
    ଫେରନ୍ତୁ hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() ଅଚଳ {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    ପାଇଁ (ଅଚଳ auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        ଯଦି (filename == V3Os::filenameRealPath(v.filename())) ଫେରନ୍ତୁ "";
    }
    ଫେରନ୍ତୁ filename;
}

ଶୂନ୍ୟ V3HierBlock::writeCommandArgsFile(ତାର୍କିକ forMkJson) ଅଚଳ {
    ଅଚଳ std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    ଯଦି (!forMkJson) {
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : outEdges()) {
            ଅଚଳ V3HierBlock* ଅଚଳ dependencyp = edge.ଶୀର୍ଷ()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(ସତ୍ୟ) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(ଏହି, of.get(), forMkJson);
    ଅଚଳ VStringList& commandOpts = commandArgs(ମିଥ୍ୟା);
    ପାଇଁ (ଅଚଳ string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : outEdges()) {
        ଅଚଳ V3HierBlock* ଅଚଳ dependencyp = edge.ଶୀର୍ଷ()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(ମିଥ୍ୟା) << "\n";
}

string V3HierBlock::commandArgsFilename(ତାର୍କିକ forMkJson) ଅଚଳ {
    ଫେରନ୍ତୁ V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() ଅଚଳ {
    ଫେରନ୍ତୁ V3HierParametersFileName(hierPrefix());
}

ଶୂନ୍ୟ V3HierBlock::writeParametersFile() ଅଚଳ {
    ଯଦି (m_typeParams.empty()) ଫେରନ୍ତୁ;

    VHashSha512 hash{"type params"};
    ଅଚଳ string moduleName = "Vhsh" + hash.digestSymbol24();
    ଅଚଳ std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    ପାଇଁ (AstParamTypeDType* ଅଚଳ gparam : m_typeParams) {
        AstTypedef* tdefp
            = ନୂତନ AstTypedef{ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
        V3EmitV::verilog_For_ବୃକ୍ଷ(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_ବୃକ୍ଷ(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
ଶ୍ରେଣୀ HierBlockUsageCollectVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* ଅଚଳ m_graphp = ନୂତନ V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ଅଚଳ AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module*) override {}  // Ignore all non-AstModule
    ଶୂନ୍ୟ visit(AstModule* nodep) override {
        // Visit each module once
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        ଯଦି (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            ଫେରନ୍ତୁ;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* ଅଚଳ blockp = ନୂତନ V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        ପାଇଁ (V3HierBlock* ଅଚଳ childp : m_childrenp) ନୂତନ V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ଅଚଳ modp = VN_CAST(nodep->modp(), Module);
        ଯଦି (!modp) ଫେରନ୍ତୁ;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        ଯଦି (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (!m_modp) ଫେରନ୍ତୁ;
        ଯଦି (!m_modp->hierBlock()) ଫେରନ୍ତୁ;
        // Can't handle interface port on hier block
        ଯଦି (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        ଯଦି (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ଠେଲ_back(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        ଯଦି (!m_modp->hierBlock()) ଫେରନ୍ତୁ;
        // Record type parameter of this hier block
        m_typeParams.ଠେଲ_back(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(AstTypeTable*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        ଯଦି (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        ଯଦି (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର V3HierGraph* apply(AstNetlist* netlistp) {
        ଫେରନ୍ତୁ HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(ଅଚଳ string& ଶୀର୍ଷ_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    ପାଇଁ (ଅଚଳ VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    ଯଦି (!ଶୀର୍ଷ_Module_File.empty()) sources.emplace_back(ଶୀର୍ଷ_Module_File);
    ଫେରନ୍ତୁ sources;
}

ଶୂନ୍ୟ V3HierGraph::writeCommandArgsFiles(ତାର୍କିକ forMkJson) ଅଚଳ {

    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    ଅଚଳ std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(ଶୀର୍ଷ_Command_Args_Filename(forMkJson))};
    ଯଦି (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(ସତ୍ୟ) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    ଯଦି (!forMkJson) {
        ଅଚଳ VStringSet& cppFiles = v3Global.opt.cppFiles();
        ପାଇଁ (ଅଚଳ string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->ଶୀର୍ଷ_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    ଯଦି (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    ଯଦି (v3Global.opt.protect_ଚାବି_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_ଚାବି_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(ସତ୍ୟ) << "\n";
}

string V3HierGraph::ଶୀର୍ଷ_Command_Args_Filename(ତାର୍କିକ forMkJson) {
    ଫେରନ୍ତୁ V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

ଶୂନ୍ୟ V3HierGraph::writeParametersFiles() ଅଚଳ {
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

ଶୂନ୍ୟ V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = netlistp->ଶୀର୍ଷ_Modulep();
    ଯଦି (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(ମିଥ୍ୟା);
    }

    V3HierGraph* ଅଚଳ graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    ଯଦି (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        ଫେରନ୍ତୁ;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
