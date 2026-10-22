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
    Ast_गाँठो_Module* अचर m_modp;

सार्वजनिक:
    LinkCellsVertex(V3Graph* graphp, Ast_गाँठो_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = स्वभाव;
    Ast_गाँठो_Module* modp() अचर VL_MT_STABLE { फर्काऊ m_modp; }
    string name() अचर override VL_MT_STABLE { फर्काऊ cvtToHex(modp()) + ' ' + modp()->name(); }
    File_पङ्क्ति* fileline() अचर override { फर्काऊ modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() अचर override {
        फर्काऊ m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

वर्ग LibraryVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
सार्वजनिक:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = स्वभाव;
    string name() अचर override VL_MT_STABLE { फर्काऊ "*LIBRARY*"; }
};

वर्ग CellEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* अचर m_cellp;

सार्वजनिक:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight, तार्किक cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, शीर्ष, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() अचर { फर्काऊ m_cellp; }
    string name() अचर override VL_MT_STABLE {
        फर्काऊ cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

शून्य LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    यदि (अचर LinkCellsVertex* अचर vvertexp = vertexp->cast<LinkCellsVertex>()) {
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
    तार्किक m_is_शीर्ष = असत्य;  // Whether we're in the top-level config
    तार्किक m_isDefault = असत्य;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    शून्य visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_शीर्ष);
        अचर स्वतः& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_शीर्ष = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::हटाओ(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(AstConfigCell* nodep) override {
        यदि (m_is_शीर्ष) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    शून्य visit(AstConfigRule* nodep) override {
        यदि (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = सत्य;
            iterateAndNextNull(nodep->usep());
        } अन्यथा यदि (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } अन्यथा {
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
        यदि (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } अन्यथा यदि (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } अन्यथा यदि (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } अन्यथा यदि (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    शून्य visit(AstConfigUse* nodep) override {
        यदि (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } अन्यथा यदि (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } अन्यथा यदि (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } अन्यथा यदि (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        यदि (!m_state.m_designs.size() && !v3Global.opt.शीर्ष_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.शीर्ष_Module());
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
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;

    // STATE
    VInFilter* अचर m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    अचर V3GraphVertex* m_शीर्ष_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_शीर्ष_Module_Name;  // original name of the top module
    पूर्णाङ्क m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_गाँठो_Module* nodep) {
        // Return corresponding vertex for this module
        यदि (!nodep->user1p()) nodep->user1p(नूतन LinkCellsVertex{&m_graph, nodep});
        फर्काऊ nodep->user1u().toGraphVertex();
    }
    शून्य newEdge(V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight, तार्किक cuttable) {
        अचर V3GraphEdge* अचर edgep = नूतन V3GraphEdge{&m_graph, fromp, शीर्ष, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << शीर्ष->name());
    }
    शून्य cellEdge(V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight, तार्किक cuttable,
                  AstCell* cellp) {
        अचर V3GraphEdge* अचर edgep
            = नूतन CellEdge{&m_graph, fromp, शीर्ष, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << शीर्ष->name());
    }
    शून्य घुसाउ_Mod_In_Lib(अचर string& name, अचर string& libname, Ast_गाँठो_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        यदि (!libSymp)
            libSymp = m_mods.rootp()->घुसाउ(libname, नूतन VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->घुसाउ(name, नूतन VSymEnt{&m_mods, nodep});
    }

    Ast_गाँठो_Module* findModuleLibSym(अचर string& modName, अचर string& libname) {
        // Given module name and library name, find within that exact library
        अचर VSymEnt* अचर libSymp = m_mods.rootp()->findIdFallback(libname);
        यदि (!libSymp) फर्काऊ nullptr;
        अचर VSymEnt* अचर foundp = libSymp->findIdFallback(modName);
        फर्काऊ foundp ? VN_AS(foundp->nodep(), गाँठो_Module) : nullptr;
    }
    Ast_गाँठो_Module* findModuleSym(अचर string& modName, अचर string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_गाँठो_Module* foundp;
        अचर std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        अचर स्वतः itCellUseList = m_state.m_uselistCell.find(fullName);
        यदि (itCellUseList != m_state.m_uselistCell.end()) {
            लागि (स्वतः अचर& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                यदि (foundp) फर्काऊ foundp;
            }
        }
        // Then search cell-specific library list
        अचर स्वतः itCellLibList = m_state.m_liblistCell.find(fullName);
        यदि (itCellLibList != m_state.m_liblistCell.end()) {
            लागि (स्वतः अचर& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                यदि (foundp) फर्काऊ foundp;
            }
        }
        // Then search IEEE config default liblist
        लागि (स्वतः अचर& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            यदि (foundp) फर्काऊ foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        यदि (foundp) फर्काऊ foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        फर्काऊ foundp;
    }

    Ast_गाँठो_Module* resolveModule(Ast_गाँठो* nodep, अचर string& modName, अचर string& libname) {
        Ast_गाँठो_Module* modp = findModuleSym(modName, libname);
        यदि (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            अचर string prettyName = Ast_गाँठो::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, सत्य, असत्य, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            यदि (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        फर्काऊ modp;
    }

    स्थिर शून्य removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        यदि (!v3Global.opt.शीर्ष_Module().empty()) फर्काऊ;
        लागि (Ast_गाँठो_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), गाँठो_Module)) {
            यदि (!nodep->inLibrary()) फर्काऊ;
        }
        लागि (Ast_गाँठो_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), गाँठो_Module)) {
            nodep->inLibrary(असत्य);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, अचर std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string शीर्ष;
        तार्किक शीर्ष_Found = असत्य;
        std::getline(ss, शीर्ष, '.');
        लागि (स्वतः अचर& pair : m_state.m_designs) {
            यदि (शीर्ष == pair.second) {
                शीर्ष_Found = सत्य;
                रोक;
            }
        }
        यदि (!शीर्ष_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            फर्काऊ nullptr;
        }

        अचर V3GraphVertex* vtx = m_शीर्ष_Vertexp;
        अचर CellEdge* finalEdgep = nullptr;
        std::string seg;
        जबसम्म (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            लागि (अचर V3GraphEdge& edge : vtx->outEdges()) {
                यदि (अचर CellEdge* अचर cedgep = edge.cast<CellEdge>()) {
                    यदि (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->शीर्ष();
                        finalEdgep = cedgep;
                        रोक;
                    }
                }
            }
            यदि (!finalEdgep) फर्काऊ nullptr;
        }

        फर्काऊ finalEdgep->cellp();
    }

    शून्य assignParamNumbers(AstPin* अचर firstPinp) {
        लागि (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(सत्य);
            यदि (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_अङ्क()));
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_गाँठो_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        लागि (स्वतः अचर& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            यदि (!cellp) जारी;
            लागि (स्वतः अचर& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                यदि (modp) {
                    cellp->modp(modp);
                    रोक;
                }
            }
        }

        // Search uselists for each instance
        लागि (स्वतः अचर& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            लागि (स्वतः अचर& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                यदि (modp) {
                    cellp->modp(modp);
                    रोक;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        यदि (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        लागि (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (अचर LinkCellsVertex* अचर vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_गाँठो_Module* अचर vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        लागि (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (अचर LinkCellsVertex* अचर vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_गाँठो_Module* अचर vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        यदि (v3Global.opt.शीर्ष_Module() != "" && !m_शीर्ष_Vertexp) {
            VSpellCheck spell;
            लागि (V3GraphVertex& vtx : m_graph.vertices()) {
                यदि (अचर LinkCellsVertex* अचर vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_गाँठो_Module* अचर vmodp = vvertexp->modp();
                    यदि (VN_IS(vmodp, Module)) spell.थप_Candidate(vmodp->prettyName());
                }
            }
            अचर string suggest
                = spell.best_Candidate_सन्देश(Ast_गाँठो::prettyName(v3Global.opt.शीर्ष_Module()));
            v3error("Specified --top-module '"
                    << Ast_गाँठो::prettyName(v3Global.opt.शीर्ष_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            यदि (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, असत्य);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            यदि (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                अचर स्वतः itFoundPair = m_declfnWarned.घुसाउ(nodep->fileline()->filename());
                यदि (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            यदि (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(सत्य);  // Interfaces can't be at top, unless asked
            }

            स्वतः अचर& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            अचर तार्किक शीर्ष_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            यदि (शीर्ष_Match) {
                m_शीर्ष_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(असत्य);  // Safer to make sure it doesn't disappear
            }
            यदि (v3Global.opt.शीर्ष_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !शीर्ष_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                यदि (!m_libVertexp) m_libVertexp = नूतन LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, असत्य);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_रूख();
        }
    }

    शून्य visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_गाँठो_Module* अचर modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        यदि (modp) {
            यदि (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                यदि (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, असत्य);
                यदि (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } अन्यथा यदि (VN_IS(modp, NotFoundModule)) {  // Will error out later
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
        यदि (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    शून्य visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        यदि (!nodep->packagep()) {
            Ast_गाँठो_Module* अचर modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            यदि (AstPackage* अचर pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            यदि (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                फर्काऊ;
            }
        }
    }
    शून्य visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        यदि (!nodep->packagep()) {
            Ast_गाँठो_Module* अचर modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            यदि (AstPackage* अचर pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            यदि (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                फर्काऊ;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, असत्य);
    }

    शून्य visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_गाँठो_Module* अचर modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        यदि (modp) {
            Ast_गाँठो* अचर cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        अचर तार्किक cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        यदि (nodep->user1p() == m_modp) फर्काऊ;  // AstBind and AstNodeModule may call a cell twice
        यदि (nodep->modName() == m_orig_शीर्ष_Module_Name) {
            यदि (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } अन्यथा {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                फर्काऊ;
            }
        }
        nodep->user1p(m_modp);
        //
        यदि (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_गाँठो_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            यदि (cellmodp) {
                यदि (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(सत्य);
                    nodep->recursive(सत्य);
                    यदि (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_गाँठो_Module* otherModp = VN_CAST(cellmodp->user2p(), गाँठो_Module);
                        यदि (!otherModp) {
                            otherModp = cellmodp->clone_रूख(असत्य);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(सत्य);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, असत्य);
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
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, असत्य, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        यदि (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            थप_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        यदि (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            थप_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        तार्किक pinStar = असत्य;
        तार्किक pinDotName = असत्य;
        लागि (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            यदि (pinp->svDotName()) pinDotName = सत्य;
            यदि (pinp->dotStar()) {
                यदि (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = सत्य;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_रूख(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        लागि (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            यदि (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_अङ्क()));
        }
        assignParamNumbers(nodep->paramsp());
        यदि (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            लागि (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                यदि (!pinp->exprp()) {
                    यदि (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } अन्यथा {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.घुसाउ(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            लागि (Ast_गाँठो* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                यदि (अचर AstPort* अचर portp = VN_CAST(portnodep, Port)) {

                    यदि (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_अङ्क())) == ports.end()) {
                        यदि (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* अचर newp
                                = नूतन AstPin{nodep->fileline(), 0, portp->name(),
                                             नूतन AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(सत्य);
                            newp->svImplicit(सत्य);
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
                            अचर AstVar* portp_varp = nullptr;
                            लागि (Ast_गाँठो* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                यदि (अचर AstVar* अचर varp = VN_CAST(module_stmtsp, Var)) {
                                    यदि (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        रोक;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            यदि (portp_varp && portp_varp->is_आगत() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* अचर newp
                                    = नूतन AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } अन्यथा {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* अचर newp
                                    = नूतन AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        यदि (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            यदि (!nodep->hasIfaceVar()) {
                अचर string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* अचर idtypep = नूतन AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                यदि (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    लागि (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.थप_back(rp);
                    }
                    Ast_गाँठो_DType* dtp = idtypep;
                    लागि (स्वतः it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = नूतन AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_रूख(असत्य)};
                    }
                    varp = नूतन AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } अन्यथा {
                    varp = नूतन AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(सत्य);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(सत्य);
            }
        }
        यदि (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    शून्य visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        यदि (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_गाँठो_Module* अचर varModp = findModuleSym(nodep->name(), m_modp->libname());
            यदि (AstIface* अचर ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, असत्य);
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

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

    // METHODS
    शून्य readModNames() {
        // mangled_name, BlockOptions
        अचर V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        अचर स्वतः hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.शीर्ष_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        लागि (Ast_गाँठो_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), गाँठो_Module);
            यदि (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            अचर Ast_गाँठो_Module* अचर libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            अचर Ast_गाँठो_Module* अचर globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            यदि (libFoundp && libFoundp == nodep) {
                // Ok
            } अन्यथा यदि (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } अन्यथा यदि (libFoundp) {
                यदि (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            } अन्यथा यदि (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                अचर string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                घुसाउ_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                घुसाउ_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } अन्यथा {
                घुसाउ_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                घुसाउ_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
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
        यदि (v3Global.opt.hierChild()) {
            अचर V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.शीर्ष_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            अचर V3HierBlockOptSet::const_iterator hierIt
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
        यदि (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
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
