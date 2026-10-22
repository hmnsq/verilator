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
    منطقي عامل()(ثابت Ast_گرہ_Module* lhsp, ثابت Ast_گرہ_Module* rhsp) ثابت {
        أرجع lhsp->level() < rhsp->level();
    }
};

فراغ V3LinkLevel::mod_ترتیب_دیو_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    لكل (Ast_گرہ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), گرہ_Module)) {
        إذا (nodep->is_چوٹی() && !VN_IS(nodep, Not_لبھیا_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.ڈالو_پٹھ(nodep);
        }
        mods.ڈالو_پٹھ(nodep);
    }
    إذا (tops.ناپ() >= 2) {
        ثابت Ast_گرہ* ثابت secp = tops[1];  // Complain about second one, as first often intended
        إذا (!secp->fileline()->warnIsOff(V3_Error_کوڈ::MULTITOP)) {
            خودکار warn_چوٹی_Modules = [](ثابت Ast_گرہ* ثابت secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                لكل (Ast_گرہ* alsop : tops) {
                    ss << secp->warn_ہور() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                أرجع ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_ہور()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_چوٹی_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_ترتیب_دیو(mods.begin(), mods.سرا(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    لكل (Ast_گرہ_Module* nodep : mods) nodep->unlink_Fr_پٹھ();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    لكل (Ast_گرہ_Module* nodep : mods) v3Global.rootp()->جوڑو_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_درخت("cellsort", خطأ, dump_درخت_Either_Level() >= 3);
}

فراغ V3LinkLevel::timescaling(ثابت ModVec& mods) {
    // Timescale determination
    ثابت Ast_گرہ_Module* modTimedp = خالی_سوچک;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    لكل (Ast_گرہ_Module* modp : mods) {
        لكل (Ast_گرہ *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            إذا (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                إذا (pragp->pragType() == VPragmaType::TIMEUNIT_سیٹ) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_پٹھ()->delete_درخت(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    لكل (Ast_گرہ_Module* modp : mods) {
        إذا (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            اكسر;
        }
    }
    unit = v3Global.opt.ویلا_Compute_Unit(unit);  // Apply override
    إذا (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    منطقي dunitTimed = خطأ;  // $unit had a timeunit
    إذا (ثابت AstPackage* ثابت upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        إذا (!upkgp->timeunit().isNone()) dunitTimed = سچ;
    }

    لكل (Ast_گرہ_Module* modp : mods) {
        إذا (!v3Global.opt.ویلا_Override_Unit().isNone()) modp->timeunit(unit);
        إذا (modp->timeunit().isNone()) {
            إذا (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.ویلا_Override_Unit().isNone()
                    && v3Global.opt.ویلا_Default_Unit().isNone())
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
        v3Global.rootp()->timeprecision_ملاؤ(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

فراغ V3LinkLevel::wrap_چوٹی(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_گرہ_Module* ثابت oldmodp = rootp->modulesp();
    إذا (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        أرجع;
    }
    rootp->resolved_چوٹی_Module_Name(oldmodp->name());

    Ast_گرہ_Module* ثابت newmodp = جديد AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_گرہ::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_پٹھ_With_اگلا();
    newmodp->جوڑو_اگلا(oldmodp);
    newmodp->ڈونگھائی(1);
    newmodp->level(1);
    newmodp->mod_عوامی(سچ);
    newmodp->protect(خطأ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->جوڑو_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_چوٹی_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    لكل (Ast_گرہ_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), گرہ_Module)) {
        إذا (VN_IS(modp, Package)) {
            AstCell* ثابت cellp
                = جديد AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), خالی_سوچک, خالی_سوچک, خالی_سوچک};
            cellp->modp(modp);
            newmodp->جوڑو_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_گرہ_Module* ثابت modp) {
        إذا (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_درخت("wraptop", 0, dump_درخت_Either_Level() >= 6);
}

فراغ V3LinkLevel::wrap_چوٹی_Cell(AstNetlist* rootp) {
    Ast_گرہ_Module* ثابت newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_چوٹی(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    ورتوں Name_سیٹ = std::unordered_سیٹ<std::string>;
    Name_سیٹ io_ناں;
    Name_سیٹ dup_ناں;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    لكل (Ast_گرہ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), گرہ_Module);
         oldmodp && oldmodp->is_چوٹی(); oldmodp = VN_AS(oldmodp->nextp(), گرہ_Module)) {
        لكل (Ast_گرہ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                إذا (oldvarp->isIO()) {
                    إذا (!io_ناں.داخل_کرو(oldvarp->name()).دوجا) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_ناں.داخل_کرو(oldvarp->name());
                    }
                } وإلا إذا (v3Global.opt.چوٹی_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ثابت Ast_گرہ_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            إذا (!io_ناں.داخل_کرو(oldvarp->name()).دوجا) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_ناں.داخل_کرو(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    لكل (Ast_گرہ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), گرہ_Module);
         oldmodp && oldmodp->is_چوٹی(); oldmodp = VN_AS(oldmodp->nextp(), گرہ_Module)) {
        إذا (VN_IS(oldmodp, Package)) استمر;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ثابت cellp = جديد AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().خالی_تھاواں() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            خالی_سوچک,
            خالی_سوچک,
            خالی_سوچک};
        cellp->modp(oldmodp);
        newmodp->جوڑو_Stmtsp(cellp);

        // Add pins
        لكل (Ast_گرہ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                إذا (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    إذا (dup_ناں.لبھو(name) != dup_ناں.سرا()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ثابت varp = oldvarp->clone_درخت(خطأ);
                    varp->name(name);
                    varp->protect(خطأ);
                    newmodp->جوڑو_Stmtsp(varp);
                    varp->sig_عوامی(سچ);  // User needs to be able to get to it...
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

                    إذا (v3Global.opt.no_Trace_چوٹی() && varp->isIO()) varp->trace(خطأ);

                    AstPin* ثابت pinp = جديد AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        جديد AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->جوڑو_Pinsp(pinp);
                } وإلا إذا (v3Global.opt.چوٹی_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ثابت Ast_گرہ_DType* ثابت subtypep = oldvarp->subDTypep();
                    إذا (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        إذا (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            إذا (dup_ناں.لبھو(name) != dup_ناں.سرا()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = جديد AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              خالی_سوچک,
                                                              خالی_سوچک,
                                                              خالی_سوچک};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->جوڑو_Stmtsp(ifacecellp);

                            AstIfaceRefDType* ثابت idtypep = جديد AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(خالی_سوچک);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->جوڑو_Typesp(idtypep);

                            AstVar* varp = جديد AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->is_Iface_ماپا(سچ);
                            ifacecellp->جوڑو_اگلا_ایتھے(varp);
                            ifacecellp->hasIfaceVar(سچ);

                            AstPin* ثابت pinp
                                = جديد AstPin{oldvarp->fileline(), 0, varp->name(),
                                             جديد AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->جوڑو_Pinsp(pinp);
                        }
                    }
                }
            }
        }
    }
}

فراغ V3LinkLevel::non_Wrap_چوٹی(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    إذا (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        أرجع;
    }

    // For all top modules
    لكل (Ast_گرہ_Module* modp = rootp->modulesp(); modp && modp->is_چوٹی();
         modp = VN_AS(modp->nextp(), گرہ_Module)) {
        إذا (VN_IS(modp, Package)) استمر;

        UINFO(5, "LOOP " << modp);

        لكل (Ast_گرہ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                إذا (varp->isIO()) {
                    varp->protect(خطأ);
                    varp->sig_عوامی(سچ);
                    varp->primaryIO(سچ);
                    إذا (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    إذا (v3Global.opt.systemC()) varp->sc(سچ);
                    إذا (v3Global.opt.no_Trace_چوٹی()) varp->trace(خطأ);
                }
            }
        }
    }
}
