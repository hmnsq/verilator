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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkCells.h"

#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Parse.h"
#అన్తర్భావ "V3SymTable.h"

#అన్తర్భావ <unordered_set>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

తరగతి Link_Cells_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
బహిరంగ:
    Link_Cells_లేఖాచిత్రం() = అప్రమేయ;
    ~Link_Cells_లేఖాచిత్రం() override = అప్రమేయ;
    శూన్య loops_సందేశం_Cb(V3_లేఖాచిత్రం_శీర్షం* vertexp, V3_అంచు_Func_P అంచు_Funcp) override;
};

తరగతి Link_Cells_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Link_Cells_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    Ast_కణుపు_Module* స్థిరరాశి m_modp;

బహిరంగ:
    Link_Cells_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు_Module* modp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_modp{modp} {}
    ~Link_Cells_శీర్షం() override = అప్రమేయ;
    Ast_కణుపు_Module* modp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_modp; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి cvt_గమ్యం_Hex(modp()) + ' ' + modp()->name(); }
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి modp()->fileline(); }
    // Recursive modules get space for maximum recursion
    uint32_t rankAdder() స్థిరరాశి override {
        తిరిగి m_modp->recursiveClone() ? (1 + v3Global.opt.module_Recursion_లోతు()) : 1;
    }
};

తరగతి Library_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Library_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
బహిరంగ:
    స్పష్టమైన Library_శీర్షం(V3_లేఖాచిత్రం* graphp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp} {}
    ~Library_శీర్షం() override = అప్రమేయ;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "*LIBRARY*"; }
};

తరగతి Cell_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(Cell_అంచు, V3_లేఖాచిత్రం_అంచు)
    AstCell* స్థిరరాశి m_cellp;

బహిరంగ:
    Cell_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2, తార్కిక cutable,
             AstCell* cellp)
        : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, బరువు_2, cutable}
        , m_cellp{cellp} {}
    AstCell* cellp() స్థిరరాశి { తిరిగి m_cellp; }
    string name() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి cellp() ? cvt_గమ్యం_Hex(cellp()) + ' ' + cellp()->name() : "";
    }
};

శూన్య Link_Cells_లేఖాచిత్రం::loops_సందేశం_Cb(V3_లేఖాచిత్రం_శీర్షం* vertexp, V3_అంచు_Func_P అంచు_Funcp) {
    ఒకవేళ (స్థిరరాశి Link_Cells_శీర్షం* స్థిరరాశి vvertexp = vertexp->cast<Link_Cells_శీర్షం>()) {
        vvertexp->modp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Recursive multiple modules (module instantiates "
                                 "something leading back to itself): "
                                     << vvertexp->modp()->prettyNameQ() << '\n'
                                     << vvertexp->modp()->warn_మరిన్ని()
                                     << "... note: self-recursion (module instantiating itself "
                                        "directly) is supported.");
        V3Error::abortIfErrors();
    } లేకపోతే {  // Everything should match above, but...
        v3fatalSrc("Recursive instantiations");
    }
}

//######################################################################
// Link state, as a visitor of each AstNode

// State to pass between config parsing and cell linking visitors.
నిర్మాణం LinkCellsState final {
    // Set of possible top module names from command line and configs
    std::సదిశ<std::pair<std::string, std::string>> m_designs;
    // Default library lists to search
    std::సదిశ<std::string> m_liblistDefault;
    // Library lists for specific cells
    std::unordered_map<std::string, std::సదిశ<std::string>> m_liblistCell;
    // Use list for specific cells (libname, cellname)
    std::unordered_map<std::string, std::సదిశ<std::pair<std::string, std::string>>>
        m_uselistCell;
    // Library lists for specific insts
    std::unordered_map<std::string, std::సదిశ<std::string>> m_liblistInst;
    // Use list for specific insts (libname, cellname)
    std::unordered_map<std::string, std::సదిశ<std::pair<std::string, std::string>>>
        m_uselistInst;
};

తరగతి LinkConfigsVisitor final : బహిరంగ VNVisitor {
    // STATE
    LinkCellsState& m_state;  // Context for linking cells
    తార్కిక m_is_శిఖరం = అబద్ధం;  // Whether we're in the top-level config
    తార్కిక m_isDefault = అబద్ధం;  // Whether we're currently in a default clause
    string m_cell;  // Current cell being processed
    string m_hierInst;  // Current hierarchical instance being processed
    AstDot* m_dotp = శూన్యసూచిక;  // Current dot being processed

    // VISITORS
    శూన్య సందర్శించు(AstConfig* nodep) override {
        VL_RESTORER(m_is_శిఖరం);
        స్థిరరాశి స్వయంచాలక& నిండిన_Name
            = std::pair<std::string, std::string>{nodep->libname(), nodep->configname()};
        m_is_శిఖరం = std::కనుగొను(m_state.m_designs.begin(), m_state.m_designs.చివర(), నిండిన_Name)
                  != m_state.m_designs.చివర();
        m_state.m_designs.erase(
            std::remove(m_state.m_designs.begin(), m_state.m_designs.చివర(), నిండిన_Name),
            m_state.m_designs.చివర());
        iterateChildren(nodep);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    శూన్య సందర్శించు(AstConfigCell* nodep) override {
        ఒకవేళ (m_is_శిఖరం) m_state.m_designs.emplace_వెనుక(nodep->libname(), nodep->cellname());
    }

    శూన్య సందర్శించు(Ast_Config_నియమం* nodep) override {
        ఒకవేళ (!nodep->cellp()) {
            VL_RESTORER(m_isDefault);
            m_isDefault = నిజం;
            iterate_And_తదుపరి_Null(nodep->usep());
        } లేకపోతే ఒకవేళ (nodep->isCell()) {
            VL_RESTORER_CLEAR(m_cell);
            m_cell = nodep->cellp()->name();
            iterate_And_తదుపరి_Null(nodep->usep());
        } లేకపోతే {
            VL_RESTORER_COPY(m_hierInst);
            {
                VL_RESTORER(m_dotp);
                m_dotp = VN_AS(nodep->cellp(), Dot);
                iterate_And_తదుపరి_Null(nodep->cellp());
            }
            iterate_And_తదుపరి_Null(nodep->usep());
        }
    }

    శూన్య సందర్శించు(AstDot* nodep) override { iterateChildren(nodep); }

    శూన్య సందర్శించు(AstParseRef* nodep) override {
        ఒకవేళ (m_isDefault) {
            m_state.m_liblistDefault.emplace_వెనుక(nodep->name());
        } లేకపోతే ఒకవేళ (!m_cell.ఖాళీలు()) {
            m_state.m_liblistCell[m_cell].emplace_వెనుక(nodep->name());
        } లేకపోతే ఒకవేళ (m_dotp) {
            m_hierInst += m_hierInst.ఖాళీలు() ? nodep->name() : '.' + nodep->name();
        } లేకపోతే ఒకవేళ (!m_hierInst.ఖాళీలు()) {
            m_state.m_liblistInst[m_hierInst].emplace_వెనుక(nodep->name());
        }
    }

    శూన్య సందర్శించు(AstConfigUse* nodep) override {
        ఒకవేళ (nodep->isConfig()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: hierarchical config rule");
        } లేకపోతే ఒకవేళ (!m_cell.ఖాళీలు()) {
            m_state.m_uselistCell[m_cell].emplace_వెనుక(nodep->libname(), nodep->cellname());
        } లేకపోతే ఒకవేళ (m_dotp) {
            m_hierInst += m_hierInst.ఖాళీలు() ? nodep->name() : '.' + nodep->name();
        } లేకపోతే ఒకవేళ (!m_hierInst.ఖాళీలు()) {
            m_state.m_uselistInst[m_hierInst].emplace_వెనుక(nodep->libname(), nodep->cellname());
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    LinkConfigsVisitor(AstNetlist* nodep, LinkCellsState& state)
        : m_state{state} {
        // Initialize top module from command line option
        ఒకవేళ (!m_state.m_designs.కొలత() && !v3Global.opt.శిఖరం_Module().ఖాళీలు()) {
            m_state.m_designs.emplace_వెనుక(v3Global.opt.work(), v3Global.opt.శిఖరం_Module());
        }
        iterate(nodep);
    }
};

తరగతి LinkCellsVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstNodeModule::user1p()        // V3GraphVertex*    Vertex describing this module
    //   AstNodeModule::user2p()        // AstNodeModule*    clone used for de-recursing
    //   AstCell::user1p()              // ==V3NodeModule* if done, != if unprocessed
    //   AstCell::user2()               // bool   clone renaming completed
    //  Allocated across all readFiles in V3Global::readFiles:
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE
    VInFilter* స్థిరరాశి m_filterp;  // Parser filter
    LinkCellsState& m_state;  // State for linking cells

    // Below state needs to be preserved between each module call.
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstVar* m_varp = శూన్యసూచిక;  // Current variable
    VSym_లేఖాచిత్రం m_mods;  // Symbol table of all module names
    Link_Cells_లేఖాచిత్రం m_లేఖాచిత్రం;  // Linked graph of all cell interconnects
    Library_శీర్షం* m_libVertexp = శూన్యసూచిక;  // Vertex at root of all libraries
    స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* m_శిఖరం_Vertexp = శూన్యసూచిక;  // Vertex of top module
    std::unordered_సమితి<string> m_declfnWarned;  // Files we issued DECLFILENAME on
    string m_orig_శిఖరం_Module_Name;  // original name of the top module
    పూర్ణాంకం m_mod_లోతు = 0;  // Depth of the current module

    // METHODS
    V3_లేఖాచిత్రం_శీర్షం* శీర్షం(Ast_కణుపు_Module* nodep) {
        // Return corresponding vertex for this module
        ఒకవేళ (!nodep->user1p()) nodep->user1p(కొత్త Link_Cells_శీర్షం{&m_లేఖాచిత్రం, nodep});
        తిరిగి nodep->user1u().గమ్యం_లేఖాచిత్రం_శీర్షం();
    }
    శూన్య new_అంచు(V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2, తార్కిక cuttable) {
        స్థిరరాశి V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep = కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, fromp, శిఖరం, బరువు_2, cuttable};
        UINFO(9, "    newEdge " << edgep << " " << fromp->name() << " -> " << శిఖరం->name());
    }
    శూన్య cell_అంచు(V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2, తార్కిక cuttable,
                  AstCell* cellp) {
        స్థిరరాశి V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep
            = కొత్త Cell_అంచు{&m_లేఖాచిత్రం, fromp, శిఖరం, బరువు_2, cuttable, cellp};
        UINFO(9, "    cellEdge " << edgep << " " << fromp->name() << " -> " << శిఖరం->name());
    }
    శూన్య చొప్పించు_Mod_లోపల_Lib(స్థిరరాశి string& name, స్థిరరాశి string& libname, Ast_కణుపు_Module* nodep) {
        // Be able to find the module under it's library using the name it was given
        VSymEnt* libSymp = m_mods.rootp()->కనుగొను_Id_Flat(libname);
        ఒకవేళ (!libSymp)
            libSymp = m_mods.rootp()->చొప్పించు(libname, కొత్త VSymEnt{&m_mods, v3Global.rootp()});
        libSymp->చొప్పించు(name, కొత్త VSymEnt{&m_mods, nodep});
    }

    Ast_కణుపు_Module* కనుగొను_Module_Lib_Sym(స్థిరరాశి string& modName, స్థిరరాశి string& libname) {
        // Given module name and library name, find within that exact library
        స్థిరరాశి VSymEnt* స్థిరరాశి libSymp = m_mods.rootp()->కనుగొను_Id_Fallback(libname);
        ఒకవేళ (!libSymp) తిరిగి శూన్యసూచిక;
        స్థిరరాశి VSymEnt* స్థిరరాశి foundp = libSymp->కనుగొను_Id_Fallback(modName);
        తిరిగి foundp ? VN_AS(foundp->nodep(), కణుపు_Module) : శూన్యసూచిక;
    }
    Ast_కణుపు_Module* కనుగొను_Module_Sym(స్థిరరాశి string& modName, స్థిరరాశి string& libname) {
        // Given module and library to start search in, resolve using config library choices
        Ast_కణుపు_Module* foundp;
        స్థిరరాశి std::string నిండిన_Name = libname + "." + modName;
        // First search cell-specific use list
        స్థిరరాశి స్వయంచాలక itCellUseList = m_state.m_uselistCell.కనుగొను(నిండిన_Name);
        ఒకవేళ (itCellUseList != m_state.m_uselistCell.చివర()) {
            కొరకు (స్వయంచాలక స్థిరరాశి& u : itCellUseList->రెండవ) {
                foundp = కనుగొను_Module_Lib_Sym(u.రెండవ, u.మొదటి);
                ఒకవేళ (foundp) తిరిగి foundp;
            }
        }
        // Then search cell-specific library list
        స్థిరరాశి స్వయంచాలక itCellLibList = m_state.m_liblistCell.కనుగొను(నిండిన_Name);
        ఒకవేళ (itCellLibList != m_state.m_liblistCell.చివర()) {
            కొరకు (స్వయంచాలక స్థిరరాశి& l : itCellLibList->రెండవ) {
                foundp = కనుగొను_Module_Lib_Sym(modName, l);
                ఒకవేళ (foundp) తిరిగి foundp;
            }
        }
        // Then search IEEE config default liblist
        కొరకు (స్వయంచాలక స్థిరరాశి& l : m_state.m_liblistDefault) {
            foundp = కనుగొను_Module_Lib_Sym(modName, l);
            ఒకవేళ (foundp) తిరిగి foundp;
        }
        // Then search local library
        foundp = కనుగొను_Module_Lib_Sym(modName, libname);
        ఒకవేళ (foundp) తిరిగి foundp;
        // Then search global
        foundp = కనుగొను_Module_Lib_Sym(modName, "__GLOBAL");
        తిరిగి foundp;
    }

    Ast_కణుపు_Module* resolveModule(Ast_కణుపు* nodep, స్థిరరాశి string& modName, స్థిరరాశి string& libname) {
        Ast_కణుపు_Module* modp = కనుగొను_Module_Sym(modName, libname);
        ఒకవేళ (!modp) {
            // Read-subfile
            // If file not found, make AstNotFoundModule, rather than error out.
            // We'll throw the error when we know the module will really be needed.
            స్థిరరాశి string prettyName = Ast_కణుపు::prettyName(modName);
            V3Parse parser{v3Global.rootp(), m_filterp};
            // true below -> other simulators treat modules in link-found files as library cells
            parser.parseFile(nodep->fileline(), prettyName, నిజం, అబద్ధం, m_modp->libname(), "",
                             modName);
            V3Error::abortIfErrors();
            // We've read new modules, grab new pointers to their names
            read_Mod_పేర్లు();
            // Check again
            modp = కనుగొను_Module_Sym(modName, libname);
            ఒకవేళ (!modp) {
                // This shouldn't throw a message as parseFile will create
                // a AstNotFoundModule for us
                nodep->v3error("Can't resolve module reference: '" << prettyName << "'");
            }
        }
        తిరిగి modp;
    }

    స్థిర శూన్య removeLibFlag() {
        // If the only NodeModules are in libraries, then presumably user
        // wants to check the library, so clear library flag
        ఒకవేళ (!v3Global.opt.శిఖరం_Module().ఖాళీలు()) తిరిగి;
        కొరకు (Ast_కణుపు_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), కణుపు_Module)) {
            ఒకవేళ (!nodep->లోపల_Library()) తిరిగి;
        }
        కొరకు (Ast_కణుపు_Module* nodep = v3Global.rootp()->modulesp(); nodep;
             nodep = VN_AS(nodep->nextp(), కణుపు_Module)) {
            nodep->లోపల_Library(అబద్ధం);
        }
    }

    AstCell* కనుగొను_Cell_By_Hier(AstNetlist* nodep, స్థిరరాశి std::string& hier_మార్గం) {
        std::stringstream ss(hier_మార్గం);
        std::string శిఖరం;
        తార్కిక శిఖరం_దొరికింది = అబద్ధం;
        std::getline(ss, శిఖరం, '.');
        కొరకు (స్వయంచాలక స్థిరరాశి& pair : m_state.m_designs) {
            ఒకవేళ (శిఖరం == pair.రెండవ) {
                శిఖరం_దొరికింది = నిజం;
                ఆపు;
            }
        }
        ఒకవేళ (!శిఖరం_దొరికింది) {
            nodep->v3error("Can't find top-level module for instance path: '" << hier_మార్గం << "'");
            V3Error::abortIfErrors();
            తిరిగి శూన్యసూచిక;
        }

        స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* vtx = m_శిఖరం_Vertexp;
        స్థిరరాశి Cell_అంచు* finalEdgep = శూన్యసూచిక;
        std::string seg;
        అయితే (std::getline(ss, seg, '.')) {
            finalEdgep = శూన్యసూచిక;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vtx->బహిర్గతం_అంచులు()) {
                ఒకవేళ (స్థిరరాశి Cell_అంచు* స్థిరరాశి cedgep = అంచు.cast<Cell_అంచు>()) {
                    ఒకవేళ (cedgep->cellp()->name() == seg) {
                        vtx = cedgep->శిఖరం();
                        finalEdgep = cedgep;
                        ఆపు;
                    }
                }
            }
            ఒకవేళ (!finalEdgep) తిరిగి శూన్యసూచిక;
        }

        తిరిగి finalEdgep->cellp();
    }

    శూన్య కేటాయించు_Param_Numbers(AstPin* స్థిరరాశి మొదటి_Pinp) {
        కొరకు (AstPin* pinp = మొదటి_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            pinp->param(నిజం);
            ఒకవేళ (pinp->name() == "") pinp->name("__paramNumber" + cvt_గమ్యం_Str(pinp->pin_అంకె()));
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        read_Mod_పేర్లు();
        iterateChildren(nodep);

        // Replace instance module pointers based on config
        Ast_కణుపు_Module* modp = శూన్యసూచిక;
        AstCell* cellp = శూన్యసూచిక;

        // Search liblists for each instance
        కొరకు (స్వయంచాలక స్థిరరాశి& pair : m_state.m_liblistInst) {
            cellp = కనుగొను_Cell_By_Hier(nodep, pair.మొదటి);
            ఒకవేళ (!cellp) కొనసాగు;
            కొరకు (స్వయంచాలక స్థిరరాశి& libname : pair.రెండవ) {
                modp = కనుగొను_Module_Lib_Sym(cellp->modName(), libname);
                ఒకవేళ (modp) {
                    cellp->modp(modp);
                    ఆపు;
                }
            }
        }

        // Search uselists for each instance
        కొరకు (స్వయంచాలక స్థిరరాశి& pair : m_state.m_uselistInst) {
            cellp = కనుగొను_Cell_By_Hier(nodep, pair.మొదటి);
            కొరకు (స్వయంచాలక స్థిరరాశి& u : pair.రెండవ) {
                modp = కనుగొను_Module_Lib_Sym(u.రెండవ, u.మొదటి);
                ఒకవేళ (modp) {
                    cellp->modp(modp);
                    ఆపు;
                }
            }
        }

        // Find levels in graph
        m_లేఖాచిత్రం.remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        ఒకవేళ (dump_లేఖాచిత్రం_Level()) m_లేఖాచిత్రం.dumpDotFilePrefixed("linkcells");
        m_లేఖాచిత్రం.rank();
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (స్థిరరాశి Link_Cells_శీర్షం* స్థిరరాశి vvertexp = vtx.cast<Link_Cells_శీర్షం>()) {
                Ast_కణుపు_Module* స్థిరరాశి vmodp = vvertexp->modp();
                vmodp->level(vvertexp->rank());
            }
        }
        m_లేఖాచిత్రం.rankMin();
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (స్థిరరాశి Link_Cells_శీర్షం* స్థిరరాశి vvertexp = vtx.cast<Link_Cells_శీర్షం>()) {
                // +1 so we leave level 1 for the new wrapper we'll make in a moment
                Ast_కణుపు_Module* స్థిరరాశి vmodp = vvertexp->modp();
                vmodp->లోతు(vvertexp->rank() + 1);
            }
        }
        ఒకవేళ (v3Global.opt.శిఖరం_Module() != "" && !m_శిఖరం_Vertexp) {
            VSpellCheck spell;
            కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
                ఒకవేళ (స్థిరరాశి Link_Cells_శీర్షం* స్థిరరాశి vvertexp = vtx.cast<Link_Cells_శీర్షం>()) {
                    Ast_కణుపు_Module* స్థిరరాశి vmodp = vvertexp->modp();
                    ఒకవేళ (VN_IS(vmodp, Module)) spell.నెట్టు_Candidate(vmodp->prettyName());
                }
            }
            స్థిరరాశి string suggest
                = spell.ఉత్తమ_Candidate_సందేశం(Ast_కణుపు::prettyName(v3Global.opt.శిఖరం_Module()));
            v3error("Specified --top-module '"
                    << Ast_కణుపు::prettyName(v3Global.opt.శిఖరం_Module())
                    << "' was not found in design.\n"
                    << (suggest.ఖాళీలు() ? "" : V3Error::warn_మరిన్ని() + suggest));
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        // Module: Pick up modnames, so we can resolve cells later
        VL_RESTORER(m_mod_లోతు);
        VL_RESTORER(m_modp);
        {
            // For nested modules/classes, child below parent
            ఒకవేళ (m_modp) new_అంచు(శీర్షం(m_modp), శీర్షం(nodep), 1, అబద్ధం);
            m_modp = nodep;
            // Need vertex to levelize even if no edges
            శీర్షం(m_modp);
            ++m_mod_లోతు;
            UINFO(4, "Link Module: " << nodep);
            ఒకవేళ (nodep->fileline()->filebasenameNoExt() != nodep->prettyName()
                && !v3Global.opt.isLibraryFile(nodep->fileline()->filename(), nodep->libname())
                && !VN_IS(nodep, Not_దొరికింది_Module) && !nodep->recursiveClone()
                && !nodep->isDollarUnit() && !nodep->isConstPool() && m_mod_లోతు == 1) {
                // We only complain once per file, otherwise library-like files
                // have a huge mess of warnings
                స్థిరరాశి స్వయంచాలక it_దొరికింది_Pair = m_declfnWarned.చొప్పించు(nodep->fileline()->filename());
                ఒకవేళ (it_దొరికింది_Pair.రెండవ) {
                    nodep->v3warn(DECLFILENAME, "Filename '"
                                                    << nodep->fileline()->filebasenameNoExt()
                                                    << "' does not match " << nodep->typeName()
                                                    << " name: " << nodep->prettyNameQ());
                }
            }
            ఒకవేళ (VN_IS(nodep, Iface) || VN_IS(nodep, Package)) {
                nodep->లోపల_Library(నిజం);  // Interfaces can't be at top, unless asked
            }

            స్వయంచాలక స్థిరరాశి& నిండిన_Name
                = std::pair<std::string, std::string>(nodep->libname(), nodep->name());
            స్థిరరాశి తార్కిక శిఖరం_సరిపోలిక
                = std::కనుగొను(m_state.m_designs.begin(), m_state.m_designs.చివర(), నిండిన_Name)
                  != m_state.m_designs.చివర();
            ఒకవేళ (శిఖరం_సరిపోలిక) {
                m_శిఖరం_Vertexp = శీర్షం(nodep);
                UINFO(2, "Link --top-module: " << nodep);
                nodep->లోపల_Library(అబద్ధం);  // Safer to make sure it doesn't disappear
            }
            ఒకవేళ (v3Global.opt.శిఖరం_Module().ఖాళీలు()
                    ? nodep->లోపల_Library()  // Library cells are lower
                    : !శిఖరం_సరిపోలిక) {  // Any non-specified module is lower
                // Put under a fake vertex so that the graph ranking won't indicate
                // this is a top level module
                ఒకవేళ (!m_libVertexp) m_libVertexp = కొత్త Library_శీర్షం{&m_లేఖాచిత్రం};
                new_అంచు(m_libVertexp, శీర్షం(nodep), 1, అబద్ధం);
            }
            // Note AstBind also has iteration on cells
            iterateChildren(nodep);
            nodep->check_వృక్షం();
        }
    }

    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        UINFO(4, "Link IfaceRef: " << nodep);
        // Use findIdUpward instead of findIdFlat; it doesn't matter for now
        // but we might support modules-under-modules someday.
        Ast_కణుపు_Module* స్థిరరాశి modp = resolveModule(nodep, nodep->ifaceName(), m_modp->libname());
        ఒకవేళ (modp) {
            ఒకవేళ (VN_IS(modp, Iface)) {
                // Track module depths, so can sort list from parent down to children
                ఒకవేళ (!nodep->isVirtual()) new_అంచు(శీర్షం(m_modp), శీర్షం(modp), 1, అబద్ధం);
                ఒకవేళ (!nodep->cellp()) nodep->ifacep(VN_AS(modp, Iface));
            } లేకపోతే ఒకవేళ (VN_IS(modp, Not_దొరికింది_Module)) {  // Will error out later
            } లేకపోతే {
                nodep->v3error("Non-interface used as an interface: "
                               << nodep->ifaceNameQ() << "\n"
                               << nodep->warn_మరిన్ని()
                                      + "... Perhaps intended an instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 23.3.2)?");
            }
        }
        iterateChildren(nodep);
        కేటాయించు_Param_Numbers(nodep->paramsp());
        // Parser didn't know what was interface, resolve now
        // For historical reasons virtual interface reference variables remain VARs
        ఒకవేళ (m_varp && !nodep->isVirtual()) m_varp->సమితి_Iface_Ref();
        // Note cannot do modport resolution here; modports are allowed underneath generates
        UINFO(4, "Link IfaceRef done: " << nodep);
    }

    శూన్య సందర్శించు(AstPackageExport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ఒకవేళ (!nodep->packagep()) {
            Ast_కణుపు_Module* స్థిరరాశి modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ఒకవేళ (AstPackage* స్థిరరాశి pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            ఒకవేళ (!nodep->packagep()) {
                nodep->v3error("Export package not found: " << nodep->prettyPkgNameQ());
                తిరిగి;
            }
        }
    }
    శూన్య సందర్శించు(AstPackageImport* nodep) override {
        // Package Import: We need to do the package before the use of a package
        iterateChildren(nodep);
        ఒకవేళ (!nodep->packagep()) {
            Ast_కణుపు_Module* స్థిరరాశి modp = resolveModule(nodep, nodep->pkgName(), m_modp->libname());
            ఒకవేళ (AstPackage* స్థిరరాశి pkgp = VN_CAST(modp, Package)) nodep->packagep(pkgp);
            // If not found, V3LinkDot will report errors
            ఒకవేళ (!nodep->packagep()) {
                nodep->v3error("Import package not found: " << nodep->prettyPkgNameQ());
                తిరిగి;
            }
        }
        new_అంచు(శీర్షం(m_modp), శీర్షం(nodep->packagep()), 1, అబద్ధం);
    }

    శూన్య సందర్శించు(AstBind* nodep) override {
        // Bind: Has cells underneath that need to be put into the new
        // module, and cells which need resolution
        // TODO this doesn't allow bind to dotted hier names, that would require
        // this move to post param, which would mean we do not auto-read modules
        // and means we cannot compute module levels until later.
        UINFO(4, "Link Bind: " << nodep);
        Ast_కణుపు_Module* స్థిరరాశి modp = resolveModule(nodep, nodep->name(), m_modp->libname());
        ఒకవేళ (modp) {
            Ast_కణుపు* స్థిరరాశి cellsp = nodep->cellsp()->unlink_Fr_వెనుక_With_తదుపరి();
            // Module may have already linked, so need to pick up these new cells
            VL_RESTORER(m_modp);
            m_modp = modp;
            // Important that this adds to end, as next iterate assumes does all cells
            modp->కూడు_Stmtsp(cellsp);
            iterate_And_తదుపరి_Null(cellsp);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    శూన్య సందర్శించు(AstCell* nodep) override {
        // Cell: Resolve its filename.  If necessary, parse it.
        // Execute only once.  Complication is that cloning may result in
        // user1 being set (for pre-clone) so check if user1() matches the
        // m_mod, if 0 never did it, if !=, it is an unprocessed clone
        స్థిరరాశి తార్కిక cloned = (nodep->user1p() && nodep->user1p() != m_modp);
        ఒకవేళ (nodep->user1p() == m_modp) తిరిగి;  // AstBind and AstNodeModule may call a cell twice
        ఒకవేళ (nodep->modName() == m_orig_శిఖరం_Module_Name) {
            ఒకవేళ (v3Global.opt.hier_బిడ్డ() && nodep->modName() == m_modp->origName()) {
                // Only the root of the recursive instantiation can be a hierarchical block.
                nodep->modName(m_modp->name());
            } లేకపోతే {
                // non-top module will be the top module of this run
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
        }
        nodep->user1p(m_modp);
        //
        ఒకవేళ (!nodep->modp() || cloned) {
            UINFO(4, "Link Cell: " << nodep);
            // Use findIdFallback instead of findIdFlat; it doesn't matter for now
            // but we might support modules-under-modules someday.
            Ast_కణుపు_Module* cellmodp = resolveModule(nodep, nodep->modName(), m_modp->libname());
            ఒకవేళ (cellmodp) {
                ఒకవేళ (cellmodp == m_modp || cellmodp->user2p() == m_modp) {
                    UINFO(1, "Self-recursive module " << cellmodp);
                    cellmodp->recursive(నిజం);
                    nodep->recursive(నిజం);
                    ఒకవేళ (!cellmodp->recursiveClone()) {
                        // In the non-Vrcm, which needs to point to Vrcm flavor
                        //
                        // Make a clone which this cell points to
                        // Later, the clone's cells will also point clone'd name
                        // This lets us link the XREFs between the (uncloned) children so
                        // they don't point to the same module which would
                        // break parameter resolution.
                        Ast_కణుపు_Module* otherModp = VN_CAST(cellmodp->user2p(), కణుపు_Module);
                        ఒకవేళ (!otherModp) {
                            otherModp = cellmodp->clone_వృక్షం(అబద్ధం);
                            otherModp->name(otherModp->name() + "__Vrcm");
                            otherModp->user1p(శూన్యసూచిక);  // Need new vertex
                            otherModp->user2p(cellmodp);
                            otherModp->recursiveClone(నిజం);
                            // user1 etc will retain its pre-clone value
                            cellmodp->user2p(otherModp);
                            v3Global.rootp()->కూడు_Modulesp(otherModp);
                            new_అంచు(శీర్షం(cellmodp), శీర్షం(otherModp), 1, అబద్ధం);
                        }
                        cellmodp = otherModp;
                        nodep->modp(cellmodp);
                    } లేకపోతే {
                        // In the Vrcm, which needs to point back to Vrcm flavor
                        // The cell already has the correct resolution (to Vrcm)
                        nodep->modp(cellmodp);
                        // We don't create a V3GraphEdge (as it would be circular)
                    }
                } లేకపోతే {  // Non-recursive
                    // Track module depths, so can sort list from parent down to children
                    nodep->modp(cellmodp);
                    cell_అంచు(శీర్షం(m_modp), శీర్షం(cellmodp), 1, అబద్ధం, nodep);
                }
            }
        }
        // Remove AstCell(AstPin("",nullptr)), it's a side effect of how we parse "()"
        // the empty middle is identical to the empty rule that must find pins in "(,)".
        ఒకవేళ (nodep->pinsp() && !nodep->pinsp()->nextp() && nodep->pinsp()->name() == ""
            && !nodep->pinsp()->exprp()) {
            నెట్టు_Deletep(nodep->pinsp()->unlink_Fr_వెనుక_With_తదుపరి());
        }
        ఒకవేళ (nodep->paramsp() && !nodep->paramsp()->nextp() && nodep->paramsp()->name() == ""
            && !nodep->paramsp()->exprp()) {
            నెట్టు_Deletep(nodep->paramsp()->unlink_Fr_వెనుక_With_తదుపరి());
        }
        // Convert .* to list of pins
        తార్కిక pin_నక్షత్రం = అబద్ధం;
        తార్కిక pinDotName = అబద్ధం;
        కొరకు (AstPin *nextp, *pinp = nodep->pinsp(); pinp; pinp = nextp) {
            nextp = VN_AS(pinp->nextp(), Pin);
            ఒకవేళ (pinp->svDotName()) pinDotName = నిజం;
            ఒకవేళ (pinp->dot_నక్షత్రం()) {
                ఒకవేళ (pin_నక్షత్రం) pinp->v3error("Duplicate .* in an instance (IEEE 1800-2023 23.3.2)");
                pin_నక్షత్రం = నిజం;
                // Done with this fake pin
                VL_DO_DANGLING(pinp->unlink_Fr_వెనుక()->delete_వృక్షం(), pinp);
            }
        }
        // Convert unnamed pins to pin number based assignments
        కొరకు (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ఒకవేళ (pinp->name() == "") pinp->name("__pinNumber" + cvt_గమ్యం_Str(pinp->pin_అంకె()));
        }
        కేటాయించు_Param_Numbers(nodep->paramsp());
        ఒకవేళ (nodep->modp()) {
            nodep->modName(nodep->modp()->name());
            // Note what pins exist
            std::unordered_సమితి<string> ports;  // Symbol table of all connected port names
            కొరకు (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ ((pin_నక్షత్రం || pinDotName) && pinp->name().substr(0, 11) == "__pinNumber") {
                    pinp->v3error("Mixing positional and .*/named instantiation connection"
                                  " (IEEE 1800-2023 23.3.2)");
                }
                ఒకవేళ (!pinp->exprp()) {
                    ఒకవేళ (pinp->name().substr(0, 11) == "__pinNumber") {
                        pinp->v3warn(PINNOCONNECT,
                                     "Instance pin is not connected: " << pinp->prettyNameQ());
                    } లేకపోతే {
                        pinp->v3warn(PINCONNECTEMPTY,
                                     "Instance pin connected by name with empty reference: "
                                         << pinp->prettyNameQ());
                    }
                }
                ports.చొప్పించు(pinp->name());
            }
            // We search ports, rather than in/out declarations as they aren't resolved yet,
            // and it's easier to do it now than in V3LinkDot when we'd need to repeat steps.
            కొరకు (Ast_కణుపు* portnodep = nodep->modp()->stmtsp(); portnodep;
                 portnodep = portnodep->nextp()) {
                ఒకవేళ (స్థిరరాశి Ast_పోర్టు* స్థిరరాశి portp = VN_CAST(portnodep, పోర్టు_2)) {

                    ఒకవేళ (ports.కనుగొను(portp->name()) == ports.చివర()
                        && ports.కనుగొను("__pinNumber" + cvt_గమ్యం_Str(portp->pin_అంకె())) == ports.చివర()) {
                        ఒకవేళ (pin_నక్షత్రం) {
                            UINFO(9, "    need .* PORT  " << portp);
                            // Create any not already connected
                            AstPin* స్థిరరాశి newp
                                = కొత్త AstPin{nodep->fileline(), 0, portp->name(),
                                             కొత్త AstParseRef{nodep->fileline(), portp->name(),
                                                             శూన్యసూచిక, శూన్యసూచిక}};
                            newp->svDotName(నిజం);
                            newp->svImplicit(నిజం);
                            nodep->కూడు_Pinsp(newp);
                        } లేకపోతే {  // warn on the CELL that needs it, not the port

                            // We *might* not want to warn on this port, if it happened to be
                            // an input with a default value in the module declaration. Our
                            // AstPort* (portp) doesn't have that information, but the Module
                            // (nodep->modp()) statements do that have information in an AstVar*
                            // with the same name() as the port. We'll look for that in-line here,
                            // if a port is missing on this instance.

                            // Get the AstVar for this AstPort, if it exists, using this
                            // inefficient O(n) lookup to match the port name.
                            స్థిరరాశి AstVar* portp_varp = శూన్యసూచిక;
                            కొరకు (Ast_కణుపు* module_stmtsp = nodep->modp()->stmtsp(); module_stmtsp;
                                 module_stmtsp = module_stmtsp->nextp()) {
                                ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(module_stmtsp, Var)) {
                                    ఒకవేళ (!varp->isParam() && varp->name() == portp->name()) {
                                        // not a parameter, same name, break, this is our varp
                                        // (AstVar*)
                                        portp_varp = varp;
                                        ఆపు;
                                    }
                                }
                            }

                            // Is the matching Module port: an INPUT, with default value (in
                            // valuep):
                            ఒకవేళ (portp_varp && portp_varp->is_ఆదానం() && portp_varp->valuep()) {
                                // Do not warn
                                // Create b/c not already connected, and it does exist.
                                AstPin* స్థిరరాశి newp
                                    = కొత్త AstPin{nodep->fileline(), 0, portp->name(), శూన్యసూచిక};
                                nodep->కూడు_Pinsp(newp);
                            } లేకపోతే {
                                nodep->v3warn(PINMISSING,
                                              "Instance has missing pin: "
                                                  << portp->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << portp->warnOther()
                                                  << "... Location of port declaration\n"
                                                  << portp->warnContextSecondary());
                                AstPin* స్థిరరాశి newp
                                    = కొత్త AstPin{nodep->fileline(), 0, portp->name(), శూన్యసూచిక};
                                nodep->కూడు_Pinsp(newp);
                            }
                        }
                    }
                }
            }
        }
        ఒకవేళ (VN_IS(nodep->modp(), Iface)) {
            // Cell really is the parent's instantiation of an interface, not a normal module
            // Make sure we have a variable to refer to this cell, so can <ifacename>.<innermember>
            // in the same way that a child does.  Rename though to avoid conflict with cell.
            // This is quite similar to how classes work; when unpacked
            // classes are better supported may remap interfaces to be more
            // like a class.
            ఒకవేళ (!nodep->hasIfaceVar()) {
                స్థిరరాశి string varName
                    = nodep->name() + "__Viftop";  // V3LinkDot looks for this naming
                AstIfaceRefDType* స్థిరరాశి idtypep = కొత్త AstIfaceRefDType{
                    nodep->fileline(), nodep->name(), nodep->modp()->name()};
                idtypep->ifacep(శూన్యసూచిక);  // cellp overrides
                idtypep->cellp(nodep);  // Only set when real parent cell known.
                AstVar* varp;
                ఒకవేళ (nodep->rangep()) {
                    // Multi-dim arrays wrap one UnpackArrayDType per range, innermost first.
                    std::సదిశ<AstRange*> rangesp;
                    కొరకు (AstRange* rp = nodep->rangep(); rp; rp = VN_CAST(rp->nextp(), Range)) {
                        rangesp.నెట్టు_వెనుక(rp);
                    }
                    Ast_కణుపు_DType* dtp = idtypep;
                    కొరకు (స్వయంచాలక it = rangesp.rbegin(); it != rangesp.rend(); ++it) {
                        dtp = కొత్త AstUnpackArrayDType{nodep->fileline(), VFlag_బిడ్డ_DType{}, dtp,
                                                      (*it)->clone_వృక్షం(అబద్ధం)};
                    }
                    varp = కొత్త AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_బిడ్డ_DType{}, dtp};
                } లేకపోతే {
                    varp = కొత్త AstVar{nodep->fileline(), VVarType::IFACEREF, varName,
                                      VFlag_బిడ్డ_DType{}, idtypep};
                }
                varp->is_Iface_మాతృ(నిజం);
                nodep->కూడు_తదుపరి_ఇక్కడ(varp);
                nodep->hasIfaceVar(నిజం);
            }
        }
        ఒకవేళ (nodep->modp()) {  //
            iterateChildren(nodep);
        }
        UINFO(4, " Link Cell done: " << nodep);
    }

    శూన్య సందర్శించు(AstRefDType* nodep) override {
        iterateChildren(nodep);
        కేటాయించు_Param_Numbers(nodep->paramsp());
        ఒకవేళ (m_varp) {  // Parser didn't know what was interface, resolve now
            Ast_కణుపు_Module* స్థిరరాశి varModp = కనుగొను_Module_Sym(nodep->name(), m_modp->libname());
            ఒకవేళ (AstIface* స్థిరరాశి ifacep = VN_CAST(varModp, Iface)) {
                // Might be an interface, but might also not really be due to interface being
                // hidden by another declaration.  Assume it is relevant and order as-if.
                // This is safe because an interface cannot instantiate a module, so false
                // module->interface edges are harmless.
                new_అంచు(శీర్షం(m_modp), శీర్షం(ifacep), 1, అబద్ధం);
            }
        }
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
        iterateChildren(nodep);
        కేటాయించు_Param_Numbers(nodep->paramsp());
        // Inside a class, an extends or reference to another class
        // Note we don't add a V3GraphEdge{vertex(m_modp), vertex(nodep->classOrPackagep()}
        // We could for an extends, but for another reference we cannot, as
        // it is legal to have classes both with parameters that link to each other
    }

    శూన్య సందర్శించు(AstVar* nodep) override {
        {
            VL_RESTORER(m_varp);
            m_varp = nodep;
            iterate_And_తదుపరి_Null(nodep->బిడ్డ_DTypep());
        }
        iterate_And_తదుపరి_Null(nodep->delayp());
        iterate_And_తదుపరి_Null(nodep->valuep());
        iterate_And_తదుపరి_Null(nodep->attrsp());
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    // METHODS
    శూన్య read_Mod_పేర్లు() {
        // mangled_name, BlockOptions
        స్థిరరాశి V3_Hier_ఖండం_Opt_సమితి& hier_ఖండాలు = v3Global.opt.hier_ఖండాలు();
        స్థిరరాశి స్వయంచాలక hierIt = vlstd::as_const(hier_ఖండాలు).కనుగొను(v3Global.opt.శిఖరం_Module());
        UASSERT((hierIt != hier_ఖండాలు.చివర()) == !!v3Global.opt.hier_బిడ్డ(),
                "information of the top module must exist if --hierarchical-child is set");
        // Look at all modules, and store pointers to all module names
        కొరకు (Ast_కణుపు_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), కణుపు_Module);
            ఒకవేళ (v3Global.opt.hier_బిడ్డ() && nodep->origName() == hierIt->రెండవ.origName()) {
                nodep->name(hierIt->మొదటి);  // Change name of this module to be mangled name
                                             // considering parameter
            }
            స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి libFoundp
                = కనుగొను_Module_Lib_Sym(nodep->origName(), nodep->libname());
            స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి globalFoundp = కనుగొను_Module_Lib_Sym(nodep->name(), "__GLOBAL");
            ఒకవేళ (libFoundp && libFoundp == nodep) {
                // Ok
            } లేకపోతే ఒకవేళ (libFoundp && !globalFoundp) {
                // Clones are locally made and don't need to user-resolve globally
                UASSERT_OBJ(nodep->recursiveClone(), nodep,
                            "Module should be found globally if inserted in lib");
            } లేకపోతే ఒకవేళ (libFoundp) {
                ఒకవేళ (!(libFoundp->fileline()->warnIsOff(V3_Error_సంకేతం::MODDUP)
                      || nodep->fileline()->warnIsOff(V3_Error_సంకేతం::MODDUP)
                      || hier_ఖండాలు.కనుగొను(nodep->name()) != hier_ఖండాలు.చివర())) {
                    nodep->v3warn(MODDUP, "Duplicate declaration of "
                                              << nodep->verilogKwd() << ": "
                                              << nodep->prettyNameQ() << '\n'
                                              << nodep->warnContextPrimary() << '\n'
                                              << libFoundp->warnOther()
                                              << "... Location of original declaration\n"
                                              << libFoundp->warnContextSecondary());
                }
                nodep->unlink_Fr_వెనుక();
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (globalFoundp && globalFoundp != nodep) {
                // ...__LIB__ stripped by prettyName
                స్థిరరాశి string newName = nodep->libname() + "__LIB__" + nodep->origName();
                UINFO(9, "Module rename as in multiple libraries " << newName << " <- " << nodep);
                చొప్పించు_Mod_లోపల_Lib(nodep->origName(), nodep->libname(), nodep);  // Original name
                nodep->name(newName);
                చొప్పించు_Mod_లోపల_Lib(nodep->name(), "__GLOBAL", nodep);
            } లేకపోతే {
                చొప్పించు_Mod_లోపల_Lib(nodep->origName(), nodep->libname(), nodep);
                చొప్పించు_Mod_లోపల_Lib(nodep->name(), "__GLOBAL", nodep);
            }
        }
        // if (debug() >= 9) m_mods.dump(cout, "-syms: ");
    }

బహిరంగ:
    // CONSTRUCTORS
    LinkCellsVisitor(AstNetlist* nodep, VInFilter* filterp, LinkCellsState& state)
        : m_filterp{filterp}
        , m_state{state}
        , m_mods{nodep} {
        ఒకవేళ (v3Global.opt.hier_బిడ్డ()) {
            స్థిరరాశి V3_Hier_ఖండం_Opt_సమితి& hier_ఖండాలు = v3Global.opt.hier_ఖండాలు();
            UASSERT(!v3Global.opt.శిఖరం_Module().ఖాళీలు(),
                    "top module must be explicitly specified in hierarchical mode");
            స్థిరరాశి V3_Hier_ఖండం_Opt_సమితి::const_iterator hierIt
                = hier_ఖండాలు.కనుగొను(v3Global.opt.శిఖరం_Module());
            UASSERT(hierIt != hier_ఖండాలు.చివర(),
                    "top module must be listed in --hierarchical-block");
            m_orig_శిఖరం_Module_Name = hierIt->రెండవ.origName();
        } లేకపోతే {
            m_orig_శిఖరం_Module_Name = v3Global.opt.శిఖరం_Module();
        }
        removeLibFlag();
        iterate(nodep);
    }
    ~LinkCellsVisitor() override {
        ఒకవేళ (debug() >= 5 || dump_లేఖాచిత్రం_Level() >= 5) { m_mods.dumpFilePrefixed("linkcells"); }
    }
};

//######################################################################
// Link class functions

శూన్య V3LinkCells::link(AstNetlist* nodep, VInFilter* filterp) {
    UINFO(4, __FUNCTION__ << ": ");
    // Configs must be parsed first because they determine the library search order for linking
    LinkCellsState state;
    { LinkConfigsVisitor{nodep, state}; }
    { LinkCellsVisitor{nodep, filterp, state}; }
}
