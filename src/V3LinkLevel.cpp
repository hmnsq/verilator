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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkLevel.h"

#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

ഘടന CmpLevel final {
    തർക്കം operator()(സ്ഥിരം Ast_കണ്ണി_Module* lhsp, സ്ഥിരം Ast_കണ്ണി_Module* rhsp) സ്ഥിരം {
        തിരികെ lhsp->level() < rhsp->level();
    }
};

ശൂന്യം V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    വേണ്ടി (Ast_കണ്ണി_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), കണ്ണി_Module)) {
        എങ്കിൽ (nodep->is_മുകൾ() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.തള്ളുക_back(nodep);
        }
        mods.തള്ളുക_back(nodep);
    }
    എങ്കിൽ (tops.size() >= 2) {
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം secp = tops[1];  // Complain about second one, as first often intended
        എങ്കിൽ (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            auto warn_മുകൾ_Modules = [](സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                വേണ്ടി (Ast_കണ്ണി* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                തിരികെ ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warnMore()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_മുകൾ_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_sort(mods.begin(), mods.end(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    വേണ്ടി (Ast_കണ്ണി_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    വേണ്ടി (Ast_കണ്ണി_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_വൃക്ഷം("cellsort", അസത്യം, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3LinkLevel::timescaling(സ്ഥിരം ModVec& mods) {
    // Timescale determination
    സ്ഥിരം Ast_കണ്ണി_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    വേണ്ടി (Ast_കണ്ണി_Module* modp : mods) {
        വേണ്ടി (Ast_കണ്ണി *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            എങ്കിൽ (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                എങ്കിൽ (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_വൃക്ഷം(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    വേണ്ടി (Ast_കണ്ണി_Module* modp : mods) {
        എങ്കിൽ (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            നിർത്തുക;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    എങ്കിൽ (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    തർക്കം dunitTimed = അസത്യം;  // $unit had a timeunit
    എങ്കിൽ (സ്ഥിരം AstPackage* സ്ഥിരം upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        എങ്കിൽ (!upkgp->timeunit().isNone()) dunitTimed = സത്യം;
    }

    വേണ്ടി (Ast_കണ്ണി_Module* modp : mods) {
        എങ്കിൽ (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        എങ്കിൽ (modp->timeunit().isNone()) {
            എങ്കിൽ (modTimedp  // Got previous
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

    എങ്കിൽ (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

ശൂന്യം V3LinkLevel::wrap_മുകൾ(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_കണ്ണി_Module* സ്ഥിരം oldmodp = rootp->modulesp();
    എങ്കിൽ (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        തിരികെ;
    }
    rootp->resolved_മുകൾ_Module_Name(oldmodp->name());

    Ast_കണ്ണി_Module* സ്ഥിരം newmodp = പുതിയ AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_കണ്ണി::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlinkFrBackWithNext();
    newmodp->addNext(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(സത്യം);
    newmodp->protect(അസത്യം);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_മുകൾ_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    വേണ്ടി (Ast_കണ്ണി_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
        എങ്കിൽ (VN_IS(modp, Package)) {
            AstCell* സ്ഥിരം cellp
                = പുതിയ AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_കണ്ണി_Module* സ്ഥിരം modp) {
        എങ്കിൽ (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_വൃക്ഷം("wraptop", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}

ശൂന്യം V3LinkLevel::wrap_മുകൾ_Cell(AstNetlist* rootp) {
    Ast_കണ്ണി_Module* സ്ഥിരം newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_മുകൾ(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    വേണ്ടി (Ast_കണ്ണി_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), കണ്ണി_Module);
         oldmodp && oldmodp->is_മുകൾ(); oldmodp = VN_AS(oldmodp->nextp(), കണ്ണി_Module)) {
        വേണ്ടി (Ast_കണ്ണി* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം oldvarp = VN_CAST(subnodep, Var)) {
                എങ്കിൽ (oldvarp->isIO()) {
                    എങ്കിൽ (!ioNames.തിരുകുക(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.തിരുകുക(oldvarp->name());
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.മുകൾ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം subtypep = oldvarp->subDTypep();
                    എങ്കിൽ (VN_IS(subtypep, IfaceRefDType)) {
                        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        എങ്കിൽ (!ifacerefp->cellp()) {
                            എങ്കിൽ (!ioNames.തിരുകുക(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.തിരുകുക(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    വേണ്ടി (Ast_കണ്ണി_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), കണ്ണി_Module);
         oldmodp && oldmodp->is_മുകൾ(); oldmodp = VN_AS(oldmodp->nextp(), കണ്ണി_Module)) {
        എങ്കിൽ (VN_IS(oldmodp, Package)) തുടരുക;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* സ്ഥിരം cellp = പുതിയ AstCell{
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
        വേണ്ടി (Ast_കണ്ണി* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                എങ്കിൽ (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    എങ്കിൽ (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* സ്ഥിരം varp = oldvarp->clone_വൃക്ഷം(അസത്യം);
                    varp->name(name);
                    varp->protect(അസത്യം);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(സത്യം);  // User needs to be able to get to it...
                    oldvarp->primaryIO(അസത്യം);
                    varp->primaryIO(സത്യം);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    എങ്കിൽ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    എങ്കിൽ (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(സത്യം);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(അസത്യം);
                    }

                    എങ്കിൽ (v3Global.opt.no_Trace_മുകൾ() && varp->isIO()) varp->trace(അസത്യം);

                    AstPin* സ്ഥിരം pinp = പുതിയ AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        പുതിയ AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.മുകൾ_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം subtypep = oldvarp->subDTypep();
                    എങ്കിൽ (VN_IS(subtypep, IfaceRefDType)) {
                        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        എങ്കിൽ (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            എങ്കിൽ (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = പുതിയ AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* സ്ഥിരം idtypep = പുതിയ AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = പുതിയ AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(സത്യം);
                            ifacecellp->addNextHere(varp);
                            ifacecellp->hasIfaceVar(സത്യം);

                            AstPin* സ്ഥിരം pinp
                                = പുതിയ AstPin{oldvarp->fileline(), 0, varp->name(),
                                             പുതിയ AstVarRef{varp->fileline(), varp,
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

ശൂന്യം V3LinkLevel::non_Wrap_മുകൾ(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    എങ്കിൽ (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        തിരികെ;
    }

    // For all top modules
    വേണ്ടി (Ast_കണ്ണി_Module* modp = rootp->modulesp(); modp && modp->is_മുകൾ();
         modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
        എങ്കിൽ (VN_IS(modp, Package)) തുടരുക;

        UINFO(5, "LOOP " << modp);

        വേണ്ടി (Ast_കണ്ണി* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                എങ്കിൽ (varp->isIO()) {
                    varp->protect(അസത്യം);
                    varp->sigPublic(സത്യം);
                    varp->primaryIO(സത്യം);
                    എങ്കിൽ (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    എങ്കിൽ (v3Global.opt.systemC()) varp->sc(സത്യം);
                    എങ്കിൽ (v3Global.opt.no_Trace_മുകൾ()) varp->trace(അസത്യം);
                }
            }
        }
    }
}
