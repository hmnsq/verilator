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
    منطقي چلوونکی()(ثابت Ast_غوټه_Module* lhsp, ثابت Ast_غوټه_Module* rhsp) ثابت {
        أرجع lhsp->level() < rhsp->level();
    }
};

فراغ V3LinkLevel::mod_ترتیبول_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    لكل (Ast_غوټه_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), غوټه_Module)) {
        إذا (nodep->is_سر() && !VN_IS(nodep, Not_وموندل_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.ټېل_شا(nodep);
        }
        mods.ټېل_شا(nodep);
    }
    إذا (tops.اندازه() >= 2) {
        ثابت Ast_غوټه* ثابت secp = tops[1];  // Complain about second one, as first often intended
        إذا (!secp->fileline()->warnIsOff(V3_Error_کوډ::MULTITOP)) {
            خپلکاری warn_سر_Modules = [](ثابت Ast_غوټه* ثابت secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                لكل (Ast_غوټه* alsop : tops) {
                    ss << secp->warn_نور() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                أرجع ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_نور()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_سر_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_ترتیبول(mods.begin(), mods.پای_ټکی(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    لكل (Ast_غوټه_Module* nodep : mods) nodep->unlink_Fr_شا();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    لكل (Ast_غوټه_Module* nodep : mods) v3Global.rootp()->جمع_کړه_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_ونه("cellsort", خطأ, dump_ونه_Either_Level() >= 3);
}

فراغ V3LinkLevel::timescaling(ثابت ModVec& mods) {
    // Timescale determination
    ثابت Ast_غوټه_Module* modTimedp = تش_اشاره;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    لكل (Ast_غوټه_Module* modp : mods) {
        لكل (Ast_غوټه *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            إذا (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                إذا (pragp->pragType() == VPragmaType::TIMEUNIT_سېټ) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_شا()->delete_ونه(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    لكل (Ast_غوټه_Module* modp : mods) {
        إذا (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            اكسر;
        }
    }
    unit = v3Global.opt.وخت_Compute_Unit(unit);  // Apply override
    إذا (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    منطقي dunitTimed = خطأ;  // $unit had a timeunit
    إذا (ثابت AstPackage* ثابت upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        إذا (!upkgp->timeunit().isNone()) dunitTimed = ریښتیا;
    }

    لكل (Ast_غوټه_Module* modp : mods) {
        إذا (!v3Global.opt.وخت_Override_Unit().isNone()) modp->timeunit(unit);
        إذا (modp->timeunit().isNone()) {
            إذا (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.وخت_Override_Unit().isNone()
                    && v3Global.opt.وخت_Default_Unit().isNone())
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
        v3Global.rootp()->timeprecision_یوځای(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

فراغ V3LinkLevel::wrap_سر(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_غوټه_Module* ثابت oldmodp = rootp->modulesp();
    إذا (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        أرجع;
    }
    rootp->resolved_سر_Module_Name(oldmodp->name());

    Ast_غوټه_Module* ثابت newmodp = جديد AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_غوټه::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_شا_With_بل();
    newmodp->جمع_کړه_بل(oldmodp);
    newmodp->ژوروالی(1);
    newmodp->level(1);
    newmodp->mod_عامه(ریښتیا);
    newmodp->protect(خطأ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->جمع_کړه_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_سر_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    لكل (Ast_غوټه_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), غوټه_Module)) {
        إذا (VN_IS(modp, Package)) {
            AstCell* ثابت cellp
                = جديد AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), تش_اشاره, تش_اشاره, تش_اشاره};
            cellp->modp(modp);
            newmodp->جمع_کړه_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_غوټه_Module* ثابت modp) {
        إذا (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_ونه("wraptop", 0, dump_ونه_Either_Level() >= 6);
}

فراغ V3LinkLevel::wrap_سر_Cell(AstNetlist* rootp) {
    Ast_غوټه_Module* ثابت newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_سر(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    کارول Name_سېټ = std::unordered_سېټ<std::string>;
    Name_سېټ io_نومونه;
    Name_سېټ dup_نومونه;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    لكل (Ast_غوټه_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), غوټه_Module);
         oldmodp && oldmodp->is_سر(); oldmodp = VN_AS(oldmodp->nextp(), غوټه_Module)) {
        لكل (Ast_غوټه* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                إذا (oldvarp->isIO()) {
                    إذا (!io_نومونه.ورننباسه(oldvarp->name()).دوهم) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_نومونه.ورننباسه(oldvarp->name());
                    }
                } وإلا إذا (v3Global.opt.سر_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ثابت Ast_غوټه_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            إذا (!io_نومونه.ورننباسه(oldvarp->name()).دوهم) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_نومونه.ورننباسه(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    لكل (Ast_غوټه_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), غوټه_Module);
         oldmodp && oldmodp->is_سر(); oldmodp = VN_AS(oldmodp->nextp(), غوټه_Module)) {
        إذا (VN_IS(oldmodp, Package)) استمر;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ثابت cellp = جديد AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().تشه() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            تش_اشاره,
            تش_اشاره,
            تش_اشاره};
        cellp->modp(oldmodp);
        newmodp->جمع_کړه_Stmtsp(cellp);

        // Add pins
        لكل (Ast_غوټه* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                إذا (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    إذا (dup_نومونه.پیدا_کړه(name) != dup_نومونه.پای_ټکی()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ثابت varp = oldvarp->clone_ونه(خطأ);
                    varp->name(name);
                    varp->protect(خطأ);
                    newmodp->جمع_کړه_Stmtsp(varp);
                    varp->sig_عامه(ریښتیا);  // User needs to be able to get to it...
                    oldvarp->primaryIO(خطأ);
                    varp->primaryIO(ریښتیا);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    إذا (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    إذا (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(ریښتیا);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(خطأ);
                    }

                    إذا (v3Global.opt.no_Trace_سر() && varp->isIO()) varp->trace(خطأ);

                    AstPin* ثابت pinp = جديد AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        جديد AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->جمع_کړه_Pinsp(pinp);
                } وإلا إذا (v3Global.opt.سر_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ثابت Ast_غوټه_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            إذا (dup_نومونه.پیدا_کړه(name) != dup_نومونه.پای_ټکی()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = جديد AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              تش_اشاره,
                                                              تش_اشاره,
                                                              تش_اشاره};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->جمع_کړه_Stmtsp(ifacecellp);

                            AstIfaceRefDType* ثابت idtypep = جديد AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(تش_اشاره);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->جمع_کړه_Typesp(idtypep);

                            AstVar* varp = جديد AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->is_Iface_پلار(ریښتیا);
                            ifacecellp->جمع_کړه_بل_دلته(varp);
                            ifacecellp->hasIfaceVar(ریښتیا);

                            AstPin* ثابت pinp
                                = جديد AstPin{oldvarp->fileline(), 0, varp->name(),
                                             جديد AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->جمع_کړه_Pinsp(pinp);
                        }
                    }
                }
            }
        }
    }
}

فراغ V3LinkLevel::non_Wrap_سر(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    إذا (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        أرجع;
    }

    // For all top modules
    لكل (Ast_غوټه_Module* modp = rootp->modulesp(); modp && modp->is_سر();
         modp = VN_AS(modp->nextp(), غوټه_Module)) {
        إذا (VN_IS(modp, Package)) استمر;

        UINFO(5, "LOOP " << modp);

        لكل (Ast_غوټه* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                إذا (varp->isIO()) {
                    varp->protect(خطأ);
                    varp->sig_عامه(ریښتیا);
                    varp->primaryIO(ریښتیا);
                    إذا (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    إذا (v3Global.opt.systemC()) varp->sc(ریښتیا);
                    إذا (v3Global.opt.no_Trace_سر()) varp->trace(خطأ);
                }
            }
        }
    }
}
