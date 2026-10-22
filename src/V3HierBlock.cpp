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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3HierBlock.h"

#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <cctype>
#अन्तर्भाव <memory>
#अन्तर्भाव <sstream>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

स्थिर string V3HierCommandArgsFilename(स्थिरांक string& prefix, तार्किक forMkJson) {
    परत v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

स्थिर string V3HierParametersFileName(स्थिरांक string& prefix) {
    परत v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

स्थिर शून्य V3HierWriteCommonInputs(स्थिरांक V3HierBlock* hblockp, std::ostream* of, तार्किक forMkJson) {
    स्थिरांक string शीर्ष_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    जर (!forMkJson) {
        साठी (स्थिरांक string& filename : V3HierGraph::sourceFiles(शीर्ष_Module_File))
            *of << filename << "\n";
    }
    साठी (स्थिरांक स्वतः& i : v3Global.opt.libraryFiles()) {
        जर (V3Os::filenameRealPath(i.filename()) != शीर्ष_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

तार्किक V3HierBlock::stringParamPassable(स्थिरांक string& मूल्य) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    जर (मूल्य.find_first_of("\n\"") != string::npos) परत असत्य;
    जर (मूल्य.find("/*") != string::npos) परत असत्य;
    साठी (माप_प्रकार pos = मूल्य.find("//"); pos != string::npos; pos = मूल्य.find("//", pos + 1)) {
        जर (pos > 0 && std::है_खाली(static_cast<अचिन्हित अक्षर>(मूल्य[pos - 1]))) परत असत्य;
    }
    परत सत्य;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(स्थिरांक std::vector<AstVar*>& gparams,
                                                     तार्किक forGOption) {
    StrGParams strParams;
    साठी (स्थिरांक AstVar* स्थिरांक gparam : gparams) {
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            जर (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<अक्षर> hexFpStr(32, '\0');
                स्थिरांक पूर्णांक len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->अंक().toDouble());
                UASSERT_OBJ(0 < len && static_cast<माप_प्रकार>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } अन्यथा जर (constp->isString()) {
                s = constp->अंक().toString();
                जर (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } अन्यथा {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_संख्या अंक{constp->अंक()};
                अंक.isSigned(constp->isSigned());
                s = अंक.ascii(सत्य, सत्य);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    परत strParams;
}

VStringList V3HierBlock::commandArgs(तार्किक forMkJson) स्थिरांक {
    VStringList opts;
    स्थिरांक string prefix = hierPrefix();
    जर (!forMkJson) {
        opts.ढकला_back(" --prefix " + prefix);
        opts.ढकला_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ढकला_back(" --top-module-encoded " + modp()->name());
    }
    opts.ढकला_back(" --lib-create " + modp()->name());  // possibly mangled name
    जर (v3Global.opt.protect_किल्ली_Provided())
        opts.ढकला_back(" --protect-key " + v3Global.opt.protect_किल्ली_Defaulted());
    opts.ढकला_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    स्थिरांक StrGParams gparamsStr = stringifyParams(m_params, सत्य);
    साठी (स्थिरांक StrGParam& param : gparamsStr) {
        opts.ढकला_back("-G" + param.first + "=" + param.second + "");
    }
    जर (!m_typeParams.empty()) {
        opts.ढकला_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    स्थिरांक पूर्णांक blockThreads = V3Control::getHierWorkers(m_modp->origName());
    जर (blockThreads > 1) {
        जर (!inEmpty()) {
            V3Control::get_Hier_Workers_File_ओळ(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } अन्यथा {
            जर (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.ढकला_back(" --threads " + std::to_string(blockThreads));
        }
    }

    परत opts;
}

VStringList V3HierBlock::hierBlockArgs() स्थिरांक {
    VStringList opts;
    स्थिरांक StrGParams gparamsStr = stringifyParams(m_params, असत्य);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    साठी (स्थिरांक StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    परत opts;
}

string V3HierBlock::hierPrefix() स्थिरांक { परत "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(तार्किक withDir, स्थिरांक अक्षर* prefix, स्थिरांक अक्षर* suffix) स्थिरांक {
    string s;
    जर (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    परत s;
}

string V3HierBlock::hierWrapperFilename(तार्किक withDir) स्थिरांक {
    परत hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(तार्किक withDir) स्थिरांक {
    परत hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(तार्किक withDir) स्थिरांक {
    परत hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(तार्किक withDir) स्थिरांक {
    परत hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() स्थिरांक {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    साठी (स्थिरांक स्वतः& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        जर (filename == V3Os::filenameRealPath(v.filename())) परत "";
    }
    परत filename;
}

शून्य V3HierBlock::writeCommandArgsFile(तार्किक forMkJson) स्थिरांक {
    स्थिरांक std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    जर (!forMkJson) {
        साठी (स्थिरांक V3GraphEdge& edge : outEdges()) {
            स्थिरांक V3HierBlock* स्थिरांक dependencyp = edge.शीर्ष()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(सत्य) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(हे, of.get(), forMkJson);
    स्थिरांक VStringList& commandOpts = commandArgs(असत्य);
    साठी (स्थिरांक string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    साठी (स्थिरांक V3GraphEdge& edge : outEdges()) {
        स्थिरांक V3HierBlock* स्थिरांक dependencyp = edge.शीर्ष()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(असत्य) << "\n";
}

string V3HierBlock::commandArgsFilename(तार्किक forMkJson) स्थिरांक {
    परत V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() स्थिरांक {
    परत V3HierParametersFileName(hierPrefix());
}

शून्य V3HierBlock::writeParametersFile() स्थिरांक {
    जर (m_typeParams.empty()) परत;

    VHashSha512 hash{"type params"};
    स्थिरांक string moduleName = "Vhsh" + hash.digestSymbol24();
    स्थिरांक std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    साठी (AstParamTypeDType* स्थिरांक gparam : m_typeParams) {
        AstTypedef* tdefp
            = नवीन AstTypedef{नवीन File_ओळ{File_ओळ::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_वृक्ष_Pure(सत्य)};
        V3EmitV::verilog_For_वृक्ष(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_वृक्ष(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
वर्ग HierBlockUsageCollectVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    स्थिरांक VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* स्थिरांक m_graphp = नवीन V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<स्थिरांक AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    शून्य visit(Ast_गाठ_Module*) override {}  // Ignore all non-AstModule
    शून्य visit(AstModule* nodep) override {
        // Visit each module once
        जर (nodep->user1SetOnce()) परत;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        जर (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            परत;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* स्थिरांक blockp = नवीन V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        साठी (V3HierBlock* स्थिरांक childp : m_childrenp) नवीन V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    शून्य visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* स्थिरांक modp = VN_CAST(nodep->modp(), Module);
        जर (!modp) परत;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        जर (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    शून्य visit(AstVar* nodep) override {
        जर (!m_modp) परत;
        जर (!m_modp->hierBlock()) परत;
        // Can't handle interface port on hier block
        जर (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        जर (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ढकला_back(nodep);
        }
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        जर (!m_modp->hierBlock()) परत;
        // Record type parameter of this hier block
        m_typeParams.ढकला_back(nodep);
    }

    शून्य visit(Ast_गाठ_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_गाठ_अभिव्यक्ती*) override {}  // Accelerate
    शून्य visit(AstTypeTable*) override {}  // Accelerate
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        जर (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        जर (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

सार्वजनिक:
    स्थिर V3HierGraph* apply(AstNetlist* netlistp) {
        परत HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(स्थिरांक string& शीर्ष_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    साठी (स्थिरांक VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    जर (!शीर्ष_Module_File.empty()) sources.emplace_back(शीर्ष_Module_File);
    परत sources;
}

शून्य V3HierGraph::writeCommandArgsFiles(तार्किक forMkJson) स्थिरांक {

    साठी (स्थिरांक V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    स्थिरांक std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(शीर्ष_Command_Args_Filename(forMkJson))};
    जर (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        साठी (स्थिरांक V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(सत्य) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    जर (!forMkJson) {
        स्थिरांक VStringSet& cppFiles = v3Global.opt.cppFiles();
        साठी (स्थिरांक string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->शीर्ष_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    साठी (स्थिरांक V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    जर (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    जर (v3Global.opt.protect_किल्ली_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_किल्ली_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(सत्य) << "\n";
}

string V3HierGraph::शीर्ष_Command_Args_Filename(तार्किक forMkJson) {
    परत V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

शून्य V3HierGraph::writeParametersFiles() स्थिरांक {
    साठी (स्थिरांक V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

शून्य V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_गाठ_Module* स्थिरांक modp = netlistp->शीर्ष_Modulep();
    जर (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(असत्य);
    }

    V3HierGraph* स्थिरांक graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    जर (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        परत;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
