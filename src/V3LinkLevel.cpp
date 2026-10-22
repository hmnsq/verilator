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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkLevel.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

ساختار CmpLevel final {
    منطقی operator()(ثابت Ast_گره_Module* lhsp, ثابت Ast_گره_Module* rhsp) ثابت {
        بازگشت lhsp->level() < rhsp->level();
    }
};

تهی V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    برای (Ast_گره_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), گره_Module)) {
        اگر (nodep->is_بالاترین() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.افزودن_back(nodep);
        }
        mods.افزودن_back(nodep);
    }
    اگر (tops.size() >= 2) {
        ثابت Ast_گره* ثابت secp = tops[1];  // Complain about second one, as first often intended
        اگر (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warn_بالاترین_Modules = [](ثابت Ast_گره* ثابت secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                برای (Ast_گره* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                بازگشت ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_بالاترین_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    برای (Ast_گره_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    برای (Ast_گره_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_درخت("cellsort", نادرست, dump_درخت_Either_Level() >= 3);
}

تهی V3LinkLevel::timescaling(ثابت ModVec& mods) {
    // Timescale determination
    ثابت Ast_گره_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    برای (Ast_گره_Module* modp : mods) {
        برای (Ast_گره *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            اگر (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                اگر (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_درخت(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    برای (Ast_گره_Module* modp : mods) {
        اگر (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            بشکن;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    اگر (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    منطقی dunitTimed = نادرست;  // $unit had a timeunit
    اگر (ثابت AstPackage* ثابت upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        اگر (!upkgp->timeunit().isNone()) dunitTimed = درست;
    }

    برای (Ast_گره_Module* modp : mods) {
        اگر (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        اگر (modp->timeunit().isNone()) {
            اگر (modTimedp  // Got previous
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

    اگر (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

تهی V3LinkLevel::wrap_بالاترین(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_گره_Module* ثابت oldmodp = rootp->modulesp();
    اگر (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        بازگشت;
    }
    rootp->resolved_بالاترین_Module_Name(oldmodp->name());

    Ast_گره_Module* ثابت newmodp = جدید AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_گره::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(درست);
    newmodp->protect(نادرست);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_بالاترین_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    برای (Ast_گره_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), گره_Module)) {
        اگر (VN_IS(modp, Package)) {
            AstCell* ثابت cellp
                = جدید AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_گره_Module* ثابت modp) {
        اگر (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_درخت("wraptop", 0, dump_درخت_Either_Level() >= 6);
}

تهی V3LinkLevel::wrap_بالاترین_Cell(AstNetlist* rootp) {
    Ast_گره_Module* ثابت newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_بالاترین(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    برای (Ast_گره_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), گره_Module);
         oldmodp && oldmodp->is_بالاترین(); oldmodp = VN_AS(oldmodp->nextp(), گره_Module)) {
        برای (Ast_گره* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            اگر (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                اگر (oldvarp->isIO()) {
                    اگر (!ioNames.درج(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.درج(oldvarp->name());
                    }
                } وگرنه اگر (v3Global.opt.بالاترین_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ثابت Ast_گره_DType* ثابت subtypep = oldvarp->subDTypep();
                    اگر (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        اگر (!ifacerefp->cellp()) {
                            اگر (!ioNames.درج(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.درج(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    برای (Ast_گره_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), گره_Module);
         oldmodp && oldmodp->is_بالاترین(); oldmodp = VN_AS(oldmodp->nextp(), گره_Module)) {
        اگر (VN_IS(oldmodp, Package)) ادامه;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ثابت cellp = جدید AstCell{
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
        برای (Ast_گره* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            اگر (AstVar* ثابت oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                اگر (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    اگر (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ثابت varp = oldvarp->clone_درخت(نادرست);
                    varp->name(name);
                    varp->protect(نادرست);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(درست);  // User needs to be able to get to it...
                    oldvarp->primaryIO(نادرست);
                    varp->primaryIO(درست);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    اگر (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    اگر (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(درست);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(نادرست);
                    }

                    اگر (v3Global.opt.no_Trace_بالاترین() && varp->isIO()) varp->trace(نادرست);

                    AstPin* ثابت pinp = جدید AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        جدید AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } وگرنه اگر (v3Global.opt.بالاترین_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ثابت Ast_گره_DType* ثابت subtypep = oldvarp->subDTypep();
                    اگر (VN_IS(subtypep, IfaceRefDType)) {
                        ثابت AstIfaceRefDType* ثابت ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        اگر (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            اگر (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = جدید AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* ثابت idtypep = جدید AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = جدید AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(درست);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(درست);

                            AstPin* ثابت pinp
                                = جدید AstPin{oldvarp->fileline(), 0, varp->name(),
                                             جدید AstVarRef{varp->fileline(), varp,
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

تهی V3LinkLevel::non_Wrap_بالاترین(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    اگر (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        بازگشت;
    }

    // For all top modules
    برای (Ast_گره_Module* modp = rootp->modulesp(); modp && modp->is_بالاترین();
         modp = VN_AS(modp->nextp(), گره_Module)) {
        اگر (VN_IS(modp, Package)) ادامه;

        UINFO(5, "LOOP " << modp);

        برای (Ast_گره* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                اگر (varp->isIO()) {
                    varp->protect(نادرست);
                    varp->sigPublic(درست);
                    varp->primaryIO(درست);
                    اگر (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    اگر (v3Global.opt.systemC()) varp->sc(درست);
                    اگر (v3Global.opt.no_Trace_بالاترین()) varp->trace(نادرست);
                }
            }
        }
    }
}
