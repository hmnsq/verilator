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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkCells.h"

#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Parse.h"
#अन्तर्भाव "V3SymTable.h"

#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

वर्ग LinkCellsGraph final : सार्वजनिक V3Graph {
सार्वजनिक:
    LinkCellsGraph() = स्वभाव;
    ~LinkCellsGraph() override = स्वभाव;
    शून्य loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

वर्ग LinkCellsVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;

सार्वजनिक:
    LinkCellsVertex(V3Graph* graphp, Ast_ग्रन्थिः_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = स्वभाव;
    Ast_ग्रन्थिः_Module* modp() ध्रुव VL_MT_STABLE { प्रत्यागम m_modp; }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम cvtToHex(modp()) + ' ' + modp()->name(); }
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ध्रुव override {
        प्रत्यागम m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

वर्ग LibraryVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
सार्वजनिक:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = स्वभाव;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "*LIBRARY*"; }
};

वर्ग CellEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* ध्रुव m_cellp;

सार्वजनिक:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight, तार्किक cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, शिखरम्, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ध्रुव { प्रत्यागम m_cellp; }
    string name() ध्रुव override VL_MT_STABLE {
        प्रत्यागम cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

शून्य LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    जइ (ध्रुव LinkCellsVertex* ध्रुव vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } अण्णहा {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
संरचना LinkCellsState final {
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

वर्ग LinkConfigsVisitor final : सार्वजनिक VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    तार्किक m_is_शिखरम् = मिच्छ;  // Whether we're in the top-level config
    तार्किक m_isDefault = मिच्छ;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    शून्य visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_शिखरम्);
        ध्रुव auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_शिखरम् = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(AstConfigCell* nodep) override {
        जइ (m_is_शिखरम्) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    शून्य visit(AstConfigRule* nodep) override {
        जइ (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = सच्च;
            iterateAndNextNull(nodep->usep());
        } अण्णहा जइ (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } अण्णहा {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    शून्य visit(AstDot* nodep) override { iterateChildren(nodep); }

    शून्य visit(AstParseRef* nodep) override {
        जइ (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } अण्णहा जइ (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } अण्णहा जइ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } अण्णहा जइ (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    शून्य visit(AstConfigUse* nodep) override {
        जइ (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } अण्णहा जइ (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } अण्णहा जइ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } अण्णहा जइ (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        जइ (!m_state.m_designs.size() && !v3Global.opt.शिखरम्_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.शिखरम्_Module());
        }
        iterate(nodep);
    }
};

वर्ग LinkCellsVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;

    // STATE
    VInFilter* ध्रुव m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    ध्रुव V3GraphVertex* m_शिखरम्_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_शिखरम्_Module_Name;  // original name of the top module
    पूर्णाङ्क m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_ग्रन्थिः_Module* nodep) {
        // Return corresponding vertex for this module
        जइ (!nodep->user1p()) nodep->user1p(नूतन LinkCellsVertex{&m_graph, nodep});
        प्रत्यागम nodep->user1u().toGraphVertex();
    }
    शून्य newEdge(V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight, तार्किक cuttable) {
        ध्रुव V3GraphEdge* ध्रुव edgep = नूतन V3GraphEdge{&m_graph, fromp, शिखरम्, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << शिखरम्->name());
    }
    शून्य cellEdge(V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight, तार्किक cuttable,
                  AstCell* cellp) {
        ध्रुव V3GraphEdge* ध्रुव edgep
            = नूतन CellEdge{&m_graph, fromp, शिखरम्, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << शिखरम्->name());
    }
    शून्य निवेशय_Mod_In_Lib(ध्रुव string& name, ध्रुव string& libname, Ast_ग्रन्थिः_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        जइ (!libSymp)
            libSymp = m_mods.rootp()->निवेशय(libname, नूतन VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->निवेशय(name, नूतन VSymEnt{&m_mods, nodep});
    }

    Ast_ग्रन्थिः_Module* findModuleLibSym(ध्रुव string& modName, ध्रुव string& libname) {
        // Given module name and library name, find within that exact library
        ध्रुव VSymEnt* ध्रुव libSymp = m_mods.rootp()->findIdFallback(libname);
        जइ (!libSymp) प्रत्यागम nullptr;
        ध्रुव VSymEnt* ध्रुव foundp = libSymp->findIdFallback(modName);
        प्रत्यागम foundp ? VN_AS(foundp->nodep(), ग्रन्थिः_Module) : nullptr;
    }
    Ast_ग्रन्थिः_Module* findModuleSym(ध्रुव string& modName, ध्रुव string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_ग्रन्थिः_Module* foundp;
        ध्रुव std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        ध्रुव auto itCellUseList = m_state.m_uselistCell.find(fullName);
        जइ (itCellUseList != m_state.m_uselistCell.end()) {
            प्रति (auto ध्रुव& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                जइ (foundp) प्रत्यागम foundp;
            }
        }
        // Then search cell-specific library list
        ध्रुव auto itCellLibList = m_state.m_liblistCell.find(fullName);
        जइ (itCellLibList != m_state.m_liblistCell.end()) {
            प्रति (auto ध्रुव& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                जइ (foundp) प्रत्यागम foundp;
            }
        }
        // Then search IEEE config default liblist
        प्रति (auto ध्रुव& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            जइ (foundp) प्रत्यागम foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        जइ (foundp) प्रत्यागम foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        प्रत्यागम foundp;
    }

    Ast_ग्रन्थिः_Module* resolveModule(Ast_ग्रन्थिः* nodep, ध्रुव string& modName, ध्रुव string& libname) {
        Ast_ग्रन्थिः_Module* modp = findModuleSym(modName, libname);
        जइ (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ध्रुव string prettyName = Ast_ग्रन्थिः::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, सच्च, मिच्छ, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            जइ (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        प्रत्यागम modp;
    }

    स्थिर शून्य removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        जइ (!v3Global.opt.शिखरम्_Module().empty()) प्रत्यागम;
        प्रति (Ast_ग्रन्थिः_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ग्रन्थिः_Module)) {
            जइ (!nodep->inLibrary()) प्रत्यागम;
        }
        प्रति (Ast_ग्रन्थिः_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ग्रन्थिः_Module)) {
            nodep->inLibrary(मिच्छ);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, ध्रुव std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string शिखरम्;
        तार्किक शिखरम्_Found = मिच्छ;
        std::getline(ss, शिखरम्, '.');
        प्रति (auto ध्रुव& pair : m_state.m_designs) {
            जइ (शिखरम् == pair.second) {
                शिखरम्_Found = सच्च;
                भङ्ग;
            }
        }
        जइ (!शिखरम्_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            प्रत्यागम nullptr;
        }

        ध्रुव V3GraphVertex* vtx = m_शिखरम्_Vertexp;
        ध्रुव CellEdge* finalEdgep = nullptr;
        std::string seg;
        जाव (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            प्रति (ध्रुव V3GraphEdge& edge : vtx->outEdges()) {
                जइ (ध्रुव CellEdge* ध्रुव cedgep = edge.cast<CellEdge>()) {
                    जइ (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->शिखरम्();
                        finalEdgep = cedgep;
                        भङ्ग;
                    }
                }
            }
            जइ (!finalEdgep) प्रत्यागम nullptr;
        }

        प्रत्यागम finalEdgep->cellp();
    }

    शून्य assignParamNumbers(AstPin* ध्रुव firstPinp) {
        प्रति (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(सच्च);
            जइ (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_अङ्कः()));
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_ग्रन्थिः_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        प्रति (auto ध्रुव& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            जइ (!cellp) अनुवर्त;
            प्रति (auto ध्रुव& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                जइ (modp) {
                    cellp->modp(modp);
                    भङ्ग;
                }
            }
        }

        // Search uselists for each instance
        प्रति (auto ध्रुव& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            प्रति (auto ध्रुव& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                जइ (modp) {
                    cellp->modp(modp);
                    भङ्ग;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        जइ (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            जइ (ध्रुव LinkCellsVertex* ध्रुव vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_ग्रन्थिः_Module* ध्रुव vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            जइ (ध्रुव LinkCellsVertex* ध्रुव vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_ग्रन्थिः_Module* ध्रुव vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        जइ (v3Global.opt.शिखरम्_Module() != "" && !m_शिखरम्_Vertexp) {
            VSpellCheck spell;
            प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
                जइ (ध्रुव LinkCellsVertex* ध्रुव vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_ग्रन्थिः_Module* ध्रुव vmodp = vvertexp->modp();
                    जइ (VN_IS(vmodp, Module)) spell.प्रक्षिप_Candidate(vmodp->prettyName());
                }
            }
            ध्रुव string suggest
                = spell.best_Candidate_सन्देशः(Ast_ग्रन्थिः::prettyName(v3Global.opt.शिखरम्_Module()));
            v3error("Specified --top-module '"
                    << Ast_ग्रन्थिः::prettyName(v3Global.opt.शिखरम्_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            जइ (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, मिच्छ);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            जइ (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ध्रुव auto itFoundPair = m_declfnWarned.निवेशय(nodep->fileline()->filename());
                जइ (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            जइ (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(सच्च);  // Interfaces can't be at top, unless asked
            }

            auto ध्रुव& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ध्रुव तार्किक शिखरम्_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            जइ (शिखरम्_Match) {
                m_शिखरम्_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(मिच्छ);  // Safer to make sure it doesn't disappear
            }
            जइ (v3Global.opt.शिखरम्_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !शिखरम्_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                जइ (!m_libVertexp) m_libVertexp = नूतन LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, मिच्छ);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_वृक्षः();
        }
    }

    शून्य visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_ग्रन्थिः_Module* ध्रुव modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        जइ (modp) {
            जइ (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                जइ (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, मिच्छ);
                जइ (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } अण्णहा जइ (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } अण्णहा {
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
        जइ (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    शून्य visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        जइ (!nodep->packagep()) {
            Ast_ग्रन्थिः_Module* ध्रुव modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            जइ (AstPackage* ध्रुव pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            जइ (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                प्रत्यागम;
            }
        }
    }
    शून्य visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        जइ (!nodep->packagep()) {
            Ast_ग्रन्थिः_Module* ध्रुव modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            जइ (AstPackage* ध्रुव pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            जइ (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                प्रत्यागम;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, मिच्छ);
    }

    शून्य visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_ग्रन्थिः_Module* ध्रुव modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        जइ (modp) {
            Ast_ग्रन्थिः* ध्रुव cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ध्रुव तार्किक cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        जइ (nodep->user1p() == m_modp) प्रत्यागम;  // AstBind and AstNodeModule may call a cell twice
        जइ (nodep->modName() == m_orig_शिखरम्_Module_Name) {
            जइ (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } अण्णहा {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                प्रत्यागम;
            }
        }
        nodep->user1p(m_modp);
        //
        जइ (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_ग्रन्थिः_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            जइ (cellmodp) {
                जइ (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(सच्च);
                    nodep->recursive(सच्च);
                    जइ (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_ग्रन्थिः_Module* otherModp = VN_CAST(cellmodp->user2p(), ग्रन्थिः_Module);
                        जइ (!otherModp) {
                            otherModp = cellmodp->clone_वृक्षः(मिच्छ);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(सच्च);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, मिच्छ);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } अण्णहा {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } अण्णहा {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, मिच्छ, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        जइ (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            प्रक्षिप_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        जइ (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            प्रक्षिप_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        तार्किक pinStar = मिच्छ;
        तार्किक pinDotName = मिच्छ;
        प्रति (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            जइ (pinp->svDotName()) pinDotName = सच्च;
            जइ (pinp->dotStar()) {
                जइ (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = सच्च;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_वृक्षः(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        प्रति (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            जइ (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_अङ्कः()));
        }
        assignParamNumbers(nodep->paramsp());
        जइ (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            प्रति (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                जइ ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                जइ (!pinp->exprp()) {
                    जइ (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } अण्णहा {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.निवेशय(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            प्रति (Ast_ग्रन्थिः* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                जइ (ध्रुव AstPort* ध्रुव portp = VN_CAST(portnodep, Port)) {

                    जइ (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_अङ्कः())) == ports.end()) {
                        जइ (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ध्रुव newp
                                = नूतन AstPin{nodep->fileline(), 0, portp->name(),
                                             नूतन AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(सच्च);
                            newp->svImplicit(सच्च);
                            nodep->addPinsp(newp);
                        } अण्णहा {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ध्रुव AstVar* portp_varp = nullptr;
                            प्रति (Ast_ग्रन्थिः* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                जइ (ध्रुव AstVar* ध्रुव varp = VN_CAST(module_stmtsp, Var)) {
                                    जइ (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        भङ्ग;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            जइ (portp_varp && portp_varp->is_निवेशः() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ध्रुव newp
                                    = नूतन AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } अण्णहा {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ध्रुव newp
                                    = नूतन AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        जइ (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            जइ (!nodep->hasIfaceVar()) {
                ध्रुव string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ध्रुव idtypep = नूतन AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                जइ (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    प्रति (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.प्रक्षिप_back(rp);
                    }
                    Ast_ग्रन्थिः_DType* dtp = idtypep;
                    प्रति (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = नूतन AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_वृक्षः(मिच्छ)};
                    }
                    varp = नूतन AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } अण्णहा {
                    varp = नूतन AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(सच्च);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(सच्च);
            }
        }
        जइ (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    शून्य visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        जइ (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_ग्रन्थिः_Module* ध्रुव varModp = findModuleSym(nodep->name(), m_modp->libname());
            जइ (AstIface* ध्रुव ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, मिच्छ);
            }
        }
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    शून्य visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    // METHODS
    शून्य readModNames() {
        // mangled_name, BlockOptions
        ध्रुव V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ध्रुव auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.शिखरम्_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        प्रति (Ast_ग्रन्थिः_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ग्रन्थिः_Module);
            जइ (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            जइ (libFoundp && libFoundp == nodep) {
                // Ok
            } अण्णहा जइ (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } अण्णहा जइ (libFoundp) {
                जइ (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अण्णहा जइ (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ध्रुव string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                निवेशय_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                निवेशय_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } अण्णहा {
                निवेशय_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                निवेशय_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

सार्वजनिक:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        जइ (v3Global.opt.hierChild()) {
            ध्रुव V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.शिखरम्_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            ध्रुव V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.शिखरम्_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_शिखरम्_Module_Name = hierIt->second.origName();
        } अण्णहा {
            m_orig_शिखरम्_Module_Name = v3Global.opt.शिखरम्_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        जइ (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

शून्य V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
