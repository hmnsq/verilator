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
    तार्किक चालक()(स्थिरांक Ast_गाठ_Module* lhsp, स्थिरांक Ast_गाठ_Module* rhsp) स्थिरांक {
        परत lhsp->level() < rhsp->level();
    }
};

शून्य V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    साठी (Ast_गाठ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), गाठ_Module)) {
        जर (nodep->is_शीर्ष() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.ढकला_back(nodep);
        }
        mods.ढकला_back(nodep);
    }
    जर (tops.size() >= 2) {
        स्थिरांक Ast_गाठ* स्थिरांक secp = tops[1];  // Complain about second one, as first often intended
        जर (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            स्वतः warn_शीर्ष_Modules = [](स्थिरांक Ast_गाठ* स्थिरांक secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                साठी (Ast_गाठ* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                परत ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_शीर्ष_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    साठी (Ast_गाठ_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    साठी (Ast_गाठ_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_वृक्ष("cellsort", असत्य, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3LinkLevel::timescaling(स्थिरांक ModVec& mods) {
    // Timescale determination
    स्थिरांक Ast_गाठ_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    साठी (Ast_गाठ_Module* modp : mods) {
        साठी (Ast_गाठ *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            जर (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                जर (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_वृक्ष(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    साठी (Ast_गाठ_Module* modp : mods) {
        जर (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            थांबा;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    जर (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    तार्किक dunitTimed = असत्य;  // $unit had a timeunit
    जर (स्थिरांक AstPackage* स्थिरांक upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        जर (!upkgp->timeunit().isNone()) dunitTimed = सत्य;
    }

    साठी (Ast_गाठ_Module* modp : mods) {
        जर (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        जर (modp->timeunit().isNone()) {
            जर (modTimedp  // Got previous
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

    जर (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

शून्य V3LinkLevel::wrap_शीर्ष(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_गाठ_Module* स्थिरांक oldmodp = rootp->modulesp();
    जर (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        परत;
    }
    rootp->resolved_शीर्ष_Module_Name(oldmodp->name());

    Ast_गाठ_Module* स्थिरांक newmodp = नवीन AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_गाठ::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(सत्य);
    newmodp->protect(असत्य);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_शीर्ष_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    साठी (Ast_गाठ_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), गाठ_Module)) {
        जर (VN_IS(modp, Package)) {
            AstCell* स्थिरांक cellp
                = नवीन AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_गाठ_Module* स्थिरांक modp) {
        जर (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_वृक्ष("wraptop", 0, dump_वृक्ष_Either_Level() >= 6);
}

शून्य V3LinkLevel::wrap_शीर्ष_Cell(AstNetlist* rootp) {
    Ast_गाठ_Module* स्थिरांक newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_शीर्ष(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    साठी (Ast_गाठ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), गाठ_Module);
         oldmodp && oldmodp->is_शीर्ष(); oldmodp = VN_AS(oldmodp->nextp(), गाठ_Module)) {
        साठी (Ast_गाठ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            जर (AstVar* स्थिरांक oldvarp = VN_CAST(subnodep, Var)) {
                जर (oldvarp->isIO()) {
                    जर (!ioNames.घाला(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.घाला(oldvarp->name());
                    }
                } अन्यथा जर (v3Global.opt.शीर्ष_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    स्थिरांक Ast_गाठ_DType* स्थिरांक subtypep = oldvarp->subDTypep();
                    जर (VN_IS(subtypep, IfaceRefDType)) {
                        स्थिरांक AstIfaceRefDType* स्थिरांक ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        जर (!ifacerefp->cellp()) {
                            जर (!ioNames.घाला(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.घाला(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    साठी (Ast_गाठ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), गाठ_Module);
         oldmodp && oldmodp->is_शीर्ष(); oldmodp = VN_AS(oldmodp->nextp(), गाठ_Module)) {
        जर (VN_IS(oldmodp, Package)) सुरूठेवा;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* स्थिरांक cellp = नवीन AstCell{
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
        साठी (Ast_गाठ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            जर (AstVar* स्थिरांक oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                जर (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    जर (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* स्थिरांक varp = oldvarp->clone_वृक्ष(असत्य);
                    varp->name(name);
                    varp->protect(असत्य);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(सत्य);  // User needs to be able to get to it...
                    oldvarp->primaryIO(असत्य);
                    varp->primaryIO(सत्य);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    जर (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    जर (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(सत्य);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(असत्य);
                    }

                    जर (v3Global.opt.no_Trace_शीर्ष() && varp->isIO()) varp->trace(असत्य);

                    AstPin* स्थिरांक pinp = नवीन AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        नवीन AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } अन्यथा जर (v3Global.opt.शीर्ष_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    स्थिरांक Ast_गाठ_DType* स्थिरांक subtypep = oldvarp->subDTypep();
                    जर (VN_IS(subtypep, IfaceRefDType)) {
                        स्थिरांक AstIfaceRefDType* स्थिरांक ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        जर (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            जर (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = नवीन AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* स्थिरांक idtypep = नवीन AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = नवीन AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(सत्य);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(सत्य);

                            AstPin* स्थिरांक pinp
                                = नवीन AstPin{oldvarp->fileline(), 0, varp->name(),
                                             नवीन AstVarRef{varp->fileline(), varp,
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

शून्य V3LinkLevel::non_Wrap_शीर्ष(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    जर (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        परत;
    }

    // For all top modules
    साठी (Ast_गाठ_Module* modp = rootp->modulesp(); modp && modp->is_शीर्ष();
         modp = VN_AS(modp->nextp(), गाठ_Module)) {
        जर (VN_IS(modp, Package)) सुरूठेवा;

        UINFO(5, "LOOP " << modp);

        साठी (Ast_गाठ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                जर (varp->isIO()) {
                    varp->protect(असत्य);
                    varp->sigPublic(सत्य);
                    varp->primaryIO(सत्य);
                    जर (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    जर (v3Global.opt.systemC()) varp->sc(सत्य);
                    जर (v3Global.opt.no_Trace_शीर्ष()) varp->trace(असत्य);
                }
            }
        }
    }
}
