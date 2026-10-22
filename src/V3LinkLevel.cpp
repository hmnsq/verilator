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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3LinkLevel.h"

#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

struct CmpLevel final {
    bool operator()(const AstNodeModule* lhsp, const AstNodeModule* rhsp) const {
        반환 lhsp->level() < rhsp->level();
    }
};

void V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    위해 (AstNodeModule* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), NodeModule)) {
        만약 (nodep->isTop() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.push_back(nodep);
        }
        mods.push_back(nodep);
    }
    만약 (tops.size() >= 2) {
        const AstNode* const secp = tops[1];  // Complain about second one, as first often intended
        만약 (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warnTopModules = [](const AstNode* const secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                위해 (AstNode* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                반환 ss.str();
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
    위해 (AstNodeModule* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    위해 (AstNodeModule* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dumpCheckGlobalTree("cellsort", false, dumpTreeEitherLevel() >= 3);
}

void V3LinkLevel::timescaling(const ModVec& mods) {
    // Timescale determination
    const AstNodeModule* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    위해 (AstNodeModule* modp : mods) {
        위해 (AstNode *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            만약 (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                만약 (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->deleteTree(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    위해 (AstNodeModule* modp : mods) {
        만약 (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            break;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    만약 (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    bool dunitTimed = false;  // $unit had a timeunit
    만약 (const AstPackage* const upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        만약 (!upkgp->timeunit().isNone()) dunitTimed = true;
    }

    위해 (AstNodeModule* modp : mods) {
        만약 (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        만약 (modp->timeunit().isNone()) {
            만약 (modTimedp  // Got previous
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

    만약 (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

void V3LinkLevel::wrapTop(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    AstNodeModule* const oldmodp = rootp->modulesp();
    만약 (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        반환;
    }
    rootp->resolvedTopModuleName(oldmodp->name());

    AstNodeModule* const newmodp = new AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(AstNode::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(true);
    newmodp->protect(false);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrapTopCell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    위해 (AstNodeModule* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), NodeModule)) {
        만약 (VN_IS(modp, Package)) {
            AstCell* const cellp
                = new AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](AstNodeModule* const modp) {
        만약 (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dumpCheckGlobalTree("wraptop", 0, dumpTreeEitherLevel() >= 6);
}

void V3LinkLevel::wrapTopCell(AstNetlist* rootp) {
    AstNodeModule* const newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->isTop(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    위해 (AstNodeModule* oldmodp = VN_AS(rootp->modulesp()->nextp(), NodeModule);
         oldmodp && oldmodp->isTop(); oldmodp = VN_AS(oldmodp->nextp(), NodeModule)) {
        위해 (AstNode* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            만약 (AstVar* const oldvarp = VN_CAST(subnodep, Var)) {
                만약 (oldvarp->isIO()) {
                    만약 (!ioNames.insert(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.insert(oldvarp->name());
                    }
                } 아니면 만약 (v3Global.opt.topIfacesSupported() && oldvarp->isIfaceRef()) {
                    const AstNodeDType* const subtypep = oldvarp->subDTypep();
                    만약 (VN_IS(subtypep, IfaceRefDType)) {
                        const AstIfaceRefDType* const ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        만약 (!ifacerefp->cellp()) {
                            만약 (!ioNames.insert(oldvarp->name()).second) {
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
    위해 (AstNodeModule* oldmodp = VN_AS(rootp->modulesp()->nextp(), NodeModule);
         oldmodp && oldmodp->isTop(); oldmodp = VN_AS(oldmodp->nextp(), NodeModule)) {
        만약 (VN_IS(oldmodp, Package)) continue;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* const cellp = new AstCell{
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
        위해 (AstNode* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            만약 (AstVar* const oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                만약 (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    만약 (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* const varp = oldvarp->cloneTree(false);
                    varp->name(name);
                    varp->protect(false);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(true);  // User needs to be able to get to it...
                    oldvarp->primaryIO(false);
                    varp->primaryIO(true);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    만약 (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    만약 (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(true);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(false);
                    }

                    만약 (v3Global.opt.noTraceTop() && varp->isIO()) varp->trace(false);

                    AstPin* const pinp = new AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        new AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } 아니면 만약 (v3Global.opt.topIfacesSupported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    const AstNodeDType* const subtypep = oldvarp->subDTypep();
                    만약 (VN_IS(subtypep, IfaceRefDType)) {
                        const AstIfaceRefDType* const ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        만약 (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            만약 (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = new AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* const idtypep = new AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = new AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(true);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(true);

                            AstPin* const pinp
                                = new AstPin{oldvarp->fileline(), 0, varp->name(),
                                             new AstVarRef{varp->fileline(), varp,
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

void V3LinkLevel::nonWrapTop(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    만약 (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        반환;
    }

    // For all top modules
    위해 (AstNodeModule* modp = rootp->modulesp(); modp && modp->isTop();
         modp = VN_AS(modp->nextp(), NodeModule)) {
        만약 (VN_IS(modp, Package)) continue;

        UINFO(5, "LOOP " << modp);

        위해 (AstNode* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            만약 (AstVar* const varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                만약 (varp->isIO()) {
                    varp->protect(false);
                    varp->sigPublic(true);
                    varp->primaryIO(true);
                    만약 (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    만약 (v3Global.opt.systemC()) varp->sc(true);
                    만약 (v3Global.opt.noTraceTop()) varp->trace(false);
                }
            }
        }
    }
}
