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
    प्रत्यागम v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

स्थिर string V3HierParametersFileName(ध्रुव string& prefix) {
    प्रत्यागम v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

स्थिर शून्य V3HierWriteCommonInputs(ध्रुव V3HierBlock* hblockp, std::ostream* of, तार्किक forMkJson) {
    ध्रुव string शीर्ष_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    यदि (!forMkJson) {
        प्रति (ध्रुव string& filename : V3HierGraph::sourceFiles(शीर्ष_Module_File))
            *of << filename << "\n";
    }
    प्रति (ध्रुव auto& i : v3Global.opt.libraryFiles()) {
        यदि (V3Os::filenameRealPath(i.filename()) != शीर्ष_Module_File)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

तार्किक V3HierBlock::stringParamPassable(ध्रुव string& मूल्य) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    यदि (मूल्य.find_first_of("\n\"") != string::npos) प्रत्यागम असत्य;
    यदि (मूल्य.find("/*") != string::npos) प्रत्यागम असत्य;
    प्रति (size_t pos = मूल्य.find("//"); pos != string::npos; pos = मूल्य.find("//", pos + 1)) {
        यदि (pos > 0 && std::isspace(static_cast<unsigned वर्ण>(मूल्य[pos - 1]))) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(ध्रुव std::vector<AstVar*>& gparams,
                                                     तार्किक forGOption) {
    StrGParams strParams;
    प्रति (ध्रुव AstVar* ध्रुव gparam : gparams) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            यदि (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<वर्ण> hexFpStr(32, '\0');
                ध्रुव पूर्णाङ्क len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->अंक().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } अन्यथा यदि (constp->isString()) {
                s = constp->अंक().toString();
                यदि (!forGOption) s = VString::quoteBackslash(s);
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
    प्रत्यागम strParams;
}

VStringList V3HierBlock::commandArgs(तार्किक forMkJson) ध्रुव {
    VStringList opts;
    ध्रुव string prefix = hierPrefix();
    यदि (!forMkJson) {
        opts.ढकला_back(" --prefix " + prefix);
        opts.ढकला_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ढकला_back(" --top-module-encoded " + modp()->name());
    }
    opts.ढकला_back(" --lib-create " + modp()->name());  // possibly mangled name
    यदि (v3Global.opt.protect_किल्ली_Provided())
        opts.ढकला_back(" --protect-key " + v3Global.opt.protect_किल्ली_Defaulted());
    opts.ढकला_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    ध्रुव StrGParams gparamsStr = stringifyParams(m_params, सत्य);
    प्रति (ध्रुव StrGParam& param : gparamsStr) {
        opts.ढकला_back("-G" + param.first + "=" + param.second + "");
    }
    यदि (!m_typeParams.empty()) {
        opts.ढकला_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    ध्रुव पूर्णाङ्क blockThreads = V3Control::getHierWorkers(m_modp->origName());
    यदि (blockThreads > 1) {
        यदि (!inEmpty()) {
            V3Control::get_Hier_Workers_File_ओळ(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } अन्यथा {
            यदि (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.ढकला_back(" --threads " + std::to_string(blockThreads));
        }
    }

    प्रत्यागम opts;
}

VStringList V3HierBlock::hierBlockArgs() ध्रुव {
    VStringList opts;
    ध्रुव StrGParams gparamsStr = stringifyParams(m_params, असत्य);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    प्रति (ध्रुव StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    प्रत्यागम opts;
}

string V3HierBlock::hierPrefix() ध्रुव { प्रत्यागम "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(तार्किक withDir, ध्रुव वर्ण* prefix, ध्रुव वर्ण* suffix) ध्रुव {
    string s;
    यदि (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    प्रत्यागम s;
}

string V3HierBlock::hierWrapperFilename(तार्किक withDir) ध्रुव {
    प्रत्यागम hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(तार्किक withDir) ध्रुव {
    प्रत्यागम hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(तार्किक withDir) ध्रुव {
    प्रत्यागम hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(तार्किक withDir) ध्रुव {
    प्रत्यागम hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() ध्रुव {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    प्रति (ध्रुव auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        यदि (filename == V3Os::filenameRealPath(v.filename())) प्रत्यागम "";
    }
    प्रत्यागम filename;
}

शून्य V3HierBlock::writeCommandArgsFile(तार्किक forMkJson) ध्रुव {
    ध्रुव std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    यदि (!forMkJson) {
        प्रति (ध्रुव V3GraphEdge& edge : outEdges()) {
            ध्रुव V3HierBlock* ध्रुव dependencyp = edge.शीर्ष()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(सत्य) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(एतत्, of.get(), forMkJson);
    ध्रुव VStringList& commandOpts = commandArgs(असत्य);
    प्रति (ध्रुव string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    प्रति (ध्रुव V3GraphEdge& edge : outEdges()) {
        ध्रुव V3HierBlock* ध्रुव dependencyp = edge.शीर्ष()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(असत्य) << "\n";
}

string V3HierBlock::commandArgsFilename(तार्किक forMkJson) ध्रुव {
    प्रत्यागम V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() ध्रुव {
    प्रत्यागम V3HierParametersFileName(hierPrefix());
}

शून्य V3HierBlock::writeParametersFile() ध्रुव {
    यदि (m_typeParams.empty()) प्रत्यागम;

    VHashSha512 hash{"type params"};
    ध्रुव string moduleName = "Vhsh" + hash.digestSymbol24();
    ध्रुव std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    प्रति (AstParamTypeDType* ध्रुव gparam : m_typeParams) {
        AstTypedef* tdefp
            = नूतन AstTypedef{नूतन File_ओळ{File_ओळ::builtInFilename()}, gparam->name(), nullptr,
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
    शून्य visit(Ast_गाठ_Module*) override {}  // Ignore all non-AstModule
    शून्य visit(AstModule* nodep) override {
        // Visit each module once
        यदि (nodep->user1SetOnce()) प्रत्यागम;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        यदि (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            प्रत्यागम;
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
        प्रति (V3HierBlock* ध्रुव childp : m_childrenp) नूतन V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    शून्य visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ध्रुव modp = VN_CAST(nodep->modp(), Module);
        यदि (!modp) प्रत्यागम;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        यदि (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    शून्य visit(AstVar* nodep) override {
        यदि (!m_modp) प्रत्यागम;
        यदि (!m_modp->hierBlock()) प्रत्यागम;
        // Can't handle interface port on hier block
        यदि (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        यदि (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ढकला_back(nodep);
        }
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        यदि (!m_modp->hierBlock()) प्रत्यागम;
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
        यदि (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        यदि (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

सार्वजनिक:
    स्थिर V3HierGraph* apply(AstNetlist* netlistp) {
        प्रत्यागम HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(ध्रुव string& शीर्ष_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    प्रति (ध्रुव VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    यदि (!शीर्ष_Module_File.empty()) sources.emplace_back(शीर्ष_Module_File);
    प्रत्यागम sources;
}

शून्य V3HierGraph::writeCommandArgsFiles(तार्किक forMkJson) ध्रुव {

    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    ध्रुव std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(शीर्ष_Command_Args_Filename(forMkJson))};
    यदि (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(सत्य) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    यदि (!forMkJson) {
        ध्रुव VStringSet& cppFiles = v3Global.opt.cppFiles();
        प्रति (ध्रुव string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->शीर्ष_Modulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    यदि (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    यदि (v3Global.opt.protect_किल्ली_Provided()) {
        *of << "--protect-key " << v3Global.opt.protect_किल्ली_Defaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(सत्य) << "\n";
}

string V3HierGraph::शीर्ष_Command_Args_Filename(तार्किक forMkJson) {
    प्रत्यागम V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

शून्य V3HierGraph::writeParametersFiles() ध्रुव {
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

शून्य V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_गाठ_Module* ध्रुव modp = netlistp->शीर्ष_Modulep();
    यदि (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(असत्य);
    }

    V3HierGraph* ध्रुव graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    यदि (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        प्रत्यागम;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
