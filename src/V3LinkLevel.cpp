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
// LINKTOP TRANSFORMATIONS:
//      Utility functions
//          Sort cells by depth
//          Create new MODULE TOP with connections to below signals
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkLevel.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

నిర్మాణం CmpLevel final {
    తార్కిక క్రియాకారకం()(స్థిరరాశి Ast_కణుపు_Module* lhsp, స్థిరరాశి Ast_కణుపు_Module* rhsp) స్థిరరాశి {
        తిరిగి lhsp->level() < rhsp->level();
    }
};

శూన్య V3LinkLevel::mod_క్రమీకరించు_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    కొరకు (Ast_కణుపు_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), కణుపు_Module)) {
        ఒకవేళ (nodep->is_శిఖరం() && !VN_IS(nodep, Not_దొరికింది_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.నెట్టు_వెనుక(nodep);
        }
        mods.నెట్టు_వెనుక(nodep);
    }
    ఒకవేళ (tops.కొలత() >= 2) {
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి secp = tops[1];  // Complain about second one, as first often intended
        ఒకవేళ (!secp->fileline()->warnIsOff(V3_Error_సంకేతం::MULTITOP)) {
            స్వయంచాలక warn_శిఖరం_Modules = [](స్థిరరాశి Ast_కణుపు* స్థిరరాశి secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                కొరకు (Ast_కణుపు* alsop : tops) {
                    ss << secp->warn_మరిన్ని() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                తిరిగి ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_మరిన్ని()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_శిఖరం_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_క్రమీకరించు(mods.begin(), mods.చివర(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    కొరకు (Ast_కణుపు_Module* nodep : mods) nodep->unlink_Fr_వెనుక();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    కొరకు (Ast_కణుపు_Module* nodep : mods) v3Global.rootp()->కూడు_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_వృక్షం("cellsort", అబద్ధం, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3LinkLevel::timescaling(స్థిరరాశి ModVec& mods) {
    // Timescale determination
    స్థిరరాశి Ast_కణుపు_Module* modTimedp = శూన్యసూచిక;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    కొరకు (Ast_కణుపు_Module* modp : mods) {
        కొరకు (Ast_కణుపు *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            ఒకవేళ (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                ఒకవేళ (pragp->pragType() == VPragmaType::TIMEUNIT_సమితి) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_వెనుక()->delete_వృక్షం(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    కొరకు (Ast_కణుపు_Module* modp : mods) {
        ఒకవేళ (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            ఆపు;
        }
    }
    unit = v3Global.opt.సమయం_Compute_Unit(unit);  // Apply override
    ఒకవేళ (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    తార్కిక dunitTimed = అబద్ధం;  // $unit had a timeunit
    ఒకవేళ (స్థిరరాశి AstPackage* స్థిరరాశి upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        ఒకవేళ (!upkgp->timeunit().isNone()) dunitTimed = నిజం;
    }

    కొరకు (Ast_కణుపు_Module* modp : mods) {
        ఒకవేళ (!v3Global.opt.సమయం_Override_Unit().isNone()) modp->timeunit(unit);
        ఒకవేళ (modp->timeunit().isNone()) {
            ఒకవేళ (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.సమయం_Override_Unit().isNone()
                    && v3Global.opt.సమయం_Default_Unit().isNone())
                && modp->timescaleMatters()) {
                modp->v3warn(TIMESCALEMOD,
                             "Timescale missing on this module as other modules have "
                             "it (IEEE 1800-2023 3.14.2.3)\n"
                                 << modp->warnContextPrimary() << '\n'
                                 << modTimedp->warnOther()
                                 << "... Location of module with timescale\n"
                                 << modTimedp->warnContextSecondary());
            }
            modp->timeunit(unit);
        }
    }

    v3Global.rootp()->timescaleSpecified(modTimedp);  // true if some module specifies timescale

    ఒకవేళ (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecision_విలీనం(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

శూన్య V3LinkLevel::wrap_శిఖరం(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_కణుపు_Module* స్థిరరాశి oldmodp = rootp->modulesp();
    ఒకవేళ (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        తిరిగి;
    }
    rootp->resolved_శిఖరం_Module_Name(oldmodp->name());

    Ast_కణుపు_Module* స్థిరరాశి newmodp = కొత్త AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_కణుపు::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_వెనుక_With_తదుపరి();
    newmodp->కూడు_తదుపరి(oldmodp);
    newmodp->లోతు(1);
    newmodp->level(1);
    newmodp->mod_ప్రజా(నిజం);
    newmodp->protect(అబద్ధం);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->కూడు_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_శిఖరం_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    కొరకు (Ast_కణుపు_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), కణుపు_Module)) {
        ఒకవేళ (VN_IS(modp, Package)) {
            AstCell* స్థిరరాశి cellp
                = కొత్త AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), శూన్యసూచిక, శూన్యసూచిక, శూన్యసూచిక};
            cellp->modp(modp);
            newmodp->కూడు_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_కణుపు_Module* స్థిరరాశి modp) {
        ఒకవేళ (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_వృక్షం("wraptop", 0, dump_వృక్షం_Either_Level() >= 6);
}

శూన్య V3LinkLevel::wrap_శిఖరం_Cell(AstNetlist* rootp) {
    Ast_కణుపు_Module* స్థిరరాశి newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_శిఖరం(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    ఉపయోగించి Name_సమితి = std::unordered_సమితి<std::string>;
    Name_సమితి io_పేర్లు;
    Name_సమితి dup_పేర్లు;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    కొరకు (Ast_కణుపు_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), కణుపు_Module);
         oldmodp && oldmodp->is_శిఖరం(); oldmodp = VN_AS(oldmodp->nextp(), కణుపు_Module)) {
        కొరకు (Ast_కణుపు* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి oldvarp = VN_CAST(subnodep, Var)) {
                ఒకవేళ (oldvarp->isIO()) {
                    ఒకవేళ (!io_పేర్లు.చొప్పించు(oldvarp->name()).రెండవ) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_పేర్లు.చొప్పించు(oldvarp->name());
                    }
                } లేకపోతే ఒకవేళ (v3Global.opt.శిఖరం_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి subtypep = oldvarp->subDTypep();
                    ఒకవేళ (VN_IS(subtypep, IfaceRefDType)) {
                        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ఒకవేళ (!ifacerefp->cellp()) {
                            ఒకవేళ (!io_పేర్లు.చొప్పించు(oldvarp->name()).రెండవ) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_పేర్లు.చొప్పించు(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    కొరకు (Ast_కణుపు_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), కణుపు_Module);
         oldmodp && oldmodp->is_శిఖరం(); oldmodp = VN_AS(oldmodp->nextp(), కణుపు_Module)) {
        ఒకవేళ (VN_IS(oldmodp, Package)) కొనసాగు;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* స్థిరరాశి cellp = కొత్త AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().ఖాళీలు() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            శూన్యసూచిక,
            శూన్యసూచిక,
            శూన్యసూచిక};
        cellp->modp(oldmodp);
        newmodp->కూడు_Stmtsp(cellp);

        // Add pins
        కొరకు (Ast_కణుపు* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                ఒకవేళ (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    ఒకవేళ (dup_పేర్లు.కనుగొను(name) != dup_పేర్లు.చివర()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* స్థిరరాశి varp = oldvarp->clone_వృక్షం(అబద్ధం);
                    varp->name(name);
                    varp->protect(అబద్ధం);
                    newmodp->కూడు_Stmtsp(varp);
                    varp->sig_ప్రజా(నిజం);  // User needs to be able to get to it...
                    oldvarp->primaryIO(అబద్ధం);
                    varp->primaryIO(నిజం);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    ఒకవేళ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ఒకవేళ (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(నిజం);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(అబద్ధం);
                    }

                    ఒకవేళ (v3Global.opt.no_Trace_శిఖరం() && varp->isIO()) varp->trace(అబద్ధం);

                    AstPin* స్థిరరాశి pinp = కొత్త AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        కొత్త AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->కూడు_Pinsp(pinp);
                } లేకపోతే ఒకవేళ (v3Global.opt.శిఖరం_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి subtypep = oldvarp->subDTypep();
                    ఒకవేళ (VN_IS(subtypep, IfaceRefDType)) {
                        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ఒకవేళ (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            ఒకవేళ (dup_పేర్లు.కనుగొను(name) != dup_పేర్లు.చివర()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = కొత్త AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              శూన్యసూచిక,
                                                              శూన్యసూచిక,
                                                              శూన్యసూచిక};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->కూడు_Stmtsp(ifacecellp);

                            AstIfaceRefDType* స్థిరరాశి idtypep = కొత్త AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(శూన్యసూచిక);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->కూడు_Typesp(idtypep);

                            AstVar* varp = కొత్త AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->is_Iface_మాతృ(నిజం);
                            ifacecellp->కూడు_తదుపరి_ఇక్కడ(varp);
                            ifacecellp->hasIfaceVar(నిజం);

                            AstPin* స్థిరరాశి pinp
                                = కొత్త AstPin{oldvarp->fileline(), 0, varp->name(),
                                             కొత్త AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->కూడు_Pinsp(pinp);
                        }
                    }
                }
            }
        }
    }
}

శూన్య V3LinkLevel::non_Wrap_శిఖరం(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    ఒకవేళ (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        తిరిగి;
    }

    // For all top modules
    కొరకు (Ast_కణుపు_Module* modp = rootp->modulesp(); modp && modp->is_శిఖరం();
         modp = VN_AS(modp->nextp(), కణుపు_Module)) {
        ఒకవేళ (VN_IS(modp, Package)) కొనసాగు;

        UINFO(5, "LOOP " << modp);

        కొరకు (Ast_కణుపు* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                ఒకవేళ (varp->isIO()) {
                    varp->protect(అబద్ధం);
                    varp->sig_ప్రజా(నిజం);
                    varp->primaryIO(నిజం);
                    ఒకవేళ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ఒకవేళ (v3Global.opt.systemC()) varp->sc(నిజం);
                    ఒకవేళ (v3Global.opt.no_Trace_శిఖరం()) varp->trace(అబద్ధం);
                }
            }
        }
    }
}
