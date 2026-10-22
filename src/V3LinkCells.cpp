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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LinkCells.h"

#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Parse.h"
#அந்தர்பாவ "V3SymTable.h"

#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

வகுப்பு LinkCellsGraph final : பொது V3Graph {
பொது:
    LinkCellsGraph() = இயல்பு;
    ~LinkCellsGraph() override = இயல்பு;
    வெறுமை loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

வகுப்பு LinkCellsVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_முனை_Module* மாறிலி m_modp;

பொது:
    LinkCellsVertex(V3Graph* graphp, Ast_முனை_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = இயல்பு;
    Ast_முனை_Module* modp() மாறிலி VL_MT_STABLE { திரும்பு m_modp; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு cvtToHex(modp()) + ' ' + modp()->name(); }
    File_வரி* fileline() மாறிலி override { திரும்பு modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() மாறிலி override {
        திரும்பு m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

வகுப்பு LibraryVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
பொது:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = இயல்பு;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "*LIBRARY*"; }
};

வகுப்பு CellEdge final : பொது V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* மாறிலி m_cellp;

பொது:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight, தருக்க cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, உச்சி, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() மாறிலி { திரும்பு m_cellp; }
    string name() மாறிலி override VL_MT_STABLE {
        திரும்பு cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

வெறுமை LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    எனில் (மாறிலி LinkCellsVertex* மாறிலி vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } இல்லையெனில் {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
கட்டமைப்பு LinkCellsState final {
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

வகுப்பு LinkConfigsVisitor final : பொது VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    தருக்க m_is_உச்சி = பொய்;  // Whether we're in the top-level config
    தருக்க m_isDefault = பொய்;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    வெறுமை visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_உச்சி);
        மாறிலி auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_உச்சி = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    வெறுமை visit(AstConfigCell* nodep) override {
        எனில் (m_is_உச்சி) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    வெறுமை visit(AstConfigRule* nodep) override {
        எனில் (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = உண்மை;
            iterateAndNextNull(nodep->usep());
        } இல்லையெனில் எனில் (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } இல்லையெனில் {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    வெறுமை visit(AstDot* nodep) override { iterateChildren(nodep); }

    வெறுமை visit(AstParseRef* nodep) override {
        எனில் (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } இல்லையெனில் எனில் (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } இல்லையெனில் எனில் (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } இல்லையெனில் எனில் (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    வெறுமை visit(AstConfigUse* nodep) override {
        எனில் (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } இல்லையெனில் எனில் (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } இல்லையெனில் எனில் (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } இல்லையெனில் எனில் (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        எனில் (!m_state.m_designs.size() && !v3Global.opt.உச்சி_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.உச்சி_Module());
        }
        iterate(nodep);
    }
};

வகுப்பு LinkCellsVisitor final : பொது VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;

    // STATE
    VInFilter* மாறிலி m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    மாறிலி V3GraphVertex* m_உச்சி_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_உச்சி_Module_Name;  // original name of the top module
    முழு m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_முனை_Module* nodep) {
        // Return corresponding vertex for this module
        எனில் (!nodep->user1p()) nodep->user1p(புதிய LinkCellsVertex{&m_graph, nodep});
        திரும்பு nodep->user1u().toGraphVertex();
    }
    வெறுமை newEdge(V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight, தருக்க cuttable) {
        மாறிலி V3GraphEdge* மாறிலி edgep = புதிய V3GraphEdge{&m_graph, fromp, உச்சி, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << உச்சி->name());
    }
    வெறுமை cellEdge(V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight, தருக்க cuttable,
                  AstCell* cellp) {
        மாறிலி V3GraphEdge* மாறிலி edgep
            = புதிய CellEdge{&m_graph, fromp, உச்சி, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << உச்சி->name());
    }
    வெறுமை செருகு_Mod_In_Lib(மாறிலி string& name, மாறிலி string& libname, Ast_முனை_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        எனில் (!libSymp)
            libSymp = m_mods.rootp()->செருகு(libname, புதிய VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->செருகு(name, புதிய VSymEnt{&m_mods, nodep});
    }

    Ast_முனை_Module* findModuleLibSym(மாறிலி string& modName, மாறிலி string& libname) {
        // Given module name and library name, find within that exact library
        மாறிலி VSymEnt* மாறிலி libSymp = m_mods.rootp()->findIdFallback(libname);
        எனில் (!libSymp) திரும்பு nullptr;
        மாறிலி VSymEnt* மாறிலி foundp = libSymp->findIdFallback(modName);
        திரும்பு foundp ? VN_AS(foundp->nodep(), முனை_Module) : nullptr;
    }
    Ast_முனை_Module* findModuleSym(மாறிலி string& modName, மாறிலி string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_முனை_Module* foundp;
        மாறிலி std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        மாறிலி auto itCellUseList = m_state.m_uselistCell.find(fullName);
        எனில் (itCellUseList != m_state.m_uselistCell.end()) {
            க்கு (auto மாறிலி& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                எனில் (foundp) திரும்பு foundp;
            }
        }
        // Then search cell-specific library list
        மாறிலி auto itCellLibList = m_state.m_liblistCell.find(fullName);
        எனில் (itCellLibList != m_state.m_liblistCell.end()) {
            க்கு (auto மாறிலி& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                எனில் (foundp) திரும்பு foundp;
            }
        }
        // Then search IEEE config default liblist
        க்கு (auto மாறிலி& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            எனில் (foundp) திரும்பு foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        எனில் (foundp) திரும்பு foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        திரும்பு foundp;
    }

    Ast_முனை_Module* resolveModule(Ast_முனை* nodep, மாறிலி string& modName, மாறிலி string& libname) {
        Ast_முனை_Module* modp = findModuleSym(modName, libname);
        எனில் (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            மாறிலி string prettyName = Ast_முனை::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, உண்மை, பொய், m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            எனில் (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        திரும்பு modp;
    }

    நிலையான வெறுமை removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        எனில் (!v3Global.opt.உச்சி_Module().empty()) திரும்பு;
        க்கு (Ast_முனை_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), முனை_Module)) {
            எனில் (!nodep->inLibrary()) திரும்பு;
        }
        க்கு (Ast_முனை_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), முனை_Module)) {
            nodep->inLibrary(பொய்);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, மாறிலி std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string உச்சி;
        தருக்க உச்சி_Found = பொய்;
        std::getline(ss, உச்சி, '.');
        க்கு (auto மாறிலி& pair : m_state.m_designs) {
            எனில் (உச்சி == pair.second) {
                உச்சி_Found = உண்மை;
                நிறுத்து;
            }
        }
        எனில் (!உச்சி_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            திரும்பு nullptr;
        }

        மாறிலி V3GraphVertex* vtx = m_உச்சி_Vertexp;
        மாறிலி CellEdge* finalEdgep = nullptr;
        std::string seg;
        போது (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            க்கு (மாறிலி V3GraphEdge& edge : vtx->outEdges()) {
                எனில் (மாறிலி CellEdge* மாறிலி cedgep = edge.cast<CellEdge>()) {
                    எனில் (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->உச்சி();
                        finalEdgep = cedgep;
                        நிறுத்து;
                    }
                }
            }
            எனில் (!finalEdgep) திரும்பு nullptr;
        }

        திரும்பு finalEdgep->cellp();
    }

    வெறுமை assignParamNumbers(AstPin* மாறிலி firstPinp) {
        க்கு (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(உண்மை);
            எனில் (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_இலக்கம்()));
        }
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_முனை_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        க்கு (auto மாறிலி& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            எனில் (!cellp) தொடர்;
            க்கு (auto மாறிலி& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                எனில் (modp) {
                    cellp->modp(modp);
                    நிறுத்து;
                }
            }
        }

        // Search uselists for each instance
        க்கு (auto மாறிலி& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            க்கு (auto மாறிலி& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                எனில் (modp) {
                    cellp->modp(modp);
                    நிறுத்து;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        எனில் (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (மாறிலி LinkCellsVertex* மாறிலி vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_முனை_Module* மாறிலி vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (மாறிலி LinkCellsVertex* மாறிலி vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_முனை_Module* மாறிலி vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        எனில் (v3Global.opt.உச்சி_Module() != "" && !m_உச்சி_Vertexp) {
            VSpellCheck spell;
            க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
                எனில் (மாறிலி LinkCellsVertex* மாறிலி vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_முனை_Module* மாறிலி vmodp = vvertexp->modp();
                    எனில் (VN_IS(vmodp, Module)) spell.தள்ளு_Candidate(vmodp->prettyName());
                }
            }
            மாறிலி string suggest
                = spell.best_Candidate_செய்தி(Ast_முனை::prettyName(v3Global.opt.உச்சி_Module()));
            v3error("Specified --top-module '"
                    << Ast_முனை::prettyName(v3Global.opt.உச்சி_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            எனில் (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, பொய்);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            எனில் (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                மாறிலி auto itFoundPair = m_declfnWarned.செருகு(nodep->fileline()->filename());
                எனில் (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            எனில் (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(உண்மை);  // Interfaces can't be at top, unless asked
            }

            auto மாறிலி& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            மாறிலி தருக்க உச்சி_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            எனில் (உச்சி_Match) {
                m_உச்சி_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(பொய்);  // Safer to make sure it doesn't disappear
            }
            எனில் (v3Global.opt.உச்சி_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !உச்சி_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                எனில் (!m_libVertexp) m_libVertexp = புதிய LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, பொய்);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_மரம்();
        }
    }

    வெறுமை visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_முனை_Module* மாறிலி modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        எனில் (modp) {
            எனில் (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                எனில் (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, பொய்);
                எனில் (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } இல்லையெனில் எனில் (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } இல்லையெனில் {
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
        எனில் (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    வெறுமை visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        எனில் (!nodep->packagep()) {
            Ast_முனை_Module* மாறிலி modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            எனில் (AstPackage* மாறிலி pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            எனில் (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                திரும்பு;
            }
        }
    }
    வெறுமை visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        எனில் (!nodep->packagep()) {
            Ast_முனை_Module* மாறிலி modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            எனில் (AstPackage* மாறிலி pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            எனில் (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                திரும்பு;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, பொய்);
    }

    வெறுமை visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_முனை_Module* மாறிலி modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        எனில் (modp) {
            Ast_முனை* மாறிலி cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    வெறுமை visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        மாறிலி தருக்க cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        எனில் (nodep->user1p() == m_modp) திரும்பு;  // AstBind and AstNodeModule may call a cell twice
        எனில் (nodep->modName() == m_orig_உச்சி_Module_Name) {
            எனில் (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } இல்லையெனில் {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
        }
        nodep->user1p(m_modp);
        //
        எனில் (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_முனை_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            எனில் (cellmodp) {
                எனில் (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(உண்மை);
                    nodep->recursive(உண்மை);
                    எனில் (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_முனை_Module* otherModp = VN_CAST(cellmodp->user2p(), முனை_Module);
                        எனில் (!otherModp) {
                            otherModp = cellmodp->clone_மரம்(பொய்);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(உண்மை);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, பொய்);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } இல்லையெனில் {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } இல்லையெனில் {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, பொய், nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        எனில் (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            தள்ளு_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        எனில் (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            தள்ளு_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        தருக்க pinStar = பொய்;
        தருக்க pinDotName = பொய்;
        க்கு (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            எனில் (pinp->svDotName()) pinDotName = உண்மை;
            எனில் (pinp->dotStar()) {
                எனில் (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = உண்மை;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_மரம்(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        க்கு (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            எனில் (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_இலக்கம்()));
        }
        assignParamNumbers(nodep->paramsp());
        எனில் (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            க்கு (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                எனில் (!pinp->exprp()) {
                    எனில் (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } இல்லையெனில் {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.செருகு(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            க்கு (Ast_முனை* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                எனில் (மாறிலி AstPort* மாறிலி portp = VN_CAST(portnodep, Port)) {

                    எனில் (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_இலக்கம்())) == ports.end()) {
                        எனில் (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* மாறிலி newp
                                = புதிய AstPin{nodep->fileline(), 0, portp->name(),
                                             புதிய AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(உண்மை);
                            newp->svImplicit(உண்மை);
                            nodep->addPinsp(newp);
                        } இல்லையெனில் {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            மாறிலி AstVar* portp_varp = nullptr;
                            க்கு (Ast_முனை* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(module_stmtsp, Var)) {
                                    எனில் (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        நிறுத்து;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            எனில் (portp_varp && portp_varp->is_உள்ளீடு() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* மாறிலி newp
                                    = புதிய AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } இல்லையெனில் {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* மாறிலி newp
                                    = புதிய AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        எனில் (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            எனில் (!nodep->hasIfaceVar()) {
                மாறிலி string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* மாறிலி idtypep = புதிய AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                எனில் (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    க்கு (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.தள்ளு_back(rp);
                    }
                    Ast_முனை_DType* dtp = idtypep;
                    க்கு (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = புதிய AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_மரம்(பொய்)};
                    }
                    varp = புதிய AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } இல்லையெனில் {
                    varp = புதிய AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(உண்மை);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(உண்மை);
            }
        }
        எனில் (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    வெறுமை visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        எனில் (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_முனை_Module* மாறிலி varModp = findModuleSym(nodep->name(), m_modp->libname());
            எனில் (AstIface* மாறிலி ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, பொய்);
            }
        }
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    வெறுமை visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

    // METHODS
    வெறுமை readModNames() {
        // mangled_name, BlockOptions
        மாறிலி V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        மாறிலி auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.உச்சி_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        க்கு (Ast_முனை_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), முனை_Module);
            எனில் (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            மாறிலி Ast_முனை_Module* மாறிலி libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            மாறிலி Ast_முனை_Module* மாறிலி globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            எனில் (libFoundp && libFoundp == nodep) {
                // Ok
            } இல்லையெனில் எனில் (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } இல்லையெனில் எனில் (libFoundp) {
                எனில் (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                மாறிலி string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                செருகு_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                செருகு_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } இல்லையெனில் {
                செருகு_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                செருகு_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

பொது:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        எனில் (v3Global.opt.hierChild()) {
            மாறிலி V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.உச்சி_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            மாறிலி V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.உச்சி_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_உச்சி_Module_Name = hierIt->second.origName();
        } இல்லையெனில் {
            m_orig_உச்சி_Module_Name = v3Global.opt.உச்சி_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        எனில் (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

வெறுமை V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
