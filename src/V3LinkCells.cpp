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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkCells.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Parse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SymTable.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

ਵਰਗ Link_Cells_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
ਜਨਤਕ:
    Link_Cells_ਗ੍ਰਾਫ() = ਮੂਲ;
    ~Link_Cells_ਗ੍ਰਾਫ() override = ਮੂਲ;
    ਖਾਲੀ loops_ਸੁਨੇਹਾ_Cb(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) override;
};

ਵਰਗ Link_Cells_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Link_Cells_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;

ਜਨਤਕ:
    Link_Cells_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ_Module* modp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_modp{modp} {}
    ~Link_Cells_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    Ast_ਗੰਢ_Module* modp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_modp; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Hex(modp()) + ' ' + modp()->name(); }
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ਅਚਲ override {
        ਵਾਪਸ m_modp->recursiveClone() ? (1 + v3Global.opt.module_Recursion_ਡੂੰਘਾਈ()) : 1;
    }
};

ਵਰਗ Library_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Library_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    ਸਪਸ਼ਟ Library_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp} {}
    ~Library_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "*LIBRARY*"; }
};

ਵਰਗ Cell_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Cell_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
    AstCell* ਅਚਲ m_cellp;

ਜਨਤਕ:
    Cell_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2, ਤਾਰਕਿਕ cutable,
             AstCell* cellp)
        : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, ਭਾਰ_2, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ਅਚਲ { ਵਾਪਸ m_cellp; }
    string name() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ cellp() ? cvt_ਮੰਜ਼ਿਲ_Hex(cellp()) + ' ' + cellp()->name() : "";
    }
};

ਖਾਲੀ Link_Cells_ਗ੍ਰਾਫ::loops_ਸੁਨੇਹਾ_Cb(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) {
    ਜੇ (ਅਚਲ Link_Cells_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vertexp->cast<Link_Cells_ਸ਼ੀਰਸ਼>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warn_ਹੋਰ()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } ਨਹੀਂਤਾਂ {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
ਬਣਤਰ LinkCellsState final {
    // Set of possible top module names from command line and configs
    std::ਸਦਿਸ਼<std::pair<std::string, std::string>> m_designs;
    // Default library lists to search
    std::ਸਦਿਸ਼<std::string> m_liblistDefault;
    // Library lists for specific cells
    std::unordered_map<std::string, std::ਸਦਿਸ਼<std::string>> m_liblistCell;
    // Use list for specific cells (libname, cellname)
    std::unordered_map<std::string, std::ਸਦਿਸ਼<std::pair<std::string, std::string>>>
        m_uselistCell;
    // Library lists for specific insts
    std::unordered_map<std::string, std::ਸਦਿਸ਼<std::string>> m_liblistInst;
    // Use list for specific insts (libname, cellname)
    std::unordered_map<std::string, std::ਸਦਿਸ਼<std::pair<std::string, std::string>>>
        m_uselistInst;
};

ਵਰਗ LinkConfigsVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    ਤਾਰਕਿਕ m_is_ਸਿਖਰ = ਝੂਠ;  // Whether we're in the top-level config
    ਤਾਰਕਿਕ m_isDefault = ਝੂਠ;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = ਖਾਲੀ_ਸੂਚਕ;  // Current dot being processed

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstConfig* nodep) override {
        VL_RESTORER(m_is_ਸਿਖਰ);
        ਅਚਲ ਸਵੈਚਾਲਿਤ& ਭਰਿਆ_Name
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_ਸਿਖਰ = std::ਲੱਭੋ(m_state.m_designs.begin(), m_state.m_designs.ਸਿਰਾ(), ਭਰਿਆ_Name)
                  != m_state.m_designs.ਸਿਰਾ();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.ਸਿਰਾ(), ਭਰਿਆ_Name),
            m_state.m_designs.ਸਿਰਾ());
        iterateChildren(nodep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstConfigCell* nodep) override {
        ਜੇ (m_is_ਸਿਖਰ) m_state.m_designs.emplace_ਪਿੱਠ(nodep->libname(), nodep->cellname());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Config_ਨਿਯਮ* nodep) override {
        ਜੇ (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Null(nodep->usep());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_ਅਗਲਾ_Null(nodep->usep());
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_ਅਗਲਾ_Null(nodep->cellp());
            }
            iterate_And_ਅਗਲਾ_Null(nodep->usep());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstDot* nodep) override { iterateChildren(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstParseRef* nodep) override {
        ਜੇ (m_isDefault) {
            m_state.m_liblistDefault.emplace_ਪਿੱਠ(nodep->name());
        } ਨਹੀਂਤਾਂ ਜੇ (!m_cell.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_state.m_liblistCell[m_cell].emplace_ਪਿੱਠ(nodep->name());
        } ਨਹੀਂਤਾਂ ਜੇ (m_dotp) {
            m_hierInst += m_hierInst.ਖਾਲੀ_ਥਾਵਾਂ() ? nodep->name() : '.' + nodep->name();
        } ਨਹੀਂਤਾਂ ਜੇ (!m_hierInst.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_state.m_liblistInst[m_hierInst].emplace_ਪਿੱਠ(nodep->name());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstConfigUse* nodep) override {
        ਜੇ (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } ਨਹੀਂਤਾਂ ਜੇ (!m_cell.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_state.m_uselistCell[m_cell].emplace_ਪਿੱਠ(nodep->libname(), nodep->cellname());
        } ਨਹੀਂਤਾਂ ਜੇ (m_dotp) {
            m_hierInst += m_hierInst.ਖਾਲੀ_ਥਾਵਾਂ() ? nodep->name() : '.' + nodep->name();
        } ਨਹੀਂਤਾਂ ਜੇ (!m_hierInst.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_state.m_uselistInst[m_hierInst].emplace_ਪਿੱਠ(nodep->libname(), nodep->cellname());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        ਜੇ (!m_state.m_designs.ਮਾਪ() && !v3Global.opt.ਸਿਖਰ_Module().ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_state.m_designs.emplace_ਪਿੱਠ(v3Global.opt.work(), v3Global.opt.ਸਿਖਰ_Module());
        }
        iterate(nodep);
    }
};

ਵਰਗ LinkCellsVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE
    VInFilter* ਅਚਲ m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstVar* m_varp = ਖਾਲੀ_ਸੂਚਕ;  // Current variable
    VSym_ਗ੍ਰਾਫ m_mods;  // Symbol table of all module names
    Link_Cells_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;  // Linked graph of all cell interconnects
    Library_ਸ਼ੀਰਸ਼* m_libVertexp = ਖਾਲੀ_ਸੂਚਕ;  // Vertex at root of all libraries
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* m_ਸਿਖਰ_Vertexp = ਖਾਲੀ_ਸੂਚਕ;  // Vertex of top module
    std::unordered_ਸੈੱਟ<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_ਸਿਖਰ_Module_Name;  // original name of the top module
    ਪੂਰਨ m_mod_ਡੂੰਘਾਈ = 0;  // Depth of the current module

    // METHODS
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸ਼ੀਰਸ਼(Ast_ਗੰਢ_Module* nodep) {
        // Return corresponding vertex for this module
        ਜੇ (!nodep->user1p()) nodep->user1p(ਨਵਾਂ Link_Cells_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, nodep});
        ਵਾਪਸ nodep->user1u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼();
    }
    ਖਾਲੀ new_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2, ਤਾਰਕਿਕ cuttable) {
        ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep = ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, fromp, ਸਿਖਰ, ਭਾਰ_2, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << ਸਿਖਰ->name());
    }
    ਖਾਲੀ cell_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2, ਤਾਰਕਿਕ cuttable,
                  AstCell* cellp) {
        ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep
            = ਨਵਾਂ Cell_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, fromp, ਸਿਖਰ, ਭਾਰ_2, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << ਸਿਖਰ->name());
    }
    ਖਾਲੀ ਪਾਓ_Mod_ਵਿੱਚ_Lib(ਅਚਲ string& name, ਅਚਲ string& libname, Ast_ਗੰਢ_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->ਲੱਭੋ_Id_Flat(libname);
        ਜੇ (!libSymp)
            libSymp = m_mods.rootp()->ਪਾਓ(libname, ਨਵਾਂ VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->ਪਾਓ(name, ਨਵਾਂ VSymEnt{&m_mods, nodep});
    }

    Ast_ਗੰਢ_Module* ਲੱਭੋ_Module_Lib_Sym(ਅਚਲ string& modName, ਅਚਲ string& libname) {
        // Given module name and library name, find within that exact library
        ਅਚਲ VSymEnt* ਅਚਲ libSymp = m_mods.rootp()->ਲੱਭੋ_Id_Fallback(libname);
        ਜੇ (!libSymp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ VSymEnt* ਅਚਲ foundp = libSymp->ਲੱਭੋ_Id_Fallback(modName);
        ਵਾਪਸ foundp ? VN_AS(foundp->nodep(), ਗੰਢ_Module) : ਖਾਲੀ_ਸੂਚਕ;
    }
    Ast_ਗੰਢ_Module* ਲੱਭੋ_Module_Sym(ਅਚਲ string& modName, ਅਚਲ string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_ਗੰਢ_Module* foundp;
        ਅਚਲ std::string ਭਰਿਆ_Name = libname + "." + modName;
        // First search cell-specific use list
        ਅਚਲ ਸਵੈਚਾਲਿਤ itCellUseList = m_state.m_uselistCell.ਲੱਭੋ(ਭਰਿਆ_Name);
        ਜੇ (itCellUseList != m_state.m_uselistCell.ਸਿਰਾ()) {
            ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& u : itCellUseList->ਦੂਜਾ) {
                foundp = ਲੱਭੋ_Module_Lib_Sym(u.ਦੂਜਾ, u.ਪਹਿਲਾ);
                ਜੇ (foundp) ਵਾਪਸ foundp;
            }
        }
        // Then search cell-specific library list
        ਅਚਲ ਸਵੈਚਾਲਿਤ itCellLibList = m_state.m_liblistCell.ਲੱਭੋ(ਭਰਿਆ_Name);
        ਜੇ (itCellLibList != m_state.m_liblistCell.ਸਿਰਾ()) {
            ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& l : itCellLibList->ਦੂਜਾ) {
                foundp = ਲੱਭੋ_Module_Lib_Sym(modName, l);
                ਜੇ (foundp) ਵਾਪਸ foundp;
            }
        }
        // Then search IEEE config default liblist
        ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& l : m_state.m_liblistDefault) {
            foundp = ਲੱਭੋ_Module_Lib_Sym(modName, l);
            ਜੇ (foundp) ਵਾਪਸ foundp;
        }
        // Then search local library
        foundp = ਲੱਭੋ_Module_Lib_Sym(modName, libname);
        ਜੇ (foundp) ਵਾਪਸ foundp;
        // Then search global
        foundp = ਲੱਭੋ_Module_Lib_Sym(modName, "__GLOBAL");
        ਵਾਪਸ foundp;
    }

    Ast_ਗੰਢ_Module* resolveModule(Ast_ਗੰਢ* nodep, ਅਚਲ string& modName, ਅਚਲ string& libname) {
        Ast_ਗੰਢ_Module* modp = ਲੱਭੋ_Module_Sym(modName, libname);
        ਜੇ (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ਅਚਲ string prettyName = Ast_ਗੰਢ::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, ਸੱਚ, ਝੂਠ, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            read_Mod_ਨਾਂ();
            // Check again
            modp = ਲੱਭੋ_Module_Sym(modName, libname);
            ਜੇ (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        ਵਾਪਸ modp;
    }

    ਸਥਿਰ ਖਾਲੀ removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        ਜੇ (!v3Global.opt.ਸਿਖਰ_Module().ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਲਈ (Ast_ਗੰਢ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ਗੰਢ_Module)) {
            ਜੇ (!nodep->ਵਿੱਚ_Library()) ਵਾਪਸ;
        }
        ਲਈ (Ast_ਗੰਢ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ਗੰਢ_Module)) {
            nodep->ਵਿੱਚ_Library(ਝੂਠ);
        }
    }

    AstCell* ਲੱਭੋ_Cell_By_Hier(AstNetlist* nodep, ਅਚਲ std::string& hier_ਰਸਤਾ) {
        std::stringstream ss(hier_ਰਸਤਾ);
        std::string ਸਿਖਰ;
        ਤਾਰਕਿਕ ਸਿਖਰ_ਮਿਲਿਆ = ਝੂਠ;
        std::getline(ss, ਸਿਖਰ, '.');
        ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& pair : m_state.m_designs) {
            ਜੇ (ਸਿਖਰ == pair.ਦੂਜਾ) {
                ਸਿਖਰ_ਮਿਲਿਆ = ਸੱਚ;
                ਰੋਕੋ;
            }
        }
        ਜੇ (!ਸਿਖਰ_ਮਿਲਿਆ) {
            nodep->v3error("Can't find top-level module for instance path: '" << hier_ਰਸਤਾ << "'");
            V3Error::abortIfErrors();
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vtx = m_ਸਿਖਰ_Vertexp;
        ਅਚਲ Cell_ਕਿਨਾਰਾ* finalEdgep = ਖਾਲੀ_ਸੂਚਕ;
        std::string seg;
        ਜਦੋਂ (std::getline(ss, seg, '.')) {
            finalEdgep = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਜੇ (ਅਚਲ Cell_ਕਿਨਾਰਾ* ਅਚਲ cedgep = ਕਿਨਾਰਾ.cast<Cell_ਕਿਨਾਰਾ>()) {
                    ਜੇ (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->ਸਿਖਰ();
                        finalEdgep = cedgep;
                        ਰੋਕੋ;
                    }
                }
            }
            ਜੇ (!finalEdgep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        ਵਾਪਸ finalEdgep->cellp();
    }

    ਖਾਲੀ ਸੌਂਪੋ_Param_Numbers(AstPin* ਅਚਲ ਪਹਿਲਾ_Pinp) {
        ਲਈ (AstPin* pinp = ਪਹਿਲਾ_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(ਸੱਚ);
            ਜੇ (pinp->name() == "") pinp->name("__paramNumber" + cvt_ਮੰਜ਼ਿਲ_Str(pinp->pin_ਅੰਕ()));
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        read_Mod_ਨਾਂ();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_ਗੰਢ_Module* modp = ਖਾਲੀ_ਸੂਚਕ;
        AstCell* cellp = ਖਾਲੀ_ਸੂਚਕ;

        // Search liblists for each instance
        ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& pair : m_state.m_liblistInst) {
            cellp = ਲੱਭੋ_Cell_By_Hier(nodep, pair.ਪਹਿਲਾ);
            ਜੇ (!cellp) ਜਾਰੀ;
            ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& libname : pair.ਦੂਜਾ) {
                modp = ਲੱਭੋ_Module_Lib_Sym(cellp->modName(), libname);
                ਜੇ (modp) {
                    cellp->modp(modp);
                    ਰੋਕੋ;
                }
            }
        }

        // Search uselists for each instance
        ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& pair : m_state.m_uselistInst) {
            cellp = ਲੱਭੋ_Cell_By_Hier(nodep, pair.ਪਹਿਲਾ);
            ਲਈ (ਸਵੈਚਾਲਿਤ ਅਚਲ& u : pair.ਦੂਜਾ) {
                modp = ਲੱਭੋ_Module_Lib_Sym(u.ਦੂਜਾ, u.ਪਹਿਲਾ);
                ਜੇ (modp) {
                    cellp->modp(modp);
                    ਰੋਕੋ;
                }
            }
        }

        // Find levels in graph
        m_ਗ੍ਰਾਫ.remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level()) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("linkcells");
        m_ਗ੍ਰਾਫ.rank();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (ਅਚਲ Link_Cells_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtx.cast<Link_Cells_ਸ਼ੀਰਸ਼>()) {
                Ast_ਗੰਢ_Module* ਅਚਲ vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_ਗ੍ਰਾਫ.rankMin();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (ਅਚਲ Link_Cells_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtx.cast<Link_Cells_ਸ਼ੀਰਸ਼>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_ਗੰਢ_Module* ਅਚਲ vmodp = vvertexp->modp();
                vmodp->ਡੂੰਘਾਈ(vvertexp->rank() + 1);
            }
        }
        ਜੇ (v3Global.opt.ਸਿਖਰ_Module() != "" && !m_ਸਿਖਰ_Vertexp) {
            VSpellCheck spell;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
                ਜੇ (ਅਚਲ Link_Cells_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtx.cast<Link_Cells_ਸ਼ੀਰਸ਼>()) {
                    Ast_ਗੰਢ_Module* ਅਚਲ vmodp = vvertexp->modp();
                    ਜੇ (VN_IS(vmodp, Module)) spell.ਧੱਕੋ_Candidate(vmodp->prettyName());
                }
            }
            ਅਚਲ string suggest
                = spell.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(Ast_ਗੰਢ::prettyName(v3Global.opt.ਸਿਖਰ_Module()));
            v3error("Specified --top-module '"
                    << Ast_ਗੰਢ::prettyName(v3Global.opt.ਸਿਖਰ_Module())
                    << "' was not found in design.\n"
                    << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : V3Error::warn_ਹੋਰ() + suggest));
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_mod_ਡੂੰਘਾਈ);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            ਜੇ (m_modp) new_ਕਿਨਾਰਾ(ਸ਼ੀਰਸ਼(m_modp), ਸ਼ੀਰਸ਼(nodep), 1, ਝੂਠ);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            ਸ਼ੀਰਸ਼(m_modp);
            ++m_mod_ਡੂੰਘਾਈ;
            UINFO(4, "Link Module: " << nodep);
            ਜੇ (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, Not_ਮਿਲਿਆ_Module) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_mod_ਡੂੰਘਾਈ == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ਅਚਲ ਸਵੈਚਾਲਿਤ it_ਮਿਲਿਆ_Pair = m_declfnWarned.ਪਾਓ(nodep->fileline()->filename());
                ਜੇ (it_ਮਿਲਿਆ_Pair.ਦੂਜਾ) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            ਜੇ (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->ਵਿੱਚ_Library(ਸੱਚ);  // Interfaces can't be at top, unless asked
            }

            ਸਵੈਚਾਲਿਤ ਅਚਲ& ਭਰਿਆ_Name
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ਅਚਲ ਤਾਰਕਿਕ ਸਿਖਰ_ਮੇਲ
                = std::ਲੱਭੋ(m_state.m_designs.begin(), m_state.m_designs.ਸਿਰਾ(), ਭਰਿਆ_Name)
                  != m_state.m_designs.ਸਿਰਾ();
            ਜੇ (ਸਿਖਰ_ਮੇਲ) {
                m_ਸਿਖਰ_Vertexp = ਸ਼ੀਰਸ਼(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->ਵਿੱਚ_Library(ਝੂਠ);  // Safer to make sure it doesn't disappear
            }
            ਜੇ (v3Global.opt.ਸਿਖਰ_Module().ਖਾਲੀ_ਥਾਵਾਂ()
                    ? nodep->ਵਿੱਚ_Library()  // Library cells are lower
                    : !ਸਿਖਰ_ਮੇਲ) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                ਜੇ (!m_libVertexp) m_libVertexp = ਨਵਾਂ Library_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ};
                new_ਕਿਨਾਰਾ(m_libVertexp, ਸ਼ੀਰਸ਼(nodep), 1, ਝੂਠ);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_ਰੁੱਖ();
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_ਗੰਢ_Module* ਅਚਲ modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        ਜੇ (modp) {
            ਜੇ (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                ਜੇ (!nodep->isVirtual()) new_ਕਿਨਾਰਾ(ਸ਼ੀਰਸ਼(m_modp), ਸ਼ੀਰਸ਼(modp), 1, ਝੂਠ);
                ਜੇ (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(modp, Not_ਮਿਲਿਆ_Module)) {  // Will error out later
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("Non-interface used as an interface: "
                               << nodep->ifaceNameQ() << "\n"
                               << nodep->warn_ਹੋਰ()
                                      + "... Perhaps intended an instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 23.3.2)?");
            }
        }
        iterateChildren(nodep);
        ਸੌਂਪੋ_Param_Numbers(nodep->paramsp());
        // Parser didn't know what was interface, resolve now
        // For historical reasons virtual interface reference variables remain VARs
        ਜੇ (m_varp && !nodep->isVirtual()) m_varp->ਸੈੱਟ_Iface_Ref();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ਜੇ (!nodep->packagep()) {
            Ast_ਗੰਢ_Module* ਅਚਲ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ਜੇ (AstPackage* ਅਚਲ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            ਜੇ (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                ਵਾਪਸ;
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ਜੇ (!nodep->packagep()) {
            Ast_ਗੰਢ_Module* ਅਚਲ modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ਜੇ (AstPackage* ਅਚਲ pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            ਜੇ (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                ਵਾਪਸ;
            }
        }
        new_ਕਿਨਾਰਾ(ਸ਼ੀਰਸ਼(m_modp), ਸ਼ੀਰਸ਼(nodep->packagep()), 1, ਝੂਠ);
    }

    ਖਾਲੀ ਦੇਖੋ(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_ਗੰਢ_Module* ਅਚਲ modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        ਜੇ (modp) {
            Ast_ਗੰਢ* ਅਚਲ cellsp = nodep->cellsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->ਜੋੜੋ_Stmtsp(cellsp);
            iterate_And_ਅਗਲਾ_Null(cellsp);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ਅਚਲ ਤਾਰਕਿਕ cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        ਜੇ (nodep->user1p() == m_modp) ਵਾਪਸ;  // AstBind and AstNodeModule may call a cell twice
        ਜੇ (nodep->modName() == m_orig_ਸਿਖਰ_Module_Name) {
            ਜੇ (v3Global.opt.hier_ਬੱਚਾ() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } ਨਹੀਂਤਾਂ {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
        }
        nodep->user1p(m_modp);
        //
        ਜੇ (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_ਗੰਢ_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            ਜੇ (cellmodp) {
                ਜੇ (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(ਸੱਚ);
                    nodep->recursive(ਸੱਚ);
                    ਜੇ (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_ਗੰਢ_Module* otherModp = VN_CAST(cellmodp->user2p(), ਗੰਢ_Module);
                        ਜੇ (!otherModp) {
                            otherModp = cellmodp->clone_ਰੁੱਖ(ਝੂਠ);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(ਖਾਲੀ_ਸੂਚਕ);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(ਸੱਚ);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->ਜੋੜੋ_Modulesp(otherModp);
                            new_ਕਿਨਾਰਾ(ਸ਼ੀਰਸ਼(cellmodp), ਸ਼ੀਰਸ਼(otherModp), 1, ਝੂਠ);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } ਨਹੀਂਤਾਂ {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } ਨਹੀਂਤਾਂ {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cell_ਕਿਨਾਰਾ(ਸ਼ੀਰਸ਼(m_modp), ਸ਼ੀਰਸ਼(cellmodp), 1, ਝੂਠ, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        ਜੇ (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ਧੱਕੋ_Deletep(nodep->pinsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        ਜੇ (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ਧੱਕੋ_Deletep(nodep->paramsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        // Convert .* to list of pins
        ਤਾਰਕਿਕ pin_ਤਾਰਾ = ਝੂਠ;
        ਤਾਰਕਿਕ pinDotName = ਝੂਠ;
        ਲਈ (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            ਜੇ (pinp->svDotName()) pinDotName = ਸੱਚ;
            ਜੇ (pinp->dot_ਤਾਰਾ()) {
                ਜੇ (pin_ਤਾਰਾ) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pin_ਤਾਰਾ = ਸੱਚ;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        ਲਈ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਜੇ (pinp->name() == "") pinp->name("__pinNumber" + cvt_ਮੰਜ਼ਿਲ_Str(pinp->pin_ਅੰਕ()));
        }
        ਸੌਂਪੋ_Param_Numbers(nodep->paramsp());
        ਜੇ (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_ਸੈੱਟ<string> ports;  // Symbol table of all connected port names
            ਲਈ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ ((pin_ਤਾਰਾ || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                ਜੇ (!pinp->exprp()) {
                    ਜੇ (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } ਨਹੀਂਤਾਂ {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.ਪਾਓ(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            ਲਈ (Ast_ਗੰਢ* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                ਜੇ (ਅਚਲ Ast_ਪੋਰਟ* ਅਚਲ portp = VN_CAST(portnodep, ਪੋਰਟ_2)) {

                    ਜੇ (ports.ਲੱਭੋ(portp->name()) == ports.ਸਿਰਾ()
                        && ports.ਲੱਭੋ("__pinNumber" + cvt_ਮੰਜ਼ਿਲ_Str(portp->pin_ਅੰਕ())) == ports.ਸਿਰਾ()) {
                        ਜੇ (pin_ਤਾਰਾ) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ਅਚਲ newp
                                = ਨਵਾਂ AstPin{nodep->fileline(), 0, portp->name(),
                                             ਨਵਾਂ AstParseRef{nodep->fileline(), portp->name(),
                                                             ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ}};
                            newp->svDotName(ਸੱਚ);
                            newp->svImplicit(ਸੱਚ);
                            nodep->ਜੋੜੋ_Pinsp(newp);
                        } ਨਹੀਂਤਾਂ {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ਅਚਲ AstVar* portp_varp = ਖਾਲੀ_ਸੂਚਕ;
                            ਲਈ (Ast_ਗੰਢ* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(module_stmtsp, Var)) {
                                    ਜੇ (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        ਰੋਕੋ;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            ਜੇ (portp_varp && portp_varp->is_ਨਿਵੇਸ਼() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ਅਚਲ newp
                                    = ਨਵਾਂ AstPin{nodep->fileline(), 0, portp->name(), ਖਾਲੀ_ਸੂਚਕ};
                                nodep->ਜੋੜੋ_Pinsp(newp);
                            } ਨਹੀਂਤਾਂ {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ਅਚਲ newp
                                    = ਨਵਾਂ AstPin{nodep->fileline(), 0, portp->name(), ਖਾਲੀ_ਸੂਚਕ};
                                nodep->ਜੋੜੋ_Pinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        ਜੇ (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            ਜੇ (!nodep->hasIfaceVar()) {
                ਅਚਲ string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ਅਚਲ idtypep = ਨਵਾਂ AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(ਖਾਲੀ_ਸੂਚਕ);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                ਜੇ (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::ਸਦਿਸ਼<AstRange*> rangesp;
                    ਲਈ (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ਧੱਕੋ_ਪਿੱਠ(rp);
                    }
                    Ast_ਗੰਢ_DType* dtp = idtypep;
                    ਲਈ (ਸਵੈਚਾਲਿਤ it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = ਨਵਾਂ AstUnpackArrayDType{nodep->fileline(), VFlag_ਬੱਚਾ_DType{}, dtp,
                                                      (*it)->clone_ਰੁੱਖ(ਝੂਠ)};
                    }
                    varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_ਬੱਚਾ_DType{}, dtp};
                } ਨਹੀਂਤਾਂ {
                    varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_ਬੱਚਾ_DType{}, idtypep};
                }
                varp->is_Iface_ਮਾਪਾ(ਸੱਚ);
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(varp);
                nodep->hasIfaceVar(ਸੱਚ);
            }
        }
        ਜੇ (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        ਸੌਂਪੋ_Param_Numbers(nodep->paramsp());
        ਜੇ (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_ਗੰਢ_Module* ਅਚਲ varModp = ਲੱਭੋ_Module_Sym(nodep->name(), m_modp->libname());
            ਜੇ (AstIface* ਅਚਲ ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                new_ਕਿਨਾਰਾ(ਸ਼ੀਰਸ਼(m_modp), ਸ਼ੀਰਸ਼(ifacep), 1, ਝੂਠ);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        ਸੌਂਪੋ_Param_Numbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_ਅਗਲਾ_Null(nodep->ਬੱਚਾ_DTypep());
        }
        iterate_And_ਅਗਲਾ_Null(nodep->delayp());
        iterate_And_ਅਗਲਾ_Null(nodep->valuep());
        iterate_And_ਅਗਲਾ_Null(nodep->attrsp());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    // METHODS
    ਖਾਲੀ read_Mod_ਨਾਂ() {
        // mangled_name, BlockOptions
        ਅਚਲ V3_Hier_ਖੰਡ_Opt_ਸੈੱਟ& hier_ਖੰਡਾਂ = v3Global.opt.hier_ਖੰਡਾਂ();
        ਅਚਲ ਸਵੈਚਾਲਿਤ hierIt = vlstd::as_const(hier_ਖੰਡਾਂ).ਲੱਭੋ(v3Global.opt.ਸਿਖਰ_Module());
        UASSERT((hierIt != hier_ਖੰਡਾਂ.ਸਿਰਾ()) == !!v3Global.opt.hier_ਬੱਚਾ(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        ਲਈ (Ast_ਗੰਢ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ਗੰਢ_Module);
            ਜੇ (v3Global.opt.hier_ਬੱਚਾ() && nodep->origName() == hierIt->ਦੂਜਾ.origName()) {
                nodep->name(hierIt->ਪਹਿਲਾ);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ libFoundp
                = ਲੱਭੋ_Module_Lib_Sym(nodep->origName(), nodep->libname());
            ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ globalFoundp = ਲੱਭੋ_Module_Lib_Sym(nodep->name(), "__GLOBAL");
            ਜੇ (libFoundp && libFoundp == nodep) {
                // Ok
            } ਨਹੀਂਤਾਂ ਜੇ (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } ਨਹੀਂਤਾਂ ਜੇ (libFoundp) {
                ਜੇ (!(libFoundp->fileline()->warnIsOff(V3_Error_ਕੋਡ::MODDUP)
                      || nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::MODDUP)
                      || hier_ਖੰਡਾਂ.ਲੱਭੋ(nodep->name()) != hier_ਖੰਡਾਂ.ਸਿਰਾ())) {
                    nodep->v3warn(MODDUP, "Duplicate declaration of "
                                              << nodep->verilogKwd() << ": "
                                              << nodep->prettyNameQ() << '\n'
                                              << nodep->warnContextPrimary() << '\n'
                                              << libFoundp->warnOther()
                                              << "... Location of original declaration\n"
                                              << libFoundp->warnContextSecondary());
                }
                nodep->unlink_Fr_ਪਿੱਠ();
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ਅਚਲ string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                ਪਾਓ_Mod_ਵਿੱਚ_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                ਪਾਓ_Mod_ਵਿੱਚ_Lib(nodep->name(), "__GLOBAL", nodep);
            } ਨਹੀਂਤਾਂ {
                ਪਾਓ_Mod_ਵਿੱਚ_Lib(nodep->origName(), nodep->libname(), nodep);
                ਪਾਓ_Mod_ਵਿੱਚ_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        ਜੇ (v3Global.opt.hier_ਬੱਚਾ()) {
            ਅਚਲ V3_Hier_ਖੰਡ_Opt_ਸੈੱਟ& hier_ਖੰਡਾਂ = v3Global.opt.hier_ਖੰਡਾਂ();
            UASSERT(!v3Global.opt.ਸਿਖਰ_Module().ਖਾਲੀ_ਥਾਵਾਂ(),
                    "top module must be explicitly specified in hierarchical mode");
            ਅਚਲ V3_Hier_ਖੰਡ_Opt_ਸੈੱਟ::const_iterator hierIt
                = hier_ਖੰਡਾਂ.ਲੱਭੋ(v3Global.opt.ਸਿਖਰ_Module());
            UASSERT(hierIt != hier_ਖੰਡਾਂ.ਸਿਰਾ(),
                    "top module must be listed in --hierarchical-block");
            m_orig_ਸਿਖਰ_Module_Name = hierIt->ਦੂਜਾ.origName();
        } ਨਹੀਂਤਾਂ {
            m_orig_ਸਿਖਰ_Module_Name = v3Global.opt.ਸਿਖਰ_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        ਜੇ (debug() >= 5 || dump_ਗ੍ਰਾਫ_Level() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

ਖਾਲੀ V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
