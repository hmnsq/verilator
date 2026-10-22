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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkCells.h"

#ضمن "V3Graph.h"
#ضمن "V3Parse.h"
#ضمن "V3SymTable.h"

#ضمن <unordered_set>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

صنف LinkCellsGraph final : عام V3Graph {
عام:
    LinkCellsGraph() = افتراضي;
    ~LinkCellsGraph() override = افتراضي;
    فراغ loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

صنف LinkCellsVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    AstNodeModule* ثابت m_modp;

عام:
    LinkCellsVertex(V3Graph* graphp, AstNodeModule* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = افتراضي;
    AstNodeModule* modp() ثابت VL_MT_STABLE { ܦܢܐ m_modp; }
    string name() ثابت override VL_MT_STABLE { ܦܢܐ cvtToHex(modp()) + ' ' + modp()->name(); }
    FileLine* fileline() ثابت override { ܦܢܐ modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ثابت override {
        ܦܢܐ m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

صنف LibraryVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
عام:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { ܦܢܐ "*LIBRARY*"; }
};

صنف CellEdge final : عام V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* ثابت m_cellp;

عام:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* top, صحيح weight, منطقي cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, top, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ثابت { ܦܢܐ m_cellp; }
    string name() ثابت override VL_MT_STABLE {
        ܦܢܐ cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

فراغ LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    ܐܢ (ثابت LinkCellsVertex* ثابت vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } ܐܚܪܢܐ {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
بنية LinkCellsState final {
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

صنف LinkConfigsVisitor final : عام VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    منطقي m_isTop = ܕܓܠܐ;  // Whether we're in the top-level config
    منطقي m_isDefault = ܕܓܠܐ;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    فراغ visit(AstConfig* nodep) override {
        VL_RESTORER(m_isTop);
        ثابت auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_isTop = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(AstConfigCell* nodep) override {
        ܐܢ (m_isTop) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    فراغ visit(AstConfigRule* nodep) override {
        ܐܢ (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = ܫܪܪܐ;
            iterateAndNextNull(nodep->usep());
        } ܐܚܪܢܐ ܐܢ (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } ܐܚܪܢܐ {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    فراغ visit(AstDot* nodep) override { iterateChildren(nodep); }

    فراغ visit(AstParseRef* nodep) override {
        ܐܢ (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } ܐܚܪܢܐ ܐܢ (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } ܐܚܪܢܐ ܐܢ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } ܐܚܪܢܐ ܐܢ (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    فراغ visit(AstConfigUse* nodep) override {
        ܐܢ (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } ܐܚܪܢܐ ܐܢ (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } ܐܚܪܢܐ ܐܢ (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } ܐܚܪܢܐ ܐܢ (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        ܐܢ (!m_state.m_designs.size() && !v3Global.opt.topModule().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.topModule());
        }
        iterate(nodep);
    }
};

صنف LinkCellsVisitor final : عام VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // STATE
    VInFilter* ثابت m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    AstNodeModule* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    ثابت V3GraphVertex* m_topVertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_origTopModuleName;  // original name of the top module
    صحيح m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(AstNodeModule* nodep) {
        // Return corresponding vertex for this module
        ܐܢ (!nodep->user1p()) nodep->user1p(جديد LinkCellsVertex{&m_graph, nodep});
        ܦܢܐ nodep->user1u().toGraphVertex();
    }
    فراغ newEdge(V3GraphVertex* fromp, V3GraphVertex* top, صحيح weight, منطقي cuttable) {
        ثابت V3GraphEdge* ثابت edgep = جديد V3GraphEdge{&m_graph, fromp, top, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << top->name());
    }
    فراغ cellEdge(V3GraphVertex* fromp, V3GraphVertex* top, صحيح weight, منطقي cuttable,
                  AstCell* cellp) {
        ثابت V3GraphEdge* ثابت edgep
            = جديد CellEdge{&m_graph, fromp, top, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << top->name());
    }
    فراغ insertModInLib(ثابت string& name, ثابت string& libname, AstNodeModule* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        ܐܢ (!libSymp)
            libSymp = m_mods.rootp()->insert(libname, جديد VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->insert(name, جديد VSymEnt{&m_mods, nodep});
    }

    AstNodeModule* findModuleLibSym(ثابت string& modName, ثابت string& libname) {
        // Given module name and library name, find within that exact library
        ثابت VSymEnt* ثابت libSymp = m_mods.rootp()->findIdFallback(libname);
        ܐܢ (!libSymp) ܦܢܐ nullptr;
        ثابت VSymEnt* ثابت foundp = libSymp->findIdFallback(modName);
        ܦܢܐ foundp ? VN_AS(foundp->nodep(), NodeModule) : nullptr;
    }
    AstNodeModule* findModuleSym(ثابت string& modName, ثابت string& libname) {
        // Given module and library to start search in, resolve using config library choices
        AstNodeModule* foundp;
        ثابت std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        ثابت auto itCellUseList = m_state.m_uselistCell.find(fullName);
        ܐܢ (itCellUseList != m_state.m_uselistCell.end()) {
            لكل (auto ثابت& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                ܐܢ (foundp) ܦܢܐ foundp;
            }
        }
        // Then search cell-specific library list
        ثابت auto itCellLibList = m_state.m_liblistCell.find(fullName);
        ܐܢ (itCellLibList != m_state.m_liblistCell.end()) {
            لكل (auto ثابت& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                ܐܢ (foundp) ܦܢܐ foundp;
            }
        }
        // Then search IEEE config default liblist
        لكل (auto ثابت& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            ܐܢ (foundp) ܦܢܐ foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        ܐܢ (foundp) ܦܢܐ foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        ܦܢܐ foundp;
    }

    AstNodeModule* resolveModule(AstNode* nodep, ثابت string& modName, ثابت string& libname) {
        AstNodeModule* modp = findModuleSym(modName, libname);
        ܐܢ (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ثابت string prettyName = AstNode::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, ܫܪܪܐ, ܕܓܠܐ, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            ܐܢ (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        ܦܢܐ modp;
    }

    ساكن فراغ removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        ܐܢ (!v3Global.opt.topModule().empty()) ܦܢܐ;
        لكل (AstNodeModule* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), NodeModule)) {
            ܐܢ (!nodep->inLibrary()) ܦܢܐ;
        }
        لكل (AstNodeModule* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), NodeModule)) {
            nodep->inLibrary(ܕܓܠܐ);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, ثابت std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string top;
        منطقي topFound = ܕܓܠܐ;
        std::getline(ss, top, '.');
        لكل (auto ثابت& pair : m_state.m_designs) {
            ܐܢ (top == pair.second) {
                topFound = ܫܪܪܐ;
                اكسر;
            }
        }
        ܐܢ (!topFound) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            ܦܢܐ nullptr;
        }

        ثابت V3GraphVertex* vtx = m_topVertexp;
        ثابت CellEdge* finalEdgep = nullptr;
        std::string seg;
        ܟܡܐ (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            لكل (ثابت V3GraphEdge& edge : vtx->outEdges()) {
                ܐܢ (ثابت CellEdge* ثابت cedgep = edge.cast<CellEdge>()) {
                    ܐܢ (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->top();
                        finalEdgep = cedgep;
                        اكسر;
                    }
                }
            }
            ܐܢ (!finalEdgep) ܦܢܐ nullptr;
        }

        ܦܢܐ finalEdgep->cellp();
    }

    فراغ assignParamNumbers(AstPin* ثابت firstPinp) {
        لكل (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(ܫܪܪܐ);
            ܐܢ (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pinNum()));
        }
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        AstNodeModule* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        لكل (auto ثابت& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            ܐܢ (!cellp) استمر;
            لكل (auto ثابت& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                ܐܢ (modp) {
                    cellp->modp(modp);
                    اكسر;
                }
            }
        }

        // Search uselists for each instance
        لكل (auto ثابت& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            لكل (auto ثابت& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                ܐܢ (modp) {
                    cellp->modp(modp);
                    اكسر;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ܐܢ (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            ܐܢ (ثابت LinkCellsVertex* ثابت vvertexp = vtx.cast<LinkCellsVertex>()) {
                AstNodeModule* ثابت vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            ܐܢ (ثابت LinkCellsVertex* ثابت vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                AstNodeModule* ثابت vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        ܐܢ (v3Global.opt.topModule() != "" && !m_topVertexp) {
            VSpellCheck spell;
            لكل (V3GraphVertex& vtx : m_graph.vertices()) {
                ܐܢ (ثابت LinkCellsVertex* ثابت vvertexp = vtx.cast<LinkCellsVertex>()) {
                    AstNodeModule* ثابت vmodp = vvertexp->modp();
                    ܐܢ (VN_IS(vmodp, Module)) spell.pushCandidate(vmodp->prettyName());
                }
            }
            ثابت string suggest
                = spell.bestCandidateMsg(AstNode::prettyName(v3Global.opt.topModule()));
            v3error("Specified --top-module '"
                    << AstNode::prettyName(v3Global.opt.topModule())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    فراغ visit(AstNodeModule* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            ܐܢ (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, ܕܓܠܐ);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            ܐܢ (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ثابت auto itFoundPair = m_declfnWarned.insert(nodep->fileline()->filename());
                ܐܢ (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            ܐܢ (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(ܫܪܪܐ);  // Interfaces can't be at top, unless asked
            }

            auto ثابت& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ثابت منطقي topMatch
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            ܐܢ (topMatch) {
                m_topVertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(ܕܓܠܐ);  // Safer to make sure it doesn't disappear
            }
            ܐܢ (v3Global.opt.topModule().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !topMatch) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                ܐܢ (!m_libVertexp) m_libVertexp = جديد LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, ܕܓܠܐ);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->checkTree();
        }
    }

    فراغ visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        AstNodeModule* ثابت modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        ܐܢ (modp) {
            ܐܢ (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                ܐܢ (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, ܕܓܠܐ);
                ܐܢ (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } ܐܚܪܢܐ ܐܢ (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } ܐܚܪܢܐ {
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
        ܐܢ (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    فراغ visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ܐܢ (!nodep->packagep()) {
            AstNodeModule* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ܐܢ (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            ܐܢ (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                ܦܢܐ;
            }
        }
    }
    فراغ visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ܐܢ (!nodep->packagep()) {
            AstNodeModule* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ܐܢ (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            ܐܢ (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                ܦܢܐ;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, ܕܓܠܐ);
    }

    فراغ visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        AstNodeModule* ثابت modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        ܐܢ (modp) {
            AstNode* ثابت cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ثابت منطقي cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        ܐܢ (nodep->user1p() == m_modp) ܦܢܐ;  // AstBind and AstNodeModule may call a cell twice
        ܐܢ (nodep->modName() == m_origTopModuleName) {
            ܐܢ (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } ܐܚܪܢܐ {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                ܦܢܐ;
            }
        }
        nodep->user1p(m_modp);
        //
        ܐܢ (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            AstNodeModule* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            ܐܢ (cellmodp) {
                ܐܢ (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(ܫܪܪܐ);
                    nodep->recursive(ܫܪܪܐ);
                    ܐܢ (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        AstNodeModule* otherModp = VN_CAST(cellmodp->user2p(), NodeModule);
                        ܐܢ (!otherModp) {
                            otherModp = cellmodp->cloneTree(ܕܓܠܐ);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(ܫܪܪܐ);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, ܕܓܠܐ);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } ܐܚܪܢܐ {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } ܐܚܪܢܐ {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, ܕܓܠܐ, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        ܐܢ (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            pushDeletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        ܐܢ (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            pushDeletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        منطقي pinStar = ܕܓܠܐ;
        منطقي pinDotName = ܕܓܠܐ;
        لكل (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            ܐܢ (pinp->svDotName()) pinDotName = ܫܪܪܐ;
            ܐܢ (pinp->dotStar()) {
                ܐܢ (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = ܫܪܪܐ;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->deleteTree(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ܐܢ (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pinNum()));
        }
        assignParamNumbers(nodep->paramsp());
        ܐܢ (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ܐܢ ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                ܐܢ (!pinp->exprp()) {
                    ܐܢ (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } ܐܚܪܢܐ {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.insert(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            لكل (AstNode* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                ܐܢ (ثابت AstPort* ثابت portp = VN_CAST(portnodep, Port)) {

                    ܐܢ (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pinNum())) == ports.end()) {
                        ܐܢ (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ثابت newp
                                = جديد AstPin{nodep->fileline(), 0, portp->name(),
                                             جديد AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(ܫܪܪܐ);
                            newp->svImplicit(ܫܪܪܐ);
                            nodep->addPinsp(newp);
                        } ܐܚܪܢܐ {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ثابت AstVar* portp_varp = nullptr;
                            لكل (AstNode* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                ܐܢ (ثابت AstVar* ثابت varp = VN_CAST(module_stmtsp, Var)) {
                                    ܐܢ (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        اكسر;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            ܐܢ (portp_varp && portp_varp->isInput() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ثابت newp
                                    = جديد AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } ܐܚܪܢܐ {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ثابت newp
                                    = جديد AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        ܐܢ (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            ܐܢ (!nodep->hasIfaceVar()) {
                ثابت string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ثابت idtypep = جديد AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                ܐܢ (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    لكل (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.push_back(rp);
                    }
                    AstNodeDType* dtp = idtypep;
                    لكل (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = جديد AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->cloneTree(ܕܓܠܐ)};
                    }
                    varp = جديد AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } ܐܚܪܢܐ {
                    varp = جديد AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(ܫܪܪܐ);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(ܫܪܪܐ);
            }
        }
        ܐܢ (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    فراغ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        ܐܢ (m_varp) {  // Parser didn't know what was interface, resolve now
            AstNodeModule* ثابت varModp = findModuleSym(nodep->name(), m_modp->libname());
            ܐܢ (AstIface* ثابت ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, ܕܓܠܐ);
            }
        }
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    فراغ visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

    // METHODS
    فراغ readModNames() {
        // mangled_name, BlockOptions
        ثابت V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ثابت auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.topModule());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        لكل (AstNodeModule *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), NodeModule);
            ܐܢ (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ثابت AstNodeModule* ثابت libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            ثابت AstNodeModule* ثابت globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            ܐܢ (libFoundp && libFoundp == nodep) {
                // Ok
            } ܐܚܪܢܐ ܐܢ (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } ܐܚܪܢܐ ܐܢ (libFoundp) {
                ܐܢ (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } ܐܚܪܢܐ ܐܢ (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ثابت string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                insertModInLib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                insertModInLib(nodep->name(), "__GLOBAL", nodep);
            } ܐܚܪܢܐ {
                insertModInLib(nodep->origName(), nodep->libname(), nodep);
                insertModInLib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

عام:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        ܐܢ (v3Global.opt.hierChild()) {
            ثابت V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.topModule().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            ثابت V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.topModule());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_origTopModuleName = hierIt->second.origName();
        } ܐܚܪܢܐ {
            m_origTopModuleName = v3Global.opt.topModule();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        ܐܢ (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

فراغ V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
