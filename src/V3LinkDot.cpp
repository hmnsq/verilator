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
// LinkDot TRANSFORMATIONS:
//      Top-down traversal in LinkDotFindVisitor
//          Cells:
//              Make graph of cell hierarchy
//          Var/Funcs's:
//              Collect all names into symtable under appropriate cell
//      Top-down traversal in LinkDotScopeVisitor
//          Find VarScope versions of signals (well past original link)
//      Top-down traversal in LinkDotParamVisitor
//          Create implicit signals
//      Top-down traversal in LinkDotResolveVisitor
//          VarXRef/Func's:
//              Find appropriate named cell and link to var they reference
//*************************************************************************
// Interfaces:
//      CELL (.port (ifref)
//                     ^--- cell                 -> IfaceDTypeRef(iface)
//                     ^--- cell.modport         -> IfaceDTypeRef(iface,modport)
//                     ^--- varref(input_ifref)  -> IfaceDTypeRef(iface)
//                     ^--- varref(input_ifref).modport -> IfaceDTypeRef(iface,modport)
//      FindVisitor:
//          #1: Insert interface Vars
//          #2: Insert ModPort names
//        IfaceVisitor:
//          #3: Update ModPortVarRef to point at interface vars (after #1)
//          #4: Create ModPortVarRef symbol table entries
//        FindVisitor-insertIfaceRefs()
//          #5: Resolve IfaceRefDtype modport names (after #2)
//          #7: Record sym of IfaceRefDType and aliased interface and/or modport (after #4,#5)
//      insertAllScopeAliases():
//          #8: Insert modport's symbols under IfaceRefDType (after #7)
//      ResolveVisitor:
//          #9: Resolve general variables, which may point into the interface or modport (after #8)
//      LinkResolve:
//          #10: Unlink modports, not needed later except for JSON/Lint
//*************************************************************************
//سر
//      {name-of-top-modulename}
//      a          (VSymEnt->AstCell)
//        {name-of-cell}
//        {name-of-cell-module}
//        aa         (VSymEnt->AstCell)
//          var        (AstVar) -- no sub symbol table needed
//          beg        (VSymEnt->AstBegin) -- can see "upper" a's symbol table
//          beg        (VSymEnt->AstCellInline) -- once V3Begin flattened it, prefix "beg__DOT__"
//      b          (VSymEnt->AstCell)
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkDot.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"
#ضمن "V3LinkDotIfaceCapture.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Parse.h"
#ضمن "V3Randomize.h"
#ضمن "V3Stats.h"
#ضمن "V3String.h"
#ضمن "V3SymTable.h"

#ضمن <algorithm>
#ضمن <cstdlib>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ساكن string ایستل_Dotted_لار(Ast_غوټه* nodep, منطقي& hasPartSelect) {
    إذا (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) {
        أرجع refp->name();
    } وإلا إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
        أرجع refp->name();
    } وإلا إذا (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
        ثابت string lhs = ایستل_Dotted_لار(dotp->lhsp(), hasPartSelect);
        ثابت string rhs = ایستل_Dotted_لار(dotp->rhsp(), hasPartSelect);
        أرجع VString::dot(lhs, ".", rhs);
    } وإلا إذا (VN_IS(nodep, Sel_بټ) || VN_IS(nodep, Sel_ایستل) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = ریښتیا;
        أرجع "";
    }
    أرجع "";
}
ساكن string lexical_Disable_لار(Ast_غوټه* ثابت targetp) {
    std::ویکټور<string> نومونه;
    لكل (Ast_غوټه* curp = targetp; curp; curp = curp->aboveLoopp()) {
        إذا (Ast_غوټه_بلاک* ثابت blockp = VN_CAST(curp, غوټه_بلاک)) {
            إذا (blockp->name() != "") نومونه.ټېل_شا(blockp->name());
        } وإلا إذا (Ast_غوټه_FTask* ثابت ftaskp = VN_CAST(curp, غوټه_FTask)) {
            نومونه.ټېل_شا(ftaskp->name());
        } وإلا إذا (VN_IS(curp, غوټه_Module)) {
            اكسر;
        }
    }
    string لار;
    لكل (خپلکاری it = نومونه.crbegin(); it != نومونه.crend(); ++it) {
        لار = VString::dot(لار, ".", *it);
    }
    أرجع لار;
}
ساكن string موخه_Instance_لار(Ast_غوټه* ثابت targetp, ثابت string& موخه_لار) {
    ثابت string lexical_لار = lexical_Disable_لار(targetp);
    إذا (lexical_لار == "" || موخه_لار == lexical_لار) أرجع "";
    ثابت string suffix = "." + lexical_لار;
    إذا (!VString::پایونه_With(موخه_لار, suffix)) أرجع "";
    أرجع موخه_لار.substr(0, موخه_لار.اندازه() - suffix.اندازه());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

صنف Link_غوټه_Matcher_Class_Or_Package final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override {
        أرجع VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
صنف Link_غوټه_Matcher_FTask final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override { أرجع VN_IS(nodep, غوټه_FTask); }
};
صنف Link_غوټه_Matcher_Modport final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override { أرجع VN_IS(nodep, Modport); }
};
صنف Link_غوټه_Matcher_Prod final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override { أرجع VN_IS(nodep, RSProd); }
};
صنف Link_غوټه_Matcher_Var final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override {
        أرجع VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
صنف Link_غوټه_Matcher_Var_IO final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        إذا (!varp) أرجع خطأ;
        أرجع varp->isIO();
    }
};
صنف Link_غوټه_Matcher_Var_Param final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        إذا (!varp) أرجع خطأ;
        أرجع varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

صنف LinkDotState final {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1p()         // VSymEnt*.      Last symbol created for this node
    //  AstNodeModule::user2()          // bool.          Currently processing for recursion check
    //  ...  Note maybe more than one, as can be multiple hierarchy places
    //  AstVarScope::user2p()           // AstVarScope*.  Base alias for AstInline of this signal
    //  AstAssignW::user2()             // bool.          Created for aliases handling.
    //  ... Don't replace var refs if set
    //  AstVar::user2p()                // AstFTask*.     If a function variable, the task
    //                                                    that links to the variable
    //  AstVar::user4()                 // bool.          True if port set for this variable
    //  AstNodeBlock::user4()           // bool.          Did name processing
    //  AstNodeModule::user4()          // bool.          Live module
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    ثابت VNUser4_دننه_Use m_inuser4;

عام:
    // ENUMS
    // In order of priority, compute first ... compute last
    تعداد SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

خاص:
    // TYPES
    کارول ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    کارول IfaceModSyms = std::ویکټور<std::pair<AstIface*, VSymEnt*>>;

    ساكن LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSym_ګراف m_syms;  // Symbol table by hierarchy
    VSym_ګراف m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = تش_اشاره;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::سېټ<std::pair<Ast_غوټه_Module*, std::string>>
        m_implicit_Name_سېټ;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::ویکټور<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ثابت VLink_Dot_ګام m_ګام;  // Operational step

عام:
    // METHODS

    فراغ dumpSelf(ثابت string& nameComment, منطقي force = خطأ) {
        إذا (debug() >= 6 || dumpLevel() >= 6 || force) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            إذا (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            منطقي لومړی = ریښتیا;
            لكل (صحيح samn = 0; samn < SAMN__MAX; ++samn) {
                إذا (!m_scopeAliasMap[samn].تشه()) {
                    إذا (لومړی) os << "\nScopeAliasMap:\n";
                    لومړی = خطأ;
                    لكل (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                         it != m_scopeAliasMap[samn].پای_ټکی(); ++it) {
                        // left side is what we will import into
                        os << "\t" << samn << "\t" << it->لومړی << " ("
                           << it->لومړی->nodep()->typeName() << ") <- " << it->دوهم << " "
                           << it->دوهم->nodep() << '\n';
                    }
                }
            }
        }
    }
    ساكن فراغ preErrorDumpHandler() {
        إذا (s_errorThisp) s_errorThisp->preErrorDump();
    }
    فراغ preErrorDump() {
        ساكن منطقي s_diddump = خطأ;
        إذا (!s_diddump && dump_ونه_Level()) {
            s_diddump = ریښتیا;
            dumpSelf("linkdot-preerr", ریښتیا);
            v3Global.rootp()->dump_ونه_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLink_Dot_ګام ګام)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_ګام{ګام} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = هذا;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        read_Mod_نومونه();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(تش_اشاره);
        s_errorThisp = تش_اشاره;
    }

    // ACCESSORS
    VSym_ګراف* symsp() { أرجع &m_syms; }
    صحيح ګام_شمېره() ثابت { أرجع static_cast<صحيح>(m_ګام); }
    منطقي forPrimary() ثابت { أرجع m_ګام == LDS_PRIMARY; }
    منطقي forParamed() ثابت { أرجع m_ګام == LDS_PARAMED; }
    منطقي forScopeCreation() ثابت { أرجع m_ګام == LDS_SCOPED; }

    // METHODS
    ساكن string غوټه_متن_Type(Ast_غوټه* nodep) {
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            إذا (varp->isIO() || varp->isIfaceRef()) {
                أرجع "port";
            } وإلا إذا (varp->isGParam()) {
                أرجع "parameter";
            } وإلا إذا (varp->varType() == VVarType::LPARAM) {
                أرجع "local parameter";
            } وإلا {
                أرجع "variable";
            }
        } وإلا إذا (ثابت AstParamTypeDType* ثابت dtypep = VN_CAST(nodep, ParamTypeDType)) {
            إذا (dtypep->isGParam()) {
                أرجع "type parameter";
            } وإلا {
                أرجع "local type parameter";
            }
        } وإلا إذا (VN_IS(nodep, Begin)) {
            أرجع "block";
        } وإلا إذا (VN_IS(nodep, Cell)) {
            أرجع "instance";
        } وإلا إذا (VN_IS(nodep, Constraint)) {
            أرجع "constraint";
        } وإلا إذا (VN_IS(nodep, Func)) {
            أرجع "function";
        } وإلا إذا (VN_IS(nodep, Gen_بلاک)) {
            أرجع "generate block";
        } وإلا إذا (VN_IS(nodep, Iface)) {
            أرجع "interface";
        } وإلا إذا (VN_IS(nodep, RSProd)) {
            أرجع "randsequence production";
        } وإلا إذا (VN_IS(nodep, Task)) {
            أرجع "task";
        } وإلا {
            أرجع nodep->prettyTypeName();
        }
    }

    Ast_غوټه_Module* پیدا_کړه_Module_Sym(ثابت string& modName) {
        ثابت VSymEnt* ثابت foundp = m_mods.rootp()->پیدا_کړه_Id_Fallback(modName);
        أرجع foundp ? VN_AS(foundp->nodep(), غوټه_Module) : تش_اشاره;
    }
    فراغ read_Mod_نومونه() {
        // Look at all modules, and store pointers to all module names
        لكل (Ast_غوټه_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), غوټه_Module);
            m_mods.rootp()->ورننباسه(nodep->name(), جديد VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* ریښه_Entp() ثابت { أرجع m_syms.rootp(); }
    VSymEnt* dunitEntp() ثابت { أرجع m_dunitEntp; }
    فراغ checkDuplicate(VSymEnt* lookupSymp, Ast_غوټه* nodep, ثابت string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ثابت VSymEnt* ثابت foundp = lookupSymp->پیدا_کړه_Id_Flat(name);
        Ast_غوټه* ثابت fnodep = foundp ? foundp->nodep() : تش_اشاره;
        إذا (!fnodep) {
            // Not found, will add in a moment.
            إذا (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ثابت VSymEnt* ثابت alt = lookupSymp->پیدا_کړه_Similar_Id_Flat(name);
                إذا (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } وإلا إذا (nodep == fnodep) {  // Already inserted.
            // Good.
        } وإلا إذا (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } وإلا إذا (forPrimary() && VN_IS(nodep, Gen_بلاک)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, Gen_بلاک))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } وإلا {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ثابت string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_غوټه::prettyNameQ(name);
            إذا (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << غوټه_متن_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_عدد_سېټ_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_عدد_سېټ_Min(0);  // Disable checking forward typedefs
            } وإلا {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(غوټه_متن_Type(nodep)) << " has the same name as "
                               << غوټه_متن_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    فراغ ورننباسه_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvt_منزل_Hex(symp));
        symp->parentp(ریښه_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(تش_اشاره);
        ریښه_Entp()->ورننباسه("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* ورننباسه_سر_Cell(Ast_غوټه_Module* nodep, ثابت string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvt_منزل_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ریښه_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(ریښه_Entp(), nodep, nodep->origName());
        ریښه_Entp()->ورننباسه(nodep->origName(), symp);
        إذا (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        أرجع symp;
    }
    VSymEnt* ورننباسه_سر_Iface(AstCell* nodep, ثابت string& scopename) {
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvt_منزل_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ریښه_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        إذا (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(ریښه_Entp(), nodep, nodep->origName());
        ریښه_Entp()->ورننباسه(nodep->origName(), symp);
        إذا (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        أرجع symp;
    }
    VSymEnt* ورننباسه_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ثابت string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvt_منزل_Hex(symp) << "  " << scopename << " above=se"
                                      << cvt_منزل_Hex(abovep) << " mods=se" << cvt_منزل_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        إذا (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        إذا (forScopeCreation() && abovep != modSymp && !modSymp->پیدا_کړه_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        إذا (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        أرجع symp;
    }
    فراغ ورننباسه_Map(VSymEnt* symp, ثابت string& scopename) {
        إذا (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* ورننباسه_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ثابت string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvt_منزل_Hex(symp) << "  " << basename << " above=se"
                                      << cvt_منزل_Hex(abovep) << " mods=se" << cvt_منزل_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->sym_مختاړی(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        إذا (abovep != modSymp && !modSymp->پیدا_کړه_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        أرجع symp;
    }
    VSymEnt* ورننباسه_بلاک(VSymEnt* abovep, ثابت string& name, Ast_غوټه* nodep,
                         Ast_غوټه_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvt_منزل_Hex(symp) << "  above=se" << cvt_منزل_Hex(abovep)
                                      << " pkg=" << cvt_منزل_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        إذا (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        أرجع symp;
    }
    VSymEnt* ورننباسه_Sym(VSymEnt* abovep, ثابت string& name, Ast_غوټه* nodep,
                       Ast_غوټه_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTsym se" << cvt_منزل_Hex(symp) << "  name='" << name << "' above=se"
                                      << cvt_منزل_Hex(abovep) << " pkg=" << cvt_منزل_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        // We don't remember the ent associated with each node, because we
        // need a unique scope entry for each instantiation
        symp->classOrPackagep(classOrPackagep);
        symp->parentp(abovep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, name);
        abovep->reinsert(name, symp);
        أرجع symp;
    }
    ساكن منطقي exists_غوټه_Sym(Ast_غوټه* nodep) { أرجع nodep->user1p() != تش_اشاره; }
    ساكن VSymEnt* get_غوټه_Sym(Ast_غوټه* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ثابت symp = nodep->user1u().منزل_Sym_Ent();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        أرجع symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ثابت خپلکاری it = m_nameScopeSymMap.پیدا_کړه(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.پای_ټکی(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        أرجع it->دوهم;
    }
    فراغ implicit_Ok_جمع_کړه(Ast_غوټه_Module* nodep, ثابت string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        إذا (nodep) {
            ثابت خپلکاری it = m_implicit_Name_سېټ.پیدا_کړه(std::make_pair(nodep, varname));
            إذا (it == m_implicit_Name_سېټ.پای_ټکی()) m_implicit_Name_سېټ.emplace(nodep, varname);
        }
    }
    منطقي implicitOk(Ast_غوټه_Module* nodep, ثابت string& varname) {
        أرجع nodep
               && (m_implicit_Name_سېټ.پیدا_کړه(std::make_pair(nodep, varname))
                   != m_implicit_Name_سېټ.پای_ټکی());
    }

    // Track and later recurse interface modules
    فراغ ورننباسه_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_شا(nodep, symp);
    }
    فراغ computeIfaceModSyms();

    // Track and later insert interface references
    فراغ ورننباسه_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ټېل_شا(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ساكن AstIfaceRefDType* ifaceRefFromArray(Ast_غوټه_DType* nodep) {
        طالما (nodep) {
            إذا (AstIfaceRefDType* ثابت ifp = VN_CAST(nodep, IfaceRefDType)) أرجع ifp;
            إذا (ثابت AstBracketArrayDType* ثابت arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } وإلا إذا (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } وإلا {
                أرجع تش_اشاره;
            }
        }
        أرجع تش_اشاره;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* ژوندی_Iface_From_Pin_بیان(Ast_غوټه* exprp, VSymEnt* پلار_Symp) {
        AstVar* resolvedVarp = تش_اشاره;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } وإلا إذا (AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ثابت lookupSymp = پلار_Symp ? پلار_Symp->parentp() : تش_اشاره;
            إذا (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = تش_اشاره;
                ثابت string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ثابت dotSymp
                    = پیدا_کړه_Dotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, ریښتیا);
                إذا (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = تش_اشاره;
        إذا (resolvedVarp) {
            AstIfaceRefDType* ثابت irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            إذا (irefp && irefp->iface_له_لارې_Cellp() && !irefp->iface_له_لارې_Cellp()->dead()) {
                resultp = irefp->iface_له_لارې_Cellp();
            }
        }
        أرجع resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    منطقي repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, منطقي isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* پلار_Symp = varSymp->parentp();
        AstCell* پلار_Cellp = تش_اشاره;
        لكل (صحيح ژوروالی = 0; پلار_Symp && ژوروالی < 20; ++ژوروالی) {
            إذا (AstCell* ثابت cp = VN_CAST(پلار_Symp->nodep(), Cell)) {
                پلار_Cellp = cp;
                اكسر;
            }
            پلار_Symp = پلار_Symp->parentp();
        }
        إذا (!پلار_Cellp) أرجع خطأ;
        // Scan pins to find the one connected to this port variable
        لكل (AstPin* pinp = پلار_Cellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->modVarp() != varp && pinp->name() != varp->name()) استمر;
            Ast_غوټه* ثابت exprp = pinp->exprp();
            إذا (!exprp) أرجع خطأ;
            AstIface* ثابت newIfacep = ژوندی_Iface_From_Pin_بیان(exprp, پلار_Symp);
            إذا (newIfacep && newIfacep != ifacerefp->iface_له_لارې_Cellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                أرجع ریښتیا;
            }
            إذا (newIfacep) أرجع !isDead;  // Same interface - OK if live
            أرجع خطأ;
        }
        أرجع خطأ;
    }

    فراغ computeIfaceVarSyms() {
        لكل (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ثابت varp = varSymp ? VN_AS(varSymp->nodep(), Var) : تش_اشاره;
            UINFO(9, "  insAllIface se" << cvt_منزل_Hex(varSymp) << " " << varp);
            AstIfaceRefDType* ثابت ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            إذا (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_عدد() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warn_نور()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            إذا (!ifacerefp->iface_له_لارې_Cellp()) {
                إذا (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_غوټه::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_غوټه::prettyNameQ(ifacerefp->ifaceName()));
                    استمر;
                } وإلا {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } وإلا إذا (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->iface_له_لارې_Cellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ثابت منطقي isDead = ifacerefp->iface_له_لارې_Cellp()->dead();
                إذا (isDead && forPrimary() && !varp->is_Iface_پلار()
                    && !v3Global.opt.سر_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ثابت منطقي repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                إذا (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_غوټه::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warn_نور()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warn_نور()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    استمر;
                }
            } وإلا إذا (ifacerefp->iface_له_لارې_Cellp()->dead()
                       || !exists_غوټه_Sym(ifacerefp->iface_له_لارې_Cellp())) {
                ifacerefp->iface_له_لارې_Cellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ثابت ifaceSymp = get_غوټه_Sym(ifacerefp->iface_له_لارې_Cellp());
            VSymEnt* if_Or_پورټ_Symp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            إذا (ifacerefp->isModport()) {
                VSymEnt* ثابت foundp = ifaceSymp->پیدا_کړه_Id_Fallback(ifacerefp->modportName());
                منطقي ok = خطأ;
                إذا (foundp) {
                    إذا (AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        if_Or_پورټ_Symp = foundp;
                        ok = ریښتیا;
                    }
                }
                إذا (!ok) {
                    ثابت string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_غوټه_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.تشه() ? "" : ifacerefp->warn_نور() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            ورننباسه_Scope_Alias(SAMN_IFTOP, varSymp, if_Or_پورټ_Symp);
        }
        m_ifaceVarSyms.clear();
    }

    فراغ ورننباسه_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvt_منزل_Hex(lhsp) << " se" << cvt_منزل_Hex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    فراغ computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        لكل (صحيح samn = 0; samn < SAMN__MAX; ++samn) {
            لكل (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].پای_ټکی(); ++it) {
                VSymEnt* ثابت lhsp = it->لومړی;
                VSymEnt* srcp = lhsp;
                طالما (ریښتیا) {  // Follow chain of aliases up to highest level non-alias
                    ثابت خپلکاری it2 = m_scopeAliasMap[samn].پیدا_کړه(srcp);
                    إذا (it2 != m_scopeAliasMap[samn].پای_ټکی()) {
                        srcp = it2->دوهم;
                        استمر;
                    } وإلا {
                        اكسر;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                إذا (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), ریښتیا);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

خاص:
    VSymEnt* پیدا_کړه_With_Alt_Fallback(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->پیدا_کړه_Id_Fallback(name);
        إذا (findp) أرجع findp;
        إذا (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->پیدا_کړه_Id_Fallback(altname);
        }
        أرجع findp;
    }

    VSymEnt* پیدا_کړه_With_Alt_Flat(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->پیدا_کړه_Id_Flat(name);
        إذا (findp) أرجع findp;
        إذا (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->پیدا_کړه_Id_Flat(altname);
        }
        أرجع findp;
    }

عام:
    VSymEnt* پیدا_کړه_Dotted(File_کرښه* refLocationp, VSymEnt* lookupSymp, ثابت string& dotname,
                        string& baddot, VSymEnt*& okSymp, منطقي لومړی_Id) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvt_منزل_Hex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        طالما (leftname != "") {  // foreach dotted part of xref name
            string::اندازه_type pos = leftname.پیدا_کړه('.');
            string ident;
            إذا (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } وإلا {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            إذا (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                طالما (VString::پایونه_With(altIdent, "__KET__")) {
                    ثابت خپلکاری braPos = altIdent.rfind("__BRA__");
                    إذا (braPos == string::npos) اكسر;
                    altIdent = altIdent.substr(0, braPos);
                }
                إذا (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            إذا (لومړی_Id) {
                // Check this module - subcellnames
                ثابت AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : تش_اشاره;  // Replicated below
                إذا (VSymEnt* ثابت پیدا_کړه_Symp = پیدا_کړه_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = پیدا_کړه_Symp;
                }

                // Check this module - cur modname
                وإلا إذا (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                وإلا إذا (ident == "$root") {
                    lookupSymp = ریښه_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    إذا (forScopeCreation()) {
                        lookupSymp = lookupSymp->پیدا_کړه_Id_Flat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                وإلا {
                    منطقي crossedCell = خطأ;  // Crossed a cell boundary
                    طالما (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : تش_اشاره;  // Replicated above
                        إذا (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            إذا (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = ریښتیا;
                            }
                            إذا (cellp && cellp->modp()->origName() == ident) {
                                اكسر;
                            } وإلا إذا (VSymEnt* ثابت پیدا_کړه_Sym2p
                                       = پیدا_کړه_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = پیدا_کړه_Sym2p;
                                إذا (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    أرجع تش_اشاره;
                                }
                                اكسر;
                            }
                        } وإلا {
                            اكسر;
                        }
                    }
                    إذا (!lookupSymp) أرجع تش_اشاره;  // Not found
                }
            } وإلا {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* پیدا_کړه_Symp = پیدا_کړه_With_Alt_Flat(lookupSymp, ident, altIdent);
                إذا (پیدا_کړه_Symp) {
                    lookupSymp = پیدا_کړه_Symp;
                } وإلا {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    پیدا_کړه_Symp = پیدا_کړه_Sym_Prefixed(lookupSymp, ident, baddot, /*fallback=*/ریښتیا);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (پیدا_کړه_Symp ? "found" : "not found") << "\n");
                    إذا (پیدا_کړه_Symp) {
                        // Check if this is an interface reference variable
                        ثابت AstVar* varp = VN_CAST(پیدا_کړه_Symp->nodep(), Var);
                        إذا (!varp) {
                            إذا (ثابت AstVarScope* vscp = VN_CAST(پیدا_کړه_Symp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        إذا (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = پیدا_کړه_Symp;
                        } وإلا {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            إذا (VN_IS(پیدا_کړه_Symp->nodep(), Cell)) {
                                lookupSymp = پیدا_کړه_Symp;
                            } وإلا {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                أرجع تش_اشاره;
                            }
                        }
                    } وإلا {
                        أرجع تش_اشاره;  // Not found
                    }
                }
            }
            إذا (lookupSymp) {
                إذا (ثابت AstCell* ثابت cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    إذا (ثابت Ast_غوټه_Module* ثابت modp = cellp->modp()) {
                        إذا (modp->hier_بلاک() && !leftname.تشه()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } وإلا إذا (VN_IS(modp, Not_وموندل_Module)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                إذا (!leftname.تشه()) {
                    ثابت خپلکاری aliasIt = m_scopeAliasMap[SAMN_IFTOP].پیدا_کړه(lookupSymp);
                    إذا (aliasIt != m_scopeAliasMap[SAMN_IFTOP].پای_ټکی()) {
                        lookupSymp = aliasIt->دوهم;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        إذا (ثابت AstVarScope* ثابت vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ثابت string varName = vscp->varp()->name();
                            ساكن constexpr ثابت حرف* ثابت VIFTOP_SUFFIX = "__Viftop";
                            إذا (VString::پایونه_With(varName, VIFTOP_SUFFIX)) {
                                ثابت string cellName
                                    = varName.substr(0, varName.اندازه() - مزي_اوږدوالی(VIFTOP_SUFFIX));
                                VSymEnt* ثابت پلار_Symp = lookupSymp->parentp();
                                إذا (پلار_Symp) {
                                    VSymEnt* ثابت cellSymp = پلار_Symp->پیدا_کړه_Id_Flat(cellName);
                                    إذا (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } وإلا {
                        // No alias; try following IfaceRefDType to interface cell
                        ثابت AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        إذا (!varp) {
                            إذا (ثابت AstVarScope* ثابت vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        إذا (varp && varp->isIfaceRef()) {
                            إذا (ثابت AstIfaceRefDType* ثابت ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                إذا (ifaceRefp->cellp() && exists_غوټه_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_غوټه_Sym(ifaceRefp->cellp());
                                } وإلا إذا (ifaceRefp->iface_له_لارې_Cellp()
                                           && exists_غوټه_Sym(ifaceRefp->iface_له_لارې_Cellp())) {
                                    lookupSymp = get_غوټه_Sym(ifaceRefp->iface_له_لارې_Cellp());
                                } وإلا إذا (ifaceRefp->ifacep()
                                           && exists_غوټه_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_غوټه_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            لومړی_Id = خطأ;
        }
        أرجع lookupSymp;
    }

    ساكن string remove_وروستی_Inline_Scope(ثابت string& name) {
        string بهر = name;
        ثابت string dot = "__DOT__";
        ثابت اندازه_t dotPos = بهر.rfind(dot, بهر.اندازه() - dot.اوږدتیا() - 1);
        إذا (dotPos == string::npos) {
            أرجع "";
        } وإلا {
            أرجع بهر.erase(dotPos + dot.اوږدتیا(), string::npos);
        }
    }

    VSymEnt* پیدا_کړه_Sym_Prefixed(VSymEnt* lookupSymp, ثابت string& dotname, string& baddot,
                             منطقي fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        إذا (!lookupSymp) أرجع تش_اشاره;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvt_منزل_Hex(lookupSymp)
                     << ((lookupSymp->sym_مختاړی() == "") ? "" : " as ")
                     << ((lookupSymp->sym_مختاړی() == "") ? "" : lookupSymp->sym_مختاړی() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string مختاړی = lookupSymp->sym_مختاړی();
        VSymEnt* foundp = تش_اشاره;
        طالما (!foundp) {
            إذا (fallback) {
                foundp = lookupSymp->پیدا_کړه_Id_Fallback(مختاړی + dotname);  // Might be nullptr
            } وإلا {
                foundp = lookupSymp->پیدا_کړه_Id_Flat(مختاړی + dotname);  // Might be nullptr
            }
            إذا (مختاړی.تشه()) اكسر;
            ثابت string بل_مختاړی = remove_وروستی_Inline_Scope(مختاړی);
            إذا (مختاړی == بل_مختاړی) اكسر;
            مختاړی = بل_مختاړی;
        }
        إذا (!foundp) baddot = dotname;
        أرجع foundp;
    }
    ساكن منطقي checkIfClassOrPackage(ثابت VSymEnt* ثابت symp) {
        إذا (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) أرجع ریښتیا;

        // Helper: check if a RefDType might resolve to a class later.
        ثابت خپلکاری isPotentialClassRef = [](ثابت AstRefDType* refp) -> منطقي {
            أرجع refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ثابت خپلکاری is_Valid_Type_غوټه = [](ثابت Ast_غوټه* nodep) -> منطقي {
            أرجع VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        إذا (ثابت AstTypedef* ثابت typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ثابت Ast_غوټه_DType* dtypep = typedefp->subDTypep();
            إذا (!dtypep) dtypep = typedefp->ماشوم_DTypep();
            إذا (VN_IS(dtypep, ClassRefDType)) أرجع ریښتیا;
            إذا (isPotentialClassRef(VN_CAST(dtypep, RefDType))) أرجع ریښتیا;
        } وإلا إذا (ثابت AstParamTypeDType* ثابت paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_غوټه* childp = paramTypep->ماشوم_DTypep();
            إذا (ثابت AstRequireDType* ثابت reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ثابت Ast_غوټه* ثابت checkp = childp ? childp : paramTypep->skipRefp();
            إذا (is_Valid_Type_غوټه(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                أرجع ریښتیا;
            }
        }
        أرجع خطأ;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, منطقي fallback,
                                   منطقي classOnly, ثابت string& forWhat,
                                   منطقي deferIfUnresolved = خطأ) {
        إذا (nodep->classOrPackageSkipp()) أرجع get_غوټه_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* پلټنه_Symp = lookSymp;
        إذا (VL_UNCOVERABLE(پلټنه_Symp && VN_IS(پلټنه_Symp->nodep(), ParamTypeDType))) {
            پلټنه_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        إذا (fallback) {
            VSymEnt* currentLookSymp = پلټنه_Symp;
            افعل {
                foundp = currentLookSymp->پیدا_کړه_Id_Flat(nodep->name());
                إذا (foundp && !checkIfClassOrPackage(foundp)) foundp = تش_اشاره;
                إذا (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } طالما (!foundp && currentLookSymp);
        } وإلا {
            foundp = پلټنه_Symp->پیدا_کړه_Id_Flat(nodep->name());
            إذا (foundp && !checkIfClassOrPackage(foundp)) foundp = تش_اشاره;
        }
        إذا (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_غوټه_Sym(v3Global.rootp()->stdPackagep())->پیدا_کړه_Id_Flat(nodep->name());
        }
        إذا (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            أرجع foundp;
        }
        إذا (deferIfUnresolved) أرجع تش_اشاره;
        ثابت string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_غوټه_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.تشه() ? "" : nodep->warn_نور() + suggest));
        أرجع تش_اشاره;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ثابت string& name,
                              ثابت VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        إذا (!lookupSymp) أرجع "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        أرجع speller.غوره_Candidate_پیغام(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ثابت string& name, ثابت VNodeMatcher& matcher,
                          منطقي includeType) {
        إذا (!lookupSymp) أرجع "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        أرجع speller.غوره_Candidate_پیغام(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = تش_اشاره;

//======================================================================

صنف Link_Dot_پیدا_کړه_Visitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3_دننه_Use m_inuser3;

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_غوټه_Module* m_classOrPackagep = تش_اشاره;  // Current package
    AstClocking* m_clockingp = تش_اشاره;  // Current clocking block
    std::ویکټور<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = تش_اشاره;  // Symbol Entry for current module
    VSymEnt* m_curSymp = تش_اشاره;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ثابت Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    منطقي m_دننه_Recursion = خطأ;  // Inside a recursive module
    صحيح m_param_عدد = 0;  // Parameter number, for position based connection
    منطقي m_explicitNew = خطأ;  // Hit a "new" function
    صحيح m_mod_بلاک_عدد = 0;  // Begin block number in module, 0=none seen
    صحيح m_mod_With_عدد = 0;  // With block number, 0=none seen
    صحيح m_mod_Arg_عدد = 0;  // Arg block number for randomize(), 0=none seen
    std::ویکټور<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    فراغ makeImplicitNew(AstClass* nodep) {
        AstFunc* ثابت newp = جديد AstFunc{nodep->fileline(), "new", تش_اشاره, تش_اشاره};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(ریښتیا);
        nodep->جمع_کړه_Membersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->ورننباسه_بلاک(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ساكن منطقي is_Hier_بلاک_Wrapper(ثابت string& name) {
        ثابت V3_Hier_بلاک_Opt_سېټ& hier_بلاکونه = v3Global.opt.hier_بلاکونه();
        أرجع hier_بلاکونه.پیدا_کړه(name) != hier_بلاکونه.پای_ټکی();
    }

    فراغ moveExternFuncDecl(Ast_غوټه_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        إذا (nodep->isExternDef()) أرجع;
        // Move it to proper spot under the target class
        nodep->unlink_Fr_شا();
        classp->جمع_کړه_Stmtsp(nodep);
        nodep->isExternDef(ریښتیا);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlink_Fr_شا()->delete_ونه();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        إذا (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    فراغ moveExternFuncDeclRefs(Ast_غوټه* nodep, AstClass* classp) {
        nodep->foreach([هذا, classp](AstClassOrPackageRef* refp) {  //
            إذا (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ثابت dotp = VN_CAST(refp->backp(), Dot);
                إذا (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlink_Fr_شا());
                    VL_DO_DANGLING2(ټېل_Deletep(dotp), dotp, refp);
                } وإلا {
                    VL_DO_DANGLING(ټېل_Deletep(refp->unlink_Fr_شا()), refp);
                }
            }
        });
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->ورننباسه_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_غوټه_Module* ثابت modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        إذا (m_statep->forPrimary()) {
            ثابت منطقي onlyBuiltins = [&]() {
                لكل (Ast_غوټه_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), غوټه_Module)) {
                    إذا (!modp->isDollarUnit() && !modp->isConstPool()) أرجع خطأ;
                }
                أرجع ریښتیا;
            }();
            إذا (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        لكل (Ast_غوټه_Module* modp = nodep->modulesp(); modp && modp->is_سر();
             modp = VN_AS(modp->nextp(), غوټه_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            إذا (!m_statep->forScopeCreation() && v3Global.opt.سر_Ifaces_Supported()) {
                لكل (Ast_غوټه* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    إذا (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                        إذا (varp->isIfaceRef()) {
                            ثابت Ast_غوټه_DType* ثابت subtypep = varp->subDTypep();
                            ثابت AstIfaceRefDType* ifacerefp = تش_اشاره;
                            إذا (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } وإلا إذا (VN_IS(subtypep, BracketArrayDType)) {
                                ثابت AstBracketArrayDType* ثابت arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ثابت Ast_غوټه_DType* ثابت arrsubtypep = arrp->subDTypep();
                                إذا (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } وإلا إذا (VN_IS(subtypep, UnpackArrayDType)) {
                                ثابت AstUnpackArrayDType* ثابت arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ثابت Ast_غوټه_DType* ثابت arrsubtypep = arrp->subDTypep();
                                إذا (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            إذا (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = جديد AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  تش_اشاره,
                                                  تش_اشاره,
                                                  تش_اشاره};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->ورننباسه_سر_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ټېل_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->ورننباسه_سر_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = تش_اشاره;
        }
    }
    فراغ لیدنه(Ast_Type_جدول*) override {}  // FindVisitor::
    فراغ لیدنه(AstIfaceRefDType* nodep) override {  // FindVisitor::
        إذا (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ټېل_شا(nodep->ifacep());
            nodep->ifacep()->user3(ریښتیا);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ثابت منطقي standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ثابت منطقي doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_عدد);
        VL_RESTORER(m_mod_بلاک_عدد);
        VL_RESTORER(m_mod_With_عدد);
        VL_RESTORER(m_mod_Arg_عدد);
        إذا (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_غوټه::prettyNameQ(nodep->origName()));
        } وإلا إذا (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ثابت pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            إذا (standalonePkg) {
                إذا (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } وإلا {
                    VSymEnt* ثابت پاسنی_Symp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->ورننباسه_بلاک(پاسنی_Symp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_عدد = 0;
            m_mod_بلاک_عدد = 0;
            m_mod_With_عدد = 0;
            m_mod_Arg_عدد = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(ریښتیا);
            iterateChildren(nodep);
            nodep->user2(خطأ);
            nodep->user4(ریښتیا);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            إذا (!m_statep->forScopeCreation()) {
                إذا (AstIface* ثابت ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->ورننباسه_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } وإلا إذا (is_Hier_بلاک_Wrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ثابت پاسنی_Symp = m_curSymp ? m_curSymp : m_statep->ریښه_Entp();
            m_curSymp = m_modSymp
                = m_statep->ورننباسه_بلاک(پاسنی_Symp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(ریښتیا);
        } وإلا {  // !doit
            إذا (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                لكل (ثابت Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (ثابت AstTypedef* ثابت tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ثابت پاسنی_Symp = m_curSymp ? m_curSymp : m_statep->ریښه_Entp();
                        m_curSymp = m_modSymp = m_statep->ورننباسه_بلاک(پاسنی_Symp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } وإلا {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    فراغ لیدنه(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_عدد);
        VL_RESTORER(m_mod_بلاک_عدد);
        VL_RESTORER(m_mod_With_عدد);
        VL_RESTORER(m_mod_Arg_عدد);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ثابت پاسنی_Symp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->ورننباسه_بلاک(پاسنی_Symp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->ورننباسه_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_عدد = 0;
            m_mod_بلاک_عدد = 0;
            m_mod_With_عدد = 0;
            m_mod_Arg_عدد = 0;
            m_explicitNew = خطأ;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(ریښتیا);
            // Implicit new needed?
            إذا (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        إذا (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        إذا (!nodep->is_سر()) {
            ++m_statScopeVisits;
            nodep->unlink_Fr_شا();
            m_scopes.ټېل_شا(nodep);
        }
    }
    فراغ لیدنه(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        إذا (nodep->recursive() && m_دننه_Recursion) أرجع;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_عدد);
        VL_RESTORER(m_دننه_Recursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ثابت string origname = Ast_غوټه::dedotName(nodep->name());
        string::اندازه_type pos = origname.rfind('.');
        إذا (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ثابت string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->پیدا_کړه_Dotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, خطأ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_غوټه::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->ورننباسه_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_دننه_Recursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            إذا (nodep->modp()) iterate(nodep->modp());
        }
    }
    فراغ لیدنه(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ثابت std::string::اندازه_type pos = ident.rfind("__DOT__");
        إذا (pos != std::string::npos) {
            ثابت std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::مزي_اوږدوالی("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->پیدا_کړه_Dotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             خطأ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_غوټه::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->ورننباسه_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    فراغ لیدنه(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        إذا (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            لكل (Ast_غوټه* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                إذا (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ثابت std::string ګام_Str = m_statep->forPrimary()
                                                    ? ""
                                                    : std::منزل_string(m_statep->ګام_شمېره()) + "_";
                    افعل {
                        ++m_mod_بلاک_عدد;
                        name = "unnamedblk" + ګام_Str + cvt_منزل_Str(m_mod_بلاک_عدد);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } طالما (m_curSymp->پیدا_کړه_Id_Flat(name));
                    nodep->name(name);
                    اكسر;
                }
            }
        }
        إذا (nodep->name() == "") {
            iterateChildren(nodep);
        } وإلا {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ورننباسه_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_بلاک* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        إذا (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ثابت خپلکاری لري_Decl = [](ثابت Ast_غوټه* nodesp) -> منطقي {
                لكل (ثابت Ast_غوټه* np = nodesp; np; np = np->nextp()) {
                    إذا (VN_IS(np, Var) || VN_IS(np, Foreach)) أرجع ریښتیا;
                }
                أرجع خطأ;
            };
            منطقي needName = nodep->declsp() || لري_Decl(nodep->stmtsp());
            إذا (ثابت AstFork* ثابت forkp = VN_CAST(nodep, Fork)) {
                needName = needName || لري_Decl(forkp->forksp());
            }
            إذا (needName) {
                ثابت std::string ګام_Str
                    = m_statep->forPrimary() ? "" : std::منزل_string(m_statep->ګام_شمېره()) + "_";
                std::string name;
                افعل {
                    ++m_mod_بلاک_عدد;
                    name = "unnamedblk" + ګام_Str + cvt_منزل_Str(m_mod_بلاک_عدد);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } طالما (m_curSymp->پیدا_کړه_Id_Flat(name));
                nodep->name(name);
            }
        }
        إذا (nodep->name() == "") {
            iterateChildren(nodep);
        } وإلا {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ورننباسه_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    فراغ moveIfaceExportBody(Ast_غوټه_FTask* nodep) {
        ثابت string& پورټ_Name = nodep->iface_پورټ_Name();
        UINFO(5, "   moveIfaceExportBody: port=" << پورټ_Name << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ثابت پورټ_Symp = m_modSymp->پیدا_کړه_Id_Fallback(پورټ_Name);
        إذا (!پورټ_Symp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << پورټ_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        AstVar* ثابت portp = VN_CAST(پورټ_Symp->nodep(), Var);
        إذا (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << پورټ_Name);
            أرجع;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = تش_اشاره;
        إذا (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        إذا (!ifaceRefDtp && portp->ماشوم_DTypep()) {
            ifaceRefDtp = VN_CAST(portp->ماشوم_DTypep(), IfaceRefDType);
        }
        إذا (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << پورټ_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        ثابت string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_غوټه_Module* ثابت ifaceModp = m_statep->پیدا_کړه_Module_Sym(ifaceName);
        إذا (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            أرجع;
        }
        // Find the prototype task in the interface
        Ast_غوټه_FTask* protoTaskp = تش_اشاره;
        لكل (Ast_غوټه* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_غوټه_FTask* ثابت ftaskp = VN_CAST(itemp, غوټه_FTask)) {
                إذا (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    اكسر;
                }
            }
        }
        إذا (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_غوټه* ثابت nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isIO()) {
                    stmtp = nextp;
                    استمر;
                }
            }
            // Clone this statement and rewrite port references
            Ast_غوټه* ثابت newStmtp = stmtp->clone_ونه(خطأ);
            rewrite_Iface_پورټ_Refs_Single(newStmtp, پورټ_Name);
            protoTaskp->جمع_کړه_Stmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(خطأ);
        // Delete the out-of-block task from the module
        nodep->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ساكن فراغ rewrite_Iface_پورټ_Refs_Single(Ast_غوټه* nodep, ثابت string& پورټ_Name) {
        إذا (!nodep) أرجع;
        // Check if this is a non-colon Dot with lhs matching portName
        إذا (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
            إذا (!dotp->colon()) {
                ثابت AstParseRef* ثابت lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                إذا (lhsRefp && lhsRefp->name() == پورټ_Name) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_غوټه* ثابت rhsp = dotp->rhsp()->unlink_Fr_شا();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_ونه(), dotp);
                    rewrite_Iface_پورټ_Refs_Single(rhsp, پورټ_Name);
                    أرجع;
                }
            }
        }
        // Recurse into all children
        لكل (Ast_غوټه* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            طالما (childp) {
                Ast_غوټه* ثابت nextp = childp->nextp();
                rewrite_Iface_پورټ_Refs_Single(childp, پورټ_Name);
                childp = nextp;
            }
        }
    }

    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        إذا (nodep->name() == "new") m_explicitNew = ریښتیا;
        // Handle out-of-block interface method definition (IEEE 25.8)
        إذا (!nodep->iface_پورټ_Name().تشه() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            أرجع;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* پورته_Symp = m_curSymp;

        إذا (VN_IS(m_curSymp->nodep(), Class)) {
            إذا (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->خالص_Virtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            إذا (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        إذا (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ثابت dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            إذا (dotp) {
                AstClassOrPackageRef* ثابت lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, ریښتیا, خطأ,
                                                "External definition :: reference");
                AstClass* ثابت lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                إذا (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وإلا {
                    m_curSymp = m_statep->get_غوټه_Sym(lhsclassp);
                    پورته_Symp = m_curSymp;
                    AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, ریښتیا, خطأ,
                                                    "External definition :: reference");
                    AstClass* ثابت rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    إذا (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } وإلا {
                        m_curSymp = m_statep->get_غوټه_Sym(rhsclassp);
                        پورته_Symp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } وإلا إذا (cpackagerefp) {
                إذا (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ریښتیا, خطأ,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                إذا (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وإلا {
                    m_curSymp = m_statep->get_غوټه_Sym(classp);
                    پورته_Symp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } وإلا {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        إذا (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(پورته_Symp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        إذا (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_غوټه_DType* dtypep = VN_CAST(nodep->fvarp(), غوټه_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            إذا (dtypep) {
                dtypep->unlink_Fr_شا();
            } وإلا {
                dtypep = جديد AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ثابت newvarp
                = جديد AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlag_ماشوم_DType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(ریښتیا);
            newvarp->trace(خطأ);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->ورننباسه_Sym(m_curSymp, newvarp->name(), newvarp, تش_اشاره /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterate_And_بل_Null(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = تش_اشاره;
        إذا (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->ورننباسه_بلاک(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(تش_اشاره);
        }
        إذا (!nodep->name().تشه()) {
            itSymp = m_statep->ورننباسه_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(تش_اشاره);
        }
        إذا (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterate_And_بل_Null(nodep->itemsp());
        }
    }
    فراغ لیدنه(Ast_Clocking_توکی* nodep) override {  // FindVisitor::
        إذا (nodep->varp()) {
            إذا (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            أرجع;
        }
        std::string varname;
        Ast_غوټه_DType* dtypep;
        إذا (Ast_ټاکل* ثابت assignp = nodep->assignp()) {
            Ast_غوټه_بیان* ثابت rhsp = assignp->rhsp()->unlink_Fr_شا();
            dtypep = جديد AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_د_بیان{},
                                     rhsp->clone_ونه(خطأ)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlink_Fr_شا()->delete_ونه(), assignp);
        } وإلا {
            Ast_غوټه_بیان* ثابت refp = nodep->exprp();
            ثابت VSymEnt* foundp = m_curSymp->پیدا_کړه_Id_Fallback(refp->name());
            إذا (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
                أرجع;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->ماشوم_DTypep()->clone_ونه(خطأ);
        }
        AstVar* ثابت newvarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlag_ماشوم_DType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    فراغ لیدنه(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* پورته_Symp = m_curSymp;
        إذا (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            إذا (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } وإلا {
                إذا (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ریښتیا, خطأ,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                إذا (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وإلا {
                    m_curSymp = m_statep->get_غوټه_Sym(classp);
                    پورته_Symp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlink_Fr_شا();
                    classp->جمع_کړه_Stmtsp(nodep);
                    nodep->classOrPackagep()->unlink_Fr_شا()->delete_ونه();
                }
            }
        }
        // Set the class as package for iteration
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ورننباسه_بلاک(پورته_Symp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        إذا (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        إذا (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ثابت VSymEnt* foundp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
            إذا (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : تش_اشاره;
            // clocking items can have duplicate names (inout)
            إذا (findvarp && VN_IS(findvarp->backp(), Clocking_توکی)
                && VN_IS(nodep->backp(), Clocking_توکی)) {
                Ast_Clocking_توکی* ثابت itemp = VN_AS(nodep->backp(), Clocking_توکی);
                Ast_Clocking_توکی* ثابت finditemp = VN_AS(findvarp->backp(), Clocking_توکی);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                إذا (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::ننوت
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = تش_اشاره;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            منطقي ins = خطأ;
            إذا (!foundp) {
                ins = ریښتیا;
            } وإلا إذا (!findvarp && m_curSymp->پیدا_کړه_Id_Flat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::غوټه_متن_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } وإلا إذا (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvt_منزل_Hex(m_curSymp) << " ;; parent=se"
                                             << cvt_منزل_Hex(foundp->parentp()));
                إذا (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    إذا (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        أرجع;
                    }
                    ثابت منطقي nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ثابت منطقي ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    إذا (ansiBad || nansiBad) {
                        منطقي ansiWarn = ansiBad && !nansiBad;
                        إذا (ansiWarn) {
                            ساكن صحيح s_didAnsiWarn = خطأ;
                            إذا (s_didAnsiWarn++) ansiWarn = خطأ;
                        }
                        nodep->v3error("Duplicate declaration of signal: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (ansiWarn ? nodep->warn_نور()
                                                          + "... note: ANSI ports must have"
                                                            " type declared with the I/O"
                                                            " (IEEE 1800-2023 23.2.2.2)\n"
                                                    : "")
                                       << nodep->warnContextPrimary() << '\n'
                                       << findvarp->warnOther()
                                       << "... Location of original declaration\n"
                                       << findvarp->warnContextSecondary());
                        // Combining most likely reduce other errors
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                    } وإلا {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(ریښتیا);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        إذا (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{ریښتیا});
                        }
                        Ast_غوټه_DType* ثابت varDtp = findvarp->subDTypep();
                        Ast_غوټه_DType* ثابت otherDtp = nodep->subDTypep();
                        إذا (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlink_Fr_شا()->delete_ونه(), varDtp);
                            findvarp->ماشوم_DTypep(otherDtp->unlink_Fr_شا());
                        } وإلا إذا (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            Ast_Attr_د* ثابت newp
                                = جديد Ast_Attr_د{otherDtp->fileline(), VAttrType::VAR_پورټ_DTYPE,
                                                otherDtp->unlink_Fr_شا()};
                            findvarp->جمع_کړه_Attrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
                } وإلا {
                    // User can disable the message at either point
                    إذا (!(m_ftaskp && m_ftaskp->dpiImport())
                        && (!m_ftaskp || m_ftaskp != foundp->nodep())  // Not the function's
                                                                       // variable hiding function
                        && !nodep->fileline()->warnIsOff(V3_Error_کوډ::VARHIDDEN)
                        && !foundp->nodep()->fileline()->warnIsOff(V3_Error_کوډ::VARHIDDEN)) {
                        nodep->v3warn(VARHIDDEN,
                                      "Declaration of signal hides declaration in upper scope: "
                                          << nodep->prettyNameQ() << '\n'
                                          << nodep->warnContextPrimary() << '\n'
                                          << foundp->nodep()->warnOther()
                                          << "... Location of original declaration\n"
                                          << foundp->nodep()->warnContextSecondary());
                    }
                    ins = ریښتیا;
                }
            }
            إذا (ins) {
                إذا (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->ریښه_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    إذا (v3Global.opt.hasParameter(nodep->name())) {
                        ثابت string svalue = v3Global.opt.parameter(nodep->name());
                        إذا (AstConst* ثابت valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            إذا (nodep->valuep()) ټېل_Deletep(nodep->valuep()->unlink_Fr_شا());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3_Error_کوډ::ENUMVALUE, ریښتیا);
                        }
                    }
                }
                m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                إذا (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_عدد;
                    VSymEnt* ثابت symp
                        = m_statep->ورننباسه_Sym(m_curSymp, "__paramNumber" + cvt_منزل_Str(m_param_عدد),
                                              nodep, m_classOrPackagep);
                    symp->exported(خطأ);
                }
            }
        }
    }
    فراغ لیدنه(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        إذا (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(ریښتیا);
        iterateChildren(nodep);
        m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    فراغ لیدنه(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, "__Vdtype" + cvt_منزل_Str(nodep->unique_عدد()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    فراغ لیدنه(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        إذا (!m_hierParamsName.تشه()) {
            إذا (ثابت VSymEnt* ثابت typedefEntp = m_curSymp->پیدا_کړه_Id_Fallback(m_hierParamsName)) {
                ثابت AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                لكل (ثابت Ast_غوټه* stmtp = modp ? modp->stmtsp() : تش_اشاره; stmtp;
                     stmtp = stmtp->nextp()) {
                    ثابت AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    إذا (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_غوټه_DType* ثابت newDtp = tdefp->ماشوم_DTypep();
                        Ast_غوټه_DType* ثابت oldDtp = nodep->ماشوم_DTypep();

                        oldDtp->replaceWith(newDtp->clone_ونه(خطأ));
                        VL_DO_DANGLING(oldDtp->delete_ونه(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        إذا (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_عدد;
            VSymEnt* ثابت symp = m_statep->ورننباسه_Sym(
                m_curSymp, "__paramNumber" + cvt_منزل_Str(m_param_عدد), nodep, m_classOrPackagep);
            symp->exported(خطأ);
        }
    }
    فراغ لیدنه(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    فراغ لیدنه(Ast_Enum_توکی* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ثابت VSymEnt* foundp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
        إذا (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        Ast_Enum_توکی* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), Enum_توکی) : تش_اشاره;
        منطقي ins = خطأ;
        إذا (!foundp) {
            ins = ریښتیا;
        } وإلا إذا (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            إذا (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } وإلا {
                // User can disable the message at either point
                إذا (!nodep->fileline()->warnIsOff(V3_Error_کوډ::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3_Error_کوډ::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = ریښتیا;
            }
        }
        إذا (ins) m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    فراغ لیدنه(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        إذا (!nodep->packagep()) أرجع;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->get_غوټه_Sym(nodep->packagep());
        إذا (nodep->name() == "*") {
            إذا (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                إذا (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } وإلا إذا (!nodep->resolvedClassp()) {
            VSymEnt* ثابت impp = srcp->پیدا_کړه_Id_Flat(nodep->name());
            إذا (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } وإلا إذا (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } وإلا إذا (AstClass* ثابت classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    جديد AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, تش_اشاره});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    فراغ لیدنه(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        إذا (!nodep->packagep()) أرجع;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->get_غوټه_Sym(nodep->packagep());
        إذا (nodep->name() != "*") {
            VSymEnt* ثابت impp = srcp->پیدا_کړه_Id_Flat(nodep->name());
            إذا (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    فراغ لیدنه(Ast_Package_Export_ستوری_ستوری* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->export_ستوری_ستوری(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_عدد;
            m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, "__Vforeach" + cvt_منزل_Str(m_mod_With_عدد),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            لكل (Ast_غوټه *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = تش_اشاره;
                إذا (AstParseRef* ثابت parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = جديد AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->پیدا_کړه_Int_DType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_ونه(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->ورننباسه_Sym(m_curSymp, argrefp->name(), argrefp, تش_اشاره);
                } وإلا إذا (ثابت خپلکاری largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->ورننباسه_Sym(m_curSymp, argrefp->name(), argrefp, تش_اشاره);
                } وإلا إذا (VN_IS(argp, تشه_2)) {
                } وإلا {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    فراغ لیدنه(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        إذا (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        لكل (Ast_غوټه* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(ریښتیا);
        }
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_غوټه_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), غوټه_FTask_Ref);
        إذا (ثابت AstDot* ثابت dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), غوټه_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_کرښه* argFl = nodep->fileline();
        AstArg* ثابت argsp = funcrefp->argsp();
        إذا (argsp) {
            argsp->unlink_Fr_شا_With_بل();
            إذا (funcrefp->name() != "randomize") {
                إذا (ثابت خپلکاری parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } وإلا {
                    argsp->v3error("'with' function expects simple variable name");
                }
                إذا (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_ونه(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        إذا (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->جمع_کړه_Argsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlink_Fr_شا());
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }
        ثابت منطقي restrictedRandomize = nodep->restricted();
        إذا (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ثابت شاخص_Arg_Refp
                = جديد AstLambdaArgRef{argFl, name + "__DOT__index", ریښتیا};
            AstLambdaArgRef* ثابت ارزښت_Arg_Refp = جديد AstLambdaArgRef{argFl, name, خطأ};
            AstWith* ثابت newp
                = جديد AstWith{nodep->fileline(), شاخص_Arg_Refp, ارزښت_Arg_Refp, تش_اشاره};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            إذا (restrictedRandomize) {
                newp->restricted(ریښتیا);
                طالما (Ast_غوټه* ثابت itemp = nodep->exprsp()) {
                    newp->جمع_کړه_Restricted_Name(VN_AS(itemp, ParseRef)->name());
                    itemp->unlink_Fr_شا();
                    VL_DO_DANGLING(ټېل_Deletep(itemp), itemp);
                }
            }
            Ast_غوټه* بیان_Or_Constraintsp = تش_اشاره;
            إذا (nodep->exprsp()) بیان_Or_Constraintsp = nodep->exprsp()->unlink_Fr_شا_With_بل();
            إذا (nodep->constraintsp())
                بیان_Or_Constraintsp = Ast_غوټه::جمع_کړه_بل(
                    بیان_Or_Constraintsp, nodep->constraintsp()->unlink_Fr_شا_With_بل());
            newp->جمع_کړه_Exprp(بیان_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->جمع_کړه_Argsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlink_Fr_شا());
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_عدد;
        m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, "__Vwith" + cvt_منزل_Str(m_mod_With_عدد), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->شاخص_Arg_Refp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->ارزښت_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->ورننباسه_Sym(m_curSymp, nodep->ارزښت_Arg_Refp()->name(), nodep->ارزښت_Arg_Refp(),
                            تش_اشاره);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterate_And_بل_Null(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_عدد;
        m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, "__VcoverWith" + cvt_منزل_Str(m_mod_With_عدد),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterate_And_بل_Null(nodep->itemp());
        iterate_And_بل_Null(nodep->filterp());
    }
    فراغ لیدنه(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterate_And_بل_Null(nodep->selectp());
        iterate_And_بل_Null(nodep->countp());
        لكل (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterate_And_بل_Null(varp->ماشوم_DTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_عدد;
        m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, "__VcoverWith" + cvt_منزل_Str(m_mod_With_عدد),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        لكل (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->ورننباسه_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterate_And_بل_Null(nodep->filterp());
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    فراغ handleUnvisitedVirtIfaces() {
        Ast_غوټه_Module* ثابت سر = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_غوټه_Sym(سر);
        لكل (AstIface* ifacep : m_virtIfaces) {
            إذا (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ثابت ifacecellp = جديد AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        تش_اشاره,
                                                        تش_اشاره,
                                                        تش_اشاره};
                ifacecellp->modp(ifacep);
                سر->جمع_کړه_Stmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

عام:
    // CONSTRUCTORS
    Link_Dot_پیدا_کړه_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        إذا (!m_virtIfaces.تشه()) handleUnvisitedVirtIfaces();
        لكل (AstScope* ثابت scopep : m_scopes) scopep->modp()->جمع_کړه_Stmtsp(scopep);
        إذا (m_statep->forScopeCreation()) {
            V3Stats::جمع_کړه_Stat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~Link_Dot_پیدا_کړه_Visitor() override = افتراضي;
};

//======================================================================

صنف Link_Dot_پیدا_کړه_Iface_Visitor final : عام VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_غوټه* m_declp = تش_اشاره;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ثابت VSymEnt* پیدا_کړه_Non_Decl_Sym(VSymEnt* symp, ثابت string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        طالما (symp) {
            ثابت VSymEnt* ثابت foundp = symp->پیدا_کړه_Id_Flat(name);
            إذا (foundp && foundp->nodep() != m_declp) أرجع foundp;
            symp = symp->fallbackp();
        }
        أرجع تش_اشاره;
    }

    // VISITORS
    فراغ لیدنه(AstRefDType* nodep) override {  // FindIfaceVisitor::
        إذا (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            إذا (m_statep->exists_غوټه_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_غوټه_Sym(nodep);
                ثابت VSymEnt* foundp = پیدا_کړه_Non_Decl_Sym(symp, nodep->name());
                Ast_غوټه* وموندل_Nodep = تش_اشاره;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                إذا (foundp) وموندل_Nodep = foundp->nodep();
                إذا (!وموندل_Nodep) وموندل_Nodep = m_statep->پیدا_کړه_Module_Sym(nodep->name());
                إذا (وموندل_Nodep) UINFO(9, "    Ifc foundNodep " << وموندل_Nodep);
                إذا (AstIface* ثابت defp = VN_CAST(وموندل_Nodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ثابت newp
                        = جديد AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    إذا (nodep->paramsp())
                        newp->جمع_کړه_Paramsp(nodep->paramsp()->unlink_Fr_شا_With_بل());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ثابت ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        إذا (ifacerefp && m_statep->exists_غوټه_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            إذا (!ifacerefp->isVirtual()) nodep->سېټ_Iface_Ref();
            VSymEnt* ثابت symp = m_statep->get_غوټه_Sym(nodep);
            m_statep->ورننباسه_Iface_Var_Sym(symp);
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

عام:
    // CONSTRUCTORS
    Link_Dot_پیدا_کړه_Iface_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~Link_Dot_پیدا_کړه_Iface_Visitor() override = افتراضي;
};

//======================================================================

صنف LinkDotParamVisitor final : عام VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module

    فراغ pin_Implicit_بیان_Recurse(Ast_غوټه* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        إذا (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } وإلا إذا (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicit_Ok_جمع_کړه(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        وإلا {
            إذا (Ast_غوټه* ثابت refp = nodep->op1p()) pin_Implicit_بیان_Recurse(refp);
            إذا (Ast_غوټه* ثابت refp = nodep->op2p()) pin_Implicit_بیان_Recurse(refp);
            إذا (Ast_غوټه* ثابت refp = nodep->op3p()) pin_Implicit_بیان_Recurse(refp);
            إذا (Ast_غوټه* ثابت refp = nodep->op4p()) pin_Implicit_بیان_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        إذا (Ast_غوټه* ثابت refp = nodep->nextp()) pin_Implicit_بیان_Recurse(refp);
    }

    // VISITORS
    فراغ لیدنه(Ast_Type_جدول*) override {}  // ParamVisitor::
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        إذا ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(ریښتیا);
            أرجع;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        إذا (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_بیان_Recurse(nodep->exprp());
        }
    }
    فراغ لیدنه(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warn_نور()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        منطقي hasPartSelect = خطأ;
        ثابت string لار = ایستل_Dotted_لار(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !لار.تشه(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = تش_اشاره;
        VSymEnt* ثابت foundp = m_statep->پیدا_کړه_Dotted(
            nodep->fileline(), m_statep->get_غوټه_Sym(nodep), لار, baddot, okSymp, ریښتیا);
        AstCell* ثابت cellp = foundp ? VN_AS(foundp->nodep(), Cell) : تش_اشاره;
        إذا (!cellp) {
            nodep->v3error("In defparam, instance " << لار << " never declared");
        } وإلا {
            Ast_غوټه_بیان* ثابت exprp = nodep->rhsp()->unlink_Fr_شا();
            UINFO(9, "Defparam cell " << لار << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ثابت pinp = جديد AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(ریښتیا);
            pinp->param_لار(لار);
            cellp->جمع_کړه_Paramsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_پورټ* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ثابت foundp = m_statep->get_غوټه_Sym(m_modp)->پیدا_کړه_Id_Flat(nodep->name());
        AstVar* ثابت refp = foundp ? VN_CAST(foundp->nodep(), Var) : تش_اشاره;
        إذا (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } وإلا إذا (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } وإلا {
            إذا (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(ریښتیا);
            VSymEnt* ثابت symp = m_statep->ورننباسه_Sym(m_statep->get_غوټه_Sym(m_modp),
                                                      "__pinNumber" + cvt_منزل_Str(nodep->pin_عدد()),
                                                      refp, تش_اشاره /*classOrPackagep*/);
            symp->exported(خطأ);
            refp->pin_عدد(nodep->pin_عدد());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->جمع_کړه_دلته_This_As_بل(refp->unlink_Fr_شا());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_بیان_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_بیان_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        إذا (خپلکاری* ثابت fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ثابت foundp = m_statep->get_غوټه_Sym(fwdp)->پیدا_کړه_Id_Fallback(fwdp->name());
            إذا (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), غوټه_Module));
            } وإلا إذا (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } وإلا {
                إذا (foundp) UINFO(1, "found nodep = " << foundp->nodep());
                nodep->v3error(
                    "Forward typedef used as class/package does not resolve to class/package: "
                    << nodep->prettyNameQ() << '\n'
                    << nodep->warnContextPrimary() << '\n'
                    << (foundp ? nodep->warn_نور() + "... Object with matching name\n"
                                     + foundp->nodep()->warnContextSecondary()
                               : ""));
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_بیان_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ثابت foundp = m_statep->get_غوټه_Sym(nodep)->پیدا_کړه_Id_Fallback(nodep->name());
        إذا (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_عدد_سېټ_Min(nodep->fileline()->token_عدد());
        } وإلا إذا (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

عام:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = افتراضي;
};

//======================================================================

صنف LinkDotScopeVisitor final : عام VNVisitor {

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    ثابت AstScope* m_scopep = تش_اشاره;  // The current scope
    VSymEnt* m_modSymp = تش_اشاره;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::ویکټور<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
عام:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ساكن AstVarScope* getAlias(AstVarScope* ثابت vscp) {
        إذا (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ثابت aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            أرجع aliasp;
        } وإلا {
            أرجع vscp;
        }
    }

خاص:
    // Alias an interface reference to the interface it is connected to
    فراغ سېټ_Iface_Alias(AstVarScope* ثابت vscp, AstVarScope* ثابت aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    فراغ سېټ_Var_Alias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        إذا (ap == bp) أرجع;
        ثابت منطقي aWins = [&]() {
            ثابت AstVar* ثابت aVarp = ap->varp();
            ثابت AstVar* ثابت bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            إذا (bVarp->isPrimaryIO()) أرجع خطأ;
            إذا (aVarp->isPrimaryIO()) أرجع ریښتیا;
            // A forced or public variable must keep its own storage. If both are, either works.
            إذا (aVarp->isForced() != bVarp->isForced()) أرجع aVarp->isForced();
            إذا (aVarp->is_Sig_عامه() != bVarp->is_Sig_عامه()) أرجع aVarp->is_Sig_عامه();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            إذا (aVarp->direction() != VDirection::OUTPUT) أرجع خطأ;
            // Is 'ap' strictly below 'bp' in the hierarchy
            لكل (ثابت AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                إذا (scopep == bp->scopep()) أرجع ریښتیا;
            }
            أرجع خطأ;
        }();
        إذا (aWins) {
            bp->user2p(ap);
        } وإلا {
            ap->user2p(bp);
        }
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ لیدنه(AstScope* nodep) override {  // ScopeVisitor::
        UINFO(8, "  SCOPE " << nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Using the CELL names, we created all hierarchy.  We now need to match this Scope
        // up with the hierarchy created by the CELL names.
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_scopep);
        m_modSymp = m_statep->getScopeSym(nodep);
        // Interfaces need another pass when signals are resolved. Register the scope's
        // entry, as that is the one holding the AstVarScopes.
        إذا (AstIface* ثابت ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->ورننباسه_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVarScope* nodep) override {  // ScopeVisitor::
        إذا (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ثابت varSymp
                = m_statep->ورننباسه_Sym(m_modSymp, nodep->varp()->name(), nodep, تش_اشاره);
            إذا (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ثابت dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                إذا (nodep->varp()->is_Iface_پلار()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ثابت string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->پیدا_کړه_Dotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, خطأ);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvt_منزل_Hex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    إذا (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ثابت mpSymp = cellSymp->پیدا_کړه_Id_Fallback(dtypep->modportName());
                        UASSERT_OBJ(mpSymp, nodep,
                                    "No symbol for interface modport: "
                                        << nodep->prettyNameQ(dtypep->modportName()));
                        cellSymp = mpSymp;
                        UINFO(5, "       Found modport cell: se" << cvt_منزل_Hex(cellSymp) << " "
                                                                 << mpSymp->nodep());
                    }
                    // Interface reference; need to put whole thing into
                    // symtable, but can't clone it now as we may have a later
                    // alias for it.
                    m_statep->ورننباسه_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->ورننباسه_بلاک(m_modSymp, nodep->name(), nodep, تش_اشاره);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->ورننباسه_بلاک(m_modSymp, nodep->name(), nodep, تش_اشاره);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    فراغ لیدنه(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->ورننباسه_بلاک(m_modSymp, nodep->name(), nodep, تش_اشاره);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = تش_اشاره;
        لكل (Ast_غوټه* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = تش_اشاره;
            إذا (ثابت AstVarRef* ثابت refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } وإلا {
                // Reaches into the scope of an instance, look it up by name
                ثابت AstVarXRef* ثابت xrefp = VN_AS(itemp, VarXRef);
                ثابت string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ثابت symp = m_statep->پیدا_کړه_Dotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, خطأ);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            إذا (aliasVscp) {
                سېټ_Var_Alias(aliasVscp, vscp);
            } وإلا {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_شا(nodep, m_modSymp);
    }
    فراغ processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ثابت refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ثابت xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().اندازه()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ثابت string dotted_لار
                = (xrefp && !xrefp->dotted().تشه()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = تش_اشاره;
            string scopename;
            طالما (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.اندازه() ? (inl + dotted_لار + xrefp->name())
                                               : (dotted_لار + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->پیدا_کړه_Dotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, ریښتیا);
                إذا (inl == "") اكسر;
                inl = LinkDotState::remove_وروستی_Inline_Scope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvt_منزل_Hex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            ثابت AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            ثابت string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* ثابت symp = m_statep->پیدا_کړه_Dotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, خطأ);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvt_منزل_Hex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->ورننباسه_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ثابت lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ثابت rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        سېټ_Iface_Alias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    فراغ processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, صحيح> ژوروالی_Map;
        لكل (ثابت خپلکاری& pair : m_deferredAliasScopes) {
            VSymEnt* ثابت symp = pair.دوهم;
            إذا (ژوروالی_Map.پیدا_کړه(symp) == ژوروالی_Map.پای_ټکی()) {
                صحيح ژوروالی = 0;
                لكل (VSymEnt* p = symp; p; p = p->parentp()) ++ژوروالی;
                ژوروالی_Map[symp] = ژوروالی;
            }
        }
        std::stable_ترتیبول(m_deferredAliasScopes.begin(), m_deferredAliasScopes.پای_ټکی(),
                         [&ژوروالی_Map](ثابت std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ثابت std::pair<AstAliasScope*, VSymEnt*>& b) {
                             أرجع ژوروالی_Map.at(a.دوهم) < ژوروالی_Map.at(b.دوهم);
                         });
        // Process in sorted order
        لكل (خپلکاری& pair : m_deferredAliasScopes) { processAliasScope(pair.لومړی, pair.دوهم); }
        m_deferredAliasScopes.clear();
    }
    فراغ لیدنه(Ast_غوټه_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    فراغ لیدنه(AstCell*) override {}  // ScopeVisitor::
    فراغ لیدنه(AstVar*) override {}  // ScopeVisitor::
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

عام:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = افتراضي;
};

//======================================================================

// Iterate an interface to resolve modports
صنف LinkDotIfaceVisitor final : عام VNVisitor {
    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    فراغ لیدنه(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, nodep->name(), nodep, تش_اشاره);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
        إذا (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وإلا إذا (Ast_غوټه_FTask* ثابت ftaskp = VN_CAST(symp->nodep(), غوټه_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ثابت subSymp
                = m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), ftaskp, تش_اشاره /*package*/);
            m_statep->ورننباسه_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } وإلا {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        إذا (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolve_Nested_Interface_لار(File_کرښه* fl, VSymEnt* okSymp, string& baddot) {
        إذا (!okSymp || baddot.تشه()) أرجع تش_اشاره;

        ساكن constexpr صحيح MAX_NESTING_ژوروالی = 64;
        VSymEnt* curOkSymp = okSymp;

        لكل (صحيح ژوروالی = 0; ژوروالی < MAX_NESTING_ژوروالی; ++ژوروالی) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = تش_اشاره;
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } وإلا إذا (ثابت AstVar* ثابت varp = VN_CAST(curOkSymp->nodep(), Var)) {
                إذا (varp->isIfaceRef()) {
                    إذا (ثابت AstIfaceRefDType* ثابت ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->iface_له_لارې_Cellp();
                    }
                }
            }

            إذا (!ifacep || !m_statep->exists_غوټه_Sym(ifacep)) أرجع تش_اشاره;

            VSymEnt* ثابت ifaceSymp = m_statep->get_غوټه_Sym(ifacep);

            إذا (baddot.پیدا_کړه('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ثابت symp = ifaceSymp->پیدا_کړه_Id_Fallback(baddot);
                إذا (symp) baddot.clear();
                أرجع symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string پاتې_شوی_Baddot;
            VSymEnt* پاتې_شوی_Ok_Symp = تش_اشاره;
            VSymEnt* ثابت symp = m_statep->پیدا_کړه_Dotted(fl, ifaceSymp, baddot, پاتې_شوی_Baddot,
                                                       پاتې_شوی_Ok_Symp, ریښتیا);
            إذا (symp) {
                baddot = پاتې_شوی_Baddot;
                أرجع symp;
            }

            // findDotted partially matched - check progress
            إذا (پاتې_شوی_Baddot == baddot || !پاتې_شوی_Ok_Symp) أرجع تش_اشاره;

            // Continue resolving with updated state
            baddot = پاتې_شوی_Baddot;
            curOkSymp = پاتې_شوی_Ok_Symp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        أرجع تش_اشاره;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolve_Modport_افاده(File_کرښه* fl, Ast_غوټه_بیان* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = تش_اشاره;
        إذا (AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->پیدا_کړه_Id_Fallback(refp->name());
        } وإلا إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            إذا (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_غوټه_Sym(refp->varScopep());
            } وإلا إذا (refp->varp()) {
                symp = m_curSymp->پیدا_کړه_Id_Fallback(refp->varp()->name());
            }
        } وإلا إذا (AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            إذا (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                إذا (m_statep->exists_غوټه_Sym(refp->varp())) {
                    symp = m_statep->get_غوټه_Sym(refp->varp());
                } وإلا {
                    // Fallback: look up in current scope
                    symp = m_curSymp->پیدا_کړه_Id_Fallback(refp->varp()->name());
                }
            } وإلا إذا (!refp->dotted().تشه()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ثابت string ډک_لار = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = تش_اشاره;
                symp = m_statep->پیدا_کړه_Dotted(fl, m_curSymp, ډک_لار, baddot, okSymp, ریښتیا);
                // Handle nested interface path when findDotted had partial match
                إذا (okSymp && !baddot.تشه()) {
                    VSymEnt* ثابت resolved = resolve_Nested_Interface_لار(fl, okSymp, baddot);
                    إذا (resolved) symp = resolved;
                }
            }
        } وإلا إذا (AstDot* ثابت dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            منطقي hasPartSelect = خطأ;
            ثابت string dotted_لار = ایستل_Dotted_لار(dotp, hasPartSelect);
            إذا (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } وإلا {
                string baddot;
                VSymEnt* okSymp = تش_اشاره;
                symp = m_statep->پیدا_کړه_Dotted(fl, m_curSymp, dotted_لار, baddot, okSymp, ریښتیا);

                // Handle nested interface path when findDotted had partial match
                إذا (okSymp && !baddot.تشه()) {
                    VSymEnt* ثابت resolved = resolve_Nested_Interface_لار(fl, okSymp, baddot);
                    إذا (resolved) symp = resolved;
                }

                إذا (!symp || !baddot.تشه()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_غوټه::prettyNameQ(dotted_لار));
                    symp = تش_اشاره;
                }
            }
        } وإلا إذا (VN_IS(exprp, Sel_بټ) || VN_IS(exprp, Sel_ایستل)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } وإلا {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        أرجع symp;
    }

    فراغ لیدنه(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = تش_اشاره;
        إذا (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolve_Modport_افاده(nodep->fileline(), nodep->exprp());
        } وإلا {
            symp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
        }
        إذا (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وإلا إذا (AstVar* ثابت varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            إذا (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ثابت subSymp
                    = m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), varp, تش_اشاره /*package*/);
                m_statep->ورننباسه_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } وإلا {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), nodep, تش_اشاره /*package*/);
            }
        } وإلا إذا (AstVarScope* ثابت vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            إذا (nodep->exprp()) {
                VSymEnt* ثابت subSymp
                    = m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), vscp, تش_اشاره /*package*/);
                m_statep->ورننباسه_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } وإلا {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        إذا (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
        إذا (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وإلا إذا (AstClocking* ثابت clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->ورننباسه_Sym(m_curSymp, nodep->name(), nodep, تش_اشاره /*package*/);
        } وإلا {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    فراغ لیدنه(AstScope* nodep) override {  // IfaceVisitor::
        إذا (nodep->user3_سېټ_Once()) أرجع;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

عام:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = افتراضي;
};

فراغ LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ثابت VNUser3_دننه_Use user3_دننه_Use;
    لكل (ثابت خپلکاری& itr : m_ifaceModSyms) {
        AstIface* ثابت nodep = itr.لومړی;
        VSymEnt* ثابت symp = itr.دوهم;
        LinkDotIfaceVisitor{nodep, symp, هذا};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

صنف LinkDotResolveVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3_دننه_Use m_inuser3;

    // TYPES
    تعداد DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_لومړی,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ثابت m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = تش_اشاره;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = تش_اشاره;  // SymEnt for current module
    VSymEnt* m_pinSymp = تش_اشاره;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = تش_اشاره;  // SymEnt for randomize target class's lookups
    ثابت AstCell* m_cellp = تش_اشاره;  // Current cell
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    Ast_لاره_Call* m_rand_لاره_Callp = تش_اشاره;  // Current randomize() method call
    صحيح m_modport_عدد = 0;  // Uniqueify modport numbers
    صحيح m_indent = 0;  // Indentation (tree depth) for debug
    منطقي m_دننه_Sens = خطأ;  // True if in senitem
    ثابت AstWith* m_currentWithp = تش_اشاره;  // Enclosing 'with' (nullptr if none)
    std::سېټ<std::string>
        m_restricted_نومونه_کارول_شوی;  // Names from current 'with (id_list)' that resolved into target
    منطقي m_genericIfaceModule = خطأ;  // True if in module containing generic interface
    std::map<std::string, Ast_غوټه*> m_if_Class_Imp_نومونه;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ثابت VSymEnt*> m_ifForwards;
    std::سېټ<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    منطقي m_insideClassExtParam = خطأ;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = تش_اشاره;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_غوټه*, AstPin*> m_کارول_شوی_Pins;  // Pin used in this cell, map to duplicate
    std::map<Ast_غوټه*, std::map<std::string, AstPin*>> m_کارول_شوی_Def_Param_Pins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_غوټه_Module*> m_modules_منزل_Revisit;  // Modules to revisit a second time
    Ast_غوټه* m_وروستی_Deferredp = تش_اشاره;  // Last node which requested a revisit of its module
    Ast_غوټه_DType* m_packedArrayDtp = تش_اشاره;  // Datatype reference for packed array
    منطقي m_دننه_Packed_Array = خطأ;  // Currently traversing a packed array tree
    منطقي m_replaceWithAlias
        = ریښتیا;  // Replace VarScope with an alias. Used in the handling of AstAlias
    منطقي m_isParam = خطأ;  // Specifies whether currently visiting param variable
    منطقي m_resolvingTypedef = خطأ;  // Currently traversing a Typedef tree

    بنية Dot_حالتونه final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ثابت AstDot* m_dotp;  // Current dot
        منطقي m_super;  // Starts with super reference
        منطقي m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        منطقي m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        منطقي m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        منطقي m_genBlk;  // Contains gen block reference
        Ast_غوټه* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        منطقي m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dot_متن;  // String of dotted names found in below parseref
        Dot_حالتونه() { init(تش_اشاره); }
        ~Dot_حالتونه() = افتراضي;
        فراغ init(VSymEnt* curSymp, منطقي unresolvedClass = خطأ) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = تش_اشاره;
            m_super = خطأ;
            m_dotErr = خطأ;
            m_dot_متن = "";
            m_unresolvedCell = خطأ;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = خطأ;
            m_genBlk = خطأ;
            m_unlinkedScopep = تش_اشاره;
            m_disablep = تش_اشاره;
        }
        string ascii() ثابت {
            ساكن ثابت حرف* ثابت نومونه[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << نومونه[m_dotPos];
            sstr << "  dse" << cvt_منزل_Hex(m_dotSymp);
            ثابت string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.تشه() ? "" : ":") << dsname
                 << ")";
            إذا (m_dotErr) sstr << "  [dotErr]";
            إذا (m_super) sstr << "  [super]";
            إذا (m_unresolvedCell) sstr << "  [unrCell]";
            إذا (m_unresolvedClass) sstr << "  [unrClass]";
            إذا (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            إذا (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dot_متن;
            أرجع sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_غوټه_Module* modp,
                              VSymEnt* moduleSymp, منطقي noWarn) {
        // Create implicit after warning
        إذا (!noWarn) {
            إذا (nodep->fileline()->warnIsOff(V3_Error_کوډ::I_DEF_NETTYPE_WIRE)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_غوټه_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.تشه() ? "" : nodep->warn_نور() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            وإلا إذا (!nodep->fileline()->warnIsOff(V3_Error_کوډ::IMPLICIT)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_غوټه_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.تشه() ? "" : nodep->warn_نور() + suggest));
            }
        }
        AstVar* ثابت newp
            = جديد AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->جمع_کړه_Stmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->ورننباسه_Sym(moduleSymp, newp->name(), newp, تش_اشاره /*classOrPackagep*/);
        أرجع newp;
    }
    AstVar* وموندل_منزل_Varp(ثابت VSymEnt* symp, Ast_غوټه* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        إذا (!symp) {
            أرجع تش_اشاره;
        } وإلا إذا (VN_IS(symp->nodep(), Var)) {
            أرجع VN_AS(symp->nodep(), Var);
        } وإلا إذا (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ثابت snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ثابت varp = snodep->varp();
            إذا (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            أرجع varp;
        } وإلا {
            أرجع تش_اشاره;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* پیدا_کړه_Virtual_پورټ_Var_Scope(VSymEnt* dotSymp, ثابت string& پورټ_Name) {
        ثابت AstVarScope* ثابت dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ثابت AstVar* ثابت dotVarp = dotVscp ? dotVscp->varp() : تش_اشاره;
        إذا (!dotVarp) أرجع تش_اشاره;

        Ast_غوټه_DType* dtypep = dotVarp->ماشوم_DTypep();
        إذا (!dtypep) dtypep = dotVarp->subDTypep();
        إذا (!dtypep) أرجع تش_اشاره;
        ثابت AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        إذا (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->iface_له_لارې_Cellp()) أرجع تش_اشاره;

        // Get the interface cell's symbol table
        VSymEnt* ثابت ifaceCellSymp = m_statep->get_غوټه_Sym(ifaceRefp->iface_له_لارې_Cellp());

        // Look up modport by name in interface cell
        VSymEnt* ثابت modportSymp = ifaceCellSymp->پیدا_کړه_Id_Fallback(ifaceRefp->modportp()->name());
        إذا (!modportSymp) أرجع تش_اشاره;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ثابت virtPortp = m_statep->پیدا_کړه_Sym_Prefixed(modportSymp, پورټ_Name, baddot, ریښتیا);
        إذا (!virtPortp) أرجع تش_اشاره;

        // Symbol may point to VarScope or Var
        إذا (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) أرجع vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        إذا (ثابت AstVar* ثابت realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ثابت realFoundp
                = m_statep->پیدا_کړه_Sym_Prefixed(ifaceCellSymp, realVarp->name(), baddot, ریښتیا);
            أرجع realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : تش_اشاره;
        }
        أرجع تش_اشاره;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    فراغ captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        إذا (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) أرجع;
        Ast_غوټه_Module* ثابت defOwnerModp = V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(defp);
        إذا (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) أرجع;
        AstCell* ثابت cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : تش_اشاره;
        إذا (!cellForCapture) أرجع;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::جمع_کړه_Param_Type(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_غوټه_Stmt* جمع_کړه_Implicit_Super_New_Call(AstFunc* ثابت nodep,
                                         ثابت AstClassExtends* ثابت classExtendsp) {
        // Returns the added node
        File_کرښه* ثابت fl = nodep->fileline();
        AstArg* argsp = تش_اشاره;
        إذا (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_ونه(ریښتیا);
        AstNew* ثابت newExprp = جديد AstNew{fl, argsp};
        newExprp->isImplicit(ریښتیا);
        AstDot* ثابت superNewp = جديد AstDot{fl, خطأ, جديد AstParseRef{fl, "super"}, newExprp};
        Ast_غوټه_Stmt* ثابت superNewStmtp = superNewp->makeStmt();
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            إذا (VN_IS(stmtp, غوټه_Stmt)) {
                stmtp->جمع_کړه_دلته_This_As_بل(superNewStmtp);
                أرجع superNewStmtp;
            }
        }
        // There were no statements
        nodep->جمع_کړه_Stmtsp(superNewStmtp);
        أرجع superNewStmtp;
    }
    فراغ checkNoDot(Ast_غوټه* nodep) {
        إذا (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = ریښتیا;
            أرجع;
        }
        إذا (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            إذا (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } وإلا {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = ریښتیا;
        }
    }
    AstVar* پیدا_کړه_Iface_سر_Varp(Ast_غوټه* nodep, VSymEnt* پلار_Entp, ثابت string& name) {
        ثابت string پیدا_کړه_Name = name + "__Viftop";
        VSymEnt* ثابت ifaceSymp = پلار_Entp->پیدا_کړه_Id_Fallback(پیدا_کړه_Name);
        AstVar* ثابت iface_سر_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : تش_اشاره;
        UASSERT_OBJ(iface_سر_Varp, nodep, "Can't find interface var ref: " << پیدا_کړه_Name);
        أرجع iface_سر_Varp;
    }
    AstRSProd* پیدا_کړه_Prod(Ast_غوټه* nodep, VSymEnt* پلار_Entp, ثابت string& name) {
        ثابت VSymEnt* ثابت foundp = پلار_Entp->پیدا_کړه_Id_Fallback(name);
        AstRSProd* وموندل_Nodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : تش_اشاره;
        إذا (!وموندل_Nodep) {
            VSpellCheck speller;
            Link_غوټه_Matcher_Prod matcher;
            پلار_Entp->candidateIdFlat(&speller, &matcher, خطأ);
            ثابت string suggest = speller.غوره_Candidate_پیغام(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvt_منزل_Hex(پلار_Entp));
            nodep->v3error("Production " << Ast_غوټه::prettyNameQ(name) << " not found\n"
                                         << (suggest.تشه() ? "" : nodep->warn_نور() + suggest));
            أرجع تش_اشاره;
        }
        أرجع وموندل_Nodep;
    }
    فراغ duplicatePinError(AstPin* nodep, Ast_غوټه* origp, ثابت حرف* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    فراغ markAndCheckPinDup(AstPin* nodep, Ast_غوټه* refp, ثابت حرف* whatp) {
        ثابت خپلکاری pair = m_کارول_شوی_Pins.emplace(refp, nodep);
        إذا (pair.دوهم) {
            إذا (!nodep->param_لار().تشه())
                m_کارول_شوی_Def_Param_Pins[refp].emplace(nodep->param_لار(), nodep);
            أرجع;
        } وإلا {
            Ast_غوټه* ثابت origp = pair.لومړی->دوهم;
            إذا (nodep->param_لار().تشه() || VN_AS(origp, Pin)->param_لار().تشه()) {
                duplicatePinError(nodep, origp, whatp);
                أرجع;
            }
        }

        خپلکاری& defParamPins = m_کارول_شوی_Def_Param_Pins[refp];
        ثابت خپلکاری defParamIt = defParamPins.پیدا_کړه(nodep->param_لار());
        إذا (defParamIt != defParamPins.پای_ټکی()) {
            duplicatePinError(nodep, defParamIt->دوهم, whatp);
            أرجع;
        }
        defParamPins.emplace(nodep->param_لار(), nodep);
    }
    ساكن منطقي extendsClass(ثابت AstClass* classp, ثابت AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::سېټ<ثابت AstClass*> لیدل_شوی;
        std::ویکټور<ثابت AstClass*> todo{classp};
        طالما (!todo.تشه()) {
            ثابت AstClass* ثابت currp = todo.شا();
            todo.راوباسه_شا();
            إذا (currp == targetp) أرجع ریښتیا;
            إذا (!لیدل_شوی.ورننباسه(currp).دوهم) استمر;
            لكل (ثابت AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                إذا (ثابت AstClass* ثابت basep = cextp->classOrNullp()) todo.ټېل_شا(basep);
            }
        }
        أرجع خطأ;
    }
    ساكن AstClocking* sensClockingp(Ast_غوټه* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        إذا (AstClocking* ثابت clockingp = VN_CAST(nodep, Clocking)) أرجع clockingp;
        إذا (ثابت AstModportClockingRef* ثابت clockingRefp = VN_CAST(nodep, ModportClockingRef))
            أرجع clockingRefp->clockingp();
        أرجع تش_اشاره;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ثابت eventp = clockingp->ensureEventp(ریښتیا);
        إذا (!eventp->user1p()) eventp->user1p(جديد VSymEnt{m_statep->symsp(), eventp});
        أرجع reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* پیدا_کړه_Inlined_Dots_Sym(Ast_غوټه* nodep, ثابت string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ثابت string inl = Ast_غوټه::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = تش_اشاره;
        dotSymp = m_statep->پیدا_کړه_Dotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, خطأ);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_غوټه::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        أرجع dotSymp;
    }
    VSymEnt* پیدا_کړه_Dotted_Ref_Symp(Ast_غوټه* nodep, VSymEnt* lookupSymp, ثابت string& dotted,
                               string& baddot, VSymEnt*& okSymp, منطقي* modportp = تش_اشاره) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        إذا (modportp) *modportp = خطأ;
        VSymEnt* dotSymp = m_statep->پیدا_کړه_Dotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, ریښتیا);  // Maybe nullptr
        إذا (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.تشه() ? Ast_غوټه::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            أرجع تش_اشاره;
        }
        إذا (ثابت AstVar* ثابت varp = VN_CAST(dotSymp->nodep(), Var)) {
            إذا (ثابت AstIfaceRefDType* ثابت ifaceRefp
                = VN_CAST(varp->ماشوم_DTypep(), IfaceRefDType)) {
                إذا (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_غوټه_Sym(ifaceRefp->modportp());
                    إذا (modportp) *modportp = ریښتیا;
                } وإلا {
                    dotSymp = m_statep->get_غوټه_Sym(ifaceRefp->ifacep());
                }
            }
        } وإلا إذا (ثابت AstModportClockingRef* ثابت clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_غوټه_Sym(clockingRefp->clockingp());
        }
        أرجع dotSymp;
    }

    ساكن منطقي isParamedClassRefDType(ثابت Ast_غوټه* classp) {
        طالما (ثابت AstRefDType* ثابت refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        أرجع (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    منطقي isParamedClassRef(ثابت Ast_غوټه* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        إذا (ثابت خپلکاری* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            إذا (classRefp->paramsp()) أرجع ریښتیا;
            ثابت خپلکاری* classp = classRefp->classOrPackageNodep();
            طالما (ثابت خپلکاری* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            أرجع isParamedClassRefDType(classp);
        }
        أرجع isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        طالما (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        أرجع classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    منطقي insideClass(ثابت AstClass* classp) ثابت {
        لكل (ثابت VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            إذا (symp->nodep() == classp) أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    منطقي rand_موخه_Unresolved() ثابت {
        أرجع m_statep->forPrimary() && m_currentWithp && m_rand_لاره_Callp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    منطقي is_Virtual_لاره(ثابت Ast_غوټه_FTask* funcp, ثابت AstClass* classp) ثابت {
        إذا (funcp->isVirtual()) أرجع ریښتیا;
        لكل (ثابت AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ثابت basep = cextp->classOrNullp();
            إذا (!basep) استمر;
            ثابت VSymEnt* ثابت foundp = m_statep->get_غوټه_Sym(basep)->پیدا_کړه_Id_Flat(funcp->name());
            ثابت Ast_غوټه_FTask* ثابت بنسټ_Funcp
                = foundp ? VN_CAST(foundp->nodep(), غوټه_FTask) : تش_اشاره;
            ثابت AstClass* ثابت بنسټ_Classp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : تش_اشاره;
            إذا (بنسټ_Funcp && بنسټ_Classp && is_Virtual_لاره(بنسټ_Funcp, بنسټ_Classp)) أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    منطقي inheritedImplements(ثابت VSymEnt* implp, ثابت VSymEnt* ifSymp, منطقي& needForwardr) {
        needForwardr = خطأ;
        ثابت Ast_غوټه_FTask* ثابت implFuncp = VN_CAST(implp->nodep(), غوټه_FTask);
        إذا (!implFuncp || implFuncp->خالص_Virtual() || implFuncp->isStatic()) أرجع خطأ;
        ثابت AstClass* ثابت implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : تش_اشاره;
        ثابت AstClass* ثابت ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : تش_اشاره;
        إذا (!implClassp || !ifClassp) أرجع خطأ;
        إذا (extendsClass(implClassp, ifClassp)) أرجع ریښتیا;
        إذا (!is_Virtual_لاره(implFuncp, implClassp)) أرجع خطأ;
        needForwardr = ریښتیا;
        أرجع ریښتیا;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    فراغ جمع_کړه_Interface_Forward(AstClass* classp, Ast_غوټه_FTask* implp, AstClass* implClassp) {
        File_کرښه* ثابت flp = implp->fileline();
        AstVar* fvarp = تش_اشاره;
        Ast_غوټه_FTask* newp;
        إذا (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_ونه(خطأ);
            newp = جديد AstFunc{flp, implp->name(), تش_اشاره, fvarp};
        } وإلا {
            newp = جديد AstTask{flp, implp->name(), تش_اشاره};
        }
        newp->class_لاره(ریښتیا);
        newp->isVirtual(ریښتیا);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = تش_اشاره;
        لكل (Ast_غوټه* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            إذا (!portp || !portp->isIO()) استمر;
            AstVar* ثابت newPortp = portp->clone_ونه(خطأ);
            newp->جمع_کړه_Stmtsp(newPortp);
            argsp = Ast_غوټه::جمع_کړه_بل(
                argsp, جديد AstArg{flp, "", جديد AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_غوټه_FTask_Ref* callp;
        إذا (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            callp = جديد AstFuncRef{flp, implFuncp, argsp};
        } وإلا {
            callp = جديد AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(ریښتیا);
        إذا (fvarp) {
            newp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } وإلا {
            newp->جمع_کړه_Stmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->جمع_کړه_Membersp(newp);
    }
    فراغ importDerivedClass(AstClass* derivedClassp, VSymEnt* بنسټ_Symp, AstClass* بنسټ_Classp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << بنسټ_Classp);
        لكل (VSymEnt::const_iterator it = بنسټ_Symp->begin(); it != بنسټ_Symp->پای_ټکی(); ++it) {
            إذا (Ast_غوټه* بنسټ_Subp = it->دوهم->nodep()) {
                UINFO(8, indent() << "  SymFunc " << بنسټ_Subp);
                ثابت string impOrExtends
                    = بنسټ_Classp->isInterfaceClass() ? " implements " : " extends ";
                إذا (VN_IS(بنسټ_Subp, غوټه_FTask)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->پیدا_کړه_Id_Flat(بنسټ_Subp->name());
                    ثابت Ast_غوټه_FTask* ثابت بنسټ_Funcp = VN_CAST(بنسټ_Subp, غوټه_FTask);
                    إذا (!بنسټ_Funcp || !بنسټ_Funcp->خالص_Virtual()) استمر;
                    منطقي needForward = خطأ;
                    ثابت منطقي exists_دننه_Derived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->دوهم, needForward));
                    إذا (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(بنسټ_Subp->name(), foundp);
                    }
                    إذا (m_statep->forPrimary() && !exists_دننه_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << بنسټ_Classp->prettyNameQ()
                                     << " but is missing implementation for "
                                     << بنسټ_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << بنسټ_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << بنسټ_Subp->warnContextSecondary());
                    }
                    ثابت خپلکاری itn = m_if_Class_Imp_نومونه.پیدا_کړه(بنسټ_Subp->name());
                    إذا (m_statep->forPrimary() && !exists_دننه_Derived
                        && itn != m_if_Class_Imp_نومونه.پای_ټکی()
                        && itn->دوهم != بنسټ_Subp) {  // Not exact same function from diamond
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << بنسټ_Classp->prettyNameQ()
                                     << " but missing inheritance conflict resolution for "
                                     << بنسټ_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26.6.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << بنسټ_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << بنسټ_Subp->warnContextSecondary());
                    }
                    m_if_Class_Imp_نومونه.emplace(بنسټ_Subp->name(), بنسټ_Subp);
                }
                إذا (VN_IS(بنسټ_Subp, Constraint)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->پیدا_کړه_Id_Flat(بنسټ_Subp->name());
                    ثابت AstConstraint* ثابت بنسټ_Funcp = VN_CAST(بنسټ_Subp, Constraint);
                    إذا (!بنسټ_Funcp || !بنسټ_Funcp->is_Kwd_خالص()) استمر;
                    ثابت منطقي exists_دننه_Derived = foundp && !foundp->imported();
                    إذا (m_statep->forPrimary() && !exists_دننه_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << بنسټ_Classp->prettyNameQ()
                                     << " but is missing constraint implementation for "
                                     << بنسټ_Subp->prettyNameQ() << " (IEEE 1800-2023 18.5.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << بنسټ_Subp->warnOther()
                                     << "... Location of interface class's pure constraint\n"
                                     << بنسټ_Subp->warnContextSecondary());
                    }
                }
            }
        }
    }
    فراغ importSymbolsFromExtended(AstClass* ثابت nodep, AstClassExtends* ثابت cextp) {
        AstClass* ثابت بنسټ_Classp = cextp->classp();
        VSymEnt* ثابت srcp = m_statep->get_غوټه_Sym(بنسټ_Classp);
        importDerivedClass(nodep, srcp, بنسټ_Classp);
        إذا (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    فراغ classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_غوټه_Sym(nodep));
    }

    فراغ check_Decl_ترتیب(Ast_غوټه* nodep, Ast_غوټه* declp) {
        ثابت uint32_t decl_Token_عدد = declp->decl_Token_عدد();
        إذا (!decl_Token_عدد) أرجع;  // Not implemented/valid on this object
        إذا (nodep->fileline()->token_عدد() < decl_Token_عدد) {
            UINFO(1, "Related node " << nodep->fileline()->token_عدد() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_عدد << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_نور()
                           << "... Suggest move the declaration before the reference, or use a "
                              "forward typedef\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }
    فراغ check_Member_Decl_ترتیب(Ast_غوټه* nodep, AstMemberDType* declp) {
        ثابت uint32_t decl_Token_عدد = declp->fileline()->token_عدد();
        إذا (nodep->fileline()->token_عدد() < decl_Token_عدد) {
            UINFO(1, "Related node " << nodep->fileline()->token_عدد() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_عدد << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_نور()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    فراغ replaceWithCheckBreak(Ast_غوټه* oldp, Ast_غوټه_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        إذا (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(جديد AstConst{newp->fileline(), AstConst::بټ_False_Erroring{}});
        } وإلا {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    فراغ revisitLater(Ast_غوټه* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_وروستی_Deferredp = deferredNodep;
        m_modules_منزل_Revisit.ورننباسه(std::make_pair(m_modp->name(), m_modp));
    }

    فراغ updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        إذا (nodep && nodep->isParam()) nodep->کارول_شوی_Param(ریښتیا);
    }

    ساكن VSymEnt* پیدا_کړه_Id_Fallback_Skip_Member_DType(VSymEnt* lookp, ثابت string& name) {
        VSymEnt* shadowEntp = تش_اشاره;  // Shadowing variable: not a type, kept for error report
        طالما (lookp) {
            VSymEnt* ثابت foundp = lookp->پیدا_کړه_Id_Flat(name);
            إذا (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                إذا (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    أرجع foundp;
                }
                إذا (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        أرجع shadowEntp;
    }

    فراغ symIterateChildren(Ast_غوټه* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    فراغ symIterateNull(Ast_غوټه* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    ساكن ثابت Ast_غوټه_DType* get_بیان_DTypep(ثابت Ast_غوټه_بیان* selp) {
        طالما (ثابت Ast_غوټه_Pre_Sel* ثابت sp = VN_CAST(selp, غوټه_Pre_Sel)) selp = sp->fromp();
        إذا (ثابت AstMemberSel* ثابت sp = VN_CAST(selp, MemberSel)) {
            إذا (ثابت Ast_غوټه_DType* dtypep = get_بیان_DTypep(sp->fromp())) {
                إذا (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ثابت AstClass* ثابت classp = classRefp->classp();
                    ثابت منطقي وموندل = classp->existsMember(
                        [&dtypep, name = selp->name()](ثابت AstClass*, ثابت AstVar* nodep) {
                            dtypep = nodep->ماشوم_DTypep();
                            أرجع nodep->name() == name;
                        });
                    إذا (وموندل) أرجع dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } وإلا {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } وإلا إذا (ثابت Ast_غوټه_Var_Ref* ثابت varRefp = VN_CAST(selp, غوټه_Var_Ref)) {
            أرجع varRefp->varp()->ماشوم_DTypep()->elemDTypep();
        }
        أرجع تش_اشاره;
    }
    ساكن ثابت AstVar* get_بل_Varp(ثابت Ast_غوټه* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        طالما (stmtsp) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtsp, Var)) {
                إذا (varp->isIO()) أرجع varp;
            }
            stmtsp = stmtsp->nextp();
        }
        أرجع تش_اشاره;
    }
    // Introduce implicit parameters for modules with generic interafeces
    فراغ جمع_کړه_Implicit_Parameters_د_Generic_Iface(AstCell* ثابت nodep, ثابت AstModule* ثابت modp) {
        // Get Param number
        صحيح param_عدد = 1;
        لكل (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_عدد;
        }

        // Add each implicit parameter type
        ثابت AstVar* modIfaceVarp = get_بل_Varp(modp->stmtsp());
        لكل (ثابت AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = get_بل_Varp(modIfaceVarp->nextp())) {
            إذا (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->ماشوم_DTypep()->elemDTypep(), IfaceGenericDType)) {
                استمر;
            }
            Ast_غوټه* exprp = pinp->exprp();
            إذا (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                استمر;
            }
            طالما (ثابت Ast_غوټه_Pre_Sel* ثابت preSelp = VN_CAST(exprp, غوټه_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = تش_اشاره;
            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            إذا (varRefp) {
                enclosingVarp = varRefp->varp();
            } وإلا إذا (ثابت AstParseRef* ثابت parseRefp = VN_CAST(exprp, ParseRef)) {
                إذا (m_modp) {
                    إذا (VSymEnt* ثابت symp
                        = m_statep->get_غوټه_Sym(m_modp)->پیدا_کړه_Id_Flat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            إذا (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->ماشوم_DTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ثابت fwdRefp
                    = جديد AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ثابت newPinp = جديد AstPin{
                    pinp->fileline(), param_عدد, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(ریښتیا);
                لیدنه(newPinp);
                nodep->جمع_کړه_Paramsp(newPinp);
            } وإلا إذا (varRefp) {
                ثابت AstVar* ثابت varp = varRefp->varp();
                إذا (ثابت AstIfaceRefDType* ثابت refp
                    = VN_CAST(varp->ماشوم_DTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ثابت ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    إذا (refp->modportp()) {
                        newIfaceRefp = جديد AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } وإلا {
                        newIfaceRefp = جديد AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    إذا (refp->cellp()->paramsp()) {
                        newIfaceRefp->جمع_کړه_Paramsp(refp->cellp()->paramsp()->clone_ونه(ریښتیا));
                    }
                    UASSERT_OBJ(pinp->name().پیدا_کړه("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ثابت newPinp
                        = جديد AstPin{pinp->fileline(), param_عدد,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(ریښتیا);
                    لیدنه(newPinp);
                    nodep->جمع_کړه_Paramsp(newPinp);
                } وإلا {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->ماشوم_DTypep()
                                             ? "of type " + varp->ماشوم_DTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } وإلا {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_عدد;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    منطقي error_Hier_Non_پورټ(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        إذا (varp->isIO()) أرجع خطأ;
        // OK if not an instance
        ثابت AstCell* ثابت cellp = VN_CAST(scopeSymp->nodep(), Cell);
        إذا (!cellp) أرجع خطأ;
        // Ok if not a hier_block
        ثابت Ast_غوټه_Module* ثابت modp = cellp->modp();
        إذا (!modp->hier_بلاک()) أرجع خطأ;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        أرجع ریښتیا;
    }

#تعریف_کړه LINKDOT_لیدنه_پیل() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ثابت {
        string پایله = "";
        پایله.ورننباسه(0, m_indent, ':');
        أرجع پایله + " ";
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ لیدنه(Ast_Type_جدول*) override {}
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        LINKDOT_لیدنه_پیل();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        إذا (ثابت AstModule* ثابت modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_غوټه_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = تش_اشاره;
        m_modp = nodep;
        m_modport_عدد = 0;
        iterateChildren(nodep);
        m_modp = تش_اشاره;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = تش_اشاره;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = خطأ;
    }
    فراغ لیدنه(AstScope* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = تش_اشاره;
    }
    فراغ لیدنه(AstCellInline* nodep) override {
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstCell* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER_CLEAR(m_کارول_شوی_Pins);
        VL_RESTORER_CLEAR(m_کارول_شوی_Def_Param_Pins);
        UASSERT_OBJ(nodep->modp(), nodep,
                    "Instance has unlinked module");  // V3LinkCell should have errored out
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_pinSymp);
        {
            m_cellp = nodep;
            إذا (VN_IS(nodep->modp(), Not_وموندل_Module)) {
                // Prevent warnings about missing pin connects
                إذا (nodep->pinsp()) nodep->pinsp()->unlink_Fr_شا_With_بل()->delete_ونه();
                إذا (nodep->paramsp()) nodep->paramsp()->unlink_Fr_شا_With_بل()->delete_ونه();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            وإلا {
                m_pinSymp = m_statep->get_غوټه_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                إذا (m_statep->forPrimary())
                    إذا (ثابت AstModule* ثابت modp = VN_CAST(nodep->modp(), Module)) {
                        إذا (modp->hasGenericIface())
                            جمع_کړه_Implicit_Parameters_د_Generic_Iface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    فراغ لیدنه(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_کارول_شوی_Pins);
        VL_RESTORER_CLEAR(m_کارول_شوی_Def_Param_Pins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_شمېر(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_غوټه_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        iterateChildren(nodep);
        إذا (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ثابت foundp = m_pinSymp->پیدا_کړه_Id_Flat(nodep->name());
            ثابت حرف* ثابت whatp = nodep->param() ? "parameter" : "pin";
            إذا (!foundp) {
                إذا (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                    أرجع;
                } وإلا إذا (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                    أرجع;
                } وإلا {
                    ثابت std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_غوټه_Matcher_Var_Param{}, خطأ)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_غوټه_Matcher_Var_IO{}, خطأ));
                    ثابت std::string decl
                        = ((m_cellp && m_cellp->modp())
                               ? "\n"s + nodep->warn_نور() + "... Location of instance's "
                                     + m_cellp->modp()->verilogKwd() + " declaration\n"
                                     + m_cellp->modp()->warnContextSecondary()
                               : "");
                    nodep->v3warn(PINNOTFOUND,
                                  ucfirst(whatp)
                                      << " not found: " << nodep->prettyNameQ() << '\n'
                                      << (suggest.تشه() ? "" : nodep->warn_نور() + suggest)
                                      << '\n'
                                      << nodep->warnContextPrimary() << decl);
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                    أرجع;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            منطقي wrongPinType = خطأ;
            إذا (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                إذا (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } وإلا {
                    wrongPinType = ریښتیا;
                }
            } وإلا إذا (AstParamTypeDType* ثابت typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } وإلا {
                wrongPinType = ریښتیا;
            }
            // Don't connect parameter pin to module ports or vice versa
            إذا (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = ریښتیا;
            إذا (wrongPinType) {
                string موخه_Type = LinkDotState::غوټه_متن_Type(foundp->nodep());
                موخه_Type = VString::aOrAn(موخه_Type) + ' ' + موخه_Type;
                إذا (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << موخه_Type);
                } وإلا {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << موخه_Type);
                }
                أرجع;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ساكن منطقي isDotThisReference(ثابت AstDot* nodep) {
        أرجع VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    فراغ لیدنه(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ثابت Dot_حالتونه وروستی_حالتونه = m_ds;
        ثابت منطقي پیل = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ثابت DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            إذا (پیل) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && rand_موخه_Unresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_لومړی;

            إذا (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                إذا (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                إذا (!classSymp && rand_موخه_Unresolved()) {
                    // Bind 'this' in post-V3Param pass
                } وإلا إذا (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = ریښتیا;
                } وإلا {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } وإلا إذا (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ثابت VSymEnt* classSymp = getThisClassSymp();
                إذا (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = ریښتیا;
                } وإلا {
                    ثابت خپلکاری classp = VN_AS(classSymp->nodep(), Class);
                    ثابت خپلکاری cextp = classp->extendsp();
                    إذا (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = ریښتیا;
                    } وإلا {
                        إذا (m_statep->forPrimary()
                            && m_extendsParam.پیدا_کړه(classp) != m_extendsParam.پای_ټکی()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = ریښتیا;
                        } وإلا {
                            ثابت خپلکاری بنسټ_Classp = cextp->classp();
                            UASSERT_OBJ(بنسټ_Classp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_غوټه_Sym(بنسټ_Classp);
                            m_ds.m_super = ریښتیا;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } وإلا إذا (AstClassOrPackageRef* ثابت lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_بل_Null(lhsp);
                إذا (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = وروستی_حالتونه;
                    // Resolve function args before bailing
                    إذا (Ast_غوټه_FTask_Ref* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), غوټه_FTask_Ref)) {
                        iterate_And_بل_Null(ftaskrefp->argsp());
                    }
                    أرجع;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                إذا (AstClassOrPackageRef* ثابت classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    إذا (Ast_غوټه* ثابت classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_غوټه_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } وإلا إذا (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterate_And_بل_Null(nodep->lhsp());
                // nodep->lhsp() may be a new node
                إذا (AstClassOrPackageRef* ثابت crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    إذا (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = وروستی_حالتونه;
                        // Resolve function args before bailing
                        إذا (Ast_غوټه_FTask_Ref* ثابت ftaskrefp
                            = VN_CAST(nodep->rhsp(), غوټه_FTask_Ref)) {
                            iterate_And_بل_Null(ftaskrefp->argsp());
                        }
                        أرجع;
                    }
                }
                إذا (m_وروستی_Deferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = وروستی_حالتونه;
                    // Resolve function args before bailing
                    إذا (Ast_غوټه_FTask_Ref* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), غوټه_FTask_Ref)) {
                        iterate_And_بل_Null(ftaskrefp->argsp());
                    }
                    أرجع;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } وإلا {
                m_ds.m_dotPos = DP_لومړی;
                m_ds.m_unresolvedGenericIface = خطأ;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_بل_Null(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                إذا (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    أرجع;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            إذا (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = ریښتیا;
            }
            إذا (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            إذا (m_ds.m_dotPos == DP_لومړی) m_ds.m_dotPos = DP_SCOPE;
            إذا (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                إذا (پیل && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterate_And_بل_Null(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            إذا (!m_ds.m_unresolvedClass) {
                إذا (پیل) {
                    Ast_غوټه* newp;
                    إذا (m_ds.m_dotErr) {
                        newp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
                    } وإلا {
                        newp = nodep->rhsp()->unlink_Fr_شا();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                } وإلا {  // Dot midpoint
                    Ast_غوټه* newp = nodep->rhsp()->unlink_Fr_شا();
                    إذا (m_ds.m_unresolvedCell) {
                        AstCellRef* ثابت crp = جديد AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlink_Fr_شا(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                }
            }
        }
        إذا (پیل) {
            m_ds = وروستی_حالتونه;
        } وإلا {
            ثابت منطقي unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = وروستی_حالتونه.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    فراغ لیدنه(AstDefaultClocking* nodep) override {
        إذا (VSymEnt* ثابت foundp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name())) {
            إذا (AstClocking* ثابت clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            } وإلا {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } وإلا {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        LINKDOT_لیدنه_پیل();
        VL_RESTORER(m_دننه_Sens);
        m_دننه_Sens = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstPatMember* nodep) override {
        LINKDOT_لیدنه_پیل();
        إذا (nodep->varrefp()) أرجع;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        إذا (Ast_متن* ثابت textp = VN_CAST(nodep->keyp(), متن_2)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            إذا (VSymEnt* ثابت foundp = m_curSymp->پیدا_کړه_Id_Fallback(textp->متن())) {
                إذا (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                    إذا (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            جديد AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : تش_اشاره,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                إذا (Ast_Enum_توکی* ثابت itemp = VN_CAST(foundp->nodep(), Enum_توکی)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        جديد Ast_Enum_توکی_Ref{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstParseRef* nodep) override {
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        إذا (m_ds.m_unresolvedClass) أرجع;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ثابت Dot_حالتونه وروستی_حالتونه = m_ds;
        ثابت منطقي پیل = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        منطقي لومړی = پیل || m_ds.m_dotPos == DP_لومړی;

        إذا (پیل) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        إذا (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = ریښتیا;
        }
        إذا (nodep->name() == "this") {
            iterateChildren(nodep);
            إذا (m_statep->forPrimary()) أرجع;  // The class might be parameterized somewhere
            ثابت VSymEnt* classSymp = getThisClassSymp();
            إذا (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                أرجع;
            }
            AstClass* ثابت classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ثابت dtypep
                = جديد AstClassRefDType{nodep->fileline(), classp, تش_اشاره};
            AstThisRef* ثابت newp = جديد AstThisRef{nodep->fileline(), VFlag_ماشوم_DType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ثابت newp = جديد AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", ریښتیا};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            إذا (m_ds.m_dotErr) {
                nodep->unlink_Fr_شا();  // Avoid circular node loop on errors
            } وإلا {
                Ast_غوټه_بیان* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_شا(), غوټه_بیان);
                Ast_غوټه_بیان* ثابت newp = جديد AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlag_ماشوم_DType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        } وإلا {
            //
            string expectWhat;
            منطقي allowScope = خطأ;
            منطقي allowVar = خطأ;
            منطقي allowFTask = خطأ;
            منطقي staticAccess = خطأ;
            إذا (m_ds.m_disablep) {
                allowScope = ریښتیا;
                allowFTask = ریښتیا;
                expectWhat = "block/task";
            } وإلا إذا (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_غوټه_Module* classOrPackagep = تش_اشاره;
                expectWhat = "scope/variable/func";
                allowScope = ریښتیا;
                allowVar = ریښتیا;
                allowFTask = ریښتیا;
                staticAccess = ریښتیا;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ثابت cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                إذا (cpackagerefp->name() == "local::") {
                    m_randSymp = تش_اشاره;
                    لومړی = ریښتیا;
                } وإلا إذا (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ریښتیا, خطأ, ":: reference");
                    إذا (!foundp) أرجع;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    إذا (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_غوټه_Sym(classOrPackagep);
                    } وإلا {
                        m_ds = وروستی_حالتونه;
                        أرجع;
                    }
                } وإلا {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_غوټه_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } وإلا إذا (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_لومړی) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = ریښتیا;
                allowVar = ریښتیا;
            } وإلا إذا (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = ریښتیا;
            } وإلا إذا (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = ریښتیا;
                allowVar = ریښتیا;
            } وإلا {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = تش_اشاره;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            إذا (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_منزل_موخه(nodep->name()))) {
                foundp = m_randSymp->پیدا_کړه_Id_Flat(nodep->name());
                إذا (foundp) {
                    إذا (m_currentWithp && m_currentWithp->restricted()) {
                        m_restricted_نومونه_کارول_شوی.ورننباسه(nodep->name());
                    }
                    إذا (!پیل) m_ds.m_dotPos = DP_MEMBER;
                    إذا (!m_currentWithp) {
                        UASSERT_OBJ(m_rand_لاره_Callp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_rand_لاره_Callp->fromp(), VarRef), m_rand_لاره_Callp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ثابت newp = جديد AstMemberSel{
                            nodep->fileline(), m_rand_لاره_Callp->fromp()->clone_ونه(خطأ),
                            VFlag_ماشوم_DType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                        أرجع;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ثابت lambdaRefp
                        = جديد AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", خطأ};
                    AstMemberSel* newp = جديد AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlag_ماشوم_DType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            إذا (allowScope) {
                foundp = m_statep->پیدا_کړه_Dotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, لومړی);  // Maybe nullptr
            } وإلا إذا (لومړی) {
                foundp = m_ds.m_dotSymp->پیدا_کړه_Id_Fallback(nodep->name());
            } وإلا {
                foundp = m_ds.m_dotSymp->پیدا_کړه_Id_Flat(nodep->name());
            }
            إذا (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ثابت AstClass* ثابت classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                إذا (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ثابت enclosingClassSymp
                        = m_statep->get_غوټه_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->پیدا_کړه_Id_Fallback(nodep->name());
                    إذا (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            إذا (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ثابت ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ثابت ifaceFoundp = ifaceFallbackp->پیدا_کړه_Id_Flat(nodep->name());
                إذا (ifaceFoundp) {
                    إذا (ثابت AstVar* ثابت varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        إذا (varp->isParam()) foundp = ifaceFoundp;
                    } وإلا إذا (VN_IS(ifaceFoundp->nodep(), Typedef)
                               || VN_IS(ifaceFoundp->nodep(), ParamTypeDType)) {
                        // Redirect dotSymp to the interface cell so that downstream
                        // typedef/ParamTypeDType handlers see the correct context
                        // (ifaceFinalSegmentAllowed checks for Cell->Iface).
                        m_ds.m_dotSymp = ifaceFallbackp;
                        foundp = ifaceFoundp;
                    }
                }
            }
            // When flat lookup in modport fails, provide dotSymp for error diagnostics
            // so the "Known scopes under..." hint appears (restores pre-routing behavior)
            إذا (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            إذا (foundp) {
                UINFO(9, indent() << "found=se" << cvt_منزل_Hex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            منطقي ok = خطأ;
            // Special case: waiting on clocking event
            إذا (m_دننه_Sens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_لومړی) {
                إذا (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            إذا (!foundp) {
            } وإلا إذا (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), غوټه_بلاک)
                       || VN_IS(foundp->nodep(), Gen_بلاک)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                إذا (allowScope) {
                    ok = ریښتیا;
                    m_ds.m_dot_متن = VString::dot(m_ds.m_dot_متن, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    إذا (m_ds.m_disablep && VN_IS(foundp->nodep(), غوټه_بلاک)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    إذا (VN_IS(foundp->nodep(), Gen_بلاک)) {
                        m_ds.m_genBlk = ریښتیا;
                        إذا (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } وإلا إذا (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ثابت cellp = VN_AS(foundp->nodep(), Cell);
                    إذا (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ثابت cellEntp = m_statep->get_غوټه_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ثابت پلار_Entp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ثابت ifaceRefVarp
                            = پیدا_کړه_Iface_سر_Varp(nodep, پلار_Entp, nodep->name());
                        //
                        ok = ریښتیا;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_غوټه_بیان* newp;
                        إذا (m_ds.m_dot_متن != "") {
                            newp = جديد AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dot_متن,
                                                  VAccess::READ};
                            إذا (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = جديد AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_شا()};
                                m_ds.m_unlinkedScopep = تش_اشاره;
                                m_ds.m_unresolvedCell = خطأ;
                                m_ds.m_dot_متن = "";
                            }
                        } وإلا {
                            AstVarRef* ثابت refp
                                = جديد AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    } وإلا إذا (VN_IS(cellp->modp(), Not_وموندل_Module)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } وإلا إذا (allowScope && !allowFTask && VN_IS(foundp->nodep(), غوټه_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = ریښتیا;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } وإلا إذا (allowFTask && VN_IS(foundp->nodep(), غوټه_FTask)) {
                Ast_غوټه_FTask_Ref* taskrefp;
                إذا (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = جديد AstTaskRef{nodep->fileline(), nodep->name()};
                } وإلا {
                    taskrefp = جديد AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                m_ds = وروستی_حالتونه;
                أرجع;
            } وإلا إذا (AstVar* ثابت varp = وموندل_منزل_Varp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                إذا (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = ریښتیا;
                    أرجع;
                }
                AstIfaceRefDType* ثابت ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                إذا (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->ماشوم_DTypep(), IfaceGenericDType) && !پیل) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = ریښتیا;
                    m_ds.m_unresolvedGenericIface = ریښتیا;
                } وإلا إذا (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->iface_له_لارې_Cellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dot_متن << "  " << nodep);
                    m_ds.m_dot_متن = VString::dot(m_ds.m_dot_متن, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    إذا (ifacerefp->modportp() && m_statep->exists_غوټه_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_غوټه_Sym(ifacerefp->modportp());
                    } وإلا {
                        m_ds.m_dotSymp = m_statep->get_غوټه_Sym(ifacerefp->iface_له_لارې_Cellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = ریښتیا;

                    AstVarRef* ثابت refp = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                } وإلا إذا (allowVar) {
                    Ast_غوټه* newp;
                    إذا (m_ds.m_dot_متن != "") {
                        AstVarXRef* ثابت refp
                            = جديد AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dot_متن,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        إذا (ثابت AstModportVarRef* ثابت mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            إذا (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(ریښتیا);
                            }
                        }
                        refp->لري_Gen_بلاک(m_ds.m_genBlk);
                        m_ds.m_dot_متن = "";
                        إذا (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ثابت string dotted = refp->dotted();
                            ثابت اندازه_t pos = dotted.پیدا_کړه("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            إذا (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } وإلا {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = جديد AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_شا()};
                                m_ds.m_unlinkedScopep = تش_اشاره;
                                m_ds.m_unresolvedCell = خطأ;
                            }
                        } وإلا {
                            newp = refp;
                        }
                    } وإلا {
                        إذا (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ثابت refp = جديد AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = ریښتیا;
                }
            } وإلا إذا (ثابت AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dot_متن << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                إذا (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } وإلا إذا (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } وإلا {
                    AstCell* ثابت cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ثابت cellEntp = m_statep->get_غوټه_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ثابت پلار_Entp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ثابت ifaceRefVarp
                        = پیدا_کړه_Iface_سر_Varp(nodep, پلار_Entp, cellp->name());
                    //
                    ok = ریښتیا;
                    m_ds.m_dot_متن = VString::dot(m_ds.m_dot_متن, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ثابت refp
                        = جديد AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_غوټه_بیان* newp = refp;

                    خپلکاری* ثابت cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    إذا (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = جديد Ast_Sel_بټ{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlink_Fr_شا()};
                        newp->user3(ریښتیا);  // Don't process again
                        VL_DO_DANGLING(ټېل_Deletep(cellarrayrefp->unlink_Fr_شا()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = تش_اشاره;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                }
            } وإلا إذا (AstConstraint* ثابت defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_غوټه* ثابت newp = جديد AstConstraintRef{nodep->fileline(), تش_اشاره, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                ok = ریښتیا;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dot_متن = "";
            } وإلا إذا (AstClass* ثابت defp = VN_CAST(foundp->nodep(), Class)) {
                إذا (allowVar) {
                    إذا (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } وإلا {
                        AstRefDType* ثابت newp
                            = جديد AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    }
                    ok = ریښتیا;
                    m_ds.m_dot_متن = "";
                } وإلا {
                    (فراغ)defp;  // Prevent unused variable warning
                }
            } وإلا إذا (AstMemberDType* ثابت defp = VN_CAST(foundp->nodep(), MemberDType)) {
                إذا (allowVar) {
                    check_Member_Decl_ترتیب(nodep, defp);
                    AstRefDType* ثابت refp = جديد AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    ok = ریښتیا;
                    m_ds.m_dot_متن = "";
                }
            } وإلا إذا (Ast_Enum_توکی* ثابت valuep = VN_CAST(foundp->nodep(), Enum_توکی)) {
                إذا (allowVar) {
                    Ast_Enum_توکی_Ref* ثابت refp
                        = جديد Ast_Enum_توکی_Ref{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_غوټه* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dot_متن);
                    refp->لري_Gen_بلاک(m_ds.m_genBlk);
                    إذا (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = جديد AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlink_Fr_شا()};
                        m_ds.m_unlinkedScopep = تش_اشاره;
                        m_ds.m_unresolvedCell = خطأ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    ok = ریښتیا;
                    m_ds.m_dot_متن = "";
                }
            } وإلا إذا (ثابت AstLambdaArgRef* ثابت argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                إذا (allowVar) {
                    Ast_غوټه* ثابت newp
                        = جديد AstLambdaArgRef{nodep->fileline(), argrefp->name(), خطأ};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    ok = ریښتیا;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dot_متن = "";
                }
            } وإلا إذا (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                إذا (m_ds.m_dot_متن != "") m_ds.m_dot_متن += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_لومړی;
            } وإلا إذا (ثابت AstModportClockingRef* ثابت clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_غوټه_Sym(clockingRefp->clockingp());
                إذا (m_ds.m_dot_متن != "") m_ds.m_dot_متن += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_لومړی;
            } وإلا إذا (ثابت Ast_غوټه_FTask* ثابت ftaskp = VN_CAST(foundp->nodep(), غوټه_FTask)) {
                إذا (!ftaskp->isFunction() || ftaskp->class_لاره()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    إذا (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ثابت funcRefp
                            = جديد AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    }
                }
            } وإلا إذا (AstTypedef* ثابت defp = VN_CAST(foundp->nodep(), Typedef)) {
                ثابت منطقي ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                إذا (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dot_متن
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                إذا (ok) {
                    AstRefDType* ثابت refp = جديد AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    إذا (m_ds.m_dotPos == DP_NONE) check_Decl_ترتیب(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<صحيح>(m_ds.m_dotPos), m_ds.m_dot_متن,
                        m_ds.m_dotSymp, m_modp, [هذا]() { أرجع indent(); });

                    إذا (VN_IS(nodep->backp(), Sel_ایستل)) {
                        m_packedArrayDtp = refp;
                    } وإلا {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    }
                }
            } وإلا إذا (AstParamTypeDType* ثابت defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                إذا (ok) {
                    AstRefDType* ثابت refp = جديد AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<صحيح>(m_ds.m_dotPos), m_ds.m_dot_متن,
                        m_ds.m_dotSymp, m_modp, [هذا]() { أرجع indent(); });

                    إذا (VN_IS(nodep->backp(), Sel_ایستل)) {
                        m_packedArrayDtp = refp;
                    } وإلا {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    }
                }
            }
            إذا (!ok) {
                إذا (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = ریښتیا;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } وإلا {
                    // Cells/interfaces can't be implicit
                    ثابت منطقي checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dot_متن == "" && !m_ds.m_disablep && !foundp);
                    ثابت منطقي err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    إذا (err) {
                        إذا (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dot_متن << (m_ds.m_dot_متن == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } وإلا إذا (m_ds.m_dot_متن == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvt_منزل_Hex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ثابت string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.تشه() ? "" : nodep->warn_نور() + suggest));
                        } وإلا {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.تشه() ? Ast_غوټه::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dot_متن + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_غوټه::prettyName(m_ds.m_dot_متن))
                                           : ""));
                        }
                        m_ds.m_dotErr = ریښتیا;
                    }
                    إذا (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ثابت varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ثابت newp
                            = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    }
                }
            }
        }
        إذا (پیل) m_ds = وروستی_حالتونه;
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {
        // Class: Recurse inside or cleanup not founds
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_لیدنه_پیل();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        VL_RESTORER_CLEAR(m_کارول_شوی_Pins);
        VL_RESTORER_CLEAR(m_کارول_شوی_Def_Param_Pins);
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp(), nodep,
                    "class reference parameter not removed by V3Param");
        {
            VL_RESTORER_COPY(m_ds);
            VL_RESTORER(m_pinSymp);

            إذا (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ثابت منطقي deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                خطأ, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            إذا (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_غوټه_Sym(nodep->classOrPackageSkipp());
            } وإلا إذا (nodep->name() != "local::") {
                أرجع;
            }

            AstClass* ثابت refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            إذا (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            إذا (m_statep->forPrimary() && refClassp && !nodep->paramsp()
                && nodep->classOrPackageSkipp()->hasGParam()
                // Don't warn on typedefs, which are hard to know if there's a param somewhere
                // buried
                && VN_IS(nodep->classOrPackageNodep(), Class)
                // References to class:: within class itself, including from its nested
                // classes, are OK per IEEE (UVM does this)
                && !insideClass(refClassp)) {
                nodep->v3warn(
                    NONSTD,
                    "Reference to parameterized class without #() (IEEE 1800-2023 8.25.1)\n"
                        << nodep->warn_نور() << "... Suggest use '"
                        << nodep->classOrPackageNodep()->prettyName() << "#()'");
                // As other simulators do, use the default specialization. Add the same
                // placeholder pin V3LinkCells makes for '#()' so it is linked below.
                AstPin* ثابت pinp = جديد AstPin{nodep->fileline(), 1, "__paramNumber1", تش_اشاره};
                pinp->param(ریښتیا);
                nodep->جمع_کړه_Paramsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        إذا (nodep->name() == "local::") {
            إذا (!m_randSymp && !m_rand_لاره_Callp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = ریښتیا;
                أرجع;
            }
        }
        إذا (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_غوټه_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    فراغ لیدنه(AstConstraint* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        إذا (nodep->isExternDef()) {
            إذا (ثابت VSymEnt* ثابت foundp
                = m_curSymp->پیدا_کړه_Id_Fallback("extern " + nodep->name())) {
                ثابت AstConstraint* ثابت protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } وإلا {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        إذا (nodep->isExternProto()) {
            إذا (!m_curSymp->پیدا_کړه_Id_Fallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_غوټه_Sym(nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstConstraintRef* nodep) override {
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_لیدنه_پیل();
        iterateChildren(nodep);
        إذا (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvt_منزل_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
            إذا (AstVar* ثابت varp
                = foundp ? وموندل_منزل_Varp(foundp, nodep, nodep->access()) : تش_اشاره) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            إذا (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        إذا (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    فراغ لیدنه(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        إذا (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(تش_اشاره);
        } وإلا {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            إذا (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = پیدا_کړه_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            منطقي modport;
            VSymEnt* ثابت dotSymp
                = پیدا_کړه_Dotted_Ref_Symp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            إذا (!dotSymp) أرجع;

            إذا (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = تش_اشاره;
                إذا (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string مختاړی = dotSymp->sym_مختاړی();
                    طالما (!foundp) {
                        foundp = dotSymp->پیدا_کړه_Id_Flat(مختاړی + nodep->name());
                        إذا (foundp) اكسر;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->پیدا_کړه_Id_Flat(مختاړی + nodep->name());
                        إذا (ثابت AstVar* ثابت varp
                            = وموندل_منزل_Varp(foundp, nodep, nodep->access())) {
                            إذا (!varp->isParam()) foundp = تش_اشاره;
                        }
                        إذا (مختاړی.تشه()) اكسر;
                        string بل_مختاړی = LinkDotState::remove_وروستی_Inline_Scope(مختاړی);
                        إذا (مختاړی == بل_مختاړی) اكسر;
                        مختاړی = std::move(بل_مختاړی);
                    }
                    baddot = nodep->name();
                } وإلا {
                    foundp = m_statep->پیدا_کړه_Sym_Prefixed(dotSymp, nodep->name(), baddot, ریښتیا);
                }
                إذا (m_دننه_Sens && foundp) {
                    إذا (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ثابت varp
                    = foundp ? وموندل_منزل_Varp(foundp, nodep, nodep->access()) : تش_اشاره;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                إذا (nodep->varp() && error_Hier_Non_پورټ(nodep, nodep->varp(), dotSymp)) أرجع;
                إذا (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_غوټه::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    أرجع;
                }
            } وإلا {
                VSymEnt* ثابت foundp
                    = m_statep->پیدا_کړه_Sym_Prefixed(dotSymp, nodep->name(), baddot, ریښتیا);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : تش_اشاره;
                // Handle modport expression virtual ports
                إذا (!vscp) vscp = پیدا_کړه_Virtual_پورټ_Var_Scope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                إذا (vscp && error_Hier_Non_پورټ(nodep, vscp->varp(), dotSymp)) أرجع;
                إذا (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_غوټه::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } وإلا {
                    إذا (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ثابت newvscp
                        = جديد AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    فراغ لیدنه(AstEnumDType* nodep) override {
        LINKDOT_لیدنه_پیل();
        iterateChildren(nodep);
        AstRefDType* ثابت refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        إذا (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* nodep) override {
        LINKDOT_لیدنه_پیل();
        symIterateChildren(nodep, m_statep->get_غوټه_Sym(nodep));
    }
    فراغ لیدنه(Ast_Enum_توکی_Ref* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        إذا (m_statep->forParamed() && !nodep->dotted().تشه() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = تش_اشاره;
            VSymEnt* ثابت dotSymp
                = پیدا_کړه_Dotted_Ref_Symp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            إذا (!dotSymp) أرجع;
            VSymEnt* ثابت foundp
                = m_statep->پیدا_کړه_Sym_Prefixed(dotSymp, nodep->name(), baddot, ریښتیا);
            Ast_Enum_توکی* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), Enum_توکی) : تش_اشاره;
            إذا (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_غوټه::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                أرجع;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        إذا (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvt_منزل_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
            إذا (Ast_Enum_توکی* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), Enum_توکی) : تش_اشاره) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            إذا (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_لاره_Call* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_rand_لاره_Callp);
        {
            m_ds.init(m_curSymp);
            إذا (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_rand_لاره_Callp = nodep;
                ثابت Ast_غوټه_DType* fromDtp = nodep->fromp()->dtypep();
                إذا (!fromDtp) {
                    إذا (ثابت Ast_غوټه_Var_Ref* ثابت varRefp = VN_CAST(nodep->fromp(), غوټه_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } وإلا {
                        fromDtp = get_بیان_DTypep(nodep->fromp());
                    }
                    إذا (!fromDtp) {
                        إذا (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } وإلا {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                إذا (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = ریښتیا;
                } وإلا إذا (fromDtp) {
                    ثابت AstClassRefDType* ثابت classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    إذا (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    وإلا إذا (m_statep->forPrimary() && m_extendsParam.شمېر(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = ریښتیا;
                    } وإلا {
                        m_randSymp = m_statep->get_غوټه_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstVar* nodep) override {
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        إذا (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    فراغ لیدنه(AstVarScope* nodep) override {
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        إذا (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            Ast_ټاکل_W* ثابت assignp = جديد Ast_ټاکل_W{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جديد AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(ریښتیا);
            nodep->scopep()->جمع_کړه_Blocksp(جديد AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        إذا (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        إذا (m_ds.m_super) {
            إذا (AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(ریښتیا);
            } وإلا إذا (AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(ریښتیا);
            }
        }

        VL_RESTORER(m_randSymp);

        منطقي لومړی = !m_ds.m_dotp || m_ds.m_dotPos == DP_لومړی;
        منطقي staticAccess = خطأ;
        إذا (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            أرجع;
        } وإلا إذا (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_غوټه_FTask_Ref* ثابت newftaskp = nodep->clone_ونه(خطأ);
            newftaskp->dotted(m_ds.m_dot_متن);
            Ast_غوټه* ثابت newp = جديد AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlink_Fr_شا()};
            m_ds.m_unlinkedScopep = تش_اشاره;
            m_ds.m_unresolvedCell = خطأ;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = ریښتیا;
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            إذا (cpackagerefp->name() == "local::") {
                m_randSymp = تش_اشاره;
                لومړی = ریښتیا;
            } وإلا إذا (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, ریښتیا, خطأ, ":: reference");
                إذا (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } وإلا {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } وإلا إذا (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dot_متن);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            إذا (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ثابت Ast_غوټه* ثابت blkp = m_ds.m_dotSymp->nodep();
                إذا (VN_IS(blkp, Gen_بلاک)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), Gen_Case_توکی))) {
                    nodep->لري_Gen_بلاک(ریښتیا);
                }
            }
        } وإلا إذا (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_غوټه_بیان* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_شا(), غوټه_بیان);
            AstArg* ثابت argsp = nodep->argsp();
            إذا (argsp) argsp->unlink_Fr_شا_With_بل();
            Ast_لاره_Call* ثابت newp = جديد Ast_لاره_Call{nodep->fileline(), varEtcp,
                                                          VFlag_ماشوم_DType{}, nodep->name(), argsp};
            إذا (AstWith* ثابت withp = nodep->withp()) newp->withp(withp->unlink_Fr_شا());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_لومړی)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dot_متن = "";
        } وإلا إذا (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        إذا (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } وإلا إذا (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(تش_اشاره);
        } وإلا إذا (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } وإلا {
            string baddot;
            VSymEnt* okSymp = تش_اشاره;
            VSymEnt* dotSymp;
            إذا (nodep->dotted().تشه()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } وإلا {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            إذا (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_غوټه_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } وإلا {
                إذا (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = پیدا_کړه_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->پیدا_کړه_Dotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, ریښتیا);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            إذا (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_منزل_موخه(nodep->name()))) {
                VSymEnt* ثابت foundp = m_randSymp->پیدا_کړه_Id_Flat(nodep->name());
                إذا (foundp && m_currentWithp) {
                    إذا (m_currentWithp->restricted()) {
                        m_restricted_نومونه_کارول_شوی.ورننباسه(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = تش_اشاره;
                    إذا (nodep->argsp()) {
                        iterate_And_بل_Null(nodep->argsp());
                        argsp = nodep->argsp()->unlink_Fr_شا_With_بل();
                    }
                    إذا (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_غوټه* ثابت newp = جديد Ast_لاره_Call{
                        nodep->fileline(),
                        جديد AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", خطأ},
                        VFlag_ماشوم_DType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            إذا (لومړی && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ثابت randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), ریښتیا, ریښتیا);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->ورننباسه_بلاک(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                أرجع;
            }
            VSymEnt* ثابت foundp
                = m_statep->پیدا_کړه_Sym_Prefixed(dotSymp, nodep->name(), baddot, لومړی);
            Ast_غوټه_FTask* ثابت taskp = foundp ? VN_CAST(foundp->nodep(), غوټه_FTask) : تش_اشاره;
            Ast_غوټه_Module* ثابت taskModulep = foundp ? foundp->classOrPackagep() : تش_اشاره;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            إذا (!m_isParam || !taskp
                || (taskp->class_لاره() && taskModulep && taskModulep->hasParameterList())) {
                إذا (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    أرجع;
                }
                إذا (AstClass* ثابت موخه_Classp = VN_CAST(dotSymp->nodep(), Class)) {
                    إذا (m_extendsParam.شمېر(موخه_Classp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        أرجع;
                    }
                }
            }

            إذا (taskp) {
                إذا (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } وإلا {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvt_منزل_Hex(m_curSymp)
                                  << " dotSymp=se" << cvt_منزل_Hex(dotSymp));
                إذا (foundp) {
                    إذا (VN_IS(foundp->nodep(), Var) && m_ds.m_dot_متن == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } وإلا {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dot_متن << (m_ds.m_dot_متن == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } وإلا إذا (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } وإلا إذا ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ثابت classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ثابت funcp
                        = جديد AstFunc{nodep->fileline(), nodep->name(), تش_اشاره, تش_اشاره};
                    funcp->class_لاره(ریښتیا);
                    funcp->dtypep(funcp->پیدا_کړه_Void_DType());
                    classp->جمع_کړه_Membersp(funcp);
                    m_statep->ورننباسه_بلاک(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    أرجع;
                } وإلا إذا (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    إذا (AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        إذا (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } وإلا إذا (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } وإلا {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                        أرجع;
                    }
                } وإلا إذا (nodep->dotted() == "") {
                    إذا (nodep->pli()) {
                        إذا (v3Global.opt.bboxSys()) {
                            Ast_غوټه* newp;
                            إذا (VN_IS(nodep, FuncRef)) {
                                newp = جديد AstConst{nodep->fileline(), AstConst::All0{}};
                            } وإلا {
                                Ast_غوټه_بیان* outp = تش_اشاره;
                                طالما (AstArg* ثابت argp = nodep->argsp()) {
                                    outp = Ast_غوټه::جمع_کړه_بل(outp, argp->exprp()->unlink_Fr_شا());
                                    VL_DO_DANGLING(ټېل_Deletep(argp->unlink_Fr_شا()), argp);
                                }
                                newp = جديد AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                            أرجع;
                        } وإلا {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ثابت string suggest = speller.غوره_Candidate_پیغام(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.تشه() ? "" : nodep->warn_نور() + suggest));
                        }
                    } وإلا {
                        ثابت string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_غوټه_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.تشه() ? "" : nodep->warn_نور() + suggest));
                    }
                } وإلا {
                    ثابت string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_غوټه_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_غوټه::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.تشه() ? "" : nodep->warn_نور() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    فراغ لیدنه(Ast_Sel_بټ* nodep) override {
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        iterate_And_بل_Null(nodep->fromp());
        إذا (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_لومړی, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            أرجع;
        }
        إذا (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_لومړی) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dot_متن += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = ریښتیا;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterate_And_بل_Null(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        إذا (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_لومړی)) {
            Ast_غوټه_بیان* ثابت exprp = nodep->bitp()->unlink_Fr_شا();
            إذا (AstCellArrayRef* ثابت fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->جمع_کړه_Selp(exprp);
                AstCellArrayRef* ثابت movedp = fromArrRefp->unlink_Fr_شا();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            } وإلا {
                AstCellArrayRef* ثابت newp
                    = جديد AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        إذا (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_لومړی) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = ریښتیا;
            أرجع;
        }
        Ast_غوټه_DType* packedArrayDtp = تش_اشاره;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterate_And_بل_Null(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            إذا (m_packedArrayDtp) {
                AstRange* ثابت newRangep
                    = جديد AstRange(nodep->fileline(), nodep->rhsp()->unlink_Fr_شا(),
                                   nodep->thsp()->unlink_Fr_شا());
                AstPackArrayDType* newArrayTypep
                    = جديد AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->ماشوم_DTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(تش_اشاره);
                إذا (VN_IS(nodep->backp(), Sel_ایستل)) {
                    packedArrayDtp = newArrayTypep;
                } وإلا {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                }
            } وإلا {
                إذا (nodep->attrp()) {
                    Ast_غوټه* ثابت attrp = nodep->attrp()->unlink_Fr_شا();
                    VL_DO_DANGLING(attrp->delete_ونه(), attrp);
                }
                Ast_غوټه* ثابت basefromp = nodep->بنسټ_Fromp(خطأ);
                إذا (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    إذا (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } وإلا {
                    nodep->attrp(جديد Ast_Attr_د{nodep->fileline(), VAttrType::VAR_بنسټ,
                                               basefromp->clone_ونه(خطأ)});
                }
            }
        }

        إذا (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_لیدنه_پیل();
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            إذا (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_غوټه_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_منزل_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_منزل_Hex(m_curSymp));
    }
    فراغ لیدنه(Ast_غوټه_بلاک* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            إذا (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_غوټه_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_منزل_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_منزل_Hex(m_curSymp));
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        إذا (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(ریښتیا);
            إذا (ثابت VSymEnt* ثابت foundp
                = m_curSymp->پیدا_کړه_Id_Fallback("extern " + nodep->name())) {
                ثابت Ast_غوټه_FTask* ثابت protop = VN_AS(foundp->nodep(), غوټه_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->جمع_کړه_Stmtsp(جديد Ast_Attr_د{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_ونه(خطأ)
                                                   : جديد AstVoidDType{protop->fileline()}});
                لكل (Ast_غوټه* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                        Ast_Attr_د* ثابت attrp
                            = جديد Ast_Attr_د{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_ونه(خطأ)};
                        attrp->name(portp->name());
                        nodep->جمع_کړه_Stmtsp(attrp);
                    }
                }
            } وإلا {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        إذا (nodep->isExternProto()) {
            إذا (!m_curSymp->پیدا_کړه_Id_Fallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_غوټه_Sym(nodep);
            ثابت منطقي isNew = nodep->name() == "new";
            إذا (isNew) m_explicitSuperNewp = تش_اشاره;
            iterateChildren(nodep);
            إذا (isNew) {
                ثابت AstClassExtends* ثابت classExtendsp = VN_AS(m_modp, Class)->extendsp();
                إذا (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
                    && classExtendsp->argsp()) {
                    m_explicitSuperNewp->v3error(
                        "Explicit super.new not allowed with class "
                        "extends arguments (IEEE 1800-2023 8.17)\n"
                        << m_explicitSuperNewp->warn_نور() << "... Suggest remove super.new\n"
                        << m_explicitSuperNewp->warnContextPrimary() << '\n'
                        << classExtendsp->argsp()->warnOther()
                        << "... Location of extends argument(s)\n"
                        << classExtendsp->argsp()->warnContextSecondary());
                }
                إذا (classExtendsp && classExtendsp->classOrNullp()) {
                    إذا (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->ماشوم_DTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_غوټه_Stmt* ثابت superNewp
                            = جمع_کړه_Implicit_Super_New_Call(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_غوټه_Sym(nodep));
    }
    فراغ لیدنه(AstRandSequence* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        إذا (!nodep->پیل().تشه())
            nodep->prodp(پیدا_کړه_Prod(nodep, m_statep->get_غوټه_Sym(nodep), nodep->پیل()));
        symIterateChildren(nodep, m_statep->get_غوټه_Sym(nodep));
    }
    فراغ لیدنه(AstRSProd* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_غوټه_Sym(nodep));
    }
    فراغ لیدنه(Ast_RSProd_توکی* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(پیدا_کړه_Prod(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCoverWith* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_بل_Null(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterate_And_بل_Null(nodep->itemp()->ماشوم_DTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_غوټه_Sym(nodep);
        iterate_And_بل_Null(nodep->itemp());
        iterate_And_بل_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    فراغ لیدنه(AstCoverCrossWith* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_بل_Null(nodep->selectp());
        iterate_And_بل_Null(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterate_And_بل_Null(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_غوټه_Sym(nodep);
        iterate_And_بل_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    فراغ لیدنه(AstWith* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        إذا (m_ds.m_unresolvedClass) أرجع;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restricted_نومونه_کارول_شوی);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_غوټه_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ثابت منطقي لومړی_لیدنه = !nodep->validated();
            إذا (لومړی_لیدنه) {
                nodep->validated(ریښتیا);
                m_restricted_نومونه_کارول_شوی.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                إذا (nodep->restricted() && m_randSymp) {
                    لكل (ثابت std::string& n : nodep->restricted_نومونه()) {
                        إذا (!m_randSymp->پیدا_کړه_Id_Flat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restricted_نومونه_کارول_شوی.ورننباسه(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            إذا (لومړی_لیدنه && nodep->restricted()) {
                لكل (ثابت std::string& n : nodep->restricted_نومونه()) {
                    إذا (!m_restricted_نومونه_کارول_شوی.شمېر(n)) {
                        nodep->v3warn(ناکارول_شوی_2, "Identifier '"
                                                  << n
                                                  << "' in 'with (...)' identifier list is"
                                                     " not referenced in the constraint"
                                                     " block");
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    فراغ لیدنه(AstLambdaArgRef* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        إذا (nodep->user3_سېټ_Once()) أرجع;
        // If the class is resolved, there is nothing more to do
        إذا (nodep->classOrNullp()) أرجع;
        LINKDOT_لیدنه_پیل();
        إذا (m_statep->forPrimary()) {
            إذا (nodep->ماشوم_DTypep()) أرجع;
            Ast_غوټه* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            إذا (AstDot* ثابت dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(ریښتیا);
                إذا (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, ریښتیا, خطأ, nodep->verilogKwd());
                    إذا (!foundp) {
                        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                        أرجع;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_غوټه_Sym(lookNodep->classOrPackageSkipp());
                } وإلا {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ثابت cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            إذا (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
            VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, ریښتیا,
                                                                    ریښتیا, nodep->verilogKwd());
            إذا (!foundp) {
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
            إذا (AstClass* ثابت classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                إذا (paramsp) {
                    paramsp = paramsp->clone_ونه(ریښتیا);
                    nodep->parameterized(ریښتیا);
                }
                nodep->ماشوم_DTypep(جديد AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->ماشوم_DTypep());
            } وإلا إذا (AstParamTypeDType* ثابت paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ثابت refParamp = جديد AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->ماشوم_DTypep(refParamp);
                nodep->parameterized(ریښتیا);
            } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_غوټه_DType* ثابت unwrappedp = typedefp->subDTypep()->skipRefp();
                إذا (AstClassRefDType* ثابت classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    إذا (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_ونه(ریښتیا);
                        nodep->parameterized(ریښتیا);
                    } وإلا إذا (paramsp) {
                        paramsp = paramsp->clone_ونه(ریښتیا);
                        nodep->parameterized(ریښتیا);
                    }
                    nodep->ماشوم_DTypep(
                        جديد AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->ماشوم_DTypep());
                } وإلا {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    أرجع;
                }
            } وإلا {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                أرجع;
            }
            إذا (!nodep->ماشوم_DTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlink_Fr_شا()->delete_ونه();
        } وإلا {
            // Probably a parameter
            إذا (AstRefDType* ثابت refp = VN_CAST(nodep->ماشوم_DTypep(), RefDType)) {
                إذا (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_ونه(خطأ));
                    VL_DO_DANGLING(ټېل_Deletep(refp), refp);
                } وإلا {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ثابت Ast_غوټه_DType* typep = refp->refDTypep();
                    طالما (ریښتیا) {
                        إذا (ثابت AstParamTypeDType* ثابت atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            استمر;
                        }
                        إذا (ثابت AstRequireDType* ثابت atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            استمر;
                        }
                        اكسر;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    فراغ لیدنه(AstClass* nodep) override {
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_modp);
        VL_RESTORER_COPY(m_if_Class_Imp_نومونه);
        VL_RESTORER_COPY(m_ifForwards);
        VL_RESTORER(m_insideClassExtParam);
        {
            m_ifForwards.clear();
            m_ds.init(m_curSymp);
            m_insideClassExtParam = nodep->isCovergroup() && nodep->covergroupEnclosingClassp()
                                    && m_extendsParam.پیدا_کړه(nodep->covergroupEnclosingClassp())
                                           != m_extendsParam.پای_ټکی();
            // Until overridden by a SCOPE
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_غوټه_Sym(nodep);
            m_modp = nodep;
            صحيح بل = 0;
            لكل (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                إذا (++بل == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                إذا (m_statep->forPrimary()) {
                    إذا (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.ورننباسه(nodep);
                        m_insideClassExtParam = ریښتیا;
                    }
                    إذا (AstClassRefDType* ثابت classRefp
                        = VN_CAST(cextp->ماشوم_DTypep(), ClassRefDType)) {
                        AstClass* ثابت classp = classRefp->classp();
                        إذا (classp != nodep) iterate(classp);
                        إذا (m_extendsParam.پیدا_کړه(classp) != m_extendsParam.پای_ټکی()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.ورننباسه(nodep);
                            m_insideClassExtParam = ریښتیا;
                        }
                    }
                }

                إذا (AstClass* ثابت بنسټ_Classp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    إذا (بنسټ_Classp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } وإلا إذا (extendsClass(بنسټ_Classp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << بنسټ_Classp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } وإلا إذا (cextp->isImplements() && !بنسټ_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << بنسټ_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } وإلا إذا (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && بنسټ_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << بنسټ_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    بنسټ_Classp->isExtended(ریښتیا);
                    nodep->isExtended(ریښتیا);
                    iterate(بنسټ_Classp);
                    importSymbolsFromExtended(nodep, cextp);
                    استمر;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            لكل (ثابت خپلکاری& itr : m_ifForwards) {
                ثابت VSymEnt* ثابت implSymp = itr.دوهم;
                جمع_کړه_Interface_Forward(nodep, VN_AS(implSymp->nodep(), غوټه_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            لكل (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->پای_ټکی(); ++it) {
                Ast_غوټه* ثابت itemp = it->دوهم->nodep();
                إذا (!memberMap.پیدا_کړه_Member(nodep, it->لومړی)) {
                    إذا (Ast_Enum_توکی* ثابت aitemp = VN_CAST(itemp, Enum_توکی)) {
                        Ast_Enum_توکی_Ref* ثابت newp = جديد Ast_Enum_توکی_Ref{
                            aitemp->fileline(), aitemp, it->دوهم->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->لومړی << "' " << newp);
                        nodep->جمع_کړه_Membersp(newp);
                        memberMap.ورننباسه(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    ساكن فراغ collectScopeRefs(Ast_غوټه* nodep, std::ویکټور<AstClassOrPackageRef*>& refps) {
        إذا (AstClassOrPackageRef* ثابت refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ټېل_شا(refp);
            أرجع;
        }
        AstDot* ثابت dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    منطقي reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::ویکټور<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        لكل (اندازه_t i = 0; i < refps.اندازه(); ++i) {
            AstClassOrPackageRef* ثابت refp = refps[i];
            إذا (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, خطأ,
                                                    "class/package reference")) {
                أرجع خطأ;  // Error already reported
            }
            إذا (i + 1 == refps.اندازه()) اكسر;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_غوټه_Module* ثابت modp = refp->classOrPackageSkipp();
            إذا (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                أرجع خطأ;
            }
            scopeSymp = m_statep->get_غوټه_Sym(modp);
        }
        AstClassOrPackageRef* ثابت innerp = refps.شا();
        innerp->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(scopeDotp->unlink_Fr_شا()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        أرجع ریښتیا;
    }

    فراغ لیدنه(AstRefDType* nodep) override {

        إذا (خپلکاری* ثابت typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            إذا (Ast_غوټه_DType* ثابت resolvedDTypep = VN_CAST(nodep->typeofp(), غوټه_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlink_Fr_شا();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                إذا (V3LinkDotIfaceCapture::enabled()) {
                    إذا (AstRefDType* ثابت resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        إذا (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            أرجع;
        }

        // Resolve its reference
        إذا (m_statep->forParamed() && nodep->user3()) {
            إذا (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(خطأ);
        }
        إذا (nodep->user3_سېټ_Once()) {
            إذا (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture skip revisit name=" << nodep->name()
                                  << " already user3 and captured cell=" << nodep->user2p());
            }
            // Originally this block re-resolved a
            // classOrPackageRef during the paramed pass for RefDTypes that
            // were already visited (user3).  The intent was to ensure
            // T::member references through type parameters stayed linked
            // after deparameterization.  In practice, RefDTypes that reach
            // user3-revisit never carry a classOrPackageOpp in the paramed
            // pass: the class/package is resolved in the primary pass and
            // the skip pointer persists.
            إذا (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            أرجع;
        }
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        إذا (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            إذا (AstDot* ثابت scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                إذا (!reduceScopeDot(nodep, scopeDotp)) أرجع;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_غوټه* ثابت cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            إذا (AstClassOrPackageRef* ثابت cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ثابت AstClass* ثابت clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                إذا (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    أرجع;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                إذا (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    أرجع;
                }

                ثابت منطقي doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                إذا (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ریښتیا, خطأ, "class/package reference");
                    إذا (!foundp) أرجع;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                إذا (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    إذا (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        أرجع;
                    }
                    // Likely impossible, as error thrown earlier
                    cpackagerefp->v3error(  // LCOV_EXCL_LINE
                        "'::' expected to reference a class/package but referenced '"
                        << (nodep->classOrPackagep() ? nodep->classOrPackagep()->prettyTypeName()
                                                     : "<unresolved-object>")
                        << "'\n"
                        << cpackagerefp->warn_نور() + "... Suggest '.' instead of '::'");
                }
            }
            VL_DO_DANGLING(ټېل_Deletep(cpackagep->unlink_Fr_شا()), cpackagep);
        }

        إذا (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            خپلکاری* ثابت cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } وإلا {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ثابت منطقي ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            إذا (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } وإلا {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        إذا (!nodep->typedefp() && !nodep->subDTypep()) {
            ثابت VSymEnt* foundp;
            إذا (nodep->classOrPackagep()) {
                foundp = m_statep->get_غوټه_Sym(nodep->classOrPackagep())->پیدا_کړه_Id_Flat(nodep->name());
            } وإلا إذا (m_ds.m_dotPos == DP_لومړی || m_ds.m_dotPos == DP_NONE) {
                foundp = پیدا_کړه_Id_Fallback_Skip_Member_DType(m_curSymp, nodep->name());
            } وإلا {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<صحيح>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = تش_اشاره;  // LCOV_EXCL_LINE
            }
            {
                إذا (AstTypedef* ثابت defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : تش_اشاره) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    إذا (!nodep->classOrPackagep() && !defp->isUnderClass())
                        check_Decl_ترتیب(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } وإلا إذا (AstParamTypeDType* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : تش_اشاره) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ثابت منطقي is_Param_Type_ماشوم = (defp == nodep->backp());
                    ثابت منطقي isTypedefRecursion
                        = is_Param_Type_ماشوم && VN_IS(defp->backp(), Typedef);
                    إذا (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dot_متن << (m_ds.m_dot_متن == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->پیدا_کړه_Void_DType());  // Try to reduce later errors
                    } وإلا {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } وإلا إذا (AstClass* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : تش_اشاره) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    إذا (!nodep->classOrPackagep()) check_Decl_ترتیب(nodep, defp);
                    AstPin* ثابت paramsp = nodep->paramsp();
                    إذا (paramsp) paramsp->unlink_Fr_شا_With_بل();
                    AstClassRefDType* ثابت newp
                        = جديد AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                } وإلا إذا (m_insideClassExtParam) {
                    أرجع;
                } وإلا {
                    إذا (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } وإلا {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        إذا (nodep->lhsp() && !VN_IS(nodep->lhsp(), غوټه_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            إذا (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } وإلا {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(جديد AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_لیدنه_پیل();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ثابت foundp = m_curSymp->پیدا_کړه_Id_Fallback(nodep->name());
        Ast_غوټه_FTask* ثابت taskp = foundp ? VN_AS(foundp->nodep(), غوټه_FTask) : تش_اشاره;
        إذا (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } وإلا إذا (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } وإلا {
            taskp->dpiExport(ریښتیا);
            إذا (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstDisable* nodep) override {
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        منطقي hasPartSelect = خطأ;
        ثابت string موخه_لار
            = nodep->موخه_Refp() ? ایستل_Dotted_لار(nodep->موخه_Refp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_لومړی;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        إذا (nodep->موخه_Refp()) {
            إذا (AstTaskRef* ثابت taskRefp = VN_CAST(nodep->موخه_Refp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } وإلا إذا (!VN_IS(nodep->موخه_Refp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->موخه_Refp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
            إذا (nodep->targetp()) {
                nodep->موخه_Refp()->unlink_Fr_شا()->delete_ونه();
                إذا (!hasPartSelect) {
                    ثابت string instance_لار = موخه_Instance_لار(nodep->targetp(), موخه_لار);
                    إذا (!instance_لار.تشه()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->موخه_Refp(
                            جديد AstVarXRef{nodep->fileline(), "", instance_لار, VAccess::READ});
                    }
                }
            }
        }
    }
    فراغ لیدنه_Package_Import_Or_Export(Ast_غوټه* nodep) {
        // No longer needed
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        إذا (m_statep->forParamed()) VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstPackageImport* nodep) override { لیدنه_Package_Import_Or_Export(nodep); }
    فراغ لیدنه(AstPackageExport* nodep) override { لیدنه_Package_Import_Or_Export(nodep); }
    فراغ لیدنه(Ast_Package_Export_ستوری_ستوری* nodep) override { لیدنه_Package_Import_Or_Export(nodep); }
    فراغ لیدنه(AstCellRef* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCellArrayRef* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    فراغ لیدنه(AstUnlinkedRef* nodep) override {
        LINKDOT_لیدنه_پیل();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        إذا (AstNew* ثابت newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } وإلا إذا (ثابت AstDot* ثابت dotp = VN_CAST(nodep->exprp(), Dot)) {
            إذا (dotp->lhsp()->name() == "super") {
                إذا (AstNew* ثابت newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstIfaceRefDType* nodep) override {
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        إذا (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ثابت ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            إذا (ifacep->dead()) أرجع;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_کارول_شوی_Pins);
            VL_RESTORER_CLEAR(m_کارول_شوی_Def_Param_Pins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_غوټه_Sym(ifacep);
            iterate_And_بل_Null(nodep->paramsp());
        }
    }

    فراغ لیدنه(Ast_Attr_د* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(AstCastParse* nodep) override {
        إذا (nodep->user3_سېټ_Once()) أرجع;
        LINKDOT_لیدنه_پیل();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ثابت منطقي dotLhs = m_ds.m_dotPos == DP_لومړی;
        إذا (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        إذا (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    فراغ لیدنه(Ast_ټاکل_W* nodep) override {
        LINKDOT_لیدنه_پیل();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        إذا (nodep->user2()) m_replaceWithAlias = خطأ;
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = ریښتیا;
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override {
        VL_RESTORER(m_دننه_Packed_Array);
        إذا (VN_IS(nodep, PackArrayDType)) {
            m_دننه_Packed_Array = ریښتیا;
        } وإلا إذا (!m_دننه_Packed_Array) {
            LINKDOT_لیدنه_پیل();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ثابت std::map<std::string, Ast_غوټه_Module*> modules_منزل_Revisit
            = std::move(m_modules_منزل_Revisit);
        m_وروستی_Deferredp = تش_اشاره;
        لكل (خپلکاری& p : modules_منزل_Revisit) {
            Ast_غوټه_Module* ثابت modp = p.دوهم;
            modp->foreach([](Ast_غوټه* ثابت nodep) { nodep->user3(خطأ); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = افتراضي;
};

//######################################################################
// Link class functions

فراغ V3LinkDot::dumpSubstep(ثابت string& name) {
    إذا (dump_ونه_Either_Level() >= 9) {
        V3Global::dump_Check_Global_ونه(name);
    } وإلا إذا (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_ونه_File(v3Global.debugFilename(name + ".tree"));
    }
}

فراغ V3LinkDot::linkDotGuts(AstNetlist* rootp, VLink_Dot_ګام ګام) {
    VIsCached::clear_Cache_ونه();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, ګام};

    { Link_Dot_پیدا_کړه_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { Link_Dot_پیدا_کړه_Iface_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    إذا (ګام == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(ریښتیا);
        dumpSubstep("prelinkdot-scoped");
    } وإلا {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    ثابت LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

فراغ V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_ونه("linkdot", 0, dump_ونه_Either_Level() >= 6);
}

فراغ V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_ونه("linkdotparam", 0, dump_ونه_Either_Level() >= 3);
}

فراغ V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_ونه("linkdot", 0, dump_ونه_Either_Level() >= 3);
}
