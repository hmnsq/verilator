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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkCells.h"

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Parse.h"
#ଅନ୍ତର୍ଭାଵ "V3SymTable.h"

#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

ଶ୍ରେଣୀ LinkCellsGraph final : ସର୍ବସାଧାରଣ V3Graph {
ସର୍ବସାଧାରଣ:
    LinkCellsGraph() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~LinkCellsGraph() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

ଶ୍ରେଣୀ LinkCellsVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;

ସର୍ବସାଧାରଣ:
    LinkCellsVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Ast_ଗଣ୍ଠି_Module* modp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_modp; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ cvtToHex(modp()) + ' ' + modp()->name(); }
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ଅଚଳ override {
        ଫେରନ୍ତୁ m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

ଶ୍ରେଣୀ LibraryVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
ସର୍ବସାଧାରଣ:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "*LIBRARY*"; }
};

ଶ୍ରେଣୀ CellEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* ଅଚଳ m_cellp;

ସର୍ବସାଧାରଣ:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight, ତାର୍କିକ cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ଅଚଳ { ଫେରନ୍ତୁ m_cellp; }
    string name() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

ଶୂନ୍ୟ LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    ଯଦି (ଅଚଳ LinkCellsVertex* ଅଚଳ vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } ଅନ୍ୟଥା {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
ସଂରଚନା LinkCellsState final {
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

ଶ୍ରେଣୀ LinkConfigsVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    ତାର୍କିକ m_is_ଶୀର୍ଷ = ମିଥ୍ୟା;  // Whether we're in the top-level config
    ତାର୍କିକ m_isDefault = ମିଥ୍ୟା;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    ଶୂନ୍ୟ visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_ଶୀର୍ଷ);
        ଅଚଳ auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_ଶୀର୍ଷ = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ଶୂନ୍ୟ visit(AstConfigCell* nodep) override {
        ଯଦି (m_is_ଶୀର୍ଷ) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    ଶୂନ୍ୟ visit(AstConfigRule* nodep) override {
        ଯଦି (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = ସତ୍ୟ;
            iterateAndNextNull(nodep->usep());
        } ଅନ୍ୟଥା ଯଦି (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } ଅନ୍ୟଥା {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    ଶୂନ୍ୟ visit(AstDot* nodep) override { iterateChildren(nodep); }

    ଶୂନ୍ୟ visit(AstParseRef* nodep) override {
        ଯଦି (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } ଅନ୍ୟଥା ଯଦି (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } ଅନ୍ୟଥା ଯଦି (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } ଅନ୍ୟଥା ଯଦି (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    ଶୂନ୍ୟ visit(AstConfigUse* nodep) override {
        ଯଦି (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } ଅନ୍ୟଥା ଯଦି (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } ଅନ୍ୟଥା ଯଦି (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } ଅନ୍ୟଥା ଯଦି (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        ଯଦି (!m_state.m_designs.size() && !v3Global.opt.ଶୀର୍ଷ_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.ଶୀର୍ଷ_Module());
        }
        iterate(nodep);
    }
};

ଶ୍ରେଣୀ LinkCellsVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE
    VInFilter* ଅଚଳ m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    ଅଚଳ V3GraphVertex* m_ଶୀର୍ଷ_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_ଶୀର୍ଷ_Module_Name;  // original name of the top module
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_ଗଣ୍ଠି_Module* nodep) {
        // Return corresponding vertex for this module
        ଯଦି (!nodep->user1p()) nodep->user1p(ନୂତନ LinkCellsVertex{&m_graph, nodep});
        ଫେରନ୍ତୁ nodep->user1u().toGraphVertex();
    }
    ଶୂନ୍ୟ newEdge(V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight, ତାର୍କିକ cuttable) {
        ଅଚଳ V3GraphEdge* ଅଚଳ edgep = ନୂତନ V3GraphEdge{&m_graph, fromp, ଶୀର୍ଷ, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << ଶୀର୍ଷ->name());
    }
    ଶୂନ୍ୟ cellEdge(V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight, ତାର୍କିକ cuttable,
                  AstCell* cellp) {
        ଅଚଳ V3GraphEdge* ଅଚଳ edgep
            = ନୂତନ CellEdge{&m_graph, fromp, ଶୀର୍ଷ, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << ଶୀର୍ଷ->name());
    }
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Mod_In_Lib(ଅଚଳ string& name, ଅଚଳ string& libname, Ast_ଗଣ୍ଠି_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        ଯଦି (!libSymp)
            libSymp = m_mods.rootp()->ଭର୍ତ୍ତି(libname, ନୂତନ VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->ଭର୍ତ୍ତି(name, ନୂତନ VSymEnt{&m_mods, nodep});
    }

    Ast_ଗଣ୍ଠି_Module* findModuleLibSym(ଅଚଳ string& modName, ଅଚଳ string& libname) {
        // Given module name and library name, find within that exact library
        ଅଚଳ VSymEnt* ଅଚଳ libSymp = m_mods.rootp()->findIdFallback(libname);
        ଯଦି (!libSymp) ଫେରନ୍ତୁ nullptr;
        ଅଚଳ VSymEnt* ଅଚଳ foundp = libSymp->findIdFallback(modName);
        ଫେରନ୍ତୁ foundp ? VN_AS(foundp->nodep(), ଗଣ୍ଠି_Module) : nullptr;
    }
    Ast_ଗଣ୍ଠି_Module* findModuleSym(ଅଚଳ string& modName, ଅଚଳ string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_ଗଣ୍ଠି_Module* foundp;
        ଅଚଳ std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        ଅଚଳ auto itCellUseList = m_state.m_uselistCell.find(fullName);
        ଯଦି (itCellUseList != m_state.m_uselistCell.end()) {
            ପାଇଁ (auto ଅଚଳ& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                ଯଦି (foundp) ଫେରନ୍ତୁ foundp;
            }
        }
        // Then search cell-specific library list
        ଅଚଳ auto itCellLibList = m_state.m_liblistCell.find(fullName);
        ଯଦି (itCellLibList != m_state.m_liblistCell.end()) {
            ପାଇଁ (auto ଅଚଳ& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                ଯଦି (foundp) ଫେରନ୍ତୁ foundp;
            }
        }
        // Then search IEEE config default liblist
        ପାଇଁ (auto ଅଚଳ& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            ଯଦି (foundp) ଫେରନ୍ତୁ foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        ଯଦି (foundp) ଫେରନ୍ତୁ foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        ଫେରନ୍ତୁ foundp;
    }

    Ast_ଗଣ୍ଠି_Module* resolveModule(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& modName, ଅଚଳ string& libname) {
        Ast_ଗଣ୍ଠି_Module* modp = findModuleSym(modName, libname);
        ଯଦି (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ଅଚଳ string prettyName = Ast_ଗଣ୍ଠି::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, ସତ୍ୟ, ମିଥ୍ୟା, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            ଯଦି (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        ଫେରନ୍ତୁ modp;
    }

    ସ୍ଥିର ଶୂନ୍ୟ removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        ଯଦି (!v3Global.opt.ଶୀର୍ଷ_Module().empty()) ଫେରନ୍ତୁ;
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Module)) {
            ଯଦି (!nodep->inLibrary()) ଫେରନ୍ତୁ;
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Module)) {
            nodep->inLibrary(ମିଥ୍ୟା);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, ଅଚଳ std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string ଶୀର୍ଷ;
        ତାର୍କିକ ଶୀର୍ଷ_Found = ମିଥ୍ୟା;
        std::getline(ss, ଶୀର୍ଷ, '.');
        ପାଇଁ (auto ଅଚଳ& pair : m_state.m_designs) {
            ଯଦି (ଶୀର୍ଷ == pair.second) {
                ଶୀର୍ଷ_Found = ସତ୍ୟ;
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (!ଶୀର୍ଷ_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            ଫେରନ୍ତୁ nullptr;
        }

        ଅଚଳ V3GraphVertex* vtx = m_ଶୀର୍ଷ_Vertexp;
        ଅଚଳ CellEdge* finalEdgep = nullptr;
        std::string seg;
        ଯେତେବେଳେ (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vtx->outEdges()) {
                ଯଦି (ଅଚଳ CellEdge* ଅଚଳ cedgep = edge.cast<CellEdge>()) {
                    ଯଦି (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->ଶୀର୍ଷ();
                        finalEdgep = cedgep;
                        ଭାଙ୍ଗ;
                    }
                }
            }
            ଯଦି (!finalEdgep) ଫେରନ୍ତୁ nullptr;
        }

        ଫେରନ୍ତୁ finalEdgep->cellp();
    }

    ଶୂନ୍ୟ assignParamNumbers(AstPin* ଅଚଳ firstPinp) {
        ପାଇଁ (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(ସତ୍ୟ);
            ଯଦି (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_ଅଙ୍କ()));
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_ଗଣ୍ଠି_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        ପାଇଁ (auto ଅଚଳ& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            ଯଦି (!cellp) ଜାରି;
            ପାଇଁ (auto ଅଚଳ& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                ଯଦି (modp) {
                    cellp->modp(modp);
                    ଭାଙ୍ଗ;
                }
            }
        }

        // Search uselists for each instance
        ପାଇଁ (auto ଅଚଳ& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            ପାଇଁ (auto ଅଚଳ& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                ଯଦି (modp) {
                    cellp->modp(modp);
                    ଭାଙ୍ଗ;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ଯଦି (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (ଅଚଳ LinkCellsVertex* ଅଚଳ vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (ଅଚଳ LinkCellsVertex* ଅଚଳ vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        ଯଦି (v3Global.opt.ଶୀର୍ଷ_Module() != "" && !m_ଶୀର୍ଷ_Vertexp) {
            VSpellCheck spell;
            ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
                ଯଦି (ଅଚଳ LinkCellsVertex* ଅଚଳ vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_ଗଣ୍ଠି_Module* ଅଚଳ vmodp = vvertexp->modp();
                    ଯଦି (VN_IS(vmodp, Module)) spell.ଠେଲ_Candidate(vmodp->prettyName());
                }
            }
            ଅଚଳ string suggest
                = spell.best_Candidate_ସନ୍ଦେଶ(Ast_ଗଣ୍ଠି::prettyName(v3Global.opt.ଶୀର୍ଷ_Module()));
            v3error("Specified --top-module '"
                    << Ast_ଗଣ୍ଠି::prettyName(v3Global.opt.ଶୀର୍ଷ_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            ଯଦି (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, ମିଥ୍ୟା);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            ଯଦି (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ଅଚଳ auto itFoundPair = m_declfnWarned.ଭର୍ତ୍ତି(nodep->fileline()->filename());
                ଯଦି (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            ଯଦି (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(ସତ୍ୟ);  // Interfaces can't be at top, unless asked
            }

            auto ଅଚଳ& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ଅଚଳ ତାର୍କିକ ଶୀର୍ଷ_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            ଯଦି (ଶୀର୍ଷ_Match) {
                m_ଶୀର୍ଷ_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(ମିଥ୍ୟା);  // Safer to make sure it doesn't disappear
            }
            ଯଦି (v3Global.opt.ଶୀର୍ଷ_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !ଶୀର୍ଷ_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                ଯଦି (!m_libVertexp) m_libVertexp = ନୂତନ LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, ମିଥ୍ୟା);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_ବୃକ୍ଷ();
        }
    }

    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        ଯଦି (modp) {
            ଯଦି (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                ଯଦି (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, ମିଥ୍ୟା);
                ଯଦି (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } ଅନ୍ୟଥା ଯଦି (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } ଅନ୍ୟଥା {
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
        ଯଦି (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    ଶୂନ୍ୟ visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ଯଦି (!nodep->packagep()) {
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ଯଦି (AstPackage* ଅଚଳ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            ଯଦି (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                ଫେରନ୍ତୁ;
            }
        }
    }
    ଶୂନ୍ୟ visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ଯଦି (!nodep->packagep()) {
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ଯଦି (AstPackage* ଅଚଳ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            ଯଦି (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                ଫେରନ୍ତୁ;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, ମିଥ୍ୟା);
    }

    ଶୂନ୍ୟ visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        ଯଦି (modp) {
            Ast_ଗଣ୍ଠି* ଅଚଳ cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ଅଚଳ ତାର୍କିକ cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        ଯଦି (nodep->user1p() == m_modp) ଫେରନ୍ତୁ;  // AstBind and AstNodeModule may call a cell twice
        ଯଦି (nodep->modName() == m_orig_ଶୀର୍ଷ_Module_Name) {
            ଯଦି (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } ଅନ୍ୟଥା {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
        }
        nodep->user1p(m_modp);
        //
        ଯଦି (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_ଗଣ୍ଠି_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            ଯଦି (cellmodp) {
                ଯଦି (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(ସତ୍ୟ);
                    nodep->recursive(ସତ୍ୟ);
                    ଯଦି (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_ଗଣ୍ଠି_Module* otherModp = VN_CAST(cellmodp->user2p(), ଗଣ୍ଠି_Module);
                        ଯଦି (!otherModp) {
                            otherModp = cellmodp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(ସତ୍ୟ);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, ମିଥ୍ୟା);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } ଅନ୍ୟଥା {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } ଅନ୍ୟଥା {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, ମିଥ୍ୟା, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        ଯଦି (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ଠେଲ_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        ଯଦି (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ଠେଲ_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        ତାର୍କିକ pinStar = ମିଥ୍ୟା;
        ତାର୍କିକ pinDotName = ମିଥ୍ୟା;
        ପାଇଁ (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            ଯଦି (pinp->svDotName()) pinDotName = ସତ୍ୟ;
            ଯଦି (pinp->dotStar()) {
                ଯଦି (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = ସତ୍ୟ;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_ବୃକ୍ଷ(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        ପାଇଁ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଯଦି (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_ଅଙ୍କ()));
        }
        assignParamNumbers(nodep->paramsp());
        ଯଦି (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            ପାଇଁ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                ଯଦି (!pinp->exprp()) {
                    ଯଦି (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } ଅନ୍ୟଥା {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.ଭର୍ତ୍ତି(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            ପାଇଁ (Ast_ଗଣ୍ଠି* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                ଯଦି (ଅଚଳ AstPort* ଅଚଳ portp = VN_CAST(portnodep, Port)) {

                    ଯଦି (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_ଅଙ୍କ())) == ports.end()) {
                        ଯଦି (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ଅଚଳ newp
                                = ନୂତନ AstPin{nodep->fileline(), 0, portp->name(),
                                             ନୂତନ AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(ସତ୍ୟ);
                            newp->svImplicit(ସତ୍ୟ);
                            nodep->addPinsp(newp);
                        } ଅନ୍ୟଥା {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ଅଚଳ AstVar* portp_varp = nullptr;
                            ପାଇଁ (Ast_ଗଣ୍ଠି* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(module_stmtsp, Var)) {
                                    ଯଦି (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        ଭାଙ୍ଗ;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            ଯଦି (portp_varp && portp_varp->is_ନିବେଶ() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ଅଚଳ newp
                                    = ନୂତନ AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } ଅନ୍ୟଥା {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ଅଚଳ newp
                                    = ନୂତନ AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        ଯଦି (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            ଯଦି (!nodep->hasIfaceVar()) {
                ଅଚଳ string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ଅଚଳ idtypep = ନୂତନ AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                ଯଦି (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    ପାଇଁ (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ଠେଲ_back(rp);
                    }
                    Ast_ଗଣ୍ଠି_DType* dtp = idtypep;
                    ପାଇଁ (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = ନୂତନ AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
                    }
                    varp = ନୂତନ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } ଅନ୍ୟଥା {
                    varp = ନୂତନ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(ସତ୍ୟ);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(ସତ୍ୟ);
            }
        }
        ଯଦି (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        ଯଦି (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ varModp = findModuleSym(nodep->name(), m_modp->libname());
            ଯଦି (AstIface* ଅଚଳ ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, ମିଥ୍ୟା);
            }
        }
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    // METHODS
    ଶୂନ୍ୟ readModNames() {
        // mangled_name, BlockOptions
        ଅଚଳ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ଅଚଳ auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.ଶୀର୍ଷ_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Module);
            ଯଦି (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            ଯଦି (libFoundp && libFoundp == nodep) {
                // Ok
            } ଅନ୍ୟଥା ଯଦି (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } ଅନ୍ୟଥା ଯଦି (libFoundp) {
                ଯଦି (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ଅଚଳ string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                ଭର୍ତ୍ତି_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                ଭର୍ତ୍ତି_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } ଅନ୍ୟଥା {
                ଭର୍ତ୍ତି_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                ଭର୍ତ୍ତି_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        ଯଦି (v3Global.opt.hierChild()) {
            ଅଚଳ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.ଶୀର୍ଷ_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            ଅଚଳ V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.ଶୀର୍ଷ_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_ଶୀର୍ଷ_Module_Name = hierIt->second.origName();
        } ଅନ୍ୟଥା {
            m_orig_ଶୀର୍ଷ_Module_Name = v3Global.opt.ଶୀର୍ଷ_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        ଯଦି (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

ଶୂନ୍ୟ V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
