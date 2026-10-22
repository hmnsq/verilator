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
    तार्किक संक्रियकः()(ध्रुव Ast_ग्रन्थिः_Module* lhsp, ध्रुव Ast_ग्रन्थिः_Module* rhsp) ध्रुव {
        प्रत्यागम lhsp->level() < rhsp->level();
    }
};

शून्य V3LinkLevel::mod_क्रमय_By_Level() {
    // Sort modules by levels, root down to lowest children
    // Calculate levels again in case we added modules
    UINFO(2, "modSortByLevel()");

    // level() was computed for us in V3LinkCells

    ModVec mods;  // Modules
    ModVec tops;  // Top level modules
    प्रति (Ast_ग्रन्थिः_Module* nodep = v3Global.rootp()->modulesp(); nodep;
         nodep = VN_AS(nodep->nextp(), ग्रन्थिः_Module)) {
        यदि (nodep->is_शिखरम्() && !VN_IS(nodep, Not_प्राप्तम्_Module)) {
            UINFO(9, "top candidate " << nodep);
            tops.प्रक्षिप_पृष्ठभागः(nodep);
        }
        mods.प्रक्षिप_पृष्ठभागः(nodep);
    }
    यदि (tops.आकार() >= 2) {
        ध्रुव Ast_ग्रन्थिः* ध्रुव secp = tops[1];  // Complain about second one, as first often intended
        यदि (!secp->fileline()->warnIsOff(V3_Error_संहिता::MULTITOP)) {
            स्वचालित warn_शिखरम्_Modules = [](ध्रुव Ast_ग्रन्थिः* ध्रुव secp, ModVec tops)
                                      VL_REQUIRES(V3Error::s().m_mutex) -> std::string {
                std::stringstream ss;
                प्रति (Ast_ग्रन्थिः* alsop : tops) {
                    ss << secp->warn_अधिकम्() << "... Top module " << alsop->prettyNameQ() << '\n'
                       << alsop->warnContextSecondary();
                }
                प्रत्यागम ss.str();
            };

            secp->v3warn(MULTITOP, "Multiple top level modules\n"
                                       << secp->warn_अधिकम्()
                                       << "... Suggest see manual; fix the duplicates, or use "
                                          "--top-module to select top."
                                       << V3Error::s().warnContextNone()
                                       << V3Error::warnAdditionalInfo()
                                       << warn_शिखरम्_Modules(secp, tops));
        }
    }

    timescaling(mods);

    // Reorder the netlist's modules to have modules in level sorted order
    stable_क्रमय(mods.begin(), mods.अन्तः_2(), CmpLevel());  // Sort the vector
    UINFO(9, "modSortByLevel() sorted");  // Comment required for gcc4.6.3 / bug666
    प्रति (Ast_ग्रन्थिः_Module* nodep : mods) nodep->unlink_Fr_पृष्ठभागः();
    UASSERT_OBJ(!v3Global.rootp()->modulesp(), v3Global.rootp(), "Unlink didn't work");
    प्रति (Ast_ग्रन्थिः_Module* nodep : mods) v3Global.rootp()->सङ्कलय_Modulesp(nodep);
    UINFO(9, "modSortByLevel() done");  // Comment required for gcc4.6.3 / bug666
    V3Global::dump_Check_Global_वृक्षः("cellsort", असत्य, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3LinkLevel::timescaling(ध्रुव ModVec& mods) {
    // Timescale determination
    ध्रुव Ast_ग्रन्थिः_Module* modTimedp = शून्यसूचकः;
    VTimescale unit{VTimescale::NONE};

    // Move timeunit attributes from parse to module unit
    // Grammar only allows timeunit as module_item, so no need to recurse full tree
    प्रति (Ast_ग्रन्थिः_Module* modp : mods) {
        प्रति (Ast_ग्रन्थिः *nextp, *childp = modp->stmtsp(); childp; childp = nextp) {
            nextp = childp->nextp();
            // cppcheck-suppress constVariablePointer
            यदि (AstPragma* pragp = VN_CAST(childp, Pragma)) {
                यदि (pragp->pragType() == VPragmaType::TIMEUNIT_समुच्चयः) {
                    modp->timeunit(pragp->timescale());
                    VL_DO_DANGLING(pragp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), pragp);
                }
            }
        }
    }
    // Use highest level module as default unit - already sorted in proper order
    // Combine timing into later modules
    प्रति (Ast_ग्रन्थिः_Module* modp : mods) {
        यदि (!modTimedp && !modp->timeunit().isNone()) {
            modTimedp = modp;
            unit = modTimedp->timeunit();
            भङ्ग;
        }
    }
    unit = v3Global.opt.timeComputeUnit(unit);  // Apply override
    यदि (unit.isNone()) unit = VTimescale{VTimescale::TS_DEFAULT};
    v3Global.rootp()->timeunit(unit);

    तार्किक dunitTimed = असत्य;  // $unit had a timeunit
    यदि (ध्रुव AstPackage* ध्रुव upkgp = v3Global.rootp()->dollarUnitPkgp()) {
        यदि (!upkgp->timeunit().isNone()) dunitTimed = सत्य;
    }

    प्रति (Ast_ग्रन्थिः_Module* modp : mods) {
        यदि (!v3Global.opt.timeOverrideUnit().isNone()) modp->timeunit(unit);
        यदि (modp->timeunit().isNone()) {
            यदि (modTimedp  // Got previous
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

    यदि (v3Global.rootp()->timeprecision().isNone()) {
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
    यदि (!oldmodp) {  // Later V3LinkDot will warn
        UINFO(1, "No module found to wrap");
        प्रत्यागम;
    }
    rootp->resolved_शिखरम्_Module_Name(oldmodp->name());

    Ast_ग्रन्थिः_Module* ध्रुव newmodp = नूतन AstModule{oldmodp->fileline(), "$root", oldmodp->libname()};
    newmodp->name(Ast_ग्रन्थिः::encodeName(newmodp->name()));  // so origName is nice
    // Make the new module first in the list
    oldmodp->unlink_Fr_पृष्ठभागः_With_अग्र();
    newmodp->सङ्कलय_अग्र(oldmodp);
    newmodp->गभीरता(1);
    newmodp->level(1);
    newmodp->mod_लौकिकम्(सत्य);
    newmodp->protect(असत्य);
    newmodp->timeunit(oldmodp->timeunit());
    rootp->सङ्कलय_Modulesp(newmodp);

    // TODO the module creation above could be done after linkcells, but
    // the rest must be done after data type resolution
    wrap_शिखरम्_Cell(rootp);

    // Instantiate all packages under the top wrapper
    // This way all later SCOPE based optimizations can ignore packages
    प्रति (Ast_ग्रन्थिः_Module* modp = rootp->modulesp(); modp; modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
        यदि (VN_IS(modp, Package)) {
            AstCell* ध्रुव cellp
                = नूतन AstCell{modp->fileline(), modp->fileline(),
                              // Could add __03a__03a="::" to prevent conflict
                              // with module names/"v"
                              modp->name(), modp->name(), शून्यसूचकः, शून्यसूचकः, शून्यसूचकः};
            cellp->modp(modp);
            newmodp->सङ्कलय_Stmtsp(cellp);
        }
    }

    // All modules and hier-classes except one we created are now a level deeper
    rootp->foreach([&](Ast_ग्रन्थिः_Module* ध्रुव modp) {
        यदि (modp != newmodp && modp->level()) modp->level(1 + modp->level());
    });

    V3Global::dump_Check_Global_वृक्षः("wraptop", 0, dump_वृक्षः_Either_Level() >= 6);
}

शून्य V3LinkLevel::wrap_शिखरम्_Cell(AstNetlist* rootp) {
    Ast_ग्रन्थिः_Module* ध्रुव newmodp = rootp->modulesp();
    UASSERT_OBJ(newmodp && newmodp->is_शिखरम्(), rootp, "No TOP module found to insert under");

    // Find all duplicate signal names (if multitop)
    उपयुज्य Name_समुच्चयः = std::unordered_समुच्चयः<std::string>;
    Name_समुच्चयः io_नामानि;
    Name_समुच्चयः dup_नामानि;
    // For all modules, skipping over new top
    // cppcheck-suppress constVariablePointer
    प्रति (Ast_ग्रन्थिः_Module* oldmodp = VN_AS(rootp->modulesp()->nextp(), ग्रन्थिः_Module);
         oldmodp && oldmodp->is_शिखरम्(); oldmodp = VN_AS(oldmodp->nextp(), ग्रन्थिः_Module)) {
        प्रति (Ast_ग्रन्थिः* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            यदि (AstVar* ध्रुव oldvarp = VN_CAST(subnodep, Var)) {
                यदि (oldvarp->isIO()) {
                    यदि (!io_नामानि.निवेशय(oldvarp->name()).second) {
                        // UINFO(8, "Multitop dup I/O found: " << oldvarp);
                        dup_नामानि.निवेशय(oldvarp->name());
                    }
                } अन्यथा यदि (v3Global.opt.शिखरम्_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = oldvarp->subDTypep();
                    यदि (VN_IS(subtypep, IfaceRefDType)) {
                        ध्रुव AstIfaceRefDType* ध्रुव ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        यदि (!ifacerefp->cellp()) {
                            यदि (!io_नामानि.निवेशय(oldvarp->name()).second) {
                                // UINFO(8, "Multitop dup interface found: " << oldvarp);
                                dup_नामानि.निवेशय(oldvarp->name());
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
        यदि (VN_IS(oldmodp, Package)) अनुवर्त;
        // Add instance
        UINFO(5, "LOOP " << oldmodp);
        AstCell* ध्रुव cellp = नूतन AstCell{
            newmodp->fileline(),
            newmodp->fileline(),
            (!v3Global.opt.l2Name().रिक्तानि() ? v3Global.opt.l2Name() : oldmodp->name()),
            oldmodp->name(),
            शून्यसूचकः,
            शून्यसूचकः,
            शून्यसूचकः};
        cellp->modp(oldmodp);
        newmodp->सङ्कलय_Stmtsp(cellp);

        // Add pins
        प्रति (Ast_ग्रन्थिः* subnodep = oldmodp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            यदि (AstVar* ध्रुव oldvarp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << oldvarp);
                यदि (oldvarp->isIO()) {
                    string name = oldvarp->name();
                    यदि (dup_नामानि.find(name) != dup_नामानि.अन्तः_2()) {
                        // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                        name = oldmodp->name() + "__02E" + name;
                    }

                    AstVar* ध्रुव varp = oldvarp->clone_वृक्षः(असत्य);
                    varp->name(name);
                    varp->protect(असत्य);
                    newmodp->सङ्कलय_Stmtsp(varp);
                    varp->sig_लौकिकम्(सत्य);  // User needs to be able to get to it...
                    oldvarp->primaryIO(असत्य);
                    varp->primaryIO(सत्य);
                    varp->icoMaybeWritten(oldvarp->icoMaybeWritten());
                    यदि (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    यदि (varp->isIO() && v3Global.opt.systemC()) {
                        varp->sc(सत्य);
                        // User can see trace one level down from the wrapper
                        // Avoids packing & unpacking SC signals a second time
                        varp->trace(असत्य);
                    }

                    यदि (v3Global.opt.no_Trace_शिखरम्() && varp->isIO()) varp->trace(असत्य);

                    AstPin* ध्रुव pinp = नूतन AstPin{
                        oldvarp->fileline(), 0, varp->name(),
                        नूतन AstVarRef{varp->fileline(), varp,
                                      oldvarp->isWritable() ? VAccess::WRITE : VAccess::READ}};
                    // Skip length and width comp; we know it's a direct assignment
                    pinp->modVarp(oldvarp);
                    cellp->सङ्कलय_Pinsp(pinp);
                } अन्यथा यदि (v3Global.opt.शिखरम्_Ifaces_Supported() && oldvarp->isIfaceRef()) {
                    // for each interface port on oldmodp instantiate a corresponding interface
                    // cell in $root
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = oldvarp->subDTypep();
                    यदि (VN_IS(subtypep, IfaceRefDType)) {
                        ध्रुव AstIfaceRefDType* ध्रुव ifacerefp = VN_AS(subtypep, IfaceRefDType);
                        यदि (!ifacerefp->cellp()) {
                            string name = oldvarp->name();
                            यदि (dup_नामानि.find(name) != dup_नामानि.अन्तः_2()) {
                                // __02E=. while __DOT__ looks nicer but will break V3LinkDot
                                name = oldmodp->name() + "__02E" + name;
                            }

                            AstCell* ifacecellp = नूतन AstCell{newmodp->fileline(),
                                                              newmodp->fileline(),
                                                              name,
                                                              ifacerefp->ifaceName(),
                                                              शून्यसूचकः,
                                                              शून्यसूचकः,
                                                              शून्यसूचकः};
                            ifacecellp->modp(ifacerefp->ifacep());
                            newmodp->सङ्कलय_Stmtsp(ifacecellp);

                            AstIfaceRefDType* ध्रुव idtypep = नूतन AstIfaceRefDType{
                                newmodp->fileline(), name, ifacerefp->ifaceName()};
                            idtypep->ifacep(शून्यसूचकः);
                            idtypep->dtypep(idtypep);
                            idtypep->cellp(ifacecellp);
                            rootp->typeTablep()->सङ्कलय_Typesp(idtypep);

                            AstVar* varp = नूतन AstVar{newmodp->fileline(), VVarType::IFACEREF,
                                                      name + "__Viftop", idtypep};
                            varp->isIfaceParent(सत्य);
                            ifacecellp->सङ्कलय_अग्र_अत्र(varp);
                            ifacecellp->hasIfaceVar(सत्य);

                            AstPin* ध्रुव pinp
                                = नूतन AstPin{oldvarp->fileline(), 0, varp->name(),
                                             नूतन AstVarRef{varp->fileline(), varp,
                                                           oldvarp->isWritable() ? VAccess::WRITE
                                                                                 : VAccess::READ}};
                            pinp->modVarp(oldvarp);
                            cellp->सङ्कलय_Pinsp(pinp);
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
    यदि (!rootp->modulesp()) {  // Later V3LinkDot will warn
        UINFO(1, "No module found");
        प्रत्यागम;
    }

    // For all top modules
    प्रति (Ast_ग्रन्थिः_Module* modp = rootp->modulesp(); modp && modp->is_शिखरम्();
         modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
        यदि (VN_IS(modp, Package)) अनुवर्त;

        UINFO(5, "LOOP " << modp);

        प्रति (Ast_ग्रन्थिः* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(subnodep, Var)) {
                UINFO(8, "VARWRAP " << varp);
                यदि (varp->isIO()) {
                    varp->protect(असत्य);
                    varp->sig_लौकिकम्(सत्य);
                    varp->primaryIO(सत्य);
                    यदि (varp->isRef() || varp->isConstRef()) {
                        varp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: ref/const ref as primary input/output: "
                                         << varp->prettyNameQ());
                    }
                    यदि (v3Global.opt.systemC()) varp->sc(सत्य);
                    यदि (v3Global.opt.no_Trace_शिखरम्()) varp->trace(असत्य);
                }
            }
        }
    }
}
