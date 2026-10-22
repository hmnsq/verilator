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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkCells.h"

#شامل "V3Graph.h"
#شامل "V3Parse.h"
#شامل "V3SymTable.h"

#شامل <unordered_set>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

رده LinkCellsGraph final : عمومی V3Graph {
عمومی:
    LinkCellsGraph() = پیشفرض;
    ~LinkCellsGraph() override = پیشفرض;
    تهی loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

رده LinkCellsVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_گره_Module* ثابت m_modp;

عمومی:
    LinkCellsVertex(V3Graph* graphp, Ast_گره_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = پیشفرض;
    Ast_گره_Module* modp() ثابت VL_MT_STABLE { بازگشت m_modp; }
    string name() ثابت override VL_MT_STABLE { بازگشت cvtToHex(modp()) + ' ' + modp()->name(); }
    File_سطر* fileline() ثابت override { بازگشت modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ثابت override {
        بازگشت m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

رده LibraryVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
عمومی:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = پیشفرض;
    string name() ثابت override VL_MT_STABLE { بازگشت "*LIBRARY*"; }
};

رده CellEdge final : عمومی V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* ثابت m_cellp;

عمومی:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight, منطقی cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, بالاترین, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ثابت { بازگشت m_cellp; }
    string name() ثابت override VL_MT_STABLE {
        بازگشت cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

تهی LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    اگر (ثابت LinkCellsVertex* ثابت vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } وگرنه {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
ساختار LinkCellsState final {
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

رده LinkConfigsVisitor final : عمومی VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    منطقی m_is_بالاترین = نادرست;  // Whether we're in the top-level config
    منطقی m_isDefault = نادرست;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    تهی visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_بالاترین);
        ثابت auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_بالاترین = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    تهی visit(AstConfigCell* nodep) override {
        اگر (m_is_بالاترین) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    تهی visit(AstConfigRule* nodep) override {
        اگر (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = درست;
            iterateAndNextNull(nodep->usep());
        } وگرنه اگر (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } وگرنه {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    تهی visit(AstDot* nodep) override { iterateChildren(nodep); }

    تهی visit(AstParseRef* nodep) override {
        اگر (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } وگرنه اگر (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } وگرنه اگر (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } وگرنه اگر (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    تهی visit(AstConfigUse* nodep) override {
        اگر (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } وگرنه اگر (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } وگرنه اگر (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } وگرنه اگر (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        اگر (!m_state.m_designs.size() && !v3Global.opt.بالاترین_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.بالاترین_Module());
        }
        iterate(nodep);
    }
};

رده LinkCellsVisitor final : عمومی VNVisitor {
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
    Ast_گره_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    ثابت V3GraphVertex* m_بالاترین_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_بالاترین_Module_Name;  // original name of the top module
    صحیح m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_گره_Module* nodep) {
        // Return corresponding vertex for this module
        اگر (!nodep->user1p()) nodep->user1p(جدید LinkCellsVertex{&m_graph, nodep});
        بازگشت nodep->user1u().toGraphVertex();
    }
    تهی newEdge(V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight, منطقی cuttable) {
        ثابت V3GraphEdge* ثابت edgep = جدید V3GraphEdge{&m_graph, fromp, بالاترین, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << بالاترین->name());
    }
    تهی cellEdge(V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight, منطقی cuttable,
                  AstCell* cellp) {
        ثابت V3GraphEdge* ثابت edgep
            = جدید CellEdge{&m_graph, fromp, بالاترین, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << بالاترین->name());
    }
    تهی درج_Mod_In_Lib(ثابت string& name, ثابت string& libname, Ast_گره_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        اگر (!libSymp)
            libSymp = m_mods.rootp()->درج(libname, جدید VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->درج(name, جدید VSymEnt{&m_mods, nodep});
    }

    Ast_گره_Module* findModuleLibSym(ثابت string& modName, ثابت string& libname) {
        // Given module name and library name, find within that exact library
        ثابت VSymEnt* ثابت libSymp = m_mods.rootp()->findIdFallback(libname);
        اگر (!libSymp) بازگشت nullptr;
        ثابت VSymEnt* ثابت foundp = libSymp->findIdFallback(modName);
        بازگشت foundp ? VN_AS(foundp->nodep(), گره_Module) : nullptr;
    }
    Ast_گره_Module* findModuleSym(ثابت string& modName, ثابت string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_گره_Module* foundp;
        ثابت std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        ثابت auto itCellUseList = m_state.m_uselistCell.find(fullName);
        اگر (itCellUseList != m_state.m_uselistCell.end()) {
            برای (auto ثابت& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                اگر (foundp) بازگشت foundp;
            }
        }
        // Then search cell-specific library list
        ثابت auto itCellLibList = m_state.m_liblistCell.find(fullName);
        اگر (itCellLibList != m_state.m_liblistCell.end()) {
            برای (auto ثابت& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                اگر (foundp) بازگشت foundp;
            }
        }
        // Then search IEEE config default liblist
        برای (auto ثابت& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            اگر (foundp) بازگشت foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        اگر (foundp) بازگشت foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        بازگشت foundp;
    }

    Ast_گره_Module* resolveModule(Ast_گره* nodep, ثابت string& modName, ثابت string& libname) {
        Ast_گره_Module* modp = findModuleSym(modName, libname);
        اگر (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ثابت string prettyName = Ast_گره::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, درست, نادرست, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            اگر (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        بازگشت modp;
    }

    ایستا تهی removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        اگر (!v3Global.opt.بالاترین_Module().empty()) بازگشت;
        برای (Ast_گره_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گره_Module)) {
            اگر (!nodep->inLibrary()) بازگشت;
        }
        برای (Ast_گره_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گره_Module)) {
            nodep->inLibrary(نادرست);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, ثابت std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string بالاترین;
        منطقی بالاترین_Found = نادرست;
        std::getline(ss, بالاترین, '.');
        برای (auto ثابت& pair : m_state.m_designs) {
            اگر (بالاترین == pair.second) {
                بالاترین_Found = درست;
                بشکن;
            }
        }
        اگر (!بالاترین_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            بازگشت nullptr;
        }

        ثابت V3GraphVertex* vtx = m_بالاترین_Vertexp;
        ثابت CellEdge* finalEdgep = nullptr;
        std::string seg;
        وقتی (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            برای (ثابت V3GraphEdge& edge : vtx->outEdges()) {
                اگر (ثابت CellEdge* ثابت cedgep = edge.cast<CellEdge>()) {
                    اگر (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->بالاترین();
                        finalEdgep = cedgep;
                        بشکن;
                    }
                }
            }
            اگر (!finalEdgep) بازگشت nullptr;
        }

        بازگشت finalEdgep->cellp();
    }

    تهی assignParamNumbers(AstPin* ثابت firstPinp) {
        برای (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(درست);
            اگر (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_رقم()));
        }
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_گره_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        برای (auto ثابت& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            اگر (!cellp) ادامه;
            برای (auto ثابت& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                اگر (modp) {
                    cellp->modp(modp);
                    بشکن;
                }
            }
        }

        // Search uselists for each instance
        برای (auto ثابت& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            برای (auto ثابت& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                اگر (modp) {
                    cellp->modp(modp);
                    بشکن;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        اگر (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (ثابت LinkCellsVertex* ثابت vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_گره_Module* ثابت vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (ثابت LinkCellsVertex* ثابت vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_گره_Module* ثابت vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        اگر (v3Global.opt.بالاترین_Module() != "" && !m_بالاترین_Vertexp) {
            VSpellCheck spell;
            برای (V3GraphVertex& vtx : m_graph.vertices()) {
                اگر (ثابت LinkCellsVertex* ثابت vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_گره_Module* ثابت vmodp = vvertexp->modp();
                    اگر (VN_IS(vmodp, Module)) spell.افزودن_Candidate(vmodp->prettyName());
                }
            }
            ثابت string suggest
                = spell.best_Candidate_پیام(Ast_گره::prettyName(v3Global.opt.بالاترین_Module()));
            v3error("Specified --top-module '"
                    << Ast_گره::prettyName(v3Global.opt.بالاترین_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    تهی visit(Ast_گره_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            اگر (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, نادرست);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            اگر (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ثابت auto itFoundPair = m_declfnWarned.درج(nodep->fileline()->filename());
                اگر (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            اگر (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(درست);  // Interfaces can't be at top, unless asked
            }

            auto ثابت& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ثابت منطقی بالاترین_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            اگر (بالاترین_Match) {
                m_بالاترین_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(نادرست);  // Safer to make sure it doesn't disappear
            }
            اگر (v3Global.opt.بالاترین_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !بالاترین_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                اگر (!m_libVertexp) m_libVertexp = جدید LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, نادرست);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_درخت();
        }
    }

    تهی visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        اگر (modp) {
            اگر (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                اگر (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, نادرست);
                اگر (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } وگرنه اگر (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } وگرنه {
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
        اگر (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    تهی visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        اگر (!nodep->packagep()) {
            Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            اگر (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            اگر (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                بازگشت;
            }
        }
    }
    تهی visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        اگر (!nodep->packagep()) {
            Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            اگر (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            اگر (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                بازگشت;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, نادرست);
    }

    تهی visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        اگر (modp) {
            Ast_گره* ثابت cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    تهی visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ثابت منطقی cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        اگر (nodep->user1p() == m_modp) بازگشت;  // AstBind and AstNodeModule may call a cell twice
        اگر (nodep->modName() == m_orig_بالاترین_Module_Name) {
            اگر (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } وگرنه {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
        }
        nodep->user1p(m_modp);
        //
        اگر (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_گره_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            اگر (cellmodp) {
                اگر (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(درست);
                    nodep->recursive(درست);
                    اگر (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_گره_Module* otherModp = VN_CAST(cellmodp->user2p(), گره_Module);
                        اگر (!otherModp) {
                            otherModp = cellmodp->clone_درخت(نادرست);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(درست);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, نادرست);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } وگرنه {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } وگرنه {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, نادرست, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        اگر (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            افزودن_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        اگر (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            افزودن_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        منطقی pinStar = نادرست;
        منطقی pinDotName = نادرست;
        برای (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            اگر (pinp->svDotName()) pinDotName = درست;
            اگر (pinp->dotStar()) {
                اگر (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = درست;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_درخت(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        برای (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_رقم()));
        }
        assignParamNumbers(nodep->paramsp());
        اگر (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            برای (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                اگر (!pinp->exprp()) {
                    اگر (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } وگرنه {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.درج(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            برای (Ast_گره* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                اگر (ثابت AstPort* ثابت portp = VN_CAST(portnodep, Port)) {

                    اگر (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_رقم())) == ports.end()) {
                        اگر (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ثابت newp
                                = جدید AstPin{nodep->fileline(), 0, portp->name(),
                                             جدید AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(درست);
                            newp->svImplicit(درست);
                            nodep->addPinsp(newp);
                        } وگرنه {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ثابت AstVar* portp_varp = nullptr;
                            برای (Ast_گره* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                اگر (ثابت AstVar* ثابت varp = VN_CAST(module_stmtsp, Var)) {
                                    اگر (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        بشکن;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            اگر (portp_varp && portp_varp->is_درون_داد() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ثابت newp
                                    = جدید AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } وگرنه {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ثابت newp
                                    = جدید AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        اگر (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            اگر (!nodep->hasIfaceVar()) {
                ثابت string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ثابت idtypep = جدید AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                اگر (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    برای (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.افزودن_back(rp);
                    }
                    Ast_گره_DType* dtp = idtypep;
                    برای (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = جدید AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_درخت(نادرست)};
                    }
                    varp = جدید AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } وگرنه {
                    varp = جدید AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(درست);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(درست);
            }
        }
        اگر (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    تهی visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        اگر (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_گره_Module* ثابت varModp = findModuleSym(nodep->name(), m_modp->libname());
            اگر (AstIface* ثابت ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, نادرست);
            }
        }
    }
    تهی visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    تهی visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    // METHODS
    تهی readModNames() {
        // mangled_name, BlockOptions
        ثابت V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ثابت auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.بالاترین_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        برای (Ast_گره_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گره_Module);
            اگر (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ثابت Ast_گره_Module* ثابت libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            ثابت Ast_گره_Module* ثابت globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            اگر (libFoundp && libFoundp == nodep) {
                // Ok
            } وگرنه اگر (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } وگرنه اگر (libFoundp) {
                اگر (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه اگر (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ثابت string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                درج_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                درج_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } وگرنه {
                درج_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                درج_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

عمومی:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        اگر (v3Global.opt.hierChild()) {
            ثابت V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.بالاترین_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            ثابت V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.بالاترین_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_بالاترین_Module_Name = hierIt->second.origName();
        } وگرنه {
            m_orig_بالاترین_Module_Name = v3Global.opt.بالاترین_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        اگر (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

تهی V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
