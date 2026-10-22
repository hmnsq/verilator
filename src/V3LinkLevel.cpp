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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkLevel.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

ਬਣਤਰ CmpLevel final {
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ Ast_ਗੰਢ_Module* lhsp, ਅਚਲ Ast_ਗੰਢ_Module* rhsp) ਅਚਲ {
        ਵਾਪਸ lhsp->level() < rhsp->level();
    }
};

ਖਾਲੀ V3LinkLevel::mod_ਛਾਂਟੋ_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    ਲਈ (Ast_ਗੰਢ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ਗੰਢ_Module)) {
        ਜੇ (nodep->is_ਸਿਖਰ() && !VN_IS(nodep, Not_ਮਿਲਿਆ_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.ਧੱਕੋ_ਪਿੱਠ(nodep);
        }
        mods.ਧੱਕੋ_ਪਿੱਠ(nodep);
    }
    ਜੇ (tops.ਮਾਪ() >= 2) {
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ secp = tops[1];  // Complain about second one, as first often intended
        ਜੇ (!secp->fileline()->warnIsOff(V3_Error_ਕੋਡ::MULTITOP)) {
            ਸਵੈਚਾਲਿਤ warn_ਸਿਖਰ_Modules = [](ਅਚਲ Ast_ਗੰਢ* ਅਚਲ secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                ਲਈ (Ast_ਗੰਢ* alsop : tops) {
                    ss << secp->warn_ਹੋਰ() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                ਵਾਪਸ ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_ਹੋਰ()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_ਸਿਖਰ_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_ਛਾਂਟੋ(mods.begin(), mods.ਸਿਰਾ(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    ਲਈ (Ast_ਗੰਢ_Module* nodep : mods) nodep->unlink_Fr_ਪਿੱਠ();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    ਲਈ (Ast_ਗੰਢ_Module* nodep : mods) v3Global.rootp()->ਜੋੜੋ_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_ਰੁੱਖ("cellsort", ਝੂਠ, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3LinkLevel::timescaling(ਅਚਲ ModVec& mods) {
    // Timescale determination
    ਅਚਲ Ast_ਗੰਢ_Module* modTimedp = ਖਾਲੀ_ਸੂਚਕ;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    ਲਈ (Ast_ਗੰਢ_Module* modp : mods) {
        ਲਈ (Ast_ਗੰਢ *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            ਜੇ (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                ਜੇ (pragp->pragType() == VPragmaType::TIMEUNIT_ਸੈੱਟ) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    ਲਈ (Ast_ਗੰਢ_Module* modp : mods) {
        ਜੇ (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            ਰੋਕੋ;
        }
    }
    unit = v3Global.opt.ਸਮਾਂ_Compute_Unit(unit);  // Apply override
    ਜੇ (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    ਤਾਰਕਿਕ dunitTimed = ਝੂਠ;  // $unit had a timeunit
    ਜੇ (ਅਚਲ AstPackage* ਅਚਲ upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        ਜੇ (!upkgp->timeunit().isNone()) dunitTimed = ਸੱਚ;
    }

    ਲਈ (Ast_ਗੰਢ_Module* modp : mods) {
        ਜੇ (!v3Global.opt.ਸਮਾਂ_Override_Unit().isNone()) modp->timeunit(unit);
        ਜੇ (modp->timeunit().isNone()) {
            ਜੇ (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.ਸਮਾਂ_Override_Unit().isNone()
                    && v3Global.opt.ਸਮਾਂ_Default_Unit().isNone())
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

    ਜੇ (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecision_ਮਿਲਾਓ(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

ਖਾਲੀ V3LinkLevel::wrap_ਸਿਖਰ(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_ਗੰਢ_Module* ਅਚਲ oldmodp = rootp->modulesp();
    ਜੇ (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        ਵਾਪਸ;
    }
    rootp->resolved_ਸਿਖਰ_Module_Name(oldmodp->name());

    Ast_ਗੰਢ_Module* ਅਚਲ newmodp = ਨਵਾਂ AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ਗੰਢ::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
    newmodp->ਜੋੜੋ_ਅਗਲਾ(oldmodp);
    newmodp->ਡੂੰਘਾਈ(1);
    newmodp->level(1);
    newmodp->mod_ਜਨਤਕ(ਸੱਚ);
    newmodp->protect(ਝੂਠ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->ਜੋੜੋ_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_ਸਿਖਰ_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    ਲਈ (Ast_ਗੰਢ_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
        ਜੇ (VN_IS(modp, Package)) {
            AstCell* ਅਚਲ cellp
                = ਨਵਾਂ AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            cellp->modp(modp);
            newmodp->ਜੋੜੋ_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ਗੰਢ_Module* ਅਚਲ modp) {
        ਜੇ (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_ਰੁੱਖ("wraptop", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}

ਖਾਲੀ V3LinkLevel::wrap_ਸਿਖਰ_Cell(AstNetlist* rootp) {
    Ast_ਗੰਢ_Module* ਅਚਲ newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_ਸਿਖਰ(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    ਵਰਤੋਂ Name_ਸੈੱਟ = std::unordered_ਸੈੱਟ<std::string>;
    Name_ਸੈੱਟ io_ਨਾਂ;
    Name_ਸੈੱਟ dup_ਨਾਂ;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    ਲਈ (Ast_ਗੰਢ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ਗੰਢ_Module);
         oldmodp && oldmodp->is_ਸਿਖਰ(); oldmodp = VN_AS(oldmodp->nextp(), ਗੰਢ_Module)) {
        ਲਈ (Ast_ਗੰਢ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ਜੇ (AstVar* ਅਚਲ oldvarp = VN_CAST(subnodep, Var)) {
                ਜੇ (oldvarp->isIO()) {
                    ਜੇ (!io_ਨਾਂ.ਪਾਓ(oldvarp->name()).ਦੂਜਾ) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_ਨਾਂ.ਪਾਓ(oldvarp->name());
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.ਸਿਖਰ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ subtypep = oldvarp->subDTypep();
                    ਜੇ (VN_IS(subtypep, IfaceRefDType)) {
                        ਅਚਲ AstIfaceRefDType* ਅਚਲ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ਜੇ (!ifacerefp->cellp()) {
                            ਜੇ (!io_ਨਾਂ.ਪਾਓ(oldvarp->name()).ਦੂਜਾ) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_ਨਾਂ.ਪਾਓ(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    ਲਈ (Ast_ਗੰਢ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ਗੰਢ_Module);
         oldmodp && oldmodp->is_ਸਿਖਰ(); oldmodp = VN_AS(oldmodp->nextp(), ਗੰਢ_Module)) {
        ਜੇ (VN_IS(oldmodp, Package)) ਜਾਰੀ;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ਅਚਲ cellp = ਨਵਾਂ AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().ਖਾਲੀ_ਥਾਵਾਂ() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            ਖਾਲੀ_ਸੂਚਕ,
            ਖਾਲੀ_ਸੂਚਕ,
            ਖਾਲੀ_ਸੂਚਕ};
        cellp->modp(oldmodp);
        newmodp->ਜੋੜੋ_Stmtsp(cellp);

        // Add pins
        ਲਈ (Ast_ਗੰਢ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ਜੇ (AstVar* ਅਚਲ oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                ਜੇ (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    ਜੇ (dup_ਨਾਂ.ਲੱਭੋ(name) != dup_ਨਾਂ.ਸਿਰਾ()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ਅਚਲ varp = oldvarp->clone_ਰੁੱਖ(ਝੂਠ);
                    varp->name(name);
                    varp->protect(ਝੂਠ);
                    newmodp->ਜੋੜੋ_Stmtsp(varp);
                    varp->sig_ਜਨਤਕ(ਸੱਚ);  // User needs to be able to get to it...
                    oldvarp->primaryIO(ਝੂਠ);
                    varp->primaryIO(ਸੱਚ);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    ਜੇ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ਜੇ (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(ਸੱਚ);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(ਝੂਠ);
                    }

                    ਜੇ (v3Global.opt.no_Trace_ਸਿਖਰ() && varp->isIO()) varp->trace(ਝੂਠ);

                    AstPin* ਅਚਲ pinp = ਨਵਾਂ AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        ਨਵਾਂ AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->ਜੋੜੋ_Pinsp(pinp);
                } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.ਸਿਖਰ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ subtypep = oldvarp->subDTypep();
                    ਜੇ (VN_IS(subtypep, IfaceRefDType)) {
                        ਅਚਲ AstIfaceRefDType* ਅਚਲ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ਜੇ (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            ਜੇ (dup_ਨਾਂ.ਲੱਭੋ(name) != dup_ਨਾਂ.ਸਿਰਾ()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = ਨਵਾਂ AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              ਖਾਲੀ_ਸੂਚਕ,
                                                              ਖਾਲੀ_ਸੂਚਕ,
                                                              ਖਾਲੀ_ਸੂਚਕ};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->ਜੋੜੋ_Stmtsp(ifacecellp);

                            AstIfaceRefDType* ਅਚਲ idtypep = ਨਵਾਂ AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(ਖਾਲੀ_ਸੂਚਕ);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->ਜੋੜੋ_Typesp(idtypep);

                            AstVar* varp = ਨਵਾਂ AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->is_Iface_ਮਾਪਾ(ਸੱਚ);
                            ifacecellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(varp);
                            ifacecellp->hasIfaceVar(ਸੱਚ);

                            AstPin* ਅਚਲ pinp
                                = ਨਵਾਂ AstPin{oldvarp->fileline(), 0, varp->name(),
                                             ਨਵਾਂ AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->ਜੋੜੋ_Pinsp(pinp);
                        }
                    }
                }
            }
        }
    }
}

ਖਾਲੀ V3LinkLevel::non_Wrap_ਸਿਖਰ(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    ਜੇ (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        ਵਾਪਸ;
    }

    // For all top modules
    ਲਈ (Ast_ਗੰਢ_Module* modp = rootp->modulesp(); modp && modp->is_ਸਿਖਰ();
         modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
        ਜੇ (VN_IS(modp, Package)) ਜਾਰੀ;

        UINFO(5, "LOOP " << modp);

        ਲਈ (Ast_ਗੰਢ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                ਜੇ (varp->isIO()) {
                    varp->protect(ਝੂਠ);
                    varp->sig_ਜਨਤਕ(ਸੱਚ);
                    varp->primaryIO(ਸੱਚ);
                    ਜੇ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ਜੇ (v3Global.opt.systemC()) varp->sc(ਸੱਚ);
                    ਜੇ (v3Global.opt.no_Trace_ਸਿਖਰ()) varp->trace(ਝੂਠ);
                }
            }
        }
    }
}
