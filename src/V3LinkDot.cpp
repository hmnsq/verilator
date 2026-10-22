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
//چوٹی
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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkDot.h"

#شامل "V3Global.h"
#شامل "V3Graph.h"
#شامل "V3LinkDotIfaceCapture.h"
#شامل "V3MemberMap.h"
#شامل "V3Parse.h"
#شامل "V3Randomize.h"
#شامل "V3Stats.h"
#شامل "V3String.h"
#شامل "V3SymTable.h"

#شامل <algorithm>
#شامل <cstdlib>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ساکن string نکالو_Dotted_راستہ(Ast_گرہ* nodep, منطقی& hasPartSelect) {
    اگر (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) {
        واپس refp->name();
    } ورنہ اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
        واپس refp->name();
    } ورنہ اگر (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
        ثابت string lhs = نکالو_Dotted_راستہ(dotp->lhsp(), hasPartSelect);
        ثابت string rhs = نکالو_Dotted_راستہ(dotp->rhsp(), hasPartSelect);
        واپس VString::dot(lhs, ".", rhs);
    } ورنہ اگر (VN_IS(nodep, Sel_ثنائی_ہندسہ) || VN_IS(nodep, Sel_نکالو) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = سچ;
        واپس "";
    }
    واپس "";
}
ساکن string lexical_Disable_راستہ(Ast_گرہ* ثابت targetp) {
    std::سمتیہ<string> نام;
    برائے (Ast_گرہ* curp = targetp; curp; curp = curp->aboveLoopp()) {
        اگر (Ast_گرہ_بلاک* ثابت blockp = VN_CAST(curp, گرہ_بلاک)) {
            اگر (blockp->name() != "") نام.ڈالو_عقب(blockp->name());
        } ورنہ اگر (Ast_گرہ_FTask* ثابت ftaskp = VN_CAST(curp, گرہ_FTask)) {
            نام.ڈالو_عقب(ftaskp->name());
        } ورنہ اگر (VN_IS(curp, گرہ_Module)) {
            توڑو;
        }
    }
    string راستہ;
    برائے (خودکار it = نام.crbegin(); it != نام.crend(); ++it) {
        راستہ = VString::dot(راستہ, ".", *it);
    }
    واپس راستہ;
}
ساکن string ہدف_Instance_راستہ(Ast_گرہ* ثابت targetp, ثابت string& ہدف_راستہ) {
    ثابت string lexical_راستہ = lexical_Disable_راستہ(targetp);
    اگر (lexical_راستہ == "" || ہدف_راستہ == lexical_راستہ) واپس "";
    ثابت string suffix = "." + lexical_راستہ;
    اگر (!VString::سرے_With(ہدف_راستہ, suffix)) واپس "";
    واپس ہدف_راستہ.substr(0, ہدف_راستہ.سائز() - suffix.سائز());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

جماعت Link_گرہ_Matcher_Class_Or_Package final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override {
        واپس VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
جماعت Link_گرہ_Matcher_FTask final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override { واپس VN_IS(nodep, گرہ_FTask); }
};
جماعت Link_گرہ_Matcher_Modport final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override { واپس VN_IS(nodep, Modport); }
};
جماعت Link_گرہ_Matcher_Prod final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override { واپس VN_IS(nodep, RSProd); }
};
جماعت Link_گرہ_Matcher_Var final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override {
        واپس VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
جماعت Link_گرہ_Matcher_Var_IO final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        اگر (!varp) واپس جھوٹ;
        واپس varp->isIO();
    }
};
جماعت Link_گرہ_Matcher_Var_Param final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        اگر (!varp) واپس جھوٹ;
        واپس varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

جماعت LinkDotState final {
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
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    ثابت VNUser4_اندر_Use m_inuser4;

عوامی:
    // ENUMS
    // In order of priority, compute first ... compute last
    شمار SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

نجی:
    // TYPES
    استعمال ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    استعمال IfaceModSyms = std::سمتیہ<std::pair<AstIface*, VSymEnt*>>;

    ساکن LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSym_گراف m_syms;  // Symbol table by hierarchy
    VSym_گراف m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = خالی_اشاریہ;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::سیٹ<std::pair<Ast_گرہ_Module*, std::string>>
        m_implicit_Name_سیٹ;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::سمتیہ<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ثابت VLink_Dot_قدم m_قدم;  // Operational step

عوامی:
    // METHODS

    خالی dumpSelf(ثابت string& nameComment, منطقی force = جھوٹ) {
        اگر (debug() >= 6 || dumpLevel() >= 6 || force) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            اگر (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            منطقی پہلا = سچ;
            برائے (صحیح samn = 0; samn < SAMN__MAX; ++samn) {
                اگر (!m_scopeAliasMap[samn].خالی_خانے()) {
                    اگر (پہلا) os << "\nScopeAliasMap:\n";
                    پہلا = جھوٹ;
                    برائے (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                         it != m_scopeAliasMap[samn].انتہا(); ++it) {
                        // left side is what we will import into
                        os << "\t" << samn << "\t" << it->پہلا << " ("
                           << it->پہلا->nodep()->typeName() << ") <- " << it->دوسرا << " "
                           << it->دوسرا->nodep() << '\n';
                    }
                }
            }
        }
    }
    ساکن خالی preErrorDumpHandler() {
        اگر (s_errorThisp) s_errorThisp->preErrorDump();
    }
    خالی preErrorDump() {
        ساکن منطقی s_diddump = جھوٹ;
        اگر (!s_diddump && dump_درخت_Level()) {
            s_diddump = سچ;
            dumpSelf("linkdot-preerr", سچ);
            v3Global.rootp()->dump_درخت_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLink_Dot_قدم قدم)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_قدم{قدم} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = یہ;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        read_Mod_نام();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(خالی_اشاریہ);
        s_errorThisp = خالی_اشاریہ;
    }

    // ACCESSORS
    VSym_گراف* symsp() { واپس &m_syms; }
    صحیح قدم_نمبر() ثابت { واپس static_cast<صحیح>(m_قدم); }
    منطقی forPrimary() ثابت { واپس m_قدم == LDS_PRIMARY; }
    منطقی forParamed() ثابت { واپس m_قدم == LDS_PARAMED; }
    منطقی forScopeCreation() ثابت { واپس m_قدم == LDS_SCOPED; }

    // METHODS
    ساکن string گرہ_متن_Type(Ast_گرہ* nodep) {
        اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            اگر (varp->isIO() || varp->isIfaceRef()) {
                واپس "port";
            } ورنہ اگر (varp->isGParam()) {
                واپس "parameter";
            } ورنہ اگر (varp->varType() == VVarType::LPARAM) {
                واپس "local parameter";
            } ورنہ {
                واپس "variable";
            }
        } ورنہ اگر (ثابت AstParamTypeDType* ثابت dtypep = VN_CAST(nodep, ParamTypeDType)) {
            اگر (dtypep->isGParam()) {
                واپس "type parameter";
            } ورنہ {
                واپس "local type parameter";
            }
        } ورنہ اگر (VN_IS(nodep, Begin)) {
            واپس "block";
        } ورنہ اگر (VN_IS(nodep, Cell)) {
            واپس "instance";
        } ورنہ اگر (VN_IS(nodep, Constraint)) {
            واپس "constraint";
        } ورنہ اگر (VN_IS(nodep, Func)) {
            واپس "function";
        } ورنہ اگر (VN_IS(nodep, Gen_بلاک)) {
            واپس "generate block";
        } ورنہ اگر (VN_IS(nodep, Iface)) {
            واپس "interface";
        } ورنہ اگر (VN_IS(nodep, RSProd)) {
            واپس "randsequence production";
        } ورنہ اگر (VN_IS(nodep, Task)) {
            واپس "task";
        } ورنہ {
            واپس nodep->prettyTypeName();
        }
    }

    Ast_گرہ_Module* ڈھونڈو_Module_Sym(ثابت string& modName) {
        ثابت VSymEnt* ثابت foundp = m_mods.rootp()->ڈھونڈو_Id_Fallback(modName);
        واپس foundp ? VN_AS(foundp->nodep(), گرہ_Module) : خالی_اشاریہ;
    }
    خالی read_Mod_نام() {
        // Look at all modules, and store pointers to all module names
        برائے (Ast_گرہ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گرہ_Module);
            m_mods.rootp()->داخل_کرو(nodep->name(), نیا VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* جڑ_Entp() ثابت { واپس m_syms.rootp(); }
    VSymEnt* dunitEntp() ثابت { واپس m_dunitEntp; }
    خالی checkDuplicate(VSymEnt* lookupSymp, Ast_گرہ* nodep, ثابت string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ثابت VSymEnt* ثابت foundp = lookupSymp->ڈھونڈو_Id_Flat(name);
        Ast_گرہ* ثابت fnodep = foundp ? foundp->nodep() : خالی_اشاریہ;
        اگر (!fnodep) {
            // Not found, will add in a moment.
            اگر (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ثابت VSymEnt* ثابت alt = lookupSymp->ڈھونڈو_Similar_Id_Flat(name);
                اگر (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } ورنہ اگر (nodep == fnodep) {  // Already inserted.
            // Good.
        } ورنہ اگر (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } ورنہ اگر (forPrimary() && VN_IS(nodep, Gen_بلاک)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, Gen_بلاک))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } ورنہ {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ثابت string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_گرہ::prettyNameQ(name);
            اگر (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << گرہ_متن_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_عدد_سیٹ_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_عدد_سیٹ_Min(0);  // Disable checking forward typedefs
            } ورنہ {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(گرہ_متن_Type(nodep)) << " has the same name as "
                               << گرہ_متن_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    خالی داخل_کرو_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ثابت symp = نیا VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvt_مقصد_Hex(symp));
        symp->parentp(جڑ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(خالی_اشاریہ);
        جڑ_Entp()->داخل_کرو("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* داخل_کرو_چوٹی_Cell(Ast_گرہ_Module* nodep, ثابت string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ثابت symp = نیا VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvt_مقصد_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(جڑ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(جڑ_Entp(), nodep, nodep->origName());
        جڑ_Entp()->داخل_کرو(nodep->origName(), symp);
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        واپس symp;
    }
    VSymEnt* داخل_کرو_چوٹی_Iface(AstCell* nodep, ثابت string& scopename) {
        VSymEnt* ثابت symp = نیا VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvt_مقصد_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(جڑ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        اگر (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(جڑ_Entp(), nodep, nodep->origName());
        جڑ_Entp()->داخل_کرو(nodep->origName(), symp);
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        واپس symp;
    }
    VSymEnt* داخل_کرو_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ثابت string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = نیا VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvt_مقصد_Hex(symp) << "  " << scopename << " above=se"
                                      << cvt_مقصد_Hex(abovep) << " mods=se" << cvt_مقصد_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        اگر (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        اگر (forScopeCreation() && abovep != modSymp && !modSymp->ڈھونڈو_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        واپس symp;
    }
    خالی داخل_کرو_Map(VSymEnt* symp, ثابت string& scopename) {
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* داخل_کرو_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ثابت string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = نیا VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvt_مقصد_Hex(symp) << "  " << basename << " above=se"
                                      << cvt_مقصد_Hex(abovep) << " mods=se" << cvt_مقصد_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->sym_سابقہ(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        اگر (abovep != modSymp && !modSymp->ڈھونڈو_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        واپس symp;
    }
    VSymEnt* داخل_کرو_بلاک(VSymEnt* abovep, ثابت string& name, Ast_گرہ* nodep,
                         Ast_گرہ_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = نیا VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvt_مقصد_Hex(symp) << "  above=se" << cvt_مقصد_Hex(abovep)
                                      << " pkg=" << cvt_مقصد_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        اگر (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        واپس symp;
    }
    VSymEnt* داخل_کرو_Sym(VSymEnt* abovep, ثابت string& name, Ast_گرہ* nodep,
                       Ast_گرہ_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = نیا VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTsym se" << cvt_مقصد_Hex(symp) << "  name='" << name << "' above=se"
                                      << cvt_مقصد_Hex(abovep) << " pkg=" << cvt_مقصد_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        // We don't remember the ent associated with each node, because we
        // need a unique scope entry for each instantiation
        symp->classOrPackagep(classOrPackagep);
        symp->parentp(abovep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, name);
        abovep->reinsert(name, symp);
        واپس symp;
    }
    ساکن منطقی exists_گرہ_Sym(Ast_گرہ* nodep) { واپس nodep->user1p() != خالی_اشاریہ; }
    ساکن VSymEnt* get_گرہ_Sym(Ast_گرہ* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ثابت symp = nodep->user1u().مقصد_Sym_Ent();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        واپس symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ثابت خودکار it = m_nameScopeSymMap.ڈھونڈو(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.انتہا(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        واپس it->دوسرا;
    }
    خالی implicit_Ok_جوڑو(Ast_گرہ_Module* nodep, ثابت string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        اگر (nodep) {
            ثابت خودکار it = m_implicit_Name_سیٹ.ڈھونڈو(std::make_pair(nodep, varname));
            اگر (it == m_implicit_Name_سیٹ.انتہا()) m_implicit_Name_سیٹ.emplace(nodep, varname);
        }
    }
    منطقی implicitOk(Ast_گرہ_Module* nodep, ثابت string& varname) {
        واپس nodep
               && (m_implicit_Name_سیٹ.ڈھونڈو(std::make_pair(nodep, varname))
                   != m_implicit_Name_سیٹ.انتہا());
    }

    // Track and later recurse interface modules
    خالی داخل_کرو_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_عقب(nodep, symp);
    }
    خالی computeIfaceModSyms();

    // Track and later insert interface references
    خالی داخل_کرو_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ڈالو_عقب(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ساکن AstIfaceRefDType* ifaceRefFromArray(Ast_گرہ_DType* nodep) {
        جب (nodep) {
            اگر (AstIfaceRefDType* ثابت ifp = VN_CAST(nodep, IfaceRefDType)) واپس ifp;
            اگر (ثابت AstBracketArrayDType* ثابت arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } ورنہ {
                واپس خالی_اشاریہ;
            }
        }
        واپس خالی_اشاریہ;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* زندہ_Iface_From_Pin_اظہاریہ(Ast_گرہ* exprp, VSymEnt* والد_Symp) {
        AstVar* resolvedVarp = خالی_اشاریہ;
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } ورنہ اگر (AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ثابت lookupSymp = والد_Symp ? والد_Symp->parentp() : خالی_اشاریہ;
            اگر (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = خالی_اشاریہ;
                ثابت string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ثابت dotSymp
                    = ڈھونڈو_Dotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, سچ);
                اگر (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = خالی_اشاریہ;
        اگر (resolvedVarp) {
            AstIfaceRefDType* ثابت irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            اگر (irefp && irefp->iface_بذریعہ_Cellp() && !irefp->iface_بذریعہ_Cellp()->dead()) {
                resultp = irefp->iface_بذریعہ_Cellp();
            }
        }
        واپس resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    منطقی repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, منطقی isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* والد_Symp = varSymp->parentp();
        AstCell* والد_Cellp = خالی_اشاریہ;
        برائے (صحیح گہرائی = 0; والد_Symp && گہرائی < 20; ++گہرائی) {
            اگر (AstCell* ثابت cp = VN_CAST(والد_Symp->nodep(), Cell)) {
                والد_Cellp = cp;
                توڑو;
            }
            والد_Symp = والد_Symp->parentp();
        }
        اگر (!والد_Cellp) واپس جھوٹ;
        // Scan pins to find the one connected to this port variable
        برائے (AstPin* pinp = والد_Cellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->modVarp() != varp && pinp->name() != varp->name()) جاری;
            Ast_گرہ* ثابت exprp = pinp->exprp();
            اگر (!exprp) واپس جھوٹ;
            AstIface* ثابت newIfacep = زندہ_Iface_From_Pin_اظہاریہ(exprp, والد_Symp);
            اگر (newIfacep && newIfacep != ifacerefp->iface_بذریعہ_Cellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                واپس سچ;
            }
            اگر (newIfacep) واپس !isDead;  // Same interface - OK if live
            واپس جھوٹ;
        }
        واپس جھوٹ;
    }

    خالی computeIfaceVarSyms() {
        برائے (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ثابت varp = varSymp ? VN_AS(varSymp->nodep(), Var) : خالی_اشاریہ;
            UINFO(9, "  insAllIface se" << cvt_مقصد_Hex(varSymp) << " " << varp);
            AstIfaceRefDType* ثابت ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            اگر (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_عدد() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warn_مزید()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            اگر (!ifacerefp->iface_بذریعہ_Cellp()) {
                اگر (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_گرہ::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_گرہ::prettyNameQ(ifacerefp->ifaceName()));
                    جاری;
                } ورنہ {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } ورنہ اگر (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->iface_بذریعہ_Cellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ثابت منطقی isDead = ifacerefp->iface_بذریعہ_Cellp()->dead();
                اگر (isDead && forPrimary() && !varp->is_Iface_والد()
                    && !v3Global.opt.چوٹی_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ثابت منطقی repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                اگر (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_گرہ::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warn_مزید()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warn_مزید()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    جاری;
                }
            } ورنہ اگر (ifacerefp->iface_بذریعہ_Cellp()->dead()
                       || !exists_گرہ_Sym(ifacerefp->iface_بذریعہ_Cellp())) {
                ifacerefp->iface_بذریعہ_Cellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ثابت ifaceSymp = get_گرہ_Sym(ifacerefp->iface_بذریعہ_Cellp());
            VSymEnt* if_Or_پورٹ_Symp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            اگر (ifacerefp->isModport()) {
                VSymEnt* ثابت foundp = ifaceSymp->ڈھونڈو_Id_Fallback(ifacerefp->modportName());
                منطقی ok = جھوٹ;
                اگر (foundp) {
                    اگر (AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        if_Or_پورٹ_Symp = foundp;
                        ok = سچ;
                    }
                }
                اگر (!ok) {
                    ثابت string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_گرہ_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.خالی_خانے() ? "" : ifacerefp->warn_مزید() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            داخل_کرو_Scope_Alias(SAMN_IFTOP, varSymp, if_Or_پورٹ_Symp);
        }
        m_ifaceVarSyms.clear();
    }

    خالی داخل_کرو_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvt_مقصد_Hex(lhsp) << " se" << cvt_مقصد_Hex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    خالی computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        برائے (صحیح samn = 0; samn < SAMN__MAX; ++samn) {
            برائے (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].انتہا(); ++it) {
                VSymEnt* ثابت lhsp = it->پہلا;
                VSymEnt* srcp = lhsp;
                جب (سچ) {  // Follow chain of aliases up to highest level non-alias
                    ثابت خودکار it2 = m_scopeAliasMap[samn].ڈھونڈو(srcp);
                    اگر (it2 != m_scopeAliasMap[samn].انتہا()) {
                        srcp = it2->دوسرا;
                        جاری;
                    } ورنہ {
                        توڑو;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                اگر (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), سچ);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

نجی:
    VSymEnt* ڈھونڈو_With_Alt_Fallback(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->ڈھونڈو_Id_Fallback(name);
        اگر (findp) واپس findp;
        اگر (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->ڈھونڈو_Id_Fallback(altname);
        }
        واپس findp;
    }

    VSymEnt* ڈھونڈو_With_Alt_Flat(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->ڈھونڈو_Id_Flat(name);
        اگر (findp) واپس findp;
        اگر (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->ڈھونڈو_Id_Flat(altname);
        }
        واپس findp;
    }

عوامی:
    VSymEnt* ڈھونڈو_Dotted(File_سطر* refLocationp, VSymEnt* lookupSymp, ثابت string& dotname,
                        string& baddot, VSymEnt*& okSymp, منطقی پہلا_Id) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvt_مقصد_Hex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        جب (leftname != "") {  // foreach dotted part of xref name
            string::سائز_type pos = leftname.ڈھونڈو('.');
            string ident;
            اگر (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } ورنہ {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            اگر (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                جب (VString::سرے_With(altIdent, "__KET__")) {
                    ثابت خودکار braPos = altIdent.rfind("__BRA__");
                    اگر (braPos == string::npos) توڑو;
                    altIdent = altIdent.substr(0, braPos);
                }
                اگر (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            اگر (پہلا_Id) {
                // Check this module - subcellnames
                ثابت AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : خالی_اشاریہ;  // Replicated below
                اگر (VSymEnt* ثابت ڈھونڈو_Symp = ڈھونڈو_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = ڈھونڈو_Symp;
                }

                // Check this module - cur modname
                ورنہ اگر (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                ورنہ اگر (ident == "$root") {
                    lookupSymp = جڑ_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    اگر (forScopeCreation()) {
                        lookupSymp = lookupSymp->ڈھونڈو_Id_Flat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                ورنہ {
                    منطقی crossedCell = جھوٹ;  // Crossed a cell boundary
                    جب (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : خالی_اشاریہ;  // Replicated above
                        اگر (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            اگر (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = سچ;
                            }
                            اگر (cellp && cellp->modp()->origName() == ident) {
                                توڑو;
                            } ورنہ اگر (VSymEnt* ثابت ڈھونڈو_Sym2p
                                       = ڈھونڈو_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = ڈھونڈو_Sym2p;
                                اگر (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    واپس خالی_اشاریہ;
                                }
                                توڑو;
                            }
                        } ورنہ {
                            توڑو;
                        }
                    }
                    اگر (!lookupSymp) واپس خالی_اشاریہ;  // Not found
                }
            } ورنہ {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* ڈھونڈو_Symp = ڈھونڈو_With_Alt_Flat(lookupSymp, ident, altIdent);
                اگر (ڈھونڈو_Symp) {
                    lookupSymp = ڈھونڈو_Symp;
                } ورنہ {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    ڈھونڈو_Symp = ڈھونڈو_Sym_Prefixed(lookupSymp, ident, baddot, /*fallback=*/سچ);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (ڈھونڈو_Symp ? "found" : "not found") << "\n");
                    اگر (ڈھونڈو_Symp) {
                        // Check if this is an interface reference variable
                        ثابت AstVar* varp = VN_CAST(ڈھونڈو_Symp->nodep(), Var);
                        اگر (!varp) {
                            اگر (ثابت AstVarScope* vscp = VN_CAST(ڈھونڈو_Symp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        اگر (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = ڈھونڈو_Symp;
                        } ورنہ {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            اگر (VN_IS(ڈھونڈو_Symp->nodep(), Cell)) {
                                lookupSymp = ڈھونڈو_Symp;
                            } ورنہ {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                واپس خالی_اشاریہ;
                            }
                        }
                    } ورنہ {
                        واپس خالی_اشاریہ;  // Not found
                    }
                }
            }
            اگر (lookupSymp) {
                اگر (ثابت AstCell* ثابت cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    اگر (ثابت Ast_گرہ_Module* ثابت modp = cellp->modp()) {
                        اگر (modp->hier_بلاک() && !leftname.خالی_خانے()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } ورنہ اگر (VN_IS(modp, Not_ملا_Module)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                اگر (!leftname.خالی_خانے()) {
                    ثابت خودکار aliasIt = m_scopeAliasMap[SAMN_IFTOP].ڈھونڈو(lookupSymp);
                    اگر (aliasIt != m_scopeAliasMap[SAMN_IFTOP].انتہا()) {
                        lookupSymp = aliasIt->دوسرا;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        اگر (ثابت AstVarScope* ثابت vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ثابت string varName = vscp->varp()->name();
                            ساکن constexpr ثابت حرف* ثابت VIFTOP_SUFFIX = "__Viftop";
                            اگر (VString::سرے_With(varName, VIFTOP_SUFFIX)) {
                                ثابت string cellName
                                    = varName.substr(0, varName.سائز() - لڑی_لمبائی(VIFTOP_SUFFIX));
                                VSymEnt* ثابت والد_Symp = lookupSymp->parentp();
                                اگر (والد_Symp) {
                                    VSymEnt* ثابت cellSymp = والد_Symp->ڈھونڈو_Id_Flat(cellName);
                                    اگر (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } ورنہ {
                        // No alias; try following IfaceRefDType to interface cell
                        ثابت AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        اگر (!varp) {
                            اگر (ثابت AstVarScope* ثابت vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        اگر (varp && varp->isIfaceRef()) {
                            اگر (ثابت AstIfaceRefDType* ثابت ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                اگر (ifaceRefp->cellp() && exists_گرہ_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_گرہ_Sym(ifaceRefp->cellp());
                                } ورنہ اگر (ifaceRefp->iface_بذریعہ_Cellp()
                                           && exists_گرہ_Sym(ifaceRefp->iface_بذریعہ_Cellp())) {
                                    lookupSymp = get_گرہ_Sym(ifaceRefp->iface_بذریعہ_Cellp());
                                } ورنہ اگر (ifaceRefp->ifacep()
                                           && exists_گرہ_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_گرہ_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            پہلا_Id = جھوٹ;
        }
        واپس lookupSymp;
    }

    ساکن string remove_آخری_Inline_Scope(ثابت string& name) {
        string برآمد = name;
        ثابت string dot = "__DOT__";
        ثابت سائز_t dotPos = برآمد.rfind(dot, برآمد.سائز() - dot.لمبائی_2() - 1);
        اگر (dotPos == string::npos) {
            واپس "";
        } ورنہ {
            واپس برآمد.erase(dotPos + dot.لمبائی_2(), string::npos);
        }
    }

    VSymEnt* ڈھونڈو_Sym_Prefixed(VSymEnt* lookupSymp, ثابت string& dotname, string& baddot,
                             منطقی fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        اگر (!lookupSymp) واپس خالی_اشاریہ;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvt_مقصد_Hex(lookupSymp)
                     << ((lookupSymp->sym_سابقہ() == "") ? "" : " as ")
                     << ((lookupSymp->sym_سابقہ() == "") ? "" : lookupSymp->sym_سابقہ() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string سابقہ = lookupSymp->sym_سابقہ();
        VSymEnt* foundp = خالی_اشاریہ;
        جب (!foundp) {
            اگر (fallback) {
                foundp = lookupSymp->ڈھونڈو_Id_Fallback(سابقہ + dotname);  // Might be nullptr
            } ورنہ {
                foundp = lookupSymp->ڈھونڈو_Id_Flat(سابقہ + dotname);  // Might be nullptr
            }
            اگر (سابقہ.خالی_خانے()) توڑو;
            ثابت string اگلا_سابقہ = remove_آخری_Inline_Scope(سابقہ);
            اگر (سابقہ == اگلا_سابقہ) توڑو;
            سابقہ = اگلا_سابقہ;
        }
        اگر (!foundp) baddot = dotname;
        واپس foundp;
    }
    ساکن منطقی checkIfClassOrPackage(ثابت VSymEnt* ثابت symp) {
        اگر (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) واپس سچ;

        // Helper: check if a RefDType might resolve to a class later.
        ثابت خودکار isPotentialClassRef = [](ثابت AstRefDType* refp) -> منطقی {
            واپس refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ثابت خودکار is_Valid_Type_گرہ = [](ثابت Ast_گرہ* nodep) -> منطقی {
            واپس VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        اگر (ثابت AstTypedef* ثابت typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ثابت Ast_گرہ_DType* dtypep = typedefp->subDTypep();
            اگر (!dtypep) dtypep = typedefp->بچہ_DTypep();
            اگر (VN_IS(dtypep, ClassRefDType)) واپس سچ;
            اگر (isPotentialClassRef(VN_CAST(dtypep, RefDType))) واپس سچ;
        } ورنہ اگر (ثابت AstParamTypeDType* ثابت paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_گرہ* childp = paramTypep->بچہ_DTypep();
            اگر (ثابت AstRequireDType* ثابت reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ثابت Ast_گرہ* ثابت checkp = childp ? childp : paramTypep->skipRefp();
            اگر (is_Valid_Type_گرہ(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                واپس سچ;
            }
        }
        واپس جھوٹ;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, منطقی fallback,
                                   منطقی classOnly, ثابت string& forWhat,
                                   منطقی deferIfUnresolved = جھوٹ) {
        اگر (nodep->classOrPackageSkipp()) واپس get_گرہ_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* تلاش_Symp = lookSymp;
        اگر (VL_UNCOVERABLE(تلاش_Symp && VN_IS(تلاش_Symp->nodep(), ParamTypeDType))) {
            تلاش_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        اگر (fallback) {
            VSymEnt* currentLookSymp = تلاش_Symp;
            کرو {
                foundp = currentLookSymp->ڈھونڈو_Id_Flat(nodep->name());
                اگر (foundp && !checkIfClassOrPackage(foundp)) foundp = خالی_اشاریہ;
                اگر (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } جب (!foundp && currentLookSymp);
        } ورنہ {
            foundp = تلاش_Symp->ڈھونڈو_Id_Flat(nodep->name());
            اگر (foundp && !checkIfClassOrPackage(foundp)) foundp = خالی_اشاریہ;
        }
        اگر (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_گرہ_Sym(v3Global.rootp()->stdPackagep())->ڈھونڈو_Id_Flat(nodep->name());
        }
        اگر (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            واپس foundp;
        }
        اگر (deferIfUnresolved) واپس خالی_اشاریہ;
        ثابت string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_گرہ_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest));
        واپس خالی_اشاریہ;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ثابت string& name,
                              ثابت VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        اگر (!lookupSymp) واپس "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        واپس speller.بہترین_Candidate_پیغام(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ثابت string& name, ثابت VNodeMatcher& matcher,
                          منطقی includeType) {
        اگر (!lookupSymp) واپس "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        واپس speller.بہترین_Candidate_پیغام(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = خالی_اشاریہ;

//======================================================================

جماعت Link_Dot_ڈھونڈو_Visitor final : عوامی VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3_اندر_Use m_inuser3;

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_گرہ_Module* m_classOrPackagep = خالی_اشاریہ;  // Current package
    AstClocking* m_clockingp = خالی_اشاریہ;  // Current clocking block
    std::سمتیہ<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = خالی_اشاریہ;  // Symbol Entry for current module
    VSymEnt* m_curSymp = خالی_اشاریہ;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ثابت Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    منطقی m_اندر_Recursion = جھوٹ;  // Inside a recursive module
    صحیح m_param_عدد = 0;  // Parameter number, for position based connection
    منطقی m_explicitNew = جھوٹ;  // Hit a "new" function
    صحیح m_mod_بلاک_عدد = 0;  // Begin block number in module, 0=none seen
    صحیح m_mod_With_عدد = 0;  // With block number, 0=none seen
    صحیح m_mod_Arg_عدد = 0;  // Arg block number for randomize(), 0=none seen
    std::سمتیہ<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    خالی makeImplicitNew(AstClass* nodep) {
        AstFunc* ثابت newp = نیا AstFunc{nodep->fileline(), "new", خالی_اشاریہ, خالی_اشاریہ};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(سچ);
        nodep->جوڑو_Membersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->داخل_کرو_بلاک(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ساکن منطقی is_Hier_بلاک_Wrapper(ثابت string& name) {
        ثابت V3_Hier_بلاک_Opt_سیٹ& hier_بلاکیں = v3Global.opt.hier_بلاکیں();
        واپس hier_بلاکیں.ڈھونڈو(name) != hier_بلاکیں.انتہا();
    }

    خالی moveExternFuncDecl(Ast_گرہ_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        اگر (nodep->isExternDef()) واپس;
        // Move it to proper spot under the target class
        nodep->unlink_Fr_عقب();
        classp->جوڑو_Stmtsp(nodep);
        nodep->isExternDef(سچ);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlink_Fr_عقب()->delete_درخت();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        اگر (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    خالی moveExternFuncDeclRefs(Ast_گرہ* nodep, AstClass* classp) {
        nodep->foreach([یہ, classp](AstClassOrPackageRef* refp) {  //
            اگر (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ثابت dotp = VN_CAST(refp->backp(), Dot);
                اگر (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlink_Fr_عقب());
                    VL_DO_DANGLING2(ڈالو_Deletep(dotp), dotp, refp);
                } ورنہ {
                    VL_DO_DANGLING(ڈالو_Deletep(refp->unlink_Fr_عقب()), refp);
                }
            }
        });
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->داخل_کرو_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_گرہ_Module* ثابت modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        اگر (m_statep->forPrimary()) {
            ثابت منطقی onlyBuiltins = [&]() {
                برائے (Ast_گرہ_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), گرہ_Module)) {
                    اگر (!modp->isDollarUnit() && !modp->isConstPool()) واپس جھوٹ;
                }
                واپس سچ;
            }();
            اگر (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        برائے (Ast_گرہ_Module* modp = nodep->modulesp(); modp && modp->is_چوٹی();
             modp = VN_AS(modp->nextp(), گرہ_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            اگر (!m_statep->forScopeCreation() && v3Global.opt.چوٹی_Ifaces_Supported()) {
                برائے (Ast_گرہ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    اگر (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                        اگر (varp->isIfaceRef()) {
                            ثابت Ast_گرہ_DType* ثابت subtypep = varp->subDTypep();
                            ثابت AstIfaceRefDType* ifacerefp = خالی_اشاریہ;
                            اگر (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } ورنہ اگر (VN_IS(subtypep, BracketArrayDType)) {
                                ثابت AstBracketArrayDType* ثابت arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ثابت Ast_گرہ_DType* ثابت arrsubtypep = arrp->subDTypep();
                                اگر (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } ورنہ اگر (VN_IS(subtypep, UnpackArrayDType)) {
                                ثابت AstUnpackArrayDType* ثابت arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ثابت Ast_گرہ_DType* ثابت arrsubtypep = arrp->subDTypep();
                                اگر (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            اگر (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = نیا AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  خالی_اشاریہ,
                                                  خالی_اشاریہ,
                                                  خالی_اشاریہ};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->داخل_کرو_چوٹی_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ڈالو_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->داخل_کرو_چوٹی_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = خالی_اشاریہ;
        }
    }
    خالی دورہ(Ast_Type_جدول*) override {}  // FindVisitor::
    خالی دورہ(AstIfaceRefDType* nodep) override {  // FindVisitor::
        اگر (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ڈالو_عقب(nodep->ifacep());
            nodep->ifacep()->user3(سچ);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ثابت منطقی standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ثابت منطقی doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_عدد);
        VL_RESTORER(m_mod_بلاک_عدد);
        VL_RESTORER(m_mod_With_عدد);
        VL_RESTORER(m_mod_Arg_عدد);
        اگر (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_گرہ::prettyNameQ(nodep->origName()));
        } ورنہ اگر (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ثابت pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            اگر (standalonePkg) {
                اگر (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } ورنہ {
                    VSymEnt* ثابت بالائی_Symp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->داخل_کرو_بلاک(بالائی_Symp, nodep->name(), nodep,
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
            nodep->user2(سچ);
            iterateChildren(nodep);
            nodep->user2(جھوٹ);
            nodep->user4(سچ);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            اگر (!m_statep->forScopeCreation()) {
                اگر (AstIface* ثابت ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->داخل_کرو_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } ورنہ اگر (is_Hier_بلاک_Wrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ثابت بالائی_Symp = m_curSymp ? m_curSymp : m_statep->جڑ_Entp();
            m_curSymp = m_modSymp
                = m_statep->داخل_کرو_بلاک(بالائی_Symp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(سچ);
        } ورنہ {  // !doit
            اگر (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                برائے (ثابت Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (ثابت AstTypedef* ثابت tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ثابت بالائی_Symp = m_curSymp ? m_curSymp : m_statep->جڑ_Entp();
                        m_curSymp = m_modSymp = m_statep->داخل_کرو_بلاک(بالائی_Symp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } ورنہ {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    خالی دورہ(AstClass* nodep) override {  // FindVisitor::
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
            VSymEnt* ثابت بالائی_Symp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->داخل_کرو_بلاک(بالائی_Symp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->داخل_کرو_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_عدد = 0;
            m_mod_بلاک_عدد = 0;
            m_mod_With_عدد = 0;
            m_mod_Arg_عدد = 0;
            m_explicitNew = جھوٹ;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(سچ);
            // Implicit new needed?
            اگر (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        اگر (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        اگر (!nodep->is_چوٹی()) {
            ++m_statScopeVisits;
            nodep->unlink_Fr_عقب();
            m_scopes.ڈالو_عقب(nodep);
        }
    }
    خالی دورہ(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        اگر (nodep->recursive() && m_اندر_Recursion) واپس;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_عدد);
        VL_RESTORER(m_اندر_Recursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ثابت string origname = Ast_گرہ::dedotName(nodep->name());
        string::سائز_type pos = origname.rfind('.');
        اگر (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ثابت string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->ڈھونڈو_Dotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, جھوٹ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_گرہ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->داخل_کرو_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_اندر_Recursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            اگر (nodep->modp()) iterate(nodep->modp());
        }
    }
    خالی دورہ(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ثابت std::string::سائز_type pos = ident.rfind("__DOT__");
        اگر (pos != std::string::npos) {
            ثابت std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::لڑی_لمبائی("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->ڈھونڈو_Dotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             جھوٹ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_گرہ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->داخل_کرو_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    خالی دورہ(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    خالی دورہ(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Gen_بلاک* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        اگر (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            برائے (Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                اگر (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ثابت std::string قدم_Str = m_statep->forPrimary()
                                                    ? ""
                                                    : std::مقصد_string(m_statep->قدم_نمبر()) + "_";
                    کرو {
                        ++m_mod_بلاک_عدد;
                        name = "unnamedblk" + قدم_Str + cvt_مقصد_Str(m_mod_بلاک_عدد);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } جب (m_curSymp->ڈھونڈو_Id_Flat(name));
                    nodep->name(name);
                    توڑو;
                }
            }
        }
        اگر (nodep->name() == "") {
            iterateChildren(nodep);
        } ورنہ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->داخل_کرو_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_پچھلا(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    خالی دورہ(Ast_گرہ_بلاک* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        اگر (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ثابت خودکار شامل_ہے_Decl = [](ثابت Ast_گرہ* nodesp) -> منطقی {
                برائے (ثابت Ast_گرہ* np = nodesp; np; np = np->nextp()) {
                    اگر (VN_IS(np, Var) || VN_IS(np, Foreach)) واپس سچ;
                }
                واپس جھوٹ;
            };
            منطقی needName = nodep->declsp() || شامل_ہے_Decl(nodep->stmtsp());
            اگر (ثابت AstFork* ثابت forkp = VN_CAST(nodep, Fork)) {
                needName = needName || شامل_ہے_Decl(forkp->forksp());
            }
            اگر (needName) {
                ثابت std::string قدم_Str
                    = m_statep->forPrimary() ? "" : std::مقصد_string(m_statep->قدم_نمبر()) + "_";
                std::string name;
                کرو {
                    ++m_mod_بلاک_عدد;
                    name = "unnamedblk" + قدم_Str + cvt_مقصد_Str(m_mod_بلاک_عدد);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } جب (m_curSymp->ڈھونڈو_Id_Flat(name));
                nodep->name(name);
            }
        }
        اگر (nodep->name() == "") {
            iterateChildren(nodep);
        } ورنہ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->داخل_کرو_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_پچھلا(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    خالی moveIfaceExportBody(Ast_گرہ_FTask* nodep) {
        ثابت string& پورٹ_Name = nodep->iface_پورٹ_Name();
        UINFO(5, "   moveIfaceExportBody: port=" << پورٹ_Name << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ثابت پورٹ_Symp = m_modSymp->ڈھونڈو_Id_Fallback(پورٹ_Name);
        اگر (!پورٹ_Symp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << پورٹ_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        AstVar* ثابت portp = VN_CAST(پورٹ_Symp->nodep(), Var);
        اگر (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << پورٹ_Name);
            واپس;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = خالی_اشاریہ;
        اگر (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        اگر (!ifaceRefDtp && portp->بچہ_DTypep()) {
            ifaceRefDtp = VN_CAST(portp->بچہ_DTypep(), IfaceRefDType);
        }
        اگر (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << پورٹ_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        ثابت string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_گرہ_Module* ثابت ifaceModp = m_statep->ڈھونڈو_Module_Sym(ifaceName);
        اگر (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            واپس;
        }
        // Find the prototype task in the interface
        Ast_گرہ_FTask* protoTaskp = خالی_اشاریہ;
        برائے (Ast_گرہ* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            اگر (Ast_گرہ_FTask* ثابت ftaskp = VN_CAST(itemp, گرہ_FTask)) {
                اگر (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    توڑو;
                }
            }
        }
        اگر (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_گرہ* ثابت nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            اگر (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isIO()) {
                    stmtp = nextp;
                    جاری;
                }
            }
            // Clone this statement and rewrite port references
            Ast_گرہ* ثابت newStmtp = stmtp->clone_درخت(جھوٹ);
            rewrite_Iface_پورٹ_Refs_Single(newStmtp, پورٹ_Name);
            protoTaskp->جوڑو_Stmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(جھوٹ);
        // Delete the out-of-block task from the module
        nodep->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ساکن خالی rewrite_Iface_پورٹ_Refs_Single(Ast_گرہ* nodep, ثابت string& پورٹ_Name) {
        اگر (!nodep) واپس;
        // Check if this is a non-colon Dot with lhs matching portName
        اگر (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
            اگر (!dotp->colon()) {
                ثابت AstParseRef* ثابت lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                اگر (lhsRefp && lhsRefp->name() == پورٹ_Name) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_گرہ* ثابت rhsp = dotp->rhsp()->unlink_Fr_عقب();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_درخت(), dotp);
                    rewrite_Iface_پورٹ_Refs_Single(rhsp, پورٹ_Name);
                    واپس;
                }
            }
        }
        // Recurse into all children
        برائے (Ast_گرہ* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            جب (childp) {
                Ast_گرہ* ثابت nextp = childp->nextp();
                rewrite_Iface_پورٹ_Refs_Single(childp, پورٹ_Name);
                childp = nextp;
            }
        }
    }

    خالی دورہ(Ast_گرہ_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        اگر (nodep->name() == "new") m_explicitNew = سچ;
        // Handle out-of-block interface method definition (IEEE 25.8)
        اگر (!nodep->iface_پورٹ_Name().خالی_خانے() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            واپس;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* اوپر_Symp = m_curSymp;

        اگر (VN_IS(m_curSymp->nodep(), Class)) {
            اگر (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->خالص_Virtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            اگر (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        اگر (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ثابت dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            اگر (dotp) {
                AstClassOrPackageRef* ثابت lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, سچ, جھوٹ,
                                                "External definition :: reference");
                AstClass* ثابت lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                اگر (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ورنہ {
                    m_curSymp = m_statep->get_گرہ_Sym(lhsclassp);
                    اوپر_Symp = m_curSymp;
                    AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, سچ, جھوٹ,
                                                    "External definition :: reference");
                    AstClass* ثابت rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    اگر (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } ورنہ {
                        m_curSymp = m_statep->get_گرہ_Sym(rhsclassp);
                        اوپر_Symp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } ورنہ اگر (cpackagerefp) {
                اگر (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, سچ, جھوٹ,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                اگر (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ورنہ {
                    m_curSymp = m_statep->get_گرہ_Sym(classp);
                    اوپر_Symp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } ورنہ {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        اگر (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(اوپر_Symp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        اگر (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_گرہ_DType* dtypep = VN_CAST(nodep->fvarp(), گرہ_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            اگر (dtypep) {
                dtypep->unlink_Fr_عقب();
            } ورنہ {
                dtypep = نیا AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ثابت newvarp
                = نیا AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlag_بچہ_DType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(سچ);
            newvarp->trace(جھوٹ);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->داخل_کرو_Sym(m_curSymp, newvarp->name(), newvarp, خالی_اشاریہ /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterate_And_اگلا_Null(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = خالی_اشاریہ;
        اگر (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->داخل_کرو_بلاک(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(خالی_اشاریہ);
        }
        اگر (!nodep->name().خالی_خانے()) {
            itSymp = m_statep->داخل_کرو_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(خالی_اشاریہ);
        }
        اگر (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterate_And_اگلا_Null(nodep->itemsp());
        }
    }
    خالی دورہ(Ast_Clocking_شے* nodep) override {  // FindVisitor::
        اگر (nodep->varp()) {
            اگر (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            واپس;
        }
        std::string varname;
        Ast_گرہ_DType* dtypep;
        اگر (Ast_تفویض* ثابت assignp = nodep->assignp()) {
            Ast_گرہ_اظہاریہ* ثابت rhsp = assignp->rhsp()->unlink_Fr_عقب();
            dtypep = نیا AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_کا_اظہاریہ{},
                                     rhsp->clone_درخت(جھوٹ)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlink_Fr_عقب()->delete_درخت(), assignp);
        } ورنہ {
            Ast_گرہ_اظہاریہ* ثابت refp = nodep->exprp();
            ثابت VSymEnt* foundp = m_curSymp->ڈھونڈو_Id_Fallback(refp->name());
            اگر (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
                واپس;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->بچہ_DTypep()->clone_درخت(جھوٹ);
        }
        AstVar* ثابت newvarp = نیا AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlag_بچہ_DType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    خالی دورہ(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* اوپر_Symp = m_curSymp;
        اگر (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            اگر (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } ورنہ {
                اگر (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, سچ, جھوٹ,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                اگر (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ورنہ {
                    m_curSymp = m_statep->get_گرہ_Sym(classp);
                    اوپر_Symp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlink_Fr_عقب();
                    classp->جوڑو_Stmtsp(nodep);
                    nodep->classOrPackagep()->unlink_Fr_عقب()->delete_درخت();
                }
            }
        }
        // Set the class as package for iteration
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->داخل_کرو_بلاک(اوپر_Symp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        اگر (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        اگر (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ثابت VSymEnt* foundp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
            اگر (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : خالی_اشاریہ;
            // clocking items can have duplicate names (inout)
            اگر (findvarp && VN_IS(findvarp->backp(), Clocking_شے)
                && VN_IS(nodep->backp(), Clocking_شے)) {
                Ast_Clocking_شے* ثابت itemp = VN_AS(nodep->backp(), Clocking_شے);
                Ast_Clocking_شے* ثابت finditemp = VN_AS(findvarp->backp(), Clocking_شے);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                اگر (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::داخلہ
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = خالی_اشاریہ;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            منطقی ins = جھوٹ;
            اگر (!foundp) {
                ins = سچ;
            } ورنہ اگر (!findvarp && m_curSymp->ڈھونڈو_Id_Flat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::گرہ_متن_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } ورنہ اگر (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvt_مقصد_Hex(m_curSymp) << " ;; parent=se"
                                             << cvt_مقصد_Hex(foundp->parentp()));
                اگر (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    اگر (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        واپس;
                    }
                    ثابت منطقی nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ثابت منطقی ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    اگر (ansiBad || nansiBad) {
                        منطقی ansiWarn = ansiBad && !nansiBad;
                        اگر (ansiWarn) {
                            ساکن صحیح s_didAnsiWarn = جھوٹ;
                            اگر (s_didAnsiWarn++) ansiWarn = جھوٹ;
                        }
                        nodep->v3error("Duplicate declaration of signal: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (ansiWarn ? nodep->warn_مزید()
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
                    } ورنہ {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(سچ);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        اگر (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{سچ});
                        }
                        Ast_گرہ_DType* ثابت varDtp = findvarp->subDTypep();
                        Ast_گرہ_DType* ثابت otherDtp = nodep->subDTypep();
                        اگر (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlink_Fr_عقب()->delete_درخت(), varDtp);
                            findvarp->بچہ_DTypep(otherDtp->unlink_Fr_عقب());
                        } ورنہ اگر (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            Ast_Attr_کا* ثابت newp
                                = نیا Ast_Attr_کا{otherDtp->fileline(), VAttrType::VAR_پورٹ_DTYPE,
                                                otherDtp->unlink_Fr_عقب()};
                            findvarp->جوڑو_Attrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
                } ورنہ {
                    // User can disable the message at either point
                    اگر (!(m_ftaskp && m_ftaskp->dpiImport())
                        && (!m_ftaskp || m_ftaskp != foundp->nodep())  // Not the function's
                                                                       // variable hiding function
                        && !nodep->fileline()->warnIsOff(V3_Error_رمز::VARHIDDEN)
                        && !foundp->nodep()->fileline()->warnIsOff(V3_Error_رمز::VARHIDDEN)) {
                        nodep->v3warn(VARHIDDEN,
                                      "Declaration of signal hides declaration in upper scope: "
                                          << nodep->prettyNameQ() << '\n'
                                          << nodep->warnContextPrimary() << '\n'
                                          << foundp->nodep()->warnOther()
                                          << "... Location of original declaration\n"
                                          << foundp->nodep()->warnContextSecondary());
                    }
                    ins = سچ;
                }
            }
            اگر (ins) {
                اگر (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->جڑ_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    اگر (v3Global.opt.hasParameter(nodep->name())) {
                        ثابت string svalue = v3Global.opt.parameter(nodep->name());
                        اگر (AstConst* ثابت valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            اگر (nodep->valuep()) ڈالو_Deletep(nodep->valuep()->unlink_Fr_عقب());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3_Error_رمز::ENUMVALUE, سچ);
                        }
                    }
                }
                m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                اگر (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_عدد;
                    VSymEnt* ثابت symp
                        = m_statep->داخل_کرو_Sym(m_curSymp, "__paramNumber" + cvt_مقصد_Str(m_param_عدد),
                                              nodep, m_classOrPackagep);
                    symp->exported(جھوٹ);
                }
            }
        }
    }
    خالی دورہ(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        اگر (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(سچ);
        iterateChildren(nodep);
        m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    خالی دورہ(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, "__Vdtype" + cvt_مقصد_Str(nodep->unique_عدد()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    خالی دورہ(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        اگر (!m_hierParamsName.خالی_خانے()) {
            اگر (ثابت VSymEnt* ثابت typedefEntp = m_curSymp->ڈھونڈو_Id_Fallback(m_hierParamsName)) {
                ثابت AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                برائے (ثابت Ast_گرہ* stmtp = modp ? modp->stmtsp() : خالی_اشاریہ; stmtp;
                     stmtp = stmtp->nextp()) {
                    ثابت AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    اگر (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_گرہ_DType* ثابت newDtp = tdefp->بچہ_DTypep();
                        Ast_گرہ_DType* ثابت oldDtp = nodep->بچہ_DTypep();

                        oldDtp->replaceWith(newDtp->clone_درخت(جھوٹ));
                        VL_DO_DANGLING(oldDtp->delete_درخت(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        اگر (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_عدد;
            VSymEnt* ثابت symp = m_statep->داخل_کرو_Sym(
                m_curSymp, "__paramNumber" + cvt_مقصد_Str(m_param_عدد), nodep, m_classOrPackagep);
            symp->exported(جھوٹ);
        }
    }
    خالی دورہ(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    خالی دورہ(Ast_Enum_شے* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ثابت VSymEnt* foundp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
        اگر (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        Ast_Enum_شے* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), Enum_شے) : خالی_اشاریہ;
        منطقی ins = جھوٹ;
        اگر (!foundp) {
            ins = سچ;
        } ورنہ اگر (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            اگر (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } ورنہ {
                // User can disable the message at either point
                اگر (!nodep->fileline()->warnIsOff(V3_Error_رمز::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3_Error_رمز::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = سچ;
            }
        }
        اگر (ins) m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    خالی دورہ(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        اگر (!nodep->packagep()) واپس;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->get_گرہ_Sym(nodep->packagep());
        اگر (nodep->name() == "*") {
            اگر (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                اگر (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } ورنہ اگر (!nodep->resolvedClassp()) {
            VSymEnt* ثابت impp = srcp->ڈھونڈو_Id_Flat(nodep->name());
            اگر (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } ورنہ اگر (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } ورنہ اگر (AstClass* ثابت classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    نیا AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, خالی_اشاریہ});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    خالی دورہ(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        اگر (!nodep->packagep()) واپس;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->get_گرہ_Sym(nodep->packagep());
        اگر (nodep->name() != "*") {
            VSymEnt* ثابت impp = srcp->ڈھونڈو_Id_Flat(nodep->name());
            اگر (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    خالی دورہ(Ast_Package_Export_ستارہ_ستارہ* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->export_ستارہ_ستارہ(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    خالی دورہ(Ast_گرہ_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_عدد;
            m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, "__Vforeach" + cvt_مقصد_Str(m_mod_With_عدد),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_پچھلا(m_curSymp));
            برائے (Ast_گرہ *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = خالی_اشاریہ;
                اگر (AstParseRef* ثابت parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = نیا AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->ڈھونڈو_Int_DType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_درخت(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->داخل_کرو_Sym(m_curSymp, argrefp->name(), argrefp, خالی_اشاریہ);
                } ورنہ اگر (ثابت خودکار largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->داخل_کرو_Sym(m_curSymp, argrefp->name(), argrefp, خالی_اشاریہ);
                } ورنہ اگر (VN_IS(argp, خالی_خانے_2)) {
                } ورنہ {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    خالی دورہ(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        اگر (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        برائے (Ast_گرہ* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(سچ);
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_گرہ_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), گرہ_FTask_Ref);
        اگر (ثابت AstDot* ثابت dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), گرہ_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_سطر* argFl = nodep->fileline();
        AstArg* ثابت argsp = funcrefp->argsp();
        اگر (argsp) {
            argsp->unlink_Fr_عقب_With_اگلا();
            اگر (funcrefp->name() != "randomize") {
                اگر (ثابت خودکار parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } ورنہ {
                    argsp->v3error("'with' function expects simple variable name");
                }
                اگر (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_درخت(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        اگر (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->جوڑو_Argsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlink_Fr_عقب());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            واپس;
        }
        ثابت منطقی restrictedRandomize = nodep->restricted();
        اگر (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ثابت اشاریہ_Arg_Refp
                = نیا AstLambdaArgRef{argFl, name + "__DOT__index", سچ};
            AstLambdaArgRef* ثابت قدر_Arg_Refp = نیا AstLambdaArgRef{argFl, name, جھوٹ};
            AstWith* ثابت newp
                = نیا AstWith{nodep->fileline(), اشاریہ_Arg_Refp, قدر_Arg_Refp, خالی_اشاریہ};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            اگر (restrictedRandomize) {
                newp->restricted(سچ);
                جب (Ast_گرہ* ثابت itemp = nodep->exprsp()) {
                    newp->جوڑو_Restricted_Name(VN_AS(itemp, ParseRef)->name());
                    itemp->unlink_Fr_عقب();
                    VL_DO_DANGLING(ڈالو_Deletep(itemp), itemp);
                }
            }
            Ast_گرہ* اظہاریہ_Or_Constraintsp = خالی_اشاریہ;
            اگر (nodep->exprsp()) اظہاریہ_Or_Constraintsp = nodep->exprsp()->unlink_Fr_عقب_With_اگلا();
            اگر (nodep->constraintsp())
                اظہاریہ_Or_Constraintsp = Ast_گرہ::جوڑو_اگلا(
                    اظہاریہ_Or_Constraintsp, nodep->constraintsp()->unlink_Fr_عقب_With_اگلا());
            newp->جوڑو_Exprp(اظہاریہ_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->جوڑو_Argsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlink_Fr_عقب());
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_عدد;
        m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, "__Vwith" + cvt_مقصد_Str(m_mod_With_عدد), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_پچھلا(m_curSymp));
        UASSERT_OBJ(nodep->اشاریہ_Arg_Refp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->قدر_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->داخل_کرو_Sym(m_curSymp, nodep->قدر_Arg_Refp()->name(), nodep->قدر_Arg_Refp(),
                            خالی_اشاریہ);
        iterateChildren(nodep);
    }
    خالی دورہ(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterate_And_اگلا_Null(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_عدد;
        m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, "__VcoverWith" + cvt_مقصد_Str(m_mod_With_عدد),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_پچھلا(m_curSymp));
        iterate_And_اگلا_Null(nodep->itemp());
        iterate_And_اگلا_Null(nodep->filterp());
    }
    خالی دورہ(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterate_And_اگلا_Null(nodep->selectp());
        iterate_And_اگلا_Null(nodep->countp());
        برائے (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterate_And_اگلا_Null(varp->بچہ_DTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_عدد;
        m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, "__VcoverWith" + cvt_مقصد_Str(m_mod_With_عدد),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_پچھلا(m_curSymp));
        برائے (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->داخل_کرو_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterate_And_اگلا_Null(nodep->filterp());
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    خالی handleUnvisitedVirtIfaces() {
        Ast_گرہ_Module* ثابت چوٹی = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_گرہ_Sym(چوٹی);
        برائے (AstIface* ifacep : m_virtIfaces) {
            اگر (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ثابت ifacecellp = نیا AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        خالی_اشاریہ,
                                                        خالی_اشاریہ,
                                                        خالی_اشاریہ};
                ifacecellp->modp(ifacep);
                چوٹی->جوڑو_Stmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

عوامی:
    // CONSTRUCTORS
    Link_Dot_ڈھونڈو_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        اگر (!m_virtIfaces.خالی_خانے()) handleUnvisitedVirtIfaces();
        برائے (AstScope* ثابت scopep : m_scopes) scopep->modp()->جوڑو_Stmtsp(scopep);
        اگر (m_statep->forScopeCreation()) {
            V3Stats::جوڑو_Stat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~Link_Dot_ڈھونڈو_Visitor() override = طےشدہ;
};

//======================================================================

جماعت Link_Dot_ڈھونڈو_Iface_Visitor final : عوامی VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_گرہ* m_declp = خالی_اشاریہ;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ثابت VSymEnt* ڈھونڈو_Non_Decl_Sym(VSymEnt* symp, ثابت string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        جب (symp) {
            ثابت VSymEnt* ثابت foundp = symp->ڈھونڈو_Id_Flat(name);
            اگر (foundp && foundp->nodep() != m_declp) واپس foundp;
            symp = symp->fallbackp();
        }
        واپس خالی_اشاریہ;
    }

    // VISITORS
    خالی دورہ(AstRefDType* nodep) override {  // FindIfaceVisitor::
        اگر (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            اگر (m_statep->exists_گرہ_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_گرہ_Sym(nodep);
                ثابت VSymEnt* foundp = ڈھونڈو_Non_Decl_Sym(symp, nodep->name());
                Ast_گرہ* ملا_Nodep = خالی_اشاریہ;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                اگر (foundp) ملا_Nodep = foundp->nodep();
                اگر (!ملا_Nodep) ملا_Nodep = m_statep->ڈھونڈو_Module_Sym(nodep->name());
                اگر (ملا_Nodep) UINFO(9, "    Ifc foundNodep " << ملا_Nodep);
                اگر (AstIface* ثابت defp = VN_CAST(ملا_Nodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ثابت newp
                        = نیا AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    اگر (nodep->paramsp())
                        newp->جوڑو_Paramsp(nodep->paramsp()->unlink_Fr_عقب_With_اگلا());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ثابت ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        اگر (ifacerefp && m_statep->exists_گرہ_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            اگر (!ifacerefp->isVirtual()) nodep->سیٹ_Iface_Ref();
            VSymEnt* ثابت symp = m_statep->get_گرہ_Sym(nodep);
            m_statep->داخل_کرو_Iface_Var_Sym(symp);
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

عوامی:
    // CONSTRUCTORS
    Link_Dot_ڈھونڈو_Iface_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~Link_Dot_ڈھونڈو_Iface_Visitor() override = طےشدہ;
};

//======================================================================

جماعت LinkDotParamVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module

    خالی pin_Implicit_اظہاریہ_Recurse(Ast_گرہ* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        اگر (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } ورنہ اگر (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicit_Ok_جوڑو(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        ورنہ {
            اگر (Ast_گرہ* ثابت refp = nodep->op1p()) pin_Implicit_اظہاریہ_Recurse(refp);
            اگر (Ast_گرہ* ثابت refp = nodep->op2p()) pin_Implicit_اظہاریہ_Recurse(refp);
            اگر (Ast_گرہ* ثابت refp = nodep->op3p()) pin_Implicit_اظہاریہ_Recurse(refp);
            اگر (Ast_گرہ* ثابت refp = nodep->op4p()) pin_Implicit_اظہاریہ_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        اگر (Ast_گرہ* ثابت refp = nodep->nextp()) pin_Implicit_اظہاریہ_Recurse(refp);
    }

    // VISITORS
    خالی دورہ(Ast_Type_جدول*) override {}  // ParamVisitor::
    خالی دورہ(Ast_گرہ_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        اگر ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(سچ);
            واپس;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        اگر (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_اظہاریہ_Recurse(nodep->exprp());
        }
    }
    خالی دورہ(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warn_مزید()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        منطقی hasPartSelect = جھوٹ;
        ثابت string راستہ = نکالو_Dotted_راستہ(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !راستہ.خالی_خانے(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = خالی_اشاریہ;
        VSymEnt* ثابت foundp = m_statep->ڈھونڈو_Dotted(
            nodep->fileline(), m_statep->get_گرہ_Sym(nodep), راستہ, baddot, okSymp, سچ);
        AstCell* ثابت cellp = foundp ? VN_AS(foundp->nodep(), Cell) : خالی_اشاریہ;
        اگر (!cellp) {
            nodep->v3error("In defparam, instance " << راستہ << " never declared");
        } ورنہ {
            Ast_گرہ_اظہاریہ* ثابت exprp = nodep->rhsp()->unlink_Fr_عقب();
            UINFO(9, "Defparam cell " << راستہ << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ثابت pinp = نیا AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(سچ);
            pinp->param_راستہ(راستہ);
            cellp->جوڑو_Paramsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_پورٹ* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ثابت foundp = m_statep->get_گرہ_Sym(m_modp)->ڈھونڈو_Id_Flat(nodep->name());
        AstVar* ثابت refp = foundp ? VN_CAST(foundp->nodep(), Var) : خالی_اشاریہ;
        اگر (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } ورنہ اگر (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } ورنہ {
            اگر (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(سچ);
            VSymEnt* ثابت symp = m_statep->داخل_کرو_Sym(m_statep->get_گرہ_Sym(m_modp),
                                                      "__pinNumber" + cvt_مقصد_Str(nodep->pin_عدد()),
                                                      refp, خالی_اشاریہ /*classOrPackagep*/);
            symp->exported(جھوٹ);
            refp->pin_عدد(nodep->pin_عدد());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->جوڑو_یہاں_This_As_اگلا(refp->unlink_Fr_عقب());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_تفویض_W* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_اظہاریہ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    خالی دورہ(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_اظہاریہ_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        اگر (خودکار* ثابت fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ثابت foundp = m_statep->get_گرہ_Sym(fwdp)->ڈھونڈو_Id_Fallback(fwdp->name());
            اگر (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), گرہ_Module));
            } ورنہ اگر (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } ورنہ {
                اگر (foundp) UINFO(1, "found nodep = " << foundp->nodep());
                nodep->v3error(
                    "Forward typedef used as class/package does not resolve to class/package: "
                    << nodep->prettyNameQ() << '\n'
                    << nodep->warnContextPrimary() << '\n'
                    << (foundp ? nodep->warn_مزید() + "... Object with matching name\n"
                                     + foundp->nodep()->warnContextSecondary()
                               : ""));
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_اظہاریہ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    خالی دورہ(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ثابت foundp = m_statep->get_گرہ_Sym(nodep)->ڈھونڈو_Id_Fallback(nodep->name());
        اگر (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_عدد_سیٹ_Min(nodep->fileline()->token_عدد());
        } ورنہ اگر (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

عوامی:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = طےشدہ;
};

//======================================================================

جماعت LinkDotScopeVisitor final : عوامی VNVisitor {

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    ثابت AstScope* m_scopep = خالی_اشاریہ;  // The current scope
    VSymEnt* m_modSymp = خالی_اشاریہ;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::سمتیہ<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
عوامی:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ساکن AstVarScope* getAlias(AstVarScope* ثابت vscp) {
        اگر (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ثابت aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            واپس aliasp;
        } ورنہ {
            واپس vscp;
        }
    }

نجی:
    // Alias an interface reference to the interface it is connected to
    خالی سیٹ_Iface_Alias(AstVarScope* ثابت vscp, AstVarScope* ثابت aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    خالی سیٹ_Var_Alias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        اگر (ap == bp) واپس;
        ثابت منطقی aWins = [&]() {
            ثابت AstVar* ثابت aVarp = ap->varp();
            ثابت AstVar* ثابت bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            اگر (bVarp->isPrimaryIO()) واپس جھوٹ;
            اگر (aVarp->isPrimaryIO()) واپس سچ;
            // A forced or public variable must keep its own storage. If both are, either works.
            اگر (aVarp->isForced() != bVarp->isForced()) واپس aVarp->isForced();
            اگر (aVarp->is_Sig_عوامی() != bVarp->is_Sig_عوامی()) واپس aVarp->is_Sig_عوامی();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            اگر (aVarp->direction() != VDirection::OUTPUT) واپس جھوٹ;
            // Is 'ap' strictly below 'bp' in the hierarchy
            برائے (ثابت AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                اگر (scopep == bp->scopep()) واپس سچ;
            }
            واپس جھوٹ;
        }();
        اگر (aWins) {
            bp->user2p(ap);
        } ورنہ {
            ap->user2p(bp);
        }
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    خالی دورہ(AstScope* nodep) override {  // ScopeVisitor::
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
        اگر (AstIface* ثابت ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->داخل_کرو_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstVarScope* nodep) override {  // ScopeVisitor::
        اگر (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ثابت varSymp
                = m_statep->داخل_کرو_Sym(m_modSymp, nodep->varp()->name(), nodep, خالی_اشاریہ);
            اگر (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ثابت dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                اگر (nodep->varp()->is_Iface_والد()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ثابت string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->ڈھونڈو_Dotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, جھوٹ);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvt_مقصد_Hex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    اگر (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ثابت mpSymp = cellSymp->ڈھونڈو_Id_Fallback(dtypep->modportName());
                        UASSERT_OBJ(mpSymp, nodep,
                                    "No symbol for interface modport: "
                                        << nodep->prettyNameQ(dtypep->modportName()));
                        cellSymp = mpSymp;
                        UINFO(5, "       Found modport cell: se" << cvt_مقصد_Hex(cellSymp) << " "
                                                                 << mpSymp->nodep());
                    }
                    // Interface reference; need to put whole thing into
                    // symtable, but can't clone it now as we may have a later
                    // alias for it.
                    m_statep->داخل_کرو_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->داخل_کرو_بلاک(m_modSymp, nodep->name(), nodep, خالی_اشاریہ);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->داخل_کرو_بلاک(m_modSymp, nodep->name(), nodep, خالی_اشاریہ);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    خالی دورہ(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    خالی دورہ(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->داخل_کرو_بلاک(m_modSymp, nodep->name(), nodep, خالی_اشاریہ);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    خالی دورہ(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = خالی_اشاریہ;
        برائے (Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = خالی_اشاریہ;
            اگر (ثابت AstVarRef* ثابت refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } ورنہ {
                // Reaches into the scope of an instance, look it up by name
                ثابت AstVarXRef* ثابت xrefp = VN_AS(itemp, VarXRef);
                ثابت string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ثابت symp = m_statep->ڈھونڈو_Dotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, جھوٹ);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            اگر (aliasVscp) {
                سیٹ_Var_Alias(aliasVscp, vscp);
            } ورنہ {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_عقب(nodep, m_modSymp);
    }
    خالی processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ثابت refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ثابت xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().سائز()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ثابت string dotted_راستہ
                = (xrefp && !xrefp->dotted().خالی_خانے()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = خالی_اشاریہ;
            string scopename;
            جب (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.سائز() ? (inl + dotted_راستہ + xrefp->name())
                                               : (dotted_راستہ + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->ڈھونڈو_Dotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, سچ);
                اگر (inl == "") توڑو;
                inl = LinkDotState::remove_آخری_Inline_Scope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvt_مقصد_Hex(symp)
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
            VSymEnt* ثابت symp = m_statep->ڈھونڈو_Dotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, جھوٹ);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvt_مقصد_Hex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->داخل_کرو_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ثابت lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ثابت rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        سیٹ_Iface_Alias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }
    خالی processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, صحیح> گہرائی_Map;
        برائے (ثابت خودکار& pair : m_deferredAliasScopes) {
            VSymEnt* ثابت symp = pair.دوسرا;
            اگر (گہرائی_Map.ڈھونڈو(symp) == گہرائی_Map.انتہا()) {
                صحیح گہرائی = 0;
                برائے (VSymEnt* p = symp; p; p = p->parentp()) ++گہرائی;
                گہرائی_Map[symp] = گہرائی;
            }
        }
        std::stable_ترتیب_دو(m_deferredAliasScopes.begin(), m_deferredAliasScopes.انتہا(),
                         [&گہرائی_Map](ثابت std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ثابت std::pair<AstAliasScope*, VSymEnt*>& b) {
                             واپس گہرائی_Map.at(a.دوسرا) < گہرائی_Map.at(b.دوسرا);
                         });
        // Process in sorted order
        برائے (خودکار& pair : m_deferredAliasScopes) { processAliasScope(pair.پہلا, pair.دوسرا); }
        m_deferredAliasScopes.clear();
    }
    خالی دورہ(Ast_گرہ_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    خالی دورہ(AstCell*) override {}  // ScopeVisitor::
    خالی دورہ(AstVar*) override {}  // ScopeVisitor::
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

عوامی:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = طےشدہ;
};

//======================================================================

// Iterate an interface to resolve modports
جماعت LinkDotIfaceVisitor final : عوامی VNVisitor {
    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    خالی دورہ(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, nodep->name(), nodep, خالی_اشاریہ);
            m_curSymp->fallbackp(VL_RESTORER_پچھلا(m_curSymp));
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
        اگر (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ورنہ اگر (Ast_گرہ_FTask* ثابت ftaskp = VN_CAST(symp->nodep(), گرہ_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ثابت subSymp
                = m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), ftaskp, خالی_اشاریہ /*package*/);
            m_statep->داخل_کرو_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } ورنہ {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        اگر (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolve_Nested_Interface_راستہ(File_سطر* fl, VSymEnt* okSymp, string& baddot) {
        اگر (!okSymp || baddot.خالی_خانے()) واپس خالی_اشاریہ;

        ساکن constexpr صحیح MAX_NESTING_گہرائی = 64;
        VSymEnt* curOkSymp = okSymp;

        برائے (صحیح گہرائی = 0; گہرائی < MAX_NESTING_گہرائی; ++گہرائی) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = خالی_اشاریہ;
            اگر (ثابت AstCell* ثابت cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } ورنہ اگر (ثابت AstVar* ثابت varp = VN_CAST(curOkSymp->nodep(), Var)) {
                اگر (varp->isIfaceRef()) {
                    اگر (ثابت AstIfaceRefDType* ثابت ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->iface_بذریعہ_Cellp();
                    }
                }
            }

            اگر (!ifacep || !m_statep->exists_گرہ_Sym(ifacep)) واپس خالی_اشاریہ;

            VSymEnt* ثابت ifaceSymp = m_statep->get_گرہ_Sym(ifacep);

            اگر (baddot.ڈھونڈو('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ثابت symp = ifaceSymp->ڈھونڈو_Id_Fallback(baddot);
                اگر (symp) baddot.clear();
                واپس symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string بقایا_Baddot;
            VSymEnt* بقایا_Ok_Symp = خالی_اشاریہ;
            VSymEnt* ثابت symp = m_statep->ڈھونڈو_Dotted(fl, ifaceSymp, baddot, بقایا_Baddot,
                                                       بقایا_Ok_Symp, سچ);
            اگر (symp) {
                baddot = بقایا_Baddot;
                واپس symp;
            }

            // findDotted partially matched - check progress
            اگر (بقایا_Baddot == baddot || !بقایا_Ok_Symp) واپس خالی_اشاریہ;

            // Continue resolving with updated state
            baddot = بقایا_Baddot;
            curOkSymp = بقایا_Ok_Symp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        واپس خالی_اشاریہ;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolve_Modport_عبارت(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = خالی_اشاریہ;
        اگر (AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->ڈھونڈو_Id_Fallback(refp->name());
        } ورنہ اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            اگر (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_گرہ_Sym(refp->varScopep());
            } ورنہ اگر (refp->varp()) {
                symp = m_curSymp->ڈھونڈو_Id_Fallback(refp->varp()->name());
            }
        } ورنہ اگر (AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            اگر (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                اگر (m_statep->exists_گرہ_Sym(refp->varp())) {
                    symp = m_statep->get_گرہ_Sym(refp->varp());
                } ورنہ {
                    // Fallback: look up in current scope
                    symp = m_curSymp->ڈھونڈو_Id_Fallback(refp->varp()->name());
                }
            } ورنہ اگر (!refp->dotted().خالی_خانے()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ثابت string بھرا_راستہ = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = خالی_اشاریہ;
                symp = m_statep->ڈھونڈو_Dotted(fl, m_curSymp, بھرا_راستہ, baddot, okSymp, سچ);
                // Handle nested interface path when findDotted had partial match
                اگر (okSymp && !baddot.خالی_خانے()) {
                    VSymEnt* ثابت resolved = resolve_Nested_Interface_راستہ(fl, okSymp, baddot);
                    اگر (resolved) symp = resolved;
                }
            }
        } ورنہ اگر (AstDot* ثابت dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            منطقی hasPartSelect = جھوٹ;
            ثابت string dotted_راستہ = نکالو_Dotted_راستہ(dotp, hasPartSelect);
            اگر (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } ورنہ {
                string baddot;
                VSymEnt* okSymp = خالی_اشاریہ;
                symp = m_statep->ڈھونڈو_Dotted(fl, m_curSymp, dotted_راستہ, baddot, okSymp, سچ);

                // Handle nested interface path when findDotted had partial match
                اگر (okSymp && !baddot.خالی_خانے()) {
                    VSymEnt* ثابت resolved = resolve_Nested_Interface_راستہ(fl, okSymp, baddot);
                    اگر (resolved) symp = resolved;
                }

                اگر (!symp || !baddot.خالی_خانے()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_گرہ::prettyNameQ(dotted_راستہ));
                    symp = خالی_اشاریہ;
                }
            }
        } ورنہ اگر (VN_IS(exprp, Sel_ثنائی_ہندسہ) || VN_IS(exprp, Sel_نکالو)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } ورنہ {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        واپس symp;
    }

    خالی دورہ(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = خالی_اشاریہ;
        اگر (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolve_Modport_عبارت(nodep->fileline(), nodep->exprp());
        } ورنہ {
            symp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
        }
        اگر (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ورنہ اگر (AstVar* ثابت varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            اگر (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ثابت subSymp
                    = m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), varp, خالی_اشاریہ /*package*/);
                m_statep->داخل_کرو_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } ورنہ {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), nodep, خالی_اشاریہ /*package*/);
            }
        } ورنہ اگر (AstVarScope* ثابت vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            اگر (nodep->exprp()) {
                VSymEnt* ثابت subSymp
                    = m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), vscp, خالی_اشاریہ /*package*/);
                m_statep->داخل_کرو_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } ورنہ {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        اگر (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
        اگر (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ورنہ اگر (AstClocking* ثابت clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->داخل_کرو_Sym(m_curSymp, nodep->name(), nodep, خالی_اشاریہ /*package*/);
        } ورنہ {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    خالی دورہ(AstScope* nodep) override {  // IfaceVisitor::
        اگر (nodep->user3_سیٹ_Once()) واپس;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

عوامی:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = طےشدہ;
};

خالی LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ثابت VNUser3_اندر_Use user3_اندر_Use;
    برائے (ثابت خودکار& itr : m_ifaceModSyms) {
        AstIface* ثابت nodep = itr.پہلا;
        VSymEnt* ثابت symp = itr.دوسرا;
        LinkDotIfaceVisitor{nodep, symp, یہ};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

جماعت LinkDotResolveVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3_اندر_Use m_inuser3;

    // TYPES
    شمار DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_پہلا,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ثابت m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = خالی_اشاریہ;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = خالی_اشاریہ;  // SymEnt for current module
    VSymEnt* m_pinSymp = خالی_اشاریہ;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = خالی_اشاریہ;  // SymEnt for randomize target class's lookups
    ثابت AstCell* m_cellp = خالی_اشاریہ;  // Current cell
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    Ast_طریقہ_Call* m_rand_طریقہ_Callp = خالی_اشاریہ;  // Current randomize() method call
    صحیح m_modport_عدد = 0;  // Uniqueify modport numbers
    صحیح m_indent = 0;  // Indentation (tree depth) for debug
    منطقی m_اندر_Sens = جھوٹ;  // True if in senitem
    ثابت AstWith* m_currentWithp = خالی_اشاریہ;  // Enclosing 'with' (nullptr if none)
    std::سیٹ<std::string>
        m_restricted_نام_مستعمل;  // Names from current 'with (id_list)' that resolved into target
    منطقی m_genericIfaceModule = جھوٹ;  // True if in module containing generic interface
    std::map<std::string, Ast_گرہ*> m_if_Class_Imp_نام;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ثابت VSymEnt*> m_ifForwards;
    std::سیٹ<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    منطقی m_insideClassExtParam = جھوٹ;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = خالی_اشاریہ;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_گرہ*, AstPin*> m_مستعمل_Pins;  // Pin used in this cell, map to duplicate
    std::map<Ast_گرہ*, std::map<std::string, AstPin*>> m_مستعمل_Def_Param_Pins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_گرہ_Module*> m_modules_مقصد_Revisit;  // Modules to revisit a second time
    Ast_گرہ* m_آخری_Deferredp = خالی_اشاریہ;  // Last node which requested a revisit of its module
    Ast_گرہ_DType* m_packedArrayDtp = خالی_اشاریہ;  // Datatype reference for packed array
    منطقی m_اندر_Packed_Array = جھوٹ;  // Currently traversing a packed array tree
    منطقی m_replaceWithAlias
        = سچ;  // Replace VarScope with an alias. Used in the handling of AstAlias
    منطقی m_isParam = جھوٹ;  // Specifies whether currently visiting param variable
    منطقی m_resolvingTypedef = جھوٹ;  // Currently traversing a Typedef tree

    ساخت Dot_حالتیں final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ثابت AstDot* m_dotp;  // Current dot
        منطقی m_super;  // Starts with super reference
        منطقی m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        منطقی m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        منطقی m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        منطقی m_genBlk;  // Contains gen block reference
        Ast_گرہ* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        منطقی m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dot_متن;  // String of dotted names found in below parseref
        Dot_حالتیں() { init(خالی_اشاریہ); }
        ~Dot_حالتیں() = طےشدہ;
        خالی init(VSymEnt* curSymp, منطقی unresolvedClass = جھوٹ) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = خالی_اشاریہ;
            m_super = جھوٹ;
            m_dotErr = جھوٹ;
            m_dot_متن = "";
            m_unresolvedCell = جھوٹ;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = جھوٹ;
            m_genBlk = جھوٹ;
            m_unlinkedScopep = خالی_اشاریہ;
            m_disablep = خالی_اشاریہ;
        }
        string ascii() ثابت {
            ساکن ثابت حرف* ثابت نام[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << نام[m_dotPos];
            sstr << "  dse" << cvt_مقصد_Hex(m_dotSymp);
            ثابت string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.خالی_خانے() ? "" : ":") << dsname
                 << ")";
            اگر (m_dotErr) sstr << "  [dotErr]";
            اگر (m_super) sstr << "  [super]";
            اگر (m_unresolvedCell) sstr << "  [unrCell]";
            اگر (m_unresolvedClass) sstr << "  [unrClass]";
            اگر (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            اگر (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dot_متن;
            واپس sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_گرہ_Module* modp,
                              VSymEnt* moduleSymp, منطقی noWarn) {
        // Create implicit after warning
        اگر (!noWarn) {
            اگر (nodep->fileline()->warnIsOff(V3_Error_رمز::I_DEF_NETTYPE_WIRE)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_گرہ_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            ورنہ اگر (!nodep->fileline()->warnIsOff(V3_Error_رمز::IMPLICIT)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_گرہ_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest));
            }
        }
        AstVar* ثابت newp
            = نیا AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->جوڑو_Stmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->داخل_کرو_Sym(moduleSymp, newp->name(), newp, خالی_اشاریہ /*classOrPackagep*/);
        واپس newp;
    }
    AstVar* ملا_مقصد_Varp(ثابت VSymEnt* symp, Ast_گرہ* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        اگر (!symp) {
            واپس خالی_اشاریہ;
        } ورنہ اگر (VN_IS(symp->nodep(), Var)) {
            واپس VN_AS(symp->nodep(), Var);
        } ورنہ اگر (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ثابت snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ثابت varp = snodep->varp();
            اگر (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            واپس varp;
        } ورنہ {
            واپس خالی_اشاریہ;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* ڈھونڈو_Virtual_پورٹ_Var_Scope(VSymEnt* dotSymp, ثابت string& پورٹ_Name) {
        ثابت AstVarScope* ثابت dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ثابت AstVar* ثابت dotVarp = dotVscp ? dotVscp->varp() : خالی_اشاریہ;
        اگر (!dotVarp) واپس خالی_اشاریہ;

        Ast_گرہ_DType* dtypep = dotVarp->بچہ_DTypep();
        اگر (!dtypep) dtypep = dotVarp->subDTypep();
        اگر (!dtypep) واپس خالی_اشاریہ;
        ثابت AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        اگر (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->iface_بذریعہ_Cellp()) واپس خالی_اشاریہ;

        // Get the interface cell's symbol table
        VSymEnt* ثابت ifaceCellSymp = m_statep->get_گرہ_Sym(ifaceRefp->iface_بذریعہ_Cellp());

        // Look up modport by name in interface cell
        VSymEnt* ثابت modportSymp = ifaceCellSymp->ڈھونڈو_Id_Fallback(ifaceRefp->modportp()->name());
        اگر (!modportSymp) واپس خالی_اشاریہ;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ثابت virtPortp = m_statep->ڈھونڈو_Sym_Prefixed(modportSymp, پورٹ_Name, baddot, سچ);
        اگر (!virtPortp) واپس خالی_اشاریہ;

        // Symbol may point to VarScope or Var
        اگر (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) واپس vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        اگر (ثابت AstVar* ثابت realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ثابت realFoundp
                = m_statep->ڈھونڈو_Sym_Prefixed(ifaceCellSymp, realVarp->name(), baddot, سچ);
            واپس realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : خالی_اشاریہ;
        }
        واپس خالی_اشاریہ;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    خالی captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        اگر (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) واپس;
        Ast_گرہ_Module* ثابت defOwnerModp = V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(defp);
        اگر (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) واپس;
        AstCell* ثابت cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : خالی_اشاریہ;
        اگر (!cellForCapture) واپس;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::جوڑو_Param_Type(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_گرہ_Stmt* جوڑو_Implicit_Super_New_Call(AstFunc* ثابت nodep,
                                         ثابت AstClassExtends* ثابت classExtendsp) {
        // Returns the added node
        File_سطر* ثابت fl = nodep->fileline();
        AstArg* argsp = خالی_اشاریہ;
        اگر (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_درخت(سچ);
        AstNew* ثابت newExprp = نیا AstNew{fl, argsp};
        newExprp->isImplicit(سچ);
        AstDot* ثابت superNewp = نیا AstDot{fl, جھوٹ, نیا AstParseRef{fl, "super"}, newExprp};
        Ast_گرہ_Stmt* ثابت superNewStmtp = superNewp->makeStmt();
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            اگر (VN_IS(stmtp, گرہ_Stmt)) {
                stmtp->جوڑو_یہاں_This_As_اگلا(superNewStmtp);
                واپس superNewStmtp;
            }
        }
        // There were no statements
        nodep->جوڑو_Stmtsp(superNewStmtp);
        واپس superNewStmtp;
    }
    خالی checkNoDot(Ast_گرہ* nodep) {
        اگر (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = سچ;
            واپس;
        }
        اگر (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            اگر (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } ورنہ {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = سچ;
        }
    }
    AstVar* ڈھونڈو_Iface_چوٹی_Varp(Ast_گرہ* nodep, VSymEnt* والد_Entp, ثابت string& name) {
        ثابت string ڈھونڈو_Name = name + "__Viftop";
        VSymEnt* ثابت ifaceSymp = والد_Entp->ڈھونڈو_Id_Fallback(ڈھونڈو_Name);
        AstVar* ثابت iface_چوٹی_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : خالی_اشاریہ;
        UASSERT_OBJ(iface_چوٹی_Varp, nodep, "Can't find interface var ref: " << ڈھونڈو_Name);
        واپس iface_چوٹی_Varp;
    }
    AstRSProd* ڈھونڈو_Prod(Ast_گرہ* nodep, VSymEnt* والد_Entp, ثابت string& name) {
        ثابت VSymEnt* ثابت foundp = والد_Entp->ڈھونڈو_Id_Fallback(name);
        AstRSProd* ملا_Nodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : خالی_اشاریہ;
        اگر (!ملا_Nodep) {
            VSpellCheck speller;
            Link_گرہ_Matcher_Prod matcher;
            والد_Entp->candidateIdFlat(&speller, &matcher, جھوٹ);
            ثابت string suggest = speller.بہترین_Candidate_پیغام(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvt_مقصد_Hex(والد_Entp));
            nodep->v3error("Production " << Ast_گرہ::prettyNameQ(name) << " not found\n"
                                         << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest));
            واپس خالی_اشاریہ;
        }
        واپس ملا_Nodep;
    }
    خالی duplicatePinError(AstPin* nodep, Ast_گرہ* origp, ثابت حرف* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    خالی markAndCheckPinDup(AstPin* nodep, Ast_گرہ* refp, ثابت حرف* whatp) {
        ثابت خودکار pair = m_مستعمل_Pins.emplace(refp, nodep);
        اگر (pair.دوسرا) {
            اگر (!nodep->param_راستہ().خالی_خانے())
                m_مستعمل_Def_Param_Pins[refp].emplace(nodep->param_راستہ(), nodep);
            واپس;
        } ورنہ {
            Ast_گرہ* ثابت origp = pair.پہلا->دوسرا;
            اگر (nodep->param_راستہ().خالی_خانے() || VN_AS(origp, Pin)->param_راستہ().خالی_خانے()) {
                duplicatePinError(nodep, origp, whatp);
                واپس;
            }
        }

        خودکار& defParamPins = m_مستعمل_Def_Param_Pins[refp];
        ثابت خودکار defParamIt = defParamPins.ڈھونڈو(nodep->param_راستہ());
        اگر (defParamIt != defParamPins.انتہا()) {
            duplicatePinError(nodep, defParamIt->دوسرا, whatp);
            واپس;
        }
        defParamPins.emplace(nodep->param_راستہ(), nodep);
    }
    ساکن منطقی extendsClass(ثابت AstClass* classp, ثابت AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::سیٹ<ثابت AstClass*> دیکھا;
        std::سمتیہ<ثابت AstClass*> todo{classp};
        جب (!todo.خالی_خانے()) {
            ثابت AstClass* ثابت currp = todo.عقب();
            todo.نکالو_عقب();
            اگر (currp == targetp) واپس سچ;
            اگر (!دیکھا.داخل_کرو(currp).دوسرا) جاری;
            برائے (ثابت AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                اگر (ثابت AstClass* ثابت basep = cextp->classOrNullp()) todo.ڈالو_عقب(basep);
            }
        }
        واپس جھوٹ;
    }
    ساکن AstClocking* sensClockingp(Ast_گرہ* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        اگر (AstClocking* ثابت clockingp = VN_CAST(nodep, Clocking)) واپس clockingp;
        اگر (ثابت AstModportClockingRef* ثابت clockingRefp = VN_CAST(nodep, ModportClockingRef))
            واپس clockingRefp->clockingp();
        واپس خالی_اشاریہ;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ثابت eventp = clockingp->ensureEventp(سچ);
        اگر (!eventp->user1p()) eventp->user1p(نیا VSymEnt{m_statep->symsp(), eventp});
        واپس reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* ڈھونڈو_Inlined_Dots_Sym(Ast_گرہ* nodep, ثابت string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ثابت string inl = Ast_گرہ::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = خالی_اشاریہ;
        dotSymp = m_statep->ڈھونڈو_Dotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, جھوٹ);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_گرہ::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        واپس dotSymp;
    }
    VSymEnt* ڈھونڈو_Dotted_Ref_Symp(Ast_گرہ* nodep, VSymEnt* lookupSymp, ثابت string& dotted,
                               string& baddot, VSymEnt*& okSymp, منطقی* modportp = خالی_اشاریہ) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        اگر (modportp) *modportp = جھوٹ;
        VSymEnt* dotSymp = m_statep->ڈھونڈو_Dotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, سچ);  // Maybe nullptr
        اگر (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.خالی_خانے() ? Ast_گرہ::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            واپس خالی_اشاریہ;
        }
        اگر (ثابت AstVar* ثابت varp = VN_CAST(dotSymp->nodep(), Var)) {
            اگر (ثابت AstIfaceRefDType* ثابت ifaceRefp
                = VN_CAST(varp->بچہ_DTypep(), IfaceRefDType)) {
                اگر (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_گرہ_Sym(ifaceRefp->modportp());
                    اگر (modportp) *modportp = سچ;
                } ورنہ {
                    dotSymp = m_statep->get_گرہ_Sym(ifaceRefp->ifacep());
                }
            }
        } ورنہ اگر (ثابت AstModportClockingRef* ثابت clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_گرہ_Sym(clockingRefp->clockingp());
        }
        واپس dotSymp;
    }

    ساکن منطقی isParamedClassRefDType(ثابت Ast_گرہ* classp) {
        جب (ثابت AstRefDType* ثابت refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        واپس (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    منطقی isParamedClassRef(ثابت Ast_گرہ* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        اگر (ثابت خودکار* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            اگر (classRefp->paramsp()) واپس سچ;
            ثابت خودکار* classp = classRefp->classOrPackageNodep();
            جب (ثابت خودکار* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            واپس isParamedClassRefDType(classp);
        }
        واپس isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        جب (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        واپس classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    منطقی insideClass(ثابت AstClass* classp) ثابت {
        برائے (ثابت VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            اگر (symp->nodep() == classp) واپس سچ;
        }
        واپس جھوٹ;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    منطقی rand_ہدف_Unresolved() ثابت {
        واپس m_statep->forPrimary() && m_currentWithp && m_rand_طریقہ_Callp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    منطقی is_Virtual_طریقہ(ثابت Ast_گرہ_FTask* funcp, ثابت AstClass* classp) ثابت {
        اگر (funcp->isVirtual()) واپس سچ;
        برائے (ثابت AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ثابت basep = cextp->classOrNullp();
            اگر (!basep) جاری;
            ثابت VSymEnt* ثابت foundp = m_statep->get_گرہ_Sym(basep)->ڈھونڈو_Id_Flat(funcp->name());
            ثابت Ast_گرہ_FTask* ثابت بنیاد_Funcp
                = foundp ? VN_CAST(foundp->nodep(), گرہ_FTask) : خالی_اشاریہ;
            ثابت AstClass* ثابت بنیاد_Classp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : خالی_اشاریہ;
            اگر (بنیاد_Funcp && بنیاد_Classp && is_Virtual_طریقہ(بنیاد_Funcp, بنیاد_Classp)) واپس سچ;
        }
        واپس جھوٹ;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    منطقی inheritedImplements(ثابت VSymEnt* implp, ثابت VSymEnt* ifSymp, منطقی& needForwardr) {
        needForwardr = جھوٹ;
        ثابت Ast_گرہ_FTask* ثابت implFuncp = VN_CAST(implp->nodep(), گرہ_FTask);
        اگر (!implFuncp || implFuncp->خالص_Virtual() || implFuncp->isStatic()) واپس جھوٹ;
        ثابت AstClass* ثابت implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : خالی_اشاریہ;
        ثابت AstClass* ثابت ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : خالی_اشاریہ;
        اگر (!implClassp || !ifClassp) واپس جھوٹ;
        اگر (extendsClass(implClassp, ifClassp)) واپس سچ;
        اگر (!is_Virtual_طریقہ(implFuncp, implClassp)) واپس جھوٹ;
        needForwardr = سچ;
        واپس سچ;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    خالی جوڑو_Interface_Forward(AstClass* classp, Ast_گرہ_FTask* implp, AstClass* implClassp) {
        File_سطر* ثابت flp = implp->fileline();
        AstVar* fvarp = خالی_اشاریہ;
        Ast_گرہ_FTask* newp;
        اگر (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_درخت(جھوٹ);
            newp = نیا AstFunc{flp, implp->name(), خالی_اشاریہ, fvarp};
        } ورنہ {
            newp = نیا AstTask{flp, implp->name(), خالی_اشاریہ};
        }
        newp->class_طریقہ(سچ);
        newp->isVirtual(سچ);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = خالی_اشاریہ;
        برائے (Ast_گرہ* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            اگر (!portp || !portp->isIO()) جاری;
            AstVar* ثابت newPortp = portp->clone_درخت(جھوٹ);
            newp->جوڑو_Stmtsp(newPortp);
            argsp = Ast_گرہ::جوڑو_اگلا(
                argsp, نیا AstArg{flp, "", نیا AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_گرہ_FTask_Ref* callp;
        اگر (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            callp = نیا AstFuncRef{flp, implFuncp, argsp};
        } ورنہ {
            callp = نیا AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(سچ);
        اگر (fvarp) {
            newp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } ورنہ {
            newp->جوڑو_Stmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->جوڑو_Membersp(newp);
    }
    خالی importDerivedClass(AstClass* derivedClassp, VSymEnt* بنیاد_Symp, AstClass* بنیاد_Classp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << بنیاد_Classp);
        برائے (VSymEnt::const_iterator it = بنیاد_Symp->begin(); it != بنیاد_Symp->انتہا(); ++it) {
            اگر (Ast_گرہ* بنیاد_Subp = it->دوسرا->nodep()) {
                UINFO(8, indent() << "  SymFunc " << بنیاد_Subp);
                ثابت string impOrExtends
                    = بنیاد_Classp->isInterfaceClass() ? " implements " : " extends ";
                اگر (VN_IS(بنیاد_Subp, گرہ_FTask)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->ڈھونڈو_Id_Flat(بنیاد_Subp->name());
                    ثابت Ast_گرہ_FTask* ثابت بنیاد_Funcp = VN_CAST(بنیاد_Subp, گرہ_FTask);
                    اگر (!بنیاد_Funcp || !بنیاد_Funcp->خالص_Virtual()) جاری;
                    منطقی needForward = جھوٹ;
                    ثابت منطقی exists_اندر_Derived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->دوسرا, needForward));
                    اگر (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(بنیاد_Subp->name(), foundp);
                    }
                    اگر (m_statep->forPrimary() && !exists_اندر_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << بنیاد_Classp->prettyNameQ()
                                     << " but is missing implementation for "
                                     << بنیاد_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << بنیاد_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << بنیاد_Subp->warnContextSecondary());
                    }
                    ثابت خودکار itn = m_if_Class_Imp_نام.ڈھونڈو(بنیاد_Subp->name());
                    اگر (m_statep->forPrimary() && !exists_اندر_Derived
                        && itn != m_if_Class_Imp_نام.انتہا()
                        && itn->دوسرا != بنیاد_Subp) {  // Not exact same function from diamond
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << بنیاد_Classp->prettyNameQ()
                                     << " but missing inheritance conflict resolution for "
                                     << بنیاد_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26.6.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << بنیاد_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << بنیاد_Subp->warnContextSecondary());
                    }
                    m_if_Class_Imp_نام.emplace(بنیاد_Subp->name(), بنیاد_Subp);
                }
                اگر (VN_IS(بنیاد_Subp, Constraint)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->ڈھونڈو_Id_Flat(بنیاد_Subp->name());
                    ثابت AstConstraint* ثابت بنیاد_Funcp = VN_CAST(بنیاد_Subp, Constraint);
                    اگر (!بنیاد_Funcp || !بنیاد_Funcp->is_Kwd_خالص()) جاری;
                    ثابت منطقی exists_اندر_Derived = foundp && !foundp->imported();
                    اگر (m_statep->forPrimary() && !exists_اندر_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << بنیاد_Classp->prettyNameQ()
                                     << " but is missing constraint implementation for "
                                     << بنیاد_Subp->prettyNameQ() << " (IEEE 1800-2023 18.5.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << بنیاد_Subp->warnOther()
                                     << "... Location of interface class's pure constraint\n"
                                     << بنیاد_Subp->warnContextSecondary());
                    }
                }
            }
        }
    }
    خالی importSymbolsFromExtended(AstClass* ثابت nodep, AstClassExtends* ثابت cextp) {
        AstClass* ثابت بنیاد_Classp = cextp->classp();
        VSymEnt* ثابت srcp = m_statep->get_گرہ_Sym(بنیاد_Classp);
        importDerivedClass(nodep, srcp, بنیاد_Classp);
        اگر (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    خالی classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_گرہ_Sym(nodep));
    }

    خالی check_Decl_ترتیب(Ast_گرہ* nodep, Ast_گرہ* declp) {
        ثابت uint32_t decl_Token_عدد = declp->decl_Token_عدد();
        اگر (!decl_Token_عدد) واپس;  // Not implemented/valid on this object
        اگر (nodep->fileline()->token_عدد() < decl_Token_عدد) {
            UINFO(1, "Related node " << nodep->fileline()->token_عدد() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_عدد << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_مزید()
                           << "... Suggest move the declaration before the reference, or use a "
                              "forward typedef\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }
    خالی check_Member_Decl_ترتیب(Ast_گرہ* nodep, AstMemberDType* declp) {
        ثابت uint32_t decl_Token_عدد = declp->fileline()->token_عدد();
        اگر (nodep->fileline()->token_عدد() < decl_Token_عدد) {
            UINFO(1, "Related node " << nodep->fileline()->token_عدد() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_عدد << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_مزید()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    خالی replaceWithCheckBreak(Ast_گرہ* oldp, Ast_گرہ_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        اگر (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(نیا AstConst{newp->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
        } ورنہ {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    خالی revisitLater(Ast_گرہ* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_آخری_Deferredp = deferredNodep;
        m_modules_مقصد_Revisit.داخل_کرو(std::make_pair(m_modp->name(), m_modp));
    }

    خالی updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        اگر (nodep && nodep->isParam()) nodep->مستعمل_Param(سچ);
    }

    ساکن VSymEnt* ڈھونڈو_Id_Fallback_Skip_Member_DType(VSymEnt* lookp, ثابت string& name) {
        VSymEnt* shadowEntp = خالی_اشاریہ;  // Shadowing variable: not a type, kept for error report
        جب (lookp) {
            VSymEnt* ثابت foundp = lookp->ڈھونڈو_Id_Flat(name);
            اگر (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                اگر (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    واپس foundp;
                }
                اگر (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        واپس shadowEntp;
    }

    خالی symIterateChildren(Ast_گرہ* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    خالی symIterateNull(Ast_گرہ* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    ساکن ثابت Ast_گرہ_DType* get_اظہاریہ_DTypep(ثابت Ast_گرہ_اظہاریہ* selp) {
        جب (ثابت Ast_گرہ_Pre_Sel* ثابت sp = VN_CAST(selp, گرہ_Pre_Sel)) selp = sp->fromp();
        اگر (ثابت AstMemberSel* ثابت sp = VN_CAST(selp, MemberSel)) {
            اگر (ثابت Ast_گرہ_DType* dtypep = get_اظہاریہ_DTypep(sp->fromp())) {
                اگر (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ثابت AstClass* ثابت classp = classRefp->classp();
                    ثابت منطقی ملا = classp->existsMember(
                        [&dtypep, name = selp->name()](ثابت AstClass*, ثابت AstVar* nodep) {
                            dtypep = nodep->بچہ_DTypep();
                            واپس nodep->name() == name;
                        });
                    اگر (ملا) واپس dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } ورنہ {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } ورنہ اگر (ثابت Ast_گرہ_Var_Ref* ثابت varRefp = VN_CAST(selp, گرہ_Var_Ref)) {
            واپس varRefp->varp()->بچہ_DTypep()->elemDTypep();
        }
        واپس خالی_اشاریہ;
    }
    ساکن ثابت AstVar* get_اگلا_Varp(ثابت Ast_گرہ* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        جب (stmtsp) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(stmtsp, Var)) {
                اگر (varp->isIO()) واپس varp;
            }
            stmtsp = stmtsp->nextp();
        }
        واپس خالی_اشاریہ;
    }
    // Introduce implicit parameters for modules with generic interafeces
    خالی جوڑو_Implicit_Parameters_کا_Generic_Iface(AstCell* ثابت nodep, ثابت AstModule* ثابت modp) {
        // Get Param number
        صحیح param_عدد = 1;
        برائے (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_عدد;
        }

        // Add each implicit parameter type
        ثابت AstVar* modIfaceVarp = get_اگلا_Varp(modp->stmtsp());
        برائے (ثابت AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = get_اگلا_Varp(modIfaceVarp->nextp())) {
            اگر (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->بچہ_DTypep()->elemDTypep(), IfaceGenericDType)) {
                جاری;
            }
            Ast_گرہ* exprp = pinp->exprp();
            اگر (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                جاری;
            }
            جب (ثابت Ast_گرہ_Pre_Sel* ثابت preSelp = VN_CAST(exprp, گرہ_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = خالی_اشاریہ;
            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            اگر (varRefp) {
                enclosingVarp = varRefp->varp();
            } ورنہ اگر (ثابت AstParseRef* ثابت parseRefp = VN_CAST(exprp, ParseRef)) {
                اگر (m_modp) {
                    اگر (VSymEnt* ثابت symp
                        = m_statep->get_گرہ_Sym(m_modp)->ڈھونڈو_Id_Flat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            اگر (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->بچہ_DTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ثابت fwdRefp
                    = نیا AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ثابت newPinp = نیا AstPin{
                    pinp->fileline(), param_عدد, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(سچ);
                دورہ(newPinp);
                nodep->جوڑو_Paramsp(newPinp);
            } ورنہ اگر (varRefp) {
                ثابت AstVar* ثابت varp = varRefp->varp();
                اگر (ثابت AstIfaceRefDType* ثابت refp
                    = VN_CAST(varp->بچہ_DTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ثابت ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    اگر (refp->modportp()) {
                        newIfaceRefp = نیا AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } ورنہ {
                        newIfaceRefp = نیا AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    اگر (refp->cellp()->paramsp()) {
                        newIfaceRefp->جوڑو_Paramsp(refp->cellp()->paramsp()->clone_درخت(سچ));
                    }
                    UASSERT_OBJ(pinp->name().ڈھونڈو("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ثابت newPinp
                        = نیا AstPin{pinp->fileline(), param_عدد,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(سچ);
                    دورہ(newPinp);
                    nodep->جوڑو_Paramsp(newPinp);
                } ورنہ {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->بچہ_DTypep()
                                             ? "of type " + varp->بچہ_DTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } ورنہ {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_عدد;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    منطقی error_Hier_Non_پورٹ(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        اگر (varp->isIO()) واپس جھوٹ;
        // OK if not an instance
        ثابت AstCell* ثابت cellp = VN_CAST(scopeSymp->nodep(), Cell);
        اگر (!cellp) واپس جھوٹ;
        // Ok if not a hier_block
        ثابت Ast_گرہ_Module* ثابت modp = cellp->modp();
        اگر (!modp->hier_بلاک()) واپس جھوٹ;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        واپس سچ;
    }

#تعریف_کرو LINKDOT_دورہ_آغاز() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ثابت {
        string نتیجہ = "";
        نتیجہ.داخل_کرو(0, m_indent, ':');
        واپس نتیجہ + " ";
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    خالی دورہ(Ast_Type_جدول*) override {}
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (nodep->dead()) واپس;
        LINKDOT_دورہ_آغاز();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        اگر (ثابت AstModule* ثابت modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_گرہ_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = خالی_اشاریہ;
        m_modp = nodep;
        m_modport_عدد = 0;
        iterateChildren(nodep);
        m_modp = خالی_اشاریہ;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = خالی_اشاریہ;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = جھوٹ;
    }
    خالی دورہ(AstScope* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = خالی_اشاریہ;
    }
    خالی دورہ(AstCellInline* nodep) override {
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstCell* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER_CLEAR(m_مستعمل_Pins);
        VL_RESTORER_CLEAR(m_مستعمل_Def_Param_Pins);
        UASSERT_OBJ(nodep->modp(), nodep,
                    "Instance has unlinked module");  // V3LinkCell should have errored out
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_pinSymp);
        {
            m_cellp = nodep;
            اگر (VN_IS(nodep->modp(), Not_ملا_Module)) {
                // Prevent warnings about missing pin connects
                اگر (nodep->pinsp()) nodep->pinsp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
                اگر (nodep->paramsp()) nodep->paramsp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            ورنہ {
                m_pinSymp = m_statep->get_گرہ_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                اگر (m_statep->forPrimary())
                    اگر (ثابت AstModule* ثابت modp = VN_CAST(nodep->modp(), Module)) {
                        اگر (modp->hasGenericIface())
                            جوڑو_Implicit_Parameters_کا_Generic_Iface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    خالی دورہ(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_مستعمل_Pins);
        VL_RESTORER_CLEAR(m_مستعمل_Def_Param_Pins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_گنتی(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_گرہ_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        iterateChildren(nodep);
        اگر (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ثابت foundp = m_pinSymp->ڈھونڈو_Id_Flat(nodep->name());
            ثابت حرف* ثابت whatp = nodep->param() ? "parameter" : "pin";
            اگر (!foundp) {
                اگر (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                    واپس;
                } ورنہ اگر (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                    واپس;
                } ورنہ {
                    ثابت std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_گرہ_Matcher_Var_Param{}, جھوٹ)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_گرہ_Matcher_Var_IO{}, جھوٹ));
                    ثابت std::string decl
                        = ((m_cellp && m_cellp->modp())
                               ? "\n"s + nodep->warn_مزید() + "... Location of instance's "
                                     + m_cellp->modp()->verilogKwd() + " declaration\n"
                                     + m_cellp->modp()->warnContextSecondary()
                               : "");
                    nodep->v3warn(PINNOTFOUND,
                                  ucfirst(whatp)
                                      << " not found: " << nodep->prettyNameQ() << '\n'
                                      << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest)
                                      << '\n'
                                      << nodep->warnContextPrimary() << decl);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                    واپس;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            منطقی wrongPinType = جھوٹ;
            اگر (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                اگر (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } ورنہ {
                    wrongPinType = سچ;
                }
            } ورنہ اگر (AstParamTypeDType* ثابت typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } ورنہ {
                wrongPinType = سچ;
            }
            // Don't connect parameter pin to module ports or vice versa
            اگر (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = سچ;
            اگر (wrongPinType) {
                string ہدف_Type = LinkDotState::گرہ_متن_Type(foundp->nodep());
                ہدف_Type = VString::aOrAn(ہدف_Type) + ' ' + ہدف_Type;
                اگر (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << ہدف_Type);
                } ورنہ {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << ہدف_Type);
                }
                واپس;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ساکن منطقی isDotThisReference(ثابت AstDot* nodep) {
        واپس VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    خالی دورہ(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ثابت Dot_حالتیں آخری_حالتیں = m_ds;
        ثابت منطقی آغاز = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ثابت DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            اگر (آغاز) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && rand_ہدف_Unresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_پہلا;

            اگر (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                اگر (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                اگر (!classSymp && rand_ہدف_Unresolved()) {
                    // Bind 'this' in post-V3Param pass
                } ورنہ اگر (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = سچ;
                } ورنہ {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } ورنہ اگر (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ثابت VSymEnt* classSymp = getThisClassSymp();
                اگر (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = سچ;
                } ورنہ {
                    ثابت خودکار classp = VN_AS(classSymp->nodep(), Class);
                    ثابت خودکار cextp = classp->extendsp();
                    اگر (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = سچ;
                    } ورنہ {
                        اگر (m_statep->forPrimary()
                            && m_extendsParam.ڈھونڈو(classp) != m_extendsParam.انتہا()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = سچ;
                        } ورنہ {
                            ثابت خودکار بنیاد_Classp = cextp->classp();
                            UASSERT_OBJ(بنیاد_Classp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_گرہ_Sym(بنیاد_Classp);
                            m_ds.m_super = سچ;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } ورنہ اگر (AstClassOrPackageRef* ثابت lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_اگلا_Null(lhsp);
                اگر (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = آخری_حالتیں;
                    // Resolve function args before bailing
                    اگر (Ast_گرہ_FTask_Ref* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), گرہ_FTask_Ref)) {
                        iterate_And_اگلا_Null(ftaskrefp->argsp());
                    }
                    واپس;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                اگر (AstClassOrPackageRef* ثابت classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    اگر (Ast_گرہ* ثابت classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_گرہ_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ورنہ اگر (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterate_And_اگلا_Null(nodep->lhsp());
                // nodep->lhsp() may be a new node
                اگر (AstClassOrPackageRef* ثابت crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    اگر (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = آخری_حالتیں;
                        // Resolve function args before bailing
                        اگر (Ast_گرہ_FTask_Ref* ثابت ftaskrefp
                            = VN_CAST(nodep->rhsp(), گرہ_FTask_Ref)) {
                            iterate_And_اگلا_Null(ftaskrefp->argsp());
                        }
                        واپس;
                    }
                }
                اگر (m_آخری_Deferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = آخری_حالتیں;
                    // Resolve function args before bailing
                    اگر (Ast_گرہ_FTask_Ref* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), گرہ_FTask_Ref)) {
                        iterate_And_اگلا_Null(ftaskrefp->argsp());
                    }
                    واپس;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ورنہ {
                m_ds.m_dotPos = DP_پہلا;
                m_ds.m_unresolvedGenericIface = جھوٹ;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_اگلا_Null(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                اگر (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    واپس;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            اگر (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = سچ;
            }
            اگر (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            اگر (m_ds.m_dotPos == DP_پہلا) m_ds.m_dotPos = DP_SCOPE;
            اگر (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                اگر (آغاز && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterate_And_اگلا_Null(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            اگر (!m_ds.m_unresolvedClass) {
                اگر (آغاز) {
                    Ast_گرہ* newp;
                    اگر (m_ds.m_dotErr) {
                        newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
                    } ورنہ {
                        newp = nodep->rhsp()->unlink_Fr_عقب();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                } ورنہ {  // Dot midpoint
                    Ast_گرہ* newp = nodep->rhsp()->unlink_Fr_عقب();
                    اگر (m_ds.m_unresolvedCell) {
                        AstCellRef* ثابت crp = نیا AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlink_Fr_عقب(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                }
            }
        }
        اگر (آغاز) {
            m_ds = آخری_حالتیں;
        } ورنہ {
            ثابت منطقی unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = آخری_حالتیں.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    خالی دورہ(AstDefaultClocking* nodep) override {
        اگر (VSymEnt* ثابت foundp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name())) {
            اگر (AstClocking* ثابت clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            } ورنہ {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } ورنہ {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    خالی دورہ(Ast_Sen_شے* nodep) override {
        LINKDOT_دورہ_آغاز();
        VL_RESTORER(m_اندر_Sens);
        m_اندر_Sens = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstPatMember* nodep) override {
        LINKDOT_دورہ_آغاز();
        اگر (nodep->varrefp()) واپس;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        اگر (Ast_متن* ثابت textp = VN_CAST(nodep->keyp(), متن_2)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            اگر (VSymEnt* ثابت foundp = m_curSymp->ڈھونڈو_Id_Fallback(textp->متن())) {
                اگر (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                    اگر (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            نیا AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : خالی_اشاریہ,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                اگر (Ast_Enum_شے* ثابت itemp = VN_CAST(foundp->nodep(), Enum_شے)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        نیا Ast_Enum_شے_Ref{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstParseRef* nodep) override {
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        اگر (m_ds.m_unresolvedClass) واپس;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ثابت Dot_حالتیں آخری_حالتیں = m_ds;
        ثابت منطقی آغاز = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        منطقی پہلا = آغاز || m_ds.m_dotPos == DP_پہلا;

        اگر (آغاز) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        اگر (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = سچ;
        }
        اگر (nodep->name() == "this") {
            iterateChildren(nodep);
            اگر (m_statep->forPrimary()) واپس;  // The class might be parameterized somewhere
            ثابت VSymEnt* classSymp = getThisClassSymp();
            اگر (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                واپس;
            }
            AstClass* ثابت classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ثابت dtypep
                = نیا AstClassRefDType{nodep->fileline(), classp, خالی_اشاریہ};
            AstThisRef* ثابت newp = نیا AstThisRef{nodep->fileline(), VFlag_بچہ_DType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        اگر (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ثابت newp = نیا AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", سچ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        } ورنہ اگر (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            اگر (m_ds.m_dotErr) {
                nodep->unlink_Fr_عقب();  // Avoid circular node loop on errors
            } ورنہ {
                Ast_گرہ_اظہاریہ* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                Ast_گرہ_اظہاریہ* ثابت newp = نیا AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlag_بچہ_DType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ {
            //
            string expectWhat;
            منطقی allowScope = جھوٹ;
            منطقی allowVar = جھوٹ;
            منطقی allowFTask = جھوٹ;
            منطقی staticAccess = جھوٹ;
            اگر (m_ds.m_disablep) {
                allowScope = سچ;
                allowFTask = سچ;
                expectWhat = "block/task";
            } ورنہ اگر (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_گرہ_Module* classOrPackagep = خالی_اشاریہ;
                expectWhat = "scope/variable/func";
                allowScope = سچ;
                allowVar = سچ;
                allowFTask = سچ;
                staticAccess = سچ;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ثابت cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                اگر (cpackagerefp->name() == "local::") {
                    m_randSymp = خالی_اشاریہ;
                    پہلا = سچ;
                } ورنہ اگر (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, سچ, جھوٹ, ":: reference");
                    اگر (!foundp) واپس;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    اگر (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_گرہ_Sym(classOrPackagep);
                    } ورنہ {
                        m_ds = آخری_حالتیں;
                        واپس;
                    }
                } ورنہ {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_گرہ_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } ورنہ اگر (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_پہلا) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = سچ;
                allowVar = سچ;
            } ورنہ اگر (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = سچ;
            } ورنہ اگر (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = سچ;
                allowVar = سچ;
            } ورنہ {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = خالی_اشاریہ;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            اگر (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_مقصد_ہدف(nodep->name()))) {
                foundp = m_randSymp->ڈھونڈو_Id_Flat(nodep->name());
                اگر (foundp) {
                    اگر (m_currentWithp && m_currentWithp->restricted()) {
                        m_restricted_نام_مستعمل.داخل_کرو(nodep->name());
                    }
                    اگر (!آغاز) m_ds.m_dotPos = DP_MEMBER;
                    اگر (!m_currentWithp) {
                        UASSERT_OBJ(m_rand_طریقہ_Callp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_rand_طریقہ_Callp->fromp(), VarRef), m_rand_طریقہ_Callp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ثابت newp = نیا AstMemberSel{
                            nodep->fileline(), m_rand_طریقہ_Callp->fromp()->clone_درخت(جھوٹ),
                            VFlag_بچہ_DType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                        واپس;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ثابت lambdaRefp
                        = نیا AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", جھوٹ};
                    AstMemberSel* newp = نیا AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlag_بچہ_DType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
            }
            اگر (allowScope) {
                foundp = m_statep->ڈھونڈو_Dotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, پہلا);  // Maybe nullptr
            } ورنہ اگر (پہلا) {
                foundp = m_ds.m_dotSymp->ڈھونڈو_Id_Fallback(nodep->name());
            } ورنہ {
                foundp = m_ds.m_dotSymp->ڈھونڈو_Id_Flat(nodep->name());
            }
            اگر (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ثابت AstClass* ثابت classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                اگر (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ثابت enclosingClassSymp
                        = m_statep->get_گرہ_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->ڈھونڈو_Id_Fallback(nodep->name());
                    اگر (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            اگر (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ثابت ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ثابت ifaceFoundp = ifaceFallbackp->ڈھونڈو_Id_Flat(nodep->name());
                اگر (ifaceFoundp) {
                    اگر (ثابت AstVar* ثابت varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        اگر (varp->isParam()) foundp = ifaceFoundp;
                    } ورنہ اگر (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            اگر (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            اگر (foundp) {
                UINFO(9, indent() << "found=se" << cvt_مقصد_Hex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            منطقی ok = جھوٹ;
            // Special case: waiting on clocking event
            اگر (m_اندر_Sens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_پہلا) {
                اگر (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            اگر (!foundp) {
            } ورنہ اگر (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), گرہ_بلاک)
                       || VN_IS(foundp->nodep(), Gen_بلاک)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                اگر (allowScope) {
                    ok = سچ;
                    m_ds.m_dot_متن = VString::dot(m_ds.m_dot_متن, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    اگر (m_ds.m_disablep && VN_IS(foundp->nodep(), گرہ_بلاک)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    اگر (VN_IS(foundp->nodep(), Gen_بلاک)) {
                        m_ds.m_genBlk = سچ;
                        اگر (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } ورنہ اگر (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ثابت cellp = VN_AS(foundp->nodep(), Cell);
                    اگر (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ثابت cellEntp = m_statep->get_گرہ_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ثابت والد_Entp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ثابت ifaceRefVarp
                            = ڈھونڈو_Iface_چوٹی_Varp(nodep, والد_Entp, nodep->name());
                        //
                        ok = سچ;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_گرہ_اظہاریہ* newp;
                        اگر (m_ds.m_dot_متن != "") {
                            newp = نیا AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dot_متن,
                                                  VAccess::READ};
                            اگر (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = نیا AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_عقب()};
                                m_ds.m_unlinkedScopep = خالی_اشاریہ;
                                m_ds.m_unresolvedCell = جھوٹ;
                                m_ds.m_dot_متن = "";
                            }
                        } ورنہ {
                            AstVarRef* ثابت refp
                                = نیا AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    } ورنہ اگر (VN_IS(cellp->modp(), Not_ملا_Module)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } ورنہ اگر (allowScope && !allowFTask && VN_IS(foundp->nodep(), گرہ_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = سچ;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } ورنہ اگر (allowFTask && VN_IS(foundp->nodep(), گرہ_FTask)) {
                Ast_گرہ_FTask_Ref* taskrefp;
                اگر (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = نیا AstTaskRef{nodep->fileline(), nodep->name()};
                } ورنہ {
                    taskrefp = نیا AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                m_ds = آخری_حالتیں;
                واپس;
            } ورنہ اگر (AstVar* ثابت varp = ملا_مقصد_Varp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                اگر (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = سچ;
                    واپس;
                }
                AstIfaceRefDType* ثابت ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                اگر (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->بچہ_DTypep(), IfaceGenericDType) && !آغاز) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = سچ;
                    m_ds.m_unresolvedGenericIface = سچ;
                } ورنہ اگر (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->iface_بذریعہ_Cellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dot_متن << "  " << nodep);
                    m_ds.m_dot_متن = VString::dot(m_ds.m_dot_متن, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    اگر (ifacerefp->modportp() && m_statep->exists_گرہ_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_گرہ_Sym(ifacerefp->modportp());
                    } ورنہ {
                        m_ds.m_dotSymp = m_statep->get_گرہ_Sym(ifacerefp->iface_بذریعہ_Cellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = سچ;

                    AstVarRef* ثابت refp = نیا AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                } ورنہ اگر (allowVar) {
                    Ast_گرہ* newp;
                    اگر (m_ds.m_dot_متن != "") {
                        AstVarXRef* ثابت refp
                            = نیا AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dot_متن,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        اگر (ثابت AstModportVarRef* ثابت mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            اگر (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(سچ);
                            }
                        }
                        refp->شامل_ہے_Gen_بلاک(m_ds.m_genBlk);
                        m_ds.m_dot_متن = "";
                        اگر (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ثابت string dotted = refp->dotted();
                            ثابت سائز_t pos = dotted.ڈھونڈو("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            اگر (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } ورنہ {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = نیا AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_عقب()};
                                m_ds.m_unlinkedScopep = خالی_اشاریہ;
                                m_ds.m_unresolvedCell = جھوٹ;
                            }
                        } ورنہ {
                            newp = refp;
                        }
                    } ورنہ {
                        اگر (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ثابت refp = نیا AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = سچ;
                }
            } ورنہ اگر (ثابت AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dot_متن << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                اگر (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } ورنہ اگر (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } ورنہ {
                    AstCell* ثابت cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ثابت cellEntp = m_statep->get_گرہ_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ثابت والد_Entp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ثابت ifaceRefVarp
                        = ڈھونڈو_Iface_چوٹی_Varp(nodep, والد_Entp, cellp->name());
                    //
                    ok = سچ;
                    m_ds.m_dot_متن = VString::dot(m_ds.m_dot_متن, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ثابت refp
                        = نیا AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_گرہ_اظہاریہ* newp = refp;

                    خودکار* ثابت cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    اگر (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = نیا Ast_Sel_ثنائی_ہندسہ{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlink_Fr_عقب()};
                        newp->user3(سچ);  // Don't process again
                        VL_DO_DANGLING(ڈالو_Deletep(cellarrayrefp->unlink_Fr_عقب()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = خالی_اشاریہ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                }
            } ورنہ اگر (AstConstraint* ثابت defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_گرہ* ثابت newp = نیا AstConstraintRef{nodep->fileline(), خالی_اشاریہ, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                ok = سچ;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dot_متن = "";
            } ورنہ اگر (AstClass* ثابت defp = VN_CAST(foundp->nodep(), Class)) {
                اگر (allowVar) {
                    اگر (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } ورنہ {
                        AstRefDType* ثابت newp
                            = نیا AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    }
                    ok = سچ;
                    m_ds.m_dot_متن = "";
                } ورنہ {
                    (خالی)defp;  // Prevent unused variable warning
                }
            } ورنہ اگر (AstMemberDType* ثابت defp = VN_CAST(foundp->nodep(), MemberDType)) {
                اگر (allowVar) {
                    check_Member_Decl_ترتیب(nodep, defp);
                    AstRefDType* ثابت refp = نیا AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    ok = سچ;
                    m_ds.m_dot_متن = "";
                }
            } ورنہ اگر (Ast_Enum_شے* ثابت valuep = VN_CAST(foundp->nodep(), Enum_شے)) {
                اگر (allowVar) {
                    Ast_Enum_شے_Ref* ثابت refp
                        = نیا Ast_Enum_شے_Ref{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_گرہ* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dot_متن);
                    refp->شامل_ہے_Gen_بلاک(m_ds.m_genBlk);
                    اگر (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = نیا AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlink_Fr_عقب()};
                        m_ds.m_unlinkedScopep = خالی_اشاریہ;
                        m_ds.m_unresolvedCell = جھوٹ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    ok = سچ;
                    m_ds.m_dot_متن = "";
                }
            } ورنہ اگر (ثابت AstLambdaArgRef* ثابت argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                اگر (allowVar) {
                    Ast_گرہ* ثابت newp
                        = نیا AstLambdaArgRef{nodep->fileline(), argrefp->name(), جھوٹ};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    ok = سچ;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dot_متن = "";
                }
            } ورنہ اگر (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                اگر (m_ds.m_dot_متن != "") m_ds.m_dot_متن += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_پہلا;
            } ورنہ اگر (ثابت AstModportClockingRef* ثابت clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_گرہ_Sym(clockingRefp->clockingp());
                اگر (m_ds.m_dot_متن != "") m_ds.m_dot_متن += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_پہلا;
            } ورنہ اگر (ثابت Ast_گرہ_FTask* ثابت ftaskp = VN_CAST(foundp->nodep(), گرہ_FTask)) {
                اگر (!ftaskp->isFunction() || ftaskp->class_طریقہ()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    اگر (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ثابت funcRefp
                            = نیا AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    }
                }
            } ورنہ اگر (AstTypedef* ثابت defp = VN_CAST(foundp->nodep(), Typedef)) {
                ثابت منطقی ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                اگر (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dot_متن
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                اگر (ok) {
                    AstRefDType* ثابت refp = نیا AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    اگر (m_ds.m_dotPos == DP_NONE) check_Decl_ترتیب(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<صحیح>(m_ds.m_dotPos), m_ds.m_dot_متن,
                        m_ds.m_dotSymp, m_modp, [یہ]() { واپس indent(); });

                    اگر (VN_IS(nodep->backp(), Sel_نکالو)) {
                        m_packedArrayDtp = refp;
                    } ورنہ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    }
                }
            } ورنہ اگر (AstParamTypeDType* ثابت defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                اگر (ok) {
                    AstRefDType* ثابت refp = نیا AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<صحیح>(m_ds.m_dotPos), m_ds.m_dot_متن,
                        m_ds.m_dotSymp, m_modp, [یہ]() { واپس indent(); });

                    اگر (VN_IS(nodep->backp(), Sel_نکالو)) {
                        m_packedArrayDtp = refp;
                    } ورنہ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    }
                }
            }
            اگر (!ok) {
                اگر (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = سچ;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } ورنہ {
                    // Cells/interfaces can't be implicit
                    ثابت منطقی checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dot_متن == "" && !m_ds.m_disablep && !foundp);
                    ثابت منطقی err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    اگر (err) {
                        اگر (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dot_متن << (m_ds.m_dot_متن == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } ورنہ اگر (m_ds.m_dot_متن == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvt_مقصد_Hex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ثابت string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest));
                        } ورنہ {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.خالی_خانے() ? Ast_گرہ::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dot_متن + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_گرہ::prettyName(m_ds.m_dot_متن))
                                           : ""));
                        }
                        m_ds.m_dotErr = سچ;
                    }
                    اگر (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ثابت varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ثابت newp
                            = نیا AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    }
                }
            }
        }
        اگر (آغاز) m_ds = آخری_حالتیں;
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {
        // Class: Recurse inside or cleanup not founds
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_دورہ_آغاز();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        VL_RESTORER_CLEAR(m_مستعمل_Pins);
        VL_RESTORER_CLEAR(m_مستعمل_Def_Param_Pins);
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp(), nodep,
                    "class reference parameter not removed by V3Param");
        {
            VL_RESTORER_COPY(m_ds);
            VL_RESTORER(m_pinSymp);

            اگر (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ثابت منطقی deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                جھوٹ, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            اگر (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_گرہ_Sym(nodep->classOrPackageSkipp());
            } ورنہ اگر (nodep->name() != "local::") {
                واپس;
            }

            AstClass* ثابت refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            اگر (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            اگر (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                        << nodep->warn_مزید() << "... Suggest use '"
                        << nodep->classOrPackageNodep()->prettyName() << "#()'");
                // As other simulators do, use the default specialization. Add the same
                // placeholder pin V3LinkCells makes for '#()' so it is linked below.
                AstPin* ثابت pinp = نیا AstPin{nodep->fileline(), 1, "__paramNumber1", خالی_اشاریہ};
                pinp->param(سچ);
                nodep->جوڑو_Paramsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        اگر (nodep->name() == "local::") {
            اگر (!m_randSymp && !m_rand_طریقہ_Callp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = سچ;
                واپس;
            }
        }
        اگر (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_گرہ_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    خالی دورہ(AstConstraint* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (nodep->isExternDef()) {
            اگر (ثابت VSymEnt* ثابت foundp
                = m_curSymp->ڈھونڈو_Id_Fallback("extern " + nodep->name())) {
                ثابت AstConstraint* ثابت protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } ورنہ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        اگر (nodep->isExternProto()) {
            اگر (!m_curSymp->ڈھونڈو_Id_Fallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_گرہ_Sym(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstConstraintRef* nodep) override {
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    خالی دورہ(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_دورہ_آغاز();
        iterateChildren(nodep);
        اگر (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvt_مقصد_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
            اگر (AstVar* ثابت varp
                = foundp ? ملا_مقصد_Varp(foundp, nodep, nodep->access()) : خالی_اشاریہ) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            اگر (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        اگر (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    خالی دورہ(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        اگر (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(خالی_اشاریہ);
        } ورنہ {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            اگر (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = ڈھونڈو_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            منطقی modport;
            VSymEnt* ثابت dotSymp
                = ڈھونڈو_Dotted_Ref_Symp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            اگر (!dotSymp) واپس;

            اگر (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = خالی_اشاریہ;
                اگر (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string سابقہ = dotSymp->sym_سابقہ();
                    جب (!foundp) {
                        foundp = dotSymp->ڈھونڈو_Id_Flat(سابقہ + nodep->name());
                        اگر (foundp) توڑو;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->ڈھونڈو_Id_Flat(سابقہ + nodep->name());
                        اگر (ثابت AstVar* ثابت varp
                            = ملا_مقصد_Varp(foundp, nodep, nodep->access())) {
                            اگر (!varp->isParam()) foundp = خالی_اشاریہ;
                        }
                        اگر (سابقہ.خالی_خانے()) توڑو;
                        string اگلا_سابقہ = LinkDotState::remove_آخری_Inline_Scope(سابقہ);
                        اگر (سابقہ == اگلا_سابقہ) توڑو;
                        سابقہ = std::move(اگلا_سابقہ);
                    }
                    baddot = nodep->name();
                } ورنہ {
                    foundp = m_statep->ڈھونڈو_Sym_Prefixed(dotSymp, nodep->name(), baddot, سچ);
                }
                اگر (m_اندر_Sens && foundp) {
                    اگر (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ثابت varp
                    = foundp ? ملا_مقصد_Varp(foundp, nodep, nodep->access()) : خالی_اشاریہ;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                اگر (nodep->varp() && error_Hier_Non_پورٹ(nodep, nodep->varp(), dotSymp)) واپس;
                اگر (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_گرہ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    واپس;
                }
            } ورنہ {
                VSymEnt* ثابت foundp
                    = m_statep->ڈھونڈو_Sym_Prefixed(dotSymp, nodep->name(), baddot, سچ);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : خالی_اشاریہ;
                // Handle modport expression virtual ports
                اگر (!vscp) vscp = ڈھونڈو_Virtual_پورٹ_Var_Scope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                اگر (vscp && error_Hier_Non_پورٹ(nodep, vscp->varp(), dotSymp)) واپس;
                اگر (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_گرہ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } ورنہ {
                    اگر (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ثابت newvscp
                        = نیا AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    خالی دورہ(AstEnumDType* nodep) override {
        LINKDOT_دورہ_آغاز();
        iterateChildren(nodep);
        AstRefDType* ثابت refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        اگر (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {
        LINKDOT_دورہ_آغاز();
        symIterateChildren(nodep, m_statep->get_گرہ_Sym(nodep));
    }
    خالی دورہ(Ast_Enum_شے_Ref* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        اگر (m_statep->forParamed() && !nodep->dotted().خالی_خانے() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = خالی_اشاریہ;
            VSymEnt* ثابت dotSymp
                = ڈھونڈو_Dotted_Ref_Symp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            اگر (!dotSymp) واپس;
            VSymEnt* ثابت foundp
                = m_statep->ڈھونڈو_Sym_Prefixed(dotSymp, nodep->name(), baddot, سچ);
            Ast_Enum_شے* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), Enum_شے) : خالی_اشاریہ;
            اگر (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_گرہ::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                واپس;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        اگر (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvt_مقصد_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
            اگر (Ast_Enum_شے* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), Enum_شے) : خالی_اشاریہ) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            اگر (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_طریقہ_Call* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_rand_طریقہ_Callp);
        {
            m_ds.init(m_curSymp);
            اگر (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_rand_طریقہ_Callp = nodep;
                ثابت Ast_گرہ_DType* fromDtp = nodep->fromp()->dtypep();
                اگر (!fromDtp) {
                    اگر (ثابت Ast_گرہ_Var_Ref* ثابت varRefp = VN_CAST(nodep->fromp(), گرہ_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } ورنہ {
                        fromDtp = get_اظہاریہ_DTypep(nodep->fromp());
                    }
                    اگر (!fromDtp) {
                        اگر (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } ورنہ {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                اگر (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = سچ;
                } ورنہ اگر (fromDtp) {
                    ثابت AstClassRefDType* ثابت classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    اگر (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    ورنہ اگر (m_statep->forPrimary() && m_extendsParam.گنتی(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = سچ;
                    } ورنہ {
                        m_randSymp = m_statep->get_گرہ_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstVar* nodep) override {
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        اگر (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    خالی دورہ(AstVarScope* nodep) override {
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        اگر (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            Ast_تفویض_W* ثابت assignp = نیا Ast_تفویض_W{
                nodep->fileline(), نیا AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                نیا AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(سچ);
            nodep->scopep()->جوڑو_Blocksp(نیا AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        اگر (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        اگر (m_ds.m_super) {
            اگر (AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(سچ);
            } ورنہ اگر (AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(سچ);
            }
        }

        VL_RESTORER(m_randSymp);

        منطقی پہلا = !m_ds.m_dotp || m_ds.m_dotPos == DP_پہلا;
        منطقی staticAccess = جھوٹ;
        اگر (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            واپس;
        } ورنہ اگر (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_گرہ_FTask_Ref* ثابت newftaskp = nodep->clone_درخت(جھوٹ);
            newftaskp->dotted(m_ds.m_dot_متن);
            Ast_گرہ* ثابت newp = نیا AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlink_Fr_عقب()};
            m_ds.m_unlinkedScopep = خالی_اشاریہ;
            m_ds.m_unresolvedCell = جھوٹ;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        } ورنہ اگر (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = سچ;
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            اگر (cpackagerefp->name() == "local::") {
                m_randSymp = خالی_اشاریہ;
                پہلا = سچ;
            } ورنہ اگر (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, سچ, جھوٹ, ":: reference");
                اگر (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } ورنہ {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } ورنہ اگر (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dot_متن);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            اگر (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ثابت Ast_گرہ* ثابت blkp = m_ds.m_dotSymp->nodep();
                اگر (VN_IS(blkp, Gen_بلاک)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), Gen_Case_شے))) {
                    nodep->شامل_ہے_Gen_بلاک(سچ);
                }
            }
        } ورنہ اگر (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_گرہ_اظہاریہ* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
            AstArg* ثابت argsp = nodep->argsp();
            اگر (argsp) argsp->unlink_Fr_عقب_With_اگلا();
            Ast_طریقہ_Call* ثابت newp = نیا Ast_طریقہ_Call{nodep->fileline(), varEtcp,
                                                          VFlag_بچہ_DType{}, nodep->name(), argsp};
            اگر (AstWith* ثابت withp = nodep->withp()) newp->withp(withp->unlink_Fr_عقب());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        } ورنہ اگر (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_پہلا)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dot_متن = "";
        } ورنہ اگر (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        اگر (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } ورنہ اگر (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(خالی_اشاریہ);
        } ورنہ اگر (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } ورنہ {
            string baddot;
            VSymEnt* okSymp = خالی_اشاریہ;
            VSymEnt* dotSymp;
            اگر (nodep->dotted().خالی_خانے()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } ورنہ {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            اگر (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_گرہ_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } ورنہ {
                اگر (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = ڈھونڈو_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->ڈھونڈو_Dotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, سچ);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            اگر (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_مقصد_ہدف(nodep->name()))) {
                VSymEnt* ثابت foundp = m_randSymp->ڈھونڈو_Id_Flat(nodep->name());
                اگر (foundp && m_currentWithp) {
                    اگر (m_currentWithp->restricted()) {
                        m_restricted_نام_مستعمل.داخل_کرو(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = خالی_اشاریہ;
                    اگر (nodep->argsp()) {
                        iterate_And_اگلا_Null(nodep->argsp());
                        argsp = nodep->argsp()->unlink_Fr_عقب_With_اگلا();
                    }
                    اگر (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_گرہ* ثابت newp = نیا Ast_طریقہ_Call{
                        nodep->fileline(),
                        نیا AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", جھوٹ},
                        VFlag_بچہ_DType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
            }
            اگر (پہلا && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ثابت randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), سچ, سچ);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->داخل_کرو_بلاک(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                واپس;
            }
            VSymEnt* ثابت foundp
                = m_statep->ڈھونڈو_Sym_Prefixed(dotSymp, nodep->name(), baddot, پہلا);
            Ast_گرہ_FTask* ثابت taskp = foundp ? VN_CAST(foundp->nodep(), گرہ_FTask) : خالی_اشاریہ;
            Ast_گرہ_Module* ثابت taskModulep = foundp ? foundp->classOrPackagep() : خالی_اشاریہ;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            اگر (!m_isParam || !taskp
                || (taskp->class_طریقہ() && taskModulep && taskModulep->hasParameterList())) {
                اگر (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    واپس;
                }
                اگر (AstClass* ثابت ہدف_Classp = VN_CAST(dotSymp->nodep(), Class)) {
                    اگر (m_extendsParam.گنتی(ہدف_Classp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        واپس;
                    }
                }
            }

            اگر (taskp) {
                اگر (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } ورنہ {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvt_مقصد_Hex(m_curSymp)
                                  << " dotSymp=se" << cvt_مقصد_Hex(dotSymp));
                اگر (foundp) {
                    اگر (VN_IS(foundp->nodep(), Var) && m_ds.m_dot_متن == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } ورنہ {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dot_متن << (m_ds.m_dot_متن == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } ورنہ اگر (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } ورنہ اگر ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ثابت classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ثابت funcp
                        = نیا AstFunc{nodep->fileline(), nodep->name(), خالی_اشاریہ, خالی_اشاریہ};
                    funcp->class_طریقہ(سچ);
                    funcp->dtypep(funcp->ڈھونڈو_Void_DType());
                    classp->جوڑو_Membersp(funcp);
                    m_statep->داخل_کرو_بلاک(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    واپس;
                } ورنہ اگر (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    اگر (AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        اگر (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } ورنہ اگر (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } ورنہ {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(نیا AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                        واپس;
                    }
                } ورنہ اگر (nodep->dotted() == "") {
                    اگر (nodep->pli()) {
                        اگر (v3Global.opt.bboxSys()) {
                            Ast_گرہ* newp;
                            اگر (VN_IS(nodep, FuncRef)) {
                                newp = نیا AstConst{nodep->fileline(), AstConst::All0{}};
                            } ورنہ {
                                Ast_گرہ_اظہاریہ* outp = خالی_اشاریہ;
                                جب (AstArg* ثابت argp = nodep->argsp()) {
                                    outp = Ast_گرہ::جوڑو_اگلا(outp, argp->exprp()->unlink_Fr_عقب());
                                    VL_DO_DANGLING(ڈالو_Deletep(argp->unlink_Fr_عقب()), argp);
                                }
                                newp = نیا AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                            واپس;
                        } ورنہ {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ثابت string suggest = speller.بہترین_Candidate_پیغام(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest));
                        }
                    } ورنہ {
                        ثابت string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_گرہ_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest));
                    }
                } ورنہ {
                    ثابت string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_گرہ_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_گرہ::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.خالی_خانے() ? "" : nodep->warn_مزید() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    خالی دورہ(Ast_Sel_ثنائی_ہندسہ* nodep) override {
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        iterate_And_اگلا_Null(nodep->fromp());
        اگر (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_پہلا, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            واپس;
        }
        اگر (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_پہلا) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dot_متن += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = سچ;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterate_And_اگلا_Null(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        اگر (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_پہلا)) {
            Ast_گرہ_اظہاریہ* ثابت exprp = nodep->bitp()->unlink_Fr_عقب();
            اگر (AstCellArrayRef* ثابت fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->جوڑو_Selp(exprp);
                AstCellArrayRef* ثابت movedp = fromArrRefp->unlink_Fr_عقب();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            } ورنہ {
                AstCellArrayRef* ثابت newp
                    = نیا AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            }
        }
    }
    خالی دورہ(Ast_گرہ_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        اگر (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_پہلا) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = سچ;
            واپس;
        }
        Ast_گرہ_DType* packedArrayDtp = خالی_اشاریہ;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterate_And_اگلا_Null(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            اگر (m_packedArrayDtp) {
                AstRange* ثابت newRangep
                    = نیا AstRange(nodep->fileline(), nodep->rhsp()->unlink_Fr_عقب(),
                                   nodep->thsp()->unlink_Fr_عقب());
                AstPackArrayDType* newArrayTypep
                    = نیا AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->بچہ_DTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(خالی_اشاریہ);
                اگر (VN_IS(nodep->backp(), Sel_نکالو)) {
                    packedArrayDtp = newArrayTypep;
                } ورنہ {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                }
            } ورنہ {
                اگر (nodep->attrp()) {
                    Ast_گرہ* ثابت attrp = nodep->attrp()->unlink_Fr_عقب();
                    VL_DO_DANGLING(attrp->delete_درخت(), attrp);
                }
                Ast_گرہ* ثابت basefromp = nodep->بنیاد_Fromp(جھوٹ);
                اگر (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    اگر (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } ورنہ {
                    nodep->attrp(نیا Ast_Attr_کا{nodep->fileline(), VAttrType::VAR_بنیاد,
                                               basefromp->clone_درخت(جھوٹ)});
                }
            }
        }

        اگر (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    خالی دورہ(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_دورہ_آغاز();
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            اگر (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_گرہ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_مقصد_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_مقصد_Hex(m_curSymp));
    }
    خالی دورہ(Ast_گرہ_بلاک* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            اگر (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_گرہ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_مقصد_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_مقصد_Hex(m_curSymp));
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(سچ);
            اگر (ثابت VSymEnt* ثابت foundp
                = m_curSymp->ڈھونڈو_Id_Fallback("extern " + nodep->name())) {
                ثابت Ast_گرہ_FTask* ثابت protop = VN_AS(foundp->nodep(), گرہ_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->جوڑو_Stmtsp(نیا Ast_Attr_کا{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_درخت(جھوٹ)
                                                   : نیا AstVoidDType{protop->fileline()}});
                برائے (Ast_گرہ* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                        Ast_Attr_کا* ثابت attrp
                            = نیا Ast_Attr_کا{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_درخت(جھوٹ)};
                        attrp->name(portp->name());
                        nodep->جوڑو_Stmtsp(attrp);
                    }
                }
            } ورنہ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        اگر (nodep->isExternProto()) {
            اگر (!m_curSymp->ڈھونڈو_Id_Fallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_گرہ_Sym(nodep);
            ثابت منطقی isNew = nodep->name() == "new";
            اگر (isNew) m_explicitSuperNewp = خالی_اشاریہ;
            iterateChildren(nodep);
            اگر (isNew) {
                ثابت AstClassExtends* ثابت classExtendsp = VN_AS(m_modp, Class)->extendsp();
                اگر (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
                    && classExtendsp->argsp()) {
                    m_explicitSuperNewp->v3error(
                        "Explicit super.new not allowed with class "
                        "extends arguments (IEEE 1800-2023 8.17)\n"
                        << m_explicitSuperNewp->warn_مزید() << "... Suggest remove super.new\n"
                        << m_explicitSuperNewp->warnContextPrimary() << '\n'
                        << classExtendsp->argsp()->warnOther()
                        << "... Location of extends argument(s)\n"
                        << classExtendsp->argsp()->warnContextSecondary());
                }
                اگر (classExtendsp && classExtendsp->classOrNullp()) {
                    اگر (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->بچہ_DTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_گرہ_Stmt* ثابت superNewp
                            = جوڑو_Implicit_Super_New_Call(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_پچھلا(m_curSymp);
    }
    خالی دورہ(Ast_گرہ_Foreach* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_گرہ_Sym(nodep));
    }
    خالی دورہ(AstRandSequence* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (!nodep->آغاز().خالی_خانے())
            nodep->prodp(ڈھونڈو_Prod(nodep, m_statep->get_گرہ_Sym(nodep), nodep->آغاز()));
        symIterateChildren(nodep, m_statep->get_گرہ_Sym(nodep));
    }
    خالی دورہ(AstRSProd* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_گرہ_Sym(nodep));
    }
    خالی دورہ(Ast_RSProd_شے* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(ڈھونڈو_Prod(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    خالی دورہ(AstCoverWith* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_اگلا_Null(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterate_And_اگلا_Null(nodep->itemp()->بچہ_DTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_گرہ_Sym(nodep);
        iterate_And_اگلا_Null(nodep->itemp());
        iterate_And_اگلا_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_پچھلا(m_curSymp);
    }
    خالی دورہ(AstCoverCrossWith* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_اگلا_Null(nodep->selectp());
        iterate_And_اگلا_Null(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterate_And_اگلا_Null(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_گرہ_Sym(nodep);
        iterate_And_اگلا_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_پچھلا(m_curSymp);
    }
    خالی دورہ(AstWith* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (m_ds.m_unresolvedClass) واپس;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restricted_نام_مستعمل);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_گرہ_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ثابت منطقی پہلا_دورہ = !nodep->validated();
            اگر (پہلا_دورہ) {
                nodep->validated(سچ);
                m_restricted_نام_مستعمل.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                اگر (nodep->restricted() && m_randSymp) {
                    برائے (ثابت std::string& n : nodep->restricted_نام()) {
                        اگر (!m_randSymp->ڈھونڈو_Id_Flat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restricted_نام_مستعمل.داخل_کرو(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            اگر (پہلا_دورہ && nodep->restricted()) {
                برائے (ثابت std::string& n : nodep->restricted_نام()) {
                    اگر (!m_restricted_نام_مستعمل.گنتی(n)) {
                        nodep->v3warn(غیر_مستعمل_2, "Identifier '"
                                                  << n
                                                  << "' in 'with (...)' identifier list is"
                                                     " not referenced in the constraint"
                                                     " block");
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_پچھلا(m_curSymp);
    }
    خالی دورہ(AstLambdaArgRef* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    خالی دورہ(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        اگر (nodep->user3_سیٹ_Once()) واپس;
        // If the class is resolved, there is nothing more to do
        اگر (nodep->classOrNullp()) واپس;
        LINKDOT_دورہ_آغاز();
        اگر (m_statep->forPrimary()) {
            اگر (nodep->بچہ_DTypep()) واپس;
            Ast_گرہ* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            اگر (AstDot* ثابت dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(سچ);
                اگر (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, سچ, جھوٹ, nodep->verilogKwd());
                    اگر (!foundp) {
                        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                        واپس;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_گرہ_Sym(lookNodep->classOrPackageSkipp());
                } ورنہ {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ثابت cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            اگر (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
            VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, سچ,
                                                                    سچ, nodep->verilogKwd());
            اگر (!foundp) {
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
            اگر (AstClass* ثابت classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                اگر (paramsp) {
                    paramsp = paramsp->clone_درخت(سچ);
                    nodep->parameterized(سچ);
                }
                nodep->بچہ_DTypep(نیا AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->بچہ_DTypep());
            } ورنہ اگر (AstParamTypeDType* ثابت paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ثابت refParamp = نیا AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->بچہ_DTypep(refParamp);
                nodep->parameterized(سچ);
            } ورنہ اگر (AstTypedef* ثابت typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_گرہ_DType* ثابت unwrappedp = typedefp->subDTypep()->skipRefp();
                اگر (AstClassRefDType* ثابت classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    اگر (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_درخت(سچ);
                        nodep->parameterized(سچ);
                    } ورنہ اگر (paramsp) {
                        paramsp = paramsp->clone_درخت(سچ);
                        nodep->parameterized(سچ);
                    }
                    nodep->بچہ_DTypep(
                        نیا AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->بچہ_DTypep());
                } ورنہ {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    واپس;
                }
            } ورنہ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                واپس;
            }
            اگر (!nodep->بچہ_DTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlink_Fr_عقب()->delete_درخت();
        } ورنہ {
            // Probably a parameter
            اگر (AstRefDType* ثابت refp = VN_CAST(nodep->بچہ_DTypep(), RefDType)) {
                اگر (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_درخت(جھوٹ));
                    VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                } ورنہ {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ثابت Ast_گرہ_DType* typep = refp->refDTypep();
                    جب (سچ) {
                        اگر (ثابت AstParamTypeDType* ثابت atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            جاری;
                        }
                        اگر (ثابت AstRequireDType* ثابت atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            جاری;
                        }
                        توڑو;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    خالی دورہ(AstClass* nodep) override {
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_modp);
        VL_RESTORER_COPY(m_if_Class_Imp_نام);
        VL_RESTORER_COPY(m_ifForwards);
        VL_RESTORER(m_insideClassExtParam);
        {
            m_ifForwards.clear();
            m_ds.init(m_curSymp);
            m_insideClassExtParam = nodep->isCovergroup() && nodep->covergroupEnclosingClassp()
                                    && m_extendsParam.ڈھونڈو(nodep->covergroupEnclosingClassp())
                                           != m_extendsParam.انتہا();
            // Until overridden by a SCOPE
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_گرہ_Sym(nodep);
            m_modp = nodep;
            صحیح اگلا = 0;
            برائے (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                اگر (++اگلا == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                اگر (m_statep->forPrimary()) {
                    اگر (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.داخل_کرو(nodep);
                        m_insideClassExtParam = سچ;
                    }
                    اگر (AstClassRefDType* ثابت classRefp
                        = VN_CAST(cextp->بچہ_DTypep(), ClassRefDType)) {
                        AstClass* ثابت classp = classRefp->classp();
                        اگر (classp != nodep) iterate(classp);
                        اگر (m_extendsParam.ڈھونڈو(classp) != m_extendsParam.انتہا()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.داخل_کرو(nodep);
                            m_insideClassExtParam = سچ;
                        }
                    }
                }

                اگر (AstClass* ثابت بنیاد_Classp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    اگر (بنیاد_Classp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } ورنہ اگر (extendsClass(بنیاد_Classp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << بنیاد_Classp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } ورنہ اگر (cextp->isImplements() && !بنیاد_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << بنیاد_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } ورنہ اگر (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && بنیاد_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << بنیاد_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    بنیاد_Classp->isExtended(سچ);
                    nodep->isExtended(سچ);
                    iterate(بنیاد_Classp);
                    importSymbolsFromExtended(nodep, cextp);
                    جاری;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            برائے (ثابت خودکار& itr : m_ifForwards) {
                ثابت VSymEnt* ثابت implSymp = itr.دوسرا;
                جوڑو_Interface_Forward(nodep, VN_AS(implSymp->nodep(), گرہ_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            برائے (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->انتہا(); ++it) {
                Ast_گرہ* ثابت itemp = it->دوسرا->nodep();
                اگر (!memberMap.ڈھونڈو_Member(nodep, it->پہلا)) {
                    اگر (Ast_Enum_شے* ثابت aitemp = VN_CAST(itemp, Enum_شے)) {
                        Ast_Enum_شے_Ref* ثابت newp = نیا Ast_Enum_شے_Ref{
                            aitemp->fileline(), aitemp, it->دوسرا->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->پہلا << "' " << newp);
                        nodep->جوڑو_Membersp(newp);
                        memberMap.داخل_کرو(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_پچھلا(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    ساکن خالی collectScopeRefs(Ast_گرہ* nodep, std::سمتیہ<AstClassOrPackageRef*>& refps) {
        اگر (AstClassOrPackageRef* ثابت refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ڈالو_عقب(refp);
            واپس;
        }
        AstDot* ثابت dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    منطقی reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::سمتیہ<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        برائے (سائز_t i = 0; i < refps.سائز(); ++i) {
            AstClassOrPackageRef* ثابت refp = refps[i];
            اگر (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, جھوٹ,
                                                    "class/package reference")) {
                واپس جھوٹ;  // Error already reported
            }
            اگر (i + 1 == refps.سائز()) توڑو;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_گرہ_Module* ثابت modp = refp->classOrPackageSkipp();
            اگر (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                واپس جھوٹ;
            }
            scopeSymp = m_statep->get_گرہ_Sym(modp);
        }
        AstClassOrPackageRef* ثابت innerp = refps.عقب();
        innerp->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(scopeDotp->unlink_Fr_عقب()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        واپس سچ;
    }

    خالی دورہ(AstRefDType* nodep) override {

        اگر (خودکار* ثابت typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            اگر (Ast_گرہ_DType* ثابت resolvedDTypep = VN_CAST(nodep->typeofp(), گرہ_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlink_Fr_عقب();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                اگر (V3LinkDotIfaceCapture::enabled()) {
                    اگر (AstRefDType* ثابت resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        اگر (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            واپس;
        }

        // Resolve its reference
        اگر (m_statep->forParamed() && nodep->user3()) {
            اگر (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(جھوٹ);
        }
        اگر (nodep->user3_سیٹ_Once()) {
            اگر (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            اگر (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            واپس;
        }
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        اگر (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            اگر (AstDot* ثابت scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                اگر (!reduceScopeDot(nodep, scopeDotp)) واپس;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_گرہ* ثابت cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            اگر (AstClassOrPackageRef* ثابت cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ثابت AstClass* ثابت clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                اگر (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    واپس;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                اگر (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    واپس;
                }

                ثابت منطقی doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                اگر (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, سچ, جھوٹ, "class/package reference");
                    اگر (!foundp) واپس;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                اگر (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    اگر (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        واپس;
                    }
                    // Likely impossible, as error thrown earlier
                    cpackagerefp->v3error(  // LCOV_EXCL_LINE
                        "'::' expected to reference a class/package but referenced '"
                        << (nodep->classOrPackagep() ? nodep->classOrPackagep()->prettyTypeName()
                                                     : "<unresolved-object>")
                        << "'\n"
                        << cpackagerefp->warn_مزید() + "... Suggest '.' instead of '::'");
                }
            }
            VL_DO_DANGLING(ڈالو_Deletep(cpackagep->unlink_Fr_عقب()), cpackagep);
        }

        اگر (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            خودکار* ثابت cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } ورنہ {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ثابت منطقی ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            اگر (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } ورنہ {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        اگر (!nodep->typedefp() && !nodep->subDTypep()) {
            ثابت VSymEnt* foundp;
            اگر (nodep->classOrPackagep()) {
                foundp = m_statep->get_گرہ_Sym(nodep->classOrPackagep())->ڈھونڈو_Id_Flat(nodep->name());
            } ورنہ اگر (m_ds.m_dotPos == DP_پہلا || m_ds.m_dotPos == DP_NONE) {
                foundp = ڈھونڈو_Id_Fallback_Skip_Member_DType(m_curSymp, nodep->name());
            } ورنہ {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<صحیح>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = خالی_اشاریہ;  // LCOV_EXCL_LINE
            }
            {
                اگر (AstTypedef* ثابت defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : خالی_اشاریہ) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    اگر (!nodep->classOrPackagep() && !defp->isUnderClass())
                        check_Decl_ترتیب(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } ورنہ اگر (AstParamTypeDType* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : خالی_اشاریہ) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ثابت منطقی is_Param_Type_بچہ = (defp == nodep->backp());
                    ثابت منطقی isTypedefRecursion
                        = is_Param_Type_بچہ && VN_IS(defp->backp(), Typedef);
                    اگر (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dot_متن << (m_ds.m_dot_متن == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->ڈھونڈو_Void_DType());  // Try to reduce later errors
                    } ورنہ {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } ورنہ اگر (AstClass* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : خالی_اشاریہ) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    اگر (!nodep->classOrPackagep()) check_Decl_ترتیب(nodep, defp);
                    AstPin* ثابت paramsp = nodep->paramsp();
                    اگر (paramsp) paramsp->unlink_Fr_عقب_With_اگلا();
                    AstClassRefDType* ثابت newp
                        = نیا AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                } ورنہ اگر (m_insideClassExtParam) {
                    واپس;
                } ورنہ {
                    اگر (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } ورنہ {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        اگر (nodep->lhsp() && !VN_IS(nodep->lhsp(), گرہ_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            اگر (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } ورنہ {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(نیا AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_دورہ_آغاز();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ثابت foundp = m_curSymp->ڈھونڈو_Id_Fallback(nodep->name());
        Ast_گرہ_FTask* ثابت taskp = foundp ? VN_AS(foundp->nodep(), گرہ_FTask) : خالی_اشاریہ;
        اگر (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } ورنہ اگر (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } ورنہ {
            taskp->dpiExport(سچ);
            اگر (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstDisable* nodep) override {
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        منطقی hasPartSelect = جھوٹ;
        ثابت string ہدف_راستہ
            = nodep->ہدف_Refp() ? نکالو_Dotted_راستہ(nodep->ہدف_Refp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_پہلا;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        اگر (nodep->ہدف_Refp()) {
            اگر (AstTaskRef* ثابت taskRefp = VN_CAST(nodep->ہدف_Refp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } ورنہ اگر (!VN_IS(nodep->ہدف_Refp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->ہدف_Refp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
            اگر (nodep->targetp()) {
                nodep->ہدف_Refp()->unlink_Fr_عقب()->delete_درخت();
                اگر (!hasPartSelect) {
                    ثابت string instance_راستہ = ہدف_Instance_راستہ(nodep->targetp(), ہدف_راستہ);
                    اگر (!instance_راستہ.خالی_خانے()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->ہدف_Refp(
                            نیا AstVarXRef{nodep->fileline(), "", instance_راستہ, VAccess::READ});
                    }
                }
            }
        }
    }
    خالی دورہ_Package_Import_Or_Export(Ast_گرہ* nodep) {
        // No longer needed
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        اگر (m_statep->forParamed()) VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstPackageImport* nodep) override { دورہ_Package_Import_Or_Export(nodep); }
    خالی دورہ(AstPackageExport* nodep) override { دورہ_Package_Import_Or_Export(nodep); }
    خالی دورہ(Ast_Package_Export_ستارہ_ستارہ* nodep) override { دورہ_Package_Import_Or_Export(nodep); }
    خالی دورہ(AstCellRef* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstCellArrayRef* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    خالی دورہ(AstUnlinkedRef* nodep) override {
        LINKDOT_دورہ_آغاز();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        اگر (AstNew* ثابت newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } ورنہ اگر (ثابت AstDot* ثابت dotp = VN_CAST(nodep->exprp(), Dot)) {
            اگر (dotp->lhsp()->name() == "super") {
                اگر (AstNew* ثابت newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstIfaceRefDType* nodep) override {
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        اگر (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ثابت ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            اگر (ifacep->dead()) واپس;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_مستعمل_Pins);
            VL_RESTORER_CLEAR(m_مستعمل_Def_Param_Pins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_گرہ_Sym(ifacep);
            iterate_And_اگلا_Null(nodep->paramsp());
        }
    }

    خالی دورہ(Ast_Attr_کا* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstCastParse* nodep) override {
        اگر (nodep->user3_سیٹ_Once()) واپس;
        LINKDOT_دورہ_آغاز();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ثابت منطقی dotLhs = m_ds.m_dotPos == DP_پہلا;
        اگر (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        اگر (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    خالی دورہ(Ast_تفویض_W* nodep) override {
        LINKDOT_دورہ_آغاز();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        اگر (nodep->user2()) m_replaceWithAlias = جھوٹ;
        iterateChildren(nodep);
    }

    خالی دورہ(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = سچ;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override {
        VL_RESTORER(m_اندر_Packed_Array);
        اگر (VN_IS(nodep, PackArrayDType)) {
            m_اندر_Packed_Array = سچ;
        } ورنہ اگر (!m_اندر_Packed_Array) {
            LINKDOT_دورہ_آغاز();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

عوامی:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ثابت std::map<std::string, Ast_گرہ_Module*> modules_مقصد_Revisit
            = std::move(m_modules_مقصد_Revisit);
        m_آخری_Deferredp = خالی_اشاریہ;
        برائے (خودکار& p : modules_مقصد_Revisit) {
            Ast_گرہ_Module* ثابت modp = p.دوسرا;
            modp->foreach([](Ast_گرہ* ثابت nodep) { nodep->user3(جھوٹ); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = طےشدہ;
};

//######################################################################
// Link class functions

خالی V3LinkDot::dumpSubstep(ثابت string& name) {
    اگر (dump_درخت_Either_Level() >= 9) {
        V3Global::dump_Check_Global_درخت(name);
    } ورنہ اگر (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_درخت_File(v3Global.debugFilename(name + ".tree"));
    }
}

خالی V3LinkDot::linkDotGuts(AstNetlist* rootp, VLink_Dot_قدم قدم) {
    VIsCached::clear_Cache_درخت();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, قدم};

    { Link_Dot_ڈھونڈو_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { Link_Dot_ڈھونڈو_Iface_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    اگر (قدم == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(سچ);
        dumpSubstep("prelinkdot-scoped");
    } ورنہ {
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

خالی V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_درخت("linkdot", 0, dump_درخت_Either_Level() >= 6);
}

خالی V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_درخت("linkdotparam", 0, dump_درخت_Either_Level() >= 3);
}

خالی V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_درخت("linkdot", 0, dump_درخت_Either_Level() >= 3);
}
