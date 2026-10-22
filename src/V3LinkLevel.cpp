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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkLevel.h"

#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

સંરચના CmpLevel final {
    તાર્કિક operator()(અચલ Ast_ગાંઠ_Module* lhsp, અચલ Ast_ગાંઠ_Module* rhsp) અચલ {
        પાછા lhsp->level() < rhsp->level();
    }
};

શૂન્ય V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    માટે (Ast_ગાંઠ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ગાંઠ_Module)) {
        જો (nodep->is_ટોચ() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.ધકેલો_back(nodep);
        }
        mods.ધકેલો_back(nodep);
    }
    જો (tops.size() >= 2) {
        અચલ Ast_ગાંઠ* અચલ secp = tops[1];  // Complain about second one, as first often intended
        જો (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warn_ટોચ_Modules = [](અચલ Ast_ગાંઠ* અચલ secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                માટે (Ast_ગાંઠ* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                પાછા ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_ટોચ_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    માટે (Ast_ગાંઠ_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    માટે (Ast_ગાંઠ_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_વૃક્ષ("cellsort", ખોટું, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3LinkLevel::timescaling(અચલ ModVec& mods) {
    // Timescale determination
    અચલ Ast_ગાંઠ_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    માટે (Ast_ગાંઠ_Module* modp : mods) {
        માટે (Ast_ગાંઠ *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            જો (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                જો (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_વૃક્ષ(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    માટે (Ast_ગાંઠ_Module* modp : mods) {
        જો (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            તોડો;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    જો (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    તાર્કિક dunitTimed = ખોટું;  // $unit had a timeunit
    જો (અચલ AstPackage* અચલ upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        જો (!upkgp->timeunit().isNone()) dunitTimed = સાચું;
    }

    માટે (Ast_ગાંઠ_Module* modp : mods) {
        જો (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        જો (modp->timeunit().isNone()) {
            જો (modTimedp  // Got previous
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

    જો (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

શૂન્ય V3LinkLevel::wrap_ટોચ(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_ગાંઠ_Module* અચલ oldmodp = rootp->modulesp();
    જો (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        પાછા;
    }
    rootp->resolved_ટોચ_Module_Name(oldmodp->name());

    Ast_ગાંઠ_Module* અચલ newmodp = નવું AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ગાંઠ::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(સાચું);
    newmodp->protect(ખોટું);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_ટોચ_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    માટે (Ast_ગાંઠ_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
        જો (VN_IS(modp, Package)) {
            AstCell* અચલ cellp
                = નવું AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ગાંઠ_Module* અચલ modp) {
        જો (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_વૃક્ષ("wraptop", 0, dump_વૃક્ષ_Either_Level() >= 6);
}

શૂન્ય V3LinkLevel::wrap_ટોચ_Cell(AstNetlist* rootp) {
    Ast_ગાંઠ_Module* અચલ newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_ટોચ(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    માટે (Ast_ગાંઠ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ગાંઠ_Module);
         oldmodp && oldmodp->is_ટોચ(); oldmodp = VN_AS(oldmodp->nextp(), ગાંઠ_Module)) {
        માટે (Ast_ગાંઠ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            જો (AstVar* અચલ oldvarp = VN_CAST(subnodep, Var)) {
                જો (oldvarp->isIO()) {
                    જો (!ioNames.ઉમેરો(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.ઉમેરો(oldvarp->name());
                    }
                } નહીંતર જો (v3Global.opt.ટોચ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    અચલ Ast_ગાંઠ_DType* અચલ subtypep = oldvarp->subDTypep();
                    જો (VN_IS(subtypep, IfaceRefDType)) {
                        અચલ AstIfaceRefDType* અચલ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        જો (!ifacerefp->cellp()) {
                            જો (!ioNames.ઉમેરો(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.ઉમેરો(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    માટે (Ast_ગાંઠ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ગાંઠ_Module);
         oldmodp && oldmodp->is_ટોચ(); oldmodp = VN_AS(oldmodp->nextp(), ગાંઠ_Module)) {
        જો (VN_IS(oldmodp, Package)) ચાલુ;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* અચલ cellp = નવું AstCell{
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
        માટે (Ast_ગાંઠ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            જો (AstVar* અચલ oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                જો (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    જો (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* અચલ varp = oldvarp->clone_વૃક્ષ(ખોટું);
                    varp->name(name);
                    varp->protect(ખોટું);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(સાચું);  // User needs to be able to get to it...
                    oldvarp->primaryIO(ખોટું);
                    varp->primaryIO(સાચું);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    જો (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    જો (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(સાચું);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(ખોટું);
                    }

                    જો (v3Global.opt.no_Trace_ટોચ() && varp->isIO()) varp->trace(ખોટું);

                    AstPin* અચલ pinp = નવું AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        નવું AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } નહીંતર જો (v3Global.opt.ટોચ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    અચલ Ast_ગાંઠ_DType* અચલ subtypep = oldvarp->subDTypep();
                    જો (VN_IS(subtypep, IfaceRefDType)) {
                        અચલ AstIfaceRefDType* અચલ ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        જો (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            જો (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = નવું AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* અચલ idtypep = નવું AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = નવું AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(સાચું);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(સાચું);

                            AstPin* અચલ pinp
                                = નવું AstPin{oldvarp->fileline(), 0, varp->name(),
                                             નવું AstVarRef{varp->fileline(), varp,
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

શૂન્ય V3LinkLevel::non_Wrap_ટોચ(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    જો (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        પાછા;
    }

    // For all top modules
    માટે (Ast_ગાંઠ_Module* modp = rootp->modulesp(); modp && modp->is_ટોચ();
         modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
        જો (VN_IS(modp, Package)) ચાલુ;

        UINFO(5, "LOOP " << modp);

        માટે (Ast_ગાંઠ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                જો (varp->isIO()) {
                    varp->protect(ખોટું);
                    varp->sigPublic(સાચું);
                    varp->primaryIO(સાચું);
                    જો (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    જો (v3Global.opt.systemC()) varp->sc(સાચું);
                    જો (v3Global.opt.no_Trace_ટોચ()) varp->trace(ખોટું);
                }
            }
        }
    }
}
