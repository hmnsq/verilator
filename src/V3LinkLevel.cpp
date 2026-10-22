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
    منطقي عملگر()(ثابت Ast_گره_Module* lhsp, ثابت Ast_گره_Module* rhsp) ثابت {
        أرجع lhsp->level() < rhsp->level();
    }
};

فراغ V3LinkLevel::mod_مرتب_سازی_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    لكل (Ast_گره_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), گره_Module)) {
        إذا (nodep->is_بالاترین() && !VN_IS(nodep, Not_یافت_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.افزودن_پشت(nodep);
        }
        mods.افزودن_پشت(nodep);
    }
    إذا (tops.اندازه() >= 2) {
        ثابت Ast_گره* ثابت secp = tops[1];  // Complain about second one, as first often intended
        إذا (!secp->fileline()->warnIsOff(V3_Error_کد::MULTITOP)) {
            خودکار warn_بالاترین_Modules = [](ثابت Ast_گره* ثابت secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                لكل (Ast_گره* alsop : tops) {
                    ss << secp->warn_بیشتر() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                أرجع ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_بیشتر()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_بالاترین_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_مرتب_سازی(mods.begin(), mods.انتها(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    لكل (Ast_گره_Module* nodep : mods) nodep->unlink_Fr_پشت();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    لكل (Ast_گره_Module* nodep : mods) v3Global.rootp()->جمع_کن_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_درخت("cellsort", خطأ, dump_درخت_Either_Level() >= 3);
}

فراغ V3LinkLevel::timescaling(ثابت ModVec& mods) {
    // Timescale determination
    ثابت Ast_گره_Module* modTimedp = اشاره_تهی;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    لكل (Ast_گره_Module* modp : mods) {
        لكل (Ast_گره *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            إذا (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                إذا (pragp->pragType() == VPragmaType::TIMEUNIT_مجموعه) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_پشت()->delete_درخت(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    لكل (Ast_گره_Module* modp : mods) {
        إذا (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            اكسر;
        }
    }
    unit = v3Global.opt.زمان_Compute_Unit(unit);  // Apply override
    إذا (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    منطقي dunitTimed = خطأ;  // $unit had a timeunit
    إذا (ثابت AstPackage* ثابت upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        إذا (!upkgp->timeunit().isNone()) dunitTimed = درست;
    }

    لكل (Ast_گره_Module* modp : mods) {
        إذا (!v3Global.opt.زمان_Override_Unit().isNone()) modp->timeunit(unit);
        إذا (modp->timeunit().isNone()) {
            إذا (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.زمان_Override_Unit().isNone()
                    && v3Global.opt.زمان_Default_Unit().isNone())
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
        v3Global.rootp()->timeprecision_ادغام(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

فراغ V3LinkLevel::wrap_بالاترین(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_گره_Module* ثابت oldmodp = rootp->modulesp();
    إذا (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        أرجع;
    }
    rootp->resolved_بالاترین_Module_Name(oldmodp->name());

    Ast_گره_Module* ثابت newmodp = جديد AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_گره::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_پشت_With_بعدی();
    newmodp->جمع_کن_بعدی(oldmodp);
    newmodp->عمق(1);
    newmodp->level(1);
    newmodp->mod_عمومی(درست);
    newmodp->protect(خطأ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->جمع_کن_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_بالاترین_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    لكل (Ast_گره_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), گره_Module)) {
        إذا (VN_IS(modp, Package)) {
            AstCell* ثابت cellp
                = جديد AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), اشاره_تهی, اشاره_تهی, اشاره_تهی};
            cellp->modp(modp);
            newmodp->جمع_کن_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_گره_Module* ثابت modp) {
        إذا (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_درخت("wraptop", 0, dump_درخت_Either_Level() >= 6);
}

فراغ V3LinkLevel::wrap_بالاترین_Cell(AstNetlist* rootp) {
    Ast_گره_Module* ثابت newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_بالاترین(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    استفاده Name_مجموعه = std::unordered_مجموعه<std::string>;
    Name_مجموعه io_نامها;
    Name_مجموعه dup_نامها;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    لكل (Ast_گره_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), گره_Module);
         oldmodp && oldmodp->is_بالاترین(); oldmodp = VN_AS(oldmodp->nextp(), گره_Module)) {
        لكل (Ast_گره* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                إذا (oldvarp->isIO()) {
                    إذا (!io_نامها.درج(oldvarp->name()).دومی) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_نامها.درج(oldvarp->name());
                    }
                } وإلا إذا (v3Global.opt.بالاترین_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ثابت Ast_گره_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            إذا (!io_نامها.درج(oldvarp->name()).دومی) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_نامها.درج(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    لكل (Ast_گره_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), گره_Module);
         oldmodp && oldmodp->is_بالاترین(); oldmodp = VN_AS(oldmodp->nextp(), گره_Module)) {
        إذا (VN_IS(oldmodp, Package)) استمر;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ثابت cellp = جديد AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().خالی() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            اشاره_تهی,
            اشاره_تهی,
            اشاره_تهی};
        cellp->modp(oldmodp);
        newmodp->جمع_کن_Stmtsp(cellp);

        // Add pins
        لكل (Ast_گره* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                إذا (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    إذا (dup_نامها.پیدا_کن(name) != dup_نامها.انتها()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ثابت varp = oldvarp->clone_درخت(خطأ);
                    varp->name(name);
                    varp->protect(خطأ);
                    newmodp->جمع_کن_Stmtsp(varp);
                    varp->sig_عمومی(درست);  // User needs to be able to get to it...
                    oldvarp->primaryIO(خطأ);
                    varp->primaryIO(درست);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    إذا (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    إذا (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(درست);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(خطأ);
                    }

                    إذا (v3Global.opt.no_Trace_بالاترین() && varp->isIO()) varp->trace(خطأ);

                    AstPin* ثابت pinp = جديد AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        جديد AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->جمع_کن_Pinsp(pinp);
                } وإلا إذا (v3Global.opt.بالاترین_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ثابت Ast_گره_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            إذا (dup_نامها.پیدا_کن(name) != dup_نامها.انتها()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = جديد AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              اشاره_تهی,
                                                              اشاره_تهی,
                                                              اشاره_تهی};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->جمع_کن_Stmtsp(ifacecellp);

                            AstIfaceRefDType* ثابت idtypep = جديد AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(اشاره_تهی);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->جمع_کن_Typesp(idtypep);

                            AstVar* varp = جديد AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->is_Iface_والد(درست);
                            ifacecellp->جمع_کن_بعدی_اینجا(varp);
                            ifacecellp->hasIfaceVar(درست);

                            AstPin* ثابت pinp
                                = جديد AstPin{oldvarp->fileline(), 0, varp->name(),
                                             جديد AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->جمع_کن_Pinsp(pinp);
                        }
                    }
                }
            }
        }
    }
}

فراغ V3LinkLevel::non_Wrap_بالاترین(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    إذا (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        أرجع;
    }

    // For all top modules
    لكل (Ast_گره_Module* modp = rootp->modulesp(); modp && modp->is_بالاترین();
         modp = VN_AS(modp->nextp(), گره_Module)) {
        إذا (VN_IS(modp, Package)) استمر;

        UINFO(5, "LOOP " << modp);

        لكل (Ast_گره* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                إذا (varp->isIO()) {
                    varp->protect(خطأ);
                    varp->sig_عمومی(درست);
                    varp->primaryIO(درست);
                    إذا (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    إذا (v3Global.opt.systemC()) varp->sc(درست);
                    إذا (v3Global.opt.no_Trace_بالاترین()) varp->trace(خطأ);
                }
            }
        }
    }
}
