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

صنف Link_Cells_گراف final : عام V3_گراف {
عام:
    Link_Cells_گراف() = افتراضي;
    ~Link_Cells_گراف() override = افتراضي;
    فراغ loops_پیام_Cb(V3_گراف_رأس* vertexp, V3_یال_Func_P یال_Funcp) override;
};

صنف Link_Cells_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Link_Cells_رأس, V3_گراف_رأس)
    Ast_گره_Module* ثابت m_modp;

عام:
    Link_Cells_رأس(V3_گراف* graphp, Ast_گره_Module* modp)
        : V3_گراف_رأس{graphp}
        , m_modp{modp} {}
    ~Link_Cells_رأس() override = افتراضي;
    Ast_گره_Module* modp() ثابت VL_MT_STABLE { أرجع m_modp; }
    string name() ثابت override VL_MT_STABLE { أرجع cvt_مقصد_Hex(modp()) + ' ' + modp()->name(); }
    File_سطر* fileline() ثابت override { أرجع modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ثابت override {
        أرجع m_modp->recursiveClone() ? (1 + v3Global.opt.module_Recursion_عمق()) : 1;
    }
};

صنف Library_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Library_رأس, V3_گراف_رأس)
عام:
    صریح Library_رأس(V3_گراف* graphp)
        : V3_گراف_رأس{graphp} {}
    ~Library_رأس() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { أرجع "*LIBRARY*"; }
};

صنف Cell_یال final : عام V3_گراف_یال {
    VL_RTTI_IMPL(Cell_یال, V3_گراف_یال)
    AstCell* ثابت m_cellp;

عام:
    Cell_یال(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین, صحيح وزن_2, منطقي cutable,
             AstCell* cellp)
        : V3_گراف_یال{graphp, fromp, بالاترین, وزن_2, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ثابت { أرجع m_cellp; }
    string name() ثابت override VL_MT_STABLE {
        أرجع cellp() ? cvt_مقصد_Hex(cellp()) + ' ' + cellp()->name() : "";
    }
};

فراغ Link_Cells_گراف::loops_پیام_Cb(V3_گراف_رأس* vertexp, V3_یال_Func_P یال_Funcp) {
    إذا (ثابت Link_Cells_رأس* ثابت vvertexp = vertexp->cast<Link_Cells_رأس>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warn_بیشتر()
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
    std::وکتور<std::pair<std::string, std::string>> m_designs;
    // Default library lists to search
    std::وکتور<std::string> m_liblistDefault;
    // Library lists for specific cells
    std::unordered_map<std::string, std::وکتور<std::string>> m_liblistCell;
    // Use list for specific cells (libname, cellname)
    std::unordered_map<std::string, std::وکتور<std::pair<std::string, std::string>>>
        m_uselistCell;
    // Library lists for specific insts
    std::unordered_map<std::string, std::وکتور<std::string>> m_liblistInst;
    // Use list for specific insts (libname, cellname)
    std::unordered_map<std::string, std::وکتور<std::pair<std::string, std::string>>>
        m_uselistInst;
};

صنف LinkConfigsVisitor final : عام VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    منطقي m_is_بالاترین = خطأ;  // Whether we're in the top-level config
    منطقي m_isDefault = خطأ;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = اشاره_تهی;  // Current dot being processed

    // VISITORS
    فراغ بازدید(AstConfig* nodep) override {
        VL_RESTORER(m_is_بالاترین);
        ثابت خودکار& پر_Name
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_بالاترین = std::پیدا_کن(m_state.m_designs.begin(), m_state.m_designs.انتها(), پر_Name)
                  != m_state.m_designs.انتها();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.انتها(), پر_Name),
            m_state.m_designs.انتها());
        iterateChildren(nodep);
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
    }

    فراغ بازدید(AstConfigCell* nodep) override {
        إذا (m_is_بالاترین) m_state.m_designs.emplace_پشت(nodep->libname(), nodep->cellname());
    }

    فراغ بازدید(Ast_Config_قاعده* nodep) override {
        إذا (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = درست;
            iterate_And_بعدی_Null(nodep->usep());
        } وإلا إذا (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_بعدی_Null(nodep->usep());
        } وإلا {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_بعدی_Null(nodep->cellp());
            }
            iterate_And_بعدی_Null(nodep->usep());
        }
    }

    فراغ بازدید(AstDot* nodep) override { iterateChildren(nodep); }

    فراغ بازدید(AstParseRef* nodep) override {
        إذا (m_isDefault) {
            m_state.m_liblistDefault.emplace_پشت(nodep->name());
        } وإلا إذا (!m_cell.خالی()) {
            m_state.m_liblistCell[m_cell].emplace_پشت(nodep->name());
        } وإلا إذا (m_dotp) {
            m_hierInst += m_hierInst.خالی() ? nodep->name() : '.' + nodep->name();
        } وإلا إذا (!m_hierInst.خالی()) {
            m_state.m_liblistInst[m_hierInst].emplace_پشت(nodep->name());
        }
    }

    فراغ بازدید(AstConfigUse* nodep) override {
        إذا (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } وإلا إذا (!m_cell.خالی()) {
            m_state.m_uselistCell[m_cell].emplace_پشت(nodep->libname(), nodep->cellname());
        } وإلا إذا (m_dotp) {
            m_hierInst += m_hierInst.خالی() ? nodep->name() : '.' + nodep->name();
        } وإلا إذا (!m_hierInst.خالی()) {
            m_state.m_uselistInst[m_hierInst].emplace_پشت(nodep->libname(), nodep->cellname());
        }
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        إذا (!m_state.m_designs.اندازه() && !v3Global.opt.بالاترین_Module().خالی()) {
            m_state.m_designs.emplace_پشت(v3Global.opt.work(), v3Global.opt.بالاترین_Module());
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
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;

    // STATE
    VInFilter* ثابت m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    AstVar* m_varp = اشاره_تهی;  // Current variable
    VSym_گراف m_mods;  // Symbol table of all module names
    Link_Cells_گراف m_گراف;  // Linked graph of all cell interconnects
    Library_رأس* m_libVertexp = اشاره_تهی;  // Vertex at root of all libraries
    ثابت V3_گراف_رأس* m_بالاترین_Vertexp = اشاره_تهی;  // Vertex of top module
    std::unordered_مجموعه<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_بالاترین_Module_Name;  // original name of the top module
    صحيح m_mod_عمق = 0;  // Depth of the current module

    // METHODS
    V3_گراف_رأس* رأس(Ast_گره_Module* nodep) {
        // Return corresponding vertex for this module
        إذا (!nodep->user1p()) nodep->user1p(جديد Link_Cells_رأس{&m_گراف, nodep});
        أرجع nodep->user1u().مقصد_گراف_رأس();
    }
    فراغ new_یال(V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین, صحيح وزن_2, منطقي cuttable) {
        ثابت V3_گراف_یال* ثابت edgep = جديد V3_گراف_یال{&m_گراف, fromp, بالاترین, وزن_2, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << بالاترین->name());
    }
    فراغ cell_یال(V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین, صحيح وزن_2, منطقي cuttable,
                  AstCell* cellp) {
        ثابت V3_گراف_یال* ثابت edgep
            = جديد Cell_یال{&m_گراف, fromp, بالاترین, وزن_2, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << بالاترین->name());
    }
    فراغ درج_Mod_درون_Lib(ثابت string& name, ثابت string& libname, Ast_گره_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->پیدا_کن_Id_Flat(libname);
        إذا (!libSymp)
            libSymp = m_mods.rootp()->درج(libname, جديد VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->درج(name, جديد VSymEnt{&m_mods, nodep});
    }

    Ast_گره_Module* پیدا_کن_Module_Lib_Sym(ثابت string& modName, ثابت string& libname) {
        // Given module name and library name, find within that exact library
        ثابت VSymEnt* ثابت libSymp = m_mods.rootp()->پیدا_کن_Id_Fallback(libname);
        إذا (!libSymp) أرجع اشاره_تهی;
        ثابت VSymEnt* ثابت foundp = libSymp->پیدا_کن_Id_Fallback(modName);
        أرجع foundp ? VN_AS(foundp->nodep(), گره_Module) : اشاره_تهی;
    }
    Ast_گره_Module* پیدا_کن_Module_Sym(ثابت string& modName, ثابت string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_گره_Module* foundp;
        ثابت std::string پر_Name = libname + "." + modName;
        // First search cell-specific use list
        ثابت خودکار itCellUseList = m_state.m_uselistCell.پیدا_کن(پر_Name);
        إذا (itCellUseList != m_state.m_uselistCell.انتها()) {
            لكل (خودکار ثابت& u : itCellUseList->دومی) {
                foundp = پیدا_کن_Module_Lib_Sym(u.دومی, u.اولی);
                إذا (foundp) أرجع foundp;
            }
        }
        // Then search cell-specific library list
        ثابت خودکار itCellLibList = m_state.m_liblistCell.پیدا_کن(پر_Name);
        إذا (itCellLibList != m_state.m_liblistCell.انتها()) {
            لكل (خودکار ثابت& l : itCellLibList->دومی) {
                foundp = پیدا_کن_Module_Lib_Sym(modName, l);
                إذا (foundp) أرجع foundp;
            }
        }
        // Then search IEEE config default liblist
        لكل (خودکار ثابت& l : m_state.m_liblistDefault) {
            foundp = پیدا_کن_Module_Lib_Sym(modName, l);
            إذا (foundp) أرجع foundp;
        }
        // Then search local library
        foundp = پیدا_کن_Module_Lib_Sym(modName, libname);
        إذا (foundp) أرجع foundp;
        // Then search global
        foundp = پیدا_کن_Module_Lib_Sym(modName, "__GLOBAL");
        أرجع foundp;
    }

    Ast_گره_Module* resolveModule(Ast_گره* nodep, ثابت string& modName, ثابت string& libname) {
        Ast_گره_Module* modp = پیدا_کن_Module_Sym(modName, libname);
        إذا (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ثابت string prettyName = Ast_گره::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, درست, خطأ, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            read_Mod_نامها();
            // Check again
            modp = پیدا_کن_Module_Sym(modName, libname);
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
        إذا (!v3Global.opt.بالاترین_Module().خالی()) أرجع;
        لكل (Ast_گره_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گره_Module)) {
            إذا (!nodep->درون_Library()) أرجع;
        }
        لكل (Ast_گره_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گره_Module)) {
            nodep->درون_Library(خطأ);
        }
    }

    AstCell* پیدا_کن_Cell_By_Hier(AstNetlist* nodep, ثابت std::string& hier_مسیر) {
        std::stringstream ss(hier_مسیر);
        std::string بالاترین;
        منطقي بالاترین_یافت = خطأ;
        std::getline(ss, بالاترین, '.');
        لكل (خودکار ثابت& pair : m_state.m_designs) {
            إذا (بالاترین == pair.دومی) {
                بالاترین_یافت = درست;
                اكسر;
            }
        }
        إذا (!بالاترین_یافت) {
            nodep->v3error("Can't find top-level module for instance path: '" << hier_مسیر << "'");
            V3Error::abortIfErrors();
            أرجع اشاره_تهی;
        }

        ثابت V3_گراف_رأس* vtx = m_بالاترین_Vertexp;
        ثابت Cell_یال* finalEdgep = اشاره_تهی;
        std::string seg;
        طالما (std::getline(ss, seg, '.')) {
            finalEdgep = اشاره_تهی;
            لكل (ثابت V3_گراف_یال& یال : vtx->بیرون_یالها()) {
                إذا (ثابت Cell_یال* ثابت cedgep = یال.cast<Cell_یال>()) {
                    إذا (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->بالاترین();
                        finalEdgep = cedgep;
                        اكسر;
                    }
                }
            }
            إذا (!finalEdgep) أرجع اشاره_تهی;
        }

        أرجع finalEdgep->cellp();
    }

    فراغ تخصیص_بده_Param_Numbers(AstPin* ثابت اولی_Pinp) {
        لكل (AstPin* pinp = اولی_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(درست);
            إذا (pinp->name() == "") pinp->name("__paramNumber" + cvt_مقصد_Str(pinp->pin_رقم()));
        }
    }

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override {
        read_Mod_نامها();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_گره_Module* modp = اشاره_تهی;
        AstCell* cellp = اشاره_تهی;

        // Search liblists for each instance
        لكل (خودکار ثابت& pair : m_state.m_liblistInst) {
            cellp = پیدا_کن_Cell_By_Hier(nodep, pair.اولی);
            إذا (!cellp) استمر;
            لكل (خودکار ثابت& libname : pair.دومی) {
                modp = پیدا_کن_Module_Lib_Sym(cellp->modName(), libname);
                إذا (modp) {
                    cellp->modp(modp);
                    اكسر;
                }
            }
        }

        // Search uselists for each instance
        لكل (خودکار ثابت& pair : m_state.m_uselistInst) {
            cellp = پیدا_کن_Cell_By_Hier(nodep, pair.اولی);
            لكل (خودکار ثابت& u : pair.دومی) {
                modp = پیدا_کن_Module_Lib_Sym(u.دومی, u.اولی);
                إذا (modp) {
                    cellp->modp(modp);
                    اكسر;
                }
            }
        }

        // Find levels in graph
        m_گراف.remove_Redundant_یالها_Max(&V3_گراف_یال::followAlwaysTrue);
        إذا (dump_گراف_Level()) m_گراف.dumpDotFilePrefixed("linkcells");
        m_گراف.rank();
        لكل (V3_گراف_رأس& vtx : m_گراف.vertices()) {
            إذا (ثابت Link_Cells_رأس* ثابت vvertexp = vtx.cast<Link_Cells_رأس>()) {
                Ast_گره_Module* ثابت vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_گراف.rankMin();
        لكل (V3_گراف_رأس& vtx : m_گراف.vertices()) {
            إذا (ثابت Link_Cells_رأس* ثابت vvertexp = vtx.cast<Link_Cells_رأس>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_گره_Module* ثابت vmodp = vvertexp->modp();
                vmodp->عمق(vvertexp->rank() + 1);
            }
        }
        إذا (v3Global.opt.بالاترین_Module() != "" && !m_بالاترین_Vertexp) {
            VSpellCheck spell;
            لكل (V3_گراف_رأس& vtx : m_گراف.vertices()) {
                إذا (ثابت Link_Cells_رأس* ثابت vvertexp = vtx.cast<Link_Cells_رأس>()) {
                    Ast_گره_Module* ثابت vmodp = vvertexp->modp();
                    إذا (VN_IS(vmodp, Module)) spell.افزودن_Candidate(vmodp->prettyName());
                }
            }
            ثابت string suggest
                = spell.بهترین_Candidate_پیام(Ast_گره::prettyName(v3Global.opt.بالاترین_Module()));
            v3error("Specified --top-module '"
                    << Ast_گره::prettyName(v3Global.opt.بالاترین_Module())
                    << "' was not found in design.\n"
                    << (suggest.خالی() ? "" : V3Error::warn_بیشتر() + suggest));
        }
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_mod_عمق);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            إذا (m_modp) new_یال(رأس(m_modp), رأس(nodep), 1, خطأ);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            رأس(m_modp);
            ++m_mod_عمق;
            UINFO(4, "Link Module: " << nodep);
            إذا (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, Not_یافت_Module) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_mod_عمق == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ثابت خودکار it_یافت_Pair = m_declfnWarned.درج(nodep->fileline()->filename());
                إذا (it_یافت_Pair.دومی) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            إذا (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->درون_Library(درست);  // Interfaces can't be at top, unless asked
            }

            خودکار ثابت& پر_Name
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ثابت منطقي بالاترین_تطابق
                = std::پیدا_کن(m_state.m_designs.begin(), m_state.m_designs.انتها(), پر_Name)
                  != m_state.m_designs.انتها();
            إذا (بالاترین_تطابق) {
                m_بالاترین_Vertexp = رأس(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->درون_Library(خطأ);  // Safer to make sure it doesn't disappear
            }
            إذا (v3Global.opt.بالاترین_Module().خالی()
                    ? nodep->درون_Library()  // Library cells are lower
                    : !بالاترین_تطابق) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                إذا (!m_libVertexp) m_libVertexp = جديد Library_رأس{&m_گراف};
                new_یال(m_libVertexp, رأس(nodep), 1, خطأ);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_درخت();
        }
    }

    فراغ بازدید(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        إذا (modp) {
            إذا (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                إذا (!nodep->isVirtual()) new_یال(رأس(m_modp), رأس(modp), 1, خطأ);
                إذا (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } وإلا إذا (VN_IS(modp, Not_یافت_Module)) {  // Will error out later
            } وإلا {
                nodep->v3error("Non-interface used as an interface: "
                               << nodep->ifaceNameQ() << "\n"
                               << nodep->warn_بیشتر()
                                      + "... Perhaps intended an instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 23.3.2)?");
            }
        }
        iterateChildren(nodep);
        تخصیص_بده_Param_Numbers(nodep->paramsp());
        // Parser didn't know what was interface, resolve now
        // For historical reasons virtual interface reference variables remain VARs
        إذا (m_varp && !nodep->isVirtual()) m_varp->مجموعه_Iface_Ref();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    فراغ بازدید(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        إذا (!nodep->packagep()) {
            Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            إذا (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            إذا (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                أرجع;
            }
        }
    }
    فراغ بازدید(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        إذا (!nodep->packagep()) {
            Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            إذا (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            إذا (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                أرجع;
            }
        }
        new_یال(رأس(m_modp), رأس(nodep->packagep()), 1, خطأ);
    }

    فراغ بازدید(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_گره_Module* ثابت modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        إذا (modp) {
            Ast_گره* ثابت cellsp = nodep->cellsp()->unlink_Fr_پشت_With_بعدی();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->جمع_کن_Stmtsp(cellsp);
            iterate_And_بعدی_Null(cellsp);
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
    }

    فراغ بازدید(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ثابت منطقي cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        إذا (nodep->user1p() == m_modp) أرجع;  // AstBind and AstNodeModule may call a cell twice
        إذا (nodep->modName() == m_orig_بالاترین_Module_Name) {
            إذا (v3Global.opt.hier_فرزند() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } وإلا {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
                أرجع;
            }
        }
        nodep->user1p(m_modp);
        //
        إذا (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_گره_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            إذا (cellmodp) {
                إذا (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(درست);
                    nodep->recursive(درست);
                    إذا (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_گره_Module* otherModp = VN_CAST(cellmodp->user2p(), گره_Module);
                        إذا (!otherModp) {
                            otherModp = cellmodp->clone_درخت(خطأ);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(اشاره_تهی);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(درست);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->جمع_کن_Modulesp(otherModp);
                            new_یال(رأس(cellmodp), رأس(otherModp), 1, خطأ);
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
                    cell_یال(رأس(m_modp), رأس(cellmodp), 1, خطأ, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        إذا (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            افزودن_Deletep(nodep->pinsp()->unlink_Fr_پشت_With_بعدی());
        }
        إذا (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            افزودن_Deletep(nodep->paramsp()->unlink_Fr_پشت_With_بعدی());
        }
        // Convert .* to list of pins
        منطقي pin_ستاره = خطأ;
        منطقي pinDotName = خطأ;
        لكل (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            إذا (pinp->svDotName()) pinDotName = درست;
            إذا (pinp->dot_ستاره()) {
                إذا (pin_ستاره) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pin_ستاره = درست;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlink_Fr_پشت()->delete_درخت(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->name() == "") pinp->name("__pinNumber" + cvt_مقصد_Str(pinp->pin_رقم()));
        }
        تخصیص_بده_Param_Numbers(nodep->paramsp());
        إذا (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_مجموعه<string> ports;  // Symbol table of all connected port names
            لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا ((pin_ستاره || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
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
                ports.درج(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            لكل (Ast_گره* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                إذا (ثابت Ast_پورت* ثابت portp = VN_CAST(portnodep, پورت_2)) {

                    إذا (ports.پیدا_کن(portp->name()) == ports.انتها()
                        && ports.پیدا_کن("__pinNumber" + cvt_مقصد_Str(portp->pin_رقم())) == ports.انتها()) {
                        إذا (pin_ستاره) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ثابت newp
                                = جديد AstPin{nodep->fileline(), 0, portp->name(),
                                             جديد AstParseRef{nodep->fileline(), portp->name(),
                                                             اشاره_تهی, اشاره_تهی}};
                            newp->svDotName(درست);
                            newp->svImplicit(درست);
                            nodep->جمع_کن_Pinsp(newp);
                        } وإلا {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ثابت AstVar* portp_varp = اشاره_تهی;
                            لكل (Ast_گره* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
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
                            إذا (portp_varp && portp_varp->is_درون_داد() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ثابت newp
                                    = جديد AstPin{nodep->fileline(), 0, portp->name(), اشاره_تهی};
                                nodep->جمع_کن_Pinsp(newp);
                            } وإلا {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ثابت newp
                                    = جديد AstPin{nodep->fileline(), 0, portp->name(), اشاره_تهی};
                                nodep->جمع_کن_Pinsp(newp);
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
                idtypep->ifacep(اشاره_تهی);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                إذا (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::وکتور<AstRange*> rangesp;
                    لكل (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.افزودن_پشت(rp);
                    }
                    Ast_گره_DType* dtp = idtypep;
                    لكل (خودکار it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = جديد AstUnpackArrayDType{nodep->fileline(), VFlag_فرزند_DType{}, dtp,
                                                      (*it)->clone_درخت(خطأ)};
                    }
                    varp = جديد AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_فرزند_DType{}, dtp};
                } وإلا {
                    varp = جديد AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_فرزند_DType{}, idtypep};
                }
                varp->is_Iface_والد(درست);
                nodep->جمع_کن_بعدی_اینجا(varp);
                nodep->hasIfaceVar(درست);
            }
        }
        إذا (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    فراغ بازدید(AstRefDType* nodep) override {
        iterateChildren(nodep);
        تخصیص_بده_Param_Numbers(nodep->paramsp());
        إذا (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_گره_Module* ثابت varModp = پیدا_کن_Module_Sym(nodep->name(), m_modp->libname());
            إذا (AstIface* ثابت ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                new_یال(رأس(m_modp), رأس(ifacep), 1, خطأ);
            }
        }
    }
    فراغ بازدید(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        تخصیص_بده_Param_Numbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    فراغ بازدید(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_بعدی_Null(nodep->فرزند_DTypep());
        }
        iterate_And_بعدی_Null(nodep->delayp());
        iterate_And_بعدی_Null(nodep->valuep());
        iterate_And_بعدی_Null(nodep->attrsp());
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

    // METHODS
    فراغ read_Mod_نامها() {
        // mangled_name, BlockOptions
        ثابت V3_Hier_بلوک_Opt_مجموعه& hier_بلوکها = v3Global.opt.hier_بلوکها();
        ثابت خودکار hierIt = vlstd::as_const(hier_بلوکها).پیدا_کن(v3Global.opt.بالاترین_Module());
        UASSERT((hierIt != hier_بلوکها.انتها()) == !!v3Global.opt.hier_فرزند(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        لكل (Ast_گره_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گره_Module);
            إذا (v3Global.opt.hier_فرزند() && nodep->origName() == hierIt->دومی.origName()) {
                nodep->name(hierIt->اولی);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ثابت Ast_گره_Module* ثابت libFoundp
                = پیدا_کن_Module_Lib_Sym(nodep->origName(), nodep->libname());
            ثابت Ast_گره_Module* ثابت globalFoundp = پیدا_کن_Module_Lib_Sym(nodep->name(), "__GLOBAL");
            إذا (libFoundp && libFoundp == nodep) {
                // Ok
            } وإلا إذا (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } وإلا إذا (libFoundp) {
                إذا (!(libFoundp->fileline()->warnIsOff(V3_Error_کد::MODDUP)
                      || nodep->fileline()->warnIsOff(V3_Error_کد::MODDUP)
                      || hier_بلوکها.پیدا_کن(nodep->name()) != hier_بلوکها.انتها())) {
                    nodep->v3warn(MODDUP, "Duplicate declaration of "
                                              << nodep->verilogKwd() << ": "
                                              << nodep->prettyNameQ() << '\n'
                                              << nodep->warnContextPrimary() << '\n'
                                              << libFoundp->warnOther()
                                              << "... Location of original declaration\n"
                                              << libFoundp->warnContextSecondary());
                }
                nodep->unlink_Fr_پشت();
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وإلا إذا (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ثابت string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                درج_Mod_درون_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                درج_Mod_درون_Lib(nodep->name(), "__GLOBAL", nodep);
            } وإلا {
                درج_Mod_درون_Lib(nodep->origName(), nodep->libname(), nodep);
                درج_Mod_درون_Lib(nodep->name(), "__GLOBAL", nodep);
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
        إذا (v3Global.opt.hier_فرزند()) {
            ثابت V3_Hier_بلوک_Opt_مجموعه& hier_بلوکها = v3Global.opt.hier_بلوکها();
            UASSERT(!v3Global.opt.بالاترین_Module().خالی(),
                    "top module must be explicitly specified in hierarchical mode");
            ثابت V3_Hier_بلوک_Opt_مجموعه::const_iterator hierIt
                = hier_بلوکها.پیدا_کن(v3Global.opt.بالاترین_Module());
            UASSERT(hierIt != hier_بلوکها.انتها(),
                    "top module must be listed in --hierarchical-block");
            m_orig_بالاترین_Module_Name = hierIt->دومی.origName();
        } وإلا {
            m_orig_بالاترین_Module_Name = v3Global.opt.بالاترین_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        إذا (debug() >= 5 || dump_گراف_Level() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
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
