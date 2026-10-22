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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkLevel.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

بنية CmpLevel final {
    منطقي عامل()(ثابت Ast_ڳنڍ_Module* lhsp, ثابت Ast_ڳنڍ_Module* rhsp) ثابت {
        أرجع lhsp->level() < rhsp->level();
    }
};

فراغ V3LinkLevel::mod_ترتيب_ڏيو_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    لكل (Ast_ڳنڍ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ڳنڍ_Module)) {
        إذا (nodep->is_چوٽي() && !VN_IS(nodep, Not_مليو_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.ڌڪو_پٺ(nodep);
        }
        mods.ڌڪو_پٺ(nodep);
    }
    إذا (tops.سائيز() >= 2) {
        ثابت Ast_ڳنڍ* ثابت secp = tops[1];  // Complain about second one, as first often intended
        إذا (!secp->fileline()->warnIsOff(V3_Error_ڪوڊ::MULTITOP)) {
            خودڪار warn_چوٽي_Modules = [](ثابت Ast_ڳنڍ* ثابت secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                لكل (Ast_ڳنڍ* alsop : tops) {
                    ss << secp->warn_وڌيڪ() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                أرجع ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_وڌيڪ()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_چوٽي_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_ترتيب_ڏيو(mods.begin(), mods.پڇاڙي(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    لكل (Ast_ڳنڍ_Module* nodep : mods) nodep->unlink_Fr_پٺ();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    لكل (Ast_ڳنڍ_Module* nodep : mods) v3Global.rootp()->جوڙيو_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_وڻ("cellsort", خطأ, dump_وڻ_Either_Level() >= 3);
}

فراغ V3LinkLevel::timescaling(ثابت ModVec& mods) {
    // Timescale determination
    ثابت Ast_ڳنڍ_Module* modTimedp = خالي_اشارو;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    لكل (Ast_ڳنڍ_Module* modp : mods) {
        لكل (Ast_ڳنڍ *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            إذا (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                إذا (pragp->pragType() == VPragmaType::TIMEUNIT_سيٽ) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_پٺ()->delete_وڻ(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    لكل (Ast_ڳنڍ_Module* modp : mods) {
        إذا (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            اكسر;
        }
    }
    unit = v3Global.opt.وقت_Compute_Unit(unit);  // Apply override
    إذا (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    منطقي dunitTimed = خطأ;  // $unit had a timeunit
    إذا (ثابت AstPackage* ثابت upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        إذا (!upkgp->timeunit().isNone()) dunitTimed = سچ;
    }

    لكل (Ast_ڳنڍ_Module* modp : mods) {
        إذا (!v3Global.opt.وقت_Override_Unit().isNone()) modp->timeunit(unit);
        إذا (modp->timeunit().isNone()) {
            إذا (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.وقت_Override_Unit().isNone()
                    && v3Global.opt.وقت_Default_Unit().isNone())
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

    إذا (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecision_ملايو(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

فراغ V3LinkLevel::wrap_چوٽي(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_ڳنڍ_Module* ثابت oldmodp = rootp->modulesp();
    إذا (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        أرجع;
    }
    rootp->resolved_چوٽي_Module_Name(oldmodp->name());

    Ast_ڳنڍ_Module* ثابت newmodp = جديد AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ڳنڍ::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_پٺ_With_ايندڙ();
    newmodp->جوڙيو_ايندڙ(oldmodp);
    newmodp->اونهائي(1);
    newmodp->level(1);
    newmodp->mod_عوامي(سچ);
    newmodp->protect(خطأ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->جوڙيو_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_چوٽي_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    لكل (Ast_ڳنڍ_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ڳنڍ_Module)) {
        إذا (VN_IS(modp, Package)) {
            AstCell* ثابت cellp
                = جديد AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), خالي_اشارو, خالي_اشارو, خالي_اشارو};
            cellp->modp(modp);
            newmodp->جوڙيو_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ڳنڍ_Module* ثابت modp) {
        إذا (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_وڻ("wraptop", 0, dump_وڻ_Either_Level() >= 6);
}

فراغ V3LinkLevel::wrap_چوٽي_Cell(AstNetlist* rootp) {
    Ast_ڳنڍ_Module* ثابت newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_چوٽي(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    استعمال Name_سيٽ = std::unordered_سيٽ<std::string>;
    Name_سيٽ io_نالا;
    Name_سيٽ dup_نالا;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    لكل (Ast_ڳنڍ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ڳنڍ_Module);
         oldmodp && oldmodp->is_چوٽي(); oldmodp = VN_AS(oldmodp->nextp(), ڳنڍ_Module)) {
        لكل (Ast_ڳنڍ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                إذا (oldvarp->isIO()) {
                    إذا (!io_نالا.وجهو(oldvarp->name()).ٻيو) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_نالا.وجهو(oldvarp->name());
                    }
                } وإلا إذا (v3Global.opt.چوٽي_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ثابت Ast_ڳنڍ_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            إذا (!io_نالا.وجهو(oldvarp->name()).ٻيو) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_نالا.وجهو(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    لكل (Ast_ڳنڍ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ڳنڍ_Module);
         oldmodp && oldmodp->is_چوٽي(); oldmodp = VN_AS(oldmodp->nextp(), ڳنڍ_Module)) {
        إذا (VN_IS(oldmodp, Package)) استمر;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ثابت cellp = جديد AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().خالي_جايون() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            خالي_اشارو,
            خالي_اشارو,
            خالي_اشارو};
        cellp->modp(oldmodp);
        newmodp->جوڙيو_Stmtsp(cellp);

        // Add pins
        لكل (Ast_ڳنڍ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                إذا (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    إذا (dup_نالا.ڳوليو(name) != dup_نالا.پڇاڙي()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ثابت varp = oldvarp->clone_وڻ(خطأ);
                    varp->name(name);
                    varp->protect(خطأ);
                    newmodp->جوڙيو_Stmtsp(varp);
                    varp->sig_عوامي(سچ);  // User needs to be able to get to it...
                    oldvarp->primaryIO(خطأ);
                    varp->primaryIO(سچ);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    إذا (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    إذا (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(سچ);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(خطأ);
                    }

                    إذا (v3Global.opt.no_Trace_چوٽي() && varp->isIO()) varp->trace(خطأ);

                    AstPin* ثابت pinp = جديد AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        جديد AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->جوڙيو_Pinsp(pinp);
                } وإلا إذا (v3Global.opt.چوٽي_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ثابت Ast_ڳنڍ_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            إذا (dup_نالا.ڳوليو(name) != dup_نالا.پڇاڙي()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = جديد AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              خالي_اشارو,
                                                              خالي_اشارو,
                                                              خالي_اشارو};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->جوڙيو_Stmtsp(ifacecellp);

                            AstIfaceRefDType* ثابت idtypep = جديد AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(خالي_اشارو);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->جوڙيو_Typesp(idtypep);

                            AstVar* varp = جديد AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->is_Iface_والد(سچ);
                            ifacecellp->جوڙيو_ايندڙ_هتي(varp);
                            ifacecellp->hasIfaceVar(سچ);

                            AstPin* ثابت pinp
                                = جديد AstPin{oldvarp->fileline(), 0, varp->name(),
                                             جديد AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->جوڙيو_Pinsp(pinp);
                        }
                    }
                }
            }
        }
    }
}

فراغ V3LinkLevel::non_Wrap_چوٽي(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    إذا (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        أرجع;
    }

    // For all top modules
    لكل (Ast_ڳنڍ_Module* modp = rootp->modulesp(); modp && modp->is_چوٽي();
         modp = VN_AS(modp->nextp(), ڳنڍ_Module)) {
        إذا (VN_IS(modp, Package)) استمر;

        UINFO(5, "LOOP " << modp);

        لكل (Ast_ڳنڍ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                إذا (varp->isIO()) {
                    varp->protect(خطأ);
                    varp->sig_عوامي(سچ);
                    varp->primaryIO(سچ);
                    إذا (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    إذا (v3Global.opt.systemC()) varp->sc(سچ);
                    إذا (v3Global.opt.no_Trace_چوٽي()) varp->trace(خطأ);
                }
            }
        }
    }
}
