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

ವರ್ಗ Link_Cells_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
ಸಾರ್ವಜನಿಕ:
    Link_Cells_ನಕ್ಷೆ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~Link_Cells_ನಕ್ಷೆ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ loops_ಸಂದೇಶ_Cb(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) override;
};

ವರ್ಗ Link_Cells_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Link_Cells_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;

ಸಾರ್ವಜನಿಕ:
    Link_Cells_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು_Module* modp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_modp{modp} {}
    ~Link_Cells_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    Ast_ಗಂಟು_Module* modp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_modp; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Hex(modp()) + ' ' + modp()->name(); }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_modp->recursiveClone() ? (1 + v3Global.opt.module_Recursion_ಆಳ()) : 1;
    }
};

ವರ್ಗ Library_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Library_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Library_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp} {}
    ~Library_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "*LIBRARY*"; }
};

ವರ್ಗ Cell_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(Cell_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)
    AstCell* ಸ್ಥಿರಾಂಕ m_cellp;

ಸಾರ್ವಜನಿಕ:
    Cell_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2, ತಾರ್ಕಿಕ cutable,
             AstCell* cellp)
        : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, ತೂಕ_2, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cellp; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ cellp() ? cvt_ಗಮ್ಯ_Hex(cellp()) + ' ' + cellp()->name() : "";
    }
};

ಶೂನ್ಯ Link_Cells_ನಕ್ಷೆ::loops_ಸಂದೇಶ_Cb(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) {
    ಆದರೆ (ಸ್ಥಿರಾಂಕ Link_Cells_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vertexp->cast<Link_Cells_ಶೃಂಗ>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warn_ಇನ್ನಷ್ಟು()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } ಇಲ್ಲದಿದ್ದರೆ {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
ರಚನೆ LinkCellsState final {
    // Set of possible top module names from command line and configs
    std::ಸದಿಶ<std::pair<std::string, std::string>> m_designs;
    // Default library lists to search
    std::ಸದಿಶ<std::string> m_liblistDefault;
    // Library lists for specific cells
    std::unordered_map<std::string, std::ಸದಿಶ<std::string>> m_liblistCell;
    // Use list for specific cells (libname, cellname)
    std::unordered_map<std::string, std::ಸದಿಶ<std::pair<std::string, std::string>>>
        m_uselistCell;
    // Library lists for specific insts
    std::unordered_map<std::string, std::ಸದಿಶ<std::string>> m_liblistInst;
    // Use list for specific insts (libname, cellname)
    std::unordered_map<std::string, std::ಸದಿಶ<std::pair<std::string, std::string>>>
        m_uselistInst;
};

ವರ್ಗ LinkConfigsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    ತಾರ್ಕಿಕ m_is_ತುದಿ = ಸುಳ್ಳು;  // Whether we're in the top-level config
    ತಾರ್ಕಿಕ m_isDefault = ಸುಳ್ಳು;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = ಶೂನ್ಯಸೂಚಕ;  // Current dot being processed

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstConfig* nodep) override {
        VL_RESTORER(m_is_ತುದಿ);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ತುಂಬಿದ_Name
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_ತುದಿ = std::ಕಂಡುಹಿಡಿ(m_state.m_designs.begin(), m_state.m_designs.ಕೊನೆ(), ತುಂಬಿದ_Name)
                  != m_state.m_designs.ಕೊನೆ();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.ಕೊನೆ(), ತುಂಬಿದ_Name),
            m_state.m_designs.ಕೊನೆ());
        iterateChildren(nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstConfigCell* nodep) override {
        ಆದರೆ (m_is_ತುದಿ) m_state.m_designs.emplace_ಹಿಂಭಾಗ(nodep->libname(), nodep->cellname());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Config_ನಿಯಮ* nodep) override {
        ಆದರೆ (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Null(nodep->usep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_ಮುಂದಿನ_Null(nodep->usep());
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_ಮುಂದಿನ_Null(nodep->cellp());
            }
            iterate_And_ಮುಂದಿನ_Null(nodep->usep());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDot* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(AstParseRef* nodep) override {
        ಆದರೆ (m_isDefault) {
            m_state.m_liblistDefault.emplace_ಹಿಂಭಾಗ(nodep->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_cell.ಖಾಲಿಗಳು()) {
            m_state.m_liblistCell[m_cell].emplace_ಹಿಂಭಾಗ(nodep->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_dotp) {
            m_hierInst += m_hierInst.ಖಾಲಿಗಳು() ? nodep->name() : '.' + nodep->name();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_hierInst.ಖಾಲಿಗಳು()) {
            m_state.m_liblistInst[m_hierInst].emplace_ಹಿಂಭಾಗ(nodep->name());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstConfigUse* nodep) override {
        ಆದರೆ (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_cell.ಖಾಲಿಗಳು()) {
            m_state.m_uselistCell[m_cell].emplace_ಹಿಂಭಾಗ(nodep->libname(), nodep->cellname());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_dotp) {
            m_hierInst += m_hierInst.ಖಾಲಿಗಳು() ? nodep->name() : '.' + nodep->name();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_hierInst.ಖಾಲಿಗಳು()) {
            m_state.m_uselistInst[m_hierInst].emplace_ಹಿಂಭಾಗ(nodep->libname(), nodep->cellname());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        ಆದರೆ (!m_state.m_designs.ಗಾತ್ರ_2() && !v3Global.opt.ತುದಿ_Module().ಖಾಲಿಗಳು()) {
            m_state.m_designs.emplace_ಹಿಂಭಾಗ(v3Global.opt.work(), v3Global.opt.ತುದಿ_Module());
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE
    VInFilter* ಸ್ಥಿರಾಂಕ m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstVar* m_varp = ಶೂನ್ಯಸೂಚಕ;  // Current variable
    VSym_ನಕ್ಷೆ m_mods;  // Symbol table of all module names
    Link_Cells_ನಕ್ಷೆ m_ನಕ್ಷೆ;  // Linked graph of all cell interconnects
    Library_ಶೃಂಗ* m_libVertexp = ಶೂನ್ಯಸೂಚಕ;  // Vertex at root of all libraries
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* m_ತುದಿ_Vertexp = ಶೂನ್ಯಸೂಚಕ;  // Vertex of top module
    std::unordered_ಗಣ<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_ತುದಿ_Module_Name;  // original name of the top module
    ಪೂರ್ಣಾಂಕ m_mod_ಆಳ = 0;  // Depth of the current module

    // METHODS
    V3_ನಕ್ಷೆ_ಶೃಂಗ* ಶೃಂಗ(Ast_ಗಂಟು_Module* nodep) {
        // Return corresponding vertex for this module
        ಆದರೆ (!nodep->user1p()) nodep->user1p(ಹೊಸ Link_Cells_ಶೃಂಗ{&m_ನಕ್ಷೆ, nodep});
        ಹಿಂತಿರುಗಿ nodep->user1u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ();
    }
    ಶೂನ್ಯ new_ಅಂಚು(V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2, ತಾರ್ಕಿಕ cuttable) {
        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, fromp, ತುದಿ, ತೂಕ_2, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << ತುದಿ->name());
    }
    ಶೂನ್ಯ cell_ಅಂಚು(V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2, ತಾರ್ಕಿಕ cuttable,
                  AstCell* cellp) {
        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep
            = ಹೊಸ Cell_ಅಂಚು{&m_ನಕ್ಷೆ, fromp, ತುದಿ, ತೂಕ_2, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << ತುದಿ->name());
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Mod_ಒಳ_Lib(ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname, Ast_ಗಂಟು_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->ಕಂಡುಹಿಡಿ_Id_Flat(libname);
        ಆದರೆ (!libSymp)
            libSymp = m_mods.rootp()->ಸೇರಿಸು(libname, ಹೊಸ VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->ಸೇರಿಸು(name, ಹೊಸ VSymEnt{&m_mods, nodep});
    }

    Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Module_Lib_Sym(ಸ್ಥಿರಾಂಕ string& modName, ಸ್ಥಿರಾಂಕ string& libname) {
        // Given module name and library name, find within that exact library
        ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ libSymp = m_mods.rootp()->ಕಂಡುಹಿಡಿ_Id_Fallback(libname);
        ಆದರೆ (!libSymp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = libSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(modName);
        ಹಿಂತಿರುಗಿ foundp ? VN_AS(foundp->nodep(), ಗಂಟು_Module) : ಶೂನ್ಯಸೂಚಕ;
    }
    Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Module_Sym(ಸ್ಥಿರಾಂಕ string& modName, ಸ್ಥಿರಾಂಕ string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_ಗಂಟು_Module* foundp;
        ಸ್ಥಿರಾಂಕ std::string ತುಂಬಿದ_Name = libname + "." + modName;
        // First search cell-specific use list
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ itCellUseList = m_state.m_uselistCell.ಕಂಡುಹಿಡಿ(ತುಂಬಿದ_Name);
        ಆದರೆ (itCellUseList != m_state.m_uselistCell.ಕೊನೆ()) {
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& u : itCellUseList->ಎರಡನೇ) {
                foundp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(u.ಎರಡನೇ, u.ಮೊದಲ);
                ಆದರೆ (foundp) ಹಿಂತಿರುಗಿ foundp;
            }
        }
        // Then search cell-specific library list
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ itCellLibList = m_state.m_liblistCell.ಕಂಡುಹಿಡಿ(ತುಂಬಿದ_Name);
        ಆದರೆ (itCellLibList != m_state.m_liblistCell.ಕೊನೆ()) {
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& l : itCellLibList->ಎರಡನೇ) {
                foundp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(modName, l);
                ಆದರೆ (foundp) ಹಿಂತಿರುಗಿ foundp;
            }
        }
        // Then search IEEE config default liblist
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& l : m_state.m_liblistDefault) {
            foundp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(modName, l);
            ಆದರೆ (foundp) ಹಿಂತಿರುಗಿ foundp;
        }
        // Then search local library
        foundp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(modName, libname);
        ಆದರೆ (foundp) ಹಿಂತಿರುಗಿ foundp;
        // Then search global
        foundp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(modName, "__GLOBAL");
        ಹಿಂತಿರುಗಿ foundp;
    }

    Ast_ಗಂಟು_Module* resolveModule(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& modName, ಸ್ಥಿರಾಂಕ string& libname) {
        Ast_ಗಂಟು_Module* modp = ಕಂಡುಹಿಡಿ_Module_Sym(modName, libname);
        ಆದರೆ (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ಸ್ಥಿರಾಂಕ string prettyName = Ast_ಗಂಟು::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, ನಿಜ, ಸುಳ್ಳು, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            read_Mod_ಹೆಸರುಗಳು();
            // Check again
            modp = ಕಂಡುಹಿಡಿ_Module_Sym(modName, libname);
            ಆದರೆ (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        ಹಿಂತಿರುಗಿ modp;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        ಆದರೆ (!v3Global.opt.ತುದಿ_Module().ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (Ast_ಗಂಟು_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ಗಂಟು_Module)) {
            ಆದರೆ (!nodep->ಒಳ_Library()) ಹಿಂತಿರುಗಿ;
        }
        ಗಾಗಿ (Ast_ಗಂಟು_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ಗಂಟು_Module)) {
            nodep->ಒಳ_Library(ಸುಳ್ಳು);
        }
    }

    AstCell* ಕಂಡುಹಿಡಿ_Cell_By_Hier(AstNetlist* nodep, ಸ್ಥಿರಾಂಕ std::string& hier_ಮಾರ್ಗ) {
        std::stringstream ss(hier_ಮಾರ್ಗ);
        std::string ತುದಿ;
        ತಾರ್ಕಿಕ ತುದಿ_ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
        std::getline(ss, ತುದಿ, '.');
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& pair : m_state.m_designs) {
            ಆದರೆ (ತುದಿ == pair.ಎರಡನೇ) {
                ತುದಿ_ಸಿಕ್ಕಿತು = ನಿಜ;
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (!ತುದಿ_ಸಿಕ್ಕಿತು) {
            nodep->v3error("Can't find top-level module for instance path: '" << hier_ಮಾರ್ಗ << "'");
            V3Error::abortIfErrors();
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* vtx = m_ತುದಿ_Vertexp;
        ಸ್ಥಿರಾಂಕ Cell_ಅಂಚು* finalEdgep = ಶೂನ್ಯಸೂಚಕ;
        std::string seg;
        ಆಗ (std::getline(ss, seg, '.')) {
            finalEdgep = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Cell_ಅಂಚು* ಸ್ಥಿರಾಂಕ cedgep = ಅಂಚು.cast<Cell_ಅಂಚು>()) {
                    ಆದರೆ (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->ತುದಿ();
                        finalEdgep = cedgep;
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
            ಆದರೆ (!finalEdgep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        ಹಿಂತಿರುಗಿ finalEdgep->cellp();
    }

    ಶೂನ್ಯ ನಿಯೋಜಿಸು_Param_Numbers(AstPin* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Pinp) {
        ಗಾಗಿ (AstPin* pinp = ಮೊದಲ_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(ನಿಜ);
            ಆದರೆ (pinp->name() == "") pinp->name("__paramNumber" + cvt_ಗಮ್ಯ_Str(pinp->pin_ಅಂಕೆ()));
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        read_Mod_ಹೆಸರುಗಳು();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_ಗಂಟು_Module* modp = ಶೂನ್ಯಸೂಚಕ;
        AstCell* cellp = ಶೂನ್ಯಸೂಚಕ;

        // Search liblists for each instance
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& pair : m_state.m_liblistInst) {
            cellp = ಕಂಡುಹಿಡಿ_Cell_By_Hier(nodep, pair.ಮೊದಲ);
            ಆದರೆ (!cellp) ಮುಂದುವರಿ;
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& libname : pair.ಎರಡನೇ) {
                modp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(cellp->modName(), libname);
                ಆದರೆ (modp) {
                    cellp->modp(modp);
                    ನಿಲ್ಲಿಸು;
                }
            }
        }

        // Search uselists for each instance
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& pair : m_state.m_uselistInst) {
            cellp = ಕಂಡುಹಿಡಿ_Cell_By_Hier(nodep, pair.ಮೊದಲ);
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& u : pair.ಎರಡನೇ) {
                modp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(u.ಎರಡನೇ, u.ಮೊದಲ);
                ಆದರೆ (modp) {
                    cellp->modp(modp);
                    ನಿಲ್ಲಿಸು;
                }
            }
        }

        // Find levels in graph
        m_ನಕ್ಷೆ.remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level()) m_ನಕ್ಷೆ.dumpDotFilePrefixed("linkcells");
        m_ನಕ್ಷೆ.rank();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Link_Cells_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtx.cast<Link_Cells_ಶೃಂಗ>()) {
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_ನಕ್ಷೆ.rankMin();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Link_Cells_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtx.cast<Link_Cells_ಶೃಂಗ>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ vmodp = vvertexp->modp();
                vmodp->ಆಳ(vvertexp->rank() + 1);
            }
        }
        ಆದರೆ (v3Global.opt.ತುದಿ_Module() != "" && !m_ತುದಿ_Vertexp) {
            VSpellCheck spell;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Link_Cells_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtx.cast<Link_Cells_ಶೃಂಗ>()) {
                    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ vmodp = vvertexp->modp();
                    ಆದರೆ (VN_IS(vmodp, Module)) spell.ತಳ್ಳು_Candidate(vmodp->prettyName());
                }
            }
            ಸ್ಥಿರಾಂಕ string suggest
                = spell.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(Ast_ಗಂಟು::prettyName(v3Global.opt.ತುದಿ_Module()));
            v3error("Specified --top-module '"
                    << Ast_ಗಂಟು::prettyName(v3Global.opt.ತುದಿ_Module())
                    << "' was not found in design.\n"
                    << (suggest.ಖಾಲಿಗಳು() ? "" : V3Error::warn_ಇನ್ನಷ್ಟು() + suggest));
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_mod_ಆಳ);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            ಆದರೆ (m_modp) new_ಅಂಚು(ಶೃಂಗ(m_modp), ಶೃಂಗ(nodep), 1, ಸುಳ್ಳು);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            ಶೃಂಗ(m_modp);
            ++m_mod_ಆಳ;
            UINFO(4, "Link Module: " << nodep);
            ಆದರೆ (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, Not_ಸಿಕ್ಕಿತು_Module) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_mod_ಆಳ == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it_ಸಿಕ್ಕಿತು_Pair = m_declfnWarned.ಸೇರಿಸು(nodep->fileline()->filename());
                ಆದರೆ (it_ಸಿಕ್ಕಿತು_Pair.ಎರಡನೇ) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            ಆದರೆ (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->ಒಳ_Library(ನಿಜ);  // Interfaces can't be at top, unless asked
            }

            ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& ತುಂಬಿದ_Name
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ತುದಿ_ಹೊಂದಿಕೆ
                = std::ಕಂಡುಹಿಡಿ(m_state.m_designs.begin(), m_state.m_designs.ಕೊನೆ(), ತುಂಬಿದ_Name)
                  != m_state.m_designs.ಕೊನೆ();
            ಆದರೆ (ತುದಿ_ಹೊಂದಿಕೆ) {
                m_ತುದಿ_Vertexp = ಶೃಂಗ(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->ಒಳ_Library(ಸುಳ್ಳು);  // Safer to make sure it doesn't disappear
            }
            ಆದರೆ (v3Global.opt.ತುದಿ_Module().ಖಾಲಿಗಳು()
                    ? nodep->ಒಳ_Library()  // Library cells are lower
                    : !ತುದಿ_ಹೊಂದಿಕೆ) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                ಆದರೆ (!m_libVertexp) m_libVertexp = ಹೊಸ Library_ಶೃಂಗ{&m_ನಕ್ಷೆ};
                new_ಅಂಚು(m_libVertexp, ಶೃಂಗ(nodep), 1, ಸುಳ್ಳು);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_ವೃಕ್ಷ();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        ಆದರೆ (modp) {
            ಆದರೆ (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                ಆದರೆ (!nodep->isVirtual()) new_ಅಂಚು(ಶೃಂಗ(m_modp), ಶೃಂಗ(modp), 1, ಸುಳ್ಳು);
                ಆದರೆ (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(modp, Not_ಸಿಕ್ಕಿತು_Module)) {  // Will error out later
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("Non-interface used as an interface: "
                               << nodep->ifaceNameQ() << "\n"
                               << nodep->warn_ಇನ್ನಷ್ಟು()
                                      + "... Perhaps intended an instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 23.3.2)?");
            }
        }
        iterateChildren(nodep);
        ನಿಯೋಜಿಸು_Param_Numbers(nodep->paramsp());
        // Parser didn't know what was interface, resolve now
        // For historical reasons virtual interface reference variables remain VARs
        ಆದರೆ (m_varp && !nodep->isVirtual()) m_varp->ಗಣ_Iface_Ref();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ಆದರೆ (!nodep->packagep()) {
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ಆದರೆ (AstPackage* ಸ್ಥಿರಾಂಕ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            ಆದರೆ (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                ಹಿಂತಿರುಗಿ;
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ಆದರೆ (!nodep->packagep()) {
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ಆದರೆ (AstPackage* ಸ್ಥಿರಾಂಕ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            ಆದರೆ (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                ಹಿಂತಿರುಗಿ;
            }
        }
        new_ಅಂಚು(ಶೃಂಗ(m_modp), ಶೃಂಗ(nodep->packagep()), 1, ಸುಳ್ಳು);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        ಆದರೆ (modp) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ cellsp = nodep->cellsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->ಕೂಡಿಸು_Stmtsp(cellsp);
            iterate_And_ಮುಂದಿನ_Null(cellsp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        ಆದರೆ (nodep->user1p() == m_modp) ಹಿಂತಿರುಗಿ;  // AstBind and AstNodeModule may call a cell twice
        ಆದರೆ (nodep->modName() == m_orig_ತುದಿ_Module_Name) {
            ಆದರೆ (v3Global.opt.hier_ಮಗು() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } ಇಲ್ಲದಿದ್ದರೆ {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
        nodep->user1p(m_modp);
        //
        ಆದರೆ (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_ಗಂಟು_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            ಆದರೆ (cellmodp) {
                ಆದರೆ (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(ನಿಜ);
                    nodep->recursive(ನಿಜ);
                    ಆದರೆ (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_ಗಂಟು_Module* otherModp = VN_CAST(cellmodp->user2p(), ಗಂಟು_Module);
                        ಆದರೆ (!otherModp) {
                            otherModp = cellmodp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(ಶೂನ್ಯಸೂಚಕ);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(ನಿಜ);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->ಕೂಡಿಸು_Modulesp(otherModp);
                            new_ಅಂಚು(ಶೃಂಗ(cellmodp), ಶೃಂಗ(otherModp), 1, ಸುಳ್ಳು);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cell_ಅಂಚು(ಶೃಂಗ(m_modp), ಶೃಂಗ(cellmodp), 1, ಸುಳ್ಳು, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        ಆದರೆ (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ತಳ್ಳು_Deletep(nodep->pinsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        ಆದರೆ (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ತಳ್ಳು_Deletep(nodep->paramsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        // Convert .* to list of pins
        ತಾರ್ಕಿಕ pin_ನಕ್ಷತ್ರ = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ pinDotName = ಸುಳ್ಳು;
        ಗಾಗಿ (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            ಆದರೆ (pinp->svDotName()) pinDotName = ನಿಜ;
            ಆದರೆ (pinp->dot_ನಕ್ಷತ್ರ()) {
                ಆದರೆ (pin_ನಕ್ಷತ್ರ) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pin_ನಕ್ಷತ್ರ = ನಿಜ;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        ಗಾಗಿ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಆದರೆ (pinp->name() == "") pinp->name("__pinNumber" + cvt_ಗಮ್ಯ_Str(pinp->pin_ಅಂಕೆ()));
        }
        ನಿಯೋಜಿಸು_Param_Numbers(nodep->paramsp());
        ಆದರೆ (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_ಗಣ<string> ports;  // Symbol table of all connected port names
            ಗಾಗಿ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ ((pin_ನಕ್ಷತ್ರ || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                ಆದರೆ (!pinp->exprp()) {
                    ಆದರೆ (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.ಸೇರಿಸು(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            ಗಾಗಿ (Ast_ಗಂಟು* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಪೋರ್ಟ್* ಸ್ಥಿರಾಂಕ portp = VN_CAST(portnodep, ಪೋರ್ಟ್_2)) {

                    ಆದರೆ (ports.ಕಂಡುಹಿಡಿ(portp->name()) == ports.ಕೊನೆ()
                        && ports.ಕಂಡುಹಿಡಿ("__pinNumber" + cvt_ಗಮ್ಯ_Str(portp->pin_ಅಂಕೆ())) == ports.ಕೊನೆ()) {
                        ಆದರೆ (pin_ನಕ್ಷತ್ರ) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ಸ್ಥಿರಾಂಕ newp
                                = ಹೊಸ AstPin{nodep->fileline(), 0, portp->name(),
                                             ಹೊಸ AstParseRef{nodep->fileline(), portp->name(),
                                                             ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ}};
                            newp->svDotName(ನಿಜ);
                            newp->svImplicit(ನಿಜ);
                            nodep->ಕೂಡಿಸು_Pinsp(newp);
                        } ಇಲ್ಲದಿದ್ದರೆ {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ಸ್ಥಿರಾಂಕ AstVar* portp_varp = ಶೂನ್ಯಸೂಚಕ;
                            ಗಾಗಿ (Ast_ಗಂಟು* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(module_stmtsp, Var)) {
                                    ಆದರೆ (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        ನಿಲ್ಲಿಸು;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            ಆದರೆ (portp_varp && portp_varp->is_ಒಳಹರಿವು() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ಸ್ಥಿರಾಂಕ newp
                                    = ಹೊಸ AstPin{nodep->fileline(), 0, portp->name(), ಶೂನ್ಯಸೂಚಕ};
                                nodep->ಕೂಡಿಸು_Pinsp(newp);
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ಸ್ಥಿರಾಂಕ newp
                                    = ಹೊಸ AstPin{nodep->fileline(), 0, portp->name(), ಶೂನ್ಯಸೂಚಕ};
                                nodep->ಕೂಡಿಸು_Pinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        ಆದರೆ (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            ಆದರೆ (!nodep->hasIfaceVar()) {
                ಸ್ಥಿರಾಂಕ string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ಸ್ಥಿರಾಂಕ idtypep = ಹೊಸ AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(ಶೂನ್ಯಸೂಚಕ);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                ಆದರೆ (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::ಸದಿಶ<AstRange*> rangesp;
                    ಗಾಗಿ (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ತಳ್ಳು_ಹಿಂಭಾಗ(rp);
                    }
                    Ast_ಗಂಟು_DType* dtp = idtypep;
                    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = ಹೊಸ AstUnpackArrayDType{nodep->fileline(), VFlag_ಮಗು_DType{}, dtp,
                                                      (*it)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
                    }
                    varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_ಮಗು_DType{}, dtp};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_ಮಗು_DType{}, idtypep};
                }
                varp->is_Iface_ಪೋಷಕ(ನಿಜ);
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(varp);
                nodep->hasIfaceVar(ನಿಜ);
            }
        }
        ಆದರೆ (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        ನಿಯೋಜಿಸು_Param_Numbers(nodep->paramsp());
        ಆದರೆ (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varModp = ಕಂಡುಹಿಡಿ_Module_Sym(nodep->name(), m_modp->libname());
            ಆದರೆ (AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                new_ಅಂಚು(ಶೃಂಗ(m_modp), ಶೃಂಗ(ifacep), 1, ಸುಳ್ಳು);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        ನಿಯೋಜಿಸು_Param_Numbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_ಮುಂದಿನ_Null(nodep->ಮಗು_DTypep());
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->delayp());
        iterate_And_ಮುಂದಿನ_Null(nodep->valuep());
        iterate_And_ಮುಂದಿನ_Null(nodep->attrsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // METHODS
    ಶೂನ್ಯ read_Mod_ಹೆಸರುಗಳು() {
        // mangled_name, BlockOptions
        ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ_Opt_ಗಣ& hier_ಖಂಡಗಳು = v3Global.opt.hier_ಖಂಡಗಳು();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ hierIt = vlstd::as_const(hier_ಖಂಡಗಳು).ಕಂಡುಹಿಡಿ(v3Global.opt.ತುದಿ_Module());
        UASSERT((hierIt != hier_ಖಂಡಗಳು.ಕೊನೆ()) == !!v3Global.opt.hier_ಮಗು(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        ಗಾಗಿ (Ast_ಗಂಟು_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ಗಂಟು_Module);
            ಆದರೆ (v3Global.opt.hier_ಮಗು() && nodep->origName() == hierIt->ಎರಡನೇ.origName()) {
                nodep->name(hierIt->ಮೊದಲ);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ libFoundp
                = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(nodep->origName(), nodep->libname());
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ globalFoundp = ಕಂಡುಹಿಡಿ_Module_Lib_Sym(nodep->name(), "__GLOBAL");
            ಆದರೆ (libFoundp && libFoundp == nodep) {
                // Ok
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (libFoundp) {
                ಆದರೆ (!(libFoundp->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::MODDUP)
                      || nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::MODDUP)
                      || hier_ಖಂಡಗಳು.ಕಂಡುಹಿಡಿ(nodep->name()) != hier_ಖಂಡಗಳು.ಕೊನೆ())) {
                    nodep->v3warn(MODDUP, "Duplicate declaration of "
                                              << nodep->verilogKwd() << ": "
                                              << nodep->prettyNameQ() << '\n'
                                              << nodep->warnContextPrimary() << '\n'
                                              << libFoundp->warnOther()
                                              << "... Location of original declaration\n"
                                              << libFoundp->warnContextSecondary());
                }
                nodep->unlink_Fr_ಹಿಂಭಾಗ();
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ಸ್ಥಿರಾಂಕ string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                ಸೇರಿಸು_Mod_ಒಳ_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                ಸೇರಿಸು_Mod_ಒಳ_Lib(nodep->name(), "__GLOBAL", nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸೇರಿಸು_Mod_ಒಳ_Lib(nodep->origName(), nodep->libname(), nodep);
                ಸೇರಿಸು_Mod_ಒಳ_Lib(nodep->name(), "__GLOBAL", nodep);
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
        ಆದರೆ (v3Global.opt.hier_ಮಗು()) {
            ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ_Opt_ಗಣ& hier_ಖಂಡಗಳು = v3Global.opt.hier_ಖಂಡಗಳು();
            UASSERT(!v3Global.opt.ತುದಿ_Module().ಖಾಲಿಗಳು(),
                    "top module must be explicitly specified in hierarchical mode");
            ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ_Opt_ಗಣ::const_iterator hierIt
                = hier_ಖಂಡಗಳು.ಕಂಡುಹಿಡಿ(v3Global.opt.ತುದಿ_Module());
            UASSERT(hierIt != hier_ಖಂಡಗಳು.ಕೊನೆ(),
                    "top module must be listed in --hierarchical-block");
            m_orig_ತುದಿ_Module_Name = hierIt->ಎರಡನೇ.origName();
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_orig_ತುದಿ_Module_Name = v3Global.opt.ತುದಿ_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        ಆದರೆ (debug() >= 5 || dump_ನಕ್ಷೆ_Level() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
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
