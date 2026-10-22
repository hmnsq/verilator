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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkLevel.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

संरचना CmpLevel final {
    तार्किक operator()(ध्रुव Ast_ग्रन्थिः_Module* lhsp, ध्रुव Ast_ग्रन्थिः_Module* rhsp) ध्रुव {
        प्रत्यागम lhsp->level() < rhsp->level();
    }
};

शून्य V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    प्रति (Ast_ग्रन्थिः_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ग्रन्थिः_Module)) {
        जइ (nodep->is_शिखरम्() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.प्रक्षिप_back(nodep);
        }
        mods.प्रक्षिप_back(nodep);
    }
    जइ (tops.size() >= 2) {
        ध्रुव Ast_ग्रन्थिः* ध्रुव secp = tops[1];  // Complain about second one, as first often intended
        जइ (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warn_शिखरम्_Modules = [](ध्रुव Ast_ग्रन्थिः* ध्रुव secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                प्रति (Ast_ग्रन्थिः* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                प्रत्यागम ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_शिखरम्_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    प्रति (Ast_ग्रन्थिः_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    प्रति (Ast_ग्रन्थिः_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_वृक्षः("cellsort", मिच्छ, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3LinkLevel::timescaling(ध्रुव ModVec& mods) {
    // Timescale determination
    ध्रुव Ast_ग्रन्थिः_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    प्रति (Ast_ग्रन्थिः_Module* modp : mods) {
        प्रति (Ast_ग्रन्थिः *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            जइ (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                जइ (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_वृक्षः(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    प्रति (Ast_ग्रन्थिः_Module* modp : mods) {
        जइ (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            भङ्ग;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    जइ (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    तार्किक dunitTimed = मिच्छ;  // $unit had a timeunit
    जइ (ध्रुव AstPackage* ध्रुव upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        जइ (!upkgp->timeunit().isNone()) dunitTimed = सच्च;
    }

    प्रति (Ast_ग्रन्थिः_Module* modp : mods) {
        जइ (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        जइ (modp->timeunit().isNone()) {
            जइ (modTimedp  // Got previous
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

    जइ (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

शून्य V3LinkLevel::wrap_शिखरम्(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_ग्रन्थिः_Module* ध्रुव oldmodp = rootp->modulesp();
    जइ (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        प्रत्यागम;
    }
    rootp->resolved_शिखरम्_Module_Name(oldmodp->name());

    Ast_ग्रन्थिः_Module* ध्रुव newmodp = नूतन AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ग्रन्थिः::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(सच्च);
    newmodp->protect(मिच्छ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_शिखरम्_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    प्रति (Ast_ग्रन्थिः_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
        जइ (VN_IS(modp, Package)) {
            AstCell* ध्रुव cellp
                = नूतन AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ग्रन्थिः_Module* ध्रुव modp) {
        जइ (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_वृक्षः("wraptop", 0, dump_वृक्षः_Either_Level() >= 6);
}

शून्य V3LinkLevel::wrap_शिखरम्_Cell(AstNetlist* rootp) {
    Ast_ग्रन्थिः_Module* ध्रुव newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_शिखरम्(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    प्रति (Ast_ग्रन्थिः_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ग्रन्थिः_Module);
         oldmodp && oldmodp->is_शिखरम्(); oldmodp = VN_AS(oldmodp->nextp(), ग्रन्थिः_Module)) {
        प्रति (Ast_ग्रन्थिः* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            जइ (AstVar* ध्रुव oldvarp = VN_CAST(subnodep, Var)) {
                जइ (oldvarp->isIO()) {
                    जइ (!ioNames.निवेशय(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.निवेशय(oldvarp->name());
                    }
                } अण्णहा जइ (v3Global.opt.शिखरम्_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = oldvarp->subDTypep();
                    जइ (VN_IS(subtypep, IfaceRefDType)) {
                        ध्रुव AstIfaceRefDType* ध्रुव ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        जइ (!ifacerefp->cellp()) {
                            जइ (!ioNames.निवेशय(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.निवेशय(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    प्रति (Ast_ग्रन्थिः_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ग्रन्थिः_Module);
         oldmodp && oldmodp->is_शिखरम्(); oldmodp = VN_AS(oldmodp->nextp(), ग्रन्थिः_Module)) {
        जइ (VN_IS(oldmodp, Package)) अनुवर्त;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ध्रुव cellp = नूतन AstCell{
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
        प्रति (Ast_ग्रन्थिः* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            जइ (AstVar* ध्रुव oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                जइ (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    जइ (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ध्रुव varp = oldvarp->clone_वृक्षः(मिच्छ);
                    varp->name(name);
                    varp->protect(मिच्छ);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(सच्च);  // User needs to be able to get to it...
                    oldvarp->primaryIO(मिच्छ);
                    varp->primaryIO(सच्च);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    जइ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    जइ (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(सच्च);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(मिच्छ);
                    }

                    जइ (v3Global.opt.no_Trace_शिखरम्() && varp->isIO()) varp->trace(मिच्छ);

                    AstPin* ध्रुव pinp = नूतन AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        नूतन AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } अण्णहा जइ (v3Global.opt.शिखरम्_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = oldvarp->subDTypep();
                    जइ (VN_IS(subtypep, IfaceRefDType)) {
                        ध्रुव AstIfaceRefDType* ध्रुव ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        जइ (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            जइ (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = नूतन AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* ध्रुव idtypep = नूतन AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = नूतन AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(सच्च);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(सच्च);

                            AstPin* ध्रुव pinp
                                = नूतन AstPin{oldvarp->fileline(), 0, varp->name(),
                                             नूतन AstVarRef{varp->fileline(), varp,
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

शून्य V3LinkLevel::non_Wrap_शिखरम्(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    जइ (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        प्रत्यागम;
    }

    // For all top modules
    प्रति (Ast_ग्रन्थिः_Module* modp = rootp->modulesp(); modp && modp->is_शिखरम्();
         modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
        जइ (VN_IS(modp, Package)) अनुवर्त;

        UINFO(5, "LOOP " << modp);

        प्रति (Ast_ग्रन्थिः* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            जइ (AstVar* ध्रुव varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                जइ (varp->isIO()) {
                    varp->protect(मिच्छ);
                    varp->sigPublic(सच्च);
                    varp->primaryIO(सच्च);
                    जइ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    जइ (v3Global.opt.systemC()) varp->sc(सच्च);
                    जइ (v3Global.opt.no_Trace_शिखरम्()) varp->trace(मिच्छ);
                }
            }
        }
    }
}
