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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkLevel.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

ರಚನೆ CmpLevel final {
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* lhsp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* rhsp) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ lhsp->level() < rhsp->level();
    }
};

ಶೂನ್ಯ V3LinkLevel::mod_ವಿಂಗಡಿಸು_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    ಗಾಗಿ (Ast_ಗಂಟು_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ಗಂಟು_Module)) {
        ಆದರೆ (nodep->is_ತುದಿ() && !VN_IS(nodep, Not_ಸಿಕ್ಕಿತು_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        }
        mods.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
    }
    ಆದರೆ (tops.ಗಾತ್ರ_2() >= 2) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ secp = tops[1];  // Complain about second one, as first often intended
        ಆದರೆ (!secp->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::MULTITOP)) {
            ಸ್ವಯಂಚಾಲಿತ warn_ತುದಿ_Modules = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                ಗಾಗಿ (Ast_ಗಂಟು* alsop : tops) {
                    ss << secp->warn_ಇನ್ನಷ್ಟು() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                ಹಿಂತಿರುಗಿ ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_ಇನ್ನಷ್ಟು()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_ತುದಿ_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_ವಿಂಗಡಿಸು(mods.begin(), mods.ಕೊನೆ(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    ಗಾಗಿ (Ast_ಗಂಟು_Module* nodep : mods) nodep->unlink_Fr_ಹಿಂಭಾಗ();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    ಗಾಗಿ (Ast_ಗಂಟು_Module* nodep : mods) v3Global.rootp()->ಕೂಡಿಸು_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_ವೃಕ್ಷ("cellsort", ಸುಳ್ಳು, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3LinkLevel::timescaling(ಸ್ಥಿರಾಂಕ ModVec& mods) {
    // Timescale determination
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modTimedp = ಶೂನ್ಯಸೂಚಕ;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp : mods) {
        ಗಾಗಿ (Ast_ಗಂಟು *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            ಆದರೆ (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                ಆದರೆ (pragp->pragType() == VPragmaType::TIMEUNIT_ಗಣ) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp : mods) {
        ಆದರೆ (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            ನಿಲ್ಲಿಸು;
        }
    }
    unit = v3Global.opt.ಸಮಯ_Compute_Unit(unit);  // Apply override
    ಆದರೆ (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    ತಾರ್ಕಿಕ dunitTimed = ಸುಳ್ಳು;  // $unit had a timeunit
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackage* ಸ್ಥಿರಾಂಕ upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        ಆದರೆ (!upkgp->timeunit().isNone()) dunitTimed = ನಿಜ;
    }

    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp : mods) {
        ಆದರೆ (!v3Global.opt.ಸಮಯ_Override_Unit().isNone()) modp->timeunit(unit);
        ಆದರೆ (modp->timeunit().isNone()) {
            ಆದರೆ (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.ಸಮಯ_Override_Unit().isNone()
                    && v3Global.opt.ಸಮಯ_Default_Unit().isNone())
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

    ಆದರೆ (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecision_ವಿಲೀನ(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

ಶೂನ್ಯ V3LinkLevel::wrap_ತುದಿ(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ oldmodp = rootp->modulesp();
    ಆದರೆ (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        ಹಿಂತಿರುಗಿ;
    }
    rootp->resolved_ತುದಿ_Module_Name(oldmodp->name());

    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newmodp = ಹೊಸ AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ಗಂಟು::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
    newmodp->ಕೂಡಿಸು_ಮುಂದಿನ(oldmodp);
    newmodp->ಆಳ(1);
    newmodp->level(1);
    newmodp->mod_ಬಹಿರಂಗ(ನಿಜ);
    newmodp->protect(ಸುಳ್ಳು);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->ಕೂಡಿಸು_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_ತುದಿ_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಆದರೆ (VN_IS(modp, Package)) {
            AstCell* ಸ್ಥಿರಾಂಕ cellp
                = ಹೊಸ AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            cellp->modp(modp);
            newmodp->ಕೂಡಿಸು_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp) {
        ಆದರೆ (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_ವೃಕ್ಷ("wraptop", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}

ಶೂನ್ಯ V3LinkLevel::wrap_ತುದಿ_Cell(AstNetlist* rootp) {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_ತುದಿ(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    ಬಳಸಿ Name_ಗಣ = std::unordered_ಗಣ<std::string>;
    Name_ಗಣ io_ಹೆಸರುಗಳು;
    Name_ಗಣ dup_ಹೆಸರುಗಳು;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    ಗಾಗಿ (Ast_ಗಂಟು_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ಗಂಟು_Module);
         oldmodp && oldmodp->is_ತುದಿ(); oldmodp = VN_AS(oldmodp->nextp(), ಗಂಟು_Module)) {
        ಗಾಗಿ (Ast_ಗಂಟು* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ oldvarp = VN_CAST(subnodep, Var)) {
                ಆದರೆ (oldvarp->isIO()) {
                    ಆದರೆ (!io_ಹೆಸರುಗಳು.ಸೇರಿಸು(oldvarp->name()).ಎರಡನೇ) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_ಹೆಸರುಗಳು.ಸೇರಿಸು(oldvarp->name());
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.ತುದಿ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = oldvarp->subDTypep();
                    ಆದರೆ (VN_IS(subtypep, IfaceRefDType)) {
                        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ಆದರೆ (!ifacerefp->cellp()) {
                            ಆದರೆ (!io_ಹೆಸರುಗಳು.ಸೇರಿಸು(oldvarp->name()).ಎರಡನೇ) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_ಹೆಸರುಗಳು.ಸೇರಿಸು(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    ಗಾಗಿ (Ast_ಗಂಟು_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ಗಂಟು_Module);
         oldmodp && oldmodp->is_ತುದಿ(); oldmodp = VN_AS(oldmodp->nextp(), ಗಂಟು_Module)) {
        ಆದರೆ (VN_IS(oldmodp, Package)) ಮುಂದುವರಿ;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ಸ್ಥಿರಾಂಕ cellp = ಹೊಸ AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().ಖಾಲಿಗಳು() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            ಶೂನ್ಯಸೂಚಕ,
            ಶೂನ್ಯಸೂಚಕ,
            ಶೂನ್ಯಸೂಚಕ};
        cellp->modp(oldmodp);
        newmodp->ಕೂಡಿಸು_Stmtsp(cellp);

        // Add pins
        ಗಾಗಿ (Ast_ಗಂಟು* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                ಆದರೆ (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    ಆದರೆ (dup_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(name) != dup_ಹೆಸರುಗಳು.ಕೊನೆ()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ಸ್ಥಿರಾಂಕ varp = oldvarp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    varp->name(name);
                    varp->protect(ಸುಳ್ಳು);
                    newmodp->ಕೂಡಿಸು_Stmtsp(varp);
                    varp->sig_ಬಹಿರಂಗ(ನಿಜ);  // User needs to be able to get to it...
                    oldvarp->primaryIO(ಸುಳ್ಳು);
                    varp->primaryIO(ನಿಜ);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    ಆದರೆ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ಆದರೆ (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(ನಿಜ);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(ಸುಳ್ಳು);
                    }

                    ಆದರೆ (v3Global.opt.no_Trace_ತುದಿ() && varp->isIO()) varp->trace(ಸುಳ್ಳು);

                    AstPin* ಸ್ಥಿರಾಂಕ pinp = ಹೊಸ AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        ಹೊಸ AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->ಕೂಡಿಸು_Pinsp(pinp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.ತುದಿ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = oldvarp->subDTypep();
                    ಆದರೆ (VN_IS(subtypep, IfaceRefDType)) {
                        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ಆದರೆ (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            ಆದರೆ (dup_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(name) != dup_ಹೆಸರುಗಳು.ಕೊನೆ()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = ಹೊಸ AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              ಶೂನ್ಯಸೂಚಕ,
                                                              ಶೂನ್ಯಸೂಚಕ,
                                                              ಶೂನ್ಯಸೂಚಕ};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->ಕೂಡಿಸು_Stmtsp(ifacecellp);

                            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ idtypep = ಹೊಸ AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(ಶೂನ್ಯಸೂಚಕ);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->ಕೂಡಿಸು_Typesp(idtypep);

                            AstVar* varp = ಹೊಸ AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->is_Iface_ಪೋಷಕ(ನಿಜ);
                            ifacecellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(varp);
                            ifacecellp->hasIfaceVar(ನಿಜ);

                            AstPin* ಸ್ಥಿರಾಂಕ pinp
                                = ಹೊಸ AstPin{oldvarp->fileline(), 0, varp->name(),
                                             ಹೊಸ AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->ಕೂಡಿಸು_Pinsp(pinp);
                        }
                    }
                }
            }
        }
    }
}

ಶೂನ್ಯ V3LinkLevel::non_Wrap_ತುದಿ(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    ಆದರೆ (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        ಹಿಂತಿರುಗಿ;
    }

    // For all top modules
    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = rootp->modulesp(); modp && modp->is_ತುದಿ();
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಆದರೆ (VN_IS(modp, Package)) ಮುಂದುವರಿ;

        UINFO(5, "LOOP " << modp);

        ಗಾಗಿ (Ast_ಗಂಟು* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                ಆದರೆ (varp->isIO()) {
                    varp->protect(ಸುಳ್ಳು);
                    varp->sig_ಬಹಿರಂಗ(ನಿಜ);
                    varp->primaryIO(ನಿಜ);
                    ಆದರೆ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ಆದರೆ (v3Global.opt.systemC()) varp->sc(ನಿಜ);
                    ಆದರೆ (v3Global.opt.no_Trace_ತುದಿ()) varp->trace(ಸುಳ್ಳು);
                }
            }
        }
    }
}
