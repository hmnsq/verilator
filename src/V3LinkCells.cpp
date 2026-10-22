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

صنف Link_Cells_ګراف final : عام V3_ګراف {
عام:
    Link_Cells_ګراف() = افتراضي;
    ~Link_Cells_ګراف() override = افتراضي;
    فراغ loops_پیغام_Cb(V3_ګراف_راس* vertexp, V3_څنډه_Func_P څنډه_Funcp) override;
};

صنف Link_Cells_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Link_Cells_راس, V3_ګراف_راس)
    Ast_غوټه_Module* ثابت m_modp;

عام:
    Link_Cells_راس(V3_ګراف* graphp, Ast_غوټه_Module* modp)
        : V3_ګراف_راس{graphp}
        , m_modp{modp} {}
    ~Link_Cells_راس() override = افتراضي;
    Ast_غوټه_Module* modp() ثابت VL_MT_STABLE { أرجع m_modp; }
    string name() ثابت override VL_MT_STABLE { أرجع cvt_منزل_Hex(modp()) + ' ' + modp()->name(); }
    File_کرښه* fileline() ثابت override { أرجع modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ثابت override {
        أرجع m_modp->recursiveClone() ? (1 + v3Global.opt.module_Recursion_ژوروالی()) : 1;
    }
};

صنف Library_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Library_راس, V3_ګراف_راس)
عام:
    څرګند Library_راس(V3_ګراف* graphp)
        : V3_ګراف_راس{graphp} {}
    ~Library_راس() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { أرجع "*LIBRARY*"; }
};

صنف Cell_څنډه final : عام V3_ګراف_څنډه {
    VL_RTTI_IMPL(Cell_څنډه, V3_ګراف_څنډه)
    AstCell* ثابت m_cellp;

عام:
    Cell_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر, صحيح وزن_2, منطقي cutable,
             AstCell* cellp)
        : V3_ګراف_څنډه{graphp, fromp, سر, وزن_2, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ثابت { أرجع m_cellp; }
    string name() ثابت override VL_MT_STABLE {
        أرجع cellp() ? cvt_منزل_Hex(cellp()) + ' ' + cellp()->name() : "";
    }
};

فراغ Link_Cells_ګراف::loops_پیغام_Cb(V3_ګراف_راس* vertexp, V3_څنډه_Func_P څنډه_Funcp) {
    إذا (ثابت Link_Cells_راس* ثابت vvertexp = vertexp->cast<Link_Cells_راس>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warn_نور()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } وإلا {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
بنية LinkCellsState final {
    // Set of possible top module names from command line and configs
    std::ویکټور<std::pair<std::string, std::string>> m_designs;
    // Default library lists to search
    std::ویکټور<std::string> m_liblistDefault;
    // Library lists for specific cells
    std::unordered_map<std::string, std::ویکټور<std::string>> m_liblistCell;
    // Use list for specific cells (libname, cellname)
    std::unordered_map<std::string, std::ویکټور<std::pair<std::string, std::string>>>
        m_uselistCell;
    // Library lists for specific insts
    std::unordered_map<std::string, std::ویکټور<std::string>> m_liblistInst;
    // Use list for specific insts (libname, cellname)
    std::unordered_map<std::string, std::ویکټور<std::pair<std::string, std::string>>>
        m_uselistInst;
};

صنف LinkConfigsVisitor final : عام VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    منطقي m_is_سر = خطأ;  // Whether we're in the top-level config
    منطقي m_isDefault = خطأ;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = تش_اشاره;  // Current dot being processed

    // VISITORS
    فراغ لیدنه(AstConfig* nodep) override {
        VL_RESTORER(m_is_سر);
        ثابت خپلکاری& ډک_Name
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_سر = std::پیدا_کړه(m_state.m_designs.begin(), m_state.m_designs.پای_ټکی(), ډک_Name)
                  != m_state.m_designs.پای_ټکی();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.پای_ټکی(), ډک_Name),
            m_state.m_designs.پای_ټکی());
        iterateChildren(nodep);
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    فراغ لیدنه(AstConfigCell* nodep) override {
        إذا (m_is_سر) m_state.m_designs.emplace_شا(nodep->libname(), nodep->cellname());
    }

    فراغ لیدنه(Ast_Config_قاعده* nodep) override {
        إذا (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = ریښتیا;
            iterate_And_بل_Null(nodep->usep());
        } وإلا إذا (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_بل_Null(nodep->usep());
        } وإلا {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_بل_Null(nodep->cellp());
            }
            iterate_And_بل_Null(nodep->usep());
        }
    }

    فراغ لیدنه(AstDot* nodep) override { iterateChildren(nodep); }

    فراغ لیدنه(AstParseRef* nodep) override {
        إذا (m_isDefault) {
            m_state.m_liblistDefault.emplace_شا(nodep->name());
        } وإلا إذا (!m_cell.تشه()) {
            m_state.m_liblistCell[m_cell].emplace_شا(nodep->name());
        } وإلا إذا (m_dotp) {
            m_hierInst += m_hierInst.تشه() ? nodep->name() : '.' + nodep->name();
        } وإلا إذا (!m_hierInst.تشه()) {
            m_state.m_liblistInst[m_hierInst].emplace_شا(nodep->name());
        }
    }

    فراغ لیدنه(AstConfigUse* nodep) override {
        إذا (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } وإلا إذا (!m_cell.تشه()) {
            m_state.m_uselistCell[m_cell].emplace_شا(nodep->libname(), nodep->cellname());
        } وإلا إذا (m_dotp) {
            m_hierInst += m_hierInst.تشه() ? nodep->name() : '.' + nodep->name();
        } وإلا إذا (!m_hierInst.تشه()) {
            m_state.m_uselistInst[m_hierInst].emplace_شا(nodep->libname(), nodep->cellname());
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        إذا (!m_state.m_designs.اندازه() && !v3Global.opt.سر_Module().تشه()) {
            m_state.m_designs.emplace_شا(v3Global.opt.work(), v3Global.opt.سر_Module());
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
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;

    // STATE
    VInFilter* ثابت m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    AstVar* m_varp = تش_اشاره;  // Current variable
    VSym_ګراف m_mods;  // Symbol table of all module names
    Link_Cells_ګراف m_ګراف;  // Linked graph of all cell interconnects
    Library_راس* m_libVertexp = تش_اشاره;  // Vertex at root of all libraries
    ثابت V3_ګراف_راس* m_سر_Vertexp = تش_اشاره;  // Vertex of top module
    std::unordered_سېټ<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_سر_Module_Name;  // original name of the top module
    صحيح m_mod_ژوروالی = 0;  // Depth of the current module

    // METHODS
    V3_ګراف_راس* راس(Ast_غوټه_Module* nodep) {
        // Return corresponding vertex for this module
        إذا (!nodep->user1p()) nodep->user1p(جديد Link_Cells_راس{&m_ګراف, nodep});
        أرجع nodep->user1u().منزل_ګراف_راس();
    }
    فراغ new_څنډه(V3_ګراف_راس* fromp, V3_ګراف_راس* سر, صحيح وزن_2, منطقي cuttable) {
        ثابت V3_ګراف_څنډه* ثابت edgep = جديد V3_ګراف_څنډه{&m_ګراف, fromp, سر, وزن_2, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << سر->name());
    }
    فراغ cell_څنډه(V3_ګراف_راس* fromp, V3_ګراف_راس* سر, صحيح وزن_2, منطقي cuttable,
                  AstCell* cellp) {
        ثابت V3_ګراف_څنډه* ثابت edgep
            = جديد Cell_څنډه{&m_ګراف, fromp, سر, وزن_2, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << سر->name());
    }
    فراغ ورننباسه_Mod_دننه_Lib(ثابت string& name, ثابت string& libname, Ast_غوټه_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->پیدا_کړه_Id_Flat(libname);
        إذا (!libSymp)
            libSymp = m_mods.rootp()->ورننباسه(libname, جديد VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->ورننباسه(name, جديد VSymEnt{&m_mods, nodep});
    }

    Ast_غوټه_Module* پیدا_کړه_Module_Lib_Sym(ثابت string& modName, ثابت string& libname) {
        // Given module name and library name, find within that exact library
        ثابت VSymEnt* ثابت libSymp = m_mods.rootp()->پیدا_کړه_Id_Fallback(libname);
        إذا (!libSymp) أرجع تش_اشاره;
        ثابت VSymEnt* ثابت foundp = libSymp->پیدا_کړه_Id_Fallback(modName);
        أرجع foundp ? VN_AS(foundp->nodep(), غوټه_Module) : تش_اشاره;
    }
    Ast_غوټه_Module* پیدا_کړه_Module_Sym(ثابت string& modName, ثابت string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_غوټه_Module* foundp;
        ثابت std::string ډک_Name = libname + "." + modName;
        // First search cell-specific use list
        ثابت خپلکاری itCellUseList = m_state.m_uselistCell.پیدا_کړه(ډک_Name);
        إذا (itCellUseList != m_state.m_uselistCell.پای_ټکی()) {
            لكل (خپلکاری ثابت& u : itCellUseList->دوهم) {
                foundp = پیدا_کړه_Module_Lib_Sym(u.دوهم, u.لومړی);
                إذا (foundp) أرجع foundp;
            }
        }
        // Then search cell-specific library list
        ثابت خپلکاری itCellLibList = m_state.m_liblistCell.پیدا_کړه(ډک_Name);
        إذا (itCellLibList != m_state.m_liblistCell.پای_ټکی()) {
            لكل (خپلکاری ثابت& l : itCellLibList->دوهم) {
                foundp = پیدا_کړه_Module_Lib_Sym(modName, l);
                إذا (foundp) أرجع foundp;
            }
        }
        // Then search IEEE config default liblist
        لكل (خپلکاری ثابت& l : m_state.m_liblistDefault) {
            foundp = پیدا_کړه_Module_Lib_Sym(modName, l);
            إذا (foundp) أرجع foundp;
        }
        // Then search local library
        foundp = پیدا_کړه_Module_Lib_Sym(modName, libname);
        إذا (foundp) أرجع foundp;
        // Then search global
        foundp = پیدا_کړه_Module_Lib_Sym(modName, "__GLOBAL");
        أرجع foundp;
    }

    Ast_غوټه_Module* resolveModule(Ast_غوټه* nodep, ثابت string& modName, ثابت string& libname) {
        Ast_غوټه_Module* modp = پیدا_کړه_Module_Sym(modName, libname);
        إذا (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ثابت string prettyName = Ast_غوټه::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, ریښتیا, خطأ, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            read_Mod_نومونه();
            // Check again
            modp = پیدا_کړه_Module_Sym(modName, libname);
            إذا (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        أرجع modp;
    }

    ساكن فراغ removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        إذا (!v3Global.opt.سر_Module().تشه()) أرجع;
        لكل (Ast_غوټه_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), غوټه_Module)) {
            إذا (!nodep->دننه_Library()) أرجع;
        }
        لكل (Ast_غوټه_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), غوټه_Module)) {
            nodep->دننه_Library(خطأ);
        }
    }

    AstCell* پیدا_کړه_Cell_By_Hier(AstNetlist* nodep, ثابت std::string& hier_لار) {
        std::stringstream ss(hier_لار);
        std::string سر;
        منطقي سر_وموندل = خطأ;
        std::getline(ss, سر, '.');
        لكل (خپلکاری ثابت& pair : m_state.m_designs) {
            إذا (سر == pair.دوهم) {
                سر_وموندل = ریښتیا;
                اكسر;
            }
        }
        إذا (!سر_وموندل) {
            nodep->v3error("Can't find top-level module for instance path: '" << hier_لار << "'");
            V3Error::abortIfErrors();
            أرجع تش_اشاره;
        }

        ثابت V3_ګراف_راس* vtx = m_سر_Vertexp;
        ثابت Cell_څنډه* finalEdgep = تش_اشاره;
        std::string seg;
        طالما (std::getline(ss, seg, '.')) {
            finalEdgep = تش_اشاره;
            لكل (ثابت V3_ګراف_څنډه& څنډه : vtx->بهر_څنډې()) {
                إذا (ثابت Cell_څنډه* ثابت cedgep = څنډه.cast<Cell_څنډه>()) {
                    إذا (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->سر();
                        finalEdgep = cedgep;
                        اكسر;
                    }
                }
            }
            إذا (!finalEdgep) أرجع تش_اشاره;
        }

        أرجع finalEdgep->cellp();
    }

    فراغ ټاکل_Param_Numbers(AstPin* ثابت لومړی_Pinp) {
        لكل (AstPin* pinp = لومړی_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(ریښتیا);
            إذا (pinp->name() == "") pinp->name("__paramNumber" + cvt_منزل_Str(pinp->pin_عدد()));
        }
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        read_Mod_نومونه();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_غوټه_Module* modp = تش_اشاره;
        AstCell* cellp = تش_اشاره;

        // Search liblists for each instance
        لكل (خپلکاری ثابت& pair : m_state.m_liblistInst) {
            cellp = پیدا_کړه_Cell_By_Hier(nodep, pair.لومړی);
            إذا (!cellp) استمر;
            لكل (خپلکاری ثابت& libname : pair.دوهم) {
                modp = پیدا_کړه_Module_Lib_Sym(cellp->modName(), libname);
                إذا (modp) {
                    cellp->modp(modp);
                    اكسر;
                }
            }
        }

        // Search uselists for each instance
        لكل (خپلکاری ثابت& pair : m_state.m_uselistInst) {
            cellp = پیدا_کړه_Cell_By_Hier(nodep, pair.لومړی);
            لكل (خپلکاری ثابت& u : pair.دوهم) {
                modp = پیدا_کړه_Module_Lib_Sym(u.دوهم, u.لومړی);
                إذا (modp) {
                    cellp->modp(modp);
                    اكسر;
                }
            }
        }

        // Find levels in graph
        m_ګراف.remove_Redundant_څنډې_Max(&V3_ګراف_څنډه::followAlwaysTrue);
        إذا (dump_ګراف_Level()) m_ګراف.dumpDotFilePrefixed("linkcells");
        m_ګراف.rank();
        لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (ثابت Link_Cells_راس* ثابت vvertexp = vtx.cast<Link_Cells_راس>()) {
                Ast_غوټه_Module* ثابت vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_ګراف.rankMin();
        لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (ثابت Link_Cells_راس* ثابت vvertexp = vtx.cast<Link_Cells_راس>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_غوټه_Module* ثابت vmodp = vvertexp->modp();
                vmodp->ژوروالی(vvertexp->rank() + 1);
            }
        }
        إذا (v3Global.opt.سر_Module() != "" && !m_سر_Vertexp) {
            VSpellCheck spell;
            لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
                إذا (ثابت Link_Cells_راس* ثابت vvertexp = vtx.cast<Link_Cells_راس>()) {
                    Ast_غوټه_Module* ثابت vmodp = vvertexp->modp();
                    إذا (VN_IS(vmodp, Module)) spell.ټېل_Candidate(vmodp->prettyName());
                }
            }
            ثابت string suggest
                = spell.غوره_Candidate_پیغام(Ast_غوټه::prettyName(v3Global.opt.سر_Module()));
            v3error("Specified --top-module '"
                    << Ast_غوټه::prettyName(v3Global.opt.سر_Module())
                    << "' was not found in design.\n"
                    << (suggest.تشه() ? "" : V3Error::warn_نور() + suggest));
        }
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_mod_ژوروالی);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            إذا (m_modp) new_څنډه(راس(m_modp), راس(nodep), 1, خطأ);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            راس(m_modp);
            ++m_mod_ژوروالی;
            UINFO(4, "Link Module: " << nodep);
            إذا (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, Not_وموندل_Module) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_mod_ژوروالی == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ثابت خپلکاری it_وموندل_Pair = m_declfnWarned.ورننباسه(nodep->fileline()->filename());
                إذا (it_وموندل_Pair.دوهم) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            إذا (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->دننه_Library(ریښتیا);  // Interfaces can't be at top, unless asked
            }

            خپلکاری ثابت& ډک_Name
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ثابت منطقي سر_سمون
                = std::پیدا_کړه(m_state.m_designs.begin(), m_state.m_designs.پای_ټکی(), ډک_Name)
                  != m_state.m_designs.پای_ټکی();
            إذا (سر_سمون) {
                m_سر_Vertexp = راس(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->دننه_Library(خطأ);  // Safer to make sure it doesn't disappear
            }
            إذا (v3Global.opt.سر_Module().تشه()
                    ? nodep->دننه_Library()  // Library cells are lower
                    : !سر_سمون) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                إذا (!m_libVertexp) m_libVertexp = جديد Library_راس{&m_ګراف};
                new_څنډه(m_libVertexp, راس(nodep), 1, خطأ);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_ونه();
        }
    }

    فراغ لیدنه(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_غوټه_Module* ثابت modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        إذا (modp) {
            إذا (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                إذا (!nodep->isVirtual()) new_څنډه(راس(m_modp), راس(modp), 1, خطأ);
                إذا (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } وإلا إذا (VN_IS(modp, Not_وموندل_Module)) {  // Will error out later
            } وإلا {
                nodep->v3error("Non-interface used as an interface: "
                               << nodep->ifaceNameQ() << "\n"
                               << nodep->warn_نور()
                                      + "... Perhaps intended an instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 23.3.2)?");
            }
        }
        iterateChildren(nodep);
        ټاکل_Param_Numbers(nodep->paramsp());
        // Parser didn't know what was interface, resolve now
        // For historical reasons virtual interface reference variables remain VARs
        إذا (m_varp && !nodep->isVirtual()) m_varp->سېټ_Iface_Ref();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    فراغ لیدنه(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        إذا (!nodep->packagep()) {
            Ast_غوټه_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            إذا (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            إذا (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                أرجع;
            }
        }
    }
    فراغ لیدنه(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        إذا (!nodep->packagep()) {
            Ast_غوټه_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            إذا (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            إذا (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                أرجع;
            }
        }
        new_څنډه(راس(m_modp), راس(nodep->packagep()), 1, خطأ);
    }

    فراغ لیدنه(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_غوټه_Module* ثابت modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        إذا (modp) {
            Ast_غوټه* ثابت cellsp = nodep->cellsp()->unlink_Fr_شا_With_بل();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->جمع_کړه_Stmtsp(cellsp);
            iterate_And_بل_Null(cellsp);
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    فراغ لیدنه(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ثابت منطقي cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        إذا (nodep->user1p() == m_modp) أرجع;  // AstBind and AstNodeModule may call a cell twice
        إذا (nodep->modName() == m_orig_سر_Module_Name) {
            إذا (v3Global.opt.hier_ماشوم() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } وإلا {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
        }
        nodep->user1p(m_modp);
        //
        إذا (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_غوټه_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            إذا (cellmodp) {
                إذا (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(ریښتیا);
                    nodep->recursive(ریښتیا);
                    إذا (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_غوټه_Module* otherModp = VN_CAST(cellmodp->user2p(), غوټه_Module);
                        إذا (!otherModp) {
                            otherModp = cellmodp->clone_ونه(خطأ);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(تش_اشاره);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(ریښتیا);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->جمع_کړه_Modulesp(otherModp);
                            new_څنډه(راس(cellmodp), راس(otherModp), 1, خطأ);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } وإلا {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } وإلا {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cell_څنډه(راس(m_modp), راس(cellmodp), 1, خطأ, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        إذا (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ټېل_Deletep(nodep->pinsp()->unlink_Fr_شا_With_بل());
        }
        إذا (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ټېل_Deletep(nodep->paramsp()->unlink_Fr_شا_With_بل());
        }
        // Convert .* to list of pins
        منطقي pin_ستوری = خطأ;
        منطقي pinDotName = خطأ;
        لكل (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            إذا (pinp->svDotName()) pinDotName = ریښتیا;
            إذا (pinp->dot_ستوری()) {
                إذا (pin_ستوری) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pin_ستوری = ریښتیا;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlink_Fr_شا()->delete_ونه(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->name() == "") pinp->name("__pinNumber" + cvt_منزل_Str(pinp->pin_عدد()));
        }
        ټاکل_Param_Numbers(nodep->paramsp());
        إذا (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_سېټ<string> ports;  // Symbol table of all connected port names
            لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا ((pin_ستوری || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                إذا (!pinp->exprp()) {
                    إذا (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } وإلا {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.ورننباسه(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            لكل (Ast_غوټه* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                إذا (ثابت Ast_پورټ* ثابت portp = VN_CAST(portnodep, پورټ_2)) {

                    إذا (ports.پیدا_کړه(portp->name()) == ports.پای_ټکی()
                        && ports.پیدا_کړه("__pinNumber" + cvt_منزل_Str(portp->pin_عدد())) == ports.پای_ټکی()) {
                        إذا (pin_ستوری) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ثابت newp
                                = جديد AstPin{nodep->fileline(), 0, portp->name(),
                                             جديد AstParseRef{nodep->fileline(), portp->name(),
                                                             تش_اشاره, تش_اشاره}};
                            newp->svDotName(ریښتیا);
                            newp->svImplicit(ریښتیا);
                            nodep->جمع_کړه_Pinsp(newp);
                        } وإلا {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ثابت AstVar* portp_varp = تش_اشاره;
                            لكل (Ast_غوټه* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                إذا (ثابت AstVar* ثابت varp = VN_CAST(module_stmtsp, Var)) {
                                    إذا (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        اكسر;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            إذا (portp_varp && portp_varp->is_ننوت() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ثابت newp
                                    = جديد AstPin{nodep->fileline(), 0, portp->name(), تش_اشاره};
                                nodep->جمع_کړه_Pinsp(newp);
                            } وإلا {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ثابت newp
                                    = جديد AstPin{nodep->fileline(), 0, portp->name(), تش_اشاره};
                                nodep->جمع_کړه_Pinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        إذا (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            إذا (!nodep->hasIfaceVar()) {
                ثابت string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* ثابت idtypep = جديد AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(تش_اشاره);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                إذا (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::ویکټور<AstRange*> rangesp;
                    لكل (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ټېل_شا(rp);
                    }
                    Ast_غوټه_DType* dtp = idtypep;
                    لكل (خپلکاری it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = جديد AstUnpackArrayDType{nodep->fileline(), VFlag_ماشوم_DType{}, dtp,
                                                      (*it)->clone_ونه(خطأ)};
                    }
                    varp = جديد AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_ماشوم_DType{}, dtp};
                } وإلا {
                    varp = جديد AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_ماشوم_DType{}, idtypep};
                }
                varp->is_Iface_پلار(ریښتیا);
                nodep->جمع_کړه_بل_دلته(varp);
                nodep->hasIfaceVar(ریښتیا);
            }
        }
        إذا (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    فراغ لیدنه(AstRefDType* nodep) override {
        iterateChildren(nodep);
        ټاکل_Param_Numbers(nodep->paramsp());
        إذا (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_غوټه_Module* ثابت varModp = پیدا_کړه_Module_Sym(nodep->name(), m_modp->libname());
            إذا (AstIface* ثابت ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                new_څنډه(راس(m_modp), راس(ifacep), 1, خطأ);
            }
        }
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        ټاکل_Param_Numbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    فراغ لیدنه(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_بل_Null(nodep->ماشوم_DTypep());
        }
        iterate_And_بل_Null(nodep->delayp());
        iterate_And_بل_Null(nodep->valuep());
        iterate_And_بل_Null(nodep->attrsp());
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

    // METHODS
    فراغ read_Mod_نومونه() {
        // mangled_name, BlockOptions
        ثابت V3_Hier_بلاک_Opt_سېټ& hier_بلاکونه = v3Global.opt.hier_بلاکونه();
        ثابت خپلکاری hierIt = vlstd::as_const(hier_بلاکونه).پیدا_کړه(v3Global.opt.سر_Module());
        UASSERT((hierIt != hier_بلاکونه.پای_ټکی()) == !!v3Global.opt.hier_ماشوم(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        لكل (Ast_غوټه_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), غوټه_Module);
            إذا (v3Global.opt.hier_ماشوم() && nodep->origName() == hierIt->دوهم.origName()) {
                nodep->name(hierIt->لومړی);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ثابت Ast_غوټه_Module* ثابت libFoundp
                = پیدا_کړه_Module_Lib_Sym(nodep->origName(), nodep->libname());
            ثابت Ast_غوټه_Module* ثابت globalFoundp = پیدا_کړه_Module_Lib_Sym(nodep->name(), "__GLOBAL");
            إذا (libFoundp && libFoundp == nodep) {
                // Ok
            } وإلا إذا (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } وإلا إذا (libFoundp) {
                إذا (!(libFoundp->fileline()->warnIsOff(V3_Error_کوډ::MODDUP)
                      || nodep->fileline()->warnIsOff(V3_Error_کوډ::MODDUP)
                      || hier_بلاکونه.پیدا_کړه(nodep->name()) != hier_بلاکونه.پای_ټکی())) {
                    nodep->v3warn(MODDUP, "Duplicate declaration of "
                                              << nodep->verilogKwd() << ": "
                                              << nodep->prettyNameQ() << '\n'
                                              << nodep->warnContextPrimary() << '\n'
                                              << libFoundp->warnOther()
                                              << "... Location of original declaration\n"
                                              << libFoundp->warnContextSecondary());
                }
                nodep->unlink_Fr_شا();
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            } وإلا إذا (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ثابت string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                ورننباسه_Mod_دننه_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                ورننباسه_Mod_دننه_Lib(nodep->name(), "__GLOBAL", nodep);
            } وإلا {
                ورننباسه_Mod_دننه_Lib(nodep->origName(), nodep->libname(), nodep);
                ورننباسه_Mod_دننه_Lib(nodep->name(), "__GLOBAL", nodep);
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
        إذا (v3Global.opt.hier_ماشوم()) {
            ثابت V3_Hier_بلاک_Opt_سېټ& hier_بلاکونه = v3Global.opt.hier_بلاکونه();
            UASSERT(!v3Global.opt.سر_Module().تشه(),
                    "top module must be explicitly specified in hierarchical mode");
            ثابت V3_Hier_بلاک_Opt_سېټ::const_iterator hierIt
                = hier_بلاکونه.پیدا_کړه(v3Global.opt.سر_Module());
            UASSERT(hierIt != hier_بلاکونه.پای_ټکی(),
                    "top module must be listed in --hierarchical-block");
            m_orig_سر_Module_Name = hierIt->دوهم.origName();
        } وإلا {
            m_orig_سر_Module_Name = v3Global.opt.سر_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        إذا (debug() >= 5 || dump_ګراف_Level() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
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
