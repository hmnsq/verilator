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

جماعت Link_Cells_گراف final : عوامی V3_گراف {
عوامی:
    Link_Cells_گراف() = طےشدہ;
    ~Link_Cells_گراف() override = طےشدہ;
    خالی loops_پیغام_Cb(V3_گراف_راس* vertexp, V3_کنارہ_Func_P کنارہ_Funcp) override;
};

جماعت Link_Cells_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Link_Cells_راس, V3_گراف_راس)
    Ast_گرہ_Module* ثابت m_modp;

عوامی:
    Link_Cells_راس(V3_گراف* graphp, Ast_گرہ_Module* modp)
        : V3_گراف_راس{graphp}
        , m_modp{modp} {}
    ~Link_Cells_راس() override = طےشدہ;
    Ast_گرہ_Module* modp() ثابت VL_MT_STABLE { واپس m_modp; }
    string name() ثابت override VL_MT_STABLE { واپس cvt_مقصد_Hex(modp()) + ' ' + modp()->name(); }
    File_سطر* fileline() ثابت override { واپس modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() ثابت override {
        واپس m_modp->recursiveClone() ? (1 + v3Global.opt.module_Recursion_گہرائی()) : 1;
    }
};

جماعت Library_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Library_راس, V3_گراف_راس)
عوامی:
    صریح Library_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    ~Library_راس() override = طےشدہ;
    string name() ثابت override VL_MT_STABLE { واپس "*LIBRARY*"; }
};

جماعت Cell_کنارہ final : عوامی V3_گراف_کنارہ {
    VL_RTTI_IMPL(Cell_کنارہ, V3_گراف_کنارہ)
    AstCell* ثابت m_cellp;

عوامی:
    Cell_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2, منطقی cutable,
             AstCell* cellp)
        : V3_گراف_کنارہ{graphp, fromp, چوٹی, وزن_2, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() ثابت { واپس m_cellp; }
    string name() ثابت override VL_MT_STABLE {
        واپس cellp() ? cvt_مقصد_Hex(cellp()) + ' ' + cellp()->name() : "";
    }
};

خالی Link_Cells_گراف::loops_پیغام_Cb(V3_گراف_راس* vertexp, V3_کنارہ_Func_P کنارہ_Funcp) {
    اگر (ثابت Link_Cells_راس* ثابت vvertexp = vertexp->cast<Link_Cells_راس>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warn_مزید()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } ورنہ {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
ساخت LinkCellsState final {
    // Set of possible top module names from command line and configs
    std::سمتیہ<std::pair<std::string, std::string>> m_designs;
    // Default library lists to search
    std::سمتیہ<std::string> m_liblistDefault;
    // Library lists for specific cells
    std::unordered_map<std::string, std::سمتیہ<std::string>> m_liblistCell;
    // Use list for specific cells (libname, cellname)
    std::unordered_map<std::string, std::سمتیہ<std::pair<std::string, std::string>>>
        m_uselistCell;
    // Library lists for specific insts
    std::unordered_map<std::string, std::سمتیہ<std::string>> m_liblistInst;
    // Use list for specific insts (libname, cellname)
    std::unordered_map<std::string, std::سمتیہ<std::pair<std::string, std::string>>>
        m_uselistInst;
};

جماعت LinkConfigsVisitor final : عوامی VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    منطقی m_is_چوٹی = جھوٹ;  // Whether we're in the top-level config
    منطقی m_isDefault = جھوٹ;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = خالی_اشاریہ;  // Current dot being processed

    // VISITORS
    خالی دورہ(AstConfig* nodep) override {
        VL_RESTORER(m_is_چوٹی);
        ثابت خودکار& بھرا_Name
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_چوٹی = std::ڈھونڈو(m_state.m_designs.begin(), m_state.m_designs.انتہا(), بھرا_Name)
                  != m_state.m_designs.انتہا();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.انتہا(), بھرا_Name),
            m_state.m_designs.انتہا());
        iterateChildren(nodep);
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    خالی دورہ(AstConfigCell* nodep) override {
        اگر (m_is_چوٹی) m_state.m_designs.emplace_عقب(nodep->libname(), nodep->cellname());
    }

    خالی دورہ(Ast_Config_قاعدہ* nodep) override {
        اگر (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = سچ;
            iterate_And_اگلا_Null(nodep->usep());
        } ورنہ اگر (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_اگلا_Null(nodep->usep());
        } ورنہ {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_اگلا_Null(nodep->cellp());
            }
            iterate_And_اگلا_Null(nodep->usep());
        }
    }

    خالی دورہ(AstDot* nodep) override { iterateChildren(nodep); }

    خالی دورہ(AstParseRef* nodep) override {
        اگر (m_isDefault) {
            m_state.m_liblistDefault.emplace_عقب(nodep->name());
        } ورنہ اگر (!m_cell.خالی_خانے()) {
            m_state.m_liblistCell[m_cell].emplace_عقب(nodep->name());
        } ورنہ اگر (m_dotp) {
            m_hierInst += m_hierInst.خالی_خانے() ? nodep->name() : '.' + nodep->name();
        } ورنہ اگر (!m_hierInst.خالی_خانے()) {
            m_state.m_liblistInst[m_hierInst].emplace_عقب(nodep->name());
        }
    }

    خالی دورہ(AstConfigUse* nodep) override {
        اگر (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } ورنہ اگر (!m_cell.خالی_خانے()) {
            m_state.m_uselistCell[m_cell].emplace_عقب(nodep->libname(), nodep->cellname());
        } ورنہ اگر (m_dotp) {
            m_hierInst += m_hierInst.خالی_خانے() ? nodep->name() : '.' + nodep->name();
        } ورنہ اگر (!m_hierInst.خالی_خانے()) {
            m_state.m_uselistInst[m_hierInst].emplace_عقب(nodep->libname(), nodep->cellname());
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        اگر (!m_state.m_designs.سائز() && !v3Global.opt.چوٹی_Module().خالی_خانے()) {
            m_state.m_designs.emplace_عقب(v3Global.opt.work(), v3Global.opt.چوٹی_Module());
        }
        iterate(nodep);
    }
};

جماعت LinkCellsVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE
    VInFilter* ثابت m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstVar* m_varp = خالی_اشاریہ;  // Current variable
    VSym_گراف m_mods;  // Symbol table of all module names
    Link_Cells_گراف m_گراف;  // Linked graph of all cell interconnects
    Library_راس* m_libVertexp = خالی_اشاریہ;  // Vertex at root of all libraries
    ثابت V3_گراف_راس* m_چوٹی_Vertexp = خالی_اشاریہ;  // Vertex of top module
    std::unordered_سیٹ<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_چوٹی_Module_Name;  // original name of the top module
    صحیح m_mod_گہرائی = 0;  // Depth of the current module

    // METHODS
    V3_گراف_راس* راس(Ast_گرہ_Module* nodep) {
        // Return corresponding vertex for this module
        اگر (!nodep->user1p()) nodep->user1p(نیا Link_Cells_راس{&m_گراف, nodep});
        واپس nodep->user1u().مقصد_گراف_راس();
    }
    خالی new_کنارہ(V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2, منطقی cuttable) {
        ثابت V3_گراف_کنارہ* ثابت edgep = نیا V3_گراف_کنارہ{&m_گراف, fromp, چوٹی, وزن_2, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << چوٹی->name());
    }
    خالی cell_کنارہ(V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2, منطقی cuttable,
                  AstCell* cellp) {
        ثابت V3_گراف_کنارہ* ثابت edgep
            = نیا Cell_کنارہ{&m_گراف, fromp, چوٹی, وزن_2, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << چوٹی->name());
    }
    خالی داخل_کرو_Mod_اندر_Lib(ثابت string& name, ثابت string& libname, Ast_گرہ_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->ڈھونڈو_Id_Flat(libname);
        اگر (!libSymp)
            libSymp = m_mods.rootp()->داخل_کرو(libname, نیا VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->داخل_کرو(name, نیا VSymEnt{&m_mods, nodep});
    }

    Ast_گرہ_Module* ڈھونڈو_Module_Lib_Sym(ثابت string& modName, ثابت string& libname) {
        // Given module name and library name, find within that exact library
        ثابت VSymEnt* ثابت libSymp = m_mods.rootp()->ڈھونڈو_Id_Fallback(libname);
        اگر (!libSymp) واپس خالی_اشاریہ;
        ثابت VSymEnt* ثابت foundp = libSymp->ڈھونڈو_Id_Fallback(modName);
        واپس foundp ? VN_AS(foundp->nodep(), گرہ_Module) : خالی_اشاریہ;
    }
    Ast_گرہ_Module* ڈھونڈو_Module_Sym(ثابت string& modName, ثابت string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_گرہ_Module* foundp;
        ثابت std::string بھرا_Name = libname + "." + modName;
        // First search cell-specific use list
        ثابت خودکار itCellUseList = m_state.m_uselistCell.ڈھونڈو(بھرا_Name);
        اگر (itCellUseList != m_state.m_uselistCell.انتہا()) {
            برائے (خودکار ثابت& u : itCellUseList->دوسرا) {
                foundp = ڈھونڈو_Module_Lib_Sym(u.دوسرا, u.پہلا);
                اگر (foundp) واپس foundp;
            }
        }
        // Then search cell-specific library list
        ثابت خودکار itCellLibList = m_state.m_liblistCell.ڈھونڈو(بھرا_Name);
        اگر (itCellLibList != m_state.m_liblistCell.انتہا()) {
            برائے (خودکار ثابت& l : itCellLibList->دوسرا) {
                foundp = ڈھونڈو_Module_Lib_Sym(modName, l);
                اگر (foundp) واپس foundp;
            }
        }
        // Then search IEEE config default liblist
        برائے (خودکار ثابت& l : m_state.m_liblistDefault) {
            foundp = ڈھونڈو_Module_Lib_Sym(modName, l);
            اگر (foundp) واپس foundp;
        }
        // Then search local library
        foundp = ڈھونڈو_Module_Lib_Sym(modName, libname);
        اگر (foundp) واپس foundp;
        // Then search global
        foundp = ڈھونڈو_Module_Lib_Sym(modName, "__GLOBAL");
        واپس foundp;
    }

    Ast_گرہ_Module* resolveModule(Ast_گرہ* nodep, ثابت string& modName, ثابت string& libname) {
        Ast_گرہ_Module* modp = ڈھونڈو_Module_Sym(modName, libname);
        اگر (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            ثابت string prettyName = Ast_گرہ::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, سچ, جھوٹ, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            read_Mod_نام();
            // Check again
            modp = ڈھونڈو_Module_Sym(modName, libname);
            اگر (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        واپس modp;
    }

    ساکن خالی removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        اگر (!v3Global.opt.چوٹی_Module().خالی_خانے()) واپس;
        برائے (Ast_گرہ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گرہ_Module)) {
            اگر (!nodep->اندر_Library()) واپس;
        }
        برائے (Ast_گرہ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گرہ_Module)) {
            nodep->اندر_Library(جھوٹ);
        }
    }

    AstCell* ڈھونڈو_Cell_By_Hier(AstNetlist* nodep, ثابت std::string& hier_راستہ) {
        std::stringstream ss(hier_راستہ);
        std::string چوٹی;
        منطقی چوٹی_ملا = جھوٹ;
        std::getline(ss, چوٹی, '.');
        برائے (خودکار ثابت& pair : m_state.m_designs) {
            اگر (چوٹی == pair.دوسرا) {
                چوٹی_ملا = سچ;
                توڑو;
            }
        }
        اگر (!چوٹی_ملا) {
            nodep->v3error("Can't find top-level module for instance path: '" << hier_راستہ << "'");
            V3Error::abortIfErrors();
            واپس خالی_اشاریہ;
        }

        ثابت V3_گراف_راس* vtx = m_چوٹی_Vertexp;
        ثابت Cell_کنارہ* finalEdgep = خالی_اشاریہ;
        std::string seg;
        جب (std::getline(ss, seg, '.')) {
            finalEdgep = خالی_اشاریہ;
            برائے (ثابت V3_گراف_کنارہ& کنارہ : vtx->برآمد_کنارے()) {
                اگر (ثابت Cell_کنارہ* ثابت cedgep = کنارہ.cast<Cell_کنارہ>()) {
                    اگر (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->چوٹی();
                        finalEdgep = cedgep;
                        توڑو;
                    }
                }
            }
            اگر (!finalEdgep) واپس خالی_اشاریہ;
        }

        واپس finalEdgep->cellp();
    }

    خالی تفویض_Param_Numbers(AstPin* ثابت پہلا_Pinp) {
        برائے (AstPin* pinp = پہلا_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(سچ);
            اگر (pinp->name() == "") pinp->name("__paramNumber" + cvt_مقصد_Str(pinp->pin_عدد()));
        }
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        read_Mod_نام();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_گرہ_Module* modp = خالی_اشاریہ;
        AstCell* cellp = خالی_اشاریہ;

        // Search liblists for each instance
        برائے (خودکار ثابت& pair : m_state.m_liblistInst) {
            cellp = ڈھونڈو_Cell_By_Hier(nodep, pair.پہلا);
            اگر (!cellp) جاری;
            برائے (خودکار ثابت& libname : pair.دوسرا) {
                modp = ڈھونڈو_Module_Lib_Sym(cellp->modName(), libname);
                اگر (modp) {
                    cellp->modp(modp);
                    توڑو;
                }
            }
        }

        // Search uselists for each instance
        برائے (خودکار ثابت& pair : m_state.m_uselistInst) {
            cellp = ڈھونڈو_Cell_By_Hier(nodep, pair.پہلا);
            برائے (خودکار ثابت& u : pair.دوسرا) {
                modp = ڈھونڈو_Module_Lib_Sym(u.دوسرا, u.پہلا);
                اگر (modp) {
                    cellp->modp(modp);
                    توڑو;
                }
            }
        }

        // Find levels in graph
        m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);
        اگر (dump_گراف_Level()) m_گراف.dumpDotFilePrefixed("linkcells");
        m_گراف.rank();
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (ثابت Link_Cells_راس* ثابت vvertexp = vtx.cast<Link_Cells_راس>()) {
                Ast_گرہ_Module* ثابت vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_گراف.rankMin();
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (ثابت Link_Cells_راس* ثابت vvertexp = vtx.cast<Link_Cells_راس>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_گرہ_Module* ثابت vmodp = vvertexp->modp();
                vmodp->گہرائی(vvertexp->rank() + 1);
            }
        }
        اگر (v3Global.opt.چوٹی_Module() != "" && !m_چوٹی_Vertexp) {
            VSpellCheck spell;
            برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
                اگر (ثابت Link_Cells_راس* ثابت vvertexp = vtx.cast<Link_Cells_راس>()) {
                    Ast_گرہ_Module* ثابت vmodp = vvertexp->modp();
                    اگر (VN_IS(vmodp, Module)) spell.ڈالو_Candidate(vmodp->prettyName());
                }
            }
            ثابت string suggest
                = spell.بہترین_Candidate_پیغام(Ast_گرہ::prettyName(v3Global.opt.چوٹی_Module()));
            v3error("Specified --top-module '"
                    << Ast_گرہ::prettyName(v3Global.opt.چوٹی_Module())
                    << "' was not found in design.\n"
                    << (suggest.خالی_خانے() ? "" : V3Error::warn_مزید() + suggest));
        }
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_mod_گہرائی);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            اگر (m_modp) new_کنارہ(راس(m_modp), راس(nodep), 1, جھوٹ);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            راس(m_modp);
            ++m_mod_گہرائی;
            UINFO(4, "Link Module: " << nodep);
            اگر (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, Not_ملا_Module) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_mod_گہرائی == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                ثابت خودکار it_ملا_Pair = m_declfnWarned.داخل_کرو(nodep->fileline()->filename());
                اگر (it_ملا_Pair.دوسرا) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            اگر (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->اندر_Library(سچ);  // Interfaces can't be at top, unless asked
            }

            خودکار ثابت& بھرا_Name
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            ثابت منطقی چوٹی_مطابقت
                = std::ڈھونڈو(m_state.m_designs.begin(), m_state.m_designs.انتہا(), بھرا_Name)
                  != m_state.m_designs.انتہا();
            اگر (چوٹی_مطابقت) {
                m_چوٹی_Vertexp = راس(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->اندر_Library(جھوٹ);  // Safer to make sure it doesn't disappear
            }
            اگر (v3Global.opt.چوٹی_Module().خالی_خانے()
                    ? nodep->اندر_Library()  // Library cells are lower
                    : !چوٹی_مطابقت) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                اگر (!m_libVertexp) m_libVertexp = نیا Library_راس{&m_گراف};
                new_کنارہ(m_libVertexp, راس(nodep), 1, جھوٹ);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_درخت();
        }
    }

    خالی دورہ(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_گرہ_Module* ثابت modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        اگر (modp) {
            اگر (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                اگر (!nodep->isVirtual()) new_کنارہ(راس(m_modp), راس(modp), 1, جھوٹ);
                اگر (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } ورنہ اگر (VN_IS(modp, Not_ملا_Module)) {  // Will error out later
            } ورنہ {
                nodep->v3error("Non-interface used as an interface: "
                               << nodep->ifaceNameQ() << "\n"
                               << nodep->warn_مزید()
                                      + "... Perhaps intended an instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 23.3.2)?");
            }
        }
        iterateChildren(nodep);
        تفویض_Param_Numbers(nodep->paramsp());
        // Parser didn't know what was interface, resolve now
        // For historical reasons virtual interface reference variables remain VARs
        اگر (m_varp && !nodep->isVirtual()) m_varp->سیٹ_Iface_Ref();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    خالی دورہ(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        اگر (!nodep->packagep()) {
            Ast_گرہ_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            اگر (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            اگر (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                واپس;
            }
        }
    }
    خالی دورہ(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        اگر (!nodep->packagep()) {
            Ast_گرہ_Module* ثابت modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            اگر (AstPackage* ثابت pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            اگر (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                واپس;
            }
        }
        new_کنارہ(راس(m_modp), راس(nodep->packagep()), 1, جھوٹ);
    }

    خالی دورہ(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_گرہ_Module* ثابت modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        اگر (modp) {
            Ast_گرہ* ثابت cellsp = nodep->cellsp()->unlink_Fr_عقب_With_اگلا();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->جوڑو_Stmtsp(cellsp);
            iterate_And_اگلا_Null(cellsp);
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    خالی دورہ(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        ثابت منطقی cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        اگر (nodep->user1p() == m_modp) واپس;  // AstBind and AstNodeModule may call a cell twice
        اگر (nodep->modName() == m_orig_چوٹی_Module_Name) {
            اگر (v3Global.opt.hier_بچہ() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } ورنہ {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
        }
        nodep->user1p(m_modp);
        //
        اگر (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_گرہ_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            اگر (cellmodp) {
                اگر (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(سچ);
                    nodep->recursive(سچ);
                    اگر (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_گرہ_Module* otherModp = VN_CAST(cellmodp->user2p(), گرہ_Module);
                        اگر (!otherModp) {
                            otherModp = cellmodp->clone_درخت(جھوٹ);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(خالی_اشاریہ);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(سچ);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->جوڑو_Modulesp(otherModp);
                            new_کنارہ(راس(cellmodp), راس(otherModp), 1, جھوٹ);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } ورنہ {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } ورنہ {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cell_کنارہ(راس(m_modp), راس(cellmodp), 1, جھوٹ, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        اگر (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            ڈالو_Deletep(nodep->pinsp()->unlink_Fr_عقب_With_اگلا());
        }
        اگر (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            ڈالو_Deletep(nodep->paramsp()->unlink_Fr_عقب_With_اگلا());
        }
        // Convert .* to list of pins
        منطقی pin_ستارہ = جھوٹ;
        منطقی pinDotName = جھوٹ;
        برائے (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            اگر (pinp->svDotName()) pinDotName = سچ;
            اگر (pinp->dot_ستارہ()) {
                اگر (pin_ستارہ) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pin_ستارہ = سچ;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlink_Fr_عقب()->delete_درخت(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        برائے (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->name() == "") pinp->name("__pinNumber" + cvt_مقصد_Str(pinp->pin_عدد()));
        }
        تفویض_Param_Numbers(nodep->paramsp());
        اگر (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_سیٹ<string> ports;  // Symbol table of all connected port names
            برائے (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر ((pin_ستارہ || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                اگر (!pinp->exprp()) {
                    اگر (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } ورنہ {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.داخل_کرو(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            برائے (Ast_گرہ* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                اگر (ثابت Ast_پورٹ* ثابت portp = VN_CAST(portnodep, پورٹ_2)) {

                    اگر (ports.ڈھونڈو(portp->name()) == ports.انتہا()
                        && ports.ڈھونڈو("__pinNumber" + cvt_مقصد_Str(portp->pin_عدد())) == ports.انتہا()) {
                        اگر (pin_ستارہ) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* ثابت newp
                                = نیا AstPin{nodep->fileline(), 0, portp->name(),
                                             نیا AstParseRef{nodep->fileline(), portp->name(),
                                                             خالی_اشاریہ, خالی_اشاریہ}};
                            newp->svDotName(سچ);
                            newp->svImplicit(سچ);
                            nodep->جوڑو_Pinsp(newp);
                        } ورنہ {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            ثابت AstVar* portp_varp = خالی_اشاریہ;
                            برائے (Ast_گرہ* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                اگر (ثابت AstVar* ثابت varp = VN_CAST(module_stmtsp, Var)) {
                                    اگر (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        توڑو;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            اگر (portp_varp && portp_varp->is_داخلہ() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* ثابت newp
                                    = نیا AstPin{nodep->fileline(), 0, portp->name(), خالی_اشاریہ};
                                nodep->جوڑو_Pinsp(newp);
                            } ورنہ {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* ثابت newp
                                    = نیا AstPin{nodep->fileline(), 0, portp->name(), خالی_اشاریہ};
                                nodep->جوڑو_Pinsp(newp);
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
                AstIfaceRefDType* ثابت idtypep = نیا AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(خالی_اشاریہ);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                اگر (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::سمتیہ<AstRange*> rangesp;
                    برائے (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.ڈالو_عقب(rp);
                    }
                    Ast_گرہ_DType* dtp = idtypep;
                    برائے (خودکار it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = نیا AstUnpackArrayDType{nodep->fileline(), VFlag_بچہ_DType{}, dtp,
                                                      (*it)->clone_درخت(جھوٹ)};
                    }
                    varp = نیا AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_بچہ_DType{}, dtp};
                } ورنہ {
                    varp = نیا AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_بچہ_DType{}, idtypep};
                }
                varp->is_Iface_والد(سچ);
                nodep->جوڑو_اگلا_یہاں(varp);
                nodep->hasIfaceVar(سچ);
            }
        }
        اگر (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    خالی دورہ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        تفویض_Param_Numbers(nodep->paramsp());
        اگر (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_گرہ_Module* ثابت varModp = ڈھونڈو_Module_Sym(nodep->name(), m_modp->libname());
            اگر (AstIface* ثابت ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                new_کنارہ(راس(m_modp), راس(ifacep), 1, جھوٹ);
            }
        }
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        تفویض_Param_Numbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    خالی دورہ(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_اگلا_Null(nodep->بچہ_DTypep());
        }
        iterate_And_اگلا_Null(nodep->delayp());
        iterate_And_اگلا_Null(nodep->valuep());
        iterate_And_اگلا_Null(nodep->attrsp());
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // METHODS
    خالی read_Mod_نام() {
        // mangled_name, BlockOptions
        ثابت V3_Hier_بلاک_Opt_سیٹ& hier_بلاکیں = v3Global.opt.hier_بلاکیں();
        ثابت خودکار hierIt = vlstd::as_const(hier_بلاکیں).ڈھونڈو(v3Global.opt.چوٹی_Module());
        UASSERT((hierIt != hier_بلاکیں.انتہا()) == !!v3Global.opt.hier_بچہ(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        برائے (Ast_گرہ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گرہ_Module);
            اگر (v3Global.opt.hier_بچہ() && nodep->origName() == hierIt->دوسرا.origName()) {
                nodep->name(hierIt->پہلا);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            ثابت Ast_گرہ_Module* ثابت libFoundp
                = ڈھونڈو_Module_Lib_Sym(nodep->origName(), nodep->libname());
            ثابت Ast_گرہ_Module* ثابت globalFoundp = ڈھونڈو_Module_Lib_Sym(nodep->name(), "__GLOBAL");
            اگر (libFoundp && libFoundp == nodep) {
                // Ok
            } ورنہ اگر (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } ورنہ اگر (libFoundp) {
                اگر (!(libFoundp->fileline()->warnIsOff(V3_Error_رمز::MODDUP)
                      || nodep->fileline()->warnIsOff(V3_Error_رمز::MODDUP)
                      || hier_بلاکیں.ڈھونڈو(nodep->name()) != hier_بلاکیں.انتہا())) {
                    nodep->v3warn(MODDUP, "Duplicate declaration of "
                                              << nodep->verilogKwd() << ": "
                                              << nodep->prettyNameQ() << '\n'
                                              << nodep->warnContextPrimary() << '\n'
                                              << libFoundp->warnOther()
                                              << "... Location of original declaration\n"
                                              << libFoundp->warnContextSecondary());
                }
                nodep->unlink_Fr_عقب();
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            } ورنہ اگر (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                ثابت string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                داخل_کرو_Mod_اندر_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                داخل_کرو_Mod_اندر_Lib(nodep->name(), "__GLOBAL", nodep);
            } ورنہ {
                داخل_کرو_Mod_اندر_Lib(nodep->origName(), nodep->libname(), nodep);
                داخل_کرو_Mod_اندر_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

عوامی:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        اگر (v3Global.opt.hier_بچہ()) {
            ثابت V3_Hier_بلاک_Opt_سیٹ& hier_بلاکیں = v3Global.opt.hier_بلاکیں();
            UASSERT(!v3Global.opt.چوٹی_Module().خالی_خانے(),
                    "top module must be explicitly specified in hierarchical mode");
            ثابت V3_Hier_بلاک_Opt_سیٹ::const_iterator hierIt
                = hier_بلاکیں.ڈھونڈو(v3Global.opt.چوٹی_Module());
            UASSERT(hierIt != hier_بلاکیں.انتہا(),
                    "top module must be listed in --hierarchical-block");
            m_orig_چوٹی_Module_Name = hierIt->دوسرا.origName();
        } ورنہ {
            m_orig_چوٹی_Module_Name = v3Global.opt.چوٹی_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        اگر (debug() >= 5 || dump_گراف_Level() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

خالی V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
