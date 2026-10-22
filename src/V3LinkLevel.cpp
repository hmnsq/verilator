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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkLevel.h"

#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

ସଂରଚନା CmpLevel final {
    ତାର୍କିକ operator()(ଅଚଳ Ast_ଗଣ୍ଠି_Module* lhsp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* rhsp) ଅଚଳ {
        ଫେରନ୍ତୁ lhsp->level() < rhsp->level();
    }
};

ଶୂନ୍ୟ V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Module)) {
        ଯଦି (nodep->is_ଶୀର୍ଷ() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.ଠେଲ_back(nodep);
        }
        mods.ଠେଲ_back(nodep);
    }
    ଯଦି (tops.size() >= 2) {
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ secp = tops[1];  // Complain about second one, as first often intended
        ଯଦି (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warn_ଶୀର୍ଷ_Modules = [](ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                ପାଇଁ (Ast_ଗଣ୍ଠି* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                ଫେରନ୍ତୁ ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_ଶୀର୍ଷ_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_ବୃକ୍ଷ("cellsort", ମିଥ୍ୟା, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3LinkLevel::timescaling(ଅଚଳ ModVec& mods) {
    // Timescale determination
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp : mods) {
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            ଯଦି (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                ଯଦି (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_ବୃକ୍ଷ(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp : mods) {
        ଯଦି (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            ଭାଙ୍ଗ;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    ଯଦି (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    ତାର୍କିକ dunitTimed = ମିଥ୍ୟା;  // $unit had a timeunit
    ଯଦି (ଅଚଳ AstPackage* ଅଚଳ upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        ଯଦି (!upkgp->timeunit().isNone()) dunitTimed = ସତ୍ୟ;
    }

    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp : mods) {
        ଯଦି (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        ଯଦି (modp->timeunit().isNone()) {
            ଯଦି (modTimedp  // Got previous
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

    ଯଦି (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

ଶୂନ୍ୟ V3LinkLevel::wrap_ଶୀର୍ଷ(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ oldmodp = rootp->modulesp();
    ଯଦି (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        ଫେରନ୍ତୁ;
    }
    rootp->resolved_ଶୀର୍ଷ_Module_Name(oldmodp->name());

    Ast_ଗଣ୍ଠି_Module* ଅଚଳ newmodp = ନୂତନ AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ଗଣ୍ଠି::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(ସତ୍ୟ);
    newmodp->protect(ମିଥ୍ୟା);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_ଶୀର୍ଷ_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
        ଯଦି (VN_IS(modp, Package)) {
            AstCell* ଅଚଳ cellp
                = ନୂତନ AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp) {
        ଯଦି (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_ବୃକ୍ଷ("wraptop", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}

ଶୂନ୍ୟ V3LinkLevel::wrap_ଶୀର୍ଷ_Cell(AstNetlist* rootp) {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_ଶୀର୍ଷ(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ଗଣ୍ଠି_Module);
         oldmodp && oldmodp->is_ଶୀର୍ଷ(); oldmodp = VN_AS(oldmodp->nextp(), ଗଣ୍ଠି_Module)) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ଯଦି (AstVar* ଅଚଳ oldvarp = VN_CAST(subnodep, Var)) {
                ଯଦି (oldvarp->isIO()) {
                    ଯଦି (!ioNames.ଭର୍ତ୍ତି(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.ଭର୍ତ୍ତି(oldvarp->name());
                    }
                } ଅନ୍ୟଥା ଯଦି (v3Global.opt.ଶୀର୍ଷ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = oldvarp->subDTypep();
                    ଯଦି (VN_IS(subtypep, IfaceRefDType)) {
                        ଅଚଳ AstIfaceRefDType* ଅଚଳ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ଯଦି (!ifacerefp->cellp()) {
                            ଯଦି (!ioNames.ଭର୍ତ୍ତି(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.ଭର୍ତ୍ତି(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ଗଣ୍ଠି_Module);
         oldmodp && oldmodp->is_ଶୀର୍ଷ(); oldmodp = VN_AS(oldmodp->nextp(), ଗଣ୍ଠି_Module)) {
        ଯଦି (VN_IS(oldmodp, Package)) ଜାରି;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ଅଚଳ cellp = ନୂତନ AstCell{
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
        ପାଇଁ (Ast_ଗଣ୍ଠି* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ଯଦି (AstVar* ଅଚଳ oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                ଯଦି (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    ଯଦି (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ଅଚଳ varp = oldvarp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    varp->name(name);
                    varp->protect(ମିଥ୍ୟା);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(ସତ୍ୟ);  // User needs to be able to get to it...
                    oldvarp->primaryIO(ମିଥ୍ୟା);
                    varp->primaryIO(ସତ୍ୟ);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    ଯଦି (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ଯଦି (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(ସତ୍ୟ);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(ମିଥ୍ୟା);
                    }

                    ଯଦି (v3Global.opt.no_Trace_ଶୀର୍ଷ() && varp->isIO()) varp->trace(ମିଥ୍ୟା);

                    AstPin* ଅଚଳ pinp = ନୂତନ AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        ନୂତନ AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } ଅନ୍ୟଥା ଯଦି (v3Global.opt.ଶୀର୍ଷ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = oldvarp->subDTypep();
                    ଯଦି (VN_IS(subtypep, IfaceRefDType)) {
                        ଅଚଳ AstIfaceRefDType* ଅଚଳ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        ଯଦି (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            ଯଦି (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = ନୂତନ AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* ଅଚଳ idtypep = ନୂତନ AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = ନୂତନ AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(ସତ୍ୟ);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(ସତ୍ୟ);

                            AstPin* ଅଚଳ pinp
                                = ନୂତନ AstPin{oldvarp->fileline(), 0, varp->name(),
                                             ନୂତନ AstVarRef{varp->fileline(), varp,
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

ଶୂନ୍ୟ V3LinkLevel::non_Wrap_ଶୀର୍ଷ(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    ଯଦି (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        ଫେରନ୍ତୁ;
    }

    // For all top modules
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = rootp->modulesp(); modp && modp->is_ଶୀର୍ଷ();
         modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
        ଯଦି (VN_IS(modp, Package)) ଜାରି;

        UINFO(5, "LOOP " << modp);

        ପାଇଁ (Ast_ଗଣ୍ଠି* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                ଯଦି (varp->isIO()) {
                    varp->protect(ମିଥ୍ୟା);
                    varp->sigPublic(ସତ୍ୟ);
                    varp->primaryIO(ସତ୍ୟ);
                    ଯଦି (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    ଯଦି (v3Global.opt.systemC()) varp->sc(ସତ୍ୟ);
                    ଯଦି (v3Global.opt.no_Trace_ଶୀର୍ଷ()) varp->trace(ମିଥ୍ୟା);
                }
            }
        }
    }
}
