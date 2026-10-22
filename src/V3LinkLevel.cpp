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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LinkLevel.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Levelizing class functions

काष्ठा CmpLevel final {
    bool चालक()(स्थिर Ast_गाँठ_Module* lhsp, स्थिर Ast_गाँठ_Module* rhsp) स्थिर {
        वापस lhsp->level() < rhsp->level();
    }
};

व्योम V3LinkLevel::modSortByLevel() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    क्रम (Ast_गाँठ_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), गाँठ_Module)) {
        अगर (nodep->is_शीर्ष() && !VN_IS(nodep, NotFoundModule)) {
            UINFO(9, "top candidate " << nodep);
            tops.धकेलो_back(nodep);
        }
        mods.धकेलो_back(nodep);
    }
    अगर (tops.आकार() >= 2) {
        स्थिर Ast_गाँठ* स्थिर secp = tops[1];  // Complain about second one, as first often intended
        अगर (!secp->fileline()->warnIsOff(V3ErrorCode::MULTITOP)) {
            स्वतः warn_शीर्ष_Modules = [](स्थिर Ast_गाँठ* स्थिर secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                क्रम (Ast_गाँठ* alsop : tops) {
                    ss << secp->warnMore() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                वापस ss.str();
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
    क्रम (Ast_गाँठ_Module* nodep : mods) nodep->unlinkFrBack();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    क्रम (Ast_गाँठ_Module* nodep : mods) v3Global.rootp()->addModulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_वृक्ष("cellsort", false, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3LinkLevel::timescaling(स्थिर ModVec& mods) {
    // Timescale determination
    स्थिर Ast_गाँठ_Module* modTimedp = nullptr;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    क्रम (Ast_गाँठ_Module* modp : mods) {
        क्रम (Ast_गाँठ *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            अगर (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                अगर (pragp->pragType() == VPragmaType::TIMEUNIT_SET) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlinkFrBack()->delete_वृक्ष(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    क्रम (Ast_गाँठ_Module* modp : mods) {
        अगर (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            अवरोध;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    अगर (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    bool dunitTimed = false;  // $unit had a timeunit
    अगर (स्थिर AstPackage* स्थिर upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        अगर (!upkgp->timeunit().isNone()) dunitTimed = true;
    }

    क्रम (Ast_गाँठ_Module* modp : mods) {
        अगर (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        अगर (modp->timeunit().isNone()) {
            अगर (modTimedp  // Got previous
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

    अगर (v3Global.rootp()->timeprecision().isNone()) {
        v3Global.rootp()->timeprecisionMerge(v3Global.rootp()->fileline(),
                                             VTimescale{VTimescale::TS_DEFAULT});
    }

    // Classes under package have timescale propagated in V3LinkParse
}

//######################################################################
// Wrapping

व्योम V3LinkLevel::wrap_शीर्ष(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    // We do ONLY the top module
    Ast_गाँठ_Module* स्थिर oldmodp = rootp->modulesp();
    अगर (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        वापस;
    }
    rootp->resolved_शीर्ष_Module_Name(oldmodp->name());

    Ast_गाँठ_Module* स्थिर newmodp = नया AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_गाँठ::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_Back_With_अग्र();
    newmodp->add_अग्र(oldmodp);
    newmodp->depth(1);
    newmodp->level(1);
    newmodp->modPublic(true);
    newmodp->protect(false);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->addModulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_शीर्ष_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    क्रम (Ast_गाँठ_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), गाँठ_Module)) {
        अगर (VN_IS(modp, Package)) {
            AstCell* स्थिर cellp
                = नया AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), nullptr, nullptr, nullptr};
            cellp->modp(modp);
            newmodp->addStmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_गाँठ_Module* स्थिर modp) {
        अगर (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_वृक्ष("wraptop", 0, dump_वृक्ष_Either_Level() >= 6);
}

व्योम V3LinkLevel::wrap_शीर्ष_Cell(AstNetlist* rootp) {
    Ast_गाँठ_Module* स्थिर newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_शीर्ष(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    using NameSet = std::unordered_set<std::string>;
    NameSet ioNames;
    NameSet dupNames;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    क्रम (Ast_गाँठ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), गाँठ_Module);
         oldmodp && oldmodp->is_शीर्ष(); oldmodp = VN_AS(oldmodp->nextp(), गाँठ_Module)) {
        क्रम (Ast_गाँठ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            अगर (AstVar* स्थिर oldvarp = VN_CAST(subnodep, Var)) {
                अगर (oldvarp->isIO()) {
                    अगर (!ioNames.जोड़ो(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dupNames.जोड़ो(oldvarp->name());
                    }
                } अन्यथा अगर (v3Global.opt.शीर्ष_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    स्थिर Ast_गाँठ_DType* स्थिर subtypep = oldvarp->subDTypep();
                    अगर (VN_IS(subtypep, IfaceRefDType)) {
                        स्थिर AstIfaceRefDType* स्थिर ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        अगर (!ifacerefp->cellp()) {
                            अगर (!ioNames.जोड़ो(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dupNames.जोड़ो(oldvarp->name());
                            }
                        }
                    }
                }
            }
        }
    }

    // For all modules, skipping over new top
    क्रम (Ast_गाँठ_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), गाँठ_Module);
         oldmodp && oldmodp->is_शीर्ष(); oldmodp = VN_AS(oldmodp->nextp(), गाँठ_Module)) {
        अगर (VN_IS(oldmodp, Package)) जारी;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* स्थिर cellp = नया AstCell{
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
        क्रम (Ast_गाँठ* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            अगर (AstVar* स्थिर oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                अगर (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    अगर (dupNames.find(name) != dupNames.end()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* स्थिर varp = oldvarp->clone_वृक्ष(false);
                    varp->name(name);
                    varp->protect(false);
                    newmodp->addStmtsp(varp);
                    varp->sigPublic(true);  // User needs to be able to get to it...
                    oldvarp->primaryIO(false);
                    varp->primaryIO(true);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    अगर (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    अगर (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(true);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(false);
                    }

                    अगर (v3Global.opt.no_Trace_शीर्ष() && varp->isIO()) varp->trace(false);

                    AstPin* स्थिर pinp = नया AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        नया AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->addPinsp(pinp);
                } अन्यथा अगर (v3Global.opt.शीर्ष_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    स्थिर Ast_गाँठ_DType* स्थिर subtypep = oldvarp->subDTypep();
                    अगर (VN_IS(subtypep, IfaceRefDType)) {
                        स्थिर AstIfaceRefDType* स्थिर ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        अगर (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            अगर (dupNames.find(name) != dupNames.end()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = नया AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->addStmtsp(ifacecellp);

                            AstIfaceRefDType* स्थिर idtypep = नया AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(nullptr);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->addTypesp(idtypep);

                            AstVar* varp = नया AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(true);
                            ifacecellp->add_अग्र_Here(varp);
                            ifacecellp->hasIfaceVar(true);

                            AstPin* स्थिर pinp
                                = नया AstPin{oldvarp->fileline(), 0, varp->name(),
                                             नया AstVarRef{varp->fileline(), varp,
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

व्योम V3LinkLevel::non_Wrap_शीर्ष(AstNetlist* rootp) {
    // Perform variable setting step for top modules to prevent
    // optimizing away primary ports when only serializing input
    अगर (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        वापस;
    }

    // For all top modules
    क्रम (Ast_गाँठ_Module* modp = rootp->modulesp(); modp && modp->is_शीर्ष();
         modp = VN_AS(modp->nextp(), गाँठ_Module)) {
        अगर (VN_IS(modp, Package)) जारी;

        UINFO(5, "LOOP " << modp);

        क्रम (Ast_गाँठ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                अगर (varp->isIO()) {
                    varp->protect(false);
                    varp->sigPublic(true);
                    varp->primaryIO(true);
                    अगर (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    अगर (v3Global.opt.systemC()) varp->sc(true);
                    अगर (v3Global.opt.no_Trace_शीर्ष()) varp->trace(false);
                }
            }
        }
    }
}
