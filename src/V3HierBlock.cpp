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

स्थिर string V3HierCommandArgsFilename(ध्रुव string& prefix, तार्किक forMkJson) {
    पटिनिवत्त v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

स्थिर string V3HierParametersFileName(ध्रुव string& prefix) {
    पटिनिवत्त v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

स्थिर शून्य V3HierWriteCommonInputs(ध्रुव V3HierBlock* hblockp, std::ostream* of, तार्किक forMkJson) {
    ध्रुव string शिखरम्_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    सचे (!forMkJson) {
        अत्थाय (ध्रुव string& filename : V3HierGraph::sourceFiles(शिखरम्_Module_File))
            *of << filename << "\n";
    }
    अत्थाय (ध्रुव auto& i : v3Global.opt.libraryFiles()) {
        सचे (V3Os::filenameRealPath(i.filename()) != शिखरम्_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

तार्किक V3HierBlock::stringParamPassable(ध्रुव string& मानम्) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    सचे (मानम्.find_first_of("\n\"") != string::npos) पटिनिवत्त मिच्छा;
    सचे (मानम्.find("/*") != string::npos) पटिनिवत्त मिच्छा;
    अत्थाय (size_t pos = मानम्.find("//"); pos != string::npos; pos = मानम्.find("//", pos + 1)) {
        सचे (pos > 0 && std::isspace(static_cast<unsigned वर्ण>(मानम्[pos - 1]))) पटिनिवत्त मिच्छा;
    }
    पटिनिवत्त सच्च;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(ध्रुव std::vector<AstVar*>& gparams,
                                                     तार्किक forGOption) {
    StrGParams strParams;
    अत्थाय (ध्रुव AstVar* ध्रुव gparam : gparams) {
        सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            सचे (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<वर्ण> hexFpStr(32, '\0');
                ध्रुव पूर्णाङ्क len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->अङ्कः().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } अञ्ञथा सचे (constp->isString()) {
                s = constp->अङ्कः().toString();
                सचे (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } अञ्ञथा {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_सङ्ख्या अङ्कः{constp->अङ्कः()};
                अङ्कः.isSigned(constp->isSigned());
                s = अङ्कः.ascii(सच्च, सच्च);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    पटिनिवत्त strParams;
}

VStringList V3HierBlock::commandArgs(तार्किक forMkJson) ध्रुव {
    VStringList opts;
    ध्रुव string prefix = hierPrefix();
    सचे (!forMkJson) {
        opts.प्रक्षिप_back(" --prefix " + prefix);
        opts.प्रक्षिप_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.प्रक्षिप_back(" --top-module-encoded " + modp()->name());
    }
    opts.प्रक्षिप_back(" --lib-create " + modp()->name());  // possibly mangled name
    सचे (v3Global.opt.protect_कुञ्चिका_Provided())
        opts.प्रक्षिप_back(" --protect-key " + v3Global.opt.protect_कुञ्चिका_Defaulted());
    opts.प्रक्षिप_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    ध्रुव StrGParams gparamsStr = stringifyParams(m_params, सच्च);
    अत्थाय (ध्रुव StrGParam& param : gparamsStr) {
        opts.प्रक्षिप_back("-G" + param.first + "=" + param.second + "");
    }
    सचे (!m_typeParams.empty()) {
        opts.प्रक्षिप_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    ध्रुव पूर्णाङ्क blockThreads = V3Control::getHierWorkers(m_modp->origName());
    सचे (blockThreads > 1) {
        सचे (!inEmpty()) {
            V3Control::get_Hier_Workers_File_पङ्क्तिः(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } अञ्ञथा {
            सचे (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.प्रक्षिप_back(" --threads " + std::to_string(blockThreads));
        }
    }

    पटिनिवत्त opts;
}

VStringList V3HierBlock::hierBlockArgs() ध्रुव {
    VStringList opts;
    ध्रुव StrGParams gparamsStr = stringifyParams(m_params, मिच्छा);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    अत्थाय (ध्रुव StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    पटिनिवत्त opts;
}

string V3HierBlock::hierPrefix() ध्रुव { पटिनिवत्त "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(तार्किक withDir, ध्रुव वर्ण* prefix, ध्रुव वर्ण* suffix) ध्रुव {
    string s;
    सचे (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    पटिनिवत्त s;
}

string V3HierBlock::hierWrapperFilename(तार्किक withDir) ध्रुव {
    पटिनिवत्त hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(तार्किक withDir) ध्रुव {
    पटिनिवत्त hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(तार्किक withDir) ध्रुव {
    पटिनिवत्त hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(तार्किक withDir) ध्रुव {
    पटिनिवत्त hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() ध्रुव {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    अत्थाय (ध्रुव auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        सचे (filename == V3Os::filenameRealPath(v.filename())) पटिनिवत्त "";
    }
    पटिनिवत्त filename;
}

शून्य V3HierBlock::writeCommandArgsFile(तार्किक forMkJson) ध्रुव {
    ध्रुव std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    सचे (!forMkJson) {
        अत्थाय (ध्रुव V3GraphEdge& edge : outEdges()) {
            ध्रुव V3HierBlock* ध्रुव dependencyp = edge.शिखरम्()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(सच्च) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(एतत्, of.get(), forMkJson);
    ध्रुव VStringList& commandOpts = commandArgs(मिच्छा);
    अत्थाय (ध्रुव string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    अत्थाय (ध्रुव V3GraphEdge& edge : outEdges()) {
        ध्रुव V3HierBlock* ध्रुव dependencyp = edge.शिखरम्()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(मिच्छा) << "\n";
}

string V3HierBlock::commandArgsFilename(तार्किक forMkJson) ध्रुव {
    पटिनिवत्त V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() ध्रुव {
    पटिनिवत्त V3HierParametersFileName(hierPrefix());
}

शून्य V3HierBlock::writeParametersFile() ध्रुव {
    सचे (m_typeParams.empty()) पटिनिवत्त;

    VHashSha512 hash{"type params"};
    ध्रुव string moduleName = "Vhsh" + hash.digestSymbol24();
    ध्रुव std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    अत्थाय (AstParamTypeDType* ध्रुव gparam : m_typeParams) {
        AstTypedef* tdefp
            = नूतन AstTypedef{नूतन File_पङ्क्तिः{File_पङ्क्तिः::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->clone_वृक्षः_Pure(सच्च)};
        V3EmitV::verilog_For_वृक्षः(tdefp, *of);
        VL_DO_DANGLING(tdefp->delete_वृक्षः(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
वग्ग HierBlockUsageCollectVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ध्रुव VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* ध्रुव m_graphp = नूतन V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ध्रुव AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    शून्य visit(Ast_ग्रन्थिः_Module*) override {}  // Ignore all non-AstModule
    शून्य visit(AstModule* nodep) override {
        // Visit each module once
        सचे (nodep->user1SetOnce()) पटिनिवत्त;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        सचे (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            पटिनिवत्त;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* ध्रुव blockp = नूतन V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        अत्थाय (V3HierBlock* ध्रुव childp : m_childrenp) नूतन V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    शून्य visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ध्रुव modp = VN_CAST(nodep->modp(), Module);
        सचे (!modp) पटिनिवत्त;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        सचे (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    शून्य visit(AstVar* nodep) override {
        सचे (!m_modp) पटिनिवत्त;
        सचे (!m_modp->hierBlock()) पटिनिवत्त;
        // Can't handle interface port on hier block
        सचे (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        सचे (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.प्रक्षिप_back(nodep);
        }
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        सचे (!m_modp->hierBlock()) पटिनिवत्त;
        // Record type parameter of this hier block
        m_typeParams.प्रक्षिप_back(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    शून्य visit(AstTypeTable*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        सचे (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        सचे (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

सार्वजनिक:
    स्थिर V3HierGraph* apply(AstNetlist* netlistp) {
        पटिनिवत्त HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(ध्रुव string& शिखरम्_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    अत्थाय (ध्रुव VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    सचे (!शिखरम्_Module_File.empty()) sources.emplace_back(शिखरम्_Module_File);
    पटिनिवत्त sources;
}

शून्य V3HierGraph::writeCommandArgsFiles(तार्किक forMkJson) ध्रुव {

    अत्थाय (ध्रुव V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    ध्रुव std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(शिखरम्_Command_Args_Filename(forMkJson))};
    सचे (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        अत्थाय (ध्रुव V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(सच्च) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    सचे (!forMkJson) {
        ध्रुव VStringSet& cppFiles = v3Global.opt.cppFiles();
        अत्थाय (ध्रुव string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->शिखरम्_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    अत्थाय (ध्रुव V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    सचे (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    सचे (v3Global.opt.protect_कुञ्चिका_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_कुञ्चिका_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(सच्च) << "\n";
}

string V3HierGraph::शिखरम्_Command_Args_Filename(तार्किक forMkJson) {
    पटिनिवत्त V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

शून्य V3HierGraph::writeParametersFiles() ध्रुव {
    अत्थाय (ध्रुव V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

शून्य V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_ग्रन्थिः_Module* ध्रुव modp = netlistp->शिखरम्_Modulep();
    सचे (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(मिच्छा);
    }

    V3HierGraph* ध्रुव graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    सचे (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        पटिनिवत्त;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
