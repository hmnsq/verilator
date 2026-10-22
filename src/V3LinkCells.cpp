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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkCells.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"
#ಅನ್ತರ್ಭಾವ "V3SymTable.h"

#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

ವರ್ಗ LinkCellsGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
ಸಾರ್ವಜನಿಕ:
    LinkCellsGraph() = ಸ್ವಭಾವ;
    ~LinkCellsGraph() override = ಸ್ವಭಾವ;
    ಶೂನ್ಯ loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

ವರ್ಗ LinkCellsVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;

ಸಾರ್ವಜನಿಕ:
    LinkCellsVertex(V3Graph* graphp, Ast_ಗಂಟು_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = ಸ್ವಭಾವ;
    Ast_ಗಂಟು_Module* modp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_modp; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ cvtToHex(modp()) + ' ' + modp()->name(); }
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

ವರ್ಗ LibraryVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
ಸಾರ್ವಜನಿಕ:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = ಸ್ವಭಾವ;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ "*LIBRARY*"; }
};

ವರ್ಗ CellEdge final : ಸಾರ್ವಜನಿಕ V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* ಧ್ರುವ m_cellp;

ಸಾರ್ವಜನಿಕ:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight, ತಾರ್ಕಿಕ cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, ತುದಿ, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cellp; }
    string name() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

ಶೂನ್ಯ LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    ಯದಿ (ಧ್ರುವ LinkCellsVertex* ಧ್ರುವ vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } ಅನ್ಯಥಾ {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
ಸಂರಚನಾ LinkCellsState final {
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

ವರ್ಗ LinkConfigsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    ತಾರ್ಕಿಕ m_is_ತುದಿ = ಅಸತ್ಯ;  // Whether we're in the top-level config
    ತಾರ್ಕಿಕ m_isDefault = ಅಸತ್ಯ;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    ಶೂನ್ಯ visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_ತುದಿ);
        ಧ್ರುವ auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_ತುದಿ = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ಶೂನ್ಯ visit(AstConfigCell* nodep) override {
        ಯದಿ (m_is_ತುದಿ) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    ಶೂನ್ಯ visit(AstConfigRule* nodep) override {
        ಯದಿ (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = ಸತ್ಯ;
            iterateAndNextNull(nodep->usep());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } ಅನ್ಯಥಾ {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    ಶೂನ್ಯ visit(AstDot* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ visit(AstParseRef* nodep) override {
        ಯದಿ (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } ಅನ್ಯಥಾ ಯದಿ (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } ಅನ್ಯಥಾ ಯದಿ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } ಅನ್ಯಥಾ ಯದಿ (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    ಶೂನ್ಯ visit(AstConfigUse* nodep) override {
        ಯದಿ (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } ಅನ್ಯಥಾ ಯದಿ (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } ಅನ್ಯಥಾ ಯದಿ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } ಅನ್ಯಥಾ ಯದಿ (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        ಯದಿ (!m_state.m_designs.size() && !v3Global.opt.ತುದಿ_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.ತುದಿ_Module());
        }
        iterate(nodep);
    }
};

ವರ್ಗ LinkCellsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE
    VInFilter* ಧ್ರುವ m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    ಧ್ರುವ V3GraphVertex* m_ತುದಿ_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_ತುದಿ_Module_Name;  // original name of the top module
    ಪೂರ್ಣಾಙ್ಕ m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_ಗಂಟು_Module* nodep) {
        // Return corresponding vertex for this module
        ಯದಿ (!nodep->user1p()) nodep->user1p(ನೂತನ LinkCellsVertex{&m_graph, nodep});
        ಪ್ರತ್ಯಾಗಮ nodep->user1u().toGraphVertex();
    }
    ಶೂನ್ಯ newEdge(V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight, ತಾರ್ಕಿಕ cuttable) {
        ಧ್ರುವ V3GraphEdge* ಧ್ರುವ edgep = ನೂತನ V3GraphEdge{&m_graph, fromp, ತುದಿ, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << ತುದಿ->name());
    }
    ಶೂನ್ಯ cellEdge(V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight, ತಾರ್ಕಿಕ cuttable,
                  AstCell* cellp) {
        ಧ್ರುವ V3GraphEdge* ಧ್ರುವ edgep
            = ನೂತನ CellEdge{&m_graph, fromp, ತುದಿ, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << ತುದಿ->name());
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Mod_In_Lib(ಧ್ರುವ string& name, ಧ್ರುವ string& libname, Ast_ಗಂಟು_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        ಯದಿ (!libSymp)
            libSymp = m_mods.rootp()->ಸೇರಿಸು(libname, ನೂತನ VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->ಸೇರಿಸು(name, ನೂತನ VSymEnt{&m_mods, nodep});
    }

    Ast_ಗಂಟು_Module* findModuleLibSym(ಧ್ರುವ string& modName, ಧ್ರುವ string& libname) {
        // Given module name and library name, find within that exact library
        ಧ್ರುವ VSymEnt* ಧ್ರುವ libSymp = m_mods.rootp()->findIdFallback(libname);
        ಯದಿ (!libSymp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = libSymp->findIdFallback(modName);
        ಪ್ರತ್ಯಾಗಮ foundp ? VN_AS(foundp->nodep(), ಗಂಟು_Module) : nullptr;
    }
    Ast_ಗಂಟು_Module* findModuleSym(ಧ್ರುವ string& modName, ಧ್ರುವ string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_ಗಂಟು_Module* foundp;
        ಧ್ರುವ std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        ಧ್ರುವ auto itCellUseList = m_state.m_uselistCell.find(fullName);
        ಯದಿ (itCellUseList != m_state.m_uselistCell.end()) {
            ಪ್ರತಿ (auto ಧ್ರುವ& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                ಯದಿ (foundp) ಪ್ರತ್ಯಾಗಮ foundp;
            }
        }
        // Then search cell-specific library list
        ಧ್ರುವ auto itCellLibList = m_state.m_liblistCell.find(fullName);
        ಯದಿ (itCellLibList != m_state.m_liblistCell.end()) {
            ಪ್ರತಿ (auto ಧ್ರುವ& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                ಯದಿ (foundp) ಪ್ರತ್ಯಾಗಮ foundp;
            }
        }
        // Then search IEEE config default liblist
        ಪ್ರತಿ (auto ಧ್ರುವ& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            ಯದಿ (foundp) ಪ್ರತ್ಯಾಗಮ foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        ಯದಿ (foundp) ಪ್ರತ್ಯಾಗಮ foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        ಪ್ರತ್ಯಾಗಮ foundp;
    }

    Ast_ಗಂಟು_Module* resolveModule(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& modName, ಧ್ರುವ string& libname) {
        Ast_ಗಂಟು_Module* modp = findModuleSym(modName, libname);
        ಯದಿ (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ಧ್ರುವ string prettyName = Ast_ಗಂಟು::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, ಸತ್ಯ, ಅಸತ್ಯ, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            ಯದಿ (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        ಪ್ರತ್ಯಾಗಮ modp;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        ಯದಿ (!v3Global.opt.ತುದಿ_Module().empty()) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ಗಂಟು_Module)) {
            ಯದಿ (!nodep->inLibrary()) ಪ್ರತ್ಯಾಗಮ;
        }
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ಗಂಟು_Module)) {
            nodep->inLibrary(ಅಸತ್ಯ);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, ಧ್ರುವ std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string ತುದಿ;
        ತಾರ್ಕಿಕ ತುದಿ_Found = ಅಸತ್ಯ;
        std::getline(ss, ತುದಿ, '.');
        ಪ್ರತಿ (auto ಧ್ರುವ& pair : m_state.m_designs) {
            ಯದಿ (ತುದಿ == pair.second) {
                ತುದಿ_Found = ಸತ್ಯ;
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (!ತುದಿ_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        ಧ್ರುವ V3GraphVertex* vtx = m_ತುದಿ_Vertexp;
        ಧ್ರುವ CellEdge* finalEdgep = nullptr;
        std::string seg;
        ಯಾವತ್ (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vtx->outEdges()) {
                ಯದಿ (ಧ್ರುವ CellEdge* ಧ್ರುವ cedgep = edge.cast<CellEdge>()) {
                    ಯದಿ (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->ತುದಿ();
                        finalEdgep = cedgep;
                        ಭಙ್ಗ;
                    }
                }
            }
            ಯದಿ (!finalEdgep) ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        ಪ್ರತ್ಯಾಗಮ finalEdgep->cellp();
    }

    ಶೂನ್ಯ assignParamNumbers(AstPin* ಧ್ರುವ firstPinp) {
        ಪ್ರತಿ (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(ಸತ್ಯ);
            ಯದಿ (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_ಅಂಕೆ()));
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_ಗಂಟು_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        ಪ್ರತಿ (auto ಧ್ರುವ& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            ಯದಿ (!cellp) ಅನುವರ್ತ;
            ಪ್ರತಿ (auto ಧ್ರುವ& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                ಯದಿ (modp) {
                    cellp->modp(modp);
                    ಭಙ್ಗ;
                }
            }
        }

        // Search uselists for each instance
        ಪ್ರತಿ (auto ಧ್ರುವ& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            ಪ್ರತಿ (auto ಧ್ರುವ& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                ಯದಿ (modp) {
                    cellp->modp(modp);
                    ಭಙ್ಗ;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ಯದಿ (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (ಧ್ರುವ LinkCellsVertex* ಧ್ರುವ vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_ಗಂಟು_Module* ಧ್ರುವ vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (ಧ್ರುವ LinkCellsVertex* ಧ್ರುವ vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_ಗಂಟು_Module* ಧ್ರುವ vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        ಯದಿ (v3Global.opt.ತುದಿ_Module() != "" && !m_ತುದಿ_Vertexp) {
            VSpellCheck spell;
            ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
                ಯದಿ (ಧ್ರುವ LinkCellsVertex* ಧ್ರುವ vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_ಗಂಟು_Module* ಧ್ರುವ vmodp = vvertexp->modp();
                    ಯದಿ (VN_IS(vmodp, Module)) spell.ತಳ್ಳು_Candidate(vmodp->prettyName());
                }
            }
            ಧ್ರುವ string suggest
                = spell.best_Candidate_ಸಂದೇಶ(Ast_ಗಂಟು::prettyName(v3Global.opt.ತುದಿ_Module()));
            v3error("Specified --top-module '"
                    << Ast_ಗಂಟು::prettyName(v3Global.opt.ತುದಿ_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            ಯದಿ (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, ಅಸತ್ಯ);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            ಯದಿ (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ಧ್ರುವ auto itFoundPair = m_declfnWarned.ಸೇರಿಸು(nodep->fileline()->filename());
                ಯದಿ (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            ಯದಿ (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(ಸತ್ಯ);  // Interfaces can't be at top, unless asked
            }

            auto ಧ್ರುವ& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ಧ್ರುವ ತಾರ್ಕಿಕ ತುದಿ_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            ಯದಿ (ತುದಿ_Match) {
                m_ತುದಿ_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(ಅಸತ್ಯ);  // Safer to make sure it doesn't disappear
            }
            ಯದಿ (v3Global.opt.ತುದಿ_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !ತುದಿ_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                ಯದಿ (!m_libVertexp) m_libVertexp = ನೂತನ LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, ಅಸತ್ಯ);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_ವೃಕ್ಷ();
        }
    }

    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_ಗಂಟು_Module* ಧ್ರುವ modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        ಯದಿ (modp) {
            ಯದಿ (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                ಯದಿ (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, ಅಸತ್ಯ);
                ಯದಿ (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } ಅನ್ಯಥಾ {
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
        ಯದಿ (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    ಶೂನ್ಯ visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ಯದಿ (!nodep->packagep()) {
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ಯದಿ (AstPackage* ಧ್ರುವ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            ಯದಿ (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }
    ಶೂನ್ಯ visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ಯದಿ (!nodep->packagep()) {
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ಯದಿ (AstPackage* ಧ್ರುವ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            ಯದಿ (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, ಅಸತ್ಯ);
    }

    ಶೂನ್ಯ visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_ಗಂಟು_Module* ಧ್ರುವ modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        ಯದಿ (modp) {
            Ast_ಗಂಟು* ಧ್ರುವ cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ಶೂನ್ಯ visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ಧ್ರುವ ತಾರ್ಕಿಕ cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        ಯದಿ (nodep->user1p() == m_modp) ಪ್ರತ್ಯಾಗಮ;  // AstBind and AstNodeModule may call a cell twice
        ಯದಿ (nodep->modName() == m_orig_ತುದಿ_Module_Name) {
            ಯದಿ (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } ಅನ್ಯಥಾ {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        nodep->user1p(m_modp);
        //
        ಯದಿ (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_ಗಂಟು_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            ಯದಿ (cellmodp) {
                ಯದಿ (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(ಸತ್ಯ);
                    nodep->recursive(ಸತ್ಯ);
                    ಯದಿ (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_ಗಂಟು_Module* otherModp = VN_CAST(cellmodp->user2p(), ಗಂಟು_Module);
                        ಯದಿ (!otherModp) {
                            otherModp = cellmodp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(ಸತ್ಯ);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, ಅಸತ್ಯ);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } ಅನ್ಯಥಾ {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } ಅನ್ಯಥಾ {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, ಅಸತ್ಯ, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        ಯದಿ (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ತಳ್ಳು_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        ಯದಿ (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ತಳ್ಳು_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        ತಾರ್ಕಿಕ pinStar = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ pinDotName = ಅಸತ್ಯ;
        ಪ್ರತಿ (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            ಯದಿ (pinp->svDotName()) pinDotName = ಸತ್ಯ;
            ಯದಿ (pinp->dotStar()) {
                ಯದಿ (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = ಸತ್ಯ;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_ವೃಕ್ಷ(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        ಪ್ರತಿ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಯದಿ (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_ಅಂಕೆ()));
        }
        assignParamNumbers(nodep->paramsp());
        ಯದಿ (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            ಪ್ರತಿ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                ಯದಿ (!pinp->exprp()) {
                    ಯದಿ (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } ಅನ್ಯಥಾ {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.ಸೇರಿಸು(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            ಪ್ರತಿ (Ast_ಗಂಟು* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                ಯದಿ (ಧ್ರುವ AstPort* ಧ್ರುವ portp = VN_CAST(portnodep, Port)) {

                    ಯದಿ (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_ಅಂಕೆ())) == ports.end()) {
                        ಯದಿ (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ಧ್ರುವ newp
                                = ನೂತನ AstPin{nodep->fileline(), 0, portp->name(),
                                             ನೂತನ AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(ಸತ್ಯ);
                            newp->svImplicit(ಸತ್ಯ);
                            nodep->addPinsp(newp);
                        } ಅನ್ಯಥಾ {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ಧ್ರುವ AstVar* portp_varp = nullptr;
                            ಪ್ರತಿ (Ast_ಗಂಟು* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(module_stmtsp, Var)) {
                                    ಯದಿ (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        ಭಙ್ಗ;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            ಯದಿ (portp_varp && portp_varp->is_ಒಳಹರಿವು() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ಧ್ರುವ newp
                                    = ನೂತನ AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } ಅನ್ಯಥಾ {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ಧ್ರುವ newp
                                    = ನೂತನ AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        ಯದಿ (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            ಯದಿ (!nodep->hasIfaceVar()) {
                ಧ್ರುವ string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ಧ್ರುವ idtypep = ನೂತನ AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                ಯದಿ (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    ಪ್ರತಿ (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ತಳ್ಳು_back(rp);
                    }
                    Ast_ಗಂಟು_DType* dtp = idtypep;
                    ಪ್ರತಿ (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = ನೂತನ AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
                    }
                    varp = ನೂತನ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } ಅನ್ಯಥಾ {
                    varp = ನೂತನ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(ಸತ್ಯ);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(ಸತ್ಯ);
            }
        }
        ಯದಿ (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        ಯದಿ (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_ಗಂಟು_Module* ಧ್ರುವ varModp = findModuleSym(nodep->name(), m_modp->libname());
            ಯದಿ (AstIface* ಧ್ರುವ ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, ಅಸತ್ಯ);
            }
        }
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    ಶೂನ್ಯ visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // METHODS
    ಶೂನ್ಯ readModNames() {
        // mangled_name, BlockOptions
        ಧ್ರುವ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ಧ್ರುವ auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.ತುದಿ_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        ಪ್ರತಿ (Ast_ಗಂಟು_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ಗಂಟು_Module);
            ಯದಿ (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            ಯದಿ (libFoundp && libFoundp == nodep) {
                // Ok
            } ಅನ್ಯಥಾ ಯದಿ (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } ಅನ್ಯಥಾ ಯದಿ (libFoundp) {
                ಯದಿ (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ಧ್ರುವ string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                ಸೇರಿಸು_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                ಸೇರಿಸು_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } ಅನ್ಯಥಾ {
                ಸೇರಿಸು_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                ಸೇರಿಸು_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        ಯದಿ (v3Global.opt.hierChild()) {
            ಧ್ರುವ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.ತುದಿ_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            ಧ್ರುವ V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.ತುದಿ_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_ತುದಿ_Module_Name = hierIt->second.origName();
        } ಅನ್ಯಥಾ {
            m_orig_ತುದಿ_Module_Name = v3Global.opt.ತುದಿ_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        ಯದಿ (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

ಶೂನ್ಯ V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
