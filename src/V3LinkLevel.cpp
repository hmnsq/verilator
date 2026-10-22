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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkLevel.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

কাঠামো CmpLevel final {
    যৌক্তিক operator()(ধ্রুবক Ast_গ্রন্থি_Module* lhsp, ধ্রুবক Ast_গ্রন্থি_Module* rhsp) ধ্রুবক {
        ফেরত lhsp->level() < rhsp->level();
    }
};

শূন্য V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    জন্য (Ast_গ্রন্থি_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), গ্রন্থি_Module)) {
        যদি (nodep->is_শীর্ষ() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.ঠেলো_back(nodep);
        }
        mods.ঠেলো_back(nodep);
    }
    যদি (tops.আকার_2() >= 2) {
        ধ্রুবক Ast_গ্রন্থি* ধ্রুবক secp = tops[1];  // Complain about second one, as first often intended
        যদি (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warn_শীর্ষ_Modules = [](ধ্রুবক Ast_গ্রন্থি* ধ্রুবক secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                জন্য (Ast_গ্রন্থি* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                ফেরত ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_শীর্ষ_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    জন্য (Ast_গ্রন্থি_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    জন্য (Ast_গ্রন্থি_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_বৃক্ষ("cellsort", মিথ্যা, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3LinkLevel::timescaling(ধ্রুবক ModVec& mods) {
    // Timescale determination
    ধ্রুবক Ast_গ্রন্থি_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    জন্য (Ast_গ্রন্থি_Module* modp : mods) {
        জন্য (Ast_গ্রন্থি *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            যদি (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                যদি (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_বৃক্ষ(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    জন্য (Ast_গ্রন্থি_Module* modp : mods) {
        যদি (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            বিরতি;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    যদি (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    যৌক্তিক dunitTimed = মিথ্যা;  // $unit had a timeunit
    যদি (ধ্রুবক AstPackage* ধ্রুবক upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        যদি (!upkgp->timeunit().isNone()) dunitTimed = সত্য;
    }

    জন্য (Ast_গ্রন্থি_Module* modp : mods) {
        যদি (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        যদি (modp->timeunit().isNone()) {
            যদি (modTimedp  // Got previous
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

    যদি (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

শূন্য V3LinkLevel::wrap_শীর্ষ(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_গ্রন্থি_Module* ধ্রুবক oldmodp = rootp->modulesp();
    যদি (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        ফেরত;
    }
    rootp->resolved_শীর্ষ_Module_Name(oldmodp->name());

    Ast_গ্রন্থি_Module* ধ্রুবক newmodp = নতুন AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_গ্রন্থি::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_Back_With_পরবর্তী();
    newmodp->add_পরবর্তী(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(সত্য);
    newmodp->protect(মিথ্যা);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_শীর্ষ_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    জন্য (Ast_গ্রন্থি_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
        যদি (VN_IS(modp, Package)) {
            AstCell* ধ্রুবক cellp
                = নতুন AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_গ্রন্থি_Module* ধ্রুবক modp) {
        যদি (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_বৃক্ষ("wraptop", 0, dump_বৃক্ষ_Either_Level() >= 6);
}

শূন্য V3LinkLevel::wrap_শীর্ষ_Cell(AstNetlist* rootp) {
    Ast_গ্রন্থি_Module* ধ্রুবক newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_শীর্ষ(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    জন্য (Ast_গ্রন্থি_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), গ্রন্থি_Module);
         oldmodp && oldmodp->is_শীর্ষ(); oldmodp = VN_AS(oldmodp->nextp(), গ্রন্থি_Module)) {
        জন্য (Ast_গ্রন্থি* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            যদি (AstVar* ধ্রুবক oldvarp = VN_CAST(subnodep, Var)) {
                যদি (oldvarp->isIO()) {
                    যদি (!ioNames.ঢোকাও(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.ঢোকাও(oldvarp->name());
                    }
                } অন্যথা যদি (v3Global.opt.শীর্ষ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক subtypep = oldvarp->subDTypep();
                    যদি (VN_IS(subtypep, IfaceRefDType)) {
                        ধ্রুবক AstIfaceRefDType* ধ্রুবক ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        যদি (!ifacerefp->cellp()) {
                            যদি (!ioNames.ঢোকাও(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.ঢোকাও(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    জন্য (Ast_গ্রন্থি_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), গ্রন্থি_Module);
         oldmodp && oldmodp->is_শীর্ষ(); oldmodp = VN_AS(oldmodp->nextp(), গ্রন্থি_Module)) {
        যদি (VN_IS(oldmodp, Package)) চালু;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ধ্রুবক cellp = নতুন AstCell{
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
        জন্য (Ast_গ্রন্থি* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            যদি (AstVar* ধ্রুবক oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                যদি (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    যদি (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ধ্রুবক varp = oldvarp->clone_বৃক্ষ(মিথ্যা);
                    varp->name(name);
                    varp->protect(মিথ্যা);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(সত্য);  // User needs to be able to get to it...
                    oldvarp->primaryIO(মিথ্যা);
                    varp->primaryIO(সত্য);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    যদি (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    যদি (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(সত্য);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(মিথ্যা);
                    }

                    যদি (v3Global.opt.no_Trace_শীর্ষ() && varp->isIO()) varp->trace(মিথ্যা);

                    AstPin* ধ্রুবক pinp = নতুন AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        নতুন AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } অন্যথা যদি (v3Global.opt.শীর্ষ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক subtypep = oldvarp->subDTypep();
                    যদি (VN_IS(subtypep, IfaceRefDType)) {
                        ধ্রুবক AstIfaceRefDType* ধ্রুবক ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        যদি (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            যদি (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = নতুন AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* ধ্রুবক idtypep = নতুন AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = নতুন AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(সত্য);
                            ifacecellp->add_পরবর্তী_Here(varp);
                            ifacecellp->hasIfaceVar(সত্য);

                            AstPin* ধ্রুবক pinp
                                = নতুন AstPin{oldvarp->fileline(), 0, varp->name(),
                                             নতুন AstVarRef{varp->fileline(), varp,
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

শূন্য V3LinkLevel::non_Wrap_শীর্ষ(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    যদি (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        ফেরত;
    }

    // For all top modules
    জন্য (Ast_গ্রন্থি_Module* modp = rootp->modulesp(); modp && modp->is_শীর্ষ();
         modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
        যদি (VN_IS(modp, Package)) চালু;

        UINFO(5, "LOOP " << modp);

        জন্য (Ast_গ্রন্থি* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                যদি (varp->isIO()) {
                    varp->protect(মিথ্যা);
                    varp->sigPublic(সত্য);
                    varp->primaryIO(সত্য);
                    যদি (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    যদি (v3Global.opt.systemC()) varp->sc(সত্য);
                    যদি (v3Global.opt.no_Trace_শীর্ষ()) varp->trace(মিথ্যা);
                }
            }
        }
    }
}
