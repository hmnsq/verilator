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
//بالاترین
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

ایستا string extractDottedPath(Ast_گره* nodep, منطقی& hasPartSelect) {
    اگر (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) {
        بازگشت refp->name();
    } وگرنه اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
        بازگشت refp->name();
    } وگرنه اگر (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
        ثابت string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        ثابت string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        بازگشت VString::dot(lhs, ".", rhs);
    } وگرنه اگر (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = درست;
        بازگشت "";
    }
    بازگشت "";
}
ایستا string lexicalDisablePath(Ast_گره* ثابت targetp) {
    std::vector<string> names;
    برای (Ast_گره* curp = targetp; curp; curp = curp->aboveLoopp()) {
        اگر (Ast_گره_Block* ثابت blockp = VN_CAST(curp, گره_Block)) {
            اگر (blockp->name() != "") names.افزودن_back(blockp->name());
        } وگرنه اگر (Ast_گره_FTask* ثابت ftaskp = VN_CAST(curp, گره_FTask)) {
            names.افزودن_back(ftaskp->name());
        } وگرنه اگر (VN_IS(curp, گره_Module)) {
            بشکن;
        }
    }
    string path;
    برای (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    بازگشت path;
}
ایستا string targetInstancePath(Ast_گره* ثابت targetp, ثابت string& targetPath) {
    ثابت string lexicalPath = lexicalDisablePath(targetp);
    اگر (lexicalPath == "" || targetPath == lexicalPath) بازگشت "";
    ثابت string suffix = "." + lexicalPath;
    اگر (!VString::endsWith(targetPath, suffix)) بازگشت "";
    بازگشت targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

رده Link_گره_Matcher_Class_Or_Package final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override {
        بازگشت VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
رده Link_گره_Matcher_FTask final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override { بازگشت VN_IS(nodep, گره_FTask); }
};
رده Link_گره_Matcher_Modport final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override { بازگشت VN_IS(nodep, Modport); }
};
رده Link_گره_Matcher_Prod final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override { بازگشت VN_IS(nodep, RSProd); }
};
رده Link_گره_Matcher_Var final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override {
        بازگشت VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
رده Link_گره_Matcher_Var_IO final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        اگر (!varp) بازگشت نادرست;
        بازگشت varp->isIO();
    }
};
رده Link_گره_Matcher_Var_Param final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        اگر (!varp) بازگشت نادرست;
        بازگشت varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

رده LinkDotState final {
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
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser4InUse m_inuser4;

عمومی:
    // ENUMS
    // In order of priority, compute first ... compute last
    شمارش SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

خصوصی:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    ایستا LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_گره_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ثابت VLinkDotStep m_step;  // Operational step

عمومی:
    // METHODS

    تهی dumpSelf(ثابت string& nameComment, منطقی force = نادرست) {
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
            منطقی first = درست;
            برای (صحیح samn = 0; samn < SAMN__MAX; ++samn) {
                اگر (!m_scopeAliasMap[samn].empty()) {
                    اگر (first) os << "\nScopeAliasMap:\n";
                    first = نادرست;
                    برای (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                         it != m_scopeAliasMap[samn].end(); ++it) {
                        // left side is what we will import into
                        os << "\t" << samn << "\t" << it->first << " ("
                           << it->first->nodep()->typeName() << ") <- " << it->second << " "
                           << it->second->nodep() << '\n';
                    }
                }
            }
        }
    }
    ایستا تهی preErrorDumpHandler() {
        اگر (s_errorThisp) s_errorThisp->preErrorDump();
    }
    تهی preErrorDump() {
        ایستا منطقی s_diddump = نادرست;
        اگر (!s_diddump && dump_درخت_Level()) {
            s_diddump = درست;
            dumpSelf("linkdot-preerr", درست);
            v3Global.rootp()->dump_درخت_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = این;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { بازگشت &m_syms; }
    صحیح step_عدد() ثابت { بازگشت static_cast<صحیح>(m_step); }
    منطقی forPrimary() ثابت { بازگشت m_step == LDS_PRIMARY; }
    منطقی forParamed() ثابت { بازگشت m_step == LDS_PARAMED; }
    منطقی forScopeCreation() ثابت { بازگشت m_step == LDS_SCOPED; }

    // METHODS
    ایستا string گره_Text_Type(Ast_گره* nodep) {
        اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            اگر (varp->isIO() || varp->isIfaceRef()) {
                بازگشت "port";
            } وگرنه اگر (varp->isGParam()) {
                بازگشت "parameter";
            } وگرنه اگر (varp->varType() == VVarType::LPARAM) {
                بازگشت "local parameter";
            } وگرنه {
                بازگشت "variable";
            }
        } وگرنه اگر (ثابت AstParamTypeDType* ثابت dtypep = VN_CAST(nodep, ParamTypeDType)) {
            اگر (dtypep->isGParam()) {
                بازگشت "type parameter";
            } وگرنه {
                بازگشت "local type parameter";
            }
        } وگرنه اگر (VN_IS(nodep, Begin)) {
            بازگشت "block";
        } وگرنه اگر (VN_IS(nodep, Cell)) {
            بازگشت "instance";
        } وگرنه اگر (VN_IS(nodep, Constraint)) {
            بازگشت "constraint";
        } وگرنه اگر (VN_IS(nodep, Func)) {
            بازگشت "function";
        } وگرنه اگر (VN_IS(nodep, GenBlock)) {
            بازگشت "generate block";
        } وگرنه اگر (VN_IS(nodep, Iface)) {
            بازگشت "interface";
        } وگرنه اگر (VN_IS(nodep, RSProd)) {
            بازگشت "randsequence production";
        } وگرنه اگر (VN_IS(nodep, Task)) {
            بازگشت "task";
        } وگرنه {
            بازگشت nodep->prettyTypeName();
        }
    }

    Ast_گره_Module* findModuleSym(ثابت string& modName) {
        ثابت VSymEnt* ثابت foundp = m_mods.rootp()->findIdFallback(modName);
        بازگشت foundp ? VN_AS(foundp->nodep(), گره_Module) : nullptr;
    }
    تهی readModNames() {
        // Look at all modules, and store pointers to all module names
        برای (Ast_گره_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گره_Module);
            m_mods.rootp()->درج(nodep->name(), جدید VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* ریشه_Entp() ثابت { بازگشت m_syms.rootp(); }
    VSymEnt* dunitEntp() ثابت { بازگشت m_dunitEntp; }
    تهی checkDuplicate(VSymEnt* lookupSymp, Ast_گره* nodep, ثابت string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ثابت VSymEnt* ثابت foundp = lookupSymp->findIdFlat(name);
        Ast_گره* ثابت fnodep = foundp ? foundp->nodep() : nullptr;
        اگر (!fnodep) {
            // Not found, will add in a moment.
            اگر (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ثابت VSymEnt* ثابت alt = lookupSymp->findSimilarIdFlat(name);
                اگر (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } وگرنه اگر (nodep == fnodep) {  // Already inserted.
            // Good.
        } وگرنه اگر (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } وگرنه اگر (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } وگرنه {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ثابت string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_گره::prettyNameQ(name);
            اگر (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << گره_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_رقم_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_رقم_Set_Min(0);  // Disable checking forward typedefs
            } وگرنه {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(گره_Text_Type(nodep)) << " has the same name as "
                               << گره_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    تهی درج_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ثابت symp = جدید VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(ریشه_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        ریشه_Entp()->درج("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* درج_بالاترین_Cell(Ast_گره_Module* nodep, ثابت string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ثابت symp = جدید VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ریشه_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(ریشه_Entp(), nodep, nodep->origName());
        ریشه_Entp()->درج(nodep->origName(), symp);
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        بازگشت symp;
    }
    VSymEnt* درج_بالاترین_Iface(AstCell* nodep, ثابت string& scopename) {
        VSymEnt* ثابت symp = جدید VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ریشه_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        اگر (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(ریشه_Entp(), nodep, nodep->origName());
        ریشه_Entp()->درج(nodep->origName(), symp);
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        بازگشت symp;
    }
    VSymEnt* درج_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ثابت string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جدید VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        اگر (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        اگر (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        بازگشت symp;
    }
    تهی درج_Map(VSymEnt* symp, ثابت string& scopename) {
        اگر (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* درج_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ثابت string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جدید VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        اگر (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        بازگشت symp;
    }
    VSymEnt* درج_Block(VSymEnt* abovep, ثابت string& name, Ast_گره* nodep,
                         Ast_گره_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جدید VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        اگر (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        بازگشت symp;
    }
    VSymEnt* درج_Sym(VSymEnt* abovep, ثابت string& name, Ast_گره* nodep,
                       Ast_گره_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جدید VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTsym se" << cvtToHex(symp) << "  name='" << name << "' above=se"
                                      << cvtToHex(abovep) << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        // We don't remember the ent associated with each node, because we
        // need a unique scope entry for each instantiation
        symp->classOrPackagep(classOrPackagep);
        symp->parentp(abovep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, name);
        abovep->reinsert(name, symp);
        بازگشت symp;
    }
    ایستا منطقی exists_گره_Sym(Ast_گره* nodep) { بازگشت nodep->user1p() != nullptr; }
    ایستا VSymEnt* get_گره_Sym(Ast_گره* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ثابت symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        بازگشت symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ثابت auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        بازگشت it->second;
    }
    تهی implicitOkAdd(Ast_گره_Module* nodep, ثابت string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        اگر (nodep) {
            ثابت auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            اگر (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    منطقی implicitOk(Ast_گره_Module* nodep, ثابت string& varname) {
        بازگشت nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    تهی درج_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    تهی computeIfaceModSyms();

    // Track and later insert interface references
    تهی درج_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.افزودن_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ایستا AstIfaceRefDType* ifaceRefFromArray(Ast_گره_DType* nodep) {
        وقتی (nodep) {
            اگر (AstIfaceRefDType* ثابت ifp = VN_CAST(nodep, IfaceRefDType)) بازگشت ifp;
            اگر (ثابت AstBracketArrayDType* ثابت arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } وگرنه اگر (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } وگرنه {
                بازگشت nullptr;
            }
        }
        بازگشت nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_عبارت(Ast_گره* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } وگرنه اگر (AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ثابت lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            اگر (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                ثابت string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ثابت dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, درست);
                اگر (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        اگر (resolvedVarp) {
            AstIfaceRefDType* ثابت irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            اگر (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        بازگشت resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    منطقی repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, منطقی isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        برای (صحیح depth = 0; parentSymp && depth < 20; ++depth) {
            اگر (AstCell* ثابت cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                بشکن;
            }
            parentSymp = parentSymp->parentp();
        }
        اگر (!parentCellp) بازگشت نادرست;
        // Scan pins to find the one connected to this port variable
        برای (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->modVarp() != varp && pinp->name() != varp->name()) ادامه;
            Ast_گره* ثابت exprp = pinp->exprp();
            اگر (!exprp) بازگشت نادرست;
            AstIface* ثابت newIfacep = live_Iface_From_Pin_عبارت(exprp, parentSymp);
            اگر (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                بازگشت درست;
            }
            اگر (newIfacep) بازگشت !isDead;  // Same interface - OK if live
            بازگشت نادرست;
        }
        بازگشت نادرست;
    }

    تهی computeIfaceVarSyms() {
        برای (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ثابت varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* ثابت ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            اگر (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_رقم() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            اگر (!ifacerefp->ifaceViaCellp()) {
                اگر (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_گره::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_گره::prettyNameQ(ifacerefp->ifaceName()));
                    ادامه;
                } وگرنه {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } وگرنه اگر (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ثابت منطقی isDead = ifacerefp->ifaceViaCellp()->dead();
                اگر (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.بالاترین_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ثابت منطقی repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                اگر (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_گره::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    ادامه;
                }
            } وگرنه اگر (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_گره_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ثابت ifaceSymp = get_گره_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            اگر (ifacerefp->isModport()) {
                VSymEnt* ثابت foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                منطقی ok = نادرست;
                اگر (foundp) {
                    اگر (AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = درست;
                    }
                }
                اگر (!ok) {
                    ثابت string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_گره_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            درج_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    تهی درج_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    تهی computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        برای (صحیح samn = 0; samn < SAMN__MAX; ++samn) {
            برای (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* ثابت lhsp = it->first;
                VSymEnt* srcp = lhsp;
                وقتی (درست) {  // Follow chain of aliases up to highest level non-alias
                    ثابت auto it2 = m_scopeAliasMap[samn].find(srcp);
                    اگر (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        ادامه;
                    } وگرنه {
                        بشکن;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                اگر (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), درست);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

خصوصی:
    VSymEnt* findWithAltFallback(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        اگر (findp) بازگشت findp;
        اگر (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        بازگشت findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        اگر (findp) بازگشت findp;
        اگر (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        بازگشت findp;
    }

عمومی:
    VSymEnt* findDotted(File_سطر* refLocationp, VSymEnt* lookupSymp, ثابت string& dotname,
                        string& baddot, VSymEnt*& okSymp, منطقی firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        وقتی (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            اگر (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } وگرنه {
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
                وقتی (VString::endsWith(altIdent, "__KET__")) {
                    ثابت auto braPos = altIdent.rfind("__BRA__");
                    اگر (braPos == string::npos) بشکن;
                    altIdent = altIdent.substr(0, braPos);
                }
                اگر (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            اگر (firstId) {
                // Check this module - subcellnames
                ثابت AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                اگر (VSymEnt* ثابت findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                وگرنه اگر (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                وگرنه اگر (ident == "$root") {
                    lookupSymp = ریشه_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    اگر (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                وگرنه {
                    منطقی crossedCell = نادرست;  // Crossed a cell boundary
                    وقتی (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        اگر (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            اگر (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = درست;
                            }
                            اگر (cellp && cellp->modp()->origName() == ident) {
                                بشکن;
                            } وگرنه اگر (VSymEnt* ثابت findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                اگر (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    بازگشت nullptr;
                                }
                                بشکن;
                            }
                        } وگرنه {
                            بشکن;
                        }
                    }
                    اگر (!lookupSymp) بازگشت nullptr;  // Not found
                }
            } وگرنه {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                اگر (findSymp) {
                    lookupSymp = findSymp;
                } وگرنه {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/درست);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    اگر (findSymp) {
                        // Check if this is an interface reference variable
                        ثابت AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        اگر (!varp) {
                            اگر (ثابت AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        اگر (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } وگرنه {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            اگر (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } وگرنه {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                بازگشت nullptr;
                            }
                        }
                    } وگرنه {
                        بازگشت nullptr;  // Not found
                    }
                }
            }
            اگر (lookupSymp) {
                اگر (ثابت AstCell* ثابت cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    اگر (ثابت Ast_گره_Module* ثابت modp = cellp->modp()) {
                        اگر (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } وگرنه اگر (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                اگر (!leftname.empty()) {
                    ثابت auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    اگر (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        اگر (ثابت AstVarScope* ثابت vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ثابت string varName = vscp->varp()->name();
                            ایستا constexpr ثابت نویسه* ثابت VIFTOP_SUFFIX = "__Viftop";
                            اگر (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                ثابت string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* ثابت parentSymp = lookupSymp->parentp();
                                اگر (parentSymp) {
                                    VSymEnt* ثابت cellSymp = parentSymp->findIdFlat(cellName);
                                    اگر (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } وگرنه {
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
                                اگر (ifaceRefp->cellp() && exists_گره_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_گره_Sym(ifaceRefp->cellp());
                                } وگرنه اگر (ifaceRefp->ifaceViaCellp()
                                           && exists_گره_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_گره_Sym(ifaceRefp->ifaceViaCellp());
                                } وگرنه اگر (ifaceRefp->ifacep()
                                           && exists_گره_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_گره_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = نادرست;
        }
        بازگشت lookupSymp;
    }

    ایستا string removeLastInlineScope(ثابت string& name) {
        string out = name;
        ثابت string dot = "__DOT__";
        ثابت size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        اگر (dotPos == string::npos) {
            بازگشت "";
        } وگرنه {
            بازگشت out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, ثابت string& dotname, string& baddot,
                             منطقی fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        اگر (!lookupSymp) بازگشت nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        وقتی (!foundp) {
            اگر (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } وگرنه {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            اگر (prefix.empty()) بشکن;
            ثابت string nextPrefix = removeLastInlineScope(prefix);
            اگر (prefix == nextPrefix) بشکن;
            prefix = nextPrefix;
        }
        اگر (!foundp) baddot = dotname;
        بازگشت foundp;
    }
    ایستا منطقی checkIfClassOrPackage(ثابت VSymEnt* ثابت symp) {
        اگر (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) بازگشت درست;

        // Helper: check if a RefDType might resolve to a class later.
        ثابت auto isPotentialClassRef = [](ثابت AstRefDType* refp) -> منطقی {
            بازگشت refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ثابت auto is_Valid_Type_گره = [](ثابت Ast_گره* nodep) -> منطقی {
            بازگشت VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        اگر (ثابت AstTypedef* ثابت typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ثابت Ast_گره_DType* dtypep = typedefp->subDTypep();
            اگر (!dtypep) dtypep = typedefp->childDTypep();
            اگر (VN_IS(dtypep, ClassRefDType)) بازگشت درست;
            اگر (isPotentialClassRef(VN_CAST(dtypep, RefDType))) بازگشت درست;
        } وگرنه اگر (ثابت AstParamTypeDType* ثابت paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_گره* childp = paramTypep->childDTypep();
            اگر (ثابت AstRequireDType* ثابت reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ثابت Ast_گره* ثابت checkp = childp ? childp : paramTypep->skipRefp();
            اگر (is_Valid_Type_گره(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                بازگشت درست;
            }
        }
        بازگشت نادرست;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, منطقی fallback,
                                   منطقی classOnly, ثابت string& forWhat,
                                   منطقی deferIfUnresolved = نادرست) {
        اگر (nodep->classOrPackageSkipp()) بازگشت get_گره_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* جستجو_Symp = lookSymp;
        اگر (VL_UNCOVERABLE(جستجو_Symp && VN_IS(جستجو_Symp->nodep(), ParamTypeDType))) {
            جستجو_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        اگر (fallback) {
            VSymEnt* currentLookSymp = جستجو_Symp;
            انجامبده {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                اگر (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                اگر (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } وقتی (!foundp && currentLookSymp);
        } وگرنه {
            foundp = جستجو_Symp->findIdFlat(nodep->name());
            اگر (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        اگر (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_گره_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        اگر (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            بازگشت foundp;
        }
        اگر (deferIfUnresolved) بازگشت nullptr;
        ثابت string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_گره_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        بازگشت nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ثابت string& name,
                              ثابت VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        اگر (!lookupSymp) بازگشت "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        بازگشت speller.best_Candidate_پیام(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ثابت string& name, ثابت VNodeMatcher& matcher,
                          منطقی includeType) {
        اگر (!lookupSymp) بازگشت "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        بازگشت speller.best_Candidate_پیام(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

رده LinkDotFindVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_گره_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ثابت Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    منطقی m_inRecursion = نادرست;  // Inside a recursive module
    صحیح m_param_رقم = 0;  // Parameter number, for position based connection
    منطقی m_explicitNew = نادرست;  // Hit a "new" function
    صحیح m_mod_Block_رقم = 0;  // Begin block number in module, 0=none seen
    صحیح m_mod_With_رقم = 0;  // With block number, 0=none seen
    صحیح m_mod_Arg_رقم = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    تهی makeImplicitNew(AstClass* nodep) {
        AstFunc* ثابت newp = جدید AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(درست);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->درج_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ایستا منطقی isHierBlockWrapper(ثابت string& name) {
        ثابت V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        بازگشت hierBlocks.find(name) != hierBlocks.end();
    }

    تهی moveExternFuncDecl(Ast_گره_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        اگر (nodep->isExternDef()) بازگشت;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(درست);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_درخت();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        اگر (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    تهی moveExternFuncDeclRefs(Ast_گره* nodep, AstClass* classp) {
        nodep->foreach([این, classp](AstClassOrPackageRef* refp) {  //
            اگر (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ثابت dotp = VN_CAST(refp->backp(), Dot);
                اگر (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(افزودن_Deletep(dotp), dotp, refp);
                } وگرنه {
                    VL_DO_DANGLING(افزودن_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->درج_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_گره_Module* ثابت modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        اگر (m_statep->forPrimary()) {
            ثابت منطقی onlyBuiltins = [&]() {
                برای (Ast_گره_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), گره_Module)) {
                    اگر (!modp->isDollarUnit() && !modp->isConstPool()) بازگشت نادرست;
                }
                بازگشت درست;
            }();
            اگر (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        برای (Ast_گره_Module* modp = nodep->modulesp(); modp && modp->is_بالاترین();
             modp = VN_AS(modp->nextp(), گره_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            اگر (!m_statep->forScopeCreation() && v3Global.opt.بالاترین_Ifaces_Supported()) {
                برای (Ast_گره* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    اگر (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                        اگر (varp->isIfaceRef()) {
                            ثابت Ast_گره_DType* ثابت subtypep = varp->subDTypep();
                            ثابت AstIfaceRefDType* ifacerefp = nullptr;
                            اگر (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } وگرنه اگر (VN_IS(subtypep, BracketArrayDType)) {
                                ثابت AstBracketArrayDType* ثابت arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ثابت Ast_گره_DType* ثابت arrsubtypep = arrp->subDTypep();
                                اگر (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } وگرنه اگر (VN_IS(subtypep, UnpackArrayDType)) {
                                ثابت AstUnpackArrayDType* ثابت arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ثابت Ast_گره_DType* ثابت arrsubtypep = arrp->subDTypep();
                                اگر (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            اگر (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = جدید AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->درج_بالاترین_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(افزودن_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->درج_بالاترین_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    تهی visit(AstTypeTable*) override {}  // FindVisitor::
    تهی visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        اگر (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.افزودن_back(nodep->ifacep());
            nodep->ifacep()->user3(درست);
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {  // FindVisitor::
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
        VL_RESTORER(m_param_رقم);
        VL_RESTORER(m_mod_Block_رقم);
        VL_RESTORER(m_mod_With_رقم);
        VL_RESTORER(m_mod_Arg_رقم);
        اگر (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_گره::prettyNameQ(nodep->origName()));
        } وگرنه اگر (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ثابت pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            اگر (standalonePkg) {
                اگر (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } وگرنه {
                    VSymEnt* ثابت upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->درج_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_رقم = 0;
            m_mod_Block_رقم = 0;
            m_mod_With_رقم = 0;
            m_mod_Arg_رقم = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(درست);
            iterateChildren(nodep);
            nodep->user2(نادرست);
            nodep->user4(درست);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            اگر (!m_statep->forScopeCreation()) {
                اگر (AstIface* ثابت ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->درج_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } وگرنه اگر (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ثابت upperSymp = m_curSymp ? m_curSymp : m_statep->ریشه_Entp();
            m_curSymp = m_modSymp
                = m_statep->درج_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(درست);
        } وگرنه {  // !doit
            اگر (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                برای (ثابت Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (ثابت AstTypedef* ثابت tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ثابت upperSymp = m_curSymp ? m_curSymp : m_statep->ریشه_Entp();
                        m_curSymp = m_modSymp = m_statep->درج_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } وگرنه {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    تهی visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_رقم);
        VL_RESTORER(m_mod_Block_رقم);
        VL_RESTORER(m_mod_With_رقم);
        VL_RESTORER(m_mod_Arg_رقم);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ثابت upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->درج_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->درج_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_رقم = 0;
            m_mod_Block_رقم = 0;
            m_mod_With_رقم = 0;
            m_mod_Arg_رقم = 0;
            m_explicitNew = نادرست;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(درست);
            // Implicit new needed?
            اگر (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    تهی visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        اگر (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    تهی visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        اگر (!nodep->is_بالاترین()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.افزودن_back(nodep);
        }
    }
    تهی visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        اگر (nodep->recursive() && m_inRecursion) بازگشت;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_رقم);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ثابت string origname = Ast_گره::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        اگر (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ثابت string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, نادرست);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_گره::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->درج_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            اگر (nodep->modp()) iterate(nodep->modp());
        }
    }
    تهی visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ثابت std::string::size_type pos = ident.rfind("__DOT__");
        اگر (pos != std::string::npos) {
            ثابت std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             نادرست);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_گره::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->درج_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    تهی visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    تهی visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    تهی visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        اگر (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            برای (Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                اگر (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ثابت std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_عدد()) + "_";
                    انجامبده {
                        ++m_mod_Block_رقم;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_رقم);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } وقتی (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    بشکن;
                }
            }
        }
        اگر (nodep->name() == "") {
            iterateChildren(nodep);
        } وگرنه {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->درج_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    تهی visit(Ast_گره_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        اگر (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ثابت auto containsDecl = [](ثابت Ast_گره* nodesp) -> منطقی {
                برای (ثابت Ast_گره* np = nodesp; np; np = np->nextp()) {
                    اگر (VN_IS(np, Var) || VN_IS(np, Foreach)) بازگشت درست;
                }
                بازگشت نادرست;
            };
            منطقی needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            اگر (ثابت AstFork* ثابت forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            اگر (needName) {
                ثابت std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_عدد()) + "_";
                std::string name;
                انجامبده {
                    ++m_mod_Block_رقم;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_رقم);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } وقتی (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        اگر (nodep->name() == "") {
            iterateChildren(nodep);
        } وگرنه {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->درج_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    تهی moveIfaceExportBody(Ast_گره_FTask* nodep) {
        ثابت string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ثابت portSymp = m_modSymp->findIdFallback(portName);
        اگر (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        AstVar* ثابت portp = VN_CAST(portSymp->nodep(), Var);
        اگر (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            بازگشت;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        اگر (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        اگر (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        اگر (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        ثابت string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_گره_Module* ثابت ifaceModp = m_statep->findModuleSym(ifaceName);
        اگر (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            بازگشت;
        }
        // Find the prototype task in the interface
        Ast_گره_FTask* protoTaskp = nullptr;
        برای (Ast_گره* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            اگر (Ast_گره_FTask* ثابت ftaskp = VN_CAST(itemp, گره_FTask)) {
                اگر (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    بشکن;
                }
            }
        }
        اگر (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_گره* ثابت nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            اگر (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isIO()) {
                    stmtp = nextp;
                    ادامه;
                }
            }
            // Clone this statement and rewrite port references
            Ast_گره* ثابت newStmtp = stmtp->clone_درخت(نادرست);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(نادرست);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ایستا تهی rewriteIfacePortRefsSingle(Ast_گره* nodep, ثابت string& portName) {
        اگر (!nodep) بازگشت;
        // Check if this is a non-colon Dot with lhs matching portName
        اگر (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
            اگر (!dotp->colon()) {
                ثابت AstParseRef* ثابت lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                اگر (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_گره* ثابت rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_درخت(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    بازگشت;
                }
            }
        }
        // Recurse into all children
        برای (Ast_گره* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            وقتی (childp) {
                Ast_گره* ثابت nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    تهی visit(Ast_گره_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        اگر (nodep->name() == "new") m_explicitNew = درست;
        // Handle out-of-block interface method definition (IEEE 25.8)
        اگر (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            بازگشت;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        اگر (VN_IS(m_curSymp->nodep(), Class)) {
            اگر (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
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
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, درست, نادرست,
                                                "External definition :: reference");
                AstClass* ثابت lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                اگر (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وگرنه {
                    m_curSymp = m_statep->get_گره_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, درست, نادرست,
                                                    "External definition :: reference");
                    AstClass* ثابت rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    اگر (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } وگرنه {
                        m_curSymp = m_statep->get_گره_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } وگرنه اگر (cpackagerefp) {
                اگر (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, درست, نادرست,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                اگر (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وگرنه {
                    m_curSymp = m_statep->get_گره_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } وگرنه {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        اگر (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->درج_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        اگر (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_گره_DType* dtypep = VN_CAST(nodep->fvarp(), گره_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            اگر (dtypep) {
                dtypep->unlinkFrBack();
            } وگرنه {
                dtypep = جدید AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ثابت newvarp
                = جدید AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(درست);
            newvarp->trace(نادرست);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->درج_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        اگر (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->درج_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        اگر (!nodep->name().empty()) {
            itSymp = m_statep->درج_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        اگر (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    تهی visit(AstClockingItem* nodep) override {  // FindVisitor::
        اگر (nodep->varp()) {
            اگر (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            بازگشت;
        }
        std::string varname;
        Ast_گره_DType* dtypep;
        اگر (AstAssign* ثابت assignp = nodep->assignp()) {
            Ast_گره_عبارت* ثابت rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = جدید AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_عبارت{},
                                     rhsp->clone_درخت(نادرست)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_درخت(), assignp);
        } وگرنه {
            Ast_گره_عبارت* ثابت refp = nodep->exprp();
            ثابت VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            اگر (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
                بازگشت;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_درخت(نادرست);
        }
        AstVar* ثابت newvarp = جدید AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    تهی visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        اگر (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            اگر (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } وگرنه {
                اگر (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, درست, نادرست,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                اگر (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وگرنه {
                    m_curSymp = m_statep->get_گره_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_درخت();
                }
            }
        }
        // Set the class as package for iteration
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->درج_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    تهی visit(AstVar* nodep) override {  // FindVisitor::
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
            ثابت VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            اگر (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            اگر (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* ثابت itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* ثابت finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                اگر (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::درون_داد
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            منطقی ins = نادرست;
            اگر (!foundp) {
                ins = درست;
            } وگرنه اگر (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::گره_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } وگرنه اگر (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                اگر (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    اگر (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        بازگشت;
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
                            ایستا صحیح s_didAnsiWarn = نادرست;
                            اگر (s_didAnsiWarn++) ansiWarn = نادرست;
                        }
                        nodep->v3error("Duplicate declaration of signal: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (ansiWarn ? nodep->warnMore()
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
                    } وگرنه {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(درست);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        اگر (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{درست});
                        }
                        Ast_گره_DType* ثابت varDtp = findvarp->subDTypep();
                        Ast_گره_DType* ثابت otherDtp = nodep->subDTypep();
                        اگر (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_درخت(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } وگرنه اگر (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* ثابت newp
                                = جدید AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
                } وگرنه {
                    // User can disable the message at either point
                    اگر (!(m_ftaskp && m_ftaskp->dpiImport())
                        && (!m_ftaskp || m_ftaskp != foundp->nodep())  // Not the function's
                                                                       // variable hiding function
                        && !nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                        && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                        nodep->v3warn(VARHIDDEN,
                                      "Declaration of signal hides declaration in upper scope: "
                                          << nodep->prettyNameQ() << '\n'
                                          << nodep->warnContextPrimary() << '\n'
                                          << foundp->nodep()->warnOther()
                                          << "... Location of original declaration\n"
                                          << foundp->nodep()->warnContextSecondary());
                    }
                    ins = درست;
                }
            }
            اگر (ins) {
                اگر (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->ریشه_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    اگر (v3Global.opt.hasParameter(nodep->name())) {
                        ثابت string svalue = v3Global.opt.parameter(nodep->name());
                        اگر (AstConst* ثابت valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            اگر (nodep->valuep()) افزودن_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, درست);
                        }
                    }
                }
                m_statep->درج_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                اگر (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_رقم;
                    VSymEnt* ثابت symp
                        = m_statep->درج_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_رقم),
                                              nodep, m_classOrPackagep);
                    symp->exported(نادرست);
                }
            }
        }
    }
    تهی visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        اگر (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(درست);
        iterateChildren(nodep);
        m_statep->درج_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    تهی visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->درج_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_رقم()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    تهی visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->درج_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    تهی visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        اگر (!m_hierParamsName.empty()) {
            اگر (ثابت VSymEnt* ثابت typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                ثابت AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                برای (ثابت Ast_گره* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    ثابت AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    اگر (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_گره_DType* ثابت newDtp = tdefp->childDTypep();
                        Ast_گره_DType* ثابت oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_درخت(نادرست));
                        VL_DO_DANGLING(oldDtp->delete_درخت(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->درج_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        اگر (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_رقم;
            VSymEnt* ثابت symp = m_statep->درج_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_رقم), nodep, m_classOrPackagep);
            symp->exported(نادرست);
        }
    }
    تهی visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    تهی visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ثابت VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        اگر (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        منطقی ins = نادرست;
        اگر (!foundp) {
            ins = درست;
        } وگرنه اگر (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            اگر (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } وگرنه {
                // User can disable the message at either point
                اگر (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = درست;
            }
        }
        اگر (ins) m_statep->درج_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    تهی visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        اگر (!nodep->packagep()) بازگشت;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->get_گره_Sym(nodep->packagep());
        اگر (nodep->name() == "*") {
            اگر (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                اگر (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } وگرنه اگر (!nodep->resolvedClassp()) {
            VSymEnt* ثابت impp = srcp->findIdFlat(nodep->name());
            اگر (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } وگرنه اگر (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } وگرنه اگر (AstClass* ثابت classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    جدید AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    تهی visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        اگر (!nodep->packagep()) بازگشت;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->get_گره_Sym(nodep->packagep());
        اگر (nodep->name() != "*") {
            VSymEnt* ثابت impp = srcp->findIdFlat(nodep->name());
            اگر (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    تهی visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    تهی visit(Ast_گره_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_رقم;
            m_curSymp = m_statep->درج_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_رقم),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            برای (Ast_گره *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                اگر (AstParseRef* ثابت parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = جدید AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_درخت(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->درج_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } وگرنه اگر (ثابت auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->درج_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } وگرنه اگر (VN_IS(argp, Empty)) {
                } وگرنه {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    تهی visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->درج_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    تهی visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->درج_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        اگر (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        برای (Ast_گره* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(درست);
        }
        iterateChildren(nodep);
    }

    تهی visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_گره_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), گره_FTask_Ref);
        اگر (ثابت AstDot* ثابت dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), گره_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_سطر* argFl = nodep->fileline();
        AstArg* ثابت argsp = funcrefp->argsp();
        اگر (argsp) {
            argsp->unlinkFrBackWithNext();
            اگر (funcrefp->name() != "randomize") {
                اگر (ثابت auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } وگرنه {
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
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }
        ثابت منطقی restrictedRandomize = nodep->restricted();
        اگر (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ثابت indexArgRefp
                = جدید AstLambdaArgRef{argFl, name + "__DOT__index", درست};
            AstLambdaArgRef* ثابت مقدار_Arg_Refp = جدید AstLambdaArgRef{argFl, name, نادرست};
            AstWith* ثابت newp
                = جدید AstWith{nodep->fileline(), indexArgRefp, مقدار_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            اگر (restrictedRandomize) {
                newp->restricted(درست);
                وقتی (Ast_گره* ثابت itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(افزودن_Deletep(itemp), itemp);
                }
            }
            Ast_گره* عبارت_Or_Constraintsp = nullptr;
            اگر (nodep->exprsp()) عبارت_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            اگر (nodep->constraintsp())
                عبارت_Or_Constraintsp = Ast_گره::addNext(
                    عبارت_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(عبارت_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_رقم;
        m_curSymp = m_statep->درج_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_رقم), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->مقدار_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->درج_Sym(m_curSymp, nodep->مقدار_Arg_Refp()->name(), nodep->مقدار_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    تهی visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_رقم;
        m_curSymp = m_statep->درج_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_رقم),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    تهی visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        برای (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_رقم;
        m_curSymp = m_statep->درج_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_رقم),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        برای (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->درج_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    تهی handleUnvisitedVirtIfaces() {
        Ast_گره_Module* ثابت بالاترین = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_گره_Sym(بالاترین);
        برای (AstIface* ifacep : m_virtIfaces) {
            اگر (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ثابت ifacecellp = جدید AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                بالاترین->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

عمومی:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        اگر (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        برای (AstScope* ثابت scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        اگر (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = پیشفرض;
};

//======================================================================

رده LinkDotFindIfaceVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_گره* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ثابت VSymEnt* findNonDeclSym(VSymEnt* symp, ثابت string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        وقتی (symp) {
            ثابت VSymEnt* ثابت foundp = symp->findIdFlat(name);
            اگر (foundp && foundp->nodep() != m_declp) بازگشت foundp;
            symp = symp->fallbackp();
        }
        بازگشت nullptr;
    }

    // VISITORS
    تهی visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        اگر (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            اگر (m_statep->exists_گره_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_گره_Sym(nodep);
                ثابت VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_گره* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                اگر (foundp) foundNodep = foundp->nodep();
                اگر (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                اگر (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                اگر (AstIface* ثابت defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ثابت newp
                        = جدید AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    اگر (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ثابت ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        اگر (ifacerefp && m_statep->exists_گره_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            اگر (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* ثابت symp = m_statep->get_گره_Sym(nodep);
            m_statep->درج_Iface_Var_Sym(symp);
        }
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

عمومی:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = پیشفرض;
};

//======================================================================

رده LinkDotParamVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    Ast_گره_Module* m_modp = nullptr;  // Current module

    تهی pin_Implicit_عبارت_Recurse(Ast_گره* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        اگر (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } وگرنه اگر (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        وگرنه {
            اگر (Ast_گره* ثابت refp = nodep->op1p()) pin_Implicit_عبارت_Recurse(refp);
            اگر (Ast_گره* ثابت refp = nodep->op2p()) pin_Implicit_عبارت_Recurse(refp);
            اگر (Ast_گره* ثابت refp = nodep->op3p()) pin_Implicit_عبارت_Recurse(refp);
            اگر (Ast_گره* ثابت refp = nodep->op4p()) pin_Implicit_عبارت_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        اگر (Ast_گره* ثابت refp = nodep->nextp()) pin_Implicit_عبارت_Recurse(refp);
    }

    // VISITORS
    تهی visit(AstTypeTable*) override {}  // ParamVisitor::
    تهی visit(Ast_گره_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        اگر ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(درست);
            بازگشت;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        اگر (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_عبارت_Recurse(nodep->exprp());
        }
    }
    تهی visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        منطقی hasPartSelect = نادرست;
        ثابت string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* ثابت foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_گره_Sym(nodep), path, baddot, okSymp, درست);
        AstCell* ثابت cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        اگر (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } وگرنه {
            Ast_گره_عبارت* ثابت exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ثابت pinp = جدید AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(درست);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    تهی visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ثابت foundp = m_statep->get_گره_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* ثابت refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        اگر (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } وگرنه اگر (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } وگرنه {
            اگر (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(درست);
            VSymEnt* ثابت symp = m_statep->درج_Sym(m_statep->get_گره_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_رقم()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(نادرست);
            refp->pin_رقم(nodep->pin_رقم());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    تهی visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_عبارت_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    تهی visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_عبارت_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    تهی visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        اگر (auto* ثابت fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ثابت foundp = m_statep->get_گره_Sym(fwdp)->findIdFallback(fwdp->name());
            اگر (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), گره_Module));
            } وگرنه اگر (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } وگرنه {
                اگر (foundp) UINFO(1, "found nodep = " << foundp->nodep());
                nodep->v3error(
                    "Forward typedef used as class/package does not resolve to class/package: "
                    << nodep->prettyNameQ() << '\n'
                    << nodep->warnContextPrimary() << '\n'
                    << (foundp ? nodep->warnMore() + "... Object with matching name\n"
                                     + foundp->nodep()->warnContextSecondary()
                               : ""));
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_عبارت_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    تهی visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ثابت foundp = m_statep->get_گره_Sym(nodep)->findIdFallback(nodep->name());
        اگر (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_رقم_Set_Min(nodep->fileline()->token_رقم());
        } وگرنه اگر (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

عمومی:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = پیشفرض;
};

//======================================================================

رده LinkDotScopeVisitor final : عمومی VNVisitor {

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    ثابت AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
عمومی:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ایستا AstVarScope* getAlias(AstVarScope* ثابت vscp) {
        اگر (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ثابت aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            بازگشت aliasp;
        } وگرنه {
            بازگشت vscp;
        }
    }

خصوصی:
    // Alias an interface reference to the interface it is connected to
    تهی setIfaceAlias(AstVarScope* ثابت vscp, AstVarScope* ثابت aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    تهی setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        اگر (ap == bp) بازگشت;
        ثابت منطقی aWins = [&]() {
            ثابت AstVar* ثابت aVarp = ap->varp();
            ثابت AstVar* ثابت bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            اگر (bVarp->isPrimaryIO()) بازگشت نادرست;
            اگر (aVarp->isPrimaryIO()) بازگشت درست;
            // A forced or public variable must keep its own storage. If both are, either works.
            اگر (aVarp->isForced() != bVarp->isForced()) بازگشت aVarp->isForced();
            اگر (aVarp->isSigPublic() != bVarp->isSigPublic()) بازگشت aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            اگر (aVarp->direction() != VDirection::OUTPUT) بازگشت نادرست;
            // Is 'ap' strictly below 'bp' in the hierarchy
            برای (ثابت AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                اگر (scopep == bp->scopep()) بازگشت درست;
            }
            بازگشت نادرست;
        }();
        اگر (aWins) {
            bp->user2p(ap);
        } وگرنه {
            ap->user2p(bp);
        }
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    تهی visit(AstScope* nodep) override {  // ScopeVisitor::
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
            m_statep->درج_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstVarScope* nodep) override {  // ScopeVisitor::
        اگر (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ثابت varSymp
                = m_statep->درج_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            اگر (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ثابت dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                اگر (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ثابت string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, نادرست);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    اگر (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ثابت mpSymp = cellSymp->findIdFallback(dtypep->modportName());
                        UASSERT_OBJ(mpSymp, nodep,
                                    "No symbol for interface modport: "
                                        << nodep->prettyNameQ(dtypep->modportName()));
                        cellSymp = mpSymp;
                        UINFO(5, "       Found modport cell: se" << cvtToHex(cellSymp) << " "
                                                                 << mpSymp->nodep());
                    }
                    // Interface reference; need to put whole thing into
                    // symtable, but can't clone it now as we may have a later
                    // alias for it.
                    m_statep->درج_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    تهی visit(Ast_گره_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->درج_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->درج_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    تهی visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    تهی visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->درج_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    تهی visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        برای (Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            اگر (ثابت AstVarRef* ثابت refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } وگرنه {
                // Reaches into the scope of an instance, look it up by name
                ثابت AstVarXRef* ثابت xrefp = VN_AS(itemp, VarXRef);
                ثابت string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ثابت symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, نادرست);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            اگر (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } وگرنه {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    تهی processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ثابت refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ثابت xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ثابت string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            وقتی (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, درست);
                اگر (inl == "") بشکن;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
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
            VSymEnt* ثابت symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, نادرست);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->درج_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ثابت lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ثابت rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    تهی processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, صحیح> depthMap;
        برای (ثابت auto& pair : m_deferredAliasScopes) {
            VSymEnt* ثابت symp = pair.second;
            اگر (depthMap.find(symp) == depthMap.end()) {
                صحیح depth = 0;
                برای (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](ثابت std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ثابت std::pair<AstAliasScope*, VSymEnt*>& b) {
                             بازگشت depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        برای (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    تهی visit(Ast_گره_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    تهی visit(AstCell*) override {}  // ScopeVisitor::
    تهی visit(AstVar*) override {}  // ScopeVisitor::
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

عمومی:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = پیشفرض;
};

//======================================================================

// Iterate an interface to resolve modports
رده LinkDotIfaceVisitor final : عمومی VNVisitor {
    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    تهی visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->درج_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    تهی visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->findIdFallback(nodep->name());
        اگر (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وگرنه اگر (Ast_گره_FTask* ثابت ftaskp = VN_CAST(symp->nodep(), گره_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ثابت subSymp
                = m_statep->درج_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->درج_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } وگرنه {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        اگر (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_سطر* fl, VSymEnt* okSymp, string& baddot) {
        اگر (!okSymp || baddot.empty()) بازگشت nullptr;

        ایستا constexpr صحیح MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        برای (صحیح depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            اگر (ثابت AstCell* ثابت cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } وگرنه اگر (ثابت AstVar* ثابت varp = VN_CAST(curOkSymp->nodep(), Var)) {
                اگر (varp->isIfaceRef()) {
                    اگر (ثابت AstIfaceRefDType* ثابت ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            اگر (!ifacep || !m_statep->exists_گره_Sym(ifacep)) بازگشت nullptr;

            VSymEnt* ثابت ifaceSymp = m_statep->get_گره_Sym(ifacep);

            اگر (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ثابت symp = ifaceSymp->findIdFallback(baddot);
                اگر (symp) baddot.clear();
                بازگشت symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* ثابت symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, درست);
            اگر (symp) {
                baddot = remainingBaddot;
                بازگشت symp;
            }

            // findDotted partially matched - check progress
            اگر (remainingBaddot == baddot || !remainingOkSymp) بازگشت nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        بازگشت nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_سطر* fl, Ast_گره_عبارت* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        اگر (AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } وگرنه اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            اگر (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_گره_Sym(refp->varScopep());
            } وگرنه اگر (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } وگرنه اگر (AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            اگر (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                اگر (m_statep->exists_گره_Sym(refp->varp())) {
                    symp = m_statep->get_گره_Sym(refp->varp());
                } وگرنه {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } وگرنه اگر (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ثابت string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, درست);
                // Handle nested interface path when findDotted had partial match
                اگر (okSymp && !baddot.empty()) {
                    VSymEnt* ثابت resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    اگر (resolved) symp = resolved;
                }
            }
        } وگرنه اگر (AstDot* ثابت dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            منطقی hasPartSelect = نادرست;
            ثابت string dottedPath = extractDottedPath(dotp, hasPartSelect);
            اگر (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } وگرنه {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, درست);

                // Handle nested interface path when findDotted had partial match
                اگر (okSymp && !baddot.empty()) {
                    VSymEnt* ثابت resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    اگر (resolved) symp = resolved;
                }

                اگر (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_گره::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } وگرنه اگر (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } وگرنه {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        بازگشت symp;
    }

    تهی visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        اگر (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } وگرنه {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        اگر (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وگرنه اگر (AstVar* ثابت varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            اگر (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ثابت subSymp
                    = m_statep->درج_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->درج_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } وگرنه {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->درج_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } وگرنه اگر (AstVarScope* ثابت vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            اگر (nodep->exprp()) {
                VSymEnt* ثابت subSymp
                    = m_statep->درج_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->درج_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } وگرنه {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        اگر (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->findIdFallback(nodep->name());
        اگر (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وگرنه اگر (AstClocking* ثابت clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->درج_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } وگرنه {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    تهی visit(AstScope* nodep) override {  // IfaceVisitor::
        اگر (nodep->user3SetOnce()) بازگشت;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

عمومی:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = پیشفرض;
};

تهی LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ثابت VNUser3InUse user3InUse;
    برای (ثابت auto& itr : m_ifaceModSyms) {
        AstIface* ثابت nodep = itr.first;
        VSymEnt* ثابت symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, این};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

رده LinkDotResolveVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3InUse m_inuser3;

    // TYPES
    شمارش DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ثابت m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    ثابت AstCell* m_cellp = nullptr;  // Current cell
    Ast_گره_Module* m_modp = nullptr;  // Current module
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    صحیح m_modport_رقم = 0;  // Uniqueify modport numbers
    صحیح m_indent = 0;  // Indentation (tree depth) for debug
    منطقی m_inSens = نادرست;  // True if in senitem
    ثابت AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    منطقی m_genericIfaceModule = نادرست;  // True if in module containing generic interface
    std::map<std::string, Ast_گره*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ثابت VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    منطقی m_insideClassExtParam = نادرست;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_گره*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_گره*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_گره_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_گره* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_گره_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    منطقی m_inPackedArray = نادرست;  // Currently traversing a packed array tree
    منطقی m_replaceWithAlias
        = درست;  // Replace VarScope with an alias. Used in the handling of AstAlias
    منطقی m_isParam = نادرست;  // Specifies whether currently visiting param variable
    منطقی m_resolvingTypedef = نادرست;  // Currently traversing a Typedef tree

    ساختار DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ثابت AstDot* m_dotp;  // Current dot
        منطقی m_super;  // Starts with super reference
        منطقی m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        منطقی m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        منطقی m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        منطقی m_genBlk;  // Contains gen block reference
        Ast_گره* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        منطقی m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = پیشفرض;
        تهی init(VSymEnt* curSymp, منطقی unresolvedClass = نادرست) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = نادرست;
            m_dotErr = نادرست;
            m_dotText = "";
            m_unresolvedCell = نادرست;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = نادرست;
            m_genBlk = نادرست;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() ثابت {
            ایستا ثابت نویسه* ثابت names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            ثابت string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            اگر (m_dotErr) sstr << "  [dotErr]";
            اگر (m_super) sstr << "  [super]";
            اگر (m_unresolvedCell) sstr << "  [unrCell]";
            اگر (m_unresolvedClass) sstr << "  [unrClass]";
            اگر (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            اگر (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            بازگشت sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_گره_Module* modp,
                              VSymEnt* moduleSymp, منطقی noWarn) {
        // Create implicit after warning
        اگر (!noWarn) {
            اگر (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_گره_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            وگرنه اگر (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_گره_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* ثابت newp
            = جدید AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->درج_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        بازگشت newp;
    }
    AstVar* foundToVarp(ثابت VSymEnt* symp, Ast_گره* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        اگر (!symp) {
            بازگشت nullptr;
        } وگرنه اگر (VN_IS(symp->nodep(), Var)) {
            بازگشت VN_AS(symp->nodep(), Var);
        } وگرنه اگر (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ثابت snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ثابت varp = snodep->varp();
            اگر (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            بازگشت varp;
        } وگرنه {
            بازگشت nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, ثابت string& portName) {
        ثابت AstVarScope* ثابت dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ثابت AstVar* ثابت dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        اگر (!dotVarp) بازگشت nullptr;

        Ast_گره_DType* dtypep = dotVarp->childDTypep();
        اگر (!dtypep) dtypep = dotVarp->subDTypep();
        اگر (!dtypep) بازگشت nullptr;
        ثابت AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        اگر (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) بازگشت nullptr;

        // Get the interface cell's symbol table
        VSymEnt* ثابت ifaceCellSymp = m_statep->get_گره_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* ثابت modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        اگر (!modportSymp) بازگشت nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ثابت virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, درست);
        اگر (!virtPortp) بازگشت nullptr;

        // Symbol may point to VarScope or Var
        اگر (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) بازگشت vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        اگر (ثابت AstVar* ثابت realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ثابت realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, درست);
            بازگشت realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        بازگشت nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    تهی captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        اگر (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) بازگشت;
        Ast_گره_Module* ثابت defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        اگر (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) بازگشت;
        AstCell* ثابت cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        اگر (!cellForCapture) بازگشت;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_گره_Stmt* addImplicitSuperNewCall(AstFunc* ثابت nodep,
                                         ثابت AstClassExtends* ثابت classExtendsp) {
        // Returns the added node
        File_سطر* ثابت fl = nodep->fileline();
        AstArg* argsp = nullptr;
        اگر (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_درخت(درست);
        AstNew* ثابت newExprp = جدید AstNew{fl, argsp};
        newExprp->isImplicit(درست);
        AstDot* ثابت superNewp = جدید AstDot{fl, نادرست, جدید AstParseRef{fl, "super"}, newExprp};
        Ast_گره_Stmt* ثابت superNewStmtp = superNewp->makeStmt();
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            اگر (VN_IS(stmtp, گره_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                بازگشت superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        بازگشت superNewStmtp;
    }
    تهی checkNoDot(Ast_گره* nodep) {
        اگر (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = درست;
            بازگشت;
        }
        اگر (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            اگر (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } وگرنه {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = درست;
        }
    }
    AstVar* find_Iface_بالاترین_Varp(Ast_گره* nodep, VSymEnt* parentEntp, ثابت string& name) {
        ثابت string findName = name + "__Viftop";
        VSymEnt* ثابت ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* ثابت iface_بالاترین_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_بالاترین_Varp, nodep, "Can't find interface var ref: " << findName);
        بازگشت iface_بالاترین_Varp;
    }
    AstRSProd* findProd(Ast_گره* nodep, VSymEnt* parentEntp, ثابت string& name) {
        ثابت VSymEnt* ثابت foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        اگر (!foundNodep) {
            VSpellCheck speller;
            Link_گره_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, نادرست);
            ثابت string suggest = speller.best_Candidate_پیام(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_گره::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            بازگشت nullptr;
        }
        بازگشت foundNodep;
    }
    تهی duplicatePinError(AstPin* nodep, Ast_گره* origp, ثابت نویسه* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    تهی markAndCheckPinDup(AstPin* nodep, Ast_گره* refp, ثابت نویسه* whatp) {
        ثابت auto pair = m_usedPins.emplace(refp, nodep);
        اگر (pair.second) {
            اگر (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            بازگشت;
        } وگرنه {
            Ast_گره* ثابت origp = pair.first->second;
            اگر (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                بازگشت;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        ثابت auto defParamIt = defParamPins.find(nodep->paramPath());
        اگر (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            بازگشت;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    ایستا منطقی extendsClass(ثابت AstClass* classp, ثابت AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<ثابت AstClass*> visited;
        std::vector<ثابت AstClass*> todo{classp};
        وقتی (!todo.empty()) {
            ثابت AstClass* ثابت currp = todo.back();
            todo.برداشت_back();
            اگر (currp == targetp) بازگشت درست;
            اگر (!visited.درج(currp).second) ادامه;
            برای (ثابت AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                اگر (ثابت AstClass* ثابت basep = cextp->classOrNullp()) todo.افزودن_back(basep);
            }
        }
        بازگشت نادرست;
    }
    ایستا AstClocking* sensClockingp(Ast_گره* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        اگر (AstClocking* ثابت clockingp = VN_CAST(nodep, Clocking)) بازگشت clockingp;
        اگر (ثابت AstModportClockingRef* ثابت clockingRefp = VN_CAST(nodep, ModportClockingRef))
            بازگشت clockingRefp->clockingp();
        بازگشت nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ثابت eventp = clockingp->ensureEventp(درست);
        اگر (!eventp->user1p()) eventp->user1p(جدید VSymEnt{m_statep->symsp(), eventp});
        بازگشت reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_گره* nodep, ثابت string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ثابت string inl = Ast_گره::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, نادرست);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_گره::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        بازگشت dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_گره* nodep, VSymEnt* lookupSymp, ثابت string& dotted,
                               string& baddot, VSymEnt*& okSymp, منطقی* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        اگر (modportp) *modportp = نادرست;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, درست);  // Maybe nullptr
        اگر (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_گره::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            بازگشت nullptr;
        }
        اگر (ثابت AstVar* ثابت varp = VN_CAST(dotSymp->nodep(), Var)) {
            اگر (ثابت AstIfaceRefDType* ثابت ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                اگر (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_گره_Sym(ifaceRefp->modportp());
                    اگر (modportp) *modportp = درست;
                } وگرنه {
                    dotSymp = m_statep->get_گره_Sym(ifaceRefp->ifacep());
                }
            }
        } وگرنه اگر (ثابت AstModportClockingRef* ثابت clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_گره_Sym(clockingRefp->clockingp());
        }
        بازگشت dotSymp;
    }

    ایستا منطقی isParamedClassRefDType(ثابت Ast_گره* classp) {
        وقتی (ثابت AstRefDType* ثابت refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        بازگشت (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    منطقی isParamedClassRef(ثابت Ast_گره* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        اگر (ثابت auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            اگر (classRefp->paramsp()) بازگشت درست;
            ثابت auto* classp = classRefp->classOrPackageNodep();
            وقتی (ثابت auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            بازگشت isParamedClassRefDType(classp);
        }
        بازگشت isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        وقتی (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        بازگشت classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    منطقی insideClass(ثابت AstClass* classp) ثابت {
        برای (ثابت VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            اگر (symp->nodep() == classp) بازگشت درست;
        }
        بازگشت نادرست;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    منطقی randTargetUnresolved() ثابت {
        بازگشت m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    منطقی isVirtualMethod(ثابت Ast_گره_FTask* funcp, ثابت AstClass* classp) ثابت {
        اگر (funcp->isVirtual()) بازگشت درست;
        برای (ثابت AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ثابت basep = cextp->classOrNullp();
            اگر (!basep) ادامه;
            ثابت VSymEnt* ثابت foundp = m_statep->get_گره_Sym(basep)->findIdFlat(funcp->name());
            ثابت Ast_گره_FTask* ثابت baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), گره_FTask) : nullptr;
            ثابت AstClass* ثابت baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            اگر (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) بازگشت درست;
        }
        بازگشت نادرست;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    منطقی inheritedImplements(ثابت VSymEnt* implp, ثابت VSymEnt* ifSymp, منطقی& needForwardr) {
        needForwardr = نادرست;
        ثابت Ast_گره_FTask* ثابت implFuncp = VN_CAST(implp->nodep(), گره_FTask);
        اگر (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) بازگشت نادرست;
        ثابت AstClass* ثابت implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        ثابت AstClass* ثابت ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        اگر (!implClassp || !ifClassp) بازگشت نادرست;
        اگر (extendsClass(implClassp, ifClassp)) بازگشت درست;
        اگر (!isVirtualMethod(implFuncp, implClassp)) بازگشت نادرست;
        needForwardr = درست;
        بازگشت درست;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    تهی addInterfaceForward(AstClass* classp, Ast_گره_FTask* implp, AstClass* implClassp) {
        File_سطر* ثابت flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_گره_FTask* newp;
        اگر (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_درخت(نادرست);
            newp = جدید AstFunc{flp, implp->name(), nullptr, fvarp};
        } وگرنه {
            newp = جدید AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(درست);
        newp->isVirtual(درست);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        برای (Ast_گره* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            اگر (!portp || !portp->isIO()) ادامه;
            AstVar* ثابت newPortp = portp->clone_درخت(نادرست);
            newp->addStmtsp(newPortp);
            argsp = Ast_گره::addNext(
                argsp, جدید AstArg{flp, "", جدید AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_گره_FTask_Ref* callp;
        اگر (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            callp = جدید AstFuncRef{flp, implFuncp, argsp};
        } وگرنه {
            callp = جدید AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(درست);
        اگر (fvarp) {
            newp->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } وگرنه {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    تهی importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        برای (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            اگر (Ast_گره* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                ثابت string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                اگر (VN_IS(baseSubp, گره_FTask)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ثابت Ast_گره_FTask* ثابت baseFuncp = VN_CAST(baseSubp, گره_FTask);
                    اگر (!baseFuncp || !baseFuncp->pureVirtual()) ادامه;
                    منطقی needForward = نادرست;
                    ثابت منطقی existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    اگر (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    اگر (m_statep->forPrimary() && !existsInDerived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << baseClassp->prettyNameQ()
                                     << " but is missing implementation for "
                                     << baseSubp->prettyNameQ() << " (IEEE 1800-2023 8.26)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << baseSubp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << baseSubp->warnContextSecondary());
                    }
                    ثابت auto itn = m_ifClassImpNames.find(baseSubp->name());
                    اگر (m_statep->forPrimary() && !existsInDerived
                        && itn != m_ifClassImpNames.end()
                        && itn->second != baseSubp) {  // Not exact same function from diamond
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << baseClassp->prettyNameQ()
                                     << " but missing inheritance conflict resolution for "
                                     << baseSubp->prettyNameQ() << " (IEEE 1800-2023 8.26.6.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << baseSubp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << baseSubp->warnContextSecondary());
                    }
                    m_ifClassImpNames.emplace(baseSubp->name(), baseSubp);
                }
                اگر (VN_IS(baseSubp, Constraint)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ثابت AstConstraint* ثابت baseFuncp = VN_CAST(baseSubp, Constraint);
                    اگر (!baseFuncp || !baseFuncp->isKwdPure()) ادامه;
                    ثابت منطقی existsInDerived = foundp && !foundp->imported();
                    اگر (m_statep->forPrimary() && !existsInDerived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << baseClassp->prettyNameQ()
                                     << " but is missing constraint implementation for "
                                     << baseSubp->prettyNameQ() << " (IEEE 1800-2023 18.5.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << baseSubp->warnOther()
                                     << "... Location of interface class's pure constraint\n"
                                     << baseSubp->warnContextSecondary());
                    }
                }
            }
        }
    }
    تهی importSymbolsFromExtended(AstClass* ثابت nodep, AstClassExtends* ثابت cextp) {
        AstClass* ثابت baseClassp = cextp->classp();
        VSymEnt* ثابت srcp = m_statep->get_گره_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        اگر (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    تهی classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_گره_Sym(nodep));
    }

    تهی checkDeclOrder(Ast_گره* nodep, Ast_گره* declp) {
        ثابت uint32_t decl_Token_رقم = declp->decl_Token_رقم();
        اگر (!decl_Token_رقم) بازگشت;  // Not implemented/valid on this object
        اگر (nodep->fileline()->token_رقم() < decl_Token_رقم) {
            UINFO(1, "Related node " << nodep->fileline()->token_رقم() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_رقم << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference, or use a "
                              "forward typedef\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }
    تهی checkMemberDeclOrder(Ast_گره* nodep, AstMemberDType* declp) {
        ثابت uint32_t decl_Token_رقم = declp->fileline()->token_رقم();
        اگر (nodep->fileline()->token_رقم() < decl_Token_رقم) {
            UINFO(1, "Related node " << nodep->fileline()->token_رقم() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_رقم << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    تهی replaceWithCheckBreak(Ast_گره* oldp, Ast_گره_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        اگر (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(جدید AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } وگرنه {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    تهی revisitLater(Ast_گره* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.درج(std::make_pair(m_modp->name(), m_modp));
    }

    تهی updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        اگر (nodep && nodep->isParam()) nodep->usedParam(درست);
    }

    ایستا VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, ثابت string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        وقتی (lookp) {
            VSymEnt* ثابت foundp = lookp->findIdFlat(name);
            اگر (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                اگر (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    بازگشت foundp;
                }
                اگر (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        بازگشت shadowEntp;
    }

    تهی symIterateChildren(Ast_گره* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    تهی symIterateNull(Ast_گره* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    ایستا ثابت Ast_گره_DType* get_عبارت_DTypep(ثابت Ast_گره_عبارت* selp) {
        وقتی (ثابت Ast_گره_Pre_Sel* ثابت sp = VN_CAST(selp, گره_Pre_Sel)) selp = sp->fromp();
        اگر (ثابت AstMemberSel* ثابت sp = VN_CAST(selp, MemberSel)) {
            اگر (ثابت Ast_گره_DType* dtypep = get_عبارت_DTypep(sp->fromp())) {
                اگر (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ثابت AstClass* ثابت classp = classRefp->classp();
                    ثابت منطقی found = classp->existsMember(
                        [&dtypep, name = selp->name()](ثابت AstClass*, ثابت AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            بازگشت nodep->name() == name;
                        });
                    اگر (found) بازگشت dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } وگرنه {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } وگرنه اگر (ثابت Ast_گره_Var_Ref* ثابت varRefp = VN_CAST(selp, گره_Var_Ref)) {
            بازگشت varRefp->varp()->childDTypep()->elemDTypep();
        }
        بازگشت nullptr;
    }
    ایستا ثابت AstVar* getNextVarp(ثابت Ast_گره* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        وقتی (stmtsp) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(stmtsp, Var)) {
                اگر (varp->isIO()) بازگشت varp;
            }
            stmtsp = stmtsp->nextp();
        }
        بازگشت nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    تهی addImplicitParametersOfGenericIface(AstCell* ثابت nodep, ثابت AstModule* ثابت modp) {
        // Get Param number
        صحیح param_رقم = 1;
        برای (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_رقم;
        }

        // Add each implicit parameter type
        ثابت AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        برای (ثابت AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            اگر (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                ادامه;
            }
            Ast_گره* exprp = pinp->exprp();
            اگر (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                ادامه;
            }
            وقتی (ثابت Ast_گره_Pre_Sel* ثابت preSelp = VN_CAST(exprp, گره_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            اگر (varRefp) {
                enclosingVarp = varRefp->varp();
            } وگرنه اگر (ثابت AstParseRef* ثابت parseRefp = VN_CAST(exprp, ParseRef)) {
                اگر (m_modp) {
                    اگر (VSymEnt* ثابت symp
                        = m_statep->get_گره_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            اگر (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ثابت fwdRefp
                    = جدید AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ثابت newPinp = جدید AstPin{
                    pinp->fileline(), param_رقم, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(درست);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } وگرنه اگر (varRefp) {
                ثابت AstVar* ثابت varp = varRefp->varp();
                اگر (ثابت AstIfaceRefDType* ثابت refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ثابت ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    اگر (refp->modportp()) {
                        newIfaceRefp = جدید AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } وگرنه {
                        newIfaceRefp = جدید AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    اگر (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_درخت(درست));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ثابت newPinp
                        = جدید AstPin{pinp->fileline(), param_رقم,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(درست);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } وگرنه {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } وگرنه {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_رقم;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    منطقی errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        اگر (varp->isIO()) بازگشت نادرست;
        // OK if not an instance
        ثابت AstCell* ثابت cellp = VN_CAST(scopeSymp->nodep(), Cell);
        اگر (!cellp) بازگشت نادرست;
        // Ok if not a hier_block
        ثابت Ast_گره_Module* ثابت modp = cellp->modp();
        اگر (!modp->hierBlock()) بازگشت نادرست;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        بازگشت درست;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ثابت {
        string result = "";
        result.درج(0, m_indent, ':');
        بازگشت result + " ";
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    تهی visit(AstTypeTable*) override {}
    تهی visit(Ast_گره_Module* nodep) override {
        اگر (nodep->dead()) بازگشت;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        اگر (ثابت AstModule* ثابت modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_گره_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_رقم = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = نادرست;
    }
    تهی visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    تهی visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstCell* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->modp(), nodep,
                    "Instance has unlinked module");  // V3LinkCell should have errored out
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_pinSymp);
        {
            m_cellp = nodep;
            اگر (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                اگر (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_درخت();
                اگر (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_درخت();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            وگرنه {
                m_pinSymp = m_statep->get_گره_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                اگر (m_statep->forPrimary())
                    اگر (ثابت AstModule* ثابت modp = VN_CAST(nodep->modp(), Module)) {
                        اگر (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    تهی visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_شمار(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_گره_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    تهی visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        اگر (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ثابت foundp = m_pinSymp->findIdFlat(nodep->name());
            ثابت نویسه* ثابت whatp = nodep->param() ? "parameter" : "pin";
            اگر (!foundp) {
                اگر (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                    بازگشت;
                } وگرنه اگر (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                    بازگشت;
                } وگرنه {
                    ثابت std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_گره_Matcher_Var_Param{}, نادرست)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_گره_Matcher_Var_IO{}, نادرست));
                    ثابت std::string decl
                        = ((m_cellp && m_cellp->modp())
                               ? "\n"s + nodep->warnMore() + "... Location of instance's "
                                     + m_cellp->modp()->verilogKwd() + " declaration\n"
                                     + m_cellp->modp()->warnContextSecondary()
                               : "");
                    nodep->v3warn(PINNOTFOUND,
                                  ucfirst(whatp)
                                      << " not found: " << nodep->prettyNameQ() << '\n'
                                      << (suggest.empty() ? "" : nodep->warnMore() + suggest)
                                      << '\n'
                                      << nodep->warnContextPrimary() << decl);
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                    بازگشت;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            منطقی wrongPinType = نادرست;
            اگر (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                اگر (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } وگرنه {
                    wrongPinType = درست;
                }
            } وگرنه اگر (AstParamTypeDType* ثابت typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } وگرنه {
                wrongPinType = درست;
            }
            // Don't connect parameter pin to module ports or vice versa
            اگر (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = درست;
            اگر (wrongPinType) {
                string targetType = LinkDotState::گره_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                اگر (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } وگرنه {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                بازگشت;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ایستا منطقی isDotThisReference(ثابت AstDot* nodep) {
        بازگشت VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    تهی visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ثابت DotStates lastStates = m_ds;
        ثابت منطقی start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ثابت DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            اگر (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            اگر (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                اگر (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                اگر (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } وگرنه اگر (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = درست;
                } وگرنه {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } وگرنه اگر (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ثابت VSymEnt* classSymp = getThisClassSymp();
                اگر (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = درست;
                } وگرنه {
                    ثابت auto classp = VN_AS(classSymp->nodep(), Class);
                    ثابت auto cextp = classp->extendsp();
                    اگر (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = درست;
                    } وگرنه {
                        اگر (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = درست;
                        } وگرنه {
                            ثابت auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_گره_Sym(baseClassp);
                            m_ds.m_super = درست;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } وگرنه اگر (AstClassOrPackageRef* ثابت lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                اگر (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    اگر (Ast_گره_FTask_Ref* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), گره_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    بازگشت;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                اگر (AstClassOrPackageRef* ثابت classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    اگر (Ast_گره* ثابت classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_گره_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } وگرنه اگر (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                اگر (AstClassOrPackageRef* ثابت crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    اگر (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        اگر (Ast_گره_FTask_Ref* ثابت ftaskrefp
                            = VN_CAST(nodep->rhsp(), گره_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        بازگشت;
                    }
                }
                اگر (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    اگر (Ast_گره_FTask_Ref* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), گره_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    بازگشت;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } وگرنه {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = نادرست;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                اگر (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    بازگشت;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            اگر (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = درست;
            }
            اگر (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            اگر (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            اگر (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                اگر (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            اگر (!m_ds.m_unresolvedClass) {
                اگر (start) {
                    Ast_گره* newp;
                    اگر (m_ds.m_dotErr) {
                        newp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } وگرنه {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                } وگرنه {  // Dot midpoint
                    Ast_گره* newp = nodep->rhsp()->unlinkFrBack();
                    اگر (m_ds.m_unresolvedCell) {
                        AstCellRef* ثابت crp = جدید AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                }
            }
        }
        اگر (start) {
            m_ds = lastStates;
        } وگرنه {
            ثابت منطقی unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    تهی visit(AstDefaultClocking* nodep) override {
        اگر (VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name())) {
            اگر (AstClocking* ثابت clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            } وگرنه {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } وگرنه {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    تهی visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        اگر (nodep->varrefp()) بازگشت;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        اگر (AstText* ثابت textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            اگر (VSymEnt* ثابت foundp = m_curSymp->findIdFallback(textp->text())) {
                اگر (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                    اگر (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            جدید AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                اگر (AstEnumItem* ثابت itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        جدید AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstParseRef* nodep) override {
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        اگر (m_ds.m_unresolvedClass) بازگشت;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ثابت DotStates lastStates = m_ds;
        ثابت منطقی start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        منطقی first = start || m_ds.m_dotPos == DP_FIRST;

        اگر (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        اگر (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = درست;
        }
        اگر (nodep->name() == "this") {
            iterateChildren(nodep);
            اگر (m_statep->forPrimary()) بازگشت;  // The class might be parameterized somewhere
            ثابت VSymEnt* classSymp = getThisClassSymp();
            اگر (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                بازگشت;
            }
            AstClass* ثابت classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ثابت dtypep
                = جدید AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* ثابت newp = جدید AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        اگر (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ثابت newp = جدید AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", درست};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        } وگرنه اگر (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            اگر (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } وگرنه {
                Ast_گره_عبارت* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), گره_عبارت);
                Ast_گره_عبارت* ثابت newp = جدید AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {
            //
            string expectWhat;
            منطقی allowScope = نادرست;
            منطقی allowVar = نادرست;
            منطقی allowFTask = نادرست;
            منطقی staticAccess = نادرست;
            اگر (m_ds.m_disablep) {
                allowScope = درست;
                allowFTask = درست;
                expectWhat = "block/task";
            } وگرنه اگر (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_گره_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = درست;
                allowVar = درست;
                allowFTask = درست;
                staticAccess = درست;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ثابت cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                اگر (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = درست;
                } وگرنه اگر (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, درست, نادرست, ":: reference");
                    اگر (!foundp) بازگشت;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    اگر (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_گره_Sym(classOrPackagep);
                    } وگرنه {
                        m_ds = lastStates;
                        بازگشت;
                    }
                } وگرنه {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_گره_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } وگرنه اگر (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = درست;
                allowVar = درست;
            } وگرنه اگر (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = درست;
            } وگرنه اگر (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = درست;
                allowVar = درست;
            } وگرنه {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            اگر (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                اگر (foundp) {
                    اگر (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.درج(nodep->name());
                    }
                    اگر (!start) m_ds.m_dotPos = DP_MEMBER;
                    اگر (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ثابت newp = جدید AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_درخت(نادرست),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                        بازگشت;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ثابت lambdaRefp
                        = جدید AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", نادرست};
                    AstMemberSel* newp = جدید AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
            اگر (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } وگرنه اگر (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } وگرنه {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            اگر (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ثابت AstClass* ثابت classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                اگر (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ثابت enclosingClassSymp
                        = m_statep->get_گره_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    اگر (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            اگر (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ثابت ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ثابت ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                اگر (ifaceFoundp) {
                    اگر (ثابت AstVar* ثابت varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        اگر (varp->isParam()) foundp = ifaceFoundp;
                    } وگرنه اگر (VN_IS(ifaceFoundp->nodep(), Typedef)
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
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            منطقی ok = نادرست;
            // Special case: waiting on clocking event
            اگر (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                اگر (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            اگر (!foundp) {
            } وگرنه اگر (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), گره_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                اگر (allowScope) {
                    ok = درست;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    اگر (m_ds.m_disablep && VN_IS(foundp->nodep(), گره_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    اگر (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = درست;
                        اگر (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } وگرنه اگر (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ثابت cellp = VN_AS(foundp->nodep(), Cell);
                    اگر (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ثابت cellEntp = m_statep->get_گره_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ثابت parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ثابت ifaceRefVarp
                            = find_Iface_بالاترین_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = درست;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_گره_عبارت* newp;
                        اگر (m_ds.m_dotText != "") {
                            newp = جدید AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            اگر (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = جدید AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = نادرست;
                                m_ds.m_dotText = "";
                            }
                        } وگرنه {
                            AstVarRef* ثابت refp
                                = جدید AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    } وگرنه اگر (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } وگرنه اگر (allowScope && !allowFTask && VN_IS(foundp->nodep(), گره_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = درست;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } وگرنه اگر (allowFTask && VN_IS(foundp->nodep(), گره_FTask)) {
                Ast_گره_FTask_Ref* taskrefp;
                اگر (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = جدید AstTaskRef{nodep->fileline(), nodep->name()};
                } وگرنه {
                    taskrefp = جدید AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                m_ds = lastStates;
                بازگشت;
            } وگرنه اگر (AstVar* ثابت varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                اگر (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = درست;
                    بازگشت;
                }
                AstIfaceRefDType* ثابت ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                اگر (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = درست;
                    m_ds.m_unresolvedGenericIface = درست;
                } وگرنه اگر (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    اگر (ifacerefp->modportp() && m_statep->exists_گره_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_گره_Sym(ifacerefp->modportp());
                    } وگرنه {
                        m_ds.m_dotSymp = m_statep->get_گره_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = درست;

                    AstVarRef* ثابت refp = جدید AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                } وگرنه اگر (allowVar) {
                    Ast_گره* newp;
                    اگر (m_ds.m_dotText != "") {
                        AstVarXRef* ثابت refp
                            = جدید AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        اگر (ثابت AstModportVarRef* ثابت mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            اگر (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(درست);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        اگر (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ثابت string dotted = refp->dotted();
                            ثابت size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            اگر (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } وگرنه {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = جدید AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = نادرست;
                            }
                        } وگرنه {
                            newp = refp;
                        }
                    } وگرنه {
                        اگر (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ثابت refp = جدید AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = درست;
                }
            } وگرنه اگر (ثابت AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                اگر (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } وگرنه اگر (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } وگرنه {
                    AstCell* ثابت cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ثابت cellEntp = m_statep->get_گره_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ثابت parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ثابت ifaceRefVarp
                        = find_Iface_بالاترین_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = درست;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ثابت refp
                        = جدید AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_گره_عبارت* newp = refp;

                    auto* ثابت cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    اگر (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = جدید AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(درست);  // Don't process again
                        VL_DO_DANGLING(افزودن_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                }
            } وگرنه اگر (AstConstraint* ثابت defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_گره* ثابت newp = جدید AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                ok = درست;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } وگرنه اگر (AstClass* ثابت defp = VN_CAST(foundp->nodep(), Class)) {
                اگر (allowVar) {
                    اگر (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } وگرنه {
                        AstRefDType* ثابت newp
                            = جدید AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    }
                    ok = درست;
                    m_ds.m_dotText = "";
                } وگرنه {
                    (تهی)defp;  // Prevent unused variable warning
                }
            } وگرنه اگر (AstMemberDType* ثابت defp = VN_CAST(foundp->nodep(), MemberDType)) {
                اگر (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* ثابت refp = جدید AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    ok = درست;
                    m_ds.m_dotText = "";
                }
            } وگرنه اگر (AstEnumItem* ثابت valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                اگر (allowVar) {
                    AstEnumItemRef* ثابت refp
                        = جدید AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_گره* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    اگر (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = جدید AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = نادرست;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    ok = درست;
                    m_ds.m_dotText = "";
                }
            } وگرنه اگر (ثابت AstLambdaArgRef* ثابت argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                اگر (allowVar) {
                    Ast_گره* ثابت newp
                        = جدید AstLambdaArgRef{nodep->fileline(), argrefp->name(), نادرست};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    ok = درست;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } وگرنه اگر (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                اگر (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } وگرنه اگر (ثابت AstModportClockingRef* ثابت clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_گره_Sym(clockingRefp->clockingp());
                اگر (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } وگرنه اگر (ثابت Ast_گره_FTask* ثابت ftaskp = VN_CAST(foundp->nodep(), گره_FTask)) {
                اگر (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    اگر (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ثابت funcRefp
                            = جدید AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    }
                }
            } وگرنه اگر (AstTypedef* ثابت defp = VN_CAST(foundp->nodep(), Typedef)) {
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
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                اگر (ok) {
                    AstRefDType* ثابت refp = جدید AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    اگر (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<صحیح>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [این]() { بازگشت indent(); });

                    اگر (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } وگرنه {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    }
                }
            } وگرنه اگر (AstParamTypeDType* ثابت defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                اگر (ok) {
                    AstRefDType* ثابت refp = جدید AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<صحیح>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [این]() { بازگشت indent(); });

                    اگر (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } وگرنه {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    }
                }
            }
            اگر (!ok) {
                اگر (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = درست;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } وگرنه {
                    // Cells/interfaces can't be implicit
                    ثابت منطقی checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    ثابت منطقی err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    اگر (err) {
                        اگر (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } وگرنه اگر (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ثابت string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } وگرنه {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_گره::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_گره::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = درست;
                    }
                    اگر (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ثابت varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ثابت newp
                            = جدید AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    }
                }
            }
        }
        اگر (start) m_ds = lastStates;
    }
    تهی visit(AstClassOrPackageRef* nodep) override {
        // Class: Recurse inside or cleanup not founds
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp(), nodep,
                    "class reference parameter not removed by V3Param");
        {
            VL_RESTORER_COPY(m_ds);
            VL_RESTORER(m_pinSymp);

            اگر (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ثابت منطقی deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                نادرست, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            اگر (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_گره_Sym(nodep->classOrPackageSkipp());
            } وگرنه اگر (nodep->name() != "local::") {
                بازگشت;
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
                        << nodep->warnMore() << "... Suggest use '"
                        << nodep->classOrPackageNodep()->prettyName() << "#()'");
                // As other simulators do, use the default specialization. Add the same
                // placeholder pin V3LinkCells makes for '#()' so it is linked below.
                AstPin* ثابت pinp = جدید AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(درست);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        اگر (nodep->name() == "local::") {
            اگر (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = درست;
                بازگشت;
            }
        }
        اگر (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_گره_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    تهی visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (nodep->isExternDef()) {
            اگر (ثابت VSymEnt* ثابت foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ثابت AstConstraint* ثابت protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } وگرنه {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        اگر (nodep->isExternProto()) {
            اگر (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_گره_Sym(nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstConstraintRef* nodep) override {
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    تهی visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        اگر (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name());
            اگر (AstVar* ثابت varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
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
    تهی visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        اگر (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } وگرنه {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            اگر (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            منطقی modport;
            VSymEnt* ثابت dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            اگر (!dotSymp) بازگشت;

            اگر (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                اگر (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    وقتی (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        اگر (foundp) بشکن;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        اگر (ثابت AstVar* ثابت varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            اگر (!varp->isParam()) foundp = nullptr;
                        }
                        اگر (prefix.empty()) بشکن;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        اگر (prefix == nextPrefix) بشکن;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } وگرنه {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, درست);
                }
                اگر (m_inSens && foundp) {
                    اگر (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ثابت varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                اگر (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) بازگشت;
                اگر (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_گره::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    بازگشت;
                }
            } وگرنه {
                VSymEnt* ثابت foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, درست);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                اگر (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                اگر (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) بازگشت;
                اگر (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_گره::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } وگرنه {
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
                        = جدید AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    تهی visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* ثابت refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        اگر (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_گره_Sym(nodep));
    }
    تهی visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        اگر (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* ثابت dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            اگر (!dotSymp) بازگشت;
            VSymEnt* ثابت foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, درست);
            AstEnumItem* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            اگر (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_گره::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                بازگشت;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        اگر (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name());
            اگر (AstEnumItem* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
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
    تهی visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            اگر (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                ثابت Ast_گره_DType* fromDtp = nodep->fromp()->dtypep();
                اگر (!fromDtp) {
                    اگر (ثابت Ast_گره_Var_Ref* ثابت varRefp = VN_CAST(nodep->fromp(), گره_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } وگرنه {
                        fromDtp = get_عبارت_DTypep(nodep->fromp());
                    }
                    اگر (!fromDtp) {
                        اگر (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } وگرنه {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                اگر (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = درست;
                } وگرنه اگر (fromDtp) {
                    ثابت AstClassRefDType* ثابت classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    اگر (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    وگرنه اگر (m_statep->forPrimary() && m_extendsParam.شمار(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = درست;
                    } وگرنه {
                        m_randSymp = m_statep->get_گره_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    تهی visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        اگر (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    تهی visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        اگر (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* ثابت assignp = جدید AstAssignW{
                nodep->fileline(), جدید AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جدید AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(درست);
            nodep->scopep()->addBlocksp(جدید AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        اگر (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        اگر (m_ds.m_super) {
            اگر (AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(درست);
            } وگرنه اگر (AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(درست);
            }
        }

        VL_RESTORER(m_randSymp);

        منطقی first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        منطقی staticAccess = نادرست;
        اگر (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            بازگشت;
        } وگرنه اگر (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_گره_FTask_Ref* ثابت newftaskp = nodep->clone_درخت(نادرست);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_گره* ثابت newp = جدید AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = نادرست;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        } وگرنه اگر (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = درست;
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            اگر (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = درست;
            } وگرنه اگر (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, درست, نادرست, ":: reference");
                اگر (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } وگرنه {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } وگرنه اگر (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            اگر (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ثابت Ast_گره* ثابت blkp = m_ds.m_dotSymp->nodep();
                اگر (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(درست);
                }
            }
        } وگرنه اگر (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_گره_عبارت* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), گره_عبارت);
            AstArg* ثابت argsp = nodep->argsp();
            اگر (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* ثابت newp = جدید AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            اگر (AstWith* ثابت withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        } وگرنه اگر (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } وگرنه اگر (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        اگر (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } وگرنه اگر (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } وگرنه اگر (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } وگرنه {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            اگر (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } وگرنه {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            اگر (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_گره_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } وگرنه {
                اگر (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, درست);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            اگر (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* ثابت foundp = m_randSymp->findIdFlat(nodep->name());
                اگر (foundp && m_currentWithp) {
                    اگر (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.درج(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    اگر (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    اگر (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_گره* ثابت newp = جدید AstMethodCall{
                        nodep->fileline(),
                        جدید AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", نادرست},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
            اگر (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ثابت randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), درست, درست);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->درج_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                بازگشت;
            }
            VSymEnt* ثابت foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_گره_FTask* ثابت taskp = foundp ? VN_CAST(foundp->nodep(), گره_FTask) : nullptr;
            Ast_گره_Module* ثابت taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            اگر (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                اگر (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    بازگشت;
                }
                اگر (AstClass* ثابت targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    اگر (m_extendsParam.شمار(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        بازگشت;
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
            } وگرنه {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                اگر (foundp) {
                    اگر (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } وگرنه {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } وگرنه اگر (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } وگرنه اگر ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ثابت classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ثابت funcp
                        = جدید AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(درست);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->درج_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    بازگشت;
                } وگرنه اگر (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    اگر (AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        اگر (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } وگرنه اگر (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } وگرنه {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(جدید AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                        بازگشت;
                    }
                } وگرنه اگر (nodep->dotted() == "") {
                    اگر (nodep->pli()) {
                        اگر (v3Global.opt.bboxSys()) {
                            Ast_گره* newp;
                            اگر (VN_IS(nodep, FuncRef)) {
                                newp = جدید AstConst{nodep->fileline(), AstConst::All0{}};
                            } وگرنه {
                                Ast_گره_عبارت* outp = nullptr;
                                وقتی (AstArg* ثابت argp = nodep->argsp()) {
                                    outp = Ast_گره::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(افزودن_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = جدید AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                            بازگشت;
                        } وگرنه {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ثابت string suggest = speller.best_Candidate_پیام(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } وگرنه {
                        ثابت string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_گره_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } وگرنه {
                    ثابت string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_گره_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_گره::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    تهی visit(AstSelBit* nodep) override {
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        اگر (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            بازگشت;
        }
        اگر (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = درست;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        اگر (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_گره_عبارت* ثابت exprp = nodep->bitp()->unlinkFrBack();
            اگر (AstCellArrayRef* ثابت fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* ثابت movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه {
                AstCellArrayRef* ثابت newp
                    = جدید AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }
    تهی visit(Ast_گره_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        اگر (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = درست;
            بازگشت;
        }
        Ast_گره_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            اگر (m_packedArrayDtp) {
                AstRange* ثابت newRangep
                    = جدید AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = جدید AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                اگر (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } وگرنه {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                }
            } وگرنه {
                اگر (nodep->attrp()) {
                    Ast_گره* ثابت attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_درخت(), attrp);
                }
                Ast_گره* ثابت basefromp = nodep->baseFromp(نادرست);
                اگر (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    اگر (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } وگرنه {
                    nodep->attrp(جدید AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_درخت(نادرست)});
                }
            }
        }

        اگر (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    تهی visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    تهی visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            اگر (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_گره_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    تهی visit(Ast_گره_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            اگر (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_گره_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(درست);
            اگر (ثابت VSymEnt* ثابت foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ثابت Ast_گره_FTask* ثابت protop = VN_AS(foundp->nodep(), گره_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(جدید AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_درخت(نادرست)
                                                   : جدید AstVoidDType{protop->fileline()}});
                برای (Ast_گره* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* ثابت attrp
                            = جدید AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_درخت(نادرست)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } وگرنه {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        اگر (nodep->isExternProto()) {
            اگر (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_گره_Sym(nodep);
            ثابت منطقی isNew = nodep->name() == "new";
            اگر (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            اگر (isNew) {
                ثابت AstClassExtends* ثابت classExtendsp = VN_AS(m_modp, Class)->extendsp();
                اگر (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
                    && classExtendsp->argsp()) {
                    m_explicitSuperNewp->v3error(
                        "Explicit super.new not allowed with class "
                        "extends arguments (IEEE 1800-2023 8.17)\n"
                        << m_explicitSuperNewp->warnMore() << "... Suggest remove super.new\n"
                        << m_explicitSuperNewp->warnContextPrimary() << '\n'
                        << classExtendsp->argsp()->warnOther()
                        << "... Location of extends argument(s)\n"
                        << classExtendsp->argsp()->warnContextSecondary());
                }
                اگر (classExtendsp && classExtendsp->classOrNullp()) {
                    اگر (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_گره_Stmt* ثابت superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    تهی visit(Ast_گره_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_گره_Sym(nodep));
    }
    تهی visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_گره_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_گره_Sym(nodep));
    }
    تهی visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_گره_Sym(nodep));
    }
    تهی visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    تهی visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_گره_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    تهی visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_گره_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    تهی visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        اگر (m_ds.m_unresolvedClass) بازگشت;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_گره_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ثابت منطقی firstVisit = !nodep->validated();
            اگر (firstVisit) {
                nodep->validated(درست);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                اگر (nodep->restricted() && m_randSymp) {
                    برای (ثابت std::string& n : nodep->restrictedNames()) {
                        اگر (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.درج(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            اگر (firstVisit && nodep->restricted()) {
                برای (ثابت std::string& n : nodep->restrictedNames()) {
                    اگر (!m_restrictedNamesUsed.شمار(n)) {
                        nodep->v3warn(UNUSED, "Identifier '"
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
    تهی visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    تهی visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        اگر (nodep->user3SetOnce()) بازگشت;
        // If the class is resolved, there is nothing more to do
        اگر (nodep->classOrNullp()) بازگشت;
        LINKDOT_VISIT_START();
        اگر (m_statep->forPrimary()) {
            اگر (nodep->childDTypep()) بازگشت;
            Ast_گره* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            اگر (AstDot* ثابت dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(درست);
                اگر (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, درست, نادرست, nodep->verilogKwd());
                    اگر (!foundp) {
                        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                        بازگشت;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_گره_Sym(lookNodep->classOrPackageSkipp());
                } وگرنه {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ثابت cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            اگر (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
            VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, درست,
                                                                    درست, nodep->verilogKwd());
            اگر (!foundp) {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
            اگر (AstClass* ثابت classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                اگر (paramsp) {
                    paramsp = paramsp->clone_درخت(درست);
                    nodep->parameterized(درست);
                }
                nodep->childDTypep(جدید AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } وگرنه اگر (AstParamTypeDType* ثابت paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ثابت refParamp = جدید AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(درست);
            } وگرنه اگر (AstTypedef* ثابت typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_گره_DType* ثابت unwrappedp = typedefp->subDTypep()->skipRefp();
                اگر (AstClassRefDType* ثابت classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    اگر (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_درخت(درست);
                        nodep->parameterized(درست);
                    } وگرنه اگر (paramsp) {
                        paramsp = paramsp->clone_درخت(درست);
                        nodep->parameterized(درست);
                    }
                    nodep->childDTypep(
                        جدید AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } وگرنه {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    بازگشت;
                }
            } وگرنه {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                بازگشت;
            }
            اگر (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_درخت();
        } وگرنه {
            // Probably a parameter
            اگر (AstRefDType* ثابت refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                اگر (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_درخت(نادرست));
                    VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                } وگرنه {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ثابت Ast_گره_DType* typep = refp->refDTypep();
                    وقتی (درست) {
                        اگر (ثابت AstParamTypeDType* ثابت atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            ادامه;
                        }
                        اگر (ثابت AstRequireDType* ثابت atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            ادامه;
                        }
                        بشکن;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    تهی visit(AstClass* nodep) override {
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_modp);
        VL_RESTORER_COPY(m_ifClassImpNames);
        VL_RESTORER_COPY(m_ifForwards);
        VL_RESTORER(m_insideClassExtParam);
        {
            m_ifForwards.clear();
            m_ds.init(m_curSymp);
            m_insideClassExtParam = nodep->isCovergroup() && nodep->covergroupEnclosingClassp()
                                    && m_extendsParam.find(nodep->covergroupEnclosingClassp())
                                           != m_extendsParam.end();
            // Until overridden by a SCOPE
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_گره_Sym(nodep);
            m_modp = nodep;
            صحیح next = 0;
            برای (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                اگر (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                اگر (m_statep->forPrimary()) {
                    اگر (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.درج(nodep);
                        m_insideClassExtParam = درست;
                    }
                    اگر (AstClassRefDType* ثابت classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* ثابت classp = classRefp->classp();
                        اگر (classp != nodep) iterate(classp);
                        اگر (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.درج(nodep);
                            m_insideClassExtParam = درست;
                        }
                    }
                }

                اگر (AstClass* ثابت baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    اگر (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } وگرنه اگر (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } وگرنه اگر (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } وگرنه اگر (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(درست);
                    nodep->isExtended(درست);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    ادامه;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            برای (ثابت auto& itr : m_ifForwards) {
                ثابت VSymEnt* ثابت implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), گره_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            برای (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_گره* ثابت itemp = it->second->nodep();
                اگر (!memberMap.findMember(nodep, it->first)) {
                    اگر (AstEnumItem* ثابت aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* ثابت newp = جدید AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.درج(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    ایستا تهی collectScopeRefs(Ast_گره* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        اگر (AstClassOrPackageRef* ثابت refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.افزودن_back(refp);
            بازگشت;
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
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        برای (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* ثابت refp = refps[i];
            اگر (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, نادرست,
                                                    "class/package reference")) {
                بازگشت نادرست;  // Error already reported
            }
            اگر (i + 1 == refps.size()) بشکن;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_گره_Module* ثابت modp = refp->classOrPackageSkipp();
            اگر (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                بازگشت نادرست;
            }
            scopeSymp = m_statep->get_گره_Sym(modp);
        }
        AstClassOrPackageRef* ثابت innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        بازگشت درست;
    }

    تهی visit(AstRefDType* nodep) override {

        اگر (auto* ثابت typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            اگر (Ast_گره_DType* ثابت resolvedDTypep = VN_CAST(nodep->typeofp(), گره_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
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
            بازگشت;
        }

        // Resolve its reference
        اگر (m_statep->forParamed() && nodep->user3()) {
            اگر (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(نادرست);
        }
        اگر (nodep->user3SetOnce()) {
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
            بازگشت;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        اگر (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            اگر (AstDot* ثابت scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                اگر (!reduceScopeDot(nodep, scopeDotp)) بازگشت;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_گره* ثابت cpackagep = nodep->classOrPackageOpp();
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
                    بازگشت;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                اگر (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    بازگشت;
                }

                ثابت منطقی doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                اگر (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, درست, نادرست, "class/package reference");
                    اگر (!foundp) بازگشت;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                اگر (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    اگر (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        بازگشت;
                    }
                    // Likely impossible, as error thrown earlier
                    cpackagerefp->v3error(  // LCOV_EXCL_LINE
                        "'::' expected to reference a class/package but referenced '"
                        << (nodep->classOrPackagep() ? nodep->classOrPackagep()->prettyTypeName()
                                                     : "<unresolved-object>")
                        << "'\n"
                        << cpackagerefp->warnMore() + "... Suggest '.' instead of '::'");
                }
            }
            VL_DO_DANGLING(افزودن_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        اگر (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* ثابت cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } وگرنه {
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
            } وگرنه {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        اگر (!nodep->typedefp() && !nodep->subDTypep()) {
            ثابت VSymEnt* foundp;
            اگر (nodep->classOrPackagep()) {
                foundp = m_statep->get_گره_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } وگرنه اگر (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } وگرنه {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<صحیح>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                اگر (AstTypedef* ثابت defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    اگر (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } وگرنه اگر (AstParamTypeDType* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ثابت منطقی isParamTypeChild = (defp == nodep->backp());
                    ثابت منطقی isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    اگر (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } وگرنه {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } وگرنه اگر (AstClass* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    اگر (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* ثابت paramsp = nodep->paramsp();
                    اگر (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* ثابت newp
                        = جدید AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                } وگرنه اگر (m_insideClassExtParam) {
                    بازگشت;
                } وگرنه {
                    اگر (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } وگرنه {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        اگر (nodep->lhsp() && !VN_IS(nodep->lhsp(), گره_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            اگر (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } وگرنه {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(جدید AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_گره_FTask* ثابت taskp = foundp ? VN_AS(foundp->nodep(), گره_FTask) : nullptr;
        اگر (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } وگرنه اگر (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } وگرنه {
            taskp->dpiExport(درست);
            اگر (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        منطقی hasPartSelect = نادرست;
        ثابت string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        اگر (nodep->targetRefp()) {
            اگر (AstTaskRef* ثابت taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } وگرنه اگر (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
            اگر (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_درخت();
                اگر (!hasPartSelect) {
                    ثابت string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    اگر (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            جدید AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    تهی visitPackageImportOrExport(Ast_گره* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        اگر (m_statep->forParamed()) VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    تهی visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    تهی visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    تهی visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    تهی visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    تهی visit(Ast_Stmt_عبارت* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        اگر (AstNew* ثابت newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } وگرنه اگر (ثابت AstDot* ثابت dotp = VN_CAST(nodep->exprp(), Dot)) {
            اگر (dotp->lhsp()->name() == "super") {
                اگر (AstNew* ثابت newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    تهی visit(AstIfaceRefDType* nodep) override {
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        اگر (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ثابت ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            اگر (ifacep->dead()) بازگشت;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_گره_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    تهی visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    تهی visit(AstCastParse* nodep) override {
        اگر (nodep->user3SetOnce()) بازگشت;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ثابت منطقی dotLhs = m_ds.m_dotPos == DP_FIRST;
        اگر (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        اگر (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    تهی visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        اگر (nodep->user2()) m_replaceWithAlias = نادرست;
        iterateChildren(nodep);
    }

    تهی visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = درست;
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره* nodep) override {
        VL_RESTORER(m_inPackedArray);
        اگر (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = درست;
        } وگرنه اگر (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

عمومی:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ثابت std::map<std::string, Ast_گره_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        برای (auto& p : modulesToRevisit) {
            Ast_گره_Module* ثابت modp = p.second;
            modp->foreach([](Ast_گره* ثابت nodep) { nodep->user3(نادرست); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = پیشفرض;
};

//######################################################################
// Link class functions

تهی V3LinkDot::dumpSubstep(ثابت string& name) {
    اگر (dump_درخت_Either_Level() >= 9) {
        V3Global::dump_Check_Global_درخت(name);
    } وگرنه اگر (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_درخت_File(v3Global.debugFilename(name + ".tree"));
    }
}

تهی V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_درخت();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    اگر (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(درست);
        dumpSubstep("prelinkdot-scoped");
    } وگرنه {
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

تهی V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_درخت("linkdot", 0, dump_درخت_Either_Level() >= 6);
}

تهی V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_درخت("linkdotparam", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_درخت("linkdot", 0, dump_درخت_Either_Level() >= 3);
}
