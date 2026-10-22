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
    منطقي operator()(ثابت AstNodeModule* lhsp, ثابت AstNodeModule* rhsp) ثابت {
        أرجع lhsp->level() < rhsp->level();
    }
};

فراغ V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    لكل (AstNodeModule* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), NodeModule)) {
        𐤀𐤌 (nodep->isTop() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.push_back(nodep);
        }
        mods.push_back(nodep);
    }
    𐤀𐤌 (tops.size() >= 2) {
        ثابت AstNode* ثابت secp = tops[1];  // Complain about second one, as first often intended
        𐤀𐤌 (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warnTopModules = [](ثابت AstNode* ثابت secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                لكل (AstNode* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                أرجع ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warnTopModules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    لكل (AstNodeModule* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    لكل (AstNodeModule* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dumpCheckGlobalTree("cellsort", خطأ, dumpTreeEitherLevel() >= 3);
}

فراغ V3LinkLevel::timescaling(ثابت ModVec& mods) {
    // Timescale determination
    ثابت AstNodeModule* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    لكل (AstNodeModule* modp : mods) {
        لكل (AstNode *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            𐤀𐤌 (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                𐤀𐤌 (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->deleteTree(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    لكل (AstNodeModule* modp : mods) {
        𐤀𐤌 (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            اكسر;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    𐤀𐤌 (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    منطقي dunitTimed = خطأ;  // $unit had a timeunit
    𐤀𐤌 (ثابت AstPackage* ثابت upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        𐤀𐤌 (!upkgp->timeunit().isNone()) dunitTimed = true;
    }

    لكل (AstNodeModule* modp : mods) {
        𐤀𐤌 (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        𐤀𐤌 (modp->timeunit().isNone()) {
            𐤀𐤌 (modTimedp  // Got previous
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

    𐤀𐤌 (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

فراغ V3LinkLevel::wrapTop(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    AstNodeModule* ثابت oldmodp = rootp->modulesp();
    𐤀𐤌 (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        أرجع;
    }
    rootp->resolvedTopModuleName(oldmodp->name());

    AstNodeModule* ثابت newmodp = جديد AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(AstNode::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(true);
    newmodp->protect(خطأ);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrapTopCell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    لكل (AstNodeModule* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), NodeModule)) {
        𐤀𐤌 (VN_IS(modp, Package)) {
            AstCell* ثابت cellp
                = جديد AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](AstNodeModule* ثابت modp) {
        𐤀𐤌 (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dumpCheckGlobalTree("wraptop", 0, dumpTreeEitherLevel() >= 6);
}

فراغ V3LinkLevel::wrapTopCell(AstNetlist* rootp) {
    AstNodeModule* ثابت newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->isTop(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    لكل (AstNodeModule* oldmodp = VN_AS(rootp->modulesp()->nextp(), NodeModule);
         oldmodp && oldmodp->isTop(); oldmodp = VN_AS(oldmodp->nextp(), NodeModule)) {
        لكل (AstNode* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            𐤀𐤌 (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                𐤀𐤌 (oldvarp->isIO()) {
                    𐤀𐤌 (!ioNames.insert(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.insert(oldvarp->name());
                    }
                } وإلا 𐤀𐤌 (v3Global.opt.topIfacesSupported() && oldvarp->isIfaceRef()) {
                    ثابت AstNodeDType* ثابت subtypep = oldvarp->subDTypep();
                    𐤀𐤌 (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        𐤀𐤌 (!ifacerefp->cellp()) {
                            𐤀𐤌 (!ioNames.insert(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.insert(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    لكل (AstNodeModule* oldmodp = VN_AS(rootp->modulesp()->nextp(), NodeModule);
         oldmodp && oldmodp->isTop(); oldmodp = VN_AS(oldmodp->nextp(), NodeModule)) {
        𐤀𐤌 (VN_IS(oldmodp, Package)) استمر;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ثابت cellp = جديد AstCell{
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
        لكل (AstNode* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            𐤀𐤌 (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                𐤀𐤌 (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    𐤀𐤌 (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ثابت varp = oldvarp->cloneTree(خطأ);
                    varp->name(name);
                    varp->protect(خطأ);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(true);  // User needs to be able to get to it...
                    oldvarp->primaryIO(خطأ);
                    varp->primaryIO(true);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    𐤀𐤌 (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    𐤀𐤌 (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(true);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(خطأ);
                    }

                    𐤀𐤌 (v3Global.opt.noTraceTop() && varp->isIO()) varp->trace(خطأ);

                    AstPin* ثابت pinp = جديد AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        جديد AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } وإلا 𐤀𐤌 (v3Global.opt.topIfacesSupported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ثابت AstNodeDType* ثابت subtypep = oldvarp->subDTypep();
                    𐤀𐤌 (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        𐤀𐤌 (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            𐤀𐤌 (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = جديد AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* ثابت idtypep = جديد AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = جديد AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(true);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(true);

                            AstPin* ثابت pinp
                                = جديد AstPin{oldvarp->fileline(), 0, varp->name(),
                                             جديد AstVarRef{varp->fileline(), varp,
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

فراغ V3LinkLevel::nonWrapTop(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    𐤀𐤌 (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        أرجع;
    }

    // For all top modules
    لكل (AstNodeModule* modp = rootp->modulesp(); modp && modp->isTop();
         modp = VN_AS(modp->nextp(), NodeModule)) {
        𐤀𐤌 (VN_IS(modp, Package)) استمر;

        UINFO(5, "LOOP " << modp);

        لكل (AstNode* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            𐤀𐤌 (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                𐤀𐤌 (varp->isIO()) {
                    varp->protect(خطأ);
                    varp->sigPublic(true);
                    varp->primaryIO(true);
                    𐤀𐤌 (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    𐤀𐤌 (v3Global.opt.systemC()) varp->sc(true);
                    𐤀𐤌 (v3Global.opt.noTraceTop()) varp->trace(خطأ);
                }
            }
        }
    }
}
