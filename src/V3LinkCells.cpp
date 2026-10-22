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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LinkCells.h"

#समावेश "V3Graph.h"
#समावेश "V3Parse.h"
#समावेश "V3SymTable.h"

#समावेश <unordered_set>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

श्रेणी LinkCellsGraph final : खुला V3Graph {
खुला:
    LinkCellsGraph() = शेष;
    ~LinkCellsGraph() override = शेष;
    व्योम loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

श्रेणी LinkCellsVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_गाँठ_Module* स्थिर m_modp;

खुला:
    LinkCellsVertex(V3Graph* graphp, Ast_गाँठ_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = शेष;
    Ast_गाँठ_Module* modp() स्थिर VL_MT_STABLE { वापस m_modp; }
    string name() स्थिर override VL_MT_STABLE { वापस cvtToHex(modp()) + ' ' + modp()->name(); }
    File_पंक्ति* fileline() स्थिर override { वापस modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() स्थिर override {
        वापस m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

श्रेणी LibraryVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
खुला:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = शेष;
    string name() स्थिर override VL_MT_STABLE { वापस "*LIBRARY*"; }
};

श्रेणी CellEdge final : खुला V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* स्थिर m_cellp;

खुला:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight, bool cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, शीर्ष, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() स्थिर { वापस m_cellp; }
    string name() स्थिर override VL_MT_STABLE {
        वापस cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

व्योम LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    अगर (स्थिर LinkCellsVertex* स्थिर vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } अन्यथा {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
काष्ठा LinkCellsState final {
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

श्रेणी LinkConfigsVisitor final : खुला VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    bool m_is_शीर्ष = false;  // Whether we're in the top-level config
    bool m_isDefault = false;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    व्योम visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_शीर्ष);
        स्थिर स्वतः& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_शीर्ष = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::हटाओ(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    व्योम visit(AstConfigCell* nodep) override {
        अगर (m_is_शीर्ष) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    व्योम visit(AstConfigRule* nodep) override {
        अगर (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = true;
            iterate_And_अग्र_Null(nodep->usep());
        } अन्यथा अगर (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_अग्र_Null(nodep->usep());
        } अन्यथा {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_अग्र_Null(nodep->cellp());
            }
            iterate_And_अग्र_Null(nodep->usep());
        }
    }

    व्योम visit(AstDot* nodep) override { iterateChildren(nodep); }

    व्योम visit(AstParseRef* nodep) override {
        अगर (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } अन्यथा अगर (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } अन्यथा अगर (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } अन्यथा अगर (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    व्योम visit(AstConfigUse* nodep) override {
        अगर (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } अन्यथा अगर (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } अन्यथा अगर (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } अन्यथा अगर (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        अगर (!m_state.m_designs.आकार() && !v3Global.opt.शीर्ष_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.शीर्ष_Module());
        }
        iterate(nodep);
    }
};

श्रेणी LinkCellsVisitor final : खुला VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;

    // STATE
    VInFilter* स्थिर m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    स्थिर V3GraphVertex* m_शीर्ष_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_शीर्ष_Module_Name;  // original name of the top module
    पूर्णांक m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_गाँठ_Module* nodep) {
        // Return corresponding vertex for this module
        अगर (!nodep->user1p()) nodep->user1p(नया LinkCellsVertex{&m_graph, nodep});
        वापस nodep->user1u().toGraphVertex();
    }
    व्योम newEdge(V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight, bool cuttable) {
        स्थिर V3GraphEdge* स्थिर edgep = नया V3GraphEdge{&m_graph, fromp, शीर्ष, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << शीर्ष->name());
    }
    व्योम cellEdge(V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight, bool cuttable,
                  AstCell* cellp) {
        स्थिर V3GraphEdge* स्थिर edgep
            = नया CellEdge{&m_graph, fromp, शीर्ष, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << शीर्ष->name());
    }
    व्योम जोड़ो_Mod_In_Lib(स्थिर string& name, स्थिर string& libname, Ast_गाँठ_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        अगर (!libSymp)
            libSymp = m_mods.rootp()->जोड़ो(libname, नया VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->जोड़ो(name, नया VSymEnt{&m_mods, nodep});
    }

    Ast_गाँठ_Module* findModuleLibSym(स्थिर string& modName, स्थिर string& libname) {
        // Given module name and library name, find within that exact library
        स्थिर VSymEnt* स्थिर libSymp = m_mods.rootp()->findIdFallback(libname);
        अगर (!libSymp) वापस nullptr;
        स्थिर VSymEnt* स्थिर foundp = libSymp->findIdFallback(modName);
        वापस foundp ? VN_AS(foundp->nodep(), गाँठ_Module) : nullptr;
    }
    Ast_गाँठ_Module* findModuleSym(स्थिर string& modName, स्थिर string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_गाँठ_Module* foundp;
        स्थिर std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        स्थिर स्वतः itCellUseList = m_state.m_uselistCell.find(fullName);
        अगर (itCellUseList != m_state.m_uselistCell.end()) {
            क्रम (स्वतः स्थिर& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                अगर (foundp) वापस foundp;
            }
        }
        // Then search cell-specific library list
        स्थिर स्वतः itCellLibList = m_state.m_liblistCell.find(fullName);
        अगर (itCellLibList != m_state.m_liblistCell.end()) {
            क्रम (स्वतः स्थिर& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                अगर (foundp) वापस foundp;
            }
        }
        // Then search IEEE config default liblist
        क्रम (स्वतः स्थिर& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            अगर (foundp) वापस foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        अगर (foundp) वापस foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        वापस foundp;
    }

    Ast_गाँठ_Module* resolveModule(Ast_गाँठ* nodep, स्थिर string& modName, स्थिर string& libname) {
        Ast_गाँठ_Module* modp = findModuleSym(modName, libname);
        अगर (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            स्थिर string prettyName = Ast_गाँठ::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, true, false, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            अगर (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        वापस modp;
    }

    अटल व्योम removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        अगर (!v3Global.opt.शीर्ष_Module().empty()) वापस;
        क्रम (Ast_गाँठ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), गाँठ_Module)) {
            अगर (!nodep->inLibrary()) वापस;
        }
        क्रम (Ast_गाँठ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), गाँठ_Module)) {
            nodep->inLibrary(false);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, स्थिर std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string शीर्ष;
        bool शीर्ष_Found = false;
        std::लो_पंक्ति(ss, शीर्ष, '.');
        क्रम (स्वतः स्थिर& pair : m_state.m_designs) {
            अगर (शीर्ष == pair.second) {
                शीर्ष_Found = true;
                अवरोध;
            }
        }
        अगर (!शीर्ष_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            वापस nullptr;
        }

        स्थिर V3GraphVertex* vtx = m_शीर्ष_Vertexp;
        स्थिर CellEdge* finalEdgep = nullptr;
        std::string seg;
        जबतक (std::लो_पंक्ति(ss, seg, '.')) {
            finalEdgep = nullptr;
            क्रम (स्थिर V3GraphEdge& edge : vtx->outEdges()) {
                अगर (स्थिर CellEdge* स्थिर cedgep = edge.cast<CellEdge>()) {
                    अगर (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->शीर्ष();
                        finalEdgep = cedgep;
                        अवरोध;
                    }
                }
            }
            अगर (!finalEdgep) वापस nullptr;
        }

        वापस finalEdgep->cellp();
    }

    व्योम assignParamNumbers(AstPin* स्थिर firstPinp) {
        क्रम (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(true);
            अगर (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_अंक()));
        }
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_गाँठ_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        क्रम (स्वतः स्थिर& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            अगर (!cellp) जारी;
            क्रम (स्वतः स्थिर& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                अगर (modp) {
                    cellp->modp(modp);
                    अवरोध;
                }
            }
        }

        // Search uselists for each instance
        क्रम (स्वतः स्थिर& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            क्रम (स्वतः स्थिर& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                अगर (modp) {
                    cellp->modp(modp);
                    अवरोध;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        अगर (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (स्थिर LinkCellsVertex* स्थिर vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_गाँठ_Module* स्थिर vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (स्थिर LinkCellsVertex* स्थिर vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_गाँठ_Module* स्थिर vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        अगर (v3Global.opt.शीर्ष_Module() != "" && !m_शीर्ष_Vertexp) {
            VSpellCheck spell;
            क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
                अगर (स्थिर LinkCellsVertex* स्थिर vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_गाँठ_Module* स्थिर vmodp = vvertexp->modp();
                    अगर (VN_IS(vmodp, Module)) spell.धकेलो_Candidate(vmodp->prettyName());
                }
            }
            स्थिर string suggest
                = spell.best_Candidate_संदेश(Ast_गाँठ::prettyName(v3Global.opt.शीर्ष_Module()));
            v3error("Specified --top-module '"
                    << Ast_गाँठ::prettyName(v3Global.opt.शीर्ष_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            अगर (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, false);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            अगर (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                स्थिर स्वतः itFoundPair = m_declfnWarned.जोड़ो(nodep->fileline()->filename());
                अगर (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            अगर (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(true);  // Interfaces can't be at top, unless asked
            }

            स्वतः स्थिर& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            स्थिर bool शीर्ष_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            अगर (शीर्ष_Match) {
                m_शीर्ष_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(false);  // Safer to make sure it doesn't disappear
            }
            अगर (v3Global.opt.शीर्ष_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !शीर्ष_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                अगर (!m_libVertexp) m_libVertexp = नया LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, false);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_वृक्ष();
        }
    }

    व्योम visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_गाँठ_Module* स्थिर modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        अगर (modp) {
            अगर (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                अगर (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, false);
                अगर (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } अन्यथा अगर (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } अन्यथा {
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
        अगर (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    व्योम visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        अगर (!nodep->packagep()) {
            Ast_गाँठ_Module* स्थिर modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            अगर (AstPackage* स्थिर pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            अगर (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                वापस;
            }
        }
    }
    व्योम visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        अगर (!nodep->packagep()) {
            Ast_गाँठ_Module* स्थिर modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            अगर (AstPackage* स्थिर pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            अगर (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                वापस;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, false);
    }

    व्योम visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_गाँठ_Module* स्थिर modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        अगर (modp) {
            Ast_गाँठ* स्थिर cellsp = nodep->cellsp()->unlink_Fr_Back_With_अग्र();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterate_And_अग्र_Null(cellsp);
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    व्योम visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        स्थिर bool cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        अगर (nodep->user1p() == m_modp) वापस;  // AstBind and AstNodeModule may call a cell twice
        अगर (nodep->modName() == m_orig_शीर्ष_Module_Name) {
            अगर (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } अन्यथा {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
        }
        nodep->user1p(m_modp);
        //
        अगर (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_गाँठ_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            अगर (cellmodp) {
                अगर (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(true);
                    nodep->recursive(true);
                    अगर (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_गाँठ_Module* otherModp = VN_CAST(cellmodp->user2p(), गाँठ_Module);
                        अगर (!otherModp) {
                            otherModp = cellmodp->clone_वृक्ष(false);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(true);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, false);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } अन्यथा {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } अन्यथा {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, false, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        अगर (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            धकेलो_Deletep(nodep->pinsp()->unlink_Fr_Back_With_अग्र());
        }
        अगर (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            धकेलो_Deletep(nodep->paramsp()->unlink_Fr_Back_With_अग्र());
        }
        // Convert .* to list of pins
        bool pinStar = false;
        bool pinDotName = false;
        क्रम (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            अगर (pinp->svDotName()) pinDotName = true;
            अगर (pinp->dotStar()) {
                अगर (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = true;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_वृक्ष(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        क्रम (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अगर (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_अंक()));
        }
        assignParamNumbers(nodep->paramsp());
        अगर (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            क्रम (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                अगर (!pinp->exprp()) {
                    अगर (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } अन्यथा {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.जोड़ो(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            क्रम (Ast_गाँठ* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                अगर (स्थिर AstPort* स्थिर portp = VN_CAST(portnodep, Port)) {

                    अगर (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_अंक())) == ports.end()) {
                        अगर (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* स्थिर newp
                                = नया AstPin{nodep->fileline(), 0, portp->name(),
                                             नया AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(true);
                            newp->svImplicit(true);
                            nodep->addPinsp(newp);
                        } अन्यथा {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            स्थिर AstVar* portp_varp = nullptr;
                            क्रम (Ast_गाँठ* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                अगर (स्थिर AstVar* स्थिर varp = VN_CAST(module_stmtsp, Var)) {
                                    अगर (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        अवरोध;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            अगर (portp_varp && portp_varp->is_निवेश() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* स्थिर newp
                                    = नया AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } अन्यथा {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* स्थिर newp
                                    = नया AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        अगर (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            अगर (!nodep->hasIfaceVar()) {
                स्थिर string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* स्थिर idtypep = नया AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                अगर (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    क्रम (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.धकेलो_back(rp);
                    }
                    Ast_गाँठ_DType* dtp = idtypep;
                    क्रम (स्वतः it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = नया AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_वृक्ष(false)};
                    }
                    varp = नया AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } अन्यथा {
                    varp = नया AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(true);
                nodep->add_अग्र_Here(varp);
                nodep->hasIfaceVar(true);
            }
        }
        अगर (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    व्योम visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        अगर (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_गाँठ_Module* स्थिर varModp = findModuleSym(nodep->name(), m_modp->libname());
            अगर (AstIface* स्थिर ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, false);
            }
        }
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    व्योम visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_अग्र_Null(nodep->childDTypep());
        }
        iterate_And_अग्र_Null(nodep->delayp());
        iterate_And_अग्र_Null(nodep->valuep());
        iterate_And_अग्र_Null(nodep->attrsp());
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    // METHODS
    व्योम readModNames() {
        // mangled_name, BlockOptions
        स्थिर V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        स्थिर स्वतः hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.शीर्ष_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        क्रम (Ast_गाँठ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), गाँठ_Module);
            अगर (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            स्थिर Ast_गाँठ_Module* स्थिर libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            स्थिर Ast_गाँठ_Module* स्थिर globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            अगर (libFoundp && libFoundp == nodep) {
                // Ok
            } अन्यथा अगर (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } अन्यथा अगर (libFoundp) {
                अगर (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा अगर (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                स्थिर string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                जोड़ो_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                जोड़ो_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } अन्यथा {
                जोड़ो_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                जोड़ो_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

खुला:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        अगर (v3Global.opt.hierChild()) {
            स्थिर V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.शीर्ष_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            स्थिर V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.शीर्ष_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_शीर्ष_Module_Name = hierIt->second.origName();
        } अन्यथा {
            m_orig_शीर्ष_Module_Name = v3Global.opt.शीर्ष_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        अगर (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

व्योम V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
