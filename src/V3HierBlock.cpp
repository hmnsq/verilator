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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3HierBlock.h"

#समावेश "V3Control.h"
#समावेश "V3EmitV.h"
#समावेश "V3File.h"
#समावेश "V3Os.h"
#समावेश "V3Stats.h"
#समावेश "V3String.h"

#समावेश <cctype>
#समावेश <memory>
#समावेश <sstream>
#समावेश <utility>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

अटल string V3HierCommandArgsFilename(स्थिर string& prefix, bool forMkJson) {
    वापस v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

अटल string V3HierParametersFileName(स्थिर string& prefix) {
    वापस v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

अटल व्योम V3HierWriteCommonInputs(स्थिर V3HierBlock* hblockp, std::ब_प्रवाह* of, bool forMkJson) {
    स्थिर string शीर्ष_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    अगर (!forMkJson) {
        क्रम (स्थिर string& filename : V3HierGraph::sourceFiles(शीर्ष_Module_File))
            *of << filename << "\n";
    }
    क्रम (स्थिर स्वतः& i : v3Global.opt.libraryFiles()) {
        अगर (V3Os::filenameRealPath(i.filename()) != शीर्ष_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

bool V3HierBlock::stringParamPassable(स्थिर string& मूल्य) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    अगर (मूल्य.find_first_of("\n\"") != string::npos) वापस false;
    अगर (मूल्य.find("/*") != string::npos) वापस false;
    क्रम (माप_प्रकार pos = मूल्य.find("//"); pos != string::npos; pos = मूल्य.find("//", pos + 1)) {
        अगर (pos > 0 && std::है_खाली(static_cast<अचिन्हित अक्षर>(मूल्य[pos - 1]))) वापस false;
    }
    वापस true;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(स्थिर std::vector<AstVar*>& gparams,
                                                     bool forGOption) {
    StrGParams strParams;
    क्रम (स्थिर AstVar* स्थिर gparam : gparams) {
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            अगर (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<अक्षर> hexFpStr(32, '\0');
                स्थिर पूर्णांक len = VL_SNPRINTF(hexFpStr.तथ्य(), hexFpStr.आकार(), "%a",
                                            constp->अंक().toDouble());
                UASSERT_OBJ(0 < len && static_cast<माप_प्रकार>(len) < hexFpStr.आकार(), constp,
                            " is not properly converted to string");
                s = hexFpStr.तथ्य();
            } अन्यथा अगर (constp->isString()) {
                s = constp->अंक().toString();
                अगर (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } अन्यथा {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_संख्या अंक{constp->अंक()};
                अंक.isSigned(constp->isSigned());
                s = अंक.ascii(true, true);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    वापस strParams;
}

VStringList V3HierBlock::commandArgs(bool forMkJson) स्थिर {
    VStringList opts;
    स्थिर string prefix = hierPrefix();
    अगर (!forMkJson) {
        opts.धकेलो_back(" --prefix " + prefix);
        opts.धकेलो_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.धकेलो_back(" --top-module-encoded " + modp()->name());
    }
    opts.धकेलो_back(" --lib-create " + modp()->name());  // possibly mangled name
    अगर (v3Global.opt.protect_कुंजी_Provided())
        opts.धकेलो_back(" --protect-key " + v3Global.opt.protect_कुंजी_Defaulted());
    opts.धकेलो_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    स्थिर StrGParams gparamsStr = stringifyParams(m_params, true);
    क्रम (स्थिर StrGParam& param : gparamsStr) {
        opts.धकेलो_back("-G" + param.first + "=" + param.second + "");
    }
    अगर (!m_typeParams.empty()) {
        opts.धकेलो_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    स्थिर पूर्णांक blockThreads = V3Control::getHierWorkers(m_modp->origName());
    अगर (blockThreads > 1) {
        अगर (!inEmpty()) {
            V3Control::get_Hier_Workers_File_पंक्ति(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } अन्यथा {
            अगर (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.धकेलो_back(" --threads " + std::to_string(blockThreads));
        }
    }

    वापस opts;
}

VStringList V3HierBlock::hierBlockArgs() स्थिर {
    VStringList opts;
    स्थिर StrGParams gparamsStr = stringifyParams(m_params, false);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    क्रम (स्थिर StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    वापस opts;
}

string V3HierBlock::hierPrefix() स्थिर { वापस "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(bool withDir, स्थिर अक्षर* prefix, स्थिर अक्षर* suffix) स्थिर {
    string s;
    अगर (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    वापस s;
}

string V3HierBlock::hierWrapperFilename(bool withDir) स्थिर {
    वापस hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(bool withDir) स्थिर {
    वापस hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(bool withDir) स्थिर {
    वापस hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(bool withDir) स्थिर {
    वापस hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() स्थिर {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    क्रम (स्थिर स्वतः& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        अगर (filename == V3Os::filenameRealPath(v.filename())) वापस "";
    }
    वापस filename;
}

व्योम V3HierBlock::writeCommandArgsFile(bool forMkJson) स्थिर {
    स्थिर std::unique_ptr<std::बख_प्रवाह> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    अगर (!forMkJson) {
        क्रम (स्थिर V3GraphEdge& edge : outEdges()) {
            स्थिर V3HierBlock* स्थिर dependencyp = edge.शीर्ष()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(true) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(यह, of.लो(), forMkJson);
    स्थिर VStringList& commandOpts = commandArgs(false);
    क्रम (स्थिर string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    क्रम (स्थिर V3GraphEdge& edge : outEdges()) {
        स्थिर V3HierBlock* स्थिर dependencyp = edge.शीर्ष()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(false) << "\n";
}

string V3HierBlock::commandArgsFilename(bool forMkJson) स्थिर {
    वापस V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() स्थिर {
    वापस V3HierParametersFileName(hierPrefix());
}

व्योम V3HierBlock::writeParametersFile() स्थिर {
    अगर (m_typeParams.empty()) वापस;

    VHashSha512 hash{"type params"};
    स्थिर string moduleName = "Vhsh" + hash.digestSymbol24();
    स्थिर std::unique_ptr<std::बख_प्रवाह> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    क्रम (AstParamTypeDType* स्थिर gparam : m_typeParams) {
        AstTypedef* tdefp
            = नया AstTypedef{नया File_पंक्ति{File_पंक्ति::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_वृक्ष_Pure(true)};
        V3EmitV::verilog_For_वृक्ष(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_वृक्ष(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
श्रेणी HierBlockUsageCollectVisitor final : खुला VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    स्थिर VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* स्थिर m_graphp = नया V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<स्थिर AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    व्योम visit(Ast_गाँठ_Module*) override {}  // Ignore all non-AstModule
    व्योम visit(AstModule* nodep) override {
        // Visit each module once
        अगर (nodep->user1SetOnce()) वापस;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        अगर (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            वापस;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* स्थिर blockp = नया V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        क्रम (V3HierBlock* स्थिर childp : m_childrenp) नया V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    व्योम visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* स्थिर modp = VN_CAST(nodep->modp(), Module);
        अगर (!modp) वापस;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        अगर (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    व्योम visit(AstVar* nodep) override {
        अगर (!m_modp) वापस;
        अगर (!m_modp->hierBlock()) वापस;
        // Can't handle interface port on hier block
        अगर (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        अगर (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.धकेलो_back(nodep);
        }
    }
    व्योम visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        अगर (!m_modp->hierBlock()) वापस;
        // Record type parameter of this hier block
        m_typeParams.धकेलो_back(nodep);
    }

    व्योम visit(Ast_गाँठ_Stmt*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    व्योम visit(AstTypeTable*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        अगर (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_Edges_योग(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        अगर (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

खुला:
    अटल V3HierGraph* apply(AstNetlist* netlistp) {
        वापस HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(स्थिर string& शीर्ष_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().आकार() + 1);
    क्रम (स्थिर VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    अगर (!शीर्ष_Module_File.empty()) sources.emplace_back(शीर्ष_Module_File);
    वापस sources;
}

व्योम V3HierGraph::writeCommandArgsFiles(bool forMkJson) स्थिर {

    क्रम (स्थिर V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    स्थिर std::unique_ptr<std::बख_प्रवाह> of{
        V3File::new_ofstream(शीर्ष_Command_Args_Filename(forMkJson))};
    अगर (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        क्रम (स्थिर V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(true) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.लो(), forMkJson);
    अगर (!forMkJson) {
        स्थिर VStringSet& cppFiles = v3Global.opt.cppFiles();
        क्रम (स्थिर string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->शीर्ष_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    अगर (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    अगर (v3Global.opt.protect_कुंजी_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_कुंजी_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(true) << "\n";
}

string V3HierGraph::शीर्ष_Command_Args_Filename(bool forMkJson) {
    वापस V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

व्योम V3HierGraph::writeParametersFiles() स्थिर {
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

व्योम V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_गाँठ_Module* स्थिर modp = netlistp->शीर्ष_Modulep();
    अगर (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(false);
    }

    V3HierGraph* स्थिर graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().आकार());
    // No hierarchical block is found, nothing to do.
    अगर (graphp->empty()) {
        VL_DO_DANGLING(मिटाओ graphp, graphp);
        वापस;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
