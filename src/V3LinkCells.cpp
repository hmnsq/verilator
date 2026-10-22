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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkCells.h"

#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Parse.h"
#അന്തര്ഭാവ "V3SymTable.h"

#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

വർഗ്ഗം LinkCellsGraph final : പൊതു V3Graph {
പൊതു:
    LinkCellsGraph() = സ്ഥിരസ്ഥിതി;
    ~LinkCellsGraph() override = സ്ഥിരസ്ഥിതി;
    ശൂന്യം loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

വർഗ്ഗം LinkCellsVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;

പൊതു:
    LinkCellsVertex(V3Graph* graphp, Ast_കണ്ണി_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = സ്ഥിരസ്ഥിതി;
    Ast_കണ്ണി_Module* modp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_modp; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ cvtToHex(modp()) + ' ' + modp()->name(); }
    File_വരി* fileline() സ്ഥിരം override { തിരികെ modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() സ്ഥിരം override {
        തിരികെ m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

വർഗ്ഗം LibraryVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
പൊതു:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = സ്ഥിരസ്ഥിതി;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ "*LIBRARY*"; }
};

വർഗ്ഗം CellEdge final : പൊതു V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* സ്ഥിരം m_cellp;

പൊതു:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight, തർക്കം cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, മുകൾ, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() സ്ഥിരം { തിരികെ m_cellp; }
    string name() സ്ഥിരം override VL_MT_STABLE {
        തിരികെ cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

ശൂന്യം LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    എങ്കിൽ (സ്ഥിരം LinkCellsVertex* സ്ഥിരം vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } അല്ലെങ്കിൽ {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
ഘടന LinkCellsState final {
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

വർഗ്ഗം LinkConfigsVisitor final : പൊതു VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    തർക്കം m_is_മുകൾ = അസത്യം;  // Whether we're in the top-level config
    തർക്കം m_isDefault = അസത്യം;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    ശൂന്യം visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_മുകൾ);
        സ്ഥിരം auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_മുകൾ = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ശൂന്യം visit(AstConfigCell* nodep) override {
        എങ്കിൽ (m_is_മുകൾ) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    ശൂന്യം visit(AstConfigRule* nodep) override {
        എങ്കിൽ (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = സത്യം;
            iterateAndNextNull(nodep->usep());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } അല്ലെങ്കിൽ {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    ശൂന്യം visit(AstDot* nodep) override { iterateChildren(nodep); }

    ശൂന്യം visit(AstParseRef* nodep) override {
        എങ്കിൽ (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    ശൂന്യം visit(AstConfigUse* nodep) override {
        എങ്കിൽ (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } അല്ലെങ്കിൽ എങ്കിൽ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        എങ്കിൽ (!m_state.m_designs.size() && !v3Global.opt.മുകൾ_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.മുകൾ_Module());
        }
        iterate(nodep);
    }
};

വർഗ്ഗം LinkCellsVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE
    VInFilter* സ്ഥിരം m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    സ്ഥിരം V3GraphVertex* m_മുകൾ_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_മുകൾ_Module_Name;  // original name of the top module
    പൂർണ്ണസംഖ്യ m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_കണ്ണി_Module* nodep) {
        // Return corresponding vertex for this module
        എങ്കിൽ (!nodep->user1p()) nodep->user1p(പുതിയ LinkCellsVertex{&m_graph, nodep});
        തിരികെ nodep->user1u().toGraphVertex();
    }
    ശൂന്യം newEdge(V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight, തർക്കം cuttable) {
        സ്ഥിരം V3GraphEdge* സ്ഥിരം edgep = പുതിയ V3GraphEdge{&m_graph, fromp, മുകൾ, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << മുകൾ->name());
    }
    ശൂന്യം cellEdge(V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight, തർക്കം cuttable,
                  AstCell* cellp) {
        സ്ഥിരം V3GraphEdge* സ്ഥിരം edgep
            = പുതിയ CellEdge{&m_graph, fromp, മുകൾ, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << മുകൾ->name());
    }
    ശൂന്യം തിരുകുക_Mod_In_Lib(സ്ഥിരം string& name, സ്ഥിരം string& libname, Ast_കണ്ണി_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        എങ്കിൽ (!libSymp)
            libSymp = m_mods.rootp()->തിരുകുക(libname, പുതിയ VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->തിരുകുക(name, പുതിയ VSymEnt{&m_mods, nodep});
    }

    Ast_കണ്ണി_Module* findModuleLibSym(സ്ഥിരം string& modName, സ്ഥിരം string& libname) {
        // Given module name and library name, find within that exact library
        സ്ഥിരം VSymEnt* സ്ഥിരം libSymp = m_mods.rootp()->findIdFallback(libname);
        എങ്കിൽ (!libSymp) തിരികെ nullptr;
        സ്ഥിരം VSymEnt* സ്ഥിരം foundp = libSymp->findIdFallback(modName);
        തിരികെ foundp ? VN_AS(foundp->nodep(), കണ്ണി_Module) : nullptr;
    }
    Ast_കണ്ണി_Module* findModuleSym(സ്ഥിരം string& modName, സ്ഥിരം string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_കണ്ണി_Module* foundp;
        സ്ഥിരം std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        സ്ഥിരം auto itCellUseList = m_state.m_uselistCell.find(fullName);
        എങ്കിൽ (itCellUseList != m_state.m_uselistCell.end()) {
            വേണ്ടി (auto സ്ഥിരം& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                എങ്കിൽ (foundp) തിരികെ foundp;
            }
        }
        // Then search cell-specific library list
        സ്ഥിരം auto itCellLibList = m_state.m_liblistCell.find(fullName);
        എങ്കിൽ (itCellLibList != m_state.m_liblistCell.end()) {
            വേണ്ടി (auto സ്ഥിരം& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                എങ്കിൽ (foundp) തിരികെ foundp;
            }
        }
        // Then search IEEE config default liblist
        വേണ്ടി (auto സ്ഥിരം& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            എങ്കിൽ (foundp) തിരികെ foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        എങ്കിൽ (foundp) തിരികെ foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        തിരികെ foundp;
    }

    Ast_കണ്ണി_Module* resolveModule(Ast_കണ്ണി* nodep, സ്ഥിരം string& modName, സ്ഥിരം string& libname) {
        Ast_കണ്ണി_Module* modp = findModuleSym(modName, libname);
        എങ്കിൽ (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            സ്ഥിരം string prettyName = Ast_കണ്ണി::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, സത്യം, അസത്യം, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            എങ്കിൽ (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        തിരികെ modp;
    }

    സ്ഥിതം ശൂന്യം removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        എങ്കിൽ (!v3Global.opt.മുകൾ_Module().empty()) തിരികെ;
        വേണ്ടി (Ast_കണ്ണി_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), കണ്ണി_Module)) {
            എങ്കിൽ (!nodep->inLibrary()) തിരികെ;
        }
        വേണ്ടി (Ast_കണ്ണി_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), കണ്ണി_Module)) {
            nodep->inLibrary(അസത്യം);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, സ്ഥിരം std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string മുകൾ;
        തർക്കം മുകൾ_Found = അസത്യം;
        std::getline(ss, മുകൾ, '.');
        വേണ്ടി (auto സ്ഥിരം& pair : m_state.m_designs) {
            എങ്കിൽ (മുകൾ == pair.second) {
                മുകൾ_Found = സത്യം;
                നിർത്തുക;
            }
        }
        എങ്കിൽ (!മുകൾ_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            തിരികെ nullptr;
        }

        സ്ഥിരം V3GraphVertex* vtx = m_മുകൾ_Vertexp;
        സ്ഥിരം CellEdge* finalEdgep = nullptr;
        std::string seg;
        ആകുമ്പോൾ (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vtx->outEdges()) {
                എങ്കിൽ (സ്ഥിരം CellEdge* സ്ഥിരം cedgep = edge.cast<CellEdge>()) {
                    എങ്കിൽ (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->മുകൾ();
                        finalEdgep = cedgep;
                        നിർത്തുക;
                    }
                }
            }
            എങ്കിൽ (!finalEdgep) തിരികെ nullptr;
        }

        തിരികെ finalEdgep->cellp();
    }

    ശൂന്യം assignParamNumbers(AstPin* സ്ഥിരം firstPinp) {
        വേണ്ടി (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(സത്യം);
            എങ്കിൽ (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_അക്കം()));
        }
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_കണ്ണി_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        വേണ്ടി (auto സ്ഥിരം& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            എങ്കിൽ (!cellp) തുടരുക;
            വേണ്ടി (auto സ്ഥിരം& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                എങ്കിൽ (modp) {
                    cellp->modp(modp);
                    നിർത്തുക;
                }
            }
        }

        // Search uselists for each instance
        വേണ്ടി (auto സ്ഥിരം& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            വേണ്ടി (auto സ്ഥിരം& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                എങ്കിൽ (modp) {
                    cellp->modp(modp);
                    നിർത്തുക;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        എങ്കിൽ (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (സ്ഥിരം LinkCellsVertex* സ്ഥിരം vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_കണ്ണി_Module* സ്ഥിരം vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (സ്ഥിരം LinkCellsVertex* സ്ഥിരം vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_കണ്ണി_Module* സ്ഥിരം vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        എങ്കിൽ (v3Global.opt.മുകൾ_Module() != "" && !m_മുകൾ_Vertexp) {
            VSpellCheck spell;
            വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
                എങ്കിൽ (സ്ഥിരം LinkCellsVertex* സ്ഥിരം vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_കണ്ണി_Module* സ്ഥിരം vmodp = vvertexp->modp();
                    എങ്കിൽ (VN_IS(vmodp, Module)) spell.തള്ളുക_Candidate(vmodp->prettyName());
                }
            }
            സ്ഥിരം string suggest
                = spell.best_Candidate_സന്ദേശം(Ast_കണ്ണി::prettyName(v3Global.opt.മുകൾ_Module()));
            v3error("Specified --top-module '"
                    << Ast_കണ്ണി::prettyName(v3Global.opt.മുകൾ_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            എങ്കിൽ (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, അസത്യം);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            എങ്കിൽ (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                സ്ഥിരം auto itFoundPair = m_declfnWarned.തിരുകുക(nodep->fileline()->filename());
                എങ്കിൽ (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            എങ്കിൽ (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(സത്യം);  // Interfaces can't be at top, unless asked
            }

            auto സ്ഥിരം& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            സ്ഥിരം തർക്കം മുകൾ_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            എങ്കിൽ (മുകൾ_Match) {
                m_മുകൾ_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(അസത്യം);  // Safer to make sure it doesn't disappear
            }
            എങ്കിൽ (v3Global.opt.മുകൾ_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !മുകൾ_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                എങ്കിൽ (!m_libVertexp) m_libVertexp = പുതിയ LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, അസത്യം);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_വൃക്ഷം();
        }
    }

    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_കണ്ണി_Module* സ്ഥിരം modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        എങ്കിൽ (modp) {
            എങ്കിൽ (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                എങ്കിൽ (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, അസത്യം);
                എങ്കിൽ (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } അല്ലെങ്കിൽ {
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
        എങ്കിൽ (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    ശൂന്യം visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->packagep()) {
            Ast_കണ്ണി_Module* സ്ഥിരം modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            എങ്കിൽ (AstPackage* സ്ഥിരം pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            എങ്കിൽ (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                തിരികെ;
            }
        }
    }
    ശൂന്യം visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->packagep()) {
            Ast_കണ്ണി_Module* സ്ഥിരം modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            എങ്കിൽ (AstPackage* സ്ഥിരം pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            എങ്കിൽ (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                തിരികെ;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, അസത്യം);
    }

    ശൂന്യം visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_കണ്ണി_Module* സ്ഥിരം modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        എങ്കിൽ (modp) {
            Ast_കണ്ണി* സ്ഥിരം cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ശൂന്യം visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        സ്ഥിരം തർക്കം cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        എങ്കിൽ (nodep->user1p() == m_modp) തിരികെ;  // AstBind and AstNodeModule may call a cell twice
        എങ്കിൽ (nodep->modName() == m_orig_മുകൾ_Module_Name) {
            എങ്കിൽ (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } അല്ലെങ്കിൽ {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
        }
        nodep->user1p(m_modp);
        //
        എങ്കിൽ (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_കണ്ണി_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            എങ്കിൽ (cellmodp) {
                എങ്കിൽ (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(സത്യം);
                    nodep->recursive(സത്യം);
                    എങ്കിൽ (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_കണ്ണി_Module* otherModp = VN_CAST(cellmodp->user2p(), കണ്ണി_Module);
                        എങ്കിൽ (!otherModp) {
                            otherModp = cellmodp->clone_വൃക്ഷം(അസത്യം);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(സത്യം);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, അസത്യം);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } അല്ലെങ്കിൽ {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } അല്ലെങ്കിൽ {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, അസത്യം, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        എങ്കിൽ (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            തള്ളുക_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        എങ്കിൽ (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            തള്ളുക_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        തർക്കം pinStar = അസത്യം;
        തർക്കം pinDotName = അസത്യം;
        വേണ്ടി (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            എങ്കിൽ (pinp->svDotName()) pinDotName = സത്യം;
            എങ്കിൽ (pinp->dotStar()) {
                എങ്കിൽ (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = സത്യം;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_വൃക്ഷം(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        വേണ്ടി (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            എങ്കിൽ (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_അക്കം()));
        }
        assignParamNumbers(nodep->paramsp());
        എങ്കിൽ (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            വേണ്ടി (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                എങ്കിൽ (!pinp->exprp()) {
                    എങ്കിൽ (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } അല്ലെങ്കിൽ {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.തിരുകുക(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            വേണ്ടി (Ast_കണ്ണി* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                എങ്കിൽ (സ്ഥിരം AstPort* സ്ഥിരം portp = VN_CAST(portnodep, Port)) {

                    എങ്കിൽ (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_അക്കം())) == ports.end()) {
                        എങ്കിൽ (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* സ്ഥിരം newp
                                = പുതിയ AstPin{nodep->fileline(), 0, portp->name(),
                                             പുതിയ AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(സത്യം);
                            newp->svImplicit(സത്യം);
                            nodep->addPinsp(newp);
                        } അല്ലെങ്കിൽ {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            സ്ഥിരം AstVar* portp_varp = nullptr;
                            വേണ്ടി (Ast_കണ്ണി* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(module_stmtsp, Var)) {
                                    എങ്കിൽ (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        നിർത്തുക;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            എങ്കിൽ (portp_varp && portp_varp->is_നിവേശം() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* സ്ഥിരം newp
                                    = പുതിയ AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } അല്ലെങ്കിൽ {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* സ്ഥിരം newp
                                    = പുതിയ AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        എങ്കിൽ (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            എങ്കിൽ (!nodep->hasIfaceVar()) {
                സ്ഥിരം string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* സ്ഥിരം idtypep = പുതിയ AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                എങ്കിൽ (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    വേണ്ടി (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.തള്ളുക_back(rp);
                    }
                    Ast_കണ്ണി_DType* dtp = idtypep;
                    വേണ്ടി (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = പുതിയ AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_വൃക്ഷം(അസത്യം)};
                    }
                    varp = പുതിയ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } അല്ലെങ്കിൽ {
                    varp = പുതിയ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(സത്യം);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(സത്യം);
            }
        }
        എങ്കിൽ (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    ശൂന്യം visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        എങ്കിൽ (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_കണ്ണി_Module* സ്ഥിരം varModp = findModuleSym(nodep->name(), m_modp->libname());
            എങ്കിൽ (AstIface* സ്ഥിരം ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, അസത്യം);
            }
        }
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    ശൂന്യം visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    // METHODS
    ശൂന്യം readModNames() {
        // mangled_name, BlockOptions
        സ്ഥിരം V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        സ്ഥിരം auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.മുകൾ_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        വേണ്ടി (Ast_കണ്ണി_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), കണ്ണി_Module);
            എങ്കിൽ (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            എങ്കിൽ (libFoundp && libFoundp == nodep) {
                // Ok
            } അല്ലെങ്കിൽ എങ്കിൽ (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } അല്ലെങ്കിൽ എങ്കിൽ (libFoundp) {
                എങ്കിൽ (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                സ്ഥിരം string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                തിരുകുക_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                തിരുകുക_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } അല്ലെങ്കിൽ {
                തിരുകുക_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                തിരുകുക_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

പൊതു:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        എങ്കിൽ (v3Global.opt.hierChild()) {
            സ്ഥിരം V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.മുകൾ_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            സ്ഥിരം V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.മുകൾ_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_മുകൾ_Module_Name = hierIt->second.origName();
        } അല്ലെങ്കിൽ {
            m_orig_മുകൾ_Module_Name = v3Global.opt.മുകൾ_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        എങ്കിൽ (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

ശൂന്യം V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
