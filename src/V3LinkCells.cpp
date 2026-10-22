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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkCells.h"

#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Parse.h"
#અન્તર્ભાવ "V3SymTable.h"

#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

વર્ગ LinkCellsGraph final : જાહેર V3Graph {
જાહેર:
    LinkCellsGraph() = મૂળભૂત;
    ~LinkCellsGraph() override = મૂળભૂત;
    શૂન્ય loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) override;
};

વર્ગ LinkCellsVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(LinkCellsVertex, V3GraphVertex)
    Ast_ગાંઠ_Module* અચલ m_modp;

જાહેર:
    LinkCellsVertex(V3Graph* graphp, Ast_ગાંઠ_Module* modp)
        : V3GraphVertex{graphp}
        , m_modp{modp} {}
    ~LinkCellsVertex() override = મૂળભૂત;
    Ast_ગાંઠ_Module* modp() અચલ VL_MT_STABLE { પાછા m_modp; }
    string name() અચલ override VL_MT_STABLE { પાછા cvtToHex(modp()) + ' ' + modp()->name(); }
    File_લીટી* fileline() અચલ override { પાછા modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() અચલ override {
        પાછા m_modp->recursiveClone() ? (1 + v3Global.opt.moduleRecursionDepth()) : 1;
    }
};

વર્ગ LibraryVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(LibraryVertex, V3GraphVertex)
જાહેર:
    explicit LibraryVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~LibraryVertex() override = મૂળભૂત;
    string name() અચલ override VL_MT_STABLE { પાછા "*LIBRARY*"; }
};

વર્ગ CellEdge final : જાહેર V3GraphEdge {
    VL_RTTI_IMPL(CellEdge, V3GraphEdge)
    AstCell* અચલ m_cellp;

જાહેર:
    CellEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight, તાર્કિક cutable,
             AstCell* cellp)
        : V3GraphEdge{graphp, fromp, ટોચ, weight, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() અચલ { પાછા m_cellp; }
    string name() અચલ override VL_MT_STABLE {
        પાછા cellp() ? cvtToHex(cellp()) + ' ' + cellp()->name() : "";
    }
};

શૂન્ય LinkCellsGraph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    જો (અચલ LinkCellsVertex* અચલ vvertexp = vertexp->cast<LinkCellsVertex>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warnMore()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } નહીંતર {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
સંરચના LinkCellsState final {
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

વર્ગ LinkConfigsVisitor final : જાહેર VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    તાર્કિક m_is_ટોચ = ખોટું;  // Whether we're in the top-level config
    તાર્કિક m_isDefault = ખોટું;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = nullptr;  // Current dot being processed

    // VISITORS
    શૂન્ય visit(AstConfig* nodep) override {
        VL_RESTORER(m_is_ટોચ);
        અચલ auto& fullName
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_ટોચ = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.end(), fullName),
            m_state.m_designs.end());
        iterateChildren(nodep);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    શૂન્ય visit(AstConfigCell* nodep) override {
        જો (m_is_ટોચ) m_state.m_designs.emplace_back(nodep->libname(), nodep->cellname());
    }

    શૂન્ય visit(AstConfigRule* nodep) override {
        જો (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = સાચું;
            iterateAndNextNull(nodep->usep());
        } નહીંતર જો (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterateAndNextNull(nodep->usep());
        } નહીંતર {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterateAndNextNull(nodep->cellp());
            }
            iterateAndNextNull(nodep->usep());
        }
    }

    શૂન્ય visit(AstDot* nodep) override { iterateChildren(nodep); }

    શૂન્ય visit(AstParseRef* nodep) override {
        જો (m_isDefault) {
            m_state.m_liblistDefault.emplace_back(nodep->name());
        } નહીંતર જો (!m_cell.empty()) {
            m_state.m_liblistCell[m_cell].emplace_back(nodep->name());
        } નહીંતર જો (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } નહીંતર જો (!m_hierInst.empty()) {
            m_state.m_liblistInst[m_hierInst].emplace_back(nodep->name());
        }
    }

    શૂન્ય visit(AstConfigUse* nodep) override {
        જો (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } નહીંતર જો (!m_cell.empty()) {
            m_state.m_uselistCell[m_cell].emplace_back(nodep->libname(), nodep->cellname());
        } નહીંતર જો (m_dotp) {
            m_hierInst += m_hierInst.empty() ? nodep->name() : '.' + nodep->name();
        } નહીંતર જો (!m_hierInst.empty()) {
            m_state.m_uselistInst[m_hierInst].emplace_back(nodep->libname(), nodep->cellname());
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        જો (!m_state.m_designs.size() && !v3Global.opt.ટોચ_Module().empty()) {
            m_state.m_designs.emplace_back(v3Global.opt.work(), v3Global.opt.ટોચ_Module());
        }
        iterate(nodep);
    }
};

વર્ગ LinkCellsVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;

    // STATE
    VInFilter* અચલ m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstVar* m_varp = nullptr;  // Current variable
    VSymGraph m_mods;  // Symbol table of all module names
    LinkCellsGraph m_graph;  // Linked graph of all cell interconnects
    LibraryVertex* m_libVertexp = nullptr;  // Vertex at root of all libraries
    અચલ V3GraphVertex* m_ટોચ_Vertexp = nullptr;  // Vertex of top module
    std::unordered_set<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_ટોચ_Module_Name;  // original name of the top module
    પૂર્ણાંક m_modDepth = 0;  // Depth of the current module

    // METHODS
    V3GraphVertex* vertex(Ast_ગાંઠ_Module* nodep) {
        // Return corresponding vertex for this module
        જો (!nodep->user1p()) nodep->user1p(નવું LinkCellsVertex{&m_graph, nodep});
        પાછા nodep->user1u().toGraphVertex();
    }
    શૂન્ય newEdge(V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight, તાર્કિક cuttable) {
        અચલ V3GraphEdge* અચલ edgep = નવું V3GraphEdge{&m_graph, fromp, ટોચ, weight, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << ટોચ->name());
    }
    શૂન્ય cellEdge(V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight, તાર્કિક cuttable,
                  AstCell* cellp) {
        અચલ V3GraphEdge* અચલ edgep
            = નવું CellEdge{&m_graph, fromp, ટોચ, weight, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << ટોચ->name());
    }
    શૂન્ય ઉમેરો_Mod_In_Lib(અચલ string& name, અચલ string& libname, Ast_ગાંઠ_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->findIdFlat(libname);
        જો (!libSymp)
            libSymp = m_mods.rootp()->ઉમેરો(libname, નવું VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->ઉમેરો(name, નવું VSymEnt{&m_mods, nodep});
    }

    Ast_ગાંઠ_Module* findModuleLibSym(અચલ string& modName, અચલ string& libname) {
        // Given module name and library name, find within that exact library
        અચલ VSymEnt* અચલ libSymp = m_mods.rootp()->findIdFallback(libname);
        જો (!libSymp) પાછા nullptr;
        અચલ VSymEnt* અચલ foundp = libSymp->findIdFallback(modName);
        પાછા foundp ? VN_AS(foundp->nodep(), ગાંઠ_Module) : nullptr;
    }
    Ast_ગાંઠ_Module* findModuleSym(અચલ string& modName, અચલ string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_ગાંઠ_Module* foundp;
        અચલ std::string fullName = libname + "." + modName;
        // First search cell-specific use list
        અચલ auto itCellUseList = m_state.m_uselistCell.find(fullName);
        જો (itCellUseList != m_state.m_uselistCell.end()) {
            માટે (auto અચલ& u : itCellUseList->second) {
                foundp = findModuleLibSym(u.second, u.first);
                જો (foundp) પાછા foundp;
            }
        }
        // Then search cell-specific library list
        અચલ auto itCellLibList = m_state.m_liblistCell.find(fullName);
        જો (itCellLibList != m_state.m_liblistCell.end()) {
            માટે (auto અચલ& l : itCellLibList->second) {
                foundp = findModuleLibSym(modName, l);
                જો (foundp) પાછા foundp;
            }
        }
        // Then search IEEE config default liblist
        માટે (auto અચલ& l : m_state.m_liblistDefault) {
            foundp = findModuleLibSym(modName, l);
            જો (foundp) પાછા foundp;
        }
        // Then search local library
        foundp = findModuleLibSym(modName, libname);
        જો (foundp) પાછા foundp;
        // Then search global
        foundp = findModuleLibSym(modName, "__GLOBAL");
        પાછા foundp;
    }

    Ast_ગાંઠ_Module* resolveModule(Ast_ગાંઠ* nodep, અચલ string& modName, અચલ string& libname) {
        Ast_ગાંઠ_Module* modp = findModuleSym(modName, libname);
        જો (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            અચલ string prettyName = Ast_ગાંઠ::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, સાચું, ખોટું, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            readModNames();
            // Check again
            modp = findModuleSym(modName, libname);
            જો (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        પાછા modp;
    }

    સ્થિર શૂન્ય removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        જો (!v3Global.opt.ટોચ_Module().empty()) પાછા;
        માટે (Ast_ગાંઠ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ગાંઠ_Module)) {
            જો (!nodep->inLibrary()) પાછા;
        }
        માટે (Ast_ગાંઠ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ગાંઠ_Module)) {
            nodep->inLibrary(ખોટું);
        }
    }

    AstCell* findCellByHier(AstNetlist* nodep, અચલ std::string& hierPath) {
        std::stringstream ss(hierPath);
        std::string ટોચ;
        તાર્કિક ટોચ_Found = ખોટું;
        std::getline(ss, ટોચ, '.');
        માટે (auto અચલ& pair : m_state.m_designs) {
            જો (ટોચ == pair.second) {
                ટોચ_Found = સાચું;
                તોડો;
            }
        }
        જો (!ટોચ_Found) {
            nodep->v3error("Can't find top-level module for instance path: '" << hierPath << "'");
            V3Error::abortIfErrors();
            પાછા nullptr;
        }

        અચલ V3GraphVertex* vtx = m_ટોચ_Vertexp;
        અચલ CellEdge* finalEdgep = nullptr;
        std::string seg;
        જ્યારે (std::getline(ss, seg, '.')) {
            finalEdgep = nullptr;
            માટે (અચલ V3GraphEdge& edge : vtx->outEdges()) {
                જો (અચલ CellEdge* અચલ cedgep = edge.cast<CellEdge>()) {
                    જો (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->ટોચ();
                        finalEdgep = cedgep;
                        તોડો;
                    }
                }
            }
            જો (!finalEdgep) પાછા nullptr;
        }

        પાછા finalEdgep->cellp();
    }

    શૂન્ય assignParamNumbers(AstPin* અચલ firstPinp) {
        માટે (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(સાચું);
            જો (pinp->name() == "") pinp->name("__paramNumber" + cvtToStr(pinp->pin_અંક()));
        }
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        readModNames();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_ગાંઠ_Module* modp = nullptr;
        AstCell* cellp = nullptr;

        // Search liblists for each instance
        માટે (auto અચલ& pair : m_state.m_liblistInst) {
            cellp = findCellByHier(nodep, pair.first);
            જો (!cellp) ચાલુ;
            માટે (auto અચલ& libname : pair.second) {
                modp = findModuleLibSym(cellp->modName(), libname);
                જો (modp) {
                    cellp->modp(modp);
                    તોડો;
                }
            }
        }

        // Search uselists for each instance
        માટે (auto અચલ& pair : m_state.m_uselistInst) {
            cellp = findCellByHier(nodep, pair.first);
            માટે (auto અચલ& u : pair.second) {
                modp = findModuleLibSym(u.second, u.first);
                જો (modp) {
                    cellp->modp(modp);
                    તોડો;
                }
            }
        }

        // Find levels in graph
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        જો (dumpGraphLevel()) m_graph.dumpDotFilePrefixed("linkcells");
        m_graph.rank();
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            જો (અચલ LinkCellsVertex* અચલ vvertexp = vtx.cast<LinkCellsVertex>()) {
                Ast_ગાંઠ_Module* અચલ vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_graph.rankMin();
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            જો (અચલ LinkCellsVertex* અચલ vvertexp = vtx.cast<LinkCellsVertex>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_ગાંઠ_Module* અચલ vmodp = vvertexp->modp();
                vmodp->depth(vvertexp->rank() + 1);
            }
        }
        જો (v3Global.opt.ટોચ_Module() != "" && !m_ટોચ_Vertexp) {
            VSpellCheck spell;
            માટે (V3GraphVertex& vtx : m_graph.vertices()) {
                જો (અચલ LinkCellsVertex* અચલ vvertexp = vtx.cast<LinkCellsVertex>()) {
                    Ast_ગાંઠ_Module* અચલ vmodp = vvertexp->modp();
                    જો (VN_IS(vmodp, Module)) spell.ધકેલો_Candidate(vmodp->prettyName());
                }
            }
            અચલ string suggest
                = spell.best_Candidate_સંદેશ(Ast_ગાંઠ::prettyName(v3Global.opt.ટોચ_Module()));
            v3error("Specified --top-module '"
                    << Ast_ગાંઠ::prettyName(v3Global.opt.ટોચ_Module())
                    << "' was not found in design.\n"
                    << (suggest.empty() ? "" : V3Error::warnMore() + suggest));
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_modDepth);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            જો (m_modp) newEdge(vertex(m_modp), vertex(nodep), 1, ખોટું);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            vertex(m_modp);
            ++m_modDepth;
            UINFO(4, "Link Module: " << nodep);
            જો (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, NotFoundModule) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_modDepth == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                અચલ auto itFoundPair = m_declfnWarned.ઉમેરો(nodep->fileline()->filename());
                જો (itFoundPair.second) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            જો (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->inLibrary(સાચું);  // Interfaces can't be at top, unless asked
            }

            auto અચલ& fullName
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            અચલ તાર્કિક ટોચ_Match
                = std::find(m_state.m_designs.begin(), m_state.m_designs.end(), fullName)
                  != m_state.m_designs.end();
            જો (ટોચ_Match) {
                m_ટોચ_Vertexp = vertex(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->inLibrary(ખોટું);  // Safer to make sure it doesn't disappear
            }
            જો (v3Global.opt.ટોચ_Module().empty()
                    ? nodep->inLibrary()  // Library cells are lower
                    : !ટોચ_Match) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                જો (!m_libVertexp) m_libVertexp = નવું LibraryVertex{&m_graph};
                newEdge(m_libVertexp, vertex(nodep), 1, ખોટું);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_વૃક્ષ();
        }
    }

    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_ગાંઠ_Module* અચલ modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        જો (modp) {
            જો (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                જો (!nodep->isVirtual()) newEdge(vertex(m_modp), vertex(modp), 1, ખોટું);
                જો (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } નહીંતર જો (VN_IS(modp, NotFoundModule)) {  // Will error out later
            } નહીંતર {
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
        જો (m_varp && !nodep->isVirtual()) m_varp->setIfaceRef();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    શૂન્ય visit(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        જો (!nodep->packagep()) {
            Ast_ગાંઠ_Module* અચલ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            જો (AstPackage* અચલ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            જો (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                પાછા;
            }
        }
    }
    શૂન્ય visit(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        જો (!nodep->packagep()) {
            Ast_ગાંઠ_Module* અચલ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            જો (AstPackage* અચલ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            જો (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                પાછા;
            }
        }
        newEdge(vertex(m_modp), vertex(nodep->packagep()), 1, ખોટું);
    }

    શૂન્ય visit(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_ગાંઠ_Module* અચલ modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        જો (modp) {
            Ast_ગાંઠ* અચલ cellsp = nodep->cellsp()->unlinkFrBackWithNext();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->addStmtsp(cellsp);
            iterateAndNextNull(cellsp);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    શૂન્ય visit(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        અચલ તાર્કિક cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        જો (nodep->user1p() == m_modp) પાછા;  // AstBind and AstNodeModule may call a cell twice
        જો (nodep->modName() == m_orig_ટોચ_Module_Name) {
            જો (v3Global.opt.hierChild() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } નહીંતર {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
        }
        nodep->user1p(m_modp);
        //
        જો (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_ગાંઠ_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            જો (cellmodp) {
                જો (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(સાચું);
                    nodep->recursive(સાચું);
                    જો (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_ગાંઠ_Module* otherModp = VN_CAST(cellmodp->user2p(), ગાંઠ_Module);
                        જો (!otherModp) {
                            otherModp = cellmodp->clone_વૃક્ષ(ખોટું);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(nullptr);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(સાચું);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->addModulesp(otherModp);
                            newEdge(vertex(cellmodp), vertex(otherModp), 1, ખોટું);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } નહીંતર {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } નહીંતર {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cellEdge(vertex(m_modp), vertex(cellmodp), 1, ખોટું, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        જો (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ધકેલો_Deletep(nodep->pinsp()->unlinkFrBackWithNext());
        }
        જો (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ધકેલો_Deletep(nodep->paramsp()->unlinkFrBackWithNext());
        }
        // Convert .* to list of pins
        તાર્કિક pinStar = ખોટું;
        તાર્કિક pinDotName = ખોટું;
        માટે (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            જો (pinp->svDotName()) pinDotName = સાચું;
            જો (pinp->dotStar()) {
                જો (pinStar) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pinStar = સાચું;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlinkFrBack()->delete_વૃક્ષ(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        માટે (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            જો (pinp->name() == "") pinp->name("__pinNumber" + cvtToStr(pinp->pin_અંક()));
        }
        assignParamNumbers(nodep->paramsp());
        જો (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_set<string> ports;  // Symbol table of all connected port names
            માટે (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                જો ((pinStar || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                જો (!pinp->exprp()) {
                    જો (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } નહીંતર {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.ઉમેરો(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            માટે (Ast_ગાંઠ* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                જો (અચલ AstPort* અચલ portp = VN_CAST(portnodep, Port)) {

                    જો (ports.find(portp->name()) == ports.end()
                        && ports.find("__pinNumber" + cvtToStr(portp->pin_અંક())) == ports.end()) {
                        જો (pinStar) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* અચલ newp
                                = નવું AstPin{nodep->fileline(), 0, portp->name(),
                                             નવું AstParseRef{nodep->fileline(), portp->name(),
                                                             nullptr, nullptr}};
                            newp->svDotName(સાચું);
                            newp->svImplicit(સાચું);
                            nodep->addPinsp(newp);
                        } નહીંતર {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            અચલ AstVar* portp_varp = nullptr;
                            માટે (Ast_ગાંઠ* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                જો (અચલ AstVar* અચલ varp = VN_CAST(module_stmtsp, Var)) {
                                    જો (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        તોડો;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            જો (portp_varp && portp_varp->is_આગત() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* અચલ newp
                                    = નવું AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            } નહીંતર {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* અચલ newp
                                    = નવું AstPin{nodep->fileline(), 0, portp->name(), nullptr};
                                nodep->addPinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        જો (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            જો (!nodep->hasIfaceVar()) {
                અચલ string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* અચલ idtypep = નવું AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(nullptr);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                જો (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::vector<AstRange*> rangesp;
                    માટે (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ધકેલો_back(rp);
                    }
                    Ast_ગાંઠ_DType* dtp = idtypep;
                    માટે (auto it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = નવું AstUnpackArrayDType{nodep->fileline(), VFlagChildDType{}, dtp,
                                                      (*it)->clone_વૃક્ષ(ખોટું)};
                    }
                    varp = નવું AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, dtp};
                } નહીંતર {
                    varp = નવું AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlagChildDType{}, idtypep};
                }
                varp->isIfaceParent(સાચું);
                nodep->addNextHere(varp);
                nodep->hasIfaceVar(સાચું);
            }
        }
        જો (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    શૂન્ય visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        જો (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_ગાંઠ_Module* અચલ varModp = findModuleSym(nodep->name(), m_modp->libname());
            જો (AstIface* અચલ ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                newEdge(vertex(m_modp), vertex(ifacep), 1, ખોટું);
            }
        }
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        assignParamNumbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    શૂન્ય visit(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterateAndNextNull(nodep->childDTypep());
        }
        iterateAndNextNull(nodep->delayp());
        iterateAndNextNull(nodep->valuep());
        iterateAndNextNull(nodep->attrsp());
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    // METHODS
    શૂન્ય readModNames() {
        // mangled_name, BlockOptions
        અચલ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        અચલ auto hierIt = vlstd::as_const(hierBlocks).find(v3Global.opt.ટોચ_Module());
        UASSERT((hierIt != hierBlocks.end()) == !!v3Global.opt.hierChild(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        માટે (Ast_ગાંઠ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ગાંઠ_Module);
            જો (v3Global.opt.hierChild() && nodep->origName() == hierIt->second.origName()) {
                nodep->name(hierIt->first);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            અચલ Ast_ગાંઠ_Module* અચલ libFoundp
                = findModuleLibSym(nodep->origName(), nodep->libname());
            અચલ Ast_ગાંઠ_Module* અચલ globalFoundp = findModuleLibSym(nodep->name(), "__GLOBAL");
            જો (libFoundp && libFoundp == nodep) {
                // Ok
            } નહીંતર જો (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } નહીંતર જો (libFoundp) {
                જો (!(libFoundp->fileline()->warnIsOff(V3ErrorCode::MODDUP)
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
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર જો (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                અચલ string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                ઉમેરો_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                ઉમેરો_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            } નહીંતર {
                ઉમેરો_Mod_In_Lib(nodep->origName(), nodep->libname(), nodep);
                ઉમેરો_Mod_In_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

જાહેર:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        જો (v3Global.opt.hierChild()) {
            અચલ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
            UASSERT(!v3Global.opt.ટોચ_Module().empty(),
                    "top module must be explicitly specified in hierarchical mode");
            અચલ V3HierBlockOptSet::const_iterator hierIt
                = hierBlocks.find(v3Global.opt.ટોચ_Module());
            UASSERT(hierIt != hierBlocks.end(),
                    "top module must be listed in --hierarchical-block");
            m_orig_ટોચ_Module_Name = hierIt->second.origName();
        } નહીંતર {
            m_orig_ટોચ_Module_Name = v3Global.opt.ટોચ_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        જો (debug() >= 5 || dumpGraphLevel() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

શૂન્ય V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
