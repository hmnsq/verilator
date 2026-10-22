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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3HierBlock.h"

#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <cctype>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ಸ್ಥಿರ string V3HierCommandArgsFilename(ಧ್ರುವ string& prefix, ತಾರ್ಕಿಕ forMkJson) {
    ಪ್ರತ್ಯಾಗಮ v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ಸ್ಥಿರ string V3HierParametersFileName(ಧ್ರುವ string& prefix) {
    ಪ್ರತ್ಯಾಗಮ v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

ಸ್ಥಿರ ಶೂನ್ಯ V3HierWriteCommonInputs(ಧ್ರುವ V3HierBlock* hblockp, std::ostream* of, ತಾರ್ಕಿಕ forMkJson) {
    ಧ್ರುವ string ತುದಿ_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    ಯದಿ (!forMkJson) {
        ಪ್ರತಿ (ಧ್ರುವ string& filename : V3HierGraph::sourceFiles(ತುದಿ_Module_File))
            *of << filename << "\n";
    }
    ಪ್ರತಿ (ಧ್ರುವ auto& i : v3Global.opt.libraryFiles()) {
        ಯದಿ (V3Os::filenameRealPath(i.filename()) != ತುದಿ_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

ತಾರ್ಕಿಕ V3HierBlock::stringParamPassable(ಧ್ರುವ string& ಮೌಲ್ಯ) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    ಯದಿ (ಮೌಲ್ಯ.find_first_of("\n\"") != string::npos) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (ಮೌಲ್ಯ.find("/*") != string::npos) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (size_t pos = ಮೌಲ್ಯ.find("//"); pos != string::npos; pos = ಮೌಲ್ಯ.find("//", pos + 1)) {
        ಯದಿ (pos > 0 && std::isspace(static_cast<unsigned ವರ್ಣ>(ಮೌಲ್ಯ[pos - 1]))) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(ಧ್ರುವ std::vector<AstVar*>& gparams,
                                                     ತಾರ್ಕಿಕ forGOption) {
    StrGParams strParams;
    ಪ್ರತಿ (ಧ್ರುವ AstVar* ಧ್ರುವ gparam : gparams) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            ಯದಿ (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<ವರ್ಣ> hexFpStr(32, '\0');
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->ಅಂಕೆ().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } ಅನ್ಯಥಾ ಯದಿ (constp->isString()) {
                s = constp->ಅಂಕೆ().toString();
                ಯದಿ (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } ಅನ್ಯಥಾ {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp->ಅಂಕೆ()};
                ಅಂಕೆ.isSigned(constp->isSigned());
                s = ಅಂಕೆ.ascii(ಸತ್ಯ, ಸತ್ಯ);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    ಪ್ರತ್ಯಾಗಮ strParams;
}

VStringList V3HierBlock::commandArgs(ತಾರ್ಕಿಕ forMkJson) ಧ್ರುವ {
    VStringList opts;
    ಧ್ರುವ string prefix = hierPrefix();
    ಯದಿ (!forMkJson) {
        opts.ತಳ್ಳು_back(" --prefix " + prefix);
        opts.ತಳ್ಳು_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ತಳ್ಳು_back(" --top-module-encoded " + modp()->name());
    }
    opts.ತಳ್ಳು_back(" --lib-create " + modp()->name());  // possibly mangled name
    ಯದಿ (v3Global.opt.protect_ಕೀಲಿ_Provided())
        opts.ತಳ್ಳು_back(" --protect-key " + v3Global.opt.protect_ಕೀಲಿ_Defaulted());
    opts.ತಳ್ಳು_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    ಧ್ರುವ StrGParams gparamsStr = stringifyParams(m_params, ಸತ್ಯ);
    ಪ್ರತಿ (ಧ್ರುವ StrGParam& param : gparamsStr) {
        opts.ತಳ್ಳು_back("-G" + param.first + "=" + param.second + "");
    }
    ಯದಿ (!m_typeParams.empty()) {
        opts.ತಳ್ಳು_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ blockThreads = V3Control::getHierWorkers(m_modp->origName());
    ಯದಿ (blockThreads > 1) {
        ಯದಿ (!inEmpty()) {
            V3Control::get_Hier_Workers_File_ಸಾಲು(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } ಅನ್ಯಥಾ {
            ಯದಿ (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.ತಳ್ಳು_back(" --threads " + std::to_string(blockThreads));
        }
    }

    ಪ್ರತ್ಯಾಗಮ opts;
}

VStringList V3HierBlock::hierBlockArgs() ಧ್ರುವ {
    VStringList opts;
    ಧ್ರುವ StrGParams gparamsStr = stringifyParams(m_params, ಅಸತ್ಯ);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    ಪ್ರತಿ (ಧ್ರುವ StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    ಪ್ರತ್ಯಾಗಮ opts;
}

string V3HierBlock::hierPrefix() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(ತಾರ್ಕಿಕ withDir, ಧ್ರುವ ವರ್ಣ* prefix, ಧ್ರುವ ವರ್ಣ* suffix) ಧ್ರುವ {
    string s;
    ಯದಿ (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    ಪ್ರತ್ಯಾಗಮ s;
}

string V3HierBlock::hierWrapperFilename(ತಾರ್ಕಿಕ withDir) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(ತಾರ್ಕಿಕ withDir) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(ತಾರ್ಕಿಕ withDir) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(ತಾರ್ಕಿಕ withDir) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() ಧ್ರುವ {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    ಪ್ರತಿ (ಧ್ರುವ auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        ಯದಿ (filename == V3Os::filenameRealPath(v.filename())) ಪ್ರತ್ಯಾಗಮ "";
    }
    ಪ್ರತ್ಯಾಗಮ filename;
}

ಶೂನ್ಯ V3HierBlock::writeCommandArgsFile(ತಾರ್ಕಿಕ forMkJson) ಧ್ರುವ {
    ಧ್ರುವ std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    ಯದಿ (!forMkJson) {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : outEdges()) {
            ಧ್ರುವ V3HierBlock* ಧ್ರುವ dependencyp = edge.ತುದಿ()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(ಸತ್ಯ) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(ಏತತ್, of.get(), forMkJson);
    ಧ್ರುವ VStringList& commandOpts = commandArgs(ಅಸತ್ಯ);
    ಪ್ರತಿ (ಧ್ರುವ string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : outEdges()) {
        ಧ್ರುವ V3HierBlock* ಧ್ರುವ dependencyp = edge.ತುದಿ()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(ಅಸತ್ಯ) << "\n";
}

string V3HierBlock::commandArgsFilename(ತಾರ್ಕಿಕ forMkJson) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ V3HierParametersFileName(hierPrefix());
}

ಶೂನ್ಯ V3HierBlock::writeParametersFile() ಧ್ರುವ {
    ಯದಿ (m_typeParams.empty()) ಪ್ರತ್ಯಾಗಮ;

    VHashSha512 hash{"type params"};
    ಧ್ರುವ string moduleName = "Vhsh" + hash.digestSymbol24();
    ಧ್ರುವ std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    ಪ್ರತಿ (AstParamTypeDType* ಧ್ರುವ gparam : m_typeParams) {
        AstTypedef* tdefp
            = ನೂತನ AstTypedef{ನೂತನ File_ಸಾಲು{File_ಸಾಲು::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
        V3EmitV::verilog_For_ವೃಕ್ಷ(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_ವೃಕ್ಷ(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
ವರ್ಗ HierBlockUsageCollectVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* ಧ್ರುವ m_graphp = ನೂತನ V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ಧ್ರುವ AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module*) override {}  // Ignore all non-AstModule
    ಶೂನ್ಯ visit(AstModule* nodep) override {
        // Visit each module once
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        ಯದಿ (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* ಧ್ರುವ blockp = ನೂತನ V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        ಪ್ರತಿ (V3HierBlock* ಧ್ರುವ childp : m_childrenp) ನೂತನ V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ಧ್ರುವ modp = VN_CAST(nodep->modp(), Module);
        ಯದಿ (!modp) ಪ್ರತ್ಯಾಗಮ;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        ಯದಿ (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (!m_modp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!m_modp->hierBlock()) ಪ್ರತ್ಯಾಗಮ;
        // Can't handle interface port on hier block
        ಯದಿ (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        ಯದಿ (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ತಳ್ಳು_back(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        ಯದಿ (!m_modp->hierBlock()) ಪ್ರತ್ಯಾಗಮ;
        // Record type parameter of this hier block
        m_typeParams.ತಳ್ಳು_back(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ visit(AstTypeTable*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        ಯದಿ (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        ಯದಿ (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3HierGraph* apply(AstNetlist* netlistp) {
        ಪ್ರತ್ಯಾಗಮ HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(ಧ್ರುವ string& ತುದಿ_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    ಪ್ರತಿ (ಧ್ರುವ VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    ಯದಿ (!ತುದಿ_Module_File.empty()) sources.emplace_back(ತುದಿ_Module_File);
    ಪ್ರತ್ಯಾಗಮ sources;
}

ಶೂನ್ಯ V3HierGraph::writeCommandArgsFiles(ತಾರ್ಕಿಕ forMkJson) ಧ್ರುವ {

    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    ಧ್ರುವ std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(ತುದಿ_Command_Args_Filename(forMkJson))};
    ಯದಿ (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(ಸತ್ಯ) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    ಯದಿ (!forMkJson) {
        ಧ್ರುವ VStringSet& cppFiles = v3Global.opt.cppFiles();
        ಪ್ರತಿ (ಧ್ರುವ string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->ತುದಿ_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    ಯದಿ (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    ಯದಿ (v3Global.opt.protect_ಕೀಲಿ_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_ಕೀಲಿ_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(ಸತ್ಯ) << "\n";
}

string V3HierGraph::ತುದಿ_Command_Args_Filename(ತಾರ್ಕಿಕ forMkJson) {
    ಪ್ರತ್ಯಾಗಮ V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

ಶೂನ್ಯ V3HierGraph::writeParametersFiles() ಧ್ರುವ {
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

ಶೂನ್ಯ V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = netlistp->ತುದಿ_Modulep();
    ಯದಿ (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(ಅಸತ್ಯ);
    }

    V3HierGraph* ಧ್ರುವ graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    ಯದಿ (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        ಪ್ರತ್ಯಾಗಮ;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
