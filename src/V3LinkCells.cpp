// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Resolve module/signal name references
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
// NO EDITS: Don't replace or delete nodes, as the parser symbol table
//           has pointers into the ast tree.
//
// LINK TRANSFORMATIONS:
//      Top-down traversal
//          Cells:
//              Read module if needed
//              Link to module that instantiates it
//*************************************************************************

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkCells.h"

#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Parse.h"
#অন্তর্ভাব "V3SymTable.h"

#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

শ্রেণী LinkCellsGraph final : সর্বজনীন V3Graph {
সর্বজনীন:
    LinkCellsGraph() = পূর্বনির্ধারিত;
    ~LinkCellsGraph() override = পূর্বনির্ধারিত;
    শূন্য loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

শ্রেণী LinkCellsVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_গ্রন্থি_Module* ধ্রুবক m_modp;

সর্বজনীন:
    LinkCellsVertex(V3Graph* graphp, Ast_গ্রন্থি_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = পূর্বনির্ধারিত;
    Ast_গ্রন্থি_Module* modp() ধ্রুবক VL_MT_STABLE { ফেরত m_modp; }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত cvtToHex(modp()) + ' ' + modp()->name(); }
    File_সারি* fileline() ধ্রুবক override { ফেরত modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ধ্রুবক override {
        ফেরত m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

শ্রেণী LibraryVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
সর্বজনীন:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = পূর্বনির্ধারিত;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত "*LIBRARY*"; }
};

শ্রেণী CellEdge final : সর্বজনীন V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* ধ্রুবক m_cellp;

সর্বজনীন:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণসংখ্যা weight, যৌক্তিক cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, শীর্ষ, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ধ্রুবক { ফেরত m_cellp; }
    string name() ধ্রুবক override VL_MT_STABLE {
        ফেরত cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

শূন্য LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    যদি (ধ্রুবক LinkCellsVertex* ধ্রুবক vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } অন্যথা {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
কাঠামো LinkCellsState final {
    // Set of possible top module names from command line and configs
    std::vector<std::pair<std::string, std::string>> m_designs;
    // Default library lists to search
    std::vector<std::string> m_liblistDefault;
    // Library lists for specific cells
    std::unordered_map<std::string, std::vector<std::string>> m_liblistCell;
    // Use list for specific cells (libname, cellname)
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>
        m_uselistCell;
    // Library lists for specific insts
    std::unordered_map<std::string, std::vector<std::string>> m_liblistInst;
    // Use list for specific insts (libname, cellname)
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>
        m_uselistInst;
};

শ্রেণী LinkConfigsVisitor final : সর্বজনীন VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    যৌক্তিক m_is_শীর্ষ = মিথ্যা;  // Whether we're in the top-level config
    যৌক্তিক m_isDefault = মিথ্যা;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    শূন্য visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_শীর্ষ);
        ধ্রুবক auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_শীর্ষ = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    শূন্য visit(AstConfigCell* nodep) override {
        যদি (m_is_শীর্ষ) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    শূন্য visit(AstConfigRule* nodep) override {
        যদি (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = সত্য;
            iterate_And_পরবর্তী_Null(nodep->usep());
        } অন্যথা যদি (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_পরবর্তী_Null(nodep->usep());
        } অন্যথা {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_পরবর্তী_Null(nodep->cellp());
            }
            iterate_And_পরবর্তী_Null(nodep->usep());
        }
    }

    শূন্য visit(AstDot* nodep) override { iterateChildren(nodep); }

    শূন্য visit(AstParseRef* nodep) override {
        যদি (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } অন্যথা যদি (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } অন্যথা যদি (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } অন্যথা যদি (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    শূন্য visit(AstConfigUse* nodep) override {
        যদি (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } অন্যথা যদি (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } অন্যথা যদি (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } অন্যথা যদি (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        যদি (!m_state.m_designs.আকার_2() && !v3Global.opt.শীর্ষ_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.শীর্ষ_Module());
        }
        iterate(nodep);
    }
};

শ্রেণী LinkCellsVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ধ্রুবক VNUser1InUse m_inuser1;
    ধ্রুবক VNUser2InUse m_inuser2;

    // STATE
    VInFilter* ধ্রুবক m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    ধ্রুবক V3GraphVertex* m_শীর্ষ_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_শীর্ষ_Module_Name;  // original name of the top module
    পূর্ণসংখ্যা m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_গ্রন্থি_Module* nodep) {
        // Return corresponding vertex for this module
        যদি (!nodep->user1p()) nodep->user1p(নতুন LinkCellsVertex{&m_graph, nodep});
        ফেরত nodep->user1u().toGraphVertex();
    }
    শূন্য newEdge(V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণসংখ্যা weight, যৌক্তিক cuttable) {
        ধ্রুবক V3GraphEdge* ধ্রুবক edgep = নতুন V3GraphEdge{&m_graph, fromp, শীর্ষ, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << শীর্ষ->name());
    }
    শূন্য cellEdge(V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণসংখ্যা weight, যৌক্তিক cuttable,
                  AstCell* cellp) {
        ধ্রুবক V3GraphEdge* ধ্রুবক edgep
            = নতুন CellEdge{&m_graph, fromp, শীর্ষ, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << শীর্ষ->name());
    }
    শূন্য ঢোকাও_Mod_In_Lib(ধ্রুবক string& name, ধ্রুবক string& libname, Ast_গ্রন্থি_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        যদি (!libSymp)
            libSymp = m_mods.rootp()->ঢোকাও(libname, নতুন VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->ঢোকাও(name, নতুন VSymEnt{&m_mods, nodep});
    }

    Ast_গ্রন্থি_Module* findModuleLibSym(ধ্রুবক string& modName, ধ্রুবক string& libname) {
        // Given module name and library name, find within that exact library
        ধ্রুবক VSymEnt* ধ্রুবক libSymp = m_mods.rootp()->findIdFallback(libname);
        যদি (!libSymp) ফেরত nullptr;
        ধ্রুবক VSymEnt* ধ্রুবক foundp = libSymp->findIdFallback(modName);
        ফেরত foundp ? VN_AS(foundp->nodep(), গ্রন্থি_Module) : nullptr;
    }
    Ast_গ্রন্থি_Module* findModuleSym(ধ্রুবক string& modName, ধ্রুবক string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_গ্রন্থি_Module* foundp;
        ধ্রুবক std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        ধ্রুবক auto itCellUseList = m_state.m_uselistCell.find(fullName);
        যদি (itCellUseList != m_state.m_uselistCell.end()) {
            জন্য (auto ধ্রুবক& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                যদি (foundp) ফেরত foundp;
            }
        }
        // Then search cell-specific library list
        ধ্রুবক auto itCellLibList = m_state.m_liblistCell.find(fullName);
        যদি (itCellLibList != m_state.m_liblistCell.end()) {
            জন্য (auto ধ্রুবক& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                যদি (foundp) ফেরত foundp;
            }
        }
        // Then search IEEE config default liblist
        জন্য (auto ধ্রুবক& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            যদি (foundp) ফেরত foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        যদি (foundp) ফেরত foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        ফেরত foundp;
    }

    Ast_গ্রন্থি_Module* resolveModule(Ast_গ্রন্থি* nodep, ধ্রুবক string& modName, ধ্রুবক string& libname) {
        Ast_গ্রন্থি_Module* modp = findModuleSym(modName, libname);
        যদি (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ধ্রুবক string prettyName = Ast_গ্রন্থি::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, সত্য, মিথ্যা, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            যদি (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        ফেরত modp;
    }

    স্থির শূন্য removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        যদি (!v3Global.opt.শীর্ষ_Module().empty()) ফেরত;
        জন্য (Ast_গ্রন্থি_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), গ্রন্থি_Module)) {
            যদি (!nodep->inLibrary()) ফেরত;
        }
        জন্য (Ast_গ্রন্থি_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), গ্রন্থি_Module)) {
            nodep->inLibrary(মিথ্যা);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, ধ্রুবক std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string শীর্ষ;
        যৌক্তিক শীর্ষ_Found = মিথ্যা;
        std::getline(ss, শীর্ষ, '.');
        জন্য (auto ধ্রুবক& pair : m_state.m_designs) {
            যদি (শীর্ষ == pair.second) {
                শীর্ষ_Found = সত্য;
                বিরতি;
            }
        }
        যদি (!শীর্ষ_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            ফেরত nullptr;
        }

        ধ্রুবক V3GraphVertex* vtx = m_শীর্ষ_Vertexp;
        ধ্রুবক CellEdge* finalEdgep = nullptr;
        std::string seg;
        যখন (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            জন্য (ধ্রুবক V3GraphEdge& edge : vtx->outEdges()) {
                যদি (ধ্রুবক CellEdge* ধ্রুবক cedgep = edge.cast<CellEdge>()) {
                    যদি (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->শীর্ষ();
                        finalEdgep = cedgep;
                        বিরতি;
                    }
                }
            }
            যদি (!finalEdgep) ফেরত nullptr;
        }

        ফেরত finalEdgep->cellp();
    }

    শূন্য assignParamNumbers(AstPin* ধ্রুবক firstPinp) {
        জন্য (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(সত্য);
            যদি (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_অঙ্ক()));
        }
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_গ্রন্থি_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        জন্য (auto ধ্রুবক& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            যদি (!cellp) চালু;
            জন্য (auto ধ্রুবক& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                যদি (modp) {
                    cellp->modp(modp);
                    বিরতি;
                }
            }
        }

        // Search uselists for each instance
        জন্য (auto ধ্রুবক& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            জন্য (auto ধ্রুবক& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                যদি (modp) {
                    cellp->modp(modp);
                    বিরতি;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        যদি (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (ধ্রুবক LinkCellsVertex* ধ্রুবক vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_গ্রন্থি_Module* ধ্রুবক vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (ধ্রুবক LinkCellsVertex* ধ্রুবক vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_গ্রন্থি_Module* ধ্রুবক vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        যদি (v3Global.opt.শীর্ষ_Module() != "" && !m_শীর্ষ_Vertexp) {
            VSpellCheck spell;
            জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
                যদি (ধ্রুবক LinkCellsVertex* ধ্রুবক vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_গ্রন্থি_Module* ধ্রুবক vmodp = vvertexp->modp();
                    যদি (VN_IS(vmodp, Module)) spell.ঠেলো_Candidate(vmodp->prettyName());
                }
            }
            ধ্রুবক string suggest
                = spell.best_Candidate_বার্তা(Ast_গ্রন্থি::prettyName(v3Global.opt.শীর্ষ_Module()));
            v3error("Specified --top-module '"
                    << Ast_গ্রন্থি::prettyName(v3Global.opt.শীর্ষ_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            যদি (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, মিথ্যা);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            যদি (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ধ্রুবক auto itFoundPair = m_declfnWarned.ঢোকাও(nodep->fileline()->filename());
                যদি (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            যদি (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(সত্য);  // Interfaces can't be at top, unless asked
            }

            auto ধ্রুবক& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ধ্রুবক যৌক্তিক শীর্ষ_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            যদি (শীর্ষ_Match) {
                m_শীর্ষ_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(মিথ্যা);  // Safer to make sure it doesn't disappear
            }
            যদি (v3Global.opt.শীর্ষ_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !শীর্ষ_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                যদি (!m_libVertexp) m_libVertexp = নতুন LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, মিথ্যা);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_বৃক্ষ();
        }
    }

    শূন্য visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_গ্রন্থি_Module* ধ্রুবক modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        যদি (modp) {
            যদি (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                যদি (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, মিথ্যা);
                যদি (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } অন্যথা যদি (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } অন্যথা {
                nodep->v3error("Non-interface used as an interface: "
                               << nodep->ifaceNameQ() << "\n"
                               << nodep->warnMore()
                                      + "... Perhaps intended an instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 23.3.2)?");
            }
        }
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Parser didn't know what was interface, resolve now
        // For historical reasons virtual interface reference variables remain VARs
        যদি (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    শূন্য visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        যদি (!nodep->packagep()) {
            Ast_গ্রন্থি_Module* ধ্রুবক modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            যদি (AstPackage* ধ্রুবক pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            যদি (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                ফেরত;
            }
        }
    }
    শূন্য visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        যদি (!nodep->packagep()) {
            Ast_গ্রন্থি_Module* ধ্রুবক modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            যদি (AstPackage* ধ্রুবক pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            যদি (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                ফেরত;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, মিথ্যা);
    }

    শূন্য visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_গ্রন্থি_Module* ধ্রুবক modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        যদি (modp) {
            Ast_গ্রন্থি* ধ্রুবক cellsp = nodep->cellsp()->unlink_Fr_Back_With_পরবর্তী();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterate_And_পরবর্তী_Null(cellsp);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    শূন্য visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ধ্রুবক যৌক্তিক cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        যদি (nodep->user1p() == m_modp) ফেরত;  // AstBind and AstNodeModule may call a cell twice
        যদি (nodep->modName() == m_orig_শীর্ষ_Module_Name) {
            যদি (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } অন্যথা {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                ফেরত;
            }
        }
        nodep->user1p(m_modp);
        //
        যদি (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_গ্রন্থি_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            যদি (cellmodp) {
                যদি (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(সত্য);
                    nodep->recursive(সত্য);
                    যদি (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_গ্রন্থি_Module* otherModp = VN_CAST(cellmodp->user2p(), গ্রন্থি_Module);
                        যদি (!otherModp) {
                            otherModp = cellmodp->clone_বৃক্ষ(মিথ্যা);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(সত্য);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, মিথ্যা);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } অন্যথা {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } অন্যথা {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, মিথ্যা, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        যদি (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ঠেলো_Deletep(nodep->pinsp()->unlink_Fr_Back_With_পরবর্তী());
        }
        যদি (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ঠেলো_Deletep(nodep->paramsp()->unlink_Fr_Back_With_পরবর্তী());
        }
        // Convert .* to list of pins
        যৌক্তিক pinStar = মিথ্যা;
        যৌক্তিক pinDotName = মিথ্যা;
        জন্য (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            যদি (pinp->svDotName()) pinDotName = সত্য;
            যদি (pinp->dotStar()) {
                যদি (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = সত্য;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_বৃক্ষ(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        জন্য (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            যদি (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_অঙ্ক()));
        }
        assignParamNumbers(nodep->paramsp());
        যদি (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            জন্য (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                যদি (!pinp->exprp()) {
                    যদি (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } অন্যথা {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.ঢোকাও(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            জন্য (Ast_গ্রন্থি* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                যদি (ধ্রুবক AstPort* ধ্রুবক portp = VN_CAST(portnodep, Port)) {

                    যদি (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_অঙ্ক())) == ports.end()) {
                        যদি (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ধ্রুবক newp
                                = নতুন AstPin{nodep->fileline(), 0, portp->name(),
                                             নতুন AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(সত্য);
                            newp->svImplicit(সত্য);
                            nodep->addPinsp(newp);
                        } অন্যথা {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ধ্রুবক AstVar* portp_varp = nullptr;
                            জন্য (Ast_গ্রন্থি* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(module_stmtsp, Var)) {
                                    যদি (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        বিরতি;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            যদি (portp_varp && portp_varp->is_প্রবেশ() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ধ্রুবক newp
                                    = নতুন AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } অন্যথা {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ধ্রুবক newp
                                    = নতুন AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        যদি (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            যদি (!nodep->hasIfaceVar()) {
                ধ্রুবক string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ধ্রুবক idtypep = নতুন AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                যদি (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    জন্য (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ঠেলো_back(rp);
                    }
                    Ast_গ্রন্থি_DType* dtp = idtypep;
                    জন্য (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = নতুন AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_বৃক্ষ(মিথ্যা)};
                    }
                    varp = নতুন AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } অন্যথা {
                    varp = নতুন AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(সত্য);
                nodep->add_পরবর্তী_Here(varp);
                nodep->hasIfaceVar(সত্য);
            }
        }
        যদি (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    শূন্য visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        যদি (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_গ্রন্থি_Module* ধ্রুবক varModp = findModuleSym(nodep->name(), m_modp->libname());
            যদি (AstIface* ধ্রুবক ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, মিথ্যা);
            }
        }
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    শূন্য visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_পরবর্তী_Null(nodep->childDTypep());
        }
        iterate_And_পরবর্তী_Null(nodep->delayp());
        iterate_And_পরবর্তী_Null(nodep->valuep());
        iterate_And_পরবর্তী_Null(nodep->attrsp());
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    // METHODS
    শূন্য readModNames() {
        // mangled_name, BlockOptions
        ধ্রুবক V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ধ্রুবক auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.শীর্ষ_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        জন্য (Ast_গ্রন্থি_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), গ্রন্থি_Module);
            যদি (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            যদি (libFoundp && libFoundp == nodep) {
                // Ok
            } অন্যথা যদি (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } অন্যথা যদি (libFoundp) {
                যদি (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
                      || nodep->fileline()->warnIsOff(V3ErrorCode::MODDUP)
                      || hierBlocks.find(nodep->name()) != hierBlocks.end())) {
                    nodep->v3warn(MODDUP, "Duplicate declaration of "
                                              << nodep->verilogKwd() << ": "
                                              << nodep->prettyNameQ() << '\n'
                                              << nodep->warnContextPrimary() << '\n'
                                              << libFoundp->warnOther()
                                              << "... Location of original declaration\n"
                                              << libFoundp->warnContextSecondary());
                }
                nodep->unlinkFrBack();
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা যদি (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ধ্রুবক string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                ঢোকাও_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                ঢোকাও_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } অন্যথা {
                ঢোকাও_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                ঢোকাও_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

সর্বজনীন:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        যদি (v3Global.opt.hierChild()) {
            ধ্রুবক V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.শীর্ষ_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            ধ্রুবক V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.শীর্ষ_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_শীর্ষ_Module_Name = hierIt->second.origName();
        } অন্যথা {
            m_orig_শীর্ষ_Module_Name = v3Global.opt.শীর্ষ_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        যদি (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

শূন্য V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
