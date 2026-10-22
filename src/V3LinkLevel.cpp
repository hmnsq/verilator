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

ಸಂರಚನಾ CmpLevel final {
    ತಾರ್ಕಿಕ operator()(ಧ್ರುವ Ast_ಗಂಟು_Module* lhsp, ಧ್ರುವ Ast_ಗಂಟು_Module* rhsp) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ lhsp->level() < rhsp->level();
    }
};

ಶೂನ್ಯ V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ಗಂಟು_Module)) {
        ಯದಿ (nodep->is_ತುದಿ() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.ತಳ್ಳು_back(nodep);
        }
        mods.ತಳ್ಳು_back(nodep);
    }
    ಯದಿ (tops.size() >= 2) {
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ secp = tops[1];  // Complain about second one, as first often intended
        ಯದಿ (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warn_ತುದಿ_Modules = [](ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                ಪ್ರತಿ (Ast_ಗಂಟು* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                ಪ್ರತ್ಯಾಗಮ ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_ತುದಿ_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_ವೃಕ್ಷ("cellsort", ಅಸತ್ಯ, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3LinkLevel::timescaling(ಧ್ರುವ ModVec& mods) {
    // Timescale determination
    ಧ್ರುವ Ast_ಗಂಟು_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp : mods) {
        ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            ಯದಿ (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                ಯದಿ (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_ವೃಕ್ಷ(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp : mods) {
        ಯದಿ (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            ಭಙ್ಗ;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    ಯದಿ (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    ತಾರ್ಕಿಕ dunitTimed = ಅಸತ್ಯ;  // $unit had a timeunit
    ಯದಿ (ಧ್ರುವ AstPackage* ಧ್ರುವ upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        ಯದಿ (!upkgp->timeunit().isNone()) dunitTimed = ಸತ್ಯ;
    }

    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp : mods) {
        ಯದಿ (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        ಯದಿ (modp->timeunit().isNone()) {
            ಯದಿ (modTimedp  // Got previous
                && !dunitTimed
                && (  // unit doesn't already include an override
                    v3Global.opt.timeOverrideUnit().isNone()
                    && v3Global.opt.timeDefaultUnit().isNone())
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

    ಯದಿ (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

ಶೂನ್ಯ V3LinkLevel::wrap_ತುದಿ(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_ಗಂಟು_Module* ಧ್ರುವ oldmodp = rootp->modulesp();
    ಯದಿ (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        ಪ್ರತ್ಯಾಗಮ;
    }
    rootp->resolved_ತುದಿ_Module_Name(oldmodp->name());

    Ast_ಗಂಟು_Module* ಧ್ರುವ newmodp = ನೂತನ AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ಗಂಟು::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(ಸತ್ಯ);
    newmodp->protect(ಅಸತ್ಯ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_ತುದಿ_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಯದಿ (VN_IS(modp, Package)) {
            AstCell* ಧ್ರುವ cellp
                = ನೂತನ AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ಗಂಟು_Module* ಧ್ರುವ modp) {
        ಯದಿ (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_ವೃಕ್ಷ("wraptop", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}

ಶೂನ್ಯ V3LinkLevel::wrap_ತುದಿ_Cell(AstNetlist* rootp) {
    Ast_ಗಂಟು_Module* ಧ್ರುವ newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_ತುದಿ(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ಗಂಟು_Module);
         oldmodp && oldmodp->is_ತುದಿ(); oldmodp = VN_AS(oldmodp->nextp(), ಗಂಟು_Module)) {
        ಪ್ರತಿ (Ast_ಗಂಟು* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ oldvarp = VN_CAST(subnodep, Var)) {
                ಯದಿ (oldvarp->isIO()) {
                    ಯದಿ (!ioNames.ಸೇರಿಸು(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.ಸೇರಿಸು(oldvarp->name());
                    }
                } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.ತುದಿ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = oldvarp->subDTypep();
                    ಯದಿ (VN_IS(subtypep, IfaceRefDType)) {
                        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ಯದಿ (!ifacerefp->cellp()) {
                            ಯದಿ (!ioNames.ಸೇರಿಸು(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.ಸೇರಿಸು(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ಗಂಟು_Module);
         oldmodp && oldmodp->is_ತುದಿ(); oldmodp = VN_AS(oldmodp->nextp(), ಗಂಟು_Module)) {
        ಯದಿ (VN_IS(oldmodp, Package)) ಅನುವರ್ತ;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ಧ್ರುವ cellp = ನೂತನ AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().empty() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            nullptr,
            nullptr,
            nullptr};
        cellp->modp(oldmodp);
        newmodp->addStmtsp(cellp);

        // Add pins
        ಪ್ರತಿ (Ast_ಗಂಟು* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                ಯದಿ (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    ಯದಿ (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ಧ್ರುವ varp = oldvarp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    varp->name(name);
                    varp->protect(ಅಸತ್ಯ);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(ಸತ್ಯ);  // User needs to be able to get to it...
                    oldvarp->primaryIO(ಅಸತ್ಯ);
                    varp->primaryIO(ಸತ್ಯ);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    ಯದಿ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ಯದಿ (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(ಸತ್ಯ);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(ಅಸತ್ಯ);
                    }

                    ಯದಿ (v3Global.opt.no_Trace_ತುದಿ() && varp->isIO()) varp->trace(ಅಸತ್ಯ);

                    AstPin* ಧ್ರುವ pinp = ನೂತನ AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        ನೂತನ AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.ತುದಿ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = oldvarp->subDTypep();
                    ಯದಿ (VN_IS(subtypep, IfaceRefDType)) {
                        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ಯದಿ (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            ಯದಿ (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = ನೂತನ AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* ಧ್ರುವ idtypep = ನೂತನ AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = ನೂತನ AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(ಸತ್ಯ);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(ಸತ್ಯ);

                            AstPin* ಧ್ರುವ pinp
                                = ನೂತನ AstPin{oldvarp->fileline(), 0, varp->name(),
                                             ನೂತನ AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->addPinsp(pinp);
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
    ಯದಿ (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        ಪ್ರತ್ಯಾಗಮ;
    }

    // For all top modules
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = rootp->modulesp(); modp && modp->is_ತುದಿ();
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಯದಿ (VN_IS(modp, Package)) ಅನುವರ್ತ;

        UINFO(5, "LOOP " << modp);

        ಪ್ರತಿ (Ast_ಗಂಟು* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                ಯದಿ (varp->isIO()) {
                    varp->protect(ಅಸತ್ಯ);
                    varp->sigPublic(ಸತ್ಯ);
                    varp->primaryIO(ಸತ್ಯ);
                    ಯದಿ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ಯದಿ (v3Global.opt.systemC()) varp->sc(ಸತ್ಯ);
                    ಯದಿ (v3Global.opt.no_Trace_ತುದಿ()) varp->trace(ಅಸತ್ಯ);
                }
            }
        }
    }
}
